package com.example.nativecrashcatching;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.util.Log;
import android.widget.TextView;


import java.io.File;
import java.security.Permission;

public class MainActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 0);

        // Example of a call to a native method
        TextView tv = findViewById(R.id.sample_text);
        tv.setText("Hello, World");
        CrashCatching.init();

        tv.setOnClickListener(v -> {
            CrashCatching.die();
            File file=new File("/sdcard/a.txt");
            Log.e("@ati",file.exists()+"--fileStatus");
        });
    }

}
