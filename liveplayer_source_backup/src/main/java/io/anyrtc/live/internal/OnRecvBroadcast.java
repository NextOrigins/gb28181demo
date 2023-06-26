package io.anyrtc.live.internal;

public interface OnRecvBroadcast {

    void onPcm(byte[] buffer, int size);
}
