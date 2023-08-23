#include "cmixout.h"
#include <cassert>

STDMETHODIMP_(ULONG) CMixerOut::AddRef()
{
    return ++m_RefCnt;
}

STDMETHODIMP_(ULONG) CMixerOut::Release()
{
    ULONG RefCnt = --m_RefCnt;

    if (RefCnt == 0)
    {
        delete this;
    }
    
    return RefCnt;
}

STDMETHODIMP CMixerOut::QueryInterface(REFIID riid, void **ppvObject)
{
    assert(ppvObject != NULL);

    HRESULT hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAMMixer))
    {
        *ppvObject = (IAMMixer *)this;
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

STDMETHODIMP_(UINT) CMixerOut::GetNumDevs()
{
    UINT numDevs;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        numDevs = 0;
    }
    else
    {
        numDevs = m_ActiveWaveOut->GetNumDevs();
    }

    return numDevs;
}

STDMETHODIMP_(MMRESULT)
CMixerOut::Open(UINT uDeviceID, LPWAVEFORMATEX lpwfx, DWORD dwCallback, DWORD dwCallbackInstance, DWORD fdwOpen)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->Open(uDeviceID, lpwfx, dwCallback, dwCallbackInstance, fdwOpen);
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CMixerOut::Close()
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->Close();
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CMixerOut::GetDevCaps(UINT uDeviceID, LPWAVEOUTCAPS lpCaps, UINT cbCaps)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->GetDevCaps(uDeviceID, lpCaps, cbCaps);
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CMixerOut::GetVolume(LPDWORD lpdwVolume)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->GetVolume(lpdwVolume);
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CMixerOut::SetVolume(DWORD dwVolume)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->SetVolume(dwVolume);
        if (mmr == MMSYSERR_NOERROR)
        {
            m_Volume = dwVolume;
        }
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CMixerOut::PrepareHeader(LPWAVEHDR pwh, UINT cbwh)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->PrepareHeader(pwh, cbwh);
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CMixerOut::UnprepareHeader(LPWAVEHDR pwh, UINT cbwh)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->UnprepareHeader(pwh, cbwh);
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CMixerOut::Write(LPWAVEHDR pwh, UINT cbwh)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->Write(pwh, cbwh);
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CMixerOut::Pause()
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->Pause();
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CMixerOut::Restart()
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->Restart();
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CMixerOut::Reset()
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->Reset();
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CMixerOut::BreakLoop()
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->BreakLoop();
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CMixerOut::GetPosition(LPMMTIME lpmmt, UINT cbmmt)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->GetPosition(lpmmt, cbmmt);
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CMixerOut::GetPitch(LPDWORD lpdwPitch)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->GetPitch(lpdwPitch);
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CMixerOut::SetPitch(DWORD dwPitch)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->SetPitch(dwPitch);
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CMixerOut::GetPlaybackRate(LPDWORD lpdwRate)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->GetPlaybackRate(lpdwRate);
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CMixerOut::SetPlaybackRate(DWORD dwRate)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->SetPlaybackRate(dwRate);
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CMixerOut::GetID(UINT *lpuDeviceID)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->GetID(lpuDeviceID);
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CMixerOut::Message(UINT uMsg, DWORD dw1, DWORD dw2)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->Message(uMsg, dw1, dw2);
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CMixerOut::GetErrorText(MMRESULT err, LPSTR lpText, UINT cchText)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || m_ActiveWaveOut == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = m_ActiveWaveOut->GetErrorText(err, lpText, cchText);
    }

    return mmr;
}

MMRESULT CMixerOut::Init(IUnknown *waveOut)
{
    HRESULT hr = S_OK;

    if (waveOut == NULL)
    {
        m_CRealOut = new CRealOut();
        if (m_CRealOut == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = m_CRealOut->Init();
            if (SUCCEEDED(hr))
            {
                m_CRealOut->AddRef();
                m_ActiveWaveOut = m_CRealOut;
                m_Initialized = TRUE;
            }
            else
            {
                delete m_CRealOut;
                m_CRealOut = NULL;
            }
        }
    }
    else
    {
        hr = waveOut->QueryInterface(IID_IAMWaveOut, (void **)&m_RealWaveOut);

        if (SUCCEEDED(hr))
        {
            m_RealWaveOut->AddRef();
            m_ActiveWaveOut = m_RealWaveOut;
            m_Initialized = TRUE;
        }
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CMixerOut::Suspend(BOOL fSuspend)
{
    HRESULT hr = S_OK;

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else
    {
        if (fSuspend == TRUE && m_CFakeOut == NULL)
        {
            // Create a new fake output device
            m_CFakeOut = new CFakeOut();

            if (m_CFakeOut == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                hr = m_CFakeOut->Init();

                if (SUCCEEDED(hr))
                {
                    // Set the active wave out device to our CFakeOut instance
                    m_CFakeOut->AddRef();
                    m_ActiveWaveOut = m_CFakeOut;
                    SetVolume(m_Volume);
                }
            }
        }
        else if (m_CFakeOut != NULL)
        {
            if (m_CRealOut == NULL)
            {
                if (m_RealWaveOut != NULL)
                {
                    assert(m_RealWaveOut != NULL);

                    // Set the active wave out device to the real output device
                    m_ActiveWaveOut = m_RealWaveOut;
                    m_CFakeOut->Release();
                    m_CFakeOut = NULL;
                }
            }
            else
            {
                // Set the active wave out device to our CRealOut instance
                m_ActiveWaveOut = m_CRealOut;
                m_CFakeOut->Release();
                m_CFakeOut = NULL;
            }
        }
    }

    return hr;
}

void CMixerOut::SuspendPump()
{
    if (m_CFakeOut != NULL && m_CFakeOut == m_ActiveWaveOut)
    {
        m_CFakeOut->SuspendPump();
    }
}

CMixerOut::~CMixerOut()
{
    if (m_CRealOut != NULL)
    {
        m_CRealOut->Release();
    }
    if (m_CFakeOut != NULL)
    {
        m_CFakeOut->Release();
    }
    if (m_RealWaveOut != NULL)
    {
        m_RealWaveOut->Release();
    }
}