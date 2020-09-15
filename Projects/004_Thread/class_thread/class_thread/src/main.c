#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Windows.h>

static int g_value = 10; // ���ݶ�
char* ptr = NULL; // ��
void test_func() { // �����

}

HANDLE wait_cond = INVALID_HANDLE_VALUE;
CRITICAL_SECTION lock; // step1:������


/*	�߳�����
	A ��1, ��2 ... �ͷ���2,��1
	B ��2, ��1 ... �ͷ���1,��2

	���:����Ҫʹ��ͬ����˳����������Ƕ����
	A ��1, ��2 ... �ͷ���1,��2
	B ��1, ��2 ... �ͷ���1,��2
*/
DWORD WINAPI thread_entry(LPVOID lpThreadParameter) {

	printf("thread id %d\n", GetCurrentThreadId());

	g_value = 8;	// �ͽ��̹������ݶ�
	ptr[0] = 10;	// �ͽ��̹��ö�
	test_func();	// �ͽ��̹��ô����

	int a = 0; // ����ջ
	// ÿ��Ҫ���ȳ�ȥ��ʱ��,���Լ���ջ����һ��;
	// ÿ�ε��Ȼ�����ʱ��,�ְ�ջ�ָ�������֮ǰ.
	// �߳���OS�������ȵ���С��Ԫ
	Sleep(5000);
	SetEvent(wait_cond);

	while (1)
	{
		printf("thread called\n");


		EnterCriticalSection(&lock); // step3: �߳�������, û���󵽾ͻ����������ȴ�
		g_value = 8; // �̲߳���ȫ
		LeaveCriticalSection(&lock); // step4: �ͷ���

		Sleep(3000);

		printf("thread called g_value = %d\n", g_value); // g_value���ܲ�Ϊ8
	}

	return 0;
}

int main(int argc, char** argv) {

	printf("main thread id %d\n", GetCurrentThreadId());
	InitializeCriticalSection(&lock); // step2:��ʼ����

	ptr = malloc(100);
	// �����ֶ����������ʱ��, bManualReset: �Ƿ��˹�����
	// ResetEvent(wait_cond);
	wait_cond = CreateEvent(NULL, FALSE, FALSE, NULL);
	int threadid;
	HANDLE h = CreateThread(NULL, 0, thread_entry, NULL, 0, &threadid);

	printf("waiting...\n");
	// INFINITE:��ʱ,������һֱ��
	WaitForSingleObject(wait_cond, INFINITE);
	printf("waiting end \n");


	// ���߳�
	while (1)
	{
		printf("main thread\n");

		EnterCriticalSection(&lock); // step3: ������, û���󵽾ͻ����������ȴ�
		g_value = 10;
		LeaveCriticalSection(&lock); // step4: �ͷ���

		Sleep(1500);
	}

	return 0;
}