#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Windows.h>

static int g_value = 10; // 数据段
char* ptr = NULL; // 堆
void test_func() { // 代码段

}

HANDLE wait_cond = INVALID_HANDLE_VALUE;
CRITICAL_SECTION lock; // step1:声明锁


/*	线程死锁
	A 锁1, 锁2 ... 释放锁2,锁1
	B 锁2, 锁1 ... 释放锁1,锁2

	解决:就是要使用同样的顺序来获得我们多个锁
	A 锁1, 锁2 ... 释放锁1,锁2
	B 锁1, 锁2 ... 释放锁1,锁2
*/
DWORD WINAPI thread_entry(LPVOID lpThreadParameter) {

	printf("thread id %d\n", GetCurrentThreadId());

	g_value = 8;	// 和进程共用数据段
	ptr[0] = 10;	// 和进程共用堆
	test_func();	// 和进程共用代码段

	int a = 0; // 独立栈
	// 每次要调度出去的时候,把自己的栈保存一下;
	// 每次调度回来的时候,又把栈恢复到调用之前.
	// 线程是OS独立调度的最小单元
	Sleep(5000);
	SetEvent(wait_cond);

	while (1)
	{
		printf("thread called\n");


		EnterCriticalSection(&lock); // step3: 线程请求锁, 没请求到就会在这里挂起等待
		g_value = 8; // 线程不安全
		LeaveCriticalSection(&lock); // step4: 释放锁

		Sleep(3000);

		printf("thread called g_value = %d\n", g_value); // g_value可能不为8
	}

	return 0;
}

int main(int argc, char** argv) {

	printf("main thread id %d\n", GetCurrentThreadId());
	InitializeCriticalSection(&lock); // step2:初始化锁

	ptr = malloc(100);
	// 不用手动的重置这个时间, bManualReset: 是否人工重置
	// ResetEvent(wait_cond);
	wait_cond = CreateEvent(NULL, FALSE, FALSE, NULL);
	int threadid;
	HANDLE h = CreateThread(NULL, 0, thread_entry, NULL, 0, &threadid);

	printf("waiting...\n");
	// INFINITE:超时,假设是一直等
	WaitForSingleObject(wait_cond, INFINITE);
	printf("waiting end \n");


	// 主线程
	while (1)
	{
		printf("main thread\n");

		EnterCriticalSection(&lock); // step3: 请求锁, 没请求到就会在这里挂起等待
		g_value = 10;
		LeaveCriticalSection(&lock); // step4: 释放锁

		Sleep(1500);
	}

	return 0;
}