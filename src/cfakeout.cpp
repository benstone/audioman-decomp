#include "cfakeout.h"
#include "utils.h"
#include "dpf.h"
#include <cassert>
#include <tchar.h>

STDMETHODIMP_(ULONG) CFakeOut::AddRef()
{
    return ++m_RefCnt;
}

STDMETHODIMP_(ULONG) CFakeOut::Release()
{
    ULONG RefCnt = --m_RefCnt;

    if (RefCnt == 0)
    {
        delete this;
    }

    return RefCnt;
}

STDMETHODIMP CFakeOut::QueryInterface(REFIID riid, void **ppvObject)
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

STDMETHODIMP_(UINT) CFakeOut::GetNumDevs()
{
    return 1;
}

STDMETHODIMP_(MMRESULT)
CFakeOut::Open(UINT uDeviceID, LPWAVEFORMATEX lpwfx, DWORD dwCallback, DWORD dwCallbackInstance, DWORD fdwOpen)
{
    MMRESULT mmr = MMSYSERR_NOERROR;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_NOTENABLED;
    }
    else if (m_WaveFormatSet == TRUE)
    {
        mmr = MMSYSERR_ALLOCATED;
    }
    else
    {
        if (ConvertWaveFormatExToFormat(lpwfx) == 0)
        {
            mmr = WAVERR_BADFORMAT;
        }
        else if ((fdwOpen & 1) == 0)
        {
            if (fdwOpen == 0)
            {
                memcpy(&m_wfxSource, lpwfx, sizeof(m_wfxSource));
                m_WaveFormatSet = TRUE;
            }
            else
            {
                mmr = MMSYSERR_NOTSUPPORTED;
            }
        }
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CFakeOut::Close()
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_NOTENABLED;
    }
    else
    {
        mmr = MMSYSERR_NOERROR;
        if (m_WaveFormatSet == FALSE)
        {
            mmr = MMSYSERR_ALLOCATED;
        }
        else
        {
            m_WaveFormatSet = FALSE;
        }
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CFakeOut::GetDevCaps(UINT uDeviceID, LPWAVEOUTCAPS lpCaps, UINT cbCaps)
{
    MMRESULT mmr = MMSYSERR_NOERROR;
    WAVEOUTCAPS waveOutCaps;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_NOTENABLED;
    }
    else if (uDeviceID == 0)
    {
        waveOutCaps.wMid = 1;
        waveOutCaps.wPid = 6;
        waveOutCaps.vDriverVersion = 0x100;
        waveOutCaps.dwFormats = 0xfff;
        waveOutCaps.wChannels = 2;
        waveOutCaps.dwSupport = 0xc;
        _tcscpy(waveOutCaps.szPname, TEXT("AudioMan WaveOut Simulator"));
        if (cbCaps > sizeof(waveOutCaps))
        {
            cbCaps = sizeof(waveOutCaps);
        }
        memcpy(lpCaps, &waveOutCaps, cbCaps);
    }
    else
    {
        mmr = MMSYSERR_BADDEVICEID;
    }

    return mmr;
}

STDMETHODIMP_(MMRESULT) CFakeOut::GetVolume(LPDWORD lpdwVolume)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE || lpdwVolume == NULL)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        *lpdwVolume = m_Volume;
        mmr = MMSYSERR_NOERROR;
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CFakeOut::SetVolume(DWORD dwVolume)
{
    MMRESULT mmr;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        m_Volume = dwVolume;
        mmr = MMSYSERR_NOERROR;
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CFakeOut::PrepareHeader(LPWAVEHDR pwh, UINT cbwh)
{
    MMRESULT mmr = MMSYSERR_NOERROR;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_NOTENABLED;
    }
    else if (pwh == NULL)
    {
        mmr = MMSYSERR_INVALPARAM;
    }
    else if ((pwh->dwFlags & WHDR_PREPARED) == 0)
    {
        pwh->dwFlags |= WHDR_PREPARED;
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CFakeOut::UnprepareHeader(LPWAVEHDR pwh, UINT cbwh)
{
    MMRESULT mmr = MMSYSERR_NOERROR;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_NOTENABLED;
    }
    else if (pwh == NULL)
    {
        mmr = MMSYSERR_INVALPARAM;
    }
    else if ((pwh->dwFlags & WHDR_PREPARED) == 0)
    {
        pwh->dwFlags &= ~WHDR_PREPARED;
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CFakeOut::Write(LPWAVEHDR pwh, UINT cbwh)
{
    MMRESULT mmr = MMSYSERR_NOERROR;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_NOTENABLED;
    }
    else if (pwh == NULL)
    {
        mmr = MMSYSERR_INVALPARAM;
    }
    else if ((pwh->dwFlags & WHDR_PREPARED) == 0)
    {
        // This error code doesn't map to an MMSYSERR_* constant
        mmr = 0x22;
    }
    else if (pwh->lpData == NULL)
    {
        mmr = MMSYSERR_ERROR;
    }
    else if (pwh->dwBufferLength == 0)
    {
        pwh->dwFlags |= WHDR_DONE;
    }
    else
    {
        if (pwh->dwBufferLength % m_wfxSource.nBlockAlign != 0)
        {
            pwh->dwBufferLength = pwh->dwBufferLength - (pwh->dwBufferLength % m_wfxSource.nBlockAlign);
            DPRINTF(0, "WARNING::TRUNCATING BUFFER, NOT BLOCK ALIGNED WITH FORMAT");
        }

        pwh->lpNext = NULL;
        AppendHeaderToQueue(pwh);

        if (m_Paused == FALSE && m_BufferQueue == pwh)
        {
            StartPlayback();
        }
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CFakeOut::Pause()
{
    MMRESULT mmr = MMSYSERR_NOERROR;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else
    {
        if (m_Paused == FALSE)
        {
            m_Paused = TRUE;
        }
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CFakeOut::Restart()
{
    MMRESULT mmr = MMSYSERR_NOERROR;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_INVALHANDLE;
    }
    else if (m_Paused)
    {
        m_Paused = FALSE;
        StartPlayback();
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CFakeOut::Reset()
{
    MMRESULT mmr = MMSYSERR_NOERROR;

    if (m_Initialized == FALSE)
    {
        mmr = MMSYSERR_NOTENABLED;
    }
    else
    {
        LPWAVEHDR current = m_BufferQueue;
        m_BufferQueue = NULL;

        while (current != NULL)
        {
            LPWAVEHDR next = current->lpNext;
            current->lpNext = NULL;
            current->dwFlags |= WHDR_DONE;
            current->dwFlags &= ~WHDR_INQUEUE;
            current = next;
        }
    }
    return mmr;
}

STDMETHODIMP_(MMRESULT) CFakeOut::BreakLoop()
{
    return MMSYSERR_NOTSUPPORTED;
}

STDMETHODIMP_(MMRESULT) CFakeOut::GetPosition(LPMMTIME lpmmt, UINT cbmmt)
{
    return MMSYSERR_NOTSUPPORTED;
}

STDMETHODIMP_(MMRESULT) CFakeOut::GetPitch(LPDWORD lpdwPitch)
{
    return MMSYSERR_NOTSUPPORTED;
}

STDMETHODIMP_(MMRESULT) CFakeOut::SetPitch(DWORD dwPitch)
{
    return MMSYSERR_NOTSUPPORTED;
}

STDMETHODIMP_(MMRESULT) CFakeOut::GetPlaybackRate(LPDWORD lpdwRate)
{
    return MMSYSERR_NOTSUPPORTED;
}

STDMETHODIMP_(MMRESULT) CFakeOut::SetPlaybackRate(DWORD dwRate)
{
    return MMSYSERR_NOTSUPPORTED;
}

STDMETHODIMP_(MMRESULT) CFakeOut::GetID(UINT *lpuDeviceID)
{
    return MMSYSERR_NOTSUPPORTED;
}

STDMETHODIMP_(MMRESULT) CFakeOut::Message(UINT uMsg, DWORD dw1, DWORD dw2)
{
    return MMSYSERR_NOTSUPPORTED;
}

STDMETHODIMP_(MMRESULT) CFakeOut::GetErrorText(MMRESULT err, LPSTR lpText, UINT cchText)
{
    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT CFakeOut::Init()
{
    m_Initialized = TRUE;
    return MMSYSERR_NOERROR;
}

void CFakeOut::AppendHeaderToQueue(LPWAVEHDR pwh)
{
    if (m_BufferQueue == NULL)
    {
        m_BufferQueue = pwh;
    }
    else
    {
        LPWAVEHDR this_wave_header;
        for (this_wave_header = m_BufferQueue; this_wave_header->lpNext != NULL; this_wave_header = this_wave_header->lpNext)
        {
            // do nothing
        }

        this_wave_header->lpNext = pwh;
    }
    pwh->dwFlags |= WHDR_INQUEUE;
}

void CFakeOut::StartPlayback()
{
    if (m_BufferQueue != NULL && m_Playing == FALSE)
    {
        m_StartTime = BytesToMillisec(&m_wfxSource, m_BufferQueue->dwBufferLength);
        m_Playing = TRUE;
        m_StartTime += timeGetTime();
    }
}

void CFakeOut::SuspendPump()
{
    if (m_Playing)
    {
        if (m_StartTime <= timeGetTime())
        {
            m_Playing = FALSE;
            m_BufferQueue->dwFlags &= WHDR_INQUEUE;
            m_BufferQueue->dwFlags |= WHDR_DONE;
            m_BufferQueue = m_BufferQueue->lpNext;
            if (m_BufferQueue != NULL && m_Paused == FALSE)
            {
                StartPlayback();
            }
        }
    }
}

CFakeOut::~CFakeOut()
{
    // do nothing
}
