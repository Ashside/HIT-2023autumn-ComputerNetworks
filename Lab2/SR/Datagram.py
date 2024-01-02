
class DataGram:
    """
    Datagram
    数据报文
    [type] [seq_num] [data]
    [type] = "ACK" or "SEQ" or "CNT" or "FIN"
    """
    def __init__(self, _DatagramType="ACK", _seq_num=0, _data=""):
        self.type = _DatagramType
        self.seq_num = _seq_num
        self.data = _data

    @property
    def makePacket(self):
        _pkt = self.type + " " + str(self.seq_num) + " \n" + self.data
        return _pkt.encode("UTF-8")