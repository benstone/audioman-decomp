#include <cassert>
#include "stream.h"

STDMETHODIMP_(ULONG) CFileStream::AddRef()
{
    return ++m_RefCnt;
}

STDMETHODIMP_(ULONG) CFileStream::Release()
{
    ULONG RefCnt = --m_RefCnt;

    if (RefCnt == 0)
    {
        delete this;
    }

    return RefCnt;
}

STDMETHODIMP CFileStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
    if (pv == NULL)
    {
        return E_POINTER;
    }
    else
    {
        long cbRead = _hread(m_FileHandle, pv, cb);
        if (pcbRead != NULL)
        {
            *pcbRead = cbRead;
        }
        return S_OK;
    }
}

STDMETHODIMP CFileStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
    HRESULT hr = S_OK;

    *pcbWritten = _hwrite(m_FileHandle, (LPCCH)pv, cb);
    if (*pcbWritten == -1)
    {
        hr = E_FAIL;
    }
    return hr;
}

STDMETHODIMP CFileStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    if (dwOrigin == STREAM_SEEK_SET)
    {
        _llseek(m_FileHandle, m_Offset + dlibMove.LowPart, 0);
    }
    else if (dwOrigin == STREAM_SEEK_CUR)
    {
        _llseek(m_FileHandle, dlibMove.LowPart, 1);
    }
    else if (dwOrigin == STREAM_SEEK_END)
    {
        _llseek(m_FileHandle, dlibMove.LowPart, 2);
    }
    else
    {
        DebugBreak();
    }

    if (plibNewPosition != NULL)
    {
        LONG pos = _llseek(m_FileHandle, 0, 1);
        plibNewPosition->LowPart = pos;
        plibNewPosition->HighPart = 0;
    }

    return S_OK;
}

STDMETHODIMP CFileStream::SetSize(ULARGE_INTEGER libNewSize)
{
    // not implemented
    return E_NOTIMPL;
}

STDMETHODIMP CFileStream::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    // not implemented
    return E_NOTIMPL;
}

STDMETHODIMP CFileStream::Commit(DWORD grfCommitFlags)
{
    // not implemented
    return E_NOTIMPL;
}

STDMETHODIMP CFileStream::Revert(void)
{
    // not implemented
    return E_NOTIMPL;
}

STDMETHODIMP CFileStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    // not implemented
    return E_NOTIMPL;
}

STDMETHODIMP CFileStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    // not implemented
    return E_NOTIMPL;
}

STDMETHODIMP CFileStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    // not implemented
    return E_NOTIMPL;
}

STDMETHODIMP CFileStream::Clone(IStream **ppstm)
{
    HRESULT hr = S_OK;

    if (ppstm == NULL)
    {
        hr = E_POINTER;
    }
    else
    {
        *ppstm = new CFileStream(m_FileName, m_Offset, m_FileMode);
    }
    return hr;
}

CFileStream::CFileStream(char *szFileName, ULONG offset, ULONG flags)
{
    m_FileName[0] = '\x0';
    m_Offset = 0;
    m_FileMode = flags;

    if (flags & 2)
    {
        m_FileHandle = _lopen(szFileName, OF_SHARE_DENY_WRITE);
        if (m_FileHandle != 0)
        {
            if (_llseek(m_FileHandle, offset, 0) == 0)
            {
                strcpy(m_FileName, szFileName);
                m_Offset = offset;
            }
        }
    }
    else
    {
        if (flags & 1)
        {
            m_FileHandle = _lcreat(szFileName, 0);
            if (m_FileHandle != 0)
            {
                _lclose(m_FileHandle);
                m_FileHandle = _lopen(szFileName, OF_SHARE_DENY_WRITE | OF_WRITE);
            }
        }
    }
}

CFileStream::~CFileStream()
{
    if (m_FileHandle != 0)
    {
        _lclose(m_FileHandle);
    }
}

STDMETHODIMP CFileStream::QueryInterface(REFIID riid, void **ppvObject)
{
    assert(ppvObject != NULL);

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IStream))
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

STDMETHODIMP_(ULONG) CMemoryStream::AddRef()
{
    return ++m_RefCnt;
}

STDMETHODIMP_(ULONG) CMemoryStream::Release()
{
    ULONG RefCnt = --m_RefCnt;

    if (RefCnt == 0)
    {
        delete this;
    }

    return RefCnt;
}

STDMETHODIMP CMemoryStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
    if (pv == NULL || m_Buffer == NULL)
    {
        return E_POINTER;
    }

    if (m_Pos + cb > m_BufferSize)
    {
        cb = m_BufferSize - m_Pos;
    }

    memcpy(pv, m_Pos + m_Buffer, cb);

    m_Pos += cb;

    if (pcbRead != NULL)
    {
        *pcbRead = cb;
    }

    return S_OK;
}

STDMETHODIMP CMemoryStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
    if (pv == NULL || m_Buffer == NULL)
    {
        return E_POINTER;
    }

    if (m_Pos + cb > m_BufferSize)
    {
        cb = m_BufferSize - m_Pos;
    }

    memcpy(m_Pos + m_Buffer, pv, cb);

    m_Pos += cb;

    if (pcbWritten != NULL)
    {
        *pcbWritten = cb;
    }

    return S_OK;
}

STDMETHODIMP CMemoryStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    if (dwOrigin == STREAM_SEEK_SET)
    {
        m_Pos = 0;
    }
    else if (dwOrigin != STREAM_SEEK_CUR)
    {
        if (dwOrigin == STREAM_SEEK_END)
        {
            m_Pos = m_BufferSize;
        }
        else
        {
            DebugBreak();
        }
    }
    m_Pos = m_Pos + dlibMove.LowPart;
    if (plibNewPosition != NULL)
    {
        plibNewPosition->LowPart = this->m_Pos;
        plibNewPosition->HighPart = 0;
    }
    return S_OK;
}

STDMETHODIMP CMemoryStream::SetSize(ULARGE_INTEGER libNewSize)
{
    // not implemented
    return E_NOTIMPL;
}

STDMETHODIMP CMemoryStream::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead,
                                   ULARGE_INTEGER *pcbWritten)
{
    // not implemented
    return E_NOTIMPL;
}

STDMETHODIMP CMemoryStream::Commit(DWORD grfCommitFlags)
{
    // not implemented
    return E_NOTIMPL;
}

STDMETHODIMP CMemoryStream::Revert(void)
{
    // not implemented
    return E_NOTIMPL;
}

STDMETHODIMP CMemoryStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    // not implemented
    return E_NOTIMPL;
}

STDMETHODIMP CMemoryStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    // not implemented
    return E_NOTIMPL;
}

STDMETHODIMP CMemoryStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    // not implemented
    return E_NOTIMPL;
}

STDMETHODIMP CMemoryStream::Clone(IStream **ppstm)
{
    HRESULT hr = S_OK;
    if (ppstm == NULL)
    {
        hr = E_POINTER;
    }
    else
    {
        *ppstm = (IStream *)new CMemoryStream(*this);
    }
    return hr;
}

STDMETHODIMP CMemoryStream::QueryInterface(REFIID riid, void **ppvObject)
{
    assert(ppvObject != NULL);

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IStream))
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

CMemoryStream::~CMemoryStream()
{
    if (m_IsAllocated)
    {
        delete[] m_Buffer;
    }
    m_Buffer = NULL;
}

CMemoryStream::CMemoryStream(BYTE *pInputBytes, ULONG dwLength, ULONG dwFlags)
{
    m_IsAllocated = FALSE;
    m_Buffer = pInputBytes;
    m_BufferSize = dwLength;
    m_Flags = dwFlags;
}

CMemoryStream::CMemoryStream(IStream *stream, ULONG size)
{
    m_IsAllocated = TRUE;
    m_Buffer = new BYTE[size];
    if (m_Buffer != NULL)
    {
        HRESULT hr = stream->Read(m_Buffer, size, &m_BufferSize);
        if (FAILED(hr))
        {
            m_BufferSize = 0;
            delete[] m_Buffer;
            m_Buffer = NULL;
        }
    }
}

CMemoryStream::CMemoryStream(const CMemoryStream &old)
{
    m_IsAllocated = TRUE;
    m_BufferSize = old.m_BufferSize;
    m_Pos = old.m_Pos;
    m_Buffer = new BYTE[m_BufferSize];
    if (m_Buffer != NULL)
    {
        memcpy(m_Buffer, old.m_Buffer, m_BufferSize);
    }
}
