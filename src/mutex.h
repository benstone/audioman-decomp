/**
 * Mutex
 **/
#pragma once

// uses CRITICAL_SECTION
#include <Windows.h>

class MUTX
{
  public:
    MUTX();
    ~MUTX();

    /**
     * Acquire the mutex
     **/
    void Enter();

    /**
     * Release the mutex
     **/
    void Leave();

  private:
    CRITICAL_SECTION m_CritSec;
};