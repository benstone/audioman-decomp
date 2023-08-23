/**
 * Cache filter
 **/
#pragma once
#include "audiomaninternal.h"

class CAMCacheFilter : public IAMSound, public IAMCacheFilter
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

    // IAMCacheFilter methods
    STDMETHOD(Init)(THIS_ LPSOUND pSoundSrc, CacheConfig *pCacheConfig);

    virtual ~CAMCacheFilter();

  private:
    ULONG m_RefCnt = 0;
    BOOL m_Initialized = FALSE;
    BOOL m_SmallerThanCache = FALSE;
    LPSOUND m_Sound = NULL;
    WAVEFORMATEX m_wfxCache = {0};
    DWORD m_fSrcFormat = 0;

    // Requested cache size in milliseconds. Does not change after init.
    DWORD m_CacheMaxMsec = 0;

    // Requested cache size in samples. Does not change after init.
    DWORD m_CacheMaxSamples = 0;

    // Samples available in the RHS of the circular buffer
    DWORD m_CacheAvailableSamples = 0;

    DWORD m_Thres = 0;

    // Buffer
    BYTE *m_Buffer = NULL;

    // Positions in circular buffer
    DWORD m_StartPos = 0;
    DWORD m_EndPos = 0;

    DWORD m_Remix = 0;

    // Total length of source sound. Does not change.
    DWORD m_SamplesTotal = 0;

    DWORD m_ActiveCount = 0;

    /**
     * Read from the cache into the caller's sample buffer
     * @param lpBuffer buffer to write samples to
     * @param dwSamplesFromCache number of samples to read from cache
     * @param lpRequestParams Read request parameters
     **/
    HRESULT __stdcall FillBufferFromCache(BYTE *lpBuffer, ULONG dwSamplesFromCache, RequestParam *lpRequestParams);

    /**
     * Read from the sound into the cache buffer
     * @param dwPosition position to start reading sounds from
     * @param dwNeededSamples number of samples to read from sound
     **/
    HRESULT __stdcall FillCache(DWORD dwPosition, DWORD dwNeededSamples);

    /**
     * Move forward in circular buffer by the given number of samples.
     **/
    void MoveCacheForward(DWORD dwSamples);

    /**
     * Move backward in circular buffer by the given number of samples.
     **/
    void MoveCacheBackward(DWORD dwNumSamples);
};