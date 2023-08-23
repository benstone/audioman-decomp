#include "riff.h"

DWORD SwapDWORD(DWORD dwSwap)
{
    // FUTURE: swap bytes on big endian
    return dwSwap;
}

WORD SwapWORD(WORD wSwap)
{
    // FUTURE: swap bytes on big endian
    return wSwap;
}

CRIFF::CRIFF(IStream *pStream)
{
    m_pStream = pStream;
    m_pStream->AddRef();
}

CRIFF::~CRIFF()
{
    if (m_pStream != NULL)
        m_pStream->Release();
}

IStream *CRIFF::GetStream()
{
    return m_pStream;
}

HRESULT CRIFF::ReadLongData(ULONG *pLongData)
{
    HRESULT hr = m_pStream->Read((void *)pLongData, sizeof(ULONG), NULL);
    if (hr == S_OK)
    {
        *pLongData = SwapDWORD(*pLongData);
    }
    return hr;
}

HRESULT CRIFF::ReadShortData(USHORT *pShortData)
{
    HRESULT hr = m_pStream->Read((void *)pShortData, sizeof(USHORT), NULL);
    if (hr == S_OK)
    {
        *pShortData = SwapWORD(*pShortData);
    }
    return hr;
}

HRESULT CRIFF::ReadRIFFTag(RIFFTag *plTag)
{
    return m_pStream->Read(plTag, 4, NULL);
}

HRESULT CRIFF::FindNextFormChunk(RIFFTag *plChunkTag, ULONG *plChunkLen)
{
    HRESULT hr = S_OK;
    RIFFTag ulChunkTag;
    ULONG ulChunkLen;

    while (hr == S_OK)
    {
        // Get the current chunk tag
        hr = ReadRIFFTag(&ulChunkTag);
        if (hr != S_OK)
        {
            break;
        }

        if (ulChunkTag == ' tmf' || ulChunkTag == 'atad' || ulChunkTag == 'tcaf')
        {
            hr = ReadLongData(&ulChunkLen);
            if (hr != S_OK)
            {
                break;
            }

            *plChunkTag = ulChunkTag;
            *plChunkLen = ulChunkLen;
            break;
        }

        // Read chunk size
        hr = ReadLongData(&ulChunkLen);
        if (hr != S_OK)
        {
            break;
        }

        // Move to next chunk
        LARGE_INTEGER pos;
        pos.LowPart = ulChunkLen;
        pos.HighPart = 0;
        hr = m_pStream->Seek(pos, STREAM_SEEK_CUR, NULL);
    }

    return hr;
}
