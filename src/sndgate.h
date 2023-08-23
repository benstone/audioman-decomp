/**
 * Gate filter
 **/
#pragma once
#include "audiomaninternal.h"
#include "sndpass.h"

class CAMGateFilter : public CAMPassThruFilter, public IAMGateFilter
{
  public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IAMSound methods
    STDMETHOD(GetSampleData)
    (LPBYTE lpBuffer, DWORD dwPosition, LPDWORD lpdwSamples, LPREQUESTPARAM lpRequestParams);

    // IAMGateFilter methods
    STDMETHOD(Init)(THIS_ LPSOUND pSoundSrc, float flDBGate);

    virtual ~CAMGateFilter();

  private:
    WAVEFORMATEX m_wfxSrc = {0};
    LONG m_GateThreshold = 0;
};