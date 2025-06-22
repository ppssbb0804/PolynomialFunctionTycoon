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
#define ENUMBER_MAX ((lf[]) { 9.0, 1.0, 9.0 })
#define ENUMBER_MIN ((lf[]) { -9.0, 1.0, 9.0 })
#define TEMPSCREENSCALE_X 300
#define TEMPSCREENSCALE_Y 150
#define screenSize screen[0] * screen[1]
#define settings_txt L"txtImages\\SETTINGS.txt"
#define maxFrameDelay 100
#define maxAnimationFrame 100
#define maxInterceptCount min((screen[0] - 6) / 6, (screen[1] - 5) / 2)

#define lld long long int
#define lf double
#define sleep Sleep
#define CLS_TEMP(buffer) FillConsoleOutputCharacter(buffer, ' ', TEMPSCREENSCALE_X * TEMPSCREENSCALE_Y, T, &dw);gotoxy(0,0)
#define CLS(buffer) FillConsoleOutputCharacter(buffer, ' ', screenSize, T, &dw);gotoxy(0,0)
#define singleKeyState(KEY) currentKeyState[KEY] && !beforeKeyState[KEY]

#pragma endregion

#pragma region structs

struct ECoord {
	lf* X;
	lf* Y;
};

struct lfCoord {
	lf X;
	lf Y;
};

struct functionType {
	lf* polynomial, * derivative;
	int degree;
	lf* t, * dt;
	struct ECoord* minCoord;
	struct ECoord* maxCoord;
};

#pragma endregion

#pragma region define functions

#pragma region defining system

static void gotoxy(int, int);
static void hidecursor();
static void fullscreen(HANDLE, int, int);
static void initDoubleBuffer();
static void* xmalloc(size_t);
static void* xrealloc(void*, size_t);
static void swap(void*, void*);
static void clearInput(void);
static void clearKeystrokeBuffer(void);
static void hprintf(wchar_t*, ...);
static void reloadScreen();
static void resetBuffer(CHAR_INFO*);
static void saveData();

#pragma region defining threads

static DWORD WINAPI screenReloadingThread(LPVOID);
static DWORD WINAPI autoSavingThread(LPVOID);
static DWORD WINAPI eventManagingThread(LPVOID);

#pragma endregion

#pragma endregion

#pragma region defining text

static CHAR_INFO wcharToUnicode(wchar_t);
static CHAR_INFO wcharToColoredUnicode(wchar_t, bool[4], bool[4]);
static wchar_t* adjustKoreanWString(wchar_t*);
static void putCharToBuffer(CHAR_INFO*, COORD, CHAR_INFO);
static void putWCharToBuffer(CHAR_INFO*, COORD, wchar_t);
static void putFileToBuffer(CHAR_INFO*, COORD, wchar_t*);
static void putWStringToBuffer(CHAR_INFO*, COORD, wchar_t*);

#pragma endregion

#pragma region defining polynomial calculation

static lf* calculateFunction(lf*, int, lf[3], lf[3]);
static lf* polynomialDifferentiation(lf*, int);
static lf* polynomialIntegral(lf*, int);

#pragma endregion

#pragma region defining Enumber

static void EnumberAdjust(lf[3], int);
static void EnumberToDouble(lf[3], lf*);
static void EnumberToChar(lf[3], char[6]);
static void EnumberAdd(lf[3], lf[3], lf[3]);
static void EnumberSubtract(lf[3], lf[3], lf[3]);
static void EnumberMultiply(lf[3], lf[3], lf[3]);
static void EnumberDivide(lf[3], lf[3], lf[3]);
static void EnumberAbs(lf[3], lf[3]);
static short EnumberCompare(lf[3], lf[3]);
static void EnumberPower(lf[3], lld, lf[3]);

#pragma endregion

#pragma region defining print

static void printMainScreen();
static void printSettingsScreen(int);
static void printGraphScreen();
static void printStoreScreen();
static void printCoordinatePlane(CHAR_INFO*);
static void printBackBufferToScreen();
static void printBufferAnimation(CHAR_INFO*, short, int);

#pragma endregion

#pragma endregion

#pragma region global variables

HANDLE hIn, hOut, hBackBuffer;
HWND hwnd;
unsigned long dw;
COORD T;
CONSOLE_SCREEN_BUFFER_INFO csbi;
int screen[2];
lf* Escreen[2];
CHAR_INFO* screenBuffer, * tempScreenBuffer;
char ECharExpression;
int currentScreenState, beforeScreenState;
int frameDelay, animationFrame;
int interceptCount;
int settingTextCount;
int autoSavingCycle, screenReloadingCycle;
HANDLE screenReloadingThreadReadyEvent, autoSavingThreadReadyEvent, eventManagingThreadReadyEvent;
struct lfCoord startCoord, endCoord;
int settingArrowIndex;
int functionCount;
struct functionType* currentFunction;

enum {
	LOADING = 0,
	PRE_MAIN,
	MAIN,
	PRE_GRAPH,
	GRAPH,
	PRE_STORE,
	STORE,
	PRE_SETTINGS,
	SETTINGS,
	SCREEN_COUNT
};

enum {
	DIRECTION_UP = 0,
	DIRECTION_DOWN,
	DIRECTION_LEFT,
	DIRECTION_RIGHT
};

#pragma endregion

#pragma region main

int main(void) {

#pragma region init

	initDoubleBuffer();
	SetConsoleOutputCP(CP_UTF8);
	system("chcp 65001 > nul");
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	hIn = GetStdHandle(STD_INPUT_HANDLE);
	FlushConsoleInputBuffer(hIn);
	currentScreenState = LOADING;
	Escreen[0] = (lf*)xmalloc(sizeof(lf) * 3);
	Escreen[1] = (lf*)xmalloc(sizeof(lf) * 3);
	screenBuffer = NULL;
	tempScreenBuffer = NULL;
	animationFrame = 10;
	settingTextCount = 6;
	autoSavingCycle = 60000;
	screenReloadingCycle = 1000;
	settingArrowIndex = 0;
	beforeScreenState = MAIN;
	currentScreenState = GRAPH;

	dw;
	T = (COORD) { 0, 0 };
	hwnd = GetForegroundWindow();

	ECharExpression = 'E';
	frameDelay = 30;
	interceptCount = 13;


	struct functionType** functions = (struct functionType**)xmalloc(sizeof(struct functionType*));

	if (scanf("%d", &functionCount) != 1) {
		CLS_TEMP(hOut);
		hprintf(L"[ERROR] Insufficient Input");
		return -1;
	}
	
	for (int index = 0; index < functionCount; index++) {

		int n;
		if (scanf("%d", &n) != 1) {
			CLS_TEMP(hOut);
			hprintf(L"[ERROR] Insufficient Input");
			return -1;
		}

		functions[index] = (struct functionType*)xmalloc(sizeof(struct functionType));
		functions[index]->degree = n;

		functions[index]->polynomial = (lf*)xmalloc(sizeof(lf) * n);

		for (int i = 0; i < n; i++) {
			if (scanf("%lf", &functions[index]->polynomial[n - i - 1]) != 1) {
				CLS_TEMP(hOut);
				hprintf(L"[ERROR] Insufficient Input");
				return -1;
			}
		}

		functions[index]->derivative = polynomialDifferentiation(functions[index]->polynomial, functions[index]->degree);

		functions[index]->minCoord = (struct ECoord*)xmalloc(sizeof(struct ECoord));
		functions[index]->maxCoord = (struct ECoord*)xmalloc(sizeof(struct ECoord));
		
		functions[index]->t = (lf*)xmalloc(sizeof(lf) * 3);
		functions[index]->dt = (lf*)xmalloc(sizeof(lf) * 3);

		CLS_TEMP(hOut);

		functions[index]->minCoord->X = (lf*)xmalloc(sizeof(lf) * 3);
		functions[index]->minCoord->X[0] = 0.0;
		functions[index]->minCoord->X[1] = 0.0;
		functions[index]->minCoord->X[2] = 0.0;
		EnumberAdjust(functions[index]->minCoord->X, 3);

		functions[index]->minCoord->Y = (lf*)xmalloc(sizeof(lf) * 3);
		calculateFunction(functions[index]->polynomial, n, functions[index]->minCoord->X, functions[index]->minCoord->Y);

		functions[index]->maxCoord->X = (lf*)xmalloc(sizeof(lf) * 3);
		functions[index]->maxCoord->X[0] = 3.0;
		functions[index]->maxCoord->X[1] = 1.0;
		functions[index]->maxCoord->X[2] = 0.0;
		EnumberAdjust(functions[index]->maxCoord->X, 3);

		functions[index]->maxCoord->Y = (lf*)xmalloc(sizeof(lf) * 3);
		calculateFunction(functions[index]->polynomial, n, functions[index]->maxCoord->X, functions[index]->maxCoord->Y);

		if (EnumberCompare(functions[index]->minCoord->Y, functions[index]->maxCoord->Y) == -1) swap(functions[index]->minCoord->Y, functions[index]->maxCoord->Y);
	}
	int functionIndex = 1;
	currentFunction = functions[functionIndex];

	CLS_TEMP(hOut);
	hprintf(L"전체화면 로딩 중...");
	fullscreen(hOut, 0, 0);

	if (screen[0] == -1) {
		CLS_TEMP(hOut);
		hprintf(L"전체화면 로딩 실패. 일반 화면으로 진행됩니다.");
		sleep(1000);
		CLS_TEMP(hOut);
	}
	
	CLS_TEMP(hOut);
	hprintf(L"스레드 로딩 중...");
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


	int minScreenSize[2] = { 30, 15 };
	if (screen[0] < minScreenSize[0] || screen[1] < minScreenSize[1]) {
		CLS(hOut);
		hprintf(L"게임 플레이를 위해서는 최소 가로 %d, 세로 %d의 콘솔 화면 크기가 필요합니다.\n", minScreenSize[0], minScreenSize[1]);
		hprintf(L"CTRL + R을 눌러 화면 리로드가 가능합니다. 가로 %d, 세로 %d 이상으로 콘솔 화면 크기를 설정하신 뒤 화면을 리로드해 주세요.", minScreenSize[0], minScreenSize[1]);
	}

#pragma endregion

#pragma region getting key

	struct ECoord modifyValue;
	modifyValue.X = (lf*)malloc(sizeof(lf) * 3);
	modifyValue.Y = (lf*)malloc(sizeof(lf) * 3);

	enum {
		KEY_LEFT = 0, KEY_RIGHT, KEY_UP, KEY_DOWN,
		KEY_W, KEY_A, KEY_S, KEY_D,
		KEY_R,
		KEY_COMMA, KEY_PERIOD,
		KEY_MINUS, KEY_PLUS,
		KEY_TAB, KEY_ESC, KEY_G, KEY_B,
		KEY_CTRL,
		KEY_0,
		KEY_NUMPAD_0 = KEY_0 + 10,
		KEY_COUNT = KEY_NUMPAD_0 + 10
	};

	int KEY[KEY_COUNT] = {
		VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN,
		'W', 'A', 'S', 'D',
		'R',
		VK_OEM_COMMA, VK_OEM_PERIOD,
		VK_OEM_MINUS, VK_OEM_PLUS,
		VK_TAB, VK_ESCAPE, 'G', 'B',
		VK_CONTROL,
		'0', VK_NUMPAD0
	};

	FlushConsoleInputBuffer(hIn);
	clearInput();
	bool currentKeyState[KEY_COUNT] = { false };
	bool beforeKeyState[KEY_COUNT] = { false };

	while (true) {

		bool reload = false;
		bool scaleAdjust = false;
		lf adjustValue = 1;

		for (int i = 0; i < KEY_COUNT; i++) {
			currentKeyState[i] = (GetAsyncKeyState(KEY[i]) & 0x8000) != 0;
		}

		if (currentScreenState == MAIN) {
			if (singleKeyState(KEY_TAB)) {
				if (beforeScreenState == SETTINGS) currentScreenState = PRE_SETTINGS;
				if (beforeScreenState == GRAPH) currentScreenState = PRE_GRAPH;
				if (beforeScreenState == STORE) currentScreenState = PRE_STORE;
				beforeScreenState = MAIN;
				reload = true;
			}
			if (singleKeyState(KEY_ESC)) {
				beforeScreenState = MAIN;
				currentScreenState = PRE_SETTINGS;
				settingArrowIndex = 0;
				reload = true;
			}
			if (singleKeyState(KEY_G)) {
				beforeScreenState = MAIN;
				currentScreenState = PRE_GRAPH;
				reload = true;
			}
			if (singleKeyState(KEY_B)) {
				beforeScreenState = MAIN;
				currentScreenState = PRE_STORE;
				reload = true;
			}
		}

		else if (currentScreenState == SETTINGS) {
			if (singleKeyState(KEY_LEFT) || singleKeyState(KEY_A)) {
				switch (settingArrowIndex) {
				case 0:
					if (ECharExpression == 'E') ECharExpression = 'e';
					else ECharExpression = 'E';
					break;
				case 1:
					if (frameDelay > 0) frameDelay -= 5;
					break;
				case 2:
					if (animationFrame > 0) animationFrame -= 5;
					break;
				case 3:
					if (interceptCount > 0) interceptCount--;
					break;
				case 4:
					if (screenReloadingCycle > 1000) screenReloadingCycle -= 500;
					else if (screenReloadingCycle > 100) screenReloadingCycle -= 100;
					else if (screenReloadingCycle > 0) screenReloadingCycle -= 10;
					else if (screenReloadingCycle == -1) screenReloadingCycle = 5000;
					break;
				case 5:
					if (autoSavingCycle > 60000) autoSavingCycle -= 60000;
					else if (autoSavingCycle > 10000) autoSavingCycle -= 10000;
					else if (autoSavingCycle > 1000) autoSavingCycle -= 1000;
					else if (autoSavingCycle == -1) autoSavingCycle = 60000 * 60;
					break;
				default:
					break;
				}
				reload = true;
			}
			if (singleKeyState(KEY_RIGHT) || singleKeyState(KEY_D)) {
				switch (settingArrowIndex) {
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
			if (singleKeyState(KEY_UP) || singleKeyState(KEY_W)) {
				if (settingArrowIndex > 0) {
					settingArrowIndex--;
					reload = true;
				}
			}
			if (singleKeyState(KEY_DOWN) || singleKeyState(KEY_S)) {
				if (settingArrowIndex < settingTextCount - 1) {
					settingArrowIndex++;
					reload = true;
				}
			}

			if (singleKeyState(KEY_ESC)) {
				if (beforeScreenState == GRAPH) currentScreenState = PRE_GRAPH;
				if (beforeScreenState == MAIN) currentScreenState = PRE_MAIN;
				if (beforeScreenState == STORE) currentScreenState = PRE_STORE;
				beforeScreenState = SETTINGS;
				reload = true;
			}
			if (singleKeyState(KEY_G)) {
				beforeScreenState = SETTINGS;
				currentScreenState = PRE_GRAPH;
				reload = true;
			}
			if (singleKeyState(KEY_TAB)) {
				beforeScreenState = SETTINGS;
				currentScreenState = PRE_MAIN;
				reload = true;
			}
			if (singleKeyState(KEY_B)) {
				beforeScreenState = SETTINGS;
				currentScreenState = PRE_STORE;
				reload = true;
			}
		}

		else if (currentScreenState == GRAPH) {
			
			if (singleKeyState(KEY_COMMA)) {
				if (functionIndex > 0) {
					functionIndex--;
					currentFunction = functions[functionIndex];
					reload = true;
				}
			}
			if (singleKeyState(KEY_PERIOD)) {
				if (functionIndex < functionCount - 1) {
					functionIndex++;
					currentFunction = functions[functionIndex];
					reload = true;
				}
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

			if (currentKeyState[KEY_LEFT] || currentKeyState[KEY_A]) {
				EnumberSubtract(currentFunction->minCoord->X, xGap, currentFunction->minCoord->X);
				EnumberSubtract(currentFunction->maxCoord->X, xGap, currentFunction->maxCoord->X);
				reload = true;
			}
			if (currentKeyState[KEY_RIGHT] || currentKeyState[KEY_D]) {
				EnumberAdd(currentFunction->minCoord->X, xGap, currentFunction->minCoord->X);
				EnumberAdd(currentFunction->maxCoord->X, xGap, currentFunction->maxCoord->X);
				reload = true;
			}
			if (currentKeyState[KEY_UP] || currentKeyState[KEY_W]) {
				EnumberAdd(currentFunction->minCoord->Y, yGap, currentFunction->minCoord->Y);
				EnumberAdd(currentFunction->maxCoord->Y, yGap, currentFunction->maxCoord->Y);
				reload = true;
			}
			if (currentKeyState[KEY_DOWN] || currentKeyState[KEY_S]) {
				EnumberSubtract(currentFunction->minCoord->Y, yGap, currentFunction->minCoord->Y);
				EnumberSubtract(currentFunction->maxCoord->Y, yGap, currentFunction->maxCoord->Y);
				reload = true;
			}

			if (singleKeyState(KEY_MINUS)) {
				adjustValue = 12.0 / 10.0;
				scaleAdjust = true;
				reload = true;
			}
			if (singleKeyState(KEY_PLUS)) {
				adjustValue = 10.0 / 12.0;
				scaleAdjust = true;
				reload = true;
			}

			if (singleKeyState(KEY_G)) {
				if (beforeScreenState == MAIN) currentScreenState = PRE_MAIN;
				if (beforeScreenState == SETTINGS) currentScreenState = PRE_SETTINGS;
				if (beforeScreenState == STORE) currentScreenState = PRE_STORE;
				beforeScreenState = GRAPH;
				reload = true;
			}
			if (singleKeyState(KEY_TAB)) {
				beforeScreenState = GRAPH;
				currentScreenState = PRE_MAIN;
				reload = true;
			}
			if (singleKeyState(KEY_ESC)) {
				beforeScreenState = GRAPH;
				currentScreenState = PRE_SETTINGS;
				settingArrowIndex = 0;
				reload = true;
			}
			if (singleKeyState(KEY_B)) {
				beforeScreenState = GRAPH;
				currentScreenState = PRE_STORE;
				reload = true;
			}

			if (scaleAdjust) {

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

			sleep(frameDelay);
		}

		else if (currentScreenState == STORE) {
			if (singleKeyState(KEY_B)) {
				if (beforeScreenState == MAIN) currentScreenState = PRE_MAIN;
				if (beforeScreenState == SETTINGS) currentScreenState = PRE_SETTINGS;
				if (beforeScreenState == GRAPH) currentScreenState = PRE_GRAPH;
				beforeScreenState = STORE;
				reload = true;
			}
			if (singleKeyState(KEY_TAB)) {
				beforeScreenState = STORE;
				currentScreenState = PRE_MAIN;
				reload = true;
			}
			if (singleKeyState(KEY_ESC)) {
				beforeScreenState = STORE;
				currentScreenState = PRE_SETTINGS;
				settingArrowIndex = 0;
				reload = true;
			}
			if (singleKeyState(KEY_G)) {
				beforeScreenState = STORE;
				currentScreenState = PRE_GRAPH;
				reload = true;
			}
		}

		if (singleKeyState(KEY_R)) {
			reload = true;
		}

		if (reload) {
			reloadScreen();

			if (screen[0] < minScreenSize[0] || screen[1] < minScreenSize[1]) {
				CLS(hOut);
				hprintf(L"게임 플레이를 위해서는 최소 가로 %d, 세로 %d의 콘솔 화면 크기가 필요합니다.\n", minScreenSize[0], minScreenSize[1]);
				hprintf(L"CTRL + R을 눌러 화면 리로드가 가능합니다. 가로 %d, 세로 %d 이상으로 콘솔 화면 크기를 설정하신 뒤 화면을 리로드해 주세요.", minScreenSize[0], minScreenSize[1]);
			}

			else {
				switch (currentScreenState) {

				case PRE_MAIN:
				case MAIN:
					printMainScreen();
					break;

				case PRE_SETTINGS:
				case SETTINGS:
					printSettingsScreen(settingArrowIndex);
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
			beforeKeyState[i] = currentKeyState[i];
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

	free(modifyValue.X);
	free(modifyValue.Y);

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

static void hidecursor() {
	static CONSOLE_CURSOR_INFO info;
	info.dwSize = 100;
	info.bVisible = FALSE;
	SetConsoleCursorInfo(hOut, &info);
	SetConsoleCursorInfo(hBackBuffer, &info);
	return;
}

#pragma endregion

#pragma region function fullscreen

static void fullscreen(HANDLE handle, int minWidth, int minHeight) {

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

static void initDoubleBuffer() {

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

static void* xmalloc(size_t bytes) {

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

static void* xrealloc(void* variable, size_t bytes) {

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

static void swap(void* a, void* b) {
	void* t = *(void**)a;
	*(void**)a = *(void**)b;
	*(void**)b = t;
	return;
}

#pragma endregion

#pragma region function clearInput

static void clearInput(void) {
	int c;
	while ((c = getchar()) != '\n' && c != EOF);
	return;
}

#pragma endregion

#pragma region function clearKeystrokeBuffer

static void clearKeystrokeBuffer(void) {
	while (_kbhit()) {
		char ch = _getch();
	}
}

#pragma endregion

#pragma region function hprintf

static void hprintf(wchar_t* fmt, ...) {

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

static void reloadScreen() {

	int newScreen[2] = { 0, 0 };
	if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
		newScreen[0] = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		newScreen[1] = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
		if (screen[0] == newScreen[0] && screen[1] == newScreen[1] && screenBuffer != NULL) return;

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
		SMALL_RECT newWindowRect = { 0, 0, (SHORT)(screen[0] - 1), (SHORT)(screen[1] - 1) };
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
		if (screenBuffer == NULL) screenBuffer = (CHAR_INFO*)xmalloc(sizeof(CHAR_INFO) * screenSize);
		else screenBuffer = xrealloc(screenBuffer, sizeof(CHAR_INFO) * screenSize);
		resetBuffer(screenBuffer);
		if (tempScreenBuffer == NULL) tempScreenBuffer = (CHAR_INFO*)xmalloc(sizeof(CHAR_INFO) * screenSize);
		else tempScreenBuffer = xrealloc(tempScreenBuffer, sizeof(CHAR_INFO) * screenSize);
		resetBuffer(tempScreenBuffer);
	}

	return;
}

#pragma endregion

#pragma region function resetBuffer

static void resetBuffer(CHAR_INFO* buffer) {
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

static void saveData() {
	return;
}

#pragma endregion

#pragma region function screenReloadingThread

static DWORD WINAPI screenReloadingThread(LPVOID param) {

	UNREFERENCED_PARAMETER(param);
	bool event = false;
	while (true) {
		if (currentScreenState == MAIN || currentScreenState == SETTINGS || currentScreenState == GRAPH || currentScreenState == STORE) {
			reloadScreen();
			switch (currentScreenState) {

			case PRE_MAIN:
			case MAIN:
				printMainScreen();
				break;

			case PRE_SETTINGS:
			case SETTINGS:
				printSettingsScreen(settingArrowIndex);
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

static DWORD WINAPI autoSavingThread(LPVOID param) {
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

static DWORD WINAPI eventManagingThread(LPVOID param) {
	UNREFERENCED_PARAMETER(param);
	SetEvent(eventManagingThreadReadyEvent);
	while (true) {
		sleep(1000);
		
	}
	return 0;
}

#pragma endregion

#pragma endregion

#pragma region text

#pragma region function wcharToUnicode

static CHAR_INFO wcharToUnicode(wchar_t ch) {
	CHAR_INFO newchar;
	newchar.Char.UnicodeChar = ch;
	newchar.Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
	return newchar;
}

#pragma endregion

#pragma region function wcharToColoredUnicode

static CHAR_INFO wcharToColoredUnicode(wchar_t ch, bool foreGround[4], bool backGround[4]) {
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

#pragma region function adjustKoreanWString

static wchar_t* adjustKoreanWString(wchar_t* str) {

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

#pragma region function putCharToBuffer

static void putCharToBuffer(CHAR_INFO* buffer, COORD coord, CHAR_INFO ci) {
	if (coord.X < 0 || coord.X >= screen[0] || coord.Y < 0 || coord.Y >= screen[1]) return;
	buffer[coord.Y * screen[0] + coord.X] = ci;
	return;
}

#pragma endregion

#pragma region function putWCharToBuffer

static void putWCharToBuffer(CHAR_INFO* buffer, COORD coord, wchar_t wc) {
	if (coord.X < 0 || coord.X >= screen[0] || coord.Y < 0 || coord.Y >= screen[1]) return;
	CHAR_INFO ci = wcharToUnicode(wc);
	buffer[coord.Y * screen[0] + coord.X] = ci;
	return;
}

#pragma endregion

#pragma region function putFileToBuffer

static void putFileToBuffer(CHAR_INFO* buffer, COORD coord, wchar_t* file_address) {

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

#pragma region function putWStringToBuffer

static void putWStringToBuffer(CHAR_INFO* buffer, COORD start, wchar_t* str) {

	int len = (int)wcslen(str);

	int px = start.X;
	int py = start.Y;

	for (int i = 0; i < len; i++) {

		if (px < 0 || px >= screen[0] || py < 0 || py >= screen[1]) continue;

		wchar_t wc = str[i];
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

static lf* polynomialDifferentiation(lf* function, int degree) {

	lf* fprime = (lf*)xmalloc(sizeof(lf) * ((lld)degree - 1));

	for (int i = 1; i < degree; i++) {
		fprime[i - 1] = function[i] * i;
	}

	return fprime;
}

#pragma endregion

#pragma region function polynomialIntegral

static lf* polynomialIntegral(lf* function, int degree) {

	lf* F = (lf*)xmalloc(sizeof(lf) * ((lld)degree + 1));

	for (int i = 0; i < degree; i++) {
		F[i] = function[i] / (i + 1);
	}

	return F;
}

#pragma endregion

#pragma region function calculateFunction

static lf* calculateFunction(lf* function, int degree, lf n[3], lf result[3]) {

	result[0] = 0.0;
	result[1] = 0.0;
	result[2] = 0.0;

	lf val[3];
	for (int i = 0; i < degree; i++) {
		lf coefficient[3] = { function[i], 0.0, 0.0 };
		EnumberAdjust(coefficient, 3);
		EnumberPower(n, i, val);
		EnumberMultiply(val, coefficient, val);
		EnumberAdd(result, val, result);
	}

	return result;
}

#pragma endregion

#pragma endregion

#pragma region Enumber

#pragma region function EnumberAdjust

static void EnumberAdjust(lf n[3], int digit) {

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

static void EnumberToDouble(lf n[3], lf* result) {

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

static void EnumberToChar(lf n[3], char result[6]) {

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

static void EnumberAdd(lf a[3], lf b[3], lf result[3]) {

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

static void EnumberSubtract(lf a[3], lf b[3], lf result[3]) {

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

static void EnumberMultiply(lf a[3], lf b[3], lf result[3]) {

	result[0] = a[0] * b[0];
	result[1] = (a[1] * pow(10, a[2])) + (b[1] * pow(10, b[2]));
	result[2] = 0;

	EnumberAdjust(result, 3);
	return;
}

#pragma endregion

#pragma region function EnumberDivide

static void EnumberDivide(lf a[3], lf b[3], lf result[3]) {

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

static void EnumberAbs(lf n[3], lf result[3]) {
	if (EnumberCompare(n, (lf[]) { 0.0, 0.0, 0.0 }) == 1) EnumberMultiply(n, (lf[]) { -1.0, 0.0, 0.0 }, result);
	else memcpy(result, n, sizeof(lf) * 3);
	return;
}

#pragma endregion

#pragma region function EnumberCompare

static short EnumberCompare(lf a[3], lf b[3]) {

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

static void EnumberPower(lf a[3], lld n, lf result[3]) {

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

#pragma region function printMainScreen

static void printMainScreen() {
	//dt = OEOOs (OOOd:OOh:OOm:OOs) = now - t
	//y = Σ (i=0 to n(f)) (Σ (k=0 to dt) fi(k))
	//fi(t+dt) = OEEO
	//fi(x) = OOOOx^(OOOO)+...

	if (!animationFrame) currentScreenState = MAIN;

	CHAR_INFO* buffer = screenBuffer;
	if (currentScreenState == PRE_MAIN) buffer = tempScreenBuffer;
	
	resetBuffer(buffer);

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

	if (currentScreenState == PRE_MAIN) printBufferAnimation(buffer, DIRECTION_LEFT, MAIN);
	else printBackBufferToScreen();

	return;
}

#pragma endregion

#pragma region function printSettingsScreen

static void printSettingsScreen(int arrowIndex) {

	if (!animationFrame) currentScreenState = SETTINGS;

	CHAR_INFO* buffer = screenBuffer;
	if (currentScreenState == PRE_SETTINGS) buffer = tempScreenBuffer;
	
	resetBuffer(buffer);

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

	short startIndexCoord;
	if (screen[0] >= 150 && screen[1] >= 20) {
		putFileToBuffer(buffer, (COORD) { (screen[0] / 2) - 41, 1 }, settings_txt);
		startIndexCoord = 9;
	}
	else {
		putWStringToBuffer(buffer, (COORD) { (screen[0] / 2) - 4, 0 }, L"SETTINGS");
		startIndexCoord = 2;
	}

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

	putWCharToBuffer(buffer, (COORD) { (screen[0] / 2) - (short)(wcslen(text[arrowIndex]) / 2) - 3, startIndexCoord + (arrowIndex * 2) + 2 }, L'>');
	putWCharToBuffer(buffer, (COORD) { (screen[0] / 2) + (short)(wcslen(text[arrowIndex]) / 2) + 2, startIndexCoord + (arrowIndex * 2) + 2 }, L'<');

	if (currentScreenState == PRE_SETTINGS) printBufferAnimation(buffer, DIRECTION_UP, SETTINGS);
	else printBackBufferToScreen();

	return;
}

#pragma endregion

#pragma region function printGraphScreen

static void printGraphScreen() {

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
	EGraphScreen.X = (lf[]) { graphScreen.X, 0.0, 0.0 };
	EGraphScreen.Y = (lf[]){ graphScreen.Y , 0.0, 0.0 };
	EnumberAdjust(EGraphScreen.X, 3);
	EnumberAdjust(EGraphScreen.Y, 3);

	lf width[3], height[3];
	lf xGap[3], yGap[3];
	lf t[3];
	EnumberSubtract(maxCoord->X, minCoord->X, width);
	EnumberSubtract(EGraphScreen.X, (lf[]) { 1.0, 0.0, 0.0 }, t);
	EnumberDivide(width, t, xGap);
	EnumberSubtract(maxCoord->Y, minCoord->Y, height);
	EnumberSubtract(EGraphScreen.Y, (lf[]) { 1.0, 0.0, 0.0 }, t);
	EnumberDivide(height, t, yGap);

	lf screenDegree[3];
	EnumberDivide(height, width, screenDegree);
	lf degree[4][3] = { {0.0,}, };
	lf divide[4] = { -2.0, -0.5, 0.5, 2.0 };
	for (int i = 0; i < 4; i++) {
		EnumberMultiply(screenDegree, (lf[]) { divide[i], 0.0, 0.0 }, degree[i]);
	}

	for (lld i = 0; i < (lld)graphScreen.X; i++) {
		lf xval[3];
		EnumberMultiply(xGap, (lf[]) { (lf)i, 0.0, 0.0 }, xval);
		EnumberAdd(minCoord->X, xval, xval);

		lf yval[3];
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

		lf incline[3];
		calculateFunction(currentFunction->derivative, currentFunction->degree - 1, xval, incline);
		wchar_t ch;
		if (EnumberCompare(incline, degree[0]) == 1) ch = L'│';
		else if (EnumberCompare(incline, degree[1]) == 1) ch = L'\\';
		else if (EnumberCompare(incline, degree[2]) == 1) ch = L'─';
		else if (EnumberCompare(incline, degree[3]) == 1) ch = L'/';
		else ch = L'│';
		if (llabs(i - (lld)graphScreen.X / 2) <= 1) {
			putCharToBuffer(buffer, (COORD) { (short)startCoord.X + 5 + (short)lfdot.X, (short)startCoord.Y + (short)lfdot.Y }, \
				wcharToColoredUnicode(ch, (bool[]) { 1, 0, 1, 0 }, (bool[]) { 0, 0, 0, 0 }));
		}
		else putWCharToBuffer(buffer, (COORD) { (short)startCoord.X + 5 + (short)lfdot.X, (short)startCoord.Y + (short)lfdot.Y }, ch);
	}

	printCoordinatePlane(buffer);

	if (currentScreenState == PRE_GRAPH) printBufferAnimation(buffer, DIRECTION_RIGHT, GRAPH);
	else printBackBufferToScreen();
	return;
}

#pragma endregion

#pragma region function printStoreScreen

static void printStoreScreen() {
	if (!animationFrame) currentScreenState = STORE;

	CHAR_INFO* buffer = screenBuffer;
	if (currentScreenState == PRE_STORE) buffer = tempScreenBuffer;

	resetBuffer(buffer);

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

	if (currentScreenState == PRE_STORE) printBufferAnimation(buffer, DIRECTION_DOWN, STORE);
	else printBackBufferToScreen();

	return;
}

#pragma endregion

#pragma region function printCoordinatePlane

static void printCoordinatePlane(CHAR_INFO* buffer) {

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
		putWStringToBuffer(buffer, (COORD) { width* (i + 1) / (n + 1) - 2, (short)endCoord.Y - 1 }, (wchar_t*)wcEnumber);
		putWCharToBuffer(buffer, (COORD) { width* (i + 1) / (n + 1), (short)endCoord.Y - 2 }, (wchar_t)L'┼');
	}

	EnumberSubtract(currentFunction->maxCoord->Y, currentFunction->minCoord->Y, value);
	for (int i = 0; i < n; i++) {
		EnumberMultiply(value, (lf[]) { ((lf)(i + 1) / (n + 1)), 0.0, 0.0 }, t);
		EnumberSubtract(currentFunction->maxCoord->Y, t, t);
		EnumberToChar(t, Enumber);
		mbstowcs(wcEnumber, Enumber, 6);
		putWStringToBuffer(buffer, (COORD) { (short)startCoord.X, height* (i + 1) / (n + 1) }, (wchar_t*)wcEnumber);
	}

	return;
}

#pragma endregion

#pragma region function printBackBufferToScreen

static void printBackBufferToScreen() {

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

	WriteConsoleOutputW(
		hBackBuffer,
		cloneBuffer,
		(COORD) {
		screen[0], screen[1]
	},
		bufferCoord,
		& region
	);

	free(cloneBuffer);
	SetConsoleActiveScreenBuffer(hBackBuffer);
	swap(&hOut, &hBackBuffer);

	return;
}

#pragma endregion

#pragma region function printBufferAnimation

static void printBufferAnimation(CHAR_INFO* tempScreenBuffer, short direction, int afterScreenState) {

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
					putCharToBuffer(screenBuffer, (COORD) { (SHORT)x, (SHORT)paste }, ci);

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
					putCharToBuffer(screenBuffer, (COORD) { (SHORT)paste, (SHORT)y }, ci);
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