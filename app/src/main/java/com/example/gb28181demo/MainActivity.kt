package com.example.gb28181demo

//import org.loka.screensharekit.EncodeBuilder
//import org.loka.screensharekit.ScreenShareKit

import android.Manifest
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.hardware.camera2.CameraAccessException
import android.hardware.camera2.CameraCaptureSession
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CaptureRequest
import android.location.Location
import android.location.LocationListener
import android.location.LocationManager
import android.net.ConnectivityManager
import android.net.wifi.WifiManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.Message
import android.util.Log
import android.view.Surface
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.core.content.PermissionChecker
import io.anyrtc.live.ArLiveDef
import io.anyrtc.live.ArLiveEngine
import io.anyrtc.live.ArLivePlayer
import io.anyrtc.live.ArLivePlayerObserver
import io.anyrtc.live.ArLivePusherObserver
import io.anyrtc.sipClient.AnySipClient
import io.anyrtc.sipClient.ArSipListener
import org.webrtc.TextureViewRenderer
import java.net.Inet4Address
import java.net.NetworkInterface
import java.net.SocketException
import java.util.Arrays
import java.util.Locale


class MainActivity : AppCompatActivity() {
    var mService: MyService? = null
    //private lateinit var mConnection: ServiceConnection


    /*companion object {
        var ssKit: EncodeBuilder? = null
    }*/
    //private lateinit var textureView: TextureView
    private lateinit var renderView: TextureViewRenderer
    //private val player = IjkMediaPlayer()

    private val liveEngine by lazy {
        ArLiveEngine.create(this)
    }
    private val pusher by lazy {
        liveEngine.createArLivePusher()
    }

    private val anySipClient = AnySipClient()

    /*private val playerView by lazy {
        val playerView = findViewById<PlayerView>(R.id.simple_player)
        playerView
    }
    private val player by lazy {
        val trackSelection = AdaptiveTrackSelection.Factory()

        val trackSelector = DefaultTrackSelector(
            this,
            trackSelection
        )
        trackSelector.setParameters(trackSelector.buildUponParameters().setMaxVideoSizeSd())

        val player = ExoPlayer.Builder(this)
            .setTrackSelector(trackSelector)
            .setBandwidthMeter(DefaultBandwidthMeter.getSingletonInstance(this))
            .build()
        playerView.player = player
        //playerView.setShowBuffering(StyledPlayerView.SHOW_BUFFERING_ALWAYS)

        player.addListener(object : Player.Listener {
            override fun onPlaybackStateChanged(playbackState: Int) {
                when (playbackState) {
                    Player.STATE_IDLE -> {
                        Log.d(TAG, "IDLE...")
                    }
                    Player.STATE_BUFFERING -> {
                        Log.d(TAG, "BUFFERING...")
                    }
                    Player.STATE_READY -> {
                        Log.d(TAG, "IS READY")
                    }
                    Player.STATE_ENDED -> {
                        Log.d(TAG, "PLAY END")
                    }
                }
            }

            override fun onIsPlayingChanged(isPlaying: Boolean) {
                Log.d(TAG, "onIsPlayingChanged: $isPlaying")
            }

            override fun onPlayerError(error: PlaybackException) {
                Log.e(TAG, "onPlayerError: ${error.errorCode}, ${error.errorCodeName}")
            }
        })
        player
    }*/

    //private lateinit var surfaceView: SurfaceView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        //surfaceView = findViewById(R.id.surface)
        //textureView = findViewById(R.id.texture)
        if (ContextCompat.checkSelfPermission(
                this.applicationContext,
                Manifest.permission.CAMERA
            ) == PermissionChecker.PERMISSION_DENIED ||
            ContextCompat.checkSelfPermission(this.applicationContext, Manifest.permission.BLUETOOTH_CONNECT
            ) == PermissionChecker.PERMISSION_DENIED ||
            ContextCompat.checkSelfPermission(this.applicationContext, Manifest.permission.ACCESS_FINE_LOCATION
            ) == PermissionChecker.PERMISSION_DENIED
        ) {
            val permissionArr = arrayListOf(
                Manifest.permission.CAMERA,
                Manifest.permission.ACCESS_FINE_LOCATION,
                Manifest.permission.RECORD_AUDIO,
                Manifest.permission.WRITE_EXTERNAL_STORAGE,
                Manifest.permission.READ_PHONE_STATE,
                Manifest.permission.ACCESS_FINE_LOCATION,
                Manifest.permission.ACCESS_COARSE_LOCATION,
            )
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                permissionArr.add(Manifest.permission.BLUETOOTH_CONNECT)
            }
            ActivityCompat.requestPermissions(this, permissionArr.toTypedArray(), 1)
        } else {
            getLocation()
        }
    }

    private fun init(longitude: Double, latitude: Double) {
        //playAudioWithRtmp("rtmp://47.101.39.57:1935/live/333")
        renderView = findViewById(R.id.render)
        renderView.postDelayed({
            Log.e(TAG, "delayed play audio")
            playAudioWithRtmp("rtmp://192.168.1.13:1935/broadcast/31011069561328897771_31011069561328897771")
        }, 10000)
        val render2 = findViewById<TextureViewRenderer>(R.id.render2)
        ap.setRenderView(render2)
        ap.setRenderFillMode(ArLiveDef.ArLiveFillMode.ArLiveFillModeFit)
        pusher.setRenderView(renderView)
        pusher.startCamera(true)
        pusher.setVideoQuality(ArLiveDef.ArLiveVideoEncoderParam(ArLiveDef.ArLiveVideoResolution.ArLiveVideoResolution1280x720).apply {
            videoFps = 15
        })
        pusher.startMicrophone()
        pusher.setObserver(object : ArLivePusherObserver() {
            override fun onCaptureFirstVideoFrame() {
                Log.i(TAG, "onCaptureFirstVideoFrame")
            }

            override fun onWarning(code: Int, msg: String?, extraInfo: Bundle?) {
                Log.w(TAG, "Warning: Code=$code, msg=$msg")
            }

            override fun onError(code: Int, msg: String?, extraInfo: Bundle?) {
                Log.e(TAG, "Error: code=$code, msg=$msg")
            }

            override fun onPushStatusUpdate(
                status: ArLiveDef.ArLivePushStatus?,
                msg: String?,
                extraInfo: Bundle?
            ) {
                when (status) {
                    ArLiveDef.ArLivePushStatus.ArLivePushStatusConnecting -> {
                        Log.i(TAG, "连接中....")
                    }

                    ArLiveDef.ArLivePushStatus.ArLivePushStatusConnectSuccess -> {
                        Log.i(TAG, "连接成功....")
                    }

                    ArLiveDef.ArLivePushStatus.ArLivePushStatusDisconnected -> {
                        Log.i(TAG, "连接断开....")
                    }

                    ArLiveDef.ArLivePushStatus.ArLivePushStatusReconnecting -> {
                        Log.i(TAG, "重连中....")
                    }
                    else -> {
                        Log.i(TAG, "???....")
                    }
                }
            }
        })
        /*
         SIP 服务器 ID：31000069562608717078
         SIP 服务器域：3100006956
         SIP 服务器地址：139.224.133.209
         SIP 服务器端口：7036
         SIP 用户名：31011069561328897771
         SIP 用户认证 ID： 31011069561328897771
         密码：123456
         */
        /* String server_ip, String server_port, String server_id, String user_id, String user_password, String ipc_ip, ArSipListener listener */
        val ipaddr = getIPAddress(this)
        anySipClient.registerSipWithLocation(
            "139.224.133.209",
            "57036",
            "31000069562608717078",
            /*"31011069561327120108",
            "aabb1122",*/
            "31011069561328897771",
            "123456",
            ipaddr,
            object : ArSipListener {
                override fun onRegisterResult(p0: Boolean) {
                    Log.d(TAG, "onRegister Result: $p0")
                }

                override fun onMediaControl(p0: String?, p1: String?, p2: String?) {
                    Log.d(TAG, "onMediaControl: p0=$p0, p1=$p1, p2=$p2")
                    val pushCode = pusher.startPush("rtp://$p1:$p2")
                    Log.d(TAG, "pushCode=$pushCode")
                }

                override fun onRtmpUrl(url: String?) {
                    Log.d(TAG, "rtmp url = $url")
                    if (url != null)
                        runOnUiThread { playAudioWithRtmp(url) }
                    /*if (url != null)
                        pusher.recvBroadcast(url
                        ) { buffer, size ->
                            Log.d(
                                TAG,
                                "buffer=${if (buffer != null) Arrays.toString(buffer) else "NULL"}, size=$size"
                            )
                        }*/
                }
            },
            longitude, latitude
        )
        //endregion

        //region GB service
        //endregion

        //region GB service
        /*mConnection = object : ServiceConnection {
            override fun onServiceConnected(name: ComponentName, service: IBinder) {
                val mBinder: MyService.MyBinder = service as MyService.MyBinder
                mService = mBinder.service
                mService?.let {
                    it.GB28181Init()
                    it.GB28181_Start()
                }
                MyService.portCallback = PortCallback {
                    runOnUiThread {
                        //pusher.stopPush()
                        val pushCode = pusher.startPush("rtp://124.77.120.129:$it")
                        Log.i(TAG, "推流rtp://124.77.120.129:$it")
                    }


                }
            }

            override fun onServiceDisconnected(name: ComponentName) {
                mService = null
            }
        }
        val mServiceIntent = Intent(this, MyService::class.java)
        bindService(mServiceIntent, mConnection, BIND_AUTO_CREATE)*/

        //ssKit = ScreenShareKit.init(this)
        /*val bandwithMeter = DefaultBandwidthMeter()
        val videoTrackSelectionFactory = AdaptiveTrackSelection.Factory()
        val trackSelector = DefaultTrackSelector(videoTrackSelectionFactory)*/

        /*surfaceView.holder.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceCreated(p0: SurfaceHolder) {
                Log.d(TAG, "surfaceCreated")
                player.setDisplay(p0)
            }

            override fun surfaceChanged(p0: SurfaceHolder, p1: Int, p2: Int, p3: Int) {
                Log.d(TAG, "surfaceChanged")
            }

            override fun surfaceDestroyed(p0: SurfaceHolder) {
                Log.d(TAG, "surfaceDestroyed")
            }

        })*/

    }

    private val TAG = "======"

    private fun getLocation() = runOnUiThread {
        if (ActivityCompat.checkSelfPermission(
                this,
                Manifest.permission.ACCESS_FINE_LOCATION
            ) != PackageManager.PERMISSION_GRANTED && ActivityCompat.checkSelfPermission(
                this,
                Manifest.permission.ACCESS_COARSE_LOCATION
            ) != PackageManager.PERMISSION_GRANTED
        ) {
            toast("需要定位权限")
            Log.d(TAG, "GPS Not Granted")
            return@runOnUiThread
        }
        val locationManager = getSystemService(Context.LOCATION_SERVICE) as LocationManager
        val gpsEnabled = locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER)
        if (!gpsEnabled) {
            toast("请打开GPS定位并重启APP")
            Log.d(TAG, "GPS disabled")
            return@runOnUiThread
        }

        val providers = locationManager.getProviders(true)
        if (providers.isNotEmpty()) {
            val location = locationManager.getLastKnownLocation(providers[0])
            location?.let {
                val latitude = it.latitude
                val longitude = it.longitude
                //anySipClient.setLocation(latitude, longitude)
                Log.d(TAG, "getLastKnownLocation: latitude=$latitude, longitude=$longitude")
                init(longitude, latitude)
            }
        }
        locationManager.requestLocationUpdates(
            LocationManager.GPS_PROVIDER, 60000L, 8f
        ) {
            //anySipClient.setLocation(it.latitude, it.longitude)
            Log.d(TAG, "requestLocationUpdates: latitude=${it.latitude}, longitude=${it.longitude}")
        }
    }

    private fun toast(text: String) = runOnUiThread {
        Toast.makeText(this, text, Toast.LENGTH_LONG).show()
    }

    override fun onDestroy() {
        super.onDestroy()
        pusher.stopPush()
        //unbindService(mConnection)
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String?>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == 1) {
            for (result in grantResults) {
                if (result != PermissionChecker.PERMISSION_GRANTED) {
                    finish()
                }
            }
            getLocation()
        }
    }

    private val ap by lazy {
        liveEngine.createArLivePlayer();
    }

    private fun playAudioWithRtmp(url: String) {
        ap.setObserver(object : ArLivePlayerObserver() {
            override fun onError(
                player: ArLivePlayer?,
                code: Int,
                msg: String?,
                extraInfo: Bundle?
            ) {
                Log.e(TAG, "onError, code=$code, msg=$msg")
            }

            override fun onWarning(
                player: ArLivePlayer?,
                code: Int,
                msg: String?,
                extraInfo: Bundle?
            ) {
                Log.e(TAG, "onWarning, code=$code, msg=$msg")
            }

            override fun onAudioPlayStatusUpdate(
                player: ArLivePlayer?,
                status: ArLiveDef.ArLivePlayStatus?,
                reason: ArLiveDef.ArLiveStatusChangeReason?,
                extraInfo: Bundle?
            ) {
                status ?: return
                when (status) {
                    ArLiveDef.ArLivePlayStatus.ArLivePlayStatusPlaying -> {
                        Log.d(TAG, "ArLivePlayStatusPlaying..")
                    }
                    ArLiveDef.ArLivePlayStatus.ArLivePlayStatusLoading -> {
                        Log.d(TAG, "ArLivePlayStatusLoading..")
                    }
                    ArLiveDef.ArLivePlayStatus.ArLivePlayStatusStopped -> {
                        Log.d(TAG, "ArLivePlayStatusStopped..")
                    }
                }
            }
        })
        val result = ap.startPlay(url);
        Log.e(TAG, "Start play, result=$result")
        //player.dataSource = url;
        //player.setDataSource(this, Uri.parse(url))
        /*player.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "fast", 1);
        player.setOption(IjkMediaPlayer.OPT_CATEGORY_FORMAT, "analyzeduration",1L);
        player.setOption(1, "analyzemaxduration", 100L);
        player.setOption(1, "probesize", 100L);
        player.setOption(1, "flush_packets", 0L);
        player.setOption(4, "framedrop", 0L);
        player.setOption(4, "packet-buffering", 0L);
        player.setOption(IjkMediaPlayer.OPT_CATEGORY_FORMAT, "fflags", "nobuffer");*/
        //player.prepareAsync()
        //player.start()


        /*val videoSource = ProgressiveMediaSource.Factory(RtmpDataSource.Factory())
            .createMediaSource(MediaItem.fromUri(url))
        player.setMediaSource(videoSource)
        player.prepare()
        player.playWhenReady = true*/

        /*val rtmpDataSourceFactory = RtmpDataSourceFactory()
        val media = MediaItem.Builder()
            .setUri(url).build()

        val mediaSource = ProgressiveMediaSource
            .Factory(rtmpDataSourceFactory)
            .createMediaSource(media)

        player.setMediaSource(mediaSource)
        player.prepare()
        player.playWhenReady = true*/

        /*val mediaPlayer = MediaPlayer()
        mediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC)
        mediaPlayer.setDataSource(url)
        mediaPlayer.prepareAsync()
        mediaPlayer.setOnPreparedListener {
            Log.d(TAG, "MediaPlayer Prepared.")
            it.start()
        }
        mediaPlayer.setOnCompletionListener {
            Log.d(TAG, "MediaPlayer Completed.")
            it.release()
        }
        mediaPlayer.setOnErrorListener { _, i, i2 ->
            Log.e(TAG, "i=$i, i2=$i2")
            false
        }*/
    }

    private var mCameraId = ""
    private var mCameraDevice: CameraDevice? = null
    private fun openCamera() {
        val cm = getSystemService(CAMERA_SERVICE) as CameraManager
        try {
            for (cameraId in cm.cameraIdList) {
                // get camera device info
                val cameraCharacteristics: CameraCharacteristics =
                    cm.getCameraCharacteristics(cameraId)
                //skip the front camera device
                val facing = cameraCharacteristics.get(CameraCharacteristics.LENS_FACING)
                if (facing != null && facing == CameraCharacteristics.LENS_FACING_FRONT) {
                    continue
                }
                cameraCharacteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
                    ?: continue
                mCameraId = cameraId
            }
            if (ActivityCompat.checkSelfPermission(
                    this,
                    Manifest.permission.CAMERA
                ) != PackageManager.PERMISSION_GRANTED
            ) {
                // TODO: Consider calling
                //    ActivityCompat#requestPermissions
                // here to request the missing permissions, and then overriding
                //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
                //                                          int[] grantResults)
                // to handle the case where the user grants the permission. See the documentation
                // for ActivityCompat#requestPermissions for more details.
                return
            }
            cm.openCamera(mCameraId, object : CameraDevice.StateCallback() {
                override fun onOpened(camera: CameraDevice) {
                    mCameraDevice = camera
                    createCameraPreviewSession()
                }

                override fun onDisconnected(camera: CameraDevice) {
                    mCameraDevice = null
                }

                override fun onError(camera: CameraDevice, error: Int) {
                }

            }, handler)
        } catch (e: CameraAccessException) {
            e.printStackTrace()
        }
    }

    private var mPreviewRequestBuilder: CaptureRequest.Builder? = null
    private var mCameraCaptureSession: CameraCaptureSession? = null
    private var mCaptureRequest: CaptureRequest? = null
    private fun createCameraPreviewSession() {
        val device = mCameraDevice ?: return
        try {
            val surfaceTexture = renderView.surfaceTexture
            val surface = Surface(surfaceTexture)
            mPreviewRequestBuilder = device.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW)
            mPreviewRequestBuilder?.addTarget(surface)
            device.createCaptureSession(
                Arrays.asList(surface),
                object : CameraCaptureSession.StateCallback() {
                    override fun onConfigured(session: CameraCaptureSession) {
                        if (null == mCameraDevice)
                            return
                        mCameraCaptureSession = session
                        mCaptureRequest = mPreviewRequestBuilder!!.build()
                        mCameraCaptureSession!!.setRepeatingRequest(
                            mCaptureRequest!!,
                            object : CameraCaptureSession.CaptureCallback() {
                                override fun onCaptureStarted(
                                    session: CameraCaptureSession,
                                    request: CaptureRequest,
                                    timestamp: Long,
                                    frameNumber: Long
                                ) {
                                    super.onCaptureStarted(session, request, timestamp, frameNumber)
                                }
                            },
                            handler
                        )

                    }

                    override fun onConfigureFailed(session: CameraCaptureSession) {
                    }

                },
                handler
            )
        } catch (e: CameraAccessException) {
            e.printStackTrace();
        }
    }

    var handler: Handler? = null

    private inner class MyThread : Thread() {
        override fun run() {
            Looper.prepare()
            handler = object : Handler(Looper.myLooper()!!) {
                override fun handleMessage(msg: Message) {
                    super.handleMessage(msg)
                }
            }
            Looper.loop()
        }
    }

    private fun getIPAddress(context: Context): String? {
        val info = (context.getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager).activeNetworkInfo
        if (info != null && info.isConnected) {
            if (info.type == ConnectivityManager.TYPE_MOBILE) {//当前使用2G/3G/4G网络
                try {
                    //Enumeration<NetworkInterface> en=NetworkInterface.getNetworkInterfaces();
                    //Enumeration< NetworkInterface > en = NetworkInterface.getNetworkInterfaces()
                    val en = NetworkInterface.getNetworkInterfaces()
                    while (en.hasMoreElements()) {
                        val intf = en.nextElement()
                        val enumIpAddr = intf.inetAddresses
                        while (enumIpAddr.hasMoreElements()) {
                            val inetAddress = enumIpAddr.nextElement()
                            if (!inetAddress.isLoopbackAddress && inetAddress is Inet4Address) {
                                return inetAddress.hostAddress
                            }
                        }
                    }
                } catch (e: SocketException) {
                    e.printStackTrace();
                }
            } else if (info.type == ConnectivityManager.TYPE_WIFI) {//当前使用无线网络
                val wifiManager = context.applicationContext.getSystemService(Context.WIFI_SERVICE) as WifiManager
                val wifiInfo = wifiManager.connectionInfo
                return intIP2StringIP(wifiInfo.ipAddress);
            }
        } else {
            // 当前无网络连接,请在设置中打开网络
        }
        return null;
    }

    private fun intIP2StringIP(ip: Int): String {
        return String.format(
            Locale.CHINA,
            "%d.%d.%d.%d",
            ip and 0xFF,
            ip.shr(8) and 0xFF,
            ip.shr(16) and 0xFF,
            ip.shr(24) and 0xFF
        )
    }
}