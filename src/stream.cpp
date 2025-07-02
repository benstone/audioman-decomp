#include <cassert>
#include <cstdio>
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
    HRESULT hr = S_OK;

    if (pv == NULL)
    {
        hr = E_POINTER;
    }
    else
    {
        DWORD cbRead = 0;
        BOOL fRet = ReadFile(m_FileHandle, pv, cb, &cbRead, NULL);
        if (fRet)
        {
            if (pcbRead != NULL)
            {
                *pcbRead = cbRead;
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

STDMETHODIMP CFileStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
    HRESULT hr = S_OK;
    DWORD cbWritten = 0;
    BOOL fRet;

    fRet = WriteFile(m_FileHandle, pv, cb, &cbWritten, NULL);
    *pcbWritten = cbWritten;

    if (!fRet)
    {
        hr = E_FAIL;
    }

    return hr;
}

STDMETHODIMP CFileStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    HRESULT hr = E_FAIL;
    DWORD dwMoveMethod = FILE_BEGIN;

    switch (dwOrigin)
    {
    case STREAM_SEEK_SET:
        dwMoveMethod = FILE_BEGIN;
        break;
    case STREAM_SEEK_CUR:
        dwMoveMethod = FILE_CURRENT;
        break;
    case STREAM_SEEK_END:
        dwMoveMethod = FILE_END;
        break;
    default:
        assert(FALSE);
        break;
    }

    LARGE_INTEGER dlibNewPos;
    if (SetFilePointerEx(m_FileHandle, dlibMove, &dlibNewPos, dwMoveMethod))
    {
        if (plibNewPosition != NULL)
        {
            plibNewPosition->HighPart = dlibNewPos.HighPart;
            plibNewPosition->LowPart = dlibNewPos.LowPart;
        }

        hr = S_OK;
    }

    return hr;
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
    HRESULT hr;
    wchar_t wszFileName[MAX_PATH] = {0};

    m_FileName[0] = '\x0';

    if (MultiByteToWideChar(CP_ACP, 0, szFileName, strlen(szFileName), wszFileName, MAX_PATH - 1) != 0)
    {
        hr = Open(wszFileName, offset, flags);
        assert(SUCCEEDED(hr));
    }
}

CFileStream::CFileStream(wchar_t *wszFileName, ULONG offset, ULONG flags)
{
    HRESULT hr;

    m_FileName[0] = '\x0';

    hr = Open(wszFileName, offset, flags);
    assert(SUCCEEDED(hr));
}

HRESULT CFileStream::Open(wchar_t *wszFileName, ULONG offset, ULONG flags)
{
    HRESULT hr = S_OK;
    m_FileMode = flags;

    if (flags & 2)
    {
        m_FileHandle =
            CreateFileW(wszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (m_FileHandle != INVALID_HANDLE_VALUE)
        {
            offset = SetFilePointer(m_FileHandle, offset, 0, FILE_BEGIN);
            m_Offset = offset;
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else if (flags & 1)
    {
        m_FileHandle = CreateFileW(wszFileName, GENERIC_ALL, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (m_FileHandle == INVALID_HANDLE_VALUE)
        {
            hr = E_FAIL;
        }
    }
    else
    {
        // invalid flags
        assert(false);
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        wcscpy_s(m_FileName, wszFileName);
    }

    return hr;
}

CFileStream::~CFileStream()
{
    if (m_FileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_FileHandle);
        m_FileHandle = INVALID_HANDLE_VALUE;
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
