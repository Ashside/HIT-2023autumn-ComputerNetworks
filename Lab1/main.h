#pragma once
#define MAXSIZE 65507 //�������ݱ��ĵ���󳤶�
#define HTTP_PORT 80 //http �������˿�
#define DATELENGTH 50 //ʱ���ֽ���
#define CACHE_NUM 50  //������󻺴�����

//������ز���
SOCKET ProxyServer;
sockaddr_in ProxyServerAddr;
const int ProxyPort = 10240;

//��ֹ������վ
const char* blocked_web = { "http://computing.hit.edu.cn/" };
//���Ʒ����û�
const char* blocked_ip = { "127.0.0.2" };
//������վ
const char* fishing_src = "http://today.hit.edu.cn";//������վԭ��ַ
const char* fishing_dest = "http://jwts.hit.edu.cn";//������վĿ����ַ
const char* fishing_dest_host = "jwts.hit.edu.cn";//����Ŀ�ĵ�ַ������

//Httpͷ������
struct HttpHeader {
	char method[4]; // POST ���� GET��ע����ЩΪ CONNECT����ʵ���ݲ�����
	char url[1024]; // ����� url
	char host[1024]; // Ŀ������
	char cookie[1024 * 10]; //cookie
	HttpHeader() {
		ZeroMemory(this, sizeof(HttpHeader));
	}
};
//cache�洢��ʱ��ȥ��Httpͷ����Ϣ�е�cookie
//ֻ��Ϊ�˱Ƚ��Ƿ���ͬ����Ӱ�컺���ʹ��
struct cacheHttpHead {
	char method[4]; // POST ���� GET��ע����ЩΪ CONNECT����ʵ���ݲ�����
	char url[1024]; // ����� url
	char host[1024]; // Ŀ������
	cacheHttpHead() {
		ZeroMemory(this, sizeof(cacheHttpHead));
	}
};
//cache���ݽṹ
struct CACHE {
	cacheHttpHead httpHead;
	char buffer[MAXSIZE];//���汨�ķ�������
	char date[DATELENGTH];//�������ݵ�����޸�ʱ��
	CACHE() {
		ZeroMemory(this->buffer, MAXSIZE);
		ZeroMemory(this->date, DATELENGTH);
	}
};

//�������
struct ProxyParam {
	SOCKET clientSocket;
	SOCKET serverSocket;
};

CACHE cache[CACHE_NUM];//�����ַ
int cache_index = 0;//��¼��ǰӦ�ý���������ĸ�λ��

BOOL InitSocket();
void ParseHttpHead(char* buffer, HttpHeader* httpHeader);
BOOL ConnectToServer(SOCKET* serverSocket, char* host);
unsigned int __stdcall ProxyThread(LPVOID lpParameter);

//�ж�����http�����Ƿ���ͬ,�жϻ���ͱ����Ƿ���ͬ
BOOL isHttpHeadEqual(cacheHttpHead http1, HttpHeader http2);
//Ѱ�һ������Ƿ���ڣ�������ڷ���index�������ڷ���-1��ͨ���Ƚ�httpͷ����Ϣ�ж��Ƿ���ͬ
int isInCache(CACHE* cache, HttpHeader httpHeader);
//�����޸�HTTP���ģ���buffer�����"If-Modified-Since: "�������date
void ModeHTTP(char* buffer, char* date);
