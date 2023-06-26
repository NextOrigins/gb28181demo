package io.anyrtc.sipClient;

public interface ArSipListener {
    void onRegisterResult(boolean success);
    void onMediaControl(String type,String mediaServerIp, String mediaServePort);

    void onRtmpUrl(String url);
}
