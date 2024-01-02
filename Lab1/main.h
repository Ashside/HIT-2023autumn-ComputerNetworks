#pragma once
#define MAXSIZE 65507 //发送数据报文的最大长度
#define HTTP_PORT 80 //http 服务器端口
#define DATELENGTH 50 //时间字节数
#define CACHE_NUM 50  //定义最大缓存数量

//代理相关参数
SOCKET ProxyServer;
sockaddr_in ProxyServerAddr;
const int ProxyPort = 10240;

//禁止访问网站
const char* blocked_web = { "http://computing.hit.edu.cn/" };
//限制访问用户
const char* blocked_ip = { "127.0.0.2" };
//钓鱼网站
const char* fishing_src = "http://today.hit.edu.cn";//钓鱼网站原网址
const char* fishing_dest = "http://jwts.hit.edu.cn";//钓鱼网站目标网址
const char* fishing_dest_host = "jwts.hit.edu.cn";//钓鱼目的地址主机名

//Http头部数据
struct HttpHeader {
	char method[4]; // POST 或者 GET，注意有些为 CONNECT，本实验暂不考虑
	char url[1024]; // 请求的 url
	char host[1024]; // 目标主机
	char cookie[1024 * 10]; //cookie
	HttpHeader() {
		ZeroMemory(this, sizeof(HttpHeader));
	}
};
//cache存储的时候去掉Http头部信息中的cookie
//只是为了比较是否相同，不影响缓存的使用
struct cacheHttpHead {
	char method[4]; // POST 或者 GET，注意有些为 CONNECT，本实验暂不考虑
	char url[1024]; // 请求的 url
	char host[1024]; // 目标主机
	cacheHttpHead() {
		ZeroMemory(this, sizeof(cacheHttpHead));
	}
};
//cache数据结构
struct CACHE {
	cacheHttpHead httpHead;
	char buffer[MAXSIZE];//储存报文返回内容
	char date[DATELENGTH];//缓存内容的最后修改时间
	CACHE() {
		ZeroMemory(this->buffer, MAXSIZE);
		ZeroMemory(this->date, DATELENGTH);
	}
};

//代理参数
struct ProxyParam {
	SOCKET clientSocket;
	SOCKET serverSocket;
};

CACHE cache[CACHE_NUM];//缓存地址
int cache_index = 0;//记录当前应该将缓存放在哪个位置

BOOL InitSocket();
void ParseHttpHead(char* buffer, HttpHeader* httpHeader);
BOOL ConnectToServer(SOCKET* serverSocket, char* host);
unsigned int __stdcall ProxyThread(LPVOID lpParameter);

//判断两个http报文是否相同,判断缓存和报文是否相同
BOOL isHttpHeadEqual(cacheHttpHead http1, HttpHeader http2);
//寻找缓存中是否存在，如果存在返回index，不存在返回-1。通过比较http头部信息判断是否相同
int isInCache(CACHE* cache, HttpHeader httpHeader);
//用于修改HTTP报文，在buffer中添加"If-Modified-Since: "后面加上date
void ModeHTTP(char* buffer, char* date);
