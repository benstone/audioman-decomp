#include "sndloop.h"
#include "dpf.h"
#include <cassert>
#include "utils.h"

STDMETHODIMP_(ULONG) __stdcall CAMLoopFilter::AddRef()
{
    return ++m_RefCnt;
}

STDMETHODIMP_(ULONG) CAMLoopFilter::Release()
{
    ULONG RefCnt = --m_RefCnt;

    if (RefCnt == 0)
    {
        delete this;
    }

    return RefCnt;
}

STDMETHODIMP CAMLoopFilter::QueryInterface(REFIID riid, void **ppvObject)
{
    assert(ppvObject != NULL);

    HRESULT hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAMSound))
    {
        *ppvObject = (IAMSound *)this;
    }
    else if (IsEqualIID(riid, IID_IAMLoopFilter))
    {
        *ppvObject = (IAMLoopFilter *)this;
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

STDMETHODIMP_(DWORD) CAMLoopFilter::GetSamples()
{
    if (m_Initialized)
    {
        return m_TotalSamples;
    }
    else
    {
        return 0;
    }
}

STDMETHODIMP CAMLoopFilter::GetAlignment(LPDWORD lpdwLeftAlign, LPDWORD lpdwRightAlign)
{
    HRESULT hr = S_OK;
    if (m_Initialized == 0 || lpdwLeftAlign == NULL || lpdwRightAlign == NULL)
    {
        hr = E_FAIL;
    }
    else
    {
        *lpdwLeftAlign = 0;
        *lpdwRightAlign = 0;
    }

    return hr;
}

STDMETHODIMP CAMLoopFilter::GetSampleData(LPBYTE lpBuffer, DWORD dwPosition, LPDWORD lpdwSamples,
                                          LPREQUESTPARAM lpRequestParams)
{
    HRESULT hr = S_OK;
    DWORD dwTotalSamples, dwSamples, dwSrcSamples, dwOffset;

    if (m_Initialized == FALSE || m_ActiveCount < 1)
    {
        hr = E_FAIL;
    }
    else
    {
        dwSamples = *lpdwSamples;
        dwTotalSamples = 0;

        if (lpRequestParams != NULL)
        {
            if (lpRequestParams->dwSize == sizeof(REQUESTPARAM))
            {
                if (lpRequestParams->dwFinishPos != INFINITE_SAMPLES && lpRequestParams->dwFinishPos < dwPosition)
                {
                    m_LoopsRemaining = 0;
                }
            }
            else
            {
                hr = E_FAIL;
            }
        }

        if (SUCCEEDED(hr))
        {
            if (m_TotalSamples < dwPosition)
            {
                hr = E_FAIL;
            }
        }

        if (SUCCEEDED(hr))
        {
            dwSrcSamples = m_Sound->GetSamples();
            if (m_TotalSamples != 1 && dwSrcSamples != INFINITE_SAMPLES)
            {
                m_LoopsRemaining = m_LoopsTotal - dwPosition / dwSrcSamples;
            }
            if (dwSrcSamples != INFINITE_SAMPLES)
            {
                dwPosition = dwPosition % dwSrcSamples;
            }

            hr = m_Sound->GetSampleData(lpBuffer, dwPosition, &dwSamples, lpRequestParams);
        }

        if (SUCCEEDED(hr))
        {
            dwTotalSamples = dwSamples;

            while (*lpdwSamples != dwTotalSamples && (m_LoopsRemaining != 0))
            {
                if (m_LoopsRemaining != INFINITE_SAMPLES)
                {
                    m_LoopsRemaining--;
                }
                dwSamples = *lpdwSamples - dwTotalSamples;
                dwOffset = SamplesToBytes(&m_wfxSrc, dwTotalSamples);

                hr = GetSampleData(lpBuffer + dwOffset, 0, &dwSamples, lpRequestParams);

                if (FAILED(hr))
                {
                    break;
                }

                dwTotalSamples += dwSamples;
            }

            if (hr == S_ENDOFSOUND)
            {
                hr = S_OK;
            }

            if (*lpdwSamples != dwTotalSamples)
            {
                *lpdwSamples = dwTotalSamples;
                hr = S_ENDOFSOUND;
            }

            if (*lpdwSamples == 0)
            {
                hr = E_FAIL;
            }
        }
    }

    return hr;
}

STDMETHODIMP CAMLoopFilter::Init(LPSOUND pSoundSrc, DWORD dwLoops)
{
    HRESULT hr = S_OK;
    DWORD dwSrcSamples;

    if (pSoundSrc == NULL)
    {
        DPRINTF(0, "Invalid pSoundSrc on CAMLoopFilter::Init call");
        m_Initialized = FALSE;
        hr = E_FAIL;
    }
    else
    {
        hr = pSoundSrc->GetFormat(&m_wfxSrc, sizeof(m_wfxSrc));
        if (hr == S_OK)
        {
            dwSrcSamples = pSoundSrc->GetSamples();
            if (dwSrcSamples == INFINITE_SAMPLES || dwLoops == INFINITE_SAMPLES)
            {
                m_TotalSamples = INFINITE_SAMPLES;
            }
            else
            {
                float flLoopSamples = (float)dwSrcSamples * (dwLoops + 1);
                if (flLoopSamples > 4.294967e+09)
                {
                    m_TotalSamples = INFINITE_SAMPLES;
                }
                else
                {
                    m_TotalSamples = (DWORD)flLoopSamples;
                }
            }

            m_LoopsRemaining = dwLoops;
            m_LoopsTotal = dwLoops;

            pSoundSrc->AddRef();
            m_Sound = pSoundSrc;
            m_Initialized = TRUE;
        }
    }
    return hr;
}

CAMLoopFilter::~CAMLoopFilter()
{
}

STDAPI AllocLoopFilter(LPSOUND FAR *ppSound, LPSOUND pSoundSrc, DWORD dwLoops)
{
    HRESULT hr = S_OK;
    IAMLoopFilter *pLoop = NULL;
    LPSOUND pSnd = NULL;

    assert(ppSound != NULL);
    assert(pSoundSrc != NULL);

    hr = AMCreate(CLSID_AMLoopFilter, IID_IAMLoopFilter, (void **)&pLoop, &pSnd);

    if (SUCCEEDED(hr))
    {
        hr = pLoop->Init(pSoundSrc, dwLoops);
        AMFinish(hr, ppSound, pLoop, pSnd);
    }

    return hr;
}