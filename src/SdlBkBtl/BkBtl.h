// BkBtl.h

#pragma once

#define VERSION_MAJOR           0
#define VERSION_MINOR           14

#if defined(PSP)
#  define SCREEN_WIDTH            480
#  define SCREEN_HEIGHT           272
#  define SCREEN_BITPERPIXEL      32
#elif defined(_DINGOO)
#  define SCREEN_WIDTH            320
#  define SCREEN_HEIGHT           240
#  define SCREEN_BITPERPIXEL      16
#else  // Win32
#  define SCREEN_WIDTH            480
#  define SCREEN_HEIGHT           272
#  define SCREEN_BITPERPIXEL      32
#endif


//////////////////////////////////////////////////////////////////////
// Commands

#define ID_EXIT                 1001
#define ID_RESET                1002
#define ID_VIDEO_MODE           1003
#define ID_VIDEO_MODE_NEXT      1004
#define ID_VIDEO_MODE_PREV      1005
#define ID_LOAD_BIN             1006
#define ID_KEYBOARD             1007
#define ID_CONFIGURATION        1008
#define ID_MENU                 1009
#define ID_MENU_ESCAPE          1010
#define ID_MENU_UP              1011
#define ID_MENU_DOWN            1012
#define ID_MENU_LEFT            1013
#define ID_MENU_RIGHT           1014
#define ID_MENU_SELECT          1015


//////////////////////////////////////////////////////////////////////
// Settings

void Settings_ParseIniFile(const char* sFileName);


//////////////////////////////////////////////////////////////////////
// Fonts

struct SDL_Surface;

void Fonts_Initialize();
void Fonts_Finalize();
void Font_DrawText(int x, int y, const char *str, SDL_Surface* surface);
void Font_DrawText(int x, int y, const char *str);


//////////////////////////////////////////////////////////////////////
