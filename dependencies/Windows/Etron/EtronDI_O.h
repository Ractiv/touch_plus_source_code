#ifndef _ETRONDI_O_H_
#define _ETRONDI_O_H_

#ifdef ETRONDI_EXPORTS
#define ETRONDI_API __declspec(dllexport)
#else
#define ETRONDI_API __declspec(dllimport)
#endif

//
// C++ compatibility
//
#ifdef  __cplusplus
extern "C" {
#endif

#ifndef BYTE
typedef unsigned char BYTE;
#endif //BYTE
#ifndef BOOL
typedef signed int BOOL;
#endif //BOOL

#pragma pack(push, 1)
typedef struct tagETRONDI_STREAM_INFO {
	int		nWidth;
	int		nHeight;
	BOOL	bFormatMJPG;
} ETRONDI_STREAM_INFO;
#pragma pack(pop)

#ifndef WM_MYMSG_NOTICE_CAPTURE
#define WM_MYMSG_NOTICE_CAPTURE	(WM_USER+101)
#endif

//define Error Code by Wolf 2013/08/30
#define  ETronDI_OK					0
#define  ETronDI_NoDevice			1
#define  ETronDI_NullPtr			2
#define  ETronDI_ErrBufLen			3
#define  ETronDI_Init_Fail			4
#define  ETronDI_NoZDTable			5 
#define  ETronDI_Reg_Read_Fail		6
#define  ETronDI_Reg_Write_Fail		7
#define  ETronDI_App_Not_Supported	8 

typedef struct tagTABLEINFO {
	int  EffectiveLength;
  int  DistanceUnit;
} TABLEINFO;

typedef struct tagDEVINFORMATION {
  WORD wPID;
  WORD wVID;
  char * strDevName;
} DEVINFORMATION;

//
// v1.0
//
bool ETRONDI_API EtronDI_Init(void **ppHandleEtronDI);
void ETRONDI_API EtronDI_Release(void **ppHandleEtronDI);
//
int ETRONDI_API EtronDI_FindDevice(void *pHandleEtronDI);
bool ETRONDI_API EtronDI_OpenDevice(void *pHandleEtronDI, int nWidth, int nHeight, bool bImageL, bool bDepth, void *phWndNotice=NULL);
void ETRONDI_API EtronDI_CloseDevice(void *pHandleEtronDI);
int ETRONDI_API EtronDI_GetZDTable(void *pHandleEtronDI, char *lpDesBuf, int BufferLength, TABLEINFO* ptableinfo);
bool ETRONDI_API EtronDI_GetImage(void *pHandleEtronDI, BYTE *pBuf, int *pSerial=NULL);
bool ETRONDI_API EtronDI_Get2Image(void *pHandleEtronDI, BYTE *pImageLBuf, BYTE *pDepthBuf, int *pSerial=NULL, int *pSerial2=NULL);
//
int  ETRONDI_API EtronDI_GetVersion(void *pHandleEtronDI);
void ETRONDI_API EtronDI_EnablePostProcess(void *pHandleEtronDI, bool bEnable);
//
int  ETRONDI_API EtronDI_GetDeviceNumber(void *pHandleEtronDI);
bool ETRONDI_API EtronDI_GetDeviceInfo(void *pHandleEtronDI, int dev_index, DEVINFORMATION* pdevinformation);
bool ETRONDI_API EtronDI_SelectDevice(void *pHandleEtronDI, int dev_index);
//
// v1.1
//
bool ETRONDI_API EtronDI_OpenDevice2(void *pHandleEtronDI, int nEP0Width, int nEP0Height, BOOL bEP0MJPG, 
									 int nEP1Width, int nEP2Height, void *phWndNotice=NULL);
int  ETRONDI_API EtronDI_GetDeviceResolutionList(void *pHandleEtronDI, int nMaxCount0, ETRONDI_STREAM_INFO *pStreamInfo0, int nMaxCount1, ETRONDI_STREAM_INFO *pStreamInfo1);


#ifdef __cplusplus
}
#endif

#endif //_ETRONDI_O_H_
