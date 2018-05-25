/*
  Hatari - main.c

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  Main initialization and event handling routines.
*/
const char Main_fileid[] = "Hatari main.c : " __DATE__ " " __TIME__;

#include <time.h>
#include <errno.h>
#include <SDL.h>

#include "main.h"
#include "version.h"
#include "configuration.h"
#include "control.h"
#include "options.h"
#include "dialog.h"
#include "audio.h"
#include "joy.h"
#include "floppy.h"
#include "floppy_ipf.h"
#include "floppy_stx.h"
#include "gemdos.h"
#include "fdc.h"
#include "hdc.h"
#include "ide.h"
#include "acia.h"
#include "ikbd.h"
#include "ioMem.h"
#include "keymap.h"
#include "log.h"
#include "m68000.h"
#include "memorySnapShot.h"
#include "midi.h"
#include "nvram.h"
#include "paths.h"
#include "printer.h"
#include "reset.h"
#include "resolution.h"
#include "rs232.h"
#include "screen.h"
#include "sdlgui.h"
#include "shortcut.h"
#include "sound.h"
#include "dmaSnd.h"
#include "statusbar.h"
#include "stMemory.h"
#include "str.h"
#include "tos.h"
#include "video.h"
#include "avi_record.h"
#include "debugui.h"
#include "clocks_timings.h"

#include "hatari-glue.h"

#include "falcon/hostscreen.h"
#include "falcon/dsp.h"

#if HAVE_GETTIMEOFDAY
#include <sys/time.h>
#endif

#ifdef WIN32
#include "gui-win/opencon.h"
#endif

bool bQuitProgram = false;                /* Flag to quit program cleanly */
static int nQuitValue;                    /* exit value */

static Uint32 nRunVBLs;                   /* Whether and how many VBLS to run before exit */
static Uint32 nFirstMilliTick;            /* Ticks when VBL counting started */
static Uint32 nVBLCount;                  /* Frame count */
static int nVBLSlowdown = 1;		  /* host VBL wait multiplier */

static bool bEmulationActive = true;      /* Run emulation when started */
static bool bAccurateDelays;              /* Host system has an accurate SDL_Delay()? */
static bool bIgnoreNextMouseMotion = false;  /* Next mouse motion will be ignored (needed after SDL_WarpMouse) */

#ifdef GEKKO
static int HatStKey[2][4];
static const int HatStArrow[4]={75,72,77,80};
bool Dpad_Dir[2][4];
bool MouseMode = false;

enum
{
	WIIPLAYER1,
	WIIPLAYER2
};
#endif

/*-----------------------------------------------------------------------*/
/**
 * Return current time as millisecond for performance measurements.
 * 
 * (On Unix only time spent by Hatari itself is counted, on other
 * platforms less accurate SDL "wall clock".)
 */
#if HAVE_SYS_TIMES_H
#include <unistd.h>
#include <sys/times.h>
static Uint32 Main_GetTicks(void)
{
	static unsigned int ticks_to_msec = 0;
	struct tms fields;
	if (!ticks_to_msec)
	{
		ticks_to_msec = sysconf(_SC_CLK_TCK);
		printf("OS clock ticks / second: %d\n", ticks_to_msec);
		/* Linux has 100Hz virtual clock so no accuracy loss there */
		ticks_to_msec = 1000UL / ticks_to_msec;
	}
	/* return milliseconds (clock ticks) spent in this process
	 */
	times(&fields);
	return ticks_to_msec * fields.tms_utime;
}
#else
# warning "times() function missing, using inaccurate SDL_GetTicks() instead."
# define Main_GetTicks SDL_GetTicks
#endif


//#undef HAVE_GETTIMEOFDAY
//#undef HAVE_NANOSLEEP

/*-----------------------------------------------------------------------*/
/**
 * Return a time counter in micro seconds.
 * If gettimeofday is available, we use it directly, else we convert the
 * return of SDL_GetTicks in micro sec.
 */

static Sint64	Time_GetTicks ( void )
{
	Sint64	ticks_micro;

#if HAVE_GETTIMEOFDAY
	struct timeval	now;
	gettimeofday ( &now , NULL );
	ticks_micro = (Sint64)now.tv_sec * 1000000 + now.tv_usec;
#else
	ticks_micro = (Sint64)SDL_GetTicks() * 1000;		/* milli sec -> micro sec */
#endif

	return ticks_micro;
}


/*-----------------------------------------------------------------------*/
/**
 * Sleep for a given number of micro seconds.
 * If nanosleep is available, we use it directly, else we use SDL_Delay
 * (which is portable, but less accurate as is uses milli-seconds)
 */

static void	Time_Delay ( Sint64 ticks_micro )
{
#if HAVE_NANOSLEEP
	struct timespec	ts;
	int		ret;
	ts.tv_sec = ticks_micro / 1000000;
	ts.tv_nsec = (ticks_micro % 1000000) * 1000;	/* micro sec -> nano sec */
	/* wait until all the delay is elapsed, including possible interruptions by signals */
	do
	{
                errno = 0;
                ret = nanosleep(&ts, &ts);
	} while ( ret && ( errno == EINTR ) );		/* keep on sleeping if we were interrupted */
#else
	SDL_Delay ( (Uint32)(ticks_micro / 1000) ) ;	/* micro sec -> milli sec */
#endif
}


/*-----------------------------------------------------------------------*/
/**
 * Pause emulation, stop sound.  'visualize' should be set true,
 * unless unpause will be called immediately afterwards.
 * 
 * @return true if paused now, false if was already paused
 */
bool Main_PauseEmulation(bool visualize)
{
	if ( !bEmulationActive )
		return false;

	Audio_EnableAudio(false);
	bEmulationActive = false;
	if (visualize)
	{
		if (nFirstMilliTick)
		{
			int interval = Main_GetTicks() - nFirstMilliTick;
			static float previous;
			float current;

			current = (1000.0 * nVBLCount) / interval;
#ifndef GEKKO
			printf("SPEED: %.1f VBL/s (%d/%.1fs), diff=%.1f%%\n",
			       current, nVBLCount, interval/1000.0,
			       previous>0.0 ? 100*(current-previous)/previous : 0.0);
#endif
			nVBLCount = nFirstMilliTick = 0;
			previous = current;
		}
		
		Statusbar_AddMessage("Emulation paused", 100);
		/* make sure msg gets shown */
		Statusbar_Update(sdlscrn, true);

		if (bGrabMouse && !bInFullScreen)
			/* Un-grab mouse pointer in windowed mode */
			SDL_WM_GrabInput(SDL_GRAB_OFF);
	}
	return true;
}

/*-----------------------------------------------------------------------*/
/**
 * Start/continue emulation
 * 
 * @return true if continued, false if was already running
 */
bool Main_UnPauseEmulation(void)
{
	if ( bEmulationActive )
		return false;

	Sound_BufferIndexNeedReset = true;
	Audio_EnableAudio(ConfigureParams.Sound.bEnableSound);
	bEmulationActive = true;

	/* Cause full screen update (to clear all) */
	Screen_SetFullUpdate();

	if (bGrabMouse)
		/* Grab mouse pointer again */
		SDL_WM_GrabInput(SDL_GRAB_ON);
	return true;
}

/*-----------------------------------------------------------------------*/
/**
 * Optionally ask user whether to quit and set bQuitProgram accordingly
 */
void Main_RequestQuit(int exitval)
{
	if (ConfigureParams.Memory.bAutoSave)
	{
		bQuitProgram = true;
		MemorySnapShot_Capture(ConfigureParams.Memory.szAutoSaveFileName, false);
	}
	else if (ConfigureParams.Log.bConfirmQuit)
	{
		bQuitProgram = false;	/* if set true, dialog exits */
		bQuitProgram = DlgAlert_Query("All unsaved data will be lost.\nDo you really want to quit?");
	}
	else
	{
		bQuitProgram = true;
	}

	if (bQuitProgram)
	{
		/* Assure that CPU core shuts down */
		M68000_SetSpecial(SPCFLAG_BRK);
	}
	nQuitValue = exitval;
}

/*-----------------------------------------------------------------------*/
/**
 * Set how many VBLs Hatari should run, from the moment this function
 * is called.
 */
void Main_SetRunVBLs(Uint32 vbls)
{
	fprintf(stderr, "Exit after %d VBLs.\n", vbls);
	nRunVBLs = vbls;
	nVBLCount = 0;
}

/*-----------------------------------------------------------------------*/
/**
 * Set VBL wait slowdown factor/multiplayer
 */
bool Main_SetVBLSlowdown(int factor)
{
	if (factor < 1 || factor > 8) {
		fprintf(stderr, "ERROR: invalid VBL slowdown factor %d, should be 1-8!\n", factor);
		return false;
	}
	fprintf(stderr, "Slow down host VBL wait by factor of %d.\n", factor);
	nVBLSlowdown = factor;
	return true;
}

/*-----------------------------------------------------------------------*/
/**
 * This function waits on each emulated VBL to synchronize the real time
 * with the emulated ST.
 * Unfortunately SDL_Delay and other sleep functions like usleep or nanosleep
 * are very inaccurate on some systems like Linux 2.4 or Mac OS X (they can only
 * wait for a multiple of 10ms due to the scheduler on these systems), so we have
 * to "busy wait" there to get an accurate timing.
 * All times are expressed as micro seconds, to avoid too much rounding error.
 */
void Main_WaitOnVbl(void)
{
	Sint64 CurrentTicks;
	static Sint64 DestTicks = 0;
	Sint64 FrameDuration_micro;
	Sint64 nDelay;

	nVBLCount++;
	if (nRunVBLs &&	nVBLCount >= nRunVBLs)
	{
		/* show VBLs/s */
		Main_PauseEmulation(true);
		exit(0);
	}

//	FrameDuration_micro = (Sint64) ( 1000000.0 / nScreenRefreshRate + 0.5 );	/* round to closest integer */
	FrameDuration_micro = ClocksTimings_GetVBLDuration_micro ( ConfigureParams.System.nMachineType , nScreenRefreshRate );
	FrameDuration_micro *= nVBLSlowdown;
	CurrentTicks = Time_GetTicks();

	if (DestTicks == 0)			/* on first call, init DestTicks */
	{
		DestTicks = CurrentTicks + FrameDuration_micro;
	}

	DestTicks += pulse_swallowing_count;	/* audio.c - Audio_CallBack() */

	nDelay = DestTicks - CurrentTicks;

	/* Do not wait if we are in fast forward mode or if we are totally out of sync */
	if (ConfigureParams.System.bFastForward == true
	    || nDelay < -4*FrameDuration_micro || nDelay > 50*FrameDuration_micro)
	{
		if (ConfigureParams.System.bFastForward == true)
		{
			if (!nFirstMilliTick)
				nFirstMilliTick = Main_GetTicks();
		}
		if (nFrameSkips < ConfigureParams.Screen.nFrameSkips)
		{
			nFrameSkips += 1;
			// Log_Printf(LOG_DEBUG, "Increased frameskip to %d\n", nFrameSkips);
		}
		/* Only update DestTicks for next VBL */
		DestTicks = CurrentTicks + FrameDuration_micro;
		return;
	}
	/* If automatic frameskip is enabled and delay's more than twice
	 * the effect of single frameskip, decrease frameskip
	 */
	if (nFrameSkips > 0
	    && ConfigureParams.Screen.nFrameSkips >= AUTO_FRAMESKIP_LIMIT
	    && 2*nDelay > FrameDuration_micro/nFrameSkips)
	{
		nFrameSkips -= 1;
		// Log_Printf(LOG_DEBUG, "Decreased frameskip to %d\n", nFrameSkips);
	}

	if (bAccurateDelays)
	{
		/* Accurate sleeping is possible -> use SDL_Delay to free the CPU */
		if (nDelay > 1000)
			Time_Delay(nDelay - 1000);
	}
	else
	{
		/* No accurate SDL_Delay -> only wait if more than 5ms to go... */
		if (nDelay > 5000)
			Time_Delay(nDelay<10000 ? nDelay-1000 : 9000);
	}

	/* Now busy-wait for the right tick: */
	while (nDelay > 0)
	{
		CurrentTicks = Time_GetTicks();
		nDelay = DestTicks - CurrentTicks;
		/* If the delay is still bigger than one frame, somebody
		 * played tricks with the system clock and we have to abort */
		if (nDelay > FrameDuration_micro)
			break;
	}

//printf ( "tick %lld\n" , CurrentTicks );
	/* Update DestTicks for next VBL */
	DestTicks += FrameDuration_micro;
}


/*-----------------------------------------------------------------------*/
/**
 * Since SDL_Delay and friends are very inaccurate on some systems, we have
 * to check if we can rely on this delay function.
 */
static void Main_CheckForAccurateDelays(void)
{
	int nStartTicks, nEndTicks;

	/* Force a task switch now, so we have a longer timeslice afterwards */
	SDL_Delay(10);

	nStartTicks = SDL_GetTicks();
	SDL_Delay(1);
	nEndTicks = SDL_GetTicks();

	/* If the delay took longer than 10ms, we are on an inaccurate system! */
	bAccurateDelays = ((nEndTicks - nStartTicks) < 9);

	if (bAccurateDelays)
		Log_Printf(LOG_DEBUG, "Host system has accurate delays. (%d)\n", nEndTicks - nStartTicks);
	else
		Log_Printf(LOG_WARN, "Host system does not have accurate delays. (%d)\n", nEndTicks - nStartTicks);
}


/* ----------------------------------------------------------------------- */
/**
 * Set mouse pointer to new x,y coordinates and set flag to ignore
 * the mouse event that is generated by SDL_WarpMouse().
 *
 * Skip the request is it's not position restore and mouse warping is disabled.
 */
void Main_WarpMouse(int x, int y, bool restore)
{
	if (!(restore || ConfigureParams.Screen.bMouseWarp))
		return;
#if WITH_SDL2
	SDL_WarpMouseInWindow(sdlWindow, x, y);
#else
	SDL_WarpMouse(x, y);
#endif
	bIgnoreNextMouseMotion = true;
}


/* ----------------------------------------------------------------------- */
/**
 * Handle mouse motion event.
 */
static void Main_HandleMouseMotion(SDL_Event *pEvent)
{
	int dx, dy;
	static int ax = 0, ay = 0;

	/* Ignore motion when position has changed right after a reset or TOS
	 * (especially version 4.04) might get confused and play key clicks */
	if (bIgnoreNextMouseMotion || nVBLs < 10)
	{
		bIgnoreNextMouseMotion = false;
		return;
	}

	dx = pEvent->motion.xrel;
	dy = pEvent->motion.yrel;

	/* In zoomed low res mode, we divide dx and dy by the zoom factor so that
	 * the ST mouse cursor stays in sync with the host mouse. However, we have
	 * to take care of lowest bit of dx and dy which will get lost when
	 * dividing. So we store these bits in ax and ay and add them to dx and dy
	 * the next time. */
	if (nScreenZoomX != 1)
	{
		dx += ax;
		ax = dx % nScreenZoomX;
		dx /= nScreenZoomX;
	}
	if (nScreenZoomY != 1)
	{
		dy += ay;
		ay = dy % nScreenZoomY;
		dy /= nScreenZoomY;
	}

	KeyboardProcessor.Mouse.dx += dx;
	KeyboardProcessor.Mouse.dy += dy;
}

#ifdef GEKKO
/* ----------------------------------------------------------------------- */
/**
 * Simulate ST mouse motion.
 *
 */
static void Main_MoveMouse(Sint16 *mousecoords)
{
	SDL_Event fake_key_event;

	fake_key_event.type = SDL_MOUSEMOTION;
	fake_key_event.motion.xrel = mousecoords[0] / 640;
	fake_key_event.motion.yrel = mousecoords[1] / 480;
	SDL_PushEvent(&fake_key_event);
}

/* ----------------------------------------------------------------------- */
/**
 * Simulate ST mouse left and right click.
 *
 * JoyType : 0 = Wiimote/Classic, 1 = GameCube controller
 */
static void Main_PressMouseButton(int sdlport, int sdlbutton, Uint8 pushed)
{
	int JoyType;
	int mousebutton;

	/* Wii buttons to ST mouse buttons. 0: left button, 1: right button */
	int StMouse[2][19] = {
	/*  { A,  B,  1,  2,  -,  +,  H, Z, C, A, B,  X,  Y, L, R, ZL, ZR,  -,  +} */
		//{-1, -1, -1, -1, -1, -1, -1, 0, 1, 0, 1, -1, -1, 0, 1, -1, -1, -1, -1},    // Wiimote / Classic controller 1 & 2
		{-1, -1, 1, 0, -1, -1, -1, 0, 1, 0, 1, -1, -1, 0, 1, -1, -1, -1, -1},    // Wiimote / Classic controller 1 & 2
	/*  {A, B,  X,  Y,  Z,  R,  L,START,...} */
		{0, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,-1,-1,-1,-1}     // Gamecube controller 1 & 2
	};	

	if (sdlport == 0 || sdlport == 1)
	{
		JoyType = 0;

		/* Don't use Wiimote button 1 & 2 for mouse button if an ST key is mapped on them */
		if (sdlbutton == 2 && ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii1Button[0] != 0)
			JoyType = 1;

		if (sdlbutton == 3 && ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii1Button[1] != 0)
			JoyType = 1;
	}
	else if (sdlport == 4 || sdlport == 5)
	{
		JoyType = 1;
	}

	mousebutton = StMouse[JoyType][sdlbutton];

	if (mousebutton == -1)
		return;

	if (!pushed)
	{
		if (mousebutton == 0) 
		{
			Keyboard.bLButtonDown &= ~BUTTON_MOUSE;
		}
		else if (mousebutton == 1)
		{
			Keyboard.bRButtonDown &= ~BUTTON_MOUSE;
		}
	}
	else
	{
		if (mousebutton == 0) 
		{
			if (Keyboard.LButtonDblClk == 0)
				Keyboard.bLButtonDown |= BUTTON_MOUSE;
		}
		else if (mousebutton == 1)
		{
			Keyboard.bRButtonDown |= BUTTON_MOUSE;
		}
	}
}
/* ----------------------------------------------------------------------- */
/**
 * Handle Wii buttons event.
 *
 * Press the corresponding ST key configured in the Wii mapper.
 * WiiButtonBinding contains the Wii buttons that can be configured.
 * StKey contains all the ST keys mapped to the Wii buttons.
 * JoyType : Wiimote and Classic controller(0) or GameCube Pad(1).
 * JoyIndex : Wii controller index.
 */
static void Main_HandleWiiButton(int sdlport, int sdlbutton, Uint8 pushed)
{
	int j;
	int JoyType;
	int KeyIndex;
	int JoyIndex;
	int StKey[4][9];
	/* Wii buttons configured in the mapper */
	int WiiButtonBinding[2][19] = {
	/*  {*,   *, 1, 2, -,  *,  *,  *, *,  *,  B, X, Y,  *,  *,  *,  *,  *,  *} */
		{-1, -1, 0, 1, 2, -1, -1, -1, -1, -1, 3, 4, 5, -1, -1, -1, -1, -1, -1},    // Wiimote / Classic controller 1 & 2
	/*  {*,   *, X, Y,  *, R,  *,  *, *,  *,  *, *, *,  *,  *,  *,  *,  *,  *} */
		{-1, -1, 6, 7, -1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}  // Gamecube controller 1 & 2
	};

	switch (sdlport)
	{
		/* Wiimote/Classic 1 */
		case 0:
			//if (MouseMode && sdlbutton == 0)
			//	return;
			JoyType = 0;
			JoyIndex = 0;
			KeyIndex = WiiButtonBinding[JoyType][sdlbutton];

			if (MouseMode) 
			{
				/* In mouse mode never send Wiimote A button. */
				if (sdlbutton == 0)
					return;	

				/* Button 1 & 2 are also used for mouse buttons, disable them here if there are no bindings */
				if (sdlbutton == 2 && ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii1Button[0] == 0)
					KeyIndex = -1;

				if (sdlbutton == 3 && ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii1Button[1] == 0)
					KeyIndex = -1;
			}
		break;
		/* Wiimote/Classic 2 */
		case 1:
			JoyType = 0;
			JoyIndex = 1;
			KeyIndex = WiiButtonBinding[JoyType][sdlbutton];
		break;
		/* GameCube Pad 1*/
		case 4:
			if (MouseMode && sdlbutton == 0)
				return;
			JoyType = 1;
			JoyIndex = 2;
			KeyIndex = WiiButtonBinding[JoyType][sdlbutton];
		break;
		/* GameCube Pad 2*/
		case 5:
			JoyType = 1;
			JoyIndex = 3;
			KeyIndex = WiiButtonBinding[JoyType][sdlbutton];
		break;
	}

	/* Retrieve Wiimotes/Classics bindings */
	for(j=0;j<6;j++)
	{
		StKey[0][j] = ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii1Button[j];
		StKey[1][j] = ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii2Button[j];
	}

	/* Retrieve GameCube Pads bindings */
	for(j=6;j<9;j++)
	{
		StKey[2][j] = ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii1Button[j];
		StKey[3][j] = ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii2Button[j];
	}

	if (KeyIndex == -1)
		return;

	if (StKey[JoyIndex][KeyIndex] == -1)
		return;

	/* Press configured ST key if any */
	if (!pushed)
	{
		IKBD_PressSTKey(StKey[JoyIndex][KeyIndex], false);
	}
	else
	{
		IKBD_PressSTKey(StKey[JoyIndex][KeyIndex], true);
	}
}

/* ----------------------------------------------------------------------- */
/**
 * Handle Wii hats event.
 *
 * Press the corresponding ST key configured in the Wii mapper.
 * JoyType : controllers grouped by player #. Each player has
 * wiimote/classic/gc pad. The hats bindings are the same on those 3
 * controllers.
 *
 */
static void Main_HandleWiiHat(int JoyType)
{
	int i;
	bool pushed = false;

	for (i=0; i < 4; i++) 
	{
		if (Dpad_Dir[JoyType][i])
		{
			pushed = true;

			/* Wiimotes/Classics */
			if (JoyType == 0)
			{
				/* FIXME : Last minute bug, ST arrows aren't sent correctly here 
				 * For now, it's handled in joy.c with Joy_WiiHatArrow*/
				if (Joy1HatMode == 1)		// ST key mode
					IKBD_PressSTKey(HatStKey[WIIPLAYER1][i], pushed);
				//else if (Joy1HatMode == 2)	// ST arrows mode
				//	IKBD_PressSTKey(HatStArrow[i], pushed);
			}
			/* GameCube Pads */
			if (JoyType == 1)
			{
				if (Joy2HatMode == 1)		// ST key mode
					IKBD_PressSTKey(HatStKey[WIIPLAYER2][i], pushed);
				//else if (Joy2HatMode == 2)	// ST arrows mode
				//	IKBD_PressSTKey(HatStArrow[i], pushed);
			}
		}
	}
}

/* ----------------------------------------------------------------------- */
/**
 * Release ST keys.
 *
 */
static void Clear_IKBD(int JoyType)
{
	int i;

	if(JoyType == WIIPLAYER1)
	{
		if (Joy1HatMode == 1)	// ST key mode
		{
			for (i=0;i<4;i++)
				IKBD_PressSTKey(HatStKey[WIIPLAYER1][i], false);
		}

		if (Joy1HatMode == 2)	// ST arrows mode
		{
			for (i=0;i<4;i++)
				IKBD_PressSTKey(HatStArrow[i], false);
		}
	}
	else if(JoyType == WIIPLAYER2)
	{
		if (Joy2HatMode == 1)	// ST key mode
		{
			for (i=0;i<4;i++)
				IKBD_PressSTKey(HatStKey[WIIPLAYER2][i], false);
		}

		if (Joy2HatMode == 2)	// ST arrows mode
		{
			for (i=0;i<4;i++)
				IKBD_PressSTKey(HatStArrow[i], false);
		}
	}
}
#endif

/* ----------------------------------------------------------------------- */
/**
 * SDL message handler.
 * Here we process the SDL events (keyboard, mouse, ...) and map it to
 * Atari IKBD events.
 */
void Main_EventHandler(void)
{
	bool bContinueProcessing;
	SDL_Event event;
	int events;
	int remotepause;

#ifdef GEKKO
	int i, nPadsConnected;
	int port = -1;

	/* ST mouse emulation with stick/dpad */
	MouseMode = (ConfigureParams.Joysticks.Joy[1].nJoystickMode == 0) ? 1 : 0;
	Sint16 mousecoords[2] = {0, 0};                    

	static SDL_Joystick *sdlJoystick[5] =   /* SDL's joystick structures */
	{
		NULL, NULL, NULL, NULL, NULL
	};

	/* Reset Wii hats */
	for (i=0; i<4; i++)
	{
		Dpad_Dir[0][i] = false;
		Dpad_Dir[1][i] = false;
	}

	/* Get ST keys assigned to Wii hats */
	for(i=0; i<4; i++)
	{
		HatStKey[WIIPLAYER1][i] = ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii1Hat[i];
		HatStKey[WIIPLAYER2][i] = ConfigureParams.Joysticks.Joy[0].nKeytoJoyWii2Hat[i];
	}

	nPadsConnected = SDL_NumJoysticks();
	for (i = 0; i < nPadsConnected && i < 5; i++)
	{
		/* Open the joysticks for use */
		sdlJoystick[i] = SDL_JoystickOpen(i);
	}
#endif


	do
	{
		bContinueProcessing = false;

		/* check remote process control */
		remotepause = Control_CheckUpdates();

		if ( bEmulationActive || remotepause )
		{
			events = SDL_PollEvent(&event);

		}
		else
		{
			ShortCut_ActKey();
			/* last (shortcut) event activated emulation? */
			if ( bEmulationActive )
				break;
			events = SDL_WaitEvent(&event);
		}
		if (!events)
		{
			/* no events -> if emulation is active or
			 * user is quitting -> return from function.
			 */
			continue;
		}


		switch (event.type)
		{
		 case SDL_QUIT:
			Main_RequestQuit(0);
			break;
			
		 case SDL_MOUSEMOTION:               /* Read/Update internal mouse position */
			Main_HandleMouseMotion(&event);
			bContinueProcessing = true;
			break;

		 case SDL_MOUSEBUTTONDOWN:
			if (event.button.button == SDL_BUTTON_LEFT)
			{
				if (Keyboard.LButtonDblClk == 0)
					Keyboard.bLButtonDown |= BUTTON_MOUSE;  /* Set button down flag */
			}
			else if (event.button.button == SDL_BUTTON_RIGHT)
			{
				Keyboard.bRButtonDown |= BUTTON_MOUSE;
			}
			else if (event.button.button == SDL_BUTTON_MIDDLE)
			{
				/* Start double-click sequence in emulation time */
				Keyboard.LButtonDblClk = 1;
			}
#if !WITH_SDL2
			else if (event.button.button == SDL_BUTTON_WHEELDOWN)
			{
				/* Simulate pressing the "cursor down" key */
				IKBD_PressSTKey(0x50, true);
			}
			else if (event.button.button == SDL_BUTTON_WHEELUP)
			{
				/* Simulate pressing the "cursor up" key */
				IKBD_PressSTKey(0x48, true);
			}
#endif
			break;

		 case SDL_MOUSEBUTTONUP:
			if (event.button.button == SDL_BUTTON_LEFT)
			{
				Keyboard.bLButtonDown &= ~BUTTON_MOUSE;
			}
			else if (event.button.button == SDL_BUTTON_RIGHT)
			{
				Keyboard.bRButtonDown &= ~BUTTON_MOUSE;
			}
#if !WITH_SDL2
			else if (event.button.button == SDL_BUTTON_WHEELDOWN)
			{
				/* Simulate releasing the "cursor down" key */
				IKBD_PressSTKey(0x50, false);
			}
			else if (event.button.button == SDL_BUTTON_WHEELUP)
			{
				/* Simulate releasing the "cursor up" key */
				IKBD_PressSTKey(0x48, false);
			}
#endif
			break;

		 case SDL_KEYDOWN:
			Keymap_KeyDown(&event.key.keysym);
			break;

		 case SDL_KEYUP:
			Keymap_KeyUp(&event.key.keysym);
			break;

		 case SDL_JOYBUTTONDOWN:
		 case SDL_JOYBUTTONUP:
			port = event.jhat.which;

			/* Send ST mouse clicks */
			if (MouseMode)
				Main_PressMouseButton(port, event.jbutton.button, event.type==SDL_JOYBUTTONDOWN);
			//else
			//	Main_PressMouseButton(port, event.jbutton.button, 0);

			/* Send ST keys configured in the Wii mapper */
			Main_HandleWiiButton(port, event.jbutton.button, event.type==SDL_JOYBUTTONDOWN);
			break;

		 case SDL_JOYHATMOTION:
			if (!MouseMode)
			{
				/* port is player id here 0 or 1. SDL reports 1st wiimote/gc pad as 0/4 */
				/* and 2d wiimote/gc pad as 1/5. */
				port = event.jhat.which & 1;

				Clear_IKBD(port);

				if (event.jhat.value & SDL_HAT_LEFT)  Dpad_Dir[port][0] = true;
				if (event.jhat.value & SDL_HAT_UP)    Dpad_Dir[port][1] = true;
				if (event.jhat.value & SDL_HAT_RIGHT) Dpad_Dir[port][2] = true;
				if (event.jhat.value & SDL_HAT_DOWN)  Dpad_Dir[port][3] = true;
			}
			break;

		 case SDL_JOYAXISMOTION:
			if (MouseMode)
			{
				if (event.jaxis.axis==0 || event.jaxis.axis==1)
		                mousecoords[event.jaxis.axis] = event.jaxis.value;

				bContinueProcessing = true;
			}
		break;

		 default:
			/* don't let unknown events delay event processing */
			bContinueProcessing = true;
			break;
		}

		if (MouseMode)
		{
			/* Move ST mouse with hats on Wiimote/GC pad 1 */
			if ((SDL_JoystickGetHat(sdlJoystick[0], 0)  & SDL_HAT_LEFT) 
				|| (SDL_JoystickGetHat(sdlJoystick[4], 0) & SDL_HAT_LEFT))  mousecoords[0] = -3000;
			if ((SDL_JoystickGetHat(sdlJoystick[0], 0)  & SDL_HAT_RIGHT)  
				|| (SDL_JoystickGetHat(sdlJoystick[4], 0) & SDL_HAT_RIGHT)) mousecoords[0] = 3000;
			if ((SDL_JoystickGetHat(sdlJoystick[0], 0)  & SDL_HAT_UP ) 
				|| (SDL_JoystickGetHat(sdlJoystick[4], 0) & SDL_HAT_UP))    mousecoords[1] = -2000;
			if ((SDL_JoystickGetHat(sdlJoystick[0], 0) & SDL_HAT_DOWN) 
				|| (SDL_JoystickGetHat(sdlJoystick[4], 0) & SDL_HAT_DOWN))  mousecoords[1] = 2000;
		}
		else
		{
			port = event.jhat.which & 1;
			/* 2 modes handled : ST arrows(Not anymore!) or ST keys configured in the mapper. */
			/* ST joystick is handled in joy.c */
			Main_HandleWiiHat(port);
		}

	} while (bContinueProcessing || !(bEmulationActive || bQuitProgram));

	if (mousecoords[0] || mousecoords[1])
		Main_MoveMouse(mousecoords);
}


/*-----------------------------------------------------------------------*/
/**
 * Set Hatari window title. Use NULL for default
 */
void Main_SetTitle(const char *title)
{
#if WITH_SDL2
	if (title)
		SDL_SetWindowTitle(sdlWindow, title);
	else
		SDL_SetWindowTitle(sdlWindow, PROG_NAME);
#else
	if (title)
		SDL_WM_SetCaption(title, "Hatari");
	else
		SDL_WM_SetCaption(PROG_NAME, "Hatari");
#endif
}

/*-----------------------------------------------------------------------*/
/**
 * Initialise emulation for some hardware components
 * It is required to init those parts before parsing the parameters
 * (for example, we should init FDC before inserting a disk and we
 * need to know valid joysticks before selecting default joystick IDs)
 */
static void Main_Init_HW(void)
{
	Joy_Init();
	FDC_Init();
	STX_Init();
}

/*-----------------------------------------------------------------------*/
/**
 * Initialise emulation
 */
static void Main_Init(void)
{
	/* Open debug log file */
	if (!Log_Init())
	{
		fprintf(stderr, "Logging/tracing initialization failed\n");
		exit(-1);
	}
	Log_Printf(LOG_INFO, PROG_NAME ", compiled on:  " __DATE__ ", " __TIME__ "\n");

	/* Init SDL's video subsystem. Note: Audio subsystem
	   will be initialized later (failure not fatal). */
	if (SDL_Init(SDL_INIT_VIDEO | Opt_GetNoParachuteFlag()) < 0)
	{
		fprintf(stderr, "Could not initialize the SDL library:\n %s\n", SDL_GetError() );
		exit(-1);
	}

	if ( IPF_Init() != true )
	{
		fprintf(stderr, "Could not initialize the IPF support\n" );
		exit(-1);
	}

	ClocksTimings_InitMachine ( ConfigureParams.System.nMachineType );
	Resolution_Init();
	SDLGui_Init();
	Printer_Init();
	RS232_Init();
	Midi_Init();
	Control_CheckUpdates();       /* enable window embedding? */
	Screen_Init();
	Main_SetTitle(NULL);
	HostScreen_Init();

	ACIA_Init( ACIA_Array , MachineClocks.ACIA_Freq , MachineClocks.ACIA_Freq );
	IKBD_Init();			/* After ACIA_Init */

	DSP_Init();
	Floppy_Init();
	M68000_Init();                /* Init CPU emulation */
	Audio_Init();
	Keymap_Init();

	/* Init HD emulation */
	HDC_Init();
	Ide_Init();
	GemDOS_Init();
	if (ConfigureParams.HardDisk.bUseHardDiskDirectories)
	{
		/* uses variables set by HDC_Init()! */
		GemDOS_InitDrives();
	}

	if (Reset_Cold())             /* Reset all systems, load TOS image */
	{
		/* If loading of the TOS failed, we bring up the GUI to let the
		 * user choose another TOS ROM file. */
		Dialog_DoProperty();
	}
	if (!bTosImageLoaded || bQuitProgram)
	{
		fprintf(stderr, "Failed to load TOS image!\n");
		SDL_Quit();
		exit(-2);
	}

	IoMem_Init();
	NvRam_Init();
	Sound_Init();
	
	/* done as last, needs CPU & DSP running... */
	DebugUI_Init();
}


/*-----------------------------------------------------------------------*/
/**
 * Un-Initialise emulation
 */
static void Main_UnInit(void)
{
	Screen_ReturnFromFullScreen();
	Floppy_UnInit();
	HDC_UnInit();
	Midi_UnInit();
	RS232_UnInit();
	Printer_UnInit();
	IoMem_UnInit();
	NvRam_UnInit();
	GemDOS_UnInitDrives();
	Ide_UnInit();
	Joy_UnInit();
	if (Sound_AreWeRecording())
		Sound_EndRecording();
	Audio_UnInit();
	SDLGui_UnInit();
	DSP_UnInit();
	HostScreen_UnInit();
	Screen_UnInit();
	Exit680x0();

	IPF_Exit();

	/* SDL uninit: */
	SDL_Quit();

	/* Close debug log file */
	Log_UnInit();
}


/*-----------------------------------------------------------------------*/
/**
 * Load initial configuration file(s)
 */
static void Main_LoadInitialConfig(void)
{
	char *psGlobalConfig;

	psGlobalConfig = malloc(FILENAME_MAX);
	if (psGlobalConfig)
	{
#if defined(__AMIGAOS4__)
		strncpy(psGlobalConfig, CONFDIR"hatari.cfg", FILENAME_MAX);
#else
		snprintf(psGlobalConfig, FILENAME_MAX, CONFDIR"%chatari.cfg", PATHSEP);
#endif
		/* Try to load the global configuration file */
		Configuration_Load(psGlobalConfig);

		free(psGlobalConfig);
	}

	/* Now try the users configuration file */
	Configuration_Load(NULL);
}

/*-----------------------------------------------------------------------*/
/**
 * Set TOS etc information and initial help message
 */
static void Main_StatusbarSetup(void)
{
	const char *name = NULL;
	SDLKey key;

	key = ConfigureParams.Shortcut.withoutModifier[SHORTCUT_OPTIONS];
	if (!key)
		key = ConfigureParams.Shortcut.withModifier[SHORTCUT_OPTIONS];
	if (key)
		name = SDL_GetKeyName(key);
	if (name)
	{
		char message[24], *keyname;
#ifdef _MUDFLAP
		__mf_register((void*)name, 32, __MF_TYPE_GUESS, "SDL keyname");
#endif
		keyname = Str_ToUpper(strdup(name));
		snprintf(message, sizeof(message), "Press %s for Options", keyname);
		free(keyname);

		Statusbar_AddMessage(message, 5000);
	}
	/* update information loaded by Main_Init() */
	Statusbar_UpdateInfo();
}

/*-----------------------------------------------------------------------*/
/**
 * Initialize Wii controllers hats mode
 * There are 3 modes available for Player 1 and 2(Joy1HatMode, Joy2HatMode)
 * 0: ST joystick, 1: ST key, 2: ST arrows
 */
static void Main_InitJoyHatMode(void)
{
	int i;
	JOYSTICKMODE state;

	state = ConfigureParams.Joysticks.Joy[1].nJoystickMode;

	/* Joystick in port 1 enabled by default */
	//if (state == JOYSTICK_KEYBOARD || state == JOYSTICK_DISABLED)
	//	ConfigureParams.Joysticks.Joy[1].nJoystickMode = JOYSTICK_REALSTICK;

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

/**
 * Main
 * 
 * Note: 'argv' cannot be declared const, MinGW would then fail to link.
 */
int main(int argc, char *argv[])
{
#ifdef GEKKO
	bool WiimoteConnected = true;

	__exception_setreload(3);

	fatInitDefault();
#endif

	/* Generate random seed */
	srand(time(NULL));

	/* Logs default to stderr at start */
	Log_Default();

	/* Initialize directory strings */
	Paths_Init(argv[0]);

	/* Init some HW components before parsing the configuration / parameters */
	Main_Init_HW();

	/* Set default configuration values */
	Configuration_SetDefault();

	/* Now load the values from the configuration file */
	Main_LoadInitialConfig();

	/* Check for any passed parameters */
	if (!Opt_ParseParameters(argc, (const char * const *)argv))
	{
		return 1;
	}
	/* monitor type option might require "reset" -> true */
	Configuration_Apply(true);

#ifdef WIN32
	Win_OpenCon();
#endif

#if HAVE_SETENV
	/* Needed on maemo but useful also with normal X11 window managers for
	 * window grouping when you have multiple Hatari SDL windows open */
	setenv("SDL_VIDEO_X11_WMCLASS", "hatari", 1);

	/* Needed for proper behavior of Caps Lock on some systems */
	setenv("SDL_DISABLE_LOCK_KEYS", "1", 1);
#endif

	/* Init emulator system */
	Main_Init();

#ifdef GEKKO
	SDL_Delay(600);
	/* Check if wiimote 1 is available, otherwise try GameCube pad 1 */
	WiimoteConnected = Joy_CheckWiimote();

	if (WiimoteConnected)
	{
		ConfigureParams.Joysticks.Joy[1].nJoyId = 0;
	}
	else
	{
		ConfigureParams.Joysticks.Joy[1].nJoyId = 4;
	}

	/* Init Joysticks hats mode */
	Main_InitJoyHatMode();
#endif

	/* Set initial Statusbar information */
	Main_StatusbarSetup();
	
	/* Check if SDL_Delay is accurate */
	Main_CheckForAccurateDelays();

	if ( AviRecordOnStartup )	/* Immediately starts avi recording ? */
		Avi_StartRecording ( ConfigureParams.Video.AviRecordFile , ConfigureParams.Screen.bCrop ,
			ConfigureParams.Video.AviRecordFps == 0 ?
				ClocksTimings_GetVBLPerSec ( ConfigureParams.System.nMachineType , nScreenRefreshRate ) :
				(Uint32)ConfigureParams.Video.AviRecordFps << CLOCKS_TIMINGS_SHIFT_VBL ,
			1 << CLOCKS_TIMINGS_SHIFT_VBL ,
			ConfigureParams.Video.AviRecordVcodec );

	/* Run emulation */
	Main_UnPauseEmulation();
	M68000_Start();                 /* Start emulation */

	if (bRecordingAvi)
	{
		/* cleanly close the avi file */
		Statusbar_AddMessage("Finishing AVI file...", 100);
		Statusbar_Update(sdlscrn, true);
		Avi_StopRecording();
	}
	/* Un-init emulation system */
	Main_UnInit();

	return nQuitValue;
}
