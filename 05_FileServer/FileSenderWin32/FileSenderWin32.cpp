#include "stdafx.h"
#include <WinSock2.h>
#pragma comment(lib,"ws2_32")
//::TransmitFile() 함수를 사용하기 위한 헤더와 라이브러리 설정
#include<MSWSock.h>
#pragma comment(lib,"Mswsock")


// 전송할 파일에 대한 정보를 담기 위한 구조체 
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

	// 전송할 파일 개방
	HANDLE hfile = ::CreateFile(_T("Sleep Away.zip"),
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN,
		NULL);
	if (hfile == INVALID_HANDLE_VALUE)
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

	// 전송할 파일에 대한 정보를 작성한다.
	MY_FILE_DATA fData = { "Sleep Away.zip",0 };
	fData.dwSize = ::GetFileSize(hfile, NULL);
	TRANSMIT_FILE_BUFFERS tfb = { 0 };
	tfb.Head = &fData;
	tfb.HeadLength = sizeof(fData);

	// 파일 송신
	if (::TransmitFile(
		hClient,	// 파일을 전송할 소켓 핸들
		hfile,		// 전송할 파일 핸들
		0,			// 전송할 크기, 0이면 전체
		65536,		// 한 번에 전송할 버퍼 크기
		NULL,		// 비동기 입/출력에 대한 OVERLAPPED 구조체
		&tfb,		// 파일 전송에 앞서 먼저 전송할 데이터
		0			// 기타 옵션
	) == FALSE)
		ErrorHandler("파일은 전송할 수 없습니다.");

	// 클라이언트가 연결을 끊기를 대기한다.
	::recv(hClient, NULL, 0, 0);
	puts("클라이언트 연결이 끊겼습니다.");

	::closesocket(hSocket);
	::closesocket(hClient);
	::CloseHandle(hfile);
	::WSACleanup();
	return 0;
}