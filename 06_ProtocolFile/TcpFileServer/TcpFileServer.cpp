#include "stdafx.h"
#include <WinSock2.h>
#pragma comment(lib,"ws2_32")
// 응용 프로그램 통신 프로토콜 정의가 들어있는 헤더파일
#include "AppProtocol.h"

// 전송할 파일정보 개수
SEND_FILELIST g_flist = { 3 };

// 클라이언트가 다운로드 가능한 파일
FILEINFO g_aFInfo[3] = {
	{0,"Sleep Away.mp3", 4842525},
	{1,"Kalimba.mp3", 8414449},
	{2,"Maid with the Flaxen Hair.mp3", 4113874},
};

void ErrorHandler(const char* pszMessage) {
	printf("ERROR: %s\n", pszMessage);
	::WSACleanup();
	exit(1);
}

// 파일 리스트 정보를 클라이언트에 전송하는 함수
void SendFileList(SOCKET hClient) {
	MYCMD cmd;
	cmd.nCode = CMD_SND_FILELIST;
	cmd.nSize = sizeof(g_flist) + sizeof(g_aFInfo);
	// 기본 헤더 전송.
	send(hClient, (const char*)&cmd, sizeof(cmd), 0);
	// 파일 리스트 헤더 전송
	send(hClient, (const char*)&g_flist, sizeof(g_flist), 0);
	// 파일 정보들 전송
	send(hClient, (const char*)&g_aFInfo, sizeof(g_aFInfo), 0);
}

// 인덱스에 해당하는 파일을 클라이언트에게 송신하는 함수
void SendFile(SOCKET hClient, int nIndex) {
	MYCMD cmd;
	ERRORDATA err;
	// 파일 리스트에서 인덱스에 맞는 파일을 검사한다.
	if (nIndex < 0 || nIndex>2) {
		cmd.nCode = CMD_ERROR;
		cmd.nSize = sizeof(err);
		err.nErrorCode = 0;
		strcpy_s(err.szDesc, "잘못된 파일 인덱스 입니다.");

		// 오류 정보를 클라이언트에게 전송.
		send(hClient, (const char*)&cmd, sizeof(cmd), 0);
		send(hClient, (const char*)&err, sizeof(err), 0);
		return;
	}

	// 파일 송신 시작을 알리는 정보를 전송한다.
	cmd.nCode = CMD_BEGIN_FILE;
	cmd.nSize = sizeof(FILEINFO);
	send(hClient, (const char*)&cmd, sizeof(cmd), 0);
	// 송신하는 파일에 대한 정보를 전송한다.
	send(hClient, (const char*)&g_aFInfo[nIndex], sizeof(FILEINFO), 0);

	FILE* fp = NULL;
	errno_t nResult = fopen_s(&fp, g_aFInfo[nIndex].szFileName, "rb");
	if (nResult != 0)
		ErrorHandler("전송할 파일을 개방할 수 없습니다.");

	// 파일을 송신한다.
	char byBuffer[65536];	// 64KB
	int nRead;
	while ((nRead = fread(byBuffer, sizeof(char), 65536, fp)) > 0)
		send(hClient, byBuffer, nRead, 0);
	fclose(fp);
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

	// 클라이언트로부터 명령을 수신하고 대응하는 Event loop.
	MYCMD cmd;
	while (::recv(hClient, (char*)&cmd, sizeof(MYCMD), 0) > 0) {
		switch (cmd.nCode) {
		case CMD_GET_LIST:
			puts("클라이언트가 파일목록을 요구함.");
			SendFileList(hClient);
			break;
		case CMD_GET_FILE:
			puts("클라이언트가 파일전송을 요구함.");
			{
				GETFILE file;
				::recv(hClient, (char*)&file, sizeof(file), 0);
				SendFile(hClient, file.nIndex);
				break;
			}
		default:
			ErrorHandler("알 수 없는 명령을 수신했습니다.");
			break;
		}
	}

	// 클라이언트가 연결을 종료함.
	::closesocket(hClient);
	puts("클라이언트 연결이 끊겼습니다.");

	// 리슨 소켓 닫기.
	::closesocket(hSocket);
	
	::WSACleanup();
	return 0;
}