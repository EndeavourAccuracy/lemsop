/* lemsop v0.8b (May 2020)
 * Copyright (C) 2019-2020 Norbert de Jonge <mail@norbertdejonge.nl>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see [ www.gnu.org/licenses/ ].
 *
 * To properly read this code, set your program's tab stop to: 2.
 */

/*========== Includes ==========*/
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <math.h> /*** For round() on WIN32. ***/
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h> /*** Maybe remove, and just change CHAR_BIT to 8? ***/
#include <string.h>
#include <ctype.h>
#if defined WIN32 || _WIN32 || WIN64 || _WIN64
#include <windows.h>
#undef PlaySound
#endif

#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_thread.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
/*========== Includes ==========*/

/*========== Defines ==========*/
#if defined WIN32 || _WIN32 || WIN64 || _WIN64
#define SLASH "\\"
#define DEVNULL "NUL"
#else
#define SLASH "/"
#define DEVNULL "/dev/null"
#endif

#define PNG_VARIOUS "png" SLASH "various" SLASH
#define PNG_BUTTONS "png" SLASH "buttons" SLASH
#define PNG_GAMEPAD "png" SLASH "gamepad" SLASH
#define PNG_DUNGEON "png" SLASH "dungeon" SLASH
#define PNG_PALACE "png" SLASH "palace" SLASH
#define PNG_ELEMENTS "png" SLASH "elements" SLASH
#define PNG_EXTRAS "png" SLASH "extras" SLASH
#define PNG_ROOMS "png" SLASH "rooms" SLASH
#define PNG_CHARS "png" SLASH "chars" SLASH

#define EXIT_NORMAL 0
#define EXIT_ERROR 1
#define EDITOR_NAME "lemsop"
#define EDITOR_VERSION "v0.8b (May 2020)"
#define COPYRIGHT "Copyright (C) 2020 Norbert de Jonge"
#define SMS_DIR "sms"
#define BACKUP SMS_DIR SLASH "backup.bak"
#define MAX_PATHFILE 300
#define MAX_OPTION 100
#define NR_LEVELS 14
#define ROOMS 24
#define COLS 16
#define ROWS 12
#define MAX_ELEM 10
#define WINDOW_WIDTH 562 /*** (COLS * 32) + 50 ***/
#define WINDOW_HEIGHT 459 /*** (ROWS * 32) + 75 ***/
#define MAX_IMG 200
#define MAX_CON 30
#define NUM_SOUNDS 20 /*** Sounds that may play at the same time. ***/
#define BAR_FULL 435
#define MAX_TEXT 100
#define REFRESH 25 /*** That is 40 frames per second, 1000/25. ***/
#define MAX_TOWRITE 720
#define ADJ_BASE_X 291
#define ADJ_BASE_Y 62
#define MAX_STATUS 100 /*** Cannot be more than MAX_TEXT. ***/
#define MAX_ERROR 200
#define TEXTS 16
#define MAX_DATA 720
#define MAX_GUARDS 37

#define VERIFY_OFFSET 0x7FF0
#define VERIFY_TEXT "TMR SEGA"
#define VERIFY_SIZE 8
#define OFFSET_PASSWORD 0x2FEF7

/*** https://stackoverflow.com/a/3208376 ***/
#define BYTE_TO_BINARY(byte) \
	(byte & 0x80 ? '1' : '0'), \
	(byte & 0x40 ? '1' : '0'), \
	(byte & 0x20 ? '1' : '0'), \
	(byte & 0x10 ? '1' : '0'), \
	(byte & 0x08 ? '1' : '0'), \
	(byte & 0x04 ? '1' : '0'), \
	(byte & 0x02 ? '1' : '0'), \
	(byte & 0x01 ? '1' : '0')

#ifndef O_BINARY
#define O_BINARY 0
#endif
/*========== Defines ==========*/

int iDebug;
char sPathFile[MAX_PATHFILE + 2];
int iNoAudio;
int iScale;
int iFullscreen;
int iStartLevel;
int iNoController;
int iBrokenRoomLinks;
int iDone[ROOMS + 2];
int iCurRoom;
SDL_Window *window;
unsigned int iWindowID;
SDL_Renderer *ascreen;
unsigned int iActiveWindowID;
SDL_Cursor *curArrow;
SDL_Cursor *curWait;
SDL_Cursor *curHand;
int iPreLoaded;
int iNrToPreLoad;
int iCurrentBarHeight;
TTF_Font *font11;
TTF_Font *font15;
TTF_Font *font20;
int iStatusBarFrame;
int iScreen;
int iXPos, iYPos;
int iOKOn;
int iChanged;
int iCurLevel;
char cCurType;
int iDownAt;
int iSelectedCol;
int iSelectedRow;
int iExtras;
int iLastTile;
int iCloseOn;
int iOnTileCol;
int iOnTileRow;
int iOnTileColOld;
int iOnTileRowOld;
int iNoAnim;
int iMovingRoom;
int iMovingNewBusy;
int iChangingBrokenRoom;
int iChangingBrokenSide;
int iRoomArray[ROOMS + 1 + 2][ROOMS + 2];
int iMovingOldX;
int iMovingOldY;
int iMovingNewX;
int iMovingNewY;
int iHelpOK;
int iInfo;
int arElementsCopyPaste[MAX_ELEM + 2][9 + 2];
int arTilesCopyPaste[COLS + 2][ROWS + 2];
int iCopied;
char sStatus[MAX_STATUS + 2], sStatusOld[MAX_STATUS + 2];
int iModified;
int iPlaytest;
int iYesOn, iNoOn;
char arToSave[4 + 2][8 + 2];
int iActiveElement;
int iActiveGuard;
int iFlameFrameDP;
int iEXESave;

/*** Levels ("The level table"). ***/
/*** (0x60C9) ***/
int arOffsetTG1[NR_LEVELS + 2];
int arOffsetTG2[NR_LEVELS + 2];
int arOffsetTiles[NR_LEVELS + 2];
char arLevelType[NR_LEVELS + 2];
int arKidDir[NR_LEVELS + 2];
int arKidPosX[NR_LEVELS + 2];
int arKidPosY[NR_LEVELS + 2];
int arKidRoom[NR_LEVELS + 2];
int arKidAnim[NR_LEVELS + 2];
int arUnknown[NR_LEVELS + 2][4 + 2];
/*** (address) ***/

/*** Room links and elements ("Room links and objects table"). ***/
/*** (chunk length) ***/
int arOffsetsElements[ROOMS + 2];
int iRoomConnections[NR_LEVELS + 2][ROOMS + 2][4 + 2];
int arElements[NR_LEVELS + 2][ROOMS + 2][MAX_ELEM + 2][9 + 2];

/*** Tiles ("Graphics map") ***/
int arDefinitions[64 + 2];
int arTiles[ROOMS + 2][COLS + 2][ROWS + 2];
unsigned char sEmpty[32 + 2];

/*** Guards ("Guard table") ***/
int arGuardR[MAX_GUARDS + 2];
int arGuardG[MAX_GUARDS + 2];
int arGuardB[MAX_GUARDS + 2];
int arGuardGameLevel[MAX_GUARDS + 2];
int arGuardHP[MAX_GUARDS + 2];
int arGuardDir[MAX_GUARDS + 2];
int arGuardMove[MAX_GUARDS + 2];

int arValue[12 + 2]; /*** 12 is OK ***/
int arOffsetRlElTable[NR_LEVELS + 2];
int iRoomConnectionsBroken[ROOMS + 2][4 + 2];
int iMinX, iMaxX, iMinY, iMaxY, iStartRoomsX, iStartRoomsY;

unsigned int gamespeed;
Uint32 looptime;

static const int iOffsetLevelTable = 0x6A7C;
static const int iOffsetGuardTable = 0x6CAD;
static const int iOffsetRlElTable = 0xAD8E;

unsigned char sOldPassword[6 + 2];
static const char *arPasswords[NR_LEVELS + 1] =
	{ "", "AAAAAA", "BJJHAK", "CJJHAL", "DJJHAM", "EJJHAN", "FJJHAO", "GJJHAP", "HJJHAQ", "IJJHAR", "JJJHAS", "KJJHAT", "LJJHAU", "MJJHAV", "NJJHAW" };

unsigned char arTextChars[TEXTS + 2][MAX_TEXT + 2];
int iTextActiveText, iTextActiveChar;
static const int arTextOffset[TEXTS + 1] = { 0x00, 0x2FD03, 0x2FD19, 0x2FD31, 0x2FD46, 0x2FD5D, 0x2FD75, 0x2FD8F, 0x2FDA7, 0x2FDC1, 0x2FDD9, 0x2FDF9, 0x2FE13, 0x2FE2E, 0x2FE46, 0x2FE61, 0x2FE79 };
static const int arTextLength[TEXTS + 1] = { -1, 21, 22, 20, 22, 23, 25, 23, 25, 21, 28, 21, 23, 23, 23, 23, 23 };
static const int arTextX[TEXTS + 1] = { -1, 42, 42, 42, 42, 42, 42, 42, 42, 42, 10, 42+16, 42+16, 42+16, 42+16, 42+16, 42+16 };
static const int arTextY[TEXTS + 1] = { -1, 66, 85, 123, 142, 161, 180, 199, 218, 237, 256, 275, 294, 313, 332, 351, 370 };

static const int broken_room_x = 307;
static const int broken_room_y = 78;
static const int broken_side_x[5] = { 0, 292, 322, 307, 307 };
static const int broken_side_y[5] = { 0, 78, 78, 63, 93 };

/*** controller ***/
int iController;
SDL_GameController *controller;
char sControllerName[MAX_CON + 2];
SDL_Joystick *joystick;
SDL_Haptic *haptic;
Uint32 joyleft;
Uint32 joyright;
Uint32 joyup;
Uint32 joydown;
Uint32 trigleft;
Uint32 trigright;

/*** for text ***/
SDL_Surface *message;
SDL_Texture *messaget;
SDL_Rect offset;
SDL_Color color_bl = {0x00, 0x00, 0x00, 255};
SDL_Color color_wh = {0xff, 0xff, 0xff, 255};
SDL_Color color_f4 = {0xf4, 0xf4, 0xf4, 255};

SDL_Texture *imgloading;
SDL_Texture *imgblack;
SDL_Texture *imgfaded;
SDL_Texture *imgpopup;
SDL_Texture *imgok[2 + 2];
SDL_Texture *imgdungeon[255 + 2];
SDL_Texture *imgpalace[255 + 2];
SDL_Texture *imgselected;
SDL_Texture *imgunknown;
SDL_Texture *imgmask;
SDL_Texture *imgleft_0, *imgleft_1;
SDL_Texture *imgright_0, *imgright_1;
SDL_Texture *imgup_0, *imgup_1;
SDL_Texture *imgdown_0, *imgdown_1;
SDL_Texture *imglrno, *imgudno, *imgudnonfo;
SDL_Texture *imgroomsoff, *imgroomson_0, *imgroomson_1;
SDL_Texture *imgbroomsoff, *imgbroomson_0, *imgbroomson_1;
SDL_Texture *imgelementsoff, *imgelementson_0, *imgelementson_1;
SDL_Texture *imgsaveoff, *imgsaveon_0, *imgsaveon_1;
SDL_Texture *imgquit_0, *imgquit_1;
SDL_Texture *imgprevoff, *imgprevon_0, *imgprevon_1;
SDL_Texture *imgnextoff, *imgnexton_0, *imgnexton_1;
/***/
SDL_Texture *imgkidl, *imgkidr;
SDL_Texture *imgguardl, *imgguardr;
SDL_Texture *imgskell, *imgskelr;
SDL_Texture *imgjaffarl, *imgjaffarr;
SDL_Texture *imgloose;
SDL_Texture *imgloosefall;
SDL_Texture *imggate;
SDL_Texture *imgsword;
SDL_Texture *imgspikes;
SDL_Texture *imgleveldoor;
SDL_Texture *imgchomper;
SDL_Texture *imgmouse;
SDL_Texture *imgshadowstep;
SDL_Texture *imgshadowfight;
SDL_Texture *imgshadowdrink;
SDL_Texture *imgpotionred;
SDL_Texture *imgpotionblue;
SDL_Texture *imgpotiongreen;
SDL_Texture *imgpotionpink;
SDL_Texture *imgmirror;
SDL_Texture *imgskelalive;
SDL_Texture *imgskelrespawn;
SDL_Texture *imgdropd;
SDL_Texture *imgdropdwall;
SDL_Texture *imgdropp;
SDL_Texture *imgdroppwall;
SDL_Texture *imgraised;
SDL_Texture *imgraisedwall;
SDL_Texture *imgraisep;
SDL_Texture *imgraisepwall;
SDL_Texture *imgprincess;
/***/
SDL_Texture *imgbar;
SDL_Texture *extras[10 + 2];
SDL_Texture *imgdungeont, *imgpalacet;
SDL_Texture *imgclosebig_0, *imgclosebig_1;
SDL_Texture *imgborder, *imgborderl;
SDL_Texture *imgrl, *imgbrl;
SDL_Texture *imgroom[25 + 2]; /*** 25 is "?", for all high room links ***/
SDL_Texture *imgsrc, *imgsrs, *imgsrm, *imgsrp, *imgsrb;
SDL_Texture *imgelements;
SDL_Texture *imgsele;
SDL_Texture *imgselg;
SDL_Texture *imghelp;
SDL_Texture *imgsave[2 + 2];
SDL_Texture *imgexe;
SDL_Texture *imgstatusbarsprite;
SDL_Texture *imgplaytest;
SDL_Texture *imgpopup_yn;
SDL_Texture *imglinkwarnlr, *imglinkwarnud;
SDL_Texture *imgyes[2 + 2], *imgno[2 + 2];
SDL_Texture *imgchars[58 + 2];
SDL_Texture *imgactivechar;
SDL_Texture *spriteflamed;
SDL_Texture *spriteflamep;
SDL_Texture *imgguards;

struct sample {
	Uint8 *data;
	Uint32 dpos;
	Uint32 dlen;
} sounds[NUM_SOUNDS];

void ShowUsage (void);
void PrIfDe (char *sString);
void GetPathFile (void);
void LoadLevel (int iLevel);
int ReadFromFile (int iFd, char *sWhat, int iSize, unsigned char *sRetString);
char cShowDirection (int iDirection);
int BinToDec (char *sBin);
void TilesToArray (int iRoom, int iRow, int iIn1, int iIn2, int iIn3,
	int iOut1, int iOut2, int iOut3, int iOut4);
void InitScreenAction (char *sAction);
void InitScreen (void);
void LoadFonts (void);
void MixAudio (void *unused, Uint8 *stream, int iLen);
void PreLoad (char *sPath, char *sPNG, SDL_Texture **imgImage);
void ShowImage (SDL_Texture *img, int iX, int iY, char *sImageInfo,
	SDL_Renderer *screen, float fMultiply, int iXYScale);
void LoadingBar (int iBarHeight);
void GetOptionValue (char *sArgv, char *sValue);
void ShowScreen (int iScreenS);
void InitPopUp (void);
void PlaySound (char *sFile);
void ShowPopUp (void);
int MapEvents (SDL_Event event);
int InArea (int iUpperLeftX, int iUpperLeftY,
	int iLowerRightX, int iLowerRightY);
void Quit (void);
void DisplayText (int iStartX, int iStartY, int iFontSize,
	char arText[9 + 2][MAX_TEXT + 2], int iLines, TTF_Font *font,
	SDL_Color back, SDL_Color fore, int iOnMap);
void InitPopUpSave (void);
void CustomRenderCopy (SDL_Texture* src, char *sImageInfo, SDL_Rect* srcrect,
	SDL_Renderer* dst, SDL_Rect *dstrect);
void ShowPopUpSave (void);
void SaveLevel (int iLevel);
void ShowTile (int iRoom, int iRow, int iCol);
void Prev (void);
void Next (void);
int BrokenRoomLinks (int iPrint, int iLevel);
void CheckSides (int iRoom, int iX, int iY, int iLevel);
int OnLevelBar (void);
void RunLevel (int iLevel);
int StartGame (void *unused);
void ModifyForPlaytest (int iLevel);
void ModifyBack (void);
void WriteByte (int iFd, int iValue);
void WriteWord (int iFd, int iValue);
void GetAsEightBits (unsigned char cChar, char *sBinary);
void SaveFourAsThree (int iFd, int iRoom);
void Zoom (int iToggleFull);
void ChangePosAction (char *sAction);
void ChangePos (int iLocationCol, int iLocationRow);
void ShowChange (void);
void OnTileOld (void);
int UseTile (int iLocationCol, int iLocationRow, int iRoom);
void SetLocation (int iRoom, int iLocationCol, int iLocationRow, int iUseTile);
void InitRooms (void);
void WhereToStart (void);
void ShowRooms (int iRoom, int iX, int iY, int iNext);
int MouseSelectAdj (void);
void LinkPlus (void);
void LinkMinus (void);
void ClearRoom (void);
void RemoveOldRoom (void);
void AddNewRoom (int iX, int iY, int iRoom);
void Help (void);
void ShowHelp (void);
void OpenURL (char *sURL);
void EXE (void);
void ShowEXE (void);
void EXELoad (void);
void EXESave (void);
void UpdateStatusBar (void);
void CenterNumber (SDL_Renderer *screen, int iNumber, int iX, int iY,
	SDL_Color fore, int iHex);
int PlusMinus (int *iWhat, int iX, int iY,
	int iMin, int iMax, int iChange, int iAddChanged);
void CopyPaste (int iRoom, int iAction);
void CreateBAK (void);
void Sprinkle (void);
unsigned long BytesAsLU (unsigned char *sData, int iBytes);
void ColorRect (int iX, int iY, int iW, int iH, int iR, int iG, int iB);
int BankedToLinear (int iAddr);
int LinearToBanked (int iAddr);
int GetChunkLength (int iLevel, int iNrElementRooms);
void Guards (void);
void ShowGuards (void);
void AddressToRoomAndElem (void);

/*****************************************************************************/
int main (int argc, char *argv[])
/*****************************************************************************/
{
	int iLoopArg;
	char sStartLevel[MAX_OPTION + 2];
	time_t tm;
	SDL_version verc, verl;

	iDebug = 0;
	iNoAudio = 0;
	iScale = 1;
	iFullscreen = 0;
	iStartLevel = 1;
	iNoController = 0;
	iExtras = 0;
	iLastTile = 0;
	iNoAnim = 0;
	iInfo = 0;
	iCopied = 0;
	iModified = 0;
	iActiveElement = 1;
	iActiveGuard = 0;

	if (argc > 1)
	{
		for (iLoopArg = 1; iLoopArg <= argc - 1; iLoopArg++)
		{
			if ((strcmp (argv[iLoopArg], "-h") == 0) ||
				(strcmp (argv[iLoopArg], "-?") == 0) ||
				(strcmp (argv[iLoopArg], "--help") == 0))
			{
				ShowUsage();
			}
			else if ((strcmp (argv[iLoopArg], "-v") == 0) ||
				(strcmp (argv[iLoopArg], "--version") == 0))
			{
				printf ("%s %s\n", EDITOR_NAME, EDITOR_VERSION);
				exit (EXIT_NORMAL);
			}
			else if ((strcmp (argv[iLoopArg], "-d") == 0) ||
				(strcmp (argv[iLoopArg], "--debug") == 0))
			{
				iDebug = 1;
			}
			else if ((strcmp (argv[iLoopArg], "-n") == 0) ||
				(strcmp (argv[iLoopArg], "--noaudio") == 0))
			{
				iNoAudio = 1;
			}
			else if ((strcmp (argv[iLoopArg], "-z") == 0) ||
				(strcmp (argv[iLoopArg], "--zoom") == 0))
			{
				iScale = 2;
			}
			else if ((strcmp (argv[iLoopArg], "-f") == 0) ||
				(strcmp (argv[iLoopArg], "--fullscreen") == 0))
			{
				iFullscreen = SDL_WINDOW_FULLSCREEN_DESKTOP;
			}
			else if ((strncmp (argv[iLoopArg], "-l=", 3) == 0) ||
				(strncmp (argv[iLoopArg], "--level=", 8) == 0))
			{
				GetOptionValue (argv[iLoopArg], sStartLevel);
				iStartLevel = atoi (sStartLevel);
				if ((iStartLevel < 1) || (iStartLevel > NR_LEVELS))
				{
					iStartLevel = 1;
				}
			}
			else if ((strcmp (argv[iLoopArg], "-s") == 0) ||
				(strcmp (argv[iLoopArg], "--static") == 0))
			{
				iNoAnim = 1;
			}
			else if ((strcmp (argv[iLoopArg], "-k") == 0) ||
				(strcmp (argv[iLoopArg], "--keyboard") == 0))
			{
				iNoController = 1;
			}
			else
			{
				ShowUsage();
			}
		}
	}

	GetPathFile();

	srand ((unsigned)time(&tm));

	LoadLevel (iStartLevel);

	/*** Show the SDL version used for compiling and linking. ***/
	if (iDebug == 1)
	{
		SDL_VERSION (&verc);
		SDL_GetVersion (&verl);
		printf ("[ INFO ] Compiled with SDL %u.%u.%u, linked with SDL %u.%u.%u.\n",
			verc.major, verc.minor, verc.patch, verl.major, verl.minor, verl.patch);
	}

	InitScreen();

	Quit();

	return 0;
}
/*****************************************************************************/
void ShowUsage (void)
/*****************************************************************************/
{
	printf ("%s %s\n%s\n\n", EDITOR_NAME, EDITOR_VERSION, COPYRIGHT);
	printf ("Usage:\n");
	printf ("  %s [OPTIONS]\n\nOptions:\n", EDITOR_NAME);
	printf ("  -h, -?,    --help           display this help and exit\n");
	printf ("  -v,        --version        output version information and"
		" exit\n");
	printf ("  -d,        --debug          also show levels on the console\n");
	printf ("  -n,        --noaudio        do not play sound effects\n");
	printf ("  -z,        --zoom           double the interface size\n");
	printf ("  -f,        --fullscreen     start in fullscreen mode\n");
	printf ("  -l=NR,     --level=NR       start in level NR\n");
	printf ("  -s,        --static         do not display animations\n");
	printf ("  -k,        --keyboard       do not use a game controller\n");
	printf ("\n");
	exit (EXIT_NORMAL);
}
/*****************************************************************************/
void PrIfDe (char *sString)
/*****************************************************************************/
{
	if (iDebug == 1) { printf ("%s", sString); }
}
/*****************************************************************************/
void GetPathFile (void)
/*****************************************************************************/
{
	int iFound;
	DIR *dDir;
	struct dirent *stDirent;
	char sExtension[100 + 2];
	char sError[MAX_ERROR + 2];
	int iFd;
	char sVerify[VERIFY_SIZE + 2];

	iFound = 0;

	dDir = opendir (SMS_DIR);
	if (dDir == NULL)
	{
		printf ("[FAILED] Cannot open directory \"%s\": %s!\n",
			SMS_DIR, strerror (errno));
		exit (EXIT_ERROR);
	}

	while ((stDirent = readdir (dDir)) != NULL)
	{
		if (iFound == 0)
		{
			if ((strcmp (stDirent->d_name, ".") != 0) &&
				(strcmp (stDirent->d_name, "..") != 0))
			{
				snprintf (sExtension, 100, "%s", strrchr (stDirent->d_name, '.'));
				if ((toupper (sExtension[1]) == 'S') &&
					(toupper (sExtension[2]) == 'M') &&
					(toupper (sExtension[3]) == 'S'))
				{
					iFound = 1;
					snprintf (sPathFile, MAX_PATHFILE, "%s%s%s", SMS_DIR, SLASH,
						stDirent->d_name);
					if (iDebug == 1)
					{
						printf ("[  OK  ] Found Sega Master System disk image \"%s\".\n",
							sPathFile);
					}
				}
			}
		}
	}

	closedir (dDir);

	if (iFound == 0)
	{
		snprintf (sError, MAX_ERROR, "Cannot find a .sms disk image in"
			" directory \"%s\"!", SMS_DIR);
		printf ("[ FAILED ] %s\n", sError);
		SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_ERROR,
			"Error", sError, NULL);
		exit (EXIT_ERROR);
	}

	/*** Is the file accessible? ***/
	if (access (sPathFile, R_OK|W_OK) == -1)
	{
		printf ("[FAILED] Cannot access \"%s\": %s!\n",
			sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}

	/*** Is the file a PoP1 for SMS disk image? ***/
	iFd = open (sPathFile, O_RDONLY|O_BINARY);
	if (iFd == -1)
	{
		printf ("[FAILED] Could not open \"%s\": %s!\n",
			sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}
	lseek (iFd, VERIFY_OFFSET, SEEK_SET);
	read (iFd, sVerify, VERIFY_SIZE);
	sVerify[VERIFY_SIZE] = '\0';
	if (strcmp (sVerify, VERIFY_TEXT) != 0)
	{
		snprintf (sError, MAX_ERROR, "File %s is not a Prince of Persia"
			" for SMS disk image!", sPathFile);
		printf ("[FAILED] %s\n", sError);
		SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_ERROR,
			"Error", sError, NULL);
		exit (EXIT_ERROR);
	}
	close (iFd);
}
/*****************************************************************************/
void LoadLevel (int iLevel)
/*****************************************************************************/
{
	int iFd;
	unsigned char sRoomLinks[(ROOMS * 4) + 2];
	unsigned char sDefinitions[64 + 2];
	unsigned char sTiles[(ROOMS * 12 * ROWS) + 2]; /*** 12 is OK ***/
	unsigned char sRead[1 + 2];
	int iNextRoom;
	unsigned char cElement;
	int iOffset, iOffsetOld;
	unsigned char sData[100 + 2];
	int iElementOffset;
	char sElementBase[MAX_TEXT + 2];
	char sElement[MAX_TEXT + 2];
	int iNrElem;
	int iDefinition;
	char sReadW[10 + 2];
	int iChunkLength;
	int iRooms;
	int iOffsetTG, iOffsetT;

	/*** Used for looping. ***/
	int iLoopLevel;
	int iLoopRoom;
	int iLoopLink;
	int iLoopDef;
	int iLoopRow;
	int iLoopCol;
	int iLoopElem;
	int iLoopEmpty;
	int iLoopGuard;

	iFd = open (sPathFile, O_RDONLY|O_BINARY);
	if (iFd == -1)
	{
		printf ("[FAILED] Could not open \"%s\": %s!\n",
			sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}

	iChanged = 0;

	/* Everything is loaded for ALL levels.
	 * The reason is that the elements are variable in length, so saving
	 * requires knowledge of all this information.
	 */
	for (iLoopLevel = 1; iLoopLevel <= NR_LEVELS; iLoopLevel++)
	{
		/*******************************/
		/* Levels ("The level table"). */
		/*******************************/

		lseek (iFd, iOffsetLevelTable + ((iLoopLevel - 1) * 20), SEEK_SET);

		/*** 0x60C9 ***/
		ReadFromFile (iFd, "", 2, sData);
		if ((sData[0] != 0xC9) || (sData[1] != 0x60))
		{
			printf ("[ WARN ] 0x60C9 not found: 0x%02X%02X!\n", sData[1], sData[0]);
		}

		/*** Tile graphics. ***/
		ReadFromFile (iFd, "", 2, sData);
		iOffsetTG = BytesAsLU (sData, 2);
		arOffsetTG1[iLoopLevel] = BankedToLinear (iOffsetTG);
		ReadFromFile (iFd, "", 2, sData);
		iOffsetTG = BytesAsLU (sData, 2);
		arOffsetTG2[iLoopLevel] = iOffsetTG;

		/*** Offset tiles. ***/
		ReadFromFile (iFd, "", 2, sData);
		iOffsetT = BytesAsLU (sData, 2);
		arOffsetTiles[iLoopLevel] = BankedToLinear (iOffsetT);
		if (iDebug == 1)
		{
			printf ("[ INFO ] (Offset) Tiles level %i start at: 0x%02X\n",
				iLoopLevel, arOffsetTiles[iLoopLevel]);
		}

		/*** Level type. ***/
		ReadFromFile (iFd, "", 1, sData);
		switch (sData[0])
		{
			case 0x05: arLevelType[iLoopLevel] = 'd'; break;
			case 0x85: arLevelType[iLoopLevel] = 'p'; break;
		}

		/*** Load start position. ***/
		ReadFromFile (iFd, "", 5, sData);
		arKidDir[iLoopLevel] = sData[0];
		arKidPosX[iLoopLevel] = sData[1];
		arKidPosY[iLoopLevel] = sData[2];
		arKidRoom[iLoopLevel] = sData[3] + 1;
		arKidAnim[iLoopLevel] = sData[4];
		if (iDebug == 1)
		{
			printf ("[ INFO ] The kid starts turned: %c, x/y: %i/%i, room: %i, "
				"anim: %i\n",
				cShowDirection (arKidDir[iLoopLevel]),
				arKidPosX[iLoopLevel],
				arKidPosY[iLoopLevel],
				arKidRoom[iLoopLevel],
				arKidAnim[iLoopLevel]);
		}

		/*** Unknown. ***/
		ReadFromFile (iFd, "", 4, sData);
		arUnknown[iLoopLevel][1] = sData[0];
		arUnknown[iLoopLevel][2] = sData[1];
		arUnknown[iLoopLevel][3] = sData[2];
		arUnknown[iLoopLevel][4] = sData[3];

		/*** Room links address. ***/
		ReadFromFile (iFd, "", 2, sData);
		iOffset = BytesAsLU (sData, 2);
		if (iDebug == 1)
		{
			printf ("[ INFO ] (Offset) Room links level %i start at: 0x%02X\n",
				iLoopLevel, iOffset);
		}
		iElementOffset = iOffset;
		lseek (iFd, iOffset, SEEK_SET);

		/*************************************************************/
		/* Room links and elements ("Room links and objects table"). */
		/*************************************************************/

		/*** Load chunk length. ***/
		ReadFromFile (iFd, "", 2, sData);
		iOffset+=2;
		iChunkLength = BytesAsLU (sData, 2);
		if (iDebug == 1)
		{
			printf ("[ INFO ] Chunk length level %i: %i\n",
				iLoopLevel, iChunkLength);
		}
		/*** Load offsets of elements. ***/
		for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
		{
			ReadFromFile (iFd, "", 2, sData);
			if ((sData[0] != 0x00) || (sData[1] != 0x00))
			{
				arOffsetsElements[iLoopRoom] = iOffset + BytesAsLU (sData, 2);
			} else {
				if ((iLoopLevel != 14) || (iLoopRoom < 9))
				{
					printf ("[ WARN ] Incorrect elements offset!\n");
				}
				arOffsetsElements[iLoopRoom] = 0;
			}
			if (iDebug == 1)
			{
				printf ("[ INFO ] (Offset) Elements l %i, r %i, start at: 0x%02X\n",
					iLoopLevel, iLoopRoom, arOffsetsElements[iLoopRoom]);
			}
		}
		iOffset+=48; /*** Do NOT move up. ***/
		/*** Load the room links. ***/
		switch (iLoopLevel)
		{
			case 14: iRooms = 5; break;
			default: iRooms = ROOMS; break;
		}
		ReadFromFile (iFd, "Room Links", (iRooms * 4), sRoomLinks);
		iOffset+=(iRooms * 4);
		for (iLoopLink = 0; iLoopLink < (iRooms * 4); iLoopLink+=4)
		{
			iRoomConnections[iLoopLevel][(iLoopLink / 4) + 1][1] =
				sRoomLinks[iLoopLink + 3] + 1; /*** left ***/
			iRoomConnections[iLoopLevel][(iLoopLink / 4) + 1][2] =
				sRoomLinks[iLoopLink + 1] + 1; /*** right ***/
			iRoomConnections[iLoopLevel][(iLoopLink / 4) + 1][3] =
				sRoomLinks[iLoopLink + 0] + 1; /*** up ***/
			iRoomConnections[iLoopLevel][(iLoopLink / 4) + 1][4] =
				sRoomLinks[iLoopLink + 2] + 1; /*** down ***/
			if (iDebug == 1)
			{
				printf ("[ INFO ] Room %i is connected to room (256 = none): u%i, "
					"r%i, d%i, l%i\n",
					(iLoopLink / 4) + 1,
					sRoomLinks[iLoopLink] + 1,
					sRoomLinks[iLoopLink + 1] + 1,
					sRoomLinks[iLoopLink + 2] + 1,
					sRoomLinks[iLoopLink + 3] + 1);
			}
		}
		/*** Load elements. ***/
		for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
		{
			if (arOffsetsElements[iLoopRoom] != 0)
			{

			/*** clear ***/
			for (iLoopElem = 1; iLoopElem <= MAX_ELEM; iLoopElem++)
			{
				arElements[iLoopLevel][iLoopRoom][iLoopElem][0] = 0x01;
				arElements[iLoopLevel][iLoopRoom][iLoopElem][8] = 0x00; /*** off ***/
			}
			iNrElem = 0;

			iOffsetOld = iOffset;
			iOffset = arOffsetsElements[iLoopRoom];
			if (iOffset != iOffsetOld)
				{ printf ("[ WARN ] Offset difference!\n"); }
			lseek (iFd, iOffset, SEEK_SET);

			iNextRoom = 0;
			do {
				read (iFd, sRead, 1);
				iOffset++;
				iNrElem++;
				cElement = sRead[0];
				arElements[iLoopLevel][iLoopRoom][iNrElem][9] =
					iOffset - iElementOffset - 3;
				switch (cElement)
				{
					case 0x27: /*** level door (8 bytes) ***/
						snprintf (sElementBase, MAX_TEXT, "%s", "Level door");
						arElements[iLoopLevel][iLoopRoom][iNrElem][0] = 0x27;
						read (iFd, sRead, 1); /*** x ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][1] = sRead[0];
						read (iFd, sRead, 1); /*** y ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][2] = sRead[0];
						read (iFd, sRead, 1); /*** ? ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][3] = sRead[0];
						read (iFd, sRead, 1); /*** ? ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][4] = sRead[0];
						read (iFd, sRead, 1); /*** ? ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][5] = sRead[0];
						read (iFd, sRead, 1); /*** ? ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][6] = sRead[0];
						read (iFd, sRead, 1); /*** ? ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][7] = sRead[0];
						arElements[iLoopLevel][iLoopRoom][iNrElem][8] = 0x01; /*** on ***/
						iOffset+=7;
						snprintf (sElement, MAX_TEXT, "%s (x:%i, y:%i,"
							" ?:%i, ?:%i, ?:%i, ?:%i, ?:%i)",
							sElementBase,
							arElements[iLoopLevel][iLoopRoom][iNrElem][1],
							arElements[iLoopLevel][iLoopRoom][iNrElem][2],
							arElements[iLoopLevel][iLoopRoom][iNrElem][3],
							arElements[iLoopLevel][iLoopRoom][iNrElem][4],
							arElements[iLoopLevel][iLoopRoom][iNrElem][5],
							arElements[iLoopLevel][iLoopRoom][iNrElem][6],
							arElements[iLoopLevel][iLoopRoom][iNrElem][7]);
						break;
					case 0x29: /*** mirror (6 bytes) ***/
						snprintf (sElementBase, MAX_TEXT, "%s", "Mirror");
						arElements[iLoopLevel][iLoopRoom][iNrElem][0] = 0x29;
						read (iFd, sRead, 1); /*** x ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][1] = sRead[0];
						read (iFd, sRead, 1); /*** y ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][2] = sRead[0];
						read (iFd, sRead, 1); /*** ? ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][3] = sRead[0];
						read (iFd, sRead, 1); /*** reflection x ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][4] = sRead[0];
						read (iFd, sRead, 1); /*** reflection y ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][5] = sRead[0];
						arElements[iLoopLevel][iLoopRoom][iNrElem][8] = 0x01; /*** on ***/
						iOffset+=5;
						snprintf (sElement, MAX_TEXT, "%s (x:%i, y:%i,"
							" ?:%i, refl x:%i, refl y:%i)",
							sElementBase,
							arElements[iLoopLevel][iLoopRoom][iNrElem][1],
							arElements[iLoopLevel][iLoopRoom][iNrElem][2],
							arElements[iLoopLevel][iLoopRoom][iNrElem][3],
							arElements[iLoopLevel][iLoopRoom][iNrElem][4],
							arElements[iLoopLevel][iLoopRoom][iNrElem][5]);
						break;
					case 0x32: /*** gate (8 bytes) ***/
						snprintf (sElementBase, MAX_TEXT, "%s", "Gate");
						arElements[iLoopLevel][iLoopRoom][iNrElem][0] = 0x32;
						read (iFd, sRead, 1); /*** x ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][1] = sRead[0];
						read (iFd, sRead, 1); /*** y ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][2] = sRead[0];
						read (iFd, sRead, 1); /*** anim ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][3] = sRead[0];
						read (iFd, sRead, 1); /*** anim spd ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][4] = sRead[0];
						read (iFd, sRead, 1); /*** openness ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][5] = sRead[0];
						read (iFd, sRead, 1); /*** ? ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][6] = sRead[0];
						read (iFd, sRead, 1); /*** ? ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][7] = sRead[0];
						arElements[iLoopLevel][iLoopRoom][iNrElem][8] = 0x01; /*** on ***/
						iOffset+=7;
						snprintf (sElement, MAX_TEXT, "%s (x:%i, y:%i,"
							" anim:%i, anim spd:%i, openness:%i, ?:%i, ?:%i)",
							sElementBase,
							arElements[iLoopLevel][iLoopRoom][iNrElem][1],
							arElements[iLoopLevel][iLoopRoom][iNrElem][2],
							arElements[iLoopLevel][iLoopRoom][iNrElem][3],
							arElements[iLoopLevel][iLoopRoom][iNrElem][4],
							arElements[iLoopLevel][iLoopRoom][iNrElem][5],
							arElements[iLoopLevel][iLoopRoom][iNrElem][6],
							arElements[iLoopLevel][iLoopRoom][iNrElem][7]);
						break;
					case 0x3E: /*** skel alive (6 bytes) ***/
						snprintf (sElementBase, MAX_TEXT, "%s", "Skel alive");
						arElements[iLoopLevel][iLoopRoom][iNrElem][0] = 0x3E;
						read (iFd, sRead, 1); /*** ? ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][1] = sRead[0];
						read (iFd, sRead, 1); /*** ? ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][2] = sRead[0];
						read (iFd, sRead, 1); /*** ? ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][3] = sRead[0];
						read (iFd, sRead, 1); /*** x ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][4] = sRead[0];
						read (iFd, sRead, 1); /*** y ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][5] = sRead[0];
						arElements[iLoopLevel][iLoopRoom][iNrElem][8] = 0x01; /*** on ***/
						iOffset+=5;
						snprintf (sElement, MAX_TEXT, "%s (?:%i, ?:%i,"
							" ?:%i, x:%i, y:%i)",
							sElementBase,
							arElements[iLoopLevel][iLoopRoom][iNrElem][1],
							arElements[iLoopLevel][iLoopRoom][iNrElem][2],
							arElements[iLoopLevel][iLoopRoom][iNrElem][3],
							arElements[iLoopLevel][iLoopRoom][iNrElem][4],
							arElements[iLoopLevel][iLoopRoom][iNrElem][5]);
						break;
					case 0x3F: /*** skel respawn (6 bytes) ***/
						snprintf (sElementBase, MAX_TEXT, "%s", "Skel respawn");
						arElements[iLoopLevel][iLoopRoom][iNrElem][0] = 0x3F;
						read (iFd, sRead, 1); /*** ? ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][1] = sRead[0];
						read (iFd, sRead, 1); /*** ? ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][2] = sRead[0];
						read (iFd, sRead, 1); /*** ? ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][3] = sRead[0];
						read (iFd, sRead, 1); /*** x ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][4] = sRead[0];
						read (iFd, sRead, 1); /*** y ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][5] = sRead[0];
						arElements[iLoopLevel][iLoopRoom][iNrElem][8] = 0x01; /*** on ***/
						iOffset+=5;
						snprintf (sElement, MAX_TEXT, "%s (?:%i, ?:%i,"
							" ?:%i, x:%i, y:%i)",
							sElementBase,
							arElements[iLoopLevel][iLoopRoom][iNrElem][1],
							arElements[iLoopLevel][iLoopRoom][iNrElem][2],
							arElements[iLoopLevel][iLoopRoom][iNrElem][3],
							arElements[iLoopLevel][iLoopRoom][iNrElem][4],
							arElements[iLoopLevel][iLoopRoom][iNrElem][5]);
						break;
					case 0x61: /*** drop button (6 bytes) ***/
						snprintf (sElementBase, MAX_TEXT, "%s", "Drop");
						arElements[iLoopLevel][iLoopRoom][iNrElem][0] = 0x61;
						read (iFd, sRead, 1); /*** x ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][1] = sRead[0];
						read (iFd, sRead, 1); /*** y ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][2] = sRead[0];
						read (iFd, sRead, 1); /*** with wall (0=n,2=y) ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][3] = sRead[0];
						read (iFd, sRead, 1); /*** address 1/2 ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][4] = sRead[0];
						read (iFd, sRead, 1); /*** address 2/2 ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][5] = sRead[0];
						arElements[iLoopLevel][iLoopRoom][iNrElem][8] = 0x01; /*** on ***/
						iOffset+=5;
						snprintf (sReadW, 10, "%02x%02x",
							arElements[iLoopLevel][iLoopRoom][iNrElem][5],
							arElements[iLoopLevel][iLoopRoom][iNrElem][4]);
						snprintf (sElement, MAX_TEXT, "%s (x:%i, y:%i,"
							" wall:%i, a1:%i, a2:%i) (@%lu)",
							sElementBase,
							arElements[iLoopLevel][iLoopRoom][iNrElem][1],
							arElements[iLoopLevel][iLoopRoom][iNrElem][2],
							arElements[iLoopLevel][iLoopRoom][iNrElem][3],
							arElements[iLoopLevel][iLoopRoom][iNrElem][4],
							arElements[iLoopLevel][iLoopRoom][iNrElem][5],
							strtoul (sReadW, NULL, 16));
						break;
					case 0x62: /*** raise button (6 bytes) ***/
						snprintf (sElementBase, MAX_TEXT, "%s", "Raise");
						arElements[iLoopLevel][iLoopRoom][iNrElem][0] = 0x62;
						read (iFd, sRead, 1); /*** x ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][1] = sRead[0];
						read (iFd, sRead, 1); /*** y ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][2] = sRead[0];
						read (iFd, sRead, 1); /*** with wall (0=n,2=y) ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][3] = sRead[0];
						read (iFd, sRead, 1); /*** address 1/2 ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][4] = sRead[0];
						read (iFd, sRead, 1); /*** address 2/2 ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][5] = sRead[0];
						arElements[iLoopLevel][iLoopRoom][iNrElem][8] = 0x01; /*** on ***/
						iOffset+=5;
						snprintf (sReadW, 10, "%02x%02x",
							arElements[iLoopLevel][iLoopRoom][iNrElem][5],
							arElements[iLoopLevel][iLoopRoom][iNrElem][4]);
						snprintf (sElement, MAX_TEXT, "%s (x:%i, y:%i,"
							" wall:%i, a1:%i, a2:%i) (@%lu)",
							sElementBase,
							arElements[iLoopLevel][iLoopRoom][iNrElem][1],
							arElements[iLoopLevel][iLoopRoom][iNrElem][2],
							arElements[iLoopLevel][iLoopRoom][iNrElem][3],
							arElements[iLoopLevel][iLoopRoom][iNrElem][4],
							arElements[iLoopLevel][iLoopRoom][iNrElem][5],
							strtoul (sReadW, NULL, 16));
						break;
					case 0x64: /*** guard (6 bytes) ***/
						snprintf (sElementBase, MAX_TEXT, "%s", "Guard");
						arElements[iLoopLevel][iLoopRoom][iNrElem][0] = 0x64;
						read (iFd, sRead, 1); /*** # ("g" to edit) ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][1] = sRead[0];
						read (iFd, sRead, 1); /*** type (0=g,1=s,2=J) ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][2] = sRead[0];
						read (iFd, sRead, 1); /*** anim ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][3] = sRead[0];
						read (iFd, sRead, 1); /*** x ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][4] = sRead[0];
						read (iFd, sRead, 1); /*** y ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][5] = sRead[0];
						arElements[iLoopLevel][iLoopRoom][iNrElem][8] = 0x01; /*** on ***/
						iOffset+=5;
						snprintf (sElement, MAX_TEXT, "%s (#:%i, type:%i,"
							" anim:%i, x:%i, y:%i)",
							sElementBase,
							arElements[iLoopLevel][iLoopRoom][iNrElem][1],
							arElements[iLoopLevel][iLoopRoom][iNrElem][2],
							arElements[iLoopLevel][iLoopRoom][iNrElem][3],
							arElements[iLoopLevel][iLoopRoom][iNrElem][4],
							arElements[iLoopLevel][iLoopRoom][iNrElem][5]);
						break;
					case 0xFF: /*** next room (1 byte) ***/
					case 0x00: /*** end (1 byte) ***/
						snprintf (sElement, MAX_TEXT, "%s", "----------");
						iNextRoom = 1;
						break;
					default: /*** default (3 bytes) ***/
						switch (cElement)
						{
							case 0x2B:
								snprintf (sElementBase, MAX_TEXT, "%s", "Spikes");
								break;
							case 0x2C:
								snprintf (sElementBase, MAX_TEXT, "%s", "Sword");
								break;
							case 0x2D:
								snprintf (sElementBase, MAX_TEXT, "%s", "Hurt potion (red)");
								break;
							case 0x2E:
								snprintf (sElementBase, MAX_TEXT, "%s", "Heal potion (blue)");
								break;
							case 0x2F:
								snprintf (sElementBase, MAX_TEXT, "%s", "Life potion (green)");
								break;
							case 0x30:
								snprintf (sElementBase, MAX_TEXT, "%s", "Float potion (pink)");
								break;
							case 0x31:
								snprintf (sElementBase, MAX_TEXT, "%s", "Chomper");
								break;
							case 0x41:
								snprintf (sElementBase, MAX_TEXT, "%s", "Loose floor");
								break;
							case 0x42:
								snprintf (sElementBase, MAX_TEXT, "%s", "Loose floor fall");
								break;
							case 0x66:
								snprintf (sElementBase, MAX_TEXT, "%s", "Shadow step");
								break;
							case 0x69:
								snprintf (sElementBase, MAX_TEXT, "%s", "Shadow run");
								break;
							case 0x6F:
								snprintf (sElementBase, MAX_TEXT, "%s", "Princess");
								break;
							case 0x70:
								snprintf (sElementBase, MAX_TEXT, "%s", "Mouse move");
								break;
							case 0x74:
								snprintf (sElementBase, MAX_TEXT, "%s", "Shadow fight");
								break;
							default:
								printf ("[ WARN ] Unknown element (level %i, room %i): %02X\n",
									iLoopLevel, iLoopRoom, cElement);
								snprintf (sElementBase, MAX_TEXT, "%s", "???");
								break;
						}
						arElements[iLoopLevel][iLoopRoom][iNrElem][0] = cElement;
						read (iFd, sRead, 1); /*** x ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][1] = sRead[0];
						read (iFd, sRead, 1); /*** y ***/
						arElements[iLoopLevel][iLoopRoom][iNrElem][2] = sRead[0];
						arElements[iLoopLevel][iLoopRoom][iNrElem][8] = 0x01; /*** on ***/
						iOffset+=2;
						snprintf (sElement, MAX_TEXT, "%s (x:%i, y:%i)",
							sElementBase,
							arElements[iLoopLevel][iLoopRoom][iNrElem][1],
							arElements[iLoopLevel][iLoopRoom][iNrElem][2]);
						break;
				}
				if (iDebug == 1)
				{
					printf ("[ INFO ] Elem l%2i, r%2i (@%i): %s\n", iLoopLevel,
						iLoopRoom, arElements[iLoopLevel][iLoopRoom][iNrElem][9], sElement);
				}
			} while (iNextRoom == 0);

			}
		}
	}

	AddressToRoomAndElem();

	PrIfDe ("[  OK  ] Checking for broken room links.\n");
	iBrokenRoomLinks = BrokenRoomLinks (1, iLevel);

	/**************************/
	/* Tiles ("Graphics map") */
	/**************************/

	lseek (iFd, arOffsetTiles[iLevel], SEEK_SET);
	ReadFromFile (iFd, "Definitions", 64, sDefinitions);
	for (iLoopDef = 0; iLoopDef < 64; iLoopDef++)
	{
		iDefinition = sDefinitions[iLoopDef];
		/* For some reason, the original game uses the impossible (because
		 * it's not 6-bit) 68 for some tiles in levels 1-3.
		 */
		if ((iDefinition == 255) || (iDefinition == 68)) { iDefinition = 0; }
		arDefinitions[iLoopDef] = iDefinition;
		if (iDebug == 1)
		{
			printf ("[ INFO ] Level %i, definition %i: %i\n",
				iLevel, iLoopDef, sDefinitions[iLoopDef]);
		}
	}
	switch (iLevel)
	{
		case 13: iRooms = 16; break;
		case 14: iRooms = 8; break;
		default: iRooms = ROOMS; break;
	}
	ReadFromFile (iFd, "Tiles", (iRooms * 12 * ROWS), sTiles); /*** 12 is OK ***/
	for (iLoopRow = 1; iLoopRow <= ROWS; iLoopRow++) /*** height ***/
	{
		for (iLoopRoom = 1; iLoopRoom <= 8; iLoopRoom++)
		{
			for (iLoopCol = 1; iLoopCol <= 12; iLoopCol++) /*** width; 12 is OK ***/
			{
				arValue[iLoopCol] = sTiles[0 + (96 * (iLoopRow - 1)) +
					(12 * (iLoopRoom - 1 - 0)) + (iLoopCol - 1)];
			}
			TilesToArray (iLoopRoom, iLoopRow, 1, 2, 3, 1, 2, 3, 4);
			TilesToArray (iLoopRoom, iLoopRow, 4, 5, 6, 5, 6, 7, 8);
			TilesToArray (iLoopRoom, iLoopRow, 7, 8, 9, 9, 10, 11, 12);
			TilesToArray (iLoopRoom, iLoopRow, 10, 11, 12, 13, 14, 15, 16);
		}
		if (iRooms > 8)
		{
			for (iLoopRoom = 9; iLoopRoom <= 16; iLoopRoom++)
			{
				for (iLoopCol = 1; iLoopCol <= 12; iLoopCol++) /*** 12 is OK ***/
				{
					arValue[iLoopCol] = sTiles[1152 + (96 * (iLoopRow - 1)) +
						(12 * (iLoopRoom - 1 - 8)) + (iLoopCol - 1)];
				}
				TilesToArray (iLoopRoom, iLoopRow, 1, 2, 3, 1, 2, 3, 4);
				TilesToArray (iLoopRoom, iLoopRow, 4, 5, 6, 5, 6, 7, 8);
				TilesToArray (iLoopRoom, iLoopRow, 7, 8, 9, 9, 10, 11, 12);
				TilesToArray (iLoopRoom, iLoopRow, 10, 11, 12, 13, 14, 15, 16);
			}
		}
		if (iRooms > 16)
		{
			for (iLoopRoom = 17; iLoopRoom <= 24; iLoopRoom++)
			{
				for (iLoopCol = 1; iLoopCol <= 12; iLoopCol++) /*** 12 is OK ***/
				{
					arValue[iLoopCol] = sTiles[1152 + 1152 + (96 * (iLoopRow - 1)) +
						(12 * (iLoopRoom - 1 - 16)) + (iLoopCol - 1)];
				}
				TilesToArray (iLoopRoom, iLoopRow, 1, 2, 3, 1, 2, 3, 4);
				TilesToArray (iLoopRoom, iLoopRow, 4, 5, 6, 5, 6, 7, 8);
				TilesToArray (iLoopRoom, iLoopRow, 7, 8, 9, 9, 10, 11, 12);
				TilesToArray (iLoopRoom, iLoopRow, 10, 11, 12, 13, 14, 15, 16);
			}
		}
	}
	if (iDebug == 1)
	{
		for (iLoopRoom = 1; iLoopRoom <= iRooms; iLoopRoom++)
		{
			printf ("\nLevel %i, room %i:\n", iLevel, iLoopRoom);
			for (iLoopRow = 1; iLoopRow <= ROWS; iLoopRow++)
			{
				for (iLoopCol = 1; iLoopCol <= COLS; iLoopCol++)
				{
					printf ("%02i ", arTiles[iLoopRoom][iLoopCol][iLoopRow]);
				}
				printf ("\n");
			}
		}
	}
	ReadFromFile (iFd, "Empty", 32, sEmpty);
	if (iDebug == 1)
	{
		for (iLoopEmpty = 1; iLoopEmpty <= 32; iLoopEmpty++)
		{
			printf ("[ INFO ] Level %i, empty %i: %i\n",
				iLevel, iLoopEmpty, sEmpty[iLoopEmpty - 1]);
		}
	}

	/**************************/
	/* Guards ("Guard table") */
	/**************************/

	lseek (iFd, iOffsetGuardTable, SEEK_SET);
	for (iLoopGuard = 0; iLoopGuard <= MAX_GUARDS; iLoopGuard++)
	{
		read (iFd, sRead, 1);
		arGuardR[iLoopGuard] = 0;
		arGuardG[iLoopGuard] = 0;
		arGuardB[iLoopGuard] = 0;
		if (sRead[0] & (1 << 0)) { arGuardR[iLoopGuard]+= 1; }
		if (sRead[0] & (1 << 1)) { arGuardR[iLoopGuard]+= 2; }
		if (sRead[0] & (1 << 2)) { arGuardG[iLoopGuard]+= 1; }
		if (sRead[0] & (1 << 3)) { arGuardG[iLoopGuard]+= 2; }
		if (sRead[0] & (1 << 4)) { arGuardB[iLoopGuard]+= 1; }
		if (sRead[0] & (1 << 5)) { arGuardB[iLoopGuard]+= 2; }
		read (iFd, sRead, 1);
		arGuardGameLevel[iLoopGuard] = sRead[0];
		read (iFd, sRead, 1);
		arGuardHP[iLoopGuard] = 0;
		if (sRead[0] & (1 << 0)) { arGuardHP[iLoopGuard]+=1; }
		if (sRead[0] & (1 << 1)) { arGuardHP[iLoopGuard]+=2; }
		if (sRead[0] & (1 << 2)) { arGuardHP[iLoopGuard]+=4; }
		if (sRead[0] & (1 << 3)) { arGuardHP[iLoopGuard]+=8; }
		arGuardDir[iLoopGuard] = 0;
		if (sRead[0] & (1 << 6)) { arGuardDir[iLoopGuard]+=1; }
		arGuardMove[iLoopGuard] = 0;
		if (sRead[0] & (1 << 7)) { arGuardMove[iLoopGuard]+=1; }
		if (iDebug == 1)
		{
			printf ("[ INFO ] Guard %i: Color:R%i/G%i/B%i GLevel:%i HP:%i Dir:%i"
				" Move:%i\n", iLoopGuard, arGuardR[iLoopGuard], arGuardG[iLoopGuard],
				arGuardB[iLoopGuard], arGuardGameLevel[iLoopGuard],
				arGuardHP[iLoopGuard], arGuardDir[iLoopGuard],
				arGuardMove[iLoopGuard]);
		}
	}

	iCurLevel = iLevel;
	cCurType = arLevelType[iCurLevel];
	iCurRoom = arKidRoom[iCurLevel];
	iOnTileCol = 1;
	iOnTileRow = 1;
	iPlaytest = 0;
	close (iFd);
}
/*****************************************************************************/
int ReadFromFile (int iFd, char *sWhat, int iSize, unsigned char *sRetString)
/*****************************************************************************/
{
	int iLength;
	int iRead;
	char sRead[1 + 2];
	int iEOF;

	if ((iDebug == 1) && (strcmp (sWhat, "") != 0))
	{
		printf ("[  OK  ] Loading: %s\n", sWhat);
	}
	iLength = 0;
	iEOF = 0;
	do {
		iRead = read (iFd, sRead, 1);
		switch (iRead)
		{
			case -1:
				printf ("[FAILED] Could not read: %s!\n", strerror (errno));
				exit (EXIT_ERROR);
				break;
			case 0: PrIfDe ("[ INFO ] End of file\n"); iEOF = 1; break;
			default:
				sRetString[iLength] = sRead[0];
				iLength++;
				break;
		}
	} while ((iLength < iSize) && (iEOF == 0));
	sRetString[iLength] = '\0';

	return (iLength);
}
/*****************************************************************************/
char cShowDirection (int iDirection)
/*****************************************************************************/
{
	switch (iDirection)
	{
		case 0: return ('l'); break;
		case 32: return ('r'); break;
		default: return ('?'); break;
	}
}
/*****************************************************************************/
int BinToDec (char *sBin)
/*****************************************************************************/
{
	int iDec;
	int iLen;

	iDec = 0;
	iLen = strlen (sBin);

	if (sBin[iLen - 1] == '1') { iDec+=1; }
	if (sBin[iLen - 2] == '1') { iDec+=2; }
	if (sBin[iLen - 3] == '1') { iDec+=4; }
	if (sBin[iLen - 4] == '1') { iDec+=8; }
	if (sBin[iLen - 5] == '1') { iDec+=16; }
	if (sBin[iLen - 6] == '1') { iDec+=32; }
	if (iLen == 8)
	{
		if (sBin[iLen - 7] == '1') { iDec+=64; }
		if (sBin[iLen - 8] == '1') { iDec+=128; }
	}

	return (iDec);
}
/*****************************************************************************/
void TilesToArray (int iRoom, int iRow, int iIn1, int iIn2, int iIn3,
	int iOut1, int iOut2, int iOut3, int iOut4)
/*****************************************************************************/
{
	char sValue1234[24 + 2];
	char sValue1[6 + 2], sValue2[6 + 2], sValue3[6 + 2], sValue4[6 + 2];

	snprintf (sValue1234, 24 + 2,
		"%c%c%c%c%c%c%c%c"
		"%c%c%c%c%c%c%c%c"
		"%c%c%c%c%c%c%c%c",
		BYTE_TO_BINARY (arValue[iIn1]),
		BYTE_TO_BINARY (arValue[iIn2]),
		BYTE_TO_BINARY (arValue[iIn3]));
	snprintf (sValue1, 6 + 2, "%c%c%c%c%c%c",
		sValue1234[2], sValue1234[3], sValue1234[4],
		sValue1234[5], sValue1234[6], sValue1234[7]);
	snprintf (sValue2, 6 + 2, "%c%c%c%c%c%c",
		sValue1234[12], sValue1234[13], sValue1234[14],
		sValue1234[15], sValue1234[0], sValue1234[1]);
	snprintf (sValue3, 6 + 2, "%c%c%c%c%c%c",
		sValue1234[22], sValue1234[23], sValue1234[8],
		sValue1234[9], sValue1234[10], sValue1234[11]);
	snprintf (sValue4, 6 + 2, "%c%c%c%c%c%c",
		sValue1234[16], sValue1234[17], sValue1234[18],
		sValue1234[19], sValue1234[20], sValue1234[21]);
	arTiles[iRoom][iOut1][iRow] = arDefinitions[BinToDec (sValue1)];
	arTiles[iRoom][iOut2][iRow] = arDefinitions[BinToDec (sValue2)];
	arTiles[iRoom][iOut3][iRow] = arDefinitions[BinToDec (sValue3)];
	arTiles[iRoom][iOut4][iRow] = arDefinitions[BinToDec (sValue4)];
}
/*****************************************************************************/
void InitScreenAction (char *sAction)
/*****************************************************************************/
{
	if (strcmp (sAction, "left") == 0)
	{
		switch (iScreen)
		{
			case 1:
				iSelectedCol--;
				if (iSelectedCol == 0) { iSelectedCol = 16; }
				break;
			case 2:
				if (iBrokenRoomLinks == 0)
				{
					if (iMovingRoom != 0)
					{
						if (iMovingNewX != 1) { iMovingNewX--; }
							else { iMovingNewX = 25; }
						PlaySound ("wav/cross.wav");
					}
				} else {
					if (iChangingBrokenSide != 1)
					{
						iChangingBrokenSide = 1;
					} else {
						switch (iChangingBrokenRoom)
						{
							case 1: iChangingBrokenRoom = 4; break;
							case 5: iChangingBrokenRoom = 8; break;
							case 9: iChangingBrokenRoom = 12; break;
							case 13: iChangingBrokenRoom = 16; break;
							case 17: iChangingBrokenRoom = 20; break;
							case 21: iChangingBrokenRoom = 24; break;
							default: iChangingBrokenRoom--; break;
						}
					}
				}
				break;
			case 3:
				if (arElements[iCurLevel][iCurRoom][iActiveElement][0] > 0x01)
				{
					arElements[iCurLevel][iCurRoom][iActiveElement][0]--;
					PlaySound ("wav/plus_minus.wav");
					iChanged++;
				}
				break;
		}
	}

	if (strcmp (sAction, "right") == 0)
	{
		switch (iScreen)
		{
			case 1:
				iSelectedCol++;
				if (iSelectedCol == 17) { iSelectedCol = 1; }
				break;
			case 2:
				if (iBrokenRoomLinks == 0)
				{
					if (iMovingRoom != 0)
					{
						if (iMovingNewX != 25) { iMovingNewX++; }
							else { iMovingNewX = 1; }
						PlaySound ("wav/cross.wav");
					}
				} else {
					if (iChangingBrokenSide != 2)
					{
						iChangingBrokenSide = 2;
					} else {
						switch (iChangingBrokenRoom)
						{
							case 4: iChangingBrokenRoom = 1; break;
							case 8: iChangingBrokenRoom = 5; break;
							case 12: iChangingBrokenRoom = 9; break;
							case 16: iChangingBrokenRoom = 13; break;
							case 20: iChangingBrokenRoom = 17; break;
							case 24: iChangingBrokenRoom = 21; break;
							default: iChangingBrokenRoom++; break;
						}
					}
				}
				break;
			case 3:
				if (arElements[iCurLevel][iCurRoom][iActiveElement][0] < 0xFE)
				{
					arElements[iCurLevel][iCurRoom][iActiveElement][0]++;
					PlaySound ("wav/plus_minus.wav");
					iChanged++;
				}
				break;
		}
	}

	if (strcmp (sAction, "up") == 0)
	{
		switch (iScreen)
		{
			case 1:
				iSelectedRow--;
				if (iSelectedRow == 0) { iSelectedRow = 12; }
				break;
			case 2:
				if (iBrokenRoomLinks == 0)
				{
					if (iMovingRoom != 0)
					{
						if (iMovingNewY != 1) { iMovingNewY--; }
							else { iMovingNewY = 24; }
						PlaySound ("wav/cross.wav");
					}
				} else {
					if (iChangingBrokenSide != 3)
					{
						iChangingBrokenSide = 3;
					} else {
						switch (iChangingBrokenRoom)
						{
							case 1: iChangingBrokenRoom = 21; break;
							case 2: iChangingBrokenRoom = 22; break;
							case 3: iChangingBrokenRoom = 23; break;
							case 4: iChangingBrokenRoom = 24; break;
							default: iChangingBrokenRoom -= 4; break;
						}
					}
				}
				break;
		}
	}

	if (strcmp (sAction, "down") == 0)
	{
		switch (iScreen)
		{
			case 1:
				iSelectedRow++;
				if (iSelectedRow == 13) { iSelectedRow = 1; }
				break;
			case 2:
				if (iBrokenRoomLinks == 0)
				{
					if (iMovingRoom != 0)
					{
						if (iMovingNewY != 24) { iMovingNewY++; }
							else { iMovingNewY = 1; }
						PlaySound ("wav/cross.wav");
					}
				} else {
					if (iChangingBrokenSide != 4)
					{
						iChangingBrokenSide = 4;
					} else {
						switch (iChangingBrokenRoom)
						{
							case 21: iChangingBrokenRoom = 1; break;
							case 22: iChangingBrokenRoom = 2; break;
							case 23: iChangingBrokenRoom = 3; break;
							case 24: iChangingBrokenRoom = 4; break;
							default: iChangingBrokenRoom += 4; break;
						}
					}
				}
				break;
		}
	}

	if (strcmp (sAction, "left bracket") == 0)
	{
		if (iScreen == 2)
		{
			if (iBrokenRoomLinks == 0)
			{
				iMovingNewBusy = 0;
				switch (iMovingRoom)
				{
					case 0: iMovingRoom = ROOMS; break;
					case 1: iMovingRoom = ROOMS; break;
					default: iMovingRoom--; break;
				}
			}
		}
	}

	if (strcmp (sAction, "right bracket") == 0)
	{
		if (iScreen == 2)
		{
			if (iBrokenRoomLinks == 0)
			{
				iMovingNewBusy = 0;
				switch (iMovingRoom)
				{
					case 0: iMovingRoom = 1; break;
					case 24: iMovingRoom = 1; break;
					default: iMovingRoom++; break;
				}
			}
		}
	}

	if (strcmp (sAction, "enter") == 0)
	{
		switch (iScreen)
		{
			case 1:
				ChangePos (iSelectedCol, iSelectedRow);
				break;
			case 2:
				if (iBrokenRoomLinks == 0)
				{
					if (iMovingRoom != 0)
					{
						if (iRoomArray[iMovingNewX][iMovingNewY] == 0)
						{
							RemoveOldRoom();
							AddNewRoom (iMovingNewX, iMovingNewY, iMovingRoom);
							iChanged++;
						}
						iMovingRoom = 0; iMovingNewBusy = 0;
					}
				} else {
					LinkPlus();
				}
				break;
		}
	}

	if (strcmp (sAction, "env") == 0)
	{
		switch (cCurType)
		{
			case 'd':
				cCurType = 'p'; arLevelType[iCurLevel] = 'p'; break;
			case 'p':
				cCurType = 'd'; arLevelType[iCurLevel] = 'd'; break;
		}
		PlaySound ("wav/extras.wav");
		iChanged++;
	}
}
/*****************************************************************************/
void InitScreen (void)
/*****************************************************************************/
{
	SDL_AudioSpec fmt;
	char sImage[MAX_IMG + 2];
	SDL_Surface *imgicon;
	int iJoyNr;
	SDL_Rect barbox;
	SDL_Event event;
	DIR *dDir;
	struct dirent *stDirent;
	char sTile[3 + 2]; /*** 2 chars, plus \0 ***/
	int iXPosOld, iYPosOld;
	char sExtra[20 + 2];
	char sRoom[20 + 2];
	const Uint8 *keystate;
	Uint32 oldticks, newticks;
	int iXJoy1, iYJoy1, iXJoy2, iYJoy2;
	int iUseTile;
	int iElem;

	/*** Used for looping. ***/
	int iLoopCol;
	int iLoopRow;
	int iLoopExtra;
	int iLoopRoom;
	int iLoopX, iLoopY;

	if (SDL_Init (SDL_INIT_AUDIO|SDL_INIT_VIDEO|
		SDL_INIT_GAMECONTROLLER|SDL_INIT_HAPTIC) < 0)
	{
		printf ("[FAILED] Unable to init SDL: %s!\n", SDL_GetError());
		exit (EXIT_ERROR);
	}
	atexit (SDL_Quit);

	/*** main window ***/
	window = SDL_CreateWindow (EDITOR_NAME " " EDITOR_VERSION,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		(WINDOW_WIDTH) * iScale, (WINDOW_HEIGHT) * iScale, iFullscreen);
	if (window == NULL)
	{
		printf ("[FAILED] Unable to create main window: %s!\n", SDL_GetError());
		exit (EXIT_ERROR);
	}
	iWindowID = SDL_GetWindowID (window);
	iActiveWindowID = iWindowID;
	ascreen = SDL_CreateRenderer (window, -1, 0);
	if (ascreen == NULL)
	{
		printf ("[FAILED] Unable to set video mode: %s!\n", SDL_GetError());
		exit (EXIT_ERROR);
	}

	/*** Some people may prefer linear, but we're going old school. ***/
	SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	if (iFullscreen != 0)
	{
		SDL_RenderSetLogicalSize (ascreen, (WINDOW_WIDTH) * iScale,
			(WINDOW_HEIGHT) * iScale);
	}

	if (TTF_Init() == -1)
	{
		printf ("[FAILED] Could not initialize TTF!\n");
		exit (EXIT_ERROR);
	}

	LoadFonts();

	curArrow = SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_ARROW);
	curWait = SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_WAIT);
	curHand = SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_HAND);

	if (iNoAudio != 1)
	{
		PrIfDe ("[  OK  ] Initializing Audio\n");
		fmt.freq = 44100;
		fmt.format = AUDIO_S16;
		fmt.channels = 2;
		fmt.samples = 512;
		fmt.callback = MixAudio;
		fmt.userdata = NULL;
		if (SDL_OpenAudio (&fmt, NULL) < 0)
		{
			printf ("[FAILED] Unable to open audio: %s!\n", SDL_GetError());
			exit (EXIT_ERROR);
		}
		SDL_PauseAudio (0);
	}

	/*** main window icon ***/
	snprintf (sImage, MAX_IMG, "%s%s", PNG_VARIOUS, "lemsop_icon.png");
	imgicon = IMG_Load (sImage);
	if (imgicon == NULL)
	{
		printf ("[ WARN ] Could not load \"%s\": %s!\n", sImage, strerror (errno));
	} else {
		SDL_SetWindowIcon (window, imgicon);
	}

	/*** Open the first available controller. ***/
	iController = 0;
	if (iNoController != 1)
	{
		for (iJoyNr = 0; iJoyNr < SDL_NumJoysticks(); iJoyNr++)
		{
			if (SDL_IsGameController (iJoyNr))
			{
				controller = SDL_GameControllerOpen (iJoyNr);
				if (controller)
				{
					snprintf (sControllerName, MAX_CON, "%s",
						SDL_GameControllerName (controller));
					if (iDebug == 1)
					{
						printf ("[ INFO ] Found a controller \"%s\"; \"%s\".\n",
							sControllerName, SDL_GameControllerNameForIndex (iJoyNr));
					}
					joystick = SDL_GameControllerGetJoystick (controller);
					iController = 1;

					/*** Just for fun, use haptic. ***/
					if (SDL_JoystickIsHaptic (joystick))
					{
						haptic = SDL_HapticOpenFromJoystick (joystick);
						if (SDL_HapticRumbleInit (haptic) == 0)
						{
							SDL_HapticRumblePlay (haptic, 1.0, 1000);
						} else {
							printf ("[ WARN ] Could not initialize the haptic device: %s\n",
								SDL_GetError());
						}
					} else {
						PrIfDe ("[ INFO ] The game controller is not haptic.\n");
					}
				} else {
					printf ("[ WARN ] Could not open game controller %i: %s\n",
						iController, SDL_GetError());
				}
			}
		}
		if (iController != 1) { PrIfDe ("[ INFO ] No controller found.\n"); }
	} else {
		PrIfDe ("[ INFO ] Using keyboard and mouse.\n");
	}

	/*******************/
	/* Preload images. */
	/*******************/

	/*** loading ***/
	SDL_SetCursor (curWait);
	PreLoad (PNG_VARIOUS, "loading.png", &imgloading);
	ShowImage (imgloading, 0, 0, "imgloading", ascreen, iScale, 1);
	SDL_SetRenderDrawColor (ascreen, 0x22, 0x22, 0x22, SDL_ALPHA_OPAQUE);
	barbox.x = 10 * iScale;
	barbox.y = 10 * iScale;
	barbox.w = 20 * iScale;
	barbox.h = 439 * iScale;
	SDL_RenderFillRect (ascreen, &barbox);
	SDL_RenderPresent (ascreen);

	iPreLoaded = 0;
	iCurrentBarHeight = 0;
	iNrToPreLoad = 325; /*** Value can be obtained via debug mode. ***/
	SDL_SetCursor (curWait);

	/*** buttons ***/
	PreLoad (PNG_BUTTONS, "left_0.png", &imgleft_0);
	PreLoad (PNG_BUTTONS, "left_1.png", &imgleft_1);
	PreLoad (PNG_BUTTONS, "right_0.png", &imgright_0);
	PreLoad (PNG_BUTTONS, "right_1.png", &imgright_1);
	PreLoad (PNG_BUTTONS, "up_0.png", &imgup_0);
	PreLoad (PNG_BUTTONS, "up_1.png", &imgup_1);
	PreLoad (PNG_BUTTONS, "down_0.png", &imgdown_0);
	PreLoad (PNG_BUTTONS, "down_1.png", &imgdown_1);
	PreLoad (PNG_BUTTONS, "left_right_no.png", &imglrno);
	PreLoad (PNG_BUTTONS, "up_down_no.png", &imgudno);
	if (iController == 0)
	{
		PreLoad (PNG_BUTTONS, "OK.png", &imgok[1]);
		PreLoad (PNG_BUTTONS, "sel_OK.png", &imgok[2]);
		PreLoad (PNG_BUTTONS, "close_big_0.png", &imgclosebig_0);
		PreLoad (PNG_BUTTONS, "close_big_1.png", &imgclosebig_1);
		PreLoad (PNG_BUTTONS, "rooms_on_0.png", &imgroomson_0);
		PreLoad (PNG_BUTTONS, "rooms_on_1.png", &imgroomson_1);
		PreLoad (PNG_BUTTONS, "rooms_off.png", &imgroomsoff);
		PreLoad (PNG_BUTTONS, "broken_rooms_on_0.png", &imgbroomson_0);
		PreLoad (PNG_BUTTONS, "broken_rooms_on_1.png", &imgbroomson_1);
		PreLoad (PNG_BUTTONS, "broken_rooms_off.png", &imgbroomsoff);
		PreLoad (PNG_BUTTONS, "elements_on_0.png", &imgelementson_0);
		PreLoad (PNG_BUTTONS, "elements_on_1.png", &imgelementson_1);
		PreLoad (PNG_BUTTONS, "elements_off.png", &imgelementsoff);
		PreLoad (PNG_BUTTONS, "save_on_0.png", &imgsaveon_0);
		PreLoad (PNG_BUTTONS, "save_on_1.png", &imgsaveon_1);
		PreLoad (PNG_BUTTONS, "save_off.png", &imgsaveoff);
		PreLoad (PNG_BUTTONS, "quit_0.png", &imgquit_0);
		PreLoad (PNG_BUTTONS, "quit_1.png", &imgquit_1);
		PreLoad (PNG_BUTTONS, "prev_on_0.png", &imgprevon_0);
		PreLoad (PNG_BUTTONS, "prev_on_1.png", &imgprevon_1);
		PreLoad (PNG_BUTTONS, "prev_off.png", &imgprevoff);
		PreLoad (PNG_BUTTONS, "next_on_0.png", &imgnexton_0);
		PreLoad (PNG_BUTTONS, "next_on_1.png", &imgnexton_1);
		PreLoad (PNG_BUTTONS, "next_off.png", &imgnextoff);
		PreLoad (PNG_BUTTONS, "up_down_no_nfo.png", &imgudnonfo);
		PreLoad (PNG_BUTTONS, "Save.png", &imgsave[1]);
		PreLoad (PNG_BUTTONS, "sel_Save.png", &imgsave[2]);
		PreLoad (PNG_BUTTONS, "Yes.png", &imgyes[1]);
		PreLoad (PNG_BUTTONS, "sel_Yes.png", &imgyes[2]);
		PreLoad (PNG_BUTTONS, "No.png", &imgno[1]);
		PreLoad (PNG_BUTTONS, "sel_No.png", &imgno[2]);
	} else {
		PreLoad (PNG_GAMEPAD, "OK.png", &imgok[1]);
		PreLoad (PNG_GAMEPAD, "sel_OK.png", &imgok[2]);
		PreLoad (PNG_GAMEPAD, "close_big_0.png", &imgclosebig_0);
		PreLoad (PNG_GAMEPAD, "close_big_1.png", &imgclosebig_1);
		PreLoad (PNG_GAMEPAD, "rooms_on_0.png", &imgroomson_0);
		PreLoad (PNG_GAMEPAD, "rooms_on_1.png", &imgroomson_1);
		PreLoad (PNG_GAMEPAD, "rooms_off.png", &imgroomsoff);
		PreLoad (PNG_GAMEPAD, "broken_rooms_on_0.png", &imgbroomson_0);
		PreLoad (PNG_GAMEPAD, "broken_rooms_on_1.png", &imgbroomson_1);
		PreLoad (PNG_GAMEPAD, "broken_rooms_off.png", &imgbroomsoff);
		PreLoad (PNG_GAMEPAD, "elements_on_0.png", &imgelementson_0);
		PreLoad (PNG_GAMEPAD, "elements_on_1.png", &imgelementson_1);
		PreLoad (PNG_GAMEPAD, "elements_off.png", &imgelementsoff);
		PreLoad (PNG_GAMEPAD, "save_on_0.png", &imgsaveon_0);
		PreLoad (PNG_GAMEPAD, "save_on_1.png", &imgsaveon_1);
		PreLoad (PNG_GAMEPAD, "save_off.png", &imgsaveoff);
		PreLoad (PNG_GAMEPAD, "quit_0.png", &imgquit_0);
		PreLoad (PNG_GAMEPAD, "quit_1.png", &imgquit_1);
		PreLoad (PNG_GAMEPAD, "prev_on_0.png", &imgprevon_0);
		PreLoad (PNG_GAMEPAD, "prev_on_1.png", &imgprevon_1);
		PreLoad (PNG_GAMEPAD, "prev_off.png", &imgprevoff);
		PreLoad (PNG_GAMEPAD, "next_on_0.png", &imgnexton_0);
		PreLoad (PNG_GAMEPAD, "next_on_1.png", &imgnexton_1);
		PreLoad (PNG_GAMEPAD, "next_off.png", &imgnextoff);
		PreLoad (PNG_GAMEPAD, "up_down_no_nfo.png", &imgudnonfo);
		PreLoad (PNG_GAMEPAD, "Save.png", &imgsave[1]);
		PreLoad (PNG_GAMEPAD, "sel_Save.png", &imgsave[2]);
		PreLoad (PNG_GAMEPAD, "Yes.png", &imgyes[1]);
		PreLoad (PNG_GAMEPAD, "sel_Yes.png", &imgyes[2]);
		PreLoad (PNG_GAMEPAD, "No.png", &imgno[1]);
		PreLoad (PNG_GAMEPAD, "sel_No.png", &imgno[2]);
	}

	/*** elements ***/
	PreLoad (PNG_ELEMENTS, "kid_l.png", &imgkidl);
	PreLoad (PNG_ELEMENTS, "kid_r.png", &imgkidr);
	PreLoad (PNG_ELEMENTS, "guard_l.png", &imgguardl);
	PreLoad (PNG_ELEMENTS, "guard_r.png", &imgguardr);
	PreLoad (PNG_ELEMENTS, "skeleton_l.png", &imgskell);
	PreLoad (PNG_ELEMENTS, "skeleton_r.png", &imgskelr);
	PreLoad (PNG_ELEMENTS, "Jaffar_l.png", &imgjaffarl);
	PreLoad (PNG_ELEMENTS, "Jaffar_r.png", &imgjaffarr);
	PreLoad (PNG_ELEMENTS, "loose.png", &imgloose);
	PreLoad (PNG_ELEMENTS, "loose_fall.png", &imgloosefall);
	PreLoad (PNG_ELEMENTS, "gate.png", &imggate);
	PreLoad (PNG_ELEMENTS, "sword.png", &imgsword);
	PreLoad (PNG_ELEMENTS, "spikes.png", &imgspikes);
	PreLoad (PNG_ELEMENTS, "level_door.png", &imgleveldoor);
	PreLoad (PNG_ELEMENTS, "chomper.png", &imgchomper);
	PreLoad (PNG_ELEMENTS, "mouse.png", &imgmouse);
	PreLoad (PNG_ELEMENTS, "shadow_step.png", &imgshadowstep);
	PreLoad (PNG_ELEMENTS, "shadow_fight.png", &imgshadowfight);
	PreLoad (PNG_ELEMENTS, "shadow_drink.png", &imgshadowdrink);
	PreLoad (PNG_ELEMENTS, "potion_red.png", &imgpotionred);
	PreLoad (PNG_ELEMENTS, "potion_blue.png", &imgpotionblue);
	PreLoad (PNG_ELEMENTS, "potion_green.png", &imgpotiongreen);
	PreLoad (PNG_ELEMENTS, "potion_pink.png", &imgpotionpink);
	PreLoad (PNG_ELEMENTS, "mirror.png", &imgmirror);
	PreLoad (PNG_ELEMENTS, "skel_alive.png", &imgskelalive);
	PreLoad (PNG_ELEMENTS, "skel_respawn.png", &imgskelrespawn);
	PreLoad (PNG_ELEMENTS, "drop_d.png", &imgdropd);
	PreLoad (PNG_ELEMENTS, "drop_d_wall.png", &imgdropdwall);
	PreLoad (PNG_ELEMENTS, "drop_p.png", &imgdropp);
	PreLoad (PNG_ELEMENTS, "drop_p_wall.png", &imgdroppwall);
	PreLoad (PNG_ELEMENTS, "raise_d.png", &imgraised);
	PreLoad (PNG_ELEMENTS, "raise_d_wall.png", &imgraisedwall);
	PreLoad (PNG_ELEMENTS, "raise_p.png", &imgraisep);
	PreLoad (PNG_ELEMENTS, "raise_p_wall.png", &imgraisepwall);
	PreLoad (PNG_ELEMENTS, "princess.png", &imgprincess);

	/*** imgdungeon[], imgpalace[], imgselected, imgunknown, imgmask ***/
	dDir = opendir (PNG_DUNGEON);
	if (dDir == NULL)
	{
		printf ("[FAILED] Cannot open directory \"%s\": %s!\n",
			PNG_DUNGEON, strerror (errno));
		exit (EXIT_ERROR);
	}
	while ((stDirent = readdir (dDir)) != NULL)
	{
		if ((strcmp (stDirent->d_name, ".") != 0) &&
			(strcmp (stDirent->d_name, "..") != 0))
		{
			snprintf (sTile, 3 + 2, "%c%c",
				stDirent->d_name[0], stDirent->d_name[1]);
			PreLoad (PNG_DUNGEON, stDirent->d_name,
				&imgdungeon[atoi (sTile)]);
		}
	}
	closedir (dDir);
	dDir = opendir (PNG_PALACE);
	if (dDir == NULL)
	{
		printf ("[FAILED] Cannot open directory \"%s\": %s!\n",
			PNG_PALACE, strerror (errno));
		exit (EXIT_ERROR);
	}
	while ((stDirent = readdir (dDir)) != NULL)
	{
		if ((strcmp (stDirent->d_name, ".") != 0) &&
			(strcmp (stDirent->d_name, "..") != 0))
		{
			snprintf (sTile, 3 + 2, "%c%c",
				stDirent->d_name[0], stDirent->d_name[1]);
			PreLoad (PNG_PALACE, stDirent->d_name,
				&imgpalace[atoi (sTile)]);
		}
	}
	closedir (dDir);
	PreLoad (PNG_VARIOUS, "selected.png", &imgselected);
	PreLoad (PNG_VARIOUS, "unknown.png", &imgunknown);
	PreLoad (PNG_VARIOUS, "mask.png", &imgmask);

	/*** extras ***/
	for (iLoopExtra = 0; iLoopExtra <= 10; iLoopExtra++)
	{
		snprintf (sExtra, 20, "extras_%02i.png", iLoopExtra);
		PreLoad (PNG_EXTRAS, sExtra, &extras[iLoopExtra]);
	}

	/*** 14x14 rooms ***/
	for (iLoopRoom = 1; iLoopRoom <= (ROOMS + 1); iLoopRoom++)
	{
		snprintf (sRoom, 20, "room%i.png", iLoopRoom);
		PreLoad (PNG_ROOMS, sRoom, &imgroom[iLoopRoom]);
	}

	/*** various ***/
	PreLoad (PNG_VARIOUS, "black.png", &imgblack);
	PreLoad (PNG_VARIOUS, "faded.png", &imgfaded);
	PreLoad (PNG_VARIOUS, "border_live.png", &imgborderl);
	PreLoad (PNG_VARIOUS, "sel_room_current.png", &imgsrc);
	PreLoad (PNG_VARIOUS, "sel_room_start.png", &imgsrs);
	PreLoad (PNG_VARIOUS, "sel_room_moving.png", &imgsrm);
	PreLoad (PNG_VARIOUS, "sel_room_cross.png", &imgsrp);
	PreLoad (PNG_VARIOUS, "sel_room_broken.png", &imgsrb);
	PreLoad (PNG_VARIOUS, "sel_element.png", &imgsele);
	PreLoad (PNG_VARIOUS, "sel_guard.png", &imgselg);
	PreLoad (PNG_VARIOUS, "help.png", &imghelp);
	PreLoad (PNG_VARIOUS, "exe.png", &imgexe);
	PreLoad (PNG_VARIOUS, "statusbar_sprite.png", &imgstatusbarsprite);
	PreLoad (PNG_VARIOUS, "Mednafen.png", &imgplaytest);
	PreLoad (PNG_VARIOUS, "popup_yn.png", &imgpopup_yn);
	PreLoad (PNG_VARIOUS, "link_warn_lr.png", &imglinkwarnlr);
	PreLoad (PNG_VARIOUS, "link_warn_ud.png", &imglinkwarnud);
	PreLoad (PNG_VARIOUS, "49_sprite.png", &spriteflamed);
	PreLoad (PNG_VARIOUS, "51_sprite.png", &spriteflamep);
	PreLoad (PNG_VARIOUS, "dungeon.png", &imgdungeont);
	PreLoad (PNG_VARIOUS, "palace.png", &imgpalacet);
	PreLoad (PNG_VARIOUS, "guards.png", &imgguards);
	if (iController == 0)
	{
		PreLoad (PNG_VARIOUS, "popup.png", &imgpopup);
		PreLoad (PNG_VARIOUS, "level_bar.png", &imgbar);
		PreLoad (PNG_VARIOUS, "border.png", &imgborder);
		PreLoad (PNG_VARIOUS, "room_links.png", &imgrl);
		PreLoad (PNG_VARIOUS, "broken_room_links.png", &imgbrl);
		PreLoad (PNG_VARIOUS, "elements.png", &imgelements);
	} else {
		PreLoad (PNG_GAMEPAD, "popup.png", &imgpopup);
		PreLoad (PNG_GAMEPAD, "level_bar.png", &imgbar);
		PreLoad (PNG_GAMEPAD, "border.png", &imgborder);
		PreLoad (PNG_GAMEPAD, "room_links.png", &imgrl);
		PreLoad (PNG_GAMEPAD, "broken_room_links.png", &imgbrl);
		PreLoad (PNG_GAMEPAD, "elements.png", &imgelements);
	}

	/*** chars ***/
	PreLoad (PNG_CHARS, "0.png", &imgchars[0]);
	PreLoad (PNG_CHARS, "1.png", &imgchars[1]);
	PreLoad (PNG_CHARS, "2.png", &imgchars[2]);
	PreLoad (PNG_CHARS, "3.png", &imgchars[3]);
	PreLoad (PNG_CHARS, "4.png", &imgchars[4]);
	PreLoad (PNG_CHARS, "5.png", &imgchars[5]);
	PreLoad (PNG_CHARS, "6.png", &imgchars[6]);
	PreLoad (PNG_CHARS, "7.png", &imgchars[7]);
	PreLoad (PNG_CHARS, "8.png", &imgchars[8]);
	PreLoad (PNG_CHARS, "9.png", &imgchars[9]);
	PreLoad (PNG_CHARS, "A.png", &imgchars[10]);
	PreLoad (PNG_CHARS, "B.png", &imgchars[11]);
	PreLoad (PNG_CHARS, "C.png", &imgchars[12]);
	PreLoad (PNG_CHARS, "D.png", &imgchars[13]);
	PreLoad (PNG_CHARS, "E.png", &imgchars[14]);
	PreLoad (PNG_CHARS, "F.png", &imgchars[15]);
	PreLoad (PNG_CHARS, "G.png", &imgchars[16]);
	PreLoad (PNG_CHARS, "H.png", &imgchars[17]);
	PreLoad (PNG_CHARS, "I.png", &imgchars[18]);
	PreLoad (PNG_CHARS, "J.png", &imgchars[19]);
	PreLoad (PNG_CHARS, "K.png", &imgchars[20]);
	PreLoad (PNG_CHARS, "L.png", &imgchars[21]);
	PreLoad (PNG_CHARS, "M.png", &imgchars[22]);
	PreLoad (PNG_CHARS, "N.png", &imgchars[23]);
	PreLoad (PNG_CHARS, "O.png", &imgchars[24]);
	PreLoad (PNG_CHARS, "P.png", &imgchars[25]);
	PreLoad (PNG_CHARS, "Q.png", &imgchars[26]);
	PreLoad (PNG_CHARS, "R.png", &imgchars[27]);
	PreLoad (PNG_CHARS, "S.png", &imgchars[28]);
	PreLoad (PNG_CHARS, "T.png", &imgchars[29]);
	PreLoad (PNG_CHARS, "U.png", &imgchars[30]);
	PreLoad (PNG_CHARS, "V.png", &imgchars[31]);
	PreLoad (PNG_CHARS, "W.png", &imgchars[32]);
	PreLoad (PNG_CHARS, "X.png", &imgchars[33]);
	PreLoad (PNG_CHARS, "Y.png", &imgchars[34]);
	PreLoad (PNG_CHARS, "Z.png", &imgchars[35]);
	PreLoad (PNG_CHARS, "ampersand.png", &imgchars[36]);
	PreLoad (PNG_CHARS, "apostrophe.png", &imgchars[37]);
	PreLoad (PNG_CHARS, "asterisk.png", &imgchars[38]);
	PreLoad (PNG_CHARS, "at.png", &imgchars[39]);
	PreLoad (PNG_CHARS, "close_parenthesis.png", &imgchars[40]);
	PreLoad (PNG_CHARS, "colon.png", &imgchars[41]);
	PreLoad (PNG_CHARS, "comma.png", &imgchars[42]);
	PreLoad (PNG_CHARS, "dollar.png", &imgchars[43]);
	PreLoad (PNG_CHARS, "equals.png", &imgchars[44]);
	PreLoad (PNG_CHARS, "exclamation.png", &imgchars[45]);
	PreLoad (PNG_CHARS, "greater-than.png", &imgchars[46]);
	PreLoad (PNG_CHARS, "hash.png", &imgchars[47]);
	PreLoad (PNG_CHARS, "hyphen-minus.png", &imgchars[48]);
	PreLoad (PNG_CHARS, "less-than.png", &imgchars[49]);
	PreLoad (PNG_CHARS, "open_parenthesis.png", &imgchars[50]);
	PreLoad (PNG_CHARS, "period.png", &imgchars[51]);
	PreLoad (PNG_CHARS, "plus.png", &imgchars[52]);
	PreLoad (PNG_CHARS, "question.png", &imgchars[53]);
	PreLoad (PNG_CHARS, "quotes.png", &imgchars[54]);
	PreLoad (PNG_CHARS, "slash.png", &imgchars[55]);
	PreLoad (PNG_CHARS, "slashed_O.png", &imgchars[56]);
	PreLoad (PNG_CHARS, "space.png", &imgchars[57]);
	PreLoad (PNG_VARIOUS, "active_char.png", &imgactivechar);

	/*** One final update of the bar. ***/
	LoadingBar (BAR_FULL);

	if (iDebug == 1)
		{ printf ("[ INFO ] Preloaded images: %i\n", iPreLoaded); }
	SDL_SetCursor (curArrow);

	iCurLevel = iStartLevel;
	if (iDebug == 1)
		{ printf ("[ INFO ] Starting in level: %i\n", iStartLevel); }

	iSelectedCol = 1; /*** Start with the upper ***/
	iSelectedRow = 1; /*** left selected. ***/
	iScreen = 1;
	iDownAt = 0;
	iFlameFrameDP = 1;
	oldticks = 0;

	ShowScreen (iScreen);
	InitPopUp();
	while (1)
	{
		if (iNoAnim == 0)
		{
			/*** This is for the animation; 20 fps (1000/50) is fine for PoP. ***/
			newticks = SDL_GetTicks();
			if (newticks > oldticks + 50)
			{
				iFlameFrameDP++;
				if (iFlameFrameDP == 4) { iFlameFrameDP = 1; }
				ShowScreen (iScreen);
				oldticks = newticks;
			}
		}

		while (SDL_PollEvent (&event))
		{
			if (MapEvents (event) == 0)
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
							InitScreenAction ("enter");
							break;
						case SDL_CONTROLLER_BUTTON_B:
							switch (iScreen)
							{
								case 1:
									Quit(); break;
								case 2:
									iBrokenRoomLinks = BrokenRoomLinks (0, iCurLevel);
									iScreen = 1; break;
								case 3:
									iScreen = 1; break;
							}
							break;
						case SDL_CONTROLLER_BUTTON_X:
							if (iScreen != 2)
							{
								iScreen = 2;
								iMovingRoom = 0;
								iMovingNewBusy = 0;
								iChangingBrokenRoom = iCurRoom;
								iChangingBrokenSide = 1;
								PlaySound ("wav/screen2or3.wav");
							} else if (iBrokenRoomLinks == 0) {
								iBrokenRoomLinks = 1;
								PlaySound ("wav/screen2or3.wav");
							}
							break;
						case SDL_CONTROLLER_BUTTON_Y:
							if (iScreen == 2)
							{
								/*** Why? ***/
								iBrokenRoomLinks = BrokenRoomLinks (0, iCurLevel);
							}
							if (iScreen != 3)
							{
								iScreen = 3;
								PlaySound ("wav/screen2or3.wav");
							}
							break;
						case SDL_CONTROLLER_BUTTON_BACK:
							if (iScreen == 2)
							{
								if (iBrokenRoomLinks == 1)
								{
									LinkMinus();
								}
							}
							if (iScreen == 3)
							{
								/*** active (8) ***/
								if (arElements[iCurLevel][iCurRoom][iActiveElement][8] == 0x00)
								{
									arElements[iCurLevel][iCurRoom][iActiveElement][8] = 0x01;
								} else {
									arElements[iCurLevel][iCurRoom][iActiveElement][8] = 0x00;
								}
								PlaySound ("wav/check_box.wav");
								iChanged++;
							}
							break;
						case SDL_CONTROLLER_BUTTON_GUIDE:
							SaveLevel (iCurLevel); break;
						case SDL_CONTROLLER_BUTTON_START:
							RunLevel (iCurLevel); break;
						case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
							if ((iChanged != 0) && (iCurLevel != 1)) { InitPopUpSave(); }
							Prev(); break;
						case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
							if ((iChanged != 0) && (iCurLevel != 14)) { InitPopUpSave(); }
							Next(); break;
						case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
							InitScreenAction ("left"); break;
						case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
							InitScreenAction ("right"); break;
						case SDL_CONTROLLER_BUTTON_DPAD_UP:
							InitScreenAction ("up"); break;
						case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
							InitScreenAction ("down"); break;
					}
					ShowScreen (iScreen);
					break;
				case SDL_CONTROLLERAXISMOTION: /*** triggers and analog sticks ***/
					iXJoy1 = SDL_JoystickGetAxis (joystick, 0);
					iYJoy1 = SDL_JoystickGetAxis (joystick, 1);
					iXJoy2 = SDL_JoystickGetAxis (joystick, 3);
					iYJoy2 = SDL_JoystickGetAxis (joystick, 4);
					if ((iXJoy1 < -30000) || (iXJoy2 < -30000)) /*** left ***/
					{
						if ((SDL_GetTicks() - joyleft) > 300)
						{
							switch (iScreen)
							{
								case 1:
									if ((iRoomConnections[iCurLevel][iCurRoom][1] != 256) &&
										(iRoomConnections[iCurLevel][iCurRoom][1] <= ROOMS))
									{
										iCurRoom = iRoomConnections[iCurLevel][iCurRoom][1];
										PlaySound ("wav/scroll.wav");
									}
									break;
								case 3:
									if (iActiveElement > 1)
									{
										iActiveElement--;
										PlaySound ("wav/plus_minus.wav");
									}
									break;
							}
							joyleft = SDL_GetTicks();
						}
					}
					if ((iXJoy1 > 30000) || (iXJoy2 > 30000)) /*** right ***/
					{
						if ((SDL_GetTicks() - joyright) > 300)
						{
							switch (iScreen)
							{
								case 1:
									if ((iRoomConnections[iCurLevel][iCurRoom][2] != 256) &&
										(iRoomConnections[iCurLevel][iCurRoom][2] <= ROOMS))
									{
										iCurRoom = iRoomConnections[iCurLevel][iCurRoom][2];
										PlaySound ("wav/scroll.wav");
									}
									break;
								case 3:
									if (iActiveElement < MAX_ELEM)
									{
										iActiveElement++;
										PlaySound ("wav/plus_minus.wav");
									}
									break;
							}
							joyright = SDL_GetTicks();
						}
					}
					if ((iYJoy1 < -30000) || (iYJoy2 < -30000)) /*** up ***/
					{
						if ((SDL_GetTicks() - joyup) > 300)
						{
							switch (iScreen)
							{
								case 1:
									if ((iRoomConnections[iCurLevel][iCurRoom][3] != 256) &&
										(iRoomConnections[iCurLevel][iCurRoom][3] <= ROOMS))
									{
										iCurRoom = iRoomConnections[iCurLevel][iCurRoom][3];
										PlaySound ("wav/scroll.wav");
									}
									break;
								case 3:
									if (iActiveElement < MAX_ELEM)
									{
										iActiveElement+=10;
										if (iActiveElement > MAX_ELEM)
											{ iActiveElement = MAX_ELEM; }
										PlaySound ("wav/plus_minus.wav");
									}
									break;
							}
							joyup = SDL_GetTicks();
						}
					}
					if ((iYJoy1 > 30000) || (iYJoy2 > 30000)) /*** down ***/
					{
						if ((SDL_GetTicks() - joydown) > 300)
						{
							switch (iScreen)
							{
								case 1:
									if ((iRoomConnections[iCurLevel][iCurRoom][4] != 256) &&
										(iRoomConnections[iCurLevel][iCurRoom][4] <= ROOMS))
									{
										iCurRoom = iRoomConnections[iCurLevel][iCurRoom][4];
										PlaySound ("wav/scroll.wav");
									}
									break;
								case 3:
									if (iActiveElement > 1)
									{
										iActiveElement-=10;
										if (iActiveElement < 1)
											{ iActiveElement = 1; }
										PlaySound ("wav/plus_minus.wav");
									}
									break;
							}
							joydown = SDL_GetTicks();
						}
					}
					if (event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT)
					{
						if ((SDL_GetTicks() - trigleft) > 300)
						{
							if (iScreen == 2) { InitScreenAction ("left bracket"); }
							if (iScreen == 3) { InitScreenAction ("left"); }
							trigleft = SDL_GetTicks();
						}
					}
					if (event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
					{
						if ((SDL_GetTicks() - trigright) > 300)
						{
							if (iScreen == 2) { InitScreenAction ("right bracket"); }
							if (iScreen == 3) { InitScreenAction ("right"); }
							trigright = SDL_GetTicks();
						}
					}
					ShowScreen (iScreen);
					break;
				case SDL_KEYDOWN: /*** https://wiki.libsdl.org/SDL_Keycode ***/
					switch (event.key.keysym.sym)
					{
						case SDLK_F1: if (iScreen == 1) { Help(); } break;
						case SDLK_F2: if (iScreen == 1) { EXE(); } break;
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						case SDLK_SPACE:
							if (((event.key.keysym.mod & KMOD_LALT) ||
								(event.key.keysym.mod & KMOD_RALT)) && (iScreen == 1))
							{
								Zoom (1);
								iExtras = 0;
								PlaySound ("wav/extras.wav");
							} else {
								InitScreenAction ("enter");
							}
							break;
						case SDLK_LEFT:
							if ((event.key.keysym.mod & KMOD_LSHIFT) ||
								(event.key.keysym.mod & KMOD_RSHIFT))
							{
								switch (iScreen)
								{
									case 1:
										if ((iRoomConnections[iCurLevel][iCurRoom][1] != 256) &&
											(iRoomConnections[iCurLevel][iCurRoom][1] <= ROOMS))
										{
											iCurRoom = iRoomConnections[iCurLevel][iCurRoom][1];
											PlaySound ("wav/scroll.wav");
										}
										break;
									case 3:
										if (iActiveElement > 1)
										{
											iActiveElement--;
											PlaySound ("wav/plus_minus.wav");
										}
										break;
								}
							} else if ((event.key.keysym.mod & KMOD_LCTRL) ||
								(event.key.keysym.mod & KMOD_RCTRL))
							{
								if (iScreen == 3)
								{
									if (iActiveElement > 1)
									{
										iActiveElement-=10;
										if (iActiveElement < 1)
											{ iActiveElement = 1; }
										PlaySound ("wav/plus_minus.wav");
									}
								}
							} else {
								InitScreenAction ("left");
							}
							break;
						case SDLK_RIGHT:
							if ((event.key.keysym.mod & KMOD_LSHIFT) ||
								(event.key.keysym.mod & KMOD_RSHIFT))
							{
								switch (iScreen)
								{
									case 1:
										if ((iRoomConnections[iCurLevel][iCurRoom][2] != 256) &&
											(iRoomConnections[iCurLevel][iCurRoom][2] <= ROOMS))
										{
											iCurRoom = iRoomConnections[iCurLevel][iCurRoom][2];
											PlaySound ("wav/scroll.wav");
										}
										break;
									case 3:
										if (iActiveElement < MAX_ELEM)
										{
											iActiveElement++;
											PlaySound ("wav/plus_minus.wav");
										}
										break;
								}
							} else if ((event.key.keysym.mod & KMOD_LCTRL) ||
								(event.key.keysym.mod & KMOD_RCTRL))
							{
								if (iScreen == 3)
								{
									if (iActiveElement < MAX_ELEM)
									{
										iActiveElement+=10;
										if (iActiveElement > MAX_ELEM)
											{ iActiveElement = MAX_ELEM; }
										PlaySound ("wav/plus_minus.wav");
									}
								}
							} else {
								InitScreenAction ("right");
							}
							break;
						case SDLK_UP:
							if ((event.key.keysym.mod & KMOD_LSHIFT) ||
								(event.key.keysym.mod & KMOD_RSHIFT))
							{
								if (iScreen == 1)
								{
									if ((iRoomConnections[iCurLevel][iCurRoom][3] != 256) &&
										(iRoomConnections[iCurLevel][iCurRoom][3] <= ROOMS))
									{
										iCurRoom = iRoomConnections[iCurLevel][iCurRoom][3];
										PlaySound ("wav/scroll.wav");
									}
								}
							} else {
								InitScreenAction ("up");
							}
							break;
						case SDLK_DOWN:
							if ((event.key.keysym.mod & KMOD_LSHIFT) ||
								(event.key.keysym.mod & KMOD_RSHIFT))
							{
								if (iScreen == 1)
								{
									if ((iRoomConnections[iCurLevel][iCurRoom][4] != 256) &&
										(iRoomConnections[iCurLevel][iCurRoom][4] <= ROOMS))
									{
										iCurRoom = iRoomConnections[iCurLevel][iCurRoom][4];
										PlaySound ("wav/scroll.wav");
									}
								}
							} else {
								InitScreenAction ("down");
							}
							break;
						case SDLK_LEFTBRACKET:
							InitScreenAction ("left bracket");
							break;
						case SDLK_RIGHTBRACKET:
							InitScreenAction ("right bracket");
							break;
						case SDLK_MINUS:
						case SDLK_KP_MINUS:
							if ((iChanged != 0) && (iCurLevel != 1)) { InitPopUpSave(); }
							Prev(); break;
						case SDLK_KP_PLUS:
						case SDLK_EQUALS:
							if ((iChanged != 0) && (iCurLevel != 14)) { InitPopUpSave(); }
							Next(); break;
						case SDLK_QUOTE:
							if (iScreen == 1)
							{
								if ((event.key.keysym.mod & KMOD_LSHIFT) ||
									(event.key.keysym.mod & KMOD_RSHIFT))
								{
									Sprinkle();
									PlaySound ("wav/extras.wav");
									iChanged++;
								} else {
									SetLocation (iCurRoom, iSelectedCol,
										iSelectedRow, iLastTile);
									PlaySound ("wav/ok_close.wav");
									iChanged++;
								}
							}
							break;
						case SDLK_d: RunLevel (iCurLevel); break;
						case SDLK_s:
							SaveLevel (iCurLevel);
							break;
						case SDLK_t:
							if (iScreen == 1) { InitScreenAction ("env"); }
							break;
						case SDLK_i:
							if (iScreen == 1)
							{
								if (iInfo == 0) { iInfo = 1; } else { iInfo = 0; }
							}
							break;
						case SDLK_r:
							if (iScreen != 2)
							{
								iScreen = 2;
								iMovingRoom = 0;
								iMovingNewBusy = 0;
								iChangingBrokenRoom = iCurRoom;
								iChangingBrokenSide = 1;
								PlaySound ("wav/screen2or3.wav");
							} else if (iBrokenRoomLinks == 0) {
								iBrokenRoomLinks = 1;
								PlaySound ("wav/screen2or3.wav");
							}
							break;
						case SDLK_e:
							if (iScreen == 2)
							{
								/*** Why? ***/
								iBrokenRoomLinks = BrokenRoomLinks (0, iCurLevel);
							}
							if (iScreen != 3)
							{
								iScreen = 3;
								PlaySound ("wav/screen2or3.wav");
							}
							break;
						case SDLK_c:
							if (iScreen == 1)
							{
								if ((event.key.keysym.mod & KMOD_LCTRL) ||
									(event.key.keysym.mod & KMOD_RCTRL))
								{
									CopyPaste (iCurRoom, 1);
									PlaySound ("wav/extras.wav");
								}
							}
							break;
						case SDLK_z:
							if (iScreen == 1)
							{
								Zoom (0);
								iExtras = 0;
								PlaySound ("wav/extras.wav");
							}
							break;
						case SDLK_f:
							if (iScreen == 1)
							{
								Zoom (1);
								iExtras = 0;
								PlaySound ("wav/extras.wav");
							}
							break;
						case SDLK_v:
							if (iScreen == 1)
							{
								if ((event.key.keysym.mod & KMOD_LCTRL) ||
									(event.key.keysym.mod & KMOD_RCTRL))
								{
									CopyPaste (iCurRoom, 2);
									PlaySound ("wav/extras.wav");
									iChanged++;
								}
							}
							break;
						case SDLK_BACKSPACE:
							if ((iScreen == 2) && (iBrokenRoomLinks == 1))
								{ LinkMinus(); }
							break;
						case SDLK_ESCAPE:
						case SDLK_q:
							switch (iScreen)
							{
								case 1:
									Quit(); break;
								case 2:
									iBrokenRoomLinks = BrokenRoomLinks (0, iCurLevel);
									iScreen = 1; break;
								case 3:
									iScreen = 1; break;
							}
							break;
						case SDLK_BACKSLASH:
							if (iScreen == 1)
							{
								/*** Randomize the entire level. ***/
								for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
								{
									for (iLoopCol = 1; iLoopCol <= COLS; iLoopCol++)
									{
										for (iLoopRow = 1; iLoopRow <= ROWS; iLoopRow++)
										{
											iUseTile = (int) (64.0 * rand() / (RAND_MAX + 1.0));
											SetLocation (iLoopRoom, iLoopCol, iLoopRow, iUseTile);
										}
									}
								}
								PlaySound ("wav/ok_close.wav");
								iChanged++;
							}
							break;
						case SDLK_SLASH:
							if (iScreen == 1) { ClearRoom(); }
							break;
						case SDLK_0: /*** empty ***/
						case SDLK_KP_0:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelectedCol, iSelectedRow, 0);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_1: /*** floor ***/
						case SDLK_KP_1:
							if (iScreen == 1)
							{
								SetLocation (iCurRoom, iSelectedCol, iSelectedRow, 7);
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_9: /*** wall ***/
						case SDLK_KP_9:
							if (iScreen == 1)
							{
								if (cCurType == 'd')
								{
									SetLocation (iCurRoom, iSelectedCol, iSelectedRow, 20);
								} else {
									SetLocation (iCurRoom, iSelectedCol, iSelectedRow, 14);
								}
								PlaySound ("wav/ok_close.wav"); iChanged++;
							}
							break;
						case SDLK_g: /*** guards ***/
							Guards();
							break;
					}
					ShowScreen (iScreen);
					break;
				case SDL_MOUSEMOTION:
					iXPosOld = iXPos;
					iYPosOld = iYPos;
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					if ((iXPosOld == iXPos) && (iYPosOld == iYPos)) { break; }

					if (OnLevelBar() == 1)
						{ iPlaytest = 1; } else { iPlaytest = 0; }

					if (iScreen == 1)
					{
						/*** hover ***/
						for (iLoopCol = 1; iLoopCol <= COLS; iLoopCol++)
						{
							for (iLoopRow = 1; iLoopRow <= ROWS; iLoopRow++)
							{
								if (InArea ((32 * iLoopCol) - 7,
									(32 * iLoopRow) + 24,
									(32 * iLoopCol) - 7 + 32,
									(32 * iLoopRow) + 24 + 32) == 1)
									{ iSelectedCol = iLoopCol; iSelectedRow = iLoopRow; }
							}
						}

						/*** extras ***/
						if (InArea (480, 3, 480 + 49, 3 + 19) == 0) { iExtras = 0; }
						if (InArea (480, 3, 480 + 9, 3 + 9) == 1) { iExtras = 1; }
						if (InArea (490, 3, 490 + 9, 3 + 9) == 1) { iExtras = 2; }
						if (InArea (500, 3, 500 + 9, 3 + 9) == 1) { iExtras = 3; }
						if (InArea (510, 3, 510 + 9, 3 + 9) == 1) { iExtras = 4; }
						if (InArea (520, 3, 520 + 9, 3 + 9) == 1) { iExtras = 5; }
						if (InArea (480, 13, 480 + 9, 13 + 9) == 1) { iExtras = 6; }
						if (InArea (490, 13, 490 + 9, 13 + 9) == 1) { iExtras = 7; }
						if (InArea (500, 13, 500 + 9, 13 + 9) == 1) { iExtras = 8; }
						if (InArea (510, 13, 510 + 9, 13 + 9) == 1) { iExtras = 9; }
						if (InArea (520, 13, 520 + 9, 13 + 9) == 1) { iExtras = 10; }
					}

					if (iScreen == 2)
					{
						/*** if (iMovingRoom != 0) { ShowScreen (2); } ***/
					}

					ShowScreen (iScreen);
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						if (InArea (0, 50, 0 + 25, 50 + 384) == 1) /*** left ***/
						{
							if (iRoomConnections[iCurLevel][iCurRoom][1] != 256)
								{ iDownAt = 1; }
						}
						if (InArea (537, 50, 537 + 25, 50 + 384) == 1) /*** right ***/
						{
							if (iRoomConnections[iCurLevel][iCurRoom][2] != 256)
								{ iDownAt = 2; }
						}
						if (InArea (25, 25, 25 + 512, 25 + 25) == 1) /*** up ***/
						{
							if (iRoomConnections[iCurLevel][iCurRoom][3] != 256)
								{ iDownAt = 3; }
						}
						if (InArea (25, 434, 25 + 512, 434 + 25) == 1) /*** down ***/
						{
							if (iRoomConnections[iCurLevel][iCurRoom][4] != 256)
								{ iDownAt = 4; }
						}
						if (InArea (0, 25, 0 + 25, 25 + 25) == 1) /*** rooms ***/
						{
							if (iBrokenRoomLinks == 0)
								{ iDownAt = 5; } else { iDownAt = 11; }
						}
						if (InArea (537, 25, 537 + 25, 25 + 25) == 1) /*** elements ***/
							{ iDownAt = 6; }
						if (InArea (0, 434, 0 + 25, 434 + 25) == 1) /*** save ***/
							{ iDownAt = 7; }
						if (InArea (537, 434, 537 + 25, 434 + 25) == 1) /*** quit ***/
							{ iDownAt = 8; }
						if (InArea (0, 0, 0 + 25, 0 + 25) == 1) /*** prev level ***/
							{ iDownAt = 9; }
						if (InArea (537, 0, 537 + 25, 0 + 25) == 1) /*** next level ***/
							{ iDownAt = 10; }

						if (iScreen == 2)
						{
							if (iBrokenRoomLinks == 0)
							{
								for (iLoopX = 1; iLoopX <= ROOMS; iLoopX++)
								{
									for (iLoopY = 1; iLoopY <= ROOMS; iLoopY++)
									{
										if (InArea (166 + ((iLoopX - 1) * 15),
											63 + ((iLoopY - 1) * 15),
											166 + ((iLoopX - 1) * 15) + 14,
											63 + ((iLoopY - 1) * 15) + 14) == 1)
										{
											if (iRoomArray[iLoopX][iLoopY] != 0)
											{
												iMovingNewBusy = 0;
												iMovingRoom = iRoomArray[iLoopX][iLoopY];
											}
										}
									}
								}

								/*** side pane ***/
								for (iLoopY = 1; iLoopY <= ROOMS; iLoopY++)
								{
									if (InArea (143, 63 + ((iLoopY - 1) * 15),
										143 + 14, 63 + ((iLoopY - 1) * 15) + 14) == 1)
									{
										if (iRoomArray[25][iLoopY] != 0)
										{
											iMovingNewBusy = 0;
											iMovingRoom = iRoomArray[25][iLoopY];
										}
									}
								}

								if (InArea (498, 65, 498 + 25, 65 + 25) == 1)
									{ iDownAt = 11; } /*** rooms broken ***/
							} else {
								MouseSelectAdj();
							}
						}
					}
					ShowScreen (iScreen);
					break;
				case SDL_MOUSEBUTTONUP:
					iDownAt = 0;
					if (event.button.button == 1)
					{
						if (InArea (0, 50, 0 + 25, 50 + 384) == 1) /*** left ***/
						{
							if ((iRoomConnections[iCurLevel][iCurRoom][1] != 256) &&
								(iRoomConnections[iCurLevel][iCurRoom][1] <= ROOMS))
							{
								iCurRoom = iRoomConnections[iCurLevel][iCurRoom][1];
								PlaySound ("wav/scroll.wav");
							}
						}
						if (InArea (537, 50, 537 + 25, 50 + 384) == 1) /*** right ***/
						{
							if ((iRoomConnections[iCurLevel][iCurRoom][2] != 256) &&
								(iRoomConnections[iCurLevel][iCurRoom][2] <= ROOMS))
							{
								iCurRoom = iRoomConnections[iCurLevel][iCurRoom][2];
								PlaySound ("wav/scroll.wav");
							}
						}
						if (InArea (25, 25, 25 + 512, 25 + 25) == 1) /*** up ***/
						{
							if ((iRoomConnections[iCurLevel][iCurRoom][3] != 256) &&
								(iRoomConnections[iCurLevel][iCurRoom][3] <= ROOMS))
							{
								iCurRoom = iRoomConnections[iCurLevel][iCurRoom][3];
								PlaySound ("wav/scroll.wav");
							}
						}
						if (InArea (25, 434, 25 + 512, 434 + 25) == 1) /*** down ***/
						{
							if ((iRoomConnections[iCurLevel][iCurRoom][4] != 256) &&
								(iRoomConnections[iCurLevel][iCurRoom][4] <= ROOMS))
							{
								iCurRoom = iRoomConnections[iCurLevel][iCurRoom][4];
								PlaySound ("wav/scroll.wav");
							}
						}
						if (InArea (0, 25, 0 + 25, 25 + 25) == 1) /*** rooms ***/
						{
							if (iScreen != 2)
							{
								iScreen = 2; iMovingRoom = 0; iMovingNewBusy = 0;
								iChangingBrokenRoom = iCurRoom;
								iChangingBrokenSide = 1;
								PlaySound ("wav/screen2or3.wav");
							}
						}
						if (InArea (537, 25, 537 + 25, 25 + 25) == 1) /*** elements ***/
						{
							if (iScreen == 2)
							{
								iBrokenRoomLinks = BrokenRoomLinks (0, iCurLevel);
							}
							if (iScreen != 3)
							{
								iScreen = 3;
								PlaySound ("wav/screen2or3.wav");
							}
						}
						if (InArea (0, 434, 0 + 25, 434 + 25) == 1) /*** save ***/
						{
							SaveLevel (iCurLevel);
						}
						if (InArea (537, 434, 537 + 25, 434 + 25) == 1) /*** quit ***/
						{
							switch (iScreen)
							{
								case 1: Quit(); break;
								case 2:
									iBrokenRoomLinks = BrokenRoomLinks (0, iCurLevel);
									iScreen = 1; break;
								case 3: iScreen = 1; break;
							}
						}
						if (InArea (0, 0, 0 + 25, 0 + 25) == 1) /*** prev level ***/
						{
							if ((iChanged != 0) && (iCurLevel != 1)) { InitPopUpSave(); }
							Prev();
							break; /*** Stop processing SDL_MOUSEBUTTONUP. ***/
						}
						if (InArea (537, 0, 537 + 25, 0 + 25) == 1) /*** next level ***/
						{
							if ((iChanged != 0) && (iCurLevel != 14)) { InitPopUpSave(); }
							Next();
							break; /*** Stop processing SDL_MOUSEBUTTONUP. ***/
						}
						if (OnLevelBar() == 1)
						{
							RunLevel (iCurLevel);
							break; /*** Stop processing SDL_MOUSEBUTTONUP. ***/
						}

						if (iScreen == 1)
						{
							if (InArea (25, 50, 25 + 512, 50 + 384) == 1) /*** tiles ***/
							{
								keystate = SDL_GetKeyboardState (NULL);
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
								{
									SetLocation (iCurRoom, iSelectedCol,
										iSelectedRow, iLastTile);
									PlaySound ("wav/ok_close.wav");
									iChanged++;
								} else {
									ChangePos (iSelectedCol, iSelectedRow);
								}
							}

							/*** 1 ***/
							if (InArea (480, 3, 480 + 9, 3 + 9) == 1)
							{
								Zoom (0);
								iExtras = 0;
								PlaySound ("wav/extras.wav");
							}

							/*** 2 ***/
							if (InArea (490, 3, 490 + 9, 3 + 9) == 1)
							{
								CopyPaste (iCurRoom, 1);
								PlaySound ("wav/extras.wav");
							}

							/*** 3 ***/
							if (InArea (500, 3, 500 + 9, 3 + 9) == 1)
								{ /*** Flip vertical. ***/ }

							/*** 4 ***/
							if (InArea (510, 3, 510 + 9, 3 + 9) == 1)
								{ InitScreenAction ("env"); }

							/*** 5 ***/
							if (InArea (520, 3, 520 + 9, 3 + 9) == 1) { Help(); }

							/*** 6 ***/
							if (InArea (480, 13, 480 + 9, 13 + 9) == 1)
							{
								Sprinkle();
								PlaySound ("wav/extras.wav");
								iChanged++;
							}

							/*** 7 ***/
							if (InArea (490, 13, 490 + 9, 13 + 9) == 1)
							{
								CopyPaste (iCurRoom, 2);
								PlaySound ("wav/extras.wav");
								iChanged++;
							}

							/*** 8 ***/
							if (InArea (500, 13, 500 + 9, 13 + 9) == 1)
								{ /*** Flip horizontal. ***/ }

							/*** 9 ***/
							if (InArea (510, 13, 510 + 9, 13 + 9) == 1)
								{ /*** Undo. ***/ }

							/*** 10 ***/
							if (InArea (520, 13, 520 + 9, 13 + 9) == 1) { EXE(); }
						}

						if (iScreen == 2)
						{
							if (iBrokenRoomLinks == 0)
							{
								for (iLoopX = 1; iLoopX <= ROOMS; iLoopX++)
								{
									for (iLoopY = 1; iLoopY <= ROOMS; iLoopY++)
									{
										if (InArea (166 + ((iLoopX - 1) * 15),
											63 + ((iLoopY - 1) * 15),
											166 + ((iLoopX - 1) * 15) + 14,
											63 + ((iLoopY - 1) * 15) + 14) == 1)
										{
											if (iMovingRoom != 0)
											{
												if (iRoomArray[iLoopX][iLoopY] == 0)
												{
													RemoveOldRoom();
													AddNewRoom (iLoopX, iLoopY, iMovingRoom);
													iChanged++;
												}
												iMovingRoom = 0; iMovingNewBusy = 0;
											}
										}
									}
								}

								/*** side pane ***/
								for (iLoopY = 1; iLoopY <= ROOMS; iLoopY++)
								{
									if (InArea (143, 63 + ((iLoopY - 1) * 15),
										143 + 14, 63 + ((iLoopY - 1) * 15) + 14) == 1)
									{
										if (iMovingRoom != 0)
										{
											if (iRoomArray[25][iLoopY] == 0)
											{
												RemoveOldRoom();
												AddNewRoom (25, iLoopY, iMovingRoom);
												iChanged++;
											}
											iMovingRoom = 0; iMovingNewBusy = 0;
										}
									}
								}

								if (InArea (498, 65, 498 + 25, 65 + 25) == 1)
								{ /*** rooms broken ***/
									iBrokenRoomLinks = 1;
									PlaySound ("wav/screen2or3.wav");
								}
							} else {
								if (MouseSelectAdj() == 1) { LinkPlus(); }
							}
						}

						if (iScreen == 3) /*** E ***/
						{
							/*** kid ***/
							if (InArea (481, 247, 481 + 14, 247 + 14) == 1) /*** L ***/
							{
								if (arKidDir[iCurLevel] != 0)
								{
									arKidDir[iCurLevel] = 0;
									PlaySound ("wav/check_box.wav");
									iChanged++;
								}
							}
							if (InArea (497, 247, 497 + 14, 247 + 14) == 1) /*** R ***/
							{
								if (arKidDir[iCurLevel] != 32)
								{
									arKidDir[iCurLevel] = 32;
									PlaySound ("wav/check_box.wav");
									iChanged++;
								}
							}
							PlusMinus (&arKidPosX[iCurLevel], 413, 279, 0, 255, -10, 1);
							PlusMinus (&arKidPosX[iCurLevel], 428, 279, 0, 255, -1, 1);
							PlusMinus (&arKidPosX[iCurLevel], 498, 279, 0, 255, +1, 1);
							PlusMinus (&arKidPosX[iCurLevel], 513, 279, 0, 255, +10, 1);
							PlusMinus (&arKidPosY[iCurLevel], 413, 311, 0, 255, -10, 1);
							PlusMinus (&arKidPosY[iCurLevel], 428, 311, 0, 255, -1, 1);
							PlusMinus (&arKidPosY[iCurLevel], 498, 311, 0, 255, +1, 1);
							PlusMinus (&arKidPosY[iCurLevel], 513, 311, 0, 255, +10, 1);
							PlusMinus (&arKidRoom[iCurLevel], 413, 343, 1, 24, -10, 1);
							PlusMinus (&arKidRoom[iCurLevel], 428, 343, 1, 24, -1, 1);
							PlusMinus (&arKidRoom[iCurLevel], 498, 343, 1, 24, +1, 1);
							PlusMinus (&arKidRoom[iCurLevel], 513, 343, 1, 24, +10, 1);
							PlusMinus (&arKidAnim[iCurLevel], 413, 375, 0, 255, -10, 1);
							PlusMinus (&arKidAnim[iCurLevel], 428, 375, 0, 255, -1, 1);
							PlusMinus (&arKidAnim[iCurLevel], 498, 375, 0, 255, +1, 1);
							PlusMinus (&arKidAnim[iCurLevel], 513, 375, 0, 255, +10, 1);

							/*** edit this element ***/
							PlusMinus (&iActiveElement, 149, 61, 1, MAX_ELEM, -10, 0);
							PlusMinus (&iActiveElement, 164, 61, 1, MAX_ELEM, -1, 0);
							PlusMinus (&iActiveElement, 234, 61, 1, MAX_ELEM, +1, 0);
							PlusMinus (&iActiveElement, 249, 61, 1, MAX_ELEM, +10, 0);

							/*** active (8) ***/
							if (InArea (511, 66, 511 + 14, 66 + 14) == 1)
							{
								if (arElements[iCurLevel][iCurRoom][iActiveElement][8] == 0x00)
								{
									arElements[iCurLevel][iCurRoom][iActiveElement][8] = 0x01;
								} else {
									arElements[iCurLevel][iCurRoom][iActiveElement][8] = 0x00;
								}
								PlaySound ("wav/check_box.wav");
								iChanged++;
							}

							/*** element (0) ***/
							iElem = 0x00;
							if (InArea (37, 109, 37 + 14, 109 + 14) == 1)
								{ iElem = 0x27; } /*** level door ***/
							if (InArea (99, 109, 99 + 14, 109 + 14) == 1)
								{ iElem = 0x29; } /*** mirror ***/
							if (InArea (161, 109, 161 + 14, 109 + 14) == 1)
								{ iElem = 0x32; } /*** gate ***/
							if (InArea (223, 109, 223 + 14, 109 + 14) == 1)
								{ iElem = 0x3E; } /*** skel alive ***/
							if (InArea (285, 109, 285 + 14, 109 + 14) == 1)
								{ iElem = 0x3F; } /*** skel respawn ***/
							if (InArea (347, 109, 347 + 14, 109 + 14) == 1)
								{ iElem = 0x61; } /*** drop button ***/
							if (InArea (409, 109, 409 + 14, 109 + 14) == 1)
								{ iElem = 0x62; } /*** raise button ***/
							if (InArea (471, 109, 471 + 14, 109 + 14) == 1)
								{ iElem = 0x64; } /*** guard ***/
							if (InArea (37, 146, 37 + 14, 146 + 14) == 1)
								{ iElem = 0x2B; } /*** spikes ***/
							if (InArea (99, 146, 99 + 14, 146 + 14) == 1)
								{ iElem = 0x2C; } /*** sword ***/
							if (InArea (161, 146, 161 + 14, 146 + 14) == 1)
								{ iElem = 0x2D; } /*** hurt potion (r) ***/
							if (InArea (223, 146, 223 + 14, 146 + 14) == 1)
								{ iElem = 0x2E; } /*** heal potion (b) ***/
							if (InArea (285, 146, 285 + 14, 146 + 14) == 1)
								{ iElem = 0x2F; } /*** life potion (g) ***/
							if (InArea (347, 146, 347 + 14, 146 + 14) == 1)
								{ iElem = 0x30; } /*** float potion (p) ***/
							if (InArea (409, 146, 409 + 14, 146 + 14) == 1)
								{ iElem = 0x31; } /*** chomper ***/
							if (InArea (471, 146, 471 + 14, 146 + 14) == 1)
								{ iElem = 0x41; } /*** loose floor ***/
							if (InArea (37, 183, 37 + 14, 183 + 14) == 1)
								{ iElem = 0x42; } /*** loose floor fall ***/
							if (InArea (99, 183, 99 + 14, 183 + 14) == 1)
								{ iElem = 0x66; } /*** shadow step ***/
							if (InArea (161, 183, 161 + 14, 183 + 14) == 1)
								{ iElem = 0x69; } /*** shadow drink ***/
							if (InArea (223, 183, 223 + 14, 183 + 14) == 1)
								{ iElem = 0x6F; } /*** princess ***/
							if (InArea (285, 183, 285 + 14, 183 + 14) == 1)
								{ iElem = 0x70; } /*** mouse move ***/
							if (InArea (347, 183, 347 + 14, 183 + 14) == 1)
								{ iElem = 0x74; } /*** shadow fight ***/
							if (iElem != 0x00)
							{
								arElements[iCurLevel][iCurRoom][iActiveElement][0] = iElem;
								PlaySound ("wav/check_box.wav");
								iChanged++;
							}

							/*** custom ***/
							PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][0],
								409, 185, 0x01, 0xFE, -16, 1);
							PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][0],
								424, 185, 0x01, 0xFE, -1, 1);
							PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][0],
								494, 185, 0x01, 0xFE, +1, 1);
							PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][0],
								509, 185, 0x01, 0xFE, +16, 1);

							/*** bytes ***/
							PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][1],
								36, 225, 0, 255, -10, 1);
							PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][1],
								51, 225, 0, 255, -1, 1);
							PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][1],
								121, 225, 0, 255, +1, 1);
							PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][1],
								136, 225, 0, 255, +10, 1);
							PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][2],
								36, 250, 0, 255, -10, 1);
							PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][2],
								51, 250, 0, 255, -1, 1);
							PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][2],
								121, 250, 0, 255, +1, 1);
							PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][2],
								136, 250, 0, 255, +10, 1);
							PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][3],
								36, 275, 0, 255, -10, 1);
							PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][3],
								51, 275, 0, 255, -1, 1);
							PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][3],
								121, 275, 0, 255, +1, 1);
							PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][3],
								136, 275, 0, 255, +10, 1);
							iElem = arElements[iCurLevel][iCurRoom][iActiveElement][0];
							if ((iElem == 0x27) || (iElem == 0x29) ||
								(iElem == 0x32) || (iElem == 0x3E) ||
								(iElem == 0x3F) || (iElem == 0x61) ||
								(iElem == 0x62) || (iElem == 0x64))
							{
								PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][4],
									36, 300, 0, 255, -10, 1);
								PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][4],
									51, 300, 0, 255, -1, 1);
								PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][4],
									121, 300, 0, 255, +1, 1);
								PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][4],
									136, 300, 0, 255, +10, 1);
								PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][5],
									36, 325, 0, 255, -10, 1);
								PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][5],
									51, 325, 0, 255, -1, 1);
								PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][5],
									121, 325, 0, 255, +1, 1);
								PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][5],
									136, 325, 0, 255, +10, 1);
							}
							if ((iElem == 0x27) || (iElem == 0x32))
							{
								PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][6],
									36, 350, 0, 255, -10, 1);
								PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][6],
									51, 350, 0, 255, -1, 1);
								PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][6],
									121, 350, 0, 255, +1, 1);
								PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][6],
									136, 350, 0, 255, +10, 1);
								PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][7],
									36, 375, 0, 255, -10, 1);
								PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][7],
									51, 375, 0, 255, -1, 1);
								PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][7],
									121, 375, 0, 255, +1, 1);
								PlusMinus (&arElements[iCurLevel][iCurRoom][iActiveElement][7],
									136, 375, 0, 255, +10, 1);
							}
						}
					}
					if (event.button.button == 2)
					{
						if (iScreen == 1) { ClearRoom(); }
					}
					if (event.button.button == 3)
					{
						if (iScreen == 1)
						{
							/*** Randomize the entire level. ***/
							for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
							{
								for (iLoopCol = 1; iLoopCol <= COLS; iLoopCol++)
								{
									for (iLoopRow = 1; iLoopRow <= ROWS; iLoopRow++)
									{
										iUseTile = (int) (64.0 * rand() / (RAND_MAX + 1.0));
										SetLocation (iLoopRoom, iLoopCol, iLoopRow, iUseTile);
									}
								}
							}
							PlaySound ("wav/ok_close.wav");
							iChanged++;
						}
						if (iScreen == 2)
						{
							if (iBrokenRoomLinks == 1)
							{
								if (MouseSelectAdj() == 1) { LinkMinus(); }
							}
						}
					}
					ShowScreen (iScreen);
					break;
				case SDL_MOUSEWHEEL:
					if (event.wheel.y > 0) /*** scroll wheel up ***/
					{
						if (InArea (25, 50, 25 + 512, 50 + 384) == 1) /*** tiles ***/
						{
							keystate = SDL_GetKeyboardState (NULL);
							if ((keystate[SDL_SCANCODE_LSHIFT]) ||
								(keystate[SDL_SCANCODE_RSHIFT]))
							{ /*** right ***/
								if ((iRoomConnections[iCurLevel][iCurRoom][2] != 256) &&
									(iRoomConnections[iCurLevel][iCurRoom][2] <= ROOMS))
								{
									iCurRoom = iRoomConnections[iCurLevel][iCurRoom][2];
									PlaySound ("wav/scroll.wav");
								}
							} else { /*** up ***/
								if ((iRoomConnections[iCurLevel][iCurRoom][3] != 256) &&
									(iRoomConnections[iCurLevel][iCurRoom][3] <= ROOMS))
								{
									iCurRoom = iRoomConnections[iCurLevel][iCurRoom][3];
									PlaySound ("wav/scroll.wav");
								}
							}
						}
					}
					if (event.wheel.y < 0) /*** scroll wheel down ***/
					{
						if (InArea (25, 50, 25 + 512, 50 + 384) == 1) /*** tiles ***/
						{
							keystate = SDL_GetKeyboardState (NULL);
							if ((keystate[SDL_SCANCODE_LSHIFT]) ||
								(keystate[SDL_SCANCODE_RSHIFT]))
							{ /*** left ***/
								if ((iRoomConnections[iCurLevel][iCurRoom][1] != 256) &&
									(iRoomConnections[iCurLevel][iCurRoom][1] <= ROOMS))
								{
									iCurRoom = iRoomConnections[iCurLevel][iCurRoom][1];
									PlaySound ("wav/scroll.wav");
								}
							} else { /*** down ***/
								if ((iRoomConnections[iCurLevel][iCurRoom][4] != 256) &&
									(iRoomConnections[iCurLevel][iCurRoom][4] <= ROOMS))
								{
									iCurRoom = iRoomConnections[iCurLevel][iCurRoom][4];
									PlaySound ("wav/scroll.wav");
								}
							}
						}
					}
					ShowScreen (iScreen);
					break;
				case SDL_WINDOWEVENT:
					switch (event.window.event)
					{
						case SDL_WINDOWEVENT_EXPOSED:
							ShowScreen (iScreen); break;
						case SDL_WINDOWEVENT_CLOSE:
							Quit(); break;
						case SDL_WINDOWEVENT_FOCUS_GAINED:
							iActiveWindowID = iWindowID; break;
					}
					break;
				case SDL_QUIT:
					Quit(); break;
			}
		}

		/*** prevent CPU eating ***/
		gamespeed = REFRESH;
		while ((SDL_GetTicks() - looptime) < gamespeed)
		{
			SDL_Delay (10);
		}
		looptime = SDL_GetTicks();
	}
}
/*****************************************************************************/
void LoadFonts (void)
/*****************************************************************************/
{
	font11 = TTF_OpenFont ("ttf/Bitstream-Vera-Sans-Bold.ttf", 11 * iScale);
	font15 = TTF_OpenFont ("ttf/Bitstream-Vera-Sans-Bold.ttf", 15 * iScale);
	font20 = TTF_OpenFont ("ttf/Bitstream-Vera-Sans-Bold.ttf", 20 * iScale);
	if ((font11 == NULL) || (font15 == NULL) || (font20 == NULL))
	{
		printf ("[FAILED] TTF_OpenFont() failed!\n");
		exit (EXIT_ERROR);
	}
}
/*****************************************************************************/
void MixAudio (void *unused, Uint8 *stream, int iLen)
/*****************************************************************************/
{
	int iTemp;
	int iAmount;

	if (unused != NULL) { } /*** To prevent warnings. ***/

	SDL_memset (stream, 0, iLen); /*** SDL2 ***/
	for (iTemp = 0; iTemp < NUM_SOUNDS; iTemp++)
	{
		iAmount = (sounds[iTemp].dlen-sounds[iTemp].dpos);
		if (iAmount > iLen)
		{
			iAmount = iLen;
		}
		SDL_MixAudio (stream, &sounds[iTemp].data[sounds[iTemp].dpos], iAmount,
			SDL_MIX_MAXVOLUME);
		sounds[iTemp].dpos += iAmount;
	}
}
/*****************************************************************************/
void PreLoad (char *sPath, char *sPNG, SDL_Texture **imgImage)
/*****************************************************************************/
{
	char sImage[MAX_IMG + 2];
	int iBarHeight;

	snprintf (sImage, MAX_IMG, "%s%s", sPath, sPNG);
	*imgImage = IMG_LoadTexture (ascreen, sImage);
	if (!*imgImage)
	{
		printf ("[FAILED] IMG_LoadTexture: %s!\n", IMG_GetError());
		exit (EXIT_ERROR);
	}

	iPreLoaded++;
	iBarHeight = (int)(((float)iPreLoaded/(float)iNrToPreLoad) * BAR_FULL);
	if (iBarHeight >= iCurrentBarHeight + 10) { LoadingBar (iBarHeight); }
}
/*****************************************************************************/
void ShowImage (SDL_Texture *img, int iX, int iY, char *sImageInfo,
	SDL_Renderer *screen, float fMultiply, int iXYScale)
/*****************************************************************************/
{
	/* Usually, fMultiply is either iScale or ZoomGet().
	 *
	 * Also, usually, iXYScale is 1 with iScale and 0 with ZoomGet().
	 *
	 * If you use this function, make sure to verify the image is properly
	 * (re)scaled and (re)positioned when the main window is zoomed ("z")!
	 */

	SDL_Rect dest;
	SDL_Rect loc;
	int iWidth, iHeight;

	if (img == NULL)
	{
		img = imgunknown;
/***
		printf ("[ WARN ] Unknown image \"%s\".\n", sImageInfo);
		return;
***/
	}

	SDL_QueryTexture (img, NULL, NULL, &iWidth, &iHeight);
	loc.x = 0; loc.y = 0; loc.w = iWidth; loc.h = iHeight;
	if (iXYScale == 0)
	{
		dest.x = iX;
		dest.y = iY;
	} else {
		dest.x = iX * fMultiply;
		dest.y = iY * fMultiply;
	}
	dest.w = iWidth * fMultiply;
	dest.h = iHeight * fMultiply;

	/*** This is for the animation. ***/
	if ((strcmp (sImageInfo, "spriteflamed") == 0) ||
		(strcmp (sImageInfo, "spriteflamep") == 0))
	{
		loc.x = (iFlameFrameDP - 1) * 32;
		loc.w = loc.w / 3;
		dest.w = dest.w / 3;
	}
	if (strcmp (sImageInfo, "imgstatusbarsprite") == 0)
	{
		loc.x = (iStatusBarFrame - 1) * 20;
		loc.w = loc.w / 18;
		dest.w = dest.w / 18;
	}

	if (SDL_RenderCopy (screen, img, &loc, &dest) != 0)
	{
		printf ("[ WARN ] SDL_RenderCopy (%s): %s\n", sImageInfo, SDL_GetError());
	}
}
/*****************************************************************************/
void LoadingBar (int iBarHeight)
/*****************************************************************************/
{
	SDL_Rect bar;

	/*** bar ***/
	bar.x = 12 * iScale;
	bar.y = (447 - iBarHeight) * iScale;
	bar.w = 16 * iScale;
	bar.h = iBarHeight * iScale;
	SDL_SetRenderDrawColor (ascreen, 0x44, 0x44, 0x44, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect (ascreen, &bar);
	iCurrentBarHeight = iBarHeight;

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void GetOptionValue (char *sArgv, char *sValue)
/*****************************************************************************/
{
	int iTemp;
	char sTemp[MAX_OPTION + 2];

	iTemp = strlen (sArgv) - 1;
	snprintf (sValue, MAX_OPTION, "%s", "");
	while (sArgv[iTemp] != '=')
	{
		snprintf (sTemp, MAX_OPTION, "%c%s", sArgv[iTemp], sValue);
		snprintf (sValue, MAX_OPTION, "%s", sTemp);
		iTemp--;
	}
}
/*****************************************************************************/
void ShowScreen (int iScreenS)
/*****************************************************************************/
{
	char sLevel[MAX_TEXT + 2];
	char arText[2 + 2][MAX_TEXT + 2];
	int iUnusedRooms;
	int iXShow, iYShow;
	char sShowRoom[MAX_TEXT + 2];
	int iToRoom;
	int iElem, iElemX, iElemY;
	char arTextB1[1 + 2][MAX_TEXT + 2];
	char arTextB2[1 + 2][MAX_TEXT + 2];
	char arTextB3[1 + 2][MAX_TEXT + 2];
	char arTextB4[1 + 2][MAX_TEXT + 2];
	char arTextB5[1 + 2][MAX_TEXT + 2];
	char arTextB6[1 + 2][MAX_TEXT + 2];
	char arTextB7[1 + 2][MAX_TEXT + 2];
	int iByte1, iByte2, iByte3, iByte4, iByte5;
	char cDir;
	SDL_Texture *img;

	/*** Used for looping. ***/
	int iLoopRow;
	int iLoopCol;
	int iLoopRoom;
	int iLoopSide;
	int iLoopElem;

	/*** black background ***/
	ShowImage (imgblack, 0, 0, "imgblack", ascreen, iScale, 1);

	if (iScreenS == 1)
	{
		for (iLoopRow = 1; iLoopRow <= ROWS; iLoopRow++)
		{
			for (iLoopCol = 1; iLoopCol <= COLS; iLoopCol++)
			{
				ShowTile (iCurRoom, iLoopRow, iLoopCol);
			}
		}

		/*** above this room ***/
		for (iLoopCol = 1; iLoopCol <= COLS; iLoopCol++)
		{
			if (iRoomConnections[iCurLevel][iCurRoom][3] != 256)
			{
				ShowTile (iRoomConnections[iCurLevel][iCurRoom][3], 0, iLoopCol);
			} else {
				ShowTile (0, 0, iLoopCol);
			}
		}

		/*** elements ***/
		for (iLoopElem = 1; iLoopElem <= MAX_ELEM; iLoopElem++)
		{
			if (arElements[iCurLevel][iCurRoom][iLoopElem][8] == 0x01) /*** on ***/
			{
				iElem = arElements[iCurLevel][iCurRoom][iLoopElem][0];
				iByte1 = arElements[iCurLevel][iCurRoom][iLoopElem][1];
				iByte2 = arElements[iCurLevel][iCurRoom][iLoopElem][2];
				iByte3 = arElements[iCurLevel][iCurRoom][iLoopElem][3];
				iByte4 = arElements[iCurLevel][iCurRoom][iLoopElem][4];
				iByte5 = arElements[iCurLevel][iCurRoom][iLoopElem][5];
				switch (iElem)
				{
					case 0x27: /*** level door ***/
						ShowImage (imgleveldoor, 25 + (16 * iByte1),
							50 + (16 * iByte2) - 10,
							"imgleveldoor", ascreen, iScale, 1);
						break;
					case 0x29: /*** mirror ***/
						ShowImage (imgmirror, 25 + (2 * iByte1), 50 + (2 * iByte2),
							"imgmirror", ascreen, iScale, 1);
						break;
					case 0x2B: /*** spikes ***/
						ShowImage (imgspikes, 25 + (2 * iByte1), 50 + (2 * iByte2),
							"imgspikes", ascreen, iScale, 1);
						break;
					case 0x2C: /*** sword ***/
						ShowImage (imgsword, 25 + (2 * iByte1), 50 + (2 * iByte2),
							"imgsword", ascreen, iScale, 1);
						break;
					case 0x2D: /*** hurt potion (r) ***/
						ShowImage (imgpotionred, 25 + (2 * iByte1), 50 + (2 * iByte2),
							"imgpotionred", ascreen, iScale, 1);
						break;
					case 0x2E: /*** heal potion (b) ***/
						ShowImage (imgpotionblue, 25 + (2 * iByte1), 50 + (2 * iByte2),
							"imgpotionblue", ascreen, iScale, 1);
						break;
					case 0x2F: /*** life potion (g) ***/
						ShowImage (imgpotiongreen, 25 + (2 * iByte1), 50 + (2 * iByte2),
							"imgpotiongreen", ascreen, iScale, 1);
						break;
					case 0x30: /*** float potion (p) ***/
						ShowImage (imgpotionpink, 25 + (2 * iByte1), 50 + (2 * iByte2),
							"imgpotionpink", ascreen, iScale, 1);
						break;
					case 0x31: /*** chomper ***/
						ShowImage (imgchomper, 25 + (2 * iByte1), 50 + (2 * iByte2),
							"imgchomper", ascreen, iScale, 1);
						break;
					case 0x32: /*** gate ***/
						ShowImage (imggate, 25 + (2 * iByte1), 50 + (2 * iByte2),
							"imggate", ascreen, iScale, 1);
						break;
					case 0x3E: /*** skel alive ***/
						ShowImage (imgskelalive, 25 + (2 * iByte4), 50 + (2 * iByte5),
							"imgskelalive", ascreen, iScale, 1);
						break;
					case 0x3F: /*** skel respawn ***/
						ShowImage (imgskelrespawn, 25 + (2 * iByte4), 50 + (2 * iByte5),
							"imgskelrespawn", ascreen, iScale, 1);
						break;
					case 0x41: /*** loose floor ***/
						ShowImage (imgloose, 25 + (16 * iByte1), 50 + (16 * iByte2) - 6,
							"imgloose", ascreen, iScale, 1);
						break;
					case 0x42: /*** loose floor fall ***/
						/*** Ignoring iByte2. ***/
						ShowImage (imgloosefall, 25 + (16 * iByte1), 50,
							"imgloosefall", ascreen, iScale, 1);
						break;
					case 0x61: /*** drop button ***/
						img = imgunknown;
						if ((cCurType == 'd') && (iByte3 == 0)) { img = imgdropd; }
						if ((cCurType == 'd') && (iByte3 == 2)) { img = imgdropdwall; }
						if ((cCurType == 'p') && (iByte3 == 0)) { img = imgdropp; }
						if ((cCurType == 'p') && (iByte3 == 2)) { img = imgdroppwall; }
						ShowImage (img, 25 + (16 * iByte1), 50 + (16 * iByte2) - 10,
							"img", ascreen, iScale, 1);
						break;
					case 0x62: /*** raise button ***/
						img = imgunknown;
						if ((cCurType == 'd') && (iByte3 == 0)) { img = imgraised; }
						if ((cCurType == 'd') && (iByte3 == 2)) { img = imgraisedwall; }
						if ((cCurType == 'p') && (iByte3 == 0)) { img = imgraisep; }
						if ((cCurType == 'p') && (iByte3 == 2)) { img = imgraisepwall; }
						ShowImage (img, 25 + (16 * iByte1), 50 + (16 * iByte2) - 10,
							"img", ascreen, iScale, 1);
						break;
					case 0x64: /*** guard ***/
						if (arGuardDir[iByte1] == 1)
							{ cDir = 'r'; } else { cDir = 'l'; }
						switch (iByte2)
						{
							case 0: /*** g ***/
								if (cDir == 'l')
								{
									ShowImage (imgguardl, 25 + (2 * iByte4), 50 + (2 * iByte5),
										"imgguardl", ascreen, iScale, 1);
								} else {
									ShowImage (imgguardr, 25 + (2 * iByte4), 50 + (2 * iByte5),
										"imgguardr", ascreen, iScale, 1);
								}
								break;
							case 1: /*** s ***/
								if (cDir == 'l')
								{
									ShowImage (imgskell, 25 + (2 * iByte4), 50 + (2 * iByte5),
										"imgguardl", ascreen, iScale, 1);
								} else {
									ShowImage (imgskelr, 25 + (2 * iByte4), 50 + (2 * iByte5),
										"imgguardl", ascreen, iScale, 1);
								}
								break;
							case 2: /*** J ***/
								if (cDir == 'l')
								{
									ShowImage (imgjaffarl, 25 + (2 * iByte4), 50 + (2 * iByte5),
										"imgguardl", ascreen, iScale, 1);
								} else {
									ShowImage (imgjaffarr, 25 + (2 * iByte4), 50 + (2 * iByte5),
										"imgguardl", ascreen, iScale, 1);
								}
								break;
						}
						snprintf (arText[0], MAX_TEXT, "%i", iByte1);
						DisplayText (25 + (2 * iByte4) + 47, 50 + (2 * iByte5) + 100,
							11, arText, 1, font11, color_wh, color_bl, 0);
						break;
					case 0x66: /*** shadow step ***/
						ShowImage (imgshadowstep, 25 + (2 * iByte1), 50 + (2 * iByte2),
							"imgshadowstep", ascreen, iScale, 1);
						break;
					case 0x69: /*** shadow drink ***/
						ShowImage (imgshadowdrink, 25, 50 + (2 * iByte2),
							"imgshadowdrink", ascreen, iScale, 1);
						break;
					case 0x6F: /*** princess ***/
						ShowImage (imgprincess, 25 + (2 * iByte1), 50 + (2 * iByte2),
							"imgprincess", ascreen, iScale, 1);
						break;
					case 0x70: /*** mouse move ***/
						ShowImage (imgmouse, 25 + (2 * iByte1), 50 + (2 * iByte2),
							"imgmouse", ascreen, iScale, 1);
						break;
					case 0x74: /*** shadow fight ***/
						/*** Ignoring iByte1. ***/
						ShowImage (imgshadowfight, 25, 50 + (2 * iByte2),
							"imgshadowfight", ascreen, iScale, 1);
						break;
					default:
						ShowImage (imgunknown, 25 + (2 * iByte1), 50 + (2 * iByte2),
							"imgunknown", ascreen, iScale, 1);
						break;
				}
			}
		}

		/*** kid ***/
		if (iCurRoom == arKidRoom[iCurLevel])
		{
			switch (arKidDir[iCurLevel])
			{
				case 0: /*** L ***/
					ShowImage (imgkidl, 25 + (2 * arKidPosX[iCurLevel]),
						50 + (2 * arKidPosY[iCurLevel]),
						"imgkidl", ascreen, iScale, 1);
					break;
				case 32: /*** R ***/
					ShowImage (imgkidr, 25 + (2 * arKidPosX[iCurLevel]),
						50 + (2 * arKidPosY[iCurLevel]),
						"imgkidr", ascreen, iScale, 1);
					break;
				default:
					printf ("[FAILED] Wrong kid dir: %i!\n", arKidDir[iCurLevel]);
					exit (EXIT_ERROR); break;
			}
		}

		/*** mask ***/
		ShowImage (imgmask, 25, 50, "imgmask", ascreen, iScale, 1);

		/*** coordinates ***/
		if (InArea (25, 50, 25 + 512, 50 + 384) == 1) /*** tiles ***/
		{
			if (iInfo == 1)
			{
				snprintf (arText[0], MAX_TEXT, "%i, %i",
					(int)(((iXPos / iScale) - 25) / 2),
					(int)(((iYPos / iScale) - 50) / 2));
				snprintf (arText[1], MAX_TEXT, "%s", "mouse");
				DisplayText (iXPos + 20, iYPos + 20, 11, arText, 1,
					font11, color_wh, color_bl, 0);
			}
		}
	}
	if (iScreenS == 2) /*** R ***/
	{
		if (iBrokenRoomLinks == 0)
		{
			InitRooms();
			/*** room links ***/
			ShowImage (imgrl, 25, 50, "imgrl", ascreen, iScale, 1);
			/*** rooms broken on ***/
			if (iDownAt == 11)
			{ /*** down ***/
				ShowImage (imgbroomson_1, 498, 65, "imgbroomson_1",
					ascreen, iScale, 1);
			} else { /*** up ***/
				ShowImage (imgbroomson_0, 498, 65, "imgbroomson_0",
					ascreen, iScale, 1);
			}
			WhereToStart();
			for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
			{
				iDone[iLoopRoom] = 0;
			}
			ShowRooms (arKidRoom[iCurLevel], iStartRoomsX, iStartRoomsY, 1);
			iUnusedRooms = 0;
			for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
			{
				if (iDone[iLoopRoom] != 1)
				{
					iUnusedRooms++;
					ShowRooms (iLoopRoom, 25, iUnusedRooms, 0);
				}
			}
			if (iMovingRoom != 0)
			{
				iXShow = (iXPos + 10) / iScale;
				iYShow = (iYPos + 10) / iScale;
				ShowImage (imgroom[iMovingRoom], iXShow, iYShow, "imgroom[]",
					ascreen, iScale, 1);
				if (iCurRoom == iMovingRoom) /*** green stripes ***/
					{ ShowImage (imgsrc, iXShow, iYShow, "imgsrc", ascreen, iScale, 1); }
				if (arKidRoom[iCurLevel] == iMovingRoom) /*** blue border ***/
					{ ShowImage (imgsrs, iXShow, iYShow, "imgsrs", ascreen, iScale, 1); }
				ShowImage (imgsrm, iXShow, iYShow, "imgsrm", ascreen, iScale, 1);
				ShowRooms (-1, iMovingNewX, iMovingNewY, 0);
			}
		} else {
			/*** broken room links ***/
			ShowImage (imgbrl, 25, 50, "imgbrl", ascreen, iScale, 1);
			for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
			{
				switch (iLoopRoom)
				{
					case 1: iXShow = (63 * 0); iYShow = (63 * 0); break;
					case 2: iXShow = (63 * 1); iYShow = (63 * 0); break;
					case 3: iXShow = (63 * 2); iYShow = (63 * 0); break;
					case 4: iXShow = (63 * 3); iYShow = (63 * 0); break;
					case 5: iXShow = (63 * 0); iYShow = (63 * 1); break;
					case 6: iXShow = (63 * 1); iYShow = (63 * 1); break;
					case 7: iXShow = (63 * 2); iYShow = (63 * 1); break;
					case 8: iXShow = (63 * 3); iYShow = (63 * 1); break;
					case 9: iXShow = (63 * 0); iYShow = (63 * 2); break;
					case 10: iXShow = (63 * 1); iYShow = (63 * 2); break;
					case 11: iXShow = (63 * 2); iYShow = (63 * 2); break;
					case 12: iXShow = (63 * 3); iYShow = (63 * 2); break;
					case 13: iXShow = (63 * 0); iYShow = (63 * 3); break;
					case 14: iXShow = (63 * 1); iYShow = (63 * 3); break;
					case 15: iXShow = (63 * 2); iYShow = (63 * 3); break;
					case 16: iXShow = (63 * 3); iYShow = (63 * 3); break;
					case 17: iXShow = (63 * 0); iYShow = (63 * 4); break;
					case 18: iXShow = (63 * 1); iYShow = (63 * 4); break;
					case 19: iXShow = (63 * 2); iYShow = (63 * 4); break;
					case 20: iXShow = (63 * 3); iYShow = (63 * 4); break;
					case 21: iXShow = (63 * 0); iYShow = (63 * 5); break;
					case 22: iXShow = (63 * 1); iYShow = (63 * 5); break;
					case 23: iXShow = (63 * 2); iYShow = (63 * 5); break;
					case 24: iXShow = (63 * 3); iYShow = (63 * 5); break;
				}
				if (iCurRoom == iLoopRoom)
				{ /*** green stripes ***/
					ShowImage (imgsrc, iXShow + broken_room_x,
						iYShow + broken_room_y, "imgsrc", ascreen, iScale, 1);
				}
				if (arKidRoom[iCurLevel] == iLoopRoom)
				{ /*** blue border ***/
					ShowImage (imgsrs, iXShow + broken_room_x,
						iYShow + broken_room_y, "imgsrs", ascreen, iScale, 1);
				}
				for (iLoopSide = 1; iLoopSide <= 4; iLoopSide++)
				{
					if (iRoomConnections[iCurLevel][iLoopRoom][iLoopSide] != 256)
					{
						iToRoom = iRoomConnections[iCurLevel][iLoopRoom][iLoopSide];
						if ((iToRoom < 1) || (iToRoom > ROOMS)) { iToRoom = 25; }
						snprintf (sShowRoom, MAX_TEXT, "imgroom[%i]", iToRoom);
						ShowImage (imgroom[iToRoom],
							iXShow + broken_side_x[iLoopSide],
							iYShow + broken_side_y[iLoopSide],
							sShowRoom, ascreen, iScale, 1);

						if (iRoomConnectionsBroken[iLoopRoom][iLoopSide] == 1)
						{ /*** blue square ***/
							ShowImage (imgsrb,
								iXShow + broken_side_x[iLoopSide],
								iYShow + broken_side_y[iLoopSide],
								"imgsrb", ascreen, iScale, 1);
						}
					}

					if ((iChangingBrokenRoom == iLoopRoom) &&
						(iChangingBrokenSide == iLoopSide))
					{ /*** red stripes ***/
						ShowImage (imgsrm,
							iXShow + broken_side_x[iLoopSide],
							iYShow + broken_side_y[iLoopSide],
							"imgsrm", ascreen, iScale, 1);
					}
				}
			}
		}
	}
	if (iScreenS == 3) /*** E ***/
	{
		/*** elements screen ***/
		ShowImage (imgelements, 25, 50, "imgelements", ascreen, iScale, 1);

		/*** kid ***/
		if (arKidDir[iCurLevel] == 0) /*** L ***/
		{
			ShowImage (imgsele, 481, 247, "imgsele", ascreen, iScale, 1);
		} else { /*** R ***/
			ShowImage (imgsele, 497, 247, "imgsele", ascreen, iScale, 1);
		}
		CenterNumber (ascreen, arKidPosX[iCurLevel], 441, 279, color_wh, 0);
		CenterNumber (ascreen, arKidPosY[iCurLevel], 441, 311, color_wh, 0);
		CenterNumber (ascreen, arKidRoom[iCurLevel], 441, 343, color_wh, 0);
		CenterNumber (ascreen, arKidAnim[iCurLevel], 441, 375, color_wh, 0);

		/*** edit this element ***/
		CenterNumber (ascreen, iActiveElement, 177, 61, color_wh, 0);
		/*** active (8) ***/
		if (arElements[iCurLevel][iCurRoom][iActiveElement][8] == 0x01)
			{ ShowImage (imgsele, 511, 66, "imgsele", ascreen, iScale, 1); }
		/*** element (0) ***/
		iElem = arElements[iCurLevel][iCurRoom][iActiveElement][0];
		switch (iElem)
		{
			case 0x27: iElemX = 37; iElemY = 109; break; /*** level door ***/
			case 0x29: iElemX = 99; iElemY = 109; break; /*** mirror ***/
			case 0x32: iElemX = 161; iElemY = 109; break; /*** gate ***/
			case 0x3E: iElemX = 223; iElemY = 109; break; /*** skel alive ***/
			case 0x3F: iElemX = 285; iElemY = 109; break; /*** skel respawn ***/
			case 0x61: iElemX = 347; iElemY = 109; break; /*** drop button ***/
			case 0x62: iElemX = 409; iElemY = 109; break; /*** raise button ***/
			case 0x64: iElemX = 471; iElemY = 109; break; /*** guard ***/
			case 0x2B: iElemX = 37; iElemY = 146; break; /*** spikes ***/
			case 0x2C: iElemX = 99; iElemY = 146; break; /*** sword ***/
			case 0x2D: iElemX = 161; iElemY = 146; break; /*** hurt potion (r) ***/
			case 0x2E: iElemX = 223; iElemY = 146; break; /*** heal potion (b) ***/
			case 0x2F: iElemX = 285; iElemY = 146; break; /*** life potion (g) ***/
			case 0x30: iElemX = 347; iElemY = 146; break; /*** float potion (p) ***/
			case 0x31: iElemX = 409; iElemY = 146; break; /*** chomper ***/
			case 0x41: iElemX = 471; iElemY = 146; break; /*** loose floor ***/
			case 0x42: iElemX = 37; iElemY = 183; break; /*** loose floor fall ***/
			case 0x66: iElemX = 99; iElemY = 183; break; /*** shadow step ***/
			case 0x69: iElemX = 161; iElemY = 183; break; /*** shadow drink ***/
			case 0x6F: iElemX = 223; iElemY = 183; break; /*** princess ***/
			case 0x70: iElemX = 285; iElemY = 183; break; /*** mouse move ***/
			case 0x74: iElemX = 347; iElemY = 183; break; /*** shadow fight ***/
			default: iElemX = 0; iElemY = 0;
		}
		if ((iElemX != 0) && (iElemY != 0))
			{ ShowImage (imgsele, iElemX, iElemY, "imgsele", ascreen, iScale, 1); }
		/*** custom ***/
		CenterNumber (ascreen, iElem, 437, 185, color_bl, 1);
		/*** bytes ***/
		switch (iElem)
		{
			case 0x27: /*** level door ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x (odd)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y (10 or 18)");
				snprintf (arTextB3[0], MAX_TEXT, "%s", "? (2 or 1)");
				snprintf (arTextB4[0], MAX_TEXT, "%s", "? (7)");
				snprintf (arTextB5[0], MAX_TEXT, "%s", "? (0)");
				snprintf (arTextB6[0], MAX_TEXT, "%s", "? (0 or 250)");
				snprintf (arTextB7[0], MAX_TEXT, "%s", "? (0)");
				break;
			case 0x29: /*** mirror ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x (always 89)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y (always 2)");
				snprintf (arTextB3[0], MAX_TEXT, "%s", "? (222)");
				snprintf (arTextB4[0], MAX_TEXT, "%s", "reflection x (81)");
				snprintf (arTextB5[0], MAX_TEXT, "%s", "reflection y (9)");
				break;
			case 0x32: /*** gate ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x (32,64,96,128,192,200,224)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y (0,64,128)");
				snprintf (arTextB3[0], MAX_TEXT, "%s", "anim (2 or 255)");
				snprintf (arTextB4[0], MAX_TEXT, "%s", "anim spd (1 or 7)");
				snprintf (arTextB5[0], MAX_TEXT, "%s", "openness (5 or 31)");
				snprintf (arTextB6[0], MAX_TEXT, "%s", "? (140,250,254)");
				snprintf (arTextB7[0], MAX_TEXT, "%s", "? (0)");
				break;
			case 0x3E: /*** skel alive ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "? (7)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "? (1)");
				snprintf (arTextB3[0], MAX_TEXT, "%s", "? (64)");
				snprintf (arTextB4[0], MAX_TEXT, "%s", "x (128)");
				snprintf (arTextB5[0], MAX_TEXT, "%s", "y (64)");
				break;
			case 0x3F: /*** skel respawn ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "? (8)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "? (1)");
				snprintf (arTextB3[0], MAX_TEXT, "%s", "? (64)");
				snprintf (arTextB4[0], MAX_TEXT, "%s", "x (144)");
				snprintf (arTextB5[0], MAX_TEXT, "%s", "y (64)");
				break;
			case 0x61: /*** drop button ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x (even)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y (7,15,23)");
				snprintf (arTextB3[0], MAX_TEXT, "%s", "with wall (0=n,2=y)");
				snprintf (arTextB4[0], MAX_TEXT, "%s", "room"); /*** address 1/2 ***/
				snprintf (arTextB5[0], MAX_TEXT, "%s", "elem"); /*** address 2/2 ***/
				break;
			case 0x62: /*** raise button ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y (7,15,23)");
				snprintf (arTextB3[0], MAX_TEXT, "%s", "with wall (0=n,2=y)");
				snprintf (arTextB4[0], MAX_TEXT, "%s", "room"); /*** address 1/2 ***/
				snprintf (arTextB5[0], MAX_TEXT, "%s", "elem"); /*** address 2/2 ***/
				break;
			case 0x64: /*** guard ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "# (\"g\" to edit)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "type (0=g,1=s,2=J)");
				snprintf (arTextB3[0], MAX_TEXT, "%s", "anim (181)");
				snprintf (arTextB4[0], MAX_TEXT, "%s", "x");
				snprintf (arTextB5[0], MAX_TEXT, "%s", "y (0,64,128)");
				break;
			case 0x2B: /*** spikes ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y (0,64,128)");
				break;
			case 0x2C: /*** sword ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x (0,80)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y (48,130)");
				break;
			case 0x2D: /*** hurt potion (r) ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x (even)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y (0,2,64,66,128,130)");
				break;
			case 0x2E: /*** heal potion (b) ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x (even)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y (0,2,64,66,128,130)");
				break;
			case 0x2F: /*** life potion (g) ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x (even)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y (0,2,64,66,128,130)");
				break;
			case 0x30: /*** float potion (p) ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x (even)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y (0,2,64,66,128,130)");
				break;
			case 0x31: /*** chomper ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x (multiples of 16)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y (0,64,128)");
				break;
			case 0x41: /*** loose floor ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x (multiples of 4)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y (7,15,23)");
				break;
			case 0x42: /*** loose floor fall ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x (multiples of 4)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y (255)");
				break;
			case 0x66: /*** shadow step ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x (36)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y (64)");
				break;
			case 0x69: /*** shadow drink ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x (240)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y (0)");
				break;
			case 0x6F: /*** princess ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x (32)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y (64)");
				break;
			case 0x70: /*** mouse move ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x (224)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y (0)");
				break;
			case 0x74: /*** shadow fight ***/
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x (250)");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y (0)");
				break;
			default:
				snprintf (arTextB1[0], MAX_TEXT, "%s", "x");
				snprintf (arTextB2[0], MAX_TEXT, "%s", "y");
				break;
		}
		CenterNumber (ascreen, arElements[iCurLevel][iCurRoom][iActiveElement][1],
			64, 225, color_wh, 0);
		DisplayText (156, 227, 15, arTextB1, 1, font15, color_bl, color_wh, 0);
		CenterNumber (ascreen, arElements[iCurLevel][iCurRoom][iActiveElement][2],
			64, 250, color_wh, 0);
		DisplayText (156, 227+25, 15, arTextB2, 1, font15, color_bl, color_wh, 0);
		if ((iElem == 0x27) || (iElem == 0x29) ||
			(iElem == 0x32) || (iElem == 0x3E) ||
			(iElem == 0x3F) || (iElem == 0x61) ||
			(iElem == 0x62) || (iElem == 0x64))
		{
			CenterNumber (ascreen,
				arElements[iCurLevel][iCurRoom][iActiveElement][3],
				64, 275, color_wh, 0);
			DisplayText (156, 227+50, 15, arTextB3, 1,
				font15, color_bl, color_wh, 0);
			CenterNumber (ascreen,
				arElements[iCurLevel][iCurRoom][iActiveElement][4],
				64, 300, color_wh, 0);
			DisplayText (156, 227+75, 15, arTextB4, 1,
				font15, color_bl, color_wh, 0);
			CenterNumber (ascreen,
				arElements[iCurLevel][iCurRoom][iActiveElement][5],
				64, 325, color_wh, 0);
			DisplayText (156, 227+100, 15, arTextB5, 1,
				font15, color_bl, color_wh, 0);
		}
		if ((iElem == 0x27) || (iElem == 0x32))
		{
			CenterNumber (ascreen,
				arElements[iCurLevel][iCurRoom][iActiveElement][6],
				64, 350, color_wh, 0);
			DisplayText (156, 227+125, 15, arTextB6, 1,
				font15, color_bl, color_wh, 0);
			CenterNumber (ascreen,
				arElements[iCurLevel][iCurRoom][iActiveElement][7],
				64, 375, color_wh, 0);
			DisplayText (156, 227+150, 15, arTextB7, 1,
				font15, color_bl, color_wh, 0);
		}
	}

	if (iRoomConnections[iCurLevel][iCurRoom][1] != 256)
	{ /*** left ***/
		if (iDownAt == 1)
		{ /*** down ***/
			ShowImage (imgleft_1, 0, 50, "imgleft_1", ascreen, iScale, 1);
		} else { /*** up ***/
			ShowImage (imgleft_0, 0, 50, "imgleft_0", ascreen, iScale, 1);
		}
		if (iRoomConnections[iCurLevel][iCurRoom][1] > 24)
			{ ShowImage (imglinkwarnlr, 0, 50, "imglinkwarnlr",
			ascreen, iScale, 1); } /*** glow ***/
	} else { /*** no left ***/
		ShowImage (imglrno, 0, 50, "imglrno", ascreen, iScale, 1);
	}
	if (iRoomConnections[iCurLevel][iCurRoom][2] != 256)
	{ /*** right ***/
		if (iDownAt == 2)
		{ /*** down ***/
			ShowImage (imgright_1, 537, 50, "imgright_1", ascreen, iScale, 1);
		} else { /*** up ***/
			ShowImage (imgright_0, 537, 50, "imgright_0", ascreen, iScale, 1);
		}
		if (iRoomConnections[iCurLevel][iCurRoom][2] > 24)
			{ ShowImage (imglinkwarnlr, 539, 50, "imglinkwarnlr",
			ascreen, iScale, 1); } /*** glow ***/
	} else { /*** no right ***/
		ShowImage (imglrno, 537, 50, "imglrno", ascreen, iScale, 1);
	}
	if (iRoomConnections[iCurLevel][iCurRoom][3] != 256)
	{ /*** up ***/
		if (iDownAt == 3)
		{ /*** down ***/
			ShowImage (imgup_1, 25, 25, "imgup_1", ascreen, iScale, 1);
		} else { /*** up ***/
			ShowImage (imgup_0, 25, 25, "imgup_0", ascreen, iScale, 1);
		}
		if (iRoomConnections[iCurLevel][iCurRoom][3] > 24)
			{ ShowImage (imglinkwarnud, 25, 25, "imglinkwarnud",
			ascreen, iScale, 1); } /*** glow ***/
	} else { /*** no up ***/
		if (iScreen != 1)
		{ /*** without info ***/
			ShowImage (imgudno, 25, 25, "imgudno", ascreen, iScale, 1);
		} else { /*** with info ***/
			ShowImage (imgudnonfo, 25, 25, "imgudnonfo", ascreen, iScale, 1);
		}
	}
	if (iRoomConnections[iCurLevel][iCurRoom][4] != 256)
	{ /*** down ***/
		if (iDownAt == 4)
		{ /*** down ***/
			ShowImage (imgdown_1, 25, 434, "imgdown_1", ascreen, iScale, 1);
		} else { /*** up ***/
			ShowImage (imgdown_0, 25, 434, "imgdown_0", ascreen, iScale, 1);
		}
		if (iRoomConnections[iCurLevel][iCurRoom][4] > 24)
			{ ShowImage (imglinkwarnud, 25, 434, "imglinkwarnud",
			ascreen, iScale, 1); } /*** glow ***/
	} else { /*** no down ***/
		ShowImage (imgudno, 25, 434, "imgudno", ascreen, iScale, 1);
	}

	/*** room links ***/
	if (iScreen != 2)
	{
		if (iBrokenRoomLinks == 1)
		{
			/*** broken room links ***/
			if (iDownAt == 11)
			{ /*** down ***/
				ShowImage (imgbroomson_1, 0, 25, "imgbroomson_1", ascreen, iScale, 1);
			} else { /*** up ***/
				ShowImage (imgbroomson_0, 0, 25, "imgbroomson_0", ascreen, iScale, 1);
			}
		} else {
			/*** regular room links ***/
			if (iDownAt == 5)
			{ /*** down ***/
				ShowImage (imgroomson_1, 0, 25, "imgroomson_1", ascreen, iScale, 1);
			} else { /*** up ***/
				ShowImage (imgroomson_0, 0, 25, "imgroomson_0", ascreen, iScale, 1);
			}
		}
	} else {
		if (iBrokenRoomLinks == 1)
		{
			/*** broken room links ***/
			ShowImage (imgbroomsoff, 0, 25, "imgbroomsoff", ascreen, iScale, 1);
		} else {
			/*** regular room links ***/
			ShowImage (imgroomsoff, 0, 25, "imgroomsoff", ascreen, iScale, 1);
		}
	}

	/*** elements ***/
	if (iScreen != 3)
	{
		if (iDownAt == 6)
		{ /*** down ***/
			ShowImage (imgelementson_1, 537, 25, "imgelementson_1",
				ascreen, iScale, 1);
		} else { /*** up ***/
			ShowImage (imgelementson_0, 537, 25, "imgelementson_0",
				ascreen, iScale, 1);
		}
	} else { /*** off ***/
		ShowImage (imgelementsoff, 537, 25, "imgelementsoff", ascreen, iScale, 1);
	}

	/*** save ***/
	if (iChanged != 0)
	{ /*** on ***/
		if (iDownAt == 7)
		{ /*** down ***/
			ShowImage (imgsaveon_1, 0, 434, "imgsaveon_1", ascreen, iScale, 1);
		} else { /*** up ***/
			ShowImage (imgsaveon_0, 0, 434, "imgsaveon_0", ascreen, iScale, 1);
		}
	} else { /*** off ***/
		ShowImage (imgsaveoff, 0, 434, "imgsaveoff", ascreen, iScale, 1);
	}

	/*** quit ***/
	if (iDownAt == 8)
	{ /*** down ***/
		ShowImage (imgquit_1, 537, 434, "imgquit_1", ascreen, iScale, 1);
	} else { /*** up ***/
		ShowImage (imgquit_0, 537, 434, "imgquit_0", ascreen, iScale, 1);
	}

	/*** prev ***/
	if (iCurLevel != 1)
	{ /*** on ***/
		if (iDownAt == 9)
		{ /*** down ***/
			ShowImage (imgprevon_1, 0, 0, "imgprevon_1", ascreen, iScale, 1);
		} else { /*** up ***/
			ShowImage (imgprevon_0, 0, 0, "imgprevon_0", ascreen, iScale, 1);
		}
	} else { /*** off ***/
		ShowImage (imgprevoff, 0, 0, "imgprevoff", ascreen, iScale, 1);
	}

	/*** next ***/
	if (iCurLevel != NR_LEVELS)
	{ /*** on ***/
		if (iDownAt == 10)
		{ /*** down ***/
			ShowImage (imgnexton_1, 537, 0, "imgnexton_1", ascreen, iScale, 1);
		} else { /*** up ***/
			ShowImage (imgnexton_0, 537, 0, "imgnexton_0", ascreen, iScale, 1);
		}
	} else { /*** off ***/
		ShowImage (imgnextoff, 537, 0, "imgnextoff", ascreen, iScale, 1);
	}

	/*** level bar ***/
	ShowImage (imgbar, 25, 0, "imgbar", ascreen, iScale, 1);
	switch (iCurLevel)
	{
		case 1: snprintf (sLevel, MAX_TEXT, "%s", "level 1 (prison)"); break;
		case 2: snprintf (sLevel, MAX_TEXT, "%s", "level 2 (guards)"); break;
		case 3: snprintf (sLevel, MAX_TEXT, "%s", "level 3 (skeleton)"); break;
		case 4: snprintf (sLevel, MAX_TEXT, "%s", "level 4 (mirror)"); break;
		case 5: snprintf (sLevel, MAX_TEXT, "%s", "level 5 (thief)"); break;
		case 6: snprintf (sLevel, MAX_TEXT, "%s", "level 6 (plunge)"); break;
		case 7: snprintf (sLevel, MAX_TEXT, "%s", "level 7 (weightless)"); break;
		case 8: snprintf (sLevel, MAX_TEXT, "%s", "level 8 (mouse)"); break;
		case 9: snprintf (sLevel, MAX_TEXT, "%s", "level 9 (twisty)"); break;
		case 10: snprintf (sLevel, MAX_TEXT, "%s", "level 10 (quad)"); break;
		case 11: snprintf (sLevel, MAX_TEXT, "%s", "level 11 (fragile)"); break;
		case 12: snprintf (sLevel, MAX_TEXT, "%s", "level 12 (12a; tower)"); break;
		case 13: snprintf (sLevel, MAX_TEXT, "%s",
			"level 13 (12b; jaffar)"); break;
		case 14: snprintf (sLevel, MAX_TEXT, "%s", "level 14 (rescue)"); break;
	}
	switch (iScreen)
	{
		case 1:
			snprintf (arText[0], MAX_TEXT, "%s, room %i", sLevel, iCurRoom);
			ShowImage (extras[iExtras], 480, 3, "extras[iExtras]",
				ascreen, iScale, 1);
			break;
		case 2:
			snprintf (arText[0], MAX_TEXT, "%s, room links", sLevel); break;
		case 3:
			snprintf (arText[0], MAX_TEXT, "%s, elements", sLevel); break;
	}
	DisplayText (31, 5, 15, arText, 1, font15, color_wh, color_bl, 0);
	if (iPlaytest == 1)
		{ ShowImage (imgplaytest, 25, 50, "imgplaytest", ascreen, iScale, 1); }

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void InitPopUp (void)
/*****************************************************************************/
{
	SDL_Event event;
	int iPopUp;

	iPopUp = 1;

	PlaySound ("wav/popup.wav");
	ShowPopUp();
	SDL_RaiseWindow (window);

	while (iPopUp == 1)
	{
		while (SDL_PollEvent (&event))
		{
			if (MapEvents (event) == 0)
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
							iPopUp = 0; break;
					}
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						case SDLK_SPACE:
						case SDLK_o:
							iPopUp = 0; break;
					}
					break;
				case SDL_MOUSEMOTION:
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						if (InArea (375, 319, 375 + 85, 319 + 32) == 1) /*** OK ***/
						{
							if (iOKOn != 1) { iOKOn = 1; }
							ShowPopUp();
						}
					}
					break;
				case SDL_MOUSEBUTTONUP:
					iOKOn = 0;
					if (event.button.button == 1)
					{
						if (InArea (375, 319, 375 + 85, 319 + 32) == 1) /*** OK ***/
						{
							iPopUp = 0;
						}
					}
					ShowPopUp(); break;
				case SDL_WINDOWEVENT:
					switch (event.window.event)
					{
						case SDL_WINDOWEVENT_EXPOSED:
							ShowScreen (iScreen); ShowPopUp(); break;
						case SDL_WINDOWEVENT_CLOSE:
							Quit(); break;
						case SDL_WINDOWEVENT_FOCUS_GAINED:
							iActiveWindowID = iWindowID; break;
					}
					break;
				case SDL_QUIT:
					Quit(); break;
			}
		}

		/*** prevent CPU eating ***/
		gamespeed = REFRESH;
		while ((SDL_GetTicks() - looptime) < gamespeed)
		{
			SDL_Delay (10);
		}
		looptime = SDL_GetTicks();
	}
	PlaySound ("wav/popup_close.wav");
	ShowScreen (iScreen);
}
/*****************************************************************************/
void PlaySound (char *sFile)
/*****************************************************************************/
{
	int iIndex;
	SDL_AudioSpec wave;
	Uint8 *data;
	Uint32 dlen;
	SDL_AudioCVT cvt;

	if (iNoAudio == 1) { return; }
	for (iIndex = 0; iIndex < NUM_SOUNDS; iIndex++)
	{
		if (sounds[iIndex].dpos == sounds[iIndex].dlen)
		{
			break;
		}
	}
	if (iIndex == NUM_SOUNDS) { return; }

	if (SDL_LoadWAV (sFile, &wave, &data, &dlen) == NULL)
	{
		printf ("[FAILED] Could not load %s: %s!\n", sFile, SDL_GetError());
		exit (EXIT_ERROR);
	}
	SDL_BuildAudioCVT (&cvt, wave.format, wave.channels, wave.freq, AUDIO_S16, 2,
		44100);
	/*** The "+ 1" is a workaround for SDL bug #2274. ***/
	cvt.buf = (Uint8 *)malloc (dlen * (cvt.len_mult + 1));
	memcpy (cvt.buf, data, dlen);
	cvt.len = dlen;
	SDL_ConvertAudio (&cvt);
	SDL_FreeWAV (data);

	if (sounds[iIndex].data)
	{
		free (sounds[iIndex].data);
	}
	SDL_LockAudio();
	sounds[iIndex].data = cvt.buf;
	sounds[iIndex].dlen = cvt.len_cvt;
	sounds[iIndex].dpos = 0;
	SDL_UnlockAudio();
}
/*****************************************************************************/
void ShowPopUp (void)
/*****************************************************************************/
{
	char arText[9 + 2][MAX_TEXT + 2];

	/*** faded background ***/
	ShowImage (imgfaded, 0, 0, "imgfaded", ascreen, iScale, 1);
	/*** popup ***/
	ShowImage (imgpopup, 33, 9, "imgpopup", ascreen, iScale, 1);
	switch (iOKOn)
	{
		case 0:
			/*** OK off ***/
			ShowImage (imgok[1], 375, 319, "imgok[1]", ascreen, iScale, 1); break;
		case 1:
			/*** OK on ***/
			ShowImage (imgok[2], 375, 319, "imgok[2]", ascreen, iScale, 1); break;
	}

	snprintf (arText[0], MAX_TEXT, "%s %s", EDITOR_NAME, EDITOR_VERSION);
	snprintf (arText[1], MAX_TEXT, "%s", COPYRIGHT);
	snprintf (arText[2], MAX_TEXT, "%s", "");
	if (iController != 1)
	{
		snprintf (arText[3], MAX_TEXT, "%s", "single tile (change or select)");
		snprintf (arText[4], MAX_TEXT, "%s", "entire room (clear or fill)");
		snprintf (arText[5], MAX_TEXT, "%s", "entire level (randomize or fill)");
	} else {
		snprintf (arText[3], MAX_TEXT, "%s", "The detected game controller:");
		snprintf (arText[4], MAX_TEXT, "%s", sControllerName);
		snprintf (arText[5], MAX_TEXT, "%s", "Have fun using it!");
	}
	snprintf (arText[6], MAX_TEXT, "%s", "");
	snprintf (arText[7], MAX_TEXT, "%s", "Using multiple guards per room may");
	snprintf (arText[8], MAX_TEXT, "%s", "have strange effects.");

	DisplayText (115, 120, 15, arText, 9,
		font15, color_wh, color_bl, 0);

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
int MapEvents (SDL_Event event)
/*****************************************************************************/
{
	if (event.button.button != 0) { } /*** To prevent warnings. ***/

	return (0); /*** To prevent warnings. ***/
}
/*****************************************************************************/
int InArea (int iUpperLeftX, int iUpperLeftY,
	int iLowerRightX, int iLowerRightY)
/*****************************************************************************/
{
	if ((iUpperLeftX * iScale <= iXPos) &&
		(iLowerRightX * iScale >= iXPos) &&
		(iUpperLeftY * iScale <= iYPos) &&
		(iLowerRightY * iScale >= iYPos))
	{
		return (1);
	} else {
		return (0);
	}
}
/*****************************************************************************/
void Quit (void)
/*****************************************************************************/
{
	if (iChanged != 0) { InitPopUpSave(); }
	if (iModified == 1) { ModifyBack(); }
	TTF_CloseFont (font11);
	TTF_CloseFont (font15);
	TTF_CloseFont (font20);
	TTF_Quit();
	SDL_Quit();
	exit (EXIT_NORMAL);
}
/*****************************************************************************/
void DisplayText (int iStartX, int iStartY, int iFontSize,
	char arText[9 + 2][MAX_TEXT + 2], int iLines, TTF_Font *font,
	SDL_Color back, SDL_Color fore, int iOnMap)
/*****************************************************************************/
{
	/*** Used for looping. ***/
	int iLoopLine;

	for (iLoopLine = 0; iLoopLine < iLines; iLoopLine++)
	{
		if (strcmp (arText[iLoopLine], "") != 0)
		{
			message = TTF_RenderText_Shaded (font,
				arText[iLoopLine], fore, back);
			if (iOnMap == 0)
			{
				messaget = SDL_CreateTextureFromSurface (ascreen, message);
			} else {
				/*** messaget = SDL_CreateTextureFromSurface (mscreen, message); ***/
			}
			if ((strcmp (arText[iLoopLine], "single tile (change or select)") == 0)
				|| (strcmp (arText[iLoopLine], "entire room (clear or fill)") == 0) ||
				(strcmp (arText[iLoopLine], "entire level (randomize or fill)") == 0))
			{
				offset.x = iStartX + 20;
			} else {
				offset.x = iStartX;
			}
			offset.y = iStartY + (iLoopLine * (iFontSize + 4));
			if (iOnMap == 0)
			{
				offset.w = message->w; offset.h = message->h;
				if (strcmp (arText[1], "mouse") != 0)
				{
					CustomRenderCopy (messaget, "message", NULL, ascreen, &offset);
				} else {
					CustomRenderCopy (messaget, "mouse message", NULL, ascreen, &offset);
				}
			} else {
/***
				offset.w = message->w / iScale; offset.h = message->h / iScale;
				CustomRenderCopy (messaget, "map message", NULL, mscreen, &offset);
***/
			}
			SDL_DestroyTexture (messaget); SDL_FreeSurface (message);
		}
	}
}
/*****************************************************************************/
void InitPopUpSave (void)
/*****************************************************************************/
{
	int iPopUp;
	SDL_Event event;

	iPopUp = 1;

	PlaySound ("wav/popup_yn.wav");
	ShowPopUpSave();
	SDL_RaiseWindow (window);

	while (iPopUp == 1)
	{
		while (SDL_PollEvent (&event))
		{
			if (MapEvents (event) == 0)
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
							SaveLevel (iCurLevel); iPopUp = 0; break;
						case SDL_CONTROLLER_BUTTON_B:
							iPopUp = 0; break;
					}
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						case SDLK_n:
							iPopUp = 0; break;
						case SDLK_y:
							SaveLevel (iCurLevel); iPopUp = 0; break;
					}
					break;
				case SDL_MOUSEMOTION:
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						if (InArea (102, 319, 102 + 85, 319 + 32) == 1) { iNoOn = 1; }
						if (InArea (375, 319, 375 + 85, 319 + 32) == 1) { iYesOn = 1; }
					}
					ShowPopUpSave();
					break;
				case SDL_MOUSEBUTTONUP:
					iNoOn = 0;
					iYesOn = 0;
					if (event.button.button == 1)
					{
						if (InArea (102, 319, 102 + 85, 319 + 32) == 1)
							{ iPopUp = 0; }
						if (InArea (375, 319, 375 + 85, 319 + 32) == 1)
							{ SaveLevel (iCurLevel); iPopUp = 0; }
					}
					ShowPopUpSave();
					break;
				case SDL_WINDOWEVENT:
					switch (event.window.event)
					{
						case SDL_WINDOWEVENT_EXPOSED:
							ShowScreen (iScreen); ShowPopUpSave(); break;
						case SDL_WINDOWEVENT_CLOSE:
							Quit(); break;
						case SDL_WINDOWEVENT_FOCUS_GAINED:
							iActiveWindowID = iWindowID; break;
					}
					break;
				case SDL_QUIT:
					Quit(); break;
			}
		}

		/*** prevent CPU eating ***/
		gamespeed = REFRESH;
		while ((SDL_GetTicks() - looptime) < gamespeed)
		{
			SDL_Delay (10);
		}
		looptime = SDL_GetTicks();
	}
	PlaySound ("wav/popup_close.wav");
	ShowScreen (iScreen);
}
/*****************************************************************************/
void CustomRenderCopy (SDL_Texture* src, char *sImageInfo, SDL_Rect* srcrect,
	SDL_Renderer* dst, SDL_Rect *dstrect)
/*****************************************************************************/
{
	SDL_Rect stuff;

	if ((strcmp (sImageInfo, "map message") == 0) ||
		(strcmp (sImageInfo, "mouse message") == 0))
	{
		stuff.x = dstrect->x;
		stuff.y = dstrect->y;
	} else {
		stuff.x = dstrect->x * iScale;
		stuff.y = dstrect->y * iScale;
	}
	if (srcrect != NULL) /*** image ***/
	{
		stuff.w = dstrect->w * iScale;
		stuff.h = dstrect->h * iScale;
	} else { /*** font ***/
		stuff.w = dstrect->w;
		stuff.h = dstrect->h;
	}
	if (SDL_RenderCopy (dst, src, srcrect, &stuff) != 0)
	{
		printf ("[ WARN ] SDL_RenderCopy (%s): %s!\n",
			sImageInfo, SDL_GetError());
	}
}
/*****************************************************************************/
void ShowPopUpSave (void)
/*****************************************************************************/
{
	char arText[2 + 2][MAX_TEXT + 2];

	/*** faded background ***/
	ShowImage (imgfaded, 0, 0, "imgfaded", ascreen, iScale, 1);
	/*** popup_yn ***/
	ShowImage (imgpopup_yn, 85, 91, "imgpopup_yn", ascreen, iScale, 1);

	switch (iNoOn)
	{
		case 0: /*** off ***/
			ShowImage (imgno[1], 102, 319, "imgno[1]", ascreen, iScale, 1);
			break;
		case 1: /*** on ***/
			ShowImage (imgno[2], 102, 319, "imgno[2]", ascreen, iScale, 1);
			break;
	}
	switch (iYesOn)
	{
		case 0: /*** off ***/
			ShowImage (imgyes[1], 375, 319, "imgyes[1]", ascreen, iScale, 1);
			break;
		case 1: /*** on ***/
			ShowImage (imgyes[2], 375, 319, "imgyes[2]", ascreen, iScale, 1);
			break;
	}

	if (iChanged == 1)
	{
		snprintf (arText[0], MAX_TEXT, "%s", "You made an unsaved change.");
		snprintf (arText[1], MAX_TEXT, "%s", "Do you want to save it?");
	} else {
		snprintf (arText[0], MAX_TEXT, "There are unsaved changes.");
		snprintf (arText[1], MAX_TEXT, "%s", "Do you wish to save these?");
	}
	DisplayText (115, 120, 15, arText, 2, font15, color_wh, color_bl, 0);

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void SaveLevel (int iLevel)
/*****************************************************************************/
{
	int iFd;
	char sToWrite[MAX_TOWRITE + 2];
	int iSaveBytes;
	int iOffset;
	int iRooms;
	int iChunkLength;
	int iOffsetEl, iOffsetUse;
	int iGuardColor, iGuardOther;

	/*** Used for looping. ***/
	int iLoopLevel;
	int iLoopLink;
	int iLoopRoom;
	int iLoopElem;
	int iLoopByte;
	int iLoopDef;
	int iLoopEmpty;
	int iLoopRow;
	int iLoopGuard;

	if (iChanged == 0) { return; }

	CreateBAK();

	iFd = open (sPathFile, O_WRONLY|O_BINARY, 0600);
	if (iFd == -1)
	{
		printf ("[FAILED] Could not open \"%s\": %s!\n",
			sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}

	/*************************************************************/
	/* Room links and elements ("Room links and objects table"). */
	/*************************************************************/

	iOffset = iOffsetRlElTable;
	lseek (iFd, iOffset, SEEK_SET);
	for (iLoopLevel = 1; iLoopLevel <= NR_LEVELS; iLoopLevel++)
	{
		/*** (This is used in the next iLoopLevel loop.) ***/
		arOffsetRlElTable[iLoopLevel] = iOffset;

		/*** Save chunk length. ***/
		switch (iLoopLevel)
		{
			case 14: iRooms = 8; break;
			default: iRooms = ROOMS; break;
		}
		iChunkLength = GetChunkLength (iLoopLevel, iRooms);
		WriteWord (iFd, iChunkLength);
		iOffset+=2;

		/*** Save offsets of elements. ***/
		for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
		{
			if ((iLoopLevel == 14) && (iLoopRoom > 8))
			{
				WriteWord (iFd, 0x00);
			} else {
				WriteWord (iFd, GetChunkLength (iLoopLevel, (iLoopRoom - 1)));
			}
			iOffset+=2;
		}

		/*** Save the room links. ***/
		switch (iLoopLevel)
		{
			case 14: iRooms = 5; break;
			default: iRooms = ROOMS; break;
		}
		for (iLoopLink = 0; iLoopLink < (iRooms * 4); iLoopLink+=4)
		{
			snprintf (sToWrite, MAX_TOWRITE, "%c",
				iRoomConnections[iLoopLevel][(iLoopLink / 4) + 1][3] - 1);
			write (iFd, sToWrite, 1);
			snprintf (sToWrite, MAX_TOWRITE, "%c",
				iRoomConnections[iLoopLevel][(iLoopLink / 4) + 1][2] - 1);
			write (iFd, sToWrite, 1);
			snprintf (sToWrite, MAX_TOWRITE, "%c",
				iRoomConnections[iLoopLevel][(iLoopLink / 4) + 1][4] - 1);
			write (iFd, sToWrite, 1);
			snprintf (sToWrite, MAX_TOWRITE, "%c",
				iRoomConnections[iLoopLevel][(iLoopLink / 4) + 1][1] - 1);
			write (iFd, sToWrite, 1);
			iOffset+=4;
		}

		switch (iLoopLevel)
		{
			case 14: iRooms = 8; break;
			default: iRooms = ROOMS; break;
		}

		/* Before saving elements, convert "room" and "elem" of buttons back to
		 * "address 1/2" and "address 2/2". This requires first updating all
		 * offsets in arElements[][][][9].
		 */
		iOffsetEl = 144;
		for (iLoopRoom = 1; iLoopRoom <= iRooms; iLoopRoom++)
		{
			for (iLoopElem = 1; iLoopElem <= MAX_ELEM; iLoopElem++)
			{
				if (arElements[iLoopLevel][iLoopRoom][iLoopElem][8] == 0x01)
				{
					arElements[iLoopLevel][iLoopRoom][iLoopElem][9] = iOffsetEl;
					switch (arElements[iLoopLevel][iLoopRoom][iLoopElem][0])
					{
						case 0x27: iOffsetEl+=8; break;
						case 0x29: iOffsetEl+=6; break;
						case 0x32: iOffsetEl+=8; break;
						case 0x3E: iOffsetEl+=6; break;
						case 0x3F: iOffsetEl+=6; break;
						case 0x61: iOffsetEl+=6; break;
						case 0x62: iOffsetEl+=6; break;
						case 0x64: iOffsetEl+=6; break;
						default: iOffsetEl+=3; break;
					}
				}
			}
			iOffsetEl++;
		}
		for (iLoopRoom = 1; iLoopRoom <= iRooms; iLoopRoom++)
		{
			for (iLoopElem = 1; iLoopElem <= MAX_ELEM; iLoopElem++)
			{
				/*** Drop or raise button. ***/
				if ((arElements[iLoopLevel][iLoopRoom][iLoopElem][0] == 0x61) ||
					(arElements[iLoopLevel][iLoopRoom][iLoopElem][0] == 0x62))
				{
					/*** Prevent crashes using fallbacks. ***/
					if ((arElements[iLoopLevel][iLoopRoom][iLoopElem][4] < 1) ||
						(arElements[iLoopLevel][iLoopRoom][iLoopElem][4] > ROOMS))
					{
						arElements[iLoopLevel][iLoopRoom][iLoopElem][4] = 1;
						printf ("[ WARN ] Using workaround for l %i,r %i,e %i!\n",
							iLoopLevel, iLoopRoom, iLoopElem);
					}
					if ((arElements[iLoopLevel][iLoopRoom][iLoopElem][5] < 1) ||
						(arElements[iLoopLevel][iLoopRoom][iLoopElem][5] > MAX_ELEM))
					{
						arElements[iLoopLevel][iLoopRoom][iLoopElem][5] = 1;
						printf ("[ WARN ] Using workaround for l %i,r %i,e %i!\n",
							iLoopLevel, iLoopRoom, iLoopElem);
					}

					iOffsetUse = arElements[iLoopLevel][
						arElements[iLoopLevel][iLoopRoom][iLoopElem][4]
					][
						arElements[iLoopLevel][iLoopRoom][iLoopElem][5]
					][9];
					if (iOffsetUse == 0)
					{
						printf ("[ WARN ] Button offset 0 for l %i,r %i,e %i!\n",
							iLoopLevel, iLoopRoom, iLoopElem);
					}
					arElements[iLoopLevel][iLoopRoom][iLoopElem][4] =
						(iOffsetUse >> 0) & 0xFF;
					arElements[iLoopLevel][iLoopRoom][iLoopElem][5] =
						(iOffsetUse >> 8) & 0xFF;
				}
			}
		}

		/*** Save elements. ***/
		for (iLoopRoom = 1; iLoopRoom <= iRooms; iLoopRoom++)
		{
			for (iLoopElem = 1; iLoopElem <= MAX_ELEM; iLoopElem++)
			{
				if (arElements[iLoopLevel][iLoopRoom][iLoopElem][8] == 0x01)
				{
					snprintf (sToWrite, MAX_TOWRITE, "%c",
						(int)arElements[iLoopLevel][iLoopRoom][iLoopElem][0]);
					write (iFd, sToWrite, 1);
					switch (arElements[iLoopLevel][iLoopRoom][iLoopElem][0])
					{
						case 0x27: iSaveBytes = 8; break;
						case 0x29: iSaveBytes = 6; break;
						case 0x32: iSaveBytes = 8; break;
						case 0x3E: iSaveBytes = 6; break;
						case 0x3F: iSaveBytes = 6; break;
						case 0x61: iSaveBytes = 6; break;
						case 0x62: iSaveBytes = 6; break;
						case 0x64: iSaveBytes = 6; break;
						default: iSaveBytes = 3; break;
					}
					for (iLoopByte = 1; iLoopByte <= (iSaveBytes - 1); iLoopByte++)
					{
						WriteByte (iFd, arElements[iLoopLevel][iLoopRoom]
							[iLoopElem][iLoopByte]);
					}
					iOffset+=iSaveBytes;
				}
			}
			WriteByte (iFd, 0xFF);
			iOffset++;
		}
	}

	AddressToRoomAndElem();

	for (iLoopLevel = 1; iLoopLevel <= NR_LEVELS; iLoopLevel++)
	{
		/*******************************/
		/* Levels ("The level table"). */
		/*******************************/

		lseek (iFd, iOffsetLevelTable + ((iLoopLevel - 1) * 20), SEEK_SET);

		/*** 0x60C9 ***/
		WriteWord (iFd, 0x60C9);

		/*** Tile graphics. ***/
		/*** Ignoring arOffsetTG1[] and arOffsetTG2[]. ***/
		if (arLevelType[iLoopLevel] == 'd')
		{
			WriteWord (iFd, LinearToBanked (0x0C000));
			WriteWord (iFd, 0xA5B2);
		} else {
			WriteWord (iFd, LinearToBanked (0x0D4C0));
			WriteWord (iFd, 0xA8D2);
		}

		/*** Offset tiles. ***/
		WriteWord (iFd, LinearToBanked (arOffsetTiles[iLoopLevel]));

		/*** Level type. ***/
		switch (arLevelType[iLoopLevel])
		{
			case 'd': WriteByte (iFd, 0x05); break;
			case 'p': WriteByte (iFd, 0x85); break;
		}

		/*** Save start position. ***/
		WriteByte (iFd, arKidDir[iLoopLevel]);
		WriteByte (iFd, arKidPosX[iLoopLevel]);
		WriteByte (iFd, arKidPosY[iLoopLevel]);
		WriteByte (iFd, arKidRoom[iLoopLevel] - 1);
		WriteByte (iFd, arKidAnim[iLoopLevel]);

		/*** Unknown. ***/
		WriteByte (iFd, arUnknown[iLoopLevel][1]);
		WriteByte (iFd, arUnknown[iLoopLevel][2]);
		WriteByte (iFd, arUnknown[iLoopLevel][3]);
		WriteByte (iFd, arUnknown[iLoopLevel][4]);

		/*** Room links address. ***/
		WriteWord (iFd, arOffsetRlElTable[iLoopLevel]);
	}

	/**************************/
	/* Tiles ("Graphics map") */
	/**************************/

	iOffset = arOffsetTiles[iLevel];
	lseek (iFd, iOffset, SEEK_SET);
	for (iLoopDef = 0; iLoopDef < 64; iLoopDef++)
	{
		WriteByte (iFd, iLoopDef);
	}
	iOffset+=64;
	switch (iLevel)
	{
		case 13: iRooms = 16; break;
		case 14: iRooms = 8; break;
		default: iRooms = ROOMS; break;
	}
	for (iLoopRow = 1; iLoopRow <= ROWS; iLoopRow++)
	{
		for (iLoopRoom = 1; iLoopRoom <= 8; iLoopRoom++)
		{
			GetAsEightBits (arTiles[iLoopRoom][1][iLoopRow], arToSave[1]);
			GetAsEightBits (arTiles[iLoopRoom][2][iLoopRow], arToSave[2]);
			GetAsEightBits (arTiles[iLoopRoom][3][iLoopRow], arToSave[3]);
			GetAsEightBits (arTiles[iLoopRoom][4][iLoopRow], arToSave[4]);
			SaveFourAsThree (iFd, iLoopRoom);
			iOffset+=3;
			GetAsEightBits (arTiles[iLoopRoom][5][iLoopRow], arToSave[1]);
			GetAsEightBits (arTiles[iLoopRoom][6][iLoopRow], arToSave[2]);
			GetAsEightBits (arTiles[iLoopRoom][7][iLoopRow], arToSave[3]);
			GetAsEightBits (arTiles[iLoopRoom][8][iLoopRow], arToSave[4]);
			SaveFourAsThree (iFd, iLoopRoom);
			iOffset+=3;
			GetAsEightBits (arTiles[iLoopRoom][9][iLoopRow], arToSave[1]);
			GetAsEightBits (arTiles[iLoopRoom][10][iLoopRow], arToSave[2]);
			GetAsEightBits (arTiles[iLoopRoom][11][iLoopRow], arToSave[3]);
			GetAsEightBits (arTiles[iLoopRoom][12][iLoopRow], arToSave[4]);
			SaveFourAsThree (iFd, iLoopRoom);
			iOffset+=3;
			GetAsEightBits (arTiles[iLoopRoom][13][iLoopRow], arToSave[1]);
			GetAsEightBits (arTiles[iLoopRoom][14][iLoopRow], arToSave[2]);
			GetAsEightBits (arTiles[iLoopRoom][15][iLoopRow], arToSave[3]);
			GetAsEightBits (arTiles[iLoopRoom][16][iLoopRow], arToSave[4]);
			SaveFourAsThree (iFd, iLoopRoom);
			iOffset+=3;
		}
	}
	if (iRooms > 8)
	{
		for (iLoopRow = 1; iLoopRow <= ROWS; iLoopRow++)
		{
			for (iLoopRoom = 9; iLoopRoom <= 16; iLoopRoom++)
			{
				GetAsEightBits (arTiles[iLoopRoom][1][iLoopRow], arToSave[1]);
				GetAsEightBits (arTiles[iLoopRoom][2][iLoopRow], arToSave[2]);
				GetAsEightBits (arTiles[iLoopRoom][3][iLoopRow], arToSave[3]);
				GetAsEightBits (arTiles[iLoopRoom][4][iLoopRow], arToSave[4]);
				SaveFourAsThree (iFd, iLoopRoom);
				iOffset+=3;
				GetAsEightBits (arTiles[iLoopRoom][5][iLoopRow], arToSave[1]);
				GetAsEightBits (arTiles[iLoopRoom][6][iLoopRow], arToSave[2]);
				GetAsEightBits (arTiles[iLoopRoom][7][iLoopRow], arToSave[3]);
				GetAsEightBits (arTiles[iLoopRoom][8][iLoopRow], arToSave[4]);
				SaveFourAsThree (iFd, iLoopRoom);
				iOffset+=3;
				GetAsEightBits (arTiles[iLoopRoom][9][iLoopRow], arToSave[1]);
				GetAsEightBits (arTiles[iLoopRoom][10][iLoopRow], arToSave[2]);
				GetAsEightBits (arTiles[iLoopRoom][11][iLoopRow], arToSave[3]);
				GetAsEightBits (arTiles[iLoopRoom][12][iLoopRow], arToSave[4]);
				SaveFourAsThree (iFd, iLoopRoom);
				iOffset+=3;
				GetAsEightBits (arTiles[iLoopRoom][13][iLoopRow], arToSave[1]);
				GetAsEightBits (arTiles[iLoopRoom][14][iLoopRow], arToSave[2]);
				GetAsEightBits (arTiles[iLoopRoom][15][iLoopRow], arToSave[3]);
				GetAsEightBits (arTiles[iLoopRoom][16][iLoopRow], arToSave[4]);
				SaveFourAsThree (iFd, iLoopRoom);
				iOffset+=3;
			}
		}
	}
	if (iRooms > 16)
	{
		for (iLoopRow = 1; iLoopRow <= ROWS; iLoopRow++)
		{
			for (iLoopRoom = 17; iLoopRoom <= 24; iLoopRoom++)
			{
				GetAsEightBits (arTiles[iLoopRoom][1][iLoopRow], arToSave[1]);
				GetAsEightBits (arTiles[iLoopRoom][2][iLoopRow], arToSave[2]);
				GetAsEightBits (arTiles[iLoopRoom][3][iLoopRow], arToSave[3]);
				GetAsEightBits (arTiles[iLoopRoom][4][iLoopRow], arToSave[4]);
				SaveFourAsThree (iFd, iLoopRoom);
				iOffset+=3;
				GetAsEightBits (arTiles[iLoopRoom][5][iLoopRow], arToSave[1]);
				GetAsEightBits (arTiles[iLoopRoom][6][iLoopRow], arToSave[2]);
				GetAsEightBits (arTiles[iLoopRoom][7][iLoopRow], arToSave[3]);
				GetAsEightBits (arTiles[iLoopRoom][8][iLoopRow], arToSave[4]);
				SaveFourAsThree (iFd, iLoopRoom);
				iOffset+=3;
				GetAsEightBits (arTiles[iLoopRoom][9][iLoopRow], arToSave[1]);
				GetAsEightBits (arTiles[iLoopRoom][10][iLoopRow], arToSave[2]);
				GetAsEightBits (arTiles[iLoopRoom][11][iLoopRow], arToSave[3]);
				GetAsEightBits (arTiles[iLoopRoom][12][iLoopRow], arToSave[4]);
				SaveFourAsThree (iFd, iLoopRoom);
				iOffset+=3;
				GetAsEightBits (arTiles[iLoopRoom][13][iLoopRow], arToSave[1]);
				GetAsEightBits (arTiles[iLoopRoom][14][iLoopRow], arToSave[2]);
				GetAsEightBits (arTiles[iLoopRoom][15][iLoopRow], arToSave[3]);
				GetAsEightBits (arTiles[iLoopRoom][16][iLoopRow], arToSave[4]);
				SaveFourAsThree (iFd, iLoopRoom);
				iOffset+=3;
			}
		}
	}
	for (iLoopEmpty = 1; iLoopEmpty <= 32; iLoopEmpty++)
	{
		WriteByte (iFd, sEmpty[iLoopEmpty - 1]);
	}

	/**************************/
	/* Guards ("Guard table") */
	/**************************/

	lseek (iFd, iOffsetGuardTable, SEEK_SET);
	for (iLoopGuard = 0; iLoopGuard <= MAX_GUARDS; iLoopGuard++)
	{
		iGuardColor = 0;
		switch (arGuardR[iLoopGuard])
		{
			/*** case 0 ***/
			case 1: iGuardColor+=1; break;
			case 2: iGuardColor+=2; break;
			case 3: iGuardColor+=1+2; break;
		}
		switch (arGuardG[iLoopGuard])
		{
			/*** case 0 ***/
			case 1: iGuardColor+=4; break;
			case 2: iGuardColor+=8; break;
			case 3: iGuardColor+=4+8; break;
		}
		switch (arGuardB[iLoopGuard])
		{
			/*** case 0 ***/
			case 1: iGuardColor+=16; break;
			case 2: iGuardColor+=32; break;
			case 3: iGuardColor+=16+32; break;
		}
		WriteByte (iFd, iGuardColor);
		WriteByte (iFd, arGuardGameLevel[iLoopGuard]);
		iGuardOther = 0;
		switch (arGuardHP[iLoopGuard])
		{
			/*** case 0 ***/
			case 1: iGuardOther+=1; break;
			case 2: iGuardOther+=2; break;
			case 3: iGuardOther+=3; break;
			case 4: iGuardOther+=4; break;
			case 5: iGuardOther+=5; break;
			case 6: iGuardOther+=6; break;
			case 7: iGuardOther+=7; break;
			case 8: iGuardOther+=8; break;
			case 9: iGuardOther+=9; break;
			case 10: iGuardOther+=10; break;
			case 11: iGuardOther+=11; break;
			case 12: iGuardOther+=12; break;
			case 13: iGuardOther+=13; break;
			case 14: iGuardOther+=14; break;
			case 15: iGuardOther+=15; break;
		}
		/*** Unused: 32 and 16 ***/
		if (arGuardDir[iLoopGuard] == 1) { iGuardOther+=64; }
		if (arGuardMove[iLoopGuard] == 1) { iGuardOther+=128; }
		WriteByte (iFd, iGuardOther);
	}

	close (iFd);

	PlaySound ("wav/save.wav");

	iChanged = 0;
}
/*****************************************************************************/
void ShowTile (int iRoom, int iRow, int iCol)
/*****************************************************************************/
{
	int iX, iY;
	char sShowTile[MAX_TEXT + 2];
	int iRowUse;
	int iTile;
	char arText[1 + 2][MAX_TEXT + 2];

	iRowUse = iRow;
	iX = ((iCol - 1) * 32) + 25;
	if (iRowUse == 0)
	{
		/*** above this room ***/
		iY = -32 + 50 + 6;
		iRowUse = 12;
	} else {
		iY = ((iRowUse - 1) * 32) + 50 + 6;
	}

	if (cCurType == 'd')
	{
		if (iRoom == 0)
		{
			/*** no room above this room ***/
			iTile = 1;
		} else {
			iTile = arTiles[iRoom][iCol][iRowUse];
		}
		if ((iNoAnim == 0) && (iTile == 49))
		{
			ShowImage (spriteflamed, iX, iY, "spriteflamed", ascreen, iScale, 1);
		} else {
			snprintf (sShowTile, MAX_TEXT, "imgdungeon[%i]", iTile);
			ShowImage (imgdungeon[iTile], iX, iY, sShowTile,
				ascreen, iScale, 1);
		}
	} else {
		if (iRoom == 0)
		{
			/*** no room above this room ***/
			iTile = 2;
		} else {
			iTile = arTiles[iRoom][iCol][iRowUse];
		}
		if ((iNoAnim == 0) && (iTile == 51))
		{
			ShowImage (spriteflamep, iX, iY, "spriteflamep", ascreen, iScale, 1);
		} else {
			snprintf (sShowTile, MAX_TEXT, "imgpalace[%i]", iTile);
			ShowImage (imgpalace[iTile], iX, iY, sShowTile,
				ascreen, iScale, 1);
		}
	}

	/*** selected ***/
	if ((iCol == iSelectedCol) && (iRow == iSelectedRow))
		{ ShowImage (imgselected, iX, iY, "imgselected", ascreen, iScale, 1); }

	/*** info ***/
	if (iInfo == 1)
	{
		snprintf (arText[0], MAX_TEXT, "%i", iTile);
		DisplayText (iX, iY, 11, arText, 1, font11, color_wh, color_bl, 0);
	}
}
/*****************************************************************************/
void Prev (void)
/*****************************************************************************/
{
	int iToLoad;

	if (iCurLevel != 1)
	{
		iToLoad = iCurLevel - 1;
		LoadLevel (iToLoad);
		ShowScreen (iScreen);
		PlaySound ("wav/level_change.wav");
	}
}
/*****************************************************************************/
void Next (void)
/*****************************************************************************/
{
	int iToLoad;

	if (iCurLevel != 14)
	{
		iToLoad = iCurLevel + 1;
		LoadLevel (iToLoad);
		ShowScreen (iScreen);
		PlaySound ("wav/level_change.wav");
	}
}
/*****************************************************************************/
int BrokenRoomLinks (int iPrint, int iLevel)
/*****************************************************************************/
{
	int iBroken;
	int iTemp;

	for (iTemp = 1; iTemp <= ROOMS; iTemp++)
	{
		iDone[iTemp] = 0;
		iRoomConnectionsBroken[iTemp][1] = 0;
		iRoomConnectionsBroken[iTemp][2] = 0;
		iRoomConnectionsBroken[iTemp][3] = 0;
		iRoomConnectionsBroken[iTemp][4] = 0;
	}
	CheckSides (arKidRoom[iCurLevel], 0, 0, iLevel);
	iBroken = 0;

	for (iTemp = 1; iTemp <= ROOMS; iTemp++)
	{
		/*** If the room is in use... ***/
		if (iDone[iTemp] == 1)
		{
			/*** check left ***/
			if (iRoomConnections[iLevel][iTemp][1] != 256)
			{
				if ((iRoomConnections[iLevel][iTemp][1] == iTemp) ||
					(iRoomConnections[iLevel]
					[iRoomConnections[iLevel][iTemp][1]][2] != iTemp))
				{
					iRoomConnectionsBroken[iTemp][1] = 1;
					iBroken = 1;
					if ((iDebug == 1) && (iPrint == 1))
					{
						printf ("[ INFO ] The left of room %i has a broken link.\n",
							iTemp);
					}
				}
			}
			/*** check right ***/
			if (iRoomConnections[iLevel][iTemp][2] != 256)
			{
				if ((iRoomConnections[iLevel][iTemp][2] == iTemp) ||
					(iRoomConnections[iLevel]
					[iRoomConnections[iLevel][iTemp][2]][1] != iTemp))
				{
					iRoomConnectionsBroken[iTemp][2] = 1;
					iBroken = 1;
					if ((iDebug == 1) && (iPrint == 1))
					{
						printf ("[ INFO ] The right of room %i has a broken link.\n",
							iTemp);
					}
				}
			}
			/*** check up ***/
			if (iRoomConnections[iLevel][iTemp][3] != 256)
			{
				if ((iRoomConnections[iLevel][iTemp][3] == iTemp) ||
					(iRoomConnections[iLevel]
					[iRoomConnections[iLevel][iTemp][3]][4] != iTemp))
				{
					iRoomConnectionsBroken[iTemp][3] = 1;
					iBroken = 1;
					if ((iDebug == 1) && (iPrint == 1))
					{
						printf ("[ INFO ] The top of room %i has a broken link.\n",
							iTemp);
					}
				}
			}
			/*** check down ***/
			if (iRoomConnections[iLevel][iTemp][4] != 256)
			{
				if ((iRoomConnections[iLevel][iTemp][4] == iTemp) ||
					(iRoomConnections[iLevel]
					[iRoomConnections[iLevel][iTemp][4]][3] != iTemp))
				{
					iRoomConnectionsBroken[iTemp][4] = 1;
					iBroken = 1;
					if ((iDebug == 1) && (iPrint == 1))
					{
						printf ("[ INFO ] The bottom of room %i has a broken link.\n",
							iTemp);
					}
				}
			}
		}
	}

	/*** if (iBroken == 1) { MapHide(); } ***/

	return (iBroken);
}
/*****************************************************************************/
void CheckSides (int iRoom, int iX, int iY, int iLevel)
/*****************************************************************************/
{
	if (iX < iMinX) { iMinX = iX; }
	if (iY < iMinY) { iMinY = iY; }
	if (iX > iMaxX) { iMaxX = iX; }
	if (iY > iMaxY) { iMaxY = iY; }

	iDone[iRoom] = 1;

	if ((iRoomConnections[iLevel][iRoom][1] != 256) &&
		(iDone[iRoomConnections[iLevel][iRoom][1]] != 1))
		{ CheckSides (iRoomConnections[iLevel][iRoom][1], iX - 1, iY, iLevel); }

	if ((iRoomConnections[iLevel][iRoom][2] != 256) &&
		(iDone[iRoomConnections[iLevel][iRoom][2]] != 1))
		{ CheckSides (iRoomConnections[iLevel][iRoom][2], iX + 1, iY, iLevel); }

	if ((iRoomConnections[iLevel][iRoom][3] != 256) &&
		(iDone[iRoomConnections[iLevel][iRoom][3]] != 1))
		{ CheckSides (iRoomConnections[iLevel][iRoom][3], iX, iY - 1, iLevel); }

	if ((iRoomConnections[iLevel][iRoom][4] != 256) &&
		(iDone[iRoomConnections[iLevel][iRoom][4]] != 1))
		{ CheckSides (iRoomConnections[iLevel][iRoom][4], iX, iY + 1, iLevel); }
}
/*****************************************************************************/
int OnLevelBar (void)
/*****************************************************************************/
{
	if (InArea (28, 3, 28 + 447, 3 + 19) == 1)
		{ return (1); } else { return (0); }
}
/*****************************************************************************/
void RunLevel (int iLevel)
/*****************************************************************************/
{
	SDL_Thread *princethread;

	if (iDebug == 1)
		{ printf ("[  OK  ] Starting the game in level %i.\n", iLevel); }

	ModifyForPlaytest (iLevel);
	princethread = SDL_CreateThread (StartGame, "StartGame", NULL);
	if (princethread == NULL)
		{ printf ("[ WARN ] Could not create thread!\n"); }
}
/*****************************************************************************/
int StartGame (void *unused)
/*****************************************************************************/
{
	char sSystem[200 + 2];
	char sSound[200 + 2];

	if (unused != NULL) { } /*** To prevent warnings. ***/

	PlaySound ("wav/playtest.wav");

	switch (iNoAudio)
	{
#if defined WIN32 || _WIN32 || WIN64 || _WIN64
		case 0: snprintf (sSound, 200, "%s", " -sound 1"); break;
#else
		case 0: snprintf (sSound, 200, "%s", " -sound 1 -sounddriver sdl"); break;
#endif
		case 1: snprintf (sSound, 200, "%s", " -sound 0"); break;
	}

	snprintf (sSystem, 200, "mednafen -md.videoip 0"
		" -md.stretch aspect -md.xscale 2 -md.yscale 2 %s %s > %s",
		sSound, sPathFile, DEVNULL);
	if (system (sSystem) == -1)
		{ printf ("[ WARN ] Could not execute mednafen!\n"); }
	if (iModified == 1) { ModifyBack(); }

	return (EXIT_NORMAL);
}
/*****************************************************************************/
void ModifyForPlaytest (int iLevel)
/*****************************************************************************/
{
	int iFd;
	char sToWrite[MAX_TOWRITE + 2];

	iFd = open (sPathFile, O_RDWR|O_BINARY);
	if (iFd == -1)
	{
		printf ("[ WARN ] Could not open \"%s\": %s!\n",
			sPathFile, strerror (errno));
	}

	/*** Store current password. ***/
	lseek (iFd, OFFSET_PASSWORD, SEEK_SET);
	ReadFromFile (iFd, "", 6, sOldPassword);

	/*** Change the password to the active in-editor level. ***/
	lseek (iFd, OFFSET_PASSWORD, SEEK_SET);
	snprintf (sToWrite, MAX_TOWRITE, "%s", arPasswords[iLevel]);
	write (iFd, sToWrite, 6);

	close (iFd);

	iModified = 1;
}
/*****************************************************************************/
void ModifyBack (void)
/*****************************************************************************/
{
	int iFd;

	iFd = open (sPathFile, O_RDWR|O_BINARY);
	if (iFd == -1)
	{
		printf ("[ WARN ] Could not open \"%s\": %s!\n",
			sPathFile, strerror (errno));
	}

	/*** Change the password back. ***/
	lseek (iFd, OFFSET_PASSWORD, SEEK_SET);
	write (iFd, sOldPassword, 6);

	close (iFd);

	iModified = 0;
}
/*****************************************************************************/
void WriteByte (int iFd, int iValue)
/*****************************************************************************/
{
	char sToWrite[MAX_TOWRITE + 2];

	snprintf (sToWrite, MAX_TOWRITE, "%c", iValue);
	write (iFd, sToWrite, 1);
}
/*****************************************************************************/
void WriteWord (int iFd, int iValue)
/*****************************************************************************/
{
	char sToWrite[MAX_TOWRITE + 2];

	snprintf (sToWrite, MAX_TOWRITE, "%c%c", (iValue >> 0) & 0xFF,
		(iValue >> 8) & 0xFF);
	write (iFd, sToWrite, 2);
}
/*****************************************************************************/
void GetAsEightBits (unsigned char cChar, char *sBinary)
/*****************************************************************************/
{
	int i = CHAR_BIT;
	int iTemp;

	iTemp = 0;
	while (i > 0)
	{
		i--;
		if (cChar&(1 << i))
		{
			sBinary[iTemp] = '1';
		} else {
			sBinary[iTemp] = '0';
		}
		iTemp++;
	}
	sBinary[iTemp] = '\0';
}
/*****************************************************************************/
void SaveFourAsThree (int iFd, int iRoom)
/*****************************************************************************/
{
	char sByte[8 + 2];

	/*** Fix tiles. ***/
	if ((arToSave[1][0] == '1') || (arToSave[1][1] == '1'))
	{
		snprintf (arToSave[1], 8 + 2, "%s", "00000000");
		if (iDebug == 1) { printf ("[ INFO ] Fixed a tile in room %i.\n", iRoom); }
	}
	if ((arToSave[2][0] == '1') || (arToSave[2][1] == '1'))
	{
		snprintf (arToSave[2], 8 + 2, "%s", "00000000");
		if (iDebug == 1) { printf ("[ INFO ] Fixed a tile in room %i.\n", iRoom); }
	}
	if ((arToSave[3][0] == '1') || (arToSave[3][1] == '1'))
	{
		snprintf (arToSave[3], 8 + 2, "%s", "00000000");
		if (iDebug == 1) { printf ("[ INFO ] Fixed a tile in room %i.\n", iRoom); }
	}
	if ((arToSave[4][0] == '1') || (arToSave[4][1] == '1'))
	{
		snprintf (arToSave[4], 8 + 2, "%s", "00000000");
		if (iDebug == 1) { printf ("[ INFO ] Fixed a tile in room %i.\n", iRoom); }
	}

	snprintf (sByte, 8 + 2, "%c%c%c%c%c%c%c%c",
		arToSave[2][6], arToSave[2][7], arToSave[1][2], arToSave[1][3],
		arToSave[1][4], arToSave[1][5], arToSave[1][6], arToSave[1][7]);
	WriteByte (iFd, BinToDec (sByte));
	snprintf (sByte, 8 + 2, "%c%c%c%c%c%c%c%c",
		arToSave[3][4], arToSave[3][5], arToSave[3][6], arToSave[3][7],
		arToSave[2][2], arToSave[2][3], arToSave[2][4], arToSave[2][5]);
	WriteByte (iFd, BinToDec (sByte));
	snprintf (sByte, 8 + 2, "%c%c%c%c%c%c%c%c",
		arToSave[4][2], arToSave[4][3], arToSave[4][4], arToSave[4][5],
		arToSave[4][6], arToSave[4][7], arToSave[3][2], arToSave[3][3]);
	WriteByte (iFd, BinToDec (sByte));
}
/*****************************************************************************/
void Zoom (int iToggleFull)
/*****************************************************************************/
{
	if (iToggleFull == 1)
	{
		if (iFullscreen == 0)
			{ iFullscreen = SDL_WINDOW_FULLSCREEN_DESKTOP; }
				else { iFullscreen = 0; }
	} else {
		if (iFullscreen == SDL_WINDOW_FULLSCREEN_DESKTOP)
		{
			iFullscreen = 0;
			iScale = 1;
		} else if (iScale == 1) {
			iScale = 2;
		} else if (iScale == 2) {
			iFullscreen = SDL_WINDOW_FULLSCREEN_DESKTOP;
		} else {
			printf ("[ WARN ] Unknown window state!\n");
		}
	}

	SDL_SetWindowFullscreen (window, iFullscreen);
	SDL_SetWindowSize (window, (WINDOW_WIDTH) * iScale,
		(WINDOW_HEIGHT) * iScale);
	SDL_RenderSetLogicalSize (ascreen, (WINDOW_WIDTH) * iScale,
		(WINDOW_HEIGHT) * iScale);
	SDL_SetWindowPosition (window, SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED);
	TTF_CloseFont (font11);
	TTF_CloseFont (font15);
	TTF_CloseFont (font20);
	LoadFonts();
}
/*****************************************************************************/
void ChangePosAction (char *sAction)
/*****************************************************************************/
{
	if (strcmp (sAction, "left") == 0)
	{
		iOnTileCol--;
		if (iOnTileCol == 0) { iOnTileCol = 15; }
	}

	if (strcmp (sAction, "right") == 0)
	{
		iOnTileCol++;
		if (iOnTileCol == 16) { iOnTileCol = 1; }
	}

	if (strcmp (sAction, "up") == 0)
	{
		iOnTileRow--;
		if (iOnTileRow == 0) { iOnTileRow = 13; }
	}

	if (strcmp (sAction, "down") == 0)
	{
		iOnTileRow++;
		if (iOnTileRow == 14) { iOnTileRow = 1; }
	}
}
/*****************************************************************************/
void ChangePos (int iLocationCol, int iLocationRow)
/*****************************************************************************/
{
	int iChanging;
	SDL_Event event;
	int iXPosOld, iYPosOld;
	int iUseTile;

	/*** Used for looping. ***/
	int iLoopCol;
	int iLoopRow;
	int iLoopRoom;

	iChanging = 1;

	ShowChange();
	while (iChanging == 1)
	{
		while (SDL_PollEvent (&event))
		{
			if (MapEvents (event) == 0)
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
							if (UseTile (iLocationCol, iLocationRow, iCurRoom) == 1)
							{
								iChanging = 0;
								iChanged++;
							}
							break;
						case SDL_CONTROLLER_BUTTON_B:
							iChanging = 0; break;
						case SDL_CONTROLLER_BUTTON_X:
							/*** Nothing for now. ***/ break;
						case SDL_CONTROLLER_BUTTON_Y:
							/*** Nothing for now. ***/ break;
						case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
							ChangePosAction ("left"); break;
						case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
							ChangePosAction ("right"); break;
						case SDL_CONTROLLER_BUTTON_DPAD_UP:
							ChangePosAction ("up"); break;
						case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
							ChangePosAction ("down"); break;
						case SDL_CONTROLLER_BUTTON_GUIDE:
							/*** Nothing for now. ***/ break;
					}
					ShowChange();
					break;
				case SDL_CONTROLLERAXISMOTION: /*** triggers and analog sticks ***/
					/*** Nothing for now. ***/
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						case SDLK_SPACE:
							if ((event.key.keysym.mod & KMOD_LCTRL) ||
								(event.key.keysym.mod & KMOD_RCTRL))
							{
								for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
								{
									for (iLoopCol = 1; iLoopCol <= COLS; iLoopCol++)
									{
										for (iLoopRow = 1; iLoopRow <= ROWS; iLoopRow++)
										{
											iUseTile = UseTile (iLoopCol, iLoopRow, iLoopRoom);
										}
									}
								}
								if (iUseTile == 1)
								{
									iChanging = 0;
									iChanged++;
								}
							} else if ((event.key.keysym.mod & KMOD_LSHIFT) ||
								(event.key.keysym.mod & KMOD_RSHIFT))
							{
								for (iLoopCol = 1; iLoopCol <= COLS; iLoopCol++)
								{
									for (iLoopRow = 1; iLoopRow <= ROWS; iLoopRow++)
									{
										iUseTile = UseTile (iLoopCol, iLoopRow, iCurRoom);
									}
								}
								if (iUseTile == 1)
								{
									iChanging = 0;
									iChanged++;
								}
							} else {
								if (UseTile (iLocationCol, iLocationRow, iCurRoom) == 1)
								{
									iChanging = 0;
									iChanged++;
								}
							}
							break;
						case SDLK_ESCAPE:
						case SDLK_q:
						case SDLK_c:
							iChanging = 0; break;
						case SDLK_LEFT:
							ChangePosAction ("left"); break;
						case SDLK_RIGHT:
							ChangePosAction ("right"); break;
						case SDLK_UP:
							ChangePosAction ("up"); break;
						case SDLK_DOWN:
							ChangePosAction ("down"); break;
					}
					ShowChange();
					break;
				case SDL_MOUSEMOTION:
					iXPosOld = iXPos;
					iYPosOld = iYPos;
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					if ((iXPosOld == iXPos) && (iYPosOld == iYPos)) { break; }

					/*** hover ***/
					for (iLoopCol = 1; iLoopCol <= 15; iLoopCol++)
					{
						for (iLoopRow = 1; iLoopRow <= 13; iLoopRow++)
						{
							if (InArea ((34 * iLoopCol) - 26,
								(34 * iLoopRow) - 26,
								(34 * iLoopCol) - 26 + 34,
								(34 * iLoopRow) - 26 + 34) == 1)
								{ iOnTileCol = iLoopCol; iOnTileRow = iLoopRow; }
						}
					}

					ShowChange();
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						if (InArea (526, 0, 526 + 36, 0 + 459) == 1) /*** close ***/
							{ iCloseOn = 1; }
					}
					ShowChange();
					break;
				case SDL_MOUSEBUTTONUP:
					iCloseOn = 0;

					if (event.button.button == 1)
					{
						if (InArea (526, 0, 526 + 36, 0 + 459) == 1) /*** close ***/
							{ iChanging = 0; }

						if (InArea (7, 7, 7 + 512, 7 + 444) == 1)
						{
							if (UseTile (iLocationCol, iLocationRow, iCurRoom) == 1)
							{
								iChanging = 0;
								iChanged++;
							}
						}
					}

					if (event.button.button == 2)
					{
						if (InArea (7, 7, 7 + 512, 7 + 444) == 1)
						{
							for (iLoopCol = 1; iLoopCol <= COLS; iLoopCol++)
							{
								for (iLoopRow = 1; iLoopRow <= ROWS; iLoopRow++)
								{
									iUseTile = UseTile (iLoopCol, iLoopRow, iCurRoom);
								}
							}
							if (iUseTile == 1)
							{
								iChanging = 0;
								iChanged++;
							}
						}
					}

					if (event.button.button == 3)
					{
						if (InArea (7, 7, 7 + 512, 7 + 444) == 1)
						{
							for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
							{
								for (iLoopCol = 1; iLoopCol <= COLS; iLoopCol++)
								{
									for (iLoopRow = 1; iLoopRow <= ROWS; iLoopRow++)
									{
										iUseTile = UseTile (iLoopCol, iLoopRow, iLoopRoom);
									}
								}
							}
							if (iUseTile == 1)
							{
								iChanging = 0;
								iChanged++;
							}
						}
					}

					ShowChange();
					break;
				case SDL_WINDOWEVENT:
					switch (event.window.event)
					{
						case SDL_WINDOWEVENT_EXPOSED: ShowChange(); break;
						case SDL_WINDOWEVENT_CLOSE: Quit(); break;
						case SDL_WINDOWEVENT_FOCUS_GAINED:
							iActiveWindowID = iWindowID; break;
					}
					break;
				case SDL_QUIT: Quit(); break;
			}
		}

		/*** prevent CPU eating ***/
		gamespeed = REFRESH;
		while ((SDL_GetTicks() - looptime) < gamespeed)
		{
			SDL_Delay (10);
		}
		looptime = SDL_GetTicks();
	}
	PlaySound ("wav/ok_close.wav");
}
/*****************************************************************************/
void ShowChange (void)
/*****************************************************************************/
{
	int iHoverX, iHoverY;

	if (cCurType == 'd')
	{
		ShowImage (imgdungeont, 0, 0, "imgdungeont", ascreen, iScale, 1);
	} else {
		ShowImage (imgpalacet, 0, 0, "imgpalacet", ascreen, iScale, 1);
	}

	/*** close ***/
	if (iCloseOn == 0)
	{ /*** off ***/
		ShowImage (imgclosebig_0, 526, 0, "imgclosebig_0", ascreen, iScale, 1);
	} else { /*** on ***/
		ShowImage (imgclosebig_1, 526, 0, "imgclosebig_1", ascreen, iScale, 1);
	}

	/*** old tile ***/
	OnTileOld();
	iHoverX = 7 + (34 * (iOnTileColOld - 1));
	iHoverY = 7 + (34 * (iOnTileRowOld - 1));
	ShowImage (imgborderl, iHoverX, iHoverY, "imgborderl",
		ascreen, iScale, 1);

	/*** hover ***/
	iHoverX = 7 + (34 * (iOnTileCol - 1));
	iHoverY = 7 + (34 * (iOnTileRow - 1));
	ShowImage (imgborder, iHoverX, iHoverY, "imgborder",
		ascreen, iScale, 1);

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void OnTileOld (void)
/*****************************************************************************/
{
	if (cCurType == 'd')
	{
		switch (arTiles[iCurRoom][iSelectedCol][iSelectedRow])
		{
			case 0: iOnTileColOld = 1; iOnTileRowOld = 1; break;
			case 1: iOnTileColOld = 12; iOnTileRowOld = 7; break;
			case 2: iOnTileColOld = 8; iOnTileRowOld = 6; break;
			case 3: iOnTileColOld = 12; iOnTileRowOld = 6; break;
			case 4: iOnTileColOld = 8; iOnTileRowOld = 8; break;
			case 5: iOnTileColOld = 9; iOnTileRowOld = 5; break;
			case 6: iOnTileColOld = 11; iOnTileRowOld = 5; break;
			case 7: iOnTileColOld = 10; iOnTileRowOld = 5; break;
			case 8: iOnTileColOld = 12; iOnTileRowOld = 5; break;
			case 9: iOnTileColOld = 14; iOnTileRowOld = 5; break;
			case 10: iOnTileColOld = 13; iOnTileRowOld = 7; break;
			case 11: iOnTileColOld = 13; iOnTileRowOld = 6; break;
			case 12: iOnTileColOld = 15; iOnTileRowOld = 8; break;
			case 13: iOnTileColOld = 14; iOnTileRowOld = 8; break;
			case 14: iOnTileColOld = 11; iOnTileRowOld = 13; break;
			case 15: iOnTileColOld = 7; iOnTileRowOld = 2; break;
			/*** 16 ***/
			case 17: iOnTileColOld = 2; iOnTileRowOld = 4; break;
			/*** 18, 19 ***/
			case 20: iOnTileColOld = 10; iOnTileRowOld = 6; break;
			case 21: iOnTileColOld = 8; iOnTileRowOld = 7; break;
			case 22: iOnTileColOld = 7; iOnTileRowOld = 1; break;
			case 23: iOnTileColOld = 8; iOnTileRowOld = 1; break;
			case 24: iOnTileColOld = 13; iOnTileRowOld = 11; break;
			case 25: iOnTileColOld = 14; iOnTileRowOld = 11; break;
			case 26: iOnTileColOld = 15; iOnTileRowOld = 11; break;
			case 27: iOnTileColOld = 5; iOnTileRowOld = 8; break;
			case 28: iOnTileColOld = 15; iOnTileRowOld = 5; break;
			case 29: iOnTileColOld = 1; iOnTileRowOld = 7; break;
			case 30: iOnTileColOld = 2; iOnTileRowOld = 7; break;
			case 31: iOnTileColOld = 1; iOnTileRowOld = 9; break;
			case 32: iOnTileColOld = 4; iOnTileRowOld = 8; break;
			case 33: iOnTileColOld = 6; iOnTileRowOld = 8; break;
			case 34: iOnTileColOld = 3; iOnTileRowOld = 8; break;
			case 35: iOnTileColOld = 13; iOnTileRowOld = 12; break;
			case 36: iOnTileColOld = 14; iOnTileRowOld = 12; break;
			case 37: iOnTileColOld = 15; iOnTileRowOld = 12; break;
			/*** 38, 39 ***/
			case 40: iOnTileColOld = 10; iOnTileRowOld = 7; break;
			case 41: iOnTileColOld = 13; iOnTileRowOld = 13; break;
			case 42: iOnTileColOld = 14; iOnTileRowOld = 13; break;
			case 43: iOnTileColOld = 15; iOnTileRowOld = 13; break;
			case 44: iOnTileColOld = 1; iOnTileRowOld = 6; break;
			case 45: iOnTileColOld = 2; iOnTileRowOld = 6; break;
			case 46: iOnTileColOld = 1; iOnTileRowOld = 12; break;
			/*** 47, 48 ***/
			case 49: iOnTileColOld = 2; iOnTileRowOld = 12; break;
			case 50: iOnTileColOld = 5; iOnTileRowOld = 12; break;
			case 51: iOnTileColOld = 6; iOnTileRowOld = 12; break;
			case 52: iOnTileColOld = 3; iOnTileRowOld = 12; break;
			/*** 53, 54 ***/
			case 55: iOnTileColOld = 1; iOnTileRowOld = 13; break;
			case 56: iOnTileColOld = 2; iOnTileRowOld = 13; break;
			case 57: iOnTileColOld = 3; iOnTileRowOld = 13; break;
			case 58: iOnTileColOld = 3; iOnTileRowOld = 4; break;
			/*** 59-63 ***/
			default: iOnTileColOld = 1; iOnTileRowOld = 1; break;
		}
	} else {
		switch (arTiles[iCurRoom][iSelectedCol][iSelectedRow])
		{
			case 0: iOnTileColOld = 1; iOnTileRowOld = 1; break;
			case 1: iOnTileColOld = 1; iOnTileRowOld = 13; break;
			case 2: iOnTileColOld = 8; iOnTileRowOld = 7; break;
			case 3: iOnTileColOld = 12; iOnTileRowOld = 6; break;
			case 4: iOnTileColOld = 8; iOnTileRowOld = 6; break;
			case 5: iOnTileColOld = 9; iOnTileRowOld = 5; break;
			case 6: iOnTileColOld = 10; iOnTileRowOld = 5; break;
			case 7: iOnTileColOld = 11; iOnTileRowOld = 5; break;
			case 8: iOnTileColOld = 12; iOnTileRowOld = 5; break;
			case 9: iOnTileColOld = 14; iOnTileRowOld = 5; break;
			case 10: iOnTileColOld = 13; iOnTileRowOld = 6; break;
			case 11: iOnTileColOld = 13; iOnTileRowOld = 7; break;
			case 12: iOnTileColOld = 15; iOnTileRowOld = 8; break;
			case 13: iOnTileColOld = 14; iOnTileRowOld = 8; break;
			case 14: iOnTileColOld = 10; iOnTileRowOld = 8; break;
			case 15: iOnTileColOld = 8; iOnTileRowOld = 8; break;
			case 16: iOnTileColOld = 1; iOnTileRowOld = 12; break;
			case 17: iOnTileColOld = 2; iOnTileRowOld = 4; break;
			case 18: iOnTileColOld = 7; iOnTileRowOld = 1; break;
			case 19: iOnTileColOld = 8; iOnTileRowOld = 1; break;
			case 20: iOnTileColOld = 1; iOnTileRowOld = 6; break;
			case 21: iOnTileColOld = 2; iOnTileRowOld = 6; break;
			case 22: iOnTileColOld = 1; iOnTileRowOld = 7; break;
			case 23: iOnTileColOld = 2; iOnTileRowOld = 7; break;
			case 24: iOnTileColOld = 5; iOnTileRowOld = 4; break;
			case 25: iOnTileColOld = 6; iOnTileRowOld = 6; break;
			case 26: iOnTileColOld = 5; iOnTileRowOld = 6; break;
			case 27: iOnTileColOld = 1; iOnTileRowOld = 9; break;
			case 28: iOnTileColOld = 4; iOnTileRowOld = 8; break;
			case 29: iOnTileColOld = 6; iOnTileRowOld = 8; break;
			case 30: iOnTileColOld = 3; iOnTileRowOld = 8; break;
			case 31: iOnTileColOld = 13; iOnTileRowOld = 12; break;
			case 32: iOnTileColOld = 14; iOnTileRowOld = 12; break;
			case 33: iOnTileColOld = 15; iOnTileRowOld = 12; break;
			case 34: iOnTileColOld = 13; iOnTileRowOld = 13; break;
			case 35: iOnTileColOld = 14; iOnTileRowOld = 13; break;
			case 36: iOnTileColOld = 15; iOnTileRowOld = 13; break;
			case 37: iOnTileColOld = 13; iOnTileRowOld = 11; break;
			case 38: iOnTileColOld = 14; iOnTileRowOld = 11; break;
			case 39: iOnTileColOld = 15; iOnTileRowOld = 11; break;
			case 40: iOnTileColOld = 15; iOnTileRowOld = 5; break;
			case 41: iOnTileColOld = 5; iOnTileRowOld = 8; break;
			case 42: iOnTileColOld = 12; iOnTileRowOld = 1; break;
			case 43: iOnTileColOld = 13; iOnTileRowOld = 1; break;
			case 44: iOnTileColOld = 14; iOnTileRowOld = 1; break;
			case 45: iOnTileColOld = 11; iOnTileRowOld = 1; break;
			case 46: iOnTileColOld = 12; iOnTileRowOld = 2; break;
			case 47: iOnTileColOld = 13; iOnTileRowOld = 2; break;
			case 48: iOnTileColOld = 15; iOnTileRowOld = 1; break;
			case 49: iOnTileColOld = 10; iOnTileRowOld = 1; break;
			case 50: iOnTileColOld = 2; iOnTileRowOld = 13; break;
			case 51: iOnTileColOld = 2; iOnTileRowOld = 12; break;
			case 52: iOnTileColOld = 5; iOnTileRowOld = 12; break;
			case 53: iOnTileColOld = 6; iOnTileRowOld = 12; break;
			case 54: iOnTileColOld = 8; iOnTileRowOld = 11; break;
			case 55: iOnTileColOld = 9; iOnTileRowOld = 11; break;
			case 56: iOnTileColOld = 8; iOnTileRowOld = 12; break;
			case 57: iOnTileColOld = 9; iOnTileRowOld = 12; break;
			case 58: iOnTileColOld = 8; iOnTileRowOld = 13; break;
			case 59: iOnTileColOld = 9; iOnTileRowOld = 13; break;
			case 60: iOnTileColOld = 1; iOnTileRowOld = 10; break;
			case 61: iOnTileColOld = 5; iOnTileRowOld = 3; break;
			case 62: iOnTileColOld = 11; iOnTileRowOld = 13; break;
			/*** 63 ***/
			default: iOnTileColOld = 1; iOnTileRowOld = 1; break;
		}
	}
}
/*****************************************************************************/
int UseTile (int iLocationCol, int iLocationRow, int iRoom)
/*****************************************************************************/
{
	int iUseTile;

	iUseTile = -1;

	if (cCurType == 'd')
	{
		if ((iOnTileCol == 1) && (iOnTileRow == 1)) { iUseTile = 0; }
		if ((iOnTileCol == 12) && (iOnTileRow == 7)) { iUseTile = 1; }
		if ((iOnTileCol == 8) && (iOnTileRow == 6)) { iUseTile = 2; }
		if ((iOnTileCol == 12) && (iOnTileRow == 6)) { iUseTile = 3; }
		if ((iOnTileCol == 8) && (iOnTileRow == 8)) { iUseTile = 4; }
		if ((iOnTileCol == 9) && (iOnTileRow == 5)) { iUseTile = 5; }
		if ((iOnTileCol == 11) && (iOnTileRow == 5)) { iUseTile = 6; }
		if ((iOnTileCol == 10) && (iOnTileRow == 5)) { iUseTile = 7; }
		if ((iOnTileCol == 12) && (iOnTileRow == 5)) { iUseTile = 8; }
		if ((iOnTileCol == 14) && (iOnTileRow == 5)) { iUseTile = 9; }
		if (( iOnTileCol == 13) && (iOnTileRow == 7)) { iUseTile = 10; }
		if (( iOnTileCol == 13) && (iOnTileRow == 6)) { iUseTile = 11; }
		if (( iOnTileCol == 15) && (iOnTileRow == 8)) { iUseTile = 12; }
		if (( iOnTileCol == 14) && (iOnTileRow == 8)) { iUseTile = 13; }
		if (( iOnTileCol == 11) && (iOnTileRow == 13)) { iUseTile = 14; }
		if (( iOnTileCol == 7) && (iOnTileRow == 2)) { iUseTile = 15; }
		/*** 16 ***/
		if ((iOnTileCol == 2) && (iOnTileRow == 4)) { iUseTile = 17; }
		/*** 18, 19 ***/
		if ((iOnTileCol == 10) && (iOnTileRow == 6)) { iUseTile = 20; }
		if ((iOnTileCol == 8) && (iOnTileRow == 7)) { iUseTile = 21; }
		if ((iOnTileCol == 7) && (iOnTileRow == 1)) { iUseTile = 22; }
		if ((iOnTileCol == 8) && (iOnTileRow == 1)) { iUseTile = 23; }
		if ((iOnTileCol == 13) && (iOnTileRow == 11)) { iUseTile = 24; }
		if ((iOnTileCol == 14) && (iOnTileRow == 11)) { iUseTile = 25; }
		if ((iOnTileCol == 15) && (iOnTileRow == 11)) { iUseTile = 26; }
		if ((iOnTileCol == 5) && (iOnTileRow == 8)) { iUseTile = 27; }
		if ((iOnTileCol == 15) && (iOnTileRow == 5)) { iUseTile = 28; }
		if ((iOnTileCol == 1) && (iOnTileRow == 7)) { iUseTile = 29; }
		if ((iOnTileCol == 2) && (iOnTileRow == 7)) { iUseTile = 30; }
		if ((iOnTileCol == 1) && (iOnTileRow == 9)) { iUseTile = 31; }
		if ((iOnTileCol == 4) && (iOnTileRow == 8)) { iUseTile = 32; }
		if ((iOnTileCol == 6) && (iOnTileRow == 8)) { iUseTile = 33; }
		if ((iOnTileCol == 3) && (iOnTileRow == 8)) { iUseTile = 34; }
		if ((iOnTileCol == 13) && (iOnTileRow == 12)) { iUseTile = 35; }
		if ((iOnTileCol == 14) && (iOnTileRow == 12)) { iUseTile = 36; }
		if ((iOnTileCol == 15) && (iOnTileRow == 12)) { iUseTile = 37; }
		/*** 38, 39 ***/
		if ((iOnTileCol == 10) && (iOnTileRow == 7)) { iUseTile = 40; }
		if ((iOnTileCol == 13) && (iOnTileRow == 13)) { iUseTile = 41; }
		if ((iOnTileCol == 14) && (iOnTileRow == 13)) { iUseTile = 42; }
		if ((iOnTileCol == 15) && (iOnTileRow == 13)) { iUseTile = 43; }
		if ((iOnTileCol == 1) && (iOnTileRow == 6)) { iUseTile = 44; }
		if ((iOnTileCol == 2) && (iOnTileRow == 6)) { iUseTile = 45; }
		if ((iOnTileCol == 1) && (iOnTileRow == 12)) { iUseTile = 46; }
		/*** 47, 48 ***/
		if ((iOnTileCol == 2) && (iOnTileRow == 12)) { iUseTile = 49; }
		if ((iOnTileCol == 5) && (iOnTileRow == 12)) { iUseTile = 50; }
		if ((iOnTileCol == 6) && (iOnTileRow == 12)) { iUseTile = 51; }
		if ((iOnTileCol == 3) && (iOnTileRow == 12)) { iUseTile = 52; }
		/*** 53, 54 ***/
		if ((iOnTileCol == 1) && (iOnTileRow == 13)) { iUseTile = 55; }
		if ((iOnTileCol == 2) && (iOnTileRow == 13)) { iUseTile = 56; }
		if ((iOnTileCol == 3) && (iOnTileRow == 13)) { iUseTile = 57; }
		if ((iOnTileCol == 3) && (iOnTileRow == 4)) { iUseTile = 58; }
		/*** 59-63 ***/
	} else {
		if ((iOnTileCol == 1) && (iOnTileRow == 1)) { iUseTile = 0; }
		if ((iOnTileCol == 1) && (iOnTileRow == 13)) { iUseTile = 1; }
		if ((iOnTileCol == 8) && (iOnTileRow == 7)) { iUseTile = 2; }
		if ((iOnTileCol == 12) && (iOnTileRow == 6)) { iUseTile = 3; }
		if ((iOnTileCol == 8) && (iOnTileRow == 6)) { iUseTile = 4; }
		if ((iOnTileCol == 9) && (iOnTileRow == 5)) { iUseTile = 5; }
		if ((iOnTileCol == 10) && (iOnTileRow == 5)) { iUseTile = 6; }
		if ((iOnTileCol == 11) && (iOnTileRow == 5)) { iUseTile = 7; }
		if ((iOnTileCol == 12) && (iOnTileRow == 5)) { iUseTile = 8; }
		if ((iOnTileCol == 14) && (iOnTileRow == 5)) { iUseTile = 9; }
		if ((iOnTileCol == 13) && (iOnTileRow == 6)) { iUseTile = 10; }
		if ((iOnTileCol == 13) && (iOnTileRow == 7)) { iUseTile = 11; }
		if ((iOnTileCol == 15) && (iOnTileRow == 8)) { iUseTile = 12; }
		if ((iOnTileCol == 14) && (iOnTileRow == 8)) { iUseTile = 13; }
		if ((iOnTileCol == 10) && (iOnTileRow == 8)) { iUseTile = 14; }
		if ((iOnTileCol == 8) && (iOnTileRow == 8)) { iUseTile = 15; }
		if ((iOnTileCol == 1) && (iOnTileRow == 12)) { iUseTile = 16; }
		if ((iOnTileCol == 2) && (iOnTileRow == 4)) { iUseTile = 17; }
		if ((iOnTileCol == 7) && (iOnTileRow == 1)) { iUseTile = 18; }
		if ((iOnTileCol == 8) && (iOnTileRow == 1)) { iUseTile = 19; }
		if ((iOnTileCol == 1) && (iOnTileRow == 6)) { iUseTile = 20; }
		if ((iOnTileCol == 2) && (iOnTileRow == 6)) { iUseTile = 21; }
		if ((iOnTileCol == 1) && (iOnTileRow == 7)) { iUseTile = 22; }
		if ((iOnTileCol == 2) && (iOnTileRow == 7)) { iUseTile = 23; }
		if ((iOnTileCol == 5) && (iOnTileRow == 4)) { iUseTile = 24; }
		if ((iOnTileCol == 6) && (iOnTileRow == 6)) { iUseTile = 25; }
		if ((iOnTileCol == 5) && (iOnTileRow == 6)) { iUseTile = 26; }
		if ((iOnTileCol == 1) && (iOnTileRow == 9)) { iUseTile = 27; }
		if ((iOnTileCol == 4) && (iOnTileRow == 8)) { iUseTile = 28; }
		if ((iOnTileCol == 6) && (iOnTileRow == 8)) { iUseTile = 29; }
		if ((iOnTileCol == 3) && (iOnTileRow == 8)) { iUseTile = 30; }
		if ((iOnTileCol == 13) && (iOnTileRow == 12)) { iUseTile = 31; }
		if ((iOnTileCol == 14) && (iOnTileRow == 12)) { iUseTile = 32; }
		if ((iOnTileCol == 15) && (iOnTileRow == 12)) { iUseTile = 33; }
		if ((iOnTileCol == 13) && (iOnTileRow == 13)) { iUseTile = 34; }
		if ((iOnTileCol == 14) && (iOnTileRow == 13)) { iUseTile = 35; }
		if ((iOnTileCol == 15) && (iOnTileRow == 13)) { iUseTile = 36; }
		if ((iOnTileCol == 13) && (iOnTileRow == 11)) { iUseTile = 37; }
		if ((iOnTileCol == 14) && (iOnTileRow == 11)) { iUseTile = 38; }
		if ((iOnTileCol == 15) && (iOnTileRow == 11)) { iUseTile = 39; }
		if ((iOnTileCol == 15) && (iOnTileRow == 5)) { iUseTile = 40; }
		if ((iOnTileCol == 5) && (iOnTileRow == 8)) { iUseTile = 41; }
		if ((iOnTileCol == 12) && (iOnTileRow == 1)) { iUseTile = 42; }
		if ((iOnTileCol == 13) && (iOnTileRow == 1)) { iUseTile = 43; }
		if ((iOnTileCol == 14) && (iOnTileRow == 1)) { iUseTile = 44; }
		if ((iOnTileCol == 11) && (iOnTileRow == 1)) { iUseTile = 45; }
		if ((iOnTileCol == 12) && (iOnTileRow == 2)) { iUseTile = 46; }
		if ((iOnTileCol == 13) && (iOnTileRow == 2)) { iUseTile = 47; }
		if ((iOnTileCol == 15) && (iOnTileRow == 1)) { iUseTile = 48; }
		if ((iOnTileCol == 10) && (iOnTileRow == 1)) { iUseTile = 49; }
		if ((iOnTileCol == 2) && (iOnTileRow == 13)) { iUseTile = 50; }
		if ((iOnTileCol == 2) && (iOnTileRow == 12)) { iUseTile = 51; }
		if ((iOnTileCol == 5) && (iOnTileRow == 12)) { iUseTile = 52; }
		if ((iOnTileCol == 6) && (iOnTileRow == 12)) { iUseTile = 53; }
		if ((iOnTileCol == 8) && (iOnTileRow == 11)) { iUseTile = 54; }
		if ((iOnTileCol == 9) && (iOnTileRow == 11)) { iUseTile = 55; }
		if ((iOnTileCol == 8) && (iOnTileRow == 12)) { iUseTile = 56; }
		if ((iOnTileCol == 9) && (iOnTileRow == 12)) { iUseTile = 57; }
		if ((iOnTileCol == 8) && (iOnTileRow == 13)) { iUseTile = 58; }
		if ((iOnTileCol == 9) && (iOnTileRow == 13)) { iUseTile = 59; }
		if ((iOnTileCol == 1) && (iOnTileRow == 10)) { iUseTile = 60; }
		if ((iOnTileCol == 5) && (iOnTileRow == 3)) { iUseTile = 61; }
		if ((iOnTileCol == 11) && (iOnTileRow == 13)) { iUseTile = 62; }
		/*** 63 ***/
	}

	if (iUseTile != -1)
	{
		SetLocation (iRoom, iLocationCol, iLocationRow, iUseTile);
		return (1);
	} else { return (0); }
}
/*****************************************************************************/
void SetLocation (int iRoom, int iLocationCol, int iLocationRow, int iUseTile)
/*****************************************************************************/
{
	arTiles[iRoom][iLocationCol][iLocationRow] = iUseTile;

	iLastTile = iUseTile;
}
/*****************************************************************************/
void InitRooms (void)
/*****************************************************************************/
{
	/*** Used for looping. ***/
	int iLoopX;
	int iLoopY;

	for (iLoopX = 0; iLoopX <= ROOMS + 1; iLoopX++)
	{
		for (iLoopY = 0; iLoopY <= ROOMS; iLoopY++)
		{
			iRoomArray[iLoopX][iLoopY] = 0;
		}
	}
}
/*****************************************************************************/
void WhereToStart (void)
/*****************************************************************************/
{
	int iLoopRoom;

	iMinX = 0;
	iMaxX = 0;
	iMinY = 0;
	iMaxY = 0;

	for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
	{
		iDone[iLoopRoom] = 0;
	}
	CheckSides (arKidRoom[iCurLevel], 0, 0, iCurLevel);

	iStartRoomsX = round (12 - (((float)iMinX + (float)iMaxX) / 2));
	iStartRoomsY = round (12 - (((float)iMinY + (float)iMaxY) / 2));
}
/*****************************************************************************/
void ShowRooms (int iRoom, int iX, int iY, int iNext)
/*****************************************************************************/
{
	int iXShow, iYShow;
	char sShowRoom[MAX_TEXT + 2];

	if (iX == 25)
	{
		iXShow = 143;
	} else {
		iXShow = 151 + (iX * 15);
	}
	iYShow = 48 + (iY * 15);

	if (iRoom != -1)
	{
		snprintf (sShowRoom, MAX_TEXT, "imgroom[%i]", iRoom);
		ShowImage (imgroom[iRoom], iXShow, iYShow, sShowRoom,
			ascreen, iScale, 1);
		iRoomArray[iX][iY] = iRoom;
		if (iCurRoom == iRoom) /*** green stripes ***/
			{ ShowImage (imgsrc, iXShow, iYShow, "imgsrc", ascreen, iScale, 1); }
		if (arKidRoom[iCurLevel] == iRoom) /*** blue border ***/
			{ ShowImage (imgsrs, iXShow, iYShow, "imgsrs", ascreen, iScale, 1); }
		if (iRoom == iMovingRoom) /*** red stripes ***/
			{ ShowImage (imgsrm, iXShow, iYShow, "imgsrm", ascreen, iScale, 1); }
	} else {
		/*** white cross ***/
		ShowImage (imgsrp, iXShow, iYShow, "imgsrp", ascreen, iScale, 1);
	}
	if (iRoom == iMovingRoom)
	{
		iMovingOldX = iX;
		iMovingOldY = iY;
		if (iMovingNewBusy == 0)
		{
			iMovingNewX = iMovingOldX;
			iMovingNewY = iMovingOldY;
			iMovingNewBusy = 1;
		}
	}

	if (iRoom != -1) { iDone[iRoom] = 1; }

	if (iNext == 1)
	{
		if ((iRoomConnections[iCurLevel][iRoom][1] != 256) &&
			(iDone[iRoomConnections[iCurLevel][iRoom][1]] != 1))
			{ ShowRooms (iRoomConnections[iCurLevel][iRoom][1], iX - 1, iY, 1); }

		if ((iRoomConnections[iCurLevel][iRoom][2] != 256) &&
			(iDone[iRoomConnections[iCurLevel][iRoom][2]] != 1))
			{ ShowRooms (iRoomConnections[iCurLevel][iRoom][2], iX + 1, iY, 1); }

		if ((iRoomConnections[iCurLevel][iRoom][3] != 256) &&
			(iDone[iRoomConnections[iCurLevel][iRoom][3]] != 1))
			{ ShowRooms (iRoomConnections[iCurLevel][iRoom][3], iX, iY - 1, 1); }

		if ((iRoomConnections[iCurLevel][iRoom][4] != 256) &&
			(iDone[iRoomConnections[iCurLevel][iRoom][4]] != 1))
			{ ShowRooms (iRoomConnections[iCurLevel][iRoom][4], iX, iY + 1, 1); }
	}
}
/*****************************************************************************/
int MouseSelectAdj (void)
/*****************************************************************************/
{
	int iOnAdj;
	int iAdjBaseX;
	int iAdjBaseY;

	/*** Used for looping. ***/
	int iLoopRoom;

	iOnAdj = 0;

	for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
	{
		switch (iLoopRoom)
		{
			case 1: iAdjBaseX = ADJ_BASE_X + (63 * 0);
				iAdjBaseY = ADJ_BASE_Y + (63 * 0); break;
			case 2: iAdjBaseX = ADJ_BASE_X + (63 * 1);
				iAdjBaseY = ADJ_BASE_Y + (63 * 0); break;
			case 3: iAdjBaseX = ADJ_BASE_X + (63 * 2);
				iAdjBaseY = ADJ_BASE_Y + (63 * 0); break;
			case 4: iAdjBaseX = ADJ_BASE_X + (63 * 3);
				iAdjBaseY = ADJ_BASE_Y + (63 * 0); break;
			case 5: iAdjBaseX = ADJ_BASE_X + (63 * 0);
				iAdjBaseY = ADJ_BASE_Y + (63 * 1); break;
			case 6: iAdjBaseX = ADJ_BASE_X + (63 * 1);
				iAdjBaseY = ADJ_BASE_Y + (63 * 1); break;
			case 7: iAdjBaseX = ADJ_BASE_X + (63 * 2);
				iAdjBaseY = ADJ_BASE_Y + (63 * 1); break;
			case 8: iAdjBaseX = ADJ_BASE_X + (63 * 3);
				iAdjBaseY = ADJ_BASE_Y + (63 * 1); break;
			case 9: iAdjBaseX = ADJ_BASE_X + (63 * 0);
				iAdjBaseY = ADJ_BASE_Y + (63 * 2); break;
			case 10: iAdjBaseX = ADJ_BASE_X + (63 * 1);
				iAdjBaseY = ADJ_BASE_Y + (63 * 2); break;
			case 11: iAdjBaseX = ADJ_BASE_X + (63 * 2);
				iAdjBaseY = ADJ_BASE_Y + (63 * 2); break;
			case 12: iAdjBaseX = ADJ_BASE_X + (63 * 3);
				iAdjBaseY = ADJ_BASE_Y + (63 * 2); break;
			case 13: iAdjBaseX = ADJ_BASE_X + (63 * 0);
				iAdjBaseY = ADJ_BASE_Y + (63 * 3); break;
			case 14: iAdjBaseX = ADJ_BASE_X + (63 * 1);
				iAdjBaseY = ADJ_BASE_Y + (63 * 3); break;
			case 15: iAdjBaseX = ADJ_BASE_X + (63 * 2);
				iAdjBaseY = ADJ_BASE_Y + (63 * 3); break;
			case 16: iAdjBaseX = ADJ_BASE_X + (63 * 3);
				iAdjBaseY = ADJ_BASE_Y + (63 * 3); break;
			case 17: iAdjBaseX = ADJ_BASE_X + (63 * 0);
				iAdjBaseY = ADJ_BASE_Y + (63 * 4); break;
			case 18: iAdjBaseX = ADJ_BASE_X + (63 * 1);
				iAdjBaseY = ADJ_BASE_Y + (63 * 4); break;
			case 19: iAdjBaseX = ADJ_BASE_X + (63 * 2);
				iAdjBaseY = ADJ_BASE_Y + (63 * 4); break;
			case 20: iAdjBaseX = ADJ_BASE_X + (63 * 3);
				iAdjBaseY = ADJ_BASE_Y + (63 * 4); break;
			case 21: iAdjBaseX = ADJ_BASE_X + (63 * 0);
				iAdjBaseY = ADJ_BASE_Y + (63 * 5); break;
			case 22: iAdjBaseX = ADJ_BASE_X + (63 * 1);
				iAdjBaseY = ADJ_BASE_Y + (63 * 5); break;
			case 23: iAdjBaseX = ADJ_BASE_X + (63 * 2);
				iAdjBaseY = ADJ_BASE_Y + (63 * 5); break;
			case 24: iAdjBaseX = ADJ_BASE_X + (63 * 3);
				iAdjBaseY = ADJ_BASE_Y + (63 * 5); break;
			default:
				printf ("[FAILED] iLoopRoom is not in the 1-24 range!\n");
				exit (EXIT_ERROR);
		}
		if (InArea (iAdjBaseX + 1, iAdjBaseY + 16,
			iAdjBaseX + 15, iAdjBaseY + 30) == 1)
		{
			iChangingBrokenRoom = iLoopRoom;
			iChangingBrokenSide = 1; /*** left ***/
			iOnAdj = 1;
		}
		if (InArea (iAdjBaseX + 31, iAdjBaseY + 16,
			iAdjBaseX + 45, iAdjBaseY + 30) == 1)
		{
			iChangingBrokenRoom = iLoopRoom;
			iChangingBrokenSide = 2; /*** right ***/
			iOnAdj = 1;
		}
		if (InArea (iAdjBaseX + 16, iAdjBaseY + 1,
			iAdjBaseX + 30, iAdjBaseY + 14) == 1)
		{
			iChangingBrokenRoom = iLoopRoom;
			iChangingBrokenSide = 3; /*** up ***/
			iOnAdj = 1;
		}
		if (InArea (iAdjBaseX + 16, iAdjBaseY + 31,
			iAdjBaseX + 30, iAdjBaseY + 45) == 1)
		{
			iChangingBrokenRoom = iLoopRoom;
			iChangingBrokenSide = 4; /*** down ***/
			iOnAdj = 1;
		}
	}

	return (iOnAdj);
}
/*****************************************************************************/
void LinkPlus (void)
/*****************************************************************************/
{
	int iCurrent, iNew;

	iCurrent = iRoomConnections[iCurLevel]
		[iChangingBrokenRoom][iChangingBrokenSide];
	if ((iCurrent > ROOMS) && (iCurrent != 256)) /*** "?"; high links ***/
	{
		iNew = 256;
	} else if (iCurrent == 256) {
		iNew = 1;
	} else if (iCurrent == ROOMS) {
		iNew = 256;
	} else {
		iNew = iCurrent + 1;
	}
	iRoomConnections[iCurLevel][iChangingBrokenRoom][iChangingBrokenSide] = iNew;
	iChanged++;
	iBrokenRoomLinks = BrokenRoomLinks (0, iCurLevel);
	PlaySound ("wav/hum_adj.wav");
}
/*****************************************************************************/
void LinkMinus (void)
/*****************************************************************************/
{
	int iCurrent, iNew;

	iCurrent = iRoomConnections[iCurLevel]
		[iChangingBrokenRoom][iChangingBrokenSide];
	if ((iCurrent > ROOMS) && (iCurrent != 256)) /*** "?"; high links ***/
	{
		iNew = 256;
	} else if (iCurrent == 256) {
		iNew = ROOMS;
	} else if (iCurrent == 1) {
		iNew = 256;
	} else {
		iNew = iCurrent - 1;
	}
	iRoomConnections[iCurLevel][iChangingBrokenRoom][iChangingBrokenSide] = iNew;
	iChanged++;
	iBrokenRoomLinks = BrokenRoomLinks (0, iCurLevel);
	PlaySound ("wav/hum_adj.wav");
}
/*****************************************************************************/
void ClearRoom (void)
/*****************************************************************************/
{
	/*** Used for looping. ***/
	int iLoopCol;
	int iLoopRow;
	int iLoopElem;

	for (iLoopCol = 1; iLoopCol <= COLS; iLoopCol++)
	{
		for (iLoopRow = 1; iLoopRow <= ROWS; iLoopRow++)
		{
			SetLocation (iCurRoom, iLoopCol, iLoopRow, 0);
		}
	}

	/*** elements ***/
	for (iLoopElem = 1; iLoopElem <= MAX_ELEM; iLoopElem++)
	{
		arElements[iCurLevel][iCurRoom][iLoopElem][8] = 0x00; /*** off ***/
	}

	PlaySound ("wav/ok_close.wav");
	iChanged++;
}
/*****************************************************************************/
void RemoveOldRoom (void)
/*****************************************************************************/
{
	iRoomArray[iMovingOldX][iMovingOldY] = 0;

	/* Change the links of the rooms around
	 * the removed room.
	 */

	/*** left of removed ***/
	if ((iMovingOldX >= 2) && (iMovingOldX <= 24))
	{
		if (iRoomArray[iMovingOldX - 1][iMovingOldY] != 0)
		{
			iRoomConnections[iCurLevel][iRoomArray[iMovingOldX - 1]
				[iMovingOldY]][2] = 256; /*** remove right ***/
		}
	}

	/*** right of removed ***/
	if ((iMovingOldX >= 1) && (iMovingOldX <= 23))
	{
		if (iRoomArray[iMovingOldX + 1][iMovingOldY] != 0)
		{
			iRoomConnections[iCurLevel][iRoomArray[iMovingOldX + 1]
				[iMovingOldY]][1] = 256; /*** remove left ***/
		}
	}

	/*** above removed ***/
	if ((iMovingOldY >= 2) && (iMovingOldY <= 24))
	{
		if (iRoomArray[iMovingOldX][iMovingOldY - 1] != 0)
		{
			iRoomConnections[iCurLevel][iRoomArray[iMovingOldX]
				[iMovingOldY - 1]][4] = 256; /*** remove below ***/
		}
	}

	/*** below removed ***/
	if ((iMovingOldY >= 1) && (iMovingOldY <= 23))
	{
		if (iRoomArray[iMovingOldX][iMovingOldY + 1] != 0)
		{
			iRoomConnections[iCurLevel][iRoomArray[iMovingOldX]
				[iMovingOldY + 1]][3] = 256; /*** remove above ***/
		}
	}
}
/*****************************************************************************/
void AddNewRoom (int iX, int iY, int iRoom)
/*****************************************************************************/
{
	iRoomArray[iX][iY] = iRoom;

	/* Change the links of the rooms around
	 * the new room and the room itself.
	 */

	iRoomConnections[iCurLevel][iRoom][1] = 256;
	iRoomConnections[iCurLevel][iRoom][2] = 256;
	iRoomConnections[iCurLevel][iRoom][3] = 256;
	iRoomConnections[iCurLevel][iRoom][4] = 256;
	if ((iX >= 2) && (iX <= 24)) /*** left of added ***/
	{
		if (iRoomArray[iX - 1][iY] != 0)
		{
			iRoomConnections[iCurLevel][iRoomArray[iX - 1]
				[iY]][2] = iRoom; /*** add room right ***/
			iRoomConnections[iCurLevel][iRoom][1] = iRoomArray[iX - 1][iY];
		}
	}
	if ((iX >= 1) && (iX <= 23)) /*** right of added ***/
	{
		if (iRoomArray[iX + 1][iY] != 0)
		{
			iRoomConnections[iCurLevel][iRoomArray[iX + 1]
				[iY]][1] = iRoom; /*** add room left ***/
			iRoomConnections[iCurLevel][iRoom][2] = iRoomArray[iX + 1][iY];
		}
	}
	if ((iY >= 2) && (iY <= 24)) /*** above added ***/
	{
		if (iRoomArray[iX][iY - 1] != 0)
		{
			iRoomConnections[iCurLevel][iRoomArray[iX]
				[iY - 1]][4] = iRoom; /*** add room below ***/
			iRoomConnections[iCurLevel][iRoom][3] = iRoomArray[iX][iY - 1];
		}
	}
	if ((iY >= 1) && (iY <= 23)) /*** below added ***/
	{
		if (iRoomArray[iX][iY + 1] != 0)
		{
			iRoomConnections[iCurLevel][iRoomArray[iX]
				[iY + 1]][3] = iRoom; /*** add room above ***/
			iRoomConnections[iCurLevel][iRoom][4] = iRoomArray[iX][iY + 1];
		}
	}
	PlaySound ("wav/move_room.wav");
}
/*****************************************************************************/
void Help (void)
/*****************************************************************************/
{
	SDL_Event event;
	int iHelp;

	iHelp = 1;

	PlaySound ("wav/popup.wav");
	ShowHelp();
	while (iHelp == 1)
	{
		while (SDL_PollEvent (&event))
		{
			if (MapEvents (event) == 0)
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
							iHelp = 0; break;
					}
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						case SDLK_SPACE:
						case SDLK_o:
							iHelp = 0; break;
					}
					break;
				case SDL_MOUSEMOTION:
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					if (InArea (55, 323, 55 + 443, 323 + 19) == 1) /*** URL ***/
					{
						SDL_SetCursor (curHand);
					} else {
						SDL_SetCursor (curArrow);
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						if (InArea (460, 410, 460 + 85, 410 + 32) == 1) /*** OK ***/
							{ iHelpOK = 1; }
					}
					ShowHelp();
					break;
				case SDL_MOUSEBUTTONUP:
					iHelpOK = 0;
					if (event.button.button == 1)
					{
						if (InArea (460, 410, 460 + 85, 410 + 32) == 1) /*** OK ***/
							{ iHelp = 0; }
						if (InArea (55, 323, 55 + 443, 323 + 19) == 1) /*** URL ***/
							{ OpenURL ("https://www.norbertdejonge.nl/lemsop/"); }
					}
					ShowHelp();
					break;
				case SDL_WINDOWEVENT:
					switch (event.window.event)
					{
						case SDL_WINDOWEVENT_EXPOSED:
							ShowHelp(); break;
						case SDL_WINDOWEVENT_CLOSE:
							Quit(); break;
						case SDL_WINDOWEVENT_FOCUS_GAINED:
							iActiveWindowID = iWindowID; break;
					}
					break;
				case SDL_QUIT:
					Quit(); break;
			}
		}

		/*** prevent CPU eating ***/
		gamespeed = REFRESH;
		while ((SDL_GetTicks() - looptime) < gamespeed)
		{
			SDL_Delay (10);
		}
		looptime = SDL_GetTicks();
	}
	PlaySound ("wav/popup_close.wav");
	SDL_SetCursor (curArrow);
	ShowScreen (iScreen);
}
/*****************************************************************************/
void ShowHelp (void)
/*****************************************************************************/
{
	/*** help ***/
	ShowImage (imghelp, 0, 0, "imghelp", ascreen, iScale, 1);

	/*** OK ***/
	switch (iHelpOK)
	{
		case 0: /*** off ***/
			ShowImage (imgok[1], 460, 410, "imgok[1]", ascreen, iScale, 1);
			break;
		case 1: /*** on ***/
			ShowImage (imgok[2], 460, 410, "imgok[2]", ascreen, iScale, 1);
			break;
	}

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void OpenURL (char *sURL)
/*****************************************************************************/
{
#if defined WIN32 || _WIN32 || WIN64 || _WIN64
ShellExecute (NULL, "open", sURL, NULL, NULL, SW_SHOWNORMAL);
#else
pid_t pid;
pid = fork();
if (pid == 0)
{
	execl ("/usr/bin/xdg-open", "xdg-open", sURL, (char *)NULL);
	exit (EXIT_NORMAL);
}
#endif
}
/*****************************************************************************/
void EXE (void)
/*****************************************************************************/
{
	int iEXE;
	SDL_Event event;
	const Uint8 *keystate;
	char cChar;

	/*** Used for looping. ***/
	int iLoopText;
	int iLoopChar;

	iEXE = 1;
	iStatusBarFrame = 1;
	snprintf (sStatus, MAX_STATUS, "%s", "");

	EXELoad();

	PlaySound ("wav/popup.wav");
	ShowEXE();
	while (iEXE == 1)
	{
		if (iNoAnim == 0)
		{
			/*** Using the global REFRESH. No need for newticks/oldticks. ***/
			iStatusBarFrame++;
			if (iStatusBarFrame == 19) { iStatusBarFrame = 1; }
			ShowEXE();
		}

		while (SDL_PollEvent (&event))
		{
			if (MapEvents (event) == 0)
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
							EXESave(); iEXE = 0; break;
					}
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							iEXE = 0; break;
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
							EXESave(); iEXE = 0; break;
					}
					if ((iTextActiveText == 0) && (iTextActiveChar == 0))
					{
						switch (event.key.keysym.sym)
						{
							case SDLK_SPACE:
							case SDLK_s:
								EXESave(); iEXE = 0; break;
						}
					} else {
						keystate = SDL_GetKeyboardState (NULL);
						switch (event.key.keysym.sym)
						{
							case SDLK_0:
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
									{ cChar = ')'; } else { cChar = '0'; }
								break;
							case SDLK_1:
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
									{ cChar = '!'; } else { cChar = '1'; }
								break;
							case SDLK_2:
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
									{ cChar = '@'; } else { cChar = '2'; }
								break;
							case SDLK_3:
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
									{ cChar = '#'; } else { cChar = '3'; }
								break;
							case SDLK_4:
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
									{ cChar = '$'; } else { cChar = '4'; }
								break;
							case SDLK_5:
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
									{ cChar = '\0'; } else { cChar = '5'; } /*** No %. ***/
								break;
							case SDLK_6:
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
									{ cChar = '\0'; } else { cChar = '6'; } /*** No ^. ***/
								break;
							case SDLK_7:
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
									{ cChar = '&'; } else { cChar = '7'; }
								break;
							case SDLK_8:
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
									{ cChar = '*'; } else { cChar = '8'; }
								break;
							case SDLK_9:
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
									{ cChar = '('; } else { cChar = '9'; }
								break;
							case SDLK_a: cChar = 'A'; break;
							case SDLK_b: cChar = 'B'; break;
							case SDLK_c: cChar = 'C'; break;
							case SDLK_d: cChar = 'D'; break;
							case SDLK_e: cChar = 'E'; break;
							case SDLK_f: cChar = 'F'; break;
							case SDLK_g: cChar = 'G'; break;
							case SDLK_h: cChar = 'H'; break;
							case SDLK_i: cChar = 'I'; break;
							case SDLK_j: cChar = 'J'; break;
							case SDLK_k: cChar = 'K'; break;
							case SDLK_l: cChar = 'L'; break;
							case SDLK_m: cChar = 'M'; break;
							case SDLK_n: cChar = 'N'; break;
							case SDLK_o: cChar = 'O'; break;
							case SDLK_p: cChar = 'P'; break;
							case SDLK_q: cChar = 'Q'; break;
							case SDLK_r: cChar = 'R'; break;
							case SDLK_s: cChar = 'S'; break;
							case SDLK_t: cChar = 'T'; break;
							case SDLK_u: cChar = 'U'; break;
							case SDLK_v: cChar = 'V'; break;
							case SDLK_w: cChar = 'W'; break;
							case SDLK_x: cChar = 'X'; break;
							case SDLK_y: cChar = 'Y'; break;
							case SDLK_z: cChar = 'Z'; break;
							case SDLK_QUOTE:
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
									{ cChar = '"'; } else { cChar = '\''; }
								break;
							case SDLK_SEMICOLON:
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
									{ cChar = ':'; } else { cChar = ';'; }
								break;
							case SDLK_COMMA:
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
									{ cChar = '<'; } else { cChar = ','; }
								break;
							case SDLK_EQUALS:
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
									{ cChar = '+'; } else { cChar = '='; }
								break;
							case SDLK_PERIOD:
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
									{ cChar = '>'; } else { cChar = '.'; }
								break;
							case SDLK_MINUS:
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
									{ cChar = '\0'; } else { cChar = '-'; } /*** No _. ***/
								break;
							case SDLK_SLASH:
								if ((keystate[SDL_SCANCODE_LSHIFT]) ||
									(keystate[SDL_SCANCODE_RSHIFT]))
									{ cChar = '?'; } else { cChar = '/'; }
								break;
							case SDLK_SPACE: cChar = ' '; break;
							default: cChar = '\0'; break;
						}
						if (cChar != '\0')
						{
							arTextChars[iTextActiveText][iTextActiveChar - 1] = cChar;
							PlaySound ("wav/hum_adj.wav");
						}
					}
					break;
				case SDL_MOUSEMOTION:
					iXPos = event.motion.x;
					iYPos = event.motion.y;

					/*** chars ***/
					iTextActiveText = 0; iTextActiveChar = 0;
					for (iLoopText = 1; iLoopText <= TEXTS; iLoopText++)
					{
						for (iLoopChar = 1; iLoopChar <=
							arTextLength[iLoopText]; iLoopChar++)
						{
							if (InArea (arTextX[iLoopText] + (iLoopChar * 16),
								arTextY[iLoopText],
								arTextX[iLoopText] + (iLoopChar * 16) + 16,
								arTextY[iLoopText] + 16) == 1)
							{
								iTextActiveText = iLoopText;
								iTextActiveChar = iLoopChar;
							}
						}
					}

					UpdateStatusBar();

					ShowEXE();
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						if (InArea (460, 410, 460 + 85, 410 + 32) == 1) /*** Save ***/
							{ iEXESave = 1; }
					}
					ShowEXE();
					break;
				case SDL_MOUSEBUTTONUP:
					iEXESave = 0;
					if (event.button.button == 1)
					{
						if (InArea (460, 410, 460 + 85, 410 + 32) == 1) /*** Save ***/
							{ EXESave(); iEXE = 0; }
					}
					ShowEXE();
					break;
				case SDL_WINDOWEVENT:
					switch (event.window.event)
					{
						case SDL_WINDOWEVENT_EXPOSED:
							ShowEXE(); break;
						case SDL_WINDOWEVENT_CLOSE:
							Quit(); break;
						case SDL_WINDOWEVENT_FOCUS_GAINED:
							iActiveWindowID = iWindowID; break;
					}
					break;
				case SDL_QUIT:
					Quit(); break;
			}
		}

		/*** prevent CPU eating ***/
		gamespeed = REFRESH;
		while ((SDL_GetTicks() - looptime) < gamespeed)
		{
			SDL_Delay (10);
		}
		looptime = SDL_GetTicks();
	}
	PlaySound ("wav/popup_close.wav");
	ShowScreen (iScreen);
}
/*****************************************************************************/
void ShowEXE (void)
/*****************************************************************************/
{
	char arText[1 + 2][MAX_TEXT + 2];
	SDL_Texture *img;

	/*** Used for looping. ***/
	int iLoopText;
	int iLoopChar;

	/*** exe ***/
	ShowImage (imgexe, 0, 0, "imgexe", ascreen, iScale, 1);

	/*** status bar ***/
	if (strcmp (sStatus, "") != 0)
	{
		/*** bulb ***/
		ShowImage (imgstatusbarsprite, 23, 415, "imgstatusbarsprite",
			ascreen, iScale, 1);
		/*** text ***/
		snprintf (arText[0], MAX_TEXT, "%s", sStatus);
		DisplayText (50, 419, 11, arText, 1, font11, color_f4, color_bl, 0);
	}

	/*** save ***/
	switch (iEXESave)
	{
		case 0: /*** off ***/
			ShowImage (imgsave[1], 460, 410, "imgsave[1]",
				ascreen, iScale, 1); break;
		case 1: /*** on ***/
			ShowImage (imgsave[2], 460, 410, "imgsave[2]",
				ascreen, iScale, 1); break;
	}

	/*** display texts ***/
	for (iLoopText = 1; iLoopText <= TEXTS; iLoopText++)
	{
		for (iLoopChar = 1; iLoopChar <= arTextLength[iLoopText]; iLoopChar++)
		{
			switch (arTextChars[iLoopText][iLoopChar - 1])
			{
				case '0': img = imgchars[0]; break;
				case '1': img = imgchars[1]; break;
				case '2': img = imgchars[2]; break;
				case '3': img = imgchars[3]; break;
				case '4': img = imgchars[4]; break;
				case '5': img = imgchars[5]; break;
				case '6': img = imgchars[6]; break;
				case '7': img = imgchars[7]; break;
				case '8': img = imgchars[8]; break;
				case '9': img = imgchars[9]; break;
				case 'A': img = imgchars[10]; break;
				case 'B': img = imgchars[11]; break;
				case 'C': img = imgchars[12]; break;
				case 'D': img = imgchars[13]; break;
				case 'E': img = imgchars[14]; break;
				case 'F': img = imgchars[15]; break;
				case 'G': img = imgchars[16]; break;
				case 'H': img = imgchars[17]; break;
				case 'I': img = imgchars[18]; break;
				case 'J': img = imgchars[19]; break;
				case 'K': img = imgchars[20]; break;
				case 'L': img = imgchars[21]; break;
				case 'M': img = imgchars[22]; break;
				case 'N': img = imgchars[23]; break;
				case 'O': img = imgchars[24]; break;
				case 'P': img = imgchars[25]; break;
				case 'Q': img = imgchars[26]; break;
				case 'R': img = imgchars[27]; break;
				case 'S': img = imgchars[28]; break;
				case 'T': img = imgchars[29]; break;
				case 'U': img = imgchars[30]; break;
				case 'V': img = imgchars[31]; break;
				case 'W': img = imgchars[32]; break;
				case 'X': img = imgchars[33]; break;
				case 'Y': img = imgchars[34]; break;
				case 'Z': img = imgchars[35]; break;
				case '&': img = imgchars[36]; break;
				case '\'': img = imgchars[37]; break;
				case '*': img = imgchars[38]; break;
				case '@': img = imgchars[39]; break;
				case ')': img = imgchars[40]; break;
				case ':': img = imgchars[41]; break;
				case ',': img = imgchars[42]; break;
				case '$': img = imgchars[43]; break;
				case '=': img = imgchars[44]; break;
				case '!': img = imgchars[45]; break;
				case '>': img = imgchars[46]; break;
				case '#': img = imgchars[47]; break;
				case '-': img = imgchars[48]; break;
				case '<': img = imgchars[49]; break;
				case '(': img = imgchars[50]; break;
				case '.': img = imgchars[51]; break;
				case '+': img = imgchars[52]; break;
				case '?': img = imgchars[53]; break;
				case '"': img = imgchars[54]; break;
				case '/': img = imgchars[55]; break;
				case ';': img = imgchars[56]; break; /*** slashed O ***/
				case ' ': img = imgchars[57]; break;
				default:
					printf ("[ WARN ] Unknown char: %c!\n",
						arTextChars[iLoopText][iLoopChar - 1]);
					img = imgchars[57]; /*** Fallback (space). ***/
					break;
			}
			ShowImage (img, arTextX[iLoopText] + (iLoopChar * 16),
				arTextY[iLoopText], "img", ascreen, iScale, 1);
		}
	}

	/*** active char ***/
	if ((iTextActiveText != 0) && (iTextActiveChar != 0))
	{
		ShowImage (imgactivechar, arTextX[iTextActiveText] + (iTextActiveChar
			* 16), arTextY[iTextActiveText], "imgactivechar", ascreen, iScale, 1);
	}

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void EXELoad (void)
/*****************************************************************************/
{
	int iFdEXE;

	/*** Used for looping. ***/
	int iLoopText;

	iFdEXE = open (sPathFile, O_RDONLY|O_BINARY);
	if (iFdEXE == -1)
	{
		printf ("[FAILED] Error opening %s: %s!\n", sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}

	for (iLoopText = 1; iLoopText <= TEXTS; iLoopText++)
	{
		lseek (iFdEXE, arTextOffset[iLoopText], SEEK_SET);
		ReadFromFile (iFdEXE, "", arTextLength[iLoopText], arTextChars[iLoopText]);
		if (iDebug == 1)
		{
			printf ("[ INFO ] Text %02i: %s\n", iLoopText, arTextChars[iLoopText]);
		}
	}

	close (iFdEXE);
}
/*****************************************************************************/
void EXESave (void)
/*****************************************************************************/
{
	int iFdEXE;

	/*** Used for looping. ***/
	int iLoopText;

	iFdEXE = open (sPathFile, O_RDWR|O_BINARY);
	if (iFdEXE == -1)
	{
		printf ("[FAILED] Error opening %s: %s!\n", sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}

	for (iLoopText = 1; iLoopText <= TEXTS; iLoopText++)
	{
		lseek (iFdEXE, arTextOffset[iLoopText], SEEK_SET);
		write (iFdEXE, arTextChars[iLoopText], arTextLength[iLoopText]);
	}

	close (iFdEXE);

	PlaySound ("wav/save.wav");
}
/*****************************************************************************/
void UpdateStatusBar (void)
/*****************************************************************************/
{
	snprintf (sStatusOld, MAX_STATUS, "%s", sStatus);
	snprintf (sStatus, MAX_STATUS, "%s", "");
	if (InArea (487, 66, 487 + 49, 66 + 320) == 1) /*** Instructions. ***/
	{
		snprintf (sStatus, MAX_STATUS, "%s",
			"To read this, turn your head 90 degrees clockwise.");
	}
	if (strcmp (sStatus, sStatusOld) != 0) { ShowEXE(); }
}
/*****************************************************************************/
void CenterNumber (SDL_Renderer *screen, int iNumber, int iX, int iY,
	SDL_Color fore, int iHex)
/*****************************************************************************/
{
	char sText[MAX_TEXT + 2];

	if (iHex == 0)
	{
		if (iNumber <= 999)
		{
			snprintf (sText, MAX_TEXT, "%i", iNumber);
		} else {
			snprintf (sText, MAX_TEXT, "%s", "X");
		}
	} else {
		snprintf (sText, MAX_TEXT, "%02X", iNumber);
	}
	message = TTF_RenderText_Blended_Wrapped (font20, sText, fore, 0);
	messaget = SDL_CreateTextureFromSurface (screen, message);
	if (iHex == 0)
	{
		if ((iNumber >= -9) && (iNumber <= -1))
		{
			offset.x = iX + 16;
		} else if ((iNumber >= 0) && (iNumber <= 9)) {
			offset.x = iX + 21;
		} else if ((iNumber >= 10) && (iNumber <= 99)) {
			offset.x = iX + 14;
		} else if ((iNumber >= 100) && (iNumber <= 999)) {
			offset.x = iX + 7;
		} else {
			offset.x = iX + 21;
		}
	} else {
		offset.x = iX + 14;
	}
	offset.y = iY - 1;
	offset.w = message->w; offset.h = message->h;
	CustomRenderCopy (messaget, "message", NULL, screen, &offset);
	SDL_DestroyTexture (messaget); SDL_FreeSurface (message);
}
/*****************************************************************************/
int PlusMinus (int *iWhat, int iX, int iY,
	int iMin, int iMax, int iChange, int iAddChanged)
/*****************************************************************************/
{
	if ((InArea (iX, iY, iX + 13, iY + 20) == 1) &&
		(((iChange < 0) && (*iWhat > iMin)) ||
		((iChange > 0) && (*iWhat < iMax))))
	{
		*iWhat = *iWhat + iChange;
		if ((iChange < 0) && (*iWhat < iMin)) { *iWhat = iMin; }
		if ((iChange > 0) && (*iWhat > iMax)) { *iWhat = iMax; }
		if (iAddChanged == 1) { iChanged++; }
		PlaySound ("wav/plus_minus.wav");
		return (1);
	} else { return (0); }
}
/*****************************************************************************/
void CopyPaste (int iRoom, int iAction)
/*****************************************************************************/
{
	/*** Used for looping. ***/
	int iLoopCol;
	int iLoopRow;
	int iLoopElem;
	int iLoopByte;

	if (iAction == 1) /*** copy ***/
	{
		/*** tiles ***/
		for (iLoopCol = 1; iLoopCol <= COLS; iLoopCol++)
		{
			for (iLoopRow = 1; iLoopRow <= ROWS; iLoopRow++)
			{
				arTilesCopyPaste[iLoopCol][iLoopRow] =
					arTiles[iRoom][iLoopCol][iLoopRow];
			}
		}
		/*** elements ***/
		for (iLoopElem = 1; iLoopElem <= MAX_ELEM; iLoopElem++)
		{
			for (iLoopByte = 0; iLoopByte <= 8; iLoopByte++)
			{
				arElementsCopyPaste[iLoopElem][iLoopByte] =
					arElements[iCurLevel][iRoom][iLoopElem][iLoopByte];
			}
		}
		iCopied = 1;
	} else { /*** paste ***/
		if (iCopied == 0)
		{
			ClearRoom();
		} else {
			/*** tiles ***/
			for (iLoopCol = 1; iLoopCol <= COLS; iLoopCol++)
			{
				for (iLoopRow = 1; iLoopRow <= ROWS; iLoopRow++)
				{
					arTiles[iRoom][iLoopCol][iLoopRow] =
						arTilesCopyPaste[iLoopCol][iLoopRow];
				}
			}
			/*** elements ***/
			for (iLoopElem = 1; iLoopElem <= MAX_ELEM; iLoopElem++)
			{
				for (iLoopByte = 0; iLoopByte <= 8; iLoopByte++)
				{
					arElements[iCurLevel][iRoom][iLoopElem][iLoopByte] =
						arElementsCopyPaste[iLoopElem][iLoopByte];
				}
			}
		}
	}
}
/*****************************************************************************/
void CreateBAK (void)
/*****************************************************************************/
{
	FILE *fBIN;
	FILE *fBAK;
	int iData;

	fBIN = fopen (sPathFile, "rb");
	if (fBIN == NULL)
		{ printf ("[ WARN ] Could not open \"%s\": %s!\n",
			sPathFile, strerror (errno)); }

	fBAK = fopen (BACKUP, "wb");
	if (fBAK == NULL)
		{ printf ("[ WARN ] Could not open \"%s\": %s!\n",
			BACKUP, strerror (errno)); }

	while (1)
	{
		iData = fgetc (fBIN);
		if (iData == EOF) { break; }
			else { putc (iData, fBAK); }
	}

	fclose (fBIN);
	fclose (fBAK);
}
/*****************************************************************************/
void Sprinkle (void)
/*****************************************************************************/
{
	int iRandom;

	/*** Used for looping. ***/
	int iLoopRoom;
	int iLoopCol;
	int iLoopRow;

	for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
	{
		for (iLoopCol = 1; iLoopCol <= COLS; iLoopCol++)
		{
			for (iLoopRow = 1; iLoopRow <= ROWS; iLoopRow++)
			{
				if (cCurType == 'd')
				{
					/*** empty? add wall pattern ***/
					if (arTiles[iLoopRoom][iLoopCol][iLoopRow] == 0)
					{
						/*** 1-32 ***/
						iRandom = 1 + (int) (32.0 * rand() / (RAND_MAX + 1.0));
						switch (iRandom)
						{
							case 1:
								arTiles[iLoopRoom][iLoopCol][iLoopRow] = 46;
								break;
							case 2:
								arTiles[iLoopRoom][iLoopCol][iLoopRow] = 52;
								break;
							case 3:
								arTiles[iLoopRoom][iLoopCol][iLoopRow] = 55;
								break;
							case 4:
								arTiles[iLoopRoom][iLoopCol][iLoopRow] = 57;
								break;
						}
					}
				} else {
					/*** empty? add wall pattern ***/
					if (arTiles[iLoopRoom][iLoopCol][iLoopRow] == 0)
					{
						/*** 1-16 ***/
						iRandom = 1 + (int) (16.0 * rand() / (RAND_MAX + 1.0));
						switch (iRandom)
						{
							case 1:
								arTiles[iLoopRoom][iLoopCol][iLoopRow] = 1;
								break;
							case 2:
								arTiles[iLoopRoom][iLoopCol][iLoopRow] = 16;
								break;
						}
					}
				}
			}
		}
	}
}
/*****************************************************************************/
unsigned long BytesAsLU (unsigned char *sData, int iBytes)
/*****************************************************************************/
{
	unsigned long luReturn;
	char sString[MAX_DATA + 2];
	char sTemp[MAX_DATA + 2];
	int iTemp;

	snprintf (sString, MAX_DATA, "%s", "");
	for (iTemp = iBytes - 1; iTemp >= 0; iTemp--)
	{
		snprintf (sTemp, MAX_DATA, "%s%02x", sString, sData[iTemp]);
		snprintf (sString, MAX_DATA, "%s", sTemp);
	}
	luReturn = strtoul (sString, NULL, 16);

	return (luReturn);
}
/*****************************************************************************/
int BankedToLinear (int iAddr)
/*****************************************************************************/
{
	int iBank, iOffset;

	iBank = iAddr & 0x000F;
	iOffset = iAddr & 0xFFF0;
	if ((iOffset < 0x8000) || (iOffset > 0xBFFF))
		{ printf ("[ WARN ] Incorrect offset in BankedToLinear()!\n"); }
	return ((iBank << 14) + (iOffset - 0x8000));
}
/*****************************************************************************/
int LinearToBanked (int iAddr)
/*****************************************************************************/
{
	int iBank, iOffset;

	if (((iAddr & 0x000F) != 0) || (iAddr < 0) || (iAddr >= 0x40000))
		{ printf ("[ WARN ] Incorrect address in LinearToBanked()!\n"); }
	iBank = (iAddr >> 14) & 0x000F;
	iOffset = (iAddr & 0x3FF0) + 0x8000;
	return (iOffset | iBank);
}
/*****************************************************************************/
int GetChunkLength (int iLevel, int iNrElementRooms)
/*****************************************************************************/
{
	int iRooms;
	int iChunkLength;

	/*** Used for looping. ***/
	int iLoopRoom;
	int iLoopElem;

	/*** offsets and room links ***/
	switch (iLevel)
	{
		case 14: iRooms = 5; break;
		default: iRooms = ROOMS; break;
	}
	iChunkLength = (24 * 2) + (iRooms * 4);

	/*** elements ***/
	if (iNrElementRooms != 0)
	{
		for (iLoopRoom = 1; iLoopRoom <= iNrElementRooms; iLoopRoom++)
		{
			for (iLoopElem = 1; iLoopElem <= MAX_ELEM; iLoopElem++)
			{
				if (arElements[iLevel][iLoopRoom][iLoopElem][8] == 0x01) /*** on ***/
				{
					switch (arElements[iLevel][iLoopRoom][iLoopElem][0])
					{
						case 0x27: iChunkLength+=8; break;
						case 0x29: iChunkLength+=6; break;
						case 0x32: iChunkLength+=8; break;
						case 0x3E: iChunkLength+=6; break;
						case 0x3F: iChunkLength+=6; break;
						case 0x61: iChunkLength+=6; break;
						case 0x62: iChunkLength+=6; break;
						case 0x64: iChunkLength+=6; break;
						default: iChunkLength+=3; break;
					}
				}
			}
			iChunkLength++; /*** 0xFF ***/
		}
	}

	return (iChunkLength);
}
/*****************************************************************************/
void ColorRect (int iX, int iY, int iW, int iH, int iR, int iG, int iB)
/*****************************************************************************/
{
	SDL_Rect rect;

	rect.x = iX * iScale;
	rect.y = iY * iScale;
	rect.w = iW * iScale;
	rect.h = iH * iScale;
	SDL_SetRenderDrawColor (ascreen, iR, iG, iB, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect (ascreen, &rect);
}
/*****************************************************************************/
void Guards (void)
/*****************************************************************************/
{
	int iGuards;
	SDL_Event event;
	int iChecked;

	if (iScreen == 3) /*** E ***/
	{
		if (arElements[iCurLevel][iCurRoom][iActiveElement][0] == 0x64)
		{
			iActiveGuard = arElements[iCurLevel][iCurRoom][iActiveElement][1];
			if (iActiveGuard > MAX_GUARDS) { iActiveGuard = MAX_GUARDS; }
		}
	}

	iGuards = 1;

	PlaySound ("wav/popup.wav");
	ShowGuards();
	while (iGuards == 1)
	{
		while (SDL_PollEvent (&event))
		{
			if (MapEvents (event) == 0)
			switch (event.type)
			{
				case SDL_CONTROLLERBUTTONDOWN:
					/*** Nothing for now. ***/
					break;
				case SDL_CONTROLLERBUTTONUP:
					switch (event.cbutton.button)
					{
						case SDL_CONTROLLER_BUTTON_A:
							iGuards = 0; break;
					}
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						case SDLK_SPACE:
						case SDLK_o:
							iGuards = 0; break;
						case SDLK_LEFT:
							if ((event.key.keysym.mod & KMOD_LSHIFT) ||
								(event.key.keysym.mod & KMOD_RSHIFT))
							{
								if (iActiveGuard > 0)
								{
									iActiveGuard--;
									PlaySound ("wav/plus_minus.wav");
								}
							} else if ((event.key.keysym.mod & KMOD_LCTRL) ||
								(event.key.keysym.mod & KMOD_RCTRL))
							{
								if (iActiveGuard > 0)
								{
									iActiveGuard-=10;
									if (iActiveGuard < 0) { iActiveGuard = 0; }
									PlaySound ("wav/plus_minus.wav");
								}
							}
							break;
						case SDLK_RIGHT:
							if ((event.key.keysym.mod & KMOD_LSHIFT) ||
								(event.key.keysym.mod & KMOD_RSHIFT))
							{
								if (iActiveGuard < MAX_GUARDS)
								{
									iActiveGuard++;
									PlaySound ("wav/plus_minus.wav");
								}
							} else if ((event.key.keysym.mod & KMOD_LCTRL) ||
								(event.key.keysym.mod & KMOD_RCTRL))
							{
								if (iActiveGuard < MAX_GUARDS)
								{
									iActiveGuard+=10;
									if (iActiveGuard > MAX_GUARDS)
										{ iActiveGuard = MAX_GUARDS; }
									PlaySound ("wav/plus_minus.wav");
								}
							}
							break;
						case SDLK_r: /*** Red. ***/
							arGuardR[iActiveGuard]++;
							if (arGuardR[iActiveGuard] == 4)
								{ arGuardR[iActiveGuard] = 0; }
							PlaySound ("wav/check_box.wav");
							iChanged++;
							break;
						case SDLK_g: /*** Green. ***/
							arGuardG[iActiveGuard]++;
							if (arGuardG[iActiveGuard] == 4)
								{ arGuardG[iActiveGuard] = 0; }
							PlaySound ("wav/check_box.wav");
							iChanged++;
							break;
						case SDLK_b: /*** Blue. ***/
							arGuardB[iActiveGuard]++;
							if (arGuardB[iActiveGuard] == 4)
								{ arGuardB[iActiveGuard] = 0; }
							PlaySound ("wav/check_box.wav");
							iChanged++;
							break;
						case SDLK_l: /*** Game level. ***/
							arGuardGameLevel[iActiveGuard]++;
							if (arGuardGameLevel[iActiveGuard] == 15)
								{ arGuardGameLevel[iActiveGuard] = 1; }
							PlaySound ("wav/check_box.wav");
							iChanged++;
							break;
						case SDLK_h: /*** Hit points. ***/
							arGuardHP[iActiveGuard]++;
							if (arGuardHP[iActiveGuard] == 16)
								{ arGuardHP[iActiveGuard] = 0; }
							PlaySound ("wav/check_box.wav");
							iChanged++;
							break;
						case SDLK_d: /*** Direction. ***/
							if (arGuardDir[iActiveGuard] == 0)
								{ arGuardDir[iActiveGuard] = 1; }
									else { arGuardDir[iActiveGuard] = 0; }
							PlaySound ("wav/check_box.wav");
							iChanged++;
							break;
						case SDLK_m: /*** Move to prince. ***/
							if (arGuardMove[iActiveGuard] == 0)
								{ arGuardMove[iActiveGuard] = 1; }
									else { arGuardMove[iActiveGuard] = 0; }
							PlaySound ("wav/check_box.wav");
							iChanged++;
							break;
					}
					ShowGuards();
					break;
				case SDL_MOUSEMOTION:
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					ShowGuards();
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						if (InArea (460, 410, 460 + 85, 410 + 32) == 1) /*** OK ***/
							{ iOKOn = 1; }
					}
					ShowGuards();
					break;
				case SDL_MOUSEBUTTONUP:
					iOKOn = 0;
					iChecked = 0;
					if (event.button.button == 1)
					{
						if (InArea (460, 410, 460 + 85, 410 + 32) == 1) /*** OK ***/
							{ iGuards = 0; }

						/*** Edit this guard. ***/
						PlusMinus (&iActiveGuard, 127, 29, 0, MAX_GUARDS, -10, 1);
						PlusMinus (&iActiveGuard, 142, 29, 0, MAX_GUARDS, -1, 1);
						PlusMinus (&iActiveGuard, 212, 29, 0, MAX_GUARDS, +1, 1);
						PlusMinus (&iActiveGuard, 227, 29, 0, MAX_GUARDS, +10, 1);

						/*** Red. ***/
						if (InArea (130, 81, 130 + 14, 81 + 14) == 1)
							{ arGuardR[iActiveGuard] = 0; iChecked = 1; }
						if (InArea (145, 81, 145 + 14, 81 + 14) == 1)
							{ arGuardR[iActiveGuard] = 1; iChecked = 1; }
						if (InArea (160, 81, 160 + 14, 81 + 14) == 1)
							{ arGuardR[iActiveGuard] = 2; iChecked = 1; }
						if (InArea (175, 81, 175 + 14, 81 + 14) == 1)
							{ arGuardR[iActiveGuard] = 3; iChecked = 1; }

						/*** Green. ***/
						if (InArea (274, 81, 274 + 14, 81 + 14) == 1)
							{ arGuardG[iActiveGuard] = 0; iChecked = 1; }
						if (InArea (289, 81, 289 + 14, 81 + 14) == 1)
							{ arGuardG[iActiveGuard] = 1; iChecked = 1; }
						if (InArea (304, 81, 304 + 14, 81 + 14) == 1)
							{ arGuardG[iActiveGuard] = 2; iChecked = 1; }
						if (InArea (319, 81, 319 + 14, 81 + 14) == 1)
							{ arGuardG[iActiveGuard] = 3; iChecked = 1; }

						/*** Blue. ***/
						if (InArea (403, 81, 403 + 14, 81 + 14) == 1)
							{ arGuardB[iActiveGuard] = 0; iChecked = 1; }
						if (InArea (418, 81, 418 + 14, 81 + 14) == 1)
							{ arGuardB[iActiveGuard] = 1; iChecked = 1; }
						if (InArea (433, 81, 433 + 14, 81 + 14) == 1)
							{ arGuardB[iActiveGuard] = 2; iChecked = 1; }
						if (InArea (448, 81, 448 + 14, 81 + 14) == 1)
							{ arGuardB[iActiveGuard] = 3; iChecked = 1; }

						/*** Game level. ***/
						if (InArea (117, 121, 117 + 14, 121 + 14) == 1)
							{ arGuardGameLevel[iActiveGuard] = 1; iChecked = 1; }
						if (InArea (132, 121, 132 + 14, 121 + 14) == 1)
							{ arGuardGameLevel[iActiveGuard] = 2; iChecked = 1; }
						if (InArea (147, 121, 147 + 14, 121 + 14) == 1)
							{ arGuardGameLevel[iActiveGuard] = 3; iChecked = 1; }
						if (InArea (162, 121, 162 + 14, 121 + 14) == 1)
							{ arGuardGameLevel[iActiveGuard] = 4; iChecked = 1; }
						if (InArea (177, 121, 177 + 14, 121 + 14) == 1)
							{ arGuardGameLevel[iActiveGuard] = 5; iChecked = 1; }
						if (InArea (192, 121, 192 + 14, 121 + 14) == 1)
							{ arGuardGameLevel[iActiveGuard] = 6; iChecked = 1; }
						if (InArea (207, 121, 207 + 14, 121 + 14) == 1)
							{ arGuardGameLevel[iActiveGuard] = 7; iChecked = 1; }
						if (InArea (222, 121, 222 + 14, 121 + 14) == 1)
							{ arGuardGameLevel[iActiveGuard] = 8; iChecked = 1; }
						if (InArea (237, 121, 237 + 14, 121 + 14) == 1)
							{ arGuardGameLevel[iActiveGuard] = 9; iChecked = 1; }
						if (InArea (252, 121, 252 + 14, 121 + 14) == 1)
							{ arGuardGameLevel[iActiveGuard] = 10; iChecked = 1; }
						if (InArea (267, 121, 267 + 14, 121 + 14) == 1)
							{ arGuardGameLevel[iActiveGuard] = 11; iChecked = 1; }
						if (InArea (282, 121, 282 + 14, 121 + 14) == 1)
							{ arGuardGameLevel[iActiveGuard] = 12; iChecked = 1; }
						if (InArea (297, 121, 297 + 14, 121 + 14) == 1)
							{ arGuardGameLevel[iActiveGuard] = 13; iChecked = 1; }
						if (InArea (312, 121, 312 + 14, 121 + 14) == 1)
							{ arGuardGameLevel[iActiveGuard] = 14; iChecked = 1; }

						/*** Hit points. ***/
						if (InArea (97, 162, 97 + 14, 162 + 14) == 1)
							{ arGuardHP[iActiveGuard] = 0; iChecked = 1; }
						if (InArea (112, 162, 112 + 14, 162 + 14) == 1)
							{ arGuardHP[iActiveGuard] = 1; iChecked = 1; }
						if (InArea (127, 162, 127 + 14, 162 + 14) == 1)
							{ arGuardHP[iActiveGuard] = 2; iChecked = 1; }
						if (InArea (142, 162, 142 + 14, 162 + 14) == 1)
							{ arGuardHP[iActiveGuard] = 3; iChecked = 1; }
						if (InArea (157, 162, 157 + 14, 162 + 14) == 1)
							{ arGuardHP[iActiveGuard] = 4; iChecked = 1; }
						if (InArea (172, 162, 172 + 14, 162 + 14) == 1)
							{ arGuardHP[iActiveGuard] = 5; iChecked = 1; }
						if (InArea (187, 162, 187 + 14, 162 + 14) == 1)
							{ arGuardHP[iActiveGuard] = 6; iChecked = 1; }
						if (InArea (202, 162, 202 + 14, 162 + 14) == 1)
							{ arGuardHP[iActiveGuard] = 7; iChecked = 1; }
						if (InArea (217, 162, 217 + 14, 162 + 14) == 1)
							{ arGuardHP[iActiveGuard] = 8; iChecked = 1; }
						if (InArea (232, 162, 232 + 14, 162 + 14) == 1)
							{ arGuardHP[iActiveGuard] = 9; iChecked = 1; }
						if (InArea (247, 162, 247 + 14, 162 + 14) == 1)
							{ arGuardHP[iActiveGuard] = 10; iChecked = 1; }
						if (InArea (262, 162, 262 + 14, 162 + 14) == 1)
							{ arGuardHP[iActiveGuard] = 11; iChecked = 1; }
						if (InArea (277, 162, 277 + 14, 162 + 14) == 1)
							{ arGuardHP[iActiveGuard] = 12; iChecked = 1; }
						if (InArea (292, 162, 292 + 14, 162 + 14) == 1)
							{ arGuardHP[iActiveGuard] = 13; iChecked = 1; }
						if (InArea (307, 162, 307 + 14, 162 + 14) == 1)
							{ arGuardHP[iActiveGuard] = 14; iChecked = 1; }
						if (InArea (322, 162, 322 + 14, 162 + 14) == 1)
							{ arGuardHP[iActiveGuard] = 15; iChecked = 1; }

						/*** Direction. ***/
						if (InArea (96, 203, 96 + 14, 203 + 14) == 1)
							{ arGuardDir[iActiveGuard] = 0; iChecked = 1; }
						if (InArea (111, 203, 111 + 14, 203 + 14) == 1)
							{ arGuardDir[iActiveGuard] = 1; iChecked = 1; }

						/*** Move to prince. ***/
						if (InArea (139, 244, 139 + 14, 244 + 14) == 1)
							{ arGuardMove[iActiveGuard] = 0; iChecked = 1; }
						if (InArea (154, 244, 154 + 14, 244 + 14) == 1)
							{ arGuardMove[iActiveGuard] = 1; iChecked = 1; }
					}
					if (iChecked == 1)
					{
						PlaySound ("wav/check_box.wav");
						iChanged++;
					}
					ShowGuards();
					break;
				case SDL_WINDOWEVENT:
					switch (event.window.event)
					{
						case SDL_WINDOWEVENT_EXPOSED:
							ShowGuards(); break;
						case SDL_WINDOWEVENT_CLOSE:
							Quit(); break;
						case SDL_WINDOWEVENT_FOCUS_GAINED:
							iActiveWindowID = iWindowID; break;
					}
					break;
				case SDL_QUIT:
					Quit(); break;
			}
		}

		/*** prevent CPU eating ***/
		gamespeed = REFRESH;
		while ((SDL_GetTicks() - looptime) < gamespeed)
		{
			SDL_Delay (10);
		}
		looptime = SDL_GetTicks();
	}
	PlaySound ("wav/popup_close.wav");
	ShowScreen (iScreen);
}
/*****************************************************************************/
void ShowGuards (void)
/*****************************************************************************/
{
	int iX;
	int iR, iG, iB;

	/*** background ***/
	ShowImage (imgguards, 0, 0, "imgguards", ascreen, iScale, 1);

	/*** OK ***/
	switch (iOKOn)
	{
		case 0: /*** off ***/
			ShowImage (imgok[1], 460, 410, "imgok[1]", ascreen, iScale, 1); break;
		case 1: /*** on ***/
			ShowImage (imgok[2], 460, 410, "imgok[2]", ascreen, iScale, 1); break;
	}

	/*** Edit this guard. ***/
	CenterNumber (ascreen, iActiveGuard, 155, 29, color_bl, 0);

	/*** Red. ***/
	switch (arGuardR[iActiveGuard])
	{
		case 0: iX = 130; break;
		case 1: iX = 145; break;
		case 2: iX = 160; break;
		case 3: iX = 175; break;
		default: iX = 130; break; /*** Fallback. ***/
	}
	ShowImage (imgselg, iX, 81, "imgselg", ascreen, iScale, 1);

	/*** Green. ***/
	switch (arGuardG[iActiveGuard])
	{
		case 0: iX = 274; break;
		case 1: iX = 289; break;
		case 2: iX = 304; break;
		case 3: iX = 319; break;
		default: iX = 274; break; /*** Fallback. ***/
	}
	ShowImage (imgselg, iX, 81, "imgselg", ascreen, iScale, 1);

	/*** Blue. ***/
	switch (arGuardB[iActiveGuard])
	{
		case 0: iX = 403; break;
		case 1: iX = 418; break;
		case 2: iX = 433; break;
		case 3: iX = 448; break;
		default: iX = 403; break; /*** Fallback. ***/
	}
	ShowImage (imgselg, iX, 81, "imgselg", ascreen, iScale, 1);

	/*** Approximate(!) color. ***/
	iR = (arGuardR[iActiveGuard] * 255) / 3;
	iG = (arGuardG[iActiveGuard] * 255) / 3;
	iB = (arGuardB[iActiveGuard] * 255) / 3;
	ColorRect (494, 66, 41, 29, iR, iG, iB);

	/*** Game level. ***/
	switch (arGuardGameLevel[iActiveGuard])
	{
		case 1: iX = 117; break;
		case 2: iX = 132; break;
		case 3: iX = 147; break;
		case 4: iX = 162; break;
		case 5: iX = 177; break;
		case 6: iX = 192; break;
		case 7: iX = 207; break;
		case 8: iX = 222; break;
		case 9: iX = 237; break;
		case 10: iX = 252; break;
		case 11: iX = 267; break;
		case 12: iX = 282; break;
		case 13: iX = 297; break;
		case 14: iX = 312; break;
		default: iX = 117; break; /*** Fallback. ***/
	}
	ShowImage (imgselg, iX, 121, "imgselg", ascreen, iScale, 1);

	/*** Hit points. ***/
	switch (arGuardHP[iActiveGuard])
	{
		case 0: iX = 97; break;
		case 1: iX = 112; break;
		case 2: iX = 127; break;
		case 3: iX = 142; break;
		case 4: iX = 157; break;
		case 5: iX = 172; break;
		case 6: iX = 187; break;
		case 7: iX = 202; break;
		case 8: iX = 217; break;
		case 9: iX = 232; break;
		case 10: iX = 247; break;
		case 11: iX = 262; break;
		case 12: iX = 277; break;
		case 13: iX = 292; break;
		case 14: iX = 307; break;
		case 15: iX = 322; break;
		default: iX = 97; break; /*** Fallback. ***/
	}
	ShowImage (imgselg, iX, 162, "imgselg", ascreen, iScale, 1);

	/*** Direction. ***/
	switch (arGuardDir[iActiveGuard])
	{
		case 0: iX = 96; break;
		case 1: iX = 111; break;
		default: iX = 96; break; /*** Fallback. ***/
	}
	ShowImage (imgselg, iX, 203, "imgselg", ascreen, iScale, 1);

	/*** Move to prince. ***/
	switch (arGuardMove[iActiveGuard])
	{
		case 0: iX = 139; break;
		case 1: iX = 154; break;
		default: iX = 139; break; /*** Fallback. ***/
	}
	ShowImage (imgselg, iX, 244, "imgselg", ascreen, iScale, 1);

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void AddressToRoomAndElem (void)
/*****************************************************************************/
{
	/* To make editing of buttons easier, the user is presented with the "room"
	 * and "elem" numbers instead of "address 1/2" and "address 2/2". This
	 * function is where the required conversion takes place.
	 */

	int iReplaced;
	char sReadW[10 + 2];

	/*** Used for looping. ***/
	int iLoopLevel;
	int iLoopRoom;
	int iLoopElem;
	int iLoopRoomS;
	int iLoopElemS;

	for (iLoopLevel = 1; iLoopLevel <= NR_LEVELS; iLoopLevel++)
	{
		for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
		{
			for (iLoopElem = 1; iLoopElem <= MAX_ELEM; iLoopElem++)
			{
				snprintf (sReadW, 10, "%02x%02x",
					arElements[iLoopLevel][iLoopRoom][iLoopElem][5],
					arElements[iLoopLevel][iLoopRoom][iLoopElem][4]);

				/*** Drop or raise button. ***/
				if ((arElements[iLoopLevel][iLoopRoom][iLoopElem][0] == 0x61) ||
					(arElements[iLoopLevel][iLoopRoom][iLoopElem][0] == 0x62))
				{
					iReplaced = 0;
					for (iLoopRoomS = 1; iLoopRoomS <= ROOMS; iLoopRoomS++)
					{
						for (iLoopElemS = 1; iLoopElemS <= MAX_ELEM; iLoopElemS++)
						{
							if (arElements[iLoopLevel][iLoopRoomS][iLoopElemS][9] ==
								(int)strtoul (sReadW, NULL, 16))
							{
								if (iDebug == 1)
								{
									printf ("[ INFO ] L %i,r %i,e %i replacing 0x%02X%02X with"
										" room %i, elem %i.\n", iLoopLevel, iLoopRoom, iLoopElem,
										arElements[iLoopLevel][iLoopRoom][iLoopElem][5],
										arElements[iLoopLevel][iLoopRoom][iLoopElem][4],
										iLoopRoomS, iLoopElemS);
								}
								arElements[iLoopLevel][iLoopRoom][iLoopElem][4] = iLoopRoomS;
								arElements[iLoopLevel][iLoopRoom][iLoopElem][5] = iLoopElemS;
								iReplaced = 1;
							}
						}
					}
					/*** Fallback. ***/
					if (iReplaced == 0)
					{
						arElements[iLoopLevel][iLoopRoom][iLoopElem][4] = 1;
						arElements[iLoopLevel][iLoopRoom][iLoopElem][5] = 1;
						printf ("[ WARN ] Using workaround for l %i,r %i,e %i!\n",
							iLoopLevel, iLoopRoom, iLoopElem);
					}
				}
			}
		}
	}
}
/*****************************************************************************/
