/*  This file is part of BKBTL.
    BKBTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    BKBTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
BKBTL. If not, see <http://www.gnu.org/licenses/>. */

// Settings.cpp

#include "stdafx.h"
#include "BKBTL.h"


//////////////////////////////////////////////////////////////////////


const TCHAR m_Settings_IniAppName[] = _T("BKBTL");
TCHAR m_Settings_IniPath[MAX_PATH];

BOOL m_Settings_Toolbar = TRUE;
BOOL m_Settings_Debug = FALSE;
BOOL m_Settings_Debug_Valid = FALSE;
BOOL m_Settings_RealSpeed = FALSE;
BOOL m_Settings_RealSpeed_Valid = FALSE;
BOOL m_Settings_Sound = FALSE;
BOOL m_Settings_Sound_Valid = FALSE;
WORD m_Settings_SoundVolume = 0x7fff;
BOOL m_Settings_SoundVolume_Valid = FALSE;
BOOL m_Settings_Covox = FALSE;
BOOL m_Settings_Covox_Valid = FALSE;
BOOL m_Settings_Joystick = FALSE;
BOOL m_Settings_Joystick_Valid = FALSE;
BOOL m_Settings_Keyboard = TRUE;
BOOL m_Settings_Keyboard_Valid = FALSE;
BOOL m_Settings_Tape = FALSE;
BOOL m_Settings_Tape_Valid = FALSE;


//////////////////////////////////////////////////////////////////////


void Settings_Init()
{
    // Prepare m_Settings_IniPath: get .exe file path and change extension to .ini
    ::GetModuleFileName(GetModuleHandle(NULL), m_Settings_IniPath, MAX_PATH);
    TCHAR* pExt = m_Settings_IniPath + _tcslen(m_Settings_IniPath) - 3;
    *pExt++ = _T('i');
    *pExt++ = _T('n');
    *pExt++ = _T('i');
}
void Settings_Done()
{
}

BOOL Settings_SaveStringValue(LPCTSTR sName, LPCTSTR sValue)
{
    BOOL result = WritePrivateProfileString(
            m_Settings_IniAppName, sName, sValue, m_Settings_IniPath);
    return result;
}
BOOL Settings_LoadStringValue(LPCTSTR sName, LPTSTR sBuffer, int nBufferLengthChars)
{
    DWORD result = GetPrivateProfileString(
            m_Settings_IniAppName, sName, NULL, sBuffer, nBufferLengthChars, m_Settings_IniPath);
    if (result > 0)
        return TRUE;

    sBuffer[0] = _T('\0');
    return FALSE;
}

BOOL Settings_SaveDwordValue(LPCTSTR sName, DWORD dwValue)
{
    TCHAR buffer[12];
    wsprintf(buffer, _T("%lu"), dwValue);

    return Settings_SaveStringValue(sName, buffer);
}
BOOL Settings_LoadDwordValue(LPCTSTR sName, DWORD* dwValue)
{
    TCHAR buffer[12];
    if (!Settings_LoadStringValue(sName, buffer, 12))
        return FALSE;

    int result = swscanf(buffer, _T("%lu"), dwValue);
    if (result == 0)
        return FALSE;

    return TRUE;
}

BOOL Settings_SaveBinaryValue(LPCTSTR sName, const void * pData, int size)
{
    TCHAR* buffer = (TCHAR*) ::malloc((size * 2 + 1) * sizeof(TCHAR));
    const BYTE* p = (const BYTE*) pData;
    TCHAR* buf = buffer;
    for (int i = 0; i < size; i++)
    {
        int a = *p;
        wsprintf(buf, _T("%02X"), a);
        p++;
        buf += 2;
    }

    BOOL result = Settings_SaveStringValue(sName, buffer);

    free(buffer);

    return result;
}

BOOL Settings_LoadBinaryValue(LPCTSTR sName, void * pData, int size)
{
    size_t buffersize = (size * 2 + 1) * sizeof(TCHAR);
    TCHAR* buffer = (TCHAR*) ::malloc(buffersize);
    if (!Settings_LoadStringValue(sName, buffer, buffersize))
    {
        free(buffer);
        return FALSE;
    }

    BYTE* p = (BYTE*) pData;
    TCHAR* buf = buffer;
    for (int i = 0; i < size; i++)
    {
        BYTE v = 0;

        TCHAR ch = *buf;
        if (ch >= _T('0') && ch <= _T('9'))
            v = ch - _T('0');
        else if (ch >= _T('A') && ch <= _T('F'))
            v = ch - _T('A') + 10;
        else  // Not hex
        {
            free(buffer);
            return FALSE;
        }
        buf++;

        v = v << 4;

        ch = *buf;
        if (ch >= _T('0') && ch <= _T('9'))
            v |= ch - _T('0');
        else if (ch >= _T('A') && ch <= _T('F'))
            v |= ch - _T('A') + 10;
        else  // Not hex
        {
            free(buffer);
            return FALSE;
        }
        buf++;

        *p = v;
        p++;
    }

    free(buffer);

    return TRUE;
}


//////////////////////////////////////////////////////////////////////


#define SETTINGS_GETSET_DWORD(PARAMNAME, PARAMNAMESTR, OUTTYPE, DEFVALUE) \
    OUTTYPE m_Settings_##PARAMNAME = DEFVALUE; \
    BOOL m_Settings_##PARAMNAME##_Valid = FALSE; \
    void Settings_Set##PARAMNAME(OUTTYPE newvalue) { \
        m_Settings_##PARAMNAME = newvalue; \
        m_Settings_##PARAMNAME##_Valid = TRUE; \
        Settings_SaveDwordValue(PARAMNAMESTR, (DWORD) newvalue); \
    } \
    OUTTYPE Settings_Get##PARAMNAME##() { \
        if (!m_Settings_##PARAMNAME##_Valid) { \
            DWORD dwValue = (DWORD) DEFVALUE; \
            Settings_LoadDwordValue(PARAMNAMESTR, &dwValue); \
            m_Settings_##PARAMNAME = (OUTTYPE) dwValue; \
            m_Settings_##PARAMNAME##_Valid = TRUE; \
        } \
        return m_Settings_##PARAMNAME; \
    }


BOOL Settings_GetWindowRect(RECT * pRect)
{
    RECT rc;
    if (Settings_LoadBinaryValue(_T("WindowRect"), &rc, sizeof(RECT)))
    {
        ::memcpy(pRect, &rc, sizeof(RECT));
        return TRUE;
    }

    return FALSE;
}
void Settings_SetWindowRect(const RECT * pRect)
{
    Settings_SaveBinaryValue(_T("WindowRect"), (const void *)pRect, sizeof(RECT));
}

SETTINGS_GETSET_DWORD(WindowMaximized, _T("WindowMaximized"), BOOL, FALSE);

SETTINGS_GETSET_DWORD(WindowFullscreen, _T("WindowFullscreen"), BOOL, FALSE);

void Settings_SetConfiguration(int configuration)
{
    Settings_SaveDwordValue(_T("Configuration"), (DWORD) configuration);
}
int Settings_GetConfiguration()
{
    DWORD dwValue = 0;
    Settings_LoadDwordValue(_T("Configuration"), &dwValue);
    return (int) dwValue;
}

void Settings_GetFloppyFilePath(int slot, LPTSTR buffer)
{
    TCHAR bufValueName[] = _T("Floppy0");
    bufValueName[6] = slot + _T('0');
    Settings_LoadStringValue(bufValueName, buffer, MAX_PATH);
}
void Settings_SetFloppyFilePath(int slot, LPCTSTR sFilePath)
{
    TCHAR bufValueName[] = _T("Floppy0");
    bufValueName[6] = slot + _T('0');
    Settings_SaveStringValue(bufValueName, sFilePath);
}

void Settings_GetCartridgeFilePath(int slot, LPTSTR buffer)
{
    TCHAR bufValueName[] = _T("Cartridge0");
    bufValueName[9] = slot + _T('0');
    Settings_LoadStringValue(bufValueName, buffer, MAX_PATH);
}
void Settings_SetCartridgeFilePath(int slot, LPCTSTR sFilePath)
{
    TCHAR bufValueName[] = _T("Cartridge0");
    bufValueName[9] = slot + _T('0');
    Settings_SaveStringValue(bufValueName, sFilePath);
}

void Settings_SetScreenViewMode(int mode)
{
    Settings_SaveDwordValue(_T("ScreenViewMode"), (DWORD) mode);
}
int Settings_GetScreenViewMode()
{
    DWORD dwValue = 0;
    Settings_LoadDwordValue(_T("ScreenViewMode"), &dwValue);
    return (int) dwValue;
}

void Settings_SetScreenHeightMode(int mode)
{
    Settings_SaveDwordValue(_T("ScreenHeightMode"), (DWORD) mode);
}
int Settings_GetScreenHeightMode()
{
    DWORD dwValue = 0;
    Settings_LoadDwordValue(_T("ScreenHeightMode"), &dwValue);
    return (int) dwValue;
}

void Settings_SetToolbar(BOOL flag)
{
    Settings_SaveDwordValue(_T("Toolbar"), (DWORD) flag);
}
BOOL Settings_GetToolbar()
{
    DWORD dwValue = (DWORD) TRUE;
    Settings_LoadDwordValue(_T("Toolbar"), &dwValue);
    return (BOOL) dwValue;
}

void Settings_SetDebug(BOOL flag)
{
    m_Settings_Debug = flag;
    m_Settings_Debug_Valid = TRUE;
    Settings_SaveDwordValue(_T("Debug"), (DWORD) flag);
}
BOOL Settings_GetDebug()
{
    if (!m_Settings_Debug_Valid)
    {
        DWORD dwValue = (DWORD) FALSE;
        Settings_LoadDwordValue(_T("Debug"), &dwValue);
        m_Settings_Debug = (BOOL) dwValue;
        m_Settings_Debug_Valid = TRUE;
    }
    return m_Settings_Debug;
}

void Settings_SetAutostart(BOOL flag)
{
    Settings_SaveDwordValue(_T("Autostart"), (DWORD) flag);
}
BOOL Settings_GetAutostart()
{
    DWORD dwValue = (DWORD) FALSE;
    Settings_LoadDwordValue(_T("Autostart"), &dwValue);
    return (BOOL) dwValue;
}

void Settings_SetRealSpeed(BOOL flag)
{
    m_Settings_RealSpeed = flag;
    m_Settings_RealSpeed_Valid = TRUE;
    Settings_SaveDwordValue(_T("RealSpeed"), (DWORD) flag);
}
BOOL Settings_GetRealSpeed()
{
    if (!m_Settings_RealSpeed_Valid)
    {
        DWORD dwValue = (DWORD) FALSE;
        Settings_LoadDwordValue(_T("RealSpeed"), &dwValue);
        m_Settings_RealSpeed = (BOOL) dwValue;
        m_Settings_RealSpeed_Valid = TRUE;
    }
    return m_Settings_RealSpeed;
}

void Settings_SetSound(BOOL flag)
{
    m_Settings_Sound = flag;
    m_Settings_Sound_Valid = TRUE;
    Settings_SaveDwordValue(_T("Sound"), (DWORD) flag);
}
BOOL Settings_GetSound()
{
    if (!m_Settings_Sound_Valid)
    {
        DWORD dwValue = (DWORD) FALSE;
        Settings_LoadDwordValue(_T("Sound"), &dwValue);
        m_Settings_Sound = (BOOL) dwValue;
        m_Settings_Sound_Valid = TRUE;
    }
    return m_Settings_Sound;
}

void Settings_SetSoundVolume(WORD value)
{
    m_Settings_SoundVolume = value;
    m_Settings_SoundVolume_Valid = TRUE;
    Settings_SaveDwordValue(_T("SoundVolume"), (DWORD) value);
}
WORD Settings_GetSoundVolume()
{
    if (!m_Settings_SoundVolume_Valid)
    {
        DWORD dwValue = (DWORD) 0x7fff;
        Settings_LoadDwordValue(_T("SoundVolume"), &dwValue);
        m_Settings_SoundVolume = (WORD)dwValue;
        m_Settings_SoundVolume_Valid = TRUE;
    }
    return m_Settings_SoundVolume;
}

void Settings_SetCovox(BOOL flag)
{
    m_Settings_Covox = flag;
    m_Settings_Covox_Valid = TRUE;
    Settings_SaveDwordValue(_T("Covox"), (DWORD) flag);
}
BOOL Settings_GetCovox()
{
    if (!m_Settings_Covox_Valid)
    {
        DWORD dwValue = (DWORD) FALSE;
        Settings_LoadDwordValue(_T("Covox"), &dwValue);
        m_Settings_Covox = (BOOL) dwValue;
        m_Settings_Covox_Valid = TRUE;
    }
    return m_Settings_Covox;
}

void Settings_SetJoystick(int joystick)
{
    m_Settings_Joystick = joystick;
    m_Settings_Joystick_Valid = TRUE;
    Settings_SaveDwordValue(_T("Joystick"), (DWORD) joystick);
}
int Settings_GetJoystick()
{
    if (!m_Settings_Joystick_Valid)
    {
        DWORD dwValue = (DWORD) FALSE;
        Settings_LoadDwordValue(_T("Joystick"), &dwValue);
        m_Settings_Joystick = (int) dwValue;
        m_Settings_Joystick_Valid = TRUE;
    }
    return m_Settings_Joystick;
}

void Settings_SetKeyboard(BOOL flag)
{
    m_Settings_Keyboard = flag;
    m_Settings_Keyboard_Valid = TRUE;
    Settings_SaveDwordValue(_T("Keyboard"), (DWORD) flag);
}
BOOL Settings_GetKeyboard()
{
    if (!m_Settings_Keyboard_Valid)
    {
        DWORD dwValue = (DWORD) TRUE;
        Settings_LoadDwordValue(_T("Keyboard"), &dwValue);
        m_Settings_Keyboard = (BOOL) dwValue;
        m_Settings_Keyboard_Valid = TRUE;
    }
    return m_Settings_Keyboard;
}

void Settings_SetTape(BOOL flag)
{
    m_Settings_Tape = flag;
    m_Settings_Tape_Valid = TRUE;
    Settings_SaveDwordValue(_T("Tape"), (DWORD) flag);
}
BOOL Settings_GetTape()
{
    if (!m_Settings_Tape_Valid)
    {
        DWORD dwValue = (DWORD) FALSE;
        Settings_LoadDwordValue(_T("Tape"), &dwValue);
        m_Settings_Tape = (BOOL) dwValue;
        m_Settings_Tape_Valid = TRUE;
    }
    return m_Settings_Tape;
}


//////////////////////////////////////////////////////////////////////
