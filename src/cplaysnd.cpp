#include "cplaysnd.h"
#include "dpf.h"
#include <cassert>

STDMETHODIMP_(ULONG) CAMPlaySnd::AddRef()
{
    return ++m_RefCnt;
}

STDMETHODIMP_(ULONG) CAMPlaySnd::Release()
{
    ULONG RefCnt = --m_RefCnt;

    if (RefCnt == 0)
    {
        delete this;
    }

    return RefCnt;
}

STDMETHODIMP CAMPlaySnd::QueryInterface(REFIID riid, void **ppvObject)
{
    HRESULT hr = S_OK;

    assert(ppvObject != NULL);

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAMNotifySink))
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

CAMPlaySnd::~CAMPlaySnd()
{
    DPRINTF(1, "Destructing PlaySnd object");
    if (m_Channel != NULL)
    {
        m_Channel->Release();
    }
}

HRESULT CAMPlaySnd::Play(LPMIXER pMixer, LPSOUND pSound)
{
    HRESULT hr;

    if (pMixer == NULL || pSound == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = pMixer->AllocChannel(&m_Channel);
        if (SUCCEEDED(hr))
        {
            hr = m_Channel->RegisterNotify(this, NOTIFYSINK_ONCOMPLETION | NOTIFYSINK_ONERROR);
        }

        if (SUCCEEDED(hr))
        {
            hr = m_Channel->SetSoundSrc(pSound);
        }

        if (SUCCEEDED(hr))
        {
            hr = m_Channel->Play();
        }
    }

    if (FAILED(hr))
    {
        if (m_Channel != NULL)
        {
            m_Channel->Release();
            m_Channel = NULL;
        }
    }
    else
    {
        AddRef();
    }

    return hr;
}

STDMETHODIMP_(void __stdcall) CAMPlaySnd::OnStart(LPSOUND pSound, DWORD dwPosition)
{
    // do nothing
}

STDMETHODIMP_(void __stdcall) CAMPlaySnd::OnCompletion(LPSOUND pSound, DWORD dwPosition)
{
    DPRINTF(1, "OnCompletion at %lu", dwPosition);

    m_Channel->RegisterNotify(NULL, 0);

    Release();
}

STDMETHODIMP_(void __stdcall) CAMPlaySnd::OnError(LPSOUND pSound, DWORD dwPosition, HRESULT hrError)
{
    DPRINTF(1, "OnError at %lu", dwPosition);

    m_Channel->RegisterNotify(NULL, 0);

    Release();
}

STDMETHODIMP_(void __stdcall) CAMPlaySnd::OnSyncObject(LPSOUND pSound, DWORD dwPosition, void *pvObject)
{
    // do nothing
}