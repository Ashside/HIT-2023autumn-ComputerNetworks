from Datagram import DataGram
from socket import *
import os
import random
import numpy as np


class SRClient:

    def __init__(self, _outFilePath="result.txt"):
        self.dropRate = 0.1
        self.recvWindow = 4
        self.recvBase = 0
        self.rcv = [False for i in range(256)]
        self.outFilePath = _outFilePath

    def randomDrop(self):
        """
        随机丢包
        """
        randomNum = random.random()
        if randomNum < self.dropRate:
            return True
        else:
            return False

    def run(self, PORT=12340):
        HOST = '127.0.0.1'  # 服务器连接地址
        # PORT = 12340  # 服务器启用端口
        BUFSIZ = 1024  # 缓冲区大小
        ADDR = (HOST, PORT)
        fileBuffer = np.array([None for i in range(256)])
        ClientSocket = socket(AF_INET, SOCK_DGRAM)
        client = SRClient()
        ClientSocket.sendto(bytes("CNT", encoding="UTF-8"), ADDR)
        if os.path.exists(self.outFilePath):
            os.remove(self.outFilePath)
        # 接受服务器的数据
        while True:

            rcvDatagram, ServerAddr = ClientSocket.recvfrom(BUFSIZ)

            if client.randomDrop():
                print("丢弃序号为", rcvDatagram.decode("UTF-8")[
                                    rcvDatagram.decode("UTF-8").find(" "):rcvDatagram.decode("UTF-8").find("\n")],
                      "的数据包")
                continue
            if rcvDatagram.decode("UTF-8")[0:3] == "SEQ":
                seq = int(rcvDatagram.decode("UTF-8")[
                          rcvDatagram.decode("UTF-8").find(" "):rcvDatagram.decode("UTF-8").find("\n")])
                # 检查seq是否在窗口内
                if seq < client.recvBase or seq >= client.recvBase + client.recvWindow:
                    # 返回ACK
                    ack = DataGram("ACK", seq, "")
                    ClientSocket.sendto(ack.makePacket, ServerAddr)
                    continue

                print("接收序号为", seq, "的数据包")
                self.rcv[seq] = True
                fileBuffer[seq] = rcvDatagram.decode("UTF-8")[rcvDatagram.decode("UTF-8").find("\n") + 1:]
                ack = DataGram("ACK", seq, "")
                print("发送ACK", seq)
                ClientSocket.sendto(ack.makePacket, ServerAddr)
                while self.rcv[client.recvBase]:
                    client.recvBase += 1
                continue

            if rcvDatagram.decode("UTF-8")[0:3] == "FIN":
                # 发送ACK
                seq = int(rcvDatagram.decode("UTF-8")[
                          rcvDatagram.decode("UTF-8").find(" "):rcvDatagram.decode("UTF-8").find("\n")])
                ack = DataGram("ACK", seq, "")
                print("接收序号为", seq, "的数据包")
                ClientSocket.sendto(ack.makePacket, ServerAddr)
                self.rcv[seq] = True
                print("发送ACK", seq)
                fileBuffer[seq] = rcvDatagram.decode("UTF-8")[rcvDatagram.decode("UTF-8").find("\n") + 1:]
                # 如果在seq之前的rcv都为True，说明文件接收完成
                if np.all(self.rcv[:seq + 1]):
                    writeBufferToFile(fileBuffer)
                    ClientSocket.close()
                    break

                continue


def writeBufferToFile(_fileBuffer, _outFilePath="result.txt"):
    """
    将fileBuffer中的数据写入文件
    """
    writeBuffer = _fileBuffer.tolist()
    with open(_outFilePath, "a") as f:
        for i in range(len(writeBuffer)):
            if writeBuffer[i] is not None:
                f.write(writeBuffer[i])
    print("文件接收完成")
    return


if __name__ == '__main__':
    if os.path.exists("result.txt"):
        os.remove("result.txt")
    client = SRClient("result.txt")
    client.run()
    exit(0)
