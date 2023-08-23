/**
 * Debug logging
 **/
#pragma once
#include <Windows.h>

/**
 * Initializes logging
 * @param fEnable if True, enable logging
 **/
UINT DbgInitialize(BOOL fEnable);

/**
 * Enables or disables logging
 * @param fEnable if True, enable logging
 **/
BOOL DbgEnable(BOOL fEnable);

/**
 * Enables or disables logging to a file
 * @param fEnable if True, enable logging to a file
 **/
void DbgLogActive(BOOL fActive);

/**
 * Enables or disables logging to debugger
 * @param fEnable if True, enable logging to debugger
 **/
void DbgOutActive(BOOL fOutActive);

/**
 * Gets logging level
 **/
UINT DbgGetLevel();

/**
 * Sets logging level
 * @param uLevel new logging level
 **/
UINT DbgSetLevel(UINT uLevel);

/**
 * Sets path to log file
 * @param rgchLog log file path
 * @param fActive if True, enable logging to file
 **/
void DbgLogFile(LPCSTR rgchLog, BOOL fActive);

/**
 * Writes to debug log
 * @param uDbgLevel log level
 * @param szFormat printf-style format string
 **/
void _dprintf(UINT uDbgLevel, LPCSTR szFormat, ...);

#ifdef _DEBUG
#define DPRINTF(level, message, ...) _dprintf(level, message, __VA_ARGS__)
#else
#define DPRINTF(level, message, ...)
#endif