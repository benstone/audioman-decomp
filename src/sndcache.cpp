#include "sndcache.h"
#include "utils.h"
#include "dpf.h"
#include <cassert>

STDMETHODIMP_(ULONG) CAMCacheFilter::AddRef()
{
    return ++m_RefCnt;
}

STDMETHODIMP_(ULONG) CAMCacheFilter::Release()
{
    ULONG RefCnt = --m_RefCnt;

    if (RefCnt == 0)
    {
        delete this;
    }

    return RefCnt;
}

STDMETHODIMP CAMCacheFilter::QueryInterface(REFIID riid, void **ppvObject)
{
    assert(ppvObject != NULL);

    HRESULT hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAMSound))
    {
        *ppvObject = (IAMSound *)this;
    }
    else if (IsEqualIID(riid, IID_IAMCacheFilter))
    {
        *ppvObject = (IAMCacheFilter *)this;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    if (SUCCEEDED(hr))
    {
        AddRef();
    }

    return hr;
}

STDMETHODIMP CAMCacheFilter::GetFormat(LPWAVEFORMATEX pFormat, DWORD cbSize)
{
    HRESULT hr = S_OK;

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else
    {
        hr = m_Sound->GetFormat(pFormat, cbSize);
    }
    return hr;
}

STDMETHODIMP_(DWORD) CAMCacheFilter::GetSamples()
{
    DWORD samples = 0;

    if (m_Initialized != FALSE)
    {
        return m_SamplesTotal;
    }

    return samples;
}

STDMETHODIMP CAMCacheFilter::GetAlignment(LPDWORD lpdwLeftAlign, LPDWORD lpdwRightAlign)
{
    HRESULT hr = S_OK;

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else
    {
        hr = m_Sound->GetAlignment(lpdwLeftAlign, lpdwRightAlign);
    }
    return hr;
}

STDMETHODIMP CAMCacheFilter::GetSampleData(LPBYTE lpBuffer, DWORD dwPosition, LPDWORD lpdwSamples,
                                           LPREQUESTPARAM lpRequestParams)
{
    HRESULT hr = S_OK;

    if (m_Initialized == FALSE || m_ActiveCount < 1)
    {
        hr = E_FAIL;
    }
    else
    {
        if (m_SmallerThanCache)
        {
            if (dwPosition >= m_SamplesTotal)
            {
                // Can't start reading past the end of the sound
                hr = E_FAIL;
            }
            else
            {
                if ((*lpdwSamples + dwPosition) > m_SamplesTotal)
                {
                    // Limit the samples read to what's left in the buffer
                    *lpdwSamples = m_SamplesTotal - dwPosition;
                }
                m_StartPos = dwPosition;

                // Read all of the samples from the cache
                FillBufferFromCache(lpBuffer, *lpdwSamples, lpRequestParams);

                // Check if we have reached the end of the sound
                if ((*lpdwSamples + dwPosition) >= m_SamplesTotal)
                {
                    hr = S_ENDOFSOUND;
                }
                m_CacheAvailableSamples = m_CacheMaxSamples - m_SamplesTotal;
            }
        }
        else
        {
            // If the caller is trying to seek to a different position than what we are expecting, fill the cache.
            if (m_EndPos != dwPosition)
            {
                hr = FillCache(dwPosition, *lpdwSamples);
            }

            // Check how many samples we have cached in the buffer.
            // If we have less samples than the caller is requesting, fill the cache.
            if (SUCCEEDED(hr) && (m_CacheMaxSamples - m_CacheAvailableSamples < *lpdwSamples))
            {
                hr = FillCache(m_EndPos, m_CacheMaxSamples);
            }

            if (SUCCEEDED(hr))
            {
                // Calculate how many samples to read into the caller's buffer
                DWORD dwAvailableSamples = m_CacheMaxSamples - m_CacheAvailableSamples;
                DWORD dwCacheChunkSamples;
                if (*lpdwSamples <= dwAvailableSamples)
                {
                    // We have enough samples to read all of what the caller requested
                    dwCacheChunkSamples = *lpdwSamples;
                }
                else
                {
                    // We don't have enough - read until we reach the end of the buffer
                    dwCacheChunkSamples = dwAvailableSamples;
                }

                // Read from the cache into the caller's buffer
                hr = FillBufferFromCache(lpBuffer, dwCacheChunkSamples, lpRequestParams);

                if (FAILED(hr) || (m_SamplesTotal > m_EndPos))
                {
                    if (FAILED(hr) || *lpdwSamples <= dwCacheChunkSamples)
                    {
                        dwCacheChunkSamples = m_Thres;
                        if (dwCacheChunkSamples < m_CacheAvailableSamples)
                        {
                            FillCache(m_EndPos, m_CacheMaxSamples);
                        }
                    }
                    else
                    {
                        // Didn't get enough samples from the cache. Get the rest from the sound.
                        *lpdwSamples = *lpdwSamples - dwCacheChunkSamples;
                        hr = m_Sound->GetSampleData(lpBuffer + SamplesToBytes(&m_wfxCache, dwCacheChunkSamples),
                                                    dwPosition + dwCacheChunkSamples, lpdwSamples, lpRequestParams);

                        // Cache more samples
                        DWORD nextPos = dwPosition + dwCacheChunkSamples + *lpdwSamples;
                        FillCache(nextPos, m_CacheMaxSamples);
                        *lpdwSamples = *lpdwSamples + dwCacheChunkSamples;
                    }
                }
                else
                {
                    *lpdwSamples = dwCacheChunkSamples;
                    hr = S_ENDOFSOUND;
                }
            }
        }
    }

    return hr;
}

STDMETHODIMP CAMCacheFilter::SetCacheSize(DWORD dwCacheSize)
{
    m_Remix = dwCacheSize;

    m_Thres = (m_CacheMaxSamples - (m_Remix / 2)) + m_Remix;
    m_Thres += (m_Thres & 3);

    DPRINTF(1, "Remix %lu %lu", m_Remix, SamplesToMillisec(&m_wfxCache, m_Remix));
    DPRINTF(1, "Cache %lu %lu", m_CacheMaxSamples, SamplesToMillisec(&m_wfxCache, m_CacheMaxSamples));
    DPRINTF(1, "Thres %lu %lu", m_Thres, SamplesToMillisec(&m_wfxCache, m_Thres));

    return S_OK;
}

STDMETHODIMP CAMCacheFilter::SetMode(BOOL fActive, BOOL fRecurse)
{
    HRESULT hr = S_OK;

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else
    {
        if (fRecurse)
        {
            hr = m_Sound->SetMode(fActive, fRecurse);
        }
        if (SUCCEEDED(hr))
        {
            if (fActive == FALSE)
            {
                m_ActiveCount--;
            }
            else
            {
                m_ActiveCount++;
            }

            assert(m_ActiveCount >= 0);

            if (m_ActiveCount < 1)
            {
                if (m_Buffer != NULL)
                {
                    delete[] m_Buffer;
                }
                m_Buffer = NULL;
            }
            else if (m_Buffer == NULL)
            {
                DWORD cbBuffer = SamplesToBytes(&m_wfxCache, m_CacheMaxSamples);
                m_Buffer = new BYTE[cbBuffer];

                m_CacheAvailableSamples = m_CacheMaxSamples;
                m_StartPos = 0;
                m_EndPos = 0;

                if (m_Buffer == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
                else if (m_SmallerThanCache)
                {
                    FillCache(0, m_SamplesTotal);
                }
            }
        }
    }
    return hr;
}

STDMETHODIMP CAMCacheFilter::Init(LPSOUND pSoundSrc, CacheConfig *pCacheConfig)
{
    HRESULT hr = S_OK;
    BOOL fConverted = FALSE;
    DWORD format = 0;
    WAVEFORMATEX wfxSrc;
    LPSOUND pConvertFilter = NULL;

    if (pSoundSrc == NULL || pCacheConfig == NULL || pCacheConfig->dwSize != sizeof(CacheConfig))
    {
        hr = E_FAIL;
    }
    else
    {
        m_fSrcFormat = pCacheConfig->fSrcFormat;
        if (m_fSrcFormat == FALSE)
        {
            if (pCacheConfig->lpFormat == NULL)
            {
                format = ConvertFormatToWaveFormatEx(&m_wfxCache, pCacheConfig->dwFormat);
                if (format == 0)
                {
                    hr = E_FAIL;
                }
            }
            else
            {
                memcpy(&m_wfxCache, pCacheConfig->lpFormat, sizeof(m_wfxCache));
            }

            if (SUCCEEDED(hr))
            {
                hr = pSoundSrc->GetFormat(&wfxSrc, sizeof(wfxSrc));
            }

            if (SUCCEEDED(hr))
            {
                if (SameFormats(&wfxSrc, &m_wfxCache) == FALSE)
                {
                    hr = AllocConvertFilter(&pConvertFilter, pSoundSrc, &m_wfxCache);
                    if (SUCCEEDED(hr))
                    {
                        fConverted = TRUE;
                        pSoundSrc = pConvertFilter;
                    }
                }
            }
        }
        else
        {
            hr = pSoundSrc->GetFormat(&wfxSrc, sizeof(wfxSrc));
            if (SUCCEEDED(hr))
            {
                memcpy(&m_wfxCache, &wfxSrc, sizeof(m_wfxCache));
            }
        }

        if (SUCCEEDED(hr))
        {
            m_Sound = pSoundSrc;
            if (fConverted == FALSE)
            {
                m_Sound->AddRef();
            }

            m_SamplesTotal = m_Sound->GetSamples();
            m_CacheMaxMsec = pCacheConfig->dwCacheTime;
            m_CacheMaxSamples = MillisecToSamples(&m_wfxCache, m_CacheMaxMsec);

            // Align number of samples to cache
            m_CacheMaxSamples = m_CacheMaxSamples + (m_CacheMaxSamples & 3);

            m_Thres = (m_CacheMaxSamples - (m_Remix / 2)) + m_Remix;
            m_Thres = m_Thres + (m_Thres & 3);

            m_EndPos = 0;

            if (m_SamplesTotal <= m_CacheMaxSamples)
            {
                m_SmallerThanCache = TRUE;
            }
            else
            {
                m_SmallerThanCache = FALSE;
            }

            m_Initialized = TRUE;
        }
    }
    return hr;
}

void CAMCacheFilter::MoveCacheForward(DWORD dwNumSamples)
{
    if (dwNumSamples < (m_CacheMaxSamples - m_CacheAvailableSamples))
    {
        m_StartPos += dwNumSamples;

        if (m_CacheMaxSamples <= m_StartPos)
        {
            m_StartPos = m_StartPos - m_CacheMaxSamples;
        }

        assert(m_StartPos >= 0);

        m_CacheAvailableSamples += dwNumSamples;
    }
    else
    {
        m_StartPos = 0;
        m_CacheAvailableSamples = m_CacheMaxSamples;
    }
}

void CAMCacheFilter::MoveCacheBackward(DWORD dwSamples)
{
    if (m_Remix < dwSamples)
    {
        m_StartPos = 0;
        m_CacheAvailableSamples = m_CacheMaxSamples;
    }
    else
    {
        if (dwSamples > m_StartPos)
        {
            m_StartPos = m_CacheMaxSamples - (dwSamples - m_StartPos);
        }
        else
        {
            m_StartPos = m_StartPos - dwSamples;
        }

        m_CacheAvailableSamples = m_CacheAvailableSamples - dwSamples;
    }
}

HRESULT __stdcall CAMCacheFilter::FillBufferFromCache(BYTE *lpBuffer, ULONG dwSamplesFromCache,
                                                      RequestParam *lpRequestParams)
{
    assert(m_CacheMaxSamples - m_CacheAvailableSamples >= dwSamplesFromCache);

    // Read from the circular buffer into the GetSampleData caller's buffer.
    // If there are enough samples in the RHS of the circular buffer, this loop should execute once.
    // If not, it will wrap around to the start of the buffer and read the rest of the samples.

    BYTE *sourceBuffer = m_Buffer + SamplesToBytes(&m_wfxCache, m_StartPos);

    do
    {
        DWORD dwChunkSamples;
        DWORD dwCachedSamples = (m_CacheMaxSamples - m_StartPos);

        if (dwCachedSamples < dwSamplesFromCache)
        {
            // Read up to the end of the cache buffer
            dwChunkSamples = dwCachedSamples;
        }
        else
        {
            // Read all of the samples
            dwChunkSamples = dwSamplesFromCache;
        }

        // Copy the chunk
        DWORD cbChunk = SamplesToBytes(&m_wfxCache, dwChunkSamples);
        memcpy(lpBuffer, sourceBuffer, cbChunk);

        // Rewind to the start of the circular buffer
        sourceBuffer = m_Buffer;
        lpBuffer += cbChunk;

        MoveCacheForward(dwChunkSamples);
        m_EndPos += dwChunkSamples;

        dwSamplesFromCache = dwSamplesFromCache - dwChunkSamples;

    } while (dwSamplesFromCache != 0);

    return S_OK;
}

HRESULT __stdcall CAMCacheFilter::FillCache(DWORD dwPosition, DWORD dwNeededSamples)
{
    HRESULT hr = S_OK;
    DWORD dwSamplesToCache;

    if (m_EndPos != dwPosition)
    {
        // Move to the requested position
        if (m_EndPos <= dwPosition)
        {
            MoveCacheForward(dwPosition - m_EndPos);
        }
        else
        {
            MoveCacheBackward(m_EndPos - dwPosition);
        }
        m_EndPos = dwPosition;
    }

    if (dwNeededSamples > (m_CacheMaxSamples - m_CacheAvailableSamples))
    {
        DWORD dwReadSample = m_CacheMaxSamples - m_CacheAvailableSamples + m_StartPos;
        if (dwReadSample >= m_CacheMaxSamples)
        {
            dwReadSample = dwReadSample - m_CacheMaxSamples;
        }

        if (m_SmallerThanCache)
        {
            dwSamplesToCache = dwNeededSamples;
        }
        else
        {
            dwSamplesToCache = m_CacheAvailableSamples - m_Remix;
        }

        if (m_EndPos + m_CacheMaxSamples - m_CacheAvailableSamples < m_SamplesTotal)
        {
            DPRINTF(1, "FillCache Read %lu (%lu ms) at time %lu", dwSamplesToCache,
                    SamplesToMillisec(&m_wfxCache, dwSamplesToCache), timeGetTime());

            while (dwSamplesToCache != 0)
            {
                DWORD dwSamplesChunk = dwSamplesToCache;

                if (m_CacheMaxSamples < (dwSamplesToCache + dwReadSample))
                {
                    dwSamplesChunk = m_CacheMaxSamples - dwReadSample;
                }

                // Read into the cache buffer
                BYTE *lpCache = m_Buffer + SamplesToBytes(&m_wfxCache, dwReadSample);
                DWORD readChunkPos = m_EndPos + m_CacheMaxSamples - m_CacheAvailableSamples;

                hr = m_Sound->GetSampleData(lpCache, readChunkPos, &dwSamplesChunk, NULL);

                if (FAILED(hr))
                {
                    return hr;
                }

                if (m_SmallerThanCache)
                {
                    m_SamplesTotal = dwSamplesChunk;
                }

                dwSamplesToCache = dwSamplesToCache - dwSamplesChunk;
                m_CacheAvailableSamples = m_CacheAvailableSamples - dwSamplesChunk;
                dwReadSample = 0;

                if (hr == S_ENDOFSOUND)
                {
                    return hr;
                }

                if (dwSamplesChunk == 0)
                {
                    return hr;
                }
            }
        }
    }

    return hr;
}

CAMCacheFilter::~CAMCacheFilter()
{
    DPRINTF(1, "Destructing Cache Filter");

    if (m_Initialized)
    {
        assert(m_ActiveCount == 0);

        if (m_Sound != NULL)
        {
            m_Sound->Release();
        }

        if (m_Buffer != NULL)
        {
            delete[] m_Buffer;
        }

        m_Initialized = FALSE;
    }
}

STDAPI AllocCacheFilter(LPSOUND FAR *ppSound, LPSOUND pSoundSrc, LPCACHECONFIG lpCacheConfig)
{
    assert(ppSound != NULL);
    assert(pSoundSrc != NULL);
    assert(lpCacheConfig != NULL);

    IAMCacheFilter *pCache = NULL;
    LPSOUND pSnd = NULL;

    HRESULT hr = AMCreate(CLSID_AMCacheFilter, IID_IAMCacheFilter, (void **)&pCache, &pSnd);

    if (SUCCEEDED(hr))
    {
        hr = pCache->Init(pSoundSrc, lpCacheConfig);
        AMFinish(hr, ppSound, pCache, pSnd);
    }

    return hr;
}