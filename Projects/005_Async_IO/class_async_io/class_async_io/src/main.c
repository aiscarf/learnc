#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Windows.h> 

int main(int argc, char** argv) {

	// ͬ����
	HANDLE hfile = INVALID_HANDLE_VALUE;

	// L:unicode�ַ���, ÿ���ַ�ռ�����ֽ�
	hfile = CreateFile(L"in.txt", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hfile == INVALID_HANDLE_VALUE)
	{
		printf("open error\n");
		goto failed;
	}

	// ׼�����ڴ�
	char buf[102400];
	int readed;
	ReadFile(hfile, buf, 102400, &readed, NULL);
	buf[readed] = 0;
	printf("%s\n", buf);

	CloseHandle(hfile);

	// async �첽
	// step1: ���첽ģʽ��
	hfile = CreateFile(L"in.txt", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile == INVALID_HANDLE_VALUE)
	{
		printf("open error\n");
		goto failed;
	}

	// step2: ����ʱ��,��������,׼��Ҳ��OVERLAPPED�������;
	// �������ǵ�OS, �����ǵ�OS��д�Ժ�,
	// ��ͨ��OVERLAPPED �������Ƿ���һ���¼�

	OVERLAPPED ov;
	HANDLE hevent = CreateEvent(NULL, FALSE, FALSE, NULL);
	memset(&ov, 0, sizeof(ov));
	ov.hEvent = hevent;
	ov.Offset = 0;
	// ���Ϸ���,IO����,û�ж�������
	ReadFile(hfile, buf, 102400, &readed, &ov);
	if (GetLastError() == ERROR_IO_PENDING)
	{
		// �ȴ����������
		WaitForSingleObject(hevent, INFINITE);
		readed = ov.InternalHigh;
		buf[readed] = 0;
		printf("async %s\n", buf);
		CloseHandle(hfile);
	}
	else {
		CloseHandle(hfile);
	}
	printf("111\n");
	// ͬ��,�첽���ڵ�,��ʲô����
	// 1> ͬ����, API�ڲ���,ֱ�ӹ���
	// 2> �첽��,ʹ�û��Լ���д�������ȴ�;
	// 3> ������, ����ͬ�µȴ��������, ��߲��������� 
	// WaitForMultipleObject

failed:
	system("pause");
	return 0;
}