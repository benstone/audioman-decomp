#include "mutex.h"

MUTX::MUTX()
{
    InitializeCriticalSection(&m_CritSec);
}

MUTX::~MUTX()
{
    DeleteCriticalSection(&m_CritSec);
}

void MUTX::Enter()
{
    EnterCriticalSection(&m_CritSec);
}

void MUTX::Leave()
{
    LeaveCriticalSection(&m_CritSec);
}
