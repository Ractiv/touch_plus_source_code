#ifndef CCAMERA_H
#define CCAMERA_H

#define WIN32_LEAN_AND_MEAN

#include <atlbase.h>
#include <windows.h>
#include "opencv/cxcore.h"

#include <qedit.h>
#include <dshow.h>

#define MYFREEMEDIATYPE(mt)	{if ((mt).cbFormat != 0)		\
					{CoTaskMemFree((PVOID)(mt).pbFormat);	\
					(mt).cbFormat = 0;						\
					(mt).pbFormat = NULL;					\
				}											\
				if ((mt).pUnk != NULL)						\
				{											\
					(mt).pUnk->Release();					\
					(mt).pUnk = NULL;						\
				}}									


typedef void(*FRAME_CB_FUNC)(BYTE * pBuffer, long lBufferSize);



class CCamFrameHandler {
public:
    virtual void CamFrameData(double dblSampleTime, BYTE * pBuffer, long lBufferSize) = 0;
};


class CSampleGrabberCB : public ISampleGrabberCB
{
public:
    long                   lWidth;
    long                   lHeight;
    CCamFrameHandler    *  frame_handler;
    BOOL                   bGrabVideo;
    FRAME_CB_FUNC          m_frame_cb;
public:
    CSampleGrabberCB(FRAME_CB_FUNC fp = NULL){
        lWidth = 0;
        lHeight = 0;
        bGrabVideo = FALSE;
        frame_handler = NULL;
        m_frame_cb = fp;
    }
    STDMETHODIMP_(ULONG) AddRef() { return 2; }
    STDMETHODIMP_(ULONG) Release() { return 1; }
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv) {
        if (riid == IID_ISampleGrabberCB || riid == IID_IUnknown){
            *ppv = (void *) static_cast<ISampleGrabberCB*> (this);
            return NOERROR;
        }
        return E_NOINTERFACE;
    }

    STDMETHODIMP SampleCB(double SampleTime, IMediaSample * pSample)  {
        return 0;
    }

    STDMETHODIMP BufferCB(double dblSampleTime, BYTE * pBuffer, long lBufferSize){
        if (NULL != m_frame_cb && NULL != pBuffer)
        {
            (*m_frame_cb)(pBuffer, lBufferSize);
        }
        return 0;
    }
};

class CCameraDS  
{
private:
	IplImage * m_pFrame;
	bool m_bConnected;
	int m_nWidth;
	int m_nHeight;
	bool m_bLock;
	bool m_bChanged;
	long m_nBufferSize;

    CSampleGrabberCB*        m_CB;

	CComPtr<IGraphBuilder> m_pGraph;
	CComPtr<IBaseFilter> m_pDeviceFilter;
	CComPtr<IMediaControl> m_pMediaControl;
	CComPtr<IBaseFilter> m_pSampleGrabberFilter;
	CComPtr<ISampleGrabber> m_pSampleGrabber;
	CComPtr<IPin> m_pGrabberInput;
	CComPtr<IPin> m_pGrabberOutput;
	CComPtr<IPin> m_pCameraOutput;
	CComPtr<IMediaEvent> m_pMediaEvent;
	CComPtr<IBaseFilter> m_pNullFilter;
	CComPtr<IPin> m_pNullInputPin;

private:
	bool BindFilter(int nCamIDX, IBaseFilter **pFilter);
	void SetCrossBar();

public:
	CCameraDS();
	virtual ~CCameraDS();

	bool OpenCamera(int nCamID, bool bDisplayProperties=true, int nWidth=320, int nHeight=240);
    bool OpenCamera(int nCamID, const int& foramt, const int& width, const int& height, const int& fps, FRAME_CB_FUNC f = NULL);
	void CloseCamera();
	static int CameraCount(); 
	static int CCameraDS::CameraName(int nCamID, char* sName, int nBufferSize);
    static int CCameraDS::CameraInfo(int nCamID, char* vid, char* pid);
	int GetWidth(){return m_nWidth;} 
	int GetHeight(){return m_nHeight;}
	IplImage * QueryFrame();
};

#endif 
