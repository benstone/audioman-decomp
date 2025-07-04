#include <cassert>
#include "audiomaninternal.h"
#include "dpf.h"
#include "riff.h"
#include "sound.h"
#include "stream.h"
#include "utils.h"

STDAPI AllocSoundFromStream(LPSOUND FAR *ppSound, LPSTREAM pStream, BOOL fSpooled, LPCACHECONFIG lpCacheConfig)
{
    LPSOUND pSnd;
    IAMWavFileSrc *pWavSrc;
    HRESULT hr = S_OK;

    assert(pStream != NULL);
    assert(ppSound != NULL);

    hr = AMCreate(CLSID_AMWavFileSrc, IID_IAMWavFileSrc, (void **)&pWavSrc, &pSnd);

    if (SUCCEEDED(hr))
    {
        hr = pWavSrc->InitFromStream(pStream, fSpooled);
        if (SUCCEEDED(hr))
        {
            hr = AMFinishCache(ppSound, pWavSrc, pSnd, fSpooled, lpCacheConfig);
        }
        else
        {
            DPRINTF(0, "AllocSoundFromStream:  InitFromStream failed!");
            pWavSrc->Release();
            pSnd->Release();
        }
    }

    return hr;
}

STDAPI AllocSoundFromFile(LPSOUND FAR *ppSound, char FAR *szFileName, DWORD dwOffset, BOOL fSpooled,
                          LPCACHECONFIG lpCacheConfig)
{
    LPSOUND pSnd;
    IAMWavFileSrc *pWavSrc;
    HRESULT hr = S_OK;

    assert(ppSound != NULL);
    assert(szFileName != NULL);

    hr = AMCreate(CLSID_AMWavFileSrc, IID_IAMWavFileSrc, (void **)&pWavSrc, &pSnd);

    if (SUCCEEDED(hr))
    {
        hr = pWavSrc->InitFromFile(szFileName, dwOffset, fSpooled);
        if (SUCCEEDED(hr))
        {
            hr = AMFinishCache(ppSound, pWavSrc, pSnd, fSpooled, lpCacheConfig);
        }
        else
        {
            DPRINTF(0, "AllocSoundFromFile:  InitFromFile failed!");
            pWavSrc->Release();
            pSnd->Release();
        }
    }

    return hr;
}

STDAPI AllocSoundFromMemory(LPSOUND FAR *ppSound, LPBYTE lpFileData, DWORD dwSize)
{
    LPSOUND pSnd;
    IAMWavFileSrc *pWavSrc;
    HRESULT hr = S_OK;

    assert(lpFileData != NULL);
    assert(ppSound != NULL);

    hr = AMCreate(CLSID_AMWavFileSrc, IID_IAMWavFileSrc, (void **)&pWavSrc, &pSnd);

    if (SUCCEEDED(hr))
    {
        hr = pWavSrc->InitFromMemory((char *)lpFileData, dwSize);
        AMFinish(hr, ppSound, pWavSrc, pSnd);
    }

    return hr;
}

STDMETHODIMP_(HRESULT __stdcall) CAMWavFileSrc::InitFromStream(IStream *pStream, BOOL fSpooled)
{
    return _FInit(pStream, fSpooled);
}

STDMETHODIMP_(HRESULT __stdcall) CAMWavFileSrc::InitFromMemory(char *lpMemory, DWORD dwLength)
{
    CMemoryStream *pMemoryStream = new CMemoryStream((BYTE *)lpMemory, dwLength, 2);
    return _FInit(pMemoryStream, FALSE);
}

STDMETHODIMP_(HRESULT __stdcall) CAMWavFileSrc::InitFromFile(char *pAbsFilePath, DWORD dwOffset, BOOL fSpooled)
{
    CFileStream *pInputStream = new CFileStream(pAbsFilePath, dwOffset, 2);
    return _FInit(pInputStream, fSpooled);
}

HRESULT CAMWavFileSrc::_FInit(IStream *pInputStream, BOOL fSpooled)
{
    HRESULT hr = S_OK;
    m_Flags = m_Flags & 0xFFFFFFFE | (fSpooled & 1);

    if (pInputStream == NULL)
    {
        hr = E_FAIL;
    }
    else
    {
        // Read header
        pInputStream->AddRef();
        hr = _ReadHeader(pInputStream);

        if (FAILED(hr))
        {
            pInputStream->Release();
        }
        else
        {
            // Seek to start of stream
            LARGE_INTEGER zero = {0};
            pInputStream->Seek(zero, STREAM_SEEK_SET, NULL);

            if (fSpooled)
            {
                m_SourceStream = pInputStream;
            }
            else
            {
                m_SourceStream = new CMemoryStream(pInputStream, m_MemoryStreamSize);
            }

            m_SourceStream->AddRef();
            pInputStream->Release();

            if (m_wfxSource->wFormatTag == WAVE_FORMAT_PCM)
            {
                m_wfxDest = m_wfxSource;
                m_wfxSource = NULL;
                m_DestDataSize = m_DataSize;
            }
            else
            {
                // Get destination wave format from ACM
                // Destination wave format can be up to 512 bytes
                BYTE wfxDest[0x200];
                memset(wfxDest, 0, sizeof(wfxDest));

                if (acmFormatSuggest(0, m_wfxSource, (LPWAVEFORMATEX)&wfxDest, sizeof(wfxDest), 0) != MMSYSERR_NOERROR)
                {
                    return E_FAIL;
                }

                DWORD wave_format_size = ((LPWAVEFORMATEX)&wfxDest)->cbSize + 0x13;
                m_wfxDest = (LPWAVEFORMATEX) new BYTE[wave_format_size]();
                if (m_wfxDest == NULL)
                {
                    return E_OUTOFMEMORY;
                }
                memcpy(m_wfxDest, wfxDest, wave_format_size);
                AllocACMStream();
            }

            m_Samples = BytesToSamples(m_wfxDest, m_DestDataSize);
        }
    }
    return hr;
}

HRESULT CAMWavFileSrc::_ReadHeader(IStream *pInputStream)
{
    HRESULT hr = S_OK;
    CRIFF RIFF(pInputStream);
    WAVEFORMATEX WaveFormatEx;
    RIFFTag ulChunkTag;
    ULONG ulChunkLen;

    // Read the RIFF tag
    hr = RIFF.ReadRIFFTag(&ulChunkTag);
    if (hr == S_OK && ulChunkTag == kRIFFTagRIFF)
    {
        hr = RIFF.ReadLongData(&ulChunkLen);
    }
    else
    {
        if (hr == 0)
        {
            // bad header tag
            hr = E_FAIL;
        }
    }

    if (hr == S_OK)
    {
        // Find the WAVE tag
        hr = RIFF.ReadRIFFTag(&ulChunkTag);
        if (hr == S_OK)
        {
            if (ulChunkTag != kRIFFTagWAVE)
            {
                // Not a WAVE
                hr = E_FAIL;
            }
        }
    }

    if (hr == S_OK)
    {
        // Find the fmt chunk
        hr = RIFF.FindNextFormChunk(&ulChunkTag, &ulChunkLen);
        if (hr == S_OK)
        {
            if (ulChunkTag != kRIFFTagFormat)
            {
                // not a fmt chunk
                hr = E_FAIL;
            }
        }
    }

    if (hr == S_OK)
    {
        // Check the fmt chunk is the expected size
        if (ulChunkLen == sizeof(PCMWAVEFORMAT))
        {
            hr = RIFF.GetStream()->Read(&WaveFormatEx, sizeof(PCMWAVEFORMAT), NULL);
            WaveFormatEx.cbSize = 0;
        }
        else if (ulChunkLen < sizeof(WAVEFORMATEX))
        {
            hr = E_FAIL;
        }
        else
        {
            hr = RIFF.GetStream()->Read(&WaveFormatEx, sizeof(WaveFormatEx), NULL);
        }
    }

    if (hr == S_OK)
    {
        // Swap endianness of fields in wave format header
        WaveFormatEx.wFormatTag = SwapWORD(WaveFormatEx.wFormatTag);
        WaveFormatEx.nChannels = SwapWORD(WaveFormatEx.nChannels);
        WaveFormatEx.nSamplesPerSec = SwapDWORD(WaveFormatEx.nSamplesPerSec);
        WaveFormatEx.nAvgBytesPerSec = SwapDWORD(WaveFormatEx.nAvgBytesPerSec);
        WaveFormatEx.nBlockAlign = SwapWORD(WaveFormatEx.nBlockAlign);
        DPRINTF(3, "CAMWavFileSrc::ReadHeader() NON PCM DATA extra data size is %d!\n", WaveFormatEx.cbSize);
    }

    if (hr == S_OK)
    {
        // Allocate the wave format
        DWORD wave_format_size = WaveFormatEx.cbSize + sizeof(WAVEFORMATEX);
        m_wfxSource = (LPWAVEFORMATEX) new BYTE[wave_format_size]();
        if (m_wfxSource == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (hr == S_OK)
    {
        // Copy the wave format
        memcpy(m_wfxSource, &WaveFormatEx, sizeof(WaveFormatEx));
        if (WaveFormatEx.cbSize != 0)
        {
            hr = RIFF.GetStream()->Read(((BYTE *)m_wfxSource) + sizeof(WAVEFORMATEX), ulChunkLen - sizeof(WAVEFORMATEX),
                                        NULL);
        }

        m_SourceFormat = ConvertWaveFormatExToFormat(m_wfxSource);
    }

    while (hr == S_OK)
    {
        // Read next chunk header
        hr = RIFF.FindNextFormChunk(&ulChunkTag, &ulChunkLen);
        if (SUCCEEDED(hr))
        {
            // Check if 'data' chunk
            if (ulChunkTag == kRIFFTagData)
            {
                m_DataSize = ulChunkLen;

                // Get current position
                LARGE_INTEGER seek_to = {0};
                ULARGE_INTEGER pos = {0};
                hr = RIFF.GetStream()->Seek(seek_to, STREAM_SEEK_CUR, &pos);

                m_DataStart = pos.LowPart;

                if (m_DataStart == 0xFFFFFFFF)
                {
                    hr = E_FAIL;
                }
                else
                {
                    m_MemoryStreamSize = m_DataStart + m_DataSize;
                }
                break;
            }
        }

        // Move to next chunk header
        if (SUCCEEDED(hr))
        {
            LARGE_INTEGER seek_to_next = {0};
            seek_to_next.LowPart = ulChunkLen;
            hr = RIFF.GetStream()->Seek(seek_to_next, STREAM_SEEK_CUR, NULL);
        }
    }

    return hr;
}

HRESULT CAMWavFileSrc::AllocACMStream()
{
    MMRESULT mmr;
    HRESULT hr = S_OK;

    assert(m_DstBuffer == NULL);

    // Open the stream
    mmr = acmStreamOpen(&m_StreamHandle, NULL, m_wfxSource, m_wfxDest, 0, 0, 0, 0);
    if (mmr == MMSYSERR_NOERROR)
    {
        // Calculate the size of the conversion buffer
        m_cbDstBuffer = 0x1000;
        do
        {
            mmr = acmStreamSize(m_StreamHandle, m_cbDstBuffer, &m_cbSrcBuffer, ACM_STREAMSIZEF_DESTINATION);
            if (mmr != MMSYSERR_NOERROR)
            {
                m_cbDstBuffer = m_cbDstBuffer + 0x1000;
                if (m_cbDstBuffer > 0x8000)
                {
                    return E_FAIL;
                }
            }
        } while (mmr != MMSYSERR_NOERROR);

        // Allocate conversion buffer
        m_DstBuffer = new byte[m_cbDstBuffer];

        if (m_DstBuffer == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            acmStreamSize(m_StreamHandle, m_wfxSource->nBlockAlign, &m_dwOutputBytes, ACM_STREAMSIZEF_SOURCE);
            m_SrcBuffer = m_DstBuffer + (m_cbDstBuffer - m_cbSrcBuffer);

            m_StreamHeader.pbSrc = m_SrcBuffer;
            m_StreamHeader.pbDst = m_DstBuffer;
            m_StreamHeader.cbSrcLength = m_cbSrcBuffer;
            m_StreamHeader.cbDstLength = m_cbDstBuffer;

            mmr = acmStreamPrepareHeader(m_StreamHandle, &m_StreamHeader, FALSE);

            if (mmr == MMSYSERR_NOERROR)
            {
                if (m_DataSize < m_cbSrcBuffer)
                {
                    m_DestDataSize = m_DataSize * 2;
                }
                else
                {
                    mmr = acmStreamSize(m_StreamHandle, m_DataSize, &m_DestDataSize, ACM_STREAMSIZEF_SOURCE);
                    if (mmr != MMSYSERR_NOERROR)
                    {
                        hr = E_FAIL;
                    }
                }
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

void CAMWavFileSrc::FreeACMStream()
{
    if (m_DstBuffer != NULL)
    {
        delete[] m_DstBuffer;
        m_DstBuffer = NULL;
        m_SrcBuffer = NULL;
    }

    if (m_StreamHandle != NULL)
    {
        m_StreamHeader.cbSrcLength = m_cbSrcBuffer;
        m_StreamHeader.cbDstLength = m_cbDstBuffer;
        acmStreamUnprepareHeader(m_StreamHandle, &m_StreamHeader, FALSE);
        acmStreamClose(m_StreamHandle, FALSE);
        m_StreamHandle = NULL;
    }
}

ULONG __stdcall CAMWavFileSrc::AddRef()
{
    return ++m_RefCnt;
}

ULONG __stdcall CAMWavFileSrc::Release()
{
    ULONG RefCnt = --m_RefCnt;

    if (RefCnt == 0)
    {
        delete this;
    }

    return RefCnt;
}

HRESULT __stdcall CAMWavFileSrc::QueryInterface(REFIID riid, void **ppvObject)
{
    if (ppvObject == NULL)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAMWavFileSrc))
    {
        *ppvObject = (IAMWavFileSrc *)this;
    }
    else if (IsEqualIID(riid, IID_IAMSound))
    {
        *ppvObject = (IAMSound *)this;
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

STDMETHODIMP CAMWavFileSrc::GetFormat(LPWAVEFORMATEX pFormat, DWORD cbSize)
{
    assert(pFormat != NULL);

    if (cbSize > sizeof(WAVEFORMATEX))
        cbSize = sizeof(WAVEFORMATEX);

    memcpy(pFormat, m_wfxDest, cbSize);

    return S_OK;
}

STDMETHODIMP_(DWORD) CAMWavFileSrc::GetSamples()
{
    return m_Samples;
}

STDMETHODIMP CAMWavFileSrc::GetAlignment(LPDWORD lpdwLeftAlign, LPDWORD lpdwRightAlign)
{
    if (lpdwLeftAlign == NULL || lpdwRightAlign == NULL)
    {
        return E_FAIL;
    }
    else
    {
        *lpdwLeftAlign = 0;
        *lpdwRightAlign = 0;
        return S_OK;
    }
}

STDMETHODIMP CAMWavFileSrc::GetSampleData(LPBYTE lpBuffer, DWORD dwPCMStartSample, LPDWORD lpdwPCMSamples,
                                          LPREQUESTPARAM lpRequestParams)
{
    HRESULT hr = S_OK;
    DWORD dwOrgSamplesToRead = 0;
    DWORD dwPCMPositionBytes = 0;
    DWORD dwSampleBytes = 0;
    DWORD dwPCMSamples = 0;

    assert(lpBuffer != NULL);
    assert(lpdwPCMSamples != NULL);

    // Source must be active
    if (m_ActiveCount < 1)
    {
        hr = E_FAIL;
    }

    if (hr == S_OK)
    {
        // Validate the number of samples to read
        dwOrgSamplesToRead = *lpdwPCMSamples;
        if (m_Samples < dwPCMStartSample || m_Samples == dwPCMStartSample)
        {
            hr = E_FAIL;
        }
    }

    if (hr == S_OK)
    {
        // Check if we need to convert the audio to PCM

        if (m_StreamHandle == NULL)
        {
            // Raw PCM audio
            dwPCMPositionBytes = SamplesToBytes(m_wfxDest, dwPCMStartSample);

            // Seek to the start of the samples
            LARGE_INTEGER seekPos = {0};
            seekPos.LowPart = m_DataStart + dwPCMPositionBytes;

            hr = m_SourceStream->Seek(seekPos, STREAM_SEEK_SET, NULL);

            if (hr == S_OK)
            {
                // Calculate the size of the raw sample data
                dwSampleBytes = SamplesToBytes(m_wfxDest, *lpdwPCMSamples);
                if (m_DestDataSize <= dwSampleBytes + dwPCMPositionBytes &&
                    dwSampleBytes + dwPCMPositionBytes != m_DestDataSize)
                {

                    dwSampleBytes = m_DestDataSize - dwPCMPositionBytes;
                }

                // Read the sample data into the caller's buffer
                hr = m_SourceStream->Read(lpBuffer, dwSampleBytes, &dwSampleBytes);
                if (SUCCEEDED(hr))
                {
                    if (dwSampleBytes == 0xFFFFFFFF)
                    {
                        *lpdwPCMSamples = 0;
                    }
                    else
                    {
                        // Return number of samples we actually read
                        *lpdwPCMSamples = BytesToSamples(m_wfxDest, dwSampleBytes);
                        if ((*lpdwPCMSamples != dwOrgSamplesToRead) ||
                            (m_Samples <= *lpdwPCMSamples + dwPCMStartSample))
                        {
                            hr = S_ENDOFSOUND;
                        }
                    }
                }
            }
        }
        else
        {
            DWORD dwACMBytesToRead = 0;
            DWORD dwACMBytesRead = 0;
            DWORD dwACMPositionBytes = 0;
            DWORD nACMReads = 0;
            DWORD nTotalPCMBytes = 0;
            DWORD dwPCMBytesLeft = 0;
            DWORD dwPCMBytesRead;

            assert(m_DstBuffer != NULL);
            assert(m_SrcBuffer != NULL);

            dwPCMPositionBytes = SamplesToBytes(m_wfxDest, dwPCMStartSample);
            acmStreamSize(m_StreamHandle, dwPCMPositionBytes, &dwACMPositionBytes, 1);

            if (dwACMPositionBytes != 0)
            {
                if (dwACMPositionBytes >= m_DataSize)
                {
                    return S_ENDOFSOUND;
                }
                dwACMPositionBytes = dwACMPositionBytes - dwACMPositionBytes % m_cbSrcBuffer;
            }

            LARGE_INTEGER pos = {0};
            pos.LowPart = m_DataStart + dwACMPositionBytes;

            hr = m_SourceStream->Seek(pos, STREAM_SEEK_SET, NULL);

            if (SUCCEEDED(hr))
            {
                dwPCMBytesLeft = SamplesToBytes(m_wfxDest, *lpdwPCMSamples);
                *lpdwPCMSamples = 0;
                while (dwPCMBytesLeft != 0)
                {
                    if (m_cbSrcBufferUsed == dwACMPositionBytes)
                    {
                        dwACMBytesRead = m_ACMBytesRead;
                        pos.LowPart = m_DataStart + dwACMPositionBytes + dwACMBytesRead;
                        pos.HighPart = 0;

                        hr = m_SourceStream->Seek(pos, STREAM_SEEK_SET, NULL);
                        if (FAILED(hr))
                        {
                            return hr;
                        }
                    }
                    else
                    {
                        if ((m_cbSrcBuffer + dwACMPositionBytes) <= m_DataSize)
                        {
                            dwACMBytesToRead = m_cbSrcBuffer;
                        }
                        else
                        {
                            assert(m_DataSize >= dwACMPositionBytes);
                            dwACMBytesToRead = m_DataSize - dwACMPositionBytes;
                            if (dwACMBytesToRead == 0)
                            {
                                return S_ENDOFSOUND;
                            }
                        }

                        hr = m_SourceStream->Read(m_SrcBuffer, dwACMBytesToRead, &dwACMBytesRead);
                        if (FAILED(hr))
                        {
                            return hr;
                        }

                        if (dwACMBytesRead == 0)
                        {
                            if (*lpdwPCMSamples != 0)
                            {
                                return S_ENDOFSOUND;
                            }
                            return E_FAIL;
                        }

                        m_StreamHeader.cbSrcLength = dwACMBytesRead;
                        m_StreamHeader.cbSrcLengthUsed = 0;
                        m_StreamHeader.cbDstLength = m_cbDstBuffer;
                        m_StreamHeader.cbDstLengthUsed = 0;

                        DWORD fdwFlags = (m_cbSrcBuffer == dwACMBytesRead) ? ACM_STREAMCONVERTF_BLOCKALIGN : 0;
                        MMRESULT mmres = acmStreamConvert(m_StreamHandle, &m_StreamHeader, fdwFlags);

                        if (mmres != MMSYSERR_NOERROR)
                        {
                            return E_FAIL;
                        }

                        if (m_cbDestBufferUsed == 0)
                        {
                            assert(m_StreamHeader.cbSrcLengthUsed == m_StreamHeader.cbSrcLength);
                            m_cbDestBufferUsed = m_StreamHeader.cbDstLengthUsed;
                        }
                    }

                    if (dwPCMPositionBytes == 0)
                    {
                        dwPCMBytesRead = m_StreamHeader.cbDstLengthUsed;
                        if (dwPCMBytesLeft <= dwPCMBytesRead)
                        {
                            dwPCMBytesRead = dwPCMBytesLeft;
                        }
                        dwPCMSamples = BytesToSamples(m_wfxDest, dwPCMBytesRead);
                        memcpy(lpBuffer, m_DstBuffer, dwPCMBytesRead);
                    }
                    else
                    {
                        DWORD dwPCMBlockOffset = dwPCMPositionBytes % m_cbDestBufferUsed;
                        if (dwPCMBlockOffset >= m_StreamHeader.cbDstLengthUsed)
                        {
                            return S_ENDOFSOUND;
                        }

                        dwPCMBytesRead = m_StreamHeader.cbDstLengthUsed - dwPCMBlockOffset;

                        if (dwPCMBytesLeft <= dwPCMBytesRead)
                        {
                            dwPCMBytesRead = dwPCMBytesLeft;
                        }
                        dwPCMSamples = BytesToSamples(this->m_wfxDest, dwPCMBytesRead);
                        memcpy(lpBuffer, m_DstBuffer + dwPCMBlockOffset, dwPCMBytesRead);
                        dwPCMPositionBytes = 0;
                    }

                    m_cbSrcBufferUsed = dwACMPositionBytes;
                    m_ACMBytesRead = dwACMBytesRead;
                    dwPCMBytesLeft = dwPCMBytesLeft - dwPCMBytesRead;
                    lpBuffer = lpBuffer + dwPCMBytesRead;
                    dwACMPositionBytes = dwACMPositionBytes + dwACMBytesRead;
                    *lpdwPCMSamples = *lpdwPCMSamples + BytesToSamples(this->m_wfxDest, dwPCMBytesRead);
                }
            }
        }
    }

    return hr;
}

STDMETHODIMP CAMWavFileSrc::SetCacheSize(DWORD dwCacheSize)
{
    return S_OK;
}

STDMETHODIMP CAMWavFileSrc::SetMode(BOOL fActive, BOOL fRecurse)
{
    UNREFERENCED_PARAMETER(fRecurse);

    if (fActive)
    {
        m_ActiveCount++;
    }
    else
    {
        m_ActiveCount--;
    }

    assert(m_ActiveCount >= 0);

    return S_OK;
}

CAMWavFileSrc::CAMWavFileSrc()
{
    m_StreamHeader.cbStruct = sizeof(m_StreamHeader);
}

CAMWavFileSrc::~CAMWavFileSrc()
{
    DPRINTF(1, "Destructing WavFileSrc object");

    assert(m_ActiveCount == 0);

    if (m_SourceStream != NULL)
    {
        m_SourceStream->Release();
    }

    if (m_wfxSource != NULL)
    {
        delete[] m_wfxSource;
    }

    if (m_wfxDest != NULL)
    {
        delete[] m_wfxDest;
    }

    FreeACMStream();
}
