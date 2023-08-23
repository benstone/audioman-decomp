#include "sndbias.h"
#include "dpf.h"
#include <cassert>
#include "utils.h"

STDMETHODIMP_(ULONG) __stdcall CAMBiasFilter::AddRef()
{
    return ++m_RefCnt;
}

STDMETHODIMP_(ULONG) CAMBiasFilter::Release()
{
    ULONG RefCnt = --m_RefCnt;

    if (RefCnt == 0)
    {
        delete this;
    }

    return RefCnt;
}

STDMETHODIMP CAMBiasFilter::QueryInterface(REFIID riid, void **ppvObject)
{
    assert(ppvObject != NULL);

    HRESULT hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAMSound))
    {
        *ppvObject = (IAMSound *)this;
    }
    else if (IsEqualIID(riid, IID_IAMBiasFilter))
    {
        *ppvObject = (IAMBiasFilter *)this;
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

STDMETHODIMP CAMBiasFilter::GetSampleData(LPBYTE lpBuffer, DWORD dwPosition, LPDWORD lpdwSamples,
                                          LPREQUESTPARAM lpRequestParams)
{
    HRESULT hr = S_OK;

    hr = CAMPassThruFilter::GetSampleData(lpBuffer, dwPosition, lpdwSamples, lpRequestParams);

    if (SUCCEEDED(hr) && m_Bias != 0)
    {
        if (m_Is8Bit)
        {
            BYTE *pData = lpBuffer;

            for (DWORD samplePos = 0; samplePos < *lpdwSamples; samplePos++)
            {
                for (USHORT dwScan = 0; dwScan < m_Channels; dwScan++)
                {
                    *pData = (*pData - (BYTE)m_Bias);
                    pData++;
                }
            }
        }
        else
        {
            SHORT *pData = (SHORT *)lpBuffer;

            for (DWORD dwScan = 0; dwScan < *lpdwSamples; dwScan++)
            {
                for (USHORT channelPos = 0; channelPos < m_Channels; channelPos++)
                {
                    *pData = (*pData - (short)m_Bias);
                    pData++;
                }
            }
        }
    }

    return hr;
}

STDMETHODIMP CAMBiasFilter::Init(LPSOUND pSoundSrc)
{
    HRESULT hr = S_OK;

    if (pSoundSrc == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = CalcBias(pSoundSrc);

        if (SUCCEEDED(hr))
        {
            m_Sound = pSoundSrc;
            m_Sound->AddRef();
            m_Initialized = TRUE;
        }
    }
    return hr;
}

HRESULT CAMBiasFilter::CalcBias(LPSOUND pSound)
{
    HRESULT hr = S_OK;
    BYTE *pBuffer = NULL;
    USHORT nChannel;
    DWORD dwReadSamples;
    DWORD dwBufferBytes;
    WAVEFORMATEX wfx;

    DWORD dwSamples = pSound->GetSamples();
    if (dwSamples != 0)
    {
        hr = pSound->GetFormat(&wfx, sizeof(wfx));
    }

    if (SUCCEEDED(hr))
    {
        dwReadSamples = MillisecToSamples(&wfx, 250);
        dwBufferBytes = SamplesToBytes(&wfx, dwReadSamples);

        m_Channels = wfx.nChannels;

        pBuffer = new BYTE[dwBufferBytes];

        if (pBuffer == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pSound->SetMode(TRUE, FALSE);

        if (dwSamples < dwReadSamples)
        {
            dwReadSamples = dwSamples;
        }

        hr = pSound->GetSampleData(pBuffer, 0, &dwReadSamples, NULL);

        if (FAILED(hr))
        {
            hr = pSound->SetMode(FALSE, TRUE);
        }
    }

    if (SUCCEEDED(hr))
    {
        LONG lSum = 0;

        if (wfx.wBitsPerSample == 16)
        {
            SHORT *pData = (SHORT *)pBuffer;
            m_Is8Bit = FALSE;

            for (DWORD dwScan = 0; dwScan < dwReadSamples; dwScan++)
            {
                for (nChannel = 0; nChannel < wfx.nChannels; nChannel++)
                {
                    lSum += *pData;
                    pData++;
                }
            }

            m_Bias = lSum / (wfx.nChannels * dwReadSamples);
            if (abs(m_Bias) < 1000)
            {
                m_Bias = 0;
            }
        }
        else if (wfx.wBitsPerSample == 8)
        {
            BYTE *pData = pBuffer;
            m_Is8Bit = TRUE;

            for (DWORD dwScan = 0; dwScan < dwReadSamples; dwScan++)
            {
                for (nChannel = 0; nChannel < wfx.nChannels; nChannel++)
                {
                    lSum += (*pData - 0x80);
                    pData++;
                }
            }

            m_Bias = lSum / (int)(wfx.nChannels * dwReadSamples);

            if (abs(m_Bias) < 3)
            {
                m_Bias = 0;
            }
        }

        hr = pSound->SetMode(FALSE, TRUE);
    }

    // Cleanup
    if (pBuffer != NULL)
    {
        delete[] pBuffer;
    }

    return hr;
}

CAMBiasFilter::~CAMBiasFilter()
{
    // do nothing
}

STDAPI AllocBiasFilter(LPSOUND FAR *ppSound, LPSOUND pSoundSrc)
{
    HRESULT hr = S_OK;
    IAMBiasFilter *pBias = NULL;
    LPSOUND pSnd = NULL;

    assert(ppSound != NULL);
    assert(pSoundSrc != NULL);

    hr = AMCreate(CLSID_AMBiasFilter, IID_IAMBiasFilter, (void **)&pBias, &pSnd);

    if (SUCCEEDED(hr))
    {
        hr = pBias->Init(pSoundSrc);
        AMFinish(hr, ppSound, pBias, pSnd);
    }

    return hr;
}