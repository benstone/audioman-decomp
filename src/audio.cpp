#include "audio.h"
#include "factory.h"

HRESULT AM_CreateInstance(REFCLSID rclsid, IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
    HRESULT hr = S_OK;
    CClassFactory class_factory;

    if (class_factory.Init(rclsid))
    {
        hr = class_factory.CreateInstance(pUnkOuter, riid, ppvObject);
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    return hr;
}