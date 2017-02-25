#include <stdlib.h>
#include <jni.h>

extern "C" int ClipAndScaleH264(char *sInH264FileName, char* sOutH264FileName,
                                int startFrame, int totalFrame,
                                int width, int height, float qp);

extern "C"
JNIEXPORT void JNICALL Java_com_instanza_cocovoice_VideoTool_clipAndScaleH264
    (JNIEnv* env, jobject thiz, jstring jsInH264FileName, jstring jsOutH264FileName,
     jint jStartFrame, jint jTotalFrame, jint jwidth, jint jheight, jfloat jqp) {
    
    char *sInH264FileName = (char*) ((*env).GetStringUTFChars (jsInH264FileName, NULL));
    char *sOutH264FileName = (char*) ((*env).GetStringUTFChars (jsOutH264FileName, NULL));

    ClipAndScaleH264(sInH264FileName, sOutH264FileName, jStartFrame, jTotalFrame, jwidth, jheight, jqp);

    (*env).ReleaseStringUTFChars(jsInH264FileName, sInH264FileName);
    (*env).ReleaseStringUTFChars(jsOutH264FileName, sOutH264FileName);
}
