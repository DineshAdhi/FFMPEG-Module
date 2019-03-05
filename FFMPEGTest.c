#include<jni.h>
#include "FFMPEGTest.h"
#include "native/include/ffmpeg.h"
#include "native/include/log.h"

JNIEXPORT void JNICALL Java_FFMPEGTest_mergevideo (JNIEnv *env, jobject obj, jstring main, jstring merge, jstring out)
{
    char *main_native = (char *) (*env)->GetStringUTFChars(env, main, 0);
    char *merge_native = (char *) (*env)->GetStringUTFChars(env, merge, 0);
    char *out_native = (char *) (*env)->GetStringUTFChars(env, out, 0);
    
    ffmpeg_wrapper *wrapper;
    init_wrapper(&wrapper, out_native, 10);

    merge_video(wrapper, main_native);
    merge_video(wrapper, merge_native);

    execute_mux(wrapper);
}