package com.example.gb28181demo;

import android.app.Application;
import android.util.Log;

/*import org.ar.rtc.IRtcEngineEventHandler;
import org.ar.rtc.RtcEngine;
import org.ar.rtc.VideoEncoderConfiguration;*/

public class MyApplication extends Application {

    //public static RtcEngine rtcEngine;

    @Override
    public void onCreate() {
        super.onCreate();
        /*rtcEngine = RtcEngine.create(this, "177e21c0d1641291c34e46e1198bd49a", new IRtcEngineEventHandler() {
            @Override
            public void onWarning(int warn) {
                super.onWarning(warn);
                Log.e("RTC::", "War: " + warn);
            }

            @Override
            public void onUserJoined(String uid, int elapsed) {
                Log.d("RTC::", "onUserJoined: uid=" + uid + ", elapsed=" + elapsed);
            }
        });
        rtcEngine.enableVideo();
        rtcEngine.enableAudio();
        rtcEngine.setExternalVideoSource(true, true, true);
        VideoEncoderConfiguration videoEncoderConfiguration = new VideoEncoderConfiguration();
        videoEncoderConfiguration.dimensions = new VideoEncoderConfiguration.VideoDimensions(1280, 720);
        videoEncoderConfiguration.bitrate = 20000;
        videoEncoderConfiguration.minBitrate = 2000;
        videoEncoderConfiguration.minFrameRate = 1;
        videoEncoderConfiguration.frameRate = 30;
        videoEncoderConfiguration.orientationMode = VideoEncoderConfiguration.ORIENTATION_MODE.ORIENTATION_MODE_FIXED_LANDSCAPE;
        rtcEngine.setVideoEncoderConfiguration(videoEncoderConfiguration);
        rtcEngine.joinChannel("", "abcd1", "", "abcd");*/
    }
}
