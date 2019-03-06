#include<jni.h>
#include "FFMPEGTest.h"
#include "native/include/ffmpeg.h"
#include "native/include/log.h"

JNIEXPORT jlong JNICALL Java_FFMPEGTest_getContext (JNIEnv *env, jobject obj, jstring out)
{
    char *out_native = (char *) (*env)->GetStringUTFChars(env, out, 0);
    ffmpeg_wrapper *wrapper;
    init_wrapper(&wrapper, out_native, 100);

    return (long)wrapper;
}

JNIEXPORT void JNICALL Java_FFMPEGTest_mergevideo (JNIEnv *env, jobject obj, long context, jstring videofile)
{
    char *videfile_native = (char *) (*env)->GetStringUTFChars(env, videofile, 0);
    
    ffmpeg_wrapper *wrapper = (ffmpeg_wrapper *)context;
    merge_video(wrapper, videfile_native);
}

JNIEXPORT void JNICALL Java_FFMPEGTest_cutvideo(JNIEnv *env, jobject obj, jlong context, jstring videofile, jint starttime, jint endtime)
{
    char *videofile_native = (char *) (*env)->GetStringUTFChars(env, videofile, 0);
    int start_time_native = (int) starttime;
    int end_time_native = (int) endtime;

    ffmpeg_wrapper *wrapper = (ffmpeg_wrapper *) context;

    cut_video(wrapper, videofile_native, start_time_native, end_time_native);
}

JNIEXPORT void JNICALL Java_FFMPEGTest_insertvideo(JNIEnv *env, jobject obj, jlong context, jstring main_file, jstring insert_file, jint timestamp)
{
    char *main_file_native = (char *) (*env)->GetStringUTFChars(env, main_file, 0);
    char *insert_file_native = (char *) (*env)->GetStringUTFChars(env, insert_file, 0);
    
    ffmpeg_wrapper *wrapper = (ffmpeg_wrapper *)context;
    insert_video(wrapper, main_file_native, insert_file_native, timestamp);
}

JNIEXPORT void JNICALL Java_FFMPEGTest_executemux (JNIEnv *env, jobject obj, jlong context)
{
    ffmpeg_wrapper *wrapper = (ffmpeg_wrapper *) context;
    execute_mux(wrapper);
}