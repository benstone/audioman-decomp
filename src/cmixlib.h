#pragma once
#include "audiomaninternal.h"

/**
 * Function that mixes audio
 * @param dwVolume volume of the source sound
 * @param lpDst Destination sample buffer
 * @param hpSrc Source sample buffer
 * @param cb Size of source buffer
 * @param lpAuxBuffer Auxillary buffer
 */
typedef void(__stdcall *MixerProc)(DWORD dwVolume, BYTE *lpDst, BYTE *hpSrc, DWORD cb, BYTE *lpAuxBuffer);

/**
 * Returns a pointer to a function that mixes audio samples
 * @param dwVolume Channel volume
 * @param wBitsPerSample Bits per sample (8 or 16)
 * @param fVolOnly
 **/
MixerProc CalcMixerProc(DWORD dwVolume, WORD wBitsPerSample, BOOL fVolOnly);

DWORD CalcIntVolumeFromOldStyle(DWORD dwVolume, WAVEFORMATEX *p2, WAVEFORMATEX *p3);