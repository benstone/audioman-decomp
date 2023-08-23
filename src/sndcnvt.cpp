#include <cassert>
#include "sndcnvt.h"
#include "utils.h"
#include "dpf.h"
#include "todo.h"

STDMETHODIMP_(ULONG) __stdcall CAMConvertFilter::AddRef()
{
    return ++m_RefCnt;
}

STDMETHODIMP_(ULONG) CAMConvertFilter::Release()
{
    ULONG RefCnt = --m_RefCnt;

    if (RefCnt == 0)
    {
        delete this;
    }

    return RefCnt;
}

STDMETHODIMP CAMConvertFilter::QueryInterface(REFIID riid, void **ppvObject)
{
    assert(ppvObject != NULL);

    HRESULT hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAMSound))
    {
        *ppvObject = (IAMSound *)this;
    }
    else if (IsEqualIID(riid, IID_IAMConvertFilter))
    {
        *ppvObject = (IAMConvertFilter *)this;
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

STDMETHODIMP CAMConvertFilter::GetFormat(LPWAVEFORMATEX pFormat, DWORD cbSize)
{
    HRESULT hr = S_OK;

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else if (pFormat == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (cbSize > 0x11)
        {
            cbSize = 0x11;
        }

        memcpy(pFormat, &m_wfxDestFormat, cbSize);
    }

    return hr;
}

STDMETHODIMP_(DWORD) CAMConvertFilter::GetSamples()
{
    DWORD dstSamples = 0;

    if (m_Initialized)
    {
        dstSamples = ConvertToDstSamples(m_Source->GetSamples());
    }

    return dstSamples;
}

STDMETHODIMP CAMConvertFilter::GetAlignment(LPDWORD lpdwLeftAlign, LPDWORD lpdwRightAlign)
{
    HRESULT hr = S_OK;

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else
    {
        hr = m_Source->GetAlignment(lpdwLeftAlign, lpdwRightAlign);

        *lpdwLeftAlign = ConvertToDstSamples(*lpdwLeftAlign);
        *lpdwRightAlign = ConvertToDstSamples(*lpdwRightAlign);
    }

    return hr;
}

STDMETHODIMP CAMConvertFilter::GetSampleData(LPBYTE lpBuffer, DWORD dwPosition, LPDWORD lpdwSamples,
                                             LPREQUESTPARAM lpRequestParams)
{
    HRESULT hr = S_OK;
    DWORD dwDstBytePos = 0;
    DWORD dwSrcBytePos = 0;
    DWORD dwSrcPosition = 0;
    DWORD dwDstBytes = 0;
    DWORD dwSrcBytes = 0;
    DWORD dwSrcSamples = 0;
    DWORD dwDstSamples = 0;
    DWORD dwLostSamples = 0;
    DWORD dwConvertedSamples = 0;

    if (m_Initialized == FALSE || m_ActiveCount < 1)
    {
        hr = E_FAIL;
    }
    else
    {
        dwDstBytePos = SamplesToBytes(&m_wfxDestFormat, dwPosition);
        dwSrcBytePos = GetSourceSize(dwDstBytePos, &m_ConversionData);
        dwSrcPosition = BytesToSamples(&m_wfxSrcFormat, dwSrcBytePos);
        dwDstBytes = SamplesToBytes(&m_wfxDestFormat, *lpdwSamples);
        dwSrcBytes = GetSourceSize(dwDstBytes, &m_ConversionData);
        dwSrcSamples = BytesToSamples(&m_wfxSrcFormat, dwSrcBytes);
        dwSrcBytes = SamplesToBytes(&m_wfxSrcFormat, dwSrcSamples);
        dwDstBytes = GetDestSize(dwSrcBytes, &m_ConversionData);

        DWORD maxSamples = BytesToSamples(&m_wfxDestFormat, dwDstBytes);

        dwDstSamples = *lpdwSamples;

        if (maxSamples <= *lpdwSamples)
        {
            dwDstSamples = maxSamples;
        }

        dwDstBytes = SamplesToBytes(&m_wfxDestFormat, dwDstSamples);
        dwLostSamples = *lpdwSamples - dwDstSamples;
        if (*lpdwSamples != 0)
        {
            if (m_lpvConversionBuffer == NULL)
            {
                DWORD dwTotalSamples = 0;
                BYTE *pb = lpBuffer;
                dwConvertedSamples = dwDstSamples;

                while (*lpdwSamples != dwTotalSamples && SUCCEEDED(hr))
                {
                    hr = m_Source->GetSampleData(pb, dwSrcPosition, &dwSrcSamples, lpRequestParams);
                    if (SUCCEEDED(hr))
                    {
                        dwSrcBytes = SamplesToBytes(&m_wfxSrcFormat, dwSrcSamples);
                        dwDstBytes = GetDestSize(dwSrcBytes, &m_ConversionData);
                        dwDstSamples = BytesToSamples(&m_wfxDestFormat, dwDstBytes);

                        if (*lpdwSamples < (dwTotalSamples + dwDstSamples))
                        {
                            dwDstSamples = *lpdwSamples - dwTotalSamples;
                        }

                        dwConvertedSamples = SamplesToBytes(&m_wfxDestFormat, dwDstSamples);

                        dwDstBytes = m_PCMConvertProc(pb, dwSrcBytes,
                                                      lpBuffer + SamplesToBytes(&m_wfxDestFormat, dwTotalSamples),
                                                      dwConvertedSamples, &m_ConversionData);
                        dwDstSamples = BytesToSamples(&m_wfxDestFormat, dwDstBytes);
                        dwSrcPosition += dwSrcSamples;
                        dwTotalSamples += dwDstSamples;
                        dwConvertedSamples = dwDstSamples;

                        if (hr == S_ENDOFSOUND)
                        {
                            break;
                        }
                        if (pb != lpBuffer)
                        {
                            dwLostSamples -= dwDstSamples;
                        }
                        dwConvertedSamples = dwTotalSamples;
                        if (dwLostSamples == 0)
                        {
                            break;
                        }

                        TODO_NOT_IMPLEMENTED;
                    }
                }
            }
            else
            {
                DWORD dwDstTotalBytes = 0;
                DWORD dwSrcTotalBytes = 0;
                DWORD dwSrcConvertBytes = 0;
                do
                {
                    if (dwDstBytes <= dwDstTotalBytes)
                        break;

                    dwSrcConvertBytes = m_cbConversionBuffer;
                    if (dwSrcBytes - dwSrcTotalBytes < dwSrcConvertBytes &&
                        (dwSrcConvertBytes = dwSrcBytes - dwSrcTotalBytes, dwSrcConvertBytes == 0))
                        break;

                    dwSrcSamples = BytesToSamples(&m_wfxSrcFormat, dwSrcConvertBytes);
                    dwConvertedSamples = BytesToSamples(&m_wfxSrcFormat, dwSrcTotalBytes);

                    hr = m_Source->GetSampleData(m_lpvConversionBuffer, dwConvertedSamples + dwSrcPosition,
                                                 &dwSrcSamples, lpRequestParams);
                    if (FAILED(hr))
                        break;

                    dwSrcConvertBytes = SamplesToBytes(&m_wfxSrcFormat, dwSrcSamples);
                    DWORD dwDstConvertBytes = GetDestSize(dwSrcConvertBytes, &m_ConversionData);
                    if (dwDstBytes - dwDstTotalBytes < dwDstConvertBytes)
                    {
                        dwDstConvertBytes = dwDstBytes - dwDstTotalBytes;
                        dwSrcConvertBytes = GetSourceSize(dwDstConvertBytes, &m_ConversionData);
                    }

                    dwDstConvertBytes = m_PCMConvertProc(m_lpvConversionBuffer, dwSrcConvertBytes, lpBuffer,
                                                         dwDstConvertBytes, &m_ConversionData);
                    dwSrcTotalBytes = dwSrcTotalBytes + dwSrcConvertBytes;
                    dwDstTotalBytes = dwDstTotalBytes + dwDstConvertBytes;
                    lpBuffer = lpBuffer + dwDstConvertBytes;

                } while (hr != S_ENDOFSOUND);

                dwConvertedSamples = BytesToSamples(&m_wfxDestFormat, dwDstTotalBytes);
            }

            dwDstSamples = dwConvertedSamples;
            if (*lpdwSamples != dwDstSamples)
            {
                *lpdwSamples = dwDstSamples;
            }
        }
    }

    return hr;
}

STDMETHODIMP CAMConvertFilter::SetCacheSize(DWORD dwCacheSize)
{
    HRESULT hr = S_OK;

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else
    {
        DWORD srcSamples = ConvertToSrcSamples(dwCacheSize);
        hr = m_Source->SetCacheSize(srcSamples);
    }

    return hr;
}

STDMETHODIMP CAMConvertFilter::SetMode(BOOL fActive, BOOL fRecurse)
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

STDMETHODIMP CAMConvertFilter::Init(LPSOUND pSoundSrc, LPWAVEFORMATEX lpwfxDest)
{
    HRESULT hr = S_OK;

    if (lpwfxDest == NULL || pSoundSrc == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        memcpy(&m_wfxDestFormat, lpwfxDest, sizeof(m_wfxDestFormat));
        m_Source = pSoundSrc;
        hr = pSoundSrc->GetFormat(&m_wfxSrcFormat, sizeof(m_wfxSrcFormat));
        if (SUCCEEDED(hr))
        {
            m_PCMConvertProc = GetPCMConvertProc(&m_wfxSrcFormat, &m_wfxDestFormat, &m_ConversionData);
            if (m_PCMConvertProc == NULL)
            {
                hr = E_FAIL;
            }
            else
            {
                DWORD dwSize = MillisecToBytes(&m_wfxDestFormat, 0x4b);
                m_cbConversionBuffer = GetSourceSize(dwSize, &m_ConversionData);

                // Allocate a new conversion buffer
                if (dwSize <= m_cbConversionBuffer && m_cbConversionBuffer != dwSize)
                {
                    m_lpvConversionBuffer = new BYTE[m_cbConversionBuffer];
                    if (m_lpvConversionBuffer == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }

                if (SUCCEEDED(hr))
                {
                    m_Source->AddRef();

                    m_Initialized = TRUE;
                }
            }
        }
    }

    return hr;
}

CAMConvertFilter::~CAMConvertFilter()
{
    DPRINTF(1, "Destructing Convert Filter");

    if (m_Initialized)
    {
        assert(m_ActiveCount == 0);

        if (m_PCMConvertProc != NULL)
        {
            ReleasePCMConvertProc(&m_ConversionData);
        }

        if (m_lpvConversionBuffer != NULL)
        {
            delete[] m_lpvConversionBuffer;
        }

        if (m_Source != NULL)
        {
            m_Source->Release();
        }

        m_Initialized = FALSE;
    }
}

DWORD CAMConvertFilter::ConvertToDstSamples(DWORD dwSrcSamples)
{
    DWORD dwDstSamples = INFINITE_SAMPLES;

    if (dwSrcSamples != INFINITE_SAMPLES)
    {
        DWORD dwSrcBytes = SamplesToBytes(&m_wfxSrcFormat, dwSrcSamples);
        DWORD dwDstBytes = GetDestSize(dwSrcBytes, &m_ConversionData);
        dwDstSamples = BytesToSamples(&m_wfxDestFormat, dwDstBytes);
    }

    return dwDstSamples;
}

DWORD CAMConvertFilter::ConvertToSrcSamples(DWORD dwDstSamples)
{
    DWORD dwSrcSamples = INFINITE_SAMPLES;

    if (dwDstSamples != INFINITE_SAMPLES)
    {
        DWORD dwDstBytes = SamplesToBytes(&m_wfxDestFormat, dwDstSamples);
        DWORD dwSrcBytes = GetSourceSize(dwDstBytes, &m_ConversionData);
        dwSrcSamples = BytesToSamples(&m_wfxSrcFormat, dwSrcBytes);
    }

    return dwSrcSamples;
}

STDAPI AllocConvertFilter(LPSOUND FAR *ppSound, LPSOUND pSoundSrc, LPWAVEFORMATEX lpwfx)
{
    HRESULT hr = S_OK;
    LPSOUND pSnd = NULL;
    IAMConvertFilter *pConvert = NULL;

    assert(ppSound != NULL);
    assert(pSoundSrc != NULL);

    hr = AMCreate(CLSID_AMConvertFilter, IID_IAMConvertFilter, (void **)&pConvert, &pSnd);

    if (SUCCEEDED(hr))
    {
        hr = pConvert->Init(pSoundSrc, lpwfx);

        AMFinish(hr, ppSound, pConvert, pSnd);
    }

    return hr;
}