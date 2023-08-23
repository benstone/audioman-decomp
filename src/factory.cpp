#include "factory.h"
#include "sound.h"
#include "sndcnvt.h"
#include "sndloop.h"
#include "sndbias.h"
#include "sndgate.h"
#include "sndclip.h"
#include "sndtrim.h"
#include "sndcache.h"
#include "cmixer.h"

// Global instance of CMixer
LPMIXER gpMixer = NULL;

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppvObject)
{
    if (ppvObject == NULL)
    {
        return E_POINTER;
    }

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObject = (IUnknown *)this;
        AddRef();
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IClassFactory))
    {
        *ppvObject = (IClassFactory *)this;
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
}

ULONG CClassFactory::AddRef()
{
    return ++m_RefCnt;
}

ULONG CClassFactory::Release()
{
    ULONG RefCnt = --m_RefCnt;

    if (RefCnt == 0)
    {
        delete this;
    }

    return RefCnt;
}

STDMETHODIMP CClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
    HRESULT hr = S_OK;

    // Validate parameters
    if (pUnkOuter != NULL)
    {
        return CLASS_E_NOAGGREGATION;
    }
    if (ppvObject == NULL)
    {
        return E_INVALIDARG;
    }

    // Create a new instance
    if (IsEqualCLSID(m_clsid, CLSID_AMWavFileSrc))
    {
        *ppvObject = new CAMWavFileSrc();
    }
    else if (IsEqualCLSID(m_clsid, CLSID_AMConvertFilter))
    {
        *ppvObject = new CAMConvertFilter();
    }
    else if (IsEqualCLSID(m_clsid, CLSID_AMLoopFilter))
    {
        *ppvObject = new CAMLoopFilter();
    }
    else if (IsEqualCLSID(m_clsid, CLSID_AMBiasFilter))
    {
        *ppvObject = new CAMBiasFilter();
    }
    else if (IsEqualCLSID(m_clsid, CLSID_AMGateFilter))
    {
        *ppvObject = new CAMGateFilter();
    }
    else if (IsEqualCLSID(m_clsid, CLSID_AMClipFilter))
    {
        *ppvObject = new CAMClipFilter();
    }
    else if (IsEqualCLSID(m_clsid, CLSID_AMTrimFilter))
    {
        *ppvObject = new CAMTrimFilter();
    }
    else if (IsEqualCLSID(m_clsid, CLSID_AMCacheFilter))
    {
        *ppvObject = new CAMCacheFilter();
    }
    else if (IsEqualCLSID(m_clsid, CLSID_AMMixer))
    {
        if (gpMixer == NULL)
        {
            gpMixer = new CAMMixer();
        }

        *ppvObject = gpMixer;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    if (SUCCEEDED(hr) && *ppvObject == NULL)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        // Call QueryInterface
        hr = ((IUnknown *)*ppvObject)->QueryInterface(riid, ppvObject);
        if (FAILED(hr))
        {
            ((IUnknown *)*ppvObject)->Release();
            *ppvObject = NULL;
        }
    }

    return hr;
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    return S_OK;
}

BOOL CClassFactory::Init(REFCLSID rclsid)
{
    m_clsid = rclsid;

    // Return true if supported CLSID
    if (IsEqualCLSID(m_clsid, CLSID_AMWavFileSrc) || IsEqualCLSID(m_clsid, CLSID_AMConvertFilter) ||
        IsEqualCLSID(m_clsid, CLSID_AMLoopFilter) || IsEqualCLSID(m_clsid, CLSID_AMBiasFilter) ||
        IsEqualCLSID(m_clsid, CLSID_AMGateFilter) || IsEqualCLSID(m_clsid, CLSID_AMClipFilter) ||
        IsEqualCLSID(m_clsid, CLSID_AMTrimFilter) || IsEqualCLSID(m_clsid, CLSID_AMCacheFilter) ||
        IsEqualCLSID(m_clsid, CLSID_AMMixer))
    {
        return TRUE;
    }

    return FALSE;
}
