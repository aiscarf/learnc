#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS

// 配置windows socket 环境
#ifdef WIN32 // WIN32 宏, Linux宏不存在  项目->属性->C/C++ ->预处理器->预处理器定义
#include <WinSock2.h>
#include <Windows.h>

// 链接库 2种方式:
// 1.项目->属性->链接器->命令行
// 2.#pragma comment (libs, "WSOCK32.LIB")
#pragma comment (lib, "WSOCK32.LIB")
#endif

int main(int argc, char** argv) {
	int ret;
	// 配置一下windows socket 版本
	// 一定要加上这个, 否则低版本的socket会有很多问题
#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 2);
	ret = WSAStartup(wVersionRequested, &wsaData);
	if (ret != 0)
	{
		printf("WSAStart up failed\n");
		system("pause");
		return -1;
	}

#endif

	// step1:创建一个TCP socket
	int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		closesocket(s);
	}

	// step2:配置一下要连接服务器的socket
	struct sockaddr_in sockaddr;
	sockaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(6080);

	// step3:发送连接请求
	ret = connect(s, &sockaddr, sizeof(sockaddr));
	if (ret != 0)
	{
		goto failed;
	}

	// 连接成功,s与服务器对应的socket就会建立连接;
	// 客户端在连接的时候他也需要一个IP地址+端口(只要是没有使用的就可以了);

	char buf[11];
	memset(buf, 0, 11);
	//send(s, "Hello", 5, 0);
	recv(s, buf, 5, 0);
	printf("recv: %s\n", buf);

failed:
	if (s != INVALID_SOCKET)
	{
		closesocket(s);
	}

	// 结束的时候也要清理
#ifdef WIN32
	WSACleanup();
#endif // WIN32


	system("pause");
}