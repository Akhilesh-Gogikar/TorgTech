# TorgTech Android TensorFlow Lite Apps

GPU-accelerated Android NDK examples for running TensorFlow Lite computer-vision models on-device. The repository is useful for Android developers evaluating native TFLite GPU Delegate builds for driver-monitoring and ADAS-style prototypes.

## Repository Map

- `Mati_DSMS/`: camera-based face detection and face/iris landmark experiments using bundled MediaPipe TensorFlow Lite models.
- `Tarzan_ADAS/`: dense depth, segmentation, object detection, and face mesh examples packaged as an Android NDK app.
- `Sahay_ADAS/`: additional ADAS app source and assets covering dense depth, detection, segmentation, and face mesh models.
- `third_party/`: TensorFlow Lite build scripts and checked-in support code used by the Android app projects.

## 1. How to Build & Run

### 1.1 setup environment

- Download and install [Android NDK](https://developer.android.com/ndk/downloads).

```
$ mkdir ~/Android/
$ mv ~/Download/android-ndk-r20b-linux-x86_64.zip ~/Android
$ cd ~/Android
$ unzip android-ndk-r20b-linux-x86_64.zip
```

- Download and install [bazel](https://docs.bazel.build/versions/master/install-ubuntu.html).

```
$ wget https://github.com/bazelbuild/bazel/releases/download/3.1.0/bazel-3.1.0-installer-linux-x86_64.sh
$ chmod 755 bazel-3.1.0-installer-linux-x86_64.sh
$ sudo ./bazel-3.1.0-installer-linux-x86_64.sh
```

### 1.2 build TensorFlow Lite library and GPU Delegate library

- run the build script to build TensorFlow Library

```
$ mkdir ~/work
$ git clone https://github.com/Akhilesh-Gogikar/TorgTech.git
$ cd TorgTech/third_party/
$ ./build_libtflite_r2.4_android.sh

(Tensorflow configure will start after a while. Please enter according to your environment)

$ ls -l tensorflow/bazel-bin/tensorflow/lite/

$ ls -l tensorflow/bazel-bin/tensorflow/lite/delegates/gpu/
```



### 1.3 Download the needed assets

```
$ cd ~/work/TorgTech
$ ./download_all_assets.sh
```


### 1.4 Build Android Applications
- Download and install [Android Studio](https://developer.android.com/studio/install).
- Start Android Studio.

```
$ cd ${ANDROID_STUDIO_INSTALL_DIR}/android-studio/bin/
$ ./studio.sh
```

- Install NDK 20.0 by SDK Manager of Android Studio.
- Open an application folder such as `~/work/TorgTech/Mati_DSMS` or `~/work/TorgTech/Tarzan_ADAS`.
- Build and Run.

## 3. Tested Environment

| Host PC             | Target Device           |
|:-------------------:|:-----------------------:|
| x86_64              | arm64-v8a               |
| Ubuntu 18.04.4 LTS  | Android 9 (API Level 28)|
| Android NDK r20b    |                         |
