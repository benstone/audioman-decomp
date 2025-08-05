#include <cassert>
#include <cstdlib>
#include "convert.h"
#include "dpf.h"
#include "todo.h"

// PCM conversion functions
USHORT Mono8ToMono16(BYTE Mono8)
{
    return (USHORT)((Mono8 - 0x80) << 8);
}

USHORT Mono8ToStereo8(BYTE Mono8)
{
    return ((USHORT)Mono8 << 8) | Mono8;
}

UINT Mono16ToStereo16(USHORT Mono16)
{
    UINT value = (UINT)Mono16 & 0xFFFF;
    return (value << 16) | value;
}

BYTE Mono16ToMono8(USHORT Mono16)
{
    return ((Mono16 >> 8) & 0xFF) + 0x80;
}

DWORD ConvertPCMMono8ToMono16(BYTE *lpSource, DWORD dwSrcLength, BYTE *lpDest, DWORD dwDestLength,
                              CONVERSIONDATA *pConversionData)
{
    BYTE *pInput = lpSource + (dwSrcLength - 1);
    USHORT *pOutput = (USHORT *)(lpDest + (dwDestLength - 2));
    DWORD dwLength = dwSrcLength;

    DWORD RateValue = (1 << (abs((int)pConversionData->rateFactor)));

    DPRINTF(3, "ConvertPCMMono8ToMono16 dwSrcLength = <%ld> dwDestLength = <%ld> RateFactor = <%ld>", dwSrcLength,
            dwDestLength, pConversionData->rateFactor);

    assert((dwDestLength & 1) == 0);

    if (pConversionData->rateFactor == 0)
    {
        // No rate conversion
        while (dwLength != 0)
        {
            *pOutput-- = Mono8ToMono16(*pInput--);
            dwLength--;
        }
    }
    else if (pConversionData->rateFactor < 0)
    {
        // Downsampling
        dwLength = dwSrcLength >> ((abs((int)pConversionData->rateFactor)) & 0x1F);
        while (dwLength != 0)
        {
            *pOutput-- = Mono8ToMono16(*pInput--);
            pInput -= (RateValue - 1);
            dwLength--;
        }
    }
    else
    {
        // Upsampling
        while (dwLength != 0)
        {
            USHORT value = Mono8ToMono16(*pInput--);
            DWORD Rate = RateValue;
            while (Rate != 0)
            {
                *pOutput-- = value;
                Rate--;
            }
            dwLength--;
        }
    }

    return dwDestLength;
}

DWORD ConvertPCMMono8ToStereo16(BYTE *lpSource, DWORD dwSrcLength, BYTE *lpDest, DWORD dwDestLength,
                                CONVERSIONDATA *pConversionData)
{
    BYTE *pInput = lpSource + (dwSrcLength - 1);
    UINT *pOutput = (UINT *)(lpDest + (dwDestLength - 4));
    DWORD dwLength = dwSrcLength;

    DWORD RateValue = (1 << (abs((int)pConversionData->rateFactor)));

    DPRINTF(3, "ConvertPCMMono8ToStereo16 dwSrcLength = <%ld> dwDestLength = <%ld> RateFactor = <%ld>", dwSrcLength,
            dwDestLength, pConversionData->rateFactor);

    assert((dwDestLength & 3) == 0);

    if (pConversionData->rateFactor == 0)
    {
        // No rate conversion
        while (dwLength != 0)
        {
            *pOutput-- = Mono16ToStereo16(Mono8ToMono16(*pInput--));
            dwLength--;
        }
    }
    else if (pConversionData->rateFactor < 0)
    {
        // Downsampling
        dwLength = dwSrcLength >> ((abs((int)pConversionData->rateFactor)) & 0x1F);
        while (dwLength != 0)
        {
            *pOutput-- = Mono16ToStereo16(Mono8ToMono16(*pInput--));
            pInput -= (RateValue - 1);
            dwLength--;
        }
    }
    else
    {
        // Upsampling
        while (dwLength != 0)
        {
            UINT value = Mono16ToStereo16(Mono8ToMono16(*pInput--));
            DWORD Rate = RateValue;
            while (Rate != 0)
            {
                *pOutput-- = value;
                Rate--;
            }
            dwLength--;
        }
    }

    return dwDestLength;
}

// Convert using ACM
DWORD ConvertPCMGeneric(BYTE *lpSrc, DWORD cbSrc, BYTE *lpDst, DWORD cbDst, CONVERSIONDATA *lpConversionData)
{
    ACMSTREAMHEADER streamHeader = {0};

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
    acmStreamConvert(lpConversionData->acmStream, &streamHeader, ACM_STREAMCONVERTF_BLOCKALIGN);
    acmStreamUnprepareHeader(lpConversionData->acmStream, &streamHeader, 0);

    return streamHeader.cbDstLengthUsed;
}

PCMConversionFn GetPCMConvertProc(WAVEFORMATEX *pWFExSource, WAVEFORMATEX *pWFExDest, CONVERSIONDATA *pConversionData)
{
    // FUTURE: Implement the rest of the PCM conversions here
    // Currently we only support some of them, and delegate conversion to ACM when unsupported.

    PCMConversionFn pfnConvert = NULL;

    HACMSTREAM acmStream = NULL;

    INT *pBuffer = &pConversionData->buffer;
    INT *pRateFactor = &pConversionData->rateFactor;

    if (pWFExSource->nSamplesPerSec == 11025)
    {
        if (pWFExDest->nSamplesPerSec == 22050)
        {
            *pBuffer -= 1;
            *pRateFactor = 1;
        }
        else if (pWFExDest->nSamplesPerSec == 44100)
        {
            *pBuffer -= 2;
            *pRateFactor = 2;
        }
    }
    else if (pWFExSource->nSamplesPerSec == 22050)
    {
        if (pWFExDest->nSamplesPerSec == 11025)
        {
            *pBuffer += 1;
            *pRateFactor = -1;
        }
        else if (pWFExDest->nSamplesPerSec == 44100)
        {
            *pBuffer -= 1;
            *pRateFactor = 1;
        }
    }
    else if (pWFExSource->nSamplesPerSec == 44100)
    {
        if (pWFExDest->nSamplesPerSec == 11025)
        {
            *pBuffer += 2;
            *pRateFactor = -2;
        }
        else if (pWFExDest->nSamplesPerSec == 22050)
        {
            *pBuffer += 1;
            *pRateFactor = -1;
        }
    }
    else
    {
        pConversionData->UseACM = TRUE;
        pConversionData->buffer = 0;
    }

    if (pWFExSource->wBitsPerSample < pWFExDest->wBitsPerSample)
    {
        *pBuffer -= 1;
    }
    else if (pWFExDest->wBitsPerSample < pWFExSource->wBitsPerSample)
    {
        *pBuffer += 1;
    }
    if (pWFExDest->nChannels < pWFExSource->nChannels)
    {
        *pBuffer += 1;
    }
    else if (pWFExSource->nChannels < pWFExDest->nChannels)
    {
        *pBuffer -= 1;
    }

    // Find a conversion function for the source and destination format
    if (pWFExSource->nChannels == 1)
    {
        if (pWFExSource->wBitsPerSample == 8)
        {
            if (pWFExDest->nChannels == 1)
            {
                if (pWFExDest->wBitsPerSample == 8)
                {
                    DPRINTF(3, "CalculateFactors() ConvertPCMMono8ToMono8 %ld=>%ld Rate=%d Buffer=%d",
                            pWFExSource->nSamplesPerSec, pWFExDest->nSamplesPerSec, *pRateFactor, *pBuffer);
                    // not implemented
                }
                else if (pWFExDest->wBitsPerSample == 16)
                {
                    DPRINTF(3, "CalculateFactors() ConvertPCMMono8ToMono16 %ld=>%ld Rate=%d Buffer=%d",
                            pWFExSource->nSamplesPerSec, pWFExDest->nSamplesPerSec, *pRateFactor, *pBuffer);
                    pfnConvert = ConvertPCMMono8ToMono16;
                }
            }
            else if (pWFExDest->nChannels == 2)
            {
                if (pWFExDest->wBitsPerSample == 8)
                {
                    DPRINTF(3, "CalculateFactors() ConvertPCMMono8ToStereo8 %ld=>%ld Rate=%d Buffer=%d",
                            pWFExSource->nSamplesPerSec, pWFExDest->nSamplesPerSec, *pRateFactor, *pBuffer);
                    // not implemented
                }
                else if (pWFExDest->wBitsPerSample == 16)
                {
                    DPRINTF(3, "CalculateFactors() ConvertPCMMono8ToStereo16 %ld=>%ld Rate=%d Buffer=%d",
                            pWFExSource->nSamplesPerSec, pWFExDest->nSamplesPerSec, *pRateFactor, *pBuffer);
                    pfnConvert = ConvertPCMMono8ToStereo16;
                }
            }
        }
        else if (pWFExSource->wBitsPerSample == 16)
        {
            if (pWFExDest->nChannels == 1)
            {
                if (pWFExDest->wBitsPerSample == 8)
                {
                    DPRINTF(3, "CalculateFactors() ConvertPCMMono16ToMono8 %ld=>%ld Rate=%d Buffer=%d",
                            pWFExSource->nSamplesPerSec, pWFExDest->nSamplesPerSec, *pRateFactor, *pBuffer);
                    // not implemented
                }
                else if (pWFExDest->wBitsPerSample == 16)
                {
                    DPRINTF(3, "CalculateFactors() ConvertPCMMono16ToMono16 %ld=>%ld Rate=%d Buffer=%d",
                            pWFExSource->nSamplesPerSec, pWFExDest->nSamplesPerSec, *pRateFactor, *pBuffer);
                    // not implemented
                }
            }
            else if (pWFExDest->nChannels == 2)
            {
                if (pWFExDest->wBitsPerSample == 8)
                {
                    DPRINTF(3, "CalculateFactors() ConvertPCMMono16ToStereo8 %ld=>%ld Rate=%d Buffer=%d",
                            pWFExSource->nSamplesPerSec, pWFExDest->nSamplesPerSec, *pRateFactor, *pBuffer);
                    // not implemented
                }
                else if (pWFExDest->wBitsPerSample == 16)
                {
                    DPRINTF(3, "CalculateFactors() ConvertPCMMono16ToStereo16 %ld=>%ld Rate=%d Buffer=%d",
                            pWFExSource->nSamplesPerSec, pWFExDest->nSamplesPerSec, *pRateFactor, *pBuffer);
                    // not implemented
                }
            }
        }
    }
    else if (pWFExSource->nChannels == 2)
    {
        if (pWFExSource->wBitsPerSample == 8)
        {
            if (pWFExDest->nChannels == 1)
            {
                if (pWFExDest->wBitsPerSample == 8)
                {
                    DPRINTF(3, "CalculateFactors() ConvertPCMStereo8ToMono8 %ld=>%ld Rate=%d Buffer=%d",
                            pWFExSource->nSamplesPerSec, pWFExDest->nSamplesPerSec, *pRateFactor, *pBuffer);
                    // not implemented
                }
                else if (pWFExDest->wBitsPerSample == 16)
                {
                    DPRINTF(3, "CalculateFactors() ConvertPCMStereo8ToMono16 %ld=>%ld Rate=%d Buffer=%d",
                            pWFExSource->nSamplesPerSec, pWFExDest->nSamplesPerSec, *pRateFactor, *pBuffer);
                    // not implemented
                }
            }
            else if (pWFExDest->nChannels == 2)
            {
                if (pWFExDest->wBitsPerSample == 8)
                {
                    DPRINTF(3, "CalculateFactors() ConvertPCMStereo8ToStereo8 %ld=>%ld Rate=%d Buffer=%d",
                            pWFExSource->nSamplesPerSec, pWFExDest->nSamplesPerSec, *pRateFactor, *pBuffer);
                    // not implemented
                }
                else if (pWFExDest->wBitsPerSample == 16)
                {
                    DPRINTF(3, "CalculateFactors() ConvertPCMStereo8ToStereo16 %ld=>%ld Rate=%d Buffer=%d",
                            pWFExSource->nSamplesPerSec, pWFExDest->nSamplesPerSec, *pRateFactor, *pBuffer);
                    // not implemented
                }
            }
        }
        else if (pWFExSource->wBitsPerSample == 16)
        {
            if (pWFExDest->nChannels == 1)
            {
                if (pWFExDest->wBitsPerSample == 8)
                {
                    DPRINTF(3, "CalculateFactors() ConvertPCMStereo16ToMono8 %ld=>%ld Rate=%d Buffer=%d",
                            pWFExSource->nSamplesPerSec, pWFExDest->nSamplesPerSec, *pRateFactor, *pBuffer);
                    // not implemented
                }
                else if (pWFExDest->wBitsPerSample == 16)
                {
                    DPRINTF(3, "CalculateFactors() ConvertPCMStereo16ToMono16 %ld=>%ld Rate=%d Buffer=%d",
                            pWFExSource->nSamplesPerSec, pWFExDest->nSamplesPerSec, *pRateFactor, *pBuffer);
                    // not implemented
                }
            }
            else if (pWFExDest->nChannels == 2)
            {
                if (pWFExDest->wBitsPerSample == 8)
                {
                    DPRINTF(3, "CalculateFactors() ConvertPCMStereo16ToStereo8 %ld=>%ld Rate=%d Buffer=%d",
                            pWFExSource->nSamplesPerSec, pWFExDest->nSamplesPerSec, *pRateFactor, *pBuffer);
                    // not implemented
                }
                else if (pWFExDest->wBitsPerSample == 16)
                {
                    DPRINTF(3, "CalculateFactors() ConvertPCMStereo16ToStereo16 %ld=>%ld Rate=%d Buffer=%d",
                            pWFExSource->nSamplesPerSec, pWFExDest->nSamplesPerSec, *pRateFactor, *pBuffer);
                    // not implemented
                }
            }
        }
    }

    if (pfnConvert == NULL || pConversionData->UseACM)
    {
        // Try using ACM instead
        if (acmStreamOpen(&acmStream, 0, pWFExSource, pWFExDest, 0, 0, 0, 0) == 0 ||
            acmStreamOpen(&acmStream, 0, pWFExSource, pWFExDest, 0, 0, 0, ACM_STREAMOPENF_NONREALTIME) == 0)
        {
            pConversionData->UseACM = TRUE;
            pConversionData->acmStream = acmStream;
            pfnConvert = ConvertPCMGeneric;
        }
    }

    if (pfnConvert == NULL)
    {
        DPRINTF(0, "NON SUPPORTED FORMAT!");
        TODO_NOT_IMPLEMENTED;
    }

    return pfnConvert;
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
    DWORD srcSize = 0;

    if (lpConversionData->UseACM == FALSE)
    {
        if (lpConversionData->buffer < 1)
        {
            srcSize = dwDestSize >> (abs((int)lpConversionData->buffer) & 0x1F);
        }
        else
        {
            srcSize = dwDestSize << (((BYTE)lpConversionData->buffer) & 0x1F);
        }
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
    DWORD destSize = 0;

    if (lpConversionData->UseACM == FALSE)
    {
        if (lpConversionData->buffer < 1)
        {
            destSize = dwSourceSize << (abs((int)lpConversionData->buffer) & 0x1F);
        }
        else
        {
            destSize = dwSourceSize >> ((BYTE)lpConversionData->buffer & 0x1F);
        }
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
