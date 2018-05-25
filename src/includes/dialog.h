/*
  Hatari - dialog.h

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.
*/

#ifndef HATARI_DIALOG_H
#define HATARI_DIALOG_H

#include "configuration.h"

/* prototypes for gui-sdl/dlg*.c functions: */
extern int Dialog_MainDlg(bool *bReset, bool *bLoadedSnapshot);
extern void Dialog_AboutDlg(void);
extern void Dialog_HaltDlg(void);
extern int DlgAlert_Notice(const char *text);
extern int DlgAlert_Query(const char *text);
extern void Dialog_DeviceDlg(void);
extern void DlgFloppy_Main(void);
extern void DlgHardDisk_Main(void);
extern void Dialog_JoyDlg(void);
extern void Dialog_KeyboardDlg(void);
extern bool Dialog_MemDlg(void);
extern char* DlgNewDisk_Main(void);
extern void Dialog_MonitorDlg(void);
extern void Dialog_WindowDlg(void);
extern void Dialog_SoundDlg(void);
extern void Dialog_SystemDlg(void);
extern void DlgRom_Main(void);
/* and dialog.c */
extern bool Dialog_DoProperty(void);
#ifdef GEKKO
extern void Dialog_Virtualkeybdlg(void);
extern bool Dialog_Virtualkeyb(void);
extern bool Dialog_SaveDlg(void);
#endif

#endif