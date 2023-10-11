// WorkerThreadSync.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.

#include "stdafx.h"
#include <Windows.h>

//_tmain() 함수와는 별도로 작동하는 작업자 스레드 함수
DWORD WINAPI ThreadFunction(LPVOID pParam) {
	puts("*** Begin Thread ***");

	for (int i = 0; i < 5; ++i)
	{
		printf("[Worker thread] %d\n", i);
		::Sleep(1);
	}

	// 스레드가 끝나기 전에 이벤트를 세트한다.
	puts("종료 이벤트 센트 전");
	// 아래 함수를 호출하면 _tmain() 함수의 WaitForSingleObject() 함수가 반환됨
	::SetEvent((HANDLE)pParam);
	puts("종료 이벤트 세트 후");
	puts("*** End Thread ***");
	return 0;
}

int _tmain(int argc, _TCHAR* argv[]) {
	
	// 이벤트 객체를 생성한다. --> kernel obj
	HANDLE hEvent = ::CreateEvent(
		NULL,	// 디폴트 보안 속성 적용
		FALSE,	// 자동으로 상태 전환
		FALSE,	// 초기상태는 false
		NULL	// 이름 없음
	);

	// 작업자 스레드를 생성하고 시작한다.
	DWORD dwThreadID = 0;
	HANDLE hThread = ::CreateThread(
		NULL,			// 보안속성 상속
		0,				// 스택 메모리는 기본크기(1MB)
		ThreadFunction,	// 스래드로 실행할 함수이름
		hEvent,			// 이벤트 핸들을 스레드 함수에 전달
		0,				// 생성 플래그는 기본값 사용
		&dwThreadID		// 생성된 스레드 ID 저장
	);

	// 작업자 스레드와 동시에 실행되는 코드 영역의 시작
	for (int i = 0; i < 5; ++i) {
		printf("[Main thread] %d\n", i);
		// i 값이 3이면 이벤트가 세트되기를 무한적 기다린다!
		if (i == 3 && ::WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0) {
			puts("종료 이벤트를 감지했습니다!");
			::CloseHandle(hEvent);
			hEvent = NULL;
		}
	}
	::CloseHandle(hThread);
	//::Sleep(10);
	return 0;
}