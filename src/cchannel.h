#pragma once
#include "audiomaninternal.h"
#include "mutex.h"
#include "cmixlib.h"

typedef struct
{
    BYTE *lpData;
    BYTE *lpAux;
    BOOL fHaveMixed;
    DWORD dwBuffer;
    DWORD unknownA; // TODO: what is this used for
    DWORD MixedVoices;
} MIXHEADER;

typedef struct
{
    BOOL fUnknown;
    DWORD Flags;
    DWORD StartPosition;
    DWORD EndPosition;
    BYTE unknown[0xc]; // TODO: what is this used for
} ChannelBuffer;

// Mixer channel interface
#define _IID_IAMMixerChannel A0434E41 - 9573 - 11CE-B61B - 00AA006EBBE5
DEFINE_GUID(IID_IAMMixerChannel, 0xA0434E41L, 0x9573, 0x11CE, 0xB6, 0x1B, 0x00, 0xAA, 0x00, 0x6E, 0xBB, 0xE5);

DECLARE_INTERFACE_(IAMMixerChannel, IUnknown)
{
    // IUnknown methods

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IAMMixerChannel methods

    STDMETHOD(SetMutx)(THIS_ CAMMutex * mutex) PURE;
    STDMETHOD(Configure)(THIS_ DWORD dwNumBuffers, DWORD dwNumSamples, WAVEFORMATEX * lpwfx) PURE;
    STDMETHOD_(BOOL, MixBuffer)(THIS_ MIXHEADER * lpMixHeader) PURE;
    STDMETHOD(MixNotify)(THIS_ MIXHEADER * lpMixHeader) PURE;
    STDMETHOD(RevertTo)(THIS_ ULONG dwBuffer) PURE;
    STDMETHOD_(ULONG, GetActiveState)(THIS_) PURE;
    STDMETHOD(DoStart)(THIS_ BOOL fStart) PURE;
    STDMETHOD(DoReset)(THIS_) PURE;
    STDMETHOD(DoPosition)(THIS_ DWORD dwPosition) PURE;
    STDMETHOD(DoVolume)(THIS_ DWORD dwVolume) PURE;
    STDMETHOD(DoGain)(THIS_ float flLeft, float flRight, BOOL fAbsolute) PURE;
    STDMETHOD(SetNext)(THIS_ IAMMixerChannel * pNextChannel) PURE;
    STDMETHOD_(IAMMixerChannel *, GetNext)(THIS_) PURE;
    STDMETHOD(SetPriority)(THIS_ DWORD dwPriority) PURE;
    STDMETHOD_(DWORD, GetPriority)(THIS_) PURE;
    STDMETHOD(SetGroup)(THIS_ DWORD dwGroup) PURE;
    STDMETHOD_(DWORD, GetGroup)(THIS_) PURE;
};

class CAMChannel : public IAMChannel, IAMMixerChannel
{
  public:
    // IUnknown methods

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR *ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    // IAMChannel methods

    STDMETHOD(RegisterNotify)(THIS_ LPNOTIFYSINK pNotifySink, DWORD fdwNotifyFlags);
    STDMETHOD(SetSoundSrc)(THIS_ LPSOUND pSound);
    STDMETHOD(SetCachedSrc)(THIS_ LPSOUND pSound, LPCACHECONFIG pCacheConfig);
    STDMETHOD(GetSoundSrc)(THIS_ LPSOUND FAR *ppSound);
    STDMETHOD(Play)(THIS);
    STDMETHOD(Stop)(THIS);
    STDMETHOD(Finish)(THIS);
    STDMETHOD_(BOOL, IsPlaying)(THIS);
    STDMETHOD_(DWORD, Samples)(THIS);
    STDMETHOD(SetPosition)(THIS_ DWORD dwSample);
    STDMETHOD(GetPosition)(THIS_ LPDWORD lpdwSample);
    STDMETHOD(Mute)(THIS_ BOOL fMute);
    STDMETHOD(SetVolume)(THIS_ DWORD dwVolume);
    STDMETHOD(GetVolume)(THIS_ LPDWORD lpdwVolume);
    STDMETHOD(SetGain)(THIS_ float flLeft, float flRight);
    STDMETHOD(GetGain)(THIS_ float FAR *lpflLeft, float FAR *lpflRight);
    STDMETHOD(GetSMPTEPos)(THIS_ LPSMPTE lpSMPTE);
    STDMETHOD(SetSMPTEPos)(THIS_ LPSMPTE lpSMPTE);
    STDMETHOD(GetTimePos)(THIS_ LPDWORD lpdwTime);
    STDMETHOD(SetTimePos)(THIS_ DWORD dwTime);

    // IAMMixerChannel methods

    STDMETHOD(SetMutx)(THIS_ CAMMutex *mutex);
    STDMETHOD(Configure)(THIS_ DWORD dwNumBuffers, DWORD dwNumSamples, WAVEFORMATEX *lpwfx);
    STDMETHOD_(BOOL, MixBuffer)(THIS_ MIXHEADER *lpMixHeader);
    STDMETHOD(MixNotify)(THIS_ MIXHEADER *lpMixHeader);
    STDMETHOD(RevertTo)(THIS_ DWORD dwBuffer);
    STDMETHOD_(DWORD, GetActiveState)(THIS_);
    STDMETHOD(DoStart)(THIS_ BOOL fStart);
    STDMETHOD(DoReset)(THIS_);
    STDMETHOD(DoPosition)(THIS_ DWORD dwPosition);
    STDMETHOD(DoVolume)(THIS_ DWORD dwVolume);
    STDMETHOD(DoGain)(THIS_ float flLeft, float flRight, BOOL fAbsolute);
    STDMETHOD(SetNext)(THIS_ IAMMixerChannel *pNextChannel);
    STDMETHOD_(IAMMixerChannel *, GetNext)(THIS_);
    STDMETHOD(SetPriority)(THIS_ DWORD dwPriority);
    STDMETHOD_(DWORD, GetPriority)(THIS_);
    STDMETHOD(SetGroup)(THIS_ DWORD dwGroup);
    STDMETHOD_(DWORD, GetGroup)(THIS_);

    CAMChannel();
    ~CAMChannel();

    STDMETHODIMP Init(LPMIXER pMixer);

  private:
    // Acquire lock for the channel
    void EnterChannel();

    // Release lock for the channel
    void LeaveChannel();

    /**
     * Reset notifications for a buffer
     * @param dwBuffer buffer to reset notifications for, or 0xFFFFFFFF to reset all
     **/
    void ClearNotify(DWORD dwBuffer);

    // Refresh the mixer
    void DoRemix();

    /**
     * Set the volume for left/right channels
     * @param wLeft - Left volume (0-16)
     * @param wRight - Right volume (0-16)
     **/
    void SetChannelVol(WORD wLeft, WORD wRight);

    ULONG m_RefCnt = 0;
    BOOL m_Initialized = FALSE;
    LPNOTIFYSINK m_NotifySink = FALSE;
    BYTE m_NotifyFlags = 0;

    // Original sound
    LPSOUND m_pOrgSrc = NULL;

    // Sound source: original sound or a conversion filter
    LPSOUND m_pSoundSrc = NULL;
    WAVEFORMATEX m_wfx = {0};

    LPMIXER m_pMixer = NULL;

    // Starting position in samples for each buffer
    DWORD *m_BufferPositions = NULL;
    ChannelBuffer *m_ArrayOfBuffers = NULL;
    BOOL m_ChannelRegistered = FALSE;
    IAMMixerChannel *m_pNextChannel = NULL;
    DWORD m_Priority = 0;
    DWORD m_Group = 0;
    BOOL m_IsPlaying = FALSE;
    BOOL m_IsActive = FALSE;

    BOOL m_FUnknown = FALSE; // TODO: what's this?

    DWORD m_OldStyleVolume = 0xFFFFFFFF;
    DWORD m_CombinedVolume = 0;
    WORD m_VolumeLeft = 16;
    WORD m_VolumeRight = 16;
    BOOL m_NeedsVolumeMix = FALSE;
    float m_GainLeft = 0;
    float m_GainRight = 0;
    BOOL m_MixerProcSet = FALSE;
    DWORD m_Position;

    // Total samples for the source sound
    DWORD m_Samples;

    // Size of each buffer in samples
    DWORD m_NumSamples;

    // Number of buffers
    DWORD m_NumBuffers;
    DWORD m_Position2; // TODO: what's this?
    BOOL m_SettingVolume = TRUE;
    BOOL m_Remixing = FALSE;
    BOOL m_Muted = FALSE;
    CAMMutex *m_ChannelLock;
    MixerProc m_MixerProc;
    MixerProc m_MixerProcVolOnly;
};