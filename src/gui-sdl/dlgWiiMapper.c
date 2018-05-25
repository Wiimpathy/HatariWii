/*
  Hatari - DlgWiiMapper.c

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  The Wii mapper dialog.
*/
const char DlgWiiMapper_fileid[] = "Hatari DlgWiiMapper.c : " __DATE__ " " __TIME__;

#include "main.h"
#include "configuration.h"
#include "dialog.h"
#include "sdlgui.h"
#include "screen.h"
#include "joy.h"
#include "log.h"
#include "keyboard.h"

/* Virtual keyboard options */
#define KEYDLG_CLOSE		98
#define KEYDLG_TIME			99
#define KEYDLG_MAP			100
#define KEYDLG_JOYMODE		101

/* BUTTONS/HATS MAPPING #1 */
#define JOY_WII1LEFT	2
#define JOY_WII1UP		3
#define JOY_WII1RIGHT   4
#define JOY_WII1DOWN	5
#define KEY_WII1LEFT	6
#define KEY_WII1UP		7
#define KEY_WII1RIGHT   8
#define KEY_WII1DOWN	9
#define JOY_WII1BUT1	10
#define JOY_WII1BUT2	11
#define JOY_WII1BUT3	12
#define KEY_WII1BUT1	13
#define KEY_WII1BUT2	14
#define KEY_WII1BUT3	15

/* BUTTONS/HATS MAPPING #2 */
#define JOY_WII2LEFT	16
#define JOY_WII2UP		17
#define JOY_WII2RIGHT   18
#define JOY_WII2DOWN	19
#define KEY_WII2LEFT	20
#define KEY_WII2UP		21
#define KEY_WII2RIGHT   22
#define KEY_WII2DOWN	23
#define JOY_WII2BUT1	24
#define JOY_WII2BUT2	25
#define JOY_WII2BUT3	26
#define KEY_WII2BUT1	27
#define KEY_WII2BUT2	28
#define KEY_WII2BUT3	29

#define KEYTOJOY_DEFAULT	30
#define KEYTOJOY_CLOSE		31
#define KEYTOJOY_PLAYER1	32
#define KEYTOJOY_PLAYER2	33

#define ARROWTOJOY1   34
#define ARROWTOJOY2   35
#define KEYTOJOY_UPDATEKEY 36

#define ROW1_ITEM1X	12
#define OFFSETROW1X	42
#define ROW1X ROW1_ITEM1X+OFFSETROW1X
#define ROW2_ITEM1X	13
#define OFFSETROW2X	26
#define ROW2X ROW2_ITEM1X+OFFSETROW2X
#define ROW3_ITEM1X 52
#define OFFSETROW3X	26
#define ROW3X ROW3_ITEM1X+1+OFFSETROW3X
#define ROW4_ITEM1X 58
#define OFFSETROW4X	26
#define ROW4X ROW4_ITEM1X+OFFSETROW4X
#define ROW5_ITEM1X 46
#define OFFSETROW5X	26
#define ROW5X ROW5_ITEM1X+2+OFFSETROW5X
#define ROW6X 48

#define ROW1Y 135
#define ROW2Y 176
#define ROW3Y 202
#define ROW4Y 228
#define ROW5Y 255
#define ROW6Y 282
#define ROW7Y 326

#define PLAYER1 1
#define PLAYER2 2

#define WIIMOTE		1
#define CLASSIC		2
#define GCPAD		3

#define STMOUSE     0
#define STJOYSTICK  1

/* The virtual keyboard dialog: */
static SGOBJ virtualkeybdlg[] =
{
	{ SGBOX, 0, 0, 0,0, 0,0, NULL },
	{ SGTEXT, 0, 0, 17,1, 16,1, "",0 }, // No key !!!
	{ SGKEY, SG_DEFAULT, 0, ROW1_ITEM1X,ROW1Y, 44,27, "F1",59 },
	{ SGKEY, 0, 0, ROW1X*1,ROW1Y, 44,27, "F2",60 },
	{ SGKEY, 0, 0, ROW1X*2,ROW1Y, 44,27, "F3",61 },
	{ SGKEY, 0, 0, ROW1X*3,ROW1Y, 44,27, "F4",62 },
	{ SGKEY, 0, 0, ROW1X*4,ROW1Y, 44,27, "F5",63 },
	{ SGKEY, 0, 0, ROW1X*5,ROW1Y, 44,27, "F6",64 },
	{ SGKEY, 0, 0, ROW1X*6,ROW1Y, 44,27, "F7",65 },
	{ SGKEY, 0, 0, ROW1X*7,ROW1Y, 44,27, "F8",66 },
	{ SGKEY, 0, 0, ROW1X*8,ROW1Y, 44,27, "F9",67 },
	{ SGKEY, 0, 0, ROW1X*9,ROW1Y, 44,27, "F10",68 },
	{ SGKEY, 0, 0, ROW2_ITEM1X,ROW2Y, 27,27, "Esc",1 },
	{ SGKEY, 0, 0, ROW2X*1,ROW2Y, 27,27, "1",2 }, // 13
	{ SGKEY, 0, 0, ROW2X*2,ROW2Y, 27,27, "2",3 },
	{ SGKEY, 0, 0, ROW2X*3,ROW2Y, 27,27, "3",4 },
	{ SGKEY, 0, 0, ROW2X*4,ROW2Y, 27,27, "4",5 },
	{ SGKEY, 0, 0, ROW2X*5,ROW2Y, 27,27, "5",6 },
	{ SGKEY, 0, 0, ROW2X*6,ROW2Y, 27,27, "6",7 },
	{ SGKEY, 0, 0, ROW2X*7,ROW2Y, 27,27, "7",8 },
	{ SGKEY, 0, 0, ROW2X*8,ROW2Y, 27,27, "8",9 },
	{ SGKEY, 0, 0, ROW2X*9,ROW2Y, 27,27, "9",10 }, // 21
	{ SGKEY, 0, 0, ROW2X*10,ROW2Y, 27,27, "0",11 }, // 22
	{ SGKEY, 0, 0, ROW2X*11,ROW2Y, 27,27, "-",12 },
	{ SGKEY, 0, 0, ROW2X*12,ROW2Y, 27,27, "=",13 },
	{ SGKEY, 0, 0, ROW2X*13,ROW2Y, 27,27, "`",41 },
	{ SGKEY, 0, 0, ROW2X*14,ROW2Y, 42,27, "Backspace",14 },
	{ SGKEY, 0, 0, 13,ROW3Y, 42,28, "Tab",15 },
	{ SGKEY, 0, 0, ROW3_ITEM1X,ROW3Y, 27,27, "Q",16 },
	{ SGKEY, 0, 0, ROW3X*1,ROW3Y, 27,27, "W",17 },
	{ SGKEY, 0, 0, ROW3X*2,ROW3Y, 27,27, "E",18 },
	{ SGKEY, 0, 0, ROW3X*3,ROW3Y, 27,27, "R",19 },
	{ SGKEY, 0, 0, ROW3X*4,ROW3Y, 27,27, "T",20 },
	{ SGKEY, 0, 0, ROW3X*5,ROW3Y, 27,27, "Y",21 },
	{ SGKEY, 0, 0, ROW3X*6,ROW3Y, 27,27, "U",22 },
	{ SGKEY, 0, 0, ROW3X*7,ROW3Y, 27,27, "I",23 },
	{ SGKEY, 0, 0, ROW3X*8,ROW3Y, 27,27, "O",24 },
	{ SGKEY, 0, 0, ROW3X*9,ROW3Y, 27,27, "P",25 },
	{ SGKEY, 0, 0, ROW3X*10,ROW3Y, 27,27, "[",26 },
	{ SGKEY, 0, 0, ROW3X*11,ROW3Y, 27,27, "]",27 },
	{ SGKEY, 0, 0, ROW3X*12,ROW3Y, 27,27, "Return",28 },
	{ SGKEY, 0, 0, ROW3X*13+1,ROW3Y, 27,27, "Del",83 },
	{ SGKEY, 0, 0, 13,ROW4Y, 48,28, "Control",29 },
	{ SGKEY, 0, 0, ROW4_ITEM1X,ROW4Y, 27,27, "A",30 },
	{ SGKEY, 0, 0, ROW4X*1,ROW4Y, 27,27, "S",31 },
	{ SGKEY, 0, 0, ROW4X*2,ROW4Y, 27,27, "D",32 },
	{ SGKEY, 0, 0, ROW4X*3,ROW4Y, 27,27, "F",33 },
	{ SGKEY, 0, 0, ROW4X*4,ROW4Y, 27,27, "G",34 },
	{ SGKEY, 0, 0, ROW4X*5,ROW4Y, 27,27, "H",35 },
	{ SGKEY, 0, 0, ROW4X*6,ROW4Y, 27,27, "J",36 },
	{ SGKEY, 0, 0, ROW4X*7,ROW4Y, 27,27, "K",37 },
	{ SGKEY, 0, 0, ROW4X*8,ROW4Y, 27,27, "L",38 },
	{ SGKEY, 0, 0, ROW4X*9,ROW4Y, 27,27, ";",39 },
	{ SGKEY, 0, 0, ROW4X*10,ROW4Y, 27,27, "'",40 },
	{ SGKEY, 0, 0, ROW4X*13-4,ROW4Y, 27,27, "#",43 },
	{ SGKEY, 0, 0, 13,ROW5Y, 36,28, "Left Shift",42 },
	{ SGKEY, 0, 0, ROW5_ITEM1X,ROW5Y, 27,27, "\\",96 },
	{ SGKEY, 0, 0, ROW5X*1,ROW5Y, 27,27, "Z",44 },
	{ SGKEY, 0, 0, ROW5X*2,ROW5Y, 27,27, "X",45 },
	{ SGKEY, 0, 0, ROW5X*3,ROW5Y, 27,27, "C",46 },
	{ SGKEY, 0, 0, ROW5X*4,ROW5Y, 27,27, "V",47 },
	{ SGKEY, 0, 0, ROW5X*5,ROW5Y, 27,27, "B",48 },
	{ SGKEY, 0, 0, ROW5X*6,ROW5Y, 27,27, "N",49 },
	{ SGKEY, 0, 0, ROW5X*7,ROW5Y, 27,27, "M",50 },
	{ SGKEY, 0, 0, ROW5X*8,ROW5Y, 27,27, ",",51 },
	{ SGKEY, 0, 0, ROW5X*9,ROW5Y, 27,27, ".",52 },
	{ SGKEY, 0, 0, ROW5X*10,ROW5Y, 27,27, "/",53 },
	{ SGKEY, 0, 0, ROW5X*11,ROW5Y, 41,28, "Right Shift",54 },
	{ SGKEY, 0, 0, ROW6X,ROW6Y, 41,28, "Alternate",56 },
	{ SGKEY, 0, 0, ROW6X+38,ROW6Y, 234,28, "Space",57 },
	{ SGKEY, 0, 0, ROW6X+269,ROW6Y, 41,28, "CapsLock",58 },
	/* Arrow keys */
	{ SGKEY, 0, 0, ROW2X*16,ROW2Y, 41,27, "Help",98 },
	{ SGKEY, 0, 0, ROW2X*16+40,ROW2Y, 41,27, "Undo",97 },
	{ SGKEY, 0, 0, ROW2X*16,ROW3Y, 27,27, "Insert",82 },
	{ SGKEY, 0, 0, ROW2X*17,ROW3Y, 27,27, "\x01",72 }, // up
	{ SGKEY, 0, 0, ROW2X*18,ROW3Y, 27,27, "Home",71 },
	{ SGKEY, 0, 0, ROW2X*16,ROW4Y, 27,27, "\x04",75 }, // left
	{ SGKEY, 0, 0, ROW2X*17,ROW4Y, 27,27, "\x02",80 }, // down
	{ SGKEY, 0, 0, ROW2X*18,ROW4Y, 27,27, "\x03",77 }, //right
	/* Numeric keypad */
	{ SGKEY, 0, 0, ROW3X*18,ROW2Y, 27,27, "(",99 },
	{ SGKEY, 0, 0, ROW3X*19,ROW2Y, 27,27, ")",100 },
	{ SGKEY, 0, 0, ROW3X*20,ROW2Y, 27,27, "/",101 },
	{ SGKEY, 0, 0, ROW3X*21,ROW2Y, 27,27, "*",102 },
	{ SGKEY, 0, 0, ROW3X*18,ROW3Y, 27,27, "7",8 },
	{ SGKEY, 0, 0, ROW3X*19,ROW3Y, 27,27, "8",9 },
	{ SGKEY, 0, 0, ROW3X*20,ROW3Y, 27,27, "9",10 },
	{ SGKEY, 0, 0, ROW3X*21,ROW3Y, 27,27, "-",74 },
	{ SGKEY, 0, 0, ROW3X*18,ROW4Y, 27,27, "4",5 },
	{ SGKEY, 0, 0, ROW3X*19,ROW4Y, 27,27, "5",6 },
	{ SGKEY, 0, 0, ROW3X*20,ROW4Y, 27,27, "6",7 },
	{ SGKEY, 0, 0, ROW3X*21,ROW4Y, 27,27, "+",78 },
	{ SGKEY, 0, 0, ROW3X*18,ROW5Y, 27,27, "1",2 },
	{ SGKEY, 0, 0, ROW3X*19,ROW5Y, 27,27, "2",3 },
	{ SGKEY, 0, 0, ROW3X*20,ROW5Y, 27,27, "3",4 },
	{ SGKEY, 0, 0, ROW3X*18,ROW6Y, 54,27, "0",11 },
	{ SGKEY, 0, 0, ROW3X*20,ROW6Y, 27,27, ".",113 },
	{ SGKEY, 0, 0, ROW3X*21,ROW5Y, 27,54, "Enter",114 },
	/* Options, Bind key */
	{ SGBOX, 0, 0, 0,20, 64,2, NULL },
	{ SGTEXT, SG_CANCEL, 0, 700,330, 60,40, "CLOSE" },  // hidden, back with B button
	{ SGCHECKBOX, 0, 0, 20,ROW7Y, 140,20, "Long press" },
	{ SGCHECKBOX, 0, 0, 200,ROW7Y, 120,20, "Bind key" },
	{ SGBUTTON, 0, 0, 418,ROW7Y, 104,20, "Joy mode"},
	{ -1, 0, 0, 0,0, 0,0, NULL }
};

enum
{
WiiBut1, WiiBut2, WiiButMinus,
ClassicButB, ClassicButX, ClassicButY,
GcButX, GcButY, GcButR
};

static char dlgWiiButton[6][2] = { "1", "2", "-", "1", "2", "-" };				// Wii button name
static char dlgSTkeyToHat[8][9] = { "?", "?", "?", "?", "?", "?", "?", "?" };	// Name of ST key mapped to the Wii d-pad
static char dlgSTkeyToButton[6][9] = { "?", "?", "?", "?", "?", "?" };			// Name of ST key mapped to the Wii button

static int Controller[2] = {1, 1};
static int Joy1CurrentButton1 = WiiBut1;
static int Joy1CurrentButton2 = WiiBut2;
static int Joy1CurrentButton3 = WiiButMinus;
static int Joy2CurrentButton1 = WiiBut1;
static int Joy2CurrentButton2 = WiiBut2;
static int Joy2CurrentButton3 = WiiButMinus;
static int WarningHats;
static int keydelay;
char CurrentKey[36];
int stjoy;
int JoyMode;
int keypressed = 0;
bool WiimoteConnected = true;

static void ST_GetKeyName(char *dlgButton, int STscancode);
static void Configure_JoyHat(char *dlgButton, int hat);
static void Configure_JoyButton(char *dlgButton, int button);
void Clear_JoyBinding(int button);
int Joy_CheckRepeat(void);


/* The Wii mapper dialog: */
static SGOBJ keystojoydlg[] =
{
	{ SGBOX, 0, 0, 0,0, 64,24, NULL },
	{ SGTEXT, 0, 0, 2,1, 5,1, CurrentKey },
	/* Player 1 */
	{ SGBUTTON, 0, 0, 6,13, 3,1, "<" },
	{ SGBUTTON, 0, 0, 8,11, 3,1, "^" },
	{ SGBUTTON, 0, 0, 10,13, 3,1, ">" },
	{ SGBUTTON, 0, 0, 8,15, 3,1, "v" },
	{ SGTEXT, 0, 0, 1,13, 3,1, dlgSTkeyToHat[0] },		// Player1 Left
	{ SGTEXT, 0, 0, 8,10, 6,1, dlgSTkeyToHat[1] },		// Player1 Up
	{ SGTEXT, 0, 0, 14,13, 6,1, dlgSTkeyToHat[2] },		// Player1 Right
	{ SGTEXT, 0, 0, 8,16, 6,1, dlgSTkeyToHat[3] },		// Player1 Down
	{ SGBUTTON, SG_DEFAULT, 0, 20,11, 3,1, dlgWiiButton[0] },
	{ SGBUTTON, 0, 0, 20,13, 3,1, dlgWiiButton[1] },
	{ SGBUTTON, 0, 0, 20,15, 3,1, dlgWiiButton[2] },
	{ SGTEXT, 0, 0, 24,11, 24,1, dlgSTkeyToButton[0] },
	{ SGTEXT, 0, 0, 24,13, 24,1, dlgSTkeyToButton[1] },
	{ SGTEXT, 0, 0, 24,15, 24,1, dlgSTkeyToButton[2] },
	/* Player 2 */
	{ SGBUTTON, 0, 0, 38,13, 3,1, "<" },
	{ SGBUTTON, 0, 0, 40,11, 3,1, "^" },
	{ SGBUTTON, 0, 0, 42,13, 3,1, ">" },
	{ SGBUTTON, 0, 0, 40,15, 3,1, "v" },
	{ SGTEXT, 0, 0, 36,13, 3,1, dlgSTkeyToHat[4] },		// Player2 Left
	{ SGTEXT, 0, 0, 40,10, 3,1, dlgSTkeyToHat[5] },		// Player2 Up
	{ SGTEXT, 0, 0, 46,13, 3,1, dlgSTkeyToHat[6] },		// Player2 Right
	{ SGTEXT, 0, 0, 40,16, 3,1, dlgSTkeyToHat[7] },		// Player2 Down
	{ SGBUTTON, 0, 0, 52,11, 3,1, dlgWiiButton[3] },
	{ SGBUTTON, 0, 0, 52,13, 3,1, dlgWiiButton[4] },
	{ SGBUTTON, 0, 0, 52,15, 3,1, dlgWiiButton[5] },
	{ SGTEXT, 0, 0, 56,11, 24,1, dlgSTkeyToButton[3] },
	{ SGTEXT, 0, 0, 56,13, 24,1, dlgSTkeyToButton[4] },
	{ SGTEXT, 0, 0, 56,15, 24,1, dlgSTkeyToButton[5] },
	{ SGBUTTON, 0, 0, 3, 21, 14,1, "Reset keys" },
	{ SGBUTTON, SG_CANCEL, 0, 55, 20, 5,3, "CLOSE" },
	/* Player 1 & Player 2 buttons: switch controllers */
	{ SGWIIMOTE1, SG_EXIT, 0, 6,3, 10, 6, "" },
	{ SGWIIMOTE2, SG_EXIT, 0, 38,3, 10,6, "" },
	/* ST arrow keys emulation */
	{ SGCHECKBOX, SG_EXIT, 0, 4,18, 18,1, "Arrows on D-pad" },
	{ SGCHECKBOX, SG_EXIT, 0, 37,18, 18,1, "Arrows on D-pad" },
	{ SGBUTTON, 0, 0, 55, 70, 16,3, "Update keyname" },	// Hidden button, change name after Clear_JoyBinding()
	{ -1, 0, 0, 0,0, 0,0, NULL }
};

static char dlgwarn[4][50];

/* The warning dialog: */
static SGOBJ warningarrowsdlg[] =
{
	{ SGBOX, 0, 0, 0,0, 40,12, NULL },
	{ SGTEXT, 0, 0, 16,1, 12,1, "WARNING" },
	{ SGTEXT, 0, 0, 16,2, 12,1, "=======" },
	{ SGTEXT, 0, 0, 1,4, 38,1, dlgwarn[0] },
	{ SGTEXT, 0, 0, 1,5, 38,1, dlgwarn[1] },
	{ SGTEXT, 0, 0, 1,7, 38,1, dlgwarn[2] },
	{ SGTEXT, 0, 0, 1,8, 38,1, dlgwarn[3] },
	{ SGBUTTON, SG_DEFAULT, 0, 16,10, 8,1, "OK" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
};

/*-----------------------------------------------------------------------*/
/**
 * Show the joystick hat warning dialog:
 */
static void DlgWiiMapper_Warning(char *tempstr)
{

	bool clearhat;

	if (strcmp(tempstr, "arrow") == 0)
	{
		sprintf(dlgwarn[0], "When this option is enabled, the D-pad");
		sprintf(dlgwarn[1], "acts like the Atari ST arrow keys.");
		sprintf(dlgwarn[2], "Disable this option to use the D-pad");
		sprintf(dlgwarn[3], "as the ST joystick!");
	}
	else if (strcmp(tempstr, "hat") == 0)
	{
		sprintf(dlgwarn[0], "When a key is assigned to the D-pad,");
		sprintf(dlgwarn[1], "the Atari ST joystick is disabled.");
		sprintf(dlgwarn[2], "Clear the key with button '-'/'Z'");
		sprintf(dlgwarn[3], "or 'Reset keys'");
	}
	else if (strcmp(tempstr, "nohat") == 0)
	{
		sprintf(dlgwarn[0], "You can't assign a key to the D-pad!");
		sprintf(dlgwarn[1], "Disable 'Arrows on D-pad' first.");
		sprintf(dlgwarn[2],"%s", "");
		sprintf(dlgwarn[3],"%s", "");
	}

	/* Show a query for joymode else call the warning dialogs */
	if  (strcmp(tempstr, "joymode") == 0)
	{
		clearhat = DlgAlert_Query("ST keys are mapped to the D-pad.\nClear them?");
		
		/* Clear player 1 & 2 hats bindings */
		if (clearhat)
		{
			Joy1HatMode = 0;
			Joy2HatMode = 0;
			Clear_JoyBinding(JOY_WII1LEFT);
			Clear_JoyBinding(JOY_WII1RIGHT);
			Clear_JoyBinding(JOY_WII1UP);
			Clear_JoyBinding(JOY_WII1DOWN);
			Clear_JoyBinding(JOY_WII2LEFT);
			Clear_JoyBinding(JOY_WII2RIGHT);
			Clear_JoyBinding(JOY_WII2UP);
			Clear_JoyBinding(JOY_WII2DOWN);
		}
	}
	else
	{
		SDLGui_CenterDlg(warningarrowsdlg);
		SDLGui_DoDialog(warningarrowsdlg, NULL,false);
	}
}


/*---------------------------------------------------------------------------*/
/**
 * Translate ST scancode to ST key name, change key name for the given button. 
 */
static void ST_GetKeyName(char *dlgButton, int STscancode)
{
	int Vkey;
	int VkeyMax = 99;
	char *temp;
	int lengthP1Leftstr;
	int lengthP1Upstr;
	int lengthP1Rightstr;
	int lengthP1Downstr;

	int lengthP2Leftstr;
	int lengthP2Upstr;
	int lengthP2Rightstr;
	int lengthP2Downstr;


	temp = malloc(56);
	if (!temp)
		perror("Error while allocating temporary memory in ST_GetKeyName()");

	/* Scan virtualkeybdlg[] to find the name corresponding to the scancode.
	 * virtualkeybdlg[k].shortcut is the ST scancode, virtualkeybdlg[k].txt is
	 * the ST key name.*/
	for(Vkey = 1; Vkey < VkeyMax; Vkey++)
	{
		if(virtualkeybdlg[Vkey].shortcut == STscancode )
		{
			/* Differentiate left and right shift */
			if(virtualkeybdlg[Vkey].shortcut == 42)
			{
				sprintf(dlgButton,  "Lshift");
				break;
			}
			if(virtualkeybdlg[Vkey].shortcut == 54)
			{
				sprintf(dlgButton,  "Rshift");
				break;
			}

			sprintf(temp,  "%s", virtualkeybdlg[Vkey].txt);

			/* Truncate, rename keys */
			if (strcmp(temp, "Backspace") == 0)
				sprintf(dlgButton,  "Bspace");
			else if (strcmp(temp, "Alternate") == 0)
				sprintf(dlgButton,  "Alt");
			else if (strcmp(temp, "CapsLock") == 0)
				sprintf(dlgButton,  "Cap.");
			else if (strcmp(temp, "Control") == 0)
				sprintf(dlgButton,  "Ctrl");
			else
				sprintf(dlgButton,  "%s", virtualkeybdlg[Vkey].txt);
			break;
		}
	}

	/* Left/right hats strings length to calculate x position */
	lengthP1Leftstr = (int)strlen(dlgSTkeyToHat[0]);
	lengthP1Upstr = (int)strlen(dlgSTkeyToHat[1]);
	lengthP1Rightstr = (int)strlen(dlgSTkeyToHat[2]);
	lengthP1Downstr = (int)strlen(dlgSTkeyToHat[3]);

	lengthP2Leftstr = (int)strlen(dlgSTkeyToHat[4]);
	lengthP2Upstr = (int)strlen(dlgSTkeyToHat[5]);
	lengthP2Rightstr = (int)strlen(dlgSTkeyToHat[6]);
	lengthP2Downstr = (int)strlen(dlgSTkeyToHat[7]);

	/* Player 1 left hat text: x position */
	switch (lengthP1Leftstr)
	{
		case 1:
			keystojoydlg[KEY_WII1LEFT].x = 4;
			break;
		case 3:
			keystojoydlg[KEY_WII1LEFT].x = 2;
			break;
		case 4:
			keystojoydlg[KEY_WII1LEFT].x = 1;
			break;
		case 5:
		case 6:
			keystojoydlg[KEY_WII1LEFT].x = 0;
			break;
	}
	/* Player 1 Up hat text: x position */
	switch (lengthP1Upstr)
	{
		case 1:
			keystojoydlg[KEY_WII1UP].x = 9;
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			keystojoydlg[KEY_WII1UP].x = 8;
			break;
	}
	/* Player 1 right hat text: x position */
	switch (lengthP1Rightstr)
	{
		case 1:
		case 3:
		case 4:
		case 5:
			keystojoydlg[KEY_WII1RIGHT].x = 14;
			break;
		case 6:
			keystojoydlg[KEY_WII1RIGHT].x = 13;
			break;
	}
	/* Player 1 Down hat text: x position */
	switch (lengthP1Downstr)
	{
		case 1:
			keystojoydlg[KEY_WII1DOWN].x = 9;
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			keystojoydlg[KEY_WII1DOWN].x = 8;
			break;
	}

	/* Player 2 left hat text: x position */
	switch (lengthP2Leftstr)
	{
		case 1:
			keystojoydlg[KEY_WII2LEFT].x = 36;
			break;
		case 3:
			keystojoydlg[KEY_WII2LEFT].x = 34;
			break;
		case 4:
			keystojoydlg[KEY_WII2LEFT].x = 33;
			break;
		case 5:
		case 6:
			keystojoydlg[KEY_WII2LEFT].x = 32;
			break;
	}
	/* Player 2 Up hat text: x position */
	switch (lengthP2Upstr)
	{
		case 1:
			keystojoydlg[KEY_WII2UP].x = 41;
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			keystojoydlg[KEY_WII2UP].x = 40;
			break;
	}
	/* Player 2 right hat text: x position */
	switch (lengthP2Rightstr)
	{
		case 1:
		case 3:
		case 4:
		case 5:
			keystojoydlg[KEY_WII2RIGHT].x = 46;
			break;
		case 6:
			keystojoydlg[KEY_WII2RIGHT].x = 45;
			break;
	}
	/* Player 2 Down hat text: x position */
	switch (lengthP2Downstr)
	{
		case 1:
			keystojoydlg[KEY_WII2DOWN].x = 41;
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			keystojoydlg[KEY_WII2DOWN].x = 40;
			break;
	}
	free(temp);
}


/*-----------------------------------------------------------------------*/
/**
 * Checks and toggles hat mode between ST joystick, or ST key mode.
 * When one key, or more, is mapped to one of the Wii controller's
 * hat, disable the Atari ST joystick directions.
 */
static void Check_HatMode(int Player)
{
	int i;

	/* Wii controller 1 */
	if (Player == 1)
	{
		for (i=0; i<4; i++)
		{
			/* Check for at least one assigned key to determine mode */
			if (ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii1Hat[i] != 0)
			{
				Joy1HatMode = 1; // Player 1 Hat ST key mode
				break;
			}
			else
			{
				Joy1HatMode = 0; // Player 1 Hat ST joystick mode
			}
		}
	}
	/* Wii controller 2 */
	else if (Player == 2)
	{
		for (i=0; i<4; i++)
		{
			/* Check for at least one assigned key to determine mode */
			if (ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii2Hat[i] != 0)
			{
				Joy2HatMode = 1; // Player 2 Hat ST key mode
				break;
			}
			else
			{
				Joy2HatMode = 0; // Player 2 Hat ST joystick mode
			}
		}
	}	
}


/*-----------------------------------------------------------------------*/
/**
 * Bind ST key to Wii controller's hat.
 */
static void Configure_JoyHat(char *dlgButton, int hat)
{
	WarningHats = ConfigureParams.Joysticks.Joy[0].nArrowtoJoy;

	/* Arrow keys enabled on controller #1 */
	if (WarningHats == 0)
	{
		/* Hat joy #1 selected: we can't assign a key in arrow mode */
		if (hat <= JOY_WII1DOWN)
			DlgWiiMapper_Warning("nohat");

		/* Hat joy #2 selected: update key name, display warning */
		if (hat >= JOY_WII2LEFT && hat <= JOY_WII2DOWN)
		{
			ST_GetKeyName(dlgButton, keypressed);
			DlgWiiMapper_Warning("hat");
		}

		Check_HatMode(2);

		switch (hat)
		{
			case JOY_WII2LEFT:
				for (stjoy=0; stjoy<6; stjoy++)
					ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Hat[0] = keypressed;
				break;
			case JOY_WII2UP:
				for (stjoy=0; stjoy<6; stjoy++)
					ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Hat[1] = keypressed;
				break;
			case JOY_WII2RIGHT:
				for (stjoy=0; stjoy<6; stjoy++)
					ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Hat[2] = keypressed;
				break;
			case JOY_WII2DOWN:
				for (stjoy=0; stjoy<6; stjoy++)
					ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Hat[3] = keypressed;
				break;
			default:
				break;
		}
	}
	/* Arrow keys enabled on controller #2 */
	else if (WarningHats == 1)
	{
		/* Hat joy #2 selected: we can't assign a key in arrow mode */
		if (hat >= JOY_WII2LEFT && hat <= JOY_WII2DOWN)
			DlgWiiMapper_Warning("nohat");

		/* Hat joy #1 selected: update key name, display warning */
		if (hat <= JOY_WII1DOWN)
		{
			ST_GetKeyName(dlgButton, keypressed);
			DlgWiiMapper_Warning("hat");
		}

		Check_HatMode(1);

		switch (hat)
		{
			case JOY_WII1LEFT:
				for (stjoy=0; stjoy<6; stjoy++)
					ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Hat[0] = keypressed;
				Check_HatMode(1);
				break;
			case JOY_WII1UP:
				for (stjoy=0; stjoy<6; stjoy++)
					ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Hat[1] = keypressed;
				Check_HatMode(1);
				break;
			case JOY_WII1RIGHT:
				for (stjoy=0; stjoy<6; stjoy++)
					ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Hat[2] = keypressed;
				Check_HatMode(1);
				break;
			case JOY_WII1DOWN:
				for (stjoy=0; stjoy<6; stjoy++)
					ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Hat[3] = keypressed;
				Check_HatMode(1);
				break;
			default:
				break;
		}
	}
	/* Arrow keys disabled on both controllers */
	else if (WarningHats == -1)
	{
		DlgWiiMapper_Warning("hat");

		ST_GetKeyName(dlgButton, keypressed);

		switch (hat)
		{
			case JOY_WII1LEFT:
				for (stjoy=0; stjoy<6; stjoy++)
					ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Hat[0] = keypressed;
				break;
			case JOY_WII1UP:
				for (stjoy=0; stjoy<6; stjoy++)
					ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Hat[1] = keypressed;
				break;
			case JOY_WII1RIGHT:
				for (stjoy=0; stjoy<6; stjoy++)
					ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Hat[2] = keypressed;
				break;
			case JOY_WII1DOWN:
				for (stjoy=0; stjoy<6; stjoy++)
					ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Hat[3] = keypressed;
				break;
			case JOY_WII2LEFT:
				for (stjoy=0; stjoy<6; stjoy++)
					ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Hat[0] = keypressed;
				break;
			case JOY_WII2UP:
				for (stjoy=0; stjoy<6; stjoy++)
					ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Hat[1] = keypressed;
				break;
			case JOY_WII2RIGHT:
				for (stjoy=0; stjoy<6; stjoy++)
					ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Hat[2] = keypressed;
				break;
			case JOY_WII2DOWN:
				for (stjoy=0; stjoy<6; stjoy++)
					ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Hat[3] = keypressed;
				break;
			default:
				break;
		}
	}
}


/*-----------------------------------------------------------------------*/
/**
 * Bind ST key to Wii controller's button.
 */
static void Configure_JoyButton(char *dlgButton, int button)
{
	ST_GetKeyName(dlgButton, keypressed);

	switch (button)
	{
		case JOY_WII1BUT1:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Button[Joy1CurrentButton1] = keypressed;
			break;
		case JOY_WII1BUT2:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Button[Joy1CurrentButton2] = keypressed;
			break;
		case JOY_WII1BUT3:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Button[Joy1CurrentButton3] = keypressed;
			break;
		case JOY_WII2BUT1:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Button[Joy2CurrentButton1] = keypressed;
			break;
		case JOY_WII2BUT2:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Button[Joy2CurrentButton2] = keypressed;
			break;
		case JOY_WII2BUT3:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Button[Joy2CurrentButton3] = keypressed;
			break;
	}
}


/*-----------------------------------------------------------------------*/
/**
 * Set default key bindings.
 */
static void Configure_JoySetDefault(void)
{
	int i, j;

	for(stjoy=0;stjoy<6;stjoy++)
	{
		/* Wii controller 1 key bindings*/
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Hat[0] = 0;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Hat[1] = 0;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Hat[2] = 0;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Hat[3] = 0;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Button[0] = 57;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Button[1] = 0;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Button[2] = 0;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Button[3] = 57;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Button[4] = 0;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Button[5] = 0;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Button[6] = 57;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Button[7] = 0;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Button[8] = 0;
		/* Wii controller 2 key bindings*/
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Hat[0] = 0;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Hat[1] = 0;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Hat[2] = 0;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Hat[3] = 0;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Button[0] = 57;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Button[1] = 0;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Button[2] = 0;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Button[3] = 57;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Button[4] = 0;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Button[5] = 0;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Button[6] = 57;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Button[7] = 0;
		ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Button[8] = 0;
	}
	/* Display default key names */
	for (i=0; i<8; i++)
		ST_GetKeyName(dlgSTkeyToHat[i], 0);
	for (i=0; i<4; i++)
		ST_GetKeyName(dlgSTkeyToButton[i], ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii1Button[i]);
	for (i=3, j=0; i<6; i++,j++)
		ST_GetKeyName(dlgSTkeyToButton[i], ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii2Button[j]);

	/* Deselect joy #1 and joy #2 arrows radio button */
	keystojoydlg[ARROWTOJOY1].state &= ~SG_SELECTED;
	keystojoydlg[ARROWTOJOY2].state &= ~SG_SELECTED;
}


/*-----------------------------------------------------------------------*/
/**
 * Clear the selected key binding.
 */
void Clear_JoyBinding(int button)
{
	/* Delete actual key value, and clear the name in dialog */
	switch (button)
	{
		/* Controller #1 button */
		case JOY_WII1BUT1:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Button[Joy1CurrentButton1] = 0;
			ST_GetKeyName(dlgSTkeyToButton[0], 0);
			break;
		case JOY_WII1BUT2:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Button[Joy1CurrentButton2] = 0;
			ST_GetKeyName(dlgSTkeyToButton[1], 0);
			break;
		case JOY_WII1BUT3:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Button[Joy1CurrentButton3] = 0;
			ST_GetKeyName(dlgSTkeyToButton[2], 0);
			break;
		/* Controller #2 button */
		case JOY_WII2BUT1:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Button[Joy2CurrentButton1] = 0;
			ST_GetKeyName(dlgSTkeyToButton[3], 0);
			break;
		case JOY_WII2BUT2:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Button[Joy2CurrentButton2] = 0;
			ST_GetKeyName(dlgSTkeyToButton[4], 0);
			break;
		case JOY_WII2BUT3:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Button[Joy2CurrentButton3] = 0;
			ST_GetKeyName(dlgSTkeyToButton[5], 0);
			break;
		/* Controller #1 hat */
		case JOY_WII1LEFT:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Hat[0] = 0;
			ST_GetKeyName(dlgSTkeyToHat[0], 0);
			break;
		case JOY_WII1UP:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Hat[1] = 0;
			ST_GetKeyName(dlgSTkeyToHat[1], 0);
			break;
		case JOY_WII1RIGHT:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Hat[2] = 0;
			ST_GetKeyName(dlgSTkeyToHat[2], 0);
			break;
		case JOY_WII1DOWN:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Hat[3] = 0;
			ST_GetKeyName(dlgSTkeyToHat[3], 0);
			break;
		/* Controller #2 hat */
		case JOY_WII2LEFT:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Hat[0] = 0;
			ST_GetKeyName(dlgSTkeyToHat[4], 0);
			break;
		case JOY_WII2UP:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Hat[1] = 0;
			ST_GetKeyName(dlgSTkeyToHat[5], 0);
			break;
		case JOY_WII2RIGHT:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Hat[2] = 0;
			ST_GetKeyName(dlgSTkeyToHat[6], 0);
			break;
		case JOY_WII2DOWN:
			for (stjoy=0; stjoy<6; stjoy++)
				ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Hat[3] = 0;
			ST_GetKeyName(dlgSTkeyToHat[7], 0);
			break;
	}
}

/*-----------------------------------------------------------------------*/
/**
 * Set ST joystick mode in port 1: mouse or joystick.
 */
static void Joy_MouseMode(void)
{
	static JOYSTICKMODE saved = JOYSTICK_DISABLED;
	JOYSTICKMODE state;

	state = ConfigureParams.Joysticks.Joy[1].nJoystickMode;

	if (state == JOYSTICK_KEYBOARD || state == JOYSTICK_REALSTICK)
		ConfigureParams.Joysticks.Joy[1].nJoystickMode = saved;
	else
		ConfigureParams.Joysticks.Joy[1].nJoystickMode = JOYSTICK_REALSTICK;

	JoyMode = ConfigureParams.Joysticks.Joy[1].nJoystickMode;

	if (JoyMode == STMOUSE)
	{
		/* Disable 'Turbo mode' in mouse mode or else no double click. */
		ConfigureParams.System.bFastForward = false;

		/* Enable ST arrows. Allows cursor movement with Alt key */
		//Joy1HatMode = 2;
	}
	else if (JoyMode == STJOYSTICK)
	{
		WiimoteConnected = Joy_CheckWiimote();

		if (!WiimoteConnected)
		{
			ConfigureParams.Joysticks.Joy[1].nJoyId = 4;
		}
		else
		{
			ConfigureParams.Joysticks.Joy[1].nJoyId = 0;
		}

		Check_HatMode(1);

		/* Look for mapped key to d-pad. Display a query to clear them. */
		if (Joy1HatMode == 1)
			DlgWiiMapper_Warning("joymode");

		/* Disable ST arrows. */
		ConfigureParams.Joysticks.Joy[0].nArrowtoJoy = -1;

		/* Deselect joy #1 & #2 arrows radio buttons */
		keystojoydlg[ARROWTOJOY1].state &= ~SG_SELECTED;
		keystojoydlg[ARROWTOJOY2].state &= ~SG_SELECTED;


	}
}

/*-----------------------------------------------------------------------*/
/**
 * Set active Wii controller
 */
static void Joy_ActiveController(int player)
{
	int i, j;

	if (player == PLAYER1)
	{
		if (Controller[0] == WIIMOTE)
		{
			keystojoydlg[JOY_WII1BUT1].txt = "1";
			keystojoydlg[JOY_WII1BUT2].txt = "2";
			keystojoydlg[JOY_WII1BUT3].txt = "-";

			for (i=0; i<3; i++)
				ST_GetKeyName(dlgSTkeyToButton[i], ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii1Button[i]);

			Joy1CurrentButton1 = WiiBut1;
			Joy1CurrentButton2 = WiiBut2;
			Joy1CurrentButton3 = WiiButMinus;

			ConfigureParams.Joysticks.Joy[1].nJoyId = 0; // ST joy 1 Wiimote 1
		}
		if (Controller[0] == CLASSIC)
		{
			keystojoydlg[JOY_WII1BUT1].txt = "B";
			keystojoydlg[JOY_WII1BUT2].txt = "X";
			keystojoydlg[JOY_WII1BUT3].txt = "Y";

			for (i=0, j=3; i<3; i++, j++)
				ST_GetKeyName(dlgSTkeyToButton[i], ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii1Button[j]);

			Joy1CurrentButton1 = ClassicButB;
			Joy1CurrentButton2 = ClassicButX;
			Joy1CurrentButton3 = ClassicButY;

			ConfigureParams.Joysticks.Joy[1].nJoyId = 0; // ST joy 1 Wiimote 1
		}
		if (Controller[0] == GCPAD)
		{
			keystojoydlg[JOY_WII1BUT1].txt = "X";
			keystojoydlg[JOY_WII1BUT2].txt = "Y";
			keystojoydlg[JOY_WII1BUT3].txt = "R";

			for (i=0, j=6; i<3; i++, j++)
				ST_GetKeyName(dlgSTkeyToButton[i], ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii1Button[j]);

			Joy1CurrentButton1 = GcButX;
			Joy1CurrentButton2 = GcButY;
			Joy1CurrentButton3 = GcButR;

			ConfigureParams.Joysticks.Joy[1].nJoyId = 4; // ST joy 1 GCpad 1
		}
	}
	else if (player == PLAYER2)
	{
		if (Controller[1] == WIIMOTE)
		{
			keystojoydlg[JOY_WII2BUT1].txt = "1";
			keystojoydlg[JOY_WII2BUT2].txt = "2";
			keystojoydlg[JOY_WII2BUT3].txt = "-";

			for (i=3, j=0; i<6; i++, j++)
				ST_GetKeyName(dlgSTkeyToButton[i], ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii2Button[j]);

			Joy2CurrentButton1 = WiiBut1;
			Joy2CurrentButton2 = WiiBut2;
			Joy2CurrentButton3 = WiiButMinus;

			ConfigureParams.Joysticks.Joy[0].nJoyId = 1; // ST joy 0 Wiimote 2
		}
		if (Controller[1] == CLASSIC)
		{
			keystojoydlg[JOY_WII2BUT1].txt = "B";
			keystojoydlg[JOY_WII2BUT2].txt = "X";
			keystojoydlg[JOY_WII2BUT3].txt = "Y";

			for (i=3; i<6; i++)
				ST_GetKeyName(dlgSTkeyToButton[i], ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii2Button[i]);

			Joy2CurrentButton1 = ClassicButB;
			Joy2CurrentButton2 = ClassicButX;
			Joy2CurrentButton3 = ClassicButY;

			ConfigureParams.Joysticks.Joy[0].nJoyId = 1; // ST joy 0 Wiimote 2
		}
		if (Controller[1] == GCPAD)
		{
			keystojoydlg[JOY_WII2BUT1].txt = "X";
			keystojoydlg[JOY_WII2BUT2].txt = "Y";
			keystojoydlg[JOY_WII2BUT3].txt = "R";

			for (i=3, j=6; i<6; i++, j++)
				ST_GetKeyName(dlgSTkeyToButton[i], ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii2Button[j]);

			Joy2CurrentButton1 = GcButX;
			Joy2CurrentButton2 = GcButY;
			Joy2CurrentButton3 = GcButR;

			ConfigureParams.Joysticks.Joy[0].nJoyId = 5; // ST joy 0 GCpad 2
		}
	}
}

/*-----------------------------------------------------------------------*/
/**
 * Set time delay 
 */
void Joy_SetRepeat(int repeat)
{
	keydelay = repeat;
}

/*-----------------------------------------------------------------------*/
/**
 * Check time delay 
 */
int Joy_CheckRepeat(void)
{
	return keydelay;
}

/*-----------------------------------------------------------------------*/
/**
 * Assign ST keys to Wii controllers buttons and hats.
 */
static void dlgWiiMapper_KeyBinding(void)
{
	int ret;
	int i, j;

	for (i=0, j=4; i<4; i++,j++)
	{
		ST_GetKeyName(dlgSTkeyToHat[i], ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii1Hat[i]);
		ST_GetKeyName(dlgSTkeyToHat[j], ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii2Hat[i]);
	}

	for (i=0; i<3; i++)
		ST_GetKeyName(dlgSTkeyToButton[i], ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii1Button[i]);

	for (i=3, j=0; i<6; i++, j++)
		ST_GetKeyName(dlgSTkeyToButton[i], ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii2Button[j]);


	if (!WiimoteConnected)
	{
		Controller[0] = GCPAD;
		keystojoydlg[KEYTOJOY_PLAYER1].type = 11;
	}
	
	/* Set active controller */
	Joy_ActiveController(PLAYER1);
	Joy_ActiveController(PLAYER2);

	SDLGui_CenterDlg(keystojoydlg);	

	/* Clear the screen */
	SDL_FillRect(sdlscrn, NULL, SDL_MapRGB(sdlscrn->format, 0, 0, 0));

	/* Draw and process the Wii Mapper dialog */
	do
	{
		ret = SDLGui_DoDialog(keystojoydlg, NULL, false);

		/* ST arrows are disabled for both controllers */
		if (keystojoydlg[ARROWTOJOY1].state == 0 && keystojoydlg[ARROWTOJOY2].state == 0)
		{
			ConfigureParams.Joysticks.Joy[0].nArrowtoJoy = -1;
			WarningHats = ConfigureParams.Joysticks.Joy[0].nArrowtoJoy;
		}

		switch (ret)
		{
			case ARROWTOJOY1:
				/* Deselect joy #2 arrows radio button */
				keystojoydlg[ARROWTOJOY2].state &= ~SG_SELECTED;
				SDL_Delay(100);

				/* Display a warning for the dpad as arrow keys */
				if(keystojoydlg[ARROWTOJOY1].state & SG_SELECTED)
				{
					DlgWiiMapper_Warning("arrow");

					/* Use ST arrow keys on joy #1 dpad */
					ConfigureParams.Joysticks.Joy[0].nArrowtoJoy = 0;
					Joy1HatMode = 2; // Player1 Hat ST arrows mode
		
					/* Clear joy #1 hat strings */
					for (i=0; i<4; i++)
						ST_GetKeyName(dlgSTkeyToHat[i], 0);

					/* Erase any joy #1 key hat bindings */
					for (stjoy=0; stjoy<6; stjoy++)
					{
						for (i=0; i<4; i++)
							ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii1Hat[i] = 0;		
					}

					/* Check joy #2 hat mode*/
 					Check_HatMode(2);

					/* Save arrow keys emulation state */
					WarningHats = ConfigureParams.Joysticks.Joy[0].nArrowtoJoy;
				}
				break;
			case ARROWTOJOY2:
				/* Deselect joy #1 arrows radio button */
				keystojoydlg[ARROWTOJOY1].state &= ~SG_SELECTED;
				SDL_Delay(100);

				/* Display a warning for the dpad as arrow keys */
				if(keystojoydlg[ARROWTOJOY2].state & SG_SELECTED)
				{
					DlgWiiMapper_Warning("arrow");

					/* Use ST arrow keys on joy #2 dpad */
					ConfigureParams.Joysticks.Joy[0].nArrowtoJoy = 1;
					Joy2HatMode = 2; // Player2 Hat ST arrows mode

					/* Clear joy #2 hat strings */
					for (i=4; i<8; i++)
						ST_GetKeyName(dlgSTkeyToHat[i], 0);

					/* Erase any joy #2 key hat bindings */
					for (stjoy=0; stjoy<6; stjoy++)
					{
						for (i=0; i<4; i++)
							ConfigureParams.Joysticks.Joy[stjoy].nKeytoJoyWii2Hat[i] = 0;
					}

					/* Check joy #1 hat mode*/
 					Check_HatMode(1);

					/* Save arrow keys emulation state */
					WarningHats = ConfigureParams.Joysticks.Joy[0].nArrowtoJoy;
				}
				break;
			/* Controller 1 : Wiimote 1, Classic controller 1, Gamecube Pad 1 */
			case KEYTOJOY_PLAYER1:
				Controller[0] += 1;
				keystojoydlg[KEYTOJOY_PLAYER1].type += 1;

				if (keystojoydlg[KEYTOJOY_PLAYER1].type > 11 )
				{
					Controller[0]=1;
					keystojoydlg[KEYTOJOY_PLAYER1].type = 9;
				}

				/* Set active controller */
				Joy_ActiveController(PLAYER1);
				break;
			/* Controller 2 : Wiimote 2, Classic controller 2, Gamecube Pad 2 */
			case KEYTOJOY_PLAYER2:
				Controller[1] += 1;
				keystojoydlg[KEYTOJOY_PLAYER2].type += 1;

				if (keystojoydlg[KEYTOJOY_PLAYER2].type > 14 )
				{
					Controller[1]=1;
					keystojoydlg[KEYTOJOY_PLAYER2].type = 12;
				}

				/* Set active controller */
				Joy_ActiveController(PLAYER2);
				break;
			/* Configure controller 1 Hat/buttons */
			case JOY_WII1LEFT:
				Configure_JoyHat(dlgSTkeyToHat[0], JOY_WII1LEFT);
				break;
			case JOY_WII1UP:
				Configure_JoyHat(dlgSTkeyToHat[1], JOY_WII1UP);
				break;
			case JOY_WII1RIGHT:
				Configure_JoyHat(dlgSTkeyToHat[2], JOY_WII1RIGHT);
				break;
			case JOY_WII1DOWN:
				Configure_JoyHat(dlgSTkeyToHat[3], JOY_WII1DOWN);
				break;
			case JOY_WII1BUT1:
				Configure_JoyButton(dlgSTkeyToButton[0], JOY_WII1BUT1);
				break;
			case JOY_WII1BUT2:
				Configure_JoyButton(dlgSTkeyToButton[1], JOY_WII1BUT2);			
				break;
			case JOY_WII1BUT3:
				Configure_JoyButton(dlgSTkeyToButton[2], JOY_WII1BUT3);
				break;
			/* Configure controller 2 Hat/buttons */
			case JOY_WII2LEFT:
				Configure_JoyHat(dlgSTkeyToHat[4], JOY_WII2LEFT);
				break;
			case JOY_WII2UP:
				Configure_JoyHat(dlgSTkeyToHat[5], JOY_WII2UP);
				break;
			case JOY_WII2RIGHT:
				Configure_JoyHat(dlgSTkeyToHat[6], JOY_WII2RIGHT);
				break;
			case JOY_WII2DOWN:
				Configure_JoyHat(dlgSTkeyToHat[7], JOY_WII2DOWN);
				break;
			case JOY_WII2BUT1 :
				Configure_JoyButton(dlgSTkeyToButton[3], JOY_WII2BUT1);
				break;
			case JOY_WII2BUT2 :
				Configure_JoyButton(dlgSTkeyToButton[4], JOY_WII2BUT2);
				break;
			case JOY_WII2BUT3 :
				Configure_JoyButton(dlgSTkeyToButton[5], JOY_WII2BUT3);
				break;
			case KEYTOJOY_DEFAULT :
				Configure_JoySetDefault();
				break;
			case KEYTOJOY_CLOSE:
				break;
			case KEYTOJOY_UPDATEKEY:
				break;
		}
	} while(ret != KEYTOJOY_CLOSE);
}


/*-----------------------------------------------------------------------*/
/**
 * Show and process the virtual keyboard dialog.
 */
void Dialog_Virtualkeybdlg(void)
{
	int keys;
	int keytime = 1;
	int i = 0;
	int JoymapMenu = 0;
	SDL_Surface *KeyboardImg;
	SDL_RWops* Keyboard_rwops = NULL;
	SDL_Rect dest;

	keypressed = 0;
	JoyMode = ConfigureParams.Joysticks.Joy[1].nJoystickMode;

	/* Check if wiimote 1 is available, otherwise try GC pad 1 */
	WiimoteConnected = Joy_CheckWiimote();

	Main_PauseEmulation(true);

	SDLGui_SetScreen(sdlscrn);
	SDL_WarpMouse(320, 800);

	/* Load keyboard image from source */
	Keyboard_rwops = SDL_RWFromMem( keyboard, keyboard_size);
	KeyboardImg = IMG_Load_RW(Keyboard_rwops, 1);

	if(!KeyboardImg)
		Log_Printf(LOG_ERROR, "Error: Cannot open keyboard image!\n");

	dest.x = 0;
	dest.y = 110;

	SDL_BlitSurface(KeyboardImg, NULL, sdlscrn, &dest);

	SDL_ShowCursor(SDL_ENABLE);

	if (ConfigureParams.Joysticks.Joy[1].nJoystickMode == 0)
		virtualkeybdlg[KEYDLG_JOYMODE].txt = "Mouse mode";
	else if (ConfigureParams.Joysticks.Joy[1].nJoystickMode == 1)
		virtualkeybdlg[KEYDLG_JOYMODE].txt = "Joy mode";

	/* Draw and process the virtual keyboard dialog */
	do
	{
		keys = SDLGui_DoDialogKeyboard(virtualkeybdlg, NULL, false);

		/* Toggle between ST mouse and ST joystick mode */
		if (keys == KEYDLG_JOYMODE)
		{
			Joy_MouseMode();

			if (ConfigureParams.Joysticks.Joy[1].nJoystickMode == 0)
				virtualkeybdlg[KEYDLG_JOYMODE].txt = "Mouse mode";
			else if (ConfigureParams.Joysticks.Joy[1].nJoystickMode == 1)
				virtualkeybdlg[KEYDLG_JOYMODE].txt = "Joy mode";
		}

		/* Simulate a long press for some games */
		if (virtualkeybdlg[KEYDLG_TIME].state & SG_SELECTED)
			keytime = 80;

		/* Checkbox 'Map to joy' to enable the mapper */
		if (virtualkeybdlg[KEYDLG_MAP].state & SG_SELECTED)
			JoymapMenu = 1;
	}
	while (keys > KEYDLG_CLOSE);

	/* Convert the key to Atari ST scancode in decimal */
	keypressed = virtualkeybdlg[keys].shortcut;

	/* ST arrow keys emulated by Wii controllers hats */	
	WarningHats = ConfigureParams.Joysticks.Joy[0].nArrowtoJoy;
	
	/* Init the Wii Mapper */					
	if (JoymapMenu == 1 && keys != KEYDLG_CLOSE)
	{
		sprintf(CurrentKey,  "Choose a button for key '%s':", virtualkeybdlg[keys].txt);
		dlgWiiMapper_KeyBinding();
	}

	/* Arrow keys emulation is disabled on controller #1 and #2 */
	if (keystojoydlg[ARROWTOJOY1].state == 0 && keystojoydlg[ARROWTOJOY2].state == 0)
	{
		ConfigureParams.Joysticks.Joy[0].nArrowtoJoy = -1;
		WarningHats = ConfigureParams.Joysticks.Joy[0].nArrowtoJoy;
	}

	/* Check both controllers hat mode : ST joystick directions/ST keys */
	if (WarningHats == -1)
	{
		if (JoyMode == STJOYSTICK)
			Check_HatMode(1);
		Check_HatMode(2);
	}

	/* When the wii controller #2 is in joystick hat mode, enable joy in port 0.
	 * For 2+ joysticks games, Joy #2 is in port 0 and Joy #1 in port 1.  */
	if (Joy2HatMode == 0)
		ConfigureParams.Joysticks.Joy[0].nJoystickMode = JOYSTICK_REALSTICK;
	else
		ConfigureParams.Joysticks.Joy[0].nJoystickMode = JOYSTICK_DISABLED;

	Main_PauseEmulation(false);
	
	/* Save time to add a delay in hat arrow mode cf joystick.cpp */
	Joy_SetRepeat(keytime);

	/* Only send ST key when exiting from virtual keyboard,
	 * not from the mapper. */
	if (JoymapMenu == 0)
	{
		/* Sometimes the key is ignored. Send multiple times
		 * if 'Long press' is enabled. */
		for (i = 0; i < keytime; ++i)
		{
			/* Press Right shift for special characters instead of 0-9 */
			if(keys > 12 && keys < 23)
				IKBD_PressSTKey(54,true);

			IKBD_PressSTKey(keypressed,true);
		}

		if(keys > 12 && keys < 23)
			IKBD_PressSTKey(54,false);

		IKBD_PressSTKey(keypressed,false);
	}

	SDL_FreeSurface(KeyboardImg);
	SDL_ShowCursor(SDL_DISABLE);
	JoymapMenu = 0;
	Main_UnPauseEmulation();
}
