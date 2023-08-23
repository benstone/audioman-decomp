#pragma once
#include <Windows.h>

typedef DWORD RIFFTag;

const RIFFTag kRIFFTagRIFF = mmioFOURCC('R', 'I', 'F', 'F');
const RIFFTag kRIFFTagWAVE = mmioFOURCC('W', 'A', 'V', 'E');
const RIFFTag kRIFFTagFormat = mmioFOURCC('f', 'm', 't', ' ');
const RIFFTag kRIFFTagData = mmioFOURCC('d', 'a', 't', 'a');
const RIFFTag kRIFFTagList = mmioFOURCC('L', 'I', 'S', 'T');

/**
 * Swap endianness of a 16-bit value. A no-op on little-endian systems.
 * @param wSwap word to swap
 **/
WORD SwapWORD(WORD wSwap);

/**
 * Swap endianness of a 32-bit value. A no-op on little-endian systems.
 * @param dwSwap dword to swap
 **/
DWORD SwapDWORD(DWORD dwSwap);

// RIFF file parser
class CRIFF
{
  public:
    CRIFF(IStream *pStream);
    ~CRIFF();

    /**
     * Get underlying stream
     **/
    IStream *GetStream();

    /**
     * Read a 32-bit value
     **/
    HRESULT ReadLongData(ULONG *pLongData);

    /**
     * Read a 16-bit value
     */
    HRESULT ReadShortData(USHORT *pShortData);

    /**
     * Read a RIFF tag
     **/
    HRESULT ReadRIFFTag(RIFFTag *plTag);

    /**
     * Find the next 'fact', 'data' or 'fmt ' chunk
     * @param plChunkTag set to the next chunk tag
     * @param plChunkLen set to the next chunk length
     **/
    HRESULT FindNextFormChunk(RIFFTag *plChunkTag, ULONG *plChunkLen);

  private:
    IStream *m_pStream;
};