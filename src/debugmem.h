/**
 * Debug memory allocator
 **/
#pragma once
#include "audiomaninternal.h"

// Flags for DetectLeaks()
#define DETECT_LEAKS_OUTPUT_DEBUG 1
#define DETECT_LEAKS_MESSAGE_BOX 2
#define DETECT_LEAKS_MAX_MEMORY 4

// Debug memory allocator
class CDebugMem
{
  public:
    /**
     * Allocate memory
     * @param uFlags Flags to pass to GlobalAlloc
     * @param dwBytes Size of allocation
     **/
    static void *AllocPtr(UINT uFlags, DWORD dwBytes);

    /**
     * Free memory
     * @param lp allocation to free
     **/
    static void FreePtr(void *lp);

    /**
     * Check for memory leaks. Logs to debugger / displays a message box if leaks are found.
     * @param uFlags Flags to indicate what to do if memory leaks are found
     * @return number of leaked memory blocks
     **/
    static int DetectLeaks(UINT uFlags);
};
