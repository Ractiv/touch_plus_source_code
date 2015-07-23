// 下列 ifdef 區塊是建立巨集以協助從 DLL 匯出的標準方式。
// 這個 DLL 中的所有檔案都是使用命令列中所定義 ESPAEAWBCTRL_EXPORTS 符號編譯的。
// 任何使用這個 DLL 的專案都不應定義這個符號。
// 這樣一來，原始程式檔中包含這檔案的任何其他專案
// 會將 ESPAEAWBCTRL_API 函式視為從 DLL 匯入的，而這個 DLL 則會將這些符號視為
// 匯出的。
#ifdef ESPAEAWBCTRL_EXPORTS
#define ESPAEAWBCTRL_API __declspec(dllexport)
#else
#define ESPAEAWBCTRL_API __declspec(dllimport)
#endif

#pragma pack(push, 1)
//
// Sensor Type
//
#define ESPAEAWB_SENSOR_TYPE_H22		0x00
#define ESPAEAWB_SENSOR_TYPE_OV7740		0x01
//
// Sensor Mode: Left, Right, or Both
//
#define ESPAEAWB_SENSOR_MODE_LEFT		0
#define ESPAEAWB_SENSOR_MODE_RIGHT		1
#define ESPAEAWB_SENSOR_MODE_BOTH		2
//
// PU Property ID
//
#define    PU_PROPERTY_ID_BRIGHTNESS                0
#define    PU_PROPERTY_ID_CONTRAST                  1
#define    PU_PROPERTY_ID_HUE                       2
#define    PU_PROPERTY_ID_SATURATION                3
#define    PU_PROPERTY_ID_SHARPNESS                 4
#define    PU_PROPERTY_ID_GAMMA                     5
#define    PU_PROPERTY_ID_COLORENABLE               6
#define    PU_PROPERTY_ID_WHITEBALANCE              7
#define    PU_PROPERTY_ID_BACKLIGHT_COMPENSATION    8
#define    PU_PROPERTY_ID_GAIN                      9
#define    PU_PROPERTY_ID_DIGITAL_MULTIPLIER        10
#define    PU_PROPERTY_ID_DIGITAL_MULTIPLIER_LIMIT  11
#define    PU_PROPERTY_ID_WHITEBALANCE_COMPONENT    12
#define    PU_PROPERTY_ID_POWERLINE_FREQUENCY       13
//
// CT Property ID
//
#define    CT_PROPERTY_ID_PAN               			0
#define    CT_PROPERTY_ID_TILT                         1
#define    CT_PROPERTY_ID_ROLL                         2
#define    CT_PROPERTY_ID_ZOOM                         3
#define    CT_PROPERTY_ID_EXPOSURE                     4
#define    CT_PROPERTY_ID_IRIS                         5
#define    CT_PROPERTY_ID_FOCUS                        6
#define    CT_PROPERTY_ID_SCANMODE                     7
#define    CT_PROPERTY_ID_PRIVACY                      8
#define    CT_PROPERTY_ID_PANTILT                      9
#define    CT_PROPERTY_ID_PAN_RELATIVE                 10
#define    CT_PROPERTY_ID_TILT_RELATIVE                11
#define    CT_PROPERTY_ID_ROLL_RELATIVE                12
#define    CT_PROPERTY_ID_ZOOM_RELATIVE                13
#define    CT_PROPERTY_ID_EXPOSURE_RELATIVE            14
#define    CT_PROPERTY_ID_IRIS_RELATIVE                15
#define    CT_PROPERTY_ID_FOCUS_RELATIVE               16
#define    CT_PROPERTY_ID_PANTILT_RELATIVE             17
#define    CT_PROPERTY_ID_AUTO_EXPOSURE_PRIORITY       19  

//
// Return Code
//
#define ESPAEAWB_RET_OK					0
#define ESPAEAWB_RET_ENUM_FAIL			-10
#define ESPAEAWB_RET_BAD_PARAM			-11
#define ESPAEAWB_RET_INVALID_DEVICE		-12
#define ESPAEAWB_RET_INVALID_SENSOR		-13
#define ESPAEAWB_RET_REG_READ_FAIL		-21
#define ESPAEAWB_RET_REG_WRITE_FAIL		-22
#define ESPAEAWB_RET_PROP_READ_FAIL		-23
#define ESPAEAWB_RET_PROP_WRITE_FAIL	-24
#define ESPAEAWB_RET_APP_NOT_SUPPORTED	-25

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
//
// v1.0
//
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_EnumDevice(int *pDeviceCount);
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_GetDevicename(int i, WCHAR *lpwszName, int nMaxCount);
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_SelectDevice(int i);
//
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_SetSensorType(int i);
//
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_EnableAE();
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_DisableAE();
//
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_EnableAWB();
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_DisableAWB();
//
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_GetExposureTime(int nSensorMode, float *pfExpTimeMS);	//unit: ms
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_SetExposureTime(int nSensorMode, float fExpTimeMS);	//unit: ms
//
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_GetGlobalGain(int nSensorMode, float *pfGlobalGain);
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_SetGlobalGain(int nSensorMode, float fGlobalGain);
//
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_GetColorGain(int nSensorMode, float *pfGainR, float *pfGainG, float *pfGainB);
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_SetColorGain(int nSensorMode, float fGainR, float fGainG, float fGainB);
//
// v1.1
//
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_GetGPIOValue(int nGPIOIndex, BYTE *pValue);
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_SetGPIOValue(int nGPIOIndex, BYTE nValue);
//
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_GetAccMeterValue(int *pX, int *pY, int *pZ);
//
// v1.2   
//
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_GetPUPropVal(int nId, int *pValue);
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_SetPUPropVal(int nId, int nValue);
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_GetCTPropVal(int nId, int *pValue);
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_SetCTPropVal(int nId, int nValue);
//
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_ReadFlash(BYTE *Data, int WriteLen);
ESPAEAWBCTRL_API int WINAPI eSPAEAWB_WriteFlash(BYTE *Data, int WriteLen);
//
// v1.3. For Ractive
//
//
ESPAEAWBCTRL_API int  WINAPI eSPAEAWB_SWUnlock(WORD wAppId); 
ESPAEAWBCTRL_API int  WINAPI eSPAEAWB_SWLock(WORD wAppId); 

//
#ifdef __cplusplus
}
#endif //__cplusplus

