/**
 * Class factory
 **/
#pragma once
#include <Windows.h>
#include "AUDIOMAN.H"

class CClassFactory : public IClassFactory
{
  public:
    // IClassFactory methods
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();
    virtual HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
    virtual HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock);

    /**
     * Initialize the class factory
     * @param rclsid CLSID of classes this factory will create
     **/
    BOOL Init(REFCLSID rclsid);

  private:
    ULONG m_RefCnt = 0;
    CLSID m_clsid = {0};
};