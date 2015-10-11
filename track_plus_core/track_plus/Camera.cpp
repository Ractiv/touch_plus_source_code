/*
 * Touch+ Software
 * Copyright (C) 2015
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the Aladdin Free Public License as
 * published by the Aladdin Enterprises, either version 9 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Aladdin Free Public License for more details.
 *
 * You should have received a copy of the Aladdin Free Public License
 * along with this program.  If not, see <http://ghostscript.com/doc/8.54/Public.htm>.
 */

#include "Camera.h"
#include "console_log.h"

#define TP_CAMERA_VID   "1e4e"
#define TP_CAMERA_PID   "0107"
#define USE_DIRECT_SHOW 0

function<void (Mat& image_in)> Camera::callback;

unsigned char * myBuffer;
Mat image_out = Mat(480, 1280, CV_8UC3);

void * pHandle = NULL;

Camera::Camera(){}

Camera::Camera(bool _useMJPEG, int _width, int _height, function<void (Mat& image_in)> callback_in)
{
    height = _height;
    width = _width;
    callback = callback_in;
    int present = doSetup(1);
    if (!present)
        return;

#ifdef __APPLE__
    startVideoStream(_width, _height, 60, MJPEG);
#endif
}

Camera::~Camera()
{
#ifdef _WIN32
    if (ds_camera_)
    {
        ds_camera_->CloseCamera();
        ds_camera_ = NULL;
    }
    free(&buffer);
    free(&bufferRGB);
    free(&depth);
    
#elif __APPLE__
    stopVideoStream();
#endif
}

#ifdef __APPLE__
int decompressJPEG = 0;
libusb_device_handle* dev_handle;
uvc_context_t* ctx = NULL;
uvc_device_handle_t* devh = NULL;
uvc_device_t* dev;
unsigned char* liveData;
uvc_frame_t* bgr;

void Camera::read_ADDR_85(libusb_device_handle* dev_handle, unsigned short wValue)
{
    unsigned short wIndex = 0x0400;
    unsigned char bmRequestType_get = 0xa1;
    unsigned char data[2];
    libusb_control_transfer(dev_handle, bmRequestType_get, 0x85, wValue,wIndex, data, 2, 10000);
}

unsigned char Camera::read_ADDR_81(libusb_device_handle* dev_handle, unsigned short wValue, int length)
{
    unsigned short wIndex = 0x0400;
    unsigned char bmRequestType_get = 0xa1;
    unsigned char data[length];
    libusb_control_transfer(dev_handle, bmRequestType_get, 0x81,wValue, wIndex,data, length, 10000);
    return data[1];
}

unsigned char Camera::read_ADDR_81(libusb_device_handle* dev_handle, unsigned char* data, unsigned short wValue, int length)
{
    unsigned short wIndex = 0x0400;
    unsigned char bmRequestType_get = 0xa1;
    libusb_control_transfer(dev_handle, bmRequestType_get, 0x81,wValue, wIndex, data, length, 10000);
    return 0;
}


void Camera::write_ADDR_01(libusb_device_handle* dev_handle, unsigned char* data, unsigned short wValue, int length)
{
    unsigned short wIndex = 0x0400;
    unsigned char bmRequestType_set = 0x21;
    libusb_control_transfer(dev_handle, bmRequestType_set, 0x01, wValue, wIndex, data, length, 10000);
}

int Camera::readFlash(unsigned char* data, int length)
{
    read_ADDR_85(dev_handle, 0x0a00);
    unsigned char count[1];
    read_ADDR_81(dev_handle,count,0x0a00,1);
    read_ADDR_85(dev_handle,0x0b00);
    unsigned char mystery[16]= { count[0], 0x41, 0x05, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x00,
        (unsigned char)length, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    
    write_ADDR_01(dev_handle, mystery, 0x0b00, 16);
    
    read_ADDR_85(dev_handle, 0x0c00);
    read_ADDR_81(dev_handle, data, 0x0c00, length);
    
    write_ADDR_01(dev_handle, count, 0x0a00, 1);
    // closeTouchPlusDevHandle(dev_handle);
    return 0;
}

void cb(uvc_frame_t *frame, void *ptr)
{
    uvc_error_t ret;
    /* Do the BGR conversion */
    
    if (decompressJPEG == 0)
        ret = uvc_any2bgr(frame, bgr);
    else
    {
        ret = uvc_mjpeg2rgb(frame, bgr);
        if (ret)
        {
            uvc_perror(ret, "change to BGR error");
            uvc_free_frame(bgr);
            return;
        }
        
        Mat image_received = Mat(480, 1280, CV_8UC3);
        image_received.data = (uchar*)bgr->data;
        
        const int width = image_received.cols;
        const int height = image_received.rows;
        for (int i = 0; i < width; ++i)
            for (int j = 0; j < height; ++j)
            {
                image_out.ptr<uchar>(j, i)[0] = image_received.ptr<uchar>(j, i)[2];
                image_out.ptr<uchar>(j, i)[1] = image_received.ptr<uchar>(j, i)[1];
                image_out.ptr<uchar>(j, i)[2] = image_received.ptr<uchar>(j, i)[0];
            }
        
        Camera::callback(image_out);
    }
}

int Camera::startVideoStream(int width, int height, int framerate, int format)
{
    //get video stream begin
    uvc_stream_ctrl_t ctrl;
    uvc_error_t res;
    fprintf(stderr,"start streaming!!!\n");
    
    if (format == UNCOMPRESSED)
    {
        res = uvc_get_stream_ctrl_format_size(
                                              devh, &ctrl, //result stored in ctrl
                                              UVC_FRAME_FORMAT_YUYV, //YUV 422, aka YUV 4:2:2. try _COMPRESSED
                                              width, height, framerate //width, height, fps
                                              );
    }
    else if (format == MJPEG)
    {
        decompressJPEG = 1;
        res = uvc_get_stream_ctrl_format_size(
                                              devh, &ctrl, //result stored in ctrl
                                              UVC_FRAME_FORMAT_MJPEG, //YUV 422, aka YUV 4:2:2. try _COMPRESSED
                                              width, height, framerate //width, height, fps
                                              );
    }
    
    /* Print out the result */
    uvc_print_stream_ctrl(&ctrl, stderr);
    if (res < 0)
    {
        console_log("error 0");
        uvc_perror(res, "get_mode"); /* device doesn't provide a matching stream */
    }
    else
    {
        int value = 0;
        bgr = uvc_allocate_frame(width*height*3);
        if (!bgr)
        {
            printf("unable to allocate bgr frame!");
            return -1;
        }

        res = uvc_start_streaming(devh, &ctrl, cb, (void*)(&value), 0);
        if (res < 0)
        {
            console_log("error 1");
            uvc_perror(res, "start_streaming"); /* unable to start stream */
        }
        else
            puts("Streaming...");
    }
    return 0;
}

int Camera::stopVideoStream()
{
    uvc_stop_streaming(devh);
    uvc_free_frame(bgr);
    uvc_close(devh);
    uvc_unref_device(dev);
    uvc_exit(ctx);
    return 0;
}
#endif

JPEGDecompressor jpeg_decompressor;

#ifdef _WIN32
static void frameCallback(BYTE* pBuffer, long lBufferSize)
{
    if (jpeg_decompressor.compute(pBuffer, lBufferSize, myBuffer, 1280, 480))
        Camera::callback(image_out);
    // else
        // console_log("bad image caught");
}
#endif

string Camera::getSerialNumber()
{
    Sleep(10);

    string result = "";
    unsigned char serialNumber[10];
#ifdef _WIN32
    eSPAEAWB_ReadFlash(serialNumber, 10);

#elif __APPLE__
    readFlash(serialNumber, 10);
#endif

    for (int i = 0; i < 10; i++)
    {
        int digit = serialNumber[i];
        result += to_string(digit);
    }
    return result;
}

int Camera::isCameraPresent()
{
    Sleep(10);

#ifdef _WIN32
    int devCount;
    int didInit = EtronDI_Init(&pHandle);
    int devNumber = EtronDI_GetDeviceNumber(pHandle);
    
    WCHAR name[100];
    if (!EtronDI_FindDevice(pHandle)) {
        console_log("Device not found!");
        return 0;
    }
    int code = eSPAEAWB_EnumDevice(&devCount);
    eSPAEAWB_SelectDevice(0);
    eSPAEAWB_SetSensorType(1);
    WCHAR myName[255];
    WCHAR targetName[255] = L"Touch+ Camera";
    int deviceWasSelected = 0;
    for (int i = 0; i < devCount; i++){
        eSPAEAWB_GetDevicename(i, myName, 255);
        
        wstring ws(myName);
        string str(ws.begin(), ws.end());
        console_log(str);
        
        myName[14] = 0;
        bool found = true;
        int j;
        for (j = 0; j < 13; j++){
            if (myName[j] != targetName[j]){
                found = false;
            }
        }
        if (found){
            eSPAEAWB_SelectDevice(i);
            eSPAEAWB_SetSensorType(1);
            deviceWasSelected = 1;
            console_log("Touch+ Camera found");
        }
        
    }
    if (!deviceWasSelected){
        device_not_detected = true;
        console_log("Did not find a Touch+ Camera");
        return 0;
    }
#endif
    
    return 1;
}

int Camera::do_software_unlock()
{
    Sleep(10);

#ifdef _WIN32
    int present = isCameraPresent();
    int retVal = eSPAEAWB_SWUnlock(0x0107);
    return present;

#elif __APPLE__
    //libusb_device** devs; //pointer to pointer of device, used to retrieve a list of devices
    libusb_device_handle* dev_handle; //a device handle
    //libusb_device_descriptor* desc;
    libusb_device* device;
    libusb_context* ctx = NULL; //a libusb session
    int r; //for return values
    //ssize_t cnt; //holding number of devices in list
    
    r = libusb_init(&ctx); //initialize the library for the session we just declared
    if(r < 0)
    {
        console_log("Init Error " + to_string(r)); //there was an error
        return 1;
    }
    
    dev_handle = libusb_open_device_with_vid_pid(ctx, 0x1e4e, 0x0107); //vendorID and productID for usb device
    if (dev_handle == NULL)
    {
        console_log("Cannot open device");
        return 0;
    }
    else
        console_log("Device Opened");
    
    //libusb_free_device_list(devs, 1); //free the list, unref the devices in it
    
    //release the driver from the kernel
    if(libusb_kernel_driver_active(dev_handle, 0) == 1)
    { //find out if kernel driver is attached
        console_log("Kernel Driver Active");
        
        if (libusb_detach_kernel_driver(dev_handle, 0) == 0) //detach it
            console_log("Kernel Driver Detached!");
    }
    
    r = libusb_claim_interface(dev_handle, 0); //claim interface 0 (the first) of device (mine had jsut 1)
    if (r < 0)
    {
        console_log("Cannot Claim Interface");
        return 0;
    }
    
    console_log("Claimed Interface");
    
    device = libusb_get_device(dev_handle);
    int configuration = -1;
    
    r = libusb_get_configuration(dev_handle,&configuration);
    if (r == 0)
    {
        console_log("GET CONFIGURATION SUCCESS -- Value is " + to_string(configuration));
    }
    
    //libusb_transfer transfer ={0};
    //unsigned char endpoint = 0x83;
    unsigned char bmRequestType_set = 0x21;
    unsigned char bmRequestType_get = 0xa1;
    unsigned char GET_CUR = 0x85;
    unsigned char SET_CUR = 0x01;
    unsigned short wValue =0x0300;
    unsigned short wIndex = 0x0400;
    unsigned char data[5];
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    
    r= libusb_control_transfer(dev_handle, bmRequestType_get, GET_CUR, wValue, wIndex, data, 2, 10000);
    data[0]=0x82;
    data[1]=0xf1;
    data[2]=0xf8;
    data[3]=0x00;
    
    r= libusb_control_transfer(dev_handle, bmRequestType_set, SET_CUR, wValue, wIndex, data, 4, 10000);
    data[0]=0x00;
    data[1]=0x00;
    r= libusb_control_transfer(dev_handle, bmRequestType_get, GET_CUR, wValue, wIndex, data, 2, 10000);
    
    data[0]=0x00;
    data[1]=0x00;
    data[2]=0x00;
    data[3]=0x00;
    GET_CUR = 0x81;
    r= libusb_control_transfer(dev_handle, bmRequestType_get, GET_CUR, wValue, wIndex, data, 4, 10000);
    
    data[0]=0x00;
    data[1]=0x00;
    GET_CUR=0x85;
    r= libusb_control_transfer(dev_handle, bmRequestType_get, GET_CUR, wValue, wIndex, data, 2, 10000);
    
    data[0]=0x02;
    data[1]=0xf1;
    data[2]=0xf8;
    data[3]=0x40;
    r= libusb_control_transfer(dev_handle, bmRequestType_set, SET_CUR, wValue, wIndex, data, 4, 0);
    
    data[0]=0x00;
    data[1]=0x00;
    r= libusb_control_transfer(dev_handle, bmRequestType_get, GET_CUR, wValue, wIndex, data, 2, 0);
    
    data[0]=0x00;
    data[1]=0x00;
    data[2]=0x00;
    data[3]=0x00;
    GET_CUR=0x81;
    r= libusb_control_transfer(dev_handle, bmRequestType_get, GET_CUR, wValue, wIndex, data, 4, 0);
    
    fprintf(stderr, "Camera unlocked\n");
    
    r = libusb_release_interface(dev_handle, 0); //release the claimed interface
    if (r != 0)
    {
        console_log("Cannot Release Interface");
        return 0;
    }
    
    libusb_close(dev_handle); //close the device we opened
    libusb_exit(ctx); //needs to be called to end the
    
    return 1;
#endif
}

int Camera::doSetup(const int & format)
{
    int present = do_software_unlock();
    if (present == 0)
        return 0;

#ifdef _WIN32
    ds_camera_ = new CCameraDS();
    int camera_count = CCameraDS::CameraCount();
    char camera_name[255] = { 0 };
    char camera_vid[10] = { 0 };
    char camera_pid[10] = { 0 };
    int i = 0, touchCameraId = -1;
    
    while (i < camera_count)
    {
        CCameraDS::CameraInfo(i, camera_vid, camera_pid);
        
        // VID PID is more reasonable
        if (0 == strncmp(camera_vid, TP_CAMERA_VID, 4) &&
            0 == strncmp(camera_pid, TP_CAMERA_PID, 4))
        {
            touchCameraId = i;
            break;
        }
        ++i;
    }
    
    if (-1 == touchCameraId)
        return 0;
    
    const int fmt = 1;
    myBuffer = (unsigned char *)malloc(1280 * 480 * 3);
    image_out.data = myBuffer;
    bool retV = ds_camera_->OpenCamera(touchCameraId, format, 1280, 480, 60, frameCallback);

    console_log("camera opened = " + to_string(retV));
    return retV;

#elif __APPLE__
    uvc_error_t res;
    
    res = uvc_init(&ctx, NULL);
    if (res < 0)
    {
        uvc_perror(res, "uvc_init");
        return 0;
    }
    
    puts("UVC initialized");
    res = uvc_find_device(ctx, &dev, 0x1e4e, 0x0107, NULL); //filter devices: vendor_id, product_id, "serial_num"
    if (res < 0)
    {
        uvc_perror(res, "uvc_find_device"); //no devices found
        return 0;
    }
    else
    {
        puts("Device found");
        res = uvc_open(dev, &devh);
        
        console_log("devh " + to_string(devh));
        uvc_print_diag(devh, stderr);
    }
    
    liveData = (unsigned char*)malloc(1280 * 480 * 3);
    
    fprintf(stderr,"returning libusb handle\n");
    
    dev_handle = uvc_get_libusb_handle(devh);
    return 1;
#endif
}

int Camera::setExposureTime(int whichSide, float expTime)
{
    Sleep(10);

#ifdef _WIN32
    int retCode= eSPAEAWB_SetExposureTime(whichSide, expTime);
    return retCode;

#elif __APPLE__
    //convert to the time to integer
    unsigned short mytime = (unsigned short)(expTime * 20);
    
    unsigned char data[6];
    
    read_ADDR_85(dev_handle);
    data[0] = 0x20;
    data[1] = 0xa0;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0x20;
    data[1] = 0xa1;
    data[2] = 0x3b;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa2;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa3;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa4;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa5;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0x20;
    data[1] = 0xa0;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0x20;
    data[1] = 0xa1;
    data[2] = 0xe0;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa2;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa2;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa3;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa4;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa5;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0x20;
    data[1] = 0xa0;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0x20;
    data[1] = 0xa1;
    data[2] = 0xe0;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa2;
    data[2] = 0xe0;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa3;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0x20;
    data[1] = 0xa0;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    
    read_ADDR_85(dev_handle);
    data[0] = 0x20;
    data[1] = 0xa1;
    data[2] = 0x33;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa2;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa3;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0x20;
    data[1] = 0xa0;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0x20;
    data[1] = 0xa1;
    data[2] = 0x37;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa2;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa3;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa4;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa5;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    unsigned char byte0 = (unsigned char)(mytime&0x00ff);
    unsigned char byte1 = (unsigned char)(mytime>>8);
    
    read_ADDR_85(dev_handle,0x0200);
    data[0] = 0x19;
    data[1] = 0x42;
    data[2] = 0x0f;
    data[3] = byte1;
    data[4] = 0x00;
    data[5] = 0x00;
    write_ADDR_01(dev_handle,data,0x0200,6);
    read_ADDR_85(dev_handle,0x0200);
    read_ADDR_81(dev_handle,0x0200,6);
    
    read_ADDR_85(dev_handle,0x0200);
    data[0] = 0x19;
    data[1] = 0x42;
    data[2] = 0x10;
    data[3] = byte0;
    data[4] = 0x00;
    data[5] = 0x00;
    write_ADDR_01(dev_handle,data,0x0200,6);
    read_ADDR_85(dev_handle,0x0200);
    read_ADDR_81(dev_handle,0x0200,6);
    
    read_ADDR_85(dev_handle,0x0200);
    data[0] = 0x19;
    data[1] = 0x42;
    data[2] = 0x2e;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    write_ADDR_01(dev_handle,data,0x0200,6);
    read_ADDR_85(dev_handle,0x0200);
    read_ADDR_81(dev_handle,0x0200,6);
    
    read_ADDR_85(dev_handle,0x0200);
    data[0] = 0x19;
    data[1] = 0x42;
    data[2] = 0x2d;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    write_ADDR_01(dev_handle,data,0x0200,6);
    read_ADDR_85(dev_handle,0x0200);
    read_ADDR_81(dev_handle,0x0200,6);
    
    return 1;
#endif
}

float Camera::getExposureTime(int whichSide)
{
    Sleep(10);

#ifdef _WIN32
    float eTime = -1.0;
    eSPAEAWB_GetExposureTime(whichSide, &eTime);
    return eTime;

#elif __APPLE__
    unsigned char data[6];
    
    read_ADDR_85(dev_handle);
    data[0] = 0x20;
    data[1] = 0xa0;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0x20;
    data[1] = 0xa1;
    data[2] = 0x3b;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa2;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa3;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa4;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa5;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0x20;
    data[1] = 0xa0;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0x20;
    data[1] = 0xa1;
    data[2] = 0xe0;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa2;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0xa3;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle);
    
    read_ADDR_85(dev_handle,0x0200);
    data[0] = 0x99;
    data[1] = 0x42;
    data[2] = 0x0f;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    
    write_ADDR_01(dev_handle,data,0x0200,6);
    read_ADDR_85(dev_handle,0x0200);
    unsigned short temp1 =(unsigned short)read_ADDR_81(dev_handle,0x0200,6);
    
    read_ADDR_85(dev_handle,0x0200);
    data[0] = 0x99;
    data[1] = 0x42;
    data[2] = 0x10;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    
    write_ADDR_01(dev_handle,data,0x0200);
    read_ADDR_85(dev_handle,0x0200);
    unsigned short temp2 = (unsigned short)read_ADDR_81(dev_handle,0x0200,6);
    
    unsigned short tempdata = 0;
    tempdata +=temp1;
    tempdata = tempdata << 8;
    tempdata += temp2;
    float toReturn = (float)tempdata;
    toReturn /=90.08;
    return toReturn;
#endif
}

int Camera::setGlobalGain(int whichSide, float gain)
{
    Sleep(10);

#ifdef _WIN32
    return eSPAEAWB_SetGlobalGain(whichSide, gain);

#elif __APPLE__
    read_ADDR_85(dev_handle,0x0200);
    unsigned char data[6];
    int toSet = (int)(gain*7.75);
    if (toSet > 255)
        toSet = 255;
    data[0] = 0x19;
    data[1] = 0x42;
    data[2] = 0x00;
    data[3] = toSet;
    data[4] = 0x00;
    data[5] = 0x00;
    write_ADDR_01(dev_handle,data,0x0200,6);
    read_ADDR_85(dev_handle,0x0200);
    read_ADDR_81(dev_handle,0x0200,6);
    
    return 1;
#endif
}

float Camera::getGlobalGain(int whichSide)
{
    Sleep(10);

#ifdef _WIN32
    float globalGain = -1.0;
    eSPAEAWB_GetGlobalGain(whichSide, &globalGain);
    return globalGain;

#elif __APPLE__
    read_ADDR_85(dev_handle,0x0200);
    unsigned char data[6];
    data[0] = 0x99;
    data[1] = 0x42;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    write_ADDR_01(dev_handle,data,0x0200,6);
    read_ADDR_85(dev_handle,0x0200);
    unsigned char gain =read_ADDR_81(dev_handle,0x0200,6);
    printf("gain = %x",gain);
    float toReturn = (float)gain;
    return toReturn/7.75;
#endif
}

int Camera::turnLEDsOn()
{
    Sleep(10);

#ifdef _WIN32
    BYTE gpio_code;
    int retCode = eSPAEAWB_GetGPIOValue(1, &gpio_code);
    gpio_code |= 0x08;
    retCode = eSPAEAWB_SetGPIOValue(1, gpio_code);
    return retCode;

#elif __APPLE__
    //libusb_transfer transfer ={0};
    
    unsigned char data[5];
    
    read_ADDR_85(dev_handle);
    
    data[0]=0x82;
    data[1]=0xf0;
    data[2]=0x17;
    data[3]=0x00;
    
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle,data);
    
    data[0]=0x02;
    data[1]=0xf0;
    data[2]=0x17;
    data[3]=0x1D;
    
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle,data);
    return 1;
#endif
}

int Camera::turnLEDsOff()
{
    Sleep(10);

#ifdef _WIN32
    BYTE gpio_code;
    int retCode = eSPAEAWB_GetGPIOValue(1, &gpio_code);
    gpio_code &= 0xf7;
    retCode = eSPAEAWB_SetGPIOValue(1, gpio_code);
    return retCode;

#elif __APPLE__
    unsigned char data[5];
    
    read_ADDR_85(dev_handle);
    data[0]=0x82;
    data[1]=0xf0;
    data[2]=0x17;
    data[3]=0x00;
    write_ADDR_01(dev_handle,data);
    
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle,data);
    read_ADDR_85(dev_handle);
    
    data[0]=0x02;
    data[1]=0xf0;
    data[2]=0x17;
    data[3]=0x15;
    
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle,data);
    return 1;
#endif
}

int Camera::getAccelerometerValues(int *x, int *y, int *z)
{
#ifdef _WIN32
    return eSPAEAWB_GetAccMeterValue(x, y, z);

#elif __APPLE__
    read_ADDR_85(dev_handle);
    unsigned char data[4];
    data[0] = 0xa0;
    data[1] = 0x80;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    unsigned char acc_x_1 =read_ADDR_81(dev_handle);
    
    // printf("val 0 = %u\n", acc_x_1);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0x81;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    /*unsigned char acc_x_2 =*/read_ADDR_81(dev_handle);
    
    //  printf("val 1 = %u\n", acc_x_2);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0x82;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    unsigned char acc_y_1 =read_ADDR_81(dev_handle);
    
    //  printf("val 2 = %u\n", acc_y_1);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0x83;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    /*unsigned char acc_y_2=*/read_ADDR_81(dev_handle);
    
    //printf("val 3 = %u\n", acc_y_2);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0x84;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    unsigned char acc_z_1 =read_ADDR_81(dev_handle);
    //printf("val 4 = %u\n", acc_z_1);
    
    read_ADDR_85(dev_handle);
    data[0] = 0xa0;
    data[1] = 0x85;
    data[2] = 0x00;
    data[3] = 0x00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    /*unsigned char acc_z_2 =*/read_ADDR_81(dev_handle);
    
    //printf("val 6 = %u\n", acc_z_2);
    
    *x= (int)acc_x_1;
    *y= (int)acc_y_1;
    *z= (int)acc_z_1;
    
    return 1;
#endif
}

int	Camera::setColorGains(int whichSide, float red, float green, float blue)
{
    Sleep(10);

#ifdef _WIN32
    return eSPAEAWB_SetColorGain(whichSide, red, green, blue);

#elif __APPLE__
    unsigned char data[4];
    read_ADDR_85(dev_handle);
    
    //set red
    int toSet = (red *64);
    if (toSet<0)toSet =0;
    if (toSet>255)toSet = 255;
    data[0] = 0x02;
    data[1] = 0xf1;
    data[2] = 0xcc;
    data[3] = (unsigned char)toSet;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle,data);
    
    data[0] = 0x02;
    data[1] = 0xf2;
    data[2] = 0x8c;
    data[3] = (unsigned char)toSet;
    read_ADDR_85(dev_handle);
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle,data);
    
    /// green
    int toSet2 = (green *64);
    if (toSet2<0)toSet2 =0;
    if (toSet2>255)toSet2 = 255;
    data[0] = 0x02;
    data[1] = 0xf1;
    data[2] = 0xcd;
    data[3] = (unsigned char)toSet2;
    read_ADDR_85(dev_handle);
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle,data);
    
    data[0] = 0x02;
    data[1] = 0xf2;
    data[2] = 0x8d;
    data[3] = (unsigned char)toSet2;
    read_ADDR_85(dev_handle);
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle,data);
    
    data[0] = 0x02;
    data[1] = 0xf1;
    data[2] = 0xce;
    data[3] = (unsigned char)toSet2;
    read_ADDR_85(dev_handle);
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle,data);
    
    data[0] = 0x02;
    data[1] = 0xf2;
    data[2] = 0x8e;
    data[3] = (unsigned char)toSet2;
    read_ADDR_85(dev_handle);
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle,data);
    
    toSet2 = (green *64);
    if (toSet2<0)toSet2 =0;
    if (toSet2>255)toSet2 = 255;
    data[0] = 0x02;
    data[1] = 0xf1;
    data[2] = 0xce;
    data[3] = (unsigned char)toSet2;
    read_ADDR_85(dev_handle);
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle,data);
    
    //blue side 1
    int toSet3 = (blue *64);
    if (toSet3<0)toSet3 =0;
    if (toSet3>255)toSet3 = 255;
    data[0] = 0x02;
    data[1] = 0xf1;
    data[2] = 0xcf;
    data[3] = (unsigned char)toSet3;
    read_ADDR_85(dev_handle);
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle,data);
    
    data[0] = 0x02;
    data[1] = 0xf2;
    data[2] = 0x8f;
    data[3] = (unsigned char)toSet3;
    read_ADDR_85(dev_handle);
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle,data);
    
    return 1;
#endif
}

int	Camera::getColorGains(int whichSide, float *red, float *green, float * blue)
{
    Sleep(10);

#ifdef _WIN32
    return eSPAEAWB_GetColorGain(whichSide, red, green, blue);

#elif __APPLE__
    unsigned char data[4];
    read_ADDR_85(dev_handle);
    
    data[0] = 0x82;
    data[1] = 0xf1;
    data[2] = 0xcc;
    data[3] = 0X00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle,data);
    float mred=(float)data[1];
    *red = mred/64;
    
    
    //  printf("red data = %x\n",data[1]);
    
    read_ADDR_85(dev_handle);
    data[0] = 0x82;
    data[1] = 0xf1;
    data[2] = 0xcd;
    data[3] = 0X00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle,data);
    //  printf("green data = %x\n",data[1]);
    float mgreen= (float)data[1];
    *green = mgreen/64;
    
    read_ADDR_85(dev_handle);
    data[0] = 0x82;
    data[1] = 0xf1;
    data[2] = 0xcf;
    data[3] = 0X00;
    write_ADDR_01(dev_handle,data);
    read_ADDR_85(dev_handle);
    read_ADDR_81(dev_handle,data);
    //   printf("blue data = %x\n",data[1]);
    float mblue = (float)data[1];
    *blue = mblue/64;
    return 1;
#endif
}

int Camera::enableAutoExposure(int whichSide)
{
    Sleep(10);

#ifdef _WIN32
    eSPAEAWB_SelectDevice(whichSide);
    return eSPAEAWB_EnableAE();

#elif __APPLE__
    int r;
    //libusb_transfer transfer ={0};
    //unsigned char endpoint = 0x83;
    unsigned char bmRequestType_set = 0x21;
    unsigned char bmRequestType_get = 0xa1;
    unsigned char GET_CUR = 0x85;
    unsigned char SET_CUR = 0x01;
    unsigned short wValue =0x0300;
    unsigned short wIndex = 0x0400;
    unsigned char data[5];
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    
    r = libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    data[0] = 0x20;
    data[1] = 0xa0;
    data[2] = 0x00;
    data[3] = 0x00;
    r= libusb_control_transfer(dev_handle,bmRequestType_set,SET_CUR,wValue,wIndex,data,4,10000 );
    console_log("set returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x81;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,4,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x85;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    
    data[0] = 0x20;
    data[1] = 0xa1;
    data[2] = 0x23;
    data[3] = 0x00;
    r= libusb_control_transfer(dev_handle,bmRequestType_set,SET_CUR,wValue,wIndex,data,4,10000 );
    console_log("set returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x81;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,4,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x85;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    data[0] = 0x20;
    data[1] = 0xa2;
    data[2] = 0x01;
    data[3] = 0x00;
    r= libusb_control_transfer(dev_handle,bmRequestType_set,SET_CUR,wValue,wIndex,data,4,10000 );
    console_log("set returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x85;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x81;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,4,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    return 1;
#endif
}

int Camera::disableAutoExposure(int whichSide)
{
    Sleep(10);

#ifdef _WIN32
    eSPAEAWB_SelectDevice(whichSide);
    return eSPAEAWB_DisableAE();

#elif __APPLE__
    //unsigned char endpoint = 0x83;
    unsigned char bmRequestType_set = 0x21;
    unsigned char bmRequestType_get = 0xa1;
    unsigned char GET_CUR = 0x85;
    unsigned char SET_CUR = 0x01;
    unsigned short wValue =0x0300;
    unsigned short wIndex = 0x0400;
    unsigned char data[5];
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    
    int r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    data[0] = 0x20;
    data[1] = 0xa0;
    data[2] = 0x00;
    data[3] = 0x00;
    r= libusb_control_transfer(dev_handle,bmRequestType_set,SET_CUR,wValue,wIndex,data,4,10000 );
    console_log("set returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x81;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,4,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x85;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    
    data[0] = 0x20;
    data[1] = 0xa1;
    data[2] = 0x23;
    data[3] = 0x00;
    r= libusb_control_transfer(dev_handle,bmRequestType_set,SET_CUR,wValue,wIndex,data,4,10000 );
    console_log("set returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x81;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,4,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x85;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    data[0] = 0x20;
    data[1] = 0xa2;
    data[2] = 0x00;
    data[3] = 0x00;
    r= libusb_control_transfer(dev_handle,bmRequestType_set,SET_CUR,wValue,wIndex,data,4,10000 );
    console_log("set returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x85;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x81;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,4,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    return 1;
#endif
}

int Camera::enableAutoWhiteBalance(int whichSide)
{
    Sleep(10);

#ifdef _WIN32
    eSPAEAWB_SelectDevice(whichSide);
    return eSPAEAWB_EnableAWB();

#elif __APPLE__
    //unsigned char endpoint = 0x83;
    unsigned char bmRequestType_set = 0x21;
    unsigned char bmRequestType_get = 0xa1;
    unsigned char GET_CUR = 0x85;
    unsigned char SET_CUR = 0x01;
    unsigned short wValue =0x0300;
    unsigned short wIndex = 0x0400;
    unsigned char data[5];
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    
    int r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    data[0] = 0x20;
    data[1] = 0xa8;
    data[2] = 0x00;
    data[3] = 0x00;
    r= libusb_control_transfer(dev_handle,bmRequestType_set,SET_CUR,wValue,wIndex,data,4,10000 );
    console_log("set returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x81;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,4,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x85;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    
    data[0] = 0x20;
    data[1] = 0xa9;
    data[2] = 0x0C;
    data[3] = 0x00;
    r= libusb_control_transfer(dev_handle,bmRequestType_set,SET_CUR,wValue,wIndex,data,4,10000 );
    console_log("set returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x81;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,4,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x85;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    data[0] = 0x20;
    data[1] = 0xaa;
    data[2] = 0x01;
    data[3] = 0x00;
    r= libusb_control_transfer(dev_handle,bmRequestType_set,SET_CUR,wValue,wIndex,data,4,10000 );
    console_log("set returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x85;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x81;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,4,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    return 1;
#endif
}

int Camera::disableAutoWhiteBalance(int whichSide)
{
    Sleep(10);

#ifdef _WIN32
    eSPAEAWB_SelectDevice(whichSide);
    return eSPAEAWB_DisableAWB();

#elif __APPLE__
    //unsigned char endpoint = 0x83;
    unsigned char bmRequestType_set = 0x21;
    unsigned char bmRequestType_get = 0xa1;
    unsigned char GET_CUR = 0x85;
    unsigned char SET_CUR = 0x01;
    unsigned short wValue =0x0300;
    unsigned short wIndex = 0x0400;
    unsigned char data[5];
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    
    int r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    data[0] = 0x20;
    data[1] = 0xa8;
    data[2] = 0x00;
    data[3] = 0x00;
    r= libusb_control_transfer(dev_handle,bmRequestType_set,SET_CUR,wValue,wIndex,data,4,10000 );
    console_log("set returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x81;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,4,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x85;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    
    data[0] = 0x20;
    data[1] = 0xa9;
    data[2] = 0x0C;
    data[3] = 0x00;
    r= libusb_control_transfer(dev_handle,bmRequestType_set,SET_CUR,wValue,wIndex,data,4,10000 );
    console_log("set returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x81;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,4,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x85;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    data[0] = 0x20;
    data[1] = 0xaa;
    data[2] = 0x00;
    data[3] = 0x00;
    r= libusb_control_transfer(dev_handle,bmRequestType_set,SET_CUR,wValue,wIndex,data,4,10000 );
    console_log("set returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x85;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,2,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<2; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    GET_CUR = 0x81;
    r= libusb_control_transfer(dev_handle,bmRequestType_get,GET_CUR,wValue,wIndex,data,4,10000 );
    console_log("get returned " + to_string(r));
    for (int i = 0;i<4; i++){
        printf("data[%d] = %x\n",i,data[i]);
    }
    return 1;
#endif
}

#ifdef _WIN32
void Camera::YUY2_to_RGB24_Microsoft(BYTE *pSrc, BYTE *pDst, int cx, int cy)
{
    int nSrcBPS, nDstBPS, x, y, x2, x3, m;
    int ma0, mb0, m02, m11, m12, m21;
    BYTE *pS0, *pD0;
    int Y, U, V, Y2;
    BYTE R, G, B, R2, G2, B2;
    
    nSrcBPS = cx * 2;
    nDstBPS = ((cx * 3 + 3) / 4) * 4;
    
    pS0 = pSrc;
    pD0 = pDst + nDstBPS*(cy - 1);
    for (y = 0; y<cy; y++)
    {
        for (x3 = 0, x2 = 0, x = 0; x<cx; x += 2, x2 += 4, x3 += 6)
        {
            Y = (int)pS0[x2 + 0] - 16;
            Y2 = (int)pS0[x2 + 2] - 16;
            U = (int)pS0[x2 + 1] - 128;
            V = (int)pS0[x2 + 3] - 128;
            //
            ma0 = 298 * Y;
            mb0 = 298 * Y2;
            m02 = 409 * V + 128;
            m11 = -100 * U;
            m12 = -208 * V + 128;
            m21 = 516 * U + 128;
            //
            m = (ma0 + m02) >> 8;
            R = (m<0) ? (0) : (m>255) ? (255) : ((BYTE)m);
            m = (ma0 + m11 + m12) >> 8;
            G = (m<0) ? (0) : (m>255) ? (255) : ((BYTE)m);
            m = (ma0 + m21) >> 8;
            B = (m<0) ? (0) : (m>255) ? (255) : ((BYTE)m);
            //
            m = (mb0 + m02) >> 8;
            R2 = (m<0) ? (0) : (m>255) ? (255) : ((BYTE)m);
            m = (mb0 + m11 + m12) >> 8;
            G2 = (m<0) ? (0) : (m>255) ? (255) : ((BYTE)m);
            m = (mb0 + m21) >> 8;
            B2 = (m<0) ? (0) : (m>255) ? (255) : ((BYTE)m);
            //
            pD0[x3] = B;
            pD0[x3 + 1] = G;
            pD0[x3 + 2] = R;
            pD0[x3 + 3] = B2;
            pD0[x3 + 4] = G2;
            pD0[x3 + 5] = R2;
        }
        pS0 += nSrcBPS;
        pD0 -= nDstBPS;
    }
}
#endif