#include "debugmem.h"
#include <cassert>
#include <tchar.h>

#include "dpf.h"

UINT g_cDebugMemAllocBlocks = 0;
UINT g_cbDebugMemAllocNet = 0;
UINT g_cbDebugMemAllocGross = 0;
UINT g_cbDebugMaxNetMemAlloc = 0;

const UINT tag = 0x56788765;

// Header placed at start of allocations
typedef struct ObjectHeader_t
{
    UINT size;
    UINT tag;
} ObjectHeader;

// Footer placed at end of allocations
typedef struct ObjectFooter_t
{
    UINT tag;
} ObjectFooter;

void *CDebugMem::AllocPtr(UINT uFlags, DWORD dwBytes)
{
    HGLOBAL hMem = GlobalAlloc(uFlags, dwBytes + sizeof(ObjectHeader) + sizeof(ObjectFooter));
    BYTE *pBytes = (BYTE *)(GlobalLock(hMem));
    if (pBytes != NULL)
    {
        g_cDebugMemAllocBlocks += 1;
        g_cbDebugMemAllocNet += dwBytes;
        g_cbDebugMemAllocGross += dwBytes + sizeof(ObjectHeader) + sizeof(ObjectFooter);
        if (g_cbDebugMaxNetMemAlloc <= g_cbDebugMemAllocNet)
        {
            g_cbDebugMaxNetMemAlloc = g_cbDebugMemAllocNet;
        }

        ObjectHeader *header = (ObjectHeader *)pBytes;
        header->size = dwBytes;
        header->tag = tag;

        ObjectFooter *footer = (ObjectFooter *)(pBytes + sizeof(ObjectHeader) + dwBytes);
        footer->tag = tag;

        return (pBytes + sizeof(ObjectHeader));
    }

    return pBytes;
}

void CDebugMem::FreePtr(void *lp)
{
    ObjectHeader *header = (ObjectHeader *)((BYTE *)lp - sizeof(ObjectHeader));
    ObjectFooter *footer = (ObjectFooter *)((BYTE *)lp + header->size);

    if (header->tag != tag)
    {
        OutputDebugString(TEXT("AUDIOMAN: Corrupt Block Detected"));
        assert(0);
    }

    g_cDebugMemAllocBlocks -= 1;
    g_cbDebugMemAllocNet -= header->size;
    g_cbDebugMemAllocGross -= (header->size + sizeof(ObjectHeader) + sizeof(ObjectFooter));

    if (footer->tag != tag)
    {
        OutputDebugString(TEXT("AUDIOMAN: Corrupt Block Detected"));
        assert(0);
    }

    HGLOBAL hMem = GlobalHandle(header);
    GlobalUnlock(hMem);
    hMem = GlobalHandle(header);
    GlobalFree(hMem);
}

int CDebugMem::DetectLeaks(UINT uFlags)
{
    CHAR ach[100];

    if (((g_cDebugMemAllocBlocks != 0) || (g_cbDebugMemAllocNet != 0)) || (g_cbDebugMemAllocGross != 0))
    {
        wsprintfA(ach,
                  "Warning: %d blocks allocated by CDebugMem not freed (%d "
                  "bytes net, %d bytes gross)\n",
                  g_cDebugMemAllocBlocks, g_cbDebugMemAllocNet, g_cbDebugMemAllocGross);
        if ((uFlags & DETECT_LEAKS_OUTPUT_DEBUG) != 0)
        {
            OutputDebugStringA(ach);
        }
        if ((uFlags & DETECT_LEAKS_MESSAGE_BOX) != 0)
        {
            MessageBoxA(NULL, ach, "CDebugMem", MB_SYSTEMMODAL);
        }
    }
    if ((uFlags & DETECT_LEAKS_MAX_MEMORY) != 0)
    {
        wsprintfA(ach, "Max Allocated Net Memory %lu \n", g_cbDebugMaxNetMemAlloc);
        OutputDebugStringA(ach);
    }
    return g_cDebugMemAllocBlocks;
}

STDAPI_(int) DetectLeaks(BOOL fDebugOut, BOOL fMessageBox)
{
    DWORD dwFlags = 0;

    if (fDebugOut)
        dwFlags |= DETECT_LEAKS_OUTPUT_DEBUG;
    if (fMessageBox)
        dwFlags |= DETECT_LEAKS_MESSAGE_BOX;

    return CDebugMem::DetectLeaks(dwFlags);
}

#ifdef AUDIOMAN_USE_DEBUG_ALLOCATOR
void *operator new(size_t size)
{
    return CDebugMem::AllocPtr(0x2040, (UINT)size);
}

void operator delete(void *ptr)
{
    CDebugMem::FreePtr(ptr);
}
#endif // AUDIOMAN_USE_DEBUG_ALLOCATOR