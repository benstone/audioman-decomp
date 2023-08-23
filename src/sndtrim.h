/**
 * Trim filter
 **/
#pragma once
#include "audiomaninternal.h"
#include "sndpass.h"

class CAMTrimFilter : public CAMPassThruFilter, public IAMTrimFilter
{
  public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IAMSound methods

    // IAMTrimFilter methods
    STDMETHOD(Init)(THIS_ LPSOUND pSound);

    virtual ~CAMTrimFilter();

  private:
    /**
     * Find the start and end of the given sound
     * @param pSound sound to analyze
     * @param lpdwFirst set to the start of the sound in samples
     * @param lpdwLast set to the end of the sound in samples
     **/
    HRESULT CalcTrimSamples(LPSOUND pSound, LPDWORD lpdwFirst, LPDWORD lpdwLast);
};