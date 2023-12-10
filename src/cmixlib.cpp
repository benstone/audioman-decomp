#include "cmixlib.h"
#include "dpf.h"
#include "todo.h"
#include <cassert>
#include <intrin.h>

void __stdcall slowMixSixteen(short *output, short *input, DWORD cbInput, BYTE *unused)
{
    long lSample;

    cbInput = cbInput >> 1;
    while (cbInput != 0)
    {
        lSample = *input + *output;
        input = input + 1;

        if ((lSample + 0x8000U) >> 16 != 0)
        {
            if (lSample < -0x8000)
            {
                lSample = -0x8000;
            }
            else if (0x7fff < lSample)
            {
                lSample = 0x7fff;
            }
        }

        *output = (short)lSample;
        output = output + 1;
        cbInput = cbInput - 1;
    }
    return;
}

void __stdcall MixSixteen(short *output, short *input, DWORD cbInput, BYTE *unused)
{
    // FUTURE: The 16-bit and 8-bit mixing functions (Sixteen and Eight) appear to be hand-optimized assembly.
    // The static lib includes "slow" C implementations that we will use instead.
    // Port the Sixteen and Eight functions later if we need them.

    slowMixSixteen(output, input, cbInput, unused);
}

void __stdcall Sixteen(DWORD dwVolume, BYTE *lpDst, BYTE *hpSrc, DWORD cb, BYTE *lpAuxBuffer)
{
    DPRINTF(2, "Sixteen");
    MixSixteen((short *)lpDst, (short *)hpSrc, cb, NULL);
}

void __stdcall VolAdjustSixteen(DWORD dwVolume, BYTE *lpDstPtr, BYTE *hpSrcPtr, DWORD cb, BYTE *lpAuxBuffer)
{
    SHORT *lpDst = (SHORT *)lpDstPtr;
    SHORT *hpSrc = (SHORT *)hpSrcPtr;

    assert((dwVolume & 0xFFFF) != 0x10 && (dwVolume & 0xFFFF) != 0);

    cb = cb / 2;

    switch (dwVolume & 0xFFFF)
    {
    case 1:
        while (cb != 0)
        {
            *lpDst = *hpSrc >> 4;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 2:
        while (cb != 0)
        {
            *lpDst = *hpSrc >> 3;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 3:
        while (cb != 0)
        {
            *lpDst = (*hpSrc >> 3) + (*hpSrc >> 4);
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 4:
        while (cb != 0)
        {
            *lpDst = *hpSrc >> 2;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 5:
        while (cb != 0)
        {
            *lpDst = (*hpSrc >> 2) + (*hpSrc >> 4);
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 6:
        while (cb != 0)
        {
            *lpDst = (*hpSrc >> 1) - (*hpSrc >> 3);
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 7:
        while (cb != 0)
        {
            *lpDst = (*hpSrc >> 1) - (*hpSrc >> 4);
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 8:
        while (cb != 0)
        {
            *lpDst = *hpSrc >> 1;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 9:
        while (cb != 0)
        {
            *lpDst = (*hpSrc >> 1) + (*hpSrc >> 4);
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0xa:
        while (cb != 0)
        {
            *lpDst = (*hpSrc >> 1) + (*hpSrc >> 3);
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0xb:
        while (cb != 0)
        {
            *lpDst = (*hpSrc - (*hpSrc >> 2)) + (*hpSrc >> 4);
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0xc:
        while (cb != 0)
        {
            *lpDst = *hpSrc - (*hpSrc >> 2);
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0xd:
        while (cb != 0)
        {
            *lpDst = (*hpSrc - (*hpSrc >> 3)) + (*hpSrc >> 4);
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0xe:
        while (cb != 0)
        {
            *lpDst = *hpSrc - (*hpSrc >> 3);
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0xf:
        while (cb != 0)
        {
            *lpDst = *hpSrc - (*hpSrc >> 4);
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
        // no case 0x10
    case 0x11:
        while (cb != 0)
        {
            LONG x = ((int)*hpSrc >> 3) + ((int)*hpSrc >> 4) + (int)*hpSrc;
            if ((x + 0x8000U) >> 0x10 != 0)
            {
                if (x < -0x8000)
                {
                    x = -0x8000;
                }
                else if (0x7fff < x)
                {
                    x = 0x7fff;
                }
            }
            *lpDst = (short)x;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0x12:
        while (cb != 0)
        {
            LONG x = ((int)*hpSrc >> 2) + (int)*hpSrc;
            if ((x + 0x8000U) >> 0x10 != 0)
            {
                if (x < -0x8000)
                {
                    x = -0x8000;
                }
                else if (0x7fff < x)
                {
                    x = 0x7fff;
                }
            }
            *lpDst = (short)x;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0x13:
        while (cb != 0)
        {
            LONG x = (((int)*hpSrc >> 1) + (int)*hpSrc) - ((int)*hpSrc >> 3);
            if ((x + 0x8000U) >> 0x10 != 0)
            {
                if (x < -0x8000)
                {
                    x = -0x8000;
                }
                else if (0x7fff < x)
                {
                    x = 0x7fff;
                }
            }
            *lpDst = (short)x;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0x14:
        while (cb != 0)
        {
            LONG x = ((int)*hpSrc >> 1) + (int)*hpSrc;
            if ((x + 0x8000U) >> 0x10 != 0)
            {
                if (x < -0x8000)
                {
                    x = -0x8000;
                }
                else if (0x7fff < x)
                {
                    x = 0x7fff;
                }
            }
            *lpDst = (short)x;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0x15:
        while (cb != 0)
        {
            LONG x = ((int)*hpSrc >> 1) + ((int)*hpSrc >> 3) + (int)*hpSrc;
            if ((x + 0x8000U) >> 0x10 != 0)
            {
                if (x < -0x8000)
                {
                    x = -0x8000;
                }
                else if (0x7fff < x)
                {
                    x = 0x7fff;
                }
            }
            *lpDst = (short)x;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0x16:
        while (cb != 0)
        {
            LONG x = *hpSrc * 2 - ((int)*hpSrc >> 2);
            if ((x + 0x8000U) >> 0x10 != 0)
            {
                if (x < -0x8000)
                {
                    x = -0x8000;
                }
                else if (0x7fff < x)
                {
                    x = 0x7fff;
                }
            }
            *lpDst = (short)x;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0x17:
        while (cb != 0)
        {
            LONG x = *hpSrc * 2 - ((int)*hpSrc >> 3);
            if ((x + 0x8000U) >> 0x10 != 0)
            {
                if (x < -0x8000)
                {
                    x = -0x8000;
                }
                else if (0x7fff < x)
                {
                    x = 0x7fff;
                }
            }
            *lpDst = (short)x;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0x18:
        while (cb != 0)
        {
            LONG x = *hpSrc * 2;
            if ((x + 0x8000U) >> 0x10 != 0)
            {
                if (x < -0x8000)
                {
                    x = -0x8000;
                }
                else if (0x7fff < x)
                {
                    x = 0x7fff;
                }
            }
            *lpDst = (short)x;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0x19:
        while (cb != 0)
        {
            LONG x = ((int)*hpSrc >> 3) + *hpSrc * 2;
            if ((x + 0x8000U) >> 0x10 != 0)
            {
                if (x < -0x8000)
                {
                    x = -0x8000;
                }
                else if (0x7fff < x)
                {
                    x = 0x7fff;
                }
            }
            *lpDst = (short)x;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0x1a:
        while (cb != 0)
        {
            LONG x = ((int)*hpSrc >> 1) + *hpSrc * 2;
            if ((x + 0x8000U) >> 0x10 != 0)
            {
                if (x < -0x8000)
                {
                    x = -0x8000;
                }
                else if (0x7fff < x)
                {
                    x = 0x7fff;
                }
            }
            *lpDst = (short)x;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0x1b:
        while (cb != 0)
        {
            LONG x = ((int)*hpSrc + *hpSrc * 2) - ((int)*hpSrc >> 2);
            if ((x + 0x8000U) >> 0x10 != 0)
            {
                if (x < -0x8000)
                {
                    x = -0x8000;
                }
                else if (0x7fff < x)
                {
                    x = 0x7fff;
                }
            }
            *lpDst = (short)x;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0x1c:
        while (cb != 0)
        {
            LONG x = (int)*hpSrc + *hpSrc * 2;
            if ((x + 0x8000U) >> 0x10 != 0)
            {
                if (x < -0x8000)
                {
                    x = -0x8000;
                }
                else if (0x7fff < x)
                {
                    x = 0x7fff;
                }
            }
            *lpDst = (short)x;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0x1d:
        while (cb != 0)
        {
            LONG x = ((int)*hpSrc >> 1) + *hpSrc * 2 + (int)*hpSrc;
            if ((x + 0x8000U) >> 0x10 != 0)
            {
                if (x < -0x8000)
                {
                    x = -0x8000;
                }
                else if (0x7fff < x)
                {
                    x = 0x7fff;
                }
            }
            *lpDst = (short)x;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0x1e:
        while (cb != 0)
        {
            LONG x = *hpSrc * 4;
            if ((x + 0x8000U) >> 0x10 != 0)
            {
                if (x < -0x8000)
                {
                    x = -0x8000;
                }
                else if (0x7fff < x)
                {
                    x = 0x7fff;
                }
            }
            *lpDst = (short)x;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0x1f:
        while (cb != 0)
        {
            LONG x = (int)*hpSrc + *hpSrc * 4;
            if ((x + 0x8000U) >> 0x10 != 0)
            {
                if (x < -0x8000)
                {
                    x = -0x8000;
                }
                else if (0x7fff < x)
                {
                    x = 0x7fff;
                }
            }
            *lpDst = (short)x;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
        break;
    case 0x20:
        while (cb != 0)
        {
            LONG x = *hpSrc * 4 + *hpSrc * 2;
            if ((x + 0x8000U) >> 0x10 != 0)
            {
                if (x < -0x8000)
                {
                    x = -0x8000;
                }
                else if (0x7fff < x)
                {
                    x = 0x7fff;
                }
            }
            *lpDst = (short)x;
            hpSrc = hpSrc + 1;
            lpDst = lpDst + 1;
            cb = cb - 1;
        }
    default:
        // do nothing
        break;
    }
}

void __stdcall SixteenVol(DWORD dwVolume, BYTE *lpDst, BYTE *hpSrc, DWORD cb, BYTE *lpAuxBuffer)
{
    DPRINTF(2, "SixteenVol");

    if (lpAuxBuffer == NULL)
    {
        lpAuxBuffer = hpSrc;
    }

    VolAdjustSixteen(dwVolume, lpAuxBuffer, hpSrc, cb, NULL);
    MixSixteen((short *)lpDst, (short *)lpAuxBuffer, cb, NULL);
}

void __stdcall PanVolAdjustSixteen(DWORD dwVolume, BYTE *lpDst, BYTE *hpSrc, DWORD cb, BYTE *lpAuxBuffer)
{
    DWORD iStereo = 2;
    DWORD cSamples;
    SHORT *hpSrcSample;
    SHORT *lpDstSample;

    while (iStereo--)
    {
        dwVolume = _lrotl(dwVolume, 16);

        hpSrcSample = (SHORT *)(hpSrc + iStereo * 2);
        lpDstSample = (SHORT *)(lpDst + iStereo * 2);
        cSamples = cb / 4;

        switch (dwVolume & 0xFFFF)
        {
        case 0:
            while (cSamples != 0)
            {
                *lpDstSample = 0;
                lpDstSample += 2;
                cSamples--;
            }
            break;
        case 1:
            while (cSamples != 0)
            {
                *lpDstSample = *hpSrcSample >> 4;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 2:
            while (cSamples != 0)
            {
                *lpDstSample = *hpSrcSample >> 3;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 3:
            while (cSamples != 0)
            {
                *lpDstSample = (*hpSrcSample >> 3) + (*hpSrcSample >> 4);
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 4:
            while (cSamples != 0)
            {
                *lpDstSample = *hpSrcSample >> 2;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 5:
            while (cSamples != 0)
            {
                *lpDstSample = (*hpSrcSample >> 2) + (*hpSrcSample >> 4);
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 6:
            while (cSamples != 0)
            {
                *lpDstSample = (*hpSrcSample >> 1) - (*hpSrcSample >> 3);
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 7:
            while (cSamples != 0)
            {
                *lpDstSample = (*hpSrcSample >> 1) - (*hpSrcSample >> 4);
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 8:
            while (cSamples != 0)
            {
                *lpDstSample = *hpSrcSample >> 1;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 9:
            while (cSamples != 0)
            {
                *lpDstSample = (*hpSrcSample >> 1) + (*hpSrcSample >> 4);
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 10:
            while (cSamples != 0)
            {
                *lpDstSample = (*hpSrcSample >> 1) + (*hpSrcSample >> 3);
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0xb:
            while (cSamples != 0)
            {
                *lpDstSample = (*hpSrcSample - (*hpSrcSample >> 2)) + (*hpSrcSample >> 4);
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0xc:
            while (cSamples != 0)
            {
                *lpDstSample = *hpSrcSample - (*hpSrcSample >> 2);
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0xd:
            while (cSamples != 0)
            {
                *lpDstSample = (*hpSrcSample - (*hpSrcSample >> 3)) + (*hpSrcSample >> 4);
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0xe:
            while (cSamples != 0)
            {
                *lpDstSample = *hpSrcSample - (*hpSrcSample >> 3);
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0xf:
            while (cSamples != 0)
            {
                *lpDstSample = *hpSrcSample - (*hpSrcSample >> 4);
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0x10:
            while (cSamples != 0)
            {
                *lpDstSample = *hpSrcSample;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0x11:
            while (cSamples != 0)
            {
                LONG x = ((int)*hpSrcSample >> 3) + ((int)*hpSrcSample >> 4) + (int)*hpSrcSample;
                if ((x + 0x8000U) >> 0x10 != 0)
                {
                    if (x < -0x8000)
                    {
                        x = -0x8000;
                    }
                    else if (0x7fff < x)
                    {
                        x = 0x7fff;
                    }
                }
                *lpDstSample = (short)x;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0x12:
            while (cSamples != 0)
            {
                LONG x = ((int)*hpSrcSample >> 2) + (int)*hpSrcSample;
                if ((x + 0x8000U) >> 0x10 != 0)
                {
                    if (x < -0x8000)
                    {
                        x = -0x8000;
                    }
                    else if (0x7fff < x)
                    {
                        x = 0x7fff;
                    }
                }
                *lpDstSample = (short)x;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0x13:
            while (cSamples != 0)
            {
                LONG x = (((int)*hpSrcSample >> 1) + (int)*hpSrcSample) - ((int)*hpSrcSample >> 3);
                if ((x + 0x8000U) >> 0x10 != 0)
                {
                    if (x < -0x8000)
                    {
                        x = -0x8000;
                    }
                    else if (0x7fff < x)
                    {
                        x = 0x7fff;
                    }
                }
                *lpDstSample = (short)x;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0x14:
            while (cSamples != 0)
            {
                LONG x = ((int)*hpSrcSample >> 1) + (int)*hpSrcSample;
                if ((x + 0x8000U) >> 0x10 != 0)
                {
                    if (x < -0x8000)
                    {
                        x = -0x8000;
                    }
                    else if (0x7fff < x)
                    {
                        x = 0x7fff;
                    }
                }
                *lpDstSample = (short)x;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0x15:
            while (cSamples != 0)
            {
                LONG x = ((int)*hpSrcSample >> 1) + ((int)*hpSrcSample >> 3) + (int)*hpSrcSample;
                if ((x + 0x8000U) >> 0x10 != 0)
                {
                    if (x < -0x8000)
                    {
                        x = -0x8000;
                    }
                    else if (0x7fff < x)
                    {
                        x = 0x7fff;
                    }
                }
                *lpDstSample = (short)x;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0x16:
            while (cSamples != 0)
            {
                LONG x = *hpSrcSample * 2 - ((int)*hpSrcSample >> 2);
                if ((x + 0x8000U) >> 0x10 != 0)
                {
                    if (x < -0x8000)
                    {
                        x = -0x8000;
                    }
                    else if (0x7fff < x)
                    {
                        x = 0x7fff;
                    }
                }
                *lpDstSample = (short)x;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0x17:
            while (cSamples != 0)
            {
                LONG x = *hpSrcSample * 2 - ((int)*hpSrcSample >> 3);
                if ((x + 0x8000U) >> 0x10 != 0)
                {
                    if (x < -0x8000)
                    {
                        x = -0x8000;
                    }
                    else if (0x7fff < x)
                    {
                        x = 0x7fff;
                    }
                }
                *lpDstSample = (short)x;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0x18:
            while (cSamples != 0)
            {
                LONG x = *hpSrcSample * 2;
                if ((x + 0x8000U) >> 0x10 != 0)
                {
                    if (x < -0x8000)
                    {
                        x = -0x8000;
                    }
                    else if (0x7fff < x)
                    {
                        x = 0x7fff;
                    }
                }
                *lpDstSample = (short)x;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0x19:
            while (cSamples != 0)
            {
                LONG x = ((int)*hpSrcSample >> 3) + *hpSrcSample * 2;
                if ((x + 0x8000U) >> 0x10 != 0)
                {
                    if (x < -0x8000)
                    {
                        x = -0x8000;
                    }
                    else if (0x7fff < x)
                    {
                        x = 0x7fff;
                    }
                }
                *lpDstSample = (short)x;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0x1a:
            while (cSamples != 0)
            {
                LONG x = ((int)*hpSrcSample >> 1) + *hpSrcSample * 2;
                if ((x + 0x8000U) >> 0x10 != 0)
                {
                    if (x < -0x8000)
                    {
                        x = -0x8000;
                    }
                    else if (0x7fff < x)
                    {
                        x = 0x7fff;
                    }
                }
                *lpDstSample = (short)x;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0x1b:
            while (cSamples != 0)
            {
                LONG x = ((int)*hpSrcSample + *hpSrcSample * 2) - ((int)*hpSrcSample >> 2);
                if ((x + 0x8000U) >> 0x10 != 0)
                {
                    if (x < -0x8000)
                    {
                        x = -0x8000;
                    }
                    else if (0x7fff < x)
                    {
                        x = 0x7fff;
                    }
                }
                *lpDstSample = (short)x;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0x1c:
            while (cSamples != 0)
            {
                LONG x = (int)*hpSrcSample + *hpSrcSample * 2;
                if ((x + 0x8000U) >> 0x10 != 0)
                {
                    if (x < -0x8000)
                    {
                        x = -0x8000;
                    }
                    else if (0x7fff < x)
                    {
                        x = 0x7fff;
                    }
                }
                *lpDstSample = (short)x;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0x1d:
            while (cSamples != 0)
            {
                LONG x = ((int)*hpSrcSample >> 1) + *hpSrcSample * 2 + (int)*hpSrcSample;
                if ((x + 0x8000U) >> 0x10 != 0)
                {
                    if (x < -0x8000)
                    {
                        x = -0x8000;
                    }
                    else if (0x7fff < x)
                    {
                        x = 0x7fff;
                    }
                }
                *lpDstSample = (short)x;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0x1e:
            while (cSamples != 0)
            {
                LONG x = *hpSrcSample * 4;
                if ((x + 0x8000U) >> 0x10 != 0)
                {
                    if (x < -0x8000)
                    {
                        x = -0x8000;
                    }
                    else if (0x7fff < x)
                    {
                        x = 0x7fff;
                    }
                }
                *lpDstSample = (short)x;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0x1f:
            while (cSamples != 0)
            {
                LONG x = (int)*hpSrcSample + *hpSrcSample * 4;
                if ((x + 0x8000U) >> 0x10 != 0)
                {
                    if (x < -0x8000)
                    {
                        x = -0x8000;
                    }
                    else if (0x7fff < x)
                    {
                        x = 0x7fff;
                    }
                }
                *lpDstSample = (short)x;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        case 0x20:
            while (cSamples != 0)
            {
                LONG x = *hpSrcSample * 2 + *hpSrcSample * 4;
                if ((x + 0x8000U) >> 0x10 != 0)
                {
                    if (x < -0x8000)
                    {
                        x = -0x8000;
                    }
                    else if (0x7fff < x)
                    {
                        x = 0x7fff;
                    }
                }
                *lpDstSample = (short)x;
                hpSrcSample = hpSrcSample + 2;
                lpDstSample = lpDstSample + 2;
                cSamples = cSamples - 1;
            }
            break;
        default:
            // do nothing
            break;
        }
    }
}

void __stdcall SixteenPanVol(DWORD dwVolume, BYTE *lpDst, BYTE *hpSrc, DWORD cb, BYTE *lpAuxBuffer)
{
    DPRINTF(2, "SixteenPanVol");

    if (lpAuxBuffer == NULL)
    {
        lpAuxBuffer = hpSrc;
    }

    PanVolAdjustSixteen(dwVolume, lpAuxBuffer, hpSrc, cb, NULL);
    MixSixteen((short *)lpDst, (short *)lpAuxBuffer, cb, NULL);
}

void __stdcall Muted(DWORD dwVolume, BYTE *lpDst, BYTE *hpSrc, DWORD cb, BYTE *lpAuxBuffer)
{
    DPRINTF(2, "Muted");
    // do nothing
}

MixerProc CalcMixerProc(DWORD dwVolume, WORD wBitsPerSample, BOOL fVolOnly)
{
    MixerProc lpMixerProc = NULL;
    BOOL fNeedVol = TRUE;
    WORD wVolLeft = dwVolume & 0xFFFF;
    WORD wVolRight = dwVolume >> 16;

    if (wVolLeft == 16 && wVolRight == 16)
    {
        fNeedVol = FALSE;
    }
    if (wBitsPerSample == 8)
    {
        TODO_NOT_IMPLEMENTED;
    }
    else
    {
        // 16-bit
        if (fNeedVol)
        {
            if (wVolLeft == wVolRight)
            {
                if (wVolLeft == 0)
                {
                    lpMixerProc = Muted;
                }
                else if (fVolOnly == FALSE)
                {
                    lpMixerProc = SixteenVol;
                }
                else
                {
                    lpMixerProc = VolAdjustSixteen;
                }
            }
            else if (fVolOnly == FALSE)
            {
                lpMixerProc = SixteenPanVol;
            }
            else
            {
                lpMixerProc = PanVolAdjustSixteen;
            }
        }
        else if (fVolOnly == FALSE)
        {
            lpMixerProc = Sixteen;
        }
    }

    return lpMixerProc;
}

DWORD CalcIntVolumeFromOldStyle(DWORD dwVolume, WAVEFORMATEX *lpwfxMixer, WAVEFORMATEX *lpwfxChannel)
{
    WORD wVolLeft;
    WORD wVolRight;

    if ((dwVolume >> 16) == (dwVolume & 0xFFFF) || (lpwfxMixer->nChannels == 1) || (lpwfxChannel->nChannels == 1))
    {
        wVolLeft = (dwVolume & 0xFFFF) / 0xFFF;
        wVolRight = wVolLeft;
    }
    else
    {
        wVolLeft = (dwVolume & 0xFFFF) / 0xFFF;
        wVolRight = (dwVolume >> 16) / 0xFFF;
    }

    return (wVolRight << 16) | (wVolLeft & 0xFFFF);
}
