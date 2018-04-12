package com.cells.stopsystem.secure.view;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.WindowManager;
import android.os.SystemProperties;

public class SecureSwitchActivity extends Activity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		SystemProperties.set("persist.sys.stop.othersystem","1");
			
		finish();
	}
}
