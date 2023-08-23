#include "cchannel.h"
#include "dpf.h"
#include "utils.h"
#include "cmixlib.h"
#include "todo.h"
#include <cassert>

STDMETHODIMP_(ULONG) CAMChannel::AddRef()
{
    return ++m_RefCnt;
}

STDMETHODIMP_(ULONG) CAMChannel::Release()
{
    DWORD RefCnt = --m_RefCnt;

    if (m_ChannelRegistered == FALSE || m_RefCnt != 1)
    {
        if (m_RefCnt == 0)
        {
            delete this;
        }
    }
    else
    {
        m_ChannelRegistered = FALSE;
        if (m_pMixer != NULL)
        {
            LPMIXER pMixer = m_pMixer;
            IUnknown *pChannel;

            pMixer->AddRef();

            if (this == NULL)
            {
                pChannel = NULL;
            }
            else
            {
                pChannel = (LPCHANNEL)this;
            }
            pMixer->UnregisterChannel(pChannel);
            pMixer->Release();
        }

        RefCnt = 0;
    }

    return RefCnt;
}

STDMETHODIMP CAMChannel::QueryInterface(REFIID riid, void **ppvObject)
{
    assert(ppvObject != NULL);

    HRESULT hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAMChannel))
    {
        *ppvObject = (IAMChannel *)this;
    }
    else if (IsEqualIID(riid, IID_IAMMixerChannel))
    {
        *ppvObject = (IAMMixerChannel *)this;
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

STDMETHODIMP CAMChannel::RegisterNotify(LPNOTIFYSINK pNotifySink, DWORD fdwNotifyFlags)
{
    HRESULT hr = S_OK;
    LPNOTIFYSINK pOldNS;

    EnterChannel();

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else
    {
        if (m_NotifySink != NULL)
        {
            pOldNS = m_NotifySink;
            m_NotifySink = NULL;
            pOldNS->Release();
        }

        m_NotifySink = pNotifySink;
        if (m_NotifySink != NULL)
        {
            m_NotifySink->AddRef();
            m_NotifyFlags = fdwNotifyFlags;
        }
    }

    LeaveChannel();
    return hr;
}

STDMETHODIMP CAMChannel::SetSoundSrc(LPSOUND pSound)
{
    HRESULT hr = S_OK;
    LPSOUND pNewOrgSrc = NULL;
    LPSOUND pConvertFilter = NULL;
    WAVEFORMATEX wfx;

    EnterChannel();

    if (pSound == NULL)
    {
        if (m_pOrgSrc != NULL)
        {
            if (m_pSoundSrc != NULL)
            {
                m_pSoundSrc->Release();
                m_pSoundSrc = NULL;
            }

            m_pOrgSrc->SetMode(FALSE, TRUE);
            m_pOrgSrc->Release();
            m_pOrgSrc = NULL;
        }
    }
    else
    {
        // Get the source sound's format
        hr = pSound->GetFormat(&wfx, sizeof(wfx));
        if (SUCCEEDED(hr))
        {
            // Create a filter to convert the sound if needed
            if (SameFormats(&wfx, &m_wfx) == FALSE)
            {
                hr = AllocConvertFilter(&pConvertFilter, pSound, &m_wfx);
                if (SUCCEEDED(hr))
                {
                    pNewOrgSrc = pSound;
                    pSound = pConvertFilter;
                }
            }

            if (SUCCEEDED(hr))
            {
                Stop();

                // Free original source
                if (m_pOrgSrc != NULL)
                {
                    if (m_pSoundSrc != NULL)
                    {
                        m_pSoundSrc->Release();
                        m_pSoundSrc = NULL;
                    }
                    m_pOrgSrc->SetMode(FALSE, TRUE);
                    m_pOrgSrc->Release();
                    m_pOrgSrc = NULL;
                }

                // Set new source
                m_pOrgSrc = pSound;
                m_pSoundSrc = pNewOrgSrc;

                if (m_pSoundSrc == NULL)
                {
                    m_pOrgSrc->AddRef();
                }
                else
                {
                    m_pSoundSrc->AddRef();
                }

                m_Samples = m_pOrgSrc->GetSamples();
                m_Position = 0;
                m_Position2 = 0;

                hr = m_pOrgSrc->SetCacheSize(m_NumSamples * m_NumBuffers);

                if (SUCCEEDED(hr))
                {
                    hr = m_pOrgSrc->SetMode(TRUE, TRUE);
                }

                if (SUCCEEDED(hr) && m_IsPlaying == FALSE && m_FUnknown == TRUE)
                {
                    m_IsPlaying = TRUE;
                    m_IsActive = TRUE;
                    DoRemix();
                }
            }
        }
    }

    LeaveChannel();
    return hr;
}

STDMETHODIMP CAMChannel::SetCachedSrc(LPSOUND pSound, LPCACHECONFIG pCacheConfig)
{
    HRESULT hr;
    BOOL fChanged = FALSE;
    LPSOUND pCache;

    EnterChannel();

    if (pSound == NULL && pCacheConfig == NULL)
    {
        hr = SetSoundSrc(NULL);
    }
    else if (pCacheConfig == NULL)
    {
        hr = SetSoundSrc(pSound);
    }
    else if (pCacheConfig->dwSize == sizeof(CACHECONFIG))
    {
        if (pCacheConfig->fSrcFormat == 0 && pCacheConfig->lpFormat == NULL && pCacheConfig->dwFormat == 0xFFFFFFFF)
        {
            pCacheConfig->lpFormat = &m_wfx;
            pCacheConfig->dwFormat = 0;
            fChanged = TRUE;
        }

        hr = AllocCacheFilter(&pCache, pSound, pCacheConfig);

        if (SUCCEEDED(hr))
        {
            hr = SetSoundSrc(NULL);

            if (SUCCEEDED(hr))
            {
                hr = SetSoundSrc(pCache);

                if (SUCCEEDED(hr))
                {
                    if (m_pSoundSrc == NULL)
                    {
                        m_pSoundSrc = pSound;
                        m_pSoundSrc->AddRef();
                    }
                    else
                    {
                        m_pSoundSrc->Release();
                        m_pSoundSrc = pSound;
                        m_pSoundSrc->AddRef();
                    }
                }
            }

            pCache->Release();
        }

        if (fChanged)
        {
            pCacheConfig->lpFormat = NULL;
            pCacheConfig->dwFormat = 0xFFFFFFFF;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    LeaveChannel();
    return hr;
}

STDMETHODIMP CAMChannel::GetSoundSrc(LPSOUND *ppSound)
{
    HRESULT hr = S_OK;

    EnterChannel();

    if (ppSound == NULL)
    {
        hr = E_FAIL;
    }
    else if (m_pSoundSrc == NULL)
    {
        if (m_pOrgSrc == NULL)
        {
            hr = E_FAIL;
        }
        else
        {
            m_pOrgSrc->AddRef();
            *ppSound = m_pOrgSrc;
        }
    }
    else
    {
        m_pSoundSrc->AddRef();
        *ppSound = m_pOrgSrc;
    }

    LeaveChannel();

    return hr;
}

STDMETHODIMP CAMChannel::Play(void)
{
    HRESULT hr = S_OK;

    EnterChannel();

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else if (m_IsPlaying == FALSE)
    {
        m_FUnknown = TRUE;
        if (m_pOrgSrc != NULL)
        {
            ClearNotify(0xFFFFFFFF);
            m_IsPlaying = TRUE;
            m_IsActive = TRUE;
            DoRemix();
        }
    }

    LeaveChannel();
    return hr;
}

STDMETHODIMP CAMChannel::Stop(void)
{
    HRESULT hr = S_OK;
    LPSOUND pSoundSrc;

    EnterChannel();

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else
    {
        m_FUnknown = FALSE;

        ClearNotify(0xFFFFFFFF);

        if (m_IsPlaying)
        {
            pSoundSrc = m_pSoundSrc;
            if (pSoundSrc == NULL)
            {
                pSoundSrc = m_pOrgSrc;
            }
            m_IsPlaying = FALSE;
            m_IsActive = FALSE;

            DoRemix();

            if (pSoundSrc != NULL && m_NotifySink != NULL && (m_NotifyFlags & NOTIFYSINK_ONCOMPLETION) != 0)
            {
                LeaveChannel();
                m_NotifySink->OnCompletion(pSoundSrc, m_Position);
                EnterChannel();
            }
        }
    }

    LeaveChannel();
    return hr;
}

STDMETHODIMP CAMChannel::Finish(void)
{
    HRESULT hr = S_OK;

    EnterChannel();
    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    LeaveChannel();
    return hr;
}

STDMETHODIMP_(BOOL) CAMChannel::IsPlaying(void)
{
    BOOL fResult = FALSE;

    EnterChannel();

    if (m_Initialized)
    {
        fResult = m_IsPlaying;
    }

    LeaveChannel();
    return fResult;
}

STDMETHODIMP_(DWORD) CAMChannel::Samples(void)
{
    TODO_NOT_IMPLEMENTED;
    return 0;
}

STDMETHODIMP CAMChannel::SetPosition(DWORD dwSample)
{
    HRESULT hr = S_OK;

    EnterChannel();

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else if (m_pOrgSrc != NULL)
    {
        if (m_Samples < dwSample)
        {
            dwSample = m_Samples;
        }

        m_Position = dwSample;
        m_Position2 = dwSample;

        if (m_IsPlaying)
        {
            m_IsActive = TRUE;
            DoRemix();
        }
    }

    LeaveChannel();
    return hr;
}

STDMETHODIMP CAMChannel::GetPosition(LPDWORD lpdwSample)
{
    HRESULT hr = S_OK;

    EnterChannel();
    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else if (lpdwSample == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *lpdwSample = m_Position2;
    }

    LeaveChannel();
    return hr;
}

STDMETHODIMP CAMChannel::Mute(BOOL fMute)
{
    TODO_NOT_IMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CAMChannel::SetVolume(DWORD dwVolume)
{
    HRESULT hr = S_OK;

    EnterChannel();

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else
    {
        m_OldStyleVolume = dwVolume;
        DWORD volume = CalcIntVolumeFromOldStyle(dwVolume, &m_wfx, &m_wfx);
        SetChannelVol(volume, volume >> 16);
    }

    LeaveChannel();
    return hr;
}

STDMETHODIMP CAMChannel::GetVolume(LPDWORD lpdwVolume)
{
    HRESULT hr = S_OK;

    EnterChannel();

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else if (lpdwVolume == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *lpdwVolume = m_OldStyleVolume;
    }

    LeaveChannel();
    return hr;
}

STDMETHODIMP CAMChannel::SetGain(float flLeft, float flRight)
{
    TODO_NOT_IMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CAMChannel::GetGain(float *lpflLeft, float *lpflRight)
{
    HRESULT hr = S_OK;

    EnterChannel();

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else if (lpflLeft == NULL || lpflRight == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *lpflLeft = m_GainLeft;
        *lpflRight = m_GainRight;
    }

    LeaveChannel();
    return hr;
}

STDMETHODIMP CAMChannel::GetSMPTEPos(LPSMPTE lpSMPTE)
{
    HRESULT hr = S_OK;
    DWORD dwSample;
    DWORD dwTime;

    EnterChannel();

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else if (lpSMPTE == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = GetPosition(&dwSample);
        if (SUCCEEDED(hr))
        {
            dwTime = SamplesToMillisec(&m_wfx, dwSample);
            if (ConvertMillisecToSMPTE(lpSMPTE, dwTime) == FALSE)
            {
                hr = E_FAIL;
            }
        }
    }

    LeaveChannel();
    return hr;
}

STDMETHODIMP CAMChannel::SetSMPTEPos(LPSMPTE lpSMPTE)
{
    TODO_NOT_IMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CAMChannel::GetTimePos(LPDWORD lpdwTime)
{
    HRESULT hr = S_OK;

    EnterChannel();

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else
    {
        DWORD dwSample;
        hr = GetPosition(&dwSample);

        if (SUCCEEDED(hr))
        {
            *lpdwTime = SamplesToMillisec(&m_wfx, dwSample);
        }
    }

    LeaveChannel();
    return hr;
}

STDMETHODIMP CAMChannel::SetTimePos(DWORD dwTime)
{
    HRESULT hr = S_OK;

    EnterChannel();

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else
    {
        hr = SetPosition(MillisecToSamples(&m_wfx, dwTime));
    }

    LeaveChannel();
    return hr;
}

STDMETHODIMP CAMChannel::SetMutx(MUTX *mutex)
{
    m_ChannelLock = mutex;
    return S_OK;
}

STDMETHODIMP CAMChannel::Configure(DWORD dwNumBuffers, DWORD dwNumSamples, WAVEFORMATEX *lpwfx)
{
    HRESULT hr = S_OK;
    LPSOUND pSound;
    DWORD dwCurNumBuffers, dwSample;
    WAVEFORMATEX wfx;

    if (m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else if (lpwfx == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        // Reallocate buffers
        if (m_ArrayOfBuffers != NULL)
        {
            delete[] m_ArrayOfBuffers;
        }
        m_ArrayOfBuffers = new ChannelBuffer[dwNumBuffers];
        if (m_ArrayOfBuffers == NULL)
        {
            if (m_BufferPositions != NULL)
            {
                delete[] m_BufferPositions;
            }
            m_BufferPositions = NULL;
            hr = E_OUTOFMEMORY;
        }
        else
        {
            if (m_BufferPositions != NULL)
            {
                delete[] m_BufferPositions;
            }

            m_BufferPositions = new DWORD[dwNumBuffers * 4];
            if (m_BufferPositions == NULL)
            {
                if (m_ArrayOfBuffers != NULL)
                {
                    delete[] m_ArrayOfBuffers;
                }
                m_ArrayOfBuffers = NULL;
                hr = E_OUTOFMEMORY;
            }
            else
            {
                pSound = NULL;
                dwCurNumBuffers = dwNumBuffers;

                wfx = m_wfx;
                m_NumBuffers = dwNumBuffers;
                m_NumSamples = dwNumSamples;
                m_wfx = *lpwfx;

                dwSample = MillisecToSamples(lpwfx, SamplesToMillisec(&wfx, m_Position));

                if (m_pSoundSrc == NULL && m_pOrgSrc != NULL)
                {
                    pSound = m_pOrgSrc;
                }
                else
                {
                    pSound = m_pSoundSrc;
                }

                if (pSound != NULL)
                {
                    pSound->AddRef();
                    hr = SetSoundSrc(pSound);
                    pSound->Release();
                }

                if (FAILED(hr))
                {
                    m_wfx = wfx;
                }
                else
                {
                    m_wfx = *lpwfx;
                    m_Position = dwSample;
                    m_MixerProc = CalcMixerProc(m_CombinedVolume, m_wfx.wBitsPerSample, FALSE);
                    m_MixerProcVolOnly = CalcMixerProc(m_CombinedVolume, m_wfx.wBitsPerSample, TRUE);
                    m_MixerProcSet = TRUE;
                }
            }
        }
    }
    return hr;
}

STDMETHODIMP_(BOOL) CAMChannel::MixBuffer(MIXHEADER *lpMixHeader)
{
    BOOL fHaveMixed = FALSE;
    BOOL fHaveMarked = FALSE;

    if (m_MixerProcSet != FALSE && m_IsPlaying != FALSE && m_IsActive != FALSE && m_pOrgSrc != NULL)
    {
        DWORD dwSamples = m_NumSamples;
        DWORD dwBytes = 0;
        BYTE *lpData = lpMixHeader->lpData;
        BYTE *lpAux = lpMixHeader->lpAux;
        DWORD dwBuffer = lpMixHeader->dwBuffer;

        ClearNotify(dwBuffer);

        if (m_Position == 0 && ((m_NotifyFlags & NOTIFYSINK_ONSTART) != 0))
        {
            m_ArrayOfBuffers[dwBuffer].fUnknown = TRUE;
            m_ArrayOfBuffers[dwBuffer].Flags |= 1;
            m_ArrayOfBuffers[dwBuffer].StartPosition = m_Position;
        }

        if (m_Samples < (m_Position + dwSamples))
        {
            m_IsActive = FALSE;
            dwSamples = m_Samples - m_Position;
            m_ArrayOfBuffers[dwBuffer].fUnknown = TRUE;
            m_ArrayOfBuffers[dwBuffer].Flags |= 2;
            m_ArrayOfBuffers->EndPosition = m_Position + dwSamples;
        }

        if (lpData != NULL && lpAux != NULL)
        {
            if (m_CombinedVolume == 0)
            {
                fHaveMixed = TRUE;
                lpMixHeader->fHaveMixed = TRUE;
            }
            else
            {
                HRESULT hr;
                if (lpMixHeader->MixedVoices == 0 && m_Muted == FALSE)
                {
                    hr = m_pOrgSrc->GetSampleData(lpData, m_Position, &dwSamples, NULL);
                }
                else
                {
                    hr = m_pOrgSrc->GetSampleData(lpAux, m_Position, &dwSamples, 0);
                }

                if (SUCCEEDED(hr))
                {
                    if (hr == S_ENDOFSOUND)
                    {
                        m_IsActive = FALSE;
                        m_ArrayOfBuffers[dwBuffer].fUnknown = TRUE;
                        m_ArrayOfBuffers[dwBuffer].Flags |= 2;
                        m_ArrayOfBuffers[dwBuffer].EndPosition = m_Position + dwSamples;
                    }

                    fHaveMixed = TRUE;
                    dwBytes = SamplesToBytes(&m_wfx, dwSamples);
                    if (lpMixHeader->MixedVoices == 0)
                    {
                        if (m_NeedsVolumeMix != FALSE && m_Muted == FALSE)
                        {
                            m_MixerProcVolOnly(m_CombinedVolume, lpData, lpData, dwBytes & 0xFFFF, NULL);
                        }
                    }
                    else if (m_Muted == FALSE)
                    {
                        m_MixerProc(m_CombinedVolume, lpData, lpAux, dwBytes & 0xFFFF, NULL);
                    }

                    lpMixHeader->fHaveMixed = TRUE;
                }
            }
        }
        m_BufferPositions[lpMixHeader->dwBuffer] = m_Position;
        m_Position = m_Position + dwSamples;
    }

    return fHaveMixed;
}

STDMETHODIMP CAMChannel::MixNotify(MIXHEADER *lpMixHeader)
{
    HRESULT hr = S_OK;
    DWORD dwBuffer = lpMixHeader->dwBuffer;

    if (m_MixerProcSet == FALSE)
    {
        hr = E_FAIL;
    }
    else if (m_pOrgSrc != NULL && m_IsPlaying != FALSE)
    {
        m_Position2 = m_Position2 + m_NumSamples;
        if (m_Samples <= m_Position2)
        {
            m_Position2 = m_Samples;
        }
        if (m_ArrayOfBuffers[dwBuffer].fUnknown != FALSE)
        {
            LPSOUND psnd = m_pSoundSrc;
            if (psnd == NULL)
            {
                psnd = m_pOrgSrc;
            }

            DWORD flags = m_ArrayOfBuffers[dwBuffer].Flags;
            ClearNotify(dwBuffer);
            if ((flags & NOTIFYSINK_ONSTART) == 0)
            {
                if ((flags & NOTIFYSINK_ONCOMPLETION) != 0)
                {
                    m_IsPlaying = FALSE;
                    if (m_NotifySink != NULL && (m_NotifyFlags & NOTIFYSINK_ONCOMPLETION) != 0)
                    {
                        LeaveChannel();
                        m_NotifySink->OnCompletion(psnd, m_ArrayOfBuffers[dwBuffer].EndPosition);
                        EnterChannel();
                    }
                }
            }
            else if (m_NotifySink != NULL && (m_NotifyFlags & NOTIFYSINK_ONSTART) != 0)
            {
                LeaveChannel();
                m_NotifySink->OnStart(psnd, m_ArrayOfBuffers[dwBuffer].StartPosition);
                EnterChannel();
            }
        }
    }

    return hr;
}

STDMETHODIMP CAMChannel::RevertTo(DWORD dwBuffer)
{
    HRESULT hr = S_OK;

    if (m_MixerProcSet == FALSE)
    {
        hr = E_FAIL;
    }
    else if (m_Remixing == FALSE)
    {
        m_Position = m_BufferPositions[dwBuffer];
        m_IsActive = TRUE;
    }
    else
    {
        m_Remixing = TRUE;
    }

    return hr;
}

STDMETHODIMP_(DWORD) CAMChannel::GetActiveState()
{
    DWORD activeState = 0;
    if (m_MixerProcSet != FALSE)
    {
        activeState = m_IsPlaying;
    }

    return activeState;
}

STDMETHODIMP CAMChannel::DoStart(BOOL fStart)
{
    HRESULT hr = S_OK;

    if (m_MixerProcSet == FALSE)
    {
        hr = E_FAIL;
    }
    else if (m_IsPlaying != fStart)
    {
        LPSOUND pSnd = m_pSoundSrc;
        if (pSnd == NULL)
        {
            pSnd = m_pOrgSrc;
        }

        ClearNotify(0xFFFFFFFF);

        m_IsPlaying = fStart;
        m_IsActive = m_IsPlaying;

        if (fStart == FALSE && pSnd != NULL && m_NotifySink != NULL && ((m_NotifyFlags & NOTIFYSINK_ONCOMPLETION) != 0))
        {
            LeaveChannel();
            m_NotifySink->OnCompletion(pSnd, m_Position);
            EnterChannel();
        }
    }

    return hr;
}

STDMETHODIMP CAMChannel::DoReset()
{
    HRESULT hr = S_OK;

    if (m_MixerProcSet == FALSE)
    {
        hr = E_FAIL;
    }
    else
    {
        m_IsPlaying = FALSE;
        m_IsActive = FALSE;
        m_Position = 0;
        m_Position2 = 0;
        ClearNotify(0xFFFFFFFF);
    }

    return hr;
}

STDMETHODIMP CAMChannel::DoPosition(DWORD dwPosition)
{
    HRESULT hr = S_OK;

    if (m_MixerProcSet == FALSE)
    {
        hr = E_FAIL;
    }
    else
    {
        if (m_Samples < dwPosition)
        {
            dwPosition = m_Samples;
        }

        m_Position = dwPosition;
        m_Position2 = dwPosition;
    }

    return hr;
}

STDMETHODIMP CAMChannel::DoVolume(DWORD dwVolume)
{
    HRESULT hr = S_OK;

    if (m_MixerProcSet == FALSE)
    {
        hr = E_FAIL;
    }
    else
    {
        m_SettingVolume = TRUE;
        hr = SetVolume(dwVolume);
        m_SettingVolume = FALSE;
    }
    return hr;
}

STDMETHODIMP CAMChannel::DoGain(float flLeft, float flRight, BOOL fAbsolute)
{
    HRESULT hr = S_OK;

    if (m_MixerProcSet == FALSE || m_Initialized == FALSE)
    {
        hr = E_FAIL;
    }
    else
    {
        m_SettingVolume = FALSE;

        if (fAbsolute == FALSE)
        {
            hr = SetGain(m_GainLeft + flLeft, m_GainRight + flRight);
        }
        else
        {
            hr = SetGain(flLeft, flRight);
        }

        m_SettingVolume = TRUE;
    }

    return hr;
}

STDMETHODIMP CAMChannel::SetNext(IAMMixerChannel *pNextChannel)
{
    m_pNextChannel = pNextChannel;
    return S_OK;
}

STDMETHODIMP_(IAMMixerChannel *) CAMChannel::GetNext()
{
    return m_pNextChannel;
}

STDMETHODIMP CAMChannel::SetPriority(DWORD dwPriority)
{
    m_Priority = dwPriority;
    return S_OK;
}

STDMETHODIMP_(DWORD) CAMChannel::GetPriority()
{
    return m_Priority;
}

STDMETHODIMP CAMChannel::SetGroup(DWORD dwGroup)
{
    m_Group = dwGroup;
    return S_OK;
}

STDMETHODIMP_(DWORD) CAMChannel::GetGroup()
{
    return m_Group;
}

CAMChannel::CAMChannel()
{
    m_OldStyleVolume = 0xFFFFFFFF;
    m_VolumeLeft = 16;
    m_VolumeRight = 16;
    m_CombinedVolume = (m_VolumeRight * 10000) + m_VolumeLeft;
    m_SettingVolume = TRUE;
}

CAMChannel::~CAMChannel()
{
    DPRINTF(1, "Destructing Channel");
    if (m_pMixer != NULL)
    {
        m_pMixer->Release();
    }

    if (m_pOrgSrc != NULL)
    {
        if (m_pSoundSrc != NULL)
        {
            m_pSoundSrc->Release();
        }
        m_pOrgSrc->SetMode(FALSE, TRUE);
        m_pOrgSrc->Release();
    }

    if (m_NotifySink != NULL)
    {
        m_NotifySink->Release();
    }

    if (m_ArrayOfBuffers != NULL)
    {
        delete[] m_ArrayOfBuffers;
    }

    if (m_BufferPositions != NULL)
    {
        delete[] m_BufferPositions;
    }
}

STDMETHODIMP CAMChannel::Init(LPMIXER pMixer)
{
    HRESULT hr = S_OK;
    MIXERCONFIG mixerConfig = {0};
    WAVEFORMATEX wfx = {0};

    if (m_Initialized == FALSE)
    {
        if (pMixer == NULL)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            m_pMixer = pMixer;
            m_pMixer->AddRef();
        }
    }
    else
    {
        hr = E_FAIL;
    }

    // Get mixer's wave format
    if (SUCCEEDED(hr))
    {
        mixerConfig.dwSize = sizeof(mixerConfig);
        mixerConfig.lpFormat = &wfx;

        hr = m_pMixer->GetConfig(&mixerConfig, NULL);

        if (SUCCEEDED(hr))
        {
            m_wfx = wfx;
        }
    }

    if (SUCCEEDED(hr))
    {
        m_Initialized = TRUE;

        hr = m_pMixer->RegisterChannel((LPCHANNEL)this);
        if (SUCCEEDED(hr))
        {
            m_ChannelRegistered = TRUE;
        }
    }

    if (FAILED(hr))
    {
        m_Initialized = FALSE;
        if (m_pMixer != NULL)
        {
            m_pMixer->Release();
            m_pMixer = NULL;
        }
    }

    return hr;
}

void CAMChannel::EnterChannel()
{
    if (m_ChannelLock != NULL)
    {
        m_ChannelLock->Enter();
    }
}

void CAMChannel::LeaveChannel()
{
    if (m_ChannelLock != NULL)
    {
        m_ChannelLock->Leave();
    }
}

void CAMChannel::ClearNotify(DWORD dwBuffer)
{
    if (dwBuffer == 0xFFFFFFFF)
    {
        for (dwBuffer = 0; dwBuffer < m_NumBuffers; dwBuffer++)
        {
            m_ArrayOfBuffers[dwBuffer].fUnknown = FALSE;
            m_ArrayOfBuffers[dwBuffer].Flags = 0;
            m_BufferPositions[dwBuffer] = 0;
        }
    }
    else
    {
        assert(dwBuffer < m_NumBuffers);
        m_ArrayOfBuffers[dwBuffer].fUnknown = FALSE;
        m_ArrayOfBuffers[dwBuffer].Flags = 0;
    }
}

void CAMChannel::DoRemix()
{
    m_Remixing = TRUE;
    m_pMixer->Refresh();
    m_Remixing = FALSE;
}

void CAMChannel::SetChannelVol(WORD wLeft, WORD wRight)
{
    m_VolumeLeft = wLeft;
    m_VolumeRight = wRight;

    if (m_VolumeLeft == 16 && m_VolumeRight == 16)
    {
        m_NeedsVolumeMix = FALSE;
    }
    else
    {
        m_NeedsVolumeMix = TRUE;
    }

    m_CombinedVolume = ((DWORD)m_VolumeRight << 16) + m_VolumeLeft;
    m_MixerProc = CalcMixerProc(m_CombinedVolume, m_wfx.wBitsPerSample, FALSE);
    m_MixerProcVolOnly = CalcMixerProc(m_CombinedVolume, m_wfx.wBitsPerSample, TRUE);

    if (m_IsPlaying != FALSE && m_SettingVolume != FALSE)
    {
        m_pMixer->Refresh();
    }
}