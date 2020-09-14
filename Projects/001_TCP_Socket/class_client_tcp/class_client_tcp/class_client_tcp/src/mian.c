#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS

// ����windows socket ����
#ifdef WIN32 // WIN32 ��, Linux�겻����  ��Ŀ->����->C/C++ ->Ԥ������->Ԥ����������
#include <WinSock2.h>
#include <Windows.h>

// ���ӿ� 2�ַ�ʽ:
// 1.��Ŀ->����->������->������
// 2.#pragma comment (libs, "WSOCK32.LIB")
#pragma comment (lib, "WSOCK32.LIB")
#endif

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

	// step1:����һ��TCP socket
	int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		closesocket(s);
	}

	// step2:����һ��Ҫ���ӷ�������socket
	struct sockaddr_in sockaddr;
	sockaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(6080);

	// step3:������������
	ret = connect(s, &sockaddr, sizeof(sockaddr));
	if (ret != 0)
	{
		goto failed;
	}

	// ���ӳɹ�,s���������Ӧ��socket�ͻὨ������;
	// �ͻ��������ӵ�ʱ����Ҳ��Ҫһ��IP��ַ+�˿�(ֻҪ��û��ʹ�õľͿ�����);

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

	// ������ʱ��ҲҪ����
#ifdef WIN32
	WSACleanup();
#endif // WIN32


	system("pause");
}