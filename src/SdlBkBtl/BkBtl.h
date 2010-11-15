// BkBtl.h

#pragma once

#define VERSION_MAJOR           0
#define VERSION_MINOR           11

#define SCREEN_WIDTH            320
#define SCREEN_HEIGHT           240
#define SCREEN_BITPERPIXEL      16


//////////////////////////////////////////////////////////////////////
// Fonts

void Fonts_Initialize();
void Fonts_Finalize();
void Font_DrawText(int x, int y, const char *str);


//////////////////////////////////////////////////////////////////////
