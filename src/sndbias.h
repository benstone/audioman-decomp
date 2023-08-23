/**
 * Bias filter
 **/
#pragma once
#include "audiomaninternal.h"
#include "sndpass.h"

class CAMBiasFilter : public CAMPassThruFilter, public IAMBiasFilter
{
  public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IAMSound methods
    STDMETHOD(GetSampleData)
    (LPBYTE lpBuffer, DWORD dwPosition, LPDWORD lpdwSamples, LPREQUESTPARAM lpRequestParams);

    // IAMBiasFilter methods
    STDMETHOD(Init)(THIS_ LPSOUND pSoundSrc);

    virtual ~CAMBiasFilter();

  private:
    /**
     * Calculate bias to remove from sound
     * @param pSound sound to calculate bias from
     **/
    HRESULT CalcBias(LPSOUND pSound);

    USHORT m_Channels = 0;
    LONG m_Bias = 0;
    BOOL m_Is8Bit = FALSE;
};