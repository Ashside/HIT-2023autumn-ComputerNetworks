#include <stdio.h>
#include <Windows.h>
#include <process.h>
#include <string.h>
#include "main.h"
#pragma comment(lib,"Ws2_32.lib")

int main(int argc, char* argv[])
{
	printf("�����������������\n");
	printf("��ʼ��...\n");
	if (!InitSocket()) {
		printf("socket ��ʼ��ʧ��\n");
		return -1;
	}
	printf("����������������У������˿� %d\n", ProxyPort);

	SOCKET acceptSocket = INVALID_SOCKET;
	ProxyParam* lpProxyParam;
	HANDLE hThread;
	DWORD dwThreadID;
	sockaddr_in addr_in;
	int addr_len = sizeof(SOCKADDR);

	//������������ϼ���
	while (true) {
		acceptSocket = accept(ProxyServer, (SOCKADDR*)&addr_in, &(addr_len));//���յ��ͻ��˵�����
		lpProxyParam = new ProxyParam;//����һ���µ��̲߳���
		if (lpProxyParam == NULL) {
			continue;
		}
		auto visit_ip = inet_ntoa(addr_in.sin_addr);
		//����ͻ���IP��ַ
		printf("��������IP");
		printf(visit_ip);//����������Ƶ�����ת���������ַ
		printf("\n");

		// ����Ƿ��ں�������
		if (strcmp(blocked_ip, visit_ip) == 0)//ת���������ַ
		{
			printf("%s�û���ֹ����\n", visit_ip);
			continue;
		}

		lpProxyParam->clientSocket = acceptSocket;//���ͻ����׽��ָ�ֵ���̲߳���
		hThread = (HANDLE)_beginthreadex(NULL, 0, &ProxyThread, (LPVOID)lpProxyParam, 0, 0);//�����߳�
		CloseHandle(hThread);//�ر��߳̾��
		Sleep(200);//��ʱ200ms
	}

	//�޷����������д���
	closesocket(ProxyServer);
	WSACleanup();
	return 0;
}
//************************************
// Method: InitSocket
// FullName: InitSocket
// Access: public
// Returns: BOOL
// Qualifier: ��ʼ���׽���
//************************************
BOOL InitSocket() {
	//�����׽��ֿ⣨���룩
	WORD wVersionRequested;
	WSADATA wsaData;
	//�׽��ּ���ʱ������ʾ
	int err;
	//�汾 2.2
	wVersionRequested = MAKEWORD(2, 2);
	//���� dll �ļ� Scoket ��
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		//�Ҳ��� winsock.dll
		printf("���� winsock ʧ�ܣ��������Ϊ: %d\n", WSAGetLastError());
		return FALSE;
	}
	//if�е������Ҫ���ڱȶ��Ƿ���2.2�汾
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		printf("�����ҵ���ȷ�� winsock �汾\n");
		WSACleanup();
		return FALSE;
	}
	// ������socket�ļ�����������IPV4��TCP
	ProxyServer = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == ProxyServer) {
		printf("�����׽���ʧ�ܣ��������Ϊ�� %d\n", WSAGetLastError());
		return FALSE;
	}
	// �����׽���ѡ������ַʹ�ô���
	ProxyServerAddr.sin_family = AF_INET;//���õ�ַ��
	ProxyServerAddr.sin_port = htons(ProxyPort);//���������޷��Ŷ�������ת���������ֽ�˳��
	ProxyServerAddr.sin_addr.S_un.S_addr = INADDR_ANY;//��ʾ����������IP
	// ���׽���
	if (bind(ProxyServer, (SOCKADDR*)&ProxyServerAddr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		printf("���׽���ʧ��\n");
		return FALSE;
	}
	if (listen(ProxyServer, SOMAXCONN) == SOCKET_ERROR) {
		printf("�����˿�%d ʧ��", ProxyPort);
		return FALSE;
	}
	return TRUE;
}
//************************************
// Method: ProxyThread
// FullName: ProxyThread
// Access: public
// Returns: unsigned int __stdcall
// Qualifier: �߳�ִ�к���
// Parameter: LPVOID lpParameter
//************************************
unsigned int __stdcall ProxyThread(LPVOID lpParameter) {
	char Buffer[MAXSIZE];//������
	ZeroMemory(Buffer, MAXSIZE);

	SOCKADDR_IN clientAddr;//�ͻ��˵�ַ
	int length = sizeof(SOCKADDR_IN);//��ַ����
	int recvSize;//���յ������ݴ�С
	int ret;//����ֵ
	recvSize = recv(((ProxyParam*)lpParameter)->clientSocket, Buffer, MAXSIZE, 0);//���յ����ģ�����Buffer��

	char* CacheBuffer;//������
	CacheBuffer = new char[recvSize + 1];
	ZeroMemory(CacheBuffer, recvSize + 1);
	memcpy(CacheBuffer, Buffer, recvSize);//��Buffer�е����ݸ��Ƶ�CacheBuffer��

	HttpHeader* httpHeader = new HttpHeader();
	//����HTTPͷ��
	if (CacheBuffer)
	{
		ParseHttpHead(CacheBuffer, httpHeader);//��CacheBuffer�е����ݽ�����httpHeader��
	}

	//�����ֹ������վ
	//��ʵ�ϵ��Ӵ�ƥ��
	if (strstr(httpHeader->url, blocked_web) != NULL)//����ں�������
	{
		printf("\n==========================================\n");
		printf("--------����վ�ѱ�����!----------\n");
		printf("�ر��׽���\n");
		Sleep(200);
		closesocket(((ProxyParam*)lpParameter)->clientSocket);
		closesocket(((ProxyParam*)lpParameter)->serverSocket);
		delete lpParameter;
		_endthreadex(0);
		return 0;
	}
	//���������վ
	if (strstr(httpHeader->url, fishing_src) != NULL)
	{
		printf("\n==========================================\n");
		printf("---�Ѵ�Դ������ַ��%s ��ת�� Ŀ����ַ ��%s ---\n", fishing_src, fishing_dest);
		//�޸�HTTP����
		memcpy(httpHeader->host, fishing_dest_host, strlen(fishing_dest_host) + 1);//�޸�host��+1�Ǹ��ƽ�ȥ\0
		memcpy(httpHeader->url, fishing_dest, strlen(fishing_dest));//�޸�url
	}
	delete CacheBuffer;

	//����Ŀ������
	//�������ʧ�ܣ��ر��׽���
	if (!ConnectToServer(&((ProxyParam*)lpParameter)->serverSocket, httpHeader->host)) {
		printf("�ر��׽���\n");
		Sleep(200);
		closesocket(((ProxyParam*)lpParameter)->clientSocket);
		closesocket(((ProxyParam*)lpParameter)->serverSocket);
		delete lpParameter;
		_endthreadex(0);
		return 0;
	}
	printf("������������ %s �ɹ�\n", httpHeader->host);

	//���²���Ϊ���沿��
	int index = isInCache(cache, *httpHeader);//�ж��Ƿ��ڻ�����
	//����ڻ����д���
	if (index > -1)
	{
		char* cacheBuffer;//������
		char Buf[MAXSIZE];//������
		ZeroMemory(Buf, MAXSIZE);
		memcpy(Buf, Buffer, recvSize);//��Buffer�е����ݸ��Ƶ�Buf�У�Buffer�е�����Ϊ�ͻ��˷��͵ı���

		//����"If-Modified-Since: "
		ModeHTTP(Buf, cache[index].date);//���ı���date������If-Modified-Since:

		printf("-------------------������------------------------\n%s\n", Buf);
		ret = send(((ProxyParam*)lpParameter)->serverSocket, Buf, strlen(Buf) + 1, 0);//��Buf�е����ݷ��͸�������
		recvSize = recv(((ProxyParam*)lpParameter)->serverSocket, Buf, MAXSIZE, 0);//���շ��������صı���
		printf("------------------Server���ر���-------------------\n%s\n", Buf);

		//������������صı���Ϊ�գ��ر��׽���
		if (recvSize <= 0) {
			printf("�ر��׽���\n");
			Sleep(200);
			closesocket(((ProxyParam*)lpParameter)->clientSocket);
			closesocket(((ProxyParam*)lpParameter)->serverSocket);
			delete lpParameter;
			_endthreadex(0);
			return 0;
		}
		//������������صı��Ĳ�Ϊ�գ������ķ��͸��ͻ���
		const char* No_Modified = "304";

		//����û�иı䣬ֱ�ӷ���cache�е�����
		if (!memcmp(&Buf[9], No_Modified, strlen(No_Modified)))//��304д��Buf��
		{
			//��cache�е����ݷ��͸��ͻ���
			ret = send(((ProxyParam*)lpParameter)->clientSocket, cache[index].buffer, strlen(cache[index].buffer) + 1, 0);
			printf("��cache�еĻ��淵�ؿͻ���\n");
			printf("=================================================\n");
			printf("�ر��׽���\n");
			Sleep(200);
			closesocket(((ProxyParam*)lpParameter)->clientSocket);
			closesocket(((ProxyParam*)lpParameter)->serverSocket);
			delete lpParameter;
			_endthreadex(0);
			return 0;
		}
	}
	//���ͻ��˷��͵� HTTP ���ݱ���ֱ��ת����Ŀ�������
	ret = send(((ProxyParam*)lpParameter)->serverSocket, Buffer, strlen(Buffer) + 1, 0);
	//�ȴ�Ŀ���������������
	recvSize = recv(((ProxyParam*)lpParameter)->serverSocket, Buffer, MAXSIZE, 0);

	//�����ر��ļ��뻺��
	//�ӷ��������ر����н���ʱ��
	char* cacheBuffer2 = new char[MAXSIZE];
	ZeroMemory(cacheBuffer2, MAXSIZE);
	memcpy(cacheBuffer2, Buffer, MAXSIZE);//��Buffer�е����ݸ��Ƶ�cacheBuffer2�У�Buffer�е�����Ϊ���������صı���
	const char* delim = "\r\n";//�ָ���
	char date[DATELENGTH];
	char* nextStr;

	ZeroMemory(date, DATELENGTH);
	char* p = strtok_s(cacheBuffer2, delim, &nextStr);//���У�ʹ��strtok_s��������cacheBuffer2�е����ݷ��У�ÿ�е����ݴ���p��
	bool flag = false;//��ʾ�Ƿ����޸�ʱ�䱨��
	//���Ϸ��У�ֱ���ֳ������޸�ʱ�����һ��
	while (p)
	{
		if (p[0] == 'L')//�ҵ� Last-Modified: ��һ��
		{
			if (strlen(p) > 15)//15��Last-Modified: �ĳ���
			{
				char header[15];//���ڴ洢 Last-Modified:
				ZeroMemory(header, sizeof(header));
				memcpy(header, p, 14);

				if (!(strcmp(header, "Last-Modified:")))//�����Last-Modified: ��һ��
				{
					//��ʱ�����date��
					memcpy(date, &p[15], strlen(p) - 15);
					flag = true;//��ʾ�Ѿ��ҵ����޸�ʱ�䱨��
					break;
				}
			}
		}
		p = strtok_s(NULL, delim, &nextStr);//��������
	}
	if (flag)//����ҵ����޸�ʱ�䱨��
	{
		if (index > -1)//˵���Ѿ������ݴ��ڣ���chache�е�ʱ�������
		{
			memcpy(&(cache[index].buffer), Buffer, strlen(Buffer));
			memcpy(&(cache[index].date), date, strlen(date));
		}
		else//��һ�η��ʣ���Ҫ��ȫ����
		{
			memcpy(&(cache[cache_index % CACHE_NUM].httpHead.host), httpHeader->host, strlen(httpHeader->host));
			memcpy(&(cache[cache_index % CACHE_NUM].httpHead.method), httpHeader->method, strlen(httpHeader->method));
			memcpy(&(cache[cache_index % CACHE_NUM].httpHead.url), httpHeader->url, strlen(httpHeader->url));
			memcpy(&(cache[cache_index % CACHE_NUM].buffer), Buffer, strlen(Buffer));
			memcpy(&(cache[cache_index % CACHE_NUM].date), date, strlen(date));
			cache_index++;
		}
	}
	//��Ŀ����������ص�����ֱ��ת�����ͻ���
	ret = send(((ProxyParam*)lpParameter)->clientSocket, Buffer, sizeof(Buffer), 0);
}
//************************************
// Method: ParseHttpHead
// FullName: ParseHttpHead
// Access: public
// Returns: void
// Qualifier: ���� TCP �����е� HTTP ͷ��
// Parameter: char * buffer
// Parameter: HttpHeader * httpHeader
//************************************
void ParseHttpHead(char* buffer, HttpHeader* httpHeader) {
	char* p;
	char* ptr;
	const char* delim = "\r\n";
	p = strtok_s(buffer, delim, &ptr);//��ȡ��һ��
	printf("%s\n", p);
	if (p == nullptr) {
		return;
	}
	if (p[0] == 'G') {//GET ��ʽ
		memcpy(httpHeader->method, "GET", 3);
		memcpy(httpHeader->url, &p[4], strlen(p) - 13);//��p[4]��p[strlen(p)-13]�����ݸ��Ƶ�httpHeader->url�У�13��GET HTTP/1.1�ĳ���
	}
	else if (p[0] == 'P') {//POST ��ʽ
		memcpy(httpHeader->method, "POST", 4);
		memcpy(httpHeader->url, &p[5], strlen(p) - 14);//��p[5]��p[strlen(p)-14]�����ݸ��Ƶ�httpHeader->url�У�14��POST HTTP/1.1�ĳ���
	}
	printf("%s\n", httpHeader->url);//���url
	p = strtok_s(NULL, delim, &ptr);//��ȡ�ڶ���
	//������ȡ��һ�У�ֱ����ȡ������
	//��ȡHTTPͷ���е�Host��Cookie
	while (p) {
		switch (p[0]) {
		case 'H'://Host
			memcpy(httpHeader->host, &p[6], strlen(p) - 6);//��p[6]��p[strlen(p)-6]�����ݸ��Ƶ�httpHeader->host�У�6��Host: �ĳ���
			break;
		case 'C'://Cookie
			if (strlen(p) > 8) {
				char header[8];//���ڴ洢 Cookie:
				ZeroMemory(header, sizeof(header));
				memcpy(header, p, 6);//��p�е����ݸ��Ƶ�header��
				//�����Cookie: ��һ��
				if (!strcmp(header, "Cookie")) {
					memcpy(httpHeader->cookie, &p[8], strlen(p) - 8);//��p[8]��p[strlen(p)-8]�����ݸ��Ƶ�httpHeader->cookie�У�8��Cookie: �ĳ���
				}
			}
			break;
		default:
			break;
		}
		p = strtok_s(NULL, delim, &ptr);//������ȡ��һ��
	}
}
//************************************
// Method: ConnectToServer
// FullName: ConnectToServer
// Access: public
// Returns: BOOL
// Qualifier: ������������Ŀ��������׽��֣�������
// Parameter: SOCKET * serverSocket
// Parameter: char * host
//************************************
BOOL ConnectToServer(SOCKET* serverSocket, char* host) {
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;//���õ�ַ��
	serverAddr.sin_port = htons(HTTP_PORT);//���������޷��Ŷ�������ת���������ֽ�˳��
	HOSTENT* hostent = gethostbyname(host);//������������ȡ������Ϣ
	if (!hostent) {
		return FALSE;
	}
	in_addr Inaddr = *((in_addr*)*hostent->h_addr_list);//��������Ϣ�еĵ�ַ���Ƶ�Inaddr��
	serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(Inaddr));//��һ���������ַת����һ������������

	*serverSocket = socket(AF_INET, SOCK_STREAM, 0);//�����׽���
	if (*serverSocket == INVALID_SOCKET) {
		return FALSE;
	}
	//����Ŀ�������
	if (connect(*serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		closesocket(*serverSocket);
		return FALSE;
	}
	return TRUE;
}
BOOL isHttpHeadEqual(cacheHttpHead http1, HttpHeader http2)
{
	//�ж�����httpͷ�Ƿ����
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
		//�Ƚ�httpͷ�Ƿ����
		if (isHttpHeadEqual(cache[index].httpHead, httpHeader)) {
			return index;
		}
	}
	return -1;
}
void ModeHTTP(char* buffer, char* date)
{
	//�˺�����HTTP�м����"If-Modified-Since: "
	const char* strHost = "Host";
	const char* inputStr = "If-Modified-Since: ";
	char temp[MAXSIZE];
	ZeroMemory(temp, MAXSIZE);
	char* pos = strstr(buffer, strHost);//�ҵ�Hostλ��

	int i = 0;
	//��host��֮��Ĳ���д��temp
	for (i = 0; i < strlen(pos); i++) {
		temp[i] = pos[i];
	}

	*pos = '\0';

	//����If-Modified-Since�ֶ�
	while (*inputStr != '\0') {  
		*pos++ = *inputStr++;
	}
	//��dateд��buffer��
	while (*date != '\0') {
		*pos++ = *date++;
	}
	*pos++ = '\r';
	*pos++ = '\n';
	//��host֮����ֶθ��Ƶ�buffer��
	for (i = 0; i < strlen(temp); i++) {
		*pos++ = temp[i];
	}
}