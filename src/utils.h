/**
 * Utilities
 **/
#pragma once
#include "audiomaninternal.h"

// Map a MMRESULT to a HRESULT
HRESULT HRESULTFromMMRESULT(MMRESULT mr);

// Convert milliseconds to an SMPTE time
BOOL ConvertMillisecToSMPTE(SMPTE *lpSMPTE, DWORD dwTime);

// Given a wave format, convert bytes to number of samples
DWORD BytesToSamples(WAVEFORMATEX *lpwfx, DWORD dwBytes);

// Given a wave format, convert number of samples to bytes
DWORD SamplesToBytes(WAVEFORMATEX *lpwfx, DWORD dwSamples);

// Convert a wave format to a dwFormat used by WAVEOUTCAPS
ULONG ConvertWaveFormatExToFormat(WAVEFORMATEX *pWaveFormatEx);

// Convert a dwFormat used by WAVEOUTCAPS to a WAVEFORMATEX
BOOL ConvertFormatToWaveFormatEx(WAVEFORMATEX *pWaveFormatEx, DWORD dwFormat);

// Given a wave format, convert milliseconds to bytes
DWORD MillisecToBytes(WAVEFORMATEX *lpwfx, DWORD dwMillisec);

// Given a wave format, convert bytes to milliseconds
DWORD BytesToMillisec(WAVEFORMATEX *lpwfx, DWORD dwBytes);

// Given a wave format, convert milliseconds to samples
DWORD MillisecToSamples(WAVEFORMATEX *lpwfx, DWORD dwTime);

// Given a wave format, convert samples to milliseconds
DWORD SamplesToMillisec(WAVEFORMATEX *lpwfx, DWORD dwSamples);

// Return True if the two given WAVE formats are the same
BOOL SameFormats(WAVEFORMATEX *lpFormatA, WAVEFORMATEX *lpFormatB);
