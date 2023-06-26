package com.example.gb28181demo;

import android.annotation.SuppressLint;
import android.app.Service;
import android.content.Intent;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.opengl.GLES11Ext;
import android.os.Binder;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.util.Log;
import android.view.Surface;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.example.gb28181demo.GB28181.gb28181.GB28181Params;
import com.example.gb28181demo.GB28181.gb28181.XMLUtil;
import com.example.gb28181demo.GB28181.net.IpAddress;
import com.example.gb28181demo.GB28181.sdp.MediaDescriptor;
import com.example.gb28181demo.GB28181.sdp.SessionDescriptor;
import com.example.gb28181demo.GB28181.sip.address.NameAddress;
import com.example.gb28181demo.GB28181.sip.address.SipURL;
import com.example.gb28181demo.GB28181.sip.authentication.DigestAuthentication;
import com.example.gb28181demo.GB28181.sip.header.AuthorizationHeader;
import com.example.gb28181demo.GB28181.sip.header.ExpiresHeader;
import com.example.gb28181demo.GB28181.sip.header.UserAgentHeader;
import com.example.gb28181demo.GB28181.sip.message.Message;
import com.example.gb28181demo.GB28181.sip.message.MessageFactory;
import com.example.gb28181demo.GB28181.sip.message.SipMethods;
import com.example.gb28181demo.GB28181.sip.message.SipResponses;
import com.example.gb28181demo.GB28181.sip.provider.SipProvider;
import com.example.gb28181demo.GB28181.sip.provider.SipProviderListener;
import com.example.gb28181demo.GB28181.tools.PSmuxer;

//import org.ar.rtc.video.ARVideoFrame;
//import org.loka.screensharekit.EncodeBuilder;
//import org.loka.screensharekit.callback.H264CallBack;

import java.io.FileDescriptor;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetSocketAddress;
import java.net.SocketException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import java.util.Timer;
import java.util.TimerTask;

public class MyService extends Service implements SipProviderListener {

    private static final String TAG = "MyService";
    private MyBinder myBinder;
    private SipProvider sipProvider;
    private Timer timerForKeepAlive;
    private long keepAliveSN = 0;
    private byte[] mPreviewData;
    private MediaCodec mediaCodec;
    private MediaCodec rtpMediaCodec;
    public byte[] configByte;
    private long count = 0;
    private SurfaceTexture surfaceTexture = new SurfaceTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES);
    private Handler handler;
    private int ssrc = 0;
    private long time = 0;
    private int cseq = 0;

    private final int FPS = 30;
    private DatagramSocket mediaSocket;
    private long sequenceNumber;
    private MediaMuxer muxer;

    public static PortCallback portCallback;
    private static int invitePort = 0;

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        myBinder = new MyBinder();
        return myBinder;
    }

    @Override
    public void onCreate() {
        super.onCreate();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        stopMediaCodec();
        GB28181_Stop();
        sipProvider.stopTrasport();
    }

    public class MyBinder extends Binder {
        public MyService getService() {
            return MyService.this;
        }
    }

    //region GB28181 部分
    /*国标模块参数初始化*/
    public void GB28181Init() {
        /*

    SIP 服务器 ID：31000069562608717078
    SIP 服务器域：3100006956
    SIP 服务器地址：139.224.133.209
    SIP 服务器端口：7036
    SIP 用户名：      31011069561328897771
    SIP 用户认证 ID： 31011069561328897771
    密码：123456

         */
        GB28181Params.setSIPServerIPAddress("139.224.133.209");//SIP服务器地址
        GB28181Params.setRemoteSIPServerPort(7036);//SIP服务器端口
        GB28181Params.setLocalSIPIPAddress("192.168.197.146");//本机地址
        GB28181Params.setRemoteSIPServerID("31000069562608717078");
        GB28181Params.setRemoteSIPServerSerial("3100006956");
        GB28181Params.setLocalSIPPort(5080);//本机端口
        GB28181Params.setCameraHeigth(720);
        GB28181Params.setCameraWidth(1280);
        GB28181Params.setPassword("123456");//密码
        GB28181Params.setLocalSIPDeviceId("31011069561328897771");
        GB28181Params.setLocalSIPMediaId("31011069561328897771");
        GB28181Params.setCurGBState(0);
        GB28181Params.setCurDeviceDownloadMeidaState(0);
        GB28181Params.setCurDeviceDownloadMeidaState(0);
        GB28181Params.setCurDevicePlayMediaState(0);
        GB28181Params.setCameraState(0);
        Log.d(TAG, "MyService -> GB28181Init()");
        /*Objects.requireNonNull(MainActivity.Companion.getSsKit())
                .config(480, 720, 25, 1000000, EncodeBuilder.SCREEN_DATA_TYPE.H264)
                .onH264(new H264CallBack() {
                    @Override
                    // fun onH264(buffer: ByteBuffer, isKeyFrame: Boolean, width:Int, height:Int, ts: Long)
                    public void onH264(@NonNull ByteBuffer byteBuffer, boolean b, int i, int i1, long l) {
                        byte[] data = new byte[byteBuffer.remaining()];
                        byteBuffer.get(data);
                        ARVideoFrame arVideoFrame = new ARVideoFrame();
                        arVideoFrame.buf = data;
                        arVideoFrame.bufType = ARVideoFrame.BUFFER_TYPE_H264;
                        arVideoFrame.rotation = 0;
                        arVideoFrame.stride = 480;
                        arVideoFrame.height = 720;
                        arVideoFrame.timeStamp = System.currentTimeMillis();
                        arVideoFrame.format = b ? ARVideoFrame.FORMAT_VIDEO_KEY_FRAME : ARVideoFrame.FORMAT_VIDEO_NOR_FRAME;
                        MyApplication.rtcEngine.pushExternalVideoFrame(arVideoFrame);

                        //encodeH264ToRtpAndSendToServer(data, b);
                        if (s != null) {
                            s.sendH264(data, b, arVideoFrame.timeStamp);
                        }*/
                        /*
        ARVideoFrame arVideoFrame = new ARVideoFrame();
        arVideoFrame.buf = buf;
        arVideoFrame.bufType = ARVideoFrame.FORMAT_NV21;
        //arVideoFrame.bufType = ARVideoFrame.BUFFER_TYPE_H264;
        arVideoFrame.rotation = 0;
        arVideoFrame.stride = 720;
        arVideoFrame.height = 480;
        arVideoFrame.timeStamp = System.currentTimeMillis();
        //arVideoFrame.format = isKeyFrame ? ARVideoFrame.FORMAT_VIDEO_KEY_FRAME : ARVideoFrame.FORMAT_VIDEO_NOR_FRAME;
        MyApplication.rtcEngine.pushExternalVideoFrame(arVideoFrame);
                    }*/
                /*}}).start()*/;
    }

    /*国标模块参数初始化*/
    public void GB28181Init(String SIPServerIPAddress, String LocalSIPIPAddress, int RemoteSIPServerPort, int LocalSIPPort,
                            int CameraHeigth, int CameraWidth, String Password, String LocalSIPDeviceId, String LocalSIPMediaId) {
        GB28181Params.setSIPServerIPAddress(SIPServerIPAddress);//SIP服务器地址
        GB28181Params.setLocalSIPIPAddress(LocalSIPIPAddress);
        GB28181Params.setRemoteSIPServerPort(RemoteSIPServerPort);//SIP服务器端口
        GB28181Params.setLocalSIPPort(LocalSIPPort);
        GB28181Params.setCameraHeigth(CameraHeigth);
        GB28181Params.setCameraWidth(CameraWidth);
        GB28181Params.setPassword(Password);
        GB28181Params.setLocalSIPDeviceId(LocalSIPDeviceId);
        GB28181Params.setLocalSIPMediaId(LocalSIPMediaId);
        GB28181Params.setCurGBState(0);
        GB28181Params.setCurDeviceDownloadMeidaState(0);
        GB28181Params.setCurDeviceDownloadMeidaState(0);
        GB28181Params.setCurDevicePlayMediaState(0);
        GB28181Params.setCameraState(0);
    }

    public void GB28181ReStart() {
        GB28181_Stop();
        GB28181_Start();
    }

    public void GB28181_Start() {
        new Thread(() -> {
            IpAddress.setLocalIpAddress(GB28181Params.getLocalSIPIPAddress());
            sipProvider = new SipProvider(GB28181Params.getLocalSIPIPAddress(), GB28181Params.getLocalSIPPort());
            sipProvider.addSipProviderListener(this);
            NameAddress to = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getRemoteSIPServerSerial()));
            NameAddress from = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getRemoteSIPServerSerial()));
            NameAddress contact = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getLocalSIPIPAddress()));
            Message message = MessageFactory.createRequest(sipProvider, SipMethods.REGISTER, new SipURL(GB28181Params.getRemoteSIPServerID(), GB28181Params.getRemoteSIPServerSerial()), to, from, contact, null);
            message.setUserAgentHeader(new UserAgentHeader(GB28181Params.defaultUserAgent));
            sipProvider.sendMessage(message, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);
        }).start();
    }

    public void GB28181_Stop() {
        new Thread(() -> {
            if (sipProvider != null && GB28181Params.CurGBState == 1) {
                NameAddress to = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getLocalSIPIPAddress()));
                NameAddress from = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getLocalSIPIPAddress()));
                NameAddress contact = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getLocalSIPIPAddress()));
                Message message = MessageFactory.createRequest(sipProvider, SipMethods.REGISTER, new SipURL(GB28181Params.getRemoteSIPServerID(), GB28181Params.getRemoteSIPServerSerial()), to, from, contact, null);
                message.setExpiresHeader(new ExpiresHeader(0));
                message.setUserAgentHeader(new UserAgentHeader(GB28181Params.defaultUserAgent));
                sipProvider.sendMessage(message, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);
                sipProvider.halt();
                GB28181Params.setCurDeviceDownloadMeidaState(0);
                GB28181Params.setCurDeviceDownloadMeidaState(0);
                GB28181Params.setCurDevicePlayMediaState(0);
            }
        }).start();

    }

    private void GB28181_KeepAlive() {
        if (sipProvider != null && GB28181Params.CurGBState == 1) {
            /*NameAddress to=new NameAddress(new SipURL(GB28181Params.getSIPServerIPAddress(),GB28181Params.getSIPServerIPAddress()));
            NameAddress from=new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(),GB28181Params.getLocalSIPIPAddress()));
            NameAddress contact=new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(),GB28181Params.getLocalSIPIPAddress()));*/
            NameAddress to = new NameAddress(new SipURL(GB28181Params.getRemoteSIPServerID(), GB28181Params.getRemoteSIPServerSerial()));
            NameAddress from = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getRemoteSIPServerSerial()));

            String body = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n" +
                    "<Notify>\r\n" +
                    "  <CmdType>Keepalive</CmdType>\r\n" +
                    "  <SN>" + keepAliveSN++ + "</SN>\r\n" +
                    "  <DeviceID>" + GB28181Params.getLocalSIPDeviceId() + "</DeviceID>\r\n" +
                    "  <Status>OK</Status>\r\n" +
                    "</Notify>";
            Message message = MessageFactory.createMessageRequest(
                    sipProvider, to, from,
                    null, XMLUtil.XML_MANSCDP_TYPE, body
            );
            sipProvider.sendMessage(message, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);
        }
    }

    public void InitCamera() {
        mPreviewData = new byte[GB28181Params.getCameraWidth() * GB28181Params.getCameraHeigth() * 3 / 2];
        if (GB28181Params.getmCamera() == null) {
            GB28181Params.setmCamera(Camera.open(Camera.CameraInfo.CAMERA_FACING_BACK));
        }
        GB28181Params.getmCamera().setDisplayOrientation(90);
        Camera.Parameters parameters = GB28181Params.getmCamera().getParameters();
        List<int[]> FPSRange = parameters.getSupportedPreviewFpsRange();
        parameters.setPreviewFormat(ImageFormat.NV21);
        parameters.setPreviewSize(GB28181Params.getCameraWidth(), GB28181Params.getCameraHeigth());
        parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE);
        parameters.setPreviewFpsRange(FPS * 1000, FPS * 1000);

        GB28181Params.getmCamera().setParameters(parameters);
        Log.i("camera", "CAMERA初始化");
        try {
            mediaSocket = new DatagramSocket(0);
        } catch (SocketException e) {
            e.printStackTrace();
        }


    }

    private final int frameRate = 25;
    private void InitMediaCodec() {
        try {
            mediaCodec = MediaCodec.createEncoderByType("VIDEO/AVC");
            MediaFormat mediaFormat = MediaFormat.createVideoFormat("VIDEO/AVC", GB28181Params.getCameraWidth(), GB28181Params.getCameraHeigth());
            mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar);
            mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, GB28181Params.getCameraWidth() * GB28181Params.getCameraHeigth());
            mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, FPS);
            mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1);
            mediaCodec.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);

            rtpMediaCodec = MediaCodec.createEncoderByType("video/avc");
            MediaFormat format = MediaFormat.createVideoFormat("video/avc", 720, 480);
            int bitRate = 10000;
            format.setInteger(MediaFormat.KEY_BIT_RATE, bitRate);
            format.setInteger(MediaFormat.KEY_FRAME_RATE, frameRate);
            format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
            format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1);
            rtpMediaCodec.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                //muxer = new MediaMuxer(new FileDescriptor(), MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);
            }
            Log.i("MediaCodec", "MediaCodec编码器初始化");
            //mediaCodec.start();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void SetCameraCallback() {
        try {
            GB28181Params.getmCamera().setPreviewTexture(surfaceTexture);
            GB28181Params.getmCamera().addCallbackBuffer(mPreviewData);
            GB28181Params.getmCamera().setPreviewCallbackWithBuffer(mPreviewCallback);
            GB28181Params.getmCamera().startPreview();
            Log.i("CAMERA", "mCamera.startPreview()");
        } catch (IOException e) {
            e.printStackTrace();
        }

    }

    public void ReleaseCamera() {
        if (GB28181Params.getmCamera() != null && GB28181Params.getCameraState() <= 0) {
            Log.i("ReleaseCamera", "" + GB28181Params.getCameraState() + "释放Camera资源");
            GB28181Params.getmCamera().stopPreview();
            GB28181Params.getmCamera().setPreviewCallbackWithBuffer(null);
            GB28181Params.getmCamera().release();
            GB28181Params.setmCamera(null);
        }
    }

    private Camera.PreviewCallback mPreviewCallback = new Camera.PreviewCallback() {
        @Override
        public void onPreviewFrame(byte[] data, Camera camera) {
            if (data == null) {
                GB28181Params.getmCamera().addCallbackBuffer(mPreviewData);
            } else {
                if (GB28181Params.getCurDevicePlayMediaState() == 1) {
                    GB28181Params.getmCamera().addCallbackBuffer(data);
                    //handler.obtainMessage(0, mPreviewData).sendToTarget();
                    //sendToRtc(data, false);
                }

            }
        }
    };

    private void NV21ToNV12(byte[] nv21, byte[] nv12, int width, int height) {
        if (nv21 == null || nv12 == null) return;
        int framesize = width * height;
        System.arraycopy(nv21, 0, nv12, 0, framesize);
        /*for (i = 0; i < framesize; i++) {
            nv12[i] = nv21[i];
        }*/
        for (int i = 0; i < framesize / 2; i += 2) {
            nv12[i + framesize - 1] = nv21[i + framesize];
            nv12[i + framesize] = nv21[i + framesize - 1];
        }
        /*for (int i = 0; i < framesize / 2; i += 2) {
            nv12[i + framesize] = nv21[i + framesize - 1];
        }*/
    }

    private List<byte[]> h264ToRtp(byte[] data) {
        MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
        boolean isEOS = false;
        int generateIndex = 0;
        List<byte[]> list = new ArrayList<>();
        while (!isEOS) {
            int inputBufferIndex = rtpMediaCodec.dequeueInputBuffer(-1);
            if (inputBufferIndex >= 0) {
                // 获取待编码的视频数据
                ByteBuffer inputBuffer = rtpMediaCodec.getInputBuffer(inputBufferIndex);
                inputBuffer.clear();
                inputBuffer.put(data);
                rtpMediaCodec.queueInputBuffer(inputBufferIndex, 0, data.length, System.nanoTime() / 1000, 0);
                generateIndex++;
            }

            int outputBufferIndex = rtpMediaCodec.dequeueOutputBuffer(bufferInfo, 0);
            if (outputBufferIndex >= 0) {
                ByteBuffer outputBuffer = rtpMediaCodec.getOutputBuffer(outputBufferIndex);
                //muxer.writeSampleData(0, outputBuffer, bufferInfo);
                if (outputBuffer == null) {
                    rtpMediaCodec.releaseOutputBuffer(outputBufferIndex, false);
                    return null;
                } else {
                    byte[] outData = new byte[outputBuffer.remaining()];
                    outputBuffer.get(outData);
                    list.add(outData);
                }
            }

            if ((bufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                isEOS = true;
            }
        }
        return list;
    }

    private byte[] encodeH264(byte[] input, boolean[] isKeyFrame) {
        try {
            byte[] nv12 = new byte[GB28181Params.getCameraWidth() * GB28181Params.getCameraHeigth() * 3 / 2];
            NV21ToNV12(input, nv12, GB28181Params.getCameraWidth(), GB28181Params.getCameraHeigth());
            int inputBufferIndex = mediaCodec.dequeueInputBuffer(-1);
            ByteBuffer inputBuffer = mediaCodec.getInputBuffer(inputBufferIndex);
            if (inputBuffer == null) {
                return null;
            }
            inputBuffer.clear();
            inputBuffer.put(nv12);

            //mediaCodec.queueInputBuffer(inputBufferIndex, 0, input.length, 132 + 1000000 * count / FPS, 0);
            //count++;
            mediaCodec.queueInputBuffer(inputBufferIndex, 0, nv12.length, System.nanoTime() / 1000, 0);
            MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
            int outputBufferIndex = mediaCodec.dequeueOutputBuffer(bufferInfo, 0);
            byte[] outData;
            byte[] h264Data = null;
            if (outputBufferIndex >= 0) {
                ByteBuffer outputBuffer = mediaCodec.getOutputBuffer(outputBufferIndex);
                if (outputBuffer == null) {
                    return null;
                }
                outData = new byte[outputBuffer.remaining()];
                //outputBuffer.get(outData, 0, outData.length);
                outputBuffer.get(outData);
                if (bufferInfo.flags == MediaCodec.BUFFER_FLAG_CODEC_CONFIG) {
                    //configByte = new byte[bufferInfo.size];
                    //configByte = outData;
                    System.arraycopy(outData, 0, configByte, 0, bufferInfo.size);
                } else if (bufferInfo.flags == MediaCodec.BUFFER_FLAG_KEY_FRAME) {
                    h264Data = new byte[bufferInfo.size + configByte.length];
                    System.arraycopy(configByte, 0, h264Data, 0, configByte.length);
                    System.arraycopy(outData, 0, h264Data, configByte.length, outData.length);
                    isKeyFrame[0] = true;
                } else {
                    h264Data = new byte[bufferInfo.size];
                    System.arraycopy(outData, 0, h264Data, 0, bufferInfo.size);
                }
                mediaCodec.releaseOutputBuffer(outputBufferIndex, false);
            }
            return h264Data;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    private void stopMediaCodec() {
        if (mediaCodec != null) {
            mediaCodec.stop();
            mediaCodec.release();
            mediaCodec = null;
        }
    }

    private List<byte[]> SplitByteArray(byte[] input, int size) {
        List<byte[]> result = new ArrayList<>();
        int length = input.length;
        int count = length / size;
        int r = length % size;

        for (int i = 0; i < count; i++) {
            byte[] newByteArray = new byte[size];
            System.arraycopy(input, size * i, newByteArray, 0, size);
            result.add(newByteArray);
        }
        if (r != 0) {
            byte[] newByteArray = new byte[r];
            System.arraycopy(input, length - r, newByteArray, 0, r);
            result.add(newByteArray);
        }
        return result;
    }

    class SendMediaThread extends Thread {
        @SuppressLint("HandlerLeak")
        @Override
        public void run() {
            Looper.prepare();
            handler = new Handler() {
                public void handleMessage(android.os.Message msg) {
                    if (msg.what == 1) {
                        //encodeH264ToRtpAndSendToServer((byte[])msg.obj, false);
                        SelfData d = (SelfData) msg.obj;
                        //List<byte[]> bytes = h264ToRtp(d.data);
                        /*assert bytes != null;
                        for (byte[] b : bytes) {
                            InetSocketAddress address = new InetSocketAddress(GB28181Params.getMediaServerIPAddress(), GB28181Params.getMediaServerPort());
                            DatagramPacket packet = new DatagramPacket(b, b.length, address);
                            Log.i("Send:", "sending rpt." + String.format("%s:%s", GB28181Params.getMediaServerIPAddress(), GB28181Params.getMediaServerPort()));
                            try {
                                mediaSocket.send(packet);
                            } catch (IOException e) {
                                e.printStackTrace();
                            }
                        }*/
                        //encodeH264ToRtp(d.data, d.t);
                        //encodeH264ToRtpAndSendToServer(d.data, false);
                        /*boolean[] isKeyFrame = {false};
                        byte[] raw = encodeH264(mPreviewData, isKeyFrame);
                        encodeH264ToRtpAndSendToServer(raw, isKeyFrame[0]);*/
                        //encodeH264ToRtp();
                    }
                }
            };
            Looper.loop();

        }
    }

    private TimerTask keepALiveTask = new TimerTask() {
        @Override
        public void run() {
            GB28181_KeepAlive();
        }
    };

    private int mSequenceNumber;

    private void encodeH264ToRtp(byte[] data, long timestamp) {
        //boolean[] isKeyFrame = {false};
        //byte[] h264Data = encodeH264(mPreviewData, isKeyFrame);

        List<byte[]> nalUnits = new ArrayList<>();

        int start = 0;
        int totalSize = 0;
        while (start < data.length) {
            int nextStart = start + 1;
            while (nextStart < data.length - 3 && !(data[nextStart] == 0x00 && data[nextStart + 1] == 0x00 && data[nextStart + 2] == 0x01)) {
                nextStart++;
            }
            byte[] bytes = Arrays.copyOfRange(data, start, nextStart);
            nalUnits.add(bytes);
            totalSize += bytes.length;
            start = nextStart + 3;
        }

// Concatenate the NAL units into a single byte array
        /*byte[] nalUnit = new byte[totalSize];
        int offset = 0;
        for (byte[] unit : nalUnits) {
            System.arraycopy(unit, 0, nalUnit, offset, unit.length);
            offset += unit.length;
        }*/

        byte[] rtpPacket = new byte[12 + totalSize]; // 12-byte header + NAL unit payload

// Set the RTP header fields
        rtpPacket[0] = (byte) 0x80; // version (2 bits) + padding flag (1 bit) + extension flag (1 bit) + CSRC count (4 bits)
        rtpPacket[1] = (byte) 0x60; // marker (1 bit) + payload type (7 bits)
        rtpPacket[2] = (byte) (sequenceNumber >> 8); // sequence number (16 bits)
        rtpPacket[3] = (byte) sequenceNumber;
        rtpPacket[4] = (byte) (timestamp >> 24); // timestamp (32 bits)
        rtpPacket[5] = (byte) (timestamp >> 16);
        rtpPacket[6] = (byte) (timestamp >> 8);
        rtpPacket[7] = (byte) timestamp;
        rtpPacket[8] = (byte) (ssrc >> 24); // synchronization source identifier (32 bits)
        rtpPacket[9] = (byte) (ssrc >> 16);
        rtpPacket[10] = (byte) (ssrc >> 8);
        rtpPacket[11] = (byte) ssrc;
        sequenceNumber = (sequenceNumber + 1) & 0xFFFF;

// Copy the NAL unit payload to the RTP packet payload
        int offset = 0;
        for (byte[] unit : nalUnits) {
            System.arraycopy(unit, 0, rtpPacket, 12 + offset, unit.length);
            offset += unit.length;
        }

        InetSocketAddress address = new InetSocketAddress(GB28181Params.getMediaServerIPAddress(), GB28181Params.getMediaServerPort());
        DatagramPacket packet = new DatagramPacket(rtpPacket, rtpPacket.length, address);
        Log.d("Send:", "sending to " + String.format("%s:%s", GB28181Params.getMediaServerIPAddress(), GB28181Params.getMediaServerPort()));
        try {
            mediaSocket.send(packet);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void sendToRtc(byte[] buf, boolean isKeyFrame) {
        /*ARVideoFrame arVideoFrame = new ARVideoFrame();
        arVideoFrame.buf = buf;
        arVideoFrame.bufType = ARVideoFrame.FORMAT_NV21;
        //arVideoFrame.bufType = ARVideoFrame.BUFFER_TYPE_H264;
        arVideoFrame.rotation = 0;
        arVideoFrame.stride = 720;
        arVideoFrame.height = 480;
        arVideoFrame.timeStamp = System.currentTimeMillis();
        //arVideoFrame.format = isKeyFrame ? ARVideoFrame.FORMAT_VIDEO_KEY_FRAME : ARVideoFrame.FORMAT_VIDEO_NOR_FRAME;
        MyApplication.rtcEngine.pushExternalVideoFrame(arVideoFrame);*/
    }

    private void encodeH264ToRtpAndSendToServer(byte[] data, boolean isKeyFrame) {
        if (data != null) {
            //sendToRtc(raw, isKeyFrame[0]);
            int index = 0;
            int length;
            List<byte[]> list = new ArrayList<>();
            byte[] naul;
            //获取一帧H264所有的naul
            for (int i = index + 3; i < data.length - 3; i++) {
                if ((data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == 0x00 && data[i + 3] == 0x01)) {
                    length = i - index - 4;
                    naul = new byte[length];
                    System.arraycopy(data, index + 4, naul, 0, length);
                    list.add(naul);
                    index += length + 4;
                }
                if (i == data.length - 3 - 1) {
                    naul = new byte[data.length - index - 4];
                    System.arraycopy(data, index + 4, naul, 0, naul.length);
                    list.add(naul);
                }
            }
            //Log.e(TAG, "handleMessage: " + list.size() + "");
            /*
            获取NALU
             */

            /*
            获取Nalu后，获取psH（？），传入isKeyFrame
            time不知道是干嘛的
             */
            byte[] psH;
            if (list.size() == 3) {
                psH = PSmuxer.GetPSHeader(0, time, 1);
            } else {
                psH = PSmuxer.GetPSHeader(0, time, 0);
            }
            byte[] Temp = new byte[psH.length + 14 * list.size() + data.length];
            byte[] startcode = new byte[]{0x00, 0x00, 0x00, 0x01};
            long pts = time;

            switch (list.size()) {
                case 1:
                    byte[] pesh = PSmuxer.GetPESHeader(list.get(0).length + startcode.length, 0, pts);
                    System.arraycopy(psH, 0, Temp, 0, psH.length);
                    System.arraycopy(pesh, 0, Temp, psH.length, pesh.length);
                    System.arraycopy(startcode, 0, Temp, psH.length + pesh.length, startcode.length);
                    System.arraycopy(list.get(0), 0, Temp, psH.length + pesh.length + 4, list.get(0).length);
                    break;
                case 2:
                    byte[] pesh_SPS = PSmuxer.GetPESHeader(list.get(0).length + startcode.length, 0, pts);
                    byte[] pesh_PPS = PSmuxer.GetPESHeader(list.get(1).length + startcode.length, 0, pts);
                    System.arraycopy(psH, 0, Temp, 0, psH.length);
                    System.arraycopy(pesh_SPS, 0, Temp, psH.length, pesh_SPS.length);
                    System.arraycopy(startcode, 0, Temp, psH.length + pesh_SPS.length, startcode.length);
                    System.arraycopy(list.get(0), 0, Temp, psH.length + pesh_SPS.length + 4, list.get(0).length);
                    System.arraycopy(pesh_PPS, 0, Temp, psH.length + pesh_SPS.length + list.get(0).length + 4, pesh_PPS.length);
                    System.arraycopy(startcode, 0, Temp, psH.length + pesh_SPS.length + list.get(0).length + 4 + pesh_PPS.length, startcode.length);
                    System.arraycopy(list.get(1), 0, Temp, psH.length + pesh_SPS.length + 4 + list.get(0).length + pesh_PPS.length + 4, list.get(1).length);
                    break;
                case 3:
                    byte[] pesH_SPS = PSmuxer.GetPESHeader(list.get(0).length + startcode.length, 0, pts);
                    byte[] pesH_PPS = PSmuxer.GetPESHeader(list.get(1).length + startcode.length, 0, pts);
                    byte[] pesH_RAW = PSmuxer.GetPESHeader(list.get(2).length + startcode.length, 0, pts);
                    System.arraycopy(psH, 0, Temp, 0, psH.length);
                    System.arraycopy(pesH_SPS, 0, Temp, psH.length, pesH_SPS.length);
                    System.arraycopy(startcode, 0, Temp, psH.length + pesH_SPS.length, startcode.length);
                    System.arraycopy(list.get(0), 0, Temp, psH.length + pesH_SPS.length + 4, list.get(0).length);
                    System.arraycopy(pesH_PPS, 0, Temp, psH.length + pesH_SPS.length + 4 + list.get(0).length, pesH_PPS.length);
                    System.arraycopy(startcode, 0, Temp, psH.length + pesH_SPS.length + 4 + list.get(0).length + pesH_PPS.length, startcode.length);
                    System.arraycopy(list.get(1), 0, Temp, psH.length + pesH_SPS.length + 4 + list.get(0).length + pesH_PPS.length + 4, list.get(1).length);
                    System.arraycopy(pesH_RAW, 0, Temp, psH.length + pesH_SPS.length + 4 + list.get(0).length + pesH_PPS.length + 4 + list.get(1).length, pesH_RAW.length);
                    System.arraycopy(startcode, 0, Temp, psH.length + pesH_SPS.length + 4 + list.get(0).length + pesH_PPS.length + 4 + list.get(1).length + pesH_RAW.length, startcode.length);
                    System.arraycopy(list.get(2), 0, Temp, psH.length + pesH_SPS.length + 4 + list.get(0).length + pesH_PPS.length + 4 + list.get(1).length + pesH_RAW.length + 4, list.get(2).length);
                    break;
            }

            List<byte[]> rtplist = SplitByteArray(Temp, 1400);
            byte[] buf;
            for (int i = 0; i < rtplist.size(); i++) {
                byte[] rtpH = PSmuxer.GetRtpHeader(0, cseq, time, ssrc);
                byte[] rtpHend = PSmuxer.GetRtpHeader(1, cseq, time, ssrc);
                if (i != rtplist.size() - 1) {
                    buf = new byte[rtplist.get(i).length + 12];
                    System.arraycopy(rtpH, 0, buf, 0, rtpH.length);
                    System.arraycopy(rtplist.get(i), 0, buf, 12, rtplist.get(i).length);
                } else {
                    buf = new byte[rtplist.get(i).length + 12];
                    System.arraycopy(rtpHend, 0, buf, 0, rtpHend.length);
                    System.arraycopy(rtplist.get(i), 0, buf, rtpHend.length, rtplist.get(i).length);
                }
                InetSocketAddress address = new InetSocketAddress(GB28181Params.getMediaServerIPAddress(), GB28181Params.getMediaServerPort());
                DatagramPacket packet = new DatagramPacket(buf, buf.length, address);
                try {
                    mediaSocket.send(packet);
                } catch (IOException e) {
                    e.printStackTrace();
                }
                cseq += 1;
            }
            time += (90000 / FPS);
        }
    }

    private final String LOG_TRIM = "______";

    @Override
    public void onReceivedMessage(SipProvider sip_provider, Message message) {
        if (message.isResponse()) {
            switch (message.getCSeqHeader().getMethod()) {
                case SipMethods.REGISTER:
                    if (message.getStatusLine().getCode() == 401) {
                        NameAddress to = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getLocalSIPIPAddress()));
                        NameAddress from = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getLocalSIPIPAddress()));
                        NameAddress contact = new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getLocalSIPIPAddress()));
                        Message res = MessageFactory.createRegisterRequest(sipProvider, to, from, contact, null, null);
                        res.setUserAgentHeader(new UserAgentHeader(GB28181Params.defaultUserAgent));
                        AuthorizationHeader ah = new AuthorizationHeader("Digest");
                        ah.addUsernameParam(GB28181Params.getLocalSIPDeviceId());
                        ah.addRealmParam(message.getWwwAuthenticateHeader().getRealmParam());
                        ah.addNonceParam(message.getWwwAuthenticateHeader().getNonceParam());
                        ah.addUriParam(res.getRequestLine().getAddress().toString());
                        ah.addQopParam(message.getWwwAuthenticateHeader().getQopParam());
                        String response = (new DigestAuthentication(SipMethods.REGISTER,
                                ah, null, GB28181Params.getPassword())).getResponse();
                        ah.addResponseParam(response);
                        res.setAuthorizationHeader(ah);
                        if (GB28181Params.getCurGBState() == 1) {
                            res.setExpiresHeader(new ExpiresHeader(0));
                        }
                        sipProvider.sendMessage(res, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);
                    } else if (message.getStatusLine().getCode() == 200) {
                        //注销成功
                        if (GB28181Params.getCurGBState() == 1) {
                            GB28181Params.setCurGBState(0);
                            //取消发送心跳包
                            timerForKeepAlive.cancel();
                        } else {//注册成功
                            GB28181Params.setCurGBState(1);
                            //每隔60秒 发送心跳包
                            timerForKeepAlive = new Timer(true);
                            timerForKeepAlive.schedule(keepALiveTask, 0, 20 * 1000);
                            Log.d("onReceivedMessage:", "KeepAlive______");
                        }
                    }
                    break;
                case SipMethods.MESSAGE:
                    Log.d("onReceivedMessage:", "MESSAGE______");
                    break;
                case SipMethods.ACK:
                    Log.d("onReceivedMessage:", "ACK______");
                    break;
                case SipMethods.BYE:
                    break;
            }
        } else if (message.isRequest()) {
            if (message.isMessage()) {
                if (message.hasBody()) {
                    String body = message.getBody();
                    String sn = body.substring(body.indexOf("<SN>") + 4, body.indexOf("</SN>"));
                    String cmdType = body.substring(body.indexOf("<CmdType>") + 9, body.indexOf("</CmdType>"));
                    if (message.getBodyType().toLowerCase().equals("application/manscdp+xml")) {
                        //发送 200 OK
                        if (cmdType.equals("Catalog")) {
                            Message CatalogResponse = MessageFactory.createResponse(message, 200, SipResponses.reasonOf(200), null);
                            CatalogResponse.setUserAgentHeader(new UserAgentHeader(GB28181Params.defaultUserAgent));
                            sipProvider.sendMessage(CatalogResponse, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);

                            //region catalogBody
                            String catalogBody = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" +
                                    "    <Response>\n" +
                                    "      <CmdType>Catalog</CmdType>\n" +
                                    "      <SN>" + sn + "</SN>\n" +
                                    "      <DeviceID>" + GB28181Params.getLocalSIPDeviceId() + "</DeviceID>\n" +
                                    "      <SumNum>1</SumNum>\n" +
                                    "      <DeviceList Num='1'>\n" +
                                    "          <Item>\n" +
                                    "            <DeviceID>" + GB28181Params.getLocalSIPDeviceId() + "</DeviceID>\n" +
                                    "            <Name>Phone</Name>\n" +
                                    "            <Manufacturer>ZBGD</Manufacturer>\n" +
                                    "            <Model>MODEL</Model>\n" +
                                    "            <Owner>AnyRtc</Owner>\n" +
                                    "            <CivilCode>3400200</CivilCode>\n" +
                                    "            <Address>local</Address>\n" +
                                    "            <Parental>0</Parental>\n" +
                                    "            <SafetyWay>0</SafetyWay>\n" +
                                    "            <RegisterWay>1</RegisterWay>\n" +
                                    "            <Secrecy>0</Secrecy>\n" +
                                    "            <IPAddress>" + GB28181Params.getLocalSIPIPAddress() + "</IPAddress>\n" +
                                    "            <Port>" + GB28181Params.getLocalSIPPort() + "</Port>\n" +
                                    "            <Password>12345678</Password>\n" +
                                    "            <Status>ON</Status>\n" +
                                    "          </Item>\n" +
                                    "      </DeviceList>\n" +
                                    "    </Response>";
                            //endregion

                            Message CatalogResponseRequest = MessageFactory.createMessageRequest(sipProvider, message.getFromHeader().getNameAddress(),
                                    message.getToHeader().getNameAddress(), null, XMLUtil.XML_MANSCDP_TYPE, catalogBody);
                            CatalogResponseRequest.setUserAgentHeader(new UserAgentHeader(GB28181Params.defaultUserAgent));
                            sipProvider.sendMessage(CatalogResponseRequest, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);
                        } else if (cmdType.equals("DeviceControl")) {
                            //ToDo 解析控制指令
                            Message DeviceControlResponse = MessageFactory.createResponse(message, 200, SipResponses.reasonOf(200), null);
                            DeviceControlResponse.setUserAgentHeader(new UserAgentHeader(GB28181Params.defaultUserAgent));
                            sipProvider.sendMessage(DeviceControlResponse, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);
                        }

                    }
                }
            } else if (message.isInvite()) {
                if (message.hasBody()) {
                    String body = message.getBody();
                    SessionDescriptor sdp = new SessionDescriptor(body);
                    MediaDescriptor mediaDescriptor = sdp.getMediaDescriptors().firstElement();
                    String address = sdp.getConnection().getAddress();
                    int port = mediaDescriptor.getMedia().getPort();
                    switch (sdp.getSessionName().getValue().toLowerCase()) {
                        case "play":
                            String y = body.substring(body.indexOf("y=") + 2, body.indexOf("y=") + 12);

                            //region InviteResponseBody
                            String InviteResponseBody = "v=0\n" +
                                    "o=" + GB28181Params.getLocalSIPDeviceId() + " 0 0 IN IP4 " + GB28181Params.getLocalSIPIPAddress() + "\n" +
                                    "s=" + sdp.getSessionName().getValue() + "\n" +
                                    "c=IN IP4 " + GB28181Params.getLocalSIPIPAddress() + "\n" +
                                    "t=0 0\n" +
                                    "m=video " + port + " RTP/AVP 96\n" +
                                    "a=sendonly\n" +
                                    "a=rtpmap:96 PS/90000\n" +
                                    "y=" + y + "";
                            //endregion

                            GB28181Params.setMediaServerIPAddress(address);
                            GB28181Params.setMediaServerPort(port);
                            ssrc = Integer.parseInt(y);
                            Log.i("======", ":收到INVITE,ADDRESS=" + GB28181Params.getMediaServerIPAddress() + ";port=" + GB28181Params.getMediaServerPort() + "；ssrc=" + ssrc + "");
                            invitePort = GB28181Params.getMediaServerPort();
                            Message InviteResponse = MessageFactory.createResponse(message, 200, SipResponses.reasonOf(200), SipProvider.pickTag(),
                                    new NameAddress(new SipURL(GB28181Params.getLocalSIPDeviceId(), GB28181Params.getLocalSIPIPAddress())), "Application/Sdp", InviteResponseBody);
                            InviteResponse.setUserAgentHeader(new UserAgentHeader(GB28181Params.defaultUserAgent));
                            sipProvider.sendMessage(InviteResponse, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);
                            break;
                        case "palyback":
                            break;
                        case "download":
                            break;
                    }
                }
            } else if (message.isAck()) {
                Log.i("======", "收到INVITE,ACK = " + GB28181Params.getCurDevicePlayMediaState());
                if (GB28181Params.getCurDevicePlayMediaState() == 0) {
                    if (null != portCallback) {
                        portCallback.onPort(invitePort);
                    }
                    /*SendMediaThread thread = new SendMediaThread();
                    thread.start();
                    //初始化Camera、编码器
                    InitMediaCodec();

                    InitCamera();
                    //启动编码器
                    mediaCodec.start();
                    rtpMediaCodec.start();
                    Log.i("Mediacodec", "mediaCodec.start()");
                    //Camera 回调、启动Camera预览
                    SetCameraCallback();
                    s = new SelfCallback() {
                        @Override
                        public void sendH264(byte[] data, boolean keyFrame, long t) {
                            android.os.Message message = handler.obtainMessage(1, new SelfData(t, keyFrame, data));
                            handler.sendMessage(message);
                        }
                    };

                    GB28181Params.setCurDevicePlayMediaState(1);
                    GB28181Params.setCameraState(GB28181Params.getCameraState() + 1);*/
                }
            } else if (message.isBye()) {
                if (GB28181Params.CurDevicePlayMediaState == 1) {
                    //200 OK
                    Message ByeResponse = MessageFactory.createResponse(message, 200, SipResponses.reasonOf(200), null);
                    ByeResponse.setUserAgentHeader(new UserAgentHeader(GB28181Params.defaultUserAgent));
                    sipProvider.sendMessage(ByeResponse, GB28181Params.defaultProtol, GB28181Params.getSIPServerIPAddress(), GB28181Params.getRemoteSIPServerPort(), 0);

                    GB28181Params.setCurDevicePlayMediaState(1);
                    if (GB28181Params.getCameraState() > 0) {
                        GB28181Params.setCameraState(GB28181Params.getCameraState() - 1);
                    }
                    Log.i("======", "Bye.");

                    //释放Camera资源
                    //ReleaseCamera();

                }
            }
        }


    }
//endregion

    private SelfCallback s;
    private interface SelfCallback {
        void sendH264(byte[] data, boolean keyFrame, long timestamp);
    }
    private class SelfData {
        long t;
        boolean k;
        byte[] data;

        public SelfData(long t, boolean k, byte[] data) {
            this.t = t;
            this.k = k;
            this.data = data;
        }
    }

    public interface PortCallback {
        void onPort(int port);
    }
}

