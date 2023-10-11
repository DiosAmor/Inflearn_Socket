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

	// 소켓 생성
	SOCKET hSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (hSocket == INVALID_SOCKET) {
		puts("ERROR: 소켓을 생성할 수 없습니다.");
		return 0;
	}

	// 소켓의 '송신' 버퍼의 크기를 확인하고 출력한다.
	int nBufSize = 0, nLen = sizeof(nBufSize);
	if (::getsockopt(hSocket, SOL_SOCKET,
		SO_SNDBUF, (char*)&nBufSize, &nLen) != SOCKET_ERROR)
		printf("Send buffer size: %d\n", nBufSize);

	// 소켓의 '수신' 버퍼의 크기를 확인하고 출력한다.
	nBufSize = 0;
	nLen = sizeof(nBufSize);
	if (::getsockopt(hSocket, SOL_SOCKET,
		SO_RCVBUF, (char*)&nBufSize, &nLen) != SOCKET_ERROR)
		printf("Receive buffer size: %d\n", nBufSize);

	// 소켓을 닫고 종료.
	::closesocket(hSocket);
	::WSACleanup();
	return 0;
}