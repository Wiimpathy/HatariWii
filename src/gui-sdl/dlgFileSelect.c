/*
  Hatari - dlgFileSelect.c

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  A file selection dialog for the graphical user interface for Hatari.
*/
const char DlgFileSelect_fileid[] = "Hatari dlgFileSelect.c : " __DATE__ " " __TIME__;

#include <SDL.h>
#include <sys/stat.h>
#include <unistd.h>

#include "main.h"
#include "scandir.h"
#include "sdlgui.h"
#include "file.h"
#include "paths.h"
#include "zip.h"

#ifdef GEKKO
#include <dirent.h>
#endif

#ifndef GEKKO
#define SGFS_NUMENTRIES   16            /* How many entries are displayed at once */

#define SGFSDLG_TITLE       1
#define SGFSDLG_FILENAME    5
#define SGFSDLG_UPDIR       6
#define SGFSDLG_CWD         7
#define SGFSDLG_HOMEDIR     8
#define SGFSDLG_ROOTDIR     9
#define SGFSDLG_ENTRYFIRST 12
#define SGFSDLG_ENTRYLAST  27
#define SGFSDLG_SCROLLBAR  28
#define SGFSDLG_UP         29
#define SGFSDLG_DOWN       30
#define SGFSDLG_SHOWHIDDEN 31
#define SGFSDLG_OKAY       32
#define SGFSDLG_CANCEL     33
#else
#define SGFS_NUMENTRIES   8

#define SGFSDLG_TITLE       1
#define SGFSDLG_FILENAME    5
#define SGFSDLG_EJECT       6
#define SGFSDLG_UPDIR       7
#define SGFSDLG_SDCARD      8
#define SGFSDLG_USB     		9
#define SGFSDLG_ENTRYFIRST 12
#define SGFSDLG_ENTRYLAST  19
#define SGFSDLG_SCROLLBAR  20
#define SGFSDLG_UP         21
#define SGFSDLG_DOWN       22
#define SGFSDLG_OKAY       23
#define SGFSDLG_CANCEL     24
#define SGFSDLG_PAGEUP     25
#define SGFSDLG_PAGEDOWN   26


#define IMGDLG_UP        1
#define IMGDLG_DOWN      2
#define IMGDLG_LEFT      3
#define IMGDLG_RIGHT     4
#define IMGDLG_OK        5
#define IMGDLG_BACK      6
#define IMGDLG_PAGEUP    7
#define IMGDLG_PAGEDOWN  8
#endif

#define SCROLLOUT_ABOVE  1
#define SCROLLOUT_UNDER  2

#define DLGPATH_SIZE 62
static char dlgpath[DLGPATH_SIZE+1];    /* Path name in the dialog */

#define DLGFNAME_SIZE 56
static char dlgfname[DLGFNAME_SIZE+1];  /* Name of the selected file in the dialog */

#define DLGFILENAMES_SIZE 56
static char dlgfilenames[SGFS_NUMENTRIES][DLGFILENAMES_SIZE+1];  /* Visible file names in the dialog */

#define SCROLLBAR_MIN_HEIGHT 4		/* Min value for yScrollbar_size */

#define TITLE_OFFSET 1
#define TITLE_MAXLEN 40

/* The dialog data: */
static SGOBJ fsdlg[] =
{
#ifndef GEKKO
	{ SGBOX, 0, 0, 0,0, 64,25, NULL },
	{ SGTEXT, 0, 0, 1,1, 13,1, NULL, },
	{ SGTEXT, 0, 0, 1,2, 7,1, "Folder:" },
	{ SGTEXT, 0, 0, 1,3, DLGPATH_SIZE,1, dlgpath },
	{ SGTEXT, 0, 0, 1,4, 6,1, "File:" },
	{ SGTEXT, 0, 0, 7,4, DLGFNAME_SIZE,1, dlgfname },
	{ SGBUTTON, 0, 0, 39,1, 4,1, "_Up" },
	{ SGBUTTON, 0, 0, 44,1, 5,1, "_CWD" },
	{ SGBUTTON, 0, 0, 50,1, 6,1, "_Home" },
	{ SGBUTTON, 0, 0, 57,1, 6,1, "_Root" },
	{ SGBOX, 0, 0, 1,6, 62,16, NULL },
	{ SGBOX, 0, 0, 62,7, 1,14, NULL },
	{ SGTEXT, SG_EXIT, 0, 2,6, DLGFILENAMES_SIZE,1, dlgfilenames[0] },
	{ SGTEXT, SG_EXIT, 0, 2,7, DLGFILENAMES_SIZE,1, dlgfilenames[1] },
	{ SGTEXT, SG_EXIT, 0, 2,8, DLGFILENAMES_SIZE,1, dlgfilenames[2] },
	{ SGTEXT, SG_EXIT, 0, 2,9, DLGFILENAMES_SIZE,1, dlgfilenames[3] },
	{ SGTEXT, SG_EXIT, 0, 2,10, DLGFILENAMES_SIZE,1, dlgfilenames[4] },
	{ SGTEXT, SG_EXIT, 0, 2,11, DLGFILENAMES_SIZE,1, dlgfilenames[5] },
	{ SGTEXT, SG_EXIT, 0, 2,12, DLGFILENAMES_SIZE,1, dlgfilenames[6] },
	{ SGTEXT, SG_EXIT, 0, 2,13, DLGFILENAMES_SIZE,1, dlgfilenames[7] },
	{ SGTEXT, SG_EXIT, 0, 2,14, DLGFILENAMES_SIZE,1, dlgfilenames[8] },
	{ SGTEXT, SG_EXIT, 0, 2,15, DLGFILENAMES_SIZE,1, dlgfilenames[9] },
	{ SGTEXT, SG_EXIT, 0, 2,16, DLGFILENAMES_SIZE,1, dlgfilenames[10] },
	{ SGTEXT, SG_EXIT, 0, 2,17, DLGFILENAMES_SIZE,1, dlgfilenames[11] },
	{ SGTEXT, SG_EXIT, 0, 2,18, DLGFILENAMES_SIZE,1, dlgfilenames[12] },
	{ SGTEXT, SG_EXIT, 0, 2,19, DLGFILENAMES_SIZE,1, dlgfilenames[13] },
	{ SGTEXT, SG_EXIT, 0, 2,20, DLGFILENAMES_SIZE,1, dlgfilenames[14] },
	{ SGTEXT, SG_EXIT, 0, 2,21, DLGFILENAMES_SIZE,1, dlgfilenames[15] },
	{ SGSCROLLBAR, SG_TOUCHEXIT, 0, 62, 7, 0, 0, NULL },       /* Scrollbar */
	{ SGBUTTON,   SG_TOUCHEXIT, 0, 62, 6,1,1, "\x01", SG_SHORTCUT_UP },
	{ SGBUTTON,   SG_TOUCHEXIT, 0, 62,21,1,1, "\x02", SG_SHORTCUT_DOWN },
	{ SGCHECKBOX, SG_EXIT, 0, 2,23, 19,1, "_Show hidden files" },
	{ SGBUTTON, SG_DEFAULT, 0, 32,23, 8,1, "OK" },
	{ SGBUTTON, SG_CANCEL, 0, 50,23, 8,1, "Cancel" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
};
#else
	{ SGBOX, 0, 0, 0,0, 64,25, NULL },
	{ SGTEXT, 0, 0, 1,1, 13,1, NULL, },
	{ SGTEXT, 0, 0, 1,2, 7,1, "Folder:" },
	{ SGTEXT, 0, 0, 1,3, DLGPATH_SIZE,1, dlgpath },
	{ SGTEXT, 0, 0, 1,4, 6,1, "File:" },
	{ SGTEXT, 0, 0, 7,4, DLGFNAME_SIZE,1, dlgfname },
	{ SGBUTTON, 0, 0, 1,1, 5,1, "Eject" },
	{ SGBUTTON, 0, 0, 48,1, 4,1, "Up" },
	{ SGBUTTON, 0, 0, 53,1, 4,1, "SD" },
	{ SGBUTTON, 0, 0, 58,1, 4,1, "USB" },
	{ SGBOX, 0, 0, 1,6, 62,16, NULL },
	{ SGBOX, 0, 0, 60,7, 3,14, NULL },
	{ SGTEXT, SG_EXIT, 0, 2,6, DLGFILENAMES_SIZE,2, dlgfilenames[0] },
	{ SGTEXT, SG_EXIT, 0, 2,8, DLGFILENAMES_SIZE,2, dlgfilenames[1] },
	{ SGTEXT, SG_EXIT, 0, 2,10, DLGFILENAMES_SIZE,2, dlgfilenames[2] },
	{ SGTEXT, SG_EXIT, 0, 2,12, DLGFILENAMES_SIZE,2, dlgfilenames[3] },
	{ SGTEXT, SG_EXIT, 0, 2,14, DLGFILENAMES_SIZE,2, dlgfilenames[4] },
	{ SGTEXT, SG_EXIT, 0, 2,16, DLGFILENAMES_SIZE,2, dlgfilenames[5] },
	{ SGTEXT, SG_EXIT, 0, 2,18, DLGFILENAMES_SIZE,2, dlgfilenames[6] },
	{ SGTEXT, SG_EXIT, 0, 2,20, DLGFILENAMES_SIZE,2, dlgfilenames[7] },
	{ SGSCROLLBAR, SG_TOUCHEXIT, 0, 60, 7, 0, 0, NULL },       /* Scrollbar */
	{ SGBUTTON,   SG_TOUCHEXIT, 0, 60, 6,3,1, "\x01", SG_SHORTCUT_UP },
	{ SGBUTTON,   SG_TOUCHEXIT, 0, 60,21,3,1, "\x02", SG_SHORTCUT_DOWN },
	{ SGBUTTON, SG_DEFAULT, 0, 38,23, 8,1, "OK" },
	{ SGBUTTON, SG_CANCEL, 0, 50,23, 8,1, "Cancel" },
	{ SGBUTTON, 0, 0, 800, 70, 6,1, "PageUp" },   /* Hidden button, scroll PageUp */
	{ SGBUTTON, 0, 0, 810, 70, 6,1, "PageDwn" },  /* Hidden button, scroll PageDown */
	{ -1, 0, 0, 0,0, 0,0, NULL }
};

/* The image viewer dialog */
static SGOBJ imgdlg[] =
{
	{ SGBOX, 0, 0, 0,0, 0,0, NULL },
	{ SGBUTTON, SG_DEFAULT, 0, 19,8, 0,0, "", 1 },      /* Go Up */
	{ SGBUTTON, SG_DEFAULT, 0, 19,12, 0,0, "", 2 },   /* Go Down */
	{ SGBUTTON, SG_DEFAULT, 0, 12,10, 0,0, "", 3 },   /* Go Left */
	{ SGBUTTON, SG_DEFAULT, 0, 26,10, 0,0, "", 4 },  /* Go Right */
	{ SGBUTTON, SG_DEFAULT, 0, 38,600, 8,1, "OK" },
	{ SGBUTTON, SG_CANCEL, 0, 50,600, 8,1, "Back" },
	{ SGBUTTON, SG_PAGEUP, 0, 800, 70, 6,1, "ImgPageUp", 5 },    /* Hidden button, scroll PageUp */
	{ SGBUTTON, SG_PAGEDOWN, 0, 810, 70, 6,1, "ImgPageDwn", 6},  /* Hidden button, scroll PageDown */
	{ -1, 0, 0, 0,0, 0,0, NULL }
};
#endif

static int ypos = -1;				/* First entry number to be displayed. If -1, file selector start on the 1st file */
						/* else we continue from the previous position when SDLGui_FileSelect is called again */
static bool refreshentries;			/* Do we have to update the file names in the dialog? */
static int entries;				/* How many files are in the actual directory? */
static int oldMouseY = 0;			/* Keep the latest Y mouse position for scrollbar move computing */
static int mouseClicked = 0;			/* used to know if mouse if down for the first time or not */
static int mouseIsOut = 0;			/* used to keep info that mouse if above or under the scrollbar when mousebutton is down */
static float scrollbar_Ypos = 0.0;		/* scrollbar heigth */

static char *dirpath;				/* for get_dtype() */
#ifndef HAVE_DIRENT_D_TYPE
enum {
	DT_UNKNOWN,
	DT_LNK,
	DT_DIR,
	DT_REG
};
#endif

/* Convert file position (in file list) to scrollbar y position */
static void DlgFileSelect_Convert_ypos_to_scrollbar_Ypos(void);



/*-----------------------------------------------------------------------*/
/**
 * Update the file name strings in the dialog.
 * Returns false if it failed, true on success.
 */
static int DlgFileSelect_RefreshEntries(struct dirent **files, char *path, bool browsingzip)
{
	int i;
	char *tempstr = malloc(FILENAME_MAX);

	if (!tempstr)
	{
		perror("DlgFileSelect_RefreshEntries");
		return false;
	}

	/* Copy entries to dialog: */
	for (i=0; i<SGFS_NUMENTRIES; i++)
	{
		if (i+ypos < entries)
		{
			struct stat filestat;
			/* Prepare entries: */
			strcpy(tempstr, "  ");
			strcat(tempstr, files[i+ypos]->d_name);
			File_ShrinkName(dlgfilenames[i], tempstr, DLGFILENAMES_SIZE);
			/* Mark folders: */
			strcpy(tempstr, path);
			strcat(tempstr, files[i+ypos]->d_name);

			if (browsingzip)
			{
				if (File_DoesFileNameEndWithSlash(tempstr))
					dlgfilenames[i][0] = SGFOLDER;    /* Mark folders */
			}
			else
			{
				if( stat(tempstr, &filestat)==0 && S_ISDIR(filestat.st_mode) )
					dlgfilenames[i][0] = SGFOLDER;    /* Mark folders */
				if (ZIP_FileNameIsZIP(tempstr) && browsingzip == false)
					dlgfilenames[i][0] = SGFOLDER;    /* Mark .ZIP archives as folders */
			}

			fsdlg[SGFSDLG_ENTRYFIRST+i].flags |= SG_EXIT;
		}
		else
		{
			dlgfilenames[i][0] = 0;  /* Clear entry */
			fsdlg[SGFSDLG_ENTRYFIRST+i].flags &= ~SG_EXIT;
		}
	}

	free(tempstr);
	return true;
}


/*-----------------------------------------------------------------------*/
/**
 * Remove all hidden files (files with file names that begin with a dot) from
 * the list.
 */
static void DlgFileSelect_RemoveHiddenFiles(struct dirent **files)
{
	int i;
	int nActPos = -1;
	int nOldEntries;

	nOldEntries = entries;

	/* Scan list for hidden files and remove them. */
	for (i = 0; i < nOldEntries; i++)
	{
		/* Does file name start with a dot? -> hidden file! */
#ifdef GEKKO
		/* Don't remove the up directory, easier browsing with d-pad */
		if (files[i]->d_name[0] == '.' && strcmp(files[i]->d_name, "..") != 0)
#else
		if (files[i]->d_name[0] == '.')
#endif
		{
			if (nActPos == -1)
				nActPos = i;
			/* Remove file from list: */
			free(files[i]);
			files[i] = NULL;
			entries -= 1;
		}
	}

	/* Now close the gaps in the list: */
	if (nActPos != -1)
	{
		for (i = nActPos; i < nOldEntries; i++)
		{
			if (files[i] != NULL)
			{
				/* Move entry to earlier position: */
				files[nActPos] = files[i];
				files[i] = NULL;
				nActPos += 1;
			}
		}
	}
}

/**
 * Reset focus to first entry if necessary.
 */
static void DlgFileSelect_ResetFocus(void)
{
	int i;

	for (i = SGFSDLG_ENTRYFIRST+1; i <= SGFSDLG_ENTRYLAST; i++)
	{
		if (fsdlg[i].state & SG_FOCUSED)
		{
			fsdlg[i].state &= ~SG_FOCUSED;
			fsdlg[SGFSDLG_ENTRYFIRST].state |= SG_FOCUSED;
			break;
		}
	}
}

/*-----------------------------------------------------------------------*/
/**
 * Prepare to scroll up one entry.
 */
static void DlgFileSelect_ScrollUp(void)
{
	if (ypos > 0)
	{
		--ypos;

		DlgFileSelect_Convert_ypos_to_scrollbar_Ypos();
		refreshentries = true;
	}
}


/*-----------------------------------------------------------------------*/
/**
 * Prepare to scroll down one entry.
 */
static void DlgFileSelect_ScrollDown(void)
{
	if (ypos+SGFS_NUMENTRIES < entries)
	{
		++ypos;

		DlgFileSelect_Convert_ypos_to_scrollbar_Ypos();
		refreshentries = true;
	}
}

/*-----------------------------------------------------------------------*/
/**
 * Manage the scrollbar up or down.
 */
static void DlgFileSelect_ManageScrollbar(void)
{
	int b, x, y;
	int scrollY, scrollYmin, scrollYmax, scrollH_half;
	float scrollMove;

	SDL_GetMouseState(&x, &y);

	/* If mouse is down on the scrollbar for the first time */
	if (fsdlg[SGFSDLG_SCROLLBAR].state & SG_MOUSEDOWN) {
		if (mouseClicked == 0) {
			mouseClicked = 1;
			mouseIsOut = 0;
			oldMouseY = y;
		}
	}
	/* Mouse button is up on the scrollbar */
	else {
		mouseClicked = 0;
		oldMouseY = y;
		mouseIsOut = 0;
	}

	/* If mouse Y position didn't change */
	if (oldMouseY == y)
		return;

	/* Compute scrollbar ymin and ymax values */

	scrollYmin = (fsdlg[SGFSDLG_SCROLLBAR].y + fsdlg[0].y) * sdlgui_fontheight;
	scrollYmax = (fsdlg[SGFSDLG_DOWN].y + fsdlg[0].y) * sdlgui_fontheight;

	scrollY = fsdlg[SGFSDLG_SCROLLBAR].y * sdlgui_fontheight + fsdlg[SGFSDLG_SCROLLBAR].h + fsdlg[0].y * sdlgui_fontheight;
	scrollH_half = scrollY + fsdlg[SGFSDLG_SCROLLBAR].w / 2;
	scrollMove = (float)(y-oldMouseY)/sdlgui_fontheight;

	/* Verify if mouse is not above the scrollbar area */
	if (y < scrollYmin) {
		mouseIsOut = SCROLLOUT_ABOVE;
		oldMouseY = y;
		return;
	}
	if (mouseIsOut == SCROLLOUT_ABOVE && y < scrollH_half) {
		oldMouseY = y;
		return;
	}

	/* Verify if mouse is not under the scrollbar area */
	if (y > scrollYmax) {
		mouseIsOut = SCROLLOUT_UNDER;
		oldMouseY = y;
		return;
	}
	if (mouseIsOut == SCROLLOUT_UNDER && y > scrollH_half) {
		oldMouseY = y;
		return;
	}

	mouseIsOut = 0;

	scrollbar_Ypos += scrollMove;
	oldMouseY = y;

	/* Verifiy if scrollbar is in correct inferior boundary */
	if (scrollbar_Ypos < 0)
		scrollbar_Ypos = 0.0;

	/* Verifiy if scrollbar is in correct superior boundary */
	b = (int) (scrollbar_Ypos * ((float)entries/(float)(SGFS_NUMENTRIES-2)) + 0.5);
	if (b+SGFS_NUMENTRIES >= entries) {
		ypos = entries - SGFS_NUMENTRIES;
		DlgFileSelect_Convert_ypos_to_scrollbar_Ypos();
	}

	refreshentries = true;
}

/*-----------------------------------------------------------------------*/
/**
 * Handle SDL events.
 */
static void DlgFileSelect_HandleSdlEvents(SDL_Event *pEvent)
{
	int oldypos = ypos;
	switch (pEvent->type)
	{
#if WITH_SDL2
	 case SDL_MOUSEWHEEL:
		if (pEvent->wheel.y>0)
			DlgFileSelect_ScrollUp();
		else if (pEvent->wheel.y<0)
			DlgFileSelect_ScrollDown();
		break;
#else
	 case SDL_MOUSEBUTTONDOWN:
		if (pEvent->button.button == SDL_BUTTON_WHEELUP)
			DlgFileSelect_ScrollUp();
		else if (pEvent->button.button == SDL_BUTTON_WHEELDOWN)
			DlgFileSelect_ScrollDown();
		break;
#endif
	 case SDL_KEYDOWN:
		switch (pEvent->key.keysym.sym)
		{
		 case SDLK_UP:
			DlgFileSelect_ScrollUp();
			break;
		 case SDLK_DOWN:
			DlgFileSelect_ScrollDown();
			break;
		 case SDLK_HOME:
			ypos = 0;
			DlgFileSelect_Convert_ypos_to_scrollbar_Ypos();
			break;
		 case SDLK_END:
		    ypos = entries-SGFS_NUMENTRIES;
			DlgFileSelect_Convert_ypos_to_scrollbar_Ypos();
		    break;
		 case SDLK_PAGEUP:
		    ypos -= SGFS_NUMENTRIES;
			DlgFileSelect_Convert_ypos_to_scrollbar_Ypos();
		    break;
		 case SDLK_PAGEDOWN:
			if (ypos+2*SGFS_NUMENTRIES < entries)
				ypos += SGFS_NUMENTRIES;
			else
				ypos = entries-SGFS_NUMENTRIES;
			DlgFileSelect_Convert_ypos_to_scrollbar_Ypos();
			break;
		 default:
			break;
		}
		break;
	default:
		break;
	}

	if (ypos < 0) {
		ypos = 0;
		scrollbar_Ypos = 0.0;
	}

	if (ypos != oldypos)
		refreshentries = true;
}


/*-----------------------------------------------------------------------*/
/**
 * Free file entries
 */
static struct dirent **files_free(struct dirent **files)
{
	int i;
	if (files != NULL)
	{
		for(i=0; i<entries; i++)
		{
			free(files[i]);
		}
		free(files);
	}
	return NULL;
}


/*-----------------------------------------------------------------------*/
/**
 * Copy to dst src+add if they are below maxlen and return true,
 * otherwise return false
 */
static int strcat_maxlen(char *dst, int maxlen, const char *src, const char *add)
{
	int slen, alen;
	slen = strlen(src);
	alen = strlen(add);
	if (slen + alen < maxlen)
	{
		strcpy(dst, src);
		strcpy(dst+slen, add);
		return 1;
	}
	return 0;
}

/*-----------------------------------------------------------------------*/
/**
 * Get given file's type, directory or a file.
 * (if name itself is symlink, stat() checks file it points to)
 */
static int get_dtype(const char *name)
{
	struct stat buf;
	char path[FILENAME_MAX];

	snprintf(path, sizeof(path), "%s%c%s", dirpath, PATHSEP, name);
#ifdef GEKKO
	/* stat fails here. We could use d_type==DT_DIR and declare buf as dirent.*/
	/* it's just quicker, and strcmp should be enough. */
	if(strcmp(name, "..") == 0)
#else
	if (stat(path, &buf) == 0 && S_ISDIR(buf.st_mode))
#endif
		return DT_DIR;
	else
		return DT_REG;
}

/*-----------------------------------------------------------------------*/
/**
 * Case insensitive sorting for directory entry names, so
 * that directory entries are listed first.
 */
static int filesort(const struct dirent **d1, const struct dirent **d2)
{
	const char *name1 = (*d1)->d_name;
	const char *name2 = (*d2)->d_name;
#ifndef HAVE_DIRENT_D_TYPE
	int type1 = DT_UNKNOWN;
	int type2 = DT_UNKNOWN;
#else
	int type1 = (*d1)->d_type;
	int type2 = (*d2)->d_type;
#endif

	/* OS / file system that doesn't support d_type field, or symlink */
	if (type1 == DT_UNKNOWN || type1 == DT_LNK)
		type1 = get_dtype(name1);
	if (type2 == DT_UNKNOWN || type2 == DT_LNK)
		type2 = get_dtype(name2);

	if (type1 == DT_DIR)
	{
		if (type2 != DT_DIR)
			return -1;
	} else if (type2 == DT_DIR)
	{
		if (type1 != DT_DIR)
			return 1;
	}
	return strcasecmp(name1, name2);
}

/*-----------------------------------------------------------------------*/
/**
 * Create and return suitable path into zip file
 */
static char* zip_get_path(const char *zipdir, const char *zipfilename, int browsingzip)
{
	if (browsingzip)
	{
		char *zippath;
		zippath = malloc(strlen(zipdir) + strlen(zipfilename) + 1);
		strcpy(zippath, zipdir);
		strcat(zippath, zipfilename);
		return zippath;
	}
	return strdup("");
}

/**
 * string for zip root needs to be empty, check and correct if needed
 */
static void correct_zip_root(char *zippath)
{
	if (zippath[0] == PATHSEP && !zippath[1])
	{
		zippath[0] = '\0';
	}
}

/**
 * Convert Ypos to Y scrollbar position
 */
static void DlgFileSelect_Convert_ypos_to_scrollbar_Ypos(void)
{
	if (entries <= SGFS_NUMENTRIES)
		scrollbar_Ypos = 0.0;
	else
		scrollbar_Ypos = (float)ypos / ((float)entries/(float)(SGFS_NUMENTRIES-2));
}

/*-----------------------------------------------------------------------*/
/**
 * Show and process a file selection dialog.
 * Returns path/name user selected or NULL if user canceled
 * input: zip_path = pointer's pointer to buffer to contain file path
 * within a selected zip file, or NULL if browsing zip files is disallowed.
 * bAllowNew: true if the user is allowed to insert new file names.
 */
char* SDLGui_FileSelect(const char *title, const char *path_and_name, char **zip_path, bool bAllowNew)
{
	struct dirent **files = NULL;
	char *pStringMem;
	char *retpath = NULL;
	const char *home;
	char *path, *fname;                 /* The actual file and path names */
	bool reloaddir = true;              /* Do we have to reload the directory file list? */
	int retbut, len;
	bool bOldMouseVisibility;
	int selection;                      /* The selection index */
	char *zipfilename;                  /* Filename in zip file */
	char *zipdir;
	bool browsingzip = false;           /* Are we browsing an archive? */
	zip_dir *zipfiles = NULL;
	SDL_Event sdlEvent;
	int yScrollbar_size;                /* Size of the vertical scrollbar */
	union {
		char *mtxt;
		const char *ctxt;
	} dlgtitle;                         /* A hack to silent recent GCCs warnings */
	bool KeepCurrentObject;

	dlgtitle.ctxt = title;

#ifdef GEKKO
	bool browsingfloppy;
	browsingfloppy = ( floppyeject == 0 || floppyeject == 1 ) ? true : false;

	/* Show eject button when browsing floppys */
	if (browsingfloppy)
	{
		fsdlg[SGFSDLG_EJECT].type = SGBUTTON;
		fsdlg[SGFSDLG_EJECT].x = 1; 
	}
	else
	{
		fsdlg[SGFSDLG_EJECT].type = SGTEXT;
		fsdlg[SGFSDLG_EJECT].x= 600;
		floppyeject = -1;
	}
#endif

	/* If this is the first call to SDLGui_FileSelect, we reset scrollbar_Ypos and ypos */
	/* Else, we keep the previous value of scrollbar_Ypos and update ypos below, to open */
	/* the fileselector at the same position it was used */
	if ( ypos < 0 )
	{
		scrollbar_Ypos = 0.0;
		ypos = 0;
	}

	refreshentries = true;
	entries = 0;

	/* Allocate memory for the file and path name strings: */
	pStringMem = malloc(4 * FILENAME_MAX);
	path = pStringMem;
	fname = pStringMem + FILENAME_MAX;
	zipdir = pStringMem + 2 * FILENAME_MAX;
	zipfilename = pStringMem + 3 * FILENAME_MAX;
	zipfilename[0] = 0;
	fname[0] = 0;
	path[0] = 0;

	len = strlen(title);
	fsdlg[SGFSDLG_TITLE].txt = dlgtitle.mtxt;
	fsdlg[SGFSDLG_TITLE].x = TITLE_OFFSET + (TITLE_MAXLEN-len)/2;
	fsdlg[SGFSDLG_TITLE].w = len;

	/* Save mouse state and enable cursor */
	bOldMouseVisibility = SDL_ShowCursor(SDL_QUERY);
	SDL_ShowCursor(SDL_ENABLE);

	SDLGui_CenterDlg(fsdlg);
	if (bAllowNew)
	{
		fsdlg[SGFSDLG_FILENAME].type = SGEDITFIELD;
		fsdlg[SGFSDLG_FILENAME].flags |= SG_EXIT;
	}
	else
	{
		fsdlg[SGFSDLG_FILENAME].type = SGTEXT;
		fsdlg[SGFSDLG_FILENAME].flags &= ~SG_EXIT;
	}

	/* Prepare the path and filename variables */
	if (path_and_name && path_and_name[0])
	{
		strncpy(path, path_and_name, FILENAME_MAX);
		path[FILENAME_MAX-1] = '\0';
	}
	if (!File_DirExists(path))
	{
		File_SplitPath(path, path, fname, NULL);
		if (!(File_DirExists(path) || getcwd(path, FILENAME_MAX)))
		{
			perror("SDLGui_FileSelect: non-existing path and CWD failed");
			goto clean_exit;
		}
	}

	File_MakeAbsoluteName(path);
	File_MakeValidPathName(path);
	File_ShrinkName(dlgpath, path, DLGPATH_SIZE);
	File_ShrinkName(dlgfname, fname, DLGFNAME_SIZE);

	/* The first time we display the dialog, we reset the current position */
	/* On next calls, current_object's value will be kept to handle scrolling */
	KeepCurrentObject = false;

	do
	{
		if (reloaddir)
		{
			files = files_free(files);

			if (browsingzip)
			{
				files = ZIP_GetFilesDir(zipfiles, zipdir, &entries);
				if(!files)
				{
					fprintf(stderr, "SDLGui_FileSelect: ZIP_GetFilesDir error!\n");
					goto clean_exit;
				}
			}
			else
			{
				/* for get_dtype() */
				dirpath = path;
				/* Load directory entries: */
				entries = scandir(path, &files, NULL, filesort);
			}

#ifndef GEKKO
			/* Remove hidden files from the list if necessary: */
			if (!(fsdlg[SGFSDLG_SHOWHIDDEN].state & SG_SELECTED))
			{
				DlgFileSelect_RemoveHiddenFiles(files);
			}
#else
			/* Always remove hidden files */
			DlgFileSelect_RemoveHiddenFiles(files);
#endif

			if (entries < 0)
			{
				fprintf(stderr, "SDLGui_FileSelect: Path not found.\n");
				goto clean_exit;
			}

			/* reload always implies refresh */
			reloaddir = false;
			refreshentries = true;

			/* Check if focus was in list - if yes then reset to first entry */
			DlgFileSelect_ResetFocus();
		}/* reloaddir */

		/* Refresh scrollbar size */
		if (entries <= SGFS_NUMENTRIES)
			yScrollbar_size = (SGFS_NUMENTRIES-2) * sdlgui_fontheight;
		else
		{
			yScrollbar_size = (int)((SGFS_NUMENTRIES-2) / ((float)entries/(float)SGFS_NUMENTRIES) * sdlgui_fontheight);
			if ( yScrollbar_size < SCROLLBAR_MIN_HEIGHT )		/* Value could be 0 for very large directory */
				yScrollbar_size = SCROLLBAR_MIN_HEIGHT;
		}
		fsdlg[SGFSDLG_SCROLLBAR].w = yScrollbar_size;

		/* Refresh scrolbar pos */
		ypos = (int) (scrollbar_Ypos * ((float)entries/(float)(SGFS_NUMENTRIES-2)) + 0.5);

		if (ypos+SGFS_NUMENTRIES >= entries) {			/* Ensure Y pos is in the correct boundaries */
			ypos = entries - SGFS_NUMENTRIES;
			if ( ypos < 0 )
				ypos = 0;
			DlgFileSelect_Convert_ypos_to_scrollbar_Ypos();
		}
		fsdlg[SGFSDLG_SCROLLBAR].h = (int) (scrollbar_Ypos * sdlgui_fontheight)*2;

		/* Update the file name strings in the dialog? */
		if (refreshentries)
		{
			if (!DlgFileSelect_RefreshEntries(files, path, browsingzip))
			{
				goto clean_exit;
			}
			refreshentries = false;
		}

		/* Show dialog: */
#ifdef GEKKO
		retbut = SDLGui_DoDialog(fsdlg, NULL, KeepCurrentObject);
#else
		retbut = SDLGui_DoDialog(fsdlg, &sdlEvent, KeepCurrentObject);
#endif
		KeepCurrentObject = true;				/* Don't reset current_object for next calls */

		/* Has the user clicked on a file or folder? */
		if (retbut>=SGFSDLG_ENTRYFIRST && retbut<=SGFSDLG_ENTRYLAST && retbut-SGFSDLG_ENTRYFIRST+ypos<entries)
		{
			char *tempstr;

			tempstr = malloc(FILENAME_MAX);
			if (!tempstr)
			{
				perror("Error while allocating temporary memory in SDLGui_FileSelect()");
				goto clean_exit;
			}

			if (browsingzip == true)
			{
				if (!strcat_maxlen(tempstr, FILENAME_MAX,
						   zipdir, files[retbut-SGFSDLG_ENTRYFIRST+ypos]->d_name))
				{
					fprintf(stderr, "SDLGui_FileSelect: Path name too long!\n");
					free(tempstr);
					goto clean_exit;
				}
				/* directory? */
				if (File_DoesFileNameEndWithSlash(tempstr))
				{
					/* handle the ../ directory */
					if (strcmp(files[retbut-SGFSDLG_ENTRYFIRST+ypos]->d_name, "../") == 0)
					{
						/* close the zip file */
						if (strcmp(tempstr, "../") == 0)
						{
							/* free zip file entries */
							ZIP_FreeZipDir(zipfiles);
							zipfiles = NULL;
							/* Copy the path name to the dialog */
							File_ShrinkName(dlgpath, path, DLGPATH_SIZE);
							browsingzip = false;
						}
						else
						{
							/* remove "../" and previous dir from path */
							File_PathShorten(tempstr, 2);
							correct_zip_root(tempstr);
							strcpy(zipdir, tempstr);
							File_ShrinkName(dlgpath, zipdir, DLGPATH_SIZE);
						}
					}
					else /* not the "../" directory */
					{
						strcpy(zipdir, tempstr);
						File_ShrinkName(dlgpath, zipdir, DLGPATH_SIZE);
					}
					reloaddir = true;
					/* Copy the path name to the dialog */
					zipfilename[0] = '\0';
					dlgfname[0] = 0;
					ypos = 0;
					scrollbar_Ypos = 0.0;
				}
				else
				{
					/* not dir, select a file in the zip */
					selection = retbut-SGFSDLG_ENTRYFIRST+ypos;
					strcpy(zipfilename, files[selection]->d_name);
					File_ShrinkName(dlgfname, zipfilename, DLGFNAME_SIZE);
				}

			}
			else /* not browsingzip */
			{
				if (!strcat_maxlen(tempstr, FILENAME_MAX,
						   path, files[retbut-SGFSDLG_ENTRYFIRST+ypos]->d_name))
				{
					fprintf(stderr, "SDLGui_FileSelect: Path name too long!\n");
					free(tempstr);
					goto clean_exit;
				}
				if (File_DirExists(tempstr))
				{
					File_HandleDotDirs(tempstr);
					File_AddSlashToEndFileName(tempstr);
					/* Copy the path name to the dialog */
					File_ShrinkName(dlgpath, tempstr, DLGPATH_SIZE);
					strcpy(path, tempstr);
					reloaddir = true;
					dlgfname[0] = 0;
					ypos = 0;
					scrollbar_Ypos = 0.0;
				}
				else if (ZIP_FileNameIsZIP(tempstr) && zip_path != NULL)
				{
					/* open a zip file */
					zipfiles = ZIP_GetFiles(tempstr);
					if (zipfiles != NULL && browsingzip == false)
					{
						selection = retbut-SGFSDLG_ENTRYFIRST+ypos;
						strcpy(fname, files[selection]->d_name);
						File_ShrinkName(dlgfname, fname, DLGFNAME_SIZE);
						browsingzip = true;
						zipdir[0] = '\0'; /* zip root */
						File_ShrinkName(dlgpath, zipdir, DLGPATH_SIZE);
						reloaddir = true;
						ypos = 0;
						scrollbar_Ypos = 0.0;
					}

				}
				else
				{
					/* Select a file */
					selection = retbut-SGFSDLG_ENTRYFIRST+ypos;
					strcpy(fname, files[selection]->d_name);
					File_ShrinkName(dlgfname, fname, DLGFNAME_SIZE);

#ifdef GEKKO
					/*  If it's an image, try to display it */
					if (File_DoesFileExtensionMatch(dlgfname, ".bmp") || File_DoesFileExtensionMatch(dlgfname, ".png")
						|| File_DoesFileExtensionMatch(dlgfname, ".jpg") || File_DoesFileExtensionMatch(dlgfname, ".jpeg"))
					{
						SDLGui_FileImgSelect(path, fname);
					}

					/*  If it's a save file, try to open it */
					if (File_DoesFileExtensionMatch(dlgfname, ".sav"))
					{
						bool restoresnap;
						char *savepath;

						savepath = malloc(FILENAME_MAX);
						if (!savepath)
						{
							perror("Error while allocating temporary memory in SDLGui_FileSelect()");
							goto clean_exit;
						}
					
						sprintf(savepath,"%s/%s", path, fname);
						restoresnap = DlgAlert_Query("Do you want to restore this memory snapshot?");

						if (restoresnap)
						{
							MemorySnapShot_Restore(savepath, true);
							SDL_ShowCursor(SDL_ENABLE);
							return (retbut == SGFSDLG_OKAY);
						}
					}
#endif
				}

			} /* not browsingzip */

			free(tempstr);
		}
		else    /* Has the user clicked on another button? */
		{
			switch(retbut)
			{
#ifdef GEKKO
			case SGFSDLG_EJECT:				
				if ( floppyeject == 0 )                         /* Eject disk in drive A: */	
					fname[0] = 0;																							
				else if ( floppyeject == 1 )                    /* Eject disk in drive B: */
					fname[0] = 0;

				/* Remove filename from dialog */
				dlgfname[0] = 0;
				break;
#endif
			case SGFSDLG_UPDIR:                 /* Change path to parent directory */
				if (browsingzip)
				{
					/* close the zip file? */
					if (!zipdir[0])
					{
						/* free zip file entries */
						ZIP_FreeZipDir(zipfiles);
						browsingzip = false;
						zipfiles = NULL;
						File_ShrinkName(dlgpath, path, DLGPATH_SIZE);
					}
					else
					{
						/* remove last dir from zipdir path */
						File_PathShorten(zipdir, 1);
						correct_zip_root(zipdir);
						File_ShrinkName(dlgpath, zipdir, DLGPATH_SIZE);
						zipfilename[0] = '\0';
					}
				}  /* not a zip file: */
				else
				{
					File_PathShorten(path, 1);
					File_ShrinkName(dlgpath, path, DLGPATH_SIZE);
				}
				reloaddir = true;
				break;
#ifndef GEKKO
			case SGFSDLG_HOMEDIR:               /* Change to home directory */
			case SGFSDLG_CWD:                   /* Change to current work directory */
				if (retbut == SGFSDLG_CWD)
					home = Paths_GetWorkingDir();
				else
					home = Paths_GetUserHome();
				if (home == NULL || !*home)
					break;
				if (browsingzip)
				{
					/* free zip file entries */
					ZIP_FreeZipDir(zipfiles);
					zipfiles = NULL;
					browsingzip = false;
				}
				strcpy(path, home);
				File_AddSlashToEndFileName(path);
				File_ShrinkName(dlgpath, path, DLGPATH_SIZE);
				reloaddir = true;
				break;

			case SGFSDLG_ROOTDIR:               /* Change to root directory */
				if (browsingzip)
				{
					/* free zip file entries */
					ZIP_FreeZipDir(zipfiles);
					zipfiles = NULL;
					browsingzip = false;
				}
				path[0] = PATHSEP; path[1] = '\0';
				strcpy(dlgpath, path);
				reloaddir = true;
				break;
#else
			case SGFSDLG_SDCARD:                /* Change to SD/USB card default directory */
			case SGFSDLG_USB:					
				if (browsingfloppy)             /* Default floppy directory */
				{
					if (retbut == SGFSDLG_SDCARD)
						home = Paths_GetGamesDir(0,"fd");
					else
						home = Paths_GetGamesDir(1,"fd");
				}
				else                            /* Default /hatari directory */
				{
					if (retbut == SGFSDLG_SDCARD)
						home = Paths_GetGamesDir(0,"");
					else
						home = Paths_GetGamesDir(1,"");
				}

				if (home == NULL || !*home)
					break;

				if (browsingzip)
				{
					/* free zip file entries */
					ZIP_FreeZipDir(zipfiles);
					zipfiles = NULL;
					browsingzip = false;
				}
				strcpy(path, home);
				File_AddSlashToEndFileName(path);
				File_ShrinkName(dlgpath, path, DLGPATH_SIZE);
				reloaddir = true;
				break;
#endif
			case SGFSDLG_UP:                    /* Scroll up */
				DlgFileSelect_ScrollUp();
				SDL_Delay(10);
				break;
			case SGFSDLG_DOWN:                  /* Scroll down */
				DlgFileSelect_ScrollDown();
				SDL_Delay(10);
				break;
			case SGFSDLG_SCROLLBAR:             /* Scrollbar selected */
				DlgFileSelect_ManageScrollbar();
				SDL_Delay(10);
				break;
			case SGFSDLG_FILENAME:              /* User entered new filename */
				strcpy(fname, dlgfname);
				break;
#ifdef GEKKO
			case SGFSDLG_PAGEUP:              	/* Scroll up one page */
				ypos -= SGFS_NUMENTRIES;
				DlgFileSelect_Convert_ypos_to_scrollbar_Ypos();

				if (ypos < 0) {
					ypos = 0;
					scrollbar_Ypos = 0.0;
				}
					refreshentries = true;
				break;
			case SGFSDLG_PAGEDOWN:              /* Scroll down one page */
				if (ypos+2*SGFS_NUMENTRIES < entries)
					ypos += SGFS_NUMENTRIES;
				else
					ypos = entries-SGFS_NUMENTRIES;
				DlgFileSelect_Convert_ypos_to_scrollbar_Ypos();

				if (ypos < 0) {
					ypos = 0;
					scrollbar_Ypos = 0.0;
				}
					refreshentries = true;
				break;
#endif

#ifndef GEKKO
			case SGFSDLG_SHOWHIDDEN:            /* Show/hide hidden files */
				reloaddir = true;
				ypos = 0;
				scrollbar_Ypos = 0.0;
				break;
#endif
			case SDLGUI_UNKNOWNEVENT:
				DlgFileSelect_HandleSdlEvents(&sdlEvent);
				break;
			} /* switch */

			if (reloaddir)
			{
				/* Remove old selection */
				fname[0] = 0;
				dlgfname[0] = 0;
				ypos = 0;
				scrollbar_Ypos = 0.0;
			}
		} /* other button code */
	} /* do */
	while (retbut!=SGFSDLG_OKAY && retbut!=SGFSDLG_CANCEL
	       && retbut!=SDLGUI_QUIT && retbut != SDLGUI_ERROR && !bQuitProgram);

	if (retbut == SGFSDLG_OKAY)
	{
		if (zip_path)
			*zip_path = zip_get_path(zipdir, zipfilename, browsingzip);
		retpath = File_MakePath(path, fname, NULL);
	}
	else
		retpath = NULL;

clean_exit:
	SDL_ShowCursor(bOldMouseVisibility);

	if (browsingzip && zipfiles != NULL)
	{
		/* free zip file entries */
		ZIP_FreeZipDir(zipfiles);
		zipfiles = NULL;
	}
	files_free(files);
	free(pStringMem);

#ifdef GEKKO
	floppyeject = -1;
#endif
	return retpath;
}

#ifdef GEKKO
/*-----------------------------------------------------------------------*/
/**
 * Display the image selected in SDLGui_FileSelect
 * File formats : bmp, png and jpg.
 * 
 * It can be used as a manual/docs viewer.
 * A 640*480 image will fit the screen only with borders enabled.
 * When the image is too large, change coordinates with the buttons.
 */
void SDLGui_FileImgSelect(char *path, char *dlgname)
{
	int button;
	char image[256];
	SDL_Surface *screen;
	SDL_Surface *temp = NULL;
	SDL_Surface *out = NULL;
	SDL_Rect src;

	SDLGui_CenterDlg(imgdlg);

	screen = SDL_GetVideoSurface();
	SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));

	sprintf (image, "%s/%s", path, dlgname);
	temp = IMG_Load(image);
	if(!temp)
		DlgAlert_Notice("Failed to open image!");

    src.w = temp->w;
    src.h = temp->h;
	src.x=0;
	src.y=0;
	SDL_BlitSurface(temp, &src, screen, NULL);

	do
	{
		/* Draw an empty box with 4 empty buttons containing the image */
		button = SDLGui_DoDialog(imgdlg, NULL, false);

		switch (button)
		{
		case IMGDLG_UP:
			if (src.y < 0)
				src.y = 0;
			src.y -= 60;
			SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
			SDL_BlitSurface(temp, &src, screen, NULL);
			break;
		case IMGDLG_DOWN:
			src.y += 60;
			SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
			SDL_BlitSurface(temp, &src, screen, NULL);
			break;
		case IMGDLG_LEFT:
			if (src.x < 60)
				src.x = 0;
			else
				src.x -= 60;
			SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
			SDL_BlitSurface(temp, &src, screen, NULL);
			break;
		case IMGDLG_RIGHT:
			src.x += 60;
			SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
			SDL_BlitSurface(temp, &src, screen, NULL);
			break;
		 case IMGDLG_BACK:                         
			break;
		 case IMGDLG_PAGEUP:                         
			if (src.y < 0)
				src.y = 0;
			src.y -= 400;
			SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
			SDL_BlitSurface(temp, &src, screen, NULL);
			break;
		 case IMGDLG_PAGEDOWN:                         
			src.y += 400;
			SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
			SDL_BlitSurface(temp, &src, screen, NULL);
			break;
		}
	}
	while ( button != IMGDLG_BACK && button != SDLGUI_ERROR && !bQuitProgram);

	SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));

	SDL_FreeSurface(temp);
	SDL_FreeSurface(screen);
}
#endif

/*-----------------------------------------------------------------------*/
/**
 * Let user browse for a file, confname is used as default.
 * If bAllowNew is true, user can select new files also.
 *
 * If no file is selected, or there's some problem with the file,
 * return false and clear dlgname & confname.
 * Otherwise return true, set dlgname & confname to the new file name
 * (dlgname is shrunken & limited to maxlen and confname is assumed
 * to have FILENAME_MAX amount of space).
 */
bool SDLGui_FileConfSelect(const char *title, char *dlgname, char *confname, int maxlen, bool bAllowNew)
{
	char *selname;

	selname = SDLGui_FileSelect(title, confname, NULL, bAllowNew);
	if (selname)
	{
		if (!File_DoesFileNameEndWithSlash(selname) &&
		    (bAllowNew || File_Exists(selname)))
		{
			strncpy(confname, selname, FILENAME_MAX);
			confname[FILENAME_MAX-1] = '\0';
			File_ShrinkName(dlgname, selname, maxlen);
		}
		else
		{
			dlgname[0] = confname[0] = 0;
		}
		free(selname);
		return true;
	}
	return false;
}
