#pragma once

#include <Windows.h>

// define INITGUID so we define the GUIDs in AUDIOMAN.H
// we only need to do this once
#define INITGUID
#include <guiddef.h>

#include "AUDIOMAN.H"

#define INFINITE_SAMPLES 0xFFFFFFFF

/**
 * Create an instance of an AudioMan class.
 * @param rclsid CLSID of class to instantiate
 * @param riid IID of interface to get from class
 * @param ppvObject set to instance of class using provided interface
 * @param ppSound set to instance of class using IAMSound interface
 **/
HRESULT AMCreate(REFCLSID rclsid, REFIID riid, void **ppvObject, LPSOUND *ppSound);

/**
 * If the given HRESULT indicates success, return the sound. If not, releases the sound.
 *
 * @param hr HRESULT indicating success/failure of previous operation
 * @param ppSound set to the sound if successful
 * @param pUnk IUnknown object instance. This will be released.
 * @param pSnd sound
 **/
void AMFinish(HRESULT hr, LPSOUND *ppSound, IUnknown *pUnk, LPSOUND pSnd);

/**
 * Create a cache filter that will cache the given sound.
 * @param ppSound set to the result sound
 * @param pUnk IUnknown object instance. This will be released.
 * @param pSnd Sound to cache
 * @param fSpooled if True, cache the sound
 * @param lpCacheConfig Cache configuration
 **/
HRESULT AMFinishCache(LPSOUND *ppSound, IUnknown *pUnk, LPSOUND pSnd, BOOL fSpooled, LPCACHECONFIG lpCacheConfig);