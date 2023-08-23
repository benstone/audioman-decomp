#pragma once
#include "audiomaninternal.h"

class CFakeOut : public IAMWaveOut
{
  public:
    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR *ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IAMWaveOut methods

    STDMETHOD_(UINT, GetNumDevs)();

    STDMETHOD_(MMRESULT, Open)
    (UINT uDeviceID, LPWAVEFORMATEX lpwfx, DWORD dwCallback, DWORD dwCallbackInstance, DWORD fdwOpen);

    STDMETHOD_(MMRESULT, Close)();

    STDMETHOD_(MMRESULT, GetDevCaps)(UINT uDeviceID, LPWAVEOUTCAPS lpCaps, UINT cbCaps);

    STDMETHOD_(MMRESULT, GetVolume)(LPDWORD lpdwVolume);

    STDMETHOD_(MMRESULT, SetVolume)(DWORD dwVolume);

    STDMETHOD_(MMRESULT, PrepareHeader)(LPWAVEHDR pwh, UINT cbwh);

    STDMETHOD_(MMRESULT, UnprepareHeader)(LPWAVEHDR pwh, UINT cbwh);

    STDMETHOD_(MMRESULT, Write)(LPWAVEHDR pwh, UINT cbwh);

    STDMETHOD_(MMRESULT, Pause)();

    STDMETHOD_(MMRESULT, Restart)();

    STDMETHOD_(MMRESULT, Reset)();

    STDMETHOD_(MMRESULT, BreakLoop)();

    STDMETHOD_(MMRESULT, GetPosition)(LPMMTIME lpmmt, UINT cbmmt);

    STDMETHOD_(MMRESULT, GetPitch)(LPDWORD lpdwPitch);

    STDMETHOD_(MMRESULT, SetPitch)(DWORD dwPitch);

    STDMETHOD_(MMRESULT, GetPlaybackRate)(LPDWORD lpdwRate);

    STDMETHOD_(MMRESULT, SetPlaybackRate)(DWORD dwRate);

    STDMETHOD_(MMRESULT, GetID)(UINT FAR *lpuDeviceID);

    STDMETHOD_(MMRESULT, Message)(UINT uMsg, DWORD dw1, DWORD dw2);

    STDMETHOD_(MMRESULT, GetErrorText)(MMRESULT err, LPSTR lpText, UINT cchText);

    virtual ~CFakeOut();

    MMRESULT Init();

    void SuspendPump();

  private:
    ULONG m_RefCnt;
    BOOL m_Initialized;
    BOOL m_WaveFormatSet;
    BOOL m_Paused;
    WAVEFORMATEX m_wfxSource;
    DWORD m_Volume;
    DWORD m_StartTime;
    BOOL m_Playing;
    WAVEHDR *m_BufferQueue;

    void AppendHeaderToQueue(LPWAVEHDR pwh);
    void StartPlayback();
};