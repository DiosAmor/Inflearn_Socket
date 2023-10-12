#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include "stdafx.h"
#include <WinSock2.h>
#pragma comment(lib,"ws2_32")

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
	
	// 수신할 파일을 생성한다.
	puts("*** 파일 수신을 시작합니다. ***");
	FILE* fp = NULL;
	errno_t nResult = fopen_s(&fp, "Sleep away.zip", "wb");
	if (nResult != 0)
		ErrorHandler("파일을 생성할 수 없습니다.");

	// 서버가 전송하는 데이터를 반복해서 파일에 붙여 넣는다.
	char byBuffer[65536];	// 64KB
	int nRecv;
	while ((nRecv = ::recv(hSocket, byBuffer, 65536, 0)) > 0) {
		// 서버에서 받은 크기만큼 데이터를 파일에 쓴다.
		fwrite(byBuffer, nRecv, 1, fp);
		putchar('#');
	}

	fclose(fp);
	printf("\n*** 파일 수신이 끝났습니다. ***\n");

	// 소켓을 닫고 프로그램 종료.
	// 이번 경우에는 서버가 연결을 끊음.
	::closesocket(hSocket);
	::WSACleanup();
	return 0;
}