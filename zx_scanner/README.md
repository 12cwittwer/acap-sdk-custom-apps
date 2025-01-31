# Parkspass QR Scanner App

This README file explains how to build a the Parkspass QR Scanner [ACAP SDK App](https://axiscommunications.github.io/acap-documentation/) and install it on your Axis Communications device.

Together with this README file, you should be able to find a directory called app. That directory contains the application source code which can easily be compiled and run with the help of the tools and step by step below.

## Table of Contents
- [Description](#description)
- [Exported Data](#exported-data)
- [Getting Started](#getting-started)
- [Software Requirements](#software-requirements)
- [How to Run The Code](#how-to-run-the-code)
  - [Build the Application](#build-the-application)
  - [Install the App](#install-your-application)
      - [Accessing Device Web Interface](#steps-to-access-the-web-interface)
      - [Checking Output](#checking-output)
- [Event](#event)

## Description

The Parkspass QR Scanner App performs the following functions:

1. **Scans QR Codes**:
   - The app using the [ZXing-C++](https://github.com/zxing-cpp/zxing-cpp) barcode scanner library to detect and decode QR codes.

2. **Data Upload**:
   - The app collects the data decoded from the QR Code and sends it to an Endpoint variable.
   - Data is uploaded upon each successfull scan.
   - After a successful scan, scanning is delayed for a few seconds.

3. **Send Event**:
   - The app sends an event with the AXEvent API.
   - Event can be subscribed to under the name BarcodeScanned.
   - SuccessValue is sent based on server response.

4. **Continuous Operation**:
   - The app runs on a loop, continually checking frames for a QR Code.

## Uploaded Data
- **location**: User given ID of the Axis device.
- **data**: Data from the QR Code scanned
- **device_id**: List of logged entries in from the Speed Monitor app.
```sh
{
  "location": "UTSNOW",
  "data": "https://parkspass.utah.gov/activate/C00000767",
  "device_id": "001"
}
```

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the app:

```sh
https-upload
├── app
│   ├── httpsUpload.c
│   ├── LICENSE
│   ├── Makefile
│   └── manifest.json
├── Dockerfile
└── README.md
```

- **app/qr_scanner.cpp** - Central code in charge of scanning for QR Codes, and managing responses to successfull scans.
- **app/imgprovider.cpp** - Copied from the [Axis Native Examples](https://github.com/AxisCommunications/acap-native-sdk-examples/blob/main/using-opencv/app/imgprovider.cpp). Manages streaming configuration and image buffering.
- **app/send_event.c** - Heavily adapted version of the send_event.c example from [Axis Native Examples](https://github.com/AxisCommunications/acap-native-sdk-examples/blob/main/axevent/send_event/app/send_event.c). Manages declaring and sending events.
- **app/LICENSE** - Text file which lists all open source licensed source code distributed with the application.
- **app/Makefile** - Makefile containing the build and link instructions for building the ACAP application.
- **app/manifest.json** - Defines the application and its configuration.
- **Dockerfile** - Docker file with the specified Axis toolchain and API container to build the app.
- **README.md** - Step by step instructions on how to run the app.

### Software Requirements

    - Docker: Required to make the container environment.
    - GNU Make: Required for building the application using the provided Makefile
    - C Compiler (GCC): Required to compile the source code.

### How to run the code

Below is the step by step instructions on how to run the app.

#### Build the application

Standing in your working directory run the following commands:

```sh
docker build --tag <APP_IMAGE> .
```

<APP_IMAGE> is the name to tag the image with, e.g., zx_scanner

Default architecture is**aarch64**. To build for **armv7hf** it's possible to
update the _ARCH_ variable in the Dockerfile or to set it in the docker build
command via build argument:

```sh
docker build --build-arg ARCH=aarch64 --tag <APP_IMAGE> .
```

Copy the result from the container image to a local directory build:

```sh
docker cp $(docker create <APP_IMAGE>):/opt/app ./build
```

The working dir now contains a build folder with the following files:

```sh
.
├── LICENSE
├── Makefile
├── Parkspass_QR_Scanner_1_0_0_LICENSE.txt
├── Parkspass_QR_Scanner_1_0_0_aarch64.eap
├── imgprovider.cpp
├── imgprovider.h
├── imgprovider.o
├── lib
│   ├── libZXing.so -> libZXing.so.3
│   ├── libZXing.so.2.2.0
│   ├── libZXing.so.3 -> libZXing.so.2.2.0
│   ├── libopencv_calib3d.so -> libopencv_calib3d.so.410
│   ├── libopencv_calib3d.so.4.10.0
│   ├── libopencv_calib3d.so.410 -> libopencv_calib3d.so.4.10.0
│   ├── libopencv_core.so -> libopencv_core.so.410
│   ├── libopencv_core.so.4.10.0
│   ├── libopencv_core.so.410 -> libopencv_core.so.4.10.0
│   ├── libopencv_features2d.so -> libopencv_features2d.so.410
│   ├── libopencv_features2d.so.4.10.0
│   ├── libopencv_features2d.so.410 -> libopencv_features2d.so.4.10.0
│   ├── libopencv_flann.so -> libopencv_flann.so.410
│   ├── libopencv_flann.so.4.10.0
│   ├── libopencv_flann.so.410 -> libopencv_flann.so.4.10.0
│   ├── libopencv_imgcodecs.so -> libopencv_imgcodecs.so.410
│   ├── libopencv_imgcodecs.so.4.10.0
│   ├── libopencv_imgcodecs.so.410 -> libopencv_imgcodecs.so.4.10.0
│   ├── libopencv_imgproc.so -> libopencv_imgproc.so.410
│   ├── libopencv_imgproc.so.4.10.0
│   ├── libopencv_imgproc.so.410 -> libopencv_imgproc.so.4.10.0
│   ├── libopencv_objdetect.so -> libopencv_objdetect.so.410
│   ├── libopencv_objdetect.so.4.10.0
│   ├── libopencv_objdetect.so.410 -> libopencv_objdetect.so.4.10.0
│   ├── libopencv_video.so -> libopencv_video.so.410
│   ├── libopencv_video.so.4.10.0
│   └── libopencv_video.so.410 -> libopencv_video.so.4.10.0
├── manifest.json
├── package.conf
├── package.conf.orig
├── param.conf
├── parkspass_qr_scanner
├── qr_scanner.cpp
├── qr_scanner.o
├── send_event.c
├── send_event.h
└── send_event.o
```

- **build/Parkspass_QR_Scanner_1_0_0_aarch64.eap** - Application package .eap file. Uploaded to the Axis device to install application.

#### Install your application

##### Steps to Access the Web Interface

1. **Find the IP Address of the Device**:

   - You can use the Axis IP Utility or Axis Camera Management tool to find the IP address of your device. Alternatively, you can check your router's DHCP client list.

2. **Open a Web Browser**:

   - Launch your preferred web browser.

3. **Enter the IP Address in the Browser**:

   - In the address bar of the web browser, type the IP address of your Axis device and press `Enter`. For example:
     ```
     http://192.168.0.90
     ```
   - If your device is configured to use HTTPS, use:
     ```
     https://192.168.0.90
     ```

4. **Log In to the Device**:

   - You will be prompted to enter the login credentials for your Axis device.
   - The default username is usually `root`, and the default password is often set during the initial setup. If you have not changed these credentials, use the default values. If you have, use the updated credentials.

5. **Access the Web Interface**:
   - After logging in, you will have access to the web interface of your Axis device. From here, you can configure settings, view live video feeds, and manage your device.

_Goto your device web page above > Click on the tab **App** in the device GUI > Add **(+)** sign and browse to
the newly built **httpsUpload_1_0_1_aarch64.eap** > Click **Install** > Run the application by enabling the **Start** switch_

#### Checking output

Application log can be found directly at:

```sh
http://<axis_device_ip>/axis-cgi/admin/systemlog.cgi?appname=parkspass_qr_scanner
```
## Event
  This application takes advantage of the built in AXEvent API. Upon each successfull scan of an QR Code, the application will send an event with a SuccessValue field.
  SuccessValue will have two values based on the server response:
    - 1: 200 response code. In our use case, a valid passes.
    - 2: Any other response code. In our case, a bad pass or no communication to the server.

  **Subscribing to the Event**
  On the Axis Communications device Web Interface:
  
    - Go to `System > Events`.
    
    - Under `Rules` click `+ Add Rule`.
  
    - In the `Condition` section, click `Select a Condition` and choose `BarcodeScanned`.
