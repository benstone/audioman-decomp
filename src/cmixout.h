#pragma once
#include "audiomaninternal.h"
#include "crealout.h"
#include "cfakeout.h"

class CMixerOut : public IAMWaveOut
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

    MMRESULT Init(IUnknown *waveOut);

    HRESULT STDMETHODCALLTYPE Suspend(BOOL fSuspend);

    void SuspendPump();

    ~CMixerOut();

  private:
    ULONG m_RefCnt = 0;
    BOOL m_Initialized = FALSE;

    LPWAVEOUT m_ActiveWaveOut = NULL;
    CRealOut *m_CRealOut = NULL;
    CFakeOut *m_CFakeOut = NULL;
    LPWAVEOUT m_RealWaveOut = NULL;
    ULONG m_Volume = 0xFFFFFFFF;
};