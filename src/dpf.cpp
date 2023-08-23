#include <cstdarg>
#include <cstdio>
#include "dpf.h"

// Globals
static BOOL __gfDbgEnabled = TRUE;
static UINT __guDbgLevel = 0;
static char rgchOutputFile[64] = {0};

static BOOL gDebugOutActive = TRUE;
static BOOL gDebugLogActive = TRUE;

static char s_buffer[256];

void DbgVPrintF(const char *szFormat, va_list va)
{
    s_buffer[0] = '\x0';

    BOOL fDebugBreak = FALSE;
    BOOL fPrefix = TRUE;
    BOOL fCRLF = TRUE;

    // If the message:
    // - starts with ! then break into the debugger
    // - starts with ` then don't add the AUDIOMAN header
    // - starts with ~ then don't add a newline
    char cVar1 = 0;
    while (true)
    {
        while (true)
        {
            for (; cVar1 = *szFormat, cVar1 == '!'; szFormat = szFormat + 1)
            {
                fDebugBreak = TRUE;
            }
            if (cVar1 != '`')
                break;
            fPrefix = FALSE;
            szFormat = szFormat + 1;
        }
        if (cVar1 != '~')
            break;
        fCRLF = FALSE;
        szFormat = szFormat + 1;
    }

    if (fPrefix)
    {
        lstrcatA(s_buffer, "AUDIOMAN: ");
    }

    // Format message
    wvsprintfA(s_buffer + lstrlenA(s_buffer), szFormat, va);

    // Add newline
    if (fCRLF)
    {
        lstrcatA(s_buffer, "\r\n");
    }

    // Log to debugger
    if (gDebugOutActive)
    {
        OutputDebugStringA(s_buffer);
    }

    // Log to file
    if (gDebugLogActive && rgchOutputFile[0] != 0)
    {
        FILE *logFile = fopen(rgchOutputFile, "a");
        if (logFile)
        {
            fputs(s_buffer, logFile);
            fclose(logFile);
        }
    }

    // Break into debugger
    if (fDebugBreak)
    {
        DebugBreak();
    }
}

UINT DbgInitialize(BOOL fEnable)
{
    UINT uLevel = GetProfileIntA("DEBUG", "AUDIOMAN", 0xFFFFFFFF);
    if (uLevel == 0xFFFFFFFF)
    {
        uLevel = 0;
        fEnable = FALSE;
    }

    DbgSetLevel(uLevel);
    DbgEnable(fEnable);

    return __guDbgLevel;
}

BOOL DbgEnable(BOOL fEnable)
{
    BOOL fOldState = __gfDbgEnabled;
    __gfDbgEnabled = fEnable;
    return fOldState;
}

void DbgLogActive(BOOL fActive)
{
    gDebugLogActive = fActive;
}

UINT DbgGetLevel()
{
    return __guDbgLevel;
}

UINT DbgSetLevel(UINT uLevel)
{
    UINT uOldLevel = __guDbgLevel;
    __guDbgLevel = uLevel;
    return uOldLevel;
}

void DbgLogFile(LPCSTR rgchLog, BOOL fActive)
{
    gDebugLogActive = fActive;
    strcpy(rgchOutputFile, rgchLog);
}

void DbgOutActive(BOOL fOutActive)
{
    gDebugOutActive = fOutActive;
}

void _dprintf(UINT uDbgLevel, LPCSTR szFormat, ...)
{
    if (__gfDbgEnabled && uDbgLevel <= __guDbgLevel)
    {
        va_list va;
        va_start(va, szFormat);

        DbgVPrintF(szFormat, va);

        va_end(va);
    }
}