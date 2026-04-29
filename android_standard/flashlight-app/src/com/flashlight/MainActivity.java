package com.flashlight;

import android.app.Activity;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraManager;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.content.Context;
import android.widget.Toast;

public class MainActivity extends Activity {
    private CameraManager cameraManager;
    private String cameraId;
    private boolean isFlashOn = false;
    private Button flashButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        cameraManager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
        flashButton = findViewById(R.id.flashButton);

        try {
            cameraId = cameraManager.getCameraIdList()[0];
        } catch (CameraAccessException e) {
            Toast.makeText(this, "Camera error: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }

        flashButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                toggleFlash();
            }
        });
    }

    private void toggleFlash() {
        try {
            if (isFlashOn) {
                cameraManager.setTorchMode(cameraId, false);
                isFlashOn = false;
                flashButton.setText("Turn Flash ON");
                flashButton.setBackgroundColor(0xFF333333);
            } else {
                cameraManager.setTorchMode(cameraId, true);
                isFlashOn = true;
                flashButton.setText("Turn Flash OFF");
                flashButton.setBackgroundColor(0xFFCC0000);
            }
        } catch (CameraAccessException e) {
            Toast.makeText(this, "Failed to toggle flash", Toast.LENGTH_SHORT).show();
        }
    }
}
