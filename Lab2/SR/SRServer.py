from Datagram import DataGram
from socket import *
import threading
import random
import numpy as np


class SRServer:
    """
    发送方，维护发送窗口
    """

    def __init__(self):
        self.dropRate = 0.1
        self.HOST = ''
        self.PORT = 12340
        self.BUFSIZ = 10
        self.ADDR = (self.HOST, self.PORT)
        self.cntFlag = False
        self.ServerSocket = socket(AF_INET, SOCK_DGRAM)  # 创建UDP连接
        self.ServerSocket.bind(self.ADDR)  # 绑定服务器地址
        print('等待连接...')
        self.cntData, self.ClientAddr = self.ServerSocket.recvfrom(self.BUFSIZ)  # 接受客户的连接

        self.windowSize = 4
        self.base = 0
        self.next_seq = 0
        self.frames = {}
        self.acks = [False for i in range(256)]
        self.timers = {}

    def randomDrop(self):
        """
        随机丢包
        """
        randomNum = random.random()
        if randomNum < self.dropRate:
            return True
        else:
            return False

    def makeDatagramBuffer(self):
        datagramBufferMap = {}
        with open("source.txt", "r") as f:
            data = f.read()
            # 将文件内容按照BUFSIZ分割，分别编号发给客户端
            for i in range(0, len(data), self.BUFSIZ):
                if i + self.BUFSIZ < len(data):
                    datagram = DataGram("SEQ", i // self.BUFSIZ, data[i:i + self.BUFSIZ])
                    datagramBufferMap[i // self.BUFSIZ] = datagram
                else:
                    datagram = DataGram("FIN", i // self.BUFSIZ, data[i:])
                    datagramBufferMap[i // self.BUFSIZ] = datagram
        return datagramBufferMap

    def sendFrame(self, _frame):
        self.ServerSocket.sendto(_frame.makePacket, self.ClientAddr)

    def SRSendALL(self, _seq_num):
        """
        超时处理
        重发在base和next_seq之间某个数据包
        """
        if not self.acks[_seq_num]:
            print("超时，重发序号为", _seq_num, "的数据包")
            self.sendFrame(self.frames[_seq_num])
            self.timers[_seq_num] = threading.Timer(2, self.SRSendALL, [_seq_num])
            self.timers[_seq_num].start()

    def SRWindow(self):
        # 读取文件
        with open("source.txt", "r") as f:
            data = f.read()
            # 将文件内容按照BUFSIZ分割，分别编号发给客户端
            for i in range(0, len(data), self.BUFSIZ):
                if i + self.BUFSIZ < len(data):
                    datagram = DataGram("SEQ", i // self.BUFSIZ, data[i:i + self.BUFSIZ])
                    self.frames[i // self.BUFSIZ] = datagram
                else:
                    datagram = DataGram("FIN", i // self.BUFSIZ, data[i:])
                    self.frames[i // self.BUFSIZ] = datagram

        # 发送窗口
        while self.base < len(self.frames):
            # 发送窗口内的数据包
            while self.next_seq < self.base + self.windowSize and self.next_seq < len(self.frames):
                print("发送序号为", self.next_seq, "的数据包")
                self.sendFrame(self.frames[self.next_seq])
                self.timers[self.next_seq] = threading.Timer(2, self.SRSendALL, [self.next_seq])
                self.timers[self.next_seq].start()
                self.next_seq += 1 # next_seq指向下一个要发送的数据包,每发一个数据包next_seq+1
            # 循环接收ACK
            while True:
                rcvDatagram, _ = self.ServerSocket.recvfrom(self.BUFSIZ)
                if self.randomDrop():
                    print("丢弃ACK", int(rcvDatagram.decode("UTF-8")[4:]))
                    continue

                if rcvDatagram.decode("UTF-8")[0:3] == "ACK":
                    self.acks[int(rcvDatagram.decode("UTF-8")[4:])] = True
                    print("确认收到ACK", int(rcvDatagram.decode("UTF-8")[4:]))
                    self.timers[int(rcvDatagram.decode("UTF-8")[4:])].cancel()
                    # 将base更新为在base和next_seq之间第一个未收到ACK的数据包
                    if self.acks[self.base]:
                        self.base += 1
                    for i in range(self.base, self.next_seq):
                        if not self.acks[i]:
                            self.base = i
                            # print("更新base为", self.base)
                            # 重发base指向的数据包，这里是不符合协议操作的，只是为了产生大量的包，方便演示
                            self.sendFrame(self.frames[self.base])
                            break

                    if self.base == self.next_seq:
                        print("传输完成")

                        return
                    break

    def run(self):
        if self.cntData.decode("UTF-8") == "CNT":
            print("连接成功\n")
            print('连接地址:', self.ClientAddr)
            self.cntFlag = True

        if self.cntFlag:
            self.SRWindow()
            print("文件传输完成")
            self.ServerSocket.close()


if __name__ == '__main__':
    server = SRServer()
    server.run()
    exit(0)
