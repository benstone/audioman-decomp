#include "audiomaninternal.h"
#include "dpf.h"
#include "stream.h"
#include "utils.h"
#include "riffstrm.h"
#include "riff.h"
#include <cassert>

// Chunk size used when writing WAVE files
const DWORD kWriteBufferSizeMs = 60;

HRESULT AMCreate(REFCLSID rclsid, REFIID riid, void **ppvObject, LPSOUND *ppSound)
{
    HRESULT hr = AM_CreateInstance(rclsid, NULL, riid, ppvObject);
    if (FAILED(hr))
    {
        DPRINTF(0, "AMCreate:  AM_CreateInstance Failed!!");
    }
    else
    {
        hr = ((IUnknown *)(*ppvObject))->QueryInterface(IID_IAMSound, (void**)(ppSound));
        if (FAILED(hr))
        {
            DPRINTF(0, "AMCreate:  Specified object doesn't support specified interface!");
            ((IUnknown *)(*ppvObject))->Release();
        }
    }

    return hr;
}

void AMFinish(HRESULT hr, LPSOUND *ppSound, IUnknown *pUnk, LPSOUND pSnd)
{
    if (FAILED(hr))
    {
        DPRINTF(0, "AMFinish:  Sound Src Initialization Failed");
        pSnd->Release();
    }
    else
    {
        *ppSound = pSnd;
    }
    pUnk->Release();
}

HRESULT AMFinishCache(LPSOUND *ppSound, IUnknown *pUnk, LPSOUND pSnd, BOOL fSpooled, LPCACHECONFIG lpCacheConfig)
{
    HRESULT hr = S_OK;

    if ((fSpooled == FALSE) || (lpCacheConfig == NULL))
    {
        *ppSound = pSnd;
    }
    else
    {
        // Cache the sound
        LPSOUND pCache = NULL;
        hr = AllocCacheFilter(&pCache, pSnd, lpCacheConfig);
        if (SUCCEEDED(hr))
        {
            *ppSound = pCache;
        }
        else
        {
            DPRINTF(0, "AMFinishCache:  AllocCacheFilter Failed!");
        }

        // Cache filter now owns the sound
        pSnd->Release();
    }

    pUnk->Release();
    return hr;
}

STDAPI_(LPMIXER) GetAudioManMixer(void)
{
    HRESULT hr = S_OK;
    LPMIXER pMixer = NULL;

    hr = AM_CreateInstance(CLSID_AMMixer, NULL, IID_IAMMixer, (void **)&pMixer);
    if (FAILED(hr) || pMixer == NULL)
    {
        DPRINTF(0, "GetAudioManMixer:  AM_CreateInstance failed!!");
        pMixer = NULL;
    }

    return pMixer;
}

STDAPI SoundToFileAsWave(LPSOUND pSound, char FAR *pAbsFilePath)
{
    HRESULT hr = S_OK;
    DWORD pcbFileSize;
    if (pAbsFilePath == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CFileStream *fileStream = new CFileStream(pAbsFilePath, 0, 1);
        if (fileStream == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            fileStream->AddRef();
            pSound->AddRef();

            hr = SoundToStreamAsWave(pSound, fileStream, &pcbFileSize);

            pSound->Release();
            fileStream->Release();
        }
    }

    return hr;
}

STDAPI SoundToStreamAsWave(LPSOUND pSound, LPSTREAM pStream, LPDWORD pcbFileSize)
{
    HRESULT hr = S_OK;
    DWORD dwSamplesRead = 0;
    DWORD dwSamplesToRead = 0;
    DWORD dwCurrentSample = 0;
    BYTE *pBuffer = NULL;
    DWORD cbBuffer = 0;
    ULONG dwBytesRead = 0;
    WAVEFORMATEX wfx = {0};
    BOOL fActivated = FALSE;
    BOOL fDone = FALSE;
    CAMRIFFStream *pRIFFStream = NULL;
    MMCKINFO ckWAVE = {0};
    MMCKINFO ck = {0};
    ULONG cb = 0;

    if (pSound == NULL || pcbFileSize == NULL)
    {
        DPRINTF(0, "SoundToStreamAsWave::Invalid pSound");
        hr = E_INVALIDARG;
    }

    if (hr == S_OK)
    {
        dwSamplesRead = pSound->GetSamples();
        if (dwSamplesRead == INFINITE_SAMPLES)
        {
            hr = E_FAIL;
        }
    }

    // Activate the sound
    if (SUCCEEDED(hr))
    {
        hr = pSound->SetMode(TRUE, TRUE);
    }

    // Get the sound format
    if (SUCCEEDED(hr))
    {
        fActivated = TRUE;
        hr = pSound->GetFormat(&wfx, sizeof(wfx));
    }

    // Check if we need to write the file or just calculate the file size
    if (SUCCEEDED(hr))
    {
        if (pStream == NULL)
        {
            // No stream provided - just calculate the final file size
            *pcbFileSize = SamplesToBytes(&wfx, pSound->GetSamples()) + 0x58;
        }
        else
        {
            // Allocate temporary buffer
            cbBuffer = MillisecToBytes(&wfx, kWriteBufferSizeMs);
            pBuffer = new byte[cbBuffer];
            if (pBuffer == NULL)
            {
                hr = E_OUTOFMEMORY;
            }

            // Create the RIFF stream
            if (SUCCEEDED(hr))
            {
                hr = AMRIFFStream_CreateFromIStream(pStream, &pRIFFStream);
            }

            // Write the RIFF header
            if (SUCCEEDED(hr))
            {
                ckWAVE.cksize = 0;
                ckWAVE.fccType = kRIFFTagWAVE;

                hr = pRIFFStream->CreateChunk(&ckWAVE, CREATE_CHUNK_RIFF);
            }

            // Write the fmt chunk header
            if (SUCCEEDED(hr))
            {
                ck.cksize = 0;
                ck.ckid = kRIFFTagFormat;

                hr = pRIFFStream->CreateChunk(&ck, 0);
            }

            // Write the fmt chunk data
            if (SUCCEEDED(hr))
            {
                hr = pStream->Write(&wfx, sizeof(wfx), &cb);

                if (SUCCEEDED(hr) && (cb != sizeof(wfx)))
                {
                    hr = E_FAIL;
                }
            }

            // Finish the fmt chunk
            if (SUCCEEDED(hr))
            {
                hr = pRIFFStream->Ascend(&ck);
            }

            // Write the data chunk header
            if (SUCCEEDED(hr))
            {
                ck.cksize = 0;
                ck.ckid = kRIFFTagData;
                hr = pRIFFStream->CreateChunk(&ck, 0);
            }

            // Write the wave data
            if (SUCCEEDED(hr))
            {
                dwSamplesToRead = BytesToSamples(&wfx, cbBuffer);
                while (fDone == FALSE && dwSamplesToRead != 0)
                {
                    dwSamplesRead = dwSamplesToRead;
                    hr = pSound->GetSampleData(pBuffer, dwCurrentSample, &dwSamplesRead, NULL);

                    // Check if we have reached the end of the sound
                    if (FAILED(hr) || (hr == S_ENDOFSOUND))
                    {
                        fDone = TRUE;
                    }

                    if (SUCCEEDED(hr))
                    {
                        // Write the chunk to disk
                        dwCurrentSample += dwSamplesRead;
                        dwBytesRead = SamplesToBytes(&wfx, dwSamplesRead);

                        hr = pStream->Write(pBuffer, dwBytesRead, &cb);
                        if (cb != dwBytesRead)
                        {
                            hr = E_FAIL;
                        }
                        if (FAILED(hr))
                        {
                            fDone = TRUE;
                        }
                    }
                }

                // Finish the data chunk
                if (SUCCEEDED(hr))
                {
                    hr = pRIFFStream->Ascend(&ck);
                }

                // Finish the file
                if (SUCCEEDED(hr))
                {
                    hr = pRIFFStream->Ascend(&ckWAVE);
                    if (hr != S_OK)
                    {
                        hr = E_FAIL;
                    }
                }
            }
        }
    }

    // Cleanup
    if (fActivated)
        pSound->SetMode(FALSE, TRUE);
    if (pBuffer != NULL)
        delete[] pBuffer;
    if (pRIFFStream != NULL)
        pRIFFStream->Release();

    return hr;
}