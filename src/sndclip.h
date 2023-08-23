/**
 * Clip filter
 **/
#pragma once
#include "audiomaninternal.h"

class CAMClipFilter : public IAMSound, public IAMClipFilter
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

    // IAMClipFilter methods
    STDMETHOD(Init)(THIS_ LPSOUND pSoundSrc, DWORD dwStartSample, DWORD dwEndSample);

    virtual ~CAMClipFilter();

  private:
    ULONG m_RefCnt = 0;
    BOOL m_Initialized = FALSE;
    ULONG m_ActiveCount = 0;
    LPSOUND m_Source = NULL;

    // Clip length in samples
    DWORD m_SamplesClip = 0;

    // Source length in samples
    DWORD m_SamplesSrc = 0;

    // Start position in samples
    DWORD m_StartPos = 0;

    // End position in samples
    DWORD m_EndPos = 0;

    // Current position
    DWORD m_Pos = 0;

    // Sample to use for padding
    BYTE m_Padding = 0;

    // Source format
    WAVEFORMATEX m_wfxSrc = {0};

    /**
     * Ensure start and end position are within the bounds of the source sound
     * @param lpdwStartPos pointer to start position
     * @param lpdwEndPos pointer to end position
     **/
    HRESULT PinSamples(DWORD *lpdwStartPos, DWORD *lpdwEndPos);
};