// BkBtl.h

#pragma once

#define VERSION_MAJOR           0
#define VERSION_MINOR           12

#define SCREEN_WIDTH            320
#define SCREEN_HEIGHT           240
#define SCREEN_BITPERPIXEL      16


//////////////////////////////////////////////////////////////////////
// Commands

#define ID_EXIT                 1001
#define ID_RESET                1002
#define ID_VIDEO_MODE           1003
#define ID_VIDEO_MODE_NEXT      1004
#define ID_VIDEO_MODE_PREV      1005
#define ID_LOAD_BIN             1006
#define ID_KEYBOARD             1007


//////////////////////////////////////////////////////////////////////
// Fonts

struct SDL_Surface;

void Fonts_Initialize();
void Fonts_Finalize();
void Font_DrawText(int x, int y, const char *str, SDL_Surface* surface);
void Font_DrawText(int x, int y, const char *str);


//////////////////////////////////////////////////////////////////////
