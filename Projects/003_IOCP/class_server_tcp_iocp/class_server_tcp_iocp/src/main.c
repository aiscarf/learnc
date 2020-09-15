/*
	IOCP ֧��windowsƽ̨
	Linuxƽ̨�� epoll
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
	IOCP_ACCPET = 0, // ����socket, ��������
	IOCP_RECV,	// ������
	IOCP_WRITE, // д����
};

// �������ݵ�ʱ������buf��С;
#define MAX_RECV_SIZE 8192

// �Լ���������ݽṹ,һ��Ҫ�ڵ�һ����Ҫ��WSAOVERLAPPED �ṹ��
struct io_package {
	// ���е�����ĵĵȴ�,���ǵ�������ṹ�����ϵ�
	WSAOVERLAPPED overlapped;
	int opt; // ��������, ��Ӧ�����ö��
	int accpet_sock; // �ύ����ľ��
	WSABUF wsabuffer;	// �ṹ��,��϶�д���ݵ�ʱ���õ�buffer wsabuffer.buf = �ڴ�; wsabuffer.len = MAX_RECV_SIZE;
	char pkg[MAX_RECV_SIZE];	// ������һ��buf,�������ڴ�
};


static void post_accept(SOCKET l_sock) {
	// step1: ����һ��io_package ���ݽṹ
	struct io_package* pkg = malloc(sizeof(struct io_package));
	memset(pkg, 0, sizeof(struct io_package));

	// ��ʼ���ý������ݵ�buf--> WSABUF
	pkg->opt = IOCP_ACCPET;
	pkg->wsabuffer.buf = pkg->pkg;
	pkg->wsabuffer.len = MAX_RECV_SIZE - 1;

	DWORD dwBytes = 0;
	SOCKET client = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, dwBytes);
	int addr_size = sizeof(struct sockaddr_in) + 16;
	pkg->accpet_sock = client; // ����һ��socket, Ȼ��ͻ������ӽ�����,�������socket�����ǵĿͻ���ͨѶ
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
	// �������socket��ô����Ҫ����
	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);
	// end

	// step1:����һ����ɶ˿�
	HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (iocp == INVALID_HANDLE_VALUE)
	{
		goto failed;
	}

	// step2:����Socket
	SOCKET l_sock = INVALID_SOCKET;
	l_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (l_sock == INVALID_SOCKET)
	{
		goto failed;
	}

	// step4:���� ip + port
	struct sockaddr_in s_address;
	memset(&s_address, 0, sizeof(s_address));
	s_address.sin_family = AF_INET;
	s_address.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	s_address.sin_port = htons(6080);
 
	if (bind(l_sock, (struct sockaddr*)&s_address, sizeof(s_address)))
	{
		goto failed;
	}

	// step5:����socket
	if (listen(l_sock, SOMAXCONN) != 0)
	{
		goto failed;
	}

	// step6:��IOCP���������ǵ�l_sock
	// ����������, ���û������Զ�������(һ��ָ�����ȥ)
	CreateIoCompletionPort((HANDLE)l_sock, iocp, (DWORD)0, 0);

	// step7:Ͷ��һ��accept��������
	post_accept(l_sock); 

	while (1)
	{
		DWORD dwTrans; // �������ݵĴ�С
		DWORD udata;
		struct io_package* io_data; 
		// ͨ����ɶ˿�, ������������Ľ��;
		// ���ò���ϵͳ��api����,�鿴�ĸ����������
		// ���û��״̬�����,��ô��������ȴ�
		int ret = GetQueuedCompletionStatus(iocp, &dwTrans, &udata, (LPOVERLAPPED*)&io_data, WSA_INFINITE);
		printf("ret == %d\n", ret);
		if (ret == 0) // ����
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

		// �ͻ���Ҫ�ر�socket,��������Ҫ���Ϲر�socket
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

				// �������������û����������, ���ﴫ���ǿͻ���socket��client_fd
				CreateIoCompletionPort((HANDLE)client_fd, iocp, (DWORD)client_fd, 0);

				// Ͷ��һ����������;
				post_recv(client_fd);

				// ����Ͷ��һ���ͻ��˽�������;
				free(io_data);
				post_accept(l_sock);
				break;
			}
			case IOCP_RECV:
			{
				io_data->pkg[dwTrans] = 0;
				printf("IOCP recv %d %s\n", dwTrans, io_data->pkg);
				send(io_data->accpet_sock, io_data->pkg, dwTrans, 0);

				// ����Ͷ����һ������;
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