
#include "stdafx.h"
#include <SDL.h>
#include "BkBtl.h"
#include "Emulator.h"

/////////////////////////////////////////////////////////////////////////////
// Globals

SDL_Surface*    g_Screen = NULL;
SDL_Surface*    g_BKScreen = NULL;
int             g_BKScreenWid, g_BKScreenHei;
ScreenMode      g_ScreenMode = ScreenModeNone;
int             g_okQuit = FALSE;
int             g_LastDelay = 0;        //DEBUG: Last delay value, milliseconds
int             g_LastFps = 0;          //DEBUG: Last Frames-per-Second value

#define FRAME_TICKS             40      // 1000 us / 40 = 25 frames per second
#define DEFAULT_BK_CONF         BK_CONF_BK0010_FDD

struct KeyMappingStruct
{
    int             source;             // Source key - SDLK_Xxx
    int             isjoystick;         // 0 - keyboard, 1 - joystick
    unsigned int    result;             // Resulting key scan or joystick bits
};

static KeyMappingStruct g_KeyMapping[] = {
    { SDLK_LEFT,    1,  0x10 },
    { SDLK_RIGHT,   1,  0x40 },
    { SDLK_UP,      1,  0x80 },
    { SDLK_DOWN,    1,  0x20 },
    { SDLK_LCTRL,   1,  0x01 },     // A button on Dingoo
    { SDLK_RETURN,  0,  012 },      // START button on Dingoo
    { SDLK_SPACE,   0,  040 },      // X button on Dingoo
};


/////////////////////////////////////////////////////////////////////////////


void Main_ClearScreen()
{
    SDL_Rect rc;
    rc.x = rc.y = 0;  rc.w = SCREEN_WIDTH; rc.h = SCREEN_HEIGHT;
    SDL_FillRect(g_Screen, &rc, 0);
}

void Main_SetScreenMode(ScreenMode screenMode)
{
    if (g_ScreenMode == screenMode)
        return;

    if (g_ScreenMode != ScreenModeNone)
    {
        SDL_FreeSurface(g_BKScreen);
    }

    g_ScreenMode = screenMode;

    if (screenMode != ScreenModeNone)
    {
        Emulator_GetScreenModeSize(screenMode, &g_BKScreenWid, &g_BKScreenHei);
        g_BKScreen = SDL_CreateRGBSurface(0, g_BKScreenWid, g_BKScreenHei, 32, 0,0,0,0);
    }
}

int Main_LoadBin(const char* fileName)
{
    FILE* file = fopen(fileName, "rb");
    if (file == NULL)
        return FALSE;

    // Load BIN header
    unsigned short bufHeader[2];
    fread(bufHeader, 1, 4, file);

    unsigned short baseAddress = bufHeader[0];
    unsigned short dataSize = bufHeader[1];
    unsigned short memoryBytes = (dataSize + 1) & 0xfffe;

    // Load file data
    void* pbuffer = malloc(memoryBytes);
    memset(pbuffer, 0, memoryBytes);
    fread(pbuffer, 1, dataSize, file);

    // Copy data to BK memory
    unsigned short* pdata = (unsigned short*)pbuffer;
    unsigned short address = baseAddress;
    while (address < baseAddress + memoryBytes)
    {
        unsigned short value = *pdata;
        g_pBoard->SetRAMWord(address, value);
        pdata++;
        address += 2;
    }

    fclose(file);
    free(pbuffer);

    return TRUE;
}

void Main_OnKeyEvent(SDL_Event evt)
{
    if (evt.type == SDL_KEYDOWN)
    {
        switch (evt.key.keysym.sym)
        {
        case SDLK_PAUSE:  // POWER UP button on Dingoo
        case SDLK_ESCAPE:  // SELECT button on Dingoo
            g_okQuit = TRUE;
            return;
        case SDLK_TAB:  // Left shoulder on Dingoo
            Main_LoadBin("GAME.BIN");
            // Print "S1000"
            Emulator_KeyboardEvent(0123, TRUE);
            Emulator_KeyboardEvent(0123, FALSE);
            Emulator_KeyboardEvent(061, TRUE);
            Emulator_KeyboardEvent(061, FALSE);
            Emulator_KeyboardEvent(060, TRUE);
            Emulator_KeyboardEvent(060, FALSE);
            Emulator_KeyboardEvent(060, TRUE);
            Emulator_KeyboardEvent(060, FALSE);
            Emulator_KeyboardEvent(060, TRUE);
            Emulator_KeyboardEvent(060, FALSE);
            return;
        case SDLK_BACKSPACE:  // Right shoulder on Dingoo
            Main_ClearScreen();
            Main_SetScreenMode((g_ScreenMode == ScreenModeLast) ? (ScreenMode)0 : (ScreenMode)(g_ScreenMode + 1));
            return;
        default:
            break;
        }
    }

    for (int i = 0; i < sizeof(g_KeyMapping) / sizeof(KeyMappingStruct); i++)
    {
        if (g_KeyMapping[i].source == evt.key.keysym.sym)
        {
            int okPressed = (evt.type == SDL_KEYDOWN);
            BYTE result = g_KeyMapping[i].result;
            if (g_KeyMapping[i].isjoystick)
                Emulator_JoystickEvent(result, okPressed);
            else
                Emulator_KeyboardEvent(result, okPressed);
            return;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#undef main  //HACK for VC error LNK1561: entry point must be defined
#endif

int main()
{
#ifndef _WIN32
    SDL_putenv("DINGOO_IGNORE_OS_EVENTS=1");  //HACK to fix "push long time on X" problem
#endif

    // Init SDL video
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        return 255;  // Unable to initialize SDL

#ifdef _WIN32
    SDL_putenv("SDL_VIDEO_WINDOW_POS=300,200");
    SDL_WM_SetCaption("BKBTL SDL", "BKBTL SDL");
#else
    SDL_ShowCursor(SDL_DISABLE);
#endif

    // Prepare screen surface
    g_Screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BITPERPIXEL, 0);
    if (g_Screen == NULL)
        return 254;  // Unable to set video mode

    if (!Emulator_Init())
        return 255;
    if (!Emulator_InitConfiguration(DEFAULT_BK_CONF))
        return 255;

    Main_ClearScreen();
    Main_SetScreenMode(ScreenColor256x256);

    Emulator_Start();

    Uint32 ticksLast;
    Uint32 ticksLastFps = SDL_GetTicks();
    int frames = 0;
    while (!g_okQuit)
    {
        ticksLast = SDL_GetTicks();  // Time at frame start
        SDL_Event evt;
        while (SDL_PollEvent(&evt))
        {
            if (evt.type == SDL_QUIT)
            {
                g_okQuit = TRUE;
                break;
            }
            else
            {
                if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP)
                {
                    Main_OnKeyEvent(evt);
                }
            }
        }

        if (g_okEmulatorRunning)
        {
            Emulator_SystemFrame();

            SDL_LockSurface(g_BKScreen);
            void* pPixels = g_BKScreen->pixels;
            Emulator_PrepareScreen(pPixels, g_ScreenMode);
            SDL_UnlockSurface(g_BKScreen);
            
            // Draw BK screen
            SDL_Rect src, dst;
            src.x = src.y = dst.x = dst.y = 0;
            src.w = dst.w = SCREEN_WIDTH;
            src.h = dst.w = SCREEN_HEIGHT;
            if (g_BKScreenWid < SCREEN_WIDTH)
            {
                src.w = dst.w = g_BKScreenWid;
                dst.x = (SCREEN_WIDTH - g_BKScreenWid) / 2;
            }
            if (g_BKScreenHei < SCREEN_HEIGHT)
            {
                src.h = dst.h = g_BKScreenHei;
                dst.y = (SCREEN_HEIGHT - g_BKScreenHei) / 2;
            }
            SDL_BlitSurface(g_BKScreen, &src, g_Screen, &dst);
            
            SDL_Flip(g_Screen);
            
            frames++;
        }

        // Delay
        Uint32 ticksNew = SDL_GetTicks();
        Uint32 ticksElapsed = ticksNew - ticksLast;
        g_LastDelay = 0;
        if (ticksElapsed < FRAME_TICKS)
        {
            g_LastDelay = FRAME_TICKS - ticksElapsed;
            SDL_Delay(FRAME_TICKS - ticksElapsed);
        }
        ticksLast = ticksNew;
        
        if (ticksLast - ticksLastFps > 1000)  //DEBUG: FPS calculation
        {
            g_LastFps = frames;
            frames = 0;
            ticksLastFps += 1000;
        }
    }

    Emulator_Stop();
    Emulator_Done();

    Main_SetScreenMode(ScreenModeNone);

    // Free memory
    SDL_FreeSurface(g_Screen);

    SDL_Quit();

    return 0;
}
