#include <jni.h>
#include <string>
#include "System.h"
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <time.h>
#include "Plane.h"
#include "Process.h"

#include <android/log.h>
#define  LOG_TAG    "native-dev"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define BOWISBIN

//ORB_SLAM2::System SLAM("/storage/emulated/0/ORBvoc.txt","/storage/emulated/0/TUM1.yaml",ORB_SLAM2::System::MONOCULAR,false);
#ifdef BOWISBIN
ORB_SLAM2::System SLAM("/storage/emulated/0/ORBvoc.bin","/storage/emulated/0/TUM1.yaml",ORB_SLAM2::System::MONOCULAR,false);
#endif
std::chrono::steady_clock::time_point t0;
double ttrack=0;


cv::Mat Plane2World=cv::Mat::eye(4,4,CV_32F);
cv::Mat Marker2World=cv::Mat::eye(4,4,CV_32F);
cv::Mat centroid;

bool load_as_text(ORB_SLAM2::ORBVocabulary* voc, const std::string infile) {
    clock_t tStart = clock();
    bool res = voc->loadFromTextFile(infile);
    LOGE("Loading fom text: %.2fs\n", (double)(clock() - tStart)/CLOCKS_PER_SEC);
    return res;
}

void save_as_binary(ORB_SLAM2::ORBVocabulary* voc, const std::string outfile) {
    clock_t tStart = clock();
    voc->saveToBinaryFile(outfile);
    LOGE("Saving as binary: %.2fs\n", (double)(clock() - tStart)/CLOCKS_PER_SEC);
}

void txt_2_bin(){
    ORB_SLAM2::ORBVocabulary* voc = new ORB_SLAM2::ORBVocabulary();

    load_as_text(voc, "/storage/emulated/0/ORBvoc.txt");
    save_as_binary(voc, "/storage/emulated/0/ORBvoc.bin");
}

cv::Point2f Camera2Pixel(cv::Mat poseCamera,cv::Mat mk){
    return Point2f(
            poseCamera.at<float>(0,0)/poseCamera.at<float>(2,0)*mk.at<float>(0,0)+mk.at<float>(0,2),
            poseCamera.at<float>(1,0)/poseCamera.at<float>(2,0)*mk.at<float>(1,1)+mk.at<float>(1,2)
    );
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ys_orbtest_OrbTest_CVTest(JNIEnv *env, jobject instance, jlong matAddr) {

#ifndef BOWISBIN
    if(ttrack == 0)
    txt_2_bin();
    ttrack++;
#else
    LOGE("Native Start");
    cv::Mat *pMat = (cv::Mat*)matAddr;

    std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
    ttrack = std::chrono::duration_cast < std::chrono::duration < double >> (t1 - t0).count();

    clock_t start,end;
    start=clock();
    cv::Mat pose = SLAM.TrackMonocular(*pMat,ttrack);
    end = clock();
    LOGE("Get Pose Use Time=%f\n",((double)end-start)/CLOCKS_PER_SEC);
    static bool instialized =false;
    static bool markerDetected =false;
    if(SLAM.MapChanged()){
        instialized = false;
        markerDetected =false;
    }

    if(!pose.empty()){
//        if(markerDetected == false){
//            Process process(pMat);
//            markerDetected = process.Run();
//            if(process.Run()){
//                cv::Mat rwc,twc,tempTwc=cv::Mat::eye(4,4,CV_32F);
//                rwc = pose.rowRange(0,3).colRange(0,3).t();
//                twc = -rwc*pose.col(3).rowRange(0,3);
//                rwc.copyTo(tempTwc.rowRange(0,3).colRange(0,3));
//                twc.copyTo(tempTwc.col(3).rowRange(0,3));
//                Marker2World = tempTwc*process.marker2origincamera;
//            }
//
//        }else{
//            vector<cv::Point3f> drawPoints(4);
//            drawPoints[0] = cv::Point3f(0,0,1);
//            drawPoints[1] = cv::Point3f(0.3,0,1);
//            drawPoints[2] = cv::Point3f(0,0.3,1);
//            drawPoints[3] = cv::Point3f(0,0,1.3);
//            cv::Mat Marker2CameraNow = pose*Marker2World;
//            cv::Mat rcm,tcm;
//            cv::Rodrigues(Marker2CameraNow.rowRange(0,3).colRange(0,3),rcm);
//            tcm = Marker2CameraNow.col(3).rowRange(0,3);
//            std::vector<cv::Point2f> markerPoints;
//            cv::projectPoints(drawPoints, rcm, tcm, SLAM.mpTracker->mK, SLAM.mpTracker->mDistCoef, markerPoints);
//
//            cv::line(*pMat, markerPoints[0],markerPoints[1], cv::Scalar(250, 0, 0), 5);
//            cv::line(*pMat, markerPoints[0],markerPoints[2], cv::Scalar(0, 250, 0), 5);
//            cv::line(*pMat, markerPoints[0],markerPoints[3], cv::Scalar(0, 0, 250), 5);
//        }




        cv::Mat rVec;
        cv::Rodrigues(pose.colRange(0, 3).rowRange(0, 3), rVec);
        cv::Mat tVec = pose.col(3).rowRange(0, 3);

        const vector<ORB_SLAM2::MapPoint*> vpMPs = SLAM.mpTracker->mpMap->GetAllMapPoints();
        const vector<ORB_SLAM2::MapPoint*> vpTMPs = SLAM.GetTrackedMapPoints();
        vector<cv::KeyPoint> vKPs = SLAM.GetTrackedKeyPointsUn();
//        for(int i=0; i<vKPs.size(); i++)
//        {
//            if(vpTMPs[i])
//            {
//                cv::circle(*pMat, vKPs[i].pt, 2, cv::Scalar(0, 255, 0), 1, 8);
//            }
//        }
//        if(vpTMPs.size() > 0){
//
//        }
        if (vpMPs.size() > 0) {
            std::vector<cv::Point3f> allmappoints;
            for (size_t i = 0; i < vpMPs.size(); i++) {
                if (vpMPs[i]) {
                    cv::Point3f pos = cv::Point3f(vpMPs[i]->GetWorldPos());
                    allmappoints.push_back(pos);
//                    LOGE("Point's world pose is %f %f %f",pos.x,pos.y,pos.z );
                }
            }
            LOGE("all map points size %d", allmappoints.size());
            std::vector<cv::Point2f> projectedPoints;
            cv::projectPoints(allmappoints, rVec, tVec, SLAM.mpTracker->mK, SLAM.mpTracker->mDistCoef, projectedPoints);
            for (size_t j = 0; j < projectedPoints.size(); ++j) {
                cv::Point2f r1 = projectedPoints[j];
                if(r1.x <640 && r1.x> 0 && r1.y >0 && r1.y <480)
                    cv::circle(*pMat, cv::Point(r1.x, r1.y), 2, cv::Scalar(0, 255, 0), 1, 8);
            }

            if(instialized == false){
                Plane mplane;
                cv::Mat tempTpw,rpw,rwp,tpw,twp;
                LOGE("Detect  Plane");
                tempTpw = mplane.DetectPlane(pose,vpTMPs,50);
                if(!tempTpw.empty()){
                    LOGE("Find  Plane");
                    rpw = tempTpw.rowRange(0,3).colRange(0,3);
                    for(int row = 0 ; row < 4;row++){
                        LOGE(" tempTpw %f %f %f %f",tempTpw.at<float>(row,0),tempTpw.at<float>(row,1),tempTpw.at<float>(row,2),tempTpw.at<float>(row,3));
                    }
                    tpw = tempTpw.col(3).rowRange(0,3);
                    rwp = rpw.t();
                    twp = -rwp*tpw;
                    rwp.copyTo(Plane2World.rowRange(0,3).colRange(0,3));
                    for(int row = 0 ; row < 3;row++){
                        LOGE(" rwp %f %f %f",rwp.at<float>(row,0),rwp.at<float>(row,1),rwp.at<float>(row,2));
                    }
                    twp.copyTo(Plane2World.col(3).rowRange(0,3));
                    for(int row = 0 ; row < 4;row++){
                        LOGE(" Plane2World %f %f %f %f",Plane2World.at<float>(row,0),Plane2World.at<float>(row,1),Plane2World.at<float>(row,2),Plane2World.at<float>(row,3));
                    }
                    centroid = mplane.o;
                    LOGE("Centroid is %f %f %f",mplane.o.at<float>(0,0),mplane.o.at<float>(1,0),mplane.o.at<float>(2,0));
                    instialized = true;
                    LOGE("Find  Plane");
                    Plane2World =tempTpw;
                }

            }else{
                cv::Mat Plane2Camera = pose*Plane2World;
//                vector<cv::Mat> axisPoints(4);
//                axisPoints[0] = (cv::Mat_ <float>(4,1)<<0,0,0,1);
//                axisPoints[1] = (cv::Mat_ <float>(4,1)<<0.3,0,0,1);
//                axisPoints[2] = (cv::Mat_ <float>(4,1)<<0,0.3,0,1);
//                axisPoints[3] = (cv::Mat_ <float>(4,1)<<0,0,0.3,1);
//                vector<cv::Point2f> drawPoints(4);
//                for(int i = 0 ; i < 4; i++){
//                    axisPoints[i] = Plane2Camera*axisPoints[i];
//
//                    drawPoints[i] = Camera2Pixel(axisPoints[i],SLAM.mpTracker->mK);
//                    LOGE("drawPoints x y %f %f",drawPoints[i].x,drawPoints[i].y);
//                }
//                LOGE("axisPoints x y %f %f %f",axisPoints[0].at<float>(0,0),axisPoints[0].at<float>(1,0),axisPoints[0].at<float>(2,0));
//                cv::line(*pMat, drawPoints[0],drawPoints[1], cv::Scalar(250, 0, 0), 5);
//                cv::line(*pMat, drawPoints[0],drawPoints[2], cv::Scalar(0, 250, 0), 5);
//                cv::line(*pMat, drawPoints[0],drawPoints[3], cv::Scalar(0, 0, 250), 5);

                vector<cv::Point3f> drawPoints(4);
                drawPoints[0] = cv::Point3f(0,0,0);
                drawPoints[1] = cv::Point3f(0.3,0.0,0.0);
                drawPoints[2] = cv::Point3f(0.0,0.3,0.0);
                drawPoints[3] = cv::Point3f(0.0,0.0,0.3);
//                for(int i = 0 ; i < 4 ;i ++){
//                    drawPoints[i].x += centroid.at<float>(0,0);
//                    drawPoints[i].y += centroid.at<float>(1,0);
//                    drawPoints[i].z += centroid.at<float>(2,0);
//                }

                for(int row = 0 ; row < 4;row++){
                    LOGE(" Plane2Camera %f %f %f %f",Plane2Camera.at<float>(row,0),Plane2Camera.at<float>(row,1),Plane2Camera.at<float>(row,2),Plane2Camera.at<float>(row,3));
                }
                cv::Mat Rcp ,Tcp;
                cv::Rodrigues(Plane2Camera.rowRange(0,3).colRange(0,3),Rcp);
                LOGE(" rwp %f %f %f",Rcp.at<float>(0,0),Rcp.at<float>(1,0),Rcp.at<float>(2,2));

                Tcp = Plane2Camera.col(3).rowRange(0,3);
                LOGE("Tcp %f %f %f",Tcp.at<float>(0,0),Tcp.at<float>(1,0),Tcp.at<float>(2,0));
//                cv::Rodrigues(pose.rowRange(0,3).colRange(0,3),Rcp);
//                Tcp = pose.col(3).rowRange(0,3);
                cv::projectPoints(drawPoints, Rcp, Tcp, SLAM.mpTracker->mK, SLAM.mpTracker->mDistCoef, projectedPoints);

                cv::line(*pMat, projectedPoints[0],projectedPoints[1], cv::Scalar(250, 0, 0), 5);
                cv::line(*pMat, projectedPoints[0],projectedPoints[2], cv::Scalar(0, 250, 0), 5);
                cv::line(*pMat, projectedPoints[0],projectedPoints[3], cv::Scalar(0, 0, 250), 5);
                    cv::circle(*pMat, projectedPoints[0], 2, cv::Scalar(0, 0, 250), 1, 8);

            }




        }
    }

    switch(SLAM.GetTrackingState()) {
        case -1:  {cv::putText(*pMat, "SYSTEM NOT READY", cv::Point(0,240), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255,0,0),2); }break;
        case 0:  {cv::putText(*pMat, "NO IMAGES YET", cv::Point(0,240), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255,0,0),2); }break;
        case 1:  {cv::putText(*pMat, "SLAM NOT INITIALIZED", cv::Point(0,240), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255,0,0),2); }break;
        case 2:  {cv::putText(*pMat, "SLAM ON", cv::Point(0,240), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,255,0),2); break;}
        case 3:  {cv::putText(*pMat, "SLAM LOST", cv::Point(0,240), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255,0,0), 2); break;}
        default:break;
    }
    LOGE("Native Finished");
#endif
}

