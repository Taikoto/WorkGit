package com.chm.hellojni;
import android.util.Log;

public class HelloWorld{

    protected static final String TAG = "HELLOJNI";
    static {
        //名字必须和libhellojni.so 名字对应起来
        System.loadLibrary("hellojni");
    }

    public static native String helloWorld();
    public static native int add(int a,int b);

}
