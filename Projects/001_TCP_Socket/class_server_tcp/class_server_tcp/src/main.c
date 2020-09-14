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

	while (1)
	{
		// step4:�ȴ��ͻ��������;
		struct sockaddr_in c_address;
		int address_len = sizeof(c_address);

		printf("waiting...!!!!\n");
		int cs = accept(s, (const struct sockaddr*) & c_address, &address_len);
		printf("new client %s: %d\n", inet_ntoa(c_address.sin_addr), ntohs(c_address.sin_port));


		// ������
		char buf[11];
		memset(buf, 0, 11);
		recv(cs, buf, 5, 0);
		printf("recv: %s\n", buf);

		// ������
		send(cs, buf, 5, 0);


		closesocket(cs);
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


/*
	�鿴�˿�ռ��: cmd netstat -ano 
	1> ESTABLISHED	: ��ʾ��������������ͨѶ
	2> CLOSE_WAIT	: �Է��Ѿ��ر�,��ҲҪ�ر����socket
	3> TIME_WAIT	: �ҷ���������close()�Ͽ�����,�յ��Է�ȷ�Ϻ�״̬��ΪTIME_WAIT

	�ر�socket
	1:����TCPЭ�鶨���3�����ֶϿ����ӹ涨,����socket�����رյ�һ��socket������TIME_WAIT״̬,
	TIME_WAIT״̬������2��MSL(MAX Segment Lifetime), ��Windows��Ĭ��Ϊ4����,��240��,TIME_WAIT״̬�µ�socket���ܱ�����ʹ��,
	���������Ƕ���һ��������������ӵķ�����,������ɷ����������رտͻ��˵�����,�����·������˴��ڴ����Ĵ���TIME_WAIT״̬��socket,
	�����ȴ���Established״̬�µ�socket��ö�,����Ӱ��������Ĵ�������,�����ľ����õ�socket,ֹͣ����,
	TIME_WAIT��TCPЭ�����Ա�֤�����·����socket�����ܵ�֮ǰ�������ӳ��ط�����Ӱ��Ļ���,�Ǳ�Ҫ���߼���֤.

	2:�յ��ر���Ϣ��ʱ��,���Ϲر�socket
*/