#include "sndclip.h"
#include <cassert>
#include "dpf.h"
#include "utils.h"

STDMETHODIMP_(ULONG) __stdcall CAMClipFilter::AddRef()
{
    return ++m_RefCnt;
}

STDMETHODIMP_(ULONG) CAMClipFilter::Release()
{
    ULONG RefCnt = --m_RefCnt;

    if (RefCnt == 0)
    {
        delete this;
    }

    return RefCnt;
}

STDMETHODIMP CAMClipFilter::QueryInterface(REFIID riid, void **ppvObject)
{
    assert(ppvObject != NULL);

    HRESULT hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAMSound))
    {
        *ppvObject = (IAMSound *)this;
    }
    else if (IsEqualIID(riid, IID_IAMClipFilter))
    {
        *ppvObject = (IAMClipFilter *)this;
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

STDMETHODIMP CAMClipFilter::GetFormat(LPWAVEFORMATEX pFormat, DWORD cbSize)
{
    HRESULT hr = S_OK;

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else
    {
        hr = m_Source->GetFormat(pFormat, cbSize);
    }

    return hr;
}

STDMETHODIMP_(DWORD) CAMClipFilter::GetSamples()
{
    DWORD samples = 0;

    if (m_Initialized)
    {
        samples = m_SamplesClip;
    }

    return samples;
}

STDMETHODIMP CAMClipFilter::GetAlignment(LPDWORD lpdwLeftAlign, LPDWORD lpdwRightAlign)
{
    HRESULT hr = S_OK;

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else
    {
        hr = m_Source->GetAlignment(lpdwLeftAlign, lpdwRightAlign);
        if (hr == S_OK)
        {
            hr = PinSamples(lpdwLeftAlign, lpdwRightAlign);
        }
    }

    return hr;
}

STDMETHODIMP CAMClipFilter::GetSampleData(LPBYTE lpBuffer, DWORD dwPosition, LPDWORD lpdwSamples,
                                          LPREQUESTPARAM lpRequestParams)
{
    HRESULT hr = S_OK;
    DWORD dwEmptySamples = 0;

    if (m_Initialized == FALSE || m_ActiveCount < 1 || m_SamplesClip < dwPosition)
    {
        hr = E_FAIL;
    }
    else
    {
        DWORD dwOrgSamples = *lpdwSamples;
        m_Pos = dwPosition;

        // Check we're not going past the end of the clip
        if (*lpdwSamples > m_SamplesClip - m_Pos)
        {
            *lpdwSamples = m_SamplesClip - m_Pos;
        }

        DWORD chunkEndPos = m_Pos + *lpdwSamples;

        // Check if we need to generate any silence
        if (chunkEndPos > m_SamplesSrc)
        {
            if (m_SamplesSrc >= m_Pos)
            {
                dwEmptySamples = *lpdwSamples;
                *lpdwSamples = 0;
            }
            else
            {
                dwEmptySamples = m_Pos - m_SamplesSrc;
                *lpdwSamples = *lpdwSamples - dwEmptySamples;
            }
        }

        // Get the samples
        if (*lpdwSamples != 0)
        {
            hr = m_Source->GetSampleData(lpBuffer, m_StartPos + m_Pos, lpdwSamples, lpRequestParams);
        }

        if (hr == S_OK)
        {
            m_Pos = m_Pos + *lpdwSamples;

            // If requested more than the length of the source sound, generate silence
            if (dwEmptySamples != 0)
            {
                memset(lpBuffer + SamplesToBytes(&m_wfxSrc, *lpdwSamples), m_Padding,
                       SamplesToBytes(&m_wfxSrc, dwEmptySamples));
                *lpdwSamples = *lpdwSamples + dwEmptySamples;

                m_Pos = m_Pos + dwEmptySamples;
            }

            if (*lpdwSamples != dwOrgSamples)
            {
                hr = S_ENDOFSOUND;
            }
        }
    }

    return hr;
}

STDMETHODIMP CAMClipFilter::SetCacheSize(DWORD dwCacheSize)
{
    HRESULT hr = S_OK;

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else
    {
        hr = m_Source->SetCacheSize(dwCacheSize);
    }
    return hr;
}

STDMETHODIMP CAMClipFilter::SetMode(BOOL fActive, BOOL fRecurse)
{
    HRESULT hr = S_OK;

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else if (fRecurse == FALSE)
    {
        if (fActive)
        {
            m_ActiveCount++;
        }
        else
        {
            m_ActiveCount--;
        }
    }
    else
    {
        hr = m_Source->SetMode(fActive, fRecurse);
        if (SUCCEEDED(hr))
        {
            if (fActive)
            {
                m_ActiveCount++;
            }
            else
            {
                m_ActiveCount--;
            }
        }
    }

    assert(m_ActiveCount >= 0);

    return hr;
}

HRESULT CAMClipFilter::PinSamples(DWORD *lpdwStartPos, DWORD *lpdwEndPos)
{
    if (*lpdwStartPos != 0)
    {
        if (*lpdwStartPos > m_StartPos)
        {
            *lpdwStartPos = *lpdwStartPos - m_StartPos;
        }
        else
        {
            *lpdwStartPos = 0;
        }
    }
    if (*lpdwEndPos != 0)
    {
        if (m_EndPos > (m_SamplesSrc - *lpdwEndPos))
        {
            *lpdwEndPos = *lpdwEndPos - (m_SamplesSrc - m_EndPos);
        }
        else
        {
            *lpdwEndPos = 0;
        }
    }

    return S_OK;
}

STDMETHODIMP CAMClipFilter::Init(LPSOUND pSoundSrc, DWORD dwStartSample, DWORD dwEndSample)
{
    HRESULT hr = S_OK;

    if (pSoundSrc == NULL)
    {
        DPRINTF(0, "Invalid pSoundSrc on CAMClipFilter::Init call");
        m_Initialized = FALSE;
        hr = E_INVALIDARG;
    }
    else
    {
        m_SamplesSrc = pSoundSrc->GetSamples();
        if (dwStartSample < dwEndSample)
        {
            hr = pSoundSrc->GetFormat(&m_wfxSrc, sizeof(m_wfxSrc));

            if (hr == S_OK)
            {
                if (m_wfxSrc.wBitsPerSample == 8)
                {
                    m_Padding = 0x80;
                }
                else
                {
                    m_Padding = 0;
                }

                m_StartPos = dwStartSample;
                m_EndPos = dwEndSample;
                m_SamplesClip = m_EndPos - m_StartPos;
                m_Pos = 0;

                pSoundSrc->AddRef();
                m_Source = pSoundSrc;

                m_Initialized = TRUE;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    return hr;
}

CAMClipFilter::~CAMClipFilter()
{
    DPRINTF(1, "Destructing Clip Filter");
    if (m_Initialized)
    {
        assert(m_ActiveCount == 0);

        if (m_Source != NULL)
        {
            m_Source->Release();
        }
        m_Initialized = FALSE;
    }
}

STDAPI AllocClipFilter(LPSOUND FAR *ppSound, LPSOUND pSoundSrc, DWORD dwStartPos, DWORD dwEndPos)
{
    HRESULT hr = S_OK;
    IAMClipFilter *pClip = NULL;
    LPSOUND pSound = NULL;

    assert(ppSound != NULL);
    assert(pSoundSrc != NULL);

    hr = AMCreate(CLSID_AMClipFilter, IID_IAMClipFilter, (void **)&pClip, &pSound);

    if (SUCCEEDED(hr))
    {
        hr = pClip->Init(pSoundSrc, dwStartPos, dwEndPos);
        AMFinish(hr, ppSound, pClip, pSound);
    }

    return hr;
}