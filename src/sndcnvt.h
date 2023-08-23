/**
 * Conversion filter
 **/
#pragma once
#include "audiomaninternal.h"

#include <mmreg.h>
#include <MSAcm.h>

#include "convert.h"

class CAMConvertFilter : public IAMSound, public IAMConvertFilter
{
  public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IAMSound methods
    STDMETHOD(GetFormat)(LPWAVEFORMATEX pFormat, DWORD cbSize);
    STDMETHOD_(DWORD, GetSamples)();
    STDMETHOD(GetAlignment)(LPDWORD lpdwLeftAlign, LPDWORD lpdwRightAlign);
    STDMETHOD(GetSampleData)(LPBYTE lpBuffer, DWORD dwPosition, LPDWORD lpdwSamples, LPREQUESTPARAM lpRequestParams);
    STDMETHOD(SetCacheSize)(DWORD dwCacheSize);
    STDMETHOD(SetMode)(BOOL fActive, BOOL fRecurse);

    // IAMConvertFilter methods
    STDMETHOD(Init)(THIS_ LPSOUND pSoundSrc, LPWAVEFORMATEX lpwfxDest);

    virtual ~CAMConvertFilter();

  private:
    ULONG m_RefCnt = 0;
    BOOL m_Initialized = FALSE;

    LPSOUND m_Source = NULL;

    // Format to convert to
    WAVEFORMATEX m_wfxDestFormat = {0};

    // Format to convert from
    WAVEFORMATEX m_wfxSrcFormat = {0};

    // Function that performs the conversion
    PCMConversionFn m_PCMConvertProc = NULL;
    // Parameters for the conversion procedure
    CONVERSIONDATA m_ConversionData = {0};

    // Temp buffer to use for conversion
    BYTE *m_lpvConversionBuffer = NULL;

    // Size of temp buffer
    DWORD m_cbConversionBuffer = 0;

    DWORD m_ActiveCount = 0;

    /**
     * Convert from source samples to destination samples
     **/
    DWORD ConvertToDstSamples(DWORD dwSrcSamples);

    /**
     * Convert from destination samples to source samples
     **/
    DWORD ConvertToSrcSamples(DWORD dwDstSamples);
};