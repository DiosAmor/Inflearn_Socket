#include "stdafx.h"
#include <WinSock2.h>
#pragma comment(lib, "ws2_32")

// 연결된 클라이언트와 통신(Echo 서비스)하기 위한 작업자 스레드 함수
DWORD WINAPI ThreadFunction(LPVOID pParam) {
	char szBuffer[128] = { 0 };
	int nReceive = 0;
	SOCKET hClient = (SOCKET)pParam;

	puts("새 클라이언트가 연결 되었습니다.");

	//클라이언트로부터 문자열을 수신한다.
	while ((nReceive = ::recv(hClient, szBuffer, sizeof(szBuffer), 0)) > 0) {
		// 수신한 문자열을 그대로 다시 전송
		::send(hClient, szBuffer, sizeof(szBuffer), 0);
		puts(szBuffer);
		fflush(stdout);
		// 메모리 초기화
		memset(szBuffer, 0, sizeof(szBuffer));
	}

	// 클라이언트가 연결을 종료함
	//::shutdown(hClient, SD_BOTH);
	::closesocket(hClient);
	puts("클라이언트 연결이 끊겼습니다.");
	fflush(stdout);
	return 0;
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
	if (hSocket == INVALID_SOCKET) {
		puts("ERROR: 접속 대기 소켓을 생성할 수 없습니다.");
		return 0;
	}

	// 2. 포트 바인딩
	SOCKADDR_IN svraddr = { 0 };
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(25000);
	svraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR) {
		puts("ERROR: 소켓에 IP주소와 포트를 바인드 할 수 없습니다.");
		return 0;
	}

	// 3. 접속대기 상태로 전환
	if (::listen(hSocket, SOMAXCONN) == SOCKET_ERROR) {
		puts("ERROR: 리슨 상태로 전환할 수 없습니다.");
		return 0;
	}

	// 4. 클라이언트 접속 처리 및 대응
	SOCKADDR_IN clientaddr = { 0 };
	int nAddrLen = sizeof(clientaddr);
	// 실제로 통신하는 소켓은 hClient
	SOCKET hClient = 0;
	DWORD dwThreadID = 0;
	HANDLE hThread;

	// 4.1 클라이언트 연결을 받아들이고 새로운 소켓 생성(개방)
	while ((hClient = ::accept(hSocket,
		(SOCKADDR*)&clientaddr,
		&nAddrLen)) != INVALID_SOCKET) {
		
		// 4.2 새 클라이언트와 통신하기 위한 스레드 생성
		// 클라이언트마다 스레드가 하나씩 생성된다.
		hThread = ::CreateThread(
			NULL,				// 보안 속성 상속
			0,					// 스택 메모리는 기본크기(1MB)
			ThreadFunction,		// 스레드로 실행할 함수 이름
			(LPVOID)hClient,	// 새로 생성된 클라이언트 소켓
			0,					// 생성 플래그는 기본값 사용
			&dwThreadID			// 생성된 스레드ID가 저장될 변수 주소
		);
		::CloseHandle(hThread);
	}

	// 5, 리슨 소켓 닫기
	::closesocket(hSocket);

	// 윈도우 소켓 해제
	::WSACleanup();
	return 0;

}