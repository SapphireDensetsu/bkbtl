// Emulator.cpp

#include "stdafx.h"
#include "Emulator.h"
#include "emubase/Emubase.h"

//////////////////////////////////////////////////////////////////////


CMotherboard* g_pBoard = NULL;
BKConfiguration g_nEmulatorConfiguration;  // Current configuration

BOOL g_okEmulatorInitialized = FALSE;
BOOL g_okEmulatorRunning = FALSE;

DWORD m_dwEmulatorUptime = 0;  // BK uptime, seconds, from turn on or reset, increments every 25 frames
long m_nUptimeFrameCount = 0;

const int KEYEVENT_QUEUE_SIZE = 16;
WORD m_EmulatorKeyQueue[KEYEVENT_QUEUE_SIZE];
int m_EmulatorKeyQueueTop = 0;
int m_EmulatorKeyQueueBottom = 0;
int m_EmulatorKeyQueueCount = 0;

BYTE m_EmulatorJoystickState = 0;

//Прототип функции преобразования экрана
// Input:
//   pVideoBuffer   Исходные данные, биты экрана БК
//   okSmallScreen  Признак "малого" экрана
//   pPalette       Палитра
//   scroll         Текущее значение скроллинга
//   pImageBits     Результат, 32-битный цвет, размер для каждой функции свой
typedef void (CALLBACK* PREPARE_SCREEN_CALLBACK)(const BYTE* pVideoBuffer, int okSmallScreen, DWORD* pPalette, int scroll, void* pImageBits);

void CALLBACK Emulator_PrepareScreenColor256x256(const BYTE* pVideoBuffer, int okSmallScreen, DWORD* pPalette, int scroll, void* pImageBits);
void CALLBACK Emulator_PrepareScreenColor320x240(const BYTE* pVideoBuffer, int okSmallScreen, DWORD* pPalette, int scroll, void* pImageBits);
void CALLBACK Emulator_PrepareScreenBW256x256(const BYTE* pVideoBuffer, int okSmallScreen, DWORD* pPalette, int scroll, void* pImageBits);
void CALLBACK Emulator_PrepareScreenBW320x240(const BYTE* pVideoBuffer, int okSmallScreen, DWORD* pPalette, int scroll, void* pImageBits);

struct ScreenModeStruct
{
    int width;
    int height;
    PREPARE_SCREEN_CALLBACK callback;
}
static ScreenModeReference[] = {
    { 320, 240, Emulator_PrepareScreenColor320x240 },
    { 320, 240, Emulator_PrepareScreenBW320x240 },
    { 256, 256, Emulator_PrepareScreenColor256x256 },
    { 256, 256, Emulator_PrepareScreenBW256x256 },
    //{ 512, 384, Emulator_PrepareScreenColor512x384 },
    //{ 512, 256, Emulator_PrepareScreenBW512x256 },
};

const int EMULATOR_SCREENMODE_COUNT = sizeof(ScreenModeReference) / sizeof(ScreenModeStruct);

void Emulator_GetScreenModeSize(int screenMode, int* pwidth, int* pheight)
{
    *pwidth = ScreenModeReference[screenMode].width;
    *pheight = ScreenModeReference[screenMode].height;
}


//////////////////////////////////////////////////////////////////////


const LPCTSTR FILENAME_BKROM_MONIT10    = _T("monit10.rom");
const LPCTSTR FILENAME_BKROM_FOCAL      = _T("focal.rom");
const LPCTSTR FILENAME_BKROM_TESTS      = _T("tests.rom");
const LPCTSTR FILENAME_BKROM_BASIC10_1  = _T("basic10_1.rom");
const LPCTSTR FILENAME_BKROM_BASIC10_2  = _T("basic10_2.rom");
const LPCTSTR FILENAME_BKROM_BASIC10_3  = _T("basic10_3.rom");
const LPCTSTR FILENAME_BKROM_DISK_326   = _T("disk_326.rom");
const LPCTSTR FILENAME_BKROM_BK11M_BOS  = _T("b11m_bos.rom");
const LPCTSTR FILENAME_BKROM_BK11M_EXT  = _T("b11m_ext.rom");
const LPCTSTR FILENAME_BKROM_BASIC11M_0 = _T("basic11m_0.rom");
const LPCTSTR FILENAME_BKROM_BASIC11M_1 = _T("basic11m_1.rom");


//////////////////////////////////////////////////////////////////////
// Colors

const DWORD ScreenView_ColorPalette[4] = {
    0x000000, 0x0000FF, 0x00FF00, 0xFF0000
};

const DWORD ScreenView_ColorPalettes[16][4] = {
    //                                          Palette#     01           10          11
    { 0x000000, 0x0000FF, 0x00FF00, 0xFF0000, },  // 00    синий   |   зеленый  |  красный
    { 0x000000, 0xFFFF00, 0xFF00FF, 0xFF0000, },  // 01   желтый   |  сиреневый |  красный
    { 0x000000, 0x00FFFF, 0x0000FF, 0xFF00FF, },  // 02   голубой  |    синий   | сиреневый
    { 0x000000, 0x00FF00, 0x00FFFF, 0xFFFF00, },  // 03   зеленый  |   голубой  |  желтый
    { 0x000000, 0xFF00FF, 0x00FFFF, 0xFFFFFF, },  // 04  сиреневый |   голубой  |   белый
    { 0x000000, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, },  // 05    белый   |    белый   |   белый
    { 0x000000, 0x7F0000, 0x7F0000, 0xFF0000, },  // 06  темн-красн| красн-корич|  красный
    { 0x000000, 0x00FF7F, 0x00FF7F, 0xFFFF00, },  // 07  салатовый | светл-зелен|  желтый
    { 0x000000, 0xFF00FF, 0x7F00FF, 0x7F007F, },  // 08  фиолетовый| фиол-синий | сиреневый
    { 0x000000, 0x00FF7F, 0x7F00FF, 0x7F0000, },  // 09 светл-зелен| фиол-синий |красн-корич
    { 0x000000, 0x00FF7F, 0x7F007F, 0x7F0000, },  // 10  салатовый | фиолетовый |темн-красный
    { 0x000000, 0x00FFFF, 0xFFFF00, 0xFF0000, },  // 11   голубой  |   желтый   |  красный
    { 0x000000, 0xFF0000, 0x00FF00, 0x00FFFF, },  // 12   красный  |   зеленый  |  голубой
    { 0x000000, 0x00FFFF, 0xFFFF00, 0xFFFFFF, },  // 13   голубой  |   желтый   |   белый
    { 0x000000, 0xFFFF00, 0x00FF00, 0xFFFFFF, },  // 14   желтый   |   зеленый  |   белый 
    { 0x000000, 0x00FFFF, 0x00FF00, 0xFFFFFF, },  // 15   голубой  |   зеленый  |   белый
};


//////////////////////////////////////////////////////////////////////


BOOL Emulator_LoadRomFile(LPCTSTR strFileName, BYTE* buffer, DWORD fileOffset, DWORD bytesToRead)
{
    FILE* fpRomFile = ::_tfopen(strFileName, _T("rb"));
    if (fpRomFile == NULL)
        return FALSE;

    ASSERT(bytesToRead <= 8192);
    ::memset(buffer, 0, 8192);

    DWORD dwBytesRead = ::fread(buffer, 1, bytesToRead, fpRomFile);
    if (dwBytesRead != bytesToRead)
    {
        ::fclose(fpRomFile);
        return FALSE;
    }

    if (fileOffset > 0)
    {
        ::fseek(fpRomFile, fileOffset, SEEK_SET);
    }

    ::fclose(fpRomFile);

    return TRUE;
}

BOOL Emulator_Init()
{
    ASSERT(g_pBoard == NULL);

    CProcessor::Init();

    g_pBoard = new CMotherboard();

    g_pBoard->Reset();

    //if (m_okEmulatorSound)
    //{
    //    SoundGen_Initialize();
    //    g_pBoard->SetSoundGenCallback(SoundGen_FeedDAC);
    //}

    m_nUptimeFrameCount = 0;
    m_dwEmulatorUptime = 0;

    g_okEmulatorInitialized = TRUE;
    return TRUE;
}

void Emulator_Done()
{
    ASSERT(g_pBoard != NULL);

    CProcessor::Done();

    g_pBoard->SetSoundGenCallback(NULL);
    //SoundGen_Finalize();

    delete g_pBoard;
    g_pBoard = NULL;

    g_okEmulatorInitialized = FALSE;
}

BOOL Emulator_InitConfiguration(BKConfiguration configuration)
{
    g_pBoard->SetConfiguration(configuration);

    BYTE buffer[8192];

    if ((configuration & BK_COPT_BK0011) == 0)
    {
        // Load Monitor ROM file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_MONIT10, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load Monitor ROM file."));
            return FALSE;
        }
        g_pBoard->LoadROM(0, buffer);
    }

    if (configuration & BK_COPT_ROM_BASIC)
    {
        // Load BASIC ROM 1 file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC10_1, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load BASIC ROM 1 file."));
            return FALSE;
        }
        g_pBoard->LoadROM(1, buffer);
        // Load BASIC ROM 2 file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC10_2, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load BASIC ROM 2 file."));
            return FALSE;
        }
        g_pBoard->LoadROM(2, buffer);
        // Load BASIC ROM 3 file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC10_3, buffer, 0, 8064))
        {
            AlertWarning(_T("Failed to load BASIC ROM 3 file."));
            return FALSE;
        }
        g_pBoard->LoadROM(3, buffer);
    }
    else if (configuration & BK_COPT_ROM_FOCAL)
    {
        // Load Focal ROM file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_FOCAL, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load Focal ROM file."));
            return FALSE;
        }
        g_pBoard->LoadROM(1, buffer);
        // Unused 8KB
        ::memset(buffer, 0, 8192);
        g_pBoard->LoadROM(2, buffer);
        // Load Tests ROM file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_TESTS, buffer, 0, 8064))
        {
            AlertWarning(_T("Failed to load Tests ROM file."));
            return FALSE;
        }
        g_pBoard->LoadROM(3, buffer);
    }

    if (configuration & BK_COPT_BK0011)
    {
        // Load BK0011M BASIC 0, part 1
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC11M_0, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load BK11M BASIC 0 ROM file."));
            return FALSE;
        }
        g_pBoard->LoadROM(0, buffer);
        // Load BK0011M BASIC 0, part 2
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC11M_0, buffer, 8192, 8192))
        {
            AlertWarning(_T("Failed to load BK11M BASIC 0 ROM file."));
            return FALSE;
        }
        g_pBoard->LoadROM(1, buffer);
        // Load BK0011M BASIC 1
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC11M_1, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load BK11M BASIC 1 ROM file."));
            return FALSE;
        }
        g_pBoard->LoadROM(2, buffer);

        // Load BK0011M EXT
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BK11M_EXT, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load BK11M EXT ROM file."));
            return FALSE;
        }
        g_pBoard->LoadROM(3, buffer);
        // Load BK0011M BOS
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BK11M_BOS, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load BK11M BOS ROM file."));
            return FALSE;
        }
        g_pBoard->LoadROM(4, buffer);
    }

    if (configuration & BK_COPT_FDD)
    {
        // Load disk driver ROM file
        ::memset(buffer, 0, 8192);
        if (!Emulator_LoadRomFile(FILENAME_BKROM_DISK_326, buffer, 0, 4096))
        {
            AlertWarning(_T("Failed to load DISK ROM file."));
            return FALSE;
        }
        g_pBoard->LoadROM((configuration & BK_COPT_BK0011) ? 5 : 3, buffer);
    }

    g_nEmulatorConfiguration = configuration;

    g_pBoard->Reset();

    m_nUptimeFrameCount = 0;
    m_dwEmulatorUptime = 0;

    return TRUE;
}

void Emulator_Start()
{
    g_okEmulatorRunning = TRUE;

    //m_nFrameCount = 0;
    //m_nTickCount = 0;
}
void Emulator_Stop()
{
    g_okEmulatorRunning = FALSE;
    //m_wEmulatorCPUBreakpoint = 0177777;
}

void Emulator_KeyboardEvent(BYTE keyscan, int pressed)
{
    if (m_EmulatorKeyQueueCount == KEYEVENT_QUEUE_SIZE) return;  // Full queue

    int ctrl = 0;  //STUB
    unsigned char keyflags = (pressed ? 128 : 0) | (ctrl ? 64 : 0);
    WORD keyevent = MAKEWORD(keyscan, keyflags);

    m_EmulatorKeyQueue[m_EmulatorKeyQueueTop] = keyevent;
    m_EmulatorKeyQueueTop++;
    if (m_EmulatorKeyQueueTop >= KEYEVENT_QUEUE_SIZE)
        m_EmulatorKeyQueueTop = 0;
    m_EmulatorKeyQueueCount++;
}

WORD Emulator_GetKeyEventFromQueue()
{
    if (m_EmulatorKeyQueueCount == 0) return 0;  // Empty queue

    WORD keyevent = m_EmulatorKeyQueue[m_EmulatorKeyQueueBottom];
    m_EmulatorKeyQueueBottom++;
    if (m_EmulatorKeyQueueBottom >= KEYEVENT_QUEUE_SIZE)
        m_EmulatorKeyQueueBottom = 0;
    m_EmulatorKeyQueueCount--;

    return keyevent;
}

void Emulator_ProcessKeyEvent()
{
    // Process next event in the keyboard queue
    WORD keyevent = Emulator_GetKeyEventFromQueue();
    if (keyevent != 0)
    {
        BOOL pressed = ((keyevent & 0x8000) != 0);
        BOOL ctrl = ((keyevent & 0x4000) != 0);
        BYTE bkscan = LOBYTE(keyevent);
        g_pBoard->KeyboardEvent(bkscan, pressed, ctrl);
    }
}

void Emulator_JoystickEvent(unsigned short mask, int okPressed)
{
    if (okPressed)
        m_EmulatorJoystickState |= mask;
    else
        m_EmulatorJoystickState &= ~mask;
}

int Emulator_SystemFrame()
{
    //g_pBoard->SetCPUBreakpoint(m_wEmulatorCPUBreakpoint);

    Emulator_ProcessKeyEvent();
    g_pBoard->SetPrinterInPort(m_EmulatorJoystickState);
    
	if (!g_pBoard->SystemFrame())
        return FALSE;

    //// Calculate frames per second
    //m_nFrameCount++;
    //int nCurrentTicks = m_emulatorTime.elapsed();
    //long nTicksElapsed = nCurrentTicks - m_nTickCount;
    //if (nTicksElapsed >= 1200)
    //{
    //    double dFramesPerSecond = m_nFrameCount * 1000.0 / nTicksElapsed;
    //    Global_showFps(dFramesPerSecond);

    //    m_nFrameCount = 0;
    //    m_nTickCount = nCurrentTicks;
    //}

    //// Calculate emulator uptime (25 frames per second)
    //m_nUptimeFrameCount++;
    //if (m_nUptimeFrameCount >= 25)
    //{
    //    m_dwEmulatorUptime++;
    //    m_nUptimeFrameCount = 0;

    //    Global_showUptime(m_dwEmulatorUptime);
    //}

    return TRUE;
}

void Emulator_PrepareScreen(void* pImageBits, int screenMode)
{
    if (pImageBits == NULL) return;
    if (!g_okEmulatorInitialized) return;

    // Get scroll value
    WORD scroll = g_pBoard->GetPortView(0177664);
    BOOL okSmallScreen = ((scroll & 01000) == 0);
    scroll &= 0377;
    scroll = (scroll >= 0330) ? scroll - 0330 : 050 + scroll;

    // Get palette
    DWORD* pPalette;
    if ((g_nEmulatorConfiguration & BK_COPT_BK0011) == 0)
        pPalette = (DWORD*)ScreenView_ColorPalette;
    else
        pPalette = (DWORD*)ScreenView_ColorPalettes[g_pBoard->GetPalette()];

    const BYTE* pVideoBuffer = g_pBoard->GetVideoBuffer();
    ASSERT(pVideoBuffer != NULL);

    // Render to bitmap
    PREPARE_SCREEN_CALLBACK callback = ScreenModeReference[screenMode].callback;
    callback(pVideoBuffer, okSmallScreen, pPalette, scroll, pImageBits);
}

void CALLBACK Emulator_PrepareScreenColor256x256(const BYTE* pVideoBuffer, int okSmallScreen, DWORD* pPalette, int scroll, void* pImageBits)
{
    int linesToShow = okSmallScreen ? 64 : 256;
    for (int y = 0; y < linesToShow; y++)
    {
        int yy = (y + scroll) & 0377;
        const WORD* pVideo = (WORD*)(pVideoBuffer + yy * 0100);
        DWORD* pBits = (DWORD*)pImageBits + y * 256;
        for (int x = 0; x < 512 / 16; x++)
        {
            WORD src = *pVideo;

            for (int bit = 0; bit < 16; bit += 2)
            {
                DWORD color = pPalette[src & 3];
                *pBits = color;
                pBits++;
                src = src >> 2;
            }

            pVideo++;
        }
    }
    if (okSmallScreen)
    {
        memset((DWORD*)pImageBits, 64 * 512, (256 - 64) * 256 * sizeof(DWORD));
    }
}

void CALLBACK Emulator_PrepareScreenColor320x240(const BYTE* pVideoBuffer, int okSmallScreen, DWORD* pPalette, int scroll, void* pImageBits)
{
    int linesToShow = okSmallScreen ? 64 : 256;
    int y = 0;
    for (int bky = 0; bky < 256; bky++)
    {
        int yy = (bky + scroll) & 0377;
        const WORD* pVideo = (WORD*)(pVideoBuffer + yy * 0100);
        DWORD* pBits = (DWORD*)pImageBits + y * 320;
        for (int x = 0; x < 512 / 16; x++)
        {
            WORD src = *pVideo;
            for (int bit = 0; bit < 16; bit += 2)
            {
                DWORD color = pPalette[src & 3];
                *pBits = color;
                pBits++;
                src = src >> 2;

                if (bit % 8 == 6)  // Duplicate every 4th pixel
                {
                    *pBits = color;
                    pBits++;
                }
            }
            pVideo++;
        }

        if (bky >= linesToShow) break;
        if (bky % 16 == 1)
        {
            // Merge this line with the previous one
            BYTE* pBytes = (BYTE*)((DWORD*)pImageBits + y * 320);
            y--;
            BYTE* pBytesPrev = (BYTE*)((DWORD*)pImageBits + y * 320);
            for (int x = 0; x < 320; x++)
            {
                *pBytesPrev = (BYTE)((((WORD)*pBytes) + ((WORD)*pBytesPrev)) / 2);
                pBytesPrev++;  pBytes++;
                *pBytesPrev = (BYTE)((((WORD)*pBytes) + ((WORD)*pBytesPrev)) / 2);
                pBytesPrev++;  pBytes++;
                *pBytesPrev = (BYTE)((((WORD)*pBytes) + ((WORD)*pBytesPrev)) / 2);
                pBytesPrev++;  pBytes++;
                *pBytesPrev = (BYTE)((((WORD)*pBytes) + ((WORD)*pBytesPrev)) / 2);
                pBytesPrev++;  pBytes++;
            }
        }
        y++;
    }
    if (okSmallScreen)
    {
        memset((DWORD*)pImageBits, 64 * 512, (256 - 64) * 256 * sizeof(DWORD));  //TODO
    }
}

void CALLBACK Emulator_PrepareScreenBW256x256(const BYTE* pVideoBuffer, int okSmallScreen, DWORD* pPalette, int scroll, void* pImageBits)
{
    const DWORD bw2palette[4] = { 0x000000, 0x7F7F7F, 0x7F7F7F, 0xFFFFFF };

    int linesToShow = okSmallScreen ? 64 : 256;
    for (int y = 0; y < linesToShow; y++)
    {
        int yy = (y + scroll) & 0377;
        const WORD* pVideo = (WORD*)(pVideoBuffer + yy * 0100);
        DWORD* pBits = (DWORD*)pImageBits + y * 256;
        for (int x = 0; x < 512 / 16; x++)
        {
            WORD src = *pVideo;

            for (int bit = 0; bit < 16; bit += 2)
            {
                DWORD color = bw2palette[src & 3];
                *pBits = color;
                pBits++;
                src = src >> 2;
            }

            pVideo++;
        }
    }
    if (okSmallScreen)
    {
        memset((DWORD*)pImageBits, 64 * 512, (256 - 64) * 256 * sizeof(DWORD));
    }
}

void CALLBACK Emulator_PrepareScreenBW320x240(const BYTE* pVideoBuffer, int okSmallScreen, DWORD* pPalette, int scroll, void* pImageBits)
{
    const DWORD bw2palette[4] = { 0x000000, 0x7F7F7F, 0x7F7F7F, 0xFFFFFF };

    int linesToShow = okSmallScreen ? 64 : 256;
    int y = 0;
    for (int bky = 0; bky < 256; bky++)
    {
        int yy = (bky + scroll) & 0377;
        const WORD* pVideo = (WORD*)(pVideoBuffer + yy * 0100);
        DWORD* pBits = (DWORD*)pImageBits + y * 320;
        for (int x = 0; x < 512 / 16; x++)
        {
            WORD src = *pVideo;

            for (int bit = 0; bit < 16; bit += 2)
            {
                DWORD color = bw2palette[src & 3];
                *pBits = color;
                pBits++;
                src = src >> 2;

                if (bit % 8 == 6)  // Duplicate every 4th pixel
                {
                    *pBits = color;
                    pBits++;
                }
            }
            pVideo++;
        }

        if (bky >= linesToShow) break;
        if (bky % 16 == 1)
        {
            // Merge this line with the previous one
            BYTE* pBytes = (BYTE*)((DWORD*)pImageBits + y * 320);
            y--;
            BYTE* pBytesPrev = (BYTE*)((DWORD*)pImageBits + y * 320);
            for (int x = 0; x < 320; x++)
            {
                *pBytesPrev = (BYTE)((((WORD)*pBytes) + ((WORD)*pBytesPrev)) / 2);
                pBytesPrev++;  pBytes++;
                *pBytesPrev = (BYTE)((((WORD)*pBytes) + ((WORD)*pBytesPrev)) / 2);
                pBytesPrev++;  pBytes++;
                *pBytesPrev = (BYTE)((((WORD)*pBytes) + ((WORD)*pBytesPrev)) / 2);
                pBytesPrev++;  pBytes++;
                *pBytesPrev = (BYTE)((((WORD)*pBytes) + ((WORD)*pBytesPrev)) / 2);
                pBytesPrev++;  pBytes++;
            }
        }
        y++;
    }
    if (okSmallScreen)
    {
        memset((DWORD*)pImageBits, 64 * 512, (256 - 64) * 256 * sizeof(DWORD));
    }
}

void CALLBACK Emulator_PrepareScreenBW512x256(const BYTE* pVideoBuffer, int okSmallScreen, DWORD* pPalette, int scroll, void* pImageBits)
{
    int linesToShow = okSmallScreen ? 64 : 256;
    for (int y = 0; y < linesToShow; y++)
    {
        int yy = (y + scroll) & 0377;
        const WORD* pVideo = (WORD*)(pVideoBuffer + yy * 0100);
        DWORD* pBits = (DWORD*)pImageBits + y * 512;
        for (int x = 0; x < 512 / 16; x++)
        {
            WORD src = *pVideo;

            for (int bit = 0; bit < 16; bit++)
            {
                DWORD color = (src & 1) ? 0x0ffffff : 0;
                *pBits = color;
                pBits++;
                src = src >> 1;
            }

            pVideo++;
        }
    }
    if (okSmallScreen)
    {
        memset((DWORD*)pImageBits, 64 * 512, (256 - 64) * 512 * sizeof(DWORD));
    }
}

void CALLBACK Emulator_PrepareScreenColor512x384(const BYTE* pVideoBuffer, int okSmallScreen, DWORD* pPalette, int scroll, void* pImageBits)
{
    int linesToShow = okSmallScreen ? 64 : 256;
    int bky = 0;
    for (int y = 0; y < 384; y++)
    {
        DWORD* pBits = (DWORD*)pImageBits + y * 512;
        if (y % 3 == 2)
        {
            DWORD* pPrevBits = (DWORD*)pImageBits + (y - 1) * 512;
            memcpy(pBits, pPrevBits, 512 * 4);
            continue;
        }

        int yy = (bky + scroll) & 0377;
        const WORD* pVideo = (WORD*)(pVideoBuffer + yy * 0100);
        for (int x = 0; x < 512 / 16; x++)
        {
            WORD src = *pVideo;

            for (int bit = 0; bit < 16; bit += 2)
            {
                DWORD color = pPalette[src & 3];
                *pBits = color;
                pBits++;
                *pBits = color;
                pBits++;
                src = src >> 2;
            }

            pVideo++;
        }
        bky++;
        if (bky >= linesToShow) break;
    }
    if (okSmallScreen)
    {
        memset((DWORD*)pImageBits, 64 * 512, (256 - 64) * 512 * sizeof(DWORD));  //TODO
    }
}

void CALLBACK Emulator_PrepareScreenBW512x384(const BYTE* pVideoBuffer, int okSmallScreen, DWORD* pPalette, int scroll, void* pImageBits)
{
    int linesToShow = okSmallScreen ? 64 : 256;
    int bky = 0;
    for (int y = 0; y < 384; y++)
    {
        DWORD* pBits = (DWORD*)pImageBits + y * 512;
        if (y % 3 == 2)
        {
            DWORD* pPrevBits = (DWORD*)pImageBits + (y - 1) * 512;
            memcpy(pBits, pPrevBits, 512 * 4);
            continue;
        }

        int yy = (bky + scroll) & 0377;
        const WORD* pVideo = (WORD*)(pVideoBuffer + yy * 0100);
        for (int x = 0; x < 512 / 16; x++)
        {
            WORD src = *pVideo;

            for (int bit = 0; bit < 16; bit++)
            {
                DWORD color = (src & 1) ? 0x0ffffff : 0;
                *pBits = color;
                pBits++;
                src = src >> 1;
            }

            pVideo++;
        }
        bky++;
        if (bky >= linesToShow) break;
    }
    if (okSmallScreen)
    {
        memset((DWORD*)pImageBits, 64 * 512, (256 - 64) * 512 * sizeof(DWORD));  //TODO
    }
}

//////////////////////////////////////////////////////////////////////
