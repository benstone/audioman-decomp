/**
 * RIFF writer stream
 **/
#pragma once
#include "audiomaninternal.h"

#define CREATE_CHUNK_RIFF 0x20
#define CREATE_CHUNK_LIST 0x40

// RIFF stream interface
#define _IID_IAMRIFFStream A0434E42 - 9573 - 11CE-B61B - 00AA006EBBE5
DEFINE_GUID(IID_IAMRIFFStream, 0xA0434E42L, 0x9573, 0x11CE, 0xB6, 0x1B, 0x00, 0xAA, 0x00, 0x6E, 0xBB, 0xE5);

DECLARE_INTERFACE_(IAMRIFFStream, IUnknown)
{
    // IUnknown methods

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IAMRIFFStream methods

    STDMETHOD(Descend)(THIS_ LPMMCKINFO lpck, LPMMCKINFO lpckParent) PURE;
    STDMETHOD(Ascend)(THIS_ LPMMCKINFO lpck) PURE;
    STDMETHOD(CreateChunk)(THIS_ LPMMCKINFO lpck, UINT wFlags) PURE;
};

class CAMRIFFStream : public IAMRIFFStream
{
  public:
    CAMRIFFStream(IStream *stream);

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IAMRIFFStream methods
    STDMETHOD(Descend)(THIS_ LPMMCKINFO lpck, LPMMCKINFO lpckParent);
    STDMETHOD(Ascend)(THIS_ LPMMCKINFO lpck);
    STDMETHOD(CreateChunk)(THIS_ LPMMCKINFO lpck, UINT wFlags);

    virtual ~CAMRIFFStream();

  private:
    UINT m_RefCnt = 0;
    IStream *m_Stream = NULL;

    LONG MySeek(DWORD dwPos, DWORD dwOrigin);
    LONG MyRead(BYTE *data, DWORD cb);
    LONG MyWrite(const BYTE *data, DWORD cb);
};

HRESULT AMRIFFStream_CreateFromIStream(IStream *stream, CAMRIFFStream **riffStream);
