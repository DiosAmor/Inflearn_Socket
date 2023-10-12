#include "stdafx.h"
#include <Windows.h>

int _tmain(int argc, _TCHAR* argv[]) {
	// 중첩된 쓰기 속성을 부여하고 파일을 생성한다.
	HANDLE hFile = ::CreateFile(
		_T("TestFile.txt"),
		GENERIC_WRITE,		// 쓰기 모드
		0,					// 공유하지 않음
		NULL,
		CREATE_ALWAYS,		// 무조건 생성
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,	// 중첩된 쓰기
		NULL
	);

	if(hFile == INVALID_HANDLE_VALUE) {
		puts("Error: 대상 파일을 열 수 없습니다.");
		return 0;
	}

	// 비동기 쓰기와 관련한 OVERLAPPED 구조체 및 이벤트 핸들을 선언 한다.
	DWORD dwRead;
	OVERLAPPED aOI[3] = { 0 };
	HANDLE aEvt[3] = { 0 };

	// 세 번의 비동기 쓰기 완료를 확인하기 위한 이벤트 객체를 생성한다.
	for (int i = 0; i < 3; ++i) {
		aEvt[i] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		aOI[i].hEvent = aEvt[i];
	}

	// 비동기 쓰기가 시작될 지점을 기술한다.
	// 두 번째 쓰기는 세 번째 쓰기보다 나중에 이루어질 가능성이 높다.
	aOI[0].Offset = 0;				// 파일의 시작.
	aOI[1].Offset = 1024*1024*128;	// 128MB.
	aOI[2].Offset = 16;				// 16바이트.

	// 세 번의 비동기 쓰기를 순차적으로 수행한다.
	for (int i = 0; i < 3; ++i) {
		printf("%d번째 중첩된 쓰기 시도.\n", i);
		::WriteFile(hFile, "0123456789", 10, &dwRead, &aOI[i]);
		// 정상적인 경우 쓰기 시도는 지연(보류)된다!
		if (::GetLastError() != ERROR_IO_PENDING)
			exit(0);
	}

	// 세 번의 비동기 쓰기가 완료되기를 대기한다.
	DWORD dwResult = 0;
	for (int i = 0; i < 3; ++i) {
		dwResult = ::WaitForMultipleObjects(3, aEvt, FALSE, INFINITE);
		printf("->%d번째 쓰기 완료.\n", dwResult - WAIT_OBJECT_0);
	}

	// 이벤트 핸들과 파일을 닫는다.
	for (int i = 0; i < 3; ++i)
		::CloseHandle(aEvt[i]);
	::CloseHandle(hFile);
	return 0;
}