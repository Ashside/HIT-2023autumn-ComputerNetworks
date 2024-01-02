import threading
from socket import *
import os
from Datagram import DataGram
import random


class GBNClient:
    """
    GBNClient
    接受来自服务器的数据，返回ACK
    """

    def __init__(self, _outFilePath="result.txt", _PORT=12340):
        self.SENDADDR = None
        self.dropRate = 0.05
        self.outFilePath = _outFilePath
        self.__PORT = _PORT
        self.BUFSIZ = 10
        self.windowSize = 4
        self.base = 0
        self.next_seq = 0
        self.frames = []
        self.acks = [False] * 256
        self.timers = None  # 只有一个定时器，用于重发所有未收到ACK的数据包

    def randomDrop(self):
        """
        随机丢包
        """
        randomNum = random.random()
        if randomNum < self.dropRate:
            return True
        else:
            return False

    def run(self):
        HOST = '127.0.0.1'  # 服务器连接地址
        PORT = self.__PORT  # 服务器连接端口
        BUFSIZ = 1024  # 缓冲区大小
        ADDR = (HOST, PORT)
        fileBuffer = []
        ClientSocket = socket(AF_INET, SOCK_DGRAM)
        client = GBNClient()
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
                # print(rcvDatagram.decode("UTF-8"))
                # 提取序号，序号是在第一个空格之后，第一个换行符之前
                seq = int(rcvDatagram.decode("UTF-8")[
                          rcvDatagram.decode("UTF-8").find(" "):rcvDatagram.decode("UTF-8").find("\n")])
                # 提取数据，数据是在第一个换行符之后
                data = rcvDatagram.decode("UTF-8")[rcvDatagram.decode("UTF-8").find("\n") + 1:]

                # 检查序号是否正确
                if seq != len(fileBuffer) and seq != 0:
                    print("序号错误，期望序号为", len(fileBuffer), "实际序号为", seq)
                    continue
                # 将数据写入fileBuffer
                print("接收序号为", seq, "的数据包")
                fileBuffer.append(data)
                # 发送ACK
                ack = DataGram("ACK", seq, "")
                ClientSocket.sendto(ack.makePacket, ServerAddr)
                # print(ack.makePacket.decode("UTF-8"))

            if rcvDatagram.decode("UTF-8")[0:3] == "FIN":
                # print(rcvDatagram.decode("UTF-8"))
                # 发送ACK
                seq = int(rcvDatagram.decode("UTF-8")[
                          rcvDatagram.decode("UTF-8").find(" "):rcvDatagram.decode("UTF-8").find("\n")])
                ack = DataGram("ACK", seq, "")
                print("接收序号为", seq, "的数据包")
                ClientSocket.sendto(ack.makePacket, ServerAddr)
                fileBuffer.append(rcvDatagram.decode("UTF-8")[rcvDatagram.decode("UTF-8").find("\n") + 1:])
                writeBufferToFile(fileBuffer, self.outFilePath)
                break

    def sendFrame(self, _frame, _ClientSocket):
        PORT = self.__PORT
        _ClientSocket.sendto(_frame.makePacket, self.SENDADDR)

    def GBNSendALL(self, _ClientSocket):
        '''
        重发在base和next_seq之间所有的数据包
        :return:
        '''
        for i in range(self.base, self.next_seq):
            if i >= len(self.frames):
                break
            self.sendFrame(self.frames[i], _ClientSocket)
        print("重传窗口为", self.base, "到", self.next_seq - 1)
        self.timers = threading.Timer(5, self.GBNSendALL)

    def sendResult(self, _PORT=12341):
        """
        将source2.txt发送给服务器
        :return:
        """
        HOST = ''  # 服务器连接地址
        PORT = _PORT  # 服务器连接端口
        ADDR = (HOST, PORT)
        BUFSIZ = 1024  # 缓冲区大小
        cntFlag = False
        ClientSocket = socket(AF_INET, SOCK_DGRAM)  # 创建UDP连接
        ClientSocket.bind(ADDR)  # 绑定服务器地址
        print('等待连接...')
        cntData, self.SENDADDR = ClientSocket.recvfrom(BUFSIZ)  # 接受客户的连接
        if cntData.decode("UTF-8") == "CNT":
            print("连接成功\n")
            print('连接地址:', self.SENDADDR)
            cntFlag = True
        if cntFlag:
            with open("source2.txt", "r") as f:
                data = f.read()
                # 将文件内容按照BUFSIZ分割，分别编号发给客户端
                for i in range(0, len(data), self.BUFSIZ):
                    if i + self.BUFSIZ < len(data):
                        datagram = DataGram("SEQ", i // self.BUFSIZ, data[i:i + self.BUFSIZ])
                        self.frames.append(datagram)
                    else:
                        datagram = DataGram("FIN", i // self.BUFSIZ, data[i:])
                        self.frames.append(datagram)

            while self.base < len(self.frames):
                self.timers = threading.Timer(5, self.GBNSendALL, args=(ClientSocket,))
                self.timers.start()
                while self.next_seq < self.base + self.windowSize and self.next_seq < len(self.frames):
                    self.sendFrame(self.frames[self.next_seq], ClientSocket)
                    self.next_seq += 1
                while True:
                    rcvDatagram, _ = ClientSocket.recvfrom(self.BUFSIZ)
                    if self.randomDrop():
                        print("丢弃ACK", rcvDatagram.decode("UTF-8")[4:])
                        continue
                    if rcvDatagram.decode("UTF-8")[0:3] == "ACK":
                        self.acks[int(rcvDatagram.decode("UTF-8")[4:])] = True
                        print("确认收到ACK" + rcvDatagram.decode("UTF-8")[4:])
                        self.base = int(rcvDatagram.decode("UTF-8")[4:]) + 1
                        if self.base == self.next_seq:
                            self.timers.cancel()
                            print("传输完成")
                            return
                        else:
                            self.timers.cancel()
                            self.timers = threading.Timer(5, self.GBNSendALL, args=(ClientSocket,))
                            self.timers.start()
                            break
            print("文件传输完成")
            return


def writeBufferToFile(_fileBuffer, _outFilePath="result.txt"):
    """
    将fileBuffer中的数据写入文件
    """
    with open(_outFilePath, "a") as f:
        for i in _fileBuffer:
            f.write(i)
    _fileBuffer.clear()


if __name__ == '__main__':
    if os.path.exists("result.txt"):
        os.remove("result.txt")
    client = GBNClient()
    # client.run()
    client.sendResult()
