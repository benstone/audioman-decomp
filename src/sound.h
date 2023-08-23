#pragma once
#include "audiomaninternal.h"

#include <mmreg.h>
#include <MSAcm.h>

class CAMWavFileSrc : public IAMSound, public IAMWavFileSrc
{
  public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IAMSound methods
    STDMETHOD(GetFormat)(LPWAVEFORMATEX pFormat, DWORD cbSize);
    STDMETHOD_(DWORD, GetSamples)();
    STDMETHOD(GetAlignment)(LPDWORD lpdwLeftAlign, LPDWORD lpdwRightAlign);
    STDMETHOD(GetSampleData)(LPBYTE lpBuffer, DWORD dwPCMStartSample, LPDWORD lpdwPCMSamples, LPREQUESTPARAM lpRequestParams);
    STDMETHOD(SetCacheSize)(DWORD dwCacheSize);
    STDMETHOD(SetMode)(BOOL fActive, BOOL fRecurse);

    // IAMWavFileSrc methods
    STDMETHOD(InitFromStream)(IStream *pStream, BOOL fSpooled);
    STDMETHOD(InitFromMemory)(char *lpMemory, DWORD dwLength);
    STDMETHOD(InitFromFile)(char *pAbsFilePath, DWORD dwOffset, BOOL fSpooled);

    CAMWavFileSrc();
    virtual ~CAMWavFileSrc();

  private:
    ULONG m_RefCnt = 0;
    LONG m_ActiveCount = 0;
    DWORD m_MemoryStreamSize = 0;
    ULONG m_DataStart = 0;
    DWORD m_DataSize = 0;
    DWORD m_DestDataSize = 0;
    ULONG m_Samples = 0;

    // Source and destination wave formats
    LPWAVEFORMATEX m_wfxSource = NULL;
    LPWAVEFORMATEX m_wfxDest = NULL;

    // Conversion stream handle
    HACMSTREAM m_StreamHandle = NULL;

    // Conversion stream header
    ACMSTREAMHEADER m_StreamHeader = {0};

    // Destination buffer
    BYTE *m_DstBuffer = NULL;
    DWORD m_cbDstBuffer = 0;
    DWORD m_cbDestBufferUsed = 0;

    // Source buffer
    BYTE *m_SrcBuffer = NULL;
    DWORD m_cbSrcBuffer = 0;
    DWORD m_cbSrcBufferUsed = 0xFFFFFFFF;
    DWORD m_ACMBytesRead;

    DWORD m_dwOutputBytes = 0;

    IStream *m_SourceStream = NULL;
    ULONG m_Flags = 0;
    DWORD m_SourceFormat = 0;

    HRESULT AllocACMStream();
    void FreeACMStream();

    HRESULT _FInit(IStream *pInputStream, BOOL fSpooled);
    HRESULT _ReadHeader(IStream *pInputStream);
};