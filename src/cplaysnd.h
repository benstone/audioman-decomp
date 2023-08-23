#pragma once
#include "audiomaninternal.h"

class CAMPlaySnd : public IAMNotifySink
{
  public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR *ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    // IAMNotifySink methods
    STDMETHOD_(void, OnStart)(THIS_ LPSOUND pSound, DWORD dwPosition);
    STDMETHOD_(void, OnCompletion)(THIS_ LPSOUND pSound, DWORD dwPosition);
    STDMETHOD_(void, OnError)(THIS_ LPSOUND pSound, DWORD dwPosition, HRESULT hrError);
    STDMETHOD_(void, OnSyncObject)(THIS_ LPSOUND pSound, DWORD dwPosition, void *pvObject);

    ~CAMPlaySnd();

    /**
     * Play a sound
     * @param pMixer Mixer to play sound on
     * @param pSound Sound to play
     * @note the sound will play asynchronously
     **/
    HRESULT Play(LPMIXER pMixer, LPSOUND pSound);

  private:
    ULONG m_RefCnt = 0;
    LPCHANNEL m_Channel = NULL;
};