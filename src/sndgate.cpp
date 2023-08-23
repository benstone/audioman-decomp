#include "sndgate.h"
#include "utils.h"
#include <cassert>
#include <cmath>

STDMETHODIMP_(ULONG) __stdcall CAMGateFilter::AddRef()
{
    return ++m_RefCnt;
}

STDMETHODIMP_(ULONG) CAMGateFilter::Release()
{
    ULONG RefCnt = --m_RefCnt;

    if (RefCnt == 0)
    {
        delete this;
    }

    return RefCnt;
}

STDMETHODIMP CAMGateFilter::QueryInterface(REFIID riid, void **ppvObject)
{
    assert(ppvObject != NULL);

    HRESULT hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAMSound))
    {
        *ppvObject = (IAMSound *)this;
    }
    else if (IsEqualIID(riid, IID_IAMGateFilter))
    {
        *ppvObject = (IAMGateFilter *)this;
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

STDMETHODIMP CAMGateFilter::GetSampleData(LPBYTE lpBuffer, DWORD dwPosition, LPDWORD lpdwSamples,
                                          LPREQUESTPARAM lpRequestParams)
{
    HRESULT hr = S_OK;

    hr = CAMPassThruFilter::GetSampleData(lpBuffer, dwPosition, lpdwSamples, lpRequestParams);

    if (SUCCEEDED(hr) && (*lpdwSamples != 0))
    {
        DWORD cb = SamplesToBytes(&m_wfxSrc, *lpdwSamples);

        if (m_wfxSrc.wBitsPerSample == 16)
        {
            SHORT *sampleShorts = (SHORT *)lpBuffer;
            cb = cb / 2;

            while (cb > 0)
            {
                if (abs(*sampleShorts) < m_GateThreshold)
                {
                    *sampleShorts = 0;
                }

                sampleShorts++;
                cb--;
            }
        }
        else
        {

            BYTE *sampleBytes = (BYTE *)lpBuffer;

            while (cb > 0)
            {
                if (abs(*sampleBytes) < m_GateThreshold)
                {
                    *sampleBytes = 0;
                }

                sampleBytes++;
                cb--;
            }
        }
    }

    return hr;
}

STDMETHODIMP CAMGateFilter::Init(LPSOUND pSoundSrc, float flDBGate)
{
    HRESULT hr = S_OK;

    if (pSoundSrc == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        m_Sound = pSoundSrc;
        hr = m_Sound->GetFormat(&m_wfxSrc, sizeof(m_wfxSrc));
    }

    if (hr == S_OK)
    {
        if (m_wfxSrc.wBitsPerSample == 16)
        {
            m_GateThreshold = (LONG)(32768.0 * pow(2.0, (long double)flDBGate / 10.0F));
        }
        else
        {
            m_GateThreshold = (LONG)(128.0 * pow(2.0, (long double)flDBGate / 10.0F));
        }
        m_Initialized = TRUE;
        m_Sound->AddRef();
    }

    return hr;
}

CAMGateFilter::~CAMGateFilter()
{
}

STDAPI AllocGateFilter(LPSOUND FAR *ppSound, LPSOUND pSoundSrc, float flDBGate)
{
    HRESULT hr = S_OK;
    IAMGateFilter *pGate = NULL;
    LPSOUND pSnd = NULL;

    assert(ppSound != NULL);
    assert(pSoundSrc != NULL);

    hr = AMCreate(CLSID_AMGateFilter, IID_IAMGateFilter, (void **)&pGate, &pSnd);

    if (SUCCEEDED(hr))
    {
        hr = pGate->Init(pSoundSrc, flDBGate);
        AMFinish(hr, ppSound, pGate, pSnd);
    }

    return hr;
}