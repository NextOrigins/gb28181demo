package io.anyrtc.sipClient;

import android.util.Log;

public class NativeLoader {
    private static volatile boolean nativeLoaded = false;

    public static synchronized boolean initNativeLibs() {
        if (nativeLoaded) {
            return true;
        }
        try {
            try {
                System.loadLibrary("exosip");
                System.loadLibrary("anySipClient");
                nativeLoaded = true;
                return true;
            } catch (Error e) {
                Log.e("anysip",e.getMessage());
            }
        } catch (Throwable e) {
            e.printStackTrace();
        }
        return false;
    }

    public static void unInit(){
        nativeLoaded = false;
    }
}
