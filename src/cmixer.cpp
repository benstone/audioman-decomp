#include "cmixer.h"
#include <cassert>
#include "dpf.h"
#include "utils.h"
#include "cplaysnd.h"
#include "todo.h"

extern LPMIXER gpMixer;

// Default options
const WORD kDefaultChannels = 1;
const WORD kDefaultSample = 16;
const DWORD kDefaultRate = 22050;
const DWORD kDefaultBufferTimeMs = 600;
const DWORD kDefaultVoices = 32;
const BOOL kDefaultRemix = TRUE;
const DWORD kDefaultLogLevel = 0;

// Limits
const DWORD kMinBufferTime = 3;
const DWORD kMaxBufferTime = 15;
const DWORD kMaxVoices = 16;

STDMETHODIMP_(ULONG) CAMMixer::AddRef()
{
    return ++m_RefCnt;
}

STDMETHODIMP_(ULONG) CAMMixer::Release()
{
    ULONG RefCnt = --m_RefCnt;

    if (RefCnt == 0)
    {
        DPRINTF(1, "Deleting the Mixer");
        if (m_IsInitialized != FALSE)
        {
            Uninit();
        }

        gpMixer = NULL;
        if (this != NULL)
        {
            delete this;
        }
    }

    return RefCnt;
}

STDMETHODIMP CAMMixer::QueryInterface(REFIID riid, void **ppvObject)
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

STDMETHODIMP
CAMMixer::TestConfig(LPWAVEOUT pWaveOut, LPMIXERCONFIG pMixerConfig, LPADVMIXCONFIG pAdvMixConfig, BOOL fRecommend)
{
    TODO_NOT_IMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP
CAMMixer::Init(HINSTANCE hInst, LPWAVEOUT pWaveOut, LPMIXERCONFIG pMixerConfig, LPADVMIXCONFIG pAdvMixConfig)
{
    HRESULT hr;

    if (m_MixerLock == NULL)
    {
        hr = E_INITFAILED;
    }
    else
    {
        EnterMixer();
#ifdef _DEBUG
        DebugSetup();
#endif // _DEBUG
        DPRINTF(1, "InitMixer");

        if (m_IsInitialized == FALSE && hInst != NULL)
        {
            hr = LoadProfile();
            if (SUCCEEDED(hr))
            {
                if (pMixerConfig != NULL || pAdvMixConfig != NULL)
                {
                    hr = SetConfig(pMixerConfig, pAdvMixConfig);
                }

                if (SUCCEEDED(hr))
                {
                    hr = AllocMixerData();
                }

                if (SUCCEEDED(hr))
                {
                    hr = AllocOutputDevice(pWaveOut);
                }

                if (SUCCEEDED(hr))
                {
                    hr = AllocMixerTimer();
                }

                if (SUCCEEDED(hr))
                {
                    m_IsInitialized = TRUE;
                }
            }
        }
        else
        {
            hr = 0x80040008;
        }
        LeaveMixer();
    }

    return hr;
}

STDMETHODIMP CAMMixer::Uninit()
{
    HRESULT hr;

    hr = S_OK;
    if (this->m_IsInitialized == FALSE)
    {
        hr = E_NOTINITED;
    }
    else
    {
        FreeMixerTimer();

        EnterMixer();
        this->m_IsInitialized = 0;
        DeactivateMixer();

        FreeOutputDevice();
        FreeMixerData();
        LeaveMixer();
    }

    return hr;
}

STDMETHODIMP CAMMixer::Activate(BOOL fActive)
{
    HRESULT hr;

    EnterMixer();

    if (fActive)
    {
        hr = ActivateMixer();
    }
    else
    {
        hr = DeactivateMixer();
    }

    LeaveMixer();

    return hr;
}

STDMETHODIMP CAMMixer::Suspend(BOOL fSuspend)
{
    HRESULT hr;

    EnterMixer();

    if (m_CMixerOut == NULL)
    {
        hr = E_NOTINITED;
    }
    else if (m_MixerActive == FALSE)
    {
        hr = E_NOTACTIVE;
    }
    else
    {
        DeactivateMixer();
        m_CMixerOut->Suspend(fSuspend);
        hr = ActivateMixer();

        if (FAILED(hr))
        {
            m_CMixerOut->Suspend(fSuspend == 0);
            ActivateMixer();
        }
        else
        {
            m_Suspended = fSuspend;
        }
    }

    LeaveMixer();
    return hr;
}

STDMETHODIMP CAMMixer::SetConfig(LPMIXERCONFIG pMixerConfig, LPADVMIXCONFIG pAdvMixConfig)
{
    HRESULT hr = S_OK;
    WAVEFORMATEX wfx;

    EnterMixer();

    if (pMixerConfig == NULL || pMixerConfig->dwSize == sizeof(MIXERCONFIG))
    {
        if (pAdvMixConfig != NULL && pAdvMixConfig->dwSize != sizeof(ADVMIXCONFIG))
        {
            DPRINTF(0, "Invalid pAdvMixConfig");
            hr = E_INVALIDARG;
        }
    }
    else
    {
        DPRINTF(0, "Invalid pMixerConfig");
        hr = E_INVALIDARG;
    }

    if (hr == S_OK)
    {
        if (pMixerConfig != NULL)
        {
            if (pMixerConfig->lpFormat == NULL)
            {
                if (ConvertFormatToWaveFormatEx(&wfx, pMixerConfig->dwFormat) == FALSE)
                {
                    hr = E_BADPCMFORMAT;
                }
                else
                {
                    hr = SetMixerFormat(&wfx);
                }
            }
            else
            {
                hr = SetMixerFormat(pMixerConfig->lpFormat);
            }
        }

        if (hr == S_OK && pAdvMixConfig != NULL)
        {
            hr = SetAdvanced(pAdvMixConfig);
        }
    }

    LeaveMixer();
    return hr;
}

STDMETHODIMP CAMMixer::GetConfig(LPMIXERCONFIG pMixerConfig, LPADVMIXCONFIG pAdvMixConfig)
{
    HRESULT hr = S_OK;
    WAVEFORMATEX wfx;

    EnterMixer();

    if (pMixerConfig == NULL || pMixerConfig->dwSize == sizeof(MIXERCONFIG))
    {
        if (pAdvMixConfig != NULL && pAdvMixConfig->dwSize != sizeof(ADVMIXCONFIG))
        {
            DPRINTF(0, "Invalid pAdvMixConfig");
            hr = E_INVALIDARG;
        }
    }
    else
    {
        DPRINTF(0, "Invalid pMixerConfig");
        hr = E_INVALIDARG;
    }

    if (hr == S_OK)
    {
        if (pMixerConfig != NULL)
        {
            if (pMixerConfig->lpFormat == NULL)
            {
                hr = GetMixerFormat(&wfx);
                if (hr == S_OK)
                {
                    pMixerConfig->dwFormat = ConvertWaveFormatExToFormat(&wfx);
                }
            }
            else
            {
                hr = GetMixerFormat(pMixerConfig->lpFormat);
            }
        }

        if (hr == S_OK && pAdvMixConfig != NULL)
        {
            hr = GetAdvanced(pAdvMixConfig);
        }
    }

    LeaveMixer();
    return hr;
}

STDMETHODIMP CAMMixer::SetMixerVolume(DWORD dwVolume)
{
    HRESULT hr;

    EnterMixer();

    if (m_CMixerOut == NULL)
    {
        hr = E_NOTINITED;
    }
    else
    {
        hr = HRESULTFromMMRESULT(m_CMixerOut->SetVolume(dwVolume));
    }

    LeaveMixer();
    return hr;
}

STDMETHODIMP CAMMixer::GetMixerVolume(LPDWORD lpdwVolume)
{
    HRESULT hr;

    EnterMixer();

    if (m_CMixerOut == NULL)
    {
        hr = E_NOTINITED;
    }
    else
    {
        hr = HRESULTFromMMRESULT(m_CMixerOut->GetVolume(lpdwVolume));
    }

    LeaveMixer();
    return hr;
}

STDMETHODIMP CAMMixer::PlaySound(LPSOUND pSound)
{
    HRESULT hr;

    EnterMixer();

    if (pSound == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CAMPlaySnd *pCPlaySnd = new CAMPlaySnd();
        if (pCPlaySnd == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = pCPlaySnd->Play(this, pSound);
            if (FAILED(hr))
            {
                if (pCPlaySnd != NULL)
                {
                    delete pCPlaySnd;
                }
            }
        }
    }

    LeaveMixer();
    return hr;
}

STDMETHODIMP_(BOOL __stdcall) CAMMixer::RemixMode(BOOL fActive)
{
    return 0;
}

STDMETHODIMP_(DWORD __stdcall) CAMMixer::GetAvgSample()
{
    return 0;
}

STDMETHODIMP CAMMixer::AllocGroup(LPDWORD lpdwGroup)
{
    HRESULT hr = S_OK;
    BOOL fGotOne = FALSE;
    DWORD dwGroup;

    EnterMixer();

    if (lpdwGroup == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *lpdwGroup = 1;
        dwGroup = 1;

        while (dwGroup != 0)
        {
            if ((dwGroup & m_GroupAllocBitmask) == 0)
            {
                m_GroupAllocBitmask = m_GroupAllocBitmask | dwGroup;
                fGotOne = TRUE;
                break;
            }
            dwGroup = dwGroup << 1;
            *lpdwGroup = *lpdwGroup + 1;
        }

        if (!fGotOne)
        {
            *lpdwGroup = 0;
            hr = E_ALLGROUPSALLOCATED;
        }
    }

    LeaveMixer();
    return hr;
}

STDMETHODIMP CAMMixer::FreeGroup(DWORD dwGroup)
{
    HRESULT hr = E_INVALIDARG;
    DWORD dwfGroup = 1 << (dwGroup - 1 & 0x1F);

    EnterMixer();

    if (dwfGroup != 0)
    {
        if ((dwfGroup & m_GroupAllocBitmask) == 0)
        {
            hr = S_OK;
        }
        else
        {
            DefectGroup(NULL, dwGroup);
            m_GroupAllocBitmask = m_GroupAllocBitmask & ~dwfGroup;
            hr = S_OK;
        }
    }

    LeaveMixer();
    return hr;
}

STDMETHODIMP CAMMixer::EnlistGroup(IUnknown *pChannel, DWORD dwGroup)
{
    HRESULT hr = S_OK;
    IAMMixerChannel *pMixerChannel;
    DWORD dwfGroup = 1 << (dwGroup - 1 & 0x1F);

    EnterMixer();

    hr = QueryMCInterface(&pMixerChannel, pChannel);
    if (SUCCEEDED(hr))
    {
        if ((dwfGroup & m_GroupAllocBitmask) == 0)
        {
            hr = E_GROUPNOTALLOCATED;
        }
        else
        {
            DWORD dwGroup = pMixerChannel->GetGroup();
            pMixerChannel->SetGroup(dwGroup | dwfGroup);
        }

        pMixerChannel->Release();
        LeaveMixer();
    }

    return hr;
}

STDMETHODIMP CAMMixer::DefectGroup(LPUNKNOWN pUnknown, DWORD dwGroup)
{

    HRESULT hr = S_OK;
    IAMMixerChannel *pMixerChannel = NULL;
    DWORD dwfGroup = 1 << (dwGroup - 1 & 0x1F);

    EnterMixer();
    if (pUnknown != NULL)
    {
        hr = QueryMCInterface(&pMixerChannel, pUnknown);
    }

    if (SUCCEEDED(hr))
    {
        if ((dwfGroup & m_GroupAllocBitmask) == 0)
        {
            hr = E_GROUPNOTALLOCATED;
        }
        else if (pMixerChannel == NULL)
        {
            for (IAMMixerChannel *pmc = m_FirstMixerChannel; pmc != NULL; pmc = pmc->GetNext())
            {
                pmc->SetGroup(pmc->GetGroup() & ~dwfGroup);
            }
        }
        else
        {
            dwGroup = pMixerChannel->GetGroup();
            pMixerChannel->SetGroup(dwGroup & ~dwfGroup);
            pMixerChannel->Release();
        }
    }

    LeaveMixer();
    return hr;
}

STDMETHODIMP CAMMixer::StartGroup(DWORD dwGroup, BOOL fStart)
{
    HRESULT hr = S_OK;
    BOOL fNeedRefresh = FALSE;
    DWORD dwfGroup = 1 << (dwGroup - 1 & 0x1F);

    EnterMixer();

    if ((dwfGroup & m_GroupAllocBitmask) == 0)
    {
        hr = E_GROUPNOTALLOCATED;
    }
    else
    {
        for (IAMMixerChannel *pmc = m_FirstMixerChannel; pmc != NULL; pmc = pmc->GetNext())
        {
            if ((pmc->GetGroup() & dwfGroup) != 0)
            {
                pmc->DoStart(fStart);
                fNeedRefresh = TRUE;
            }
        }

        if (fNeedRefresh)
        {
            Refresh();
        }
    }

    LeaveMixer();
    return hr;
}

STDMETHODIMP CAMMixer::ResetGroup(DWORD dwGroup)
{
    TODO_NOT_IMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CAMMixer::SetGroupVolume(DWORD dwGroup, DWORD dwVolume)
{
    TODO_NOT_IMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CAMMixer::SetGroupGain(DWORD dwGroup, float flDBLeft, float flDBRight, BOOL fAbsolute)
{
    TODO_NOT_IMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CAMMixer::SetGroupPosition(DWORD dwGroup, DWORD dwPosition)
{
    TODO_NOT_IMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CAMMixer::AllocChannel(LPCHANNEL *ppChannel)
{
    HRESULT hr = S_OK;

    EnterMixer();

    if (ppChannel == NULL)
    {
        DPRINTF(0, "CAMMixer::AllocChannel invalid argument!");
        hr = E_INVALIDARG;
    }
    else
    {
        CAMChannel *pChannel = new CAMChannel();
        if (pChannel == NULL)
        {
            DPRINTF(0, "CAMMixer::AllocChannel:  Could not allocate");
        }
        else
        {
            hr = pChannel->Init(this);
            if (FAILED(hr))
            {
                DPRINTF(0, "CAMMixer::AllocChannel:  Init failed on pChannel");
                if (pChannel != NULL)
                {
                    delete pChannel;
                }
            }
            else
            {
                pChannel->AddRef();
                *ppChannel = (LPCHANNEL)pChannel;
            }
        }
    }

    LeaveMixer();
    return hr;
}

STDMETHODIMP CAMMixer::RegisterChannel(LPUNKNOWN pUnknown)
{
    HRESULT hr = S_OK;
    IAMMixerChannel *pMC;

    EnterMixer();

    hr = QueryMCInterface(&pMC, pUnknown);
    if (SUCCEEDED(hr))
    {
        if (pMC->GetNext() != 0)
        {
            assert(FALSE);
        }

        if (m_FirstMixerChannel == NULL)
        {
            m_FirstMixerChannel = pMC;
            pMC->SetMutx(m_MixerLock);
            pMC->Configure(m_MixBufferCount, m_MixBufferSizeSamples, &m_ActiveMixerSettings.Format);
        }
        else
        {
            hr = E_ALREADYREGISTERED;

            for (IAMMixerChannel *pMixerChannel = m_FirstMixerChannel; pMC != pMixerChannel;
                 pMixerChannel = pMixerChannel->GetNext())
            {
                if (pMixerChannel->GetNext() == NULL)
                {
                    pMC->SetMutx(m_MixerLock);
                    pMC->Configure(m_MixBufferCount, m_MixBufferSizeSamples, &m_ActiveMixerSettings.Format);
                    pMixerChannel->SetNext(pMC);

                    hr = S_OK;
                    break;
                }
            }
        }
    }

    LeaveMixer();
    return hr;
}

STDMETHODIMP CAMMixer::UnregisterChannel(LPUNKNOWN pUnknown)
{
    HRESULT hr = S_OK;
    IAMMixerChannel *pMixerChannel, *pMCNext;

    EnterMixer();

    hr = QueryMCInterface(&pMixerChannel, pUnknown);
    if (SUCCEEDED(hr))
    {
        if (m_FirstMixerChannel == NULL)
        {
            hr = E_CHANNELNOTREGISTERED;
        }
        else if (m_FirstMixerChannel == pMixerChannel)
        {
            m_FirstMixerChannel = pMixerChannel->GetNext();
            pMixerChannel->SetNext(NULL);
            pMixerChannel->SetMutx(NULL);
            pMixerChannel->Release();
        }
        else
        {
            pMCNext = m_FirstMixerChannel;

            while (pMCNext->GetNext() != 0)
            {
                if (pMCNext->GetNext() == pMixerChannel)
                {
                    pMCNext->SetNext(pMixerChannel->GetNext());
                    pMixerChannel->SetNext(NULL);
                    pMixerChannel->SetMutx(NULL);
                    pMixerChannel->Release();
                    break;
                }

                pMCNext = pMCNext->GetNext();
            }
        }

        pMixerChannel->Release();
    }

    LeaveMixer();
    return hr;
}

STDMETHODIMP CAMMixer::SetPriority(LPUNKNOWN pUnknown, DWORD dwPriority)
{
    TODO_NOT_IMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CAMMixer::GetPriority(LPUNKNOWN pUnknown, LPDWORD lpdwPriority)
{
    TODO_NOT_IMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP_(void __stdcall) CAMMixer::Refresh()
{
    HRESULT hr = S_OK;
    BOOL fActiveChannels = FALSE;

    EnterMixer();

    IAMMixerChannel *pmc = m_FirstMixerChannel;

    // Check if there are any active channels
    while (pmc != NULL)
    {
        if (pmc->GetActiveState() != 0)
        {
            fActiveChannels = TRUE;
            break;
        }
        pmc = pmc->GetNext();
    }

    if (m_MixerStarted == FALSE)
    {
        if (fActiveChannels)
        {
            StartMixer();
        }
    }
    else if (m_ActiveMixerSettings.RemixEnabled)
    {
        if (fActiveChannels)
        {
            m_IsRemixing = TRUE;

            assert(m_NextMixBuffer != m_MixBufferCount);

            DWORD dwBufferNumber = CalcRemixBuffers();

            fActiveChannels = FALSE;
            for (pmc = m_FirstMixerChannel; pmc != NULL; pmc = pmc->GetNext())
            {
                if (pmc->GetActiveState() != 0)
                {
                    pmc->RevertTo(dwBufferNumber);
                    fActiveChannels = TRUE;
                }
            }

            if (fActiveChannels)
            {
                RemixBuffers(dwBufferNumber);
            }
            else
            {
                for (DWORD dwBuffer = 0; dwBuffer < m_MixBufferCount; dwBuffer++)
                {
                    // TODO: This loop seems to do nothing?
                }
            }

            m_IsRemixing = FALSE;
        }
        else
        {
            StopMixer();
        }
    }

    LeaveMixer();
}

CAMMixer::CAMMixer()
{
    m_MixerLock = new CAMMutex();
    if (m_MixerLock != NULL)
    {
        m_PumpLock = new CAMMutex();

        if (m_PumpLock == NULL)
        {
            delete m_MixerLock;
            m_MixerLock = NULL;
        }
    }
}

CAMMixer::~CAMMixer()
{
    if (m_PumpLock != NULL)
    {
        CAMMutex *pMutx = m_PumpLock;
        m_PumpLock = NULL;

        delete pMutx;
    }

    if (m_MixerLock != NULL)
    {
        // not sure why this sets m_PumpLock = NULL - it should already be NULL
        m_PumpLock = NULL;
        if (m_MixerLock != NULL)
        {
            delete m_MixerLock;
        }
    }
}

void CAMMixer::EnterMixer()
{
    if (m_MixerLock != NULL)
        m_MixerLock->Enter();
}

void CAMMixer::LeaveMixer()
{
    if (m_MixerLock != NULL)
        m_MixerLock->Leave();
}

void CAMMixer::EnterPump()
{
    if (m_PumpLock != NULL)
        m_PumpLock->Enter();
}

void CAMMixer::LeavePump()
{
    if (m_PumpLock != NULL)
        m_PumpLock->Leave();
}

STDMETHODIMP CAMMixer::DebugSetup()
{
    char rgDbgLog[64];
    char *szInitFile = GetInitFile();

    // Get log level
    DWORD uDbgLevel = GetPrivateProfileIntA("debug", "level", kDefaultLogLevel, szInitFile);

    // Get log file
    GetPrivateProfileStringA("debug", "logfile", "", rgDbgLog, sizeof(rgDbgLog), szInitFile);

    // Configure logging
    DbgInitialize(TRUE);
    DbgEnable(TRUE);
    DbgLogFile(rgDbgLog, TRUE);
    DbgSetLevel(uDbgLevel);
    DbgOutActive(TRUE);

    return S_OK;
}

STDMETHODIMP CAMMixer::LoadProfile()
{
    HRESULT hr = S_OK;
    MIXERSETTINGS ms;

    // Get options from config file
    char *szInitFile = GetInitFile();

    ms.Format.nChannels = GetPrivateProfileIntA("settings", "channels", kDefaultChannels, szInitFile);
    ms.Format.wBitsPerSample = GetPrivateProfileIntA("settings", "sample", kDefaultSample, szInitFile);
    ms.Format.nSamplesPerSec = GetPrivateProfileIntA("settings", "rate", kDefaultRate, szInitFile);
    ms.BufferTime = GetPrivateProfileIntA("settings", "buffertime", kDefaultBufferTimeMs, szInitFile);
    ms.Voices = GetPrivateProfileIntA("settings", "voices", kDefaultVoices, szInitFile);
    ms.RemixEnabled = GetPrivateProfileIntA("settings", "remix", kDefaultRemix, szInitFile);

    FinishMixerSettings(&ms);

    if (SetMixerSettings(&ms) == FALSE)
    {
        assert(FALSE);
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP_(void __stdcall) CAMMixer::FinishMixerSettings(MIXERSETTINGS *pms)
{
    if (pms != NULL)
    {
        pms->Format.wFormatTag = WAVE_FORMAT_PCM;
        pms->Format.nBlockAlign = pms->Format.nChannels << ((pms->Format.wBitsPerSample >> 4) & 0x1f);
        pms->Format.nAvgBytesPerSec = pms->Format.nBlockAlign * pms->Format.nSamplesPerSec;
    }
}

STDMETHODIMP_(BOOL __stdcall) CAMMixer::SetMixerSettings(MIXERSETTINGS *pms)
{
    IAMMixerChannel *pmc;

    if (pms != NULL)
    {
        BOOL fReactivate = m_MixerActive != FALSE;
        if (fReactivate)
        {
            StopMixer();
            DeactivateMixer();
        }

        BOOL fReInit = m_AllocatedMixerData != FALSE;
        if (fReInit)
        {
            FreeMixerData();
        }

        // Copy mixer settings and wave format
        m_MixerSettings = *pms;
        m_ActiveMixerSettings = m_MixerSettings;

        // Set buffer time
        m_MixBufferCount = pms->BufferTime / 60;
        if (m_MixBufferCount < kMinBufferTime)
        {
            m_MixBufferCount = kMinBufferTime;
        }
        if (m_MixBufferCount > kMaxBufferTime)
        {
            m_MixBufferCount = kMaxBufferTime;
        }

        // Set voices
        m_Voices = pms->Voices;
        if (m_Voices > kMaxVoices)
        {
            m_Voices = kMaxVoices;
        }

        // Set padding
        if (m_ActiveMixerSettings.Format.wBitsPerSample == 8)
        {
            m_Padding = 0x80;
        }
        else
        {
            m_Padding = 0;
        }

        if (fReInit)
        {
            AllocMixerData();
        }

        // Configure all channels
        for (pmc = m_FirstMixerChannel; pmc != NULL; pmc = pmc->GetNext())
        {
            pmc->Configure(m_MixBufferCount, m_MixBufferSizeSamples, &m_ActiveMixerSettings.Format);
        }

        m_MixerConfigured = TRUE;

        DPRINTF(2, "Mixer Format %u %lu %lu %u %u", m_MixerSettings.Format.nChannels,
                m_MixerSettings.Format.nSamplesPerSec, m_MixerSettings.Format.nAvgBytesPerSec,
                m_MixerSettings.Format.nBlockAlign, m_MixerSettings.Format.wBitsPerSample);

        if (fReactivate)
        {
            ActivateMixer();
            StartMixer();
        }
    }
    return TRUE;
}

STDMETHODIMP_(void __stdcall) CAMMixer::StartMixer()
{
    DWORD dwBuffersFilled, dwErr;
    MIXBUFFER *pmb;

    dwBuffersFilled = 0;
    if (m_MixerStarted == FALSE)
    {
        m_MixerStarted = TRUE;

        // If the mixer has never started, reset the output
        BOOL fColdStart = (m_NextMixBuffer == -1);
        if (fColdStart)
        {
            m_CMixerOut->Reset();
            m_CMixerOut->Pause();
        }

        pmb = GetNextFreeBuffer();
        assert(pmb != NULL);

        while (pmb != NULL)
        {
            if (FillMixBuffer(pmb) == FALSE)
            {
                pmb = NULL;
                m_MixerStarted = FALSE;
            }
            else
            {
                AllocMixerBuffer(pmb);

                MMRESULT mmr = m_CMixerOut->Write(&pmb->WaveHeader, sizeof(pmb->WaveHeader));
                if (mmr != MMSYSERR_NOERROR)
                {
                    DPRINTF(0, "waveOutWrite Failed in StartMixer %lu", mmr);
                    pmb->InUse = FALSE;
                }

                dwBuffersFilled++;
                pmb = GetNextFreeBuffer();
            }
        }

        if (dwBuffersFilled == 0)
        {
            m_CMixerOut->Restart();
            m_NextMixBuffer = -1;
        }
        else if (fColdStart)
        {
            m_CMixerOut->Restart();
            m_NextMixBuffer = 0;
            m_StartTime = timeGetTime();
        }
    }
}

STDMETHODIMP_(void __stdcall) CAMMixer::StopMixer()
{
    DWORD dwIndex;
    MIXBUFFER *pmb;

    if (m_MixerStarted != FALSE)
    {
        m_MixerStarted = FALSE;
        m_NextMixBuffer = -1;
        m_CMixerOut->Reset();

        pmb = m_MixBuffers;
        for (dwIndex = 0; dwIndex < m_MixBufferCount; dwIndex++)
        {
            pmb->WaveHeader.dwFlags |= WHDR_DONE;
            pmb->InUse = FALSE;
            pmb++;
        }
    }
}

STDMETHODIMP CAMMixer::ActivateMixer()
{
    HRESULT hr = S_OK;

    if (m_IsInitialized == FALSE)
    {
        hr = E_NOTINITED;
    }
    else if (m_MixerActive == FALSE)
    {
        hr = AudioDeviceCheck();

        if (SUCCEEDED(hr))
        {
            m_ActiveMixerSettings = m_MixerSettings;

            hr = OpenMixerDevice(m_DeviceID);
            if (SUCCEEDED(hr))
            {
                hr = PrepareMixerBuffers();
            }

            if (SUCCEEDED(hr))
            {
                if (m_PrevMixerStarted == FALSE || m_NextMixBuffer == -1)
                {
                    m_MixerStarted = m_PrevMixerStarted;
                    m_NextMixBuffer = -1;
                }
                else
                {
                    m_PrevMixerStarted = FALSE;
                    m_CMixerOut->Pause();

                    MIXBUFFER *pmb = &m_MixBuffers[m_NextMixBuffer];
                    DWORD dwIndex;

                    for (dwIndex = m_NextMixBuffer; dwIndex < m_MixBufferCount; dwIndex++)
                    {
                        if (pmb->InUse == FALSE)
                        {
                            pmb->WaveHeader.dwFlags |= WHDR_DONE;
                        }
                        else
                        {
                            pmb->WaveHeader.dwFlags &= ~WHDR_DONE;
                            DWORD mmr = m_CMixerOut->Write(&pmb->WaveHeader, sizeof(pmb->WaveHeader));
                            if (mmr != MMSYSERR_NOERROR)
                            {
                                DPRINTF(0, "waveOutWrite Failed in Activate %lu", mmr);
                                pmb->InUse = FALSE;
                            }
                        }

                        pmb++;
                    }

                    pmb = m_MixBuffers;
                    for (dwIndex = m_NextMixBuffer; dwIndex < m_NextMixBuffer; dwIndex++)
                    {
                        if (pmb->InUse == FALSE)
                        {
                            pmb->WaveHeader.dwFlags |= WHDR_DONE;
                        }
                        else
                        {
                            pmb->WaveHeader.dwFlags &= ~WHDR_DONE;
                            DWORD mmr = m_CMixerOut->Write(&pmb->WaveHeader, sizeof(pmb->WaveHeader));
                            if (mmr != MMSYSERR_NOERROR)
                            {
                                DPRINTF(0, "waveOutWrite Failed in Activate %lu", mmr);
                                pmb->InUse = FALSE;
                            }
                        }

                        pmb++;
                    }

                    m_CMixerOut->Restart();
                    m_MixerStarted = TRUE;
                }
                m_MixerActive = TRUE;
                DPRINTF(2, "MIXER IS ACTIVE");
            }
        }

        if (FAILED(hr))
        {
            DPRINTF(0, "Mixer didn't activate!!!");
            m_CMixerOut->Close();
        }
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP CAMMixer::DeactivateMixer()
{
    HRESULT hr = S_OK;

    if (m_MixerActive == FALSE)
    {
        hr = E_NOTACTIVE;
    }
    else
    {
        m_CMixerOut->Reset();
        UnprepareMixerBuffers();

        MMRESULT mr = m_CMixerOut->Close();
        if (mr != 0)
        {
            DPRINTF(0, "Couldn't Close the Device %lu", mr);
            hr = HRESULTFromMMRESULT(mr);
        }

        m_MixerActive = FALSE;
        m_PrevMixerStarted = m_MixerStarted;
        m_MixerStarted = FALSE;
        DPRINTF(2, "MIXER IS NOT ACTIVE");
    }

    return hr;
}

STDMETHODIMP CAMMixer::AllocMixerData()
{
    HRESULT hr = S_OK;
    MIXBUFFER *pmb;
    DWORD dwIndex;

    if (m_AllocatedMixerData == FALSE)
    {
        assert(m_MixerConfigured);

        // Calculate buffer size
        m_MixBufferSizeBytes = ((m_ActiveMixerSettings.Format.nAvgBytesPerSec * 60) / 1000) & 0xFFFFFFFC;
        m_MixBufferSizeMsec = BytesToMillisec(&m_ActiveMixerSettings.Format, m_MixBufferSizeBytes);
        m_MixBufferSizeSamples = (BytesToSamples(&m_ActiveMixerSettings.Format, m_MixBufferSizeBytes) & 0xFFFFFFFC);

        m_MixBufferSizeMsec = SamplesToMillisec(&m_ActiveMixerSettings.Format, m_MixBufferSizeSamples);
        m_MixBufferSizeBytes = SamplesToBytes(&m_ActiveMixerSettings.Format, m_MixBufferSizeSamples);

        // Allocate aux buffer
        m_AuxBuffer = new BYTE[m_MixBufferSizeBytes];
        if (m_AuxBuffer == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            // Allocate mix buffers
            m_MixBuffers = new MIXBUFFER[m_MixBufferCount];
            if (m_MixBuffers == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                pmb = m_MixBuffers;

                // Initialize each mix buffer
                for (dwIndex = 0; dwIndex < m_MixBufferCount; dwIndex++)
                {
                    pmb->InUse = FALSE;
                    pmb->WaveHeader.dwBufferLength = m_MixBufferSizeBytes;
                    pmb->WaveHeader.dwFlags |= WHDR_DONE;
                    pmb->WaveHeader.lpData = new CHAR[m_MixBufferSizeBytes];
                    if (pmb->WaveHeader.lpData == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                    pmb->MixHeader.lpData = (BYTE *)pmb->WaveHeader.lpData;
                    pmb->MixHeader.lpAux = m_AuxBuffer;
                    pmb->MixHeader.dwBuffer = dwIndex;
                    pmb->MixHeader.unknownA = 1 << (dwIndex & 0x1F);
                    pmb->MixHeader.fHaveMixed = FALSE;

                    pmb++;
                }

                if (SUCCEEDED(hr))
                {
                    m_AllocatedMixerData = TRUE;
                }
            }
        }

        if (FAILED(hr))
        {
            if (m_AuxBuffer != NULL)
            {
                delete[] m_AuxBuffer;
                m_AuxBuffer = NULL;
            }
            m_AllocatedMixerData = TRUE;
            FreeMixerData();
            DPRINTF(0, "Didn't Get Mixer Data");
        }
    }
    else
    {
        // already allocated
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP CAMMixer::FreeMixerData()
{
    HRESULT hr = S_OK;
    DWORD dwIndex = 0;
    MIXBUFFER *pmb;

    if (m_AllocatedMixerData == FALSE)
    {
        hr = E_NOTINITED;
    }
    else
    {
        if (m_MixBuffers != NULL)
        {
            // Free all allocated buffers
            pmb = m_MixBuffers;
            for (dwIndex = 0; dwIndex < m_MixBufferCount; dwIndex++)
            {
                if (pmb->WaveHeader.lpData != NULL)
                {
                    delete[] pmb->WaveHeader.lpData;
                }
                pmb++;
            }

            // Free the array
            delete[] m_MixBuffers;
            m_MixBuffers = NULL;
        }

        // Free the aux buffer
        if (m_AuxBuffer != NULL)
        {
            delete[] m_AuxBuffer;
            m_AuxBuffer = NULL;
        }

        m_AllocatedMixerData = FALSE;
    }

    return hr;
}

STDMETHODIMP CAMMixer::PrepareMixerBuffers()
{
    HRESULT hr = S_OK;
    DWORD dwIndex;
    MMRESULT mr;

    if (m_AllocatedMixerData == FALSE)
    {
        hr = E_NOTINITED;
    }
    else
    {
        for (dwIndex = 0; dwIndex < m_MixBufferCount; dwIndex++)
        {
            m_MixBuffers[dwIndex].WaveHeader.dwBufferLength = m_MixBufferSizeBytes;
            m_MixBuffers[dwIndex].WaveHeader.dwFlags = 0;
            m_MixBuffers[dwIndex].WaveHeader.reserved = 0;
            mr = m_CMixerOut->PrepareHeader(&(m_MixBuffers[dwIndex].WaveHeader), sizeof(WAVEHDR));
            if (mr != 0)
            {
                hr = HRESULTFromMMRESULT(mr);
                while (dwIndex != 0)
                {
                    dwIndex--;
                    m_CMixerOut->UnprepareHeader(&(m_MixBuffers[dwIndex].WaveHeader), sizeof(WAVEHDR));
                }
                break;
            }
            m_MixBuffers[dwIndex].WaveHeader.dwFlags |= WHDR_DONE;
        }
    }

    if (FAILED(hr))
    {
        DPRINTF(0, "Couldn't Prepare Mixer Headers");
    }

    return hr;
}

STDMETHODIMP CAMMixer::UnprepareMixerBuffers()
{
    HRESULT hr = S_OK;
    DWORD dwIndex;

    if (m_AllocatedMixerData == FALSE)
    {
        hr = E_NOTINITED;
    }

    for (dwIndex = 0; dwIndex < m_MixBufferCount; dwIndex++)
    {
        m_CMixerOut->UnprepareHeader(&(m_MixBuffers[dwIndex].WaveHeader), 0x20);
    }

    return hr;
}

STDMETHODIMP CAMMixer::AudioDeviceCheck()
{
    HRESULT hr = S_OK;
    DWORD uAsyncCount = 0;
    DWORD uDevice, u;
    WAVEOUTCAPS caps;

    m_NumDevices = m_CMixerOut->GetNumDevs();
    if (m_NumDevices == 0)
    {
        DPRINTF(0, "NumDevices is ZERO");
        hr = E_NOSOUNDCARD;
    }
    else
    {
        for (uDevice = 0; uDevice < m_NumDevices; uDevice++)
        {
            u = m_CMixerOut->GetDevCaps(uDevice, &caps, sizeof(caps));
            if (u == 0 && (caps.dwSupport & WAVECAPS_SYNC) == 0)
            {
                uAsyncCount++;
            }
        }

        if (uAsyncCount == 0)
        {
            DPRINTF(0, "Only %u async devices found", 0);
            hr = E_INVALIDCARD;
        }
    }

    DPRINTF(3, "AudioDeviceCheck succeeded! %u devices % async", m_NumDevices, uAsyncCount);
    return hr;
}

STDMETHODIMP CAMMixer::OpenMixerDevice(DWORD uDevice)
{
    HRESULT hr = S_OK;
    MMRESULT mr;

    mr = m_CMixerOut->Open(uDevice, &m_MixerSettings.Format, 0, 0, 0);
    if (mr != MMSYSERR_NOERROR)
    {
        DPRINTF(0, "Didn't Get Mixer Device %lu", mr);
        hr = HRESULTFromMMRESULT(mr);
    }
    return hr;
}

STDMETHODIMP_(MIXBUFFER *__stdcall) CAMMixer::GetNextFreeBuffer()
{
    DWORD dwIndex;
    MIXBUFFER *pmb, *pFree;

    pmb = m_MixBuffers + (m_MixBufferCount - 1);
    pFree = NULL;

    for (dwIndex = 0; dwIndex < m_MixBufferCount; dwIndex++)
    {
        if (pmb->InUse == FALSE)
        {
            pFree = pmb;
        }
        else if (pFree != NULL)
        {
            break;
        }
        pmb--;
    }

    if (pFree == NULL && m_MixBuffers[0].InUse == FALSE)
    {
        pFree = &m_MixBuffers[0];
    }

    return pFree;
}

STDMETHODIMP_(BOOL __stdcall) CAMMixer::FillMixBuffer(MIXBUFFER *pmb)
{
    BOOL fHaveMixed = FALSE;
    DPRINTF(4, "MixBuffer in");

    memset(pmb->WaveHeader.lpData, m_Padding, m_MixBufferSizeBytes);

    pmb->MixHeader.MixedVoices = 0;
    pmb->MixHeader.fHaveMixed = FALSE;

    // Mix each channel's samples into the mix buffer
    for (IAMMixerChannel *pmc = m_FirstMixerChannel; pmc != NULL; pmc = pmc->GetNext())
    {
        if (pmc->GetActiveState() != 0)
        {
            if (m_ActiveMixerSettings.Voices <= pmb->MixHeader.MixedVoices)
            {
                pmb->MixHeader.lpData = NULL;
            }
            else
            {
                pmb->MixHeader.lpData = (BYTE *)pmb->WaveHeader.lpData;
            }

            if (pmc->MixBuffer(&pmb->MixHeader) != FALSE)
            {
                fHaveMixed = TRUE;
                pmb->MixHeader.MixedVoices++;
            }
        }
    }

    DPRINTF(4, "MixBuffer out");
    return fHaveMixed;
}

STDMETHODIMP_(void __stdcall) CAMMixer::AllocMixerBuffer(MIXBUFFER *pmb)
{
    assert(pmb->InUse == FALSE);

    pmb->WaveHeader.dwFlags &= ~(WHDR_DONE);
    pmb->InUse = TRUE;
}

STDMETHODIMP CAMMixer::AllocOutputDevice(LPWAVEOUT pWaveOut)
{
    HRESULT hr;

    m_CMixerOut = new CMixerOut();
    if (m_CMixerOut == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = m_CMixerOut->Init((IUnknown *)pWaveOut);

        if (SUCCEEDED(hr))
        {
            m_CMixerOut->AddRef();
        }
        else
        {
            if (m_CMixerOut != NULL)
            {
                delete m_CMixerOut;
            }
            m_CMixerOut = NULL;
        }
    }

    return hr;
}

STDMETHODIMP CAMMixer::FreeOutputDevice()
{
    if (m_CMixerOut != NULL)
    {
        delete m_CMixerOut;
    }
    m_CMixerOut = NULL;

    return S_OK;
}

STDMETHODIMP CAMMixer::AllocMixerTimer()
{
    HRESULT hr = S_OK;
    MMRESULT mr;
    TIMECAPS tc;

    mr = timeGetDevCaps(&tc, sizeof(tc));

    if (mr == MMSYSERR_NOERROR)
    {
        if (tc.wPeriodMin < 61)
        {
            m_TimerID = timeSetEvent(20, 60, MixerTimerFunction, (DWORD_PTR)this, 1);
            if (m_TimerID == 0)
            {
                hr = E_NOTIMER;
                DPRINTF(0, "Timer was not acquired!!!");
            }
            else
            {
                DPRINTF(1, "Timer is Active");
            }
        }
        else
        {
            DPRINTF(0, "Timer min period too small!");
            assert(FALSE);
            hr = E_BADTIMERPERIOD;
        }
    }
    else
    {
        hr = HRESULTFromMMRESULT(mr);
    }

    return hr;
}

STDMETHODIMP CAMMixer::FreeMixerTimer()
{
    HRESULT hr = S_OK;

    if (m_TimerID == 0)
    {
        hr = E_NOTIMER;
    }
    else
    {
        timeKillEvent(m_TimerID);
        m_TimerID = 0;
    }

    return hr;
}

STDMETHODIMP_(void __stdcall) CAMMixer::Pump()
{
    do
    {
        // nothing
    } while (MixNextBuffer() != FALSE);
}

STDMETHODIMP_(BOOL __stdcall) CAMMixer::MixNextBuffer()
{
    BOOL fHaveMixed = FALSE;
    BOOL fNotify = FALSE;
    MIXBUFFER *pmb;
    MIXHEADER mh;

    if (m_IsInitialized == FALSE || m_MixerActive == FALSE || m_IsRemixing != FALSE)
    {
        fHaveMixed = FALSE;
    }
    else
    {
        EnterPump();
        EnterMixer();

        if (m_IsInitialized != FALSE && m_MixerActive != FALSE && m_IsRemixing == FALSE && m_NextMixBuffer != -1)
        {
            if (m_Suspended != FALSE)
            {
                m_CMixerOut->SuspendPump();
            }

            pmb = &m_MixBuffers[m_NextMixBuffer];
            if ((pmb->WaveHeader.dwFlags & WHDR_DONE) != 0)
            {
                if (pmb->InUse != TRUE)
                {
                    DPRINTF(0, "Buffer Not Busy");
                }

                if (pmb->MixHeader.fHaveMixed != FALSE)
                {
                    fNotify = TRUE;

                    mh = pmb->MixHeader;
                    pmb->MixHeader.fHaveMixed = FALSE;
                }

                pmb->InUse = FALSE;

                if (m_MixerStarted)
                {
                    if (FillMixBuffer(pmb) == FALSE)
                    {
                        m_MixerStarted = FALSE;
                    }
                    else
                    {
                        fHaveMixed = TRUE;

                        AllocMixerBuffer(pmb);

                        MMRESULT mmr = m_CMixerOut->Write(&pmb->WaveHeader, sizeof(pmb->WaveHeader));
                        if (mmr != MMSYSERR_NOERROR)
                        {
                            DPRINTF(0, "waveOutWrite FAILED err = %lu", mmr);
                            pmb->InUse = FALSE;
                        }
                    }
                }

                m_NextMixBuffer++;
                if (m_NextMixBuffer == m_MixBufferCount)
                {
                    m_NextMixBuffer = 0;
                }

                if (m_MixerStarted == FALSE)
                {
                    if (CountFreeMixerHeaders() == m_MixBufferCount)
                    {
                        m_NextMixBuffer = -1;
                    }
                }

                if (fNotify != FALSE)
                {
                    MakeNotifications(&mh);
                }
            }
        }

        LeaveMixer();
        LeavePump();
    }

    return fHaveMixed;
}

STDMETHODIMP_(BOOL __stdcall) CAMMixer::GetMixerSettings(MIXERSETTINGS *pms)
{
    if (m_MixerConfigured == FALSE || pms == NULL)
    {
        return FALSE;
    }
    else
    {
        *pms = m_MixerSettings;
        return TRUE;
    }
}

STDMETHODIMP CAMMixer::GetMixerFormat(WAVEFORMATEX *lpPCMFormat)
{
    HRESULT hr = S_OK;
    MIXERSETTINGS ms;

    if (GetMixerSettings(&ms) == FALSE)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *lpPCMFormat = ms.Format;
    }

    return hr;
}

STDMETHODIMP CAMMixer::SetMixerFormat(WAVEFORMATEX *lpPCMFormat)
{
    HRESULT hr = S_OK;
    MIXERSETTINGS ms;

    if (GetMixerSettings(&ms) == FALSE)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        ms.Format = *lpPCMFormat;
        if (SetMixerSettings(&ms) == FALSE)
        {
            hr = E_INVALIDARG;
        }
    }

    return hr;
}

STDMETHODIMP CAMMixer::GetAdvanced(AdvMixerConfig *lpAdvConfig)
{
    HRESULT hr = S_OK;
    MIXERSETTINGS ms;

    if (lpAdvConfig->dwSize == sizeof(AdvMixerConfig))
    {
        if (GetMixerSettings(&ms) == FALSE)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            lpAdvConfig->uVoices = ms.Voices;
            lpAdvConfig->fRemixEnabled = ms.RemixEnabled;
            lpAdvConfig->uBufferTime = ms.BufferTime;
        }
    }
    else
    {
        DPRINTF(0, "Invalid AdvancedConfig Pointer");
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP CAMMixer::SetAdvanced(AdvMixerConfig *lpAdvConfig)
{
    HRESULT hr = S_OK;
    MIXERSETTINGS ms;

    if (lpAdvConfig->dwSize != sizeof(AdvMixerConfig))
    {
        DPRINTF(0, "Invalid AdvancedConfig Pointer");
        hr = E_INVALIDARG;
    }

    if (GetMixerSettings(&ms) == FALSE)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        ms.Voices = lpAdvConfig->uVoices;
        ms.RemixEnabled = lpAdvConfig->fRemixEnabled;
        ms.BufferTime = lpAdvConfig->uBufferTime;
        if (SetMixerSettings(&ms) == FALSE)
        {
            hr = E_INVALIDARG;
        }
    }

    return hr;
}

void __stdcall CAMMixer::MixerTimerFunction(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    CAMMixer *this_mixer = (CAMMixer *)dwUser;
    if (this_mixer->m_TimerID == uTimerID)
    {
        this_mixer->Pump();
    }
}

STDMETHODIMP_(WORD __stdcall) CAMMixer::CountFreeMixerHeaders()
{
    MIXBUFFER *pmb;
    WORD wNumFree;
    DWORD dwIndex;

    wNumFree = 0;
    pmb = m_MixBuffers;

    for (dwIndex = 0; dwIndex < m_MixBufferCount; dwIndex++)
    {
        if (pmb->InUse == FALSE)
        {
            wNumFree++;
        }
        pmb++;
    }

    return wNumFree;
}

STDMETHODIMP_(void __stdcall) CAMMixer::MakeNotifications(MIXHEADER *pmh)
{
    DPRINTF(1, "MakeNotifications in");

    IAMMixerChannel *pmc = m_FirstMixerChannel;
    IAMMixerChannel *pmcLast;

    while (pmc != NULL)
    {
        pmc->AddRef();

        EnterPump();
        pmc->MixNotify(pmh);
        LeavePump();

        pmcLast = pmc->GetNext();
        pmc->Release();
        pmc = pmcLast;
    }

    DPRINTF(1, "MakeNotifications out");
}

STDMETHODIMP CAMMixer::QueryMCInterface(IAMMixerChannel **ppMixerChannel, IUnknown *pUnknown)
{
    HRESULT hr;
    if (pUnknown == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = pUnknown->QueryInterface(IID_IAMMixerChannel, (void **)ppMixerChannel);
    }
    return hr;
}

STDMETHODIMP_(DWORD __stdcall) CAMMixer::CalcRemixBuffers()
{
    DWORD dwBufferNumber = (m_NextMixBuffer + 4) % m_MixBufferCount;
    DWORD dwBuffer = dwBufferNumber;

    for (dwBuffer = dwBufferNumber; dwBuffer != m_NextMixBuffer; dwBuffer++)
    {
        // Check for wrap around
        if (dwBuffer == m_MixBufferCount)
        {
            dwBuffer = 0;
            if (m_NextMixBuffer == 0)
            {
                break;
            }
        }

        // Mark in use buffers for remix
        if (dwBuffer != m_NextMixBuffer && m_MixBuffers[dwBuffer].InUse != FALSE)
        {
            m_MixBuffers[dwBuffer].Remix = TRUE;
        }
    }

    return dwBufferNumber;
}

STDMETHODIMP_(void __stdcall) CAMMixer::RemixBuffers(DWORD dwBufferNumber)
{
    DPRINTF(1, "Remix");

    DWORD dwBuffer = dwBufferNumber;

    while (dwBuffer != m_NextMixBuffer)
    {
        if (dwBuffer == m_MixBufferCount)
        {
            dwBuffer = 0;
            if (m_NextMixBuffer == 0)
            {
                break;
            }
        }

        if (m_NextMixBuffer != dwBuffer && m_MixBuffers[dwBuffer].Remix != FALSE)
        {
            if (FillMixBuffer(&m_MixBuffers[dwBuffer]) == FALSE)
            {
                m_MixBuffers[dwBuffer].InUse = FALSE;
                m_MixBuffers[dwBuffer].WaveHeader.dwFlags |= WHDR_DONE;
                if (m_NextMixBuffer == dwBuffer)
                {
                    m_MixerStarted = FALSE;
                    m_NextMixBuffer = -1;
                }
            }

            m_MixBuffers[dwBuffer].Remix = FALSE;
        }

        dwBuffer++;
    }

    if (CountFreeMixerHeaders() < m_MixBufferCount)
    {
        StartMixer();
    }
}
