/*
  Hatari - sdlgui.c

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  A tiny graphical user interface for Hatari.
*/
const char SDLGui_fileid[] = "Hatari sdlgui.c : " __DATE__ " " __TIME__;

#include <SDL.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "screen.h"
#include "sdlgui.h"
#include "str.h"

#include "font5x8.h"
#include "font10x16.h"

#if WITH_SDL2
#define SDL_SRCCOLORKEY SDL_TRUE
#define SDLKey SDL_Keycode

#endif

#ifdef GEKKO
#include <wiiuse/wpad.h>
#include "configuration.h"
#include "classic1.xpm"
#include "classic2.xpm"
#include "wiimote1.xpm"
#include "wiimote2.xpm"
#include "gcpad1.xpm"
#include "gcpad2.xpm"
#include "floppyA.xpm"
#include "floppyB.xpm"
#include "quit.xpm"
#include "reset.xpm"
#include "save.xpm"
#include "settings.xpm"
#include "snap_empty.h"

/* The controller to draw in the Mapper dialog */
#define WIIMOTE1 0
#define WIIMOTE2 1
#define CLASSIC1 2
#define CLASSIC2 3
#define GC_PAD1  4
#define GC_PAD2  5

/* Analog sticks sensitivity */
#define ANALOG_SENSITIVITY 30

/* Delay before held keys triggering */
/* higher is the value, less responsive is the key update */
#define HELD_DELAY 30

/* Direction & selection update speed when a key is being held */
/* lower is the value, faster is the key update */
#define HELD_SPEED 3

#define WPAD_BUTTONS_HELD (WPAD_BUTTON_UP | WPAD_BUTTON_DOWN | WPAD_BUTTON_LEFT | WPAD_BUTTON_RIGHT | \
		WPAD_CLASSIC_BUTTON_UP | WPAD_CLASSIC_BUTTON_DOWN | WPAD_CLASSIC_BUTTON_LEFT | WPAD_CLASSIC_BUTTON_RIGHT)

#define PAD_BUTTONS_HELD  (PAD_BUTTON_UP | PAD_BUTTON_DOWN | PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT)

/* Image viewer dialog type */
#define SGIMGDIAL 22

/* Image viewer dialog shortcuts */
#define IMG_UPSHORTCUT   1
#define IMG_DOWNSHORTCUT 2
#define IMG_LEFTSHORTCUT   3
#define IMG_RIGHTSHORTCUT 4
#define IMG_PAGEUPSHORTCUT   5
#define IMG_PAGEDOWNSHORTCUT 6

bool vkeyboard = false;
u16 MenuInput;

/* Copy screen for snapshots */
SDL_Surface *lastscreen;

/* Create a Wii specific cursor */
static SDL_Cursor* mycursor;

static const char *arrow[] = {
  /* width height num_colors chars_per_pixel */
  "    12    17        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
"                                 ",
"                                 ",
"                                 ",
"                                 ",
"      XXXXX                      ",
"     X....X                      ",
"     X.....X                     ",
"     X.....X                     ",
"     X.....X                     ",
"     X.....X                     ",
"     X.....X                     ",
"     X.....XXXXX                 ",
"     X.....X...X XXXX            ",
"     X.....X...XX....XXXXXX      ",
" XXXXX.....X....X....XX...X      ",  
" X..XX....................X      ",
" X..XX....................X      ",
" X..XX....................X      ",
" X..XX....................X      ",
" X..XX....................X      ",
" X........................X      ",
" X........................X      ",
" XX......................XX      ",
"   X....................X        ",
"    X..................X         ",
"     X................X          ",
"     X................X          ",
"      XXXXXXXXXXXXXXXX           ",
"                                 ",
"                                 ",
"                                 ",
"                                 ",
  "0,0"
};

static SDL_Cursor *init_system_cursor()  
{  
	int i, row, col;  
	Uint8 data[4*32];  
	Uint8 mask[4*32];  
	int hot_x, hot_y;  
   
	i = -1;  
	for ( row=0; row<32; ++row ) {  
		for ( col=0; col<32; ++col ) {  
			if ( col % 8 ) {  
				data[i] <<= 1;  
				mask[i] <<= 1;  
			} else {  
				++i;  
				data[i] = mask[i] = 0;  
			}  
			switch (arrow[4+row][col]) {  
			case 'X':  
				data[i] |= 0x01;  
				mask[i] |= 0x01;  
				break;  
			case '.':  
				mask[i] |= 0x01;  
				break;  
			case ' ':  
				break;  
			}  
		}  
	}  
   sscanf(arrow[4+row], "%d,%d", &hot_x, &hot_y);  
   return SDL_CreateCursor(data, mask, 32, 32, hot_x, hot_y);  
}  
#endif

static SDL_Surface *pSdlGuiScrn;            /* Pointer to the actual main SDL screen surface */
static SDL_Surface *pSmallFontGfx = NULL;   /* The small font graphics */
static SDL_Surface *pBigFontGfx = NULL;     /* The big font graphics */
static SDL_Surface *pFontGfx = NULL;        /* The actual font graphics */
static int current_object = 0;				/* Current selected object */

static struct {
	Uint32 darkbar, midbar, lightbar;
	Uint32 darkgrey, midgrey, lightgrey;
	Uint32 focus, cursor, underline, editfield;
} colors;

int sdlgui_fontwidth;			/* Width of the actual font */
int sdlgui_fontheight;			/* Height of the actual font */

#define UNDERLINE_INDICATOR '_'


/*-----------------------------------------------------------------------*/
/**
 * Load an 1 plane XBM into a 8 planes SDL_Surface.
 */
static SDL_Surface *SDLGui_LoadXBM(int w, int h, const Uint8 *pXbmBits)
{
	SDL_Surface *bitmap;
	Uint8 *dstbits;
	const Uint8 *srcbits;
	int x, y, srcpitch;
	int mask;

	srcbits = pXbmBits;

	/* Allocate the bitmap */
	bitmap = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 8, 0, 0, 0, 0);
	if (bitmap == NULL)
	{
		fprintf(stderr, "Failed to allocate bitmap: %s", SDL_GetError());
		return NULL;
	}

	srcpitch = ((w + 7) / 8);
	dstbits = (Uint8 *)bitmap->pixels;
	mask = 1;

	/* Copy the pixels */
	for (y = 0 ; y < h ; y++)
	{
		for (x = 0 ; x < w ; x++)
		{
			dstbits[x] = (srcbits[x / 8] & mask) ? 1 : 0;
			mask <<= 1;
			mask |= (mask >> 8);
			mask &= 0xFF;
		}
		dstbits += bitmap->pitch;
		srcbits += srcpitch;
	}

	return bitmap;
}


/*-----------------------------------------------------------------------*/
/**
 * Initialize the GUI.
 */
int SDLGui_Init(void)
{
	SDL_Color blackWhiteColors[2] = {{255, 255, 255, 255}, {0, 0, 0, 255}};

	if (pSmallFontGfx && pBigFontGfx)
	{
		/* already initialized */
		return 0;
	}

	/* Initialize the font graphics: */
	pSmallFontGfx = SDLGui_LoadXBM(font5x8_width, font5x8_height, font5x8_bits);
	pBigFontGfx = SDLGui_LoadXBM(font10x16_width, font10x16_height, font10x16_bits);
	if (pSmallFontGfx == NULL || pBigFontGfx == NULL)
	{
		fprintf(stderr, "Error: Can not init font graphics!\n");
		return -1;
	}

	/* Set color palette of the font graphics: */
	SDL_SetColors(pSmallFontGfx, blackWhiteColors, 0, 2);
	SDL_SetColors(pBigFontGfx, blackWhiteColors, 0, 2);

	/* Set font color 0 as transparent: */
	SDL_SetColorKey(pSmallFontGfx, (SDL_SRCCOLORKEY|SDL_RLEACCEL), 0);
	SDL_SetColorKey(pBigFontGfx, (SDL_SRCCOLORKEY|SDL_RLEACCEL), 0);
#ifdef GEKKO
	mycursor = init_system_cursor();
#endif
	return 0;
}


/*-----------------------------------------------------------------------*/
/**
 * Uninitialize the GUI.
 */
int SDLGui_UnInit(void)
{
	if (pSmallFontGfx)
	{
		SDL_FreeSurface(pSmallFontGfx);
		pSmallFontGfx = NULL;
	}

	if (pBigFontGfx)
	{
		SDL_FreeSurface(pBigFontGfx);
		pBigFontGfx = NULL;
	}

	return 0;
}


/*-----------------------------------------------------------------------*/
/**
 * Inform the SDL-GUI about the actual SDL_Surface screen pointer and
 * prepare the font to suit the actual resolution.
 */
int SDLGui_SetScreen(SDL_Surface *pScrn)
{
	pSdlGuiScrn = pScrn;

	/* Decide which font to use - small or big one: */
	if (pSdlGuiScrn->w >= 640 && pSdlGuiScrn->h >= 400 && pBigFontGfx != NULL)
	{
		pFontGfx = pBigFontGfx;
	}
	else
	{
		pFontGfx = pSmallFontGfx;
	}

	if (pFontGfx == NULL)
	{
		fprintf(stderr, "Error: A problem with the font occurred!\n");
		return -1;
	}

	/* Get the font width and height: */
	sdlgui_fontwidth = pFontGfx->w/16;
	sdlgui_fontheight = pFontGfx->h/16;

	/* scrollbar */
	colors.darkbar   = SDL_MapRGB(pSdlGuiScrn->format, 64, 64, 64);
	colors.midbar    = SDL_MapRGB(pSdlGuiScrn->format,128,128,128);
	colors.lightbar  = SDL_MapRGB(pSdlGuiScrn->format,196,196,196);
	/* buttons, midgray is also normal bg color */
	colors.darkgrey  = SDL_MapRGB(pSdlGuiScrn->format,128,128,128);
	colors.midgrey   = SDL_MapRGB(pSdlGuiScrn->format,192,192,192);
	colors.lightgrey = SDL_MapRGB(pSdlGuiScrn->format,255,255,255);
	/* others */
#ifdef GEKKO
	colors.focus     = SDL_MapRGB(pSdlGuiScrn->format,222,222,175);
#else
	colors.focus     = SDL_MapRGB(pSdlGuiScrn->format,212,212,212);
#endif
	colors.cursor    = SDL_MapRGB(pSdlGuiScrn->format,128,128,128);
	if (sdlgui_fontheight < 16)
		colors.underline = SDL_MapRGB(pSdlGuiScrn->format,255,0,255);
	else
		colors.underline = SDL_MapRGB(pSdlGuiScrn->format,0,0,0);
	
	colors.editfield = SDL_MapRGB(pSdlGuiScrn->format,160,160,160);

	return 0;
}

/*-----------------------------------------------------------------------*/
/**
 * Return character size for current font in given arguments.
 */
void SDLGui_GetFontSize(int *width, int *height)
{
	*width = sdlgui_fontwidth;
	*height = sdlgui_fontheight;
}

/*-----------------------------------------------------------------------*/
/**
 * Center a dialog so that it appears in the middle of the screen.
 * Note: We only store the coordinates in the root box of the dialog,
 * all other objects in the dialog are positioned relatively to this one.
 */
void SDLGui_CenterDlg(SGOBJ *dlg)
{
	dlg[0].x = (pSdlGuiScrn->w/sdlgui_fontwidth-dlg[0].w)/2;
	dlg[0].y = (pSdlGuiScrn->h/sdlgui_fontheight-dlg[0].h)/2;
}

/*-----------------------------------------------------------------------*/
/**
 * Return text length which ignores underlining.
 */
static int SDLGui_TextLen(const char *str)
{
	int len;
	for (len = 0; *str; str++)
	{
		if (*str != UNDERLINE_INDICATOR)
			len++;
	}
	return len;
}

/*-----------------------------------------------------------------------*/
/**
 * Draw a text string (internal version).
 */
static void SDLGui_TextInt(int x, int y, const char *txt, bool underline)
{
	int i, offset;
	unsigned char c;
	SDL_Rect sr, dr;

	/* underline offset needs to go outside the box for smaller font */
	if (sdlgui_fontheight < 16)
		offset = sdlgui_fontheight - 1;
	else
		offset = sdlgui_fontheight - 2;

	i = 0;
	while (txt[i])
	{
		dr.x=x;
		dr.y=y;
		dr.w=sdlgui_fontwidth;
		dr.h=sdlgui_fontheight;

		c = txt[i++];
		if (c == UNDERLINE_INDICATOR && underline)
		{
			dr.h = 1;
			dr.y += offset;
			SDL_FillRect(pSdlGuiScrn, &dr, colors.underline);
			continue;
		}
		/* for now, assume (only) Linux file paths are UTF-8 */
#if !(defined(WIN32) || defined(USE_LOCALE_CHARSET))
		/* Quick and dirty convertion for latin1 characters only... */
		if ((c & 0xc0) == 0xc0)
		{
			c = c << 6;
			c |= (txt[i++]) & 0x7f;
		}
		else if (c >= 0x80)
		{
			printf("Unsupported character '%c' (0x%x)\n", c, c);
		}
#endif
		x += sdlgui_fontwidth;

		sr.x=sdlgui_fontwidth*(c%16);
		sr.y=sdlgui_fontheight*(c/16);
		sr.w=sdlgui_fontwidth;
		sr.h=sdlgui_fontheight;
		SDL_BlitSurface(pFontGfx, &sr, pSdlGuiScrn, &dr);
	}
}

/*-----------------------------------------------------------------------*/
/**
 * Draw a text string (generic).
 */
void SDLGui_Text(int x, int y, const char *txt)
{
	SDLGui_TextInt(x, y, txt, false);
}

/*-----------------------------------------------------------------------*/
/**
 * Draw a dialog text object.
 */
static void SDLGui_DrawText(const SGOBJ *tdlg, int objnum)
{
	int x, y;
	x = (tdlg[0].x+tdlg[objnum].x)*sdlgui_fontwidth;
	y = (tdlg[0].y+tdlg[objnum].y)*sdlgui_fontheight;

	if (tdlg[objnum].flags & SG_EXIT)
	{
		SDL_Rect rect;
		/* Draw background: */
		rect.x = x;
		rect.y = y;
		rect.w = tdlg[objnum].w * sdlgui_fontwidth;
		rect.h = tdlg[objnum].h * sdlgui_fontheight;
		if (tdlg[objnum].state & SG_FOCUSED)
			SDL_FillRect(pSdlGuiScrn, &rect, colors.focus);
		else
			SDL_FillRect(pSdlGuiScrn, &rect, colors.midgrey);
	}

	SDLGui_Text(x, y, tdlg[objnum].txt);
}


/*-----------------------------------------------------------------------*/
/**
 * Draw a edit field object.
 */
static void SDLGui_DrawEditField(const SGOBJ *edlg, int objnum)
{
	int x, y;
	SDL_Rect rect;

	x = (edlg[0].x+edlg[objnum].x)*sdlgui_fontwidth;
	y = (edlg[0].y+edlg[objnum].y)*sdlgui_fontheight;
	SDLGui_Text(x, y, edlg[objnum].txt);

	rect.x = x;
	rect.y = y + edlg[objnum].h * sdlgui_fontheight;
	rect.w = edlg[objnum].w * sdlgui_fontwidth;
	rect.h = 1;
	SDL_FillRect(pSdlGuiScrn, &rect, colors.editfield);
}

#ifdef GEKKO
#define abs_max(x, y) ((abs(x)>abs(y))?abs(x):abs(y))

/* Draw a pixel */
static void dot (SDL_Surface * surf, int x, int y, Uint32 color)
{
  SDL_Rect rect;
  rect.x = x, rect.y = y;
  rect.w = rect.h = 1;
  SDL_FillRect (surf, &rect, color);
}

/* Draw a line */
void DrawLine (SDL_Surface * surf, int x1, int y1, int x2, int y2, Uint32 color)
{
  int div,i;
  typedef struct
  {
      float x, y;
  }
  f_vecteur2D;
  f_vecteur2D pos,move;
  // Initial position
  pos.x = x1;
  pos.y = y1;
  // Max move
  move.x = x2 - x1;
  move.y = y2 - y1;

  div = abs_max (x2 - x1, y2 - y1);
  move.x = move.x / div;
  move.y = move.y / div;

  for (i = 0 ; i < div ; i++)
    {
      // Dot
      dot(surf,(int)pos.x,(int)pos.y,color);
      // move forward
      pos.x += move.x;
      pos.y += move.y;
    }
}
/*-----------------------------------------------------------------------*/
/**
 * Draw the virtual keyboard keys
 */
static void SDLGui_DrawKey(const SGOBJ *bdlg, int objnum, int offsetw, int offseth)
{
	SDL_Rect rect;
	int x, y, x2, y2, w, h, offset;
	Uint32 upleftc, downrightc;

	/* Black underlined when focus, items are more visible in main menu */
	if (bdlg[objnum].state & SG_FOCUSED)
	{
		upleftc = colors.underline;
		downrightc = colors.underline;
	}
	else
	{
		if (bdlg[objnum].type == SGBUTTON)
		{
			upleftc = colors.lightgrey;
			downrightc = colors.darkgrey;
		}
		else
		{
			upleftc = colors.darkgrey;
			downrightc = colors.darkgrey;
		}
	}

	x = bdlg[objnum].x;
	y = bdlg[objnum].y;

	if (objnum > 0)                 /* Since the root object is a box, too, */
	{
		/* we have to look for it now here and only */

		x += bdlg[0].x;   /* add its absolute coordinates if we need to */
		y += bdlg[0].y;
	}

	w = bdlg[objnum].w;
	h = bdlg[objnum].h;

	/* The root box should be bigger than the screen, so we disable the offset there: */
	if (objnum != 0)
		offset = 1;
	else
		offset = 0;

	if (bdlg[objnum].txt[0] == 'F' && strcmp(bdlg[objnum].txt, "F") != 0)
	{
		offset = 26;
		x2 = x + bdlg[objnum].w;
		y2 = y;

		/* Bottom left to top left */
		DrawLine (pSdlGuiScrn, x, (y + h - 1), x + offset, y, downrightc);

		/* Bottom right to top right */
		DrawLine (pSdlGuiScrn, x+offset+w-1, y, x+w-1, (y + h - 1), downrightc);

		/* Draw upper border: */
		rect.x = x + offset;
		rect.y = y;
		rect.w = w;
		rect.h = 1;
		SDL_FillRect(pSdlGuiScrn, &rect, downrightc);

		/* Draw bottom border: */
		rect.x = x;
		rect.y = (y + h - 1) ;
		rect.w = w;
		rect.h = 1;
		SDL_FillRect(pSdlGuiScrn, &rect, downrightc);
	}
	else if  (strcmp(bdlg[objnum].txt, "Return") == 0)
	{
		x2 = x + bdlg[objnum].w;
		y2 = y+(bdlg[objnum].h)*2;

		/* Top left to down */
		DrawLine (pSdlGuiScrn, x, y, x, y+bdlg[objnum].h, downrightc);

		/* Middle to left */
		DrawLine (pSdlGuiScrn, x, y+bdlg[objnum].h, x-bdlg[objnum].w+8, y+bdlg[objnum].h, downrightc);

		/* Left middle to down */
		DrawLine (pSdlGuiScrn, x-bdlg[objnum].w+8, y+bdlg[objnum].h, x-bdlg[objnum].w+8, y2, downrightc);

		/* Bottom left to right */
		DrawLine (pSdlGuiScrn, x-bdlg[objnum].w+8, y2, x2, y2, downrightc);

		/* Bottom right to up */
		DrawLine (pSdlGuiScrn, x2, y2, x2, y, downrightc);

		/* Top right to left */
		DrawLine (pSdlGuiScrn, x2, y, x, y, downrightc);
	}
	else
	{
		/* Draw upper border: */
		rect.x = x;
		rect.y = y;
		rect.w = w;
		rect.h = 1;
		SDL_FillRect(pSdlGuiScrn, &rect, upleftc);

		/* Draw left border: */
		rect.x = x;
		rect.y = y;
		rect.w = 1;
		rect.h = h;
		SDL_FillRect(pSdlGuiScrn, &rect, upleftc);

		/* Draw bottom border: */
		rect.x = x;
		rect.y = (y + h - 1) ;
		rect.w = w;
		rect.h = 1;
		SDL_FillRect(pSdlGuiScrn, &rect, downrightc);

		/* Draw right border: */
		rect.x = x + w - 1;
		rect.y = y;
		rect.w = 1;
		rect.h = h;
		SDL_FillRect(pSdlGuiScrn, &rect, downrightc);
	}
}
#endif

/*-----------------------------------------------------------------------*/
/**
 * Draw a dialog box object.
 */
static void SDLGui_DrawBox(const SGOBJ *bdlg, int objnum)
{
	SDL_Rect rect;
	int x, y, w, h, offset;
	Uint32 color, upleftc, downrightc;

#ifndef GEKKO
	if (bdlg[objnum].state & SG_FOCUSED)
		color = colors.focus;
	else
		color = colors.midgrey;
#else
	/* Black underlined when focus, items are more visible in main menu */
	if (bdlg[objnum].state & SG_FOCUSED)
	{
		color = colors.focus;
		upleftc = colors.underline;
		downrightc = colors.underline;
	}
	else
	{
		color = colors.midgrey;
		upleftc = colors.lightgrey;
		downrightc = colors.darkgrey;
	}
#endif
	x = bdlg[objnum].x*sdlgui_fontwidth;
	y = bdlg[objnum].y*sdlgui_fontheight;
	if (objnum > 0)                 /* Since the root object is a box, too, */
	{
		/* we have to look for it now here and only */
		x += bdlg[0].x*sdlgui_fontwidth;   /* add its absolute coordinates if we need to */
		y += bdlg[0].y*sdlgui_fontheight;
	}
	w = bdlg[objnum].w*sdlgui_fontwidth;
	h = bdlg[objnum].h*sdlgui_fontheight;

#ifndef GEKKO
	if (bdlg[objnum].state & SG_SELECTED)
	{
		upleftc = colors.darkgrey;
		downrightc = colors.lightgrey;
	}
	else
	{
		upleftc = colors.lightgrey;
		downrightc = colors.darkgrey;
	}
#endif

	/* The root box should be bigger than the screen, so we disable the offset there: */
	if (objnum != 0)
		offset = 1;
	else
		offset = 0;

	/* Draw background: */
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
#ifdef GEKKO
	/* Only draw background for box & buttons */
	if (bdlg[objnum].type == SGBOX || bdlg[objnum].type == SGBUTTON)
#endif
	SDL_FillRect(pSdlGuiScrn, &rect, color);

	/* Draw upper border: */
	rect.x = x;
	rect.y = y - offset;
	rect.w = w;
	rect.h = 1;
	SDL_FillRect(pSdlGuiScrn, &rect, upleftc);

	/* Draw left border: */
	rect.x = x - offset;
	rect.y = y;
	rect.w = 1;
	rect.h = h;
	SDL_FillRect(pSdlGuiScrn, &rect, upleftc);

	/* Draw bottom border: */
	rect.x = x;
	rect.y = y + h - 1 + offset;
	rect.w = w;
	rect.h = 1;
	SDL_FillRect(pSdlGuiScrn, &rect, downrightc);

	/* Draw right border: */
	rect.x = x + w - 1 + offset;
	rect.y = y;
	rect.w = 1;
	rect.h = h;
	SDL_FillRect(pSdlGuiScrn, &rect, downrightc);
}


/*-----------------------------------------------------------------------*/
/**
 * Draw a normal button.
 */
static void SDLGui_DrawButton(const SGOBJ *bdlg, int objnum)
{
	int x,y;

#ifdef GEKKO
if (vkeyboard)
{
	SDLGui_DrawKey(bdlg, objnum,0,0);
	x = (bdlg[0].x + bdlg[objnum].x + 2);
	y = (bdlg[0].y + bdlg[objnum].y + 1);
}
else
{
	SDLGui_DrawBox(bdlg, objnum);
	x = (bdlg[0].x + bdlg[objnum].x + (bdlg[objnum].w-SDLGui_TextLen(bdlg[objnum].txt))/2) * sdlgui_fontwidth;
	y = (bdlg[0].y + bdlg[objnum].y + (bdlg[objnum].h-1)/2) * sdlgui_fontheight;
}
#else
	SDLGui_DrawBox(bdlg, objnum);

	x = (bdlg[0].x + bdlg[objnum].x + (bdlg[objnum].w-SDLGui_TextLen(bdlg[objnum].txt))/2) * sdlgui_fontwidth;
	y = (bdlg[0].y + bdlg[objnum].y + (bdlg[objnum].h-1)/2) * sdlgui_fontheight;
#endif
	if (bdlg[objnum].state & SG_SELECTED)
	{
		x+=1;
		y+=1;
	}
	SDLGui_TextInt(x, y, bdlg[objnum].txt, true);
}

/*-----------------------------------------------------------------------*/
/**
 * If object is focused, draw a focused background to it
 */
static void SDLGui_DrawFocusBg(const SGOBJ *obj, int x, int y)
{
	SDL_Rect rect;
	Uint32 color;

	if (obj->state & SG_WASFOCUSED)
		color = colors.midgrey;
	else if (obj->state & SG_FOCUSED)
		color = colors.focus;
	else
		return;

	rect.x = x;
	rect.y = y;
#ifdef GEKKO
	if (vkeyboard)
	{
		rect.w = obj->w;
		rect.h = obj->h;
	}
	else
	{
		rect.w = obj->w * sdlgui_fontwidth;
		rect.h = obj->h * sdlgui_fontheight;
	}
#else
	rect.w = obj->w * sdlgui_fontwidth;
	rect.h = obj->h * sdlgui_fontheight;
#endif

	SDL_FillRect(pSdlGuiScrn, &rect, color);
}

/*-----------------------------------------------------------------------*/
/**
 * Draw a dialog radio button object.
 */
static void SDLGui_DrawRadioButton(const SGOBJ *rdlg, int objnum)
{
	char str[80];
	int x, y;

	x = (rdlg[0].x + rdlg[objnum].x) * sdlgui_fontwidth;
	y = (rdlg[0].y + rdlg[objnum].y) * sdlgui_fontheight;
	SDLGui_DrawFocusBg(&(rdlg[objnum]), x, y);

	if (rdlg[objnum].state & SG_SELECTED)
		str[0]=SGRADIOBUTTON_SELECTED;
	else
		str[0]=SGRADIOBUTTON_NORMAL;
	str[1]=' ';
	strcpy(&str[2], rdlg[objnum].txt);

	SDLGui_TextInt(x, y, str, true);
}


/*-----------------------------------------------------------------------*/
/**
 * Draw a dialog check box object.
 */
static void SDLGui_DrawCheckBox(const SGOBJ *cdlg, int objnum)
{
	char str[80];
	int x, y;

#ifdef GEKKO
if (vkeyboard)
{
	x = (cdlg[0].x + cdlg[objnum].x);
	y = (cdlg[0].y + cdlg[objnum].y);
}
else
{
	x = (cdlg[0].x + cdlg[objnum].x) * sdlgui_fontwidth;
	y = (cdlg[0].y + cdlg[objnum].y) * sdlgui_fontheight;
}
#else
	x = (cdlg[0].x + cdlg[objnum].x) * sdlgui_fontwidth;
	y = (cdlg[0].y + cdlg[objnum].y) * sdlgui_fontheight;
#endif
	SDLGui_DrawFocusBg(&(cdlg[objnum]), x, y);

	if ( cdlg[objnum].state&SG_SELECTED )
		str[0]=SGCHECKBOX_SELECTED;
	else
		str[0]=SGCHECKBOX_NORMAL;
	str[1]=' ';
	strcpy(&str[2], cdlg[objnum].txt);

	SDLGui_TextInt(x, y, str, true);
}


/*-----------------------------------------------------------------------*/
/**
 * Draw a scrollbar button.
 */
static void SDLGui_DrawScrollbar(const SGOBJ *bdlg, int objnum)
{
	SDL_Rect rect;
	int x, y, w, h;

	x = bdlg[objnum].x * sdlgui_fontwidth;
	y = bdlg[objnum].y * sdlgui_fontheight + bdlg[objnum].h;

	x += bdlg[0].x*sdlgui_fontwidth;   /* add mainbox absolute coordinates */
	y += bdlg[0].y*sdlgui_fontheight;  /* add mainbox absolute coordinates */

#ifndef GEKKO
	w = 1 * sdlgui_fontwidth;
	h = bdlg[objnum].w;
#else
	w = 1 * sdlgui_fontwidth + 20;
	h = bdlg[objnum].w + 54;
#endif

	/* Draw background: */
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	SDL_FillRect(pSdlGuiScrn, &rect, colors.midbar);

	/* Draw upper border: */
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = 1;
	SDL_FillRect(pSdlGuiScrn, &rect, colors.lightbar);

	/* Draw bottom border: */
	rect.x = x;
	rect.y = y + h - 1;
	rect.w = w;
	rect.h = 1;
	SDL_FillRect(pSdlGuiScrn, &rect, colors.darkbar);
}

/*-----------------------------------------------------------------------*/
/**
 *  Draw a dialog popup button object.
 */
static void SDLGui_DrawPopupButton(const SGOBJ *pdlg, int objnum)
{
	int x, y, w;
	const char *downstr = "\x02";

	SDLGui_DrawBox(pdlg, objnum);

	x = (pdlg[0].x + pdlg[objnum].x) * sdlgui_fontwidth;
	y = (pdlg[0].y + pdlg[objnum].y) * sdlgui_fontheight;
	w = pdlg[objnum].w * sdlgui_fontwidth;

	SDLGui_TextInt(x, y, pdlg[objnum].txt, true);
	SDLGui_Text(x+w-sdlgui_fontwidth, y, downstr);
}


/*-----------------------------------------------------------------------*/
/**
 * Let the user insert text into an edit field object.
 * NOTE: The dlg[objnum].txt must point to an an array that is big enough
 * for dlg[objnum].w characters!
 */
static void SDLGui_EditField(SGOBJ *dlg, int objnum)
{
	size_t cursorPos;                   /* Position of the cursor in the edit field */
	int blinkState = 0;                 /* Used for cursor blinking */
	int bStopEditing = false;           /* true if user wants to exit the edit field */
	char *txt;                          /* Shortcut for dlg[objnum].txt */
	SDL_Rect rect;
	SDL_Event event;
#if !WITH_SDL2
	int nOldUnicodeMode;
#endif

	rect.x = (dlg[0].x + dlg[objnum].x) * sdlgui_fontwidth;
	rect.y = (dlg[0].y + dlg[objnum].y) * sdlgui_fontheight;
	rect.w = (dlg[objnum].w + 1) * sdlgui_fontwidth - 1;
	rect.h = dlg[objnum].h * sdlgui_fontheight;

#if WITH_SDL2
	SDL_SetTextInputRect(&rect);
	SDL_StartTextInput();
#else
	/* Enable unicode translation to get shifted etc chars with SDL_PollEvent */
	nOldUnicodeMode = SDL_EnableUNICODE(true);
#endif

	txt = dlg[objnum].txt;
	cursorPos = strlen(txt);

	do
	{
		/* Look for events */
		if (SDL_PollEvent(&event) == 0)
		{
			/* No event: Wait some time for cursor blinking */
			SDL_Delay(250);
			blinkState ^= 1;
		}
		else
		{
			/* Handle events */
			do
			{
				switch (event.type)
				{
				 case SDL_QUIT:                     /* User wants to quit */
					bQuitProgram = true;
					bStopEditing = true;
					break;
				 case SDL_MOUSEBUTTONDOWN:          /* Mouse pressed -> stop editing */
					bStopEditing = true;
					break;
#if WITH_SDL2
				 case SDL_TEXTINPUT:
					if (strlen(txt) < (size_t)dlg[objnum].w)
					{
						memmove(&txt[cursorPos+1], &txt[cursorPos],
						        strlen(&txt[cursorPos])+1);
 						txt[cursorPos] = event.text.text[0];
						cursorPos += 1;
					}
					break;
#endif
				 case SDL_KEYDOWN:                  /* Key pressed */
					switch (event.key.keysym.sym)
					{
					 case SDLK_RETURN:
					 case SDLK_KP_ENTER:
						bStopEditing = true;
						break;
					 case SDLK_LEFT:
						if (cursorPos > 0)
							cursorPos -= 1;
						break;
					 case SDLK_RIGHT:
						if (cursorPos < strlen(txt))
							cursorPos += 1;
						break;
					 case SDLK_BACKSPACE:
						if (cursorPos > 0)
						{
							memmove(&txt[cursorPos-1], &txt[cursorPos], strlen(&txt[cursorPos])+1);
							cursorPos -= 1;
						}
						break;
					 case SDLK_DELETE:
						if (cursorPos < strlen(txt))
							memmove(&txt[cursorPos], &txt[cursorPos+1], strlen(&txt[cursorPos+1])+1);
						break;
					 default:
#if !WITH_SDL2
						/* If it is a "good" key then insert it into the text field */
						if (event.key.keysym.unicode >= 32 && event.key.keysym.unicode < 128
						        && event.key.keysym.unicode != PATHSEP)
						{
							if (strlen(txt) < (size_t)dlg[objnum].w)
							{
								memmove(&txt[cursorPos+1], &txt[cursorPos], strlen(&txt[cursorPos])+1);
								txt[cursorPos] = event.key.keysym.unicode;
								cursorPos += 1;
							}
						}
#endif
						break;
					}
					break;
				}
			}
			while (SDL_PollEvent(&event));

			blinkState = 1;
		}

		/* Redraw the text field: */
		SDL_FillRect(pSdlGuiScrn, &rect, colors.midgrey);  /* Draw background */
		/* Draw the cursor: */
		if (blinkState && !bStopEditing)
		{
			SDL_Rect cursorrect;
			cursorrect.x = rect.x + cursorPos * sdlgui_fontwidth;
			cursorrect.y = rect.y;
			cursorrect.w = sdlgui_fontwidth;
			cursorrect.h = rect.h;
			SDL_FillRect(pSdlGuiScrn, &cursorrect, colors.cursor);
		}
		SDLGui_Text(rect.x, rect.y, dlg[objnum].txt);  /* Draw text */
		SDL_UpdateRects(pSdlGuiScrn, 1, &rect);
	}
	while (!bStopEditing);

#if WITH_SDL2
	SDL_StopTextInput();
#else
	SDL_EnableUNICODE(nOldUnicodeMode);
#endif
}

#ifdef GEKKO
/*-----------------------------------------------------------------------*/
/**
 * Draw Wii controller button object
 */
static void SDLGui_DrawWiiController(const SGOBJ *cdlg, int objnum, int wiictrl)
{
	int xoffset[6] = {40, 40, 10, 10, 15, 15}; 
	int yoffset[6] = {4, 4,30,30, 25, 25};
	char **menu[6] = {wiimote1_xpm, wiimote2_xpm, classic1_xpm, classic2_xpm, gcpad1_xpm, gcpad2_xpm}; 
	SDL_Surface* s_menu;
	SDL_Rect src, dest;
	 
	src.x = 0;
	src.y = 0;

	SDLGui_DrawBox(cdlg, objnum);

	s_menu = IMG_ReadXPMFromArray(menu[wiictrl]);

	src.w = cdlg[objnum].w * sdlgui_fontwidth;
	src.h = cdlg[objnum].h * sdlgui_fontheight;

	dest.x = ((cdlg[0].x + cdlg[objnum].x) * sdlgui_fontwidth) + xoffset[wiictrl];
	dest.y = ((cdlg[0].y + cdlg[objnum].y) * sdlgui_fontheight) + yoffset[wiictrl];

	SDL_BlitSurface(s_menu, &src, pSdlGuiScrn, &dest);
	SDL_FreeSurface(s_menu);
}

/*-----------------------------------------------------------------------*/
/**
 * Draw Wii main menu buttons
 */
static void SDLGui_DrawWiiMenu(const SGOBJ *cdlg, int objnum, int option)
{
	int xoffset = 9;
	int yoffset = 2;
	char **menu[6] = {floppyA_xpm, floppyB_xpm, save_xpm, settings_xpm, reset_xpm, quit_xpm}; 
	SDL_Surface *s_menu;
	SDL_Surface *screen;

	SDL_Rect src, dest;

	src.x = 0;
	src.y = 0;

	SDLGui_DrawBox(cdlg, objnum);

	s_menu = IMG_ReadXPMFromArray(menu[option]);

	src.w = cdlg[objnum].w * sdlgui_fontwidth;
	src.h = cdlg[objnum].h * sdlgui_fontheight;

	dest.x = ((cdlg[0].x + cdlg[objnum].x) * sdlgui_fontwidth) + xoffset;
	dest.y = ((cdlg[0].y + cdlg[objnum].y) * sdlgui_fontheight) + yoffset;

	SDL_BlitSurface(s_menu, &src, pSdlGuiScrn, &dest);
	SDL_FreeSurface(s_menu);
}

/*-----------------------------------------------------------------------*/
/**
 * Draw a Snapshot image
 */
static void SDLGui_DrawSnapshot(const SGOBJ *bdlg, int objnum)
{
	SDL_Surface*  temp;
	SDL_Rect src, dest;
	SDL_RWops* Empty_Snaphot = NULL;

	char thumbdir[256];

	temp = IMG_Load(snapfile);

	if (temp == NULL)
	{
		Empty_Snaphot = SDL_RWFromMem(snap_empty, snap_empty_size);
		temp = IMG_Load_RW(Empty_Snaphot, 1);
	}

	src.x = 0;
	src.y = 0;

	src.w = temp->w;
	src.h = temp->h;

	dest.x = (bdlg[0].w * sdlgui_fontwidth) - src.w;
	dest.y = ((bdlg[0].y * sdlgui_fontheight) + src.h) + bdlg[objnum].y;

	SDL_BlitSurface(temp, &src, pSdlGuiScrn, &dest);
	SDL_FreeSurface(temp);
}

/*-----------------------------------------------------------------------*/
/**
 * Get the value of a pixel
 * 
 * http://sdl.beuc.net/sdl.wiki/Pixel_Access
 */
Uint32 GetPixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        return *p;
        break;

    case 2:
        return *(Uint16 *)p;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
        break;

    case 4:
        return *(Uint32 *)p;
        break;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}

/*-----------------------------------------------------------------------*/
/**
 * Set the value of a pixel
 */
void SetPixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16 *)p = pixel;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        } else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(Uint32 *)p = pixel;
        break;
    }
}

/*-----------------------------------------------------------------------*/
/**
 * Scale a surface
 *
 * http://www.sdltutorials.com/sdl-scale-surface
 */
SDL_Surface *ScaleSurface(SDL_Surface *Surface, Uint16 Width, Uint16 Height)
{
	int x,y,o_x,o_y;

    if(!Surface || !Width || !Height)
        return 0;
     
    SDL_Surface *_ret = SDL_CreateRGBSurface(Surface->flags, Width, Height, Surface->format->BitsPerPixel,
        Surface->format->Rmask, Surface->format->Gmask, Surface->format->Bmask, Surface->format->Amask);
 
    double  _stretch_factor_x = (double)(Width)  / (double)(Surface->w),
        _stretch_factor_y = (double)(Height) / (double)(Surface->h);
 
    for( y = 0; y < Surface->h; y++)
        for( x = 0; x < Surface->w; x++)
            for( o_y = 0; o_y < _stretch_factor_y; ++o_y)
                for( o_x = 0; o_x < _stretch_factor_x; ++o_x)

				SetPixel(_ret, (Sint32)(_stretch_factor_x * x) + o_x, 
                        (Sint32)(_stretch_factor_y * y) + o_y, GetPixel(Surface, x, y));

    return _ret;
}
#endif

/*-----------------------------------------------------------------------*/
/**
 * Draw single object based on its type
 */
static void SDLGui_DrawObj(const SGOBJ *dlg, int i)
{
	switch (dlg[i].type)
	{
	case SGBOX:
		SDLGui_DrawBox(dlg, i);
		break;
	case SGTEXT:
		SDLGui_DrawText(dlg, i);
		break;
	case SGEDITFIELD:
		SDLGui_DrawEditField(dlg, i);
		break;
	case SGBUTTON:
		SDLGui_DrawButton(dlg, i);
		break;
	case SGRADIOBUT:
		SDLGui_DrawRadioButton(dlg, i);
		break;
	case SGCHECKBOX:
		SDLGui_DrawCheckBox(dlg, i);
		break;
	case SGPOPUP:
		SDLGui_DrawPopupButton(dlg, i);
		break;
	case SGSCROLLBAR:
		SDLGui_DrawScrollbar(dlg, i);
		break;
#ifdef GEKKO
	case SGWIIMOTE1:
		SDLGui_DrawWiiController(dlg, i, WIIMOTE1);
		break;
	case SGCLASSIC1:
		SDLGui_DrawWiiController(dlg, i, CLASSIC1);
		break;
	case SGGCPAD1:
		SDLGui_DrawWiiController(dlg, i, GC_PAD1);
		break;
	case SGWIIMOTE2:
		SDLGui_DrawWiiController(dlg, i, WIIMOTE2);
		break;
	case SGCLASSIC2:
		SDLGui_DrawWiiController(dlg, i, CLASSIC2);
		break;
	case SGGCPAD2:
		SDLGui_DrawWiiController(dlg, i, GC_PAD2);
		break;
	case SGFLOPPYA:
		SDLGui_DrawWiiMenu(dlg, i, 0);
		break;
	case SGFLOPPYB:
		SDLGui_DrawWiiMenu(dlg, i, 1);
		break;
	case SGLOAD_SAVE:
		SDLGui_DrawWiiMenu(dlg, i, 2);
		break;
	case SGSETTINGS:
		SDLGui_DrawWiiMenu(dlg, i, 3);
		break;
	case SGRESET:
		SDLGui_DrawWiiMenu(dlg, i, 4);
		break;
	case SGQUIT:
		SDLGui_DrawWiiMenu(dlg, i, 5);
		break;
	case SGSNAPSHOT:
		SDLGui_DrawSnapshot(dlg, i);
		break;
	case SGKEY:
		SDLGui_DrawKey(dlg, i, 10, 8);
		break;
#endif
	}
}

/*-----------------------------------------------------------------------*/
/**
 * Draw a whole dialog.
 */
void SDLGui_DrawDialog(const SGOBJ *dlg)
{
	int i;

	for (i = 0; dlg[i].type != -1; i++)
	{
		SDLGui_DrawObj(dlg, i);
	}
	SDL_UpdateRect(pSdlGuiScrn, 0,0,0,0);
}


/*-----------------------------------------------------------------------*/
/**
 * Search an object at a certain position.
 * Return object index or -1 if it wasn't found.
 */
static int SDLGui_FindObj(const SGOBJ *dlg, int fx, int fy)
{
	int len, i;
	int ob = -1;
	int xpos, ypos;

	len = 0;
	while (dlg[len].type != -1)   len++;
	xpos = fx / sdlgui_fontwidth;
	ypos = fy / sdlgui_fontheight;
	/* Now search for the object: */
	for (i = len; i >= 0; i--)
	{
		/* clicked on a scrollbar ? */
		if (dlg[i].type == SGSCROLLBAR) {
			if (xpos >= dlg[0].x+dlg[i].x && xpos < dlg[0].x+dlg[i].x+1) {
				ypos = dlg[i].y * sdlgui_fontheight + dlg[i].h + dlg[0].y * sdlgui_fontheight;
				if (fy >= ypos && fy < ypos + dlg[i].w) {
					ob = i;
					break;
				}
			}
		}
		/* clicked on another object ? */
		else if (xpos >= dlg[0].x+dlg[i].x && ypos >= dlg[0].y+dlg[i].y
		    && xpos < dlg[0].x+dlg[i].x+dlg[i].w && ypos < dlg[0].y+dlg[i].y+dlg[i].h)
		{
			ob = i;
			break;
		}
	}

	return ob;
}


/*-----------------------------------------------------------------------*/
/**
 * Search an object at a certain position.
 * Return object index or -1 if it wasn't found.
 */
static int SDLGui_FindObjKey(const SGOBJ *dlg, int fx, int fy)
{
	int len, i;
	int ob = -1;
	int xpos, ypos;

	len = 0;
	while (dlg[len].type != -1)   len++;
	xpos = fx;
	ypos = fy;
	/* Now search for the object: */
	for (i = len; i >= 0; i--)
	{	
		/* clicked on another object ? */
		if (xpos >= dlg[0].x+dlg[i].x && ypos >= dlg[0].y+dlg[i].y
		    && xpos < dlg[0].x+dlg[i].x+dlg[i].w && ypos < dlg[0].y+dlg[i].y+dlg[i].h)
		{
			ob = i;
			break;
		}
	}

	return ob;
}

/*-----------------------------------------------------------------------*/
/**
 * Search an object with a special flag (e.g. SG_DEFAULT or SG_CANCEL).
 */
static int SDLGui_SearchFlags(const SGOBJ *dlg, int flag)
{
	int i = 0;

	while (dlg[i].type != -1)
	{
		if (dlg[i].flags & flag)
			return i;
		i++;
	}
	return 0;
}

/*-----------------------------------------------------------------------*/
/**
 * Search an object with a special state (e.g. SG_FOCUSED).
 */
static int SDLGui_SearchState(const SGOBJ *dlg, int state)
{
	int i = 0;

	while (dlg[i].type != -1)
	{
		if (dlg[i].state & state)
			return i;
		i++;
	}
	return 0;
}

#ifdef GEKKO
/*-----------------------------------------------------------------------*/
/**
 * Search the dialog type (File selection etc.)
 */
static int SDLGui_SearchType(const SGOBJ *dlg)
{
	int i = 0;

	while (dlg[i].type != -1)
	{
		/* Wii mapper dialog */
		if (dlg[i].type == SGWIIMOTE1 || dlg[i].type == SGCLASSIC1 || dlg[i].type == SGGCPAD1)
			return SGWIIMOTE1;

		/* File selection dialog*/
		if (dlg[i].type == SGSCROLLBAR)
			return SGSCROLLBAR;

		/* Image viewer: SDLGui_FileImgSelect */
		if (dlg[i].flags == SG_PAGEUP)
			return SGIMGDIAL;
		i++;
	}
	return 0;
}
#endif

/*-----------------------------------------------------------------------*/
/**
 * For given dialog object type, returns whether it could have shortcut key
 */
static bool SDLGui_CanHaveShortcut(int kind)
{
#ifdef GEKKO
	if (kind == SGBUTTON || kind == SGRADIOBUT || kind == SGCHECKBOX || kind == SGKEY)
#else
	if (kind == SGBUTTON || kind == SGRADIOBUT || kind == SGCHECKBOX)
#endif
		return true;
	return false;
}

/*-----------------------------------------------------------------------*/
/**
 * Check & set dialog item shortcut values based on their text strings.
 * Asserts if dialog has same shortcut defined multiple times.
 */
static void SDLGui_SetShortcuts(SGOBJ *dlg)
{
	unsigned chr, used[256];
	const char *str;
	unsigned int i;

	memset(used, 0, sizeof(used));
	for (i = 0; dlg[i].type != -1; i++)
	{
		if (!SDLGui_CanHaveShortcut(dlg[i].type))
			continue;
		if (!(str = dlg[i].txt))
			continue;
		while(*str)
		{
			if (*str++ == UNDERLINE_INDICATOR)
			{
				/* TODO: conversion */
				chr = toupper(*str);
				dlg[i].shortcut = chr;
				if (used[chr])
				{
					fprintf(stderr, "ERROR: Duplicate Hatari SDL GUI shortcut in '%s'!\n", dlg[i].txt);
					exit(1);
				}
				used[chr] = 1;
			}
		}
	}
}

/*-----------------------------------------------------------------------*/
/**
 * Unfocus given button
 */
static void SDLGui_RemoveFocus(SGOBJ *dlg, int old)
{
	if (!old)
		return;
	dlg[old].state &= ~SG_FOCUSED;
	dlg[old].state |= SG_WASFOCUSED;
	SDLGui_DrawObj(dlg, old);
	dlg[old].state ^= SG_WASFOCUSED;
}

/*-----------------------------------------------------------------------*/
/**
 * Search a next button to focus and focus it
 */
static int SDLGui_FocusNext(SGOBJ *dlg, int i, int inc)
{
	int old = i;
	if (!i)
		return i;

	for (;;)
	{
		i += inc;

		/* wrap */
		if (dlg[i].type == -1)
		{
			i = 0;
		}
		else if (i == 0)
		{
			while (dlg[i].type != -1)
				i++;
			i--;
		}
		/* change focus for items that can have shortcuts
		 * and for items in Fsel lists
		 */
		if (SDLGui_CanHaveShortcut(dlg[i].type) || (dlg[i].flags & SG_EXIT) != 0 )
		{
			dlg[i].state |= SG_FOCUSED;
			SDLGui_DrawObj(dlg, i);
			SDL_UpdateRect(pSdlGuiScrn, 0,0,0,0);
			return i;
		}
		/* wrapped around without even initial one matching */
		if (i == old)
			return 0;
	}
	return old;
}


/*-----------------------------------------------------------------------*/
/**
 * Handle button selection, either with mouse or keyboard
 */
static int SDLGui_HandleSelection(SGOBJ *dlg, int obj, int oldbutton)
{
	SDL_Rect rct;
	int i, retbutton = 0;

	switch (dlg[obj].type)
	{
	case SGBUTTON:
		if (oldbutton==obj)
			retbutton=obj;
		break;
#ifdef GEKKO
	case SGWIIMOTE1:
	case SGWIIMOTE2:
	case SGCLASSIC1:
	case SGCLASSIC2:
	case SGGCPAD1:
	case SGGCPAD2:
		if (oldbutton==obj)
			retbutton=obj;
		break;
	case SGFLOPPYA:
	case SGFLOPPYB:
	case SGLOAD_SAVE:
	case SGSETTINGS:
	case SGRESET:
	case SGQUIT:
	case SGKEY:
		if (oldbutton==obj)
			retbutton=obj;
		break;
#endif
	case SGSCROLLBAR:
		dlg[obj].state &= ~SG_MOUSEDOWN;

		if (oldbutton==obj)
			retbutton=obj;
		break;
	case SGEDITFIELD:
		SDLGui_EditField(dlg, obj);
		break;
	case SGRADIOBUT:
		for (i = obj-1; i > 0 && dlg[i].type == SGRADIOBUT; i--)
		{
			dlg[i].state &= ~SG_SELECTED;  /* Deselect all radio buttons in this group */
			rct.x = (dlg[0].x+dlg[i].x)*sdlgui_fontwidth;
			rct.y = (dlg[0].y+dlg[i].y)*sdlgui_fontheight;
			rct.w = sdlgui_fontwidth;
			rct.h = sdlgui_fontheight;
			SDL_FillRect(pSdlGuiScrn, &rct, colors.midgrey); /* Clear old */
			SDLGui_DrawRadioButton(dlg, i);
			SDL_UpdateRects(pSdlGuiScrn, 1, &rct);
		}
		for (i = obj+1; dlg[i].type == SGRADIOBUT; i++)
		{
			dlg[i].state &= ~SG_SELECTED;  /* Deselect all radio buttons in this group */
			rct.x = (dlg[0].x+dlg[i].x)*sdlgui_fontwidth;
			rct.y = (dlg[0].y+dlg[i].y)*sdlgui_fontheight;
			rct.w = sdlgui_fontwidth;
			rct.h = sdlgui_fontheight;
			SDL_FillRect(pSdlGuiScrn, &rct, colors.midgrey); /* Clear old */
			SDLGui_DrawRadioButton(dlg, i);
			SDL_UpdateRects(pSdlGuiScrn, 1, &rct);
		}
		dlg[obj].state |= SG_SELECTED;  /* Select this radio button */
		rct.x = (dlg[0].x+dlg[obj].x)*sdlgui_fontwidth;
		rct.y = (dlg[0].y+dlg[obj].y)*sdlgui_fontheight;
		rct.w = sdlgui_fontwidth;
		rct.h = sdlgui_fontheight;
		SDL_FillRect(pSdlGuiScrn, &rct, colors.midgrey); /* Clear old */
		SDLGui_DrawRadioButton(dlg, obj);
		SDL_UpdateRects(pSdlGuiScrn, 1, &rct);
		break;
	case SGCHECKBOX:
		dlg[obj].state ^= SG_SELECTED;
#ifdef GEKKO
		if (vkeyboard)
		{
			rct.x = (dlg[0].x+dlg[obj].x);
			rct.y = (dlg[0].y+dlg[obj].y);
		}
		else
		{
			rct.x = (dlg[0].x+dlg[obj].x)*sdlgui_fontwidth;
			rct.y = (dlg[0].y+dlg[obj].y)*sdlgui_fontheight;
		}
#else
		rct.x = (dlg[0].x+dlg[obj].x)*sdlgui_fontwidth;
		rct.y = (dlg[0].y+dlg[obj].y)*sdlgui_fontheight;
#endif
		rct.w = sdlgui_fontwidth;
		rct.h = sdlgui_fontheight;
		SDL_FillRect(pSdlGuiScrn, &rct, colors.midgrey); /* Clear old */
		SDLGui_DrawCheckBox(dlg, obj);
		SDL_UpdateRects(pSdlGuiScrn, 1, &rct);
		break;
	case SGPOPUP:
		dlg[obj].state |= SG_SELECTED;
		SDLGui_DrawPopupButton(dlg, obj);
		SDL_UpdateRect(pSdlGuiScrn,
			       (dlg[0].x+dlg[obj].x)*sdlgui_fontwidth-2,
			       (dlg[0].y+dlg[obj].y)*sdlgui_fontheight-2,
			       dlg[obj].w*sdlgui_fontwidth+4,
			       dlg[obj].h*sdlgui_fontheight+4);
		retbutton=obj;
		break;
	}

	if (!retbutton && (dlg[obj].flags & SG_EXIT) != 0)
	{
		retbutton = obj;
	}

	return retbutton;
}


/*-----------------------------------------------------------------------*/
/**
 * If given shortcut matches item, handle that & return it's code,
 * otherwise return zero.
 */
static int SDLGui_HandleShortcut(SGOBJ *dlg, int key)
{
	int i = 0;
	while (dlg[i].type != -1)
	{
		if (dlg[i].shortcut == key)
			return SDLGui_HandleSelection(dlg, i, i);
		i++;
	}
	return 0;
}

#ifdef GEKKO

/**
 * Wii menu input functions.
 * This code comes from Genesis Plus GX. Clean, short and handy. Merci!
 */
static int wpad_StickX(WPADData *data, u8 right)
{
	struct joystick_t* js = NULL;

	switch (data->exp.type)
	{
		case WPAD_EXP_NUNCHUK:
			js = right ? NULL : &data->exp.nunchuk.js;
			break;

		case WPAD_EXP_CLASSIC:
			js = right ? &data->exp.classic.rjs : &data->exp.classic.ljs;
			break;

		default:
			break;
	}

	if (js)
	{
		/* raw X position */
		int pos = js->pos.x;

		/* X range calibration */
		int min = js->min.x;
		int max = js->max.x;
		int center = js->center.x;
 
		/* value returned could be above calibration limits */
		if (pos > max) return 127;
		if (pos < min) return -128;
		
		/* adjust against center position */
		pos -= center;

		/* return interpolated range [-128;127] */
		if (pos > 0)
		{
			return (int)(127.0 * ((float)pos / (float)(max - center)));
		}
		else
		{
			return (int)(128.0 * ((float)pos / (float)(center - min)));
		}
	}

	return 0;
}

static int wpad_StickY(WPADData *data, u8 right)
{
	struct joystick_t* js = NULL;

	switch (data->exp.type)
	{
		case WPAD_EXP_NUNCHUK:
			js = right ? NULL : &data->exp.nunchuk.js;
			break;

		case WPAD_EXP_CLASSIC:
			js = right ? &data->exp.classic.rjs : &data->exp.classic.ljs;
			break;

		default:
			break;
	}

	if (js)
	{
		/* raw Y position */
		int pos = js->pos.y;

		/* Y range calibration */
		int min = js->min.y;
		int max = js->max.y;
		int center = js->center.y;
 
		/* value returned could be above calibration limits */
		if (pos > max) return 127;
		if (pos < min) return -128;
		
		/* adjust against center position */
		pos -= center;

		/* return interpolated range [-128;127] */
		if (pos > 0)
		{
			return (int)(127.0 * ((float)pos / (float)(max - center)));
		}
		else
		{
			return (int)(128.0 * ((float)pos / (float)(center - min)));
		}
	}

	return 0;
}

/*-----------------------------------------------------------------------*/
/**
 * Menu inputs used in dialogs
 */
void Input_Update(void)
{
	uint32_t held; 
	static int held_cnt = 0;

	/* PAD status update */
	PAD_ScanPads();

	/* PAD pressed keys */
	s16 pp = PAD_ButtonsDown(0);

	/* PAD held keys (direction/selection) */
	s16 hp = PAD_ButtonsHeld(0) & PAD_BUTTONS_HELD;

	/* PAD analog sticks (handled as PAD held direction keys) */
	s8 x= PAD_StickX(0);
	s8 y= PAD_StickY(0);
	if (x > ANALOG_SENSITIVITY)       hp |= PAD_BUTTON_RIGHT;
	else if (x < -ANALOG_SENSITIVITY) hp |= PAD_BUTTON_LEFT;
	else if (y > ANALOG_SENSITIVITY)  hp |= PAD_BUTTON_UP;
	else if (y < -ANALOG_SENSITIVITY) hp |= PAD_BUTTON_DOWN;

	/* WPAD status update */
	WPAD_ScanPads();
	WPADData *data = WPAD_Data(0);

	/* WPAD pressed keys */
	u32 pw = data->btns_d;

	/* WPAD held keys (direction/selection) */
	u32 hw = data->btns_h & WPAD_BUTTONS_HELD;

	/* WPAD analog sticks (handled as PAD held direction keys) */
	x = wpad_StickX(data, 0);
	y = wpad_StickY(data, 0);

	if (x > ANALOG_SENSITIVITY)       hp |= PAD_BUTTON_RIGHT;
	else if (x < -ANALOG_SENSITIVITY) hp |= PAD_BUTTON_LEFT;
	else if (y > ANALOG_SENSITIVITY)  hp |= PAD_BUTTON_UP;
	else if (y < -ANALOG_SENSITIVITY) hp |= PAD_BUTTON_DOWN;

	/* check if any direction/selection key is being held or just being pressed/released */
	if (pp||pw) held_cnt = 0;
	else if (hp||hw) held_cnt++;
	else held_cnt = 0;
		
	/* initial delay (prevents triggering to start immediately) */
	if (held_cnt > HELD_DELAY)
	{
		/* key triggering */
		pp |= hp;
		pw |= hw;

		/* delay until next triggering (adjusts direction/selection update speed) */
		held_cnt -= HELD_SPEED;
	}

	/* Wiimote & Classic Controller direction keys */
	if(data->ir.valid)
	{
		/* Wiimote is handled vertically */
		if (pw & (WPAD_BUTTON_UP))          pp |= PAD_BUTTON_UP;
		else if (pw & (WPAD_BUTTON_DOWN))   pp |= PAD_BUTTON_DOWN;
		else if (pw & (WPAD_BUTTON_LEFT))   pp |= PAD_BUTTON_LEFT;
		else if (pw & (WPAD_BUTTON_RIGHT))  pp |= PAD_BUTTON_RIGHT;
	}
	else
	{
		/* Wiimote is handled horizontally */
		if (pw & (WPAD_BUTTON_UP))          pp |= PAD_BUTTON_LEFT;
		else if (pw & (WPAD_BUTTON_DOWN))   pp |= PAD_BUTTON_RIGHT;
		else if (pw & (WPAD_BUTTON_LEFT))   pp |= PAD_BUTTON_DOWN;
		else if (pw & (WPAD_BUTTON_RIGHT))  pp |= PAD_BUTTON_UP;
	}

	/* Classic Controller direction keys */
	if (pw & WPAD_CLASSIC_BUTTON_UP)          pp |= PAD_BUTTON_UP;
	else if (pw & WPAD_CLASSIC_BUTTON_DOWN)   pp |= PAD_BUTTON_DOWN;
	else if (pw & WPAD_CLASSIC_BUTTON_LEFT)   pp |= PAD_BUTTON_LEFT;
	else if (pw & WPAD_CLASSIC_BUTTON_RIGHT)  pp |= PAD_BUTTON_RIGHT;

	/* WPAD button keys */
	if(!data->ir.valid)
		if (pw & (WPAD_BUTTON_2|WPAD_BUTTON_A|WPAD_CLASSIC_BUTTON_A))  pp |= PAD_BUTTON_A;

	if (pw & (WPAD_BUTTON_1|WPAD_BUTTON_B|WPAD_CLASSIC_BUTTON_B))  pp |= PAD_BUTTON_B;

	if (pw & WPAD_CLASSIC_BUTTON_FULL_L)        pp |= PAD_TRIGGER_L;
	if (pw & WPAD_CLASSIC_BUTTON_FULL_R)        pp |= PAD_TRIGGER_R;
	if (pw & WPAD_CLASSIC_BUTTON_MINUS)         pp |= PAD_TRIGGER_Z;

	if (pw & WPAD_BUTTON_MINUS) pp |=  WPAD_BUTTON_MINUS;
	if (pw & WPAD_BUTTON_PLUS) pp |= WPAD_BUTTON_PLUS;

	/* Update menu inputs */
	MenuInput = pp;
}
#endif

/*-----------------------------------------------------------------------*/
/**
 * Show and process a dialog. Returns the button number that has been
 * pressed or SDLGUI_UNKNOWNEVENT if an unsupported event occurred (will be
 * stored in parameter pEventOut).
 */
int SDLGui_DoDialog(SGOBJ *dlg, SDL_Event *pEventOut, bool KeepCurrentObject)
{
	int obj=0;
	int oldbutton=0;
	int retbutton=0;
	int i, j, b, value;
	SDLKey key;
	int focused;
	SDL_Event sdlEvent;
	SDL_Surface *pBgSurface;
	SDL_Rect dlgrect, bgrect;
	SDL_Joystick *joy = NULL;
#if !WITH_SDL2
	int nOldUnicodeMode;
#endif

#ifdef GEKKO
	int dlgwiibutton = 0;	/* Selected button in the Wii mapper */
	int dialog_type = 0;
	dialog_type = SDLGui_SearchType(dlg);
#endif

	/* In the case of dialog using a scrollbar, we must keep the previous */
	/* value of current_object, as the same dialog is displayed in a loop */
	/* to handle scrolling. For other dialogs, we need to reset current_object */
	/* (ie no object selected at start when displaying the dialog) */
	if ( !KeepCurrentObject )
		current_object = 0;

	if (pSdlGuiScrn->h / sdlgui_fontheight < dlg[0].h)
	{
		fprintf(stderr, "Screen size too small for dialog!\n");
		return SDLGUI_ERROR;
	}

	dlgrect.x = dlg[0].x * sdlgui_fontwidth;
	dlgrect.y = dlg[0].y * sdlgui_fontheight;
	dlgrect.w = dlg[0].w * sdlgui_fontwidth;
	dlgrect.h = dlg[0].h * sdlgui_fontheight;

	bgrect.x = bgrect.y = 0;
	bgrect.w = dlgrect.w;
	bgrect.h = dlgrect.h;

	/* Save background */
	pBgSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, dlgrect.w, dlgrect.h, pSdlGuiScrn->format->BitsPerPixel,
	                                  pSdlGuiScrn->format->Rmask, pSdlGuiScrn->format->Gmask, pSdlGuiScrn->format->Bmask, pSdlGuiScrn->format->Amask);
	if (pSdlGuiScrn->format->palette != NULL)
	{
		SDL_SetColors(pBgSurface, pSdlGuiScrn->format->palette->colors, 0, pSdlGuiScrn->format->palette->ncolors-1);
	}

	if (pBgSurface != NULL)
	{
		SDL_BlitSurface(pSdlGuiScrn,  &dlgrect, pBgSurface, &bgrect);
	}
	else
	{
		fprintf(stderr, "SDLGUI_DoDialog: CreateRGBSurface failed: %s\n", SDL_GetError());
	}

	/* focus default button if nothing else is focused */
	focused = SDLGui_SearchState(dlg, SG_FOCUSED);
	if (!focused)
	{
		int defocus = SDLGui_SearchFlags(dlg, SG_DEFAULT);
		if (defocus)
		{
			dlg[focused].state &= ~SG_FOCUSED;
			dlg[defocus].state |= SG_FOCUSED;
			focused = defocus;
		}
	}
	SDLGui_SetShortcuts(dlg);

#ifdef GEKKO
	SDL_SetCursor(mycursor);
	SDL_ShowCursor(SDL_DISABLE);
#endif

	/* (Re-)draw the dialog */
	SDLGui_DrawDialog(dlg);

#ifdef GEKKO
	SDL_ShowCursor(SDL_ENABLE);
	SDL_Flip(pSdlGuiScrn);
#endif

	/* Is the left mouse button still pressed? Yes -> Handle TOUCHEXIT objects here */
	SDL_PumpEvents();
	b = SDL_GetMouseState(&i, &j);

	/* If current object is the scrollbar, and mouse is still down, we can scroll it */
	/* also if the mouse pointer has left the scrollbar */
	if (current_object >= 0 && dlg[current_object].type == SGSCROLLBAR) {
		if (b & SDL_BUTTON(1)) {
			obj = current_object;
			dlg[obj].state |= SG_MOUSEDOWN;
			oldbutton = obj;
			retbutton = obj;
		}
		else {
			obj = current_object;
			current_object = 0;
			dlg[obj].state &= ~SG_MOUSEDOWN;
			retbutton = obj;
			oldbutton = obj;
		}
	}
	else {
		obj = SDLGui_FindObj(dlg, i, j);
		current_object = obj;
		if (obj > 0 && (dlg[obj].flags&SG_TOUCHEXIT) )
		{
			oldbutton = obj;
			if (b & SDL_BUTTON(1))
			{
				dlg[obj].state |= SG_SELECTED;
				retbutton = obj;
			}
		}
	}

	if (SDL_NumJoysticks() > 0)
		joy = SDL_JoystickOpen(0);

#if !WITH_SDL2
	/* Enable unicode translation to get shifted etc chars with SDL_PollEvent */
	nOldUnicodeMode = SDL_EnableUNICODE(true);
#endif

	/* The main loop */
	while (retbutton == 0 && !bQuitProgram)
	{
#ifdef GEKKO
		SDL_Flip(pSdlGuiScrn);
		Input_Update();

		if (MenuInput & PAD_BUTTON_UP)
		{
			/* Image viewer up */
			if (dialog_type == SGIMGDIAL)
				retbutton = SDLGui_HandleShortcut(dlg, IMG_UPSHORTCUT);
			else
			{
				/* Focus previous item */
				SDLGui_RemoveFocus(dlg, focused);
				focused = SDLGui_FocusNext(dlg, focused, -1);
			}
		}
		else  if (MenuInput & PAD_BUTTON_DOWN)
		{
			/* Image viewer down */
			if (dialog_type == SGIMGDIAL)
				retbutton = SDLGui_HandleShortcut(dlg, IMG_DOWNSHORTCUT);
			else
			{	
				/* Focus next item */
				SDLGui_RemoveFocus(dlg, focused);
				focused = SDLGui_FocusNext(dlg, focused, +1);
			}
		}
		else  if (MenuInput & PAD_BUTTON_LEFT)
		{
			/* Press scrollbar page up */
			if (dialog_type == SGSCROLLBAR)
			{
				SDLGui_RemoveFocus(dlg, focused);
				/* Focus first file in dialog */
				focused = SDLGui_FocusNext(dlg, 11, +1);
				retbutton = SDLGui_HandleSelection(dlg, 25, 25);
				}
			/* Image viewer left */
			else if (dialog_type == SGIMGDIAL)
				retbutton = SDLGui_HandleShortcut(dlg, IMG_LEFTSHORTCUT);
				/* Focus previous item */
			else
			{
				SDLGui_RemoveFocus(dlg, focused);
				focused = SDLGui_FocusNext(dlg, focused, -1);
			}
		}
		else  if (MenuInput & PAD_BUTTON_RIGHT)
		{
			/* Press scrollbar page down */
			if (dialog_type == SGSCROLLBAR)
			{
				SDLGui_RemoveFocus(dlg, focused);
				/* Focus first file in dialog */
				focused = SDLGui_FocusNext(dlg, 11, +1);
				retbutton = SDLGui_HandleSelection(dlg, 26, 26);
			}
			/* Image viewer right */
			else if (dialog_type == SGIMGDIAL)
				retbutton = SDLGui_HandleShortcut(dlg, IMG_RIGHTSHORTCUT);
			else
			{
				/* Focus next item */
				SDLGui_RemoveFocus(dlg, focused);
				focused = SDLGui_FocusNext(dlg, focused, +1);
			}
		}

		if (MenuInput & PAD_BUTTON_A)
		{
			retbutton = SDLGui_HandleSelection(dlg, focused, focused);
		}
		else if (MenuInput & PAD_BUTTON_B)
		{
			retbutton = SDLGui_SearchFlags(dlg, SG_CANCEL);	
		}
		else if (MenuInput & PAD_TRIGGER_L)
		{
			/* Image viewer Page up */
			if (dialog_type == SGIMGDIAL)
			{
				retbutton = SDLGui_HandleShortcut(dlg, IMG_PAGEUPSHORTCUT);
			}
			else
			{
				SDLGui_RemoveFocus(dlg, focused);								/* Wiimote/Classic - : Go to first item */
				focused = SDLGui_FocusNext(dlg, 1, +1);
			}
		}
		else if (MenuInput & PAD_TRIGGER_R || MenuInput & WPAD_BUTTON_PLUS)
		{
			/* Image viewer Page down */
			if (dialog_type == SGIMGDIAL)
				retbutton = SDLGui_HandleShortcut(dlg, IMG_PAGEDOWNSHORTCUT);
			else
			{
				SDLGui_RemoveFocus(dlg, focused);								/* Wiimote/Classic R : Go to last item */
				focused = SDLGui_SearchFlags(dlg, SG_CANCEL);
				focused = SDLGui_FocusNext(dlg, focused, -1);
			}
		}
		/* Wii controller type found : Wii mapper dialog */	
		else if (MenuInput & PAD_TRIGGER_Z && dialog_type == SGWIIMOTE1)
		{
			/* Send the selected button to clear */
			dlgwiibutton = SDLGui_HandleSelection(dlg, focused, focused);
			Clear_JoyBinding(dlgwiibutton);

			/* Push the button to update key names */
			retbutton = SDLGui_HandleSelection(dlg, 36, 36);
		}
		else if (MenuInput & WPAD_BUTTON_MINUS)
		{
			/* Wii controller type found : Wii mapper dialog */	
			if (dialog_type == SGWIIMOTE1)
			{
				/* Send the selected button to clear */
				dlgwiibutton = SDLGui_HandleSelection(dlg, focused, focused);
				Clear_JoyBinding(dlgwiibutton);
				/* Push the button to update key names */
				retbutton = SDLGui_HandleSelection(dlg, 36, 36);
			}
			/* Image viewer Page up */
			else if (dialog_type == SGIMGDIAL)
			{
				retbutton = SDLGui_HandleShortcut(dlg, IMG_PAGEUPSHORTCUT);
			}
			else
			{
				SDLGui_RemoveFocus(dlg, focused);								/* Wiimote/Classic - : Go to first item */
				focused = SDLGui_FocusNext(dlg, 1, +1);
			}
		}

		if (SDL_ShowCursor(SDL_QUERY) == SDL_DISABLE)
			SDL_ShowCursor(SDL_ENABLE);

		while(SDL_PollEvent(&sdlEvent))
		{
#else
		if (SDL_WaitEvent(&sdlEvent) == 1)  /* Wait for events */
#endif		
			switch (sdlEvent.type)
			{
			 case SDL_QUIT:
				retbutton = SDLGUI_QUIT;
				break;
				
			 case SDL_MOUSEBUTTONDOWN:
				if (sdlEvent.button.button != SDL_BUTTON_LEFT)
				{
					/* Not left mouse button -> unsupported event */
					if (pEventOut)
						retbutton = SDLGUI_UNKNOWNEVENT;
					break;
				}
				/* It was the left button: Find the object under the mouse cursor */
				obj = SDLGui_FindObj(dlg, sdlEvent.button.x, sdlEvent.button.y);
				if (obj>0)
				{
					if (dlg[obj].type==SGBUTTON)
					{
						dlg[obj].state |= SG_SELECTED;
						SDLGui_DrawButton(dlg, obj);
						SDL_UpdateRect(pSdlGuiScrn, (dlg[0].x+dlg[obj].x)*sdlgui_fontwidth-2, (dlg[0].y+dlg[obj].y)*sdlgui_fontheight-2,
						               dlg[obj].w*sdlgui_fontwidth+4, dlg[obj].h*sdlgui_fontheight+4);
						oldbutton=obj;
					}
#ifdef GEKKO
					if (dlg[obj].type == SGWIIMOTE1)
					{
						dlg[obj].state |= SG_SELECTED;
						oldbutton=obj;
					}
					if (dlg[obj].type == SGWIIMOTE2)
					{
						dlg[obj].state |= SG_SELECTED;
						oldbutton=obj;
					}
					if (dlg[obj].type == SGCLASSIC1)
					{
						dlg[obj].state |= SG_SELECTED;
						oldbutton=obj;
					}
					if (dlg[obj].type == SGCLASSIC2)
					{
						dlg[obj].state |= SG_SELECTED;
						oldbutton=obj;
					}
					if (dlg[obj].type == SGGCPAD1)
					{
						dlg[obj].state |= SG_SELECTED;
						oldbutton=obj;
					}
					if (dlg[obj].type == SGGCPAD2)
					{
						dlg[obj].state |= SG_SELECTED;
						oldbutton=obj;
					}
					if (dlg[obj].type == SGFLOPPYA)
					{
						dlg[obj].state |= SG_SELECTED;
						oldbutton=obj;
					}
					if (dlg[obj].type == SGFLOPPYB)
					{
						dlg[obj].state |= SG_SELECTED;
						oldbutton=obj;
					}
					if (dlg[obj].type == SGLOAD_SAVE)
					{
						dlg[obj].state |= SG_SELECTED;
						oldbutton=obj;
					}
					if (dlg[obj].type == SGSETTINGS)
					{
						dlg[obj].state |= SG_SELECTED;
						oldbutton=obj;
					}
					if (dlg[obj].type == SGRESET)
					{
						dlg[obj].state |= SG_SELECTED;
						oldbutton=obj;
					}
					if (dlg[obj].type == SGQUIT)
					{
						dlg[obj].state |= SG_SELECTED;
						SDL_UpdateRect(pBgSurface, (dlg[0].x+dlg[obj].x)*sdlgui_fontwidth-2, (dlg[0].y+dlg[obj].y)*sdlgui_fontheight-2,
						               dlg[obj].w*sdlgui_fontwidth+4, dlg[obj].h*sdlgui_fontheight+4);
						oldbutton=obj;
					}
#endif
					if (dlg[obj].type == SGSCROLLBAR)
					{
						dlg[obj].state |= SG_MOUSEDOWN;
						oldbutton=obj;
					}
					if ( dlg[obj].flags & SG_TOUCHEXIT )
					{
						dlg[obj].state |= SG_SELECTED;
						retbutton = obj;
					}

				}
				break;

			 case SDL_MOUSEBUTTONUP:
				if (sdlEvent.button.button != SDL_BUTTON_LEFT)
				{
					/* Not left mouse button -> unsupported event */
					if (pEventOut)
						retbutton = SDLGUI_UNKNOWNEVENT;
					break;
				}
				/* It was the left button: Find the object under the mouse cursor */
				obj = SDLGui_FindObj(dlg, sdlEvent.button.x, sdlEvent.button.y);
				if (obj>0)
				{
					retbutton = SDLGui_HandleSelection(dlg, obj, oldbutton);
				}
				if (oldbutton > 0 && dlg[oldbutton].type == SGBUTTON)
				{
					dlg[oldbutton].state &= ~SG_SELECTED;
					SDLGui_DrawButton(dlg, oldbutton);
					SDL_UpdateRect(pSdlGuiScrn, (dlg[0].x+dlg[oldbutton].x)*sdlgui_fontwidth-2, (dlg[0].y+dlg[oldbutton].y)*sdlgui_fontheight-2,
					               dlg[oldbutton].w*sdlgui_fontwidth+4, dlg[oldbutton].h*sdlgui_fontheight+4);
					oldbutton = 0;
				}
#ifdef GEKKO
				if (oldbutton > 0 && dlg[oldbutton].type == SGWIIMOTE1)
				{
					dlg[oldbutton].state &= ~SG_SELECTED;
					oldbutton = 0;
				}
				if (oldbutton > 0 && dlg[oldbutton].type == SGWIIMOTE2)
				{
					dlg[oldbutton].state &= ~SG_SELECTED;
					oldbutton = 0;
				}
				if (oldbutton > 0 && dlg[oldbutton].type == SGCLASSIC1)
				{
					dlg[oldbutton].state &= ~SG_SELECTED;
					oldbutton = 0;
				}
				if (oldbutton > 0 && dlg[oldbutton].type == SGCLASSIC2)
				{
					dlg[oldbutton].state &= ~SG_SELECTED;
					oldbutton = 0;
				}
				if (oldbutton > 0 && dlg[oldbutton].type == SGGCPAD1)
				{
					dlg[oldbutton].state &= ~SG_SELECTED;
					oldbutton = 0;
				}
				if (oldbutton > 0 && dlg[oldbutton].type == SGGCPAD2)
				{
					dlg[oldbutton].state &= ~SG_SELECTED;
					oldbutton = 0;
				}
				if (oldbutton > 0 && dlg[oldbutton].type == SGFLOPPYA)
				{
					dlg[oldbutton].state &= ~SG_SELECTED;
					oldbutton = 0;
				}
				if (oldbutton > 0 && dlg[oldbutton].type == SGFLOPPYB)
				{
					dlg[oldbutton].state &= ~SG_SELECTED;
					oldbutton = 0;
				}
				if (oldbutton > 0 && dlg[oldbutton].type == SGLOAD_SAVE)
				{
					dlg[oldbutton].state &= ~SG_SELECTED;
					oldbutton = 0;
				}
				if (oldbutton > 0 && dlg[oldbutton].type == SGSETTINGS)
				{
					dlg[oldbutton].state &= ~SG_SELECTED;
					oldbutton = 0;
				}
				if (oldbutton > 0 && dlg[oldbutton].type == SGRESET)
				{
					dlg[oldbutton].state &= ~SG_SELECTED;
					oldbutton = 0;
				}
				if (oldbutton > 0 && dlg[oldbutton].type == SGQUIT)
				{
					dlg[oldbutton].state &= ~SG_SELECTED;
						SDL_UpdateRect(pBgSurface, (dlg[0].x+dlg[obj].x)*sdlgui_fontwidth-2, (dlg[0].y+dlg[obj].y)*sdlgui_fontheight-2,
						               dlg[obj].w*sdlgui_fontwidth+4, dlg[obj].h*sdlgui_fontheight+4);
					SDL_ShowCursor(SDL_DISABLE);
					oldbutton = 0;
				}
#endif
				break;
				
			 case SDL_JOYAXISMOTION:
#ifndef GEKKO
				value = sdlEvent.jaxis.value;
				if (value < -3200 || value > 3200)
				{
					if(sdlEvent.jaxis.axis == 0)
					{
						/* Left-right movement */
						if (value < 0)
							retbutton = SDLGui_HandleShortcut(dlg, SG_SHORTCUT_LEFT);
						else
							retbutton = SDLGui_HandleShortcut(dlg, SG_SHORTCUT_RIGHT);
					}
					else if(sdlEvent.jaxis.axis == 1)
					{
						/* Up-Down movement */
						if (value < 0)
						{
							SDLGui_RemoveFocus(dlg, focused);
							focused = SDLGui_FocusNext(dlg, focused, -1);
						}
						else
						{
							SDLGui_RemoveFocus(dlg, focused);
							focused = SDLGui_FocusNext(dlg, focused, +1);
						}
					}
				}
#endif
				break;
				
				case SDL_JOYBUTTONDOWN:
#ifdef GEKKO
					/* FIXME : GC pad only due to issue with checkboxes in main menu. */	
					if (dialog_type != SGWIIMOTE1 && (sdlEvent.jbutton.which == 4 && sdlEvent.jbutton.button == 0))
#endif
						retbutton = SDLGui_HandleSelection(dlg, focused, focused);
				break;

			 case SDL_JOYBALLMOTION:
			 case SDL_JOYHATMOTION:
			 case SDL_MOUSEMOTION:
				break;

			 case SDL_KEYDOWN:                     /* Key pressed */
				key = sdlEvent.key.keysym.sym;
				/* keyboard shortcuts are with modifiers */
				if (sdlEvent.key.keysym.mod & KMOD_LALT
				    || sdlEvent.key.keysym.mod & KMOD_RALT)
				{
					if (key == SDLK_LEFT)
						retbutton = SDLGui_HandleShortcut(dlg, SG_SHORTCUT_LEFT);
					else if (key == SDLK_RIGHT)
						retbutton = SDLGui_HandleShortcut(dlg, SG_SHORTCUT_RIGHT);
					else if (key == SDLK_UP)
						retbutton = SDLGui_HandleShortcut(dlg, SG_SHORTCUT_UP);
					else if (key == SDLK_DOWN)
						retbutton = SDLGui_HandleShortcut(dlg, SG_SHORTCUT_DOWN);
					else
					{
#if !WITH_SDL2
						/* unicode member is needed to handle shifted etc special chars */
						key = sdlEvent.key.keysym.unicode;
#endif
						if (key >= 33 && key <= 126)
							retbutton = SDLGui_HandleShortcut(dlg, toupper(key));
					}
					if (!retbutton && pEventOut)
						retbutton = SDLGUI_UNKNOWNEVENT;
					break;
				}
				switch (key)
				{
				 case SDLK_UP:
				 case SDLK_LEFT:
					SDLGui_RemoveFocus(dlg, focused);
					focused = SDLGui_FocusNext(dlg, focused, -1);
					break;
				 case SDLK_TAB:
				 case SDLK_DOWN:
				 case SDLK_RIGHT:
					SDLGui_RemoveFocus(dlg, focused);
					focused = SDLGui_FocusNext(dlg, focused, +1);
					break;
				 case SDLK_HOME:
					SDLGui_RemoveFocus(dlg, focused);
					focused = SDLGui_FocusNext(dlg, 1, +1);
					break;
				 case SDLK_END:
					SDLGui_RemoveFocus(dlg, focused);
					focused = SDLGui_FocusNext(dlg, 1, -1);
					break;
				 case SDLK_SPACE:
				 case SDLK_RETURN:
				 case SDLK_KP_ENTER:
					retbutton = SDLGui_HandleSelection(dlg, focused, focused);
					break;
				 case SDLK_ESCAPE:
					retbutton = SDLGui_SearchFlags(dlg, SG_CANCEL);
					break;
				 default:
					if (pEventOut)
						retbutton = SDLGUI_UNKNOWNEVENT;
					break;
				}
				break;

			 default:
				if (pEventOut)
					retbutton = SDLGUI_UNKNOWNEVENT;
				break;
			}
#ifdef GEKKO
		}
#endif
	}

	/* Restore background */
	if (pBgSurface)
	{
		SDL_BlitSurface(pBgSurface, &bgrect, pSdlGuiScrn,  &dlgrect);
		SDL_FreeSurface(pBgSurface);
	}

	/* Copy event data of unsupported events if caller wants to have it */
	if (retbutton == SDLGUI_UNKNOWNEVENT && pEventOut)
		memcpy(pEventOut, &sdlEvent, sizeof(SDL_Event));

	if (retbutton == SDLGUI_QUIT)
		bQuitProgram = true;

#if !WITH_SDL2
	SDL_EnableUNICODE(nOldUnicodeMode);
#endif
	if (joy)
		SDL_JoystickClose(joy);

	return retbutton;
}

#ifdef GEKKO
/*-----------------------------------------------------------------------*/
/**
 * Show and process a dialog. Returns the button number that has been
 * pressed or SDLGUI_UNKNOWNEVENT if an unsupported event occurred (will be
 * stored in parameter pEventOut).
 * It's a duplicate function used for the virtual keyboard dialog.
 * The main difference is the mouse object detection(SDLGui_FindObjKey).
 */
int SDLGui_DoDialogKeyboard(SGOBJ *dlg, SDL_Event *pEventOut, bool KeepCurrentObject)
{
	int obj=0;
	int oldbutton=0;
	int retbutton=0;
	int i, j, b, value;
	SDLKey key;
	int focused;
	SDL_Event sdlEvent;
	SDL_Surface *pBgSurface;
	SDL_Rect dlgrect, bgrect;
	SDL_Joystick *joy = NULL;
#if !WITH_SDL2
	int nOldUnicodeMode;
#endif

	vkeyboard = true;

	/* In the case of dialog using a scrollbar, we must keep the previous */
	/* value of current_object, as the same dialog is displayed in a loop */
	/* to handle scrolling. For other dialogs, we need to reset current_object */
	/* (ie no object selected at start when displaying the dialog) */
	if ( !KeepCurrentObject )
		current_object = 0;

	if (pSdlGuiScrn->h / sdlgui_fontheight < dlg[0].h)
	{
		fprintf(stderr, "Screen size too small for dialog!\n");
		return SDLGUI_ERROR;
	}

	dlgrect.x = dlg[0].x * sdlgui_fontwidth;
	dlgrect.y = dlg[0].y * sdlgui_fontheight;
	dlgrect.w = dlg[0].w * sdlgui_fontwidth;
	dlgrect.h = dlg[0].h * sdlgui_fontheight;

	bgrect.x = bgrect.y = 0;
	bgrect.w = dlgrect.w;
	bgrect.h = dlgrect.h;

	/* Save background */
	pBgSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, dlgrect.w, dlgrect.h, pSdlGuiScrn->format->BitsPerPixel,
	                                  pSdlGuiScrn->format->Rmask, pSdlGuiScrn->format->Gmask, pSdlGuiScrn->format->Bmask, pSdlGuiScrn->format->Amask);
	if (pSdlGuiScrn->format->palette != NULL)
	{
		SDL_SetColors(pBgSurface, pSdlGuiScrn->format->palette->colors, 0, pSdlGuiScrn->format->palette->ncolors-1);
	}

	if (pBgSurface != NULL)
	{
		SDL_BlitSurface(pSdlGuiScrn,  &dlgrect, pBgSurface, &bgrect);
	}
	else
	{
		fprintf(stderr, "SDLGUI_DoDialog: CreateRGBSurface failed: %s\n", SDL_GetError());
	}

	/* focus default button if nothing else is focused */
	focused = SDLGui_SearchState(dlg, SG_FOCUSED);
	if (!focused)
	{
		int defocus = SDLGui_SearchFlags(dlg, SG_DEFAULT);
		if (defocus)
		{
			dlg[focused].state &= ~SG_FOCUSED;
			dlg[defocus].state |= SG_FOCUSED;
			focused = defocus;
		}
	}
	SDLGui_SetShortcuts(dlg);

	SDL_SetCursor(mycursor);
	SDL_ShowCursor(SDL_DISABLE);

	/* (Re-)draw the dialog */
	SDLGui_DrawDialog(dlg);

	SDL_ShowCursor(SDL_ENABLE);
	SDL_Flip(pSdlGuiScrn);

	/* Is the left mouse button still pressed? Yes -> Handle TOUCHEXIT objects here */
	SDL_PumpEvents();
	b = SDL_GetMouseState(&i, &j);

	/* If current object is the scrollbar, and mouse is still down, we can scroll it */
	/* also if the mouse pointer has left the scrollbar */
	if (current_object >= 0 && dlg[current_object].type == SGSCROLLBAR) {
		if (b & SDL_BUTTON(1)) {
			obj = current_object;
			dlg[obj].state |= SG_MOUSEDOWN;
			oldbutton = obj;
			retbutton = obj;
		}
		else {
			obj = current_object;
			current_object = 0;
			dlg[obj].state &= ~SG_MOUSEDOWN;
			retbutton = obj;
			oldbutton = obj;
		}
	}
	else {
		obj = SDLGui_FindObjKey(dlg, i, j);
		current_object = obj;
		if (obj > 0 && (dlg[obj].flags&SG_TOUCHEXIT) )
		{
			oldbutton = obj;
			if (b & SDL_BUTTON(1))
			{
				dlg[obj].state |= SG_SELECTED;
				retbutton = obj;
			}
		}
	}

	if (SDL_NumJoysticks() > 0)
		joy = SDL_JoystickOpen(0);

#if !WITH_SDL2
	/* Enable unicode translation to get shifted etc chars with SDL_PollEvent */
	nOldUnicodeMode = SDL_EnableUNICODE(true);
#endif

	/* The main loop */
	while (retbutton == 0 && !bQuitProgram)
	{
		SDL_Flip(pSdlGuiScrn);
		Input_Update();

		if (MenuInput & PAD_BUTTON_UP)
		{
				/* Focus previous item */
				SDLGui_RemoveFocus(dlg, focused);
				focused = SDLGui_FocusNext(dlg, focused, -1);
		}
		else  if (MenuInput & PAD_BUTTON_DOWN)
		{
				/* Focus next item */
				SDLGui_RemoveFocus(dlg, focused);
				focused = SDLGui_FocusNext(dlg, focused, +1);
		}
		else  if (MenuInput & PAD_BUTTON_LEFT)
		{
				/* Focus previous item */
				SDLGui_RemoveFocus(dlg, focused);
				focused = SDLGui_FocusNext(dlg, focused, -1);
		}
		else  if (MenuInput & PAD_BUTTON_RIGHT)
		{
				/* Focus next item */
				SDLGui_RemoveFocus(dlg, focused);
				focused = SDLGui_FocusNext(dlg, focused, +1);
		}

		if (MenuInput & PAD_BUTTON_A)
		{
			retbutton = SDLGui_HandleSelection(dlg, focused, focused);
		}
		else if (MenuInput & PAD_BUTTON_B)
		{
			retbutton = SDLGui_SearchFlags(dlg, SG_CANCEL);	
		}
		else if (MenuInput & PAD_TRIGGER_L ||  MenuInput & WPAD_BUTTON_MINUS)
		{
			SDLGui_RemoveFocus(dlg, focused);								/* Wiimote/Classic - : Go to first item */
			focused = SDLGui_FocusNext(dlg, 1, +1);
		}
		else if (MenuInput & PAD_TRIGGER_R || MenuInput & WPAD_BUTTON_PLUS)
		{
			SDLGui_RemoveFocus(dlg, focused);								/* Wiimote/Classic R : Go to last item */
			focused = SDLGui_SearchFlags(dlg, SG_CANCEL);
			focused = SDLGui_FocusNext(dlg, focused, -1);
		}

		if (SDL_ShowCursor(SDL_QUERY) == SDL_DISABLE)
			SDL_ShowCursor(SDL_ENABLE);

		 while(SDL_PollEvent(&sdlEvent))
		{
		switch (sdlEvent.type)
			{
			 case SDL_MOUSEBUTTONDOWN:
				if (sdlEvent.button.button != SDL_BUTTON_LEFT)
				{
					/* Not left mouse button -> unsupported event */
					if (pEventOut)
						retbutton = SDLGUI_UNKNOWNEVENT;
					break;
				}
				/* It was the left button: Find the object under the mouse cursor */
				obj = SDLGui_FindObjKey(dlg, sdlEvent.button.x, sdlEvent.button.y);
				if (obj>0)
				{
					if (dlg[obj].type==SGBUTTON)
					{
						dlg[obj].state |= SG_SELECTED;
						SDLGui_DrawButton(dlg, obj);
						SDL_UpdateRect(pSdlGuiScrn, (dlg[0].x+dlg[obj].x)-2, (dlg[0].y+dlg[obj].y)-2,
						               dlg[obj].w+4, dlg[obj].h+4);
						oldbutton=obj;
					}
					if (dlg[obj].type == SGKEY)
					{
						dlg[obj].state |= SG_SELECTED;
						oldbutton=obj;
					}
					if ( dlg[obj].flags & SG_TOUCHEXIT )
					{
						dlg[obj].state |= SG_SELECTED;
						retbutton = obj;
					}
				}
				break;

			 case SDL_MOUSEBUTTONUP:
				if (sdlEvent.button.button != SDL_BUTTON_LEFT)
				{
					/* Not left mouse button -> unsupported event */
					if (pEventOut)
						retbutton = SDLGUI_UNKNOWNEVENT;
					break;
				}
				/* It was the left button: Find the object under the mouse cursor */
				obj = SDLGui_FindObjKey(dlg, sdlEvent.button.x, sdlEvent.button.y);
				if (obj>0)
				{
					retbutton = SDLGui_HandleSelection(dlg, obj, oldbutton);
				}
				if (oldbutton > 0 && dlg[oldbutton].type == SGBUTTON)
				{

					dlg[oldbutton].state &= ~SG_SELECTED;
					SDLGui_DrawButton(dlg, oldbutton);
					SDL_UpdateRect(pSdlGuiScrn, (dlg[0].x+dlg[oldbutton].x)-2, (dlg[0].y+dlg[oldbutton].y)-2,
					               dlg[oldbutton].w+4, dlg[oldbutton].h+4);

					SDL_ShowCursor(SDL_DISABLE);

					oldbutton = 0;
				}
				if (oldbutton > 0 && dlg[oldbutton].type == SGKEY)
				{
					dlg[oldbutton].state &= ~SG_SELECTED;
					oldbutton = 0;
				}
				break;
				case SDL_JOYBUTTONDOWN:
					if (sdlEvent.jbutton.which == 4 && sdlEvent.jbutton.button == 0)
					{
						retbutton = SDLGui_HandleSelection(dlg, focused, focused);
					}
				break;

				case SDL_JOYHATMOTION:
				break;
			}
		}
	}

	/* Restore background */
	if (pBgSurface)
	{
		SDL_BlitSurface(pBgSurface, &bgrect, pSdlGuiScrn,  &dlgrect);
		SDL_FreeSurface(pBgSurface);
	}

	/* Copy event data of unsupported events if caller wants to have it */
	if (retbutton == SDLGUI_UNKNOWNEVENT && pEventOut)
		memcpy(pEventOut, &sdlEvent, sizeof(SDL_Event));

	if (retbutton == SDLGUI_QUIT)
		bQuitProgram = true;

#if !WITH_SDL2
	SDL_EnableUNICODE(nOldUnicodeMode);
#endif
	if (joy)
		SDL_JoystickClose(joy);

	vkeyboard = false;

	return retbutton;
}
#endif
