package com.cells.twosystemswitch.secure.view;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.WindowManager;
import android.os.SystemProperties;
import java.io.IOException;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileInputStream;
import android.widget.Toast;

public class SecureSwitchActivity extends Activity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		//setContentView(R.layout.activity_main);
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
		String v = SystemProperties.get("persist.sys.exit","0");
		if(!v.equals("1"))
			SystemProperties.set("persist.sys.exit","1");
		finish();
	}
}
