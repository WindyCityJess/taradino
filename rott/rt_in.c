/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "modexlib.h"

#include "rt_main.h"
#include "rt_def.h"
#include "rt_in.h"
#include "_rt_in.h"
#include "isr.h"
#include "rt_util.h"
#include "rt_cfg.h"
#include "rt_msg.h"
#include "rt_playr.h"
#include "rt_net.h"
#include "rt_com.h"
#include "rt_cfg.h"
#include "keyb.h"

#define MAXMESSAGELENGTH (COM_MAXTEXTSTRINGLENGTH - 1)

//****************************************************************************
//
// GLOBALS
//
//****************************************************************************]

//
// Used by menu routines that need to wait for a button release.
// Sometimes the mouse driver misses an interrupt, so you can't wait for
// a button to be released.  Instead, you must ignore any buttons that
// are pressed.
//
int IgnoreMouse = 0;

// configuration variables
//
boolean MousePresent;
boolean JoysPresent[MaxJoys];
boolean JoyPadPresent = 0;

//    Global variables
//
boolean Paused;
char LastASCII;
int LastScan;

byte Joy_xb, Joy_yb, Joy_xs, Joy_ys;
unsigned short Joy_x, Joy_y;

int LastLetter = 0;
char LetterQueue[MAXLETTERS];
ModemMessage MSG;

static SDL_Joystick *sdl_joysticks[MaxJoys];
static unsigned short sdl_mouse_button_mask = 0;
static int sdl_total_sticks = 0;
static unsigned short sdl_stick_button_state[MaxJoys];
static int sdl_mouse_grabbed = 0;
extern boolean sdl_fullscreen;

//   'q','w','e','r','t','y','u','i','o','p','[',']','\\', 0 ,'a','s',

const char ScanChars[128] = // Scan code names with single chars
	{ 0,   0,	 '1', '2', '3',	 '4', '5', '6', '7', '8', '9', '0', '-',
	  '=', 0,	 0,	  'q', 'w',	 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
	  '[', ']',	 0,	  0,   'a',	 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
	  ';', '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
	  '.', '/',	 0,	  0,   0,	 ' ', 0,   0,	0,	 0,	  0,   0,	0,
	  0,   0,	 0,	  0,   0,	 0,	  0,   0,	0,	 '-', 0,   '5', 0,
	  '+', 0,	 0,	  0,   0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,
	  0,   0,	 0,	  0,   0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,
	  0,   0,	 0,	  0,   0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,
	  0,   0,	 0,	  0,   0,	 0,	  0,   0,	0,	 0,	  0 };

const char ShiftedScanChars[128] = // Shifted Scan code names with single chars
	{ 0,   0,	'!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,
	  0,   'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,	0,
	  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|', 'Z',
	  'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,	0,	 0,	  ' ', 0,	0,
	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	'-',
	  0,   '5', 0,	 '+', 0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,
	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,
	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,	 0,	  0,   0,	0,
	  0,   0,	0,	 0,	  0,   0,	0,	 0 };

//****************************************************************************
//
// LOCALS
//
//****************************************************************************]

#define SCANCODE_TO_KEYS_ARRAY \
	{ 0, \
	  0, \
	  0, \
	  0, \
	  sc_A, \
	  sc_B, \
	  sc_C, \
	  sc_D, \
	  sc_E, \
	  sc_F, \
	  sc_G, \
	  sc_H, \
	  sc_I, \
	  sc_J, \
	  sc_K, \
	  sc_L, \
	  sc_M, \
	  sc_N, \
	  sc_O, \
	  sc_P, \
	  sc_Q, \
	  sc_R, \
	  sc_S, \
	  sc_T, \
	  sc_U, \
	  sc_V, \
	  sc_W, \
	  sc_X, \
	  sc_Y, \
	  sc_Z, \
	  sc_1, \
	  sc_2, \
	  sc_3, \
	  sc_4, \
	  sc_5, \
	  sc_6, \
	  sc_7, \
	  sc_8, \
	  sc_9, \
	  sc_0, \
	  sc_Return, \
	  sc_Escape, \
	  sc_BackSpace, \
	  sc_Tab, \
	  sc_Space, \
	  sc_Minus, \
	  sc_Equals, \
	  sc_OpenBracket, \
	  sc_CloseBracket, \
	  0x2B, \
	  0, \
	  0x27, \
	  0x28, \
	  0x29, \
	  sc_Comma, \
	  sc_Period, \
	  0x35, \
	  sc_CapsLock, \
	  sc_F1, \
	  sc_F2, \
	  sc_F3, \
	  sc_F4, \
	  sc_F5, \
	  sc_F6, \
	  sc_F7, \
	  sc_F8, \
	  sc_F9, \
	  sc_F10, \
	  sc_F11, \
	  sc_F12, \
	  sc_PrintScreen, \
	  0x46, \
	  0, \
	  sc_Insert, \
	  sc_Home, \
	  sc_PgUp, \
	  sc_Delete, \
	  sc_End, \
	  sc_PgDn, \
	  sc_RightArrow, \
	  sc_LeftArrow, \
	  sc_DownArrow, \
	  sc_UpArrow, \
	  0x45, \
	  0x35, \
	  0x37, \
	  sc_Minus, \
	  sc_Plus, \
	  sc_Return, \
	  sc_End, \
	  sc_DownArrow, \
	  sc_PgDn, \
	  sc_LeftArrow, \
	  0x4c, \
	  sc_RightArrow, \
	  sc_Home, \
	  sc_UpArrow, \
	  sc_PgUp, \
	  sc_Insert, \
	  sc_Period, \
	  0, \
	  0, \
	  0, \
	  sc_Equals, \
	  sc_F13, \
	  sc_F14 }

static const int scancode_translate_table[] = SCANCODE_TO_KEYS_ARRAY;

static int GetScancode(const int scancode)
{
	switch (scancode)
	{
		case SDL_SCANCODE_LCTRL:
		case SDL_SCANCODE_RCTRL:
			return sc_Control;

		case SDL_SCANCODE_LSHIFT:
			return sc_LShift;
		case SDL_SCANCODE_RSHIFT:
			return sc_RShift;

		case SDL_SCANCODE_LALT:
		case SDL_SCANCODE_RALT:
			return sc_Alt;

		default:
			if (scancode >= 0 && scancode < arrlen(scancode_translate_table))
			{
				return scancode_translate_table[scancode];
			}
			else
			{
				return 0;
			}
	}
}

static KeyboardDef KbdDefs = { 0x1d, 0x38, 0x47, 0x48, 0x49,
							   0x4b, 0x4d, 0x4f, 0x50, 0x51 };
static JoystickDef JoyDefs[MaxJoys];
static ControlType Controls[MAXPLAYERS];

static boolean IN_Started;

static Direction DirTable[] = // Quick lookup for total direction
	{ dir_NorthWest, dir_North,		dir_NorthEast, dir_West,	 dir_None,
	  dir_East,		 dir_SouthWest, dir_South,	   dir_SouthEast };

static char *ParmStrings[] = { "nojoys", "nomouse", NULL };

static int sdl_mouse_button_filter(SDL_Event const *event)
{
	/*
	 * What DOS games expect:
	 *  0	left button pressed if 1
	 *  1	right button pressed if 1
	 *  2	middle button pressed if 1
	 *
	 *   (That is, this is what Int 33h (AX=0x05) returns...)
	 */

	Uint8 bmask = SDL_GetMouseState(NULL, NULL);
	sdl_mouse_button_mask = 0; /* this is a static var. */
	if (bmask & SDL_BUTTON_LMASK)
		sdl_mouse_button_mask |= 1;
	if (bmask & SDL_BUTTON_RMASK)
		sdl_mouse_button_mask |= 2;
	if (bmask & SDL_BUTTON_MMASK)
		sdl_mouse_button_mask |= 4;
	return (0);
} /* sdl_mouse_up_filter */

static int sdl_mouse_motion_filter(SDL_Event const *event)
{
	static int mouse_x = 0;
	static int mouse_y = 0;
	int mouse_relative_x = 0;
	int mouse_relative_y = 0;

	if (event->type == SDL_JOYBALLMOTION)
	{
		mouse_relative_x = event->jball.xrel / 100;
		mouse_relative_y = event->jball.yrel / 100;
		mouse_x += mouse_relative_x;
		mouse_y += mouse_relative_y;
	} /* if */
	else
	{
		if (sdl_mouse_grabbed || sdl_fullscreen)
		{
			mouse_relative_x = event->motion.xrel;
			mouse_relative_y = event->motion.yrel;
			mouse_x += mouse_relative_x;
			mouse_y += mouse_relative_y;
		} /* if */
		else
		{
			mouse_relative_x = event->motion.x - mouse_x;
			mouse_relative_y = event->motion.y - mouse_y;
			mouse_x = event->motion.x;
			mouse_y = event->motion.y;
		} /* else */
	} /* else */

	return (0);
} /* sdl_mouse_motion_filter */

/*
 * The windib driver can't alert us to the keypad enter key, which
 *  Ken's code depends on heavily. It sends it as the same key as the
 *  regular return key. These users will have to hit SHIFT-ENTER,
 *  which we check for explicitly, and give the engine a keypad enter
 *  enter event.
 */
static int handle_keypad_enter_hack(const SDL_Event *event)
{
	static int kp_enter_hack = 0;
	int retval = 0;

	if (event->key.keysym.scancode == SDL_SCANCODE_RETURN)
	{
		if (event->key.state == SDL_PRESSED)
		{
			if (event->key.keysym.mod & KMOD_SHIFT)
			{
				kp_enter_hack = 1;
				retval = GetScancode(SDL_SCANCODE_KP_ENTER);
			} /* if */
		} /* if */

		else /* key released */
		{
			if (kp_enter_hack)
			{
				kp_enter_hack = 0;
				retval = GetScancode(SDL_SCANCODE_KP_ENTER);
			} /* if */
		} /* if */
	} /* if */

	return (retval);
} /* handle_keypad_enter_hack */

static int sdl_key_filter(const SDL_Event *event)
{
	int k;
	int keyon;
	int strippedkey;
	int grab_mode = 0;
	int extended;

	if ((event->key.keysym.sym == SDLK_g) &&
		(event->key.state == SDL_PRESSED) &&
		(event->key.keysym.mod & KMOD_CTRL))
	{
		if (!sdl_fullscreen)
		{
			sdl_mouse_grabbed = ((sdl_mouse_grabbed) ? 0 : 1);
			if (sdl_mouse_grabbed)
				grab_mode = 1;
			SetShowCursor(!grab_mode);
		}
		return (0);
	} /* if */

	else if (((event->key.keysym.sym == SDLK_RETURN) ||
			  (event->key.keysym.sym == SDLK_KP_ENTER)) &&
			 (event->key.state == SDL_PRESSED) &&
			 (event->key.keysym.mod & KMOD_ALT))
	{
		ToggleFullScreen();
		return (0);
	} /* if */

	/* HDG: put this above the scancode lookup otherwise it is never reached */
	if ((event->key.keysym.sym == SDLK_PAUSE) &&
		(event->key.state == SDL_PRESSED))
	{
		PausePressed = true;
		return (0);
	}

	k = handle_keypad_enter_hack(event);
	if (!k)
	{
		k = GetScancode(event->key.keysym.scancode);
		if (!k) /* No DOS equivalent defined. */
			return (0);
	} /* if */

	/* Fix elweirdo SDL capslock/numlock handling, always treat as press */
	if ((event->key.keysym.sym != SDLK_CAPSLOCK) &&
		(event->key.keysym.sym != SDLK_NUMLOCKCLEAR) &&
		(event->key.state == SDL_RELEASED))
		k += 128; /* +128 signifies that the key is released in DOS. */

	if (event->key.keysym.sym == SDLK_SCROLLLOCK)
		PanicPressed = true;

	else
	{
		extended = ((k & 0xFF00) >> 8);

		keyon = k & 0x80;
		strippedkey = k & 0x7f;

		if (extended != 0)
		{
			KeyboardQueue[Keytail] = extended;
			Keytail = (Keytail + 1) & (KEYQMAX - 1);
			k = GetScancode(event->key.keysym.scancode) & 0xFF;
			if (event->key.state == SDL_RELEASED)
				k += 128; /* +128 signifies that the key is released in DOS. */
		}

		if (keyon) // Up event
			Keystate[strippedkey] = 0;
		else // Down event
		{
			Keystate[strippedkey] = 1;
			LastScan = k;
		}

		KeyboardQueue[Keytail] = k;
		Keytail = (Keytail + 1) & (KEYQMAX - 1);
	}

	return (0);
} /* sdl_key_filter */

static int sdl_joystick_button_filter(const SDL_Event *event)
{
	if (event->jbutton.which >= sdl_total_sticks)
		return 0;

	if (event->jbutton.button >= 16)
		return 0;

	if (event->type == SDL_JOYBUTTONDOWN)
	{
		sdl_stick_button_state[event->jbutton.which] |=
			(1 << event->jbutton.button);
	}
	else if (event->type == SDL_JOYBUTTONUP)
	{
		sdl_stick_button_state[event->jbutton.which] &=
			~(1 << event->jbutton.button);
	}

	return 0;
}

// map mouse wheel to key presses
static unsigned int mwheelsym = 0;
static inline void mwheelkey(int type)
{
	SDL_Event event;

	event.type = type;
	event.key.keysym.scancode = mwheelsym;
	event.key.state = SDL_RELEASED;
	SDL_PushEvent(&event);

	mwheelsym = 0;
}


/* MiSTer (patch_rottmister.py): the hybrid core's gamepad as keys. */
static SDL_Joystick *mh_joy = NULL;
static SDL_JoystickID mh_joy_id = -1;   /* its instance id: the events carry this, not the index */
static int mh_btn_key[32];          /* the scancode a held button pressed, released with it */
static int mh_hat_down[4];          /* up, down, left, right of the d-pad: the arrow keys (walk, turn; the menus) */
static int mh_stick_down[4];        /* the left stick: Up/Down arrows and comma/period (walk, strafe) */
static int mh_log;                  /* MISTER_PAD_LOG=1: every pad key to the log */
static int mh_trig_down[2];         /* L2, R2 */
static const int mh_hat_key[4]   = { 0x48, 0x50, 0x4b, 0x4d };   /* Up, Down, Left, Right */
static const int mh_stick_key[4] = { 0x48, 0x50, 0x33, 0x34 };   /* Up, Down, comma, period */
#define MH_STICK_ON  12000
#define MH_STICK_OFF 8000
#define MH_TRIG_ON   16000

static void mh_push(int k)
{
    KeyboardQueue[ Keytail ] = k;
    Keytail = ( Keytail + 1 )&( KEYQMAX - 1 );
    Keystate[k & 0x7f] = !(k & 0x80);
    if (!(k & 0x80)) LastScan = k;      /* the title, "press any key" and the menus wait on LastScan */
    if (mh_log) fprintf(stderr, "mister: pad key %02x%s\n", k & 0x7f, (k & 0x80) ? " up" : "");
}

static int mh_button_key(int b)
{
    switch (b)
    {
    case 0: return 0x1d;            /* Fire: Ctrl */
    case 1: return 0x39;            /* Open: Space */
    case 2: return 0x1c;            /* Jump: Enter = swap weapon */
    case 3: return 0x0e;            /* Crouch: Backspace = turn around */
    case 4: {                       /* Next Weapon: the number key of the next one */
        int w = locplayerstate ? locplayerstate->weapon : 0;
        if (w < 0 || w > 2) return 0x02;      /* a missile weapon or none: pistol */
        return 0x03 + w;                      /* pistol -> dual, dual -> MP40, MP40 -> missile */
    }
    case 5: return 0x36;            /* Use Item: Right Shift = run */
    case 6: return 0x0f;            /* Map: Tab */
    case 7: return 0x01;            /* Menu: Esc */
    case 28: return 0x1c;           /* Menu OK: Enter */
    case 29: return 0x01;           /* Menu Back: Esc */
    }
    return 0;
}

static void mh_pad_init(void)
{
    if (mh_joy) return;
    if (!SDL_WasInit(SDL_INIT_JOYSTICK)) { SDL_InitSubSystem(SDL_INIT_JOYSTICK); mh_log = getenv("MISTER_PAD_LOG") != NULL; }
    if (SDL_NumJoysticks() > 0) mh_joy = SDL_JoystickOpen(0);   /* the hybrid SDL's virtual pad: there once the video is up */
    if (!mh_joy) return;
    mh_joy_id = SDL_JoystickInstanceID(mh_joy);
    SDL_JoystickEventState(SDL_ENABLE);
    fprintf(stderr, "mister: pad open: %d buttons, %d axes, %d hats, instance %d\n", SDL_JoystickNumButtons(mh_joy), SDL_JoystickNumAxes(mh_joy), SDL_JoystickNumHats(mh_joy), (int)mh_joy_id);
}

static void mh_dir(int *state, const int *key, int i, int down)
{
    if (down == state[i]) return;
    state[i] = down;
    mh_push(down ? key[i] : (key[i] | 0x80));
}

static void mh_pad_release(void)
{
    int i;
    for (i = 0; i < 32; i++) if (mh_btn_key[i]) { mh_push(mh_btn_key[i] | 0x80); mh_btn_key[i] = 0; }
    for (i = 0; i < 4; i++) { mh_dir(mh_hat_down, mh_hat_key, i, 0); mh_dir(mh_stick_down, mh_stick_key, i, 0); }
    for (i = 0; i < 2; i++) if (mh_trig_down[i]) { mh_push((i ? 0x1d : 0x36) | 0x80); mh_trig_down[i] = 0; }
}

static int mh_pad_event(const SDL_Event *e)
{
    if (mh_log && e->type >= SDL_JOYAXISMOTION && e->type <= SDL_JOYDEVICEREMOVED)
        fprintf(stderr, "mister: pad event %x which %d (pad %d)\n", e->type, (int)e->jbutton.which, (int)mh_joy_id);
    switch (e->type)
    {
    case SDL_JOYDEVICEADDED:
        mh_pad_init();
        return 1;
    case SDL_JOYDEVICEREMOVED:
        if (mh_joy && e->jdevice.which == mh_joy_id)
        {
            mh_pad_release();
            SDL_JoystickClose(mh_joy); mh_joy = NULL; mh_joy_id = -1;
            fprintf(stderr, "mister: pad detached (a video restart), reopened when it is back\n");
        }
        return 1;
    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP: {
        int b = e->jbutton.button;
        if (!mh_joy || e->jbutton.which != mh_joy_id || b < 0 || b >= 32) return 0;
        if (e->type == SDL_JOYBUTTONDOWN)
        {
            int k = mh_button_key(b);
            if (!k || mh_btn_key[b]) return 0;
            mh_btn_key[b] = k; mh_push(k);
        }
        else if (mh_btn_key[b])
        {
            mh_push(mh_btn_key[b] | 0x80); mh_btn_key[b] = 0;
        }
        return 1;
    }
    case SDL_JOYHATMOTION: {
        int v = e->jhat.value;
        if (!mh_joy || e->jhat.which != mh_joy_id || e->jhat.hat != 0) return 0;
        mh_dir(mh_hat_down, mh_hat_key, 0, (v & SDL_HAT_UP) != 0); mh_dir(mh_hat_down, mh_hat_key, 1, (v & SDL_HAT_DOWN) != 0);
        mh_dir(mh_hat_down, mh_hat_key, 2, (v & SDL_HAT_LEFT) != 0); mh_dir(mh_hat_down, mh_hat_key, 3, (v & SDL_HAT_RIGHT) != 0);
        return 1;
    }
    case SDL_JOYAXISMOTION: {
        int a = e->jaxis.axis, v = e->jaxis.value;
        if (!mh_joy || e->jaxis.which != mh_joy_id) return 0;
        if (a == 0 || a == 1)
        {
            int neg, pos;                        /* axis 0: left(2)/right(3); axis 1: up(0)/down(1) */
            if (a == 0) { neg = 2; pos = 3; } else { neg = 0; pos = 1; }
            if (v <= -MH_STICK_ON) mh_dir(mh_stick_down, mh_stick_key, neg, 1); else if (v > -MH_STICK_OFF) mh_dir(mh_stick_down, mh_stick_key, neg, 0);
            if (v >= MH_STICK_ON) mh_dir(mh_stick_down, mh_stick_key, pos, 1); else if (v < MH_STICK_OFF) mh_dir(mh_stick_down, mh_stick_key, pos, 0);
            return 1;
        }
        if (a == 4 || a == 5)
        {
            int t = a - 4, down = v >= MH_TRIG_ON, k = t ? 0x1d : 0x36;   /* R2 fire, L2 run */
            if (down != mh_trig_down[t]) { mh_trig_down[t] = down; mh_push(down ? k : (k | 0x80)); }
            return 1;
        }
        return 0;
    }
    }
    return 0;
}

/* the right stick as mouse motion, once per poll of the mouse */
static int mh_look_dx, mh_look_dy;
static void mh_pad_look(void)
{
    int x, y;
    if (!mh_joy) return;
    x = SDL_JoystickGetAxis(mh_joy, 2); y = SDL_JoystickGetAxis(mh_joy, 3);
    if (x > -MH_STICK_OFF && x < MH_STICK_OFF) x = 0;
    if (y > -MH_STICK_OFF && y < MH_STICK_OFF) y = 0;
    mh_look_dx += x / 1200;
    mh_look_dy += y / 2400;
}

static int root_sdl_event_filter(SDL_Event *event)
{
	if (mh_pad_event(event)) return 0;
	switch (event->type)
	{
		// map mouse wheel to fake F13/F14 key presses
		case SDL_MOUSEWHEEL: {
			if (event->wheel.y > 0)
			{
				mwheelsym = SDL_SCANCODE_F13;
			}
			else
			{
				mwheelsym = SDL_SCANCODE_F14;
			}
			// fake key press event
			event->key.keysym.scancode = mwheelsym;
			event->key.state = SDL_PRESSED;
		} // fall through
		case SDL_KEYUP:
		case SDL_KEYDOWN:
			return (sdl_key_filter(event));
		case SDL_JOYBALLMOTION:
		case SDL_MOUSEMOTION:
			return (sdl_mouse_motion_filter(event));
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
			return (sdl_mouse_button_filter(event));
		case SDL_JOYBUTTONUP:
		case SDL_JOYBUTTONDOWN:
			return (sdl_joystick_button_filter(event));
		case SDL_QUIT:
			/* !!! rcg TEMP */
			fprintf(stderr, "\n\n\nSDL_QUIT!\n\n\n");
			SDL_Quit();
			exit(42);
	} /* switch */

	return (1);
} /* root_sdl_event_filter */

static void sdl_handle_events(void)
{
	SDL_Event event;
	mh_pad_init();   /* MiSTer: the pad shows up with the video, after IN_Startup */
	while (SDL_PollEvent(&event))
		root_sdl_event_filter(&event);

	// fake key release event
	if (mwheelsym)
	{
		mwheelkey(SDL_KEYUP);
	}

} /* sdl_handle_events */

//******************************************************************************
//
// IN_PumpEvents () - Let platform process an event queue.
//
//******************************************************************************
void IN_PumpEvents(void)
{
	sdl_handle_events();
}

//******************************************************************************
//
// INL_GetMouseDelta () - Gets the amount that the mouse has moved from the
//                        mouse driver
//
//******************************************************************************

void INL_GetMouseDelta(int *x, int *y)
{
	IN_PumpEvents();

	SDL_GetRelativeMouseState(x, y);
	mh_pad_look();   /* MiSTer: the right stick */
	*x += mh_look_dx; *y += mh_look_dy;
	mh_look_dx = mh_look_dy = 0;
}

//******************************************************************************
//
// IN_GetMouseButtons () - Gets the status of the mouse buttons from the
//                         mouse driver
//
//******************************************************************************

unsigned short IN_GetMouseButtons(void)

{
	unsigned short buttons = 0;

	IN_PumpEvents();

	buttons = sdl_mouse_button_mask;

	// Used by menu routines that need to wait for a button release.
	// Sometimes the mouse driver misses an interrupt, so you can't wait for
	// a button to be released.  Instead, you must ignore any buttons that
	// are pressed.

	IgnoreMouse &= buttons;
	buttons &= ~IgnoreMouse;

	return (buttons);
}

//******************************************************************************
//
// IN_IgnoreMouseButtons () -
//    Tells the mouse to ignore the currently pressed buttons.
//
//******************************************************************************

void IN_IgnoreMouseButtons(void)

{
	IgnoreMouse |= IN_GetMouseButtons();
}

//******************************************************************************
//
// IN_GetJoyAbs () - Reads the absolute position of the specified joystick
//
//******************************************************************************

void IN_GetJoyAbs(unsigned short joy, unsigned short *xp, unsigned short *yp)
{
	Joy_x = Joy_y = 0;
	Joy_xs = joy ? 2 : 0; // Find shift value for x axis
	Joy_xb = 1 << Joy_xs; // Use shift value to get x bit mask
	Joy_ys = joy ? 3 : 1; // Do the same for y axis
	Joy_yb = 1 << Joy_ys;

	if (joy < sdl_total_sticks)
	{
		Sint32 jx, jy;

		jx = (Sint32)SDL_JoystickGetAxis(sdl_joysticks[joy], 0) +
			 SDL_JOYSTICK_AXIS_MAX;
		jy = (Sint32)SDL_JoystickGetAxis(sdl_joysticks[joy], 1) +
			 SDL_JOYSTICK_AXIS_MAX;

		jx = ((float)jx / 65536.0f) * MaxJoyValue;
		jy = ((float)jy / 65536.0f) * MaxJoyValue;

		Joy_x = (unsigned short)jx;
		Joy_y = (unsigned short)jy;
	}
	else
	{
		Joy_x = 0;
		Joy_y = 0;
	}

	*xp = Joy_x;
	*yp = Joy_y;
}

//******************************************************************************
//
// INL_GetJoyDelta () - Returns the relative movement of the specified
//                     joystick (from +/-127)
//
//******************************************************************************

void INL_GetJoyDelta(unsigned short joy, int *dx, int *dy)
{
	unsigned short x, y;
	JoystickDef *def;

	IN_GetJoyAbs(joy, &x, &y);
	def = JoyDefs + joy;

	if (x < def->threshMinX)
	{
		if (x < def->joyMinX)
			x = def->joyMinX;

		x = -(x - def->threshMinX);
		x *= def->joyMultXL;
		x >>= JoyScaleShift;
		*dx = (x > 127) ? -127 : -x;
	}
	else if (x > def->threshMaxX)
	{
		if (x > def->joyMaxX)
			x = def->joyMaxX;

		x = x - def->threshMaxX;
		x *= def->joyMultXH;
		x >>= JoyScaleShift;
		*dx = (x > 127) ? 127 : x;
	}
	else
		*dx = 0;

	if (y < def->threshMinY)
	{
		if (y < def->joyMinY)
			y = def->joyMinY;

		y = -(y - def->threshMinY);
		y *= def->joyMultYL;
		y >>= JoyScaleShift;
		*dy = (y > 127) ? -127 : -y;
	}
	else if (y > def->threshMaxY)
	{
		if (y > def->joyMaxY)
			y = def->joyMaxY;

		y = y - def->threshMaxY;
		y *= def->joyMultYH;
		y >>= JoyScaleShift;
		*dy = (y > 127) ? 127 : y;
	}
	else
		*dy = 0;
}

//******************************************************************************
//
// INL_GetJoyButtons () - Returns the button status of the specified
//                        joystick
//
//******************************************************************************

unsigned short INL_GetJoyButtons(unsigned short joy)
{
	unsigned short result = 0;

	if (joy < sdl_total_sticks)
		result = sdl_stick_button_state[joy];

	return result;
}

//******************************************************************************
//
// INL_StartMouse () - Detects and sets up the mouse
//
//******************************************************************************

boolean INL_StartMouse(void)
{

	boolean retval = false;

	/* no-op. */
	retval = true;

	return (retval);
}

//******************************************************************************
//
// INL_SetJoyScale () - Sets up scaling values for the specified joystick
//
//******************************************************************************

void INL_SetJoyScale(unsigned short joy)
{
	JoystickDef *def;

	def = &JoyDefs[joy];
	def->joyMultXL = JoyScaleMax / (def->threshMinX - def->joyMinX);
	def->joyMultXH = JoyScaleMax / (def->joyMaxX - def->threshMaxX);
	def->joyMultYL = JoyScaleMax / (def->threshMinY - def->joyMinY);
	def->joyMultYH = JoyScaleMax / (def->joyMaxY - def->threshMaxY);
}

//******************************************************************************
//
// IN_SetupJoy () - Sets up thresholding values and calls INL_SetJoyScale()
//                  to set up scaling values
//
//******************************************************************************

void IN_SetupJoy(unsigned short joy, unsigned short minx, unsigned short maxx,
				 unsigned short miny, unsigned short maxy)
{
	unsigned short d, r;
	JoystickDef *def;

	def = &JoyDefs[joy];

	def->joyMinX = minx;
	def->joyMaxX = maxx;
	r = maxx - minx;
	d = r / 3;
	def->threshMinX = ((r / 2) - d) + minx;
	def->threshMaxX = ((r / 2) + d) + minx;

	def->joyMinY = miny;
	def->joyMaxY = maxy;
	r = maxy - miny;
	d = r / 3;
	def->threshMinY = ((r / 2) - d) + miny;
	def->threshMaxY = ((r / 2) + d) + miny;

	INL_SetJoyScale(joy);
}

//******************************************************************************
//
// INL_StartJoy () - Detects & auto-configures the specified joystick
//                   The auto-config assumes the joystick is centered
//
//******************************************************************************

boolean INL_StartJoy(unsigned short joy)
{
	unsigned short x, y;

	if (!SDL_WasInit(SDL_INIT_JOYSTICK))
	{
		SDL_Init(SDL_INIT_JOYSTICK);
		sdl_total_sticks = SDL_NumJoysticks();
		if (sdl_total_sticks > MaxJoys)
			sdl_total_sticks = MaxJoys;
		SDL_JoystickEventState(SDL_ENABLE);
	}

	if (joy >= sdl_total_sticks)
		return (false);
	sdl_joysticks[joy] = SDL_JoystickOpen(joy);

	IN_GetJoyAbs(joy, &x, &y);

	if (((x == 0) || (x > MaxJoyValue - 10)) ||
		((y == 0) || (y > MaxJoyValue - 10)))
		return (false);
	else
	{
		IN_SetupJoy(joy, 0, x * 2, 0, y * 2);
		return (true);
	}
}

//******************************************************************************
//
// INL_ShutJoy() - Cleans up the joystick stuff
//
//******************************************************************************

void INL_ShutJoy(unsigned short joy)
{
	JoysPresent[joy] = false;
	if (joy < sdl_total_sticks)
		SDL_JoystickClose(sdl_joysticks[joy]);
}

//******************************************************************************
//
// IN_Startup() - Starts up the Input Mgr
//
//******************************************************************************

void IN_Startup(void)
{
	boolean checkjoys, checkmouse;

	unsigned short i;

	if (IN_Started == true)
		return;

#ifdef _WIN32
	// fixme: remove this.
	sdl_mouse_grabbed = 1;
#endif

	checkjoys = true;
	checkmouse = true;

	for (i = 1; i < _argc; i++)
	{
		switch (US_CheckParm(_argv[i], ParmStrings))
		{
			case 0:
				checkjoys = false;
				break;

			case 1:
				checkmouse = false;
				break;
		}
	}

	MousePresent = checkmouse ? INL_StartMouse() : false;

	if (!MousePresent)
		mouseenabled = false;
	else
	{
		if (!quiet)
			printf("IN_Startup: Mouse Present\n");
	}

	for (i = 0; i < MaxJoys; i++)
	{
		JoysPresent[i] = checkjoys ? INL_StartJoy(i) : false;
		if (INL_StartJoy(i))
		{
			if (!quiet)
				printf("IN_Startup: Joystick Present\n");
		}
	}

	mh_pad_init();
	IN_Started = true;
}

//******************************************************************************
//
// IN_Shutdown() - Shuts down the Input Mgr
//
//******************************************************************************

void IN_Shutdown(void)
{
	unsigned short i;

	if (IN_Started == false)
		return;

	//   INL_ShutMouse();

	for (i = 0; i < MaxJoys; i++)
		INL_ShutJoy(i);

	IN_Started = false;
}

//******************************************************************************
//
// IN_ClearKeysDown() - Clears the keyboard array
//
//******************************************************************************

void IN_ClearKeysDown(void)
{
	LastScan = sc_None;
	memset((void *)Keyboard, 0, sizeof(Keyboard));
}

//******************************************************************************
//
// IN_ReadControl() - Reads the device associated with the specified
//    player and fills in the control info struct
//
//******************************************************************************

void IN_ReadControl(int player, ControlInfo *info)
{
	boolean realdelta = false;
	unsigned short buttons;
	int dx, dy;
	Motion mx, my;
	ControlType type;

	KeyboardDef *def;

	dx = dy = 0;
	mx = my = motion_None;
	buttons = 0;

	switch (type = Controls[player])
	{
		case ctrl_Keyboard:
			def = &KbdDefs;

			if (Keyboard[sc_UpArrow])
				my = motion_Up;
			else if (Keyboard[sc_DownArrow])
				my = motion_Down;

			if (Keyboard[sc_LeftArrow])
				mx = motion_Left;
			else if (Keyboard[sc_RightArrow])
				mx = motion_Right;

			if (Keyboard[def->button0])
				buttons += 1 << 0;
			if (Keyboard[def->button1])
				buttons += 1 << 1;
			realdelta = false;
			break;

		default:;
	}

	if (realdelta)
	{
		mx = (dx < 0) ? motion_Left : ((dx > 0) ? motion_Right : motion_None);
		my = (dy < 0) ? motion_Up : ((dy > 0) ? motion_Down : motion_None);
	}
	else
	{
		dx = mx * 127;
		dy = my * 127;
	}

	info->x = dx;
	info->xaxis = mx;
	info->y = dy;
	info->yaxis = my;
	info->button0 = buttons & (1 << 0);
	info->button1 = buttons & (1 << 1);
	info->button2 = buttons & (1 << 2);
	info->button3 = buttons & (1 << 3);
	info->dir = DirTable[((my + 1) * 3) + (mx + 1)];
}

//******************************************************************************
//
// IN_WaitForKey() - Waits for a scan code, then clears LastScan and
//    returns the scan code
//
//******************************************************************************

ScanCode IN_WaitForKey(void)
{
	ScanCode result;

	while (!(result = LastScan))
		IN_PumpEvents();
	LastScan = 0;
	return (result);
}

//******************************************************************************
//
// IN_Ack() - waits for a button or key press.  If a button is down, upon
// calling, it must be released for it to be recognized
//
//******************************************************************************

boolean btnstate[8];

void IN_StartAck(void)
{
	unsigned int i, buttons = 0;

	//
	// get initial state of everything
	//
	LastScan = 0;

	IN_ClearKeysDown();
	memset(btnstate, 0, sizeof(btnstate));

	IN_PumpEvents();

	buttons = IN_JoyButtons() << 4;

	buttons |= IN_GetMouseButtons();

	for (i = 0; i < 8; i++, buttons >>= 1)
		if (buttons & 1)
			btnstate[i] = true;
}

//******************************************************************************
//
// IN_CheckAck ()
//
//******************************************************************************

boolean IN_CheckAck(void)
{
	unsigned int i, buttons = 0;

	//
	// see if something has been pressed
	//
	if (LastScan)
		return true;

	IN_PumpEvents();

	buttons = IN_JoyButtons() << 4;

	buttons |= IN_GetMouseButtons();

	for (i = 0; i < 8; i++, buttons >>= 1)
		if (buttons & 1)
		{
			if (!btnstate[i])
				return true;
		}
		else
			btnstate[i] = false;

	return false;
}

//******************************************************************************
//
// IN_Ack ()
//
//******************************************************************************

void IN_Ack(void)
{
	IN_StartAck();

	while (!IN_CheckAck())
		;
}

//******************************************************************************
//
// IN_UserInput() - Waits for the specified delay time (in ticks) or the
//    user pressing a key or a mouse button. If the clear flag is set, it
//    then either clears the key or waits for the user to let the mouse
//    button up.
//
//******************************************************************************

boolean IN_UserInput(long delay)
{
	long lasttime;

	lasttime = GetTicCount();

	IN_StartAck();
	do
	{
		if (IN_CheckAck())
			return true;
	} while ((GetTicCount() - lasttime) < delay);

	return (false);
}

//===========================================================================

/*
===================
=
= IN_JoyButtons
=
===================
*/

byte IN_JoyButtons(void)
{
	return (byte)sdl_stick_button_state[joystickport];
}

//******************************************************************************
//
// IN_UpdateKeyboard ()
//
//******************************************************************************

/* HACK HACK HACK */
static int queuegotit = 0;

void IN_UpdateKeyboard(void)
{
	int tail;
	int key;

	if (!queuegotit)
		IN_PumpEvents();

	queuegotit = 0;

	if (Keytail != Keyhead)
	{
		tail = Keytail;

		while (Keyhead != tail)
		{
			if (KeyboardQueue[Keyhead] & 0x80) // Up event
			{
				key = KeyboardQueue[Keyhead] & 0x7F; // AND off high bit

				//            if (keysdown[key])
				//            {
				//               KeyboardQueue[Keytail] =
				//               KeyboardQueue[Keyhead]; Keytail =
				//               (Keytail+1)&(KEYQMAX-1);
				//            }
				//            else
				Keyboard[key] = 0;
			}
			else // Down event
			{
				Keyboard[KeyboardQueue[Keyhead]] = 1;
				//            keysdown[KeyboardQueue[Keyhead]] = 1;
			}

			Keyhead = (Keyhead + 1) & (KEYQMAX - 1);

		} // while
	} // if

	// Carry over movement keys from the last refresh
	//   keysdown[sc_RightArrow] = Keyboard[sc_RightArrow];
	//   keysdown[sc_LeftArrow]  = Keyboard[sc_LeftArrow];
	//   keysdown[sc_UpArrow]    = Keyboard[sc_UpArrow];
	//   keysdown[sc_DownArrow]  = Keyboard[sc_DownArrow];
}

//******************************************************************************
//
// IN_InputUpdateKeyboard ()
//
//******************************************************************************

int IN_InputUpdateKeyboard(void)
{
	int key;
	int returnval = 0;
	boolean done = false;

	//   _disable ();

	if (Keytail != Keyhead)
	{
		int tail = Keytail;

		while (!done && (Keyhead != tail))
		{
			if (KeyboardQueue[Keyhead] & 0x80) // Up event
			{
				key = KeyboardQueue[Keyhead] & 0x7F; // AND off high bit

				Keyboard[key] = 0;
			}
			else // Down event
			{
				Keyboard[KeyboardQueue[Keyhead]] = 1;
				returnval = KeyboardQueue[Keyhead];
				done = true;
			}

			Keyhead = (Keyhead + 1) & (KEYQMAX - 1);
		}
	} // if

	//   _enable ();

	return (returnval);
}

//******************************************************************************
//
// IN_ClearKeyboardQueue ()
//
//******************************************************************************

void IN_ClearKeyboardQueue(void)
{
	return;

	//   IN_ClearKeysDown ();

	//   Keytail = Keyhead = 0;
	//   memset (KeyboardQueue, 0, sizeof (KeyboardQueue));
	//   I_SendKeyboardData(0xf6);
	//   I_SendKeyboardData(0xf4);
}

//******************************************************************************
//
// QueueLetterInput ()
//
//******************************************************************************

void QueueLetterInput(void)
{
	int head = Keyhead;
	int tail = Keytail;
	char c;
	int scancode;
	boolean send = false;

	/* HACK HACK HACK */
	/*
	  OK, we want the new keys NOW, and not when the update gets them.
	  Since this called before IN_UpdateKeyboard in PollKeyboardButtons,
	  we shall update here.  The hack is there to prevent IN_UpdateKeyboard
	  from stealing any keys... - SBF
	 */
	IN_PumpEvents();
	head = Keyhead;
	tail = Keytail;
	queuegotit = 1;
	/* HACK HACK HACK */

	while (head != tail)
	{
		if (!(KeyboardQueue[head] & 0x80)) // Down event
		{
			scancode = KeyboardQueue[head];

			if (Keyboard[sc_RShift] || Keyboard[sc_LShift])
			{
				c = ShiftedScanChars[scancode];
			}
			else
			{
				c = ScanChars[scancode];
			}

			// If "is printable char", queue the character
			if (c)
			{
				LetterQueue[LastLetter] = c;
				LastLetter = (LastLetter + 1) & (MAXLETTERS - 1);

				// If typing a message, update the text with 'c'

				if (MSG.messageon)
				{
					Keystate[scancode] = 0;
					KeyboardQueue[head] = 0;
					if (MSG.inmenu)
					{
						if ((c == 'A') || (c == 'a'))
						{
							MSG.towho = MSG_DIRECTED_TO_ALL;
							send = true;
						}

						if ((gamestate.teamplay) && ((c == 'T') || (c == 't')))
						{
							MSG.towho = MSG_DIRECTED_TO_TEAM;
							send = true;
						}

						if ((c >= '0') && (c <= '9'))
						{
							int who;

							if (c == '0')
							{
								who = 10;
							}
							else
							{
								who = c - '1';
							}

							// Skip over local player
							if (who >= consoleplayer)
							{
								who++;
							}

							if (who < numplayers)
							{
								MSG.towho = who;
								send = true;
							}
						}

						if (send)
						{
							MSG.messageon = false;
							KeyboardQueue[head] = 0;
							Keyboard[scancode] = 0;
							LastScan = 0;
							FinishModemMessage(MSG.textnum, true);
						}
					}
					else if ((scancode >= sc_1) && (scancode <= sc_0) &&
							 (Keyboard[sc_Alt]))
					{
						int msg;

						msg = scancode - sc_1;

						if (CommbatMacros[msg].avail)
						{
							MSG.length = strlen(CommbatMacros[msg].macro) + 1;
							strcpy(Messages[MSG.textnum].text,
								   CommbatMacros[msg].macro);

							MSG.messageon = false;
							FinishModemMessage(MSG.textnum, true);
							KeyboardQueue[head] = 0;
							Keyboard[sc_Enter] = 0;
							Keyboard[sc_Escape] = 0;
							LastScan = 0;
						}
						else
						{
							MSG.messageon = false;
							MSG.directed = false;

							FinishModemMessage(MSG.textnum, false);
							AddMessage("No macro.", MSG_MACRO);
							KeyboardQueue[head] = 0;
							Keyboard[sc_Enter] = 0;
							Keyboard[sc_Escape] = 0;
							LastScan = 0;
						}
					}
					else if (MSG.length < MAXMESSAGELENGTH)
					{
						UpdateModemMessage(MSG.textnum, c);
					}
				}
			}
			else
			{
				// If typing a message, check for special characters

				if (MSG.messageon && MSG.inmenu)
				{
					if (scancode == sc_Escape)
					{
						MSG.messageon = false;
						MSG.directed = false;
						FinishModemMessage(MSG.textnum, false);
						KeyboardQueue[head] = 0;
						Keyboard[sc_Enter] = 0;
						Keyboard[sc_Escape] = 0;
						LastScan = 0;
					}
				}
				else if (MSG.messageon && !MSG.inmenu)
				{
					if ((scancode >= sc_F1) && (scancode <= sc_F10))
					{
						MSG.remoteridicule = scancode - sc_F1;
						MSG.messageon = false;
						FinishModemMessage(MSG.textnum, true);
						KeyboardQueue[head] = 0;
						Keyboard[sc_Enter] = 0;
						Keyboard[sc_Escape] = 0;
						LastScan = 0;
					}

					switch (scancode)
					{
						case sc_BackSpace:
							KeyboardQueue[head] = 0;
							if (MSG.length > 1)
							{
								ModemMessageDeleteChar(MSG.textnum);
							}
							Keystate[scancode] = 0;
							break;

						case sc_Enter:
							MSG.messageon = false;
							FinishModemMessage(MSG.textnum, true);
							KeyboardQueue[head] = 0;
							Keyboard[sc_Enter] = 0;
							Keyboard[sc_Escape] = 0;
							LastScan = 0;
							Keystate[scancode] = 0;
							break;

						case sc_Escape:
							MSG.messageon = false;
							MSG.directed = false;
							FinishModemMessage(MSG.textnum, false);
							KeyboardQueue[head] = 0;
							Keyboard[sc_Enter] = 0;
							Keyboard[sc_Escape] = 0;
							LastScan = 0;
							break;
					}
				}
			}
		}

		head = (head + 1) & (KEYQMAX - 1);
	} // while
}
