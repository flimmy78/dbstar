package com.dbstar.app;

import java.io.File;

import com.dbstar.R;
import com.dbstar.browser.GDWebChromeClient;
import com.dbstar.browser.GDWebViewClient;
import com.dbstar.util.LogUtil;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

public class GDEReaderActivity extends GDBaseActivity {

	private static final String TAG = "GDEReaderActivity";
	
	WebView mWebView;
	WebViewClient mWVClient;
	WebChromeClient mChromeClient;
	
	String mCategory;
	
	Activity activity = this;

	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.webview);
		
		mWVClient = new GDWebViewClient();
		mChromeClient = new GDWebChromeClient(this);
		
		mWebView = (WebView) findViewById(R.id.web_view);
		mWebView.getSettings().setJavaScriptEnabled(true);
		mWebView.getSettings().setCacheMode(WebSettings.LOAD_NORMAL);

		mWebView.setWebViewClient(mWVClient);
		mWebView.setWebChromeClient(mChromeClient);
		
		mWebView.setOnKeyListener(new View.OnKeyListener() {
			
			@Override
			public boolean onKey(View v, int keyCode, KeyEvent event) {
			    LogUtil.d(TAG, "onKey " + keyCode);
				boolean ret = false;
				int action = event.getAction();
				if (action == KeyEvent.ACTION_DOWN) {
					switch (keyCode) {
					case KeyEvent.KEYCODE_ESCAPE:
					case KeyEvent.KEYCODE_BACK: {
						ret = true;
						activity.onBackPressed();
						break;
					}
					default:
						break;
					}
				}
				return ret;
			}
		});
		
		Intent intent = getIntent();
		mCategory = intent.getStringExtra("category");
	}
	
	public void onServiceStart () {
		super.onServiceStart();
		
		String ebookFile = mService.getEBookFile(mCategory);
		File file = new File(ebookFile);
		if (ebookFile != null && !ebookFile.equals("") && file.exists())
		{
			mWebView.loadUrl("file://" + ebookFile);
		}
	}
}
