#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char** argv) {
	WSADATA ws;
	WSAStartup(MAKEWORD(2, 2), &ws);

	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(6080);
	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	int len = sizeof(addr);
	bind(s, (SOCKADDR*)&addr, len);

	char buf[128];
	SOCKADDR_IN client;
	int recv_len = recvfrom(s, buf, 128, 0, (SOCKADDR*)&client, &len);
	if (recv_len > 0)
	{
		buf[recv_len] = 0;
		printf("recv_len = %d\n", recv_len);
	}

	int send_len = sendto(s, buf, recv_len, 0, (const SOCKADDR*)&client, len);
	printf("send data %d\n", send_len);

	closesocket(s);
	WSACleanup();
	system("pause");
	return 0;
}