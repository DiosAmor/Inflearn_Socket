#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include "stdafx.h"
#include <WinSock2.h>
#pragma comment(lib,"ws2_32")

// 수신 받을 파일에 대한 정보를 담기 위한 구조체
typedef struct MY_FILE_DATA {
	char szName[_MAX_FNAME];
	DWORD dwSize;
}MY_FILE_DATA;


void ErrorHandler(const char* pszMessage) {
	printf("ERROR: %s\n", pszMessage);
	::WSACleanup();
	exit(1);
}

int _tmain(int argc, _TCHAR* argv[]) {
	// 윈도우 소켓 초기화
	WSADATA wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		puts("ERROR: 윈속을 초기화 할 수 없습니다.");
		return 0;
	}

	// 1. 접속 대기 소켓 생성
	SOCKET hSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (hSocket == INVALID_SOCKET)
		ErrorHandler("접속 대기 소켓을 생성할 수 없습니다.");

	// 2. 포트 바인딩
	SOCKADDR_IN svraddr = { 0 };
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(25000);
	svraddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (::connect(hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR)
		ErrorHandler("서버에 연결할 수 없습니다.");

	// 수신할  파일명, 크기 정보를 먼저 받는다.
	MY_FILE_DATA fData = { 0 };
	if (::recv(hSocket, (char*)&fData, sizeof(fData), 0) < sizeof(fData))
		ErrorHandler("파일 정보를 수신하지 못했습니다.");

	// 수신할 파일을 생성한다.
	puts("*** 파일 수신을 시작합니다. ***");
	HANDLE hfile = ::CreateFileA(
		fData.szName,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,	// 파일을 생성한다.
		0,
		NULL);
	if (hfile == INVALID_HANDLE_VALUE)
		ErrorHandler("전송할 파일을 열 수 없습니다.");
	
	// 서버가 전송하는 데이터를 반복해서 파일에 붙여 넣는다.
	char byBuffer[65536];	// 64KB
	int nRecv;
	DWORD dwTotalRecv = 0, dwRead = 0;
	while (dwTotalRecv < fData.dwSize) {
		if ((nRecv = ::recv(hSocket, byBuffer, 65536, 0)) > 0) {
			dwTotalRecv += nRecv;
			// 서버에서 받은 크기만큼 데이터를 파일에 쓴다.
			::WriteFile(hfile, byBuffer, nRecv, &dwRead, NULL);
			printf("Receive: %d/%d\n", dwTotalRecv, fData.dwSize);
			fflush(stdout);
		}
		else {
			puts("ERROR: 파일 수신 중에 오류가 발생했습니다.");
			break;
		}
	}
	
	::CloseHandle(hfile);
	printf("\n*** 파일 수신이 끝났습니다. ***\n");

	// 소켓을 닫고 프로그램 종료.
	// 이번 경우에는 서버가 연결을 끊음.
	::closesocket(hSocket);
	::WSACleanup();
	return 0;
}