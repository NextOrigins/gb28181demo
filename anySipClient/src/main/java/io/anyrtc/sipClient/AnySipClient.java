package io.anyrtc.sipClient;


public class AnySipClient {

    public AnySipClient() {
        boolean load = NativeLoader.initNativeLibs();
        if (!load){
            throw new RuntimeException("AnySipClient!!! load so failed");
        }
    }

    public native void registerSip(String server_ip, String server_port, String server_id, String user_id, String user_password, String ipc_ip, ArSipListener listener);

    public native void registerSipWithLocation(
            String server_ip,
            String server_port,
            String server_id,
            String user_id,
            String user_password,
            String ipc_ip,
            ArSipListener listener,
            double longitude,
            double latitude
    );

    public native void unRegister();
}
