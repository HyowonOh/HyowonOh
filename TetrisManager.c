#include <stdio.h>
#include <process.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include "TetrisManager.h"
#include "Util.h"
#include "Constant.h"

#define MAX_MAKE_OBSTACLE_ONE_LINE_COUNT 2
#define MILLI_SECONDS_PER_SECOND 1000
#define INITIAL_SPEED 300
#define SPEED_LEVEL_OFFSET 40
#define LEVELP_UP_CONDITION 3
#define STATUS_POSITION_X_TO_PRINT 38
#define STATUS_POSITION_Y_TO_PRINT 1

#define LINES_TO_DELETE_HIGHTING_COUNT 3
#define LINES_TO_DELETE_HIGHTING_MILLISECOND 100

#define BOARD_TYPES_TO_PRINT_ROW_SIZE 12
#define BOARD_TYPES_TO_PRINT_COL_SIZE 3

static const char boardTypesToPrint[BOARD_TYPES_TO_PRINT_ROW_SIZE][BOARD_TYPES_TO_PRINT_COL_SIZE] = {
	("  "), ("■"), ("▩"), ("□"), ("┃"), ("┃"), ("━"), ("━"), ("┏"), ("┓"), ("┗"), ("┛")
};

static void _TetrisManager_PrintStatus(TetrisManager* tetrisManager, int x, int y);
static void _TetrisManager_PrintKeys(TetrisManager* tetrisManager, int x, int y);
static void _TetrisManager_PrintBlock(TetrisManager* tetrisManager, int blockType, int status);
static void _TetrisManager_InitBoard(TetrisManager* tetrisManager);
static void _TetrisManager_UpSpeedLevel(TetrisManager* tetrisManager);
static void _TetrisManager_SearchLineIndexesToDelete(TetrisManager* tetrisManager, int* indexes, int* count);
static void _TetrisManager_DeleteLines(TetrisManager* tetrisManager, int* indexes, int count);
static void _TetrisManager_HighlightLinesToDelete(TetrisManager* tetrisManager, int* indexes, int count);
static Block _TetrisManager_GetBlockByType(TetrisManager* tetrisManager, int blockType);
static void _TetrisManager_MakeShadow(TetrisManager* tetrisManager);
static int _TetrisManager_CheckValidPosition(TetrisManager* tetrisManager, int blockType, int direction);
static void _TetrisManager_ChangeBoardByDirection(TetrisManager* tetrisManager, int blockType, int direction);
static void _TetrisManager_ChangeBoardByStatus(TetrisManager* tetrisManager, int blockType, int status);
//테스트->
static void _TetrisManager_LevelMap(TetrisManager* tetrisManager, int MapLevel, int status);
//테스트<-
static DWORD WINAPI _TetrisManager_OnTotalTimeThreadStarted(void *tetrisManager);
static void _TetrisManager_PrintTotalTime(TetrisManager tetrisManager);
static void _TetrisManager_MakeObstacleOneLine(TetrisManager* tetrisManager);
// 민지꺼테스트
static void _TetrisManager_RandomSpeed(TetrisManager* tetrisManager, int speedLevel);
static void _TetrisManager_MakeFog(TetrisManager* tetrisManager,int blockType, int mode);
// 민지꺼여기까지
void TetrisManager_Init(TetrisManager* tetrisManager, int speedLevel){
	Block block;
	block.current = -1;
	_TetrisManager_InitBoard(tetrisManager, speedLevel); //변수 speedLevel추가*****************************************************************************************
	tetrisManager->block = Block_Make(True, block);
	tetrisManager->shadow = tetrisManager->block;
	tetrisManager->isHoldAvailable = True;
	_TetrisManager_MakeShadow(tetrisManager);
	tetrisManager->deletedLineCount = 0;
	tetrisManager->speedLevel = speedLevel;
	tetrisManager->score = 0;
	tetrisManager->totalTimeThread = NULL;
	tetrisManager->totalTime = 0;
	tetrisManager->isTotalTimeAvailable = False;
}

void TetrisManager_ProcessDirection(TetrisManager* tetrisManager, int direction){
	if (direction != DOWN){
		_TetrisManager_PrintBlock(tetrisManager, SHADOW_BLOCK, EMPTY);
		_TetrisManager_ChangeBoardByStatus(tetrisManager, SHADOW_BLOCK, EMPTY);
	}
	_TetrisManager_PrintBlock(tetrisManager, MOVING_BLOCK, EMPTY);
	_TetrisManager_ChangeBoardByDirection(tetrisManager, MOVING_BLOCK, direction);
	_TetrisManager_PrintBlock(tetrisManager, MOVING_BLOCK, MOVING_BLOCK);
	if (direction != DOWN){
		_TetrisManager_MakeShadow(tetrisManager);
	}
}

void TetrisManager_ProcessAuto(TetrisManager* tetrisManager){
	_TetrisManager_PrintBlock(tetrisManager, MOVING_BLOCK, EMPTY);
	_TetrisManager_ChangeBoardByDirection(tetrisManager, MOVING_BLOCK, DOWN);
	_TetrisManager_PrintBlock(tetrisManager, MOVING_BLOCK, MOVING_BLOCK);
}

void TetrisManager_ProcessDirectDown(TetrisManager* tetrisManager){
	_TetrisManager_PrintBlock(tetrisManager, MOVING_BLOCK, EMPTY);
	while (!TetrisManager_IsReachedToBottom(tetrisManager, MOVING_BLOCK)){
		_TetrisManager_ChangeBoardByDirection(tetrisManager, MOVING_BLOCK, DOWN);
	}
	_TetrisManager_PrintBlock(tetrisManager, MOVING_BLOCK, MOVING_BLOCK);
}

void TetrisManager_ProcessDeletingLines(TetrisManager* tetrisManager){
	int indexes[BOARD_ROW_SIZE];
	int count;

	// use temp size (magic number)
	int x = 38;
	int y = 1;

	_TetrisManager_SearchLineIndexesToDelete(tetrisManager, indexes, &count);
	if (count > 0){

		//during hightlighting the lines to delete, hide moving block and shadow block
		_TetrisManager_PrintBlock(tetrisManager, SHADOW_BLOCK, EMPTY);
		_TetrisManager_PrintBlock(tetrisManager, MOVING_BLOCK, EMPTY);
		_TetrisManager_HighlightLinesToDelete(tetrisManager, indexes, count);
		_TetrisManager_DeleteLines(tetrisManager, indexes, count);
		_TetrisManager_ChangeBoardByStatus(tetrisManager, MOVING_BLOCK, MOVING_BLOCK);
		TetrisManager_PrintBoard(tetrisManager);
		_TetrisManager_PrintStatus(tetrisManager, x, y);
	}
}

int TetrisManager_IsReachedToBottom(TetrisManager* tetrisManager, int blockType){
	int i;
	Block block = _TetrisManager_GetBlockByType(tetrisManager, blockType);
	for (i = 0; i < POSITIONS_SIZE; i++){
		int x = Block_GetPositions(block)[i].x;
		int y = Block_GetPositions(block)[i].y;
		if (!(tetrisManager->board[x + 1][y] == EMPTY || tetrisManager->board[x + 1][y] == MOVING_BLOCK || tetrisManager->board[x + 1][y] == SHADOW_BLOCK)){
			return True;
		}
	}
	return False;
}

int TetrisManager_ProcessReachedCase(TetrisManager* tetrisManager){

	// use temp size (magic number)
	int x = 40;
	int y = 15;

	// if this variable equals to 
	static int makeObstacleOneLineCount = 0;

	_TetrisManager_PrintBlock(tetrisManager, SHADOW_BLOCK, EMPTY);
	_TetrisManager_ChangeBoardByStatus(tetrisManager, SHADOW_BLOCK, EMPTY);
	_TetrisManager_PrintBlock(tetrisManager, MOVING_BLOCK, EMPTY);
	_TetrisManager_ChangeBoardByStatus(tetrisManager, MOVING_BLOCK, FIXED_BLOCK);
	_TetrisManager_PrintBlock(tetrisManager, FIXED_BLOCK, FIXED_BLOCK);
	tetrisManager->block = Block_Make(False, tetrisManager->block);
	_TetrisManager_PrintBlock(tetrisManager, MOVING_BLOCK, MOVING_BLOCK);
	_TetrisManager_MakeShadow(tetrisManager);
	if (makeObstacleOneLineCount == MAX_MAKE_OBSTACLE_ONE_LINE_COUNT){
		if (tetrisManager->speedLevel == MAX_SPEED_LEVEL){
			_TetrisManager_MakeObstacleOneLine(tetrisManager);
		}
		makeObstacleOneLineCount = 0;
	}
	else{
		makeObstacleOneLineCount++;
	}
	Block_PrintNext(tetrisManager->block, 0, x, y);
	x += 20;
	Block_PrintNext(tetrisManager->block, 1, x, y);
	tetrisManager->isHoldAvailable = True;
	if (TetrisManager_IsReachedToBottom(tetrisManager, MOVING_BLOCK)){
		Block_Destroy(tetrisManager->block);
		return END;
	}
	else{
		return PLAYING;
	}
}

void TetrisManager_PrintBoard(TetrisManager* tetrisManager){
	int i;
	int j;
	int x = 0;
	int y = 0;
	for (i = 0; i < BOARD_ROW_SIZE; i++){
		CursorUtil_GotoXY(x, y++);
		for (j = 0; j < BOARD_COL_SIZE; j++){
			switch (tetrisManager->board[i][j]){
			case LEFT_TOP_EDGE: case RIGHT_TOP_EDGE: case LEFT_BOTTOM_EDGE: case RIGHT_BOTTOM_EDGE:
			case LEFT_WALL: case RIGHT_WALL: case TOP_WALL: case BOTTOM_WALL:
			case EMPTY:
				FontUtil_ChangeFontColor(DEFAULT_FONT_COLOR);
				break;
			case MOVING_BLOCK:
				FontUtil_ChangeFontColor(tetrisManager->block.color);
				break;
			case FIXED_BLOCK:
				FontUtil_ChangeFontColor(WHITE);
				break;
			case SHADOW_BLOCK:
				FontUtil_ChangeFontColor(GRAY);
				break;
			}
			printf("%s", boardTypesToPrint[tetrisManager->board[i][j]]);
			FontUtil_ChangeFontColor(DEFAULT_FONT_COLOR);
		}
	}
}

void TetrisManager_PrintDetailInfomation(TetrisManager* tetrisManager){
	int x = STATUS_POSITION_X_TO_PRINT;
	int y = STATUS_POSITION_Y_TO_PRINT;
	_TetrisManager_PrintStatus(tetrisManager, x, y);
	x += 6;
	y += 4;
	_TetrisManager_PrintKeys(tetrisManager, x, y);
	x -= 4;
	y += 10;
	Block_PrintNext(tetrisManager->block, 0, x, y);
	x += 20;
	Block_PrintNext(tetrisManager->block, 1, x, y);
	y += 5;
	Block_PrintHold(tetrisManager->block, x, y);
	TetrisManager_StartTotalTime(tetrisManager);
	_TetrisManager_PrintTotalTime(*tetrisManager);
}

DWORD TetrisManager_GetDownMilliSecond(TetrisManager* tetrisManager){
	int i;
	DWORD milliSecond = INITIAL_SPEED;
	for (i = MIN_SPEED_LEVEL; i < tetrisManager->speedLevel; i++){
		if (i < MAX_SPEED_LEVEL / 2){
			milliSecond -= SPEED_LEVEL_OFFSET;
		}
		else{
			milliSecond -= (SPEED_LEVEL_OFFSET / 5);
		}
	}
	return milliSecond;
}

void TetrisManager_MakeHold(TetrisManager* tetrisManager){

	// use temp size (magic number)
	int x = 60;
	int y = 20;

	if (tetrisManager->isHoldAvailable){
		_TetrisManager_PrintBlock(tetrisManager, MOVING_BLOCK, EMPTY);
		_TetrisManager_ChangeBoardByStatus(tetrisManager, MOVING_BLOCK, EMPTY);
		_TetrisManager_PrintBlock(tetrisManager, SHADOW_BLOCK, EMPTY);
		_TetrisManager_ChangeBoardByStatus(tetrisManager, SHADOW_BLOCK, EMPTY);
		Block_ChangeCurrentForHold(&tetrisManager->block);
		tetrisManager->isHoldAvailable = False;
		Block_PrintHold(tetrisManager->block, x, y);
		_TetrisManager_PrintBlock(tetrisManager, MOVING_BLOCK, MOVING_BLOCK);
		_TetrisManager_MakeShadow(tetrisManager);
	}
}

void TetrisManager_StartTotalTime(TetrisManager* tetrisManager){
	DWORD totalTimeThreadID;
	tetrisManager->isTotalTimeAvailable = True;
	tetrisManager->totalTimeThread = (HANDLE)_beginthreadex(NULL, 0, _TetrisManager_OnTotalTimeThreadStarted, tetrisManager, 0, (unsigned *)&totalTimeThreadID);
}

void TetrisManager_PauseTotalTime(TetrisManager* tetrisManager){
	tetrisManager->isTotalTimeAvailable = False;
	tetrisManager->totalTime--; // to show not one added time but paused time
}

void TetrisManager_StopTotalTime(TetrisManager* tetrisManager){
	tetrisManager->isTotalTimeAvailable = False;
	tetrisManager->totalTime = 0;
}

static void _TetrisManager_PrintStatus(TetrisManager* tetrisManager, int x, int y){
	ScreenUtil_ClearRectangle(x + 2, y + 1, 4, 1); // use temp size (magic number)
	ScreenUtil_ClearRectangle(x + 13, y + 1, 6, 1); // use temp size (magic number)
	ScreenUtil_ClearRectangle(x + 26, y + 1, 12, 1); // use temp size (magic number)
	CursorUtil_GotoXY(x, y++);
	printf("┏ Lv ┓   ┏ Line ┓   ┏ TotalScore ┓");
	CursorUtil_GotoXY(x, y++);
	printf("┃%3d ┃   ┃%4d  ┃   ┃%7d     ┃", tetrisManager->speedLevel, tetrisManager->deletedLineCount, tetrisManager->score);
	CursorUtil_GotoXY(x, y++);
	printf("┗━━┛   ┗━━━┛   ┗━━━━━━┛");
}

static void _TetrisManager_PrintKeys(TetrisManager* tetrisManager, int x, int y){
	ScreenUtil_ClearRectangle(x, y, 26, 9); // use temp size (magic number)
	CursorUtil_GotoXY(x, y++);
	printf("┏━━━━ Keys ━━━━┓");
	CursorUtil_GotoXY(x, y++);
	printf("┃←       ┃move left  ┃");
	CursorUtil_GotoXY(x, y++);
	printf("┃→       ┃move right ┃");
	CursorUtil_GotoXY(x, y++);
	printf("┃↓       ┃move down  ┃");
	CursorUtil_GotoXY(x, y++);
	printf("┃↑       ┃rotate     ┃");
	CursorUtil_GotoXY(x, y++);
	printf("┃SpaceBar ┃direct down┃");
	CursorUtil_GotoXY(x, y++);
	printf("┃ESC      ┃pause      ┃");
	CursorUtil_GotoXY(x, y++);
	printf("┃L (l)    ┃hold       ┃");
	CursorUtil_GotoXY(x, y++);
	printf("┗━━━━━━━━━━━┛");
}

static void _TetrisManager_PrintBlock(TetrisManager* tetrisManager, int blockType, int status){
	int i;
	Block block = _TetrisManager_GetBlockByType(tetrisManager, blockType);
	switch (blockType){
	case MOVING_BLOCK:
		FontUtil_ChangeFontColor(tetrisManager->block.color);
		break;
	case FIXED_BLOCK:
		FontUtil_ChangeFontColor(WHITE);
		break;
	case SHADOW_BLOCK:
		FontUtil_ChangeFontColor(GRAY);
		break;
	}
	for (i = 0; i < POSITIONS_SIZE; i++){
		int x = Block_GetPositions(block)[i].x;
		int y = Block_GetPositions(block)[i].y;
		CursorUtil_GotoXY(2 * y, x);
		printf("%s", boardTypesToPrint[status]);
	}
	FontUtil_ChangeFontColor(DEFAULT_FONT_COLOR);
	_TetrisManager_PrintTotalTime(*tetrisManager); // because of multi thread problem, this function covers total time
}

static void _TetrisManager_InitBoard(TetrisManager* tetrisManager, int MapLevel){
	int i;
	memset(tetrisManager->board, EMPTY, sizeof(char)* BOARD_ROW_SIZE * BOARD_COL_SIZE);
	for (i = 0; i < BOARD_ROW_SIZE; i++){
		tetrisManager->board[i][0] = LEFT_WALL;
		tetrisManager->board[i][BOARD_COL_SIZE - 1] = RIGHT_WALL;
	}
	for (i = 0; i < BOARD_COL_SIZE; i++){
		tetrisManager->board[0][i] = TOP_WALL;
		tetrisManager->board[BOARD_ROW_SIZE - 1][i] = BOTTOM_WALL;
	}

	//in order to make center hole at top wall, we convert center top wall into empty intentionally
	tetrisManager->board[0][(BOARD_COL_SIZE - 2) / 2 - 1] = EMPTY;
	tetrisManager->board[0][(BOARD_COL_SIZE - 2) / 2] = EMPTY;
	tetrisManager->board[0][(BOARD_COL_SIZE - 2) / 2 + 1] = EMPTY;
	tetrisManager->board[0][(BOARD_COL_SIZE - 2) / 2 + 2] = EMPTY;

	tetrisManager->board[0][0] = LEFT_TOP_EDGE;
	tetrisManager->board[0][BOARD_COL_SIZE - 1] = RIGHT_TOP_EDGE;
	tetrisManager->board[BOARD_ROW_SIZE - 1][0] = LEFT_BOTTOM_EDGE;
	tetrisManager->board[BOARD_ROW_SIZE - 1][BOARD_COL_SIZE - 1] = RIGHT_BOTTOM_EDGE;
	//테스트****************************************************************************************************************************************************8
	_TetrisManager_LevelMap(tetrisManager, MapLevel, FIXED_BLOCK);
}

//여기부터 테스트**************************************************************************************************************************************
static void _TetrisManager_LevelMap(TetrisManager* tetrisManager, int MapLevel, int status){
	int i, j, k;

	switch(MapLevel){
	case 1 :
		break;
	case 2:
		for(i=17; i<=22; i++){
			for(j=1; j<=4; j++)
				tetrisManager->board[i][j]=status;
			for(j=7; j<=12; j++)
				tetrisManager->board[i][j]=status;
		}
		break;
	case 3:
		for(j=2; j<=11; j++)
			tetrisManager->board[22][j]=status;
		for(j=3; j<=10; j++)
			tetrisManager->board[21][j]=status;
		for(j=4; j<=9; j++)
			tetrisManager->board[20][j]=status;
		for(j=5; j<=8; j++)
			tetrisManager->board[19][j]=status;
		for(j=6; j<=7; j++)
			tetrisManager->board[18][j]=status;
		break;
	case 4:
		for(i=17; i<=22; i++){
			for(j=1; j<=4; j++)
				tetrisManager->board[i][j]=status;
			for(j=9; j<=12; j++)
				tetrisManager->board[i][j]=status;
		}
		tetrisManager->board[17][5]=status;
		tetrisManager->board[17][8]=status;
		tetrisManager->board[22][5]=status;
		tetrisManager->board[22][8]=status;
		break;
	case 5:
		for(j=2; j<=12; j++)
			tetrisManager->board[22][j]=status;
		for(j=3; j<=12; j++)
			tetrisManager->board[21][j]=status;
		for(j=4; j<=12; j++)
			tetrisManager->board[20][j]=status;
		for(j=5; j<=12; j++)
			tetrisManager->board[19][j]=status;
		for(j=6; j<=12; j++)
			tetrisManager->board[18][j]=status;
		for(j=7; j<=12; j++)
			tetrisManager->board[17][j]=status;
		for(j=8; j<=12; j++)
			tetrisManager->board[16][j]=status;
		for(j=9; j<=12; j++)
			tetrisManager->board[15][j]=status;
		for(j=10; j<=12; j++)
			tetrisManager->board[14][j]=status;
		for(j=11; j<=12; j++)
			tetrisManager->board[13][j]=status;
		tetrisManager->board[12][12]=status;
		break;
	case 6:
		for(i=17; i<=18; i++){
			tetrisManager->board[i][2]=status;
			tetrisManager->board[i][11]=status;
		}
		for(i=16; i<=19; i++){
			tetrisManager->board[i][3]=status;
			tetrisManager->board[i][10]=status;
		}
		for(i=16; i<=20; i++){
			tetrisManager->board[i][4]=status;
			tetrisManager->board[i][9]=status;
		}
		for(i=17; i<=21; i++){
			tetrisManager->board[i][5]=status;
			tetrisManager->board[i][8]=status;
		}
		for(i=18; i<=22; i++){
			tetrisManager->board[i][6]=status;
			tetrisManager->board[i][7]=status;
		}
		break;
	case 7:
		for(j=1; j<=11; j++)
			tetrisManager->board[12][j]=status;
		for(j=1; j<=10; j++)
			tetrisManager->board[13][j]=status;
		for(j=1; j<=9; j++)
			tetrisManager->board[14][j]=status;
		for(j=1; j<=8; j++)
			tetrisManager->board[15][j]=status;
		for(j=1; j<=7; j++)
			tetrisManager->board[16][j]=status;
		for(j=1; j<=6; j++)
			tetrisManager->board[17][j]=status;
		for(j=1; j<=5; j++)
			tetrisManager->board[18][j]=status;
		for(j=1; j<=4; j++)
			tetrisManager->board[19][j]=status;
		for(j=1; j<=3; j++)
			tetrisManager->board[20][j]=status;
		for(j=1; j<=2; j++)
			tetrisManager->board[21][j]=status;
		tetrisManager->board[22][1]=status;
		break;
	case 8:
		for(i=14; i<=22; i++){
			j=26-i;
			tetrisManager->board[i][j]=status;
		}
		for(i=16; i<=22; i++){
			j=28-i;
			tetrisManager->board[i][j]=status;
		}
		for(i=18; i<=22; i++){
			j=30-i;
			tetrisManager->board[i][j]=status;
		}
		for(i=20; i<=22; i++){
			j=32-i;
			tetrisManager->board[i][j]=status;
		}
		tetrisManager->board[22][12]=status;
		break;
	case 9:
		for(j=2; j<=3; j++)
			tetrisManager->board[12][j]=status;
		for(j=1; j<=5; j++)
			tetrisManager->board[13][j]=status;
		tetrisManager->board[14][1]=status;
		for(j=5; j<=6; j++)
			tetrisManager->board[14][j]=status;
		for(j=6; j<=7; j++)
			tetrisManager->board[15][j]=status;
		for(i=14; i<=19; i++)
			tetrisManager->board[i][4]=status;
		tetrisManager->board[18][1]=status;
		for(j=8; j<=11; j++)
			tetrisManager->board[18][j]=status;
		for(j=1; j<=2; j++)
			tetrisManager->board[19][j]=status;
		for(j=7; j<=12; j++)
			tetrisManager->board[19][j]=status;
		for(j=1; j<=8; j++)
			tetrisManager->board[20][j]=status;
		for(j=10; j<=12; j++)
			tetrisManager->board[20][j]=status;
		for(j=1; j<=7; j++)
			tetrisManager->board[21][j]=status;
		tetrisManager->board[21][9]=status;
		for(j=11; j<=12; j++)
			tetrisManager->board[21][j]=status;
		for(j=1; j<=8; j++)
			tetrisManager->board[22][j]=status;
		for(j=10; j<=12; j++)
			tetrisManager->board[22][j]=status;
		break;
	case 10:
		srand((unsigned int)time(NULL));
		for(k=1; k<=95; k++){
			i=22-(rand()%9);
			j=12-(rand()%12);
			tetrisManager->board[i][j]=status;
		}
		break;
	}
}
//여기까지*********************************************************************************************************************************************

static void _TetrisManager_UpSpeedLevel(TetrisManager* tetrisManager){
	if (tetrisManager->speedLevel < MAX_SPEED_LEVEL){
		tetrisManager->speedLevel++;
	}
}

static void _TetrisManager_SearchLineIndexesToDelete(TetrisManager* tetrisManager, int* indexes, int* count){
	int i;
	int j;
	int toDelete;
	memset(indexes, -1, sizeof(int)* (BOARD_ROW_SIZE - 2));
	*count = 0;
	for (i = 1; i < BOARD_ROW_SIZE - 1; i++){
		toDelete = True;
		for (j = 1; j < BOARD_COL_SIZE - 1; j++){
			if (tetrisManager->board[i][j] != FIXED_BLOCK){
				toDelete = False;
				break;
			}
		}
		if (toDelete){
			indexes[(*count)++] = i;
		}
	}
}

static void _TetrisManager_DeleteLines(TetrisManager* tetrisManager, int* indexes, int count){
	int i;
	int j;
	int k = BOARD_ROW_SIZE - 2;
	int toDelete;
	char temp[BOARD_ROW_SIZE][BOARD_COL_SIZE] = { EMPTY, };
	for (i = BOARD_ROW_SIZE - 2; i > 0; i--){
		toDelete = False;
		for (j = 0; j < BOARD_COL_SIZE; j++){
			if (i == indexes[j]){
				toDelete = True;
				break;
			}
		}
		if (!toDelete){
			for (j = 0; j < BOARD_COL_SIZE; j++){
				temp[k][j] = tetrisManager->board[i][j];
			}
			k--;
		}
	}
	for (i = 1; i < BOARD_ROW_SIZE - 1; i++){
		for (j = 1; j < BOARD_COL_SIZE - 1; j++){
			tetrisManager->board[i][j] = temp[i][j];
		}
	}
	for (i = 0; i < count; i++){
		tetrisManager->shadow = Block_Move(tetrisManager->shadow, DOWN); // lower shadow block by deleted count
		tetrisManager->score += tetrisManager->speedLevel * 100;
		tetrisManager->deletedLineCount++;
		if (tetrisManager->deletedLineCount % LEVELP_UP_CONDITION == 0){
			_TetrisManager_UpSpeedLevel(tetrisManager);
		}
	}
}

static void _TetrisManager_HighlightLinesToDelete(TetrisManager* tetrisManager, int* indexes, int count){
	int i;
	int j;
	int k;
	for (i = 0; i < LINES_TO_DELETE_HIGHTING_COUNT; i++){
		FontUtil_ChangeFontColor(JADE);
		Sleep(LINES_TO_DELETE_HIGHTING_MILLISECOND);
		for (j = 0; j < count; j++){
			CursorUtil_GotoXY(2, indexes[j]);
			for (k = 0; k < BOARD_COL_SIZE - 2; k++){
				printf("▧");
			}
		}
		FontUtil_ChangeFontColor(DEFAULT_FONT_COLOR);
		Sleep(LINES_TO_DELETE_HIGHTING_MILLISECOND);
		for (j = 0; j < count; j++){
			CursorUtil_GotoXY(2, indexes[j]);
			for (k = 0; k < BOARD_COL_SIZE - 2; k++){
				printf("  ");
			}
		}
	}
}

static Block _TetrisManager_GetBlockByType(TetrisManager* tetrisManager, int blockType){
	if (blockType == SHADOW_BLOCK){
		return tetrisManager->shadow;
	}
	else{
		return tetrisManager->block;
	}
}

static void _TetrisManager_MakeShadow(TetrisManager* tetrisManager){
	tetrisManager->shadow = tetrisManager->block;
	while (!TetrisManager_IsReachedToBottom(tetrisManager, SHADOW_BLOCK)){
		_TetrisManager_ChangeBoardByDirection(tetrisManager, SHADOW_BLOCK, DOWN);
	}
	_TetrisManager_PrintBlock(tetrisManager, SHADOW_BLOCK, SHADOW_BLOCK);
}

static int _TetrisManager_CheckValidPosition(TetrisManager* tetrisManager, int blockType, int direction){
	Block temp = Block_Move(_TetrisManager_GetBlockByType(tetrisManager, blockType), direction);
	int i;
	int x;
	int y;
	for (i = 0; i < POSITIONS_SIZE; i++){
		x = Block_GetPositions(temp)[i].x;

		//but now, x == 0 is empty
		//originally, x == 0 is top wall
		//because we convert the center top wall into empty intentionally
		if (blockType == MOVING_BLOCK && x == 0){
			return TOP_WALL;
		}
		y = Block_GetPositions(temp)[i].y;
		if (!(tetrisManager->board[x][y] == EMPTY || tetrisManager->board[x][y] == MOVING_BLOCK || tetrisManager->board[x][y] == SHADOW_BLOCK)){
			return tetrisManager->board[x][y];
		}
	}
	return EMPTY;
}

static void _TetrisManager_ChangeBoardByDirection(TetrisManager* tetrisManager, int blockType, int direction){
	int tempDirection = DOWN;
	int tempCheckResult = EMPTY;
	int checkResult;
	_TetrisManager_ChangeBoardByStatus(tetrisManager, blockType, EMPTY);
	checkResult = _TetrisManager_CheckValidPosition(tetrisManager, blockType, direction);
	if (checkResult == EMPTY){
		if (blockType == MOVING_BLOCK){
			tetrisManager->block = Block_Move(tetrisManager->block, direction);
		}
		else if (blockType == SHADOW_BLOCK){
			tetrisManager->shadow = Block_Move(tetrisManager->shadow, direction);
		}
	}
	else{
		if ((direction == UP || direction == LEFT || direction == RIGHT) && checkResult != FIXED_BLOCK){
			if (checkResult == TOP_WALL){
				tempDirection = DOWN;
				tempCheckResult = TOP_WALL;
			}
			else if (checkResult == RIGHT_WALL){
				tempDirection = LEFT;
				tempCheckResult = RIGHT_WALL;
			}
			else if (checkResult == LEFT_WALL){
				tempDirection = RIGHT;
				tempCheckResult = LEFT_WALL;
			}
			do{
				tetrisManager->block = Block_Move(tetrisManager->block, tempDirection);
			} while (_TetrisManager_CheckValidPosition(tetrisManager, MOVING_BLOCK, direction) == tempCheckResult);
			tetrisManager->block = Block_Move(tetrisManager->block, direction);
		}
	}
	_TetrisManager_ChangeBoardByStatus(tetrisManager, blockType, blockType);
}

static void _TetrisManager_ChangeBoardByStatus(TetrisManager* tetrisManager, int blockType, int status){
	int i;
	Block block = _TetrisManager_GetBlockByType(tetrisManager, blockType);
	for (i = 0; i < POSITIONS_SIZE; i++){
		int x = Block_GetPositions(block)[i].x;
		int y = Block_GetPositions(block)[i].y;
		tetrisManager->board[x][y] = status;
	}
}

static DWORD WINAPI _TetrisManager_OnTotalTimeThreadStarted(void *tetrisManager){
	while (True){
		if (!((TetrisManager*)tetrisManager)->isTotalTimeAvailable){
			break;
		}
		Sleep(MILLI_SECONDS_PER_SECOND);
		((TetrisManager*)tetrisManager)->totalTime++;
	}
	return 0;
}

static void _TetrisManager_PrintTotalTime(TetrisManager tetrisManager){
	int hour = tetrisManager.totalTime / (60 * 60);
	int minute = tetrisManager.totalTime % (60 * 60) / 60;
	int second = tetrisManager.totalTime % 60;

	// use temp size (magic number)
	int x = 42;
	int y = 20;
	CursorUtil_GotoXY(x, y++);
	printf("┏  time  ┓");
	CursorUtil_GotoXY(x, y++);
	printf("┃%02d:%02d:%02d┃", hour, minute, second);
	CursorUtil_GotoXY(x, y++);
	printf("┗━━━━┛");
}

static void _TetrisManager_MakeObstacleOneLine(TetrisManager* tetrisManager){
	int i;
	int j;
	int isFixedBlock;
	int fixedBlockCount = 0;
	char temp[BOARD_ROW_SIZE][BOARD_COL_SIZE] = { EMPTY, };
	for (i = 1; i < BOARD_COL_SIZE - 1; i++){
		if (tetrisManager->board[1][i] == FIXED_BLOCK){
			return;
		}
	}
	srand((unsigned int)time(NULL));
	for (i = 1; i < BOARD_COL_SIZE - 1; i++){
		isFixedBlock = rand() % 2;
		fixedBlockCount += isFixedBlock;
		temp[BOARD_ROW_SIZE - 2][i] = isFixedBlock ? FIXED_BLOCK : EMPTY;
	}
	if (fixedBlockCount == BOARD_COL_SIZE - 2){
		temp[BOARD_ROW_SIZE - 2][rand() % (BOARD_COL_SIZE - 2) + 1] = EMPTY;
	}
	for (i = BOARD_ROW_SIZE - 2; i > 0; i--){
		for (j = 1; j < BOARD_COL_SIZE - 1; j++){
			temp[i - 1][j] = tetrisManager->board[i][j];
		}
	}
	for (i = 1; i < BOARD_ROW_SIZE - 1; i++){
		for (j = 1; j < BOARD_COL_SIZE - 1; j++){
			tetrisManager->board[i][j] = temp[i][j];
		}
	}
	_TetrisManager_MakeShadow(tetrisManager);
	TetrisManager_PrintBoard(tetrisManager);
}

// 민지꺼 여기부터 끝까지 ********************************************************************************************
static void _TetrisManager_RandomSpeed(TetrisManager* tetrisManager, int speedLevel)
{
   int i, t;
   scanf("%d");

   srand((unsigned int)time(NULL));
   (rand() % 4 - 3) + 4;

   if (t==3) {
      for (i = 0; i < 6; i++) {
         speedLevel += 2;
      }
   }
   else {
      for (i = 0; i < 6; i++) {
         speedLevel -= 2;
      }
   }
}


static void _TetrisManager_MakeFog(TetrisManager* tetrisManager, int blockType, int mode)
{
	if(mode==3) {
	int i,j;
	Block block = _TetrisManager_GetBlockByType(tetrisManager, blockType);
	int x = Block_GetPositions(block)[i].x;
	int y = Block_GetPositions(block)[i].y;
	int r = BOARD_ROW_SIZE;
	int c = BOARD_COL_SIZE;

	scanf("%d");

	CursorUtil_GotoXY(x,y);
	for (j = 2; j < r-2; j++) {
		printf("▧");
	}
   
	for (j = 2; j < c - 2; j++) {
		printf("▧");
	}
	}
}