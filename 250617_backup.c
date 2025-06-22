#pragma region include, define

#define WINVER        0x0501
#define _WIN32_WINNT  0x0501

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>
#include <time.h>
#include <string.h>
#include <math.h>

#ifdef _MSC_VER
int ide = 1;
#elif __MINGW32__ || __MINGW64__ || __GNUC__
int ide = 2;
#define vswprintf _vsnwprintf
#define swprintf _snwprintf
#else
int ide = 0;
#endif

#ifndef CONSOLE_FULLSCREEN_MODE
#define CONSOLE_FULLSCREEN_MODE 1
#endif

#ifndef WEOF
#define WEOF ((wint_t)(0xFFFF))
#endif

#ifndef LC_ALL
#define LC_ALL 0
#endif

#define LF_MAX ((pow(2,1022) - 1) * 2 + 1)
#define LF_MIN (LF_MAX * (-1) - 1)
#define ENUMBER_MAX ((lf[]) { 9.0, 9.0, 9.0 })
#define ENUMBER_MIN ((lf[]) { -9.0, 9.0, 9.0 })
#define TEMPSCREENSCALE_X 300 //화면 크기를 불러오기 전 임시로 사용되는 화면 크기
#define TEMPSCREENSCALE_Y 150
#define screenSize screen[0] * screen[1]
#define settings_txt L"txt_images\\SETTINGS.txt" //SETTINGS.txt 파일 위치.
#define store_txt L"txt_images\\STORE.txt" //STORE.txt 파일 위치.
#define savefile_txt L"saved_data\\savefile.txt" //saveFile.txt 파일 위치.
//만약 SETTINGS.txt가 정상적으로 불러와지지 않는 경우 txtImages 폴더 내에 SETTINGS.txt가 존재하지 않을 가능성이 높음
#define maxFrameDelay 100
#define maxAnimationFrame 100
#define maxInterceptCount min((screen[0] - 6) / 6, (screen[1] - 5) / 2) //표기 가능한 절편의 최대 개수

#define lld long long int
#define lf double
#define sleep Sleep
#define CLS_TEMP(buffer) FillConsoleOutputCharacter(buffer, ' ', TEMPSCREENSCALE_X * TEMPSCREENSCALE_Y, T, &dw);gotoxy(0,0)
#define CLS(buffer) FillConsoleOutputCharacter(buffer, ' ', screenSize, T, &dw);gotoxy(0,0) //CLS: 화면 초기화
#define singleKeyState(KEY) currentKeyState[KEY] && !beforeKeyState[KEY] //키가 눌렸을 때만 true, 눌린 채로 있을 경우 false
#define currentFunction functions[functionIndex]

#pragma endregion

#pragma region structs

struct ECoord { //E 표기법 형식의 좌표 표기용
	lf X[3];
	lf Y[3];
};

struct lfCoord { //double 형식의 좌표 표기용
	lf X;
	lf Y;
};

struct functionType {
	lf** polynomial; //내림차순으로 정렬된 f(x)의 각 항의 계수
	lf** derivative; //내림차순으로 정렬된 f'(x)의 각 항의 계수
	int degree; //f(x)의 차수
	lf xval[3]; //xn의 값
	struct ECoord* minCoord; //좌표평면에서 좌측 하단의 좌표
	struct ECoord* maxCoord; //좌표평면에서 우측 상단의 좌표
	bool foreGroundColor[4]; //텍스트 색상. Intensity, R, G, B 순서
	bool backGroundColor[4]; //배경 색상. Intensity, R, G, B 순서
};


#pragma endregion

#pragma region define functions

#pragma region defining system

static void gotoxy(int, int);
static void hidecursor(); //커서 숨김
static void fullscreen(HANDLE, int, int); //fullscreen. 최소 화면 크기를 입력받아 전체화면을 적용함.
static void initDoubleBuffer(); //백 버퍼 초기 설정
static void* xmalloc(size_t); //malloc 실패 시 exit, malloc 이후 주소값 return
static void* xrealloc(void*, size_t); //realloc 실패 시 exit, realloc 이후 주소값 return
static void swap(void*, void*); //두 주소값을 swap
static void clearInput(void); //입력 버퍼 초기화
static void clearKeystrokeBuffer(void); //키보드 입력 초기화
static void hprintf(wchar_t*, ...); //버퍼에 출력하는 printf
static void reloadScreen(); //화면 리로드 함수
static void resetBuffer(CHAR_INFO*); //출력 버퍼 초기화 함수
static void saveData(); //데이터 저장 함수
static void loadData(); //데이터 로드 함수

#pragma region defining threads

static DWORD WINAPI screenReloadingThread(LPVOID); //화면 자동 리로딩 스레드
static DWORD WINAPI autoSavingThread(LPVOID); //자동저장 스레드
static DWORD WINAPI eventManagingThread(LPVOID); //매 초마다 발생하는 이벤트 처리 스레드

#pragma endregion

#pragma endregion

#pragma region defining text

static CHAR_INFO wcharToUnicode(wchar_t); //wchar_t 형식의 문자를 흰색 문자, 검은색 배경의 CHAR_INFO 형식으로 변환
static CHAR_INFO wcharToColoredUnicode(wchar_t, bool[4], bool[4]); //wchar_t 형식의 문자를 특정 색상의 CHAR_INFO 형식으로 변환
static CHAR_INFO* wstringToColoredUnicode(wchar_t*, bool[4], bool[4]); //wchar_t* 형식의 문자열을 특정 색상의 CHAR_INFO* 형식으로 변환
static wchar_t* polynomialToWString(int); //lf* 형식의 다항식을 wchar_t* 형식의 문자열로 표기. 입력값은 함수의 index
static wchar_t* adjustKoreanWString(wchar_t*); //한국어가 포함된 wchar_t* 형식의 한국어 크기를 보정해 줌
static void putCharInfoToBuffer(CHAR_INFO*, COORD, CHAR_INFO); //CHAR_INFO를 출력 버퍼에 출력
static void putWCharToBuffer(CHAR_INFO*, COORD, wchar_t); //wchar_t를 출력 버퍼에 출력
static void putFileToBuffer(CHAR_INFO*, COORD, wchar_t*); //wchar_t*의 파일 위치의 txt 파일을 출력 버퍼에 출력
//txt 파일 맨 처음에는 출력할 x, y 크기가 명시되어 있어야 함
static void putStringInfoToBuffer(CHAR_INFO*, COORD, CHAR_INFO*, size_t); //CHAR_INFO* 형식의 문자열을 크기와 함께 입력받아 출력 버퍼에 출력
static void putWStringToBuffer(CHAR_INFO*, COORD, wchar_t*); //wchar_t* 형식의 문자열을 출력 버퍼에 출력

#pragma endregion

#pragma region defining polynomial calculation

static void calculateFunction(lf**, int, lf[3], lf[3]); //함수의 함숫값을 계산
static lf** polynomialDifferentiation(lf**, int); //함수를 미분하여 배열에 저장 후 해당 배열의 주소값을 return
static lf** polynomialIntegral(lf**, int); //함수를 적분하여 배열에 저장 후 해당 배열의 주소값을 return

#pragma endregion

#pragma region defining Enumber

static void EnumberAdjust(lf[3], int); //E 표기법이 적용된 수를 E-차수와 함께 입력받아 각 자리를 1 이상 10 미만의 실수로 수정
static void EnumberToDouble(lf[3], lf*); //E 표기법이 적용된 수의 크기가 double의 최댓값보다 작은 경우 double로 변환
static void EnumberToChar(lf[3], char[6]); //E 표기법이 적용된 수를 문자열로 표기
static void EnumberAdd(lf[3], lf[3], lf[3]); //E 표기법이 적용된 두 수를 더함
static void EnumberSubtract(lf[3], lf[3], lf[3]); //E 표기법이 적용된 두 수를 뺌
static void EnumberMultiply(lf[3], lf[3], lf[3]); //E 표기법이 적용된 두 수를 곱함
static void EnumberDivide(lf[3], lf[3], lf[3]); //E 표기법이 적용된 두 수를 나눔
static void EnumberAbs(lf[3], lf[3]); //E 표기법이 적용된 수를 절댓값을 씌움
static short EnumberCompare(lf[3], lf[3]); //E 표기법이 적용된 두 수를 비교하여 전자가 더 크다면 -1, 작다면 1, 같다면 0을 return
static void EnumberPower(lf[3], lld, lf[3]); //E 표기법이 적용된 수를 n제곱함

#pragma endregion

#pragma region defining print

static void printStatusScreen(int); //상태창 화면 출력
static void printSettingsScreen(int); //설정 화면 출력
static void printGraphScreen(); //그래프 화면 출력
static void printStoreScreen(); //상점 화면 출력
static void printCoordinatePlane(CHAR_INFO*); //좌표평면 출력. 그래프 화면 출력 함수에서만 쓰임
static void printBackBufferToScreen(); //백 버퍼를 현재 버퍼로 전환하여 화면에 띄움
static void printBufferAnimation(CHAR_INFO*, short, int); //화면 출력 애니메이션. 입력 값에 따라 애니메이션 방향을 조절 가능

#pragma endregion

#pragma endregion

#pragma region global variables

HANDLE hIn, hOut, hBackBuffer;
HWND hwnd;
unsigned long dw;
COORD T;
CONSOLE_SCREEN_BUFFER_INFO csbi;
int screen[2]; //화면 크기
lf* Escreen[2]; //E 표기법으로 표기된 화면 크기
CHAR_INFO* screenBuffer, * tempScreenBuffer;
char ECharExpression; //E 표기법. 'E' 또는 'e'가 될 수 있음
int currentScreenState, beforeScreenState; //현재 화면 창과 이전 화면 창

int frameDelay, animationFrame;
int interceptCount;
int settingTextCount;
int autoSavingCycle, screenReloadingCycle; //설정 항목들

HANDLE screenReloadingThreadReadyEvent, autoSavingThreadReadyEvent, eventManagingThreadReadyEvent; //스레드 생성 완료 이벤트

struct lfCoord startCoord, endCoord; //화면에 그래프를 출력하는 좌표
int statusCursorIndex, settingCursorIndex; //상태창, 설정 화면에서 현재 커서(화살표) 위치
int functionCount; //함수의 개수
wchar_t subscriptNumbers[10]; //하단에 표기되는 수 (0 ~ 9)
int functionIndex; //현재 표기되는 함수의 index
struct functionType** functions;
lf asset[3]; //현재 보유 자산

enum { //화면 상태
	LOADING = 0,
	PRE_STATUS,
	STATUS,
	PRE_GRAPH,
	GRAPH,
	PRE_STORE,
	STORE,
	PRE_SETTINGS,
	SETTINGS,
	SCREEN_COUNT
};

enum { //애니메이션 방향
	DIRECTION_UP = 0,
	DIRECTION_DOWN,
	DIRECTION_LEFT,
	DIRECTION_RIGHT
};

#pragma endregion

#pragma region main

int main(void) {

#pragma region init

	initDoubleBuffer(); //버퍼 설정

	SetConsoleOutputCP(CP_UTF8);
	system("chcp 65001 > nul"); //화면 표기 수정 (유니코드로 변환)

	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	hIn = GetStdHandle(STD_INPUT_HANDLE);
	FlushConsoleInputBuffer(hIn); //출력 화면 설정

	currentScreenState = LOADING; //시작 화면 설정

	//기타 변수 초기화 ~
	Escreen[0] = (lf*)xmalloc(sizeof(lf) * 3);
	Escreen[1] = (lf*)xmalloc(sizeof(lf) * 3);
	screenBuffer = NULL;
	tempScreenBuffer = NULL;
	settingTextCount = 6;
	statusCursorIndex = 0;
	settingCursorIndex = 0;
	beforeScreenState = STATUS;
	currentScreenState = GRAPH;
	functionIndex = 0;
	for (int i = 0; i < 10; i++) {
		subscriptNumbers[i] = 0x2080 + i;
	}
	//~ 기타 변수 초기화

	time_t current_time;
	struct tm* time_info;
	wchar_t timeString[100];
	time(&current_time);
	time_info = localtime(&current_time);
	wcsftime(timeString, sizeof(wchar_t) * 50, L"%Y%m%d_%H%M%S.txt", time_info);
	wchar_t address[100] = L"saved_data\\savefile_"; //새로운 저장 파일 주소 설정
	lstrcatW(address, timeString);
	char ch;
	FILE* source = _wfopen(savefile_txt, L"r");
	FILE* dest = _wfopen(address, L"w");
	while ((ch = fgetc(source)) != EOF) {
		fputc(ch, dest); //실행 이전 세이브 파일 새로 저장
	}

	dw;
	T = (COORD) { 0, 0 };
	hwnd = GetForegroundWindow(); //CLS용 변수 설정

	loadData(); //세이브 파일 불러오기

	CLS_TEMP(hOut);
	hprintf(L"전체화면 로딩 중...");
	fullscreen(hOut, 0, 0); //전체화면

	if (screen[0] == -1) {
		CLS_TEMP(hOut);
		hprintf(L"전체화면 로딩 실패. 일반 화면으로 진행됩니다.");
		sleep(1000);
		CLS_TEMP(hOut);
	}

	//스레드 할당 ~
	CLS_TEMP(hOut);
	hprintf(L"스레드 할당 중...");
	DWORD screenReloadingThreadId;
	DWORD autoSavingThreadId;
	screenReloadingThreadReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	autoSavingThreadReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	eventManagingThreadReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	HANDLE reloadingThreadHandle = CreateThread(NULL, 0, screenReloadingThread, NULL, 0, &screenReloadingThreadId);
	HANDLE autoSavingThreadHandle = CreateThread(NULL, 0, autoSavingThread, NULL, 0, &autoSavingThreadId);
	HANDLE eventManagingThreadHandle = CreateThread(NULL, 0, eventManagingThread, NULL, 0, &autoSavingThreadId);
	WaitForSingleObject(screenReloadingThreadReadyEvent, INFINITE);
	WaitForSingleObject(autoSavingThreadReadyEvent, INFINITE);
	WaitForSingleObject(eventManagingThreadReadyEvent, INFINITE);

	if (reloadingThreadHandle == NULL || autoSavingThreadHandle == NULL || eventManagingThreadHandle == NULL) {
		CLS_TEMP(hOut);
		hprintf(L"[ERROR] Failed To Create Thread");
		return -1;
	}
	//~ 스레드 할당

	int minScreenSize[2] = { 30, 15 };
	if (screen[0] < minScreenSize[0] || screen[1] < minScreenSize[1]) { //최소 화면 크기 미충족 시 -> 유저에게 화면 새로고침 요청
		CLS(hOut);
		hprintf(L"게임 플레이를 위해서는 최소 가로 %d, 세로 %d의 콘솔 화면 크기가 필요합니다.\n", minScreenSize[0], minScreenSize[1]);
		hprintf(L"CTRL + R을 눌러 화면 리로드가 가능합니다. 가로 %d, 세로 %d 이상으로 콘솔 화면 크기를 설정하신 뒤 화면을 리로드해 주세요.", minScreenSize[0], minScreenSize[1]);
	}

#pragma endregion

#pragma region getting key

	enum { //사용하는 키 이름 설정
		KEY_LEFT = 0, KEY_RIGHT, KEY_UP, KEY_DOWN,
		KEY_W, KEY_A, KEY_S, KEY_D,
		KEY_R, KEY_T,
		KEY_COMMA, KEY_PERIOD,
		KEY_MINUS, KEY_PLUS,
		KEY_TAB, KEY_ESC, KEY_G, KEY_B,
		KEY_CTRL,
		KEY_ENTER,
		KEY_COUNT
	};

	int KEY[KEY_COUNT] = { //각각의 키 이름을 virtual key 상의 값으로 할당
		VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN,
		'W', 'A', 'S', 'D',
		'R', 'T',
		VK_OEM_COMMA, VK_OEM_PERIOD,
		VK_OEM_MINUS, VK_OEM_PLUS,
		VK_TAB, VK_ESCAPE, 'G', 'B',
		VK_CONTROL,
		VK_RETURN
	};

	FlushConsoleInputBuffer(hIn);
	clearInput(); //이전 입력 버퍼 초기화
	bool currentKeyState[KEY_COUNT] = { false };
	bool beforeKeyState[KEY_COUNT] = { false }; //키 입력 상태 초기화

	while (true) {

		bool reload = false;
		bool scaleAdjust = false;
		lf adjustValue = 1;

		for (int i = 0; i < KEY_COUNT; i++) {
			currentKeyState[i] = (GetAsyncKeyState(KEY[i]) & 0x8000) != 0; //현재 키 입력 상태 설정
		}

		if (currentScreenState == STATUS) { //상태창 화면에서 키 입력 시:

			if (singleKeyState(KEY_UP) || singleKeyState(KEY_W)) { //커서 위로 이동
				if (statusCursorIndex > 0) {
					statusCursorIndex--;
					reload = true;
				}
			}
			if (singleKeyState(KEY_DOWN) || (singleKeyState(KEY_S) && !currentKeyState[KEY_CTRL])) { //커서 아래로 이동
				if (statusCursorIndex < functionCount - 1) {
					statusCursorIndex++;
					reload = true;
				}
			}
			if (singleKeyState(KEY_TAB)) { //이전 화면으로 전환
				if (beforeScreenState == SETTINGS) currentScreenState = PRE_SETTINGS;
				if (beforeScreenState == GRAPH) currentScreenState = PRE_GRAPH;
				if (beforeScreenState == STORE) currentScreenState = PRE_STORE;
				beforeScreenState = STATUS;
				reload = true;
			}
			if (singleKeyState(KEY_ESC)) { //설정 화면으로 전환
				beforeScreenState = STATUS;
				currentScreenState = PRE_SETTINGS;
				settingCursorIndex = 0;
				reload = true;
			}
			if (singleKeyState(KEY_G)) { //현재 커서 위치의 그래프로 이동
				beforeScreenState = STATUS;
				currentScreenState = PRE_GRAPH;
				functionIndex = statusCursorIndex;
				reload = true;
			}
			if (singleKeyState(KEY_B)) { //상점 화면으로 전환
				beforeScreenState = STATUS;
				currentScreenState = PRE_STORE;
				reload = true;
			}
			if (singleKeyState(KEY_ENTER)) { //현재 커서 위치의 그래프로 이동
				beforeScreenState = STATUS;
				currentScreenState = PRE_GRAPH;
				functionIndex = statusCursorIndex;
				reload = true;
			}
		}

		else if (currentScreenState == SETTINGS) { //설정 화면에서 키 입력 시:
			if (singleKeyState(KEY_LEFT) || singleKeyState(KEY_A)) { //설정 값에서 좌측 키(감소 키) 입력 시:
				switch (settingCursorIndex) {
				case 0: //커서 위치가 E 표기법 설정인 경우
					if (ECharExpression == 'E') ECharExpression = 'e';
					else ECharExpression = 'E';
					break;
				case 1: //커서 위치가 프레임 지연 설정인 경우
					if (frameDelay > 0) frameDelay -= 5;
					break;
				case 2: //커서 위치가 애니메이션 프레임 설정인 경우
					if (animationFrame > 0) animationFrame -= 5;
					break;
				case 3: //커서 위치가 좌표평면 개수 설정인 경우
					if (interceptCount > 0) interceptCount--;
					break;
				case 4: //커서 위치가 화면 자동 새로고침 주기 설정인 경우
					if (screenReloadingCycle > 1000) screenReloadingCycle -= 500;
					else if (screenReloadingCycle > 100) screenReloadingCycle -= 100;
					else if (screenReloadingCycle > 0) screenReloadingCycle -= 10;
					else if (screenReloadingCycle == -1) screenReloadingCycle = 5000;
					break;
				case 5: //커서 위치가 자동 저장 주기 설정인 경우
					if (autoSavingCycle > 60000) autoSavingCycle -= 60000;
					else if (autoSavingCycle > 10000) autoSavingCycle -= 10000;
					else if (autoSavingCycle > 1000) autoSavingCycle -= 1000;
					else if (autoSavingCycle == -1) autoSavingCycle = 60000 * 60;
					break;
				default: //이외의 경우 (존재하지 않음)
					break;
				}
				reload = true;
			}
			if (singleKeyState(KEY_RIGHT) || singleKeyState(KEY_D)) { //위와 같음
				switch (settingCursorIndex) {
				case 0:
					if (ECharExpression == 'E') ECharExpression = 'e';
					else ECharExpression = 'E';
					break;
				case 1:
					if (frameDelay < maxFrameDelay) frameDelay += 5;
					break;
				case 2:
					if (animationFrame < maxAnimationFrame) animationFrame += 5;
					break;
				case 3:
					if (interceptCount < maxInterceptCount) interceptCount++;
					break;
				case 4:
					if (screenReloadingCycle == -1) break;
					if (screenReloadingCycle < 100) screenReloadingCycle += 10;
					else if (screenReloadingCycle < 1000) screenReloadingCycle += 100;
					else if (screenReloadingCycle < 5000) screenReloadingCycle += 500;
					else if (screenReloadingCycle == 5000) screenReloadingCycle = -1;
					break;
				case 5:
					if (autoSavingCycle == -1) break;
					if (autoSavingCycle < 10000) autoSavingCycle += 1000;
					else if (autoSavingCycle < 60000) autoSavingCycle += 10000;
					else if (autoSavingCycle < 60000 * 60) autoSavingCycle += 60000;
					else if (autoSavingCycle == 60000 * 60) autoSavingCycle = -1;
					break;
				default:
					break;
				}
				reload = true;
			}
			if (singleKeyState(KEY_UP) || singleKeyState(KEY_W)) { //커서 위로 이동
				if (settingCursorIndex > 0) {
					settingCursorIndex--;
					reload = true;
				}
			}
			if (singleKeyState(KEY_DOWN) || (singleKeyState(KEY_S) && !currentKeyState[KEY_CTRL])) { //커서 아래로 이동
				if (settingCursorIndex < settingTextCount - 1) {
					settingCursorIndex++;
					reload = true;
				}
			}

			if (singleKeyState(KEY_ESC)) { //상태창과 같은 화면 전환
				if (beforeScreenState == GRAPH) currentScreenState = PRE_GRAPH;
				if (beforeScreenState == STATUS) currentScreenState = PRE_STATUS;
				if (beforeScreenState == STORE) currentScreenState = PRE_STORE;
				beforeScreenState = SETTINGS;
				reload = true;
			}
			if (singleKeyState(KEY_TAB)) { //상태창으로 이동
				beforeScreenState = SETTINGS;
				currentScreenState = PRE_STATUS;
				statusCursorIndex = 0;
				reload = true;
			}
			if (singleKeyState(KEY_G)) { //상태창과 같음
				beforeScreenState = SETTINGS;
				currentScreenState = PRE_GRAPH;
				reload = true;
			}
			if (singleKeyState(KEY_B)) { //상태창과 같음
				beforeScreenState = SETTINGS;
				currentScreenState = PRE_STORE;
				reload = true;
			}
		}

		else if (currentScreenState == GRAPH) { //그래프 화면에서 키 입력 시:

			if (singleKeyState(KEY_COMMA)) { //좌측 그래프로 이동
				if (functionIndex > 0) {
					functionIndex--;
					currentFunction = functions[functionIndex];
					reload = true;
				}
			}
			if (singleKeyState(KEY_PERIOD)) { //우측 그래프로 이동
				if (functionIndex < functionCount - 1) {
					functionIndex++;
					currentFunction = functions[functionIndex];
					reload = true;
				}
			}

			if (singleKeyState(KEY_T)) {
				currentFunction->minCoord->X[0] = 0.0;
				currentFunction->minCoord->X[1] = 0.0;
				currentFunction->minCoord->X[2] = 0.0;
				EnumberAdjust(currentFunction->minCoord->X, 3);
				calculateFunction(currentFunction->polynomial, currentFunction->degree, currentFunction->minCoord->X, currentFunction->minCoord->Y);
				currentFunction->maxCoord->X;
				EnumberMultiply(currentFunction->xval, (lf[]) { 2.0, 0.0, 0.0 }, currentFunction->maxCoord->X);
				calculateFunction(currentFunction->polynomial, currentFunction->degree, currentFunction->maxCoord->X, currentFunction->maxCoord->Y);
				if (EnumberCompare(currentFunction->minCoord->Y, currentFunction->maxCoord->Y) == -1) {
					lf t[3];
					memcpy(t, currentFunction->minCoord->Y, sizeof(lf) * 3);
					memcpy(currentFunction->minCoord->Y, currentFunction->maxCoord->Y, sizeof(lf) * 3);
					memcpy(currentFunction->maxCoord->Y, t, sizeof(lf) * 3);
				}
				reload = true;
			}

			lf width[3], height[3];
			lf xGap[3], yGap[3];
			lf t[3];
			EnumberSubtract(currentFunction->maxCoord->X, currentFunction->minCoord->X, width);
			EnumberSubtract(Escreen[0], (lf[]) { 1.0, 0.0, 0.0 }, t);
			EnumberDivide(width, t, xGap);
			EnumberSubtract(currentFunction->maxCoord->Y, currentFunction->minCoord->Y, height);
			EnumberSubtract(Escreen[1], (lf[]) { 1.0, 0.0, 0.0 }, t);
			EnumberDivide(height, t, yGap);

			if (currentKeyState[KEY_LEFT] || currentKeyState[KEY_A]) { //좌측으로 좌표평면 이동
				EnumberSubtract(currentFunction->minCoord->X, xGap, currentFunction->minCoord->X);
				EnumberSubtract(currentFunction->maxCoord->X, xGap, currentFunction->maxCoord->X);
				reload = true;
			}
			if (currentKeyState[KEY_RIGHT] || currentKeyState[KEY_D]) { //우측으로 좌표평면 이동
				EnumberAdd(currentFunction->minCoord->X, xGap, currentFunction->minCoord->X);
				EnumberAdd(currentFunction->maxCoord->X, xGap, currentFunction->maxCoord->X);
				reload = true;
			}
			if (currentKeyState[KEY_UP] || currentKeyState[KEY_W]) { //위로 좌표평면 이동
				EnumberAdd(currentFunction->minCoord->Y, yGap, currentFunction->minCoord->Y);
				EnumberAdd(currentFunction->maxCoord->Y, yGap, currentFunction->maxCoord->Y);
				reload = true;
			}
			if (currentKeyState[KEY_DOWN] || (currentKeyState[KEY_S] && !currentKeyState[KEY_CTRL])) { //아래로 좌표평면 이동
				EnumberSubtract(currentFunction->minCoord->Y, yGap, currentFunction->minCoord->Y);
				EnumberSubtract(currentFunction->maxCoord->Y, yGap, currentFunction->maxCoord->Y);
				reload = true;
			}

			if (currentKeyState[KEY_MINUS]) { //그래프 축소
				adjustValue = 12.0 / 10.0;
				scaleAdjust = true;
				reload = true;
			}
			if (currentKeyState[KEY_PLUS]) { //그래프 확대
				adjustValue = 10.0 / 12.0;
				scaleAdjust = true;
				reload = true;
			}

			if (singleKeyState(KEY_G)) { //위와 같은 화면전환
				if (beforeScreenState == STATUS) currentScreenState = PRE_STATUS;
				if (beforeScreenState == SETTINGS) currentScreenState = PRE_SETTINGS;
				if (beforeScreenState == STORE) currentScreenState = PRE_STORE;
				beforeScreenState = GRAPH;
				reload = true;
			}
			if (singleKeyState(KEY_TAB)) {
				beforeScreenState = GRAPH;
				currentScreenState = PRE_STATUS;
				statusCursorIndex = 0;
				reload = true;
			}
			if (singleKeyState(KEY_ESC)) {
				beforeScreenState = GRAPH;
				currentScreenState = PRE_SETTINGS;
				settingCursorIndex = 0;
				reload = true;
			}
			if (singleKeyState(KEY_B)) {
				beforeScreenState = GRAPH;
				currentScreenState = PRE_STORE;
				reload = true;
			}

			if (scaleAdjust) { //+ 또는 -로 그래프 크기 조절 시 이벤트

				lf minAdjustValue[3] = { 1.0, -8.0, 0.0 };

				lf centerX[3], halfX[3], rangeX[3], tmpX[3], absTmpX[3];
				EnumberSubtract(currentFunction->maxCoord->X, currentFunction->minCoord->X, rangeX);
				EnumberDivide(rangeX, (lf[]) { 2.0, 0.0, 0.0 }, halfX);
				EnumberAdd(currentFunction->minCoord->X, halfX, centerX);
				EnumberMultiply(halfX, (lf[]) { adjustValue, 0.0, 0.0 }, tmpX);
				EnumberAbs(tmpX, absTmpX);
				if (EnumberCompare(absTmpX, minAdjustValue) == 1) {
					if (EnumberCompare(tmpX, (lf[]) { 0.0, 0.0, 0.0 }) == 1) tmpX[0] = -1.0;
					else tmpX[0] = 1.0;
					tmpX[1] = -8.0;
					tmpX[2] = 0.0;
				}
				EnumberSubtract(centerX, tmpX, currentFunction->minCoord->X);
				EnumberAdd(centerX, tmpX, currentFunction->maxCoord->X);

				lf centerY[3], halfY[3], rangeY[3], tmpY[3], absTmpY[3];
				EnumberSubtract(currentFunction->maxCoord->Y, currentFunction->minCoord->Y, rangeY);
				EnumberDivide(rangeY, (lf[]) { 2.0, 0.0, 0.0 }, halfY);
				EnumberAdd(currentFunction->minCoord->Y, halfY, centerY);
				EnumberMultiply(halfY, (lf[]) { adjustValue, 0.0, 0.0 }, tmpY);
				EnumberAbs(tmpY, absTmpY);
				if (EnumberCompare(absTmpY, minAdjustValue) == 1) {
					if (EnumberCompare(tmpY, (lf[]) { 0.0, 0.0, 0.0 }) == 1) tmpY[0] = -1.0;
					else tmpY[0] = 1.0;
					tmpY[1] = -8.0;
					tmpY[2] = 0.0;
				}
				EnumberSubtract(centerY, tmpY, currentFunction->minCoord->Y);
				EnumberAdd(centerY, tmpY, currentFunction->maxCoord->Y);
			}

			sleep(frameDelay); //입력 간 딜레이
		}

		else if (currentScreenState == STORE) { //상점 화면 (미구현)
			if (singleKeyState(KEY_B)) {
				if (beforeScreenState == STATUS) currentScreenState = PRE_STATUS;
				if (beforeScreenState == SETTINGS) currentScreenState = PRE_SETTINGS;
				if (beforeScreenState == GRAPH) currentScreenState = PRE_GRAPH;
				beforeScreenState = STORE;
				reload = true;
			}
			if (singleKeyState(KEY_TAB)) {
				beforeScreenState = STORE;
				currentScreenState = PRE_STATUS;
				statusCursorIndex = 0;
				reload = true;
			}
			if (singleKeyState(KEY_ESC)) {
				beforeScreenState = STORE;
				currentScreenState = PRE_SETTINGS;
				settingCursorIndex = 0;
				reload = true;
			}
			if (singleKeyState(KEY_G)) {
				beforeScreenState = STORE;
				currentScreenState = PRE_GRAPH;
				reload = true;
			}
		}

		if (singleKeyState(KEY_R)) { //R키 입력 시 화면 새로고침
			reload = true;
		}

		if (singleKeyState(KEY_S) && currentKeyState[KEY_CTRL]) {
			saveData();
		}

		if (reload) { //화면 새로고침 시 이벤트
			reloadScreen();

			if (screen[0] < minScreenSize[0] || screen[1] < minScreenSize[1]) {
				CLS(hOut);
				hprintf(L"게임 플레이를 위해서는 최소 가로 %d, 세로 %d의 콘솔 화면 크기가 필요합니다.\n", minScreenSize[0], minScreenSize[1]);
				hprintf(L"CTRL + R을 눌러 화면 리로드가 가능합니다. 가로 %d, 세로 %d 이상으로 콘솔 화면 크기를 설정하신 뒤 화면을 리로드해 주세요.", minScreenSize[0], minScreenSize[1]);
			}

			else {
				switch (currentScreenState) {

				case PRE_STATUS:
				case STATUS:
					printStatusScreen(statusCursorIndex);
					break;

				case PRE_SETTINGS:
				case SETTINGS:
					printSettingsScreen(settingCursorIndex);
					break;

				case PRE_GRAPH:
				case GRAPH:
					printGraphScreen();
					break;

				case PRE_STORE:
				case STORE:
					printStoreScreen();
					break;

				default: break;
				}
			}
		}

		for (int i = 0; i < KEY_COUNT; i++) {
			beforeKeyState[i] = currentKeyState[i]; //이전 키 입력을 현재 키 입력으로 설정
		}
	}

#pragma endregion

#pragma region free

	for (int i = 0; i < functionCount; i++) {
		free(functions[i]->minCoord->X);
		free(functions[i]->minCoord->Y);
		free(functions[i]->maxCoord->X);
		free(functions[i]->maxCoord->Y);
		free(functions[i]->minCoord);
		free(functions[i]->maxCoord);
		free(functions[i]->polynomial);
		free(functions[i]->derivative);
		free(functions[i]);
	}
	free(functions);

	free(screen);
	free(screenBuffer);

	free(Escreen[0]);
	free(Escreen[1]);

	return 0;

#pragma endregion

}

#pragma endregion

#pragma region functions

#pragma region system

#pragma region function gotoxy

static void gotoxy(int x, int y) {
	COORD pos = { x, y };
	SetConsoleCursorPosition(hOut, pos);
	return;
}

#pragma endregion

#pragma region function hidecursor

static void hidecursor() {  //커서 숨김
	static CONSOLE_CURSOR_INFO info;
	info.dwSize = 100;
	info.bVisible = FALSE;
	SetConsoleCursorInfo(hOut, &info);
	SetConsoleCursorInfo(hBackBuffer, &info);
	return;
}

#pragma endregion

#pragma region function fullscreen

static void fullscreen(HANDLE handle, int minWidth, int minHeight) { //fullscreen. 최소 화면 크기를 입력받아 전체화면을 적용함.

	int x = GetSystemMetrics(SM_CXSCREEN);
	int y = GetSystemMetrics(SM_CYSCREEN);
	LONG winstyle = GetWindowLong(hwnd, GWL_STYLE);
	DWORD mode = 0;
	PCOORD pMode = 0;
	const int maxTries = 40;
	int tries = 0;

	if (!minWidth && !minHeight) {
		if (GetConsoleScreenBufferInfo(handle, &csbi)) {
			minWidth = csbi.srWindow.Right - csbi.srWindow.Left + 2;
			minHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 2;
		}
	}
	COORD newSize = { (SHORT)minWidth, (SHORT)minHeight };

	switch (ide) {
	case 1: goto VisualStudio;
	case 2: goto CodeBlocks;
	default: goto Others;
	}


VisualStudio:

	SetWindowLong(hwnd, GWL_STYLE, (winstyle | WS_POPUP | WS_MAXIMIZE) & ~WS_CAPTION & ~WS_THICKFRAME & ~WS_BORDER);

	SetWindowPos(
		hwnd,
		HWND_TOP,
		0, 0,
		x, y,
		0
	);

	goto Exit;

CodeBlocks:

	SetConsoleScreenBufferSize(handle, newSize);

	winstyle &= ~(WS_BORDER | WS_DLGFRAME | WS_CAPTION | WS_THICKFRAME);
	winstyle |= WS_POPUP | WS_VISIBLE;
	SetWindowLong(hwnd, GWL_STYLE, winstyle);

	SetWindowPos(
		hwnd,
		HWND_TOPMOST,
		0, 0,
		x, y,
		SWP_FRAMECHANGED | SWP_SHOWWINDOW
	);

	goto Exit;

Others:

	if (!SetConsoleDisplayMode(handle, CONSOLE_FULLSCREEN_MODE, pMode)) {
		winstyle &= ~(WS_BORDER | WS_DLGFRAME | WS_CAPTION | WS_THICKFRAME);
		winstyle |= WS_POPUP | WS_VISIBLE;
		SetWindowLong(hwnd, GWL_STYLE, winstyle);
		SetWindowPos(
			hwnd,
			HWND_TOPMOST,
			0, 0,
			x, y,
			SWP_FRAMECHANGED | SWP_SHOWWINDOW
		);
	}

	goto Exit;

Exit:

	while (tries++ < maxTries) {
		if (GetConsoleScreenBufferInfo(handle, &csbi)) {
			int width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
			int height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
			if (width < minWidth || height < minHeight) sleep(50);
			else {
				screen[0] = width;
				screen[1] = height;
				return;
			}
		}
	}

	screen[0] = -1;
	screen[1] = -1;
	return;
}

#pragma endregion

#pragma region function initDoubleBuffer

static void initDoubleBuffer() { //백 버퍼 초기 설정

	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	hBackBuffer = CreateConsoleScreenBuffer(
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		CONSOLE_TEXTMODE_BUFFER,
		NULL
	);
	if (hBackBuffer == INVALID_HANDLE_VALUE) {
		CLS(hOut);
		hprintf(L"[ERROR] Invalid Handle Value\n");
		exit(EXIT_FAILURE);
	}

	hidecursor();

	return;
}

#pragma endregion

#pragma region function xmalloc

static void* xmalloc(size_t bytes) { //malloc 실패 시 exit, malloc 이후 주소값 return

	void* p = malloc(bytes);
	if (!p) {
		CLS(hOut);
		hprintf(L"[ERROR] Memory Allocation Failed\n");
		exit(EXIT_FAILURE);
	}
	return p;
}

#pragma endregion

#pragma region function xrealloc

static void* xrealloc(void* variable, size_t bytes) { //realloc 실패 시 exit, realloc 이후 주소값 return

	void* p = realloc(variable, bytes);
	if (p == NULL) {
		CLS(hOut);
		hprintf(L"[ERROR] Memory Reallocation Failed\n");
		exit(EXIT_FAILURE);
	}
	return p;
}

#pragma endregion

#pragma region function swap

static void swap(void* a, void* b) { //두 주소값을 swap
	void* t = *(void**)a;
	*(void**)a = *(void**)b;
	*(void**)b = t;
	return;
}

#pragma endregion

#pragma region function clearInput

static void clearInput(void) { //입력 버퍼 초기화
	int c;
	while ((c = getchar()) != '\n' && c != EOF);
	return;
}

#pragma endregion

#pragma region function clearKeystrokeBuffer

static void clearKeystrokeBuffer(void) { //키보드 입력 초기화
	while (_kbhit()) {
		char ch = _getch();
	}
}

#pragma endregion

#pragma region function hprintf

static void hprintf(wchar_t* fmt, ...) { //버퍼에 출력하는 printf

	va_list args;
	va_start(args, fmt);

	int needed = _vscwprintf(fmt, args);
	if (needed < 0) {
		va_end(args);
		return;
	}

	wchar_t* buffer = (wchar_t*)malloc((needed + 1) * sizeof(wchar_t));
	if (!buffer) {
		va_end(args);
		return;
	}

	va_start(args, fmt);
	vswprintf(buffer, needed + 1, fmt, args);
	va_end(args);

	DWORD written;
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteConsoleW(hOut, buffer, (DWORD)wcslen(buffer), &written, NULL);

	free(buffer);
	return;
}

#pragma endregion

#pragma region function reloadScreen

static void reloadScreen() { //화면 리로드 함수

	int newScreen[2] = { 0, 0 };
	if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
		newScreen[0] = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		newScreen[1] = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
		if (screen[0] == newScreen[0] && screen[1] == newScreen[1] && screenBuffer != NULL) return;
		//현재 화면 크기와 같아 화면 크기 수정이 필요없는 경우 return

		screen[0] = newScreen[0];
		screen[1] = newScreen[1];
		endCoord.X = screen[0];
		endCoord.Y = screen[1] - 1;
		Escreen[0][0] = screen[0];
		Escreen[0][1] = 0.0;
		Escreen[0][2] = 0.0;
		Escreen[1][0] = screen[1];
		Escreen[1][1] = 0.0;
		Escreen[1][2] = 0.0;
		EnumberAdjust(Escreen[0], 3);
		EnumberAdjust(Escreen[1], 3);

		if (!GetConsoleScreenBufferInfo(hBackBuffer, &csbi)) {
			CLS(hOut);
			hprintf(L"Failed To Get Back-Buffer Window Info");
			exit(EXIT_FAILURE);
		}
		SMALL_RECT newWindowRect = { 0, 0, (SHORT)(screen[0] - 1), (SHORT)(screen[1] - 1) }; //백 버퍼 재설정
		if (!SetConsoleWindowInfo(hBackBuffer, TRUE, &newWindowRect)) {
			CLS(hOut);
			hprintf(L"[ERROR] Failed To Edit Back-Buffer Window Info");
			exit(EXIT_FAILURE);
		}
		if (!SetConsoleScreenBufferSize(hBackBuffer, (COORD) { screen[0], screen[1] })) {
			CLS(hOut);
			hprintf(L"[ERROR] Failed To Resize Back-Buffer Window");
			exit(EXIT_FAILURE);
		}
		if (screenBuffer == NULL) screenBuffer = (CHAR_INFO*)xmalloc(sizeof(CHAR_INFO) * screenSize); //백 버퍼 텍스트 배열 재설정
		else screenBuffer = xrealloc(screenBuffer, sizeof(CHAR_INFO) * screenSize);
		resetBuffer(screenBuffer);
		if (tempScreenBuffer == NULL) tempScreenBuffer = (CHAR_INFO*)xmalloc(sizeof(CHAR_INFO) * screenSize); //임시 백 버퍼 텍스트 배열 재설정
		else tempScreenBuffer = xrealloc(tempScreenBuffer, sizeof(CHAR_INFO) * screenSize);
		resetBuffer(tempScreenBuffer);
	}

	return;
}

#pragma endregion

#pragma region function resetBuffer

static void resetBuffer(CHAR_INFO* buffer) { //출력 버퍼 초기화 함수
	for (int y = 0; y < screen[1]; y++) {
		for (int x = 0; x < screen[0]; x++) {
			buffer[y * screen[0] + x].Char.UnicodeChar = L' ';
			buffer[y * screen[0] + x].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
		}
	}
	return;
}

#pragma endregion

#pragma region function saveData

static void saveData() { //데이터 저장 함수

	FILE* fp = _wfopen(savefile_txt, L"w");
	if (fp == NULL) {
		CLS_TEMP(hOut);
		hprintf(L"[ERROR] Failed To Open File: %ls", savefile_txt);
		exit(EXIT_FAILURE);
	}

	fprintf(fp, "%d\n\n", functionCount);

	for (int index = 0; index < functionCount; index++) {

		fprintf(fp, "%d ", functions[index]->degree);

		for (int i = 0; i < functions[index]->degree; i++) {
			fprintf(fp, "%lf %lf %lf ", functions[index]->polynomial[i][0], functions[index]->polynomial[i][1], functions[index]->polynomial[i][2]);
		}
		fprintf(fp, "\n");

		for (int i = 0; i < 4; i++) {
			fprintf(fp, "%d ", functions[index]->foreGroundColor[i]);
		}
		for (int i = 0; i < 4; i++) {
			fprintf(fp, "%d ", functions[index]->backGroundColor[i]);
		}
		fprintf(fp, "\n");

		fprintf(fp, "%lf %lf %lf\n\n", functions[index]->xval[0], functions[index]->xval[1], functions[index]->xval[2]);
	}

	fprintf(fp, "%lf %lf %lf\n", asset[0], asset[1], asset[2]);
	fprintf(fp, "%c %d %d %d\n", ECharExpression, frameDelay, interceptCount, animationFrame);
	fprintf(fp, "%d %d\n", autoSavingCycle, screenReloadingCycle);

	fclose(fp);
	return;
}

#pragma endregion

#pragma region loadData

static void loadData() { //데이터 로드 함수

	FILE* fp = _wfopen(savefile_txt, L"r");
	if (fp == NULL) {
		CLS_TEMP(hOut);
		hprintf(L"[ERROR] Failed To Open File: %ls", savefile_txt);
		exit(EXIT_FAILURE);
	}

	if (fscanf(fp, "%d", &functionCount) != 1) {
		CLS_TEMP(hOut);
		hprintf(L"[ERROR] Failed To Read File: %ls", savefile_txt);
		exit(EXIT_FAILURE);
	}

	functions = (struct functionType**)xmalloc(sizeof(struct functionType*) * functionCount);

	for (int index = 0; index < functionCount; index++) {

		int n;
		if (fscanf(fp, "%d", &n) != 1) {
			CLS_TEMP(hOut);
			hprintf(L"[ERROR] Failed To Read File: %ls", savefile_txt);
			exit(EXIT_FAILURE);
		}

		functions[index] = (struct functionType*)xmalloc(sizeof(struct functionType));
		functions[index]->degree = n;

		functions[index]->polynomial = (lf**)xmalloc(sizeof(lf*) * n);

		for (int i = 0; i < n; i++) {
			functions[index]->polynomial[n - i - 1] = (lf*)xmalloc(sizeof(lf) * 3);
			if (fscanf(fp, "%lf %lf %lf", &functions[index]->polynomial[n - i - 1][0], \
				&functions[index]->polynomial[n - i - 1][1], &functions[index]->polynomial[n - i - 1][2]) != 3) {
				CLS_TEMP(hOut);
				hprintf(L"[ERROR] Failed To Read File: %ls", savefile_txt);
				exit(EXIT_FAILURE);
			}
			EnumberAdjust(functions[index]->polynomial[n - i - 1], 3);
		}

		for (int i = 0; i < 4; i++) {
			int t;
			if (fscanf(fp, "%d", &t) != 1) {
				CLS_TEMP(hOut);
				hprintf(L"[ERROR] Failed To Read File: %ls", savefile_txt);
				exit(EXIT_FAILURE);
			}
			functions[index]->foreGroundColor[i] = t;
		}
		for (int i = 0; i < 4; i++) {
			int t;
			if (fscanf(fp, "%d", &t) != 1) {
				CLS_TEMP(hOut);
				hprintf(L"[ERROR] Failed To Read File: %ls", savefile_txt);
				exit(EXIT_FAILURE);
			}
			functions[index]->backGroundColor[i] = t;
		}

		functions[index]->derivative = polynomialDifferentiation(functions[index]->polynomial, functions[index]->degree);

		functions[index]->minCoord = (struct ECoord*)xmalloc(sizeof(struct ECoord));
		functions[index]->maxCoord = (struct ECoord*)xmalloc(sizeof(struct ECoord));

		if (fscanf(fp, "%lf %lf %lf", &functions[index]->xval[0], &functions[index]->xval[1], &functions[index]->xval[2]) != 3) {
			CLS_TEMP(hOut);
			hprintf(L"[ERROR] Failed To Read File: %ls", savefile_txt);
			exit(EXIT_FAILURE);
		}

		functions[index]->minCoord->X[0] = 0.0;
		functions[index]->minCoord->X[1] = 0.0;
		functions[index]->minCoord->X[2] = 0.0;
		EnumberAdjust(functions[index]->minCoord->X, 3);
		calculateFunction(functions[index]->polynomial, n, functions[index]->minCoord->X, functions[index]->minCoord->Y);

		functions[index]->maxCoord->X;
		EnumberMultiply(functions[index]->xval, (lf[]) { 2.0, 0.0, 0.0 }, functions[index]->maxCoord->X);
		calculateFunction(functions[index]->polynomial, n, functions[index]->maxCoord->X, functions[index]->maxCoord->Y);

		if (EnumberCompare(functions[index]->minCoord->Y, functions[index]->maxCoord->Y) == -1) {
			lf t[3];
			memcpy(t, functions[index]->minCoord->Y, sizeof(lf) * 3);
			memcpy(functions[index]->minCoord->Y, functions[index]->maxCoord->Y, sizeof(lf) * 3);
			memcpy(functions[index]->maxCoord->Y, t, sizeof(lf) * 3);
		}
	}

	if (fscanf(fp, "%lf %lf %lf", &asset[0], &asset[1], &asset[2]) != 3) {
		CLS_TEMP(hOut);
		hprintf(L"[ERROR] Failed To Read File: %ls", savefile_txt);
		exit(EXIT_FAILURE);
	}

	if (fscanf(fp, " %c %d %d %d", &ECharExpression, &frameDelay, &interceptCount, &animationFrame) != 4) {
		CLS_TEMP(hOut);
		hprintf(L"[ERROR] Failed To Read File: %ls", savefile_txt);
		exit(EXIT_FAILURE);
	}

	if (fscanf(fp, "%d %d", &autoSavingCycle, &screenReloadingCycle) != 2) {
		CLS_TEMP(hOut);
		hprintf(L"[ERROR] Failed To Read File: %ls", savefile_txt);
		exit(EXIT_FAILURE);
	}

	fclose(fp);

	return;
}

#pragma endregion

#pragma region function screenReloadingThread

static DWORD WINAPI screenReloadingThread(LPVOID param) { //화면 자동 리로딩 스레드

	UNREFERENCED_PARAMETER(param);
	bool event = false;
	while (true) {
		if (currentScreenState == STATUS || currentScreenState == SETTINGS || currentScreenState == GRAPH || currentScreenState == STORE) {
			reloadScreen();
			switch (currentScreenState) {

			case PRE_STATUS:
			case STATUS:
				printStatusScreen(statusCursorIndex);
				break;

			case PRE_SETTINGS:
			case SETTINGS:
				printSettingsScreen(settingCursorIndex);
				break;

			case PRE_GRAPH:
			case GRAPH:
				printGraphScreen();
				break;

			case PRE_STORE:
			case STORE:
				printStoreScreen();
				break;

			default: break;
			}

			if (!event) {
				SetEvent(screenReloadingThreadReadyEvent);
				event = true;
			}
		}
		while (screenReloadingCycle == -1) {
			sleep(5000);
		}
		if (screenReloadingCycle != -1) sleep(screenReloadingCycle);
	}
	return 0;
}

#pragma endregion

#pragma region function autoSavingThread

static DWORD WINAPI autoSavingThread(LPVOID param) { //자동저장 스레드
	UNREFERENCED_PARAMETER(param);
	SetEvent(autoSavingThreadReadyEvent);
	while (true) {
		sleep(autoSavingCycle);
		saveData();
	}
	return 0;
}

#pragma endregion

#pragma region function eventManagingThread

static DWORD WINAPI eventManagingThread(LPVOID param) { //매 초마다 발생하는 이벤트 처리 스레드
	UNREFERENCED_PARAMETER(param);
	SetEvent(eventManagingThreadReadyEvent);
	while (true) {
		sleep(1000);
		for (int i = 0; i < functionCount; i++) {
			lf value[3];
			calculateFunction(functions[i]->polynomial, functions[i]->degree, functions[i]->xval, value);
			EnumberAdd(asset, value, asset);
		}
	}
	return 0;
}

#pragma endregion

#pragma endregion

#pragma region text

#pragma region function wcharToUnicode

static CHAR_INFO wcharToUnicode(wchar_t ch) { //wchar_t 형식의 문자를 흰색 문자, 검은색 배경의 CHAR_INFO 형식으로 변환
	CHAR_INFO newchar;
	newchar.Char.UnicodeChar = ch;
	newchar.Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
	return newchar;
}

#pragma endregion

#pragma region function wcharToColoredUnicode

static CHAR_INFO wcharToColoredUnicode(wchar_t ch, bool foreGround[4], bool backGround[4]) { //wchar_t 형식의 문자를 특정 색상의 CHAR_INFO 형식으로 변환
	CHAR_INFO newchar;
	newchar.Char.UnicodeChar = ch;
	newchar.Attributes = 0x0000;
	for (int i = 0; i < 4; i++) {
		if (foreGround[i]) newchar.Attributes |= (1 << (3 - i));
		if (backGround[i]) newchar.Attributes |= (1 << (3 - i)) << 4;
	}
	return newchar;
}

#pragma endregion

#pragma region function WstringToColoredUnicode

static CHAR_INFO* wstringToColoredUnicode(wchar_t* ch, bool foreGround[4], bool backGround[4]) { //wchar_t* 형식의 문자열을 특정 색상의 CHAR_INFO* 형식으로 변환
	int len = lstrlenW(ch);
	CHAR_INFO* newstring = (CHAR_INFO*)xmalloc(sizeof(CHAR_INFO) * len);
	for (int i = 0; i < len; i++) {
		newstring[i].Char.UnicodeChar = ch[i];
		newstring[i].Attributes = 0x0000;
		for (int j = 0; j < 4; j++) {
			if (foreGround[j]) newstring[i].Attributes |= (1 << (3 - j));
			if (backGround[j]) newstring[i].Attributes |= (1 << (3 - j)) << 4;
		}
	}
	return newstring;
}

#pragma endregion

#pragma region function polynomialToWString

static wchar_t* polynomialToWString(int index) { //lf* 형식의 다항식을 wchar_t* 형식의 문자열로 표기. 입력값은 함수의 index

	wchar_t* formula = (wchar_t*)xmalloc(sizeof(wchar_t) * screen[0]);
	wchar_t text[20];
	if (index + 1 < 10) swprintf(formula, TEMPSCREENSCALE_X, L"f%lc(x) = ", subscriptNumbers[index + 1]);
	else swprintf(formula, TEMPSCREENSCALE_X, L"f%lc%lc(x) = ", subscriptNumbers[((int)index + 1) / 10], subscriptNumbers[(index + 1) % 10]);
	size_t len = lstrlenW(formula);
	for (int i = 0; i < functions[index]->degree; i++) {
		char coefficient[6];
		EnumberToChar(functions[index]->polynomial[functions[index]->degree - i - 1], coefficient);
		if (functions[index]->degree - i - 1 > 1) swprintf(text, 20, L"(%hs)x^%d", coefficient, functions[index]->degree - i - 1);
		else if (functions[index]->degree - i - 1 == 1) swprintf(text, 20, L"(%hs)x", coefficient);
		else swprintf(text, 20, L"(%hs)", coefficient);
		len += lstrlenW(text);
		if (len >= screen[0]) return formula;
		formula = lstrcatW(formula, text);
		if (i != functions[index]->degree - 1) {
			len += lstrlenW(L" + ");
			if (len >= screen[0]) return formula;
			formula = lstrcatW(formula, L" + ");
		}
	}

	return formula;
}

#pragma endregion

#pragma region function adjustKoreanWString

static wchar_t* adjustKoreanWString(wchar_t* str) { //한국어가 포함된 wchar_t* 형식의 한국어 크기를 보정해 줌

	int len = (int)wcslen(str);
	int newSize = 0;
	for (int i = 0; i < len; i++) {
		if (str[i] >= 0xAC00 && str[i] <= 0xD7AF) newSize++;
		newSize++;
	}
	wchar_t* newStr = (wchar_t*)xmalloc(sizeof(wchar_t) * (newSize + 1));

	for (int i = 0, index = 0; i < len; i++, index++) {
		if (str[i] >= 0xAC00 && str[i] <= 0xD7AF) {
			newStr[index++] = (wchar_t)' ';
		}
		newStr[index] = str[i];
	}
	newStr[newSize] = '\0';

	return newStr;
}

#pragma endregion

#pragma region function putCharInfoToBuffer

static void putCharInfoToBuffer(CHAR_INFO* buffer, COORD coord, CHAR_INFO ci) { //CHAR_INFO를 출력 버퍼에 출력
	if (coord.X < 0 || coord.X >= screen[0] || coord.Y < 0 || coord.Y >= screen[1]) return;
	buffer[coord.Y * screen[0] + coord.X] = ci;
	return;
}

#pragma endregion

#pragma region function putWCharToBuffer

static void putWCharToBuffer(CHAR_INFO* buffer, COORD coord, wchar_t wc) { //wchar_t를 출력 버퍼에 출력
	if (coord.X < 0 || coord.X >= screen[0] || coord.Y < 0 || coord.Y >= screen[1]) return;
	CHAR_INFO ci = wcharToUnicode(wc);
	buffer[coord.Y * screen[0] + coord.X] = ci;
	return;
}

#pragma endregion

#pragma region function putFileToBuffer

static void putFileToBuffer(CHAR_INFO* buffer, COORD coord, wchar_t* file_address) { //wchar_t*의 파일 위치의 txt 파일을 출력 버퍼에 출력
	//txt 파일 맨 처음에는 출력할 x, y 크기가 명시되어 있어야 함

	FILE* fp = _wfopen(file_address, L"r, ccs=UTF-8");
	if (fp == NULL) {
		CLS(hOut);
		hprintf(L"[ERROR] Failed To Open File: %ls", file_address);
		exit(EXIT_FAILURE);
	}
	int width, height;
	if (fwscanf(fp, L"%d %d", &width, &height) != 2) {
		CLS(hOut);
		hprintf(L"[ERROR] Failed To Read File: %ls", file_address);
		fclose(fp);
		exit(EXIT_FAILURE);
	}
	fgetwc(fp);

	for (lld i = 0; i < height; i++) {
		for (lld j = 0; j < width; j++) {
			wint_t wc = fgetwc(fp);
			if (wc == WEOF) {
				fclose(fp);
				return;
			}
			if (wc == L'\n') break;
			COORD newCoord = { (short)(coord.X + j), (short)(coord.Y + i) };
			putWCharToBuffer(buffer, newCoord, (wchar_t)wc);
		}

		wint_t tail = fgetwc(fp);
		if (tail == WEOF) {
			break;
		}
		if (tail != L'\n') {
			ungetwc(tail, fp);
		}
	}

	fclose(fp);
	return;
}

#pragma endregion

#pragma region putStringInfoToBuffer

static void putStringInfoToBuffer(CHAR_INFO* buffer, COORD start, CHAR_INFO* str, size_t len) { //CHAR_INFO* 형식의 문자열을 크기와 함께 입력받아 출력 버퍼에 출력

	int px = start.X;
	int py = start.Y;

	for (int i = 0; i < len; i++) {

		if (px < 0 || px >= screen[0] || py < 0 || py >= screen[1]) continue;
		buffer[py * screen[0] + px] = str[i];

		px++;
	}

	return;
}

#pragma endregion
#pragma region function putWStringToBuffer

static void putWStringToBuffer(CHAR_INFO* buffer, COORD start, wchar_t* str) { //wchar_t* 형식의 문자열을 출력 버퍼에 출력

	int len = (int)wcslen(str);

	int px = start.X;
	int py = start.Y;

	for (int i = 0; i < len; i++) {

		if (px < 0 || px >= screen[0] || py < 0 || py >= screen[1]) continue;

		wchar_t wc = str[i];
		if (wc == L'\0') break;
		CHAR_INFO ci = wcharToUnicode(wc);
		buffer[py * screen[0] + px] = ci;

		px++;
	}

	return;
}

#pragma endregion

#pragma endregion

#pragma region polynomial calculation

#pragma region function polynomialDifferentiation

static lf** polynomialDifferentiation(lf** function, int degree) { //함수를 미분하여 배열에 저장 후 해당 배열의 주소값을 return

	lf** fprime = (lf**)xmalloc(sizeof(lf*) * ((lld)degree - 1));

	for (int i = 1; i < degree; i++) {
		fprime[i - 1] = (lf*)xmalloc(sizeof(lf) * 3);
		EnumberMultiply(function[i], (lf[]) { i, 0.0, 0.0 }, fprime[i - 1]);
	}

	return fprime;
}

#pragma endregion

#pragma region function polynomialIntegral

static lf** polynomialIntegral(lf** function, int degree) { //함수를 적분하여 배열에 저장 후 해당 배열의 주소값을 return

	lf** F = (lf**)xmalloc(sizeof(lf*) * ((lld)degree + 1));

	for (int i = 0; i < degree; i++) {
		F[i] = (lf*)xmalloc(sizeof(lf) * 3);
		EnumberDivide(function[i], (lf[]) { i + 1, 0.0, 0.0 }, F[i]);
	}

	return F;
}

#pragma endregion

#pragma region function calculateFunction

static void calculateFunction(lf** function, int degree, lf n[3], lf result[3]) { //함수의 함숫값을 계산

	result[0] = 0.0;
	result[1] = 0.0;
	result[2] = 0.0;

	lf val[3];
	for (int i = 0; i < degree; i++) {
		EnumberPower(n, i, val);
		EnumberMultiply(val, function[i], val);
		EnumberAdd(result, val, result);
	}

	return;
}

#pragma endregion

#pragma endregion

#pragma region Enumber

#pragma region function EnumberAdjust

static void EnumberAdjust(lf n[3], int digit) { //E 표기법이 적용된 수를 E-차수와 함께 입력받아 각 자리를 1 이상 10 미만의 실수로 수정

	for (int i = 0; i < digit - 1; i++) {
		if (n[i] == 0) n[i + 1] = 0;
	}

	lld logval;

	for (int i = 0; i < digit - 1; i++) {
		logval = 0;
		if (n[i]) logval = (lld)(log10(fabs(n[i])));
		n[i + 1] += logval;
		n[i] /= pow(10.0, (lf)logval);
	}

	return;
}

#pragma endregion

#pragma region function EnumberToDouble

static void EnumberToDouble(lf n[3], lf* result) { //E 표기법이 적용된 수의 크기가 double의 최댓값보다 작은 경우 double로 변환

	lf cn[3];
	memcpy(cn, n, sizeof(cn));

	cn[1] *= pow(10, cn[2]);
	if (cn[1] > 308) *result = cn[0] < 0 ? LF_MIN : LF_MAX;
	else if (cn[1] == 308 && fabs(cn[0]) > 1.79) *result = cn[0] < 0 ? LF_MIN : LF_MAX;
	else *result = cn[0] * pow(10, cn[1]);

	return;
}

#pragma endregion

#pragma region function EnumberToChar

static void EnumberToChar(lf n[3], char result[6]) { //E 표기법이 적용된 수를 문자열로 표기

	lf cn[3];
	memcpy(cn, n, sizeof(cn));
	char sign = (EnumberCompare(cn, (lf[]){ 0.0,0.0,0.0 }) == 1) ? '-' : '+';
	if (sign == '-') EnumberMultiply(cn, (lf[]) { -1.0, 0.0, 0.0 }, cn);

	if (EnumberCompare(cn, (lf[]) { 9.9995, 3.0, 0.0 }) == 1) {
		lf value;
		EnumberToDouble(cn, &value);
		if (value < 10.0) {
			snprintf(result, 6, "%c%.2f", sign, value);
		}
		else if (value < 100.0) {
			snprintf(result, 6, "%c%.1f", sign, value);
		}
		else snprintf(result, 6, "%c%04d", sign, min((int)round(value), 9999));
	}
	else if (EnumberCompare(cn, (lf[]) { 9.5, 9.9, 1 }) == 1) {
		cn[1] *= pow(10, cn[2]);
		lf error = cn[1] - (int)round(cn[1]);
		cn[0] *= pow(10, error);
		if (cn[0] < 1.0) {
			cn[0] *= 10;
			cn[1]--;
		}
		snprintf(result, 6, "%c%01d%c%02d", sign, min((int)round(cn[0]), 9), ECharExpression, min((int)round(cn[1]), 99));
	}
	else if (EnumberCompare(cn, ENUMBER_MAX) >= 0) {
		cn[2] += log10(cn[1]);
		snprintf(result, 6, "%c%01d%c%c%01d", sign, min((int)round(cn[0]), 9), ECharExpression, ECharExpression, min((int)round(cn[2]), 9));
	}
	else {
		snprintf(result, 6, "%c9EE9", sign);
	}

	return;
}

#pragma endregion

#pragma region function EnumberAdd

static void EnumberAdd(lf a[3], lf b[3], lf result[3]) { //E 표기법이 적용된 두 수를 더함

	lf ca[3], cb[3];
	memcpy(ca, a, sizeof(ca));
	memcpy(cb, b, sizeof(cb));

	ca[1] = ca[1] * pow(10, ca[2]);
	cb[1] = cb[1] * pow(10, cb[2]);
	ca[2] = 0;
	cb[2] = 0;

	if (ca[1] < cb[1]) {
		ca[0] *= pow(10.0, ca[1] - cb[1]);
		cb[0] += ca[0];
		EnumberAdjust(cb, 3);
		result[0] = cb[0];
		result[1] = cb[1];
		result[2] = cb[2];
	}
	else {
		cb[0] *= pow(10.0, cb[1] - ca[1]);
		ca[0] += cb[0];
		EnumberAdjust(ca, 3);
		result[0] = ca[0];
		result[1] = ca[1];
		result[2] = ca[2];
	}

	return;
}

#pragma endregion

#pragma region function EnumberSubtract

static void EnumberSubtract(lf a[3], lf b[3], lf result[3]) { //E 표기법이 적용된 두 수를 뺌

	lf ca[3], cb[3];
	memcpy(ca, a, sizeof(ca));
	memcpy(cb, b, sizeof(cb));

	ca[1] = ca[1] * pow(10, ca[2]);
	cb[1] = cb[1] * pow(10, cb[2]);
	ca[2] = 0;
	cb[2] = 0;

	if (ca[1] < cb[1]) ca[0] /= pow(10.0, cb[1] - ca[1]);
	else cb[0] /= pow(10.0, ca[1] - cb[1]);

	ca[0] -= cb[0];
	result[0] = ca[0];
	result[1] = max(ca[1], cb[1]);
	result[2] = 0;
	EnumberAdjust(result, 3);

	return;
}

#pragma endregion

#pragma region function EnumberMultiply

static void EnumberMultiply(lf a[3], lf b[3], lf result[3]) { //E 표기법이 적용된 두 수를 곱함

	result[0] = a[0] * b[0];
	result[1] = (a[1] * pow(10, a[2])) + (b[1] * pow(10, b[2]));
	result[2] = 0;

	EnumberAdjust(result, 3);
	return;
}

#pragma endregion

#pragma region function EnumberDivide

static void EnumberDivide(lf a[3], lf b[3], lf result[3]) { //E 표기법이 적용된 두 수를 나눔

	if (b[0] == 0) {
		result[1] = 9.0;
		result[2] = 9.0;
		if (a[0] < 0) result[0] = -9.0;
		else result[0] = 9.0;
		return;
	}

	result[0] = a[0] / b[0];
	result[1] = (a[1] * pow(10, a[2])) - (b[1] * pow(10, b[2]));
	result[2] = 0;

	EnumberAdjust(result, 3);
	return;
}

#pragma endregion

#pragma region function EnumberAbs

static void EnumberAbs(lf n[3], lf result[3]) { //E 표기법이 적용된 수를 절댓값을 씌움
	if (EnumberCompare(n, (lf[]) { 0.0, 0.0, 0.0 }) == 1) EnumberMultiply(n, (lf[]) { -1.0, 0.0, 0.0 }, result);
	else memcpy(result, n, sizeof(lf) * 3);
	return;
}

#pragma endregion

#pragma region function EnumberCompare

static short EnumberCompare(lf a[3], lf b[3]) { //E 표기법이 적용된 두 수를 비교하여 전자가 더 크다면 -1, 작다면 1, 같다면 0을 return

	lf ca[3], cb[3];
	memcpy(ca, a, sizeof(ca));
	memcpy(cb, b, sizeof(cb));

	ca[1] = ca[1] * pow(10, ca[2]);
	cb[1] = cb[1] * pow(10, cb[2]);
	ca[2] = 0;
	cb[2] = 0;

	if (ca[1] < cb[1]) ca[0] /= pow(10.0, cb[1] - ca[1]);
	else cb[0] /= pow(10.0, ca[1] - cb[1]);

	if (ca[0] < cb[0]) return 1;
	else if (ca[0] > cb[0]) return -1;
	else return 0;
}

#pragma endregion

#pragma region function EnumberPower

static void EnumberPower(lf a[3], lld n, lf result[3]) { //E 표기법이 적용된 수를 n제곱함

	if (n == 1) {
		result[0] = a[0];
		result[1] = a[1];
		result[2] = a[2];
		return;
	}
	if (n == 0) {
		result[0] = 1.0;
		result[1] = 0.0;
		result[2] = 0.0;
		return;
	}

	EnumberPower(a, (lld)n / 2, result);
	EnumberMultiply(result, result, result);
	if (n % 2) EnumberMultiply(result, a, result);

	return;
}

#pragma endregion

#pragma endregion

#pragma region print

#pragma region function printStatusScreen

static void printStatusScreen(int cursorIndex) { //상태창 화면 출력

	if (!animationFrame) currentScreenState = STATUS;

	CHAR_INFO* buffer = screenBuffer;
	if (currentScreenState == PRE_STATUS) buffer = tempScreenBuffer;

	resetBuffer(buffer);

	//화면 틀 출력 ~
	for (short y = 0; y < screen[1] - 1; y++) {
		putWCharToBuffer(buffer, (COORD) { (short)0, y }, (wchar_t)L'│');
		putWCharToBuffer(buffer, (COORD) { (short)screen[0] - 1, y }, (wchar_t)L'│');
	}
	for (short x = 0; x < screen[0]; x++) {
		putWCharToBuffer(buffer, (COORD) { x, (short)0 }, (wchar_t)L'─');
		putWCharToBuffer(buffer, (COORD) { x, (short)screen[1] - 2 }, (wchar_t)L'─');
	}
	putWCharToBuffer(buffer, (COORD) { (short)0, (short)0 }, (wchar_t)L'┌');
	putWCharToBuffer(buffer, (COORD) { (short)screen[0] - 1, (short)0 }, (wchar_t)L'┐');
	putWCharToBuffer(buffer, (COORD) { (short)0, (short)screen[1] - 2 }, (wchar_t)L'└');
	putWCharToBuffer(buffer, (COORD) { (short)screen[0] - 1, (short)screen[1] - 2 }, (wchar_t)L'┘');

	for (short y = 0; y < 5; y++) {
		putWCharToBuffer(buffer, (COORD) { (short)screen[0] / 2, y }, (wchar_t)L'│');
	}
	for (short x = 0; x < screen[0]; x++) {
		putWCharToBuffer(buffer, (COORD) { x, (short)5 }, (wchar_t)L'─');
	}
	putWCharToBuffer(buffer, (COORD) { (short)screen[0] / 2, (short)5 }, (wchar_t)L'┴');
	putWCharToBuffer(buffer, (COORD) { (short)screen[0] / 2, (short)0 }, (wchar_t)L'┬');
	putWCharToBuffer(buffer, (COORD) { (short)0, (short)5 }, (wchar_t)L'├');
	putWCharToBuffer(buffer, (COORD) { (short)screen[0] - 1, (short)5 }, (wchar_t)L'┤');
	//~ 화면 틀 출력

	wchar_t text[20];
	char EnumberChar[6];
	EnumberToChar(asset, EnumberChar);
	swprintf(text, 20, L"y = %hs", EnumberChar);
	putWStringToBuffer(buffer, (COORD) { (short)2, 1 }, text);

	//함수 공식을 문자열로 배열에 저장 ~
	wchar_t** formulas = (wchar_t**)xmalloc(sizeof(wchar_t*) * functionCount);
	int maxlen = 0;
	for (int i = 0; i < functionCount; i++) {
		formulas[i] = polynomialToWString(i);
		maxlen = max(maxlen, lstrlenW(formulas[i]));
	}
	//~ 함수 공식을 문자열로 배열에 저장

	//함수 출력 ~
	lf Enumber[3];
	short yCoord = 7;
	int start = max(0, min(cursorIndex, functionCount - (screen[1] - 8) / 3));
	for (int i = start; i < min(functionCount, start + (screen[1] - 8) / 3); i++) {
		if ((int)yCoord + 1 >= screen[1] - 1) break;
		if (lstrlenW(formulas[i]) >= screen[0] - 25) {
			formulas[i][screen[0] - 28] = L'.';
			formulas[i][screen[0] - 27] = L'.';
			formulas[i][screen[0] - 26] = L'.';
			formulas[i][screen[0] - 25] = L'\0';
		}
		putWStringToBuffer(buffer, (COORD) { (short)1, yCoord }, formulas[i]);
		if (i == cursorIndex) putWStringToBuffer(buffer, (COORD) { min((short)maxlen + 5, screen[0] - 20), yCoord }, \
			adjustKoreanWString(L"<< [그래프로 이동]"));
		calculateFunction(functions[i]->polynomial, functions[i]->degree, functions[i]->xval, Enumber);
		EnumberToChar(Enumber, EnumberChar);
		if (i + 1 < 10) swprintf(text, 20, L"f%lc(x%lc) = %hs", subscriptNumbers[i + 1], subscriptNumbers[i + 1], EnumberChar);
		else swprintf(text, 20, L"f%lc%lc(x%lc%lc) = %hs", subscriptNumbers[((int)i + 1) / 10], subscriptNumbers[(i + 1) % 10], \
			subscriptNumbers[((int)i + 1) / 10], subscriptNumbers[(i + 1) % 10], EnumberChar);
		putWStringToBuffer(buffer, (COORD) { (short)1, yCoord + 1 }, text);
		yCoord += 3;
	}
	//~ 함수 출력

	if (currentScreenState == PRE_STATUS) printBufferAnimation(buffer, DIRECTION_LEFT, STATUS);
	else printBackBufferToScreen();

	return;
}

#pragma endregion

#pragma region function printSettingsScreen

static void printSettingsScreen(int cursorIndex) { //설정 화면 출력

	if (!animationFrame) currentScreenState = SETTINGS;

	CHAR_INFO* buffer = screenBuffer;
	if (currentScreenState == PRE_SETTINGS) buffer = tempScreenBuffer;

	resetBuffer(buffer);

	//화면 틀 출력 ~
	for (short y = 0; y < screen[1] - 1; y++) {
		putWCharToBuffer(buffer, (COORD) { (short)0, y }, (wchar_t)L'│');
		putWCharToBuffer(buffer, (COORD) { (short)screen[0] - 1, y }, (wchar_t)L'│');
	}
	for (short x = 0; x < screen[0]; x++) {
		putWCharToBuffer(buffer, (COORD) { x, (short)0 }, (wchar_t)L'─');
		putWCharToBuffer(buffer, (COORD) { x, (short)screen[1] - 2 }, (wchar_t)L'─');
	}
	putWCharToBuffer(buffer, (COORD) { (short)0, (short)0 }, (wchar_t)L'┌');
	putWCharToBuffer(buffer, (COORD) { (short)screen[0] - 1, (short)0 }, (wchar_t)L'┐');
	putWCharToBuffer(buffer, (COORD) { (short)0, (short)screen[1] - 2 }, (wchar_t)L'└');
	putWCharToBuffer(buffer, (COORD) { (short)screen[0] - 1, (short)screen[1] - 2 }, (wchar_t)L'┘');
	//~ 화면 틀 출력

	//설정 이미지 불러오기 ~
	short startIndexCoord;
	if (screen[0] >= 150 && screen[1] >= 20) {
		putFileToBuffer(buffer, (COORD) { (screen[0] / 2) - 35, 1 }, settings_txt);
		startIndexCoord = 9;
	}
	else {
		putWStringToBuffer(buffer, (COORD) { (screen[0] / 2) - 4, 0 }, L"SETTINGS");
		startIndexCoord = 2;
	}
	//~ 설정 이미지 불러오기

	//설정 가능 항목 출력 ~
	wchar_t** text = (wchar_t**)xmalloc(sizeof(wchar_t*) * settingTextCount);
	int len = 100;
	for (int i = 0; i < settingTextCount; i++) {
		text[i] = (wchar_t*)xmalloc(sizeof(wchar_t) * len);
	}
	int index = 0;
	swprintf(text[index++], len, L"큰 수(E/e) 표기법: %c", ECharExpression);
	swprintf(text[index++], len, L"프레임 당 지연 시간: %dms (%.3lf초)", frameDelay, (lf)frameDelay / 1000);
	swprintf(text[index++], len, L"애니메이션 프레임 수: %d프레임", animationFrame);
	swprintf(text[index++], len, L"표시되는 절편 개수: %d / %d개", interceptCount, maxInterceptCount);
	if (screenReloadingCycle != -1) swprintf(text[index++], len, L"자동 화면 새로고침 주기: %dms (%.3lf초)", screenReloadingCycle, (lf)screenReloadingCycle / 1000);
	else swprintf(text[index++], len, L"자동 화면 새로고침 주기: 새로고침하지 않음");
	if (autoSavingCycle == -1) swprintf(text[index++], len, L"자동 데이터 저장 주기: 저장하지 않음");
	else if (autoSavingCycle < 60000) swprintf(text[index++], len, L"자동 데이터 저장 주기: %dms (%d초)", autoSavingCycle, autoSavingCycle / 1000);
	else swprintf(text[index++], len, L"자동 데이터 저장 주기: %dms (%d분)", autoSavingCycle, autoSavingCycle / 60000);
	for (int i = 0; i < settingTextCount; i++) {
		text[i] = adjustKoreanWString(text[i]);
		putWStringToBuffer(buffer, (COORD) { (screen[0] / 2) - (short)(wcslen(text[i]) / 2), startIndexCoord + (i * 2) + 2 }, text[i]);
	}

	putWCharToBuffer(buffer, (COORD) { (screen[0] / 2) - (short)(wcslen(text[cursorIndex]) / 2) - 3, startIndexCoord + (cursorIndex * 2) + 2 }, L'>');
	putWCharToBuffer(buffer, (COORD) { (screen[0] / 2) + (short)(wcslen(text[cursorIndex]) / 2) + 2, startIndexCoord + (cursorIndex * 2) + 2 }, L'<');
	//~ 설정 가능 항목 출력

	if (currentScreenState == PRE_SETTINGS) printBufferAnimation(buffer, DIRECTION_UP, SETTINGS);
	else printBackBufferToScreen();

	return;
}

#pragma endregion

#pragma region function printGraphScreen

static void printGraphScreen() { //그래프 화면 출력

	if (!animationFrame) currentScreenState = GRAPH;

	CHAR_INFO* buffer = screenBuffer;
	if (currentScreenState == PRE_GRAPH) {
		buffer = tempScreenBuffer;
	}
	resetBuffer(buffer);

	struct ECoord* minCoord = currentFunction->minCoord;
	struct ECoord* maxCoord = currentFunction->maxCoord;

	struct lfCoord graphScreen;
	graphScreen.X = endCoord.X - startCoord.X - 10;
	graphScreen.Y = endCoord.Y - startCoord.Y - 3;
	struct ECoord EGraphScreen;
	EGraphScreen.X[0] = graphScreen.X;
	EGraphScreen.X[1] = 0.0;
	EGraphScreen.X[2] = 0.0;
	EGraphScreen.Y[0] = graphScreen.Y;
	EGraphScreen.Y[1] = 0.0;
	EGraphScreen.Y[2] = 0.0;
	EnumberAdjust(EGraphScreen.X, 3);
	EnumberAdjust(EGraphScreen.Y, 3);

	lf width[3], height[3];
	lf xGap[3], yGap[3];
	lf t[3];
	EnumberSubtract(maxCoord->X, minCoord->X, width);
	EnumberSubtract(EGraphScreen.X, (lf[]) { 1.0, 0.0, 0.0 }, t);
	EnumberDivide(width, t, xGap); //글자 하나 당 변동되는 x값
	EnumberSubtract(maxCoord->Y, minCoord->Y, height);
	EnumberSubtract(EGraphScreen.Y, (lf[]) { 1.0, 0.0, 0.0 }, t);
	EnumberDivide(height, t, yGap); //글자 하나당 변동되는 y값

	lf screenDegree[3]; //전체 화면에서 x 변화량에 대한 y 변화량 (화면 좌측 하단부터 우측 상단까지의 기울기)
	EnumberDivide(height, width, screenDegree);
	lf degree[4][3] = { {0.0,}, };
	lf divide[4] = { -2.0, -0.5, 0.5, 2.0 };
	for (int i = 0; i < 4; i++) {
		EnumberMultiply(screenDegree, (lf[]) { divide[i], 0.0, 0.0 }, degree[i]);
	}

	//그래프 출력 ~
	for (lld i = 0; i < (lld)graphScreen.X; i++) {
		lf xval[3]; //x값
		EnumberMultiply(xGap, (lf[]) { (lf)i, 0.0, 0.0 }, xval);
		EnumberAdd(minCoord->X, xval, xval);

		lf yval[3]; //y값
		calculateFunction(currentFunction->polynomial, currentFunction->degree, xval, yval);
		static struct lfCoord lfdot;
		lf ycoord[3];
		lfdot.X = (lf)i;
		EnumberSubtract(yval, minCoord->Y, ycoord);
		EnumberDivide(ycoord, yGap, ycoord);
		EnumberSubtract(EGraphScreen.Y, ycoord, ycoord);
		EnumberToDouble(ycoord, &lfdot.Y);
		lfdot.Y--;
		if (lfdot.Y < 0 || lfdot.Y >= graphScreen.Y) continue;

		lf incline[3]; //기울기
		calculateFunction(currentFunction->derivative, currentFunction->degree - 1, xval, incline);
		wchar_t ch;
		if (EnumberCompare(incline, degree[0]) == 1) ch = L'│';
		else if (EnumberCompare(incline, degree[1]) == 1) ch = L'\\';
		else if (EnumberCompare(incline, degree[2]) == 1) ch = L'─';
		else if (EnumberCompare(incline, degree[3]) == 1) ch = L'/';
		else ch = L'│';
		lf t[3];
		EnumberSubtract(xval, currentFunction->xval, t);
		EnumberAbs(t, t);
		if (EnumberCompare(t, xGap) != -1) putCharInfoToBuffer(buffer, (COORD) { (short)startCoord.X + 5 + (short)lfdot.X, (short)startCoord.Y + (short)lfdot.Y }, \
			wcharToColoredUnicode(ch, (bool[]) { 1, 0, 1, 0 }, (bool[]) { 0, 0, 0, 0 })); //xn 기준 3칸의 색상 변경
		else putCharInfoToBuffer(buffer, (COORD) { (short)startCoord.X + 5 + (short)lfdot.X, (short)startCoord.Y + (short)lfdot.Y }, \
			wcharToColoredUnicode(ch, currentFunction->foreGroundColor, currentFunction->backGroundColor));
	}
	//~ 그래프 출력

	//우측 상단 함수 표기 설정 ~
	for (lld x = (lld)endCoord.X - 20; x < (lld)endCoord.X; x++) {
		putWCharToBuffer(buffer, (COORD) { (short)x, 2 }, L'─');
	}
	putWCharToBuffer(buffer, (COORD) { (short)endCoord.X - 20, 0 }, L'│');
	putWCharToBuffer(buffer, (COORD) { (short)endCoord.X - 20, 1 }, L'│');
	putWCharToBuffer(buffer, (COORD) { (short)endCoord.X - 20, 2 }, L'└');
	putWStringToBuffer(buffer, (COORD) { (short)endCoord.X - 19, 0 }, L"              ");
	wchar_t text[20];
	if (functionIndex + 1 < 10) swprintf(text, 20, L" 현재 함수: f%lc(x)", subscriptNumbers[functionIndex + 1]);
	else swprintf(text, 20, L" 현재 함수: f%lc%lc(x)", subscriptNumbers[((int)functionIndex + 1) / 10], subscriptNumbers[(functionIndex + 1) % 10]);
	putWStringToBuffer(buffer, (COORD) { (short)endCoord.X - 19, 0 }, adjustKoreanWString(text));
	putWStringToBuffer(buffer, (COORD) { (short)endCoord.X - 19, 1 }, L"              ");
	lf Enumber[3];
	char EnumberChar[6];
	calculateFunction(currentFunction->polynomial, currentFunction->degree, currentFunction->xval, Enumber);
	EnumberToChar(Enumber, EnumberChar);
	if (functionIndex + 1 < 10) swprintf(text, 20, L" f%lc(x%lc) = %hs", subscriptNumbers[functionIndex + 1], subscriptNumbers[functionIndex + 1], EnumberChar);
	else swprintf(text, 20, L" f%lc%lc(x%lc%lc) = %hs", subscriptNumbers[((int)functionIndex + 1) / 10], subscriptNumbers[(functionIndex + 1) % 10], \
		subscriptNumbers[((int)functionIndex + 1) / 10], subscriptNumbers[(functionIndex + 1) % 10], EnumberChar);
	putWStringToBuffer(buffer, (COORD) { (short)endCoord.X - 19, 1 }, text);
	//~ 우측 상단 함수 표기 설정

	wchar_t* formula = polynomialToWString(functionIndex); //하단 함수 식 출력
	putStringInfoToBuffer(buffer, (COORD) { 0, (short)endCoord.Y }, wstringToColoredUnicode(formula, (bool[]) { 1, 1, 1, 1 }, (bool[]) { 0, 0, 0, 0 }), lstrlenW(formula));


	printCoordinatePlane(buffer); //좌표평면 출력

	if (currentScreenState == PRE_GRAPH) printBufferAnimation(buffer, DIRECTION_RIGHT, GRAPH);
	else printBackBufferToScreen();
	return;
}

#pragma endregion

#pragma region function printStoreScreen

static void printStoreScreen() { //상점 화면 출력
	if (!animationFrame) currentScreenState = STORE;

	CHAR_INFO* buffer = screenBuffer;
	if (currentScreenState == PRE_STORE) buffer = tempScreenBuffer;

	resetBuffer(buffer);

	//화면 틀 출력 ~
	for (short y = 0; y < screen[1] - 1; y++) {
		putWCharToBuffer(buffer, (COORD) { (short)0, y }, (wchar_t)L'│');
		putWCharToBuffer(buffer, (COORD) { (short)screen[0] - 1, y }, (wchar_t)L'│');
	}
	for (short x = 0; x < screen[0]; x++) {
		putWCharToBuffer(buffer, (COORD) { x, (short)0 }, (wchar_t)L'─');
		putWCharToBuffer(buffer, (COORD) { x, (short)screen[1] - 2 }, (wchar_t)L'─');
	}
	putWCharToBuffer(buffer, (COORD) { (short)0, (short)0 }, (wchar_t)L'┌');
	putWCharToBuffer(buffer, (COORD) { (short)screen[0] - 1, (short)0 }, (wchar_t)L'┐');
	putWCharToBuffer(buffer, (COORD) { (short)0, (short)screen[1] - 2 }, (wchar_t)L'└');
	putWCharToBuffer(buffer, (COORD) { (short)screen[0] - 1, (short)screen[1] - 2 }, (wchar_t)L'┘');
	//~ 화면 틀 출력

	//상점 이미지 불러오기 ~
	short startIndexCoord;
	if (screen[0] >= 150 && screen[1] >= 20) {
		putFileToBuffer(buffer, (COORD) { (screen[0] / 2) - 22, 1 }, store_txt);
		startIndexCoord = 9;
	}
	else {
		putWStringToBuffer(buffer, (COORD) { (screen[0] / 2) - 4, 0 }, L"STORE");
		startIndexCoord = 2;
	}
	//~ 상점 이미지 불러오기

	if (currentScreenState == PRE_STORE) printBufferAnimation(buffer, DIRECTION_DOWN, STORE);
	else printBackBufferToScreen();

	return;
}

#pragma endregion

#pragma region function printCoordinatePlane

static void printCoordinatePlane(CHAR_INFO* buffer) { //좌표평면 출력. 그래프 화면 출력 함수에서만 쓰임

	for (lf y = startCoord.Y; y < endCoord.Y - 1; y++) {
		putWCharToBuffer(buffer, (COORD) { (short)startCoord.X + 2, (short)y }, (wchar_t)L'│');
	}
	for (lf x = startCoord.X + 2; x < endCoord.X - 2; x++) {
		putWCharToBuffer(buffer, (COORD) { (short)x, (short)endCoord.Y - 2 }, (wchar_t)L'─');
	}
	putWCharToBuffer(buffer, (COORD) { (short)startCoord.X + 2, (short)endCoord.Y - 2 }, (wchar_t)L'└');

	short width = (short)(startCoord.X + endCoord.X - 4);
	short height = (short)(startCoord.Y + endCoord.Y - 1);

	int n = interceptCount;
	char Enumber[6];
	wchar_t wcEnumber[6];
	lf value[3];
	lf t[3];
	EnumberSubtract(currentFunction->maxCoord->X, currentFunction->minCoord->X, value);

	for (int i = 0; i < n; i++) {
		EnumberMultiply(value, (lf[]) { ((lf)(i + 1) / (n + 1)), 0.0, 0.0 }, t);
		EnumberAdd(currentFunction->minCoord->X, t, t);
		EnumberToChar(t, Enumber);
		mbstowcs(wcEnumber, Enumber, 6);
		putWStringToBuffer(buffer, (COORD) { width* (i + 1) / (n + 1) - 2, (short)endCoord.Y - 1 }, wcEnumber);
		putWCharToBuffer(buffer, (COORD) { width* (i + 1) / (n + 1), (short)endCoord.Y - 2 }, (wchar_t)L'┼');
	}

	EnumberSubtract(currentFunction->maxCoord->Y, currentFunction->minCoord->Y, value);
	for (int i = 0; i < n; i++) {
		EnumberMultiply(value, (lf[]) { ((lf)(i + 1) / (n + 1)), 0.0, 0.0 }, t);
		EnumberSubtract(currentFunction->maxCoord->Y, t, t);
		EnumberToChar(t, Enumber);
		mbstowcs(wcEnumber, Enumber, 6);
		putWStringToBuffer(buffer, (COORD) { (short)startCoord.X, height* (i + 1) / (n + 1) }, wcEnumber);
	}

	return;
}

#pragma endregion

#pragma region function printBackBufferToScreen

static void printBackBufferToScreen() { //백 버퍼를 현재 버퍼로 전환하여 화면에 띄움

	reloadScreen();

	COORD bufferCoord = { 0, 0 };
	SMALL_RECT region = { 0, 0, (SHORT)(screen[0] - 1), (SHORT)(screen[1] - 1) };

	CHAR_INFO* cloneBuffer = (CHAR_INFO*)xmalloc(sizeof(CHAR_INFO) * screenSize);
	memcpy(cloneBuffer, screenBuffer, sizeof(CHAR_INFO) * screenSize);



	for (int y = 0; y < screen[1]; y++) {
		int dx = 0;
		for (int x = 0; x < screen[0]; x++) {
			int index = y * screen[0] + x;
			wchar_t ch = cloneBuffer[index].Char.UnicodeChar;
			if (ch >= 0xAC00 && ch <= 0xD7AF) dx++;
			if (dx && index - dx >= 0) cloneBuffer[index - dx] = cloneBuffer[index];
		}
		for (int i = (y + 1) * screen[0] - dx; i < (y + 1) * screen[0]; i++) {
			cloneBuffer[i].Char.UnicodeChar = 0;
		}
	}

	WriteConsoleOutputW(hBackBuffer, cloneBuffer, (COORD) { screen[0], screen[1] }, bufferCoord, &region); //화면에 버퍼 텍스트 출력

	free(cloneBuffer);
	SetConsoleActiveScreenBuffer(hBackBuffer);
	swap(&hOut, &hBackBuffer); //백 버퍼와 현재 버퍼를 전환

	return;
}

#pragma endregion

#pragma region function printBufferAnimation

static void printBufferAnimation(CHAR_INFO* tempScreenBuffer, short direction, int afterScreenState) { //화면 출력 애니메이션. 입력 값에 따라 애니메이션 방향을 조절 가능

	int dx = 0, dy = 0;
	int startLine = 0;

	switch (direction) {
	case DIRECTION_UP:
		dx = 0;
		dy = -1;
		startLine = screen[1] - 1;
		break;

	case DIRECTION_DOWN:
		dx = 0;
		dy = +1;
		startLine = 0;
		break;

	case DIRECTION_LEFT:
		dx = -1;
		dy = 0;
		startLine = screen[0] - 1;
		break;

	case DIRECTION_RIGHT:
		dx = +1;
		dy = 0;
		startLine = 0;
		break;

	default:
		return;
	}

	int maxSteps = (direction == DIRECTION_UP || direction == DIRECTION_DOWN) ? screen[1] : screen[0];

	for (int i = 0; i < maxSteps; i++) {

		int copy = 0, paste = 0;
		if (direction == DIRECTION_UP || direction == DIRECTION_DOWN) {
			for (int j = 0; j <= i; j++) {

				if (direction == DIRECTION_UP) {
					copy = startLine - j;
					paste = i - j;
				}
				else {
					copy = startLine + j;
					paste = (screen[1] - 1) - (i - j);
				}
				for (int x = 0; x < screen[0]; x++) {
					int index = copy * screen[0] + x;
					if (index >= screenSize) {
						currentScreenState = afterScreenState;
						return;
					}
					CHAR_INFO ci = tempScreenBuffer[index];
					putCharInfoToBuffer(screenBuffer, (COORD) { (SHORT)x, (SHORT)paste }, ci);

				}
			}
		}

		else {
			for (int j = 0; j <= i; j++) {
				if (direction == DIRECTION_LEFT) {
					copy = (screen[0] - 1) - j;
					paste = i - j;
				}
				else {
					copy = j;
					paste = (screen[0] - 1) - (i - j);
				}
				for (int y = 0; y < screen[1]; y++) {
					int index = y * screen[0] + copy;
					if (index >= screenSize) {
						currentScreenState = afterScreenState;
						return;
					}
					CHAR_INFO ci = tempScreenBuffer[index];
					putCharInfoToBuffer(screenBuffer, (COORD) { (SHORT)paste, (SHORT)y }, ci);
				}
			}
		}

		printBackBufferToScreen();

		if ((i % max((int)(maxSteps / animationFrame), 1)) == 0) {
			sleep(10);
		}
	}

	currentScreenState = afterScreenState;
	return;
}

#pragma endregion

#pragma endregion

#pragma endregion
