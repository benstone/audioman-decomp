#include "sndtrim.h"
#include "utils.h"
#include <cassert>

STDMETHODIMP_(ULONG) __stdcall CAMTrimFilter::AddRef()
{
    return ++m_RefCnt;
}

STDMETHODIMP_(ULONG) CAMTrimFilter::Release()
{
    ULONG RefCnt = --m_RefCnt;

    if (RefCnt == 0)
    {
        delete this;
    }

    return RefCnt;
}

STDMETHODIMP CAMTrimFilter::QueryInterface(REFIID riid, void **ppvObject)
{
    assert(ppvObject != NULL);

    HRESULT hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAMSound))
    {
        *ppvObject = (IAMSound *)this;
    }
    else if (IsEqualIID(riid, IID_IAMTrimFilter))
    {
        *ppvObject = (IAMTrimFilter *)this;
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

STDMETHODIMP CAMTrimFilter::Init(LPSOUND pSound)
{
    HRESULT hr = S_OK;
    DWORD first;
    DWORD last;

    if (pSound == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = CalcTrimSamples(pSound, &first, &last);

        if (SUCCEEDED(hr))
        {
            if (first != INFINITE_SAMPLES)
            {
                hr = AllocClipFilter(&m_Sound, pSound, first, last);
                if (SUCCEEDED(hr))
                {
                    m_Initialized = TRUE;
                }
            }
            else
            {
                m_Sound = pSound;
                m_Sound->AddRef();
                m_Initialized = TRUE;
            }
        }
    }

    return hr;
}

CAMTrimFilter::~CAMTrimFilter()
{
}

HRESULT CAMTrimFilter::CalcTrimSamples(LPSOUND pSound, LPDWORD lpdwFirst, LPDWORD lpdwLast)
{
    HRESULT hr = S_OK;
    DWORD samplesSrc, samplesFirst, samplesLast;
    DWORD bufferSamples, bufferBytes;
    BYTE *buffer = NULL;
    LPSOUND psndGateFilter = NULL;
    WAVEFORMATEX wfxGateFilter;

    samplesSrc = pSound->GetSamples();

    if (samplesSrc != INFINITE_SAMPLES && samplesSrc != 0)
    {
        samplesLast = samplesSrc;

        hr = AllocGateFilter(&psndGateFilter, pSound, -32.0);

        if (SUCCEEDED(hr))
        {
            hr = psndGateFilter->GetFormat(&wfxGateFilter, sizeof(wfxGateFilter));

            // Allocate a buffer
            if (SUCCEEDED(hr))
            {
                bufferSamples = 1000;
                bufferBytes = SamplesToBytes(&wfxGateFilter, bufferSamples);
                buffer = new BYTE[bufferBytes];
                if (buffer == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
            }

            if (hr == S_OK)
            {
                hr = psndGateFilter->SetMode(TRUE, TRUE);
            }

            if (SUCCEEDED(hr))
            {
                samplesLast = INFINITE_SAMPLES;
                samplesFirst = INFINITE_SAMPLES;

                for (DWORD samplesPos = 0; samplesPos < samplesSrc; samplesPos = samplesPos + bufferSamples)
                {
                    if (samplesSrc < bufferSamples + samplesPos)
                    {
                        bufferSamples = samplesSrc - samplesPos;
                    }

                    hr = psndGateFilter->GetSampleData(buffer, samplesPos, &bufferSamples, NULL);

                    if (FAILED(hr))
                    {
                        break;
                    }

                    if (wfxGateFilter.wBitsPerSample == 16)
                    {
                        // 16-bit samples
                        SHORT *sampleShorts = (SHORT *)buffer;

                        for (DWORD sampleChunkPos = 0; sampleChunkPos < bufferSamples; sampleChunkPos++)
                        {
                            if (*sampleShorts != 0)
                            {
                                samplesLast = sampleChunkPos + samplesPos;
                                if (samplesFirst == INFINITE_SAMPLES)
                                {
                                    samplesFirst = samplesLast;
                                }
                            }

                            sampleShorts++;
                        }
                    }
                    else
                    {
                        // 8-bit samples
                        BYTE *sampleBytes = buffer;

                        for (DWORD sampleChunkPos = 0; sampleChunkPos < bufferSamples; sampleChunkPos++)
                        {
                            if (*sampleBytes != 0x80)
                            {
                                samplesLast = sampleChunkPos + samplesPos;
                                if (samplesFirst == INFINITE_SAMPLES)
                                {
                                    samplesFirst = samplesLast;
                                }
                            }

                            sampleBytes++;
                        }
                    }

                    if (hr == S_ENDOFSOUND)
                    {
                        break;
                    }
                }

                if (samplesFirst != INFINITE_SAMPLES)
                {
                    DWORD samples250msec = MillisecToSamples(&wfxGateFilter, 250);
                    if (samples250msec < samplesFirst)
                    {
                        samplesFirst = samplesFirst - samples250msec;
                    }

                    if (samplesSrc < samplesLast + (samples250msec * 2))
                    {
                        samplesLast = samplesSrc;
                    }
                    else
                    {
                        samplesLast = samplesLast + (samples250msec * 2);
                    }
                }

                *lpdwFirst = samplesFirst;
                *lpdwLast = samplesLast;

                hr = psndGateFilter->SetMode(FALSE, TRUE);
            }

            if (buffer != NULL)
            {
                delete[] buffer;
            }

            psndGateFilter->Release();
        }
    }

    return hr;
}

STDAPI AllocTrimFilter(LPSOUND FAR *ppSound, LPSOUND pSoundSrc)
{
    assert(ppSound != NULL);
    assert(pSoundSrc != NULL);

    IAMTrimFilter *pTrim = NULL;
    LPSOUND pSnd = NULL;

    HRESULT hr = AMCreate(CLSID_AMTrimFilter, IID_IAMTrimFilter, (void **)&pTrim, &pSnd);
    if (SUCCEEDED(hr))
    {
        hr = pTrim->Init(pSoundSrc);
        AMFinish(hr, ppSound, pTrim, pSnd);
    }

    return hr;
}