#pragma once

#include <stdlib.h>
#include <minwindef.h>

// MYCMD 구조체의 nCode 멤버에 적용될 수 있는 값
typedef enum CMDCODE {
	CMD_ERROR = 50,				// 에러
	CMD_GET_LIST = 100,			// C->S: 파일 리스트 요구
	CMD_GET_FILE,				// C->S: 파일 전송 요구
	CMD_SND_FILELIST = 200,		// S->C: 파일 리스트 전송
	CMD_BEGIN_FILE				// S->C: 파일 전송 시작을 알림.
} CMDCODE;

// 기본 헤더
typedef struct MYCMD {
	int nCode;	// 명령코드
	int nSize;	// 데이터의 바이트 단위 크기
}MYCMD;

// 확장 헤더: 에러 메시지 전송헤더
typedef struct ERRORDATA {
	int nErrorCode;		// 에러코드: 향후 확장을 위한 멤버
	char szDesc[256];	// 에러내용
}ERRORDATA;

// 확장 헤더: S->C 파일 리스트 전송
typedef struct SEND_FILELIST {
	unsigned int nCount;	// 전송할 파일 정보(GETFILE 구조체) 개수
}SEND_FILELIST;

// 확장 헤더: CMD_GET_FILE
typedef struct GETFILE {
	unsigned int nIndex;	// 전송받으려는 파일의 인덱스
}GETFILE;

// 확장 헤더:
typedef struct FILEINFO {
	unsigned int nIndex;			// 파일의 인덱스
	char szFileName[_MAX_FNAME];	// 파일의 이름
	DWORD dwFileSize;				// 파일의 바이트 단위 크기
} FILEINFO;