/**
 * Sound conversion utilities
 **/
#pragma once
#include "audiomaninternal.h"

#include <mmreg.h>
#include <MSAcm.h>

// Parameters required for a PCM conversion function
typedef struct _CONVERSIONDATA
{
    BOOL UseACM;
    union {
        HACMSTREAM acmStream;
        INT buffer;
    };
    INT rateFactor;
} CONVERSIONDATA;

/**
 * Convert PCM audio
 * @param lpSrc source sample buffer
 * @param cbSrc size of source sample buffer in bytes
 * @param lpDst destination sample buffer
 * @param cbDst size of destination sample buffer in bytes
 * @param lpConversionData conversion parameters
 * @return number of bytes written
 **/
typedef DWORD(__cdecl *PCMConversionFn)(BYTE *lpSrc, DWORD cbSrc, BYTE *lpDst, DWORD cbDst,
                                        CONVERSIONDATA *lpConversionData);

/**
 * Return a function that converts between the given source and destination formats.
 * @param pWFExSource Source format
 * @param pWFExDest Destination format
 * @param pConversionData Pointer to a CONVERSIONDATA structure. This function will fill the structure with parameters
 * required for conversion.
 * @return function pointer or NULL if no suitable conversion function was found.
 **/
PCMConversionFn GetPCMConvertProc(WAVEFORMATEX *pWFExSource, WAVEFORMATEX *pWFExDest, CONVERSIONDATA *pConversionData);

/**
 * Release resources allocated by a PCM conversion function
 **/
BOOL ReleasePCMConvertProc(CONVERSIONDATA *pConversionData);

/**
 * Given the destination size, calculate the source size
 **/
DWORD GetSourceSize(DWORD dwDestSize, CONVERSIONDATA *lpConversionData);

/**
 * Given the source size, calculate the destination size
 **/
DWORD GetDestSize(DWORD dwSourceSize, CONVERSIONDATA *lpConversionData);
