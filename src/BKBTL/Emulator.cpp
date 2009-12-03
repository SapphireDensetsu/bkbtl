// Emulator.cpp

#include "stdafx.h"
#include "BKBTL.h"
#include "Emulator.h"
#include "Views.h"
#include "emubase\Emubase.h"
#include "SoundGen.h"


//////////////////////////////////////////////////////////////////////


CMotherboard* g_pBoard = NULL;
BKConfiguration g_nEmulatorConfiguration;  // Current configuration
BOOL g_okEmulatorRunning = FALSE;

WORD m_wEmulatorCPUBreakpoint = 0177777;

BOOL m_okEmulatorSound = FALSE;

long m_nFrameCount = 0;
DWORD m_dwTickCount = 0;
DWORD m_dwEmulatorUptime = 0;  // BK uptime, seconds, from turn on or reset, increments every 25 frames
long m_nUptimeFrameCount = 0;

BYTE* g_pEmulatorRam;  // RAM values - for change tracking
BYTE* g_pEmulatorChangedRam;  // RAM change flags
WORD g_wEmulatorCpuPC = 0177777;      // Current PC value
WORD g_wEmulatorPrevCpuPC = 0177777;  // Previous PC value


//////////////////////////////////////////////////////////////////////


const LPCTSTR FILENAME_BKROM_MONIT10    = _T("monit10.bin");
const LPCTSTR FILENAME_BKROM_FOCAL      = _T("focal.bin");
const LPCTSTR FILENAME_BKROM_TESTS      = _T("tests.bin");
const LPCTSTR FILENAME_BKROM_BASIC10_1  = _T("basic10_1.bin");
const LPCTSTR FILENAME_BKROM_BASIC10_2  = _T("basic10_2.bin");
const LPCTSTR FILENAME_BKROM_BASIC10_3  = _T("basic10_3.bin");
const LPCTSTR FILENAME_BKROM_DISK_327   = _T("disk_327.bin");


BOOL Emulator_LoadRomFile(LPCTSTR strFileName, BYTE* buffer, DWORD bytesToRead)
{
    HANDLE hRomFile = CreateFile(strFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hRomFile == INVALID_HANDLE_VALUE)
        return false;

    ASSERT(bytesToRead <= 8192);
    ZeroMemory(buffer, 8192);

    DWORD dwBytesRead;
    ReadFile(hRomFile, buffer, bytesToRead, &dwBytesRead, NULL);
    if (dwBytesRead != bytesToRead)
    {
        CloseHandle(hRomFile);
        return false;
    }

    CloseHandle(hRomFile);

    return TRUE;
}

BOOL Emulator_Init()
{
    ASSERT(g_pBoard == NULL);

    CProcessor::Init();

    g_pBoard = new CMotherboard();

    // Allocate memory for old RAM values
    g_pEmulatorRam = (BYTE*) ::LocalAlloc(LPTR, 65536);
    g_pEmulatorChangedRam = (BYTE*) ::LocalAlloc(LPTR, 65536);

    g_pBoard->Reset();

    if (m_okEmulatorSound)
    {
	    SoundGen_Initialize();
        g_pBoard->SetSoundGenCallback(SoundGen_FeedDAC);
    }

    return TRUE;
}

void Emulator_Done()
{
    ASSERT(g_pBoard != NULL);

    CProcessor::Done();

    g_pBoard->SetSoundGenCallback(NULL);
    SoundGen_Finalize();

    delete g_pBoard;
    g_pBoard = NULL;

    // Free memory used for old RAM values
    ::LocalFree(g_pEmulatorRam);
    ::LocalFree(g_pEmulatorChangedRam);
}

BOOL Emulator_InitConfiguration(BKConfiguration configuration)
{
    BYTE buffer[8192];

    // Load Monitor ROM file - in all configurations
    if (!Emulator_LoadRomFile(FILENAME_BKROM_MONIT10, buffer, 8192))
    {
        AlertWarning(_T("Failed to load Monitor ROM file."));
        return FALSE;
    }
    g_pBoard->LoadROM(0, buffer);

    ZeroMemory(buffer, 8192);
    g_pBoard->LoadROM(1, buffer);
    g_pBoard->LoadROM(2, buffer);
    g_pBoard->LoadROM(3, buffer);

    if (configuration & BK_COPT_ROM_BASIC)
    {
        // Load BASIC ROM 1 file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC10_1, buffer, 8192))
        {
            AlertWarning(_T("Failed to load BASIC ROM 1 file."));
            return FALSE;
        }
        g_pBoard->LoadROM(1, buffer);
        // Load BASIC ROM 2 file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC10_2, buffer, 8192))
        {
            AlertWarning(_T("Failed to load BASIC ROM 2 file."));
            return FALSE;
        }
        g_pBoard->LoadROM(2, buffer);
        // Load BASIC ROM 3 file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC10_3, buffer, 8064))
        {
            AlertWarning(_T("Failed to load BASIC ROM 3 file."));
            return FALSE;
        }
        g_pBoard->LoadROM(3, buffer);
    }
    else if (configuration & BK_COPT_ROM_FOCAL)
    {
        // Load Focal ROM file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_FOCAL, buffer, 8192))
        {
            AlertWarning(_T("Failed to load Focal ROM file."));
            return FALSE;
        }
        g_pBoard->LoadROM(1, buffer);
        // Unused 8KB
        ZeroMemory(buffer, 8192);
        g_pBoard->LoadROM(2, buffer);
        // Load Tests ROM file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_TESTS, buffer, 8064))
        {
            AlertWarning(_T("Failed to load Tests ROM file."));
            return FALSE;
        }
        g_pBoard->LoadROM(3, buffer);
    }

    if (configuration & BK_COPT_FDD)
    {
        // Load disk driver ROM file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_DISK_327, buffer, 4096))
        {
            AlertWarning(_T("Failed to load DISK 327 ROM file."));
            return FALSE;
        }
        g_pBoard->LoadROM(3, buffer);
    }

    g_nEmulatorConfiguration = configuration;

    g_pBoard->SetConfiguration(configuration);
    g_pBoard->Reset();

    m_nUptimeFrameCount = 0;
    m_dwEmulatorUptime = 0;

    return TRUE;
}

void Emulator_Start()
{
    g_okEmulatorRunning = TRUE;

    // Set title bar text
    SetWindowText(g_hwnd, _T("BK Back to Life [run]"));
    MainWindow_UpdateMenu();

    m_nFrameCount = 0;
    m_dwTickCount = GetTickCount();
}
void Emulator_Stop()
{
    g_okEmulatorRunning = FALSE;
    m_wEmulatorCPUBreakpoint = 0177777;

    // Reset title bar message
    SetWindowText(g_hwnd, _T("BK Back to Life [stop]"));
    MainWindow_UpdateMenu();
    // Reset FPS indicator
    MainWindow_SetStatusbarText(StatusbarPartFPS, _T(""));

    MainWindow_UpdateAllViews();
}

void Emulator_Reset()
{
    ASSERT(g_pBoard != NULL);

    g_pBoard->Reset();

    m_nUptimeFrameCount = 0;
    m_dwEmulatorUptime = 0;

    MainWindow_UpdateAllViews();
}

void Emulator_SetCPUBreakpoint(WORD address)
{
    m_wEmulatorCPUBreakpoint = address;
}

BOOL Emulator_IsBreakpoint()
{
    WORD wCPUAddr = g_pBoard->GetCPU()->GetPC();
    if (wCPUAddr == m_wEmulatorCPUBreakpoint)
        return TRUE;
    return FALSE;
}

void Emulator_SetSound(BOOL soundOnOff)
{
    if (m_okEmulatorSound != soundOnOff)
    {
        if (soundOnOff)
        {
            SoundGen_Initialize();
            g_pBoard->SetSoundGenCallback(SoundGen_FeedDAC);
        }
        else
        {
            g_pBoard->SetSoundGenCallback(NULL);
            SoundGen_Finalize();
        }
    }

    m_okEmulatorSound = soundOnOff;
}

int Emulator_SystemFrame()
{
    g_pBoard->SetCPUBreakpoint(m_wEmulatorCPUBreakpoint);

    ScreenView_ScanKeyboard();
    ScreenView_ProcessKeyboard();
    
	if (!g_pBoard->SystemFrame())
        return 0;

	// Calculate frames per second
	m_nFrameCount++;
	DWORD dwCurrentTicks = GetTickCount();
	long nTicksElapsed = dwCurrentTicks - m_dwTickCount;
	if (nTicksElapsed >= 1200)
	{
		double dFramesPerSecond = m_nFrameCount * 1000.0 / nTicksElapsed;
		TCHAR buffer[16];
		swprintf_s(buffer, 16, _T("FPS: %05.2f"), dFramesPerSecond);
		MainWindow_SetStatusbarText(StatusbarPartFPS, buffer);

		m_nFrameCount = 0;
		m_dwTickCount = dwCurrentTicks;
	}

	// Calculate emulator uptime (25 frames per second)
	m_nUptimeFrameCount++;
	if (m_nUptimeFrameCount >= 25)
	{
		m_dwEmulatorUptime++;
		m_nUptimeFrameCount = 0;

		int seconds = (int) (m_dwEmulatorUptime % 60);
		int minutes = (int) (m_dwEmulatorUptime / 60 % 60);
		int hours   = (int) (m_dwEmulatorUptime / 3600 % 60);

		TCHAR buffer[20];
		swprintf_s(buffer, 20, _T("Uptime: %02d:%02d:%02d"), hours, minutes, seconds);
		MainWindow_SetStatusbarText(StatusbarPartUptime, buffer);
	}

    return 1;
}

// Update cached values after Run or Step
void Emulator_OnUpdate()
{
    // Update stored PC value
    g_wEmulatorPrevCpuPC = g_wEmulatorCpuPC;
    g_wEmulatorCpuPC = g_pBoard->GetCPU()->GetPC();

    // Update memory change flags
    {
        BYTE* pOld = g_pEmulatorRam;
        BYTE* pChanged = g_pEmulatorChangedRam;
        WORD addr = 0;
        do
        {
            BYTE newvalue = g_pBoard->GetRAMByte(addr);
            BYTE oldvalue = *pOld;
            *pChanged = (newvalue != oldvalue) ? 255 : 0;
            *pOld = newvalue;
            addr++;
            pOld++;  pChanged++;
        }
        while (addr < 65535);
    }
}

// Get RAM change flag
//   addrtype - address mode - see ADDRTYPE_XXX constants
WORD Emulator_GetChangeRamStatus(WORD address)
{
    return *((WORD*)(g_pEmulatorChangedRam + address));
}


//////////////////////////////////////////////////////////////////////
//
// Emulator image format - see CMotherboard::SaveToImage()
// Image header format (32 bytes):
//   4 bytes        BK_IMAGE_HEADER1
//   4 bytes        BK_IMAGE_HEADER2
//   4 bytes        BK_IMAGE_VERSION
//   4 bytes        BK_IMAGE_SIZE
//   4 bytes        BK uptime
//   12 bytes       Not used

void Emulator_SaveImage(LPCTSTR sFilePath)
{
    // Create file
    HANDLE hFile = CreateFile(sFilePath,
            GENERIC_WRITE, FILE_SHARE_READ, NULL,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        AlertWarning(_T("Failed to save image file."));
        return;
    }

    // Allocate memory
    BYTE* pImage = (BYTE*) ::LocalAlloc(LPTR, BKIMAGE_SIZE);
    ZeroMemory(pImage, BKIMAGE_SIZE);
    // Prepare header
    DWORD* pHeader = (DWORD*) pImage;
    *pHeader++ = BKIMAGE_HEADER1;
    *pHeader++ = BKIMAGE_HEADER2;
    *pHeader++ = BKIMAGE_VERSION;
    *pHeader++ = BKIMAGE_SIZE;
    // Store emulator state to the image
    //g_pBoard->SaveToImage(pImage);
    *(DWORD*)(pImage + 16) = m_dwEmulatorUptime;

    // Save image to the file
    DWORD dwBytesWritten = 0;
    WriteFile(hFile, pImage, BKIMAGE_SIZE, &dwBytesWritten, NULL);
    //TODO: Check if dwBytesWritten != BKIMAGE_SIZE

    // Free memory, close file
    LocalFree(pImage);
    CloseHandle(hFile);
}

void Emulator_LoadImage(LPCTSTR sFilePath)
{
    // Open file
    HANDLE hFile = CreateFile(sFilePath,
            GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        AlertWarning(_T("Failed to load image file."));
        return;
    }

    // Read header
    DWORD bufHeader[BKIMAGE_HEADER_SIZE / sizeof(DWORD)];
    DWORD dwBytesRead = 0;
    ReadFile(hFile, bufHeader, BKIMAGE_HEADER_SIZE, &dwBytesRead, NULL);
    //TODO: Check if dwBytesRead != BKIMAGE_HEADER_SIZE
    
    //TODO: Check version and size

    // Allocate memory
    BYTE* pImage = (BYTE*) ::LocalAlloc(LPTR, BKIMAGE_SIZE);

    // Read image
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    dwBytesRead = 0;
    ReadFile(hFile, pImage, BKIMAGE_SIZE, &dwBytesRead, NULL);
    //TODO: Check if dwBytesRead != BKIMAGE_SIZE

    // Restore emulator state from the image
    //g_pBoard->LoadFromImage(pImage);

    m_dwEmulatorUptime = *(DWORD*)(pImage + 16);

    // Free memory, close file
    LocalFree(pImage);
    CloseHandle(hFile);

    g_okEmulatorRunning = FALSE;

    MainWindow_UpdateAllViews();
}


//////////////////////////////////////////////////////////////////////
