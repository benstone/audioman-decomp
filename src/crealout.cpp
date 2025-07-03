#include "crealout.h"
#include <cassert>

STDMETHODIMP_(ULONG) CRealOut::AddRef()
{
    return ++m_RefCnt;
}

STDMETHODIMP_(ULONG) CRealOut::Release()
{
    ULONG RefCnt = --m_RefCnt;

    if (RefCnt == 0)
    {
        delete this;
    }

    return RefCnt;
}

STDMETHODIMP CRealOut::QueryInterface(REFIID riid, void **ppvObject)
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

STDMETHODIMP_(UINT) CRealOut::GetNumDevs()
{
    return waveOutGetNumDevs();
}

STDMETHODIMP_(MMRESULT)
CRealOut::Open(UINT uDeviceID, LPWAVEFORMATEX lpwfx, DWORD dwCallback, DWORD dwCallbackInstance, DWORD fdwOpen)
{
    MMRESULT mmr = waveOutOpen(&m_Device, uDeviceID, lpwfx, dwCallback, dwCallbackInstance, fdwOpen);
    if (mmr == MMSYSERR_NOERROR)
    {
        waveOutGetID(m_Device, &m_DeviceID);
    }
    else
    {
        m_Device = NULL;
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CRealOut::Close()
{
    MMRESULT mmr = MMSYSERR_NOERROR;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else if (m_Device != NULL)
    {
        mmr = waveOutClose(m_Device);
        if (mmr == MMSYSERR_NOERROR)
        {
            m_Device = NULL;
        }
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CRealOut::GetDevCaps(UINT uDeviceID, LPWAVEOUTCAPS lpCaps, UINT cbCaps)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = waveOutGetDevCaps(uDeviceID, lpCaps, cbCaps);
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CRealOut::GetVolume(LPDWORD lpdwVolume)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = waveOutGetVolume(m_Device, lpdwVolume);
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CRealOut::SetVolume(DWORD dwVolume)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = waveOutSetVolume(m_Device, dwVolume);
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CRealOut::PrepareHeader(LPWAVEHDR pwh, UINT cbwh)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = waveOutPrepareHeader(m_Device, pwh, cbwh);
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CRealOut::UnprepareHeader(LPWAVEHDR pwh, UINT cbwh)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = waveOutUnprepareHeader(m_Device, pwh, cbwh);
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CRealOut::Write(LPWAVEHDR pwh, UINT cbwh)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = waveOutWrite(m_Device, pwh, cbwh);
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CRealOut::Pause()
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = waveOutPause(m_Device);
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CRealOut::Restart()
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = waveOutRestart(m_Device);
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CRealOut::Reset()
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = waveOutReset(m_Device);
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CRealOut::BreakLoop()
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = waveOutBreakLoop(m_Device);
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CRealOut::GetPosition(LPMMTIME lpmmt, UINT cbmmt)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = waveOutGetPosition(m_Device, lpmmt, cbmmt);
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CRealOut::GetPitch(LPDWORD lpdwPitch)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = waveOutGetPitch(m_Device, lpdwPitch);
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CRealOut::SetPitch(DWORD dwPitch)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = waveOutSetPitch(m_Device, dwPitch);
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CRealOut::GetPlaybackRate(LPDWORD lpdwRate)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = waveOutGetPlaybackRate(m_Device, lpdwRate);
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CRealOut::SetPlaybackRate(DWORD dwRate)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = waveOutSetPlaybackRate(m_Device, dwRate);
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CRealOut::GetID(UINT *lpuDeviceID)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = waveOutGetID(m_Device, lpuDeviceID);
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CRealOut::Message(UINT uMsg, DWORD dw1, DWORD dw2)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = waveOutMessage(m_Device, uMsg, dw1, dw2);
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CRealOut::GetErrorText(MMRESULT err, LPSTR lpText, UINT cchText)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        mmr = waveOutGetErrorTextA(err, lpText, cchText);
    }
    return mmr;
}

MMRESULT CRealOut::Init()
{
    m_Initialized = TRUE;
    return MMSYSERR_NOERROR;
}

CRealOut::~CRealOut()
{
    if (m_Device != NULL)
    {
        waveOutReset(m_Device);
        waveOutClose(m_Device);
        m_Device = NULL;
    }
}