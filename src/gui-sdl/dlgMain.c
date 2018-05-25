/*
  Hatari - dlgMain.c

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  The main dialog.
*/
const char DlgMain_fileid[] = "Hatari dlgMain.c : " __DATE__ " " __TIME__;

#include "main.h"
#include "configuration.h"
#include "dialog.h"
#include "sdlgui.h"
#include "screen.h"

#ifdef GEKKO
/* Wii Main menu */
#define MAINDLG_FLOPPYA    2
#define MAINDLG_FLOPPYB    3
#define MAINDLG_LOAD_SAVE  4
#define MAINDLG_SETTINGS   5
#define MAINDLG_RESET      6
#define MAINDLG_QUIT       7
#define MAINDLG_SPEED      8
#define MAINDLG_OK         9
#define MAINDLG_CANCEL     10
/* Wii Settings menu */
#define OPTIONSDLG_ABOUT    2
#define OPTIONSDLG_SYSTEM   3
#define OPTIONSDLG_ROM      4
#define OPTIONSDLG_MEMORY   5
#define OPTIONSDLG_FLOPPYS  6
#define OPTIONSDLG_HARDDISK 7
#define OPTIONSDLG_MONITOR  8
#define OPTIONSDLG_WINDOW   9
#define OPTIONSDLG_JOY      10
#define OPTIONSDLG_KEYBD    11
#define OPTIONSDLG_DEVICES  12
#define OPTIONSDLG_SOUND    13
#define OPTIONSDLG_LOADCFG  14
#define OPTIONSDLG_SAVECFG  15
#define OPTIONSDLG_EXIT     16
#define OPTIONSDLG_BACK     17
#else
#define MAINDLG_ABOUT    2
#define MAINDLG_SYSTEM   3
#define MAINDLG_ROM      4
#define MAINDLG_MEMORY   5
#define MAINDLG_FLOPPYS  6
#define MAINDLG_HARDDISK 7
#define MAINDLG_MONITOR  8
#define MAINDLG_WINDOW   9
#define MAINDLG_JOY      10
#define MAINDLG_KEYBD    11
#define MAINDLG_DEVICES  12
#define MAINDLG_SOUND    13
#define MAINDLG_LOADCFG  14
#define MAINDLG_SAVECFG  15
#define MAINDLG_NORESET  16
#define MAINDLG_RESET    17
#define MAINDLG_OK       18
#define MAINDLG_QUIT     19
#define MAINDLG_CANCEL   20
#endif

char dlgname[MAX_FLOPPYDRIVES][64];
void DlgDisk_BrowseDisk(char *dlgname, int drive, int diskid);
int floppyeject = -1;
static int speed = 1;

/* The main dialog: */
static SGOBJ maindlg[] =
{
#ifdef GEKKO
	{ SGBOX, 0, 0, 0,0, 55,25, NULL },
	{ SGTEXT, 0, 0, 17,1, 16,1, "Hatari main menu" },
	{ SGFLOPPYA, SG_EXIT, 0, 4,3, 12,7, "" },
	{ SGFLOPPYB, SG_EXIT, 0, 22,3, 12,7, "" },
	{ SGLOAD_SAVE, SG_EXIT, 0, 40,3, 12,7, "" },
	{ SGSETTINGS, SG_EXIT, 0, 4,12, 12,7, "" },
	{ SGRESET, SG_EXIT, 0, 22,12, 12,7, "" },
	{ SGQUIT, SG_EXIT, 0, 40,12, 12,7, "" },
	{ SGBUTTON,  0, 0, 3,22, 15,1, "Speed: Normal" },
	{ SGBUTTON, SG_DEFAULT, 0, 35,21, 8,3, "OK" },
	{ SGBUTTON, SG_CANCEL, 0, 44,21, 8,3, "Cancel" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
#else
	{ SGBOX, 0, 0, 0,0, 50,19, NULL },
	{ SGTEXT, 0, 0, 17,2, 16,1, "Hatari main menu" },
	{ SGBUTTON, 0, 0,  2, 4, 13,1, "A_bout" },
	{ SGBUTTON, 0, 0,  2, 6, 13,1, "S_ystem" },
	{ SGBUTTON, 0, 0,  2, 8, 13,1, "_ROM" },
	{ SGBUTTON, 0, 0,  2,10, 13,1, "_Memory" },
	{ SGBUTTON, 0, 0, 17, 4, 16,1, "_Floppy disks" },
	{ SGBUTTON, 0, 0, 17, 6, 16,1, "Hard _disks" },
	{ SGBUTTON, 0, 0, 17, 8, 16,1, "_Atari screen" },
	{ SGBUTTON, 0, 0, 17,10, 16,1, "_Hatari screen" },
	{ SGBUTTON, 0, 0, 35, 4, 13,1, "_Joysticks" },
	{ SGBUTTON, 0, 0, 35, 6, 13,1, "_Keyboard" },
	{ SGBUTTON, 0, 0, 35, 8, 13,1, "D_evices" },
	{ SGBUTTON, 0, 0, 35,10, 13,1, "S_ound" },
	{ SGBUTTON, 0, 0,  7,13, 16,1, "_Load config." },
	{ SGBUTTON, 0, 0, 27,13, 16,1, "_Save config." },
	{ SGRADIOBUT, 0, 0, 3,15, 10,1, "_No Reset" },
	{ SGRADIOBUT, 0, 0, 3,17, 15,1, "Reset ma_chine" },
	{ SGBUTTON, SG_DEFAULT, 0, 21,15, 8,3, "OK" },
	{ SGBUTTON, 0, 0, 36,15, 10,1, "_Quit" },
	{ SGBUTTON, SG_CANCEL, 0, 36,17, 10,1, "Cancel" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
#endif
};

#ifdef GEKKO
/* The main dialog: */
static SGOBJ settingsdlg[] =
{
	{ SGBOX, 0, 0, 0,0, 50,23, NULL },
	{ SGTEXT, 0, 0, 17,1, 16,1, "Hatari settings" },
	{ SGBUTTON, 0, 0,  2, 4, 13,1, "A_bout" },
	{ SGBUTTON, 0, 0,  2, 6, 13,1, "S_ystem" },
	{ SGBUTTON, 0, 0,  2, 8, 13,1, "_ROM" },
	{ SGBUTTON, 0, 0,  2,10, 13,1, "_Memory" },
	{ SGBUTTON, 0, 0, 17, 4, 16,1, "_Floppy disks" },
	{ SGBUTTON, 0, 0, 17, 6, 16,1, "Hard _disks" },
	{ SGBUTTON, 0, 0, 17, 8, 16,1, "_Atari screen" },
	{ SGBUTTON, 0, 0, 17,10, 16,1, "_Hatari screen" },
	{ SGBUTTON, 0, 0, 35, 4, 13,1, "_Joysticks" },
	{ SGBUTTON, 0, 0, 35, 6, 13,1, "_Keyboard" },
	{ SGBUTTON, 0, 0, 35, 8, 13,1, "D_evices" },
	{ SGBUTTON, 0, 0, 35,10, 13,1, "S_ound" },
	{ SGBUTTON, 0, 0,  7,13, 16,1, "_Load config." },
	{ SGBUTTON, 0, 0, 27,13, 16,1, "_Save config." },
	{ SGBUTTON, SG_DEFAULT, 0, 15,18, 20,1, "Back to main menu" },
	{ SGTEXT, SG_CANCEL, 0, 800,23, 2,1, "B" },	// hidden, back with B button
	{ -1, 0, 0, 0,0, 0,0, NULL }
};
#endif


/**
 * Show and process the floppy disk image dialog.
 */
void DlgSettings_Main(void)
{
	int but;
	char *psNewCfg;

	SDLGui_CenterDlg(settingsdlg);

	/* Clear the screen */
	SDL_FillRect(sdlscrn, NULL, SDL_MapRGB(sdlscrn->format, 0, 0, 0));

	/* Draw and process the dialog */
	do
	{
		but = SDLGui_DoDialog(settingsdlg, NULL, false);
		switch (but)
		{
		case OPTIONSDLG_SYSTEM:                         
			Dialog_SystemDlg();
			break;
		 case OPTIONSDLG_ROM:                       
			DlgRom_Main();
			break;
		case OPTIONSDLG_MEMORY:
			Dialog_MemDlg();
			break;
		 case OPTIONSDLG_FLOPPYS:                         
			DlgFloppy_Main();
			break;
		case OPTIONSDLG_HARDDISK:                       
			DlgHardDisk_Main();
			break;
		 case OPTIONSDLG_MONITOR:
			Dialog_MonitorDlg();
			break;
		 case OPTIONSDLG_WINDOW:
			Dialog_WindowDlg();
			break;
		 case OPTIONSDLG_JOY:
			Dialog_JoyDlg();
			break;
		 case OPTIONSDLG_KEYBD:
			Dialog_KeyboardDlg();
			break;
		 case OPTIONSDLG_DEVICES:
			Dialog_DeviceDlg();
			break;
		 case OPTIONSDLG_SOUND:
			Dialog_SoundDlg();
			break;
		 case OPTIONSDLG_ABOUT:
			Dialog_AboutDlg();
			break;
		 case OPTIONSDLG_LOADCFG:
			psNewCfg = SDLGui_FileSelect("Load configuration:", sConfigFileName, NULL, false);
			if (psNewCfg)
			{
				strcpy(sConfigFileName, psNewCfg);
				Configuration_Load(NULL);
				free(psNewCfg);
			}
			break;
		 case OPTIONSDLG_SAVECFG:
			psNewCfg = SDLGui_FileSelect("Save configuration:", sConfigFileName, NULL, true);
			if (psNewCfg)
			{
				strcpy(sConfigFileName, psNewCfg);
				Configuration_Save();
				free(psNewCfg);
			}
			break;
		}
	}
	while (but != OPTIONSDLG_EXIT && but != SDLGUI_QUIT && but != OPTIONSDLG_BACK
	        && but != SDLGUI_ERROR && !bQuitProgram);
}


/**
 * This functions sets up the actual font and then displays the main dialog.
 */
int Dialog_MainDlg(bool *bReset, bool *bLoadedSnapshot)
{
	int retbut;
	bool bOldMouseVisibility;
	int nOldMouseX, nOldMouseY;
#ifndef GEKKO
	char *psNewCfg;
#endif

	floppyeject = -1;
	*bReset = false;
	*bLoadedSnapshot = false;

#ifdef GEKKO
	/* Copy screen for snapshots */

	if (ConfigureParams.Screen.nMonitorType == MONITOR_TYPE_TV)
	{
		/* Disable TV mode before copying the screen. The snapshot wouldn't be visible otherwise. */
		ConfigureParams.Screen.nMonitorType = MONITOR_TYPE_VGA;
		Screen_ModeChanged(true);
		Screen_Draw();
		lastscreen = SDL_DisplayFormat(sdlscrn);
		ConfigureParams.Screen.nMonitorType = MONITOR_TYPE_TV;
	}
	else
	{
		lastscreen = SDL_DisplayFormat(sdlscrn);
	}
#endif

	if (SDLGui_SetScreen(sdlscrn))
		return false;

	SDL_GetMouseState(&nOldMouseX, &nOldMouseY);
	bOldMouseVisibility = SDL_ShowCursor(SDL_QUERY);
	SDL_ShowCursor(SDL_ENABLE);

	SDLGui_CenterDlg(maindlg);

#ifndef GEKKO
	maindlg[MAINDLG_NORESET].state |= SG_SELECTED;
	maindlg[MAINDLG_RESET].state &= ~SG_SELECTED;
#endif

#ifdef GEKKO
	/* Disable 'Turbo mode' in mouse mode or else no double click. */
	if (ConfigureParams.Joysticks.Joy[1].nJoystickMode == 0 && ConfigureParams.System.bFastForward)
	{
		ConfigureParams.System.bFastForward = false;
	}

	if (ConfigureParams.System.bFastForward)
		maindlg[MAINDLG_SPEED].txt = "Speed: Turbo";
	else
		maindlg[MAINDLG_SPEED].txt = "Speed: Normal";
#endif

	do
	{
		retbut = SDLGui_DoDialog(maindlg, NULL, false);
		switch (retbut)
		{
#ifdef GEKKO
		 case MAINDLG_FLOPPYA:
			floppyeject = 0;
			maindlg[MAINDLG_FLOPPYA].type = 15;
			DlgDisk_BrowseDisk(dlgname[0], 0, 7);
			Floppy_InsertDiskIntoDrive(0);
			break;
		 case MAINDLG_FLOPPYB:
			floppyeject = 1;
			maindlg[MAINDLG_FLOPPYB].type = 16;
			DlgDisk_BrowseDisk(dlgname[1], 1, 13);
			Floppy_InsertDiskIntoDrive(1);
			break;
		case MAINDLG_LOAD_SAVE:
			maindlg[MAINDLG_LOAD_SAVE].type = 17;

			if (Dialog_SaveDlg())
			{
				SDL_ShowCursor(SDL_DISABLE);
				/* Memory snapshot has been loaded - leave GUI immediately */
				*bLoadedSnapshot = true;
				return true;
			}
			break;
		case MAINDLG_SETTINGS:
			maindlg[MAINDLG_SETTINGS].type = 18;
			DlgSettings_Main();
			break;
		case MAINDLG_RESET:
			maindlg[MAINDLG_RESET].type = 19;
			*bReset = true;
			SDL_ShowCursor(SDL_DISABLE);
			Reset_Cold();
			return true;
		case MAINDLG_QUIT:
			maindlg[MAINDLG_QUIT].type = 20;
			bQuitProgram = true;
			break;
		case MAINDLG_SPEED:
			speed += 1;

			if (speed > 2)
				speed = 1;

			if (speed == 1)
			{
				maindlg[MAINDLG_SPEED].txt = "Speed: Normal";
				ConfigureParams.System.bFastForward = false;
			}
			else if (speed == 2)
			{
				maindlg[MAINDLG_SPEED].txt = "Speed: Turbo";
				ConfigureParams.System.bFastForward = true;
			}
			break;
#else
		 case MAINDLG_ABOUT:
			Dialog_AboutDlg();
			break;
		 case MAINDLG_FLOPPYS:
			DlgFloppy_Main();
			break;
		 case MAINDLG_HARDDISK:
			DlgHardDisk_Main();
			break;
		 case MAINDLG_ROM:
			DlgRom_Main();
			break;
		 case MAINDLG_MONITOR:
			Dialog_MonitorDlg();
			break;
		 case MAINDLG_WINDOW:
			Dialog_WindowDlg();
			break;
		 case MAINDLG_SYSTEM:
			Dialog_SystemDlg();
			break;
		 case MAINDLG_MEMORY:
			if (Dialog_MemDlg())
			{
				/* Memory snapshot has been loaded - leave GUI immediately */
				*bLoadedSnapshot = true;
				SDL_ShowCursor(bOldMouseVisibility);
				Main_WarpMouse(nOldMouseX, nOldMouseY, true);
				return true;
			}
			break;
		 case MAINDLG_JOY:
			Dialog_JoyDlg();
			break;
		 case MAINDLG_KEYBD:
			Dialog_KeyboardDlg();
			break;
		 case MAINDLG_DEVICES:
			Dialog_DeviceDlg();
			break;
		 case MAINDLG_SOUND:
			Dialog_SoundDlg();
			break;
		 case MAINDLG_LOADCFG:
			psNewCfg = SDLGui_FileSelect("Load configuration:", sConfigFileName, NULL, false);
			if (psNewCfg)
			{
				strcpy(sConfigFileName, psNewCfg);
				Configuration_Load(NULL);
				free(psNewCfg);
			}
			break;
		 case MAINDLG_SAVECFG:
			psNewCfg = SDLGui_FileSelect("Save configuration:", sConfigFileName, NULL, true);
			if (psNewCfg)
			{
				strcpy(sConfigFileName, psNewCfg);
				Configuration_Save();
				free(psNewCfg);
			}
			break;
		 case MAINDLG_QUIT:
			bQuitProgram = true;
			break;
#endif
		}
	}
	while (retbut != MAINDLG_OK && retbut != MAINDLG_CANCEL && retbut != SDLGUI_QUIT
	        && retbut != SDLGUI_ERROR && !bQuitProgram);

	if (maindlg[MAINDLG_RESET].state & SG_SELECTED)
		*bReset = true;
#ifdef GEKKO
	/* Disable 'Turbo mode' in mouse mode or else no double click. */
	if (ConfigureParams.Joysticks.Joy[1].nJoystickMode == 0 && ConfigureParams.System.bFastForward)
	{
		DlgAlert_Notice("Turbo is disabled in mouse mode! Double click is impossible if enabled.\n Enable Joy mode in the virtual keyboard to use Turbo.");
		ConfigureParams.System.bFastForward = false;
	}

	SDL_FreeSurface(lastscreen);
#endif

	SDL_ShowCursor(bOldMouseVisibility);
	Main_WarpMouse(nOldMouseX, nOldMouseY, true);

	return (retbut == MAINDLG_OK);
}
