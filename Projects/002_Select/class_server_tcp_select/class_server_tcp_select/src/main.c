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
	����Select����,Ȼ���������,�ȴ����ǹ���ľ��,���¼�����:
	1:������;
	2:�µ��û�����;
	3:�ر�;

	1> ׼��һ���������;
	2> �����ǵ�socket������뵽�����������;
	3> ����select�����ȴ������������,
	4> ������һ��������¼�������ʱ��,OS�������ǵ������select����;
	5> �����¼�,����select;

	ȱ��:
	1.���ܲ���,ÿ�����¼���ʱ��Ҫ�������еľ��,Ȼ������ĸ�������¼�;
	2.�ܹ�����ľ������Ŀ�����Ƶ� ���2048��;
	3.socket�Ķ���д����ͬ����,���Ƿ��ͺͽ������ݵ�ʱ��������������;
*/

static int client_fd[4096];
static int socket_count = 0;

#define MAX_BUF_LEN 4096
static unsigned char recv_buf[MAX_BUF_LEN];

int main(int argc, char** argv) {
	int ret;
	// ����һ��windows socket �汾
	// һ��Ҫ�������, ����Ͱ汾��socket���кܶ�����
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

	// step1:����һ��socket,ָ����TCP��socket;
	int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		goto failed;
	}
	// end

	// step2:bind ip��ַ��˿ں�
	struct sockaddr_in sockaddr;
	sockaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(6080);
	ret = bind(s, (const struct sockaddr*) & sockaddr, sizeof(sockaddr));
	if (ret != 0)
	{
		goto failed;
	}
	// step3:���������˿�
	ret = listen(s, 1);

	fd_set set; // ����һ���������

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

		// ��ʱ,�೤ʱ��û���κ��¼���Ҳ����,��ʱ����
		ret = select(0, &set, NULL, NULL, NULL);  // ���û���κ��¼�, �������͹���select����
		if (ret < 0) // select ���������˴���
		{
			printf("select error\n");
			continue;
		}
		else if (ret == 0) {
			printf("select timeout\n");
			continue;
		}

		// ���ĸ�������¼�������
		if (FD_ISSET(s, &set))
		{
			// step4:�ȴ��ͻ��������;
			struct sockaddr_in c_address;
			int address_len = sizeof(c_address);

			printf("waiting...!!!!\n");
			int cs = accept(s, (const struct sockaddr*) & c_address, &address_len);
			printf("new client %s: %d\n", inet_ntoa(c_address.sin_addr), ntohs(c_address.sin_port));

			// ����� �ͻ���socket���
			client_fd[socket_count] = cs;
			socket_count++;

			continue;
		}

		for (int j = 0; j < socket_count; j++)
		{
			if (client_fd[j] != INVALID_SOCKET && FD_ISSET(client_fd[j], &set))
			{
				int len = recv(client_fd[j], recv_buf, MAX_BUF_LEN, 0);
				if (len <= 0) // �ͻ��˵Ĺر���
				{
					closesocket(client_fd[j]);
					client_fd[j] = INVALID_SOCKET;
				}
				else {	// �յ�������
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

	// ������ʱ��ҲҪ����
#ifdef WIN32
	WSACleanup();
#endif // WIN32


	system("pause");
}