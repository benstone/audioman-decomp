#include <cassert>
#include "sndpass.h"
#include "dpf.h"

STDMETHODIMP_(ULONG) __stdcall CAMPassThruFilter::AddRef()
{
    return ++m_RefCnt;
}

STDMETHODIMP_(ULONG) CAMPassThruFilter::Release()
{
    ULONG RefCnt = --m_RefCnt;

    if (RefCnt == 0)
    {
        delete this;
    }

    return RefCnt;
}

STDMETHODIMP CAMPassThruFilter::QueryInterface(REFIID riid, void **ppvObject)
{
    assert(ppvObject != NULL);

    HRESULT hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAMSound))
    {
        *ppvObject = (IAMSound *)this;
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

STDMETHODIMP CAMPassThruFilter::GetFormat(LPWAVEFORMATEX pFormat, DWORD cbSize)
{
    if (m_Initialized == FALSE)
    {
        return E_FAIL;
    }
    else
    {
        return m_Sound->GetFormat(pFormat, cbSize);
    }
}

STDMETHODIMP_(DWORD) CAMPassThruFilter::GetSamples()
{
    if (m_Initialized == FALSE)
    {
        return 0;
    }
    else
    {
        return m_Sound->GetSamples();
    }
}

STDMETHODIMP CAMPassThruFilter::GetAlignment(LPDWORD lpdwLeftAlign, LPDWORD lpdwRightAlign)
{
    if (m_Initialized == FALSE)
    {
        return E_FAIL;
    }
    else
    {
        return m_Sound->GetAlignment(lpdwLeftAlign, lpdwRightAlign);
    }
}

STDMETHODIMP
CAMPassThruFilter::GetSampleData(LPBYTE lpBuffer, DWORD dwPosition, LPDWORD lpdwSamples, LPREQUESTPARAM lpRequestParams)
{
    if (m_Initialized == FALSE || m_ActiveCount < 1)
    {
        return E_FAIL;
    }
    else
    {
        return m_Sound->GetSampleData(lpBuffer, dwPosition, lpdwSamples, lpRequestParams);
    }
}

STDMETHODIMP CAMPassThruFilter::SetCacheSize(DWORD dwCacheSize)
{
    if (m_Initialized == FALSE)
    {
        return E_FAIL;
    }
    else
    {
        return m_Sound->SetCacheSize(dwCacheSize);
    }
}

STDMETHODIMP CAMPassThruFilter::SetMode(BOOL fActive, BOOL fRecurse)
{
    HRESULT hr = S_OK;

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else
    {
        if (!fRecurse)
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
            hr = m_Sound->SetMode(fActive, fRecurse);
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
    }

    assert(m_ActiveCount >= 0);

    return hr;
}

CAMPassThruFilter::~CAMPassThruFilter()
{
    DPRINTF(1, "Destructing PassThru Filter");

    if (m_Initialized)
    {
        assert(m_ActiveCount == 0);

        if (m_Sound != NULL)
        {
            m_Sound->Release();
        }

        m_Initialized = FALSE;
    }
}
