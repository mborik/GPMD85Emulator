/*	globals.h: Global includes, constatns, enumerators and functions.
	Copyright (c) 2011-2019 Martin Borik <mborik@users.sourceforge.net>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
//-----------------------------------------------------------------------------
#ifndef GLOBALS_H_
#define GLOBALS_H_
//-----------------------------------------------------------------------------
#include "config.h"
//-----------------------------------------------------------------------------
#ifdef HAVE_STDIO_H
#  include <stdio.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h>
#endif
#ifdef STDC_HEADERS
#  include <stdlib.h>
#  include <stddef.h>
#elif HAVE_STDLIB_H
#  include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#  if !defined STDC_HEADERS && defined HAVE_MEMORY_H
#    include <memory.h>
#  endif
#  include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#  include <strings.h>
#endif
#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#if HAVE_DIRENT_H
#  include <dirent.h>
#  define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#  define dirent direct
#  define NAMLEN(dirent) (dirent)->d_namlen
#  if HAVE_SYS_NDIR_H
#    include <sys/ndir.h>
#  endif
#  if HAVE_SYS_DIR_H
#    include <sys/dir.h>
#  endif
#  if HAVE_NDIR_H
#    include <ndir.h>
#  endif
#endif
#ifdef HAVE_SYS_SYSLIMITS_H
#  include <sys/syslimits.h>
#endif
#ifdef HAVE_STDBOOL_H
#  include <stdbool.h>
#else
#  ifndef HAVE__BOOL
#    ifdef __cplusplus
typedef bool _Bool;
#    else
#      define _Bool signed char
#    endif
#  endif
#  define bool _Bool
#  define false 0
#  define true 1
#  define __bool_true_false_are_defined	1
#endif
//-----------------------------------------------------------------------------
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(__BORLANDC__)
#  define DIR_DELIMITER '\\'
#  define WIN32 1
#else
#  define DIR_DELIMITER '/'
#endif
//-----------------------------------------------------------------------------
#ifdef FILENAME_MAX
#  define MAX_PATH FILENAME_MAX
#else
#  define MAX_PATH 4096
#endif
//-----------------------------------------------------------------------------
#ifdef __GNUC__
#define UNUSED_VARIABLE __attribute__ ((unused))
#elif __LCLINT__
#define UNUSED_VARIABLE /\*@unused@*/
#else
#define UNUSED_VARIABLE
#endif
//-----------------------------------------------------------------------------
#define SIGSLOT_DEFAULT_MT_POLICY single_threaded
#include "sigslot.h"
//-----------------------------------------------------------------------------
#include "SDL.h"
#ifdef SDL_MAX_SINT32
#  define MAX_SIGNED_INT SDL_MAX_SINT32
#  define MIN_SIGNED_INT SDL_MIN_SINT32
#elif INT_MAX
#  define MAX_SIGNED_INT INT_MAX
#  define MIN_SIGNED_INT (-INT_MAX-1)
#else
#  define MAX_SIGNED_INT (0x7FFFFFFF)
#  define MIN_SIGNED_INT (0x80000000)
#endif
//-----------------------------------------------------------------------------
#define BYTE  uint8_t
#define WORD  uint16_t
#define DWORD uint32_t
#define QWORD uint64_t
//-----------------------------------------------------------------------------
#define strccnt(ptr, c, i) char *_strccnt_ptr = ptr; \
	while (*_strccnt_ptr != '\0') if (*_strccnt_ptr++ == c) i++;
//-----------------------------------------------------------------------------
#define CPU_FREQ            2048000
#define CPU_FRAMES_PER_SEC  50
#define CPU_TIMER_INTERVAL  (1000 / CPU_FRAMES_PER_SEC)
#define TCYCLES_PER_FRAME   (CPU_FREQ / CPU_FRAMES_PER_SEC)
#define GPU_FRAMES_PER_SEC  25
#define GPU_TIMER_INTERVAL  (1000 / GPU_FRAMES_PER_SEC)
#define MEASURE_PERIOD      1000

#define SAMPLE_RATE         44100
#define SAMPLE_TICK_INC     ((1 << 24) / SAMPLE_RATE)
#define FADEOUT_RATE        ((1 << 24) / 150)
#define SAMPS_PER_CPU_FRAME (SAMPLE_RATE / CPU_FRAMES_PER_SEC)
#define BYTES_PER_SAMPLE    2 // 8-bit stereo
#define AUDIO_BUFF_SIZE     1024
#define AUDIO_BEEP_CHANNELS 2
#define AUDIO_MAX_CHANNELS  3

#define CHNL_SPEAKER        0
#define CHNL_TAPE           1
#define CHNL_MIF85          2

#define LED_YELLOW          1
#define LED_RED             2
#define LED_BLUE            4

#define KM_SHIFT            0x200
#define KM_CTRL             0x400
#define KM_ALT              0x800

#define KB                  1024
#define CHUNK               128
#define TAPE_BLOCK_SIZE     65535
#define SNAP_BLOCK_LEN      16384
#define SNAP_HEADER_LEN     56
//-----------------------------------------------------------------------------
enum TComputerModel { CM_UNKNOWN = -1, CM_V1 = 0, CM_V2, CM_V2A, CM_V3, CM_MATO, CM_ALFA, CM_ALFA2, CM_C2717, CM_LAST = CM_C2717 };
enum TAutoStopType { AS_OFF = 0, AS_NEXTHEAD, AS_CURSOR };
enum TGpioPort { GP_GPIO_0 = 0, GP_GPIO_1, GP_GPIO_BOTH };
enum TJoyType { JT_NONE = -1, JT_KEYS = 0, JT_AXES, JT_POV, JT_BUTTONS };
enum TMouseType { MT_NONE = -1, MT_M602 = 0, MT_POLY8 };
enum TDisplayMode { DM_NORMAL = 1, DM_DOUBLESIZE, DM_TRIPLESIZE, DM_QUADRUPLESIZE, DM_QUINTUPLESIZE,  DM_FULLSCREEN = 0 };
enum TColorProfile { CP_MONO, CP_STANDARD, CP_COLOR, CP_COLORACE };
enum TColorPalette { CL_DEFINED, CL_RGB, CL_VIDEO };
enum THalfPassMode { HP_OFF, HP_75, HP_50, HP_25, HP_0 };
enum TColor { BLACK = 0, MAROON, GREEN, OLIVE, NAVY, PURPLE, TEAL, GRAY, SILVER, RED, LIME, YELLOW, BLUE, FUCHSIA, AQUA, WHITE };
enum TProcessSettingsMode { PS_MACHINE = 1, PS_SCREEN_SIZE = 2, PS_SCREEN_MODE = 4, PS_SOUND = 8, PS_CONTROLS = 16, PS_PERIPHERALS = 32, PS_CLOSEALL = 128 };
enum TMenuItemType { MENU_END = -1, MI_TITLE = 0, MI_STANDARD, MI_SUBMENU, MI_DIALOG, MI_VALUE, MI_CHECKBOX, MI_RADIO, MI_FIXED = 16, MI_SEPARATOR = 32 };
enum TMenuQueryType { GUI_QUERY_CANCEL = -1, GUI_QUERY_NONE = 0, GUI_QUERY_YES, GUI_QUERY_SAVE, GUI_QUERY_DONTSAVE };
enum TFileSelectType { GUI_FS_BASELOAD, GUI_FS_BASESAVE, GUI_FS_SNAPLOAD, GUI_FS_SNAPSAVE };
enum TDebugListType { DL_DUMP, DL_ASCII, DL_DISASM };
enum TDebugListSource { MEM, HL, DE, BC, AF, SP, PC };
//-----------------------------------------------------------------------------
// screen offsets of top-left -> bottom-right corner
typedef struct TDrawRegion { WORD tl, br; } TDrawRegion;
//-----------------------------------------------------------------------------
static char msgbuffer[1024];
//-----------------------------------------------------------------------------
inline void error(const char *ns, const char *msg, ...)
{
	if (ns) {
		if (strlen(ns))
			sprintf(msgbuffer + 1000, "[%s] ", ns);
		else
			msgbuffer[1000] = '\0';
	}

	va_list va;
	va_start(va, msg);
	vsprintf(msgbuffer, msg, va);
	va_end(va);
	fprintf(stderr, "ERR: %s%s!\n", msgbuffer + 1000, msgbuffer);

	SDL_Quit();
	exit(EXIT_FAILURE);
}
//-----------------------------------------------------------------------------
inline void warning(const char *ns, const char *msg, ...)
{
	if (ns) {
		if (strlen(ns))
			sprintf(msgbuffer + 1000, "[%s] ", ns);
		else
			msgbuffer[1000] = '\0';
	}

	va_list va;
	va_start(va, msg);
	vsprintf(msgbuffer, msg, va);
	va_end(va);
	fprintf(stderr, "WRN: %s%s!\n", msgbuffer + 1000, msgbuffer);
}
//-----------------------------------------------------------------------------
#if !defined NOTRACEMSG || defined DEBUG
inline void _debug(const char *ns, const char *msg, ...)
{
	if (ns) {
		if (strlen(ns))
			sprintf(msgbuffer + 1000, "[%s] ", ns);
		else
			msgbuffer[1000] = '\0';
	}

	va_list va;
	va_start(va, msg);
	vsprintf(msgbuffer, msg, va);
	va_end(va);
	fprintf(stdout, "DBG: %s%s\n", msgbuffer + 1000, msgbuffer);
	fflush(stdout);
}
#define debug(args...) _debug(args)
#else
#define debug(args...)
#endif
//-----------------------------------------------------------------------------
#endif
