package com.example.camport3_android_demo;

import java.util.HashMap;
import java.util.Iterator;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewFrame;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.android.Utils;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewListener2;


import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Bitmap;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;

public class MainActivity extends Activity implements CvCameraViewListener2 {
    private static final String    TAG = "PERCIPIO_CAMERA";
    private Mat                    mDepth, mDepthGray, mColor;
    final Bitmap bitmap = Bitmap.createBitmap(DEFAULT_PREVIEW_WIDTH, DEFAULT_PREVIEW_HEIGHT, Bitmap.Config.RGB_565);
    final Bitmap bitmap_color = Bitmap.createBitmap(DEFAULT_PREVIEW_WIDTH, DEFAULT_PREVIEW_HEIGHT, Bitmap.Config.RGB_565);
    private Button 				   mOpenButton;
    private Button 				   mCloseButton;
    private boolean               mRun = false;
    private boolean               mFinished = true;
    public static final int DEFAULT_PREVIEW_WIDTH = 640;
    public static final int DEFAULT_PREVIEW_HEIGHT = 480;
    private ImageView mImageView, mImageView_Color;
    private PendingIntent mPermissionIntent;
    private static final String ACTION_USB_PERMISSION = "com.Android.example.USB_PERMISSION";
    private UsbManager mUsbManager;

    //初始化cvMat
    private void tryCreateCvMat(){
        mDepth = new Mat(DEFAULT_PREVIEW_HEIGHT, DEFAULT_PREVIEW_WIDTH, CvType.CV_16U);
        mDepthGray = new Mat(DEFAULT_PREVIEW_HEIGHT, DEFAULT_PREVIEW_WIDTH, CvType.CV_8UC1);
        mColor = new Mat(DEFAULT_PREVIEW_HEIGHT, DEFAULT_PREVIEW_WIDTH, CvType.CV_8UC3);
        Log.i(TAG, "create cvMat successfully");
    }
    //获取USB权限
    private void tryGetUsbPermission(){
        mPermissionIntent = PendingIntent.getBroadcast(this, 0, new Intent(ACTION_USB_PERMISSION), 0);
        IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
        registerReceiver(usbReceiver, filter);
        mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
        HashMap<String,UsbDevice> deviceHashMap = mUsbManager.getDeviceList();
        Iterator<UsbDevice> iterator = deviceHashMap.values().iterator();
        while (iterator.hasNext()) {
            UsbDevice device = iterator.next();
            Log.d(TAG, "Device Name = " + device.getDeviceName());
            Log.d(TAG, "Device Vendor Id = " + device.getVendorId());
            Log.d(TAG, "Device Product Id = " + device.getProductId());
            if(device.getProductId() == 4099 && device.getVendorId() == 1204) {
                mUsbManager.requestPermission(device,mPermissionIntent);
            }
        }
    }

    private BaseLoaderCallback  mLoaderCallback = new BaseLoaderCallback(this) {
        @Override
        public void onManagerConnected(int status) {
            switch (status) {
                case LoaderCallbackInterface.SUCCESS: {
                    Log.i(TAG, "OpenCV loaded successfully");

                    // Load native library after(!) OpenCV initialization
                    System.loadLibrary("native-lib");
                    Log.i(TAG, "loadLibrary successfully");
                } break;
                default: {
                    super.onManagerConnected(status);
                } break;
            }
        }
    };

    private final BroadcastReceiver usbReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (ACTION_USB_PERMISSION.equals(action)) {
                synchronized (this) {
                    UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                    if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                        if (device != null) {
                            Log.d(TAG, "Granted");
                            try {
                                UsbDeviceConnection connection = ((UsbManager) context.getSystemService(Context.USB_SERVICE)).openDevice(device);
                                if (connection == null)
                                {
                                    Log.d(TAG, "connection is null");
                                    return;
                                }
                            } catch (Exception ex) {
                                Log.d(TAG, "Can't open device though permission was granted: " + ex);
                            }
                        }
                    } else {
                        finish();
                    }
                }
            }
        }
    };

    public MainActivity() {
        Log.i(TAG, "Instantiated new " + this.getClass());
    }

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "called onCreate");
        super.onCreate(savedInstanceState);

        tryGetUsbPermission();
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.deep_camera);
        mImageView = (ImageView)findViewById(R.id.deep_camera_view);
        mImageView_Color = (ImageView)findViewById(R.id.color_view);
        mOpenButton = (Button)findViewById(R.id.open_button);
        mOpenButton.setOnClickListener(mOpenListener);
        mCloseButton = (Button)findViewById(R.id.close_button);
        mCloseButton.setOnClickListener(mCloseListener);

        mOpenButton.setEnabled(true);
        mCloseButton.setEnabled(false);
    }

    public class GetDataThread extends Thread {
        @Override
        public void run() {

            while (true) {
                if (mRun) {
                    Log.d(TAG, "fatch data ......");
                    FetchData(mDepth.getNativeObjAddr(), mDepthGray.getNativeObjAddr(), mColor.getNativeObjAddr());
                    Utils.matToBitmap(mDepthGray, bitmap);
                    Utils.matToBitmap(mColor, bitmap_color);
                    mImageView.post(mUpdateImageTask);
                    mImageView_Color.post(mUpdateImageTask_color);
                } else
                    break;
            }

            mFinished = true;
            Log.d(TAG, "GetDataThread over...");
        }
    }

    public GetDataThread getDataThread;
        private final OnClickListener mOpenListener = new OnClickListener() {
            @Override
            public void onClick(final View view) {
                Log.i(TAG, "mOpenListener onClick");
                int status;
                ///////////////////////////////////////
                tryGetUsbPermission();
                tryCreateCvMat();
                ///////////////////////////////////////

                status = OpenDevice();
                Log.i(TAG, "mOpenListener OpenDevice :" + status );
                if(0 == status) {
                    mRun = true;
                    mFinished = false;
                    getDataThread = new GetDataThread();
                    getDataThread.start();

                    mOpenButton.setEnabled(false);
                    mCloseButton.setEnabled(true);
                }
            }
        };

    private final OnClickListener mCloseListener = new OnClickListener() {
        @Override
        public void onClick(final View view) {
            mRun = false;
            try
            {
                getDataThread.join();
            }
            catch (InterruptedException e)
            {
                e.printStackTrace();
            }

            while(!mFinished);

            CloseDevice();

            mOpenButton.setEnabled(true);
            mCloseButton.setEnabled(false);
            Log.i(TAG, "CloseDevice finished...");
            //finish();
            //System.exit(0);
        }
    };

    @Override
    public void onPause()
    {
        super.onPause();
    }

    @Override
    public void onResume()
    {
        Log.i(TAG, "called onResume");
        super.onResume();
        //OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION, this, mLoaderCallback);

        if (!OpenCVLoader.initDebug()) {
            Log.d(TAG, "Internal OpenCV library not found. Using OpenCV Manager for initialization");
            OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION, this, mLoaderCallback);
        } else {
            Log.d(TAG, "OpenCV library found inside package. Using it!");
            mLoaderCallback.onManagerConnected(LoaderCallbackInterface.SUCCESS);
            Log.d(TAG, "onManagerConnected finished!");
        }
    }

    public void onDestroy() {
        super.onDestroy();
    }

    private final Runnable mUpdateImageTask = new Runnable() {
        @Override
        public void run() {
            synchronized (bitmap) {
                Log.d(TAG, "mIFrameCallback set image");
                mImageView.setImageBitmap(bitmap);
            }
        }
    };

    private final Runnable mUpdateImageTask_color = new Runnable() {
        @Override
        public void run() {
            synchronized (bitmap_color) {
                Log.d(TAG, "mIFrameCallback set image");
                mImageView_Color.setImageBitmap(bitmap_color);
            }
        }
    };

    public native int OpenDevice();
    public native void CloseDevice();
    public native void FetchData(final long matAddr, final long matAddr_gray,final long matAddr_color);

    @Override
    public void onCameraViewStarted(int width, int height) {
        // TODO Auto-generated method stub

    }

    @Override
    public void onCameraViewStopped() {
        // TODO Auto-generated method stub

    }

    @Override
    public Mat onCameraFrame(CvCameraViewFrame inputFrame) {
        // TODO Auto-generated method stub
        return null;
    }
}