//自己创建一个c文件，实现自己定义的native方法，也就是.h文件中的方法
//引入自己生成的.h头文件
#include <stdio.h>
#include "com_chm_hellojni_HelloWorld.h"

//返回一个字符串
JNIEXPORT jstring JNICALL Java_com_chm_hellojni_HelloWorld_helloWorld(JNIEnv *env, jclass jobj) {
      printf("helloWorld\n");
      return (*env)->NewStringUTF(env,"HelloWorld 我是用jni调用出来的字符串");
}
//返回 a+b的结果
JNIEXPORT jint JNICALL Java_com_chm_hellojni_HelloWorld_add(JNIEnv *env, jclass jobj, jint a, jint b){
     printf("a+b = %d\n",a+b);
     return a+b;
}
