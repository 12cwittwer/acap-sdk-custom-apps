# HTTPS Upload App

This README file explains how to build a the HTTPS Upload app and install it on your Axis Communications device.

Together with this README file, you should be able to find a directory called app. That directory contains the "https-upload" application source code which can easily be compiled and run with the help of the tools and step by step below.

## Table of Contents
- [Getting Started](#getting-started)
- [Software Requirements](#software-requirements)
- [How to Run The Code](#how-to-run-the-code)
  - [Build the Application](#build-the-application)
  - [Install the App](#install-your-application)
      - [Accessing Device Web Interface](#steps-to-access-the-web-interface)
      - [Expected Output](#the-expected-output)
- [Setting Parameters](#setting-custom-parameters)

## Getting started

These instructions will guide you on how to execute the code. Below is the structure and scripts used in the example:

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

- **app/httpsUpload.c** - HTTPS Upload app that uploads data to endpoint.
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

<APP_IMAGE> is the name to tag the image with, e.g., https_upload:1.0

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
https-upload
├── app
│   ├── httpsUpload.c
│   ├── LICENSE
│   ├── Makefile
│   └── manifest.json
├── build
│   ├── httpsUpload*
│   ├── httpsUpload_1_0_1_aarch64.eap
│   ├── httpsUpload_1_0_1_LICENSE.txt
│   ├── httpsUpload.c
│   ├── LICENSE
│   ├── Makefile
│   ├── manifest.json
│   ├── package.conf
│   ├── package.conf.orig
│   └── param.conf
├── Dockerfile
└── README.md
```

- **build/httpsUpload\*** - Application executable binary file.
- **build/httpsUpload_1_0_1_aarch64.eap** - Application package .eap file.
- **build/httpsUpload_1_0_1_LICENSE.txt** - Copy of LICENSE file.
- **build/manifest.json** - Defines the application and its configuration.
- **build/package.conf** - Defines the application and its configuration.
- **build/package.conf.orig** - Defines the application and its configuration, original file.
- **build/param.conf** - File containing application parameters.

#### Install your application

##### Steps to Access the Web Interface

1. **Find the IP Address of the Device**:

   - You can use the Axis IP Utility or Axis Camera Management tool to find the IP address of your device. Alternatively, you can check your router's DHCP client list.

2. **Open a Web Browser**:

   - Launch your preferred web browser.

3. **Enter the IP Address in the Browser**:

   - In the address bar of the web browser, type the IP address of your Axis device and press `Enter`. For example:
     ```
     http://192.168.0.100
     ```
   - If your device is configured to use HTTPS, use:
     ```
     https://192.168.0.100
     ```

4. **Log In to the Device**:

   - You will be prompted to enter the login credentials for your Axis device.
   - The default username is usually `root`, and the default password is often set during the initial setup. If you have not changed these credentials, use the default values. If you have, use the updated credentials.

5. **Access the Web Interface**:
   - After logging in, you will have access to the web interface of your Axis device. From here, you can configure settings, view live video feeds, and manage your device.

_Goto your device web page above > Click on the tab **App** in the device GUI > Add **(+)** sign and browse to
the newly built **httpsUpload_1_0_1_aarch64.eap** > Click **Install** > Run the application by enabling the **Start** switch_

#### The expected output

Application log can be found directly at:

```sh
http://<axis_device_ip>/axis-cgi/admin/systemlog.cgi?appname=httpsUpload
```

### Setting Custom Parameters
