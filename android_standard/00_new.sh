#!/usr/bin/bash

APP="flashlight"
mkdir ${APP}-app
cd ${APP}-app


mkdir -p src/com/${APP} app/src/main/res/layout app/src/main/res/values

set -e  # Stop on error

echo "Setting up Android build tools on Ubuntu..."

# Install system packages
sudo apt update
sudo apt install -y openjdk-17-jdk wget unzip zip

# Download Android command-line tools
cd ~
if [ ! -f "commandlinetools-linux-11076708_latest.zip" ]; then
    wget https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip
fi

# Extract tools
rm -rf Android
mkdir -p Android/cmdline-tools
unzip -q commandlinetools-linux-11076708_latest.zip -d Android/cmdline-tools
mv Android/cmdline-tools/cmdline-tools Android/cmdline-tools/latest

# Set environment variables for this session
export ANDROID_HOME=$HOME/Android
export PATH=$PATH:$ANDROID_HOME/cmdline-tools/latest/bin
export PATH=$PATH:$ANDROID_HOME/platform-tools
export PATH=$PATH:$ANDROID_HOME/build-tools/30.0.3

# Add to .bashrc for future sessions
if ! grep -q "ANDROID_HOME" ~/.bashrc; then
    echo "export ANDROID_HOME=\$HOME/Android" >> ~/.bashrc
    echo 'export PATH=$PATH:$ANDROID_HOME/cmdline-tools/latest/bin' >> ~/.bashrc
    echo 'export PATH=$PATH:$ANDROID_HOME/platform-tools' >> ~/.bashrc
    echo 'export PATH=$PATH:$ANDROID_HOME/build-tools/30.0.3' >> ~/.bashrc
fi

# Accept licenses (automatic)
echo "Accepting Android licenses..."
yes | $ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager --licenses > /dev/null 2>&1 || true

# Install required components
echo "Installing SDK components..."
$ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager "platform-tools" > /dev/null 2>&1
$ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager "platforms;android-30" > /dev/null 2>&1
$ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager "build-tools;30.0.3" > /dev/null 2>&1

echo "✓ Android build tools installed successfully!"
echo ""
echo "Now run: bash build.sh"
