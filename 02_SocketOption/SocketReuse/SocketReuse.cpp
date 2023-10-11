#include "stdafx.h"
#include <WinSock2.h>
#pragma comment(lib, "ws2_32")

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

	// * 바인딩 전에 IP 주소와 포트를 재사용하도록 소켓 옵션을 변경한다.
	BOOL bOption = TRUE;
	if (::setsockopt(hSocket, SOL_SOCKET,
		SO_REUSEADDR, (char*)&bOption, sizeof(BOOL)) == SOCKET_ERROR){
		puts("ERROR: 소켓 옵션을 변경할 수 없습니다.");
		return 0;
	}

	// 2. 포트 바인딩
	SOCKADDR_IN svraddr = { 0 };
	svraddr.sin_family = AF_INET;

	// host 와 network는 endian 방식이 다름. bid endian, little endian
	// htons: host to network short (16 bits)
	// htons: long (32 bits)
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
	char szBuffer[128] = { 0 };
	int nReceive = 0;

	// 4.1 클라이언트 연결을 받아들이고 새로운 소켓 생성(개방)
	while ((hClient = ::accept(hSocket,
		(SOCKADDR*)&clientaddr,
		&nAddrLen)) != INVALID_SOCKET) {
		puts("새 클라이언트가 연결되었습니다.");
		fflush(stdout);

		// 4.2 클라이언트로부터 문자열을 수신함
		while ((nReceive = ::recv(hClient, szBuffer, sizeof(szBuffer), 0)) > 0) {
			// 4.3 수신한 문자열을 그대로 다시 전송
			::send(hClient, szBuffer, sizeof(szBuffer), 0);
			puts(szBuffer);
			fflush(stdout);
			// 메모리 초기화
			memset(szBuffer, 0, sizeof(szBuffer));
		}

		// 4.4 클라이언트가 연결을 종료함
		::shutdown(hClient, SD_BOTH);
		::closesocket(hClient);
		puts("클라이언트 연결이 끊겼습니다.");
		fflush(stdout);
	}

	// 5, 리슨 소켓 닫기
	::closesocket(hSocket);

	// 윈도우 소켓 해제
	::WSACleanup();
	return 0;

}