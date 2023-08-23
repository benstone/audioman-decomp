#pragma once
#include "audiomaninternal.h"


/**
 * Create an instance of an AudioMan class.
 * @param rclsid CLSID of class to instantiate
 * @param pUnkOuter pointer to outer class (not used)
 * @param riid interface to request from class
 * @param ppvObject if successful, set to the pointer to the new object
 **/
HRESULT AM_CreateInstance(REFCLSID rclsid, IUnknown *pUnkOuter, REFIID riid, void **ppvObject);