#!/bin/bash

# Single-file Android APK builder for Flashlight app
# Run with: bash build.sh

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}========================================${NC}"
echo -e "${YELLOW}  Flashlight APK Builder${NC}"
echo -e "${YELLOW}========================================${NC}"

# Set Android SDK paths
export ANDROID_HOME=$HOME/Android
export PATH=$PATH:$ANDROID_HOME/cmdline-tools/latest/bin
export PATH=$PATH:$ANDROID_HOME/platform-tools
export PATH=$PATH:$ANDROID_HOME/build-tools/30.0.3

# Create project structure
echo -e "${GREEN}[1/8] Creating project structure...${NC}"
rm -rf build gen
mkdir -p build/classes build/dex build/apk gen/com/flashlight src/com/flashlight app/src/main/res/layout

# Create MainActivity.java
cat > src/com/flashlight/MainActivity.java << 'JAVA'
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
JAVA

# Create layout XML
cat > app/src/main/res/layout/activity_main.xml << 'XML'
<?xml version="1.0" encoding="utf-8"?>
<LinearLayout xmlns:android="http://schemas.android.com/apk/res/android"
    android:layout_width="match_parent"
    android:layout_height="match_parent"
    android:orientation="vertical"
    android:gravity="center"
    android:background="#000000">
    
    <Button
        android:id="@+id/flashButton"
        android:layout_width="200dp"
        android:layout_height="200dp"
        android:text="Turn Flash ON"
        android:textSize="22sp"
        android:textColor="#FFFFFF"
        android:background="#333333" />
</LinearLayout>
XML

# Create AndroidManifest.xml
cat > app/src/main/AndroidManifest.xml << 'MANIFEST'
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.flashlight">
    
    <uses-permission android:name="android.permission.CAMERA" />
    <uses-feature android:name="android.hardware.camera2" android:required="true" />
    
    <application
        android:allowBackup="true"
        android:icon="@android:drawable/ic_menu_camera"
        android:label="Flashlight"
        android:theme="@android:style/Theme.Black.NoTitleBar.Fullscreen">
        
        <activity
            android:name=".MainActivity"
            android:exported="true"
            android:screenOrientation="portrait">
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
    </application>
</manifest>
MANIFEST

# Check if Android SDK is installed
if [ ! -f "$ANDROID_HOME/build-tools/30.0.3/aapt" ]; then
    echo -e "${YELLOW}[2/8] Android SDK not found. Installing...${NC}"
    sudo apt update
    sudo apt install -y openjdk-17-jdk wget unzip zip
    
    cd ~
    if [ ! -f "commandlinetools-linux-11076708_latest.zip" ]; then
        wget -q https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip
    fi
    
    rm -rf Android
    mkdir -p Android/cmdline-tools
    unzip -q commandlinetools-linux-11076708_latest.zip -d Android/cmdline-tools
    mv Android/cmdline-tools/cmdline-tools Android/cmdline-tools/latest
    
    # Accept licenses
    yes | $ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager --licenses > /dev/null 2>&1 || true
    
    # Install components
    $ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager "platform-tools" > /dev/null 2>&1
    $ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager "platforms;android-30" > /dev/null 2>&1
    $ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager "build-tools;30.0.3" > /dev/null 2>&1
    
    # Add to .bashrc
    if ! grep -q "ANDROID_HOME" ~/.bashrc 2>/dev/null; then
        echo "export ANDROID_HOME=\$HOME/Android" >> ~/.bashrc
        echo 'export PATH=$PATH:$ANDROID_HOME/cmdline-tools/latest/bin' >> ~/.bashrc
        echo 'export PATH=$PATH:$ANDROID_HOME/platform-tools' >> ~/.bashrc
        echo 'export PATH=$PATH:$ANDROID_HOME/build-tools/30.0.3' >> ~/.bashrc
    fi
    
    echo -e "${GREEN}SDK installed!${NC}"
fi

# Set tool paths
AAPT=$ANDROID_HOME/build-tools/30.0.3/aapt
DX=$ANDROID_HOME/build-tools/30.0.3/dx
ZIPALIGN=$ANDROID_HOME/build-tools/30.0.3/zipalign
APKSIGNER=$ANDROID_HOME/build-tools/30.0.3/apksigner
PLATFORM_JAR=$ANDROID_HOME/platforms/android-30/android.jar

if [ ! -f "$AAPT" ]; then
    echo -e "${RED}Error: Android build tools not found. Run: bash build.sh again${NC}"
    exit 1
fi

# Generate R.java
echo -e "${GREEN}[3/8] Generating R.java...${NC}"
$AAPT package -f \
    -M app/src/main/AndroidManifest.xml \
    -S app/src/main/res \
    -I $PLATFORM_JAR \
    -J gen \
    -m 2>/dev/null

# Package resources
echo -e "${GREEN}[4/8] Packaging resources...${NC}"
$AAPT package -f \
    -M app/src/main/AndroidManifest.xml \
    -S app/src/main/res \
    -I $PLATFORM_JAR \
    -F build/resources.ap_ 2>/dev/null

cp build/resources.ap_ build/apk/app-unaligned.apk

# Compile Java
echo -e "${GREEN}[5/8] Compiling Java...${NC}"
javac -d build/classes \
    -cp $PLATFORM_JAR \
    -source 1.7 -target 1.7 \
    -Xlint:-options \
    gen/com/flashlight/R.java \
    src/com/flashlight/MainActivity.java 2>/dev/null

# Convert to DEX
echo -e "${GREEN}[6/8] Converting to DEX...${NC}"
$DX --dex --output=build/dex/classes.dex build/classes/ 2>/dev/null

# Add to APK
echo -e "${GREEN}[7/8] Creating APK...${NC}"
zip -j build/apk/app-unaligned.apk build/dex/classes.dex > /dev/null 2>&1

# Align and sign
$ZIPALIGN -p -f -v 4 build/apk/app-unaligned.apk build/apk/app-aligned.apk > /dev/null 2>&1

# Create keystore if needed
if [ ! -f "debug.keystore" ]; then
    keytool -genkey -v -keystore debug.keystore \
        -alias android -keyalg RSA -keysize 2048 -validity 10000 \
        -storepass android -keypass android \
        -dname "CN=Debug, OU=Dev, O=User, L=City, S=State, C=US" 2>/dev/null
fi

# Sign APK
$APKSIGNER sign --ks debug.keystore \
    --ks-pass pass:android \
    --key-pass pass:android \
    --out FlashlightApp.apk \
    build/apk/app-aligned.apk 2>/dev/null

echo -e "${GREEN}[8/8] Done!${NC}"
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}✓ APK created: $(pwd)/FlashlightApp.apk${NC}"
ls -lh FlashlightApp.apk
echo -e "${GREEN}========================================${NC}"

# Install if device connected
if command -v adb >/dev/null 2>&1 && adb devices 2>/dev/null | grep -q "device$"; then
    echo -e "${YELLOW}Installing to connected device...${NC}"
    adb install -r FlashlightApp.apk 2>/dev/null
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}Installed! Launching...${NC}"
        adb shell am start -n com.flashlight/.MainActivity 2>/dev/null
    fi
else
    echo -e "${YELLOW}To install: adb install FlashlightApp.apk${NC}"
    echo -e "${YELLOW}Or copy to phone and tap to install${NC}"
fi
