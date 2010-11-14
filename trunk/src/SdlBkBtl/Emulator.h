// Emulator.h

#pragma once

#include "emubase/Board.h"

//////////////////////////////////////////////////////////////////////

extern const int EMULATOR_SCREENMODE_COUNT;

extern CMotherboard* g_pBoard;
extern BOOL g_okEmulatorRunning;


BOOL Emulator_Init();
BOOL Emulator_InitConfiguration(BKConfiguration configuration);
void Emulator_GetScreenModeSize(int screenMode, int* pwidth, int* pheight);
void Emulator_Done();
void Emulator_Start();
void Emulator_Stop();
void Emulator_Reset();

int  Emulator_SystemFrame();
void Emulator_PrepareScreen(void* pBits, int screenMode);

void Emulator_KeyboardEvent(BYTE key, int okPressed);
void Emulator_JoystickEvent(unsigned short mask, int okPressed);


//////////////////////////////////////////////////////////////////////
