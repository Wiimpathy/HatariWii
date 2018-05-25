/*
  Hatari - DLGSAVEory.c

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.
*/
const char DLGSAVEory_fileid[] = "Hatari DLGSAVEory.c : " __DATE__ " " __TIME__;

#include "main.h"
#include "dialog.h"
#include "sdlgui.h"
#include "memorySnapShot.h"
#include "file.h"
#include "screen.h"
#include "options.h"

#ifdef GEKKO
#include <sys/stat.h>



char lastgame[256];

#define DLGSAVE_SLOT      4
#define DLGSAVE_PREVSLOT  5
#define DLGSAVE_NEXTSLOT  6
#define DLGSAVE_SAVE      7
#define DLGSAVE_RESTORE   8
#define DLGSAVE_AUTOSAVE  9
#define DLGSAVE_EXIT     10
#define DLGSAVE_BACK     11
#endif

#ifdef GEKKO
#define DLGMEM_512KB    4
#define DLGMEM_1MB      5
#define DLGMEM_2MB      6
#define DLGMEM_4MB      7
#define DLGMEM_8MB      8
#define DLGMEM_14MB     9
#define DLGMEM_TTRAM_LESS    11
#define DLGMEM_TTRAM_TEXT    12
#define DLGMEM_TTRAM_MORE    13
#define DLGMEM_EXIT     14
#define DLGMEM_BACK     15
#else
#define DLGMEM_512KB    4
#define DLGMEM_1MB      5
#define DLGMEM_2MB      6
#define DLGMEM_4MB      7
#define DLGMEM_8MB      8
#define DLGMEM_14MB     9
#define DLGMEM_TTRAM_LESS    11
#define DLGMEM_TTRAM_TEXT    12
#define DLGMEM_TTRAM_MORE    13
#define DLGMEM_FILENAME 17
#define DLGMEM_SAVE     18
#define DLGMEM_RESTORE  19
#define DLGMEM_AUTOSAVE 20
#define DLGMEM_EXIT     21
#endif

/* String for TT RAM size */
static char sTTRamSize[4];

#define DLG_TTRAM_STEP	4
#define DLG_TTRAM_MIN	0
#define DLG_TTRAM_MAX	256

static char dlgSnapShotName[36+1];


/* The memory dialog: */
static SGOBJ memorydlg[] =
{
#ifdef GEKKO
	{ SGBOX, 0, 0, 0,0, 40,14, NULL },
	{ SGBOX, 0, 0, 1,1, 38,9, NULL },
	{ SGTEXT, 0, 0, 15,2, 12,1, "Memory setup" },
	{ SGTEXT, 0, 0, 4,4, 12,1, "ST-RAM size:" },
	{ SGRADIOBUT, 0, 0, 18,4, 9,1, "_512 KiB" },
	{ SGRADIOBUT, 0, 0, 18,5, 7,1, "_1 MiB" },
	{ SGRADIOBUT, 0, 0, 18,6, 7,1, "_2 MiB" },
	{ SGRADIOBUT, 0, 0, 29,4, 7,1, "_4 MiB" },
	{ SGRADIOBUT, 0, 0, 29,5, 7,1, "_8 MiB" },
	{ SGRADIOBUT, 0, 0, 29,6, 8,1, "14 _MiB" },
	{ SGTEXT,     0, 0,  4,8,12,1, "TT-RAM size:" },
	{ SGBUTTON,   0, 0, 18,8, 1,1, "\x04", SG_SHORTCUT_LEFT },
	{ SGTEXT,     0, 0, 20,8, 3,1, sTTRamSize },
	{ SGBUTTON,   0, 0, 24,8, 1,1, "\x03", SG_SHORTCUT_RIGHT },
	{ SGBUTTON, SG_DEFAULT, 0, 10,12, 20,1, "Back to main menu" },
	{ SGTEXT, SG_CANCEL, 0, 800,23, 2,1, "B" },	// hidden, back with B button
	{ -1, 0, 0, 0,0, 0,0, NULL }
#else
	{ SGBOX, 0, 0, 0,0, 40,24, NULL },

	{ SGBOX, 0, 0, 1,1, 38,9, NULL },
	{ SGTEXT, 0, 0, 15,2, 12,1, "Memory setup" },
	{ SGTEXT, 0, 0, 4,4, 12,1, "ST-RAM size:" },
	{ SGRADIOBUT, 0, 0, 18,4, 9,1, "_512 KiB" },
	{ SGRADIOBUT, 0, 0, 18,5, 7,1, "_1 MiB" },
	{ SGRADIOBUT, 0, 0, 18,6, 7,1, "_2 MiB" },
	{ SGRADIOBUT, 0, 0, 29,4, 7,1, "_4 MiB" },
	{ SGRADIOBUT, 0, 0, 29,5, 7,1, "_8 MiB" },
	{ SGRADIOBUT, 0, 0, 29,6, 8,1, "14 _MiB" },
	{ SGTEXT,     0, 0,  4,8,12,1, "TT-RAM size:" },
	{ SGBUTTON,   0, 0, 18,8, 1,1, "\x04", SG_SHORTCUT_LEFT },
	{ SGTEXT,     0, 0, 20,8, 3,1, sTTRamSize },
	{ SGBUTTON,   0, 0, 24,8, 1,1, "\x03", SG_SHORTCUT_RIGHT },

	{ SGBOX,      0, 0,  1,11, 38,10, NULL },
	{ SGTEXT,     0, 0, 12,12, 17,1, "Memory state save" },
	{ SGTEXT,     0, 0,  2,14, 20,1, "Snap-shot file name:" },
	{ SGTEXT,     0, 0,  2,15, 36,1, dlgSnapShotName },
	{ SGBUTTON,   0, 0,  8,17, 10,1, "_Save" },
	{ SGBUTTON,   0, 0, 22,17, 10,1, "_Restore" },
	{ SGCHECKBOX, 0, 0,  2,19, 34,1, "_Load/save state at start-up/exit" },

	{ SGBUTTON, SG_DEFAULT, 0, 10,22, 20,1, "Back to main menu" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
#endif
};

#ifdef GEKKO
char snapfile[256];
char statefile[256];
char lastsave[256]="";
static int slotnbr = 1;


/* The load/save dialog: */
static SGOBJ savedlg[] =
{
	{ SGBOX, 0, 0, 0,0, 40,24, NULL },
	{ SGTEXT,     0, 0, 11,3, 17,1, "Memory state save" },
	{ SGSNAPSHOT,     0, 0,  21, -20, 0,0, "thumbnail" },
	{ SGBOX, 0, 0, 9,15, 24,1, NULL },
	{ SGTEXT, 0, 0, 10,15, 22,1, "Slot 1 :" },
	{ SGBUTTON, 0, 0,  6,15, 3,1, "\x04", SG_SHORTCUT_LEFT },
	{ SGBUTTON, 0, 0, 30,15, 3,1, "\x03", SG_SHORTCUT_RIGHT },
	{ SGBUTTON,   0, 0,  8,17, 10,1, "_Save" },
	{ SGBUTTON, SG_TOUCHEXIT, 0, 22,17, 10,1, "_Restore" },
	{ SGCHECKBOX, 0, 0,  2,19, 34,1, "_Load/save state at start-up/exit" },
	{ SGBUTTON, SG_DEFAULT, 0, 10,22, 20,1, "Back to settings" },
	{ SGTEXT, SG_CANCEL, 0, 800,23, 2,1, "B" },	// hidden, back with B button
	{ -1, 0, 0, 0,0, 0,0, NULL }
};
#endif

/*-----------------------------------------------------------------------*/
/**
 * Make the snapshot path.
 * Create the .sav  and .bmp path based on the loaded floppy name.
 * Add slot number to the filename.
 */
void StateSnapshot_MakePath(const char *path, int number )
{
	static char Dir[256],  Ext[256];
	static char tempname[256];
	static char savedir[256];

	sprintf(savedir, Paths_GetGamesDir(-1, "saves"));

	File_SplitPath(path, Dir, tempname, Ext);

	sprintf(statefile, "%s/%s %d", savedir, tempname, number);
	strcat (statefile,".sav");
	sprintf(ConfigureParams.Memory.szMemoryCaptureFileName, statefile);

	sprintf(lastsave, statefile);

	File_SplitPath(statefile, Dir, tempname, Ext);
	sprintf(snapfile, "%s/%s", savedir, tempname);
	strcat (snapfile,".bmp");
}

/*-----------------------------------------------------------------------*/
/**
 * Get snapshot date and time.
 */
char * StateSnapshot_GetDate(const char *path )
{
	static char details[20];
	static char datetxt[256], timetxt[256];
	struct stat filestat;
	struct tm * timeinfo;

	if (File_Exists(path))
	{
		timeinfo = localtime(&filestat.st_mtime);
		strftime(datetxt, 80,"%x %I:%M%p", timeinfo);
		sprintf(details, "%d: ", slotnbr);
		strcat(details, datetxt);
	}
	else
	{
		sprintf(details, "%d: ", slotnbr);
	}

	return details;
}

/*-----------------------------------------------------------------------*/
/**
 * Save the snapshot thumbnail.
 */
void StateSnapshot_SaveThumbnail()
{
	SDL_Surface *screen;
	SDL_Surface *thumbnail = NULL;

	/* Save thumbnail */
	SDL_Surface *copy = SDL_DisplayFormat(lastscreen);

	thumbnail = ScaleSurface(copy, 160,120);
	SDL_SaveBMP(thumbnail, snapfile);

	SDL_FreeSurface(copy);
	SDL_FreeSurface(thumbnail);
}


/**
 * Show and process the load/save dialog.
 * @return  true if a memory snapshot has been loaded, false otherwise
 */
bool Dialog_SaveDlg(void)
{
	int i, memsize;
	int but;
	char currentgame[256];

	SDLGui_CenterDlg(savedlg);

	/* Clear the screen */
	SDL_FillRect(sdlscrn, NULL, SDL_MapRGB(sdlscrn->format, 0, 0, 0));

	if (ConfigureParams.Memory.bAutoSave)
		savedlg[DLGSAVE_AUTOSAVE].state |= SG_SELECTED;
	else
		savedlg[DLGSAVE_AUTOSAVE].state &= ~SG_SELECTED;

	if( lastsave[0] == '\0' )
	{
		sprintf(currentgame, "%s: ", ConfigureParams.DiskImage.szDiskFileName[0]);
		StateSnapshot_MakePath(currentgame, slotnbr);
		savedlg[DLGSAVE_SLOT].txt = StateSnapshot_GetDate(statefile);
	}
	else
	{
		if (strcmp (currentgame, lastgame) == 0)
		{
			savedlg[DLGSAVE_SLOT].txt = StateSnapshot_GetDate(lastsave);
		}
		else
		{
			sprintf(currentgame, "%s: ", ConfigureParams.DiskImage.szDiskFileName[0]);
			StateSnapshot_MakePath(currentgame, slotnbr);
			savedlg[DLGSAVE_SLOT].txt = StateSnapshot_GetDate(statefile);
		}
	}

	do
	{
		but = SDLGui_DoDialog(savedlg, NULL, false);

		switch (but)
		{
		case DLGSAVE_PREVSLOT:        /* Previous save slot */
			if (slotnbr > 1)
				slotnbr -= 1;

			sprintf(currentgame, "%s: ", ConfigureParams.DiskImage.szDiskFileName[0]);
			StateSnapshot_MakePath(currentgame, slotnbr);
			savedlg[DLGSAVE_SLOT].txt = StateSnapshot_GetDate(statefile);
			break;
		case DLGSAVE_NEXTSLOT:        /* Next save slot */
			if (slotnbr < 10)
				slotnbr += 1;

			sprintf(currentgame, "%s: ", ConfigureParams.DiskImage.szDiskFileName[0]);
			StateSnapshot_MakePath(currentgame, slotnbr);
			savedlg[DLGSAVE_SLOT].txt = StateSnapshot_GetDate(statefile);
			break;
		 case DLGSAVE_SAVE:          /* Save memory snap-shot */
			sprintf(currentgame, "%s: ", ConfigureParams.DiskImage.szDiskFileName[0]);
			StateSnapshot_MakePath(currentgame, slotnbr);
			MemorySnapShot_Capture(statefile, true);
			savedlg[DLGSAVE_SLOT].txt = StateSnapshot_GetDate(statefile);
			StateSnapshot_SaveThumbnail();
			break;
		 case DLGSAVE_RESTORE:        /* Load memory snap-shot */
			if (lastsave != NULL)
			{
				savedlg[DLGSAVE_SLOT].txt = StateSnapshot_GetDate(lastsave);
				MemorySnapShot_Restore(lastsave, true);
				return true;
			}
			break;
		}
	}
#ifdef GEKKO
	while (but != DLGSAVE_EXIT && but != DLGSAVE_BACK && but != SDLGUI_QUIT
	        && but != SDLGUI_ERROR && !bQuitProgram );

	/* Clear the screen */
	SDL_FillRect(sdlscrn, NULL, SDL_MapRGB(sdlscrn->format, 0, 0, 0));
#else
	while (but != DLGSAVE_EXIT && but != SDLGUI_QUIT
	        && but != SDLGUI_ERROR && !bQuitProgram );
#endif
	/* Read new values from dialog: */

	return false;
}


/**
 * Show and process the memory dialog.
 * @return  true if a memory snapshot has been loaded, false otherwise
 */
bool Dialog_MemDlg(void)
{
	int i, memsize;
	int but;

	SDLGui_CenterDlg(memorydlg);
#ifdef GEKKO
	/* Clear the screen */
	SDL_FillRect(sdlscrn, NULL, SDL_MapRGB(sdlscrn->format, 0, 0, 0));
#endif
	for (i = DLGMEM_512KB; i <= DLGMEM_14MB; i++)
	{
		memorydlg[i].state &= ~SG_SELECTED;
	}

	switch (ConfigureParams.Memory.nMemorySize)
	{
	 case 0:
		memorydlg[DLGMEM_512KB].state |= SG_SELECTED;
		break;
	 case 1:
		memorydlg[DLGMEM_1MB].state |= SG_SELECTED;
		break;
	 case 2:
		memorydlg[DLGMEM_2MB].state |= SG_SELECTED;
		break;
	 case 4:
		memorydlg[DLGMEM_4MB].state |= SG_SELECTED;
		break;
	 case 8:
		memorydlg[DLGMEM_8MB].state |= SG_SELECTED;
		break;
	 default:
		memorydlg[DLGMEM_14MB].state |= SG_SELECTED;
		break;
	}
	memsize = ConfigureParams.Memory.nTTRamSize;
#if ENABLE_WINUAE_CPU
	sprintf(sTTRamSize, "%3i", memsize);
#else
	strcpy(sTTRamSize, "N/A");
#endif

#ifndef GEKKO
	File_ShrinkName(dlgSnapShotName, ConfigureParams.Memory.szMemoryCaptureFileName, memorydlg[DLGMEM_FILENAME].w);


	if (ConfigureParams.Memory.bAutoSave)
		memorydlg[DLGMEM_AUTOSAVE].state |= SG_SELECTED;
	else
		memorydlg[DLGMEM_AUTOSAVE].state &= ~SG_SELECTED;
#endif

	do
	{
		but = SDLGui_DoDialog(memorydlg, NULL, false);

		switch (but)
		{
#if ENABLE_WINUAE_CPU
		 case DLGMEM_TTRAM_LESS:
			memsize = Opt_ValueAlignMinMax(memsize - DLG_TTRAM_STEP, DLG_TTRAM_STEP, DLG_TTRAM_MIN, DLG_TTRAM_MAX);
			sprintf(sTTRamSize, "%3i", memsize);
			break;
		 case DLGMEM_TTRAM_MORE:
			memsize = Opt_ValueAlignMinMax(memsize + DLG_TTRAM_STEP, DLG_TTRAM_STEP, DLG_TTRAM_MIN, DLG_TTRAM_MAX);
			sprintf(sTTRamSize, "%3i", memsize);
			break;
#endif

#ifndef GEKKO
		 case DLGMEM_SAVE:              /* Save memory snap-shot */
			if (SDLGui_FileConfSelect("Save memory snapshot:", dlgSnapShotName,
			                          ConfigureParams.Memory.szMemoryCaptureFileName,
			                          memorydlg[DLGMEM_FILENAME].w, true))
			{
				MemorySnapShot_Capture(ConfigureParams.Memory.szMemoryCaptureFileName, true);
			}
			break;
		 case DLGMEM_RESTORE:           /* Load memory snap-shot */
			if (SDLGui_FileConfSelect("Load memory snapshot:", dlgSnapShotName,
			                          ConfigureParams.Memory.szMemoryCaptureFileName,
			                          memorydlg[DLGMEM_FILENAME].w, false))
			{
				MemorySnapShot_Restore(ConfigureParams.Memory.szMemoryCaptureFileName, true);
				return true;
			}
			break;
#endif
		}
	}
#ifdef GEKKO
	while (but != DLGMEM_EXIT && but != DLGMEM_BACK && but != SDLGUI_QUIT
	        && but != SDLGUI_ERROR && !bQuitProgram );

	/* Clear the screen */
	SDL_FillRect(sdlscrn, NULL, SDL_MapRGB(sdlscrn->format, 0, 0, 0));
#else
	while (but != DLGMEM_EXIT && but != SDLGUI_QUIT
	        && but != SDLGUI_ERROR && !bQuitProgram );
#endif
	/* Read new values from dialog: */

	if (memorydlg[DLGMEM_512KB].state & SG_SELECTED)
		ConfigureParams.Memory.nMemorySize = 0;
	else if (memorydlg[DLGMEM_1MB].state & SG_SELECTED)
		ConfigureParams.Memory.nMemorySize = 1;
	else if (memorydlg[DLGMEM_2MB].state & SG_SELECTED)
		ConfigureParams.Memory.nMemorySize = 2;
	else if (memorydlg[DLGMEM_4MB].state & SG_SELECTED)
		ConfigureParams.Memory.nMemorySize = 4;
	else if (memorydlg[DLGMEM_8MB].state & SG_SELECTED)
		ConfigureParams.Memory.nMemorySize = 8;
	else
		ConfigureParams.Memory.nMemorySize = 14;

	ConfigureParams.Memory.nTTRamSize = memsize;
#ifndef GEKKO
	ConfigureParams.Memory.bAutoSave = (memorydlg[DLGMEM_AUTOSAVE].state & SG_SELECTED);
#endif
	return false;
}
