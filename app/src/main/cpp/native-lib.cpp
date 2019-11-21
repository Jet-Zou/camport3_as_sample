#include <jni.h>
#include <string>

#include "common/common.hpp"
#include <opencv2/opencv.hpp>


//#define VIRTUAL_CAM_TEST

struct CallbackData {
    int             index;
    TY_DEV_HANDLE   hDevice;
};

static TY_DEV_HANDLE hDevice;
static TY_INTERFACE_HANDLE hIface;
static CallbackData cb_data;
static char* frameBuffer[2];
static cv::Mat *t, *t_gray, *t_color;

static int fps_counter = 0;
static clock_t fps_tm = 0;

#ifdef VIRTUAL_CAM_TEST
#define VIR_DEPTH_IMG_WIDTH     (640)
#define VIR_DEPTH_IMG_HEIGHT    (480)
#define VIR_COLOR_IMG_WIDTH     (640)
#define VIR_COLOR_IMG_HEIGHT    (480)
static int frm_idx = 0;
static cv::Mat vir_depth[3];
static cv::Mat vir_color[3];
#endif

int get_fps() {
    const int kMaxCounter = 20;
    struct timeval start;
    fps_counter++;
    if (fps_counter < kMaxCounter) {
        return -1;
    }

    gettimeofday(&start, NULL);
    int elapse = start.tv_sec * 1000 + start.tv_usec / 1000 - fps_tm;
    int v = (int)(((float)fps_counter) / elapse * 1000);
    gettimeofday(&start, NULL);
    fps_tm = start.tv_sec * 1000 + start.tv_usec / 1000;

    fps_counter = 0;
    return v;
}

void eventCallback(TY_EVENT_INFO *event_info, void *userdata)
{
    if (event_info->eventId == TY_EVENT_DEVICE_OFFLINE) {
        LOGD("=== Event Callback: Device Offline!");
    }
    else if (event_info->eventId == TY_EVENT_LICENSE_ERROR) {
        LOGD("=== Event Callback: License Error!");
    }
}

#ifndef VIRTUAL_CAM_TEST
void frameHandler(TY_FRAME_DATA frame, void* userdata, jlong mat, jlong mat_gray, jlong mat_color)
{
    CallbackData* pData = (CallbackData*) userdata;
    LOGD("=== Get frame %d", ++pData->index);

    int fps = get_fps();
    if (fps > 0){
        LOGD("fps: %d", fps);
    }

    cv::Mat depth, irl, irr, color;
    parseFrame(frame, &depth, &irl, &irr, &color);

    if(!depth.empty()){
        //    cv::Mat colorDepth = pData->render->Compute(depth);
        t = (cv::Mat*)mat;
        depth.copyTo(*t);

        t_gray = (cv::Mat*)mat_gray;
        for(int i = 0; i < depth.cols; i++) {
            for(int j = 0; j < depth.rows; j++) {
                unsigned short val = depth.ptr<unsigned short>(j)[i];

                val = ((20 * val)>> 8);
                t_gray->ptr<unsigned char>(j)[i] = (unsigned char)val;
            }
        }
    }

    if (!color.empty()) {
        t_color = (cv::Mat*)mat_color;
        color.copyTo(*t_color);
    }

    LOGD("=== Callback: Re-enqueue buffer(%p, %d)", frame.userBuffer, frame.bufferSize);
    ASSERT_OK( TYEnqueueBuffer(pData->hDevice, frame.userBuffer, frame.bufferSize) );
}
#endif

#if 0
extern "C" JNIEXPORT jstring JNICALL
Java_com_example_camport3_1android_1demo_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";

    //cv::Mat depth = cv::Mat(640, 480, CV_16U);
    return env->NewStringUTF(hello.c_str());
}
#endif

int OpenDevice() {

#ifdef VIRTUAL_CAM_TEST
    for(int i = 0; i < 3; i++) {
        vir_depth[i] = cv::Mat(VIR_DEPTH_IMG_HEIGHT, VIR_DEPTH_IMG_WIDTH, CV_16U);
        vir_color[i] = cv::Mat(VIR_COLOR_IMG_HEIGHT, VIR_COLOR_IMG_WIDTH, CV_8UC3);
    }

    //color Test image
    for(int i=0;i<vir_color[0].rows;i++)
    {
        for(int j=0;j<vir_color[0].cols;j++)
        {
            vir_color[0].at<cv::Vec3b>(i,j)[0]=255;//蓝色通道
            vir_color[0].at<cv::Vec3b>(i,j)[1]=0;//红色通道
            vir_color[0].at<cv::Vec3b>(i,j)[2]=0;//绿色通道
        }
    }

    for(int i=0;i<vir_color[1].rows;i++)
    {
        for(int j=0;j<vir_color[1].cols;j++)
        {
            vir_color[1].at<cv::Vec3b>(i,j)[0]=0;//蓝色通道
            vir_color[1].at<cv::Vec3b>(i,j)[1]=255;//红色通道
            vir_color[1].at<cv::Vec3b>(i,j)[2]=0;//绿色通道
        }
    }

    for(int i=0;i<vir_color[2].rows;i++)
    {
        for(int j=0;j<vir_color[2].cols;j++)
        {
            vir_color[2].at<cv::Vec3b>(i,j)[0]=0;//蓝色通道
            vir_color[2].at<cv::Vec3b>(i,j)[1]=0;//红色通道
            vir_color[2].at<cv::Vec3b>(i,j)[2]=255;//绿色通道
        }
    }

    //depth Test image
    for(int i=0;i<vir_depth[0].rows;i++)
    {
        for(int j=0;j<vir_depth[0].cols;j++)
        {
            vir_depth[0].at<ushort>(i,j)=0;//
        }
    }

    for(int i=0;i<vir_color[1].rows;i++)
    {
        for(int j=0;j<vir_color[1].cols;j++)
        {
            vir_depth[1].at<ushort>(i,j)=0x7f7f;//
        }
    }

    for(int i=0;i<vir_depth[2].rows;i++)
    {
        for(int j=0;j<vir_depth[2].cols;j++)
        {
            vir_depth[2].at<ushort>(i,j)=0xffff;
        }
    }


#else
    std::string ID, IP;
    TY_STATUS status = TY_STATUS_OK;

    LOGD("Init lib");
    status = TYInitLib();
    if(status) {
        LOGD("Init lib failed， status = %d!", status);
        return status;
    }
    TY_VERSION_INFO ver;
    status = TYLibVersion(&ver);
    if(status) {
        TYDeinitLib();
        return status;
    }

    LOGD("     - lib version: %d.%d.%d", ver.major, ver.minor, ver.patch);

    std::vector<TY_DEVICE_BASE_INFO> selected;
    status = selectDevice(TY_INTERFACE_USB, ID, IP, 1, selected);
    if(status) {
        LOGD("selectDevice failed!");
        return status;
    }

    ASSERT(selected.size() > 0);
    TY_DEVICE_BASE_INFO& selectedDev = selected[0];


    status = TYOpenInterface(selectedDev.iface.id, &hIface) ;

    status = TYOpenDevice(hIface, selectedDev.id, &hDevice) ;

    int32_t allComps;
    status = TYGetComponentIDs(hDevice, &allComps) ;
    if(allComps & TY_COMPONENT_RGB_CAM) {
        LOGD("Has RGB camera, open RGB cam");
        status = TYEnableComponents(hDevice, TY_COMPONENT_RGB_CAM) ;
    }

    if (allComps & TY_COMPONENT_IR_CAM_LEFT) {
        LOGD("Has IR left camera, open IR left cam");
        status = TYEnableComponents(hDevice, TY_COMPONENT_IR_CAM_LEFT);
    }

    if (allComps & TY_COMPONENT_IR_CAM_RIGHT) {
        LOGD("Has IR right camera, open IR right cam");
        status = TYEnableComponents(hDevice, TY_COMPONENT_IR_CAM_RIGHT);
    }

    LOGD("Configure components, open depth cam");
    if (allComps & TY_COMPONENT_DEPTH_CAM ) {
        std::vector<TY_ENUM_ENTRY> image_mode_list;
        status = get_feature_enum_list(hDevice, TY_COMPONENT_DEPTH_CAM, TY_ENUM_IMAGE_MODE, image_mode_list);
        for (int idx = 0; idx < image_mode_list.size(); idx++){
            TY_ENUM_ENTRY &entry = image_mode_list[idx];
            //try to select a vga resolution
            if (TYImageWidth(entry.value) == 640){
                LOGD("Select Depth Image Mode: %s", entry.description);
                int err = TYSetEnum(hDevice, TY_COMPONENT_DEPTH_CAM, TY_ENUM_IMAGE_MODE, entry.value);
                ASSERT(err == TY_STATUS_OK || err == TY_STATUS_NOT_PERMITTED);
                status = TYEnableComponents(hDevice, TY_COMPONENT_DEPTH_CAM) ;
                break;
            } else {
                LOGD("640x480 is not work for depth, disabled depth component");
                status = TYDisableComponents(hDevice, TY_COMPONENT_DEPTH_CAM) ;
            }
        }
    }

    LOGD("Configure components, open rgb cam");
    if (allComps & TY_COMPONENT_RGB_CAM) {
        std::vector<TY_ENUM_ENTRY> image_mode_list;
        status = get_feature_enum_list(hDevice, TY_COMPONENT_RGB_CAM, TY_ENUM_IMAGE_MODE, image_mode_list);
        for (int idx = 0; idx < image_mode_list.size(); idx++){
            TY_ENUM_ENTRY &entry = image_mode_list[idx];
            //try to select a vga resolution
            if (TYImageWidth(entry.value) == 640){
                LOGD("Select RGB Image Mode: %s", entry.description);
                int err = TYSetEnum(hDevice, TY_COMPONENT_RGB_CAM, TY_ENUM_IMAGE_MODE, entry.value);
                ASSERT(err == TY_STATUS_OK || err == TY_STATUS_NOT_PERMITTED);
                status = TYEnableComponents(hDevice, TY_COMPONENT_RGB_CAM) ;
                break;
            } else {
                LOGD("640x480 is not work for RGB, disabled RGB component");
                status = TYDisableComponents(hDevice, TY_COMPONENT_RGB_CAM) ;
            }
        }
    }

    LOGD("Prepare image buffer");
    uint32_t frameSize;
    ASSERT_OK( TYGetFrameBufferSize(hDevice, &frameSize) );
    LOGD("     - Get size of framebuffer, %d", frameSize);
    ASSERT( frameSize >= 640 * 480 * 2 );

    LOGD("     - Allocate & enqueue buffers");
    char* frameBuffer[2];
    frameBuffer[0] = new char[frameSize];
    frameBuffer[1] = new char[frameSize];
    LOGD("     - Enqueue buffer (%p, %d)", frameBuffer[0], frameSize);
    ASSERT_OK( TYEnqueueBuffer(hDevice, frameBuffer[0], frameSize) );
    LOGD("     - Enqueue buffer (%p, %d)", frameBuffer[1], frameSize);
    ASSERT_OK( TYEnqueueBuffer(hDevice, frameBuffer[1], frameSize) );

    cb_data.index = 0;
    cb_data.hDevice = hDevice;

    LOGD("Register event callback");
    ASSERT_OK(TYRegisterEventCallback(hDevice, eventCallback, NULL));

    bool hasTrigger;
    ASSERT_OK(TYHasFeature(hDevice, TY_COMPONENT_DEVICE, TY_STRUCT_TRIGGER_PARAM, &hasTrigger));
    if (hasTrigger) {
        LOGD("Disable trigger mode");
        TY_TRIGGER_PARAM trigger;
        trigger.mode = TY_TRIGGER_MODE_OFF;
        ASSERT_OK(TYSetStruct(hDevice, TY_COMPONENT_DEVICE, TY_STRUCT_TRIGGER_PARAM, &trigger, sizeof(trigger)));
    }

    LOGD("Start capture");
    ASSERT_OK( TYStartCapture(hDevice) );

    LOGD("open device successful");
#endif
    return 0;
}

int CloseDevice() {
#ifdef VIRTUAL_CAM_TEST

#else
    ASSERT_OK(TYStopCapture(hDevice));
    ASSERT_OK(TYCloseDevice(hDevice));
    ASSERT_OK(TYCloseInterface(hIface));
    ASSERT_OK(TYDeinitLib());
    delete frameBuffer[0];
    delete frameBuffer[1];
    LOGD("CloseDevice finished....");
#endif
    return 0;
}


extern "C" {
JNIEXPORT int JNICALL  Java_com_example_camport3_1android_1demo_MainActivity_OpenDevice(JNIEnv* env, jobject thiz);
JNIEXPORT void JNICALL Java_com_example_camport3_1android_1demo_MainActivity_CloseDevice(JNIEnv* env, jobject thiz);
JNIEXPORT void JNICALL Java_com_example_camport3_1android_1demo_MainActivity_FetchData(JNIEnv* env, jobject thiz, jlong matAddr, jlong matAddr_gray, jlong matAddr_color);
}

JNIEXPORT int JNICALL Java_com_example_camport3_1android_1demo_MainActivity_OpenDevice(JNIEnv* env, jobject thiz)
{
    return OpenDevice();
}

JNIEXPORT void JNICALL Java_com_example_camport3_1android_1demo_MainActivity_CloseDevice(JNIEnv* env, jobject thiz)
{
    CloseDevice();
}

JNIEXPORT void JNICALL Java_com_example_camport3_1android_1demo_MainActivity_FetchData(JNIEnv* env, jobject thiz, jlong matAddr, jlong matAddr_gray, jlong matAddr_color)
{
    LOGD("Fetch Data");
    LOGD("=== While loop to fetch frame");
#ifdef VIRTUAL_CAM_TEST
    int idx = frm_idx % 3;
    t = (cv::Mat*)matAddr;
    t_gray = (cv::Mat*)matAddr_gray;

    //copy depth image to java
    vir_depth[idx].copyTo(*t);

    //set depth image data to y image
    for(int i = 0; i < vir_depth[idx].cols; i++) {
        for(int j = 0; j < vir_depth[idx].rows; j++) {
            unsigned short val = vir_depth[idx].ptr<unsigned short>(j)[i];

            t_gray->ptr<unsigned char>(j)[i] = (unsigned char)val;
        }
    }

    //copy rgb image to java
    t_color = (cv::Mat*)matAddr_color;
    vir_color[idx].copyTo(*t_color);

    frm_idx++;

    usleep(500 * 1000);
#else
    TY_FRAME_DATA frame;

    /*
    struct timeval xTime;
    int xRet = gettimeofday(&xTime, NULL);
    long long xFactor = 1;
    long long now = (long long)(( xFactor * xTime.tv_sec * 1000) + (xTime.tv_usec / 1000));
    LOGD("sec_d = %d, sec_ld = %ld, sec_lld = %lld\n", xTime.tv_sec, xTime.tv_sec, xTime.tv_sec);
    LOGD("usec_d = %d, usec_ld = %ld, usec_lld = %lld\n", xTime.tv_usec, xTime.tv_usec, xTime.tv_usec);
    LOGD("now_d = %d, now_ld = %ld, now_ld = %lld\n", now, now, now);
    */

    int err = TYFetchFrame(hDevice, &frame, -1);
    LOGD("err = %d", err);
    if( err != TY_STATUS_OK ){
        LOGD("... Drop one frame");
    }

    /*
    LOGD("-----------------");
    xRet = gettimeofday(&xTime, NULL);
    xFactor = 1;
    now = (long long)(( xFactor * xTime.tv_sec * 1000) + (xTime.tv_usec / 1000));
    LOGD("sec_d = %d, sec_ld = %ld, sec_lld = %lld\n", xTime.tv_sec, xTime.tv_sec, xTime.tv_sec);
    LOGD("usec_d = %d, usec_ld = %ld, usec_lld = %lld\n", xTime.tv_usec, xTime.tv_usec, xTime.tv_usec);
    LOGD("now_d = %d, now_ld = %ld, now_ld = %lld\n", now, now, now);
    */

    frameHandler(frame, &cb_data, matAddr, matAddr_gray, matAddr_color);
#endif
}
