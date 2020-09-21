#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Windows.h> 

int main(int argc, char** argv) {

	// 同步读
	HANDLE hfile = INVALID_HANDLE_VALUE;

	// L:unicode字符串, 每个字符占两个字节
	hfile = CreateFile(L"in.txt", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hfile == INVALID_HANDLE_VALUE)
	{
		printf("open error\n");
		goto failed;
	}

	// 准备好内存
	char buf[102400];
	int readed;
	ReadFile(hfile, buf, 102400, &readed, NULL);
	buf[readed] = 0;
	printf("%s\n", buf);

	CloseHandle(hfile);

	// async 异步
	// step1: 以异步模式打开
	hfile = CreateFile(L"in.txt", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile == INVALID_HANDLE_VALUE)
	{
		printf("open error\n");
		goto failed;
	}

	// step2: 读的时候,发送请求,准备也给OVERLAPPED这个对象;
	// 传给我们的OS, 等我们的OS读写以后,
	// 会通过OVERLAPPED 来给我们发送一个事件

	OVERLAPPED ov;
	HANDLE hevent = CreateEvent(NULL, FALSE, FALSE, NULL);
	memset(&ov, 0, sizeof(ov));
	ov.hEvent = hevent;
	ov.Offset = 0;
	// 马上返回,IO挂起,没有读到数据
	ReadFile(hfile, buf, 102400, &readed, &ov);
	if (GetLastError() == ERROR_IO_PENDING)
	{
		// 等待读请求完成
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
	// 同步,异步都在等,有什么区别
	// 1> 同步等, API内部等,直接挂起
	// 2> 异步等,使用户自己来写代码来等待;
	// 3> 我们用, 可以同事等待多个请求, 提高并发请求数 
	// WaitForMultipleObject

failed:
	system("pause");
	return 0;
}