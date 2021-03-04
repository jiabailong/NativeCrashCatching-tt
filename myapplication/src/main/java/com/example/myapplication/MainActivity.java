package com.example.myapplication;

import android.Manifest;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import com.example.nativecrashcatching.CrashCatching;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 0);

        // Example of a call to a native method
        TextView tv = findViewById(R.id.test);
        tv.setText("Hello, World");
        CrashCatching.init();

        tv.setOnClickListener(v -> {
            CrashCatching.die();
        });
    }
}