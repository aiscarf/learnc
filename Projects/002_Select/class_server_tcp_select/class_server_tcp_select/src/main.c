#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifdef WIN32
#include <WinSock2.h>
#include <Windows.h>
#pragma comment (lib, "WSOCK32.LIB")
#endif

/*
	Select:
	调用Select函数,然后任务挂起,等待我们管理的句柄,有事件发生:
	1:读数据;
	2:新的用户进来;
	3:关闭;

	1> 准备一个句柄集合;
	2> 将我们的socket句柄加入到我们这个集合;
	3> 调用select函数等待在这个集合上,
	4> 当其中一个句柄有事件发生的时候,OS唤醒我们的任务从select返回;
	5> 处理事件,继续select;

	缺点:
	1.性能不好,每次有事件的时候都要遍历所有的句柄,然后查是哪个句柄的事件;
	2.能够管理的句柄的数目是限制的 最多2048个;
	3.socket的读和写还是同步的,我们发送和接受数据的时候会等在网卡上面;
*/

static int client_fd[4096];
static int socket_count = 0;

#define MAX_BUF_LEN 4096
static unsigned char recv_buf[MAX_BUF_LEN];

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

	fd_set set; // 定义一个句柄集合

	while (1)
	{
		FD_ZERO(&set);
		FD_SET(s, &set);

		for (int j = 0; j < socket_count; j++)
		{
			if (client_fd[j]!=INVALID_SOCKET)
			{
				FD_SET(client_fd[j], &set);
			}
		}

		// 超时,多长时间没有任何事件我也返回,超时返回
		ret = select(0, &set, NULL, NULL, NULL);  // 如果没有任何事件, 这个任务就挂起到select里面
		if (ret < 0) // select 函数发生了错误
		{
			printf("select error\n");
			continue;
		}
		else if (ret == 0) {
			printf("select timeout\n");
			continue;
		}

		// 是哪个句柄有事件发生了
		if (FD_ISSET(s, &set))
		{
			// step4:等待客户接入进来;
			struct sockaddr_in c_address;
			int address_len = sizeof(c_address);

			printf("waiting...!!!!\n");
			int cs = accept(s, (const struct sockaddr*) & c_address, &address_len);
			printf("new client %s: %d\n", inet_ntoa(c_address.sin_addr), ntohs(c_address.sin_port));

			// 保存好 客户端socket句柄
			client_fd[socket_count] = cs;
			socket_count++;

			continue;
		}

		for (int j = 0; j < socket_count; j++)
		{
			if (client_fd[j] != INVALID_SOCKET && FD_ISSET(client_fd[j], &set))
			{
				int len = recv(client_fd[j], recv_buf, MAX_BUF_LEN, 0);
				if (len <= 0) // 客户端的关闭了
				{
					closesocket(client_fd[j]);
					client_fd[j] = INVALID_SOCKET;
				}
				else {	// 收到了数据
					recv_buf[len] = 0;
					printf("%s\n", recv_buf);
					send(client_fd[j], recv_buf, len, 0);
				}
			}
		}
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