/*
	IOCP 支持windows平台
	Linux平台有 epoll
*/

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <WinSock2.h>
#include <MSWSock.h>
#include <Windows.h>

#pragma comment(lib, "WSOCK32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "odbc32.lib")
#pragma comment(lib, "odbccp32.lib")

enum {
	IOCP_ACCPET = 0, // 监听socket, 接入请求
	IOCP_RECV,	// 读请求
	IOCP_WRITE, // 写请求
};

// 接收数据的时候最大的buf大小;
#define MAX_RECV_SIZE 8192

// 自己定义的数据结构,一定要在第一个就要用WSAOVERLAPPED 结构体
struct io_package {
	// 所有的请求的的等待,都是等在这个结构对象上的
	WSAOVERLAPPED overlapped;
	int opt; // 操作类型, 对应上面的枚举
	int accpet_sock; // 提交请求的句柄
	WSABUF wsabuffer;	// 结构体,配合读写数据的时候用的buffer wsabuffer.buf = 内存; wsabuffer.len = MAX_RECV_SIZE;
	char pkg[MAX_RECV_SIZE];	// 定义了一个buf,真正的内存
};


static void post_accept(SOCKET l_sock) {
	// step1: 分配一个io_package 数据结构
	struct io_package* pkg = malloc(sizeof(struct io_package));
	memset(pkg, 0, sizeof(struct io_package));

	// 初始化好接受数据的buf--> WSABUF
	pkg->opt = IOCP_ACCPET;
	pkg->wsabuffer.buf = pkg->pkg;
	pkg->wsabuffer.len = MAX_RECV_SIZE - 1;

	DWORD dwBytes = 0;
	SOCKET client = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, dwBytes);
	int addr_size = sizeof(struct sockaddr_in) + 16;
	pkg->accpet_sock = client; // 创建一个socket, 然后客户端连接进来后,就用这个socket和我们的客户端通讯
	AcceptEx(l_sock, client, pkg->wsabuffer.buf, 0, addr_size, addr_size, &dwBytes, &pkg->overlapped);
}

static void post_recv(SOCKET client_fd) {
	struct io_package* io_data = malloc(sizeof(struct io_package));
	memset(io_data, 0, sizeof(struct io_package));

	io_data->opt = IOCP_RECV;
	io_data->wsabuffer.buf = io_data->pkg;
	io_data->wsabuffer.len = MAX_RECV_SIZE - 1;
	io_data->accpet_sock = client_fd;

	DWORD dwRecv = 0;
	DWORD dwFlags = 0;
	WSARecv(client_fd, &(io_data->wsabuffer), 1, &dwRecv, &dwFlags, &(io_data->overlapped), NULL);
}


int main(int argc, char** argv) {
	// 如果你做socket那么必须要加上
	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);
	// end

	// step1:创建一个完成端口
	HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (iocp == INVALID_HANDLE_VALUE)
	{
		goto failed;
	}

	// step2:创建Socket
	SOCKET l_sock = INVALID_SOCKET;
	l_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (l_sock == INVALID_SOCKET)
	{
		goto failed;
	}

	// step4:配置 ip + port
	struct sockaddr_in s_address;
	memset(&s_address, 0, sizeof(s_address));
	s_address.sin_family = AF_INET;
	s_address.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	s_address.sin_port = htons(6080);
 
	if (bind(l_sock, (struct sockaddr*)&s_address, sizeof(s_address)))
	{
		goto failed;
	}

	// step5:监听socket
	if (listen(l_sock, SOMAXCONN) != 0)
	{
		goto failed;
	}

	// step6:让IOCP来管理我们的l_sock
	// 第三个参数, 是用户传的自定义数据(一个指针带进去)
	CreateIoCompletionPort((HANDLE)l_sock, iocp, (DWORD)0, 0);

	// step7:投递一个accept接入请求
	post_accept(l_sock); 

	while (1)
	{
		DWORD dwTrans; // 读到数据的大小
		DWORD udata;
		struct io_package* io_data; 
		// 通过完成端口, 来获得这个请求的结果;
		// 调用操作系统的api函数,查看哪个请求完成了
		// 如果没有状态完成了,那么任务会挂起等待
		int ret = GetQueuedCompletionStatus(iocp, &dwTrans, &udata, (LPOVERLAPPED*)&io_data, WSA_INFINITE);
		printf("ret == %d\n", ret);
		if (ret == 0) // 意外
		{
			if (io_data)
			{
				if (io_data->opt == IOCP_RECV)
				{
					closesocket(io_data->accpet_sock);
					free(io_data);
				}
				else if (io_data->opt == IOCP_ACCPET)
				{
					free(io_data);
					post_accept(l_sock);
				}
			}
			continue;
		}

		// 客户端要关闭socket,服务器需要马上关闭socket
		if (dwTrans == 0 && io_data->opt == IOCP_RECV)
		{
			printf("client is closed\n");
			closesocket(io_data->accpet_sock);
			free(io_data);
			continue;
		}

		printf("io_data->opt = %d\n", io_data->opt);
		switch (io_data->opt)
		{
			case IOCP_ACCPET: 
			{
				int client_fd = io_data->accpet_sock;
				int addr_size = (sizeof(struct sockaddr_in) + 16);
				struct sockaddr_in* l_addr = NULL;
				int l_len = sizeof(struct sockaddr_in);

				struct sockaddr_in* r_addr = NULL;
				int r_len = sizeof(struct sockaddr_in);

				GetAcceptExSockaddrs(io_data->wsabuffer.buf, 0, addr_size, addr_size, (struct sockaddr**) & l_addr, &l_len, (struct sockaddr**) & r_addr, &r_len);

				printf("local = %s: %d\n", inet_ntoa(l_addr->sin_addr), ntohs(l_addr->sin_port));
				printf("remote = %s: %d\n",inet_ntoa(r_addr->sin_addr),ntohs(r_addr->sin_port));

				// 第三个参数是用户定义的数据, 这里传的是客户端socket的client_fd
				CreateIoCompletionPort((HANDLE)client_fd, iocp, (DWORD)client_fd, 0);

				// 投递一个读的请求;
				post_recv(client_fd);

				// 重新投递一个客户端接入请求;
				free(io_data);
				post_accept(l_sock);
				break;
			}
			case IOCP_RECV:
			{
				io_data->pkg[dwTrans] = 0;
				printf("IOCP recv %d %s\n", dwTrans, io_data->pkg);
				send(io_data->accpet_sock, io_data->pkg, dwTrans, 0);

				// 再来投递下一个请求;
				DWORD dwRecv = 0;
				DWORD dwFlags = 0;

				int ret = WSARecv(io_data->accpet_sock, &(io_data->wsabuffer), 1, &dwRecv, &dwFlags, &(io_data->overlapped), NULL);
				break;
			}

			default:
				break;
		}
	}

failed:
	if (l_sock != INVALID_SOCKET)
	{
		closesocket(l_sock);
	}
	if (iocp != INVALID_HANDLE_VALUE)
	{
		CloseHandle(iocp);
	}
	WSACleanup();
	printf("failed");
	system("pause");
	return 0;
}