
#include "stdafx.h"
#include <SDL.h>
#include "BkBtl.h"
#include "Emulator.h"

/////////////////////////////////////////////////////////////////////////////
// Globals

char            g_AppDirectory[256];
char            g_AppIniPath[256];
SDL_Surface*    g_Screen = NULL;
SDL_Surface*    g_BKScreen = NULL;
SDL_Surface*    g_Keyboard = NULL;
SDL_Joystick*   g_Joystick = NULL;
int             g_BKScreenWid, g_BKScreenHei;
int             g_ScreenMode = -1;
int             g_okQuit = FALSE;
int             g_okKeyboard = FALSE;   // Onscreen keyboard on/off
int             g_KeyboardCurrent = 53; // Current onscreen keyboard key, default is Enter
int             g_LastDelay = 0;        //DEBUG: Last delay value, milliseconds
int             g_LastFps = 0;          //DEBUG: Last Frames-per-Second value

#define FRAME_TICKS             40      // 1000 us / 40 = 25 frames per second
#define DEFAULT_BK_CONF         BK_CONF_BK0010_BASIC

#define KEYBOARD_LEFT           22
#define KEYBOARD_TOP            120
#define KEYBOARD_WIDTH          276
#define KEYBOARD_HEIGHT         106

void Main_Menu();


enum EventType
{
    EVKEY = 1,
    EVJOY = 2,
    EVCMD = 3,
};

struct KeyMappingStruct
{
    int             sourcetype;         // Source: 0 - none, 1 - keyboard, 2 - joystick
    int             sourcecd;           // Source code: keyboard - SDLK_Xxx, joystick - button number
    int             resulttype;         // Result: 0 - none, 1 - BK keyboard, 2 - BK joystick, 3 - command
    unsigned int    resultcd;           // Result code: keyboard - key scan, joystick - bits, command - ID
};

static KeyMappingStruct g_KeyMapping[] = {
#if defined(PSP)
    { EVJOY, 7,             EVJOY,  0x10 },         // Left
    { EVJOY, 9,             EVJOY,  0x40 },         // Right
    { EVJOY, 8,             EVJOY,  0x80 },         // Up
    { EVJOY, 6,             EVJOY,  0x20 },         // Down
    { EVJOY, 11,            EVKEY,  012 },          // Start    ->  BK Enter
    { EVJOY, 4,             EVCMD,  ID_MENU },      // Left Trigger
    { EVJOY, 5,             EVCMD,  ID_KEYBOARD },  // Right Trigger
    { EVJOY, 8,             EVCMD,  ID_MENU_UP },   // Up
    { EVJOY, 6,             EVCMD,  ID_MENU_DOWN }, // Down
    { EVJOY, 7,             EVCMD,  ID_MENU_LEFT },  // Left
    { EVJOY, 9,             EVCMD,  ID_MENU_RIGHT }, // Right
    { EVJOY, 2,             EVCMD,  ID_MENU_SELECT }, // X
    { EVJOY, 1,             EVCMD,  ID_MENU_ESCAPE }, // O
#elif defined(_DINGOO)
    { EVKEY, SDLK_LEFT,     EVJOY,  0x10 },
    { EVKEY, SDLK_RIGHT,    EVJOY,  0x40 },
    { EVKEY, SDLK_UP,       EVJOY,  0x80 },
    { EVKEY, SDLK_DOWN,     EVJOY,  0x20 },
    { EVKEY, SDLK_LCTRL,    EVJOY,  0x01 },         // A button on Dingoo
    { EVKEY, SDLK_RETURN,   EVKEY,  012 },          // START button on Dingoo
    { EVKEY, SDLK_SPACE,    EVKEY,  040 },          // X button on Dingoo
    { EVKEY, SDLK_PAUSE,    EVCMD,  ID_EXIT },      // POWER UP on Dingoo
    { EVKEY, SDLK_BACKSPACE, EVCMD, ID_KEYBOARD },  // Right shoulder on Dingoo
    { EVKEY, SDLK_TAB,      EVCMD,  ID_MENU },      // Left shoulder on Dingoo
    { EVKEY, SDLK_UP,       EVCMD,  ID_MENU_UP },
    { EVKEY, SDLK_DOWN,     EVCMD,  ID_MENU_DOWN },
    { EVKEY, SDLK_LEFT,     EVCMD,  ID_MENU_LEFT },
    { EVKEY, SDLK_RIGHT,    EVCMD,  ID_MENU_RIGHT },
    { EVKEY, SDLK_RETURN,   EVCMD,  ID_MENU_SELECT }, // START button on Dingoo
#else  // Win32
    { EVKEY, SDLK_LEFT,     EVJOY,  0x10 },
    { EVKEY, SDLK_RIGHT,    EVJOY,  0x40 },
    { EVKEY, SDLK_UP,       EVJOY,  0x80 },
    { EVKEY, SDLK_DOWN,     EVJOY,  0x20 },
    { EVKEY, SDLK_RETURN,   EVKEY,  012 },
    { EVKEY, SDLK_SPACE,    EVKEY,  040 },
    { EVKEY, SDLK_F10,      EVCMD,  ID_EXIT },
    { EVKEY, SDLK_ESCAPE,   EVCMD,  ID_MENU_ESCAPE },
    { EVKEY, SDLK_TAB,      EVCMD,  ID_MENU },
    { EVKEY, SDLK_UP,       EVCMD,  ID_MENU_UP },
    { EVKEY, SDLK_DOWN,     EVCMD,  ID_MENU_DOWN },
    { EVKEY, SDLK_LEFT,     EVCMD,  ID_MENU_LEFT },
    { EVKEY, SDLK_RIGHT,    EVCMD,  ID_MENU_RIGHT },
    { EVKEY, SDLK_RETURN,   EVCMD,  ID_MENU_SELECT },
    { EVKEY, SDLK_BACKSPACE, EVCMD, ID_KEYBOARD },
#endif
};

// Search for suitable mapping.
//  sourcetype: 1 - keyboard, 2 - joystick
//  ismenu: 0 - search for keyboard/joystick result; 1 - search for menu result
KeyMappingStruct* FindKeyMapping(int sourcetype, int sourcecd, int ismenu)
{
    for (int i = 0; i < sizeof(g_KeyMapping) / sizeof(KeyMappingStruct); i++)
    {
        if (g_KeyMapping[i].sourcetype == sourcetype &&
            g_KeyMapping[i].sourcecd == sourcecd)
        {
            if (!ismenu &&
                (g_KeyMapping[i].resulttype == EVKEY || g_KeyMapping[i].resulttype == EVJOY))
                return g_KeyMapping + i;
            if (ismenu && g_KeyMapping[i].resulttype == EVCMD)
                return g_KeyMapping + i;
        }
    }

    return NULL;  // Mapping not found
}


/////////////////////////////////////////////////////////////////////////////

// Keyboard key mapping to bitmap
struct VirtKeyboardKey
{
    int             x, y;
    int             w, h;
    unsigned int    code;
    const char *    label;
    const char *    labelru;
}
static m_arrKeyboardKeys[] = {
/*   x1, y1   w, h     code             labelen         labelru  */
    {   3,  4, 54,34,    BK_KEY_REPEAT,   "\360\367\364", "\360\367\364" }, // REPEAT
    {  56,  4, 52,34,    0003,            "\353\364",     "\353\364" }, // KT
    { 109,  4, 54,34,    0231,            "=>|",          "=>|" },      // Arrow right    =|=>|
    { 162,  4, 52,34,    0026,            "|<=",          "|<=" },      // Arrow left     |<===
    { 215,  4, 54,34,    0027,            "|=>",          "|=>" },      // Arrow right    |===>
    { 268,  4, 52,34,    0202,            "\351\363\365", "\351\363\365" }, // »Õƒ —”
    { 321,  4, 54,34,    0204,            "\342\362",     "\342\362" },     // ¡ÀŒ  –≈ƒ
    { 374,  4, 52,34,    0220,            "\373\341\347", "\373\341\347" }, // STEP
    { 426,  4, 52,34,    0014,            "\363\342\362", "\363\342\362" }, // SBR
    { 478,  4, 70,34,    BK_KEY_STOP,     "\363\364\357\360", "\363\364\357\360" }, // STOP

    {   3,38,  36,34,    BK_KEY_BACKSHIFT,"",             ""  },        // Big Arrow Down
    {  38,38,  36,34,    0073,            ";",            ";" },        // ; +
    {  74,38,  34,34,    0061,            "1",            "1" },        // 1 !
    { 109,38,  36,34,    0062,            "2",            "2" },        // 2 "
    { 144,38,  34,34,    0063,            "3",            "3" },        // 3 #
    { 179,38,  36,34,    0064,            "4",            "4" },        // 4 $
    { 214,38,  36,34,    0065,            "5",            "5" },        // 5 %
    { 250,38,  34,34,    0066,            "6",            "6" },        // 6 &
    { 285,38,  36,34,    0067,            "7",            "7" },        // 7 '
    { 320,38,  34,34,    0070,            "8",            "8" },        // 8 (
    { 355,38,  36,34,    0071,            "9",            "9" },        // 9 )
    { 391,38,  36,34,    0060,            "0",            "0" },        // 0 {
    { 426,38,  34,34,    0055,            "-",            "-" },        // - =
    { 461,38,  36,34,    0057,            "/",            "/" },        // / ?
    { 496,38,  52,34,    0030,            "<=",           "<=" },       // Backspace

    {   3,72,  52,34,    0015,            "\364\341\342", "\364\341\342" }, // TAB
    {  55,72,  36,34,    0112,            "J",            "\352" },     // … J
    {  91,72,  36,34,    0103,            "C",            "\343" },     // ÷ C
    { 126,72,  36,34,    0125,            "U",            "\365" },     // ” U
    { 162,72,  34,34,    0113,            "K",            "\353" },     //   K
    { 197,72,  36,34,    0105,            "E",            "\345" },     // ≈ E
    { 232,72,  34,34,    0116,            "N",            "\356" },     // Õ N
    { 267,72,  36,34,    0107,            "G",            "\347" },     // √ G
    { 302,72,  36,34,    0133,            "[",            "\373" },     // ÿ [
    { 338,72,  34,34,    0135,            "]",            "\375" },     // Ÿ ]
    { 373,72,  36,34,    0132,            "Z",            "\372" },     // « Z
    { 408,72,  34,34,    0110,            "H",            "\350" },     // ’ H
    { 443,72,  36,34,    0072,            ":",            ":"    },     // : *
    { 479,72,  36,34,    0137,            "}",            "\377" },     // ⁄ }
    { 514,72,  34,34,    0023,            "\367\363",     "\367\363" }, // ¬—

    {  12,106, 52,34,    0000,            "\363\365",     "\363\365" }, // SU
    {  64,106, 36,34,    0106,            "F",            "\346" },     // ‘ F
    { 100,106, 36,34,    0131,            "Y",            "\371" },     // € Y
    { 136,106, 36,34,    0127,            "W",            "\367" },     // ¬ W
    { 172,106, 34,34,    0101,            "A",            "\341" },     // ¿ A
    { 206,106, 36,34,    0120,            "P",            "\360" },     // œ P
    { 242,106, 34,34,    0122,            "R",            "\362" },     // – R
    { 276,106, 36,34,    0117,            "O",            "\357" },     // Œ O
    { 312,106, 34,34,    0114,            "L",            "\354" },     // À L
    { 346,106, 34,34,    0104,            "D",            "\344" },     // ƒ D
    { 381,106, 36,34,    0126,            "V",            "\366" },     // ∆ V
    { 416,106, 36,34,    0134,            "\\",           "\374" },     // › Backslash
    { 452,106, 34,34,    0076,            ".",            "."    },     // . >
    { 486,106, 52,34,    0012,            "",             ""     },     // ENTER

    {  12,140, 36,34,    BK_KEY_LOWER,    "\363\364",     "\363\364" }, // ST
    {  48,140, 34,34,    BK_KEY_UPPER,    "\372\341",     "\372\341" }, // «¿√À
    {  82,140, 36,34,    0121,            "Q",            "\361" },     // ﬂ Q
    { 118,140, 34,34,    0136,            "^",            "\376" },     // ◊ ^
    { 152,140, 36,34,    0123,            "S",            "\363" },     // — S
    { 188,140, 34,34,    0115,            "M",            "\355" },     // Ã M
    { 222,140, 36,34,    0111,            "I",            "\351" },     // » I
    { 258,140, 34,34,    0124,            "T",            "\364" },     // “ T
    { 292,140, 36,34,    0130,            "X",            "\370" },     // ‹ X
    { 328,140, 34,34,    0102,            "B",            "\342" },     // ¡ B
    { 363,140, 36,34,    0100,            "@",            "\340" },     // ﬁ @
    { 399,140, 36,34,    0074,            ",",            ","    },     // , <

    {  12,174, 52,34,    0016,            "\362\365\363", "\362\365\363" }, // RUS
    {  64,174, 36,34,    BK_KEY_AR2,      "\341\362",     "\341\362" }, // AR2
    { 100,174,283,34,    0040,            "",             "" },         // Space bar
    { 382,174, 52,34,    0017,            "\354\341\364", "\354\341\364" },      // LAT

    { 434,140, 34,69,    0010,            NULL,           NULL },       // Left
    { 468,140, 36,34,    0032,            NULL,           NULL },       // Up
    { 468,174, 36,34,    0033,            NULL,           NULL },       // Down
    { 504,140, 34,69,    0031,            NULL,           NULL },       // Right
};
const int m_nKeyboardKeysCount = sizeof(m_arrKeyboardKeys) / sizeof(VirtKeyboardKey);

void Main_PrepareKeyboard(int latrus)
{
    SDL_Rect rc;
    rc.x = rc.y = 0;  rc.w = KEYBOARD_WIDTH;  rc.h = KEYBOARD_HEIGHT;
    SDL_FillRect(g_Keyboard, &rc, SDL_MapRGB(g_Keyboard->format, 0,0,0));

    Uint32 colorbk = SDL_MapRGB(g_Keyboard->format, 64, 120, 64);
    for (int i = 0; i < m_nKeyboardKeysCount; i++)
    {
        int x = m_arrKeyboardKeys[i].x / 2;
        int y = m_arrKeyboardKeys[i].y / 2;
        int w = m_arrKeyboardKeys[i].w / 2;
        int h = m_arrKeyboardKeys[i].h / 2;
        rc.x = x + 1;  rc.y = y + 1;  rc.w = w - 2;  rc.h = h - 2;
        SDL_FillRect(g_Keyboard, &rc, colorbk);
    }
    for (int i = 0; i < m_nKeyboardKeysCount; i++)
    {
        int x = m_arrKeyboardKeys[i].x / 2;
        int y = m_arrKeyboardKeys[i].y / 2;
        int w = m_arrKeyboardKeys[i].w / 2;
        int h = m_arrKeyboardKeys[i].h / 2;

        const char* label = latrus ? m_arrKeyboardKeys[i].label : m_arrKeyboardKeys[i].labelru;
        if (m_arrKeyboardKeys[i].label != NULL && (*label) != 0)
        {
            int len = strlen(label) * 8;
            Font_DrawText(x + w/2 - len/2, y + 4, label, g_Keyboard);
        }
    }
}

void Main_DrawKeyboard()
{
    unsigned short bkregister = g_pBoard->GetKeyboardRegister();
    Main_PrepareKeyboard(((bkregister & KEYB_LAT) != 0) ? TRUE : FALSE);

    // Draw the keyboard on the screen
    SDL_Rect src, dst;
    dst.x = KEYBOARD_LEFT;  dst.y = KEYBOARD_TOP;  dst.w = KEYBOARD_WIDTH;  dst.h = KEYBOARD_HEIGHT;
    src.x = 0;  src.y = 0;  src.w = KEYBOARD_WIDTH;  src.h = KEYBOARD_HEIGHT;
    SDL_BlitSurface(g_Keyboard, &src, g_Screen, &dst);

    // Draw frame around the current key
    int x = m_arrKeyboardKeys[g_KeyboardCurrent].x / 2 + KEYBOARD_LEFT;
    int y = m_arrKeyboardKeys[g_KeyboardCurrent].y / 2 + KEYBOARD_TOP;
    int w = m_arrKeyboardKeys[g_KeyboardCurrent].w / 2;
    int h = m_arrKeyboardKeys[g_KeyboardCurrent].h / 2;
    Uint32 color = SDL_MapRGB(g_Keyboard->format, 224,128,128);
    SDL_Rect rc;
    rc.x = x;  rc.y = y - 1;  rc.w = w;  rc.h = 2;
    SDL_FillRect(g_Screen, &rc, color);
    rc.x = x;  rc.y = y - 1;  rc.w = 2;  rc.h = h + 2;
    SDL_FillRect(g_Screen, &rc, color);
    rc.x = x;  rc.y = y + h - 1;  rc.w = w;  rc.h = 2;
    SDL_FillRect(g_Screen, &rc, color);
    rc.x = x + w - 1;  rc.y = y - 1;  rc.w = 2;  rc.h = h + 2;
    SDL_FillRect(g_Screen, &rc, color);
}

int Main_KeyboardFindNearestKeyXY(int x, int y)
{
    int minidx = g_KeyboardCurrent;
    int mindist = 0x7fffff;
    for (int i = 0; i < m_nKeyboardKeysCount; i++)
    {
        int left = m_arrKeyboardKeys[i].x / 2;
        int top = m_arrKeyboardKeys[i].y / 2;
        int right = left + m_arrKeyboardKeys[i].w / 2;
        int bottom = top + m_arrKeyboardKeys[i].h / 2;

        if (x > left && x < right && y > top && y < bottom)
            return i;

        int cx = (left + right) / 2;
        int cy = (top + bottom) / 2;

        int dist = (x - cx) * (x - cx) + (y - cy) * (y - cy);
        if (dist < mindist)
        {
            minidx = i;
            mindist = dist;
        }
    }

    return minidx;
}

int Main_KeyboardFindNearestKey(int direction)
{
    int left = m_arrKeyboardKeys[g_KeyboardCurrent].x / 2;
    int top = m_arrKeyboardKeys[g_KeyboardCurrent].y / 2;
    int right = left + m_arrKeyboardKeys[g_KeyboardCurrent].w / 2;
    int bottom = top + m_arrKeyboardKeys[g_KeyboardCurrent].h / 2;

    int x, y;
    switch (direction)
    {
    case ID_MENU_LEFT:   x = left - 36/4;         y = (top + bottom) / 2;  break;
    case ID_MENU_RIGHT:  x = right + 36/4;        y = (top + bottom) / 2;  break;
    case ID_MENU_UP:     x = (left + right) / 2;  y = top - 34/4;          break;
    case ID_MENU_DOWN:   x = (left + right) / 2;  y = bottom + 34/4;       break;
    default: return g_KeyboardCurrent;
    }

    return Main_KeyboardFindNearestKeyXY(x, y);
}


/////////////////////////////////////////////////////////////////////////////


void Main_ClearScreen()
{
    SDL_Rect rc;
    rc.x = rc.y = 0;  rc.w = SCREEN_WIDTH; rc.h = SCREEN_HEIGHT;
    SDL_FillRect(g_Screen, &rc, 0);
}

void Main_SetScreenMode(int screenMode)
{
    if (g_ScreenMode == screenMode)
        return;

    if (g_ScreenMode != -1)
    {
        SDL_FreeSurface(g_BKScreen);
    }

    g_ScreenMode = screenMode;

    if (screenMode != -1)
    {
        Emulator_GetScreenModeSize(screenMode, &g_BKScreenWid, &g_BKScreenHei);
        g_BKScreen = SDL_CreateRGBSurface(0, g_BKScreenWid, g_BKScreenHei, 32, 0,0,0,0);
    }
}

void Main_DrawScreen()
{
    SDL_LockSurface(g_BKScreen);
    void* pPixels = g_BKScreen->pixels;
    Emulator_PrepareScreen(pPixels, g_ScreenMode);
    SDL_UnlockSurface(g_BKScreen);
    
    // Draw BK screen
    SDL_Rect src, dst, rc;
    src.x = src.y = dst.x = dst.y = 0;
    src.w = dst.w = SCREEN_WIDTH;
    src.h = dst.w = SCREEN_HEIGHT;
    if (g_BKScreenWid < SCREEN_WIDTH)
    {
        int scrleft = (SCREEN_WIDTH - g_BKScreenWid) / 2;
        src.w = dst.w = g_BKScreenWid;  dst.x = scrleft;
        rc.x = rc.y = 0;  rc.w = scrleft;  rc.h = SCREEN_HEIGHT;
        SDL_FillRect(g_Screen, &rc, 0);
        rc.x = scrleft + g_BKScreenWid;  rc.y = 0;  rc.w = SCREEN_WIDTH - scrleft - g_BKScreenWid;  rc.h = SCREEN_HEIGHT;
        SDL_FillRect(g_Screen, &rc, 0);
    }
    if (g_BKScreenHei < SCREEN_HEIGHT)
    {
        src.h = dst.h = g_BKScreenHei;
        dst.y = (SCREEN_HEIGHT - g_BKScreenHei) / 2;
    }
    SDL_BlitSurface(g_BKScreen, &src, g_Screen, &dst);
    
    if (g_okKeyboard)
        Main_DrawKeyboard();

    SDL_Flip(g_Screen);
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

// Show directory browser for *.BIN mask
void Main_BrowseAndLoadBin()
{
    // Get file list by mask
    char ** pfilenames = Common_FindFiles(g_AppDirectory, "*.bin");

    int exitBrowser = FALSE;
    int currentItem = 0;
    int redrawScreen = TRUE;
    int menuItemCount = 0;
    const int menuLeft = 12;
    const int menuWidth = 8 * 32;
    const int menuHeight = 11 * 20;

    while (!exitBrowser)
    {
        if (redrawScreen)
        {
            Main_DrawScreen();

            // Draw menu background 
            SDL_Rect rc;
            rc.x = menuLeft - 8; rc.y = 8 - 4;
            rc.w = 12 + menuWidth; rc.h = 8 + menuHeight;
            SDL_FillRect(g_Screen, &rc, SDL_MapRGB(g_Screen->format, 32, 32, 192));
            // Draw selected item background
            rc.x = menuLeft - 4; rc.y = 8 - 1 + currentItem * 11;
            rc.w = 4 + menuWidth; rc.h = 11 + 2;
            SDL_FillRect(g_Screen, &rc, SDL_MapRGB(g_Screen->format, 192, 32, 32));

            // Draw menu items
            char ** pitem = pfilenames;
            int y = 8;
            menuItemCount = 0;
            while (*pitem != NULL)
            {
                Font_DrawText(menuLeft, y, *pitem);
                pitem++;
                y += 11;
                menuItemCount++;
            }

            SDL_Flip(g_Screen);

            redrawScreen = FALSE;
        }

        SDL_Event evt;
        while (SDL_PollEvent(&evt))
        {
            redrawScreen = TRUE;
            if (evt.type == SDL_QUIT)
            {
                g_okQuit = exitBrowser = TRUE;
                break;
            }
            if (evt.type == SDL_KEYDOWN)
            {
                switch (evt.key.keysym.sym)
                {
                case SDLK_PAUSE:  // POWER UP button on Dingoo
                    g_okQuit = exitBrowser = TRUE;
                    break;
                case SDLK_TAB:  // Left shoulder on Dingoo
                case SDLK_ESCAPE:  // SELECT button on Dingoo
                    exitBrowser = TRUE;
                    break;
                case SDLK_UP:
                    if (currentItem > 0) currentItem--; else currentItem = menuItemCount - 1;
                    break;
                case SDLK_DOWN:
                    if (currentItem < menuItemCount - 1) currentItem++; else currentItem = 0;
                    break;
                case SDLK_LCTRL:  // A button on Dingoo
                case SDLK_SPACE:  // X button on Dingoo
                case SDLK_RETURN:  // START button on Dingoo
                    {
                        char * filename = pfilenames[currentItem];
                        Main_LoadBin(filename);
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
                        exitBrowser = TRUE;
                    }
                    break;
                default:
                    break;
                }
            }
        }

        SDL_Delay(50);
    }

    Common_FindFiles_Cleanup(pfilenames);
}

void Main_ExecuteCommand(int command)
{
    switch (command)
    {
    case ID_EXIT:
        g_okQuit = TRUE;
        break;
    case ID_MENU:
        Main_Menu();
        break;
    case ID_VIDEO_MODE_NEXT:
        Main_SetScreenMode((g_ScreenMode + 1 == EMULATOR_SCREENMODE_COUNT) ? 0 : g_ScreenMode + 1);
        break;
    case ID_VIDEO_MODE_PREV:
        Main_SetScreenMode((g_ScreenMode == 0) ? EMULATOR_SCREENMODE_COUNT - 1 : g_ScreenMode - 1);
        break;
    case ID_RESET:
        Emulator_Reset();
        break;
    case ID_LOAD_BIN:
        Main_BrowseAndLoadBin();
        break;
    case ID_KEYBOARD:
        g_okKeyboard = !g_okKeyboard;
        break;
    default:
        break;
    }
}

// Returns: TRUE - close menu, FALSE - do not close menu
int Main_ExecuteMenuCommand(int command, int rightleft)
{
    switch (command)
    {
    case ID_VIDEO_MODE:
        Main_ExecuteCommand(rightleft ? ID_VIDEO_MODE_NEXT : ID_VIDEO_MODE_PREV);
        return FALSE;
    default:
        Main_ExecuteCommand(command);
        return TRUE;
    }
}

struct MenuItemStruct
{
    const char* text;
    int         command;
}
static m_MainMenuItems[] =
{
    { "Video Mode    <>",  ID_VIDEO_MODE },
    { "Keyboard",          ID_KEYBOARD },
    { "Load BIN       >",  ID_LOAD_BIN },
    { "Reset",             ID_RESET },
    //{ "Configuration <>",  ID_CONFIGURATION },
};

void Main_Menu()
{
    int exitMenu = FALSE;
    int currentItem = 0;
    int redrawScreen = TRUE;
    const int menuItemCount = sizeof(m_MainMenuItems) / sizeof(MenuItemStruct);
    const int menuLeft = 12;
    const int menuWidth = 8 * 16;
    char progname[50];
    char buffer[32];

    sprintf(progname, "BKBTL SDL version %d.%d  " __DATE__ " ", VERSION_MAJOR, VERSION_MINOR);

    while (!exitMenu)
    {
        if (redrawScreen)
        {
            Main_DrawScreen();

            // Draw menu background 
            SDL_Rect rc;
            rc.x = menuLeft - 8; rc.y = 8 - 4;
            rc.w = 16 + menuWidth; rc.h = 8 + 12 * menuItemCount;
            SDL_FillRect(g_Screen, &rc, SDL_MapRGB(g_Screen->format, 32, 32, 192));
            // Draw selected item background
            rc.x = menuLeft - 4; rc.y = 8 - 1 + currentItem * 12;
            rc.w = 8 + menuWidth; rc.h = 11 + 2;
            SDL_FillRect(g_Screen, &rc, SDL_MapRGB(g_Screen->format, 192, 32, 32));

            // Draw menu items
            int y = 8;
            for (int i = 0; i < menuItemCount; i++)
            {
                Font_DrawText(menuLeft, y + 12 * i, m_MainMenuItems[i].text);
            }

            // Emulator name and version number
            Font_DrawText(menuLeft, SCREEN_HEIGHT - 12, progname);
            // Last FPS
            sprintf(buffer, "FPS: %d, Delay: %d", g_LastFps, g_LastDelay);
            Font_DrawText(menuLeft, SCREEN_HEIGHT - 24, buffer);

            SDL_Flip(g_Screen);

            redrawScreen = FALSE;
        }

        SDL_Event evt;
        while (SDL_PollEvent(&evt))
        {
            redrawScreen = TRUE;
            if (evt.type == SDL_QUIT)
            {
                g_okQuit = exitMenu = TRUE;
                break;
            }
            if (evt.type == SDL_KEYDOWN || evt.type == SDL_JOYBUTTONDOWN)
            {
                KeyMappingStruct* mapping = FindKeyMapping(1, evt.key.keysym.sym, TRUE);
                if (mapping != NULL)
                {
                    switch (mapping->resultcd)
                    {
                    case ID_EXIT:
                        g_okQuit = exitMenu = TRUE;
                        break;
                    case ID_MENU:
                    case ID_MENU_ESCAPE:
                        exitMenu = TRUE;
                        break;
                    case ID_MENU_UP:
                        if (currentItem > 0) currentItem--; else currentItem = menuItemCount - 1;
                        break;
                    case ID_MENU_DOWN:
                        if (currentItem < menuItemCount - 1) currentItem++; else currentItem = 0;
                        break;
                    case ID_MENU_RIGHT:
                    case ID_MENU_SELECT:
                        if (Main_ExecuteMenuCommand(m_MainMenuItems[currentItem].command, TRUE))
                            exitMenu = TRUE;
                        break;
                    case ID_MENU_LEFT:
                        if (Main_ExecuteMenuCommand(m_MainMenuItems[currentItem].command, FALSE))
                            exitMenu = TRUE;
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        SDL_Delay(50);
    }
}

// Handles SDL keyboard press/release and joystick button press/release events.
void Main_OnKeyJoyEvent(SDL_Event evt)
{
    KeyMappingStruct* mapping;
    int pressed = (evt.type == SDL_KEYDOWN || evt.type == SDL_JOYBUTTONDOWN);
    int sourcetype = (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP) ? EVKEY : EVJOY;

    if (g_okKeyboard)  // Onscreen keyboard mode
    {
        mapping = FindKeyMapping(sourcetype, evt.key.keysym.sym, TRUE);
        if (mapping == NULL)
            return;
        int command = mapping->resultcd;
        if (pressed)
        {
            switch (command)
            {
            case ID_MENU_ESCAPE:
            case ID_MENU:
            case ID_KEYBOARD:
                Main_ExecuteCommand(ID_KEYBOARD);
                return;
            case ID_MENU_UP:
            case ID_MENU_DOWN:
            case ID_MENU_LEFT:
            case ID_MENU_RIGHT:
                g_KeyboardCurrent = Main_KeyboardFindNearestKey(command);
                return;
            case ID_MENU_SELECT:
                Emulator_KeyboardEvent(m_arrKeyboardKeys[g_KeyboardCurrent].code, TRUE);
                break;
            default:
                break;
            }
        }
        else
        {
            if (command == ID_MENU_SELECT)
            {
                Emulator_KeyboardEvent(m_arrKeyboardKeys[g_KeyboardCurrent].code, FALSE);
            }
        }
        return;
    }

    mapping = FindKeyMapping(sourcetype, evt.key.keysym.sym, FALSE);
    if (mapping != NULL)  // BK event mapping found
    {
        BYTE result = mapping->resultcd;
        if (mapping->resulttype == EVJOY)
            Emulator_JoystickEvent(result, pressed);
        else
            Emulator_KeyboardEvent(result, pressed);
    }
    else if (pressed)  // Commands works only on key/button press, not release
    {
        mapping = FindKeyMapping(sourcetype, evt.key.keysym.sym, TRUE);
        if (mapping != NULL)  // Command mapping found
        {
            Main_ExecuteCommand(mapping->resultcd);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////

#if defined(_WIN32)
#  undef main  //HACK for VC error LNK1561: entry point must be defined
#elif defined(PSP)
   extern "C" int SDL_main (int argc, char* args[]);
#endif

int main(int argc, char** argv)
{
    const char *inPath = argv[0];
    // Get application directory
	int i, j;
	for (i = 0, j = 0; inPath[i] != '\0'; i++) {
		if ((inPath[i] == '\\') || (inPath[i] == '/'))
			j = i + 1;
	}
	strncpy(g_AppDirectory, inPath, j);
	g_AppDirectory[j] = '\0';
    
    // Get application INI file path
    const char *inPathDot = strrchr(inPath, '.');
    int c = (inPathDot == NULL) ? strlen(inPath) : inPathDot - inPath;
    strncpy(g_AppIniPath, inPath, c);
    strncpy(g_AppIniPath + c, ".ini", 4);

#if defined(_DINGOO)
    SDL_putenv("DINGOO_IGNORE_OS_EVENTS=1");  //HACK to fix "push long time on X" problem
#endif

    Settings_ParseIniFile(g_AppIniPath);

    // Init SDL video
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
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
    g_Keyboard = SDL_CreateRGBSurface(0, KEYBOARD_WIDTH, KEYBOARD_HEIGHT, 32, 0,0,0,0);
    SDL_SetAlpha(g_Keyboard, SDL_SRCALPHA, 180);
    SDL_SetColorKey(g_Keyboard, SDL_SRCCOLORKEY, SDL_MapRGB(g_Keyboard->format, 0,0,0));

    SDL_Delay(1000);
    if (SDL_NumJoysticks())
        g_Joystick = SDL_JoystickOpen(0);

    Fonts_Initialize();

    if (!Emulator_Init())
        return 255;
    if (!Emulator_InitConfiguration(DEFAULT_BK_CONF))
        return 255;

    Main_ClearScreen();
    Main_SetScreenMode(0);

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
                if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP ||
                    evt.type == SDL_JOYBUTTONDOWN || evt.type == SDL_JOYBUTTONUP)
                {
                    Main_OnKeyJoyEvent(evt);
                }
            }
        }

        if (g_okEmulatorRunning)
        {
            Emulator_SystemFrame();

            Main_DrawScreen();
            
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

    Main_SetScreenMode(-1);

    if (g_Joystick != NULL)
    {
        SDL_JoystickClose(g_Joystick);
    }

    // Free memory
    SDL_FreeSurface(g_Keyboard);
    SDL_FreeSurface(g_Screen);

    Fonts_Finalize();

    SDL_Quit();

    return 0;
}
