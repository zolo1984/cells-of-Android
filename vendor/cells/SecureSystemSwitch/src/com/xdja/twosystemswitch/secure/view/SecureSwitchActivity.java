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
		//getWindow().addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
		//String v = SystemProperties.get("persist.sys.exit","0");
		//if(!v.equals("1"))
		//	SystemProperties.set("persist.sys.exit","1");
		//finish();

		try{
			File f = new File("/dev/log/mdm");
			if(!f.exists()){
				Toast.makeText(this, " no file", Toast.LENGTH_LONG).show();
			}

            FileInputStream fin = new FileInputStream(f);
            int length = fin.available(); 
            byte [] buffer = new byte[length];
            fin.read(buffer);
            fin.close();  
        }catch(Exception e){
			e.printStackTrace();
			Toast.makeText(this, " " + e, Toast.LENGTH_LONG).show();

        }
	}
}
