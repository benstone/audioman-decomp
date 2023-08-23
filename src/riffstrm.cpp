#include <cassert>
#include "riffstrm.h"
#include "dpf.h"
#include "riff.h"
#include "todo.h"

const char kPadding[] = {0};

CAMRIFFStream::CAMRIFFStream(IStream *stream)
{
    m_Stream = stream;
    if (m_Stream != NULL)
        m_Stream->AddRef();
}

STDMETHODIMP CAMRIFFStream::QueryInterface(REFIID riid, void **ppvObject)
{
    assert(ppvObject != NULL);

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAMRIFFStream))
    {
        *ppvObject = this;
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) CAMRIFFStream::AddRef()
{
    return ++m_RefCnt;
}

STDMETHODIMP_(ULONG) CAMRIFFStream::Release()
{
    ULONG RefCnt = --m_RefCnt;

    if (RefCnt == 0)
    {
        delete this;
    }

    return RefCnt;
}

STDMETHODIMP CAMRIFFStream::Descend(LPMMCKINFO lpck, LPMMCKINFO lpckParent)
{
    // This function doesn't seem to be called
    TODO_NOT_IMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CAMRIFFStream::Ascend(LPMMCKINFO lpck)
{
    HRESULT hr = S_OK;
    LONG lOffset = 0;

    if (lpck->dwFlags & MMIO_DIRTY)
    {
        lOffset = MySeek(0, STREAM_SEEK_CUR);
        if (lOffset == -1)
        {
            return MMIOERR_CANNOTSEEK;
        }

        lOffset = lOffset - lpck->dwDataOffset;
        if (lOffset < 0)
        {
            return MMIOERR_CANNOTWRITE;
        }

        // Align start of chunk
        if ((lOffset & 1) != 0)
        {
            if (MyWrite((BYTE *)&kPadding, 1) != 1)
            {
                return MMIOERR_CANNOTWRITE;
            }
        }

        // Return if the chunk is empty
        if (lpck->cksize == lOffset)
        {
            return S_OK;
        }
        lpck->cksize = lOffset;

        // Seek to location of chunk size
        if (MySeek(lpck->dwDataOffset - 4, STREAM_SEEK_SET) == -1)
        {
            return MMIOERR_CANNOTSEEK;
        }

        // Write chunk size
        if (MyWrite((BYTE *)(&lpck->cksize), 4) != 4)
        {
            return MMIOERR_CANNOTWRITE;
        }
    }

    if (MySeek((lpck->cksize & 1) + lpck->cksize + lpck->dwDataOffset, STREAM_SEEK_SET) == -1)
    {
        hr = MMIOERR_CANNOTSEEK;
    }
    return hr;
}

STDMETHODIMP CAMRIFFStream::CreateChunk(LPMMCKINFO lpck, UINT wFlags)
{
    HRESULT hr = S_OK;
    LONG lOffset = 0;
    INT iBytes = 0;

    // Get current position
    lOffset = MySeek(0, STREAM_SEEK_CUR);
    if (lOffset == -1)
    {
        hr = MMIOERR_CANNOTSEEK;
    }
    else
    {
        lpck->dwDataOffset = lOffset + 8;
        if (wFlags & CREATE_CHUNK_RIFF)
        {
            // RIFF chunk
            lpck->ckid = kRIFFTagRIFF;
            iBytes = 0xC;
        }
        else if (wFlags & CREATE_CHUNK_LIST)
        {
            // LIST chunk
            lpck->ckid = kRIFFTagList;
            iBytes = 0xC;
        }
        else
        {
            iBytes = 8;
        }

        // Write chunk header
        if (MyWrite((BYTE *)lpck, iBytes) == iBytes)
        {
            lpck->dwFlags = MMIO_DIRTY;
            hr = S_OK;
        }
        else
        {
            hr = MMIOERR_CANNOTWRITE;
        }
    }

    return hr;
}

CAMRIFFStream::~CAMRIFFStream()
{
    DPRINTF(1, "Destructing RIFFStream");
    if (m_Stream != NULL)
        m_Stream->Release();
}

LONG CAMRIFFStream::MySeek(DWORD dwPos, DWORD dwOrigin)
{
    LARGE_INTEGER dlibMove = {0};
    ULARGE_INTEGER libNewPosition = {0};
    dlibMove.LowPart = dwPos;

    if (FAILED(m_Stream->Seek(dlibMove, dwOrigin, &libNewPosition)))
    {
        libNewPosition.LowPart = -1;
    }

    return libNewPosition.LowPart;
}

LONG CAMRIFFStream::MyRead(BYTE *data, DWORD cb)
{
    LONG cbRead = 0;
    if (FAILED(m_Stream->Read(data, cb, (ULONG *)&cbRead)))
    {
        cbRead = -1;
    }
    return cbRead;
}

LONG CAMRIFFStream::MyWrite(const BYTE *data, DWORD cb)
{
    LONG cbWritten = 0;
    if (FAILED(m_Stream->Write(data, cb, (ULONG *)&cbWritten)))
    {
        cbWritten = -1;
    }
    return cbWritten;
}

HRESULT AMRIFFStream_CreateFromIStream(IStream *stream, CAMRIFFStream **riffStream)
{
    *riffStream = new CAMRIFFStream(stream);
    if (*riffStream == NULL)
    {
        return E_OUTOFMEMORY;
    }
    else
    {
        (*riffStream)->AddRef();
        return S_OK;
    }
}
