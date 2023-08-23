/**
 * Pass-through filter base class
 **/
#pragma once
#include "audiomaninternal.h"

class CAMPassThruFilter : public IAMSound
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

    virtual ~CAMPassThruFilter();

  protected:
    ULONG m_RefCnt = 0;
    BOOL m_Initialized = FALSE;
    LPSOUND m_Sound = NULL;
    LONG m_ActiveCount = 0;
};