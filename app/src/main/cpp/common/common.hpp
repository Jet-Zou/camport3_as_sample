#ifndef SAMPLE_COMMON_COMMON_HPP_
#define SAMPLE_COMMON_COMMON_HPP_

#include "Utils.hpp"
#include <opencv2/opencv.hpp>
//#include "DepthRender.hpp"
//#include "MatViewer.hpp"
//#include "TYThread.hpp"
//#include "TyIsp.h"

static inline int parseFrame(const TY_FRAME_DATA& frame, cv::Mat* pDepth
                             , cv::Mat* pLeftIR, cv::Mat* pRightIR
                             , cv::Mat* pColor)
{
    for (int i = 0; i < frame.validCount; i++){
        // get depth image
        if (pDepth && frame.image[i].componentID == TY_COMPONENT_DEPTH_CAM){
            *pDepth = cv::Mat(frame.image[i].height, frame.image[i].width
                              , CV_16U, frame.image[i].buffer).clone();
        }
        // get left ir image
        if (pLeftIR && frame.image[i].componentID == TY_COMPONENT_IR_CAM_LEFT){
            *pLeftIR = cv::Mat(frame.image[i].height, frame.image[i].width
                               , CV_8U, frame.image[i].buffer).clone();
        }
        // get right ir image
        if (pRightIR && frame.image[i].componentID == TY_COMPONENT_IR_CAM_RIGHT){
            *pRightIR = cv::Mat(frame.image[i].height, frame.image[i].width
                                , CV_8U, frame.image[i].buffer).clone();
        }
        // get BGR
        if (pColor && frame.image[i].componentID == TY_COMPONENT_RGB_CAM){
            if (frame.image[i].pixelFormat == TY_PIXEL_FORMAT_JPEG){
                cv::Mat jpeg(frame.image[i].height, frame.image[i].width
                             , CV_8UC1, frame.image[i].buffer);
                *pColor = cv::imdecode(jpeg, CV_LOAD_IMAGE_COLOR);
            }
            if (frame.image[i].pixelFormat == TY_PIXEL_FORMAT_YVYU){
                cv::Mat yuv(frame.image[i].height, frame.image[i].width
                            , CV_8UC2, frame.image[i].buffer);
                cv::cvtColor(yuv, *pColor, cv::COLOR_YUV2RGB_YVYU);
            }
            else if (frame.image[i].pixelFormat == TY_PIXEL_FORMAT_YUYV){
                cv::Mat yuv(frame.image[i].height, frame.image[i].width
                            , CV_8UC2, frame.image[i].buffer);
                cv::cvtColor(yuv, *pColor, cv::COLOR_YUV2RGB_YUYV);
            }
            else if (frame.image[i].pixelFormat == TY_PIXEL_FORMAT_RGB){
                cv::Mat rgb(frame.image[i].height, frame.image[i].width
                            , CV_8UC3, frame.image[i].buffer);
                cv::cvtColor(rgb, *pColor, cv::COLOR_RGB2BGR);
            }
            else if (frame.image[i].pixelFormat == TY_PIXEL_FORMAT_BGR){
                *pColor = cv::Mat(frame.image[i].height, frame.image[i].width
                                  , CV_8UC3, frame.image[i].buffer);
            }
            else if (frame.image[i].pixelFormat == TY_PIXEL_FORMAT_BAYER8GB){
                cv::Mat raw(frame.image[i].height, frame.image[i].width
                        , CV_8U, frame.image[i].buffer);
                cv::cvtColor(raw, *pColor, cv::COLOR_BayerGB2BGR);
            }
            else if (frame.image[i].pixelFormat == TY_PIXEL_FORMAT_MONO){
                cv::Mat gray(frame.image[i].height, frame.image[i].width
                             , CV_8U, frame.image[i].buffer);
                cv::cvtColor(gray, *pColor, cv::COLOR_GRAY2BGR);
            }
        }
    }

    return 0;
}
#endif
