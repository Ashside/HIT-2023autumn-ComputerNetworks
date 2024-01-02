#include <stdio.h>
#include <Windows.h>
#include <process.h>
#include <string.h>
#include "main.h"
#pragma comment(lib,"Ws2_32.lib")

int main(int argc, char* argv[])
{
	printf("代理服务器正在启动\n");
	printf("初始化...\n");
	if (!InitSocket()) {
		printf("socket 初始化失败\n");
		return -1;
	}
	printf("代理服务器正在运行，监听端口 %d\n", ProxyPort);

	SOCKET acceptSocket = INVALID_SOCKET;
	ProxyParam* lpProxyParam;
	HANDLE hThread;
	DWORD dwThreadID;
	sockaddr_in addr_in;
	int addr_len = sizeof(SOCKADDR);

	//代理服务器不断监听
	while (true) {
		acceptSocket = accept(ProxyServer, (SOCKADDR*)&addr_in, &(addr_len));//接收到客户端的连接
		lpProxyParam = new ProxyParam;//创建一个新的线程参数
		if (lpProxyParam == NULL) {
			continue;
		}
		auto visit_ip = inet_ntoa(addr_in.sin_addr);
		//输出客户端IP地址
		printf("访问来自IP");
		printf(visit_ip);//将网络二进制的数字转换成网络地址
		printf("\n");

		// 检查是否在黑名单中
		if (strcmp(blocked_ip, visit_ip) == 0)//转换成网络地址
		{
			printf("%s用户禁止访问\n", visit_ip);
			continue;
		}

		lpProxyParam->clientSocket = acceptSocket;//将客户端套接字赋值给线程参数
		hThread = (HANDLE)_beginthreadex(NULL, 0, &ProxyThread, (LPVOID)lpProxyParam, 0, 0);//创建线程
		CloseHandle(hThread);//关闭线程句柄
		Sleep(200);//延时200ms
	}

	//无法访问下三行代码
	closesocket(ProxyServer);
	WSACleanup();
	return 0;
}
//************************************
// Method: InitSocket
// FullName: InitSocket
// Access: public
// Returns: BOOL
// Qualifier: 初始化套接字
//************************************
BOOL InitSocket() {
	//加载套接字库（必须）
	WORD wVersionRequested;
	WSADATA wsaData;
	//套接字加载时错误提示
	int err;
	//版本 2.2
	wVersionRequested = MAKEWORD(2, 2);
	//加载 dll 文件 Scoket 库
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		//找不到 winsock.dll
		printf("加载 winsock 失败，错误代码为: %d\n", WSAGetLastError());
		return FALSE;
	}
	//if中的语句主要用于比对是否是2.2版本
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		printf("不能找到正确的 winsock 版本\n");
		WSACleanup();
		return FALSE;
	}
	// 创建的socket文件描述符基于IPV4，TCP
	ProxyServer = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == ProxyServer) {
		printf("创建套接字失败，错误代码为： %d\n", WSAGetLastError());
		return FALSE;
	}
	// 设置套接字选项避免地址使用错误
	ProxyServerAddr.sin_family = AF_INET;//设置地址族
	ProxyServerAddr.sin_port = htons(ProxyPort);//将主机的无符号短整形数转换成网络字节顺序
	ProxyServerAddr.sin_addr.S_un.S_addr = INADDR_ANY;//表示本机的所有IP
	// 绑定套接字
	if (bind(ProxyServer, (SOCKADDR*)&ProxyServerAddr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		printf("绑定套接字失败\n");
		return FALSE;
	}
	if (listen(ProxyServer, SOMAXCONN) == SOCKET_ERROR) {
		printf("监听端口%d 失败", ProxyPort);
		return FALSE;
	}
	return TRUE;
}
//************************************
// Method: ProxyThread
// FullName: ProxyThread
// Access: public
// Returns: unsigned int __stdcall
// Qualifier: 线程执行函数
// Parameter: LPVOID lpParameter
//************************************
unsigned int __stdcall ProxyThread(LPVOID lpParameter) {
	char Buffer[MAXSIZE];//缓冲区
	ZeroMemory(Buffer, MAXSIZE);

	SOCKADDR_IN clientAddr;//客户端地址
	int length = sizeof(SOCKADDR_IN);//地址长度
	int recvSize;//接收到的数据大小
	int ret;//返回值
	recvSize = recv(((ProxyParam*)lpParameter)->clientSocket, Buffer, MAXSIZE, 0);//接收到报文，存入Buffer中

	char* CacheBuffer;//缓存区
	CacheBuffer = new char[recvSize + 1];
	ZeroMemory(CacheBuffer, recvSize + 1);
	memcpy(CacheBuffer, Buffer, recvSize);//将Buffer中的内容复制到CacheBuffer中

	HttpHeader* httpHeader = new HttpHeader();
	//处理HTTP头部
	if (CacheBuffer)
	{
		ParseHttpHead(CacheBuffer, httpHeader);//将CacheBuffer中的内容解析到httpHeader中
	}

	//处理禁止访问网站
	//事实上的子串匹配
	if (strstr(httpHeader->url, blocked_web) != NULL)//如果在黑名单中
	{
		printf("\n==========================================\n");
		printf("--------该网站已被屏蔽!----------\n");
		printf("关闭套接字\n");
		Sleep(200);
		closesocket(((ProxyParam*)lpParameter)->clientSocket);
		closesocket(((ProxyParam*)lpParameter)->serverSocket);
		delete lpParameter;
		_endthreadex(0);
		return 0;
	}
	//处理钓鱼网站
	if (strstr(httpHeader->url, fishing_src) != NULL)
	{
		printf("\n==========================================\n");
		printf("---已从源钓鱼网址：%s 跳转到 目的网址 ：%s ---\n", fishing_src, fishing_dest);
		//修改HTTP报文
		memcpy(httpHeader->host, fishing_dest_host, strlen(fishing_dest_host) + 1);//修改host，+1是复制进去\0
		memcpy(httpHeader->url, fishing_dest, strlen(fishing_dest));//修改url
	}
	delete CacheBuffer;

	//连接目标主机
	//如果连接失败，关闭套接字
	if (!ConnectToServer(&((ProxyParam*)lpParameter)->serverSocket, httpHeader->host)) {
		printf("关闭套接字\n");
		Sleep(200);
		closesocket(((ProxyParam*)lpParameter)->clientSocket);
		closesocket(((ProxyParam*)lpParameter)->serverSocket);
		delete lpParameter;
		_endthreadex(0);
		return 0;
	}
	printf("代理连接主机 %s 成功\n", httpHeader->host);

	//以下部分为缓存部分
	int index = isInCache(cache, *httpHeader);//判断是否在缓存中
	//如果在缓存中存在
	if (index > -1)
	{
		char* cacheBuffer;//缓存区
		char Buf[MAXSIZE];//缓存区
		ZeroMemory(Buf, MAXSIZE);
		memcpy(Buf, Buffer, recvSize);//将Buffer中的内容复制到Buf中，Buffer中的内容为客户端发送的报文

		//插入"If-Modified-Since: "
		ModeHTTP(Buf, cache[index].date);//更改报文date，插入If-Modified-Since:

		printf("-------------------请求报文------------------------\n%s\n", Buf);
		ret = send(((ProxyParam*)lpParameter)->serverSocket, Buf, strlen(Buf) + 1, 0);//将Buf中的内容发送给服务器
		recvSize = recv(((ProxyParam*)lpParameter)->serverSocket, Buf, MAXSIZE, 0);//接收服务器返回的报文
		printf("------------------Server返回报文-------------------\n%s\n", Buf);

		//如果服务器返回的报文为空，关闭套接字
		if (recvSize <= 0) {
			printf("关闭套接字\n");
			Sleep(200);
			closesocket(((ProxyParam*)lpParameter)->clientSocket);
			closesocket(((ProxyParam*)lpParameter)->serverSocket);
			delete lpParameter;
			_endthreadex(0);
			return 0;
		}
		//如果服务器返回的报文不为空，将报文发送给客户端
		const char* No_Modified = "304";

		//内容没有改变，直接返回cache中的内容
		if (!memcmp(&Buf[9], No_Modified, strlen(No_Modified)))//将304写入Buf中
		{
			//将cache中的内容发送给客户端
			ret = send(((ProxyParam*)lpParameter)->clientSocket, cache[index].buffer, strlen(cache[index].buffer) + 1, 0);
			printf("将cache中的缓存返回客户端\n");
			printf("=================================================\n");
			printf("关闭套接字\n");
			Sleep(200);
			closesocket(((ProxyParam*)lpParameter)->clientSocket);
			closesocket(((ProxyParam*)lpParameter)->serverSocket);
			delete lpParameter;
			_endthreadex(0);
			return 0;
		}
	}
	//将客户端发送的 HTTP 数据报文直接转发给目标服务器
	ret = send(((ProxyParam*)lpParameter)->serverSocket, Buffer, strlen(Buffer) + 1, 0);
	//等待目标服务器返回数据
	recvSize = recv(((ProxyParam*)lpParameter)->serverSocket, Buffer, MAXSIZE, 0);

	//将返回报文加入缓存
	//从服务器返回报文中解析时间
	char* cacheBuffer2 = new char[MAXSIZE];
	ZeroMemory(cacheBuffer2, MAXSIZE);
	memcpy(cacheBuffer2, Buffer, MAXSIZE);//将Buffer中的内容复制到cacheBuffer2中，Buffer中的内容为服务器返回的报文
	const char* delim = "\r\n";//分隔符
	char date[DATELENGTH];
	char* nextStr;

	ZeroMemory(date, DATELENGTH);
	char* p = strtok_s(cacheBuffer2, delim, &nextStr);//分行，使用strtok_s函数，将cacheBuffer2中的内容分行，每行的内容存入p中
	bool flag = false;//表示是否含有修改时间报文
	//不断分行，直到分出具有修改时间的那一行
	while (p)
	{
		if (p[0] == 'L')//找到 Last-Modified: 那一行
		{
			if (strlen(p) > 15)//15是Last-Modified: 的长度
			{
				char header[15];//用于存储 Last-Modified:
				ZeroMemory(header, sizeof(header));
				memcpy(header, p, 14);

				if (!(strcmp(header, "Last-Modified:")))//如果是Last-Modified: 那一行
				{
					//将时间存入date中
					memcpy(date, &p[15], strlen(p) - 15);
					flag = true;//表示已经找到了修改时间报文
					break;
				}
			}
		}
		p = strtok_s(NULL, delim, &nextStr);//继续分行
	}
	if (flag)//如果找到了修改时间报文
	{
		if (index > -1)//说明已经有内容存在，改chache中的时间和内容
		{
			memcpy(&(cache[index].buffer), Buffer, strlen(Buffer));
			memcpy(&(cache[index].date), date, strlen(date));
		}
		else//第一次访问，需要完全缓存
		{
			memcpy(&(cache[cache_index % CACHE_NUM].httpHead.host), httpHeader->host, strlen(httpHeader->host));
			memcpy(&(cache[cache_index % CACHE_NUM].httpHead.method), httpHeader->method, strlen(httpHeader->method));
			memcpy(&(cache[cache_index % CACHE_NUM].httpHead.url), httpHeader->url, strlen(httpHeader->url));
			memcpy(&(cache[cache_index % CACHE_NUM].buffer), Buffer, strlen(Buffer));
			memcpy(&(cache[cache_index % CACHE_NUM].date), date, strlen(date));
			cache_index++;
		}
	}
	//将目标服务器返回的数据直接转发给客户端
	ret = send(((ProxyParam*)lpParameter)->clientSocket, Buffer, sizeof(Buffer), 0);
}
//************************************
// Method: ParseHttpHead
// FullName: ParseHttpHead
// Access: public
// Returns: void
// Qualifier: 解析 TCP 报文中的 HTTP 头部
// Parameter: char * buffer
// Parameter: HttpHeader * httpHeader
//************************************
void ParseHttpHead(char* buffer, HttpHeader* httpHeader) {
	char* p;
	char* ptr;
	const char* delim = "\r\n";
	p = strtok_s(buffer, delim, &ptr);//提取第一行
	printf("%s\n", p);
	if (p == nullptr) {
		return;
	}
	if (p[0] == 'G') {//GET 方式
		memcpy(httpHeader->method, "GET", 3);
		memcpy(httpHeader->url, &p[4], strlen(p) - 13);//将p[4]到p[strlen(p)-13]的内容复制到httpHeader->url中，13是GET HTTP/1.1的长度
	}
	else if (p[0] == 'P') {//POST 方式
		memcpy(httpHeader->method, "POST", 4);
		memcpy(httpHeader->url, &p[5], strlen(p) - 14);//将p[5]到p[strlen(p)-14]的内容复制到httpHeader->url中，14是POST HTTP/1.1的长度
	}
	printf("%s\n", httpHeader->url);//输出url
	p = strtok_s(NULL, delim, &ptr);//提取第二行
	//不断提取下一行，直到提取到空行
	//提取HTTP头部中的Host和Cookie
	while (p) {
		switch (p[0]) {
		case 'H'://Host
			memcpy(httpHeader->host, &p[6], strlen(p) - 6);//将p[6]到p[strlen(p)-6]的内容复制到httpHeader->host中，6是Host: 的长度
			break;
		case 'C'://Cookie
			if (strlen(p) > 8) {
				char header[8];//用于存储 Cookie:
				ZeroMemory(header, sizeof(header));
				memcpy(header, p, 6);//将p中的内容复制到header中
				//如果是Cookie: 那一行
				if (!strcmp(header, "Cookie")) {
					memcpy(httpHeader->cookie, &p[8], strlen(p) - 8);//将p[8]到p[strlen(p)-8]的内容复制到httpHeader->cookie中，8是Cookie: 的长度
				}
			}
			break;
		default:
			break;
		}
		p = strtok_s(NULL, delim, &ptr);//继续提取下一行
	}
}
//************************************
// Method: ConnectToServer
// FullName: ConnectToServer
// Access: public
// Returns: BOOL
// Qualifier: 根据主机创建目标服务器套接字，并连接
// Parameter: SOCKET * serverSocket
// Parameter: char * host
//************************************
BOOL ConnectToServer(SOCKET* serverSocket, char* host) {
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;//设置地址族
	serverAddr.sin_port = htons(HTTP_PORT);//将主机的无符号短整形数转换成网络字节顺序
	HOSTENT* hostent = gethostbyname(host);//根据主机名获取主机信息
	if (!hostent) {
		return FALSE;
	}
	in_addr Inaddr = *((in_addr*)*hostent->h_addr_list);//将主机信息中的地址复制到Inaddr中
	serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(Inaddr));//将一个将网络地址转换成一个长整数型数

	*serverSocket = socket(AF_INET, SOCK_STREAM, 0);//创建套接字
	if (*serverSocket == INVALID_SOCKET) {
		return FALSE;
	}
	//连接目标服务器
	if (connect(*serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		closesocket(*serverSocket);
		return FALSE;
	}
	return TRUE;
}
BOOL isHttpHeadEqual(cacheHttpHead http1, HttpHeader http2)
{
	//判断两个http头是否相等
	if (strcmp(http1.method, http2.method)) {
		return false;
	}
	if (strcmp(http1.host, http2.host)) {
		return false;
	}
	if (strcmp(http1.url, http2.url)) {
		return false;
	}
	return true;
}
int isInCache(CACHE* cache, HttpHeader httpHeader)
{
	int index = 0;
	for (; index < CACHE_NUM; index++)
	{
		//比较http头是否相等
		if (isHttpHeadEqual(cache[index].httpHead, httpHeader)) {
			return index;
		}
	}
	return -1;
}
void ModeHTTP(char* buffer, char* date)
{
	//此函数在HTTP中间插入"If-Modified-Since: "
	const char* strHost = "Host";
	const char* inputStr = "If-Modified-Since: ";
	char temp[MAXSIZE];
	ZeroMemory(temp, MAXSIZE);
	char* pos = strstr(buffer, strHost);//找到Host位置

	int i = 0;
	//将host与之后的部分写入temp
	for (i = 0; i < strlen(pos); i++) {
		temp[i] = pos[i];
	}

	*pos = '\0';

	//插入If-Modified-Since字段
	while (*inputStr != '\0') {  
		*pos++ = *inputStr++;
	}
	//将date写入buffer中
	while (*date != '\0') {
		*pos++ = *date++;
	}
	*pos++ = '\r';
	*pos++ = '\n';
	//将host之后的字段复制到buffer中
	for (i = 0; i < strlen(temp); i++) {
		*pos++ = temp[i];
	}
}