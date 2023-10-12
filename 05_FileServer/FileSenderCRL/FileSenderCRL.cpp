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

	// 전송할 파일 개방
	FILE* fp = NULL;
	errno_t nResult = fopen_s(&fp, "Sleep Away.zip", "rb");
	if (nResult != 0)
		ErrorHandler("전송할 파일을 열 수 없습니다.");

	// 1. 접속 대기 소켓 생성
	SOCKET hSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (hSocket == INVALID_SOCKET)
		ErrorHandler("접속 대기 소켓을 생성할 수 없습니다.");

	// 2. 포트 바인딩
	SOCKADDR_IN svraddr = { 0 };
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(25000);
	svraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR)
		ErrorHandler("소켓에 IP주소와 포트를 바인드 할 수 없습니다.");

	// 3. 접속대기 상태로 전환
	if (::listen(hSocket, SOMAXCONN) == SOCKET_ERROR)
		ErrorHandler("리슨 상태로 전환할 수 없습니다.");

	puts("파일송신서버를 시작합니다.");

	// 클라이언트 연결을 받아들이고 새로운 소켓 생성
	SOCKADDR_IN clientaddr = { 0 };
	int nAddrLen = sizeof(clientaddr);
	SOCKET hClient = ::accept(hSocket,
		(SOCKADDR*)&clientaddr, &nAddrLen);
	if (hClient == INVALID_SOCKET)
		ErrorHandler("클라이언트 통신 소켓을 생성할 수 없습니다.");
	puts("클라이언트가 연결되었습니다.");

	// 파일 송신
	char byBuffer[65536];	// 64KB
	int nRead, nSent, i = 0;
	while ((nRead = fread(byBuffer, sizeof(char), 65536, fp)) > 0) {
		// 파일에서 읽고 소켓으로 전송한다.
		// 전송에 성공하더라도 nRead와 nSent 값은 다를 수 있다.
		nSent = send(hClient, byBuffer, nRead, 0);
		printf("[%04d] 전송된 데이터 크기: %d\n", ++i, nSent);
		fflush(stdout);
	}

	// 클라이언트가 파일을 수신할 때까지 일정시간 대기한다.
	::Sleep(100);

	// 서버가 먼저 클라이언트 연결을 종료함.
	::closesocket(hSocket);
	::closesocket(hClient);
	puts("클라이언트 연결이 끊겼습니다.");

	fclose(fp);
	::WSACleanup();
	return 0;
}