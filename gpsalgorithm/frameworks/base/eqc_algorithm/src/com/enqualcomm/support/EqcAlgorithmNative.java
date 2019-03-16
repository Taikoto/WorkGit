package com.enqualcomm.support;

import android.util.Log;
import android.content.Context;
import android.content.Intent;

public class EqcAlgorithmNative {

    private static final String TAG = "EqcAlgorithmNative";
    

    private static EqcAlgorithmNative.EqcAlgorithmInterface mInterface;
      
    public interface EqcAlgorithmInterface {

        public void EqcAlgorithmInterfaceMethod1(int arg1 ,int arg2,int arg3, double arg4,double arg5, double arg6, float arg7, float arg8,int arg9,float arg10,long arg11,long arg12, double arg13,double arg14,int arg15,long arg16,int arg17 );
        public void EqcAlgorithmInterfaceMethod2();
        public void EqcAlgorithmInterfaceMethod3();
        public void EqcAlgorithmInterfaceMethod4();
        public void EqcAlgorithmInterfaceMethod5(int arg1);
        public void EqcAlgorithmInterfaceMethod6(int arg1);
    }
	static {
	    System.loadLibrary("eqc_algorithm_jni");	    
	}

    private EqcAlgorithmNative() {
    }

    public static void EqcAlgorithmNativeSetInterface(EqcAlgorithmNative.EqcAlgorithmInterface tmp) {
        mInterface = tmp;
        Log.i(TAG,"mInterface="+mInterface);
    }


    public static void EqcAlgorithmNativeMethod1(int arg1 ,double arg4,double arg5, double arg6, float arg7, float arg8,int arg9,float arg10,long arg11,int arg15,long arg16,int arg17 ){

        Log.i(TAG,"report 1 location mInterface="+mInterface+",arg17="+arg17);

       int arg2 =0,arg3 =0;
       long arg12 = 0;
       double arg13 = 0, arg14 =0;

        if (mInterface != null) {
            mInterface.EqcAlgorithmInterfaceMethod1(arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10,arg11,arg12,arg13,arg14,arg15,arg16,arg17);
	}
    }

    public static void EqcAlgorithmNativeMethod2(){	 

        Log.i(TAG,"open mInterface="+mInterface);
        if (mInterface != null) {
            mInterface.EqcAlgorithmInterfaceMethod2();	
	}
	}


    public static void EqcAlgorithmNativeMethod3(){	 
        Log.i(TAG,"close mInterface="+mInterface);
        if (mInterface != null) {
            mInterface.EqcAlgorithmInterfaceMethod3();
	}

	}

    public static void EqcAlgorithmNativeMethod4(){	 
         Log.i(TAG,"get mInterface="+mInterface);
        if (mInterface != null) {
            mInterface.EqcAlgorithmInterfaceMethod4();
	}
	}

    public static void EqcAlgorithmNativeSetNetwork(int arg1){	 
         Log.i(TAG,"network mInterface="+mInterface);
        if (mInterface != null) {
            mInterface.EqcAlgorithmInterfaceMethod5(arg1);
	}
	}

    public static void EqcAlgorithmNativeSetAirpaneMode(int arg1){	 
         Log.i(TAG,"airplane mInterface="+mInterface);
        if (mInterface != null) {
            mInterface.EqcAlgorithmInterfaceMethod6(arg1);
	}
	}

    public static boolean isPedometerAvailable(){
        Log.i(TAG, "[isPedometerAvailable]");
        return native_EqcAlgorithm_isPedometerAvailable() == 1;
    }

    public static boolean isHeartrateAvailable(){
        Log.i(TAG, "[isHeartrateAvailable]");
        return native_EqcAlgorithm_isHeartrateAvailable() == 1;
    }

    public static boolean isLightPerceptionAvailable(){
        Log.i(TAG, "[isLightPerceptionAvailable]");
        return native_EqcAlgorithm_isLightPerceptionAvailable() == 1;
    }


    public static native int native_EqcAlgorithmMethod1(int a,int b);	
    public static native void native_EqcAlgorithmMethod2();
    public static native int native_EqcAlgorithmMethod3(int i);
    public static native int native_EqcAlgorithmMethod4(int arg1,int arg2,int arg3,int arg4,int arg5,int arg6,long arg7);
    public static native int native_EqcAlgorithmMethod5(String str);
    public static native int native_EqcAlgorithmMethod6(float arg1,float arg2,float arg3,int arg4);
    public static native int native_EqcAlgorithmMethod7(int arg1,int arg2);
    public static native int native_EqcAlgorithmMethod8(int arg1);
    public static native int native_EqcAlgorithmMethod9(int arg1);
    public static native int native_EqcAlgorithm_Gsensor_eint_work_start();
    public static native int native_EqcAlgorithm_Gsensor_eint_work_stop();
    public static native int native_EqcAlgorithm_Gsensor_clear_step();
    public static native int native_EqcAlgorithm_Gsensor_get_step();
    public static native int native_EqcAlgorithm_isPedometerAvailable();
    public static native int native_EqcAlgorithm_isHeartrateAvailable();
    public static native int native_EqcAlgorithm_isLightPerceptionAvailable();
}

