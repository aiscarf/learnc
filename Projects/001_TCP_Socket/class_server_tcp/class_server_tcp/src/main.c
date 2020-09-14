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

	// step1:创建一个socket,指明是TCP的socket;
	int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		goto failed;
	}
	// end

	// step2:bind ip地址与端口号
	struct sockaddr_in sockaddr;
	sockaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(6080);
	ret = bind(s, (const struct sockaddr*) & sockaddr, sizeof(sockaddr));
	if (ret != 0)
	{
		goto failed;
	}
	// step3:开启监听端口
	ret = listen(s, 1);

	while (1)
	{
		// step4:等待客户接入进来;
		struct sockaddr_in c_address;
		int address_len = sizeof(c_address);

		printf("waiting...!!!!\n");
		int cs = accept(s, (const struct sockaddr*) & c_address, &address_len);
		printf("new client %s: %d\n", inet_ntoa(c_address.sin_addr), ntohs(c_address.sin_port));


		// 收数据
		char buf[11];
		memset(buf, 0, 11);
		recv(cs, buf, 5, 0);
		printf("recv: %s\n", buf);

		// 发数据
		send(cs, buf, 5, 0);


		closesocket(cs);
	}

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


/*
	查看端口占用: cmd netstat -ano 
	1> ESTABLISHED	: 表示建立了连接正在通讯
	2> CLOSE_WAIT	: 对方已经关闭,你也要关闭你的socket
	3> TIME_WAIT	: 我方主动调用close()断开连接,收到对方确认后状态变为TIME_WAIT

	关闭socket
	1:根据TCP协议定义的3次握手断开连接规定,发起socket主动关闭的一方socket将进入TIME_WAIT状态,
	TIME_WAIT状态将持续2个MSL(MAX Segment Lifetime), 在Windows下默认为4分钟,即240秒,TIME_WAIT状态下的socket不能被回收使用,
	具体现象是对于一个处理大量短连接的服务器,如果是由服务器主动关闭客户端的连接,将导致服务器端存在大量的处于TIME_WAIT状态的socket,
	甚至比处于Established状态下的socket多得多,严重影响服务器的处理能力,甚至耗尽可用的socket,停止服务,
	TIME_WAIT是TCP协议用以保证被重新分配的socket不会受到之前残留的延迟重发报文影响的机制,是必要的逻辑保证.

	2:收到关闭消息的时候,马上关闭socket
*/