/**
 * Mutex
 **/
#pragma once

// uses CRITICAL_SECTION
#include <Windows.h>

// NOTE: This class was called MUTX, but that conflicts with the MUTX class in the Kauai library used in 3D Movie Maker.
class CAMMutex
{
  public:
    CAMMutex();
    ~CAMMutex();

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