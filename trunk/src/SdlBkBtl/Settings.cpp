// Settings.cpp

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////


void Settings_ParseSection(const char* pSectName)
{
    //TODO
}

void Settings_ParseKeyValue(const char* pKey, const char* pValue)
{
    //TODO
}

// Parse loaded settings file (INI format)
void Settings_ParseSettings(char* pText)
{
    char* pSection = NULL;

    for (;;)  // Text reading loop - line by line
    {
        // Line starts
        if (*pText == 0) break;  // End of text
        if (*pText == '\n' || *pText == '\r')  // Empty line
        {
            pText++;
            continue;
        }
        if (*pText == ';' || *pText == '#')  // Comment line
        {
            pText++;
            while (*pText != 0 && *pText != '\n' && *pText != '\r') pText++;  // Skip to the end of line
            continue;
        }

        while (*pText == ' ' || *pText == '\t') pText++;  // Skip leading spaces and tabs

        if (*pText == '[')  // Section header
        {
            pText++;
            char* pNewSection = pText;
            while (*pText != 0 && *pText != '\n' && *pText != '\r' && *pText != ']') pText++;
            if (*pText == ']')
            {
                *pText++ = 0;  // Mark end of the section name
                pSection = pNewSection;
                Settings_ParseSection(pSection);
            }

            while (*pText != 0 && *pText != '\n' && *pText != '\r') pText++;  // Skip to the end of line
            continue;
        }

        // Parse key=value clause
        char* pKey = pText;
        while (*pText >= 'A' && *pText <= 'Z' || *pText >= 'a' && *pText <= 'z' || *pText >= '0' && *pText <= '9') pText++;
        if (*pText == 0) break;  // End of text
        *pText++ = 0;  // End of the key
        while (*pText == ' ' || *pText == '\t') pText++;  // Skip spaces and tabs before '='
        if (*pText != '=')  // Invalid clause, skipping
        {
            while (*pText != 0 && *pText != '\n' && *pText != '\r') pText++;  // Skip to the end of line
            continue;
        }
        pText++;
        while (*pText == ' ' || *pText == '\t') pText++;  // Skip spaces and tabs after '='
        if (*pText == 0 || *pText == '\n' || *pText == '\r')
        {
            Settings_ParseKeyValue(pKey, NULL);
            continue;
        }
        char* pValue = pText;
        while (*pText != 0 && *pText != '\n' && *pText != '\r') pText++;  // Seek to the end of line
        while (*(pText-1) == ' ' || *(pText-1) == '\t') pText--;  // Trailing spaces and tabs
        if (*pText != 0) *pText++ = 0;  // Mark end of the value
        Settings_ParseKeyValue(pKey, pValue);

        while (*pText != 0 && *pText != '\n' && *pText != '\r') pText++;  // Skip to the end of line
    }
}

void Settings_ParseIniFile(const char* sFileName)
{
    //TODO
}


/////////////////////////////////////////////////////////////////////////////
