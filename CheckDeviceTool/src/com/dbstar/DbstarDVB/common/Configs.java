package com.dbstar.DbstarDVB.common;

import com.guodian.checkdevicetool.util.APPVersion;

public class Configs {
    public static final String TEST_TYPE_ORDER_FILE_NAME = "checkdeviceflag";
    public static final String TEST_TYPE = "testType";
    public static final int TYPE_BOARD_TEST = 0;
    public static final int TYPE_ALL_TEST = 1;
    public static final int TYPE_AGING_TEST = 2;
    public static final int TYPE_SELECTOR_TEST = 3;
    public static final int TEST_SUCCESS = 0x1000;
    public static final int TEST_FAIL = 0x10001;
    
    public static final String MESSAGE = "message";
    public static final String MSG_SHOW_RESULT = "shwoResult";
    public static final String MSG_SHOW_AUTO_NEXT = "autoNext";
    public static final String TARGET_VIDEO_FILE = "/data/test.ts";
    public static final String TEST_RESULT_PATH = "test_result";
    public static final String PRODUCT_SN = "ProductSN";
    public static final String SSID = "SSID";
    public static final String PASSWORD = "Password";
    public static final String SOCKET = "Socket";
    public static final String SMART_CARD_NUMBER = "SmartCard";
    public static final String VIDEO_PATH = "VideoPath";
    public static final String PLAY_TIME = "TestPlayVideoTime";
    public static final String USB_1 = "usb1";
    public static final String USB_2 = "usb2";
    public static final String USB_3 = "usb3";
    
    public static String TEST_CONFIG_FILE_PAHT = "/storage/external_storage/sdcard1/test_configure.xml";
    public static String DEFALUT_DISK = "";
    
    public String mWifiSSID;
    public String mWifiPassword;
    public String mScoketNumber;
    public String mSmartCardNum;
    public String mVideoPath;
    public String mPlayTime;
    public String mProductSN;
    
    static {
        if(APPVersion.SINGLE){
            TEST_CONFIG_FILE_PAHT = "/mnt/sdcard/external_sdcard/test_configure.xml";
            DEFALUT_DISK= "";
        }
    }
    @Override
    public String toString() {
        return "Configs [mWifiSSID=" + mWifiSSID + ", mWifiPassword=" + mWifiPassword + ", mScoketNumber=" + mScoketNumber + ", mSmartCardNum=" + mSmartCardNum + ", mVideoPath=" + mVideoPath
                + ", mPlayTime=" + mPlayTime + ", mProductSN=" + mProductSN + "]";
    }
    
    
}