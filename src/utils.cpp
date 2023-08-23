#include "utils.h"
#include <cassert>

HRESULT HRESULTFromMMRESULT(MMRESULT mr)
{
    HRESULT hr;

    switch (mr)
    {
    case MMSYSERR_NOERROR:
        hr = S_OK;
    case MMSYSERR_ERROR:
        hr = E_MMSYSERROR;
    case MMSYSERR_BADDEVICEID:
        hr = E_MMSYSBADDEVICEID;
    case MMSYSERR_NOTENABLED:
        hr = E_MMSYSNOTENABLED;
    case MMSYSERR_ALLOCATED:
        hr = E_MMSYSALLOCATED;
    case MMSYSERR_INVALHANDLE:
        hr = E_MMSYSINVALHANDLE;
    case MMSYSERR_NODRIVER:
        hr = E_MMSYSNODRIVER;
    case MMSYSERR_NOMEM:
        hr = E_MMSYSNOMEM;
    case MMSYSERR_NOTSUPPORTED:
        hr = E_MMSYSNOTSUPPORTED;
    case MMSYSERR_BADERRNUM:
        hr = E_MMSYSBADERRNUM;
    case MMSYSERR_INVALFLAG:
        hr = E_MMSYSINVALFLAG;
    case MMSYSERR_INVALPARAM:
        hr = E_MMSYSINVALPARAM;
    case MMSYSERR_HANDLEBUSY:
        hr = E_MMSYSHANDLEBUSY;
    case MMSYSERR_INVALIDALIAS:
        hr = E_MMSYSINVALIDALIAS;
    case MMSYSERR_BADDB:
        hr = E_MMSYSBADDB;
    case MMSYSERR_KEYNOTFOUND:
        hr = E_MMSYSKEYNOTFOUND;
    case MMSYSERR_READERROR:
        hr = E_MMSYSREADERROR;
    case MMSYSERR_WRITEERROR:
        hr = E_MMSYSWRITEERROR;
    case MMSYSERR_DELETEERROR:
        hr = E_MMSYSDELETEERROR;
    case MMSYSERR_VALNOTFOUND:
        hr = E_MMSYSVALNOTFOUND;
    case MMSYSERR_NODRIVERCB:
        hr = E_MMSYSNODRIVERCB;
    default:
        hr = E_FAIL;
    }

    return hr;
}

BOOL ConvertMillisecToSMPTE(SMPTE *lpSMPTE, DWORD dwTime)
{
    DWORD uVar1;
    if (lpSMPTE == NULL)
    {
        return FALSE;
    }
    else if (lpSMPTE->fps == 0)
    {
        return FALSE;
    }
    else
    {
        lpSMPTE->hour = dwTime / 3600000;
        uVar1 = dwTime + lpSMPTE->hour * -3600000;
        lpSMPTE->min = uVar1 / 60000;
        uVar1 = uVar1 + lpSMPTE->min * -60000;
        lpSMPTE->sec = uVar1 / 1000;
        DWORD uVar3 = MulDiv(uVar1 + lpSMPTE->sec * -1000, lpSMPTE->fps, 100000);
        lpSMPTE->frame = uVar3;
        return TRUE;
    }
}

DWORD BytesToSamples(WAVEFORMATEX *lpwfx, DWORD dwBytes)
{
    return dwBytes / lpwfx->nBlockAlign;
}

DWORD SamplesToBytes(WAVEFORMATEX *lpwfx, DWORD dwSamples)
{
    return lpwfx->nBlockAlign * dwSamples;
}

ULONG ConvertWaveFormatExToFormat(WAVEFORMATEX *pWaveFormatEx)
{
    ULONG fmt = WAVE_INVALIDFORMAT;

    if (pWaveFormatEx->wFormatTag == WAVE_FORMAT_PCM)
    {
        fmt = pWaveFormatEx->nSamplesPerSec;
        if (fmt == 11025)
        {
            if (pWaveFormatEx->nChannels == 2)
            {
                if (pWaveFormatEx->wBitsPerSample == 8)
                {
                    fmt = WAVE_FORMAT_1S08;
                }
                else
                {
                    fmt = WAVE_FORMAT_1S16;
                }
            }
            else if (pWaveFormatEx->wBitsPerSample == 8)
            {
                fmt = WAVE_FORMAT_1M08;
            }
            else
            {
                fmt = WAVE_FORMAT_1M16;
            }
        }
        else if (fmt == 22050)
        {
            if (pWaveFormatEx->nChannels == 2)
            {
                if (pWaveFormatEx->wBitsPerSample == 8)
                {
                    fmt = WAVE_FORMAT_2S08;
                }
                else
                {
                    fmt = WAVE_FORMAT_2S16;
                }
            }
            else if (pWaveFormatEx->wBitsPerSample == 8)
            {
                fmt = WAVE_FORMAT_2M08;
            }
            else
            {
                fmt = WAVE_FORMAT_2M16;
            }
        }
        else if (fmt == 44100)
        {
            if (pWaveFormatEx->nChannels == 2)
            {
                if (pWaveFormatEx->wBitsPerSample == 8)
                {
                    fmt = WAVE_FORMAT_4S08;
                }
                else
                {
                    fmt = WAVE_FORMAT_4S16;
                }
            }
            else if (pWaveFormatEx->wBitsPerSample == 8)
            {
                fmt = WAVE_FORMAT_44M08;
            }
            else
            {
                fmt = WAVE_FORMAT_44M16;
            }
        }
    }

    return fmt;
}

BOOL ConvertFormatToWaveFormatEx(WAVEFORMATEX *pWaveFormatEx, DWORD dwFormat)
{
    pWaveFormatEx->wFormatTag = WAVE_FORMAT_PCM;
    pWaveFormatEx->cbSize = 0;

    if ((dwFormat & 0xaaa) == 0)
    {
        pWaveFormatEx->nChannels = 1;
    }
    else
    {
        pWaveFormatEx->nChannels = 2;
    }
    if ((dwFormat & 0xf) != 0)
    {
        pWaveFormatEx->nSamplesPerSec = 11025;
    }
    if ((dwFormat & 0xf0) != 0)
    {
        pWaveFormatEx->nSamplesPerSec = 22050;
    }
    if ((dwFormat & 0xf00) != 0)
    {
        pWaveFormatEx->nSamplesPerSec = 44100;
    }
    if ((dwFormat & 0x333) == 0)
    {
        pWaveFormatEx->wBitsPerSample = 16;
    }
    else
    {
        pWaveFormatEx->wBitsPerSample = 8;
    }

    pWaveFormatEx->nBlockAlign = (pWaveFormatEx->nChannels * pWaveFormatEx->wBitsPerSample) / 8;
    pWaveFormatEx->nAvgBytesPerSec = pWaveFormatEx->nBlockAlign * pWaveFormatEx->nSamplesPerSec;

    return TRUE;
}

DWORD MillisecToBytes(WAVEFORMATEX *lpwfx, DWORD dwMillisec)
{
    WORD nBlockAlign = lpwfx->nBlockAlign;

    DWORD dwBytes = MulDiv(dwMillisec, lpwfx->nAvgBytesPerSec, 1000);

    return (dwBytes >> ((nBlockAlign >> 1) & 0x1F)) << ((nBlockAlign >> 1) & 0x1F);
}

DWORD BytesToMillisec(WAVEFORMATEX *lpwfx, DWORD dwBytes)
{
    return MulDiv(dwBytes, 1000, lpwfx->nAvgBytesPerSec);
}

DWORD MillisecToSamples(WAVEFORMATEX *lpwfx, DWORD dwTime)
{
    return MillisecToBytes(lpwfx, dwTime) / lpwfx->nBlockAlign;
}

DWORD SamplesToMillisec(WAVEFORMATEX *lpwfx, DWORD dwSamples)
{
    return BytesToMillisec(lpwfx, SamplesToBytes(lpwfx, dwSamples));
}

BOOL SameFormats(WAVEFORMATEX *lpFormatA, WAVEFORMATEX *lpFormatB)
{
    BOOL fSame = FALSE;

    if (lpFormatA->wFormatTag == lpFormatB->wFormatTag && lpFormatA->nChannels == lpFormatB->nChannels &&
        lpFormatA->nSamplesPerSec == lpFormatB->nSamplesPerSec &&
        lpFormatA->nAvgBytesPerSec == lpFormatB->nAvgBytesPerSec && lpFormatA->nBlockAlign == lpFormatB->nBlockAlign &&
        lpFormatA->wBitsPerSample == lpFormatB->wBitsPerSample)
    {
        fSame = TRUE;
    }
    return fSame;
}
