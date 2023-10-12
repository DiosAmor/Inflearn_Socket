#include "stdafx.h"
#include <WinSock2.h>
#pragma comment(lib,"ws2_32")
#include <Windows.h>
#include <list>
#include <iterator>

typedef struct _USERSESSION {
	SOCKET hSocket;
	char buffer[8192];	// 8KB
} USERSESSION;

////////////////////////////////////////////////
// 클라이언트 처리를 위한 작업자 스레드 개수.
#define MAX_THREAD_CNT  4

CRITICAL_SECTION g_cs;			// 스레드 동기화 객체
std::list<SOCKET> g_listClient;	// 연결된 클라이언트 소켓 리스트
SOCKET g_hSocket;				// 서버의 리슨 소켓
HANDLE g_hlocp;					// IOCP 핸들

////////////////////////////////////////////////
// 연결된 클라이언트 모두에게 메시지를 전송한다.
void SendMessageAll(char* pszMessage, int nSize) {
	std::list<SOCKET>::iterator it;

	::EnterCriticalSection(&g_cs);
	for (it = g_listClient.begin(); it != g_listClient.end(); ++it)
		::send(*it, pszMessage, nSize, 0);
	::LeaveCriticalSection(&g_cs);
}

/////////////////////////////////////////////////
// 연결된 모든 클라이언트 및 리슨 소켓을 닫는다.
void CloseAll() {
	std::list<SOCKET>::iterator it;
	
	::EnterCriticalSection(&g_cs);
	for (it = g_listClient.begin(); it != g_listClient.end(); ++it) {
		::shutdown(*it, SD_BOTH);
		::closesocket(*it);
	}
	::LeaveCriticalSection(&g_cs);
}

void CloseClient(SOCKET hSock) {
	::shutdown(hSock, SD_BOTH);
	::closesocket(hSock);

	::EnterCriticalSection(&g_cs);
	g_listClient.remove(hSock);
	::LeaveCriticalSection(&g_cs);
}

void ReleaseServer() {
	// 클라이언트 연결을 모두 종료한다.
	CloseAll();
	::Sleep(500);

	// Listen 소켓을 닫는다.
	::shutdown(g_hSocket, SD_BOTH);
	::closesocket(g_hSocket);
	g_hSocket = NULL;

	// IOCP 핸들을 닫는다. 이렇게 하면 GQCS() 함수가 FALSE를 반환하며
	// ::GetLastError() 함수가 ERROR_ABANDONED_WAIT_0을 반환한다.
	// IOCP 스레드들이 모두 종료된다.
	::CloseHandle(g_hlocp);
	g_hlocp = NULL;

	// IOCP 스레드들이 종료되기를 일정시간 동안 기다린다.
	::Sleep(500);
	::DeleteCriticalSection(&g_cs);
}

// Ctrl+C 이벤트를 감지하고 프로그램을 종료한다.
BOOL CtrlHandler(DWORD dwType) {
	if (dwType == CTRL_C_EVENT) {
		ReleaseServer();

		puts("*** 채팅 서버를 종료합니다. ***");
		::WSACleanup();
		exit(0);
		return TRUE;
	}
	return FALSE;
}

DWORD WINAPI THREADComplete(LPVOID pParam) {
	DWORD dwTransferredSize = 0;
	DWORD dwFlag = 0;
	USERSESSION* pSession = NULL;
	LPWSAOVERLAPPED pWol = NULL;
	BOOL bResult;

	puts("[IOCP 작업자 스레드 시작]");
	while (1) {
		bResult = ::GetQueuedCompletionStatus(
			g_hlocp,				// Dequeue할 IOCP 핸들
			&dwTransferredSize,		// 수신한 데이터 크기
			(PULONG_PTR)&pSession,	// 수신된 데이터가 저장된 메모리
			&pWol,					// OVERLAPPED 구조체
			INFINITE);				// 이벤트를 무한정 대기

		if (bResult == TRUE) {
			// 정상적인 경우

			// 1, 클라이언트가 소켓을 정상적으로 닫고 연결을 끊은 경우
			if (dwTransferredSize == 0) {
				CloseClient(pSession->hSocket);
				delete pWol;
				delete pSession;
				puts("\tGQCS: 클라이언트가 정상적으로 연결을 종료함.");
			}

			// 2. 클라이언트가 보낸 데이터를 수신한 경우
			else {
				SendMessageAll(pSession->buffer, dwTransferredSize);
				memset(pSession->buffer, 0, sizeof(pSession->buffer));

				// 다시 IOCP에 등록.
				DWORD dwReceiveSize = 0;
				DWORD dwFlag = 0;
				WSABUF wsaBuf = { 0 };
				wsaBuf.buf = pSession->buffer;
				wsaBuf.len = sizeof(pSession->buffer);

				::WSARecv(
					pSession->hSocket,	// 클라이언트 소켓 핸들
					&wsaBuf,			// WSABUF 구조체 배열의 주소
					1,					// 배열 요소의 개수
					&dwReceiveSize,
					&dwFlag,
					pWol,
					NULL);
				if (::WSAGetLastError() != WSA_IO_PENDING)
					puts("\tGQCS: ERROR: WSARecv()");
			}
		}
		else{
			// 비정상적인 경우

			// 3. 완료 큐에서 완료 패킷을 꺼내지 못하고 반환한 경우
			if (pWol == NULL) {
				// IOCP 핸들이 닫힌 경우(서버를 종료하는 경우)도 해당된다.
				puts("\tGQCS: IOCP 핸들이 닫혔습니다.");
				break;
			}

			// 4. 클라이언트가 비정삭적으로 종료되었거나
			// 서버가 먼저 연결을 종료한 경우
			else {
				if (pSession != NULL) {
					CloseClient(pSession->hSocket);
					delete pWol;
					delete pSession;
				}
				puts("\tGQCS: 서버 종료 혹은 비정상적 연결 종료");
			}
		}
	}
	puts("[IOCP 작업자 스레드 종료]");
	return 0;
}

DWORD WINAPI ThreadAcceptLoop(LPVOID pParam) {
	LPWSAOVERLAPPED pWol = NULL;
	DWORD dwReceiveSize, dwFlag;
	USERSESSION* pNewUser;
	int nAddrSize = sizeof(SOCKADDR);
	WSABUF wsaBuf;
	SOCKADDR ClientAddr;
	SOCKET hClient;
	int nRecvResult = 0;

	while ((hClient = ::accept(g_hSocket, &ClientAddr, &nAddrSize)) != INVALID_SOCKET) {
		puts("새 클라이언트가 연결됐습니다.");
		::EnterCriticalSection(&g_cs);
		g_listClient.push_back(hClient);
		::LeaveCriticalSection(&g_cs);

		// 새 클라이언트에 대한 세션 객체 생성
		pNewUser = new USERSESSION;
		::ZeroMemory(pNewUser, sizeof(USERSESSION));
		pNewUser->hSocket = hClient;

		// 비동기 수신 처리를 위한 OVERLAPPED 구조체 생성
		pWol = new WSAOVERLAPPED;
		::ZeroMemory(pWol, sizeof(WSAOVERLAPPED));

		// 연결된 클라이언트 소켓 핸들을 IOCP에 연결
		::CreateIoCompletionPort((HANDLE)hClient, g_hlocp,
			(ULONG_PTR)pNewUser,		// KEY!!!
			0);

		dwReceiveSize = 0;
		dwFlag = 0;
		wsaBuf.buf = pNewUser->buffer;
		wsaBuf.len = sizeof(pNewUser->buffer);

		// 클라이언트가 보낸 정보를 비동기 수신한다.
		nRecvResult = ::WSARecv(hClient, &wsaBuf, 1, &dwReceiveSize,
			&dwFlag, pWol, NULL);
		if (::WSAGetLastError() != WSA_IO_PENDING)
			puts("ERROR: WSARecv() != WSA_IO_PENDING");
	}
	return 0;
}

int _tmain(int argc, _TCHAR* argv[]) {
	// 윈속 초기화
	WSADATA wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		puts("ERROR: 윈속을 초기화 할 수 없습니다.");
		return 0;
	}

	// 임계영역객체를 생성한다.
	::InitializeCriticalSection(&g_cs);

	// Ctrl+C 키를 눌렀을 때
	if (::SetConsoleCtrlHandler(
		(PHANDLER_ROUTINE)CtrlHandler, TRUE) == FALSE)
		puts("ERROR: Ctrl+C 처리기를 등록할 수 없습니다.");

	// IOCP 생성
	g_hlocp = ::CreateIoCompletionPort(
		INVALID_HANDLE_VALUE,	// 연결된 파일 없음
		NULL,					// 기존 핸들 없음
		0,						// 식별자(Key) 해당되지 않음.
		0);						// 스레드 개수는 OS에 맡김.
	if (g_hlocp == NULL) {
		puts("ERROR: IOCP를 생성할 수 없습니다.");
		return 0;
	}

	// IOCP 스레드를 생성
	HANDLE hThread;
	DWORD dwThreadID;
	for (int i = 0; i < MAX_THREAD_CNT; ++i) {
		dwThreadID = 0;
		// 클라이언트로부터 문자열을 수신함.
		hThread = ::CreateThread(
			NULL,				// 보안 속성 상속
			0,					// 스택 메모리는 기본크기 (1MB)
			THREADComplete,	// 스래드로 실행될 함수이름.
			(LPVOID)NULL,				// 함수에 전달할 매개변수
			0,					// 생성 플래그는 기본값 사용
			&dwThreadID			// 생성된 스레드 ID 저장
		);
		::CloseHandle(hThread);
	}

	// 서버 리슨 소켓 생성
	g_hSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	//bind()/listen()
	SOCKADDR_IN svraddr;
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(25000);
	svraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (::bind(g_hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR) {
		puts("ERROR: 포트가 이미 사용중입니다.");
		ReleaseServer();
		return 0;
	}
	if (::listen(g_hSocket, SOMAXCONN) == SOCKET_ERROR) {
		puts("ERROR: 리슨 상태로 전환할 수 없습니다.");
		ReleaseServer();
		return 0;
	}

	// 반복해서 클라이언트의 연결을 accept() 한다.
	hThread = ::CreateThread(NULL, 0, ThreadAcceptLoop, (LPVOID)NULL, 0, &dwThreadID);
	::CloseHandle(hThread);

	// _tmain() 함수가 반환하지 않도록 대기한다.
	puts("*** 채팅 서버를 시작합니다! ***");
	while (1)
		getchar();
	return 0;

}
