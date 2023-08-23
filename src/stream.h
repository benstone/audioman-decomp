/**
 * File and memory streams
 **/
#pragma once
#include <Windows.h>

class CFileStream : public IStream
{
  public:
    CFileStream(char *szFileName, ULONG offset, ULONG flags);

    virtual ~CFileStream();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // ISequentialStream methods
    STDMETHOD(Read)(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD(Write)(const void *pv, ULONG cb, ULONG *pcbWritten);

    // IStream methods
    STDMETHOD(Seek)(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHOD(SetSize)(ULARGE_INTEGER libNewSize);
    STDMETHOD(CopyTo)(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHOD(Commit)(DWORD grfCommitFlags);
    STDMETHOD(Revert)(void);
    STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHOD(Clone)(IStream **ppstm);

  private:
    ULONG m_RefCnt = 0;
    char m_FileName[256];
    HFILE m_FileHandle = 0;
    ULONG m_Offset = 0;
    ULONG m_FileMode = 0;
};

class CMemoryStream : public IStream
{
  public:
    virtual ~CMemoryStream();

    CMemoryStream(BYTE *pInputBytes, ULONG dwLength, ULONG dwFlags);
    CMemoryStream(IStream *stream, ULONG size);

    // copy constructor
    CMemoryStream(const CMemoryStream &old);

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // ISequentialStream methods
    STDMETHOD(Read)(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD(Write)(const void *pv, ULONG cb, ULONG *pcbWritten);

    // IStream methods
    STDMETHOD(Seek)(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHOD(SetSize)(ULARGE_INTEGER libNewSize);
    STDMETHOD(CopyTo)(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHOD(Commit)(DWORD grfCommitFlags);
    STDMETHOD(Revert)(void);
    STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHOD(Clone)(IStream **ppstm);

  private:
    ULONG m_RefCnt = 0;

    ULONG m_BufferSize = 0;
    ULONG m_Pos = 0;
    ULONG m_Flags = 0;
    ULONG m_Unknown = 0;
    BOOL m_IsAllocated = FALSE;
    BYTE *m_Buffer = NULL;
};