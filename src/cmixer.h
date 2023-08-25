#pragma once
#include "audiomaninternal.h"
#include "mutex.h"
#include "cmixer.h"
#include "cchannel.h"
#include "cmixout.h"

typedef struct
{
    WAVEFORMATEX Format;
    WORD BufferTime;
    WORD Voices;
    BOOL RemixEnabled;
} MIXERSETTINGS;

typedef struct
{
    WAVEHDR WaveHeader;
    MIXHEADER MixHeader;
    BOOL InUse;
    BOOL Remix;
} MIXBUFFER;

class CAMMixer : public IAMMixer
{
  public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR *ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IAMMixer methods
    STDMETHOD(TestConfig)
    (LPWAVEOUT pWaveOut, LPMIXERCONFIG pMixerConfig, LPADVMIXCONFIG pAdvMixConfig, BOOL fRecommend);

    STDMETHOD(Init)
    (HINSTANCE hInst, LPWAVEOUT pWaveOut, LPMIXERCONFIG pMixerConfig, LPADVMIXCONFIG pAdvMixConfig);

    STDMETHOD(Uninit)();

    STDMETHOD(Activate)(BOOL fActive);

    STDMETHOD(Suspend)(BOOL fSuspend);

    STDMETHOD(SetConfig)(LPMIXERCONFIG pMixerConfig, LPADVMIXCONFIG pAdvMixConfig);

    STDMETHOD(GetConfig)(LPMIXERCONFIG pMixerConfig, LPADVMIXCONFIG pAdvMixConfig);

    STDMETHOD(SetMixerVolume)(DWORD dwVolume);

    STDMETHOD(GetMixerVolume)(LPDWORD lpdwVolume);

    STDMETHOD(PlaySound)(LPSOUND pSound);

    STDMETHOD_(BOOL, RemixMode)(BOOL fActive);

    STDMETHOD_(DWORD, GetAvgSample)();

    STDMETHOD(AllocGroup)(LPDWORD lpdwGroup);

    STDMETHOD(FreeGroup)(DWORD dwGroup);

    STDMETHOD(EnlistGroup)(IUnknown FAR *pChannel, DWORD dwGroup);

    STDMETHOD(DefectGroup)(LPUNKNOWN pUnknown, DWORD dwGroup);

    STDMETHOD(StartGroup)(DWORD dwGroup, BOOL fStart);

    STDMETHOD(ResetGroup)(DWORD dwGroup);

    STDMETHOD(SetGroupVolume)(DWORD dwGroup, DWORD dwVolume);

    STDMETHOD(SetGroupGain)(DWORD dwGroup, float flDBLeft, float flDBRight, BOOL fAbsolute);

    STDMETHOD(SetGroupPosition)(DWORD dwGroup, DWORD dwPosition);

    STDMETHOD(AllocChannel)(LPCHANNEL FAR *ppChannel);

    STDMETHOD(RegisterChannel)(LPUNKNOWN pUnknown);

    STDMETHOD(UnregisterChannel)(LPUNKNOWN pUnknown);

    STDMETHOD(SetPriority)(LPUNKNOWN pUnknown, DWORD dwPriority);

    STDMETHOD(GetPriority)(LPUNKNOWN pUnknown, LPDWORD lpdwPriority);

    STDMETHOD_(void, Refresh)();

    CAMMixer();
    ~CAMMixer();

  private:
    // Acquire mixer lock
    void EnterMixer();

    // Release mixer lock
    void LeaveMixer();

    // Acquire pump lock
    void EnterPump();

    // Release pump lock
    void LeavePump();

    // Return name of config file
    const char *GetInitFile()
    {
        return "audioman.ini";
    }

    // Load logging options from config file
    STDMETHODIMP DebugSetup();

    // Load mixer settings from config file
    STDMETHODIMP LoadProfile();

    // Set default fields in the MIXERSETTINGS structure
    STDMETHODIMP_(void) FinishMixerSettings(MIXERSETTINGS *pms);

    // Configure this mixer with the given MIXERSETTINGS
    STDMETHODIMP_(BOOL) SetMixerSettings(MIXERSETTINGS *pms);

    STDMETHODIMP_(void) StartMixer();
    STDMETHODIMP_(void) StopMixer();

    STDMETHODIMP ActivateMixer();
    STDMETHODIMP DeactivateMixer();

    STDMETHODIMP AllocMixerData();
    STDMETHODIMP FreeMixerData();

    STDMETHODIMP PrepareMixerBuffers();
    STDMETHODIMP UnprepareMixerBuffers();

    // Check if any devices support async playback
    STDMETHODIMP AudioDeviceCheck();

    STDMETHODIMP OpenMixerDevice(DWORD uDevice);

    STDMETHODIMP_(MIXBUFFER *) GetNextFreeBuffer();

    // Mix each channel's samples into a buffer
    STDMETHODIMP_(BOOL) FillMixBuffer(MIXBUFFER *pmb);

    // Set mixer buffer's flags to indicate that it is in use
    STDMETHODIMP_(void) AllocMixerBuffer(MIXBUFFER *pmb);

    STDMETHODIMP AllocOutputDevice(LPWAVEOUT pWaveOut);
    STDMETHODIMP FreeOutputDevice();

    STDMETHODIMP AllocMixerTimer();
    STDMETHODIMP FreeMixerTimer();

    // Called periodically to mix the next buffers
    STDMETHODIMP_(void) Pump();

    STDMETHODIMP_(BOOL) MixNextBuffer();

    STDMETHODIMP_(BOOL) GetMixerSettings(MIXERSETTINGS *pms);

    STDMETHODIMP GetMixerFormat(WAVEFORMATEX *lpPCMFormat);
    STDMETHODIMP SetMixerFormat(WAVEFORMATEX *lpPCMFormat);

    STDMETHODIMP GetAdvanced(AdvMixerConfig *lpAdvConfig);
    STDMETHODIMP SetAdvanced(AdvMixerConfig *lpAdvConfig);

    // Timer callback
    static void __stdcall MixerTimerFunction(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

    STDMETHODIMP_(WORD) CountFreeMixerHeaders();

    // Call notification callbacks on all channels
    STDMETHODIMP_(void) MakeNotifications(MIXHEADER *pmh);

    // Get the IAMMixerChannel interface from an object
    STDMETHODIMP QueryMCInterface(IAMMixerChannel **ppMixerChannel, IUnknown *pUnknown);

    // Return index into buffer arrays for next remix buffer
    STDMETHODIMP_(DWORD) CalcRemixBuffers();
    STDMETHODIMP_(void) RemixBuffers(DWORD dwBufferNumber);

    // Class members
    CAMMutex *m_MixerLock = NULL;
    CAMMutex *m_PumpLock = NULL;
    ULONG m_RefCnt = 0;

    IAMMixerChannel *m_FirstMixerChannel = NULL;
    BOOL m_IsInitialized = FALSE;
    CMixerOut *m_CMixerOut = NULL;
    DWORD m_NumDevices = 0;
    MIXERSETTINGS m_MixerSettings;
    MIXERSETTINGS m_ActiveMixerSettings;
    BOOL m_MixerConfigured = FALSE;
    BOOL m_AllocatedMixerData = FALSE;
    BOOL m_MixerStarted = FALSE;
    BOOL m_PrevMixerStarted = FALSE;
    BOOL m_MixerActive = FALSE;
    BOOL m_IsRemixing = FALSE;
    MIXBUFFER *m_MixBuffers = NULL;
    BYTE *m_AuxBuffer = NULL;
    WORD m_MixBufferCount = 0;
    SHORT m_NextMixBuffer = -1;
    WORD m_MixBufferSizeMsec = 0;
    DWORD m_MixBufferSizeBytes = 0;
    DWORD m_MixBufferSizeSamples = 0;
    WORD m_Voices = 0x20;
    DWORD m_DeviceID = -1;
    BYTE m_Padding = 0;
    DWORD m_StartTime = 0;
    BOOL m_Suspended = FALSE;
    DWORD m_GroupAllocBitmask = 0;
    MMRESULT m_TimerID = 0;
};