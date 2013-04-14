// Common.cpp

#include "stdafx.h"

#if defined(_WIN32)
#  include <io.h>
#elif defined(_DINGOO)
#  include <dingoo/fsys.h>
#elif defined(PSP)
#  include <dirent.h>
#else  // POSIX
#  include <strings.h>
#  include <dirent.h>
#endif


//////////////////////////////////////////////////////////////////////


void AlertWarning(LPCTSTR sMessage)
{
#if defined(_WIN32)
    _tprintf(_T("! %s\n"), sMessage);
//TODO: Implement for other platforms
#endif
}

#if !defined(PRODUCT)

void DebugPrintFormat(LPCTSTR pszFormat, ...)
{
    //STUB
}

void DebugLog(LPCTSTR message)
{
    //STUB
}

void DebugLogFormat(LPCTSTR pszFormat, ...)
{
    //STUB
}

#endif // !defined(PRODUCT)


char ** Common_FindFiles(const char* directory, const char* filemask)
{
    int arrsize = 64;
    int filesfound = 0;
    char ** pfilenames = (char **)malloc(arrsize * sizeof(char*));
    memset(pfilenames, 0, arrsize * sizeof(char*));

#if defined(_WIN32)
    //char buffer[256];
    //sprintf(buffer, "%s%s", directory, filemask);
    _finddata_t finddata;
    intptr_t hfinddata = _findfirst(filemask, &finddata);
    if (hfinddata == -1)
    {
        free(pfilenames);
        return NULL;
    }
    while (1)
    {
        if (arrsize == filesfound)  // Expand the array
        {
            arrsize *= 2;
            pfilenames = (char **)realloc(pfilenames, arrsize * sizeof(char*));
        }
        char* pname = (char*)malloc(strlen(finddata.name) + 1);
        strcpy(pname, finddata.name);
        pfilenames[filesfound] = pname;
        filesfound++;

        int nextres = _findnext(hfinddata, &finddata);
        if (nextres != 0)
            break;
    }
    _findclose(hfinddata);
#elif defined(_DINGOO)
    char buffer[256];
    sprintf(buffer, "%s%s", directory, filemask);
    fsys_file_info_t fdata;
    int ret = fsys_findfirst(buffer, FSYS_FIND_FILE, &fdata);
    if (ret != 0)
    {
        free(pfilenames);
        return NULL;
    }
    while (1)
    {
        if (arrsize == filesfound)  // Expand the array
        {
            arrsize *= 2;
            pfilenames = (char **)realloc(pfilenames, arrsize * sizeof(char*));
        }
        char* pname = (char*)malloc(strlen(fdata.name) + 1);
        strcpy(pname, fdata.name);
        pfilenames[filesfound] = pname;
        filesfound++;

        int nextres = fsys_findnext(&fdata);
        if (nextres != 0)
            break;
    }
    fsys_findclose(&fdata);
#else  // POSIX -- not tested
    DIR *dirp = opendir(".");
    if (dirp == NULL)
    {
        free(pfilenames);
        return NULL;
    }
    int keylen = strlen(filemask);
    struct dirent *entry;
    while ((entry = readdir(dirp)))
    {
        //if (entry->d_type != 4/*DT_REG*/) continue;  // Skip other than regular files

        // Check if the filename ends with ".BIN"
        int namelen = strlen(entry->d_name);
        //if (strcasecmp(entry->d_name + (namelen - keylen), filemask) != 0)
        //    continue;
        if (arrsize == filesfound)  // Expand the array
        {
            arrsize *= 2;
            pfilenames = (char **)realloc(pfilenames, arrsize * sizeof(char*));
        }
        char* pname = (char*)malloc(namelen + 1);
        strcpy(pname, entry->d_name);
        pfilenames[filesfound] = pname;
        filesfound++;
    }
    closedir(dirp);
#endif

    if (arrsize != filesfound + 1)  // Shrink the array
        pfilenames = (char **)realloc(pfilenames, (filesfound + 1) * sizeof(char*));
    pfilenames[filesfound] = NULL;

    return (char **)pfilenames;
}

void Common_FindFiles_Cleanup(char ** parray)
{
    char ** parray2 = parray;
    while (*parray2 != NULL)
    {
        free(*parray2);
        parray2++;
    }

    free(parray);
}


//////////////////////////////////////////////////////////////////////
