/**
 * Loop filter
 **/
#pragma once
#include "audiomaninternal.h"
#include "sndpass.h"

class CAMLoopFilter : public CAMPassThruFilter, public IAMLoopFilter
{
  public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IAMSound methods
    STDMETHOD_(DWORD, GetSamples)();
    STDMETHOD(GetAlignment)(LPDWORD lpdwLeftAlign, LPDWORD lpdwRightAlign);
    STDMETHOD(GetSampleData)
    (LPBYTE lpBuffer, DWORD dwPosition, LPDWORD lpdwSamples, LPREQUESTPARAM lpRequestParams);

    // IAMLoopFilter methods
    STDMETHOD(Init)(THIS_ LPSOUND pSoundSrc, DWORD dwLoops);

    virtual ~CAMLoopFilter();

  protected:
    DWORD m_TotalSamples = 0;
    WAVEFORMATEX m_wfxSrc = {0};
    DWORD m_LoopsRemaining = 0;
    DWORD m_LoopsTotal = 0;
};