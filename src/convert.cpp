#include <cassert>
#include "convert.h"
#include "dpf.h"
#include "todo.h"

// Convert using ACM
DWORD ConvertPCMGeneric(BYTE *lpSrc, DWORD cbSrc, BYTE *lpDst, DWORD cbDst, CONVERSIONDATA *lpConversionData)
{
    ACMSTREAMHEADER streamHeader;

    assert(lpConversionData->acmStream != NULL);
    assert(lpConversionData->UseACM == TRUE);

    if (lpDst == lpSrc)
    {
        assert(cbDst > cbSrc);
        memmove(lpSrc + (cbDst - cbSrc), lpSrc, cbSrc);
        lpSrc = lpSrc + (cbDst - cbSrc);
    }

    // Set up stream header
    streamHeader.cbStruct = sizeof(streamHeader);
    streamHeader.fdwStatus = 0;
    streamHeader.dwUser = 0;
    streamHeader.dwSrcUser = 0;
    streamHeader.dwDstUser = 0;
    streamHeader.pbSrc = lpSrc;
    streamHeader.cbSrcLength = cbSrc;
    streamHeader.cbSrcLengthUsed = cbSrc;
    streamHeader.pbDst = lpDst;
    streamHeader.cbDstLength = cbDst;
    streamHeader.cbDstLengthUsed = cbDst;

    // Convert
    acmStreamPrepareHeader(lpConversionData->acmStream, &streamHeader, 0);
    acmStreamConvert(lpConversionData->acmStream, &streamHeader, 4);
    acmStreamUnprepareHeader(lpConversionData->acmStream, &streamHeader, 0);

    return streamHeader.cbDstLengthUsed;
}

PCMConversionFn GetPCMConvertProc(WAVEFORMATEX *pWFExSource, WAVEFORMATEX *pWFExDest, CONVERSIONDATA *pConversionData)
{
    // FUTURE: Implement all of the PCM conversions here
    // Currently we only support delegating conversion to ACM
    // 3DMOVIE uses ConvertPCMMono8ToMono16

    HACMSTREAM acmStream = NULL;

    if (acmStreamOpen(&acmStream, 0, pWFExSource, pWFExDest, 0, 0, 0, 0) == 0 ||
        acmStreamOpen(&acmStream, 0, pWFExSource, pWFExDest, 0, 0, 0, ACM_STREAMOPENF_NONREALTIME) == 0)
    {
        pConversionData->UseACM = TRUE;
        pConversionData->acmStream = acmStream;
        return ConvertPCMGeneric;
    }

    DPRINTF(0, "NON SUPPORTED FORMAT!");
    TODO_NOT_IMPLEMENTED;
    return NULL;
}

BOOL ReleasePCMConvertProc(CONVERSIONDATA *pConversionData)
{
    if (pConversionData->UseACM && pConversionData->acmStream != NULL)
    {
        acmStreamClose(pConversionData->acmStream, 0);
    }
    return TRUE;
}

DWORD GetSourceSize(DWORD dwDestSize, CONVERSIONDATA *lpConversionData)
{
    LONG temp;
    DWORD srcSize;

    if (lpConversionData->UseACM == FALSE)
    {
        // TODO: implement this when other PCM conversion functions are implemented
        TODO_NOT_IMPLEMENTED;
    }
    else if (lpConversionData->UseACM == TRUE)
    {
        srcSize = 0;
        acmStreamSize(lpConversionData->acmStream, dwDestSize, &srcSize, 1);
    }
    else
    {
        srcSize = 0;
    }
    return srcSize;
}

DWORD GetDestSize(DWORD dwSourceSize, CONVERSIONDATA *lpConversionData)
{
    LONG temp;
    DWORD destSize;

    if (lpConversionData->UseACM == FALSE)
    {
        // TODO: implement this when other PCM conversion functions are implemented
        TODO_NOT_IMPLEMENTED;
    }
    else if (lpConversionData->UseACM == TRUE)
    {
        destSize = 0;
        acmStreamSize(lpConversionData->acmStream, dwSourceSize, &destSize, 0);
    }
    else
    {
        destSize = 0;
    }
    return destSize;
}
