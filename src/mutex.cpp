#include "mutex.h"

CAMMutex::CAMMutex()
{
    InitializeCriticalSection(&m_CritSec);
}

CAMMutex::~CAMMutex()
{
    DeleteCriticalSection(&m_CritSec);
}

void CAMMutex::Enter()
{
    EnterCriticalSection(&m_CritSec);
}

void CAMMutex::Leave()
{
    LeaveCriticalSection(&m_CritSec);
}
