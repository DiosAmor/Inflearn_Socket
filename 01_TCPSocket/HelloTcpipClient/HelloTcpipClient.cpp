#define _WINSOCK_DEPRECATED_NO_WARNINGS

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

	// 2. 포트 바인딩 및 연결
	SOCKADDR_IN svraddr = { 0 };
	svraddr.sin_family = AF_INET;
	// host 와 network는 endian 방식이 다름. bid endian, little endian
	// htons: host to network short (16 bits)
	// htons: long (32 bits)
	svraddr.sin_port = htons(25000);
	svraddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (::connect(hSocket,
		(SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR) {
		puts("ERROR: 서버에 연결할 수 없습니다.");
		return 0;
	}

	// 3. 채팅 메시지 송/수신
	char szBuffer[128] = { 0 };
	while (1) {
		// 사용자로부터 문자열을 입력 받는다.
		gets_s(szBuffer);
		if (strcmp(szBuffer, "EXIT") == 0) break;
		
		// 사용자가 입력한 문자열을 서버에 전송한다.
		::send(hSocket, szBuffer, strlen(szBuffer)+1, 0);
		// 서버로부터 방금 보낸 문자열에 대한 에코 메시지를 수신한다.
		// 메모리 초기화
		memset(szBuffer, 0, sizeof(szBuffer));
		::recv(hSocket, szBuffer, sizeof(szBuffer), 0);
		printf("From server: %s\n", szBuffer);

	}

	// 4. 소켓을 닫고 종료
	::closesocket(hSocket);

	// 윈도우 소켓 해제
	::WSACleanup();
	return 0;

}