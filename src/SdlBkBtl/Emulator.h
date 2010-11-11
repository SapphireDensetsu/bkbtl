// Emulator.h

#pragma once

#include "emubase/Board.h"

//////////////////////////////////////////////////////////////////////


enum ScreenMode {
    ScreenModeNone = -1,
    //
    ScreenColor256x256 = 0,
    ScreenBlackWhite256x256 = 1,
    ScreenBlackWhite512x256 = 2,
    //
    ScreenModeLast = 2,
};

extern CMotherboard* g_pBoard;
extern BOOL g_okEmulatorRunning;


BOOL Emulator_Init();
BOOL Emulator_InitConfiguration(BKConfiguration configuration);
void Emulator_GetScreenModeSize(ScreenMode screenMode, int* pwidth, int* pheight);
void Emulator_Done();
void Emulator_Start();
void Emulator_Stop();
void Emulator_Reset();

int  Emulator_SystemFrame();
void Emulator_PrepareScreen(void* pBits, ScreenMode screenMode);

void Emulator_KeyboardEvent(BYTE key, int okPressed);
void Emulator_JoystickEvent(unsigned short mask, int okPressed);


//////////////////////////////////////////////////////////////////////
