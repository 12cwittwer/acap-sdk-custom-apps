# HTTPS Upload App

This README file explains how to build a the HTTPS Upload [ACAP SDK App](https://axiscommunications.github.io/acap-documentation/) and install it on your Axis Communications device.

Together with this README file, you should be able to find a directory called app. That directory contains the "https-upload" application source code which can easily be compiled and run with the help of the tools and step by step below.

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
- [Setting Parameters](#setting-custom-parameters)

## Description

The HTTPS Upload App performs the following functions:

1. **Configuration Retrieval**:
   - The app retrieves configuration parameters from the device, including the HTTPS endpoint, authentication token, upload interval, and the number of days of data to upload.

2. **Data Extraction**:
   - The app extracts recent entries from a local SQLite database (`statistics.db`) based on the number of days specified. It constructs a JSON object containing the relevant data entries.

3. **Data Upload**:
   - The app uses the `libcurl` library to upload the JSON data to the configured HTTPS endpoint. It includes the authentication token in the request headers.
   - It handles and logs HTTP responses, including successful uploads and errors.

4. **Error Handling**:
   - The app logs errors using syslog and terminates the application if critical errors occur during configuration retrieval.

5. **Continuous Operation**:
   - The app runs in a loop, performing the data extraction and upload at intervals specified by the configuration parameter.

## Exported Data
- **device_id**: User given ID of the Axis device.
- **location**: User given text description of the device location.
- **entries**: List of logged entries in from the Speed Monitor app.
  - **internal_id**: ID of tracked object in the Speed Monitor database
  - **track_id**: ID of the tracked object
  - **profile_id**: Scenario ID
  - **profile_trigger_id**: The number of times the scenario has been triggered while there is an active tracked object in the scenario
  - **classification**: Object classification (2: Unknown, 3: Human, 4: Vehicle)
  - **start_timestamp**: Epoch time in microseconds
  - **duration**: Time the vehicle spent in the track area in milliseconds
  - **min_speed**: Minimum speed of vehicle while in the track area. Can be converted to m/s by dividing speed by 280.
  - **max_speed**: Maximum speed of vehicle while in the track area. Can be converted to m/s by dividing speed by 280.
  - **avg_speed**: Average speed of vehicle while in the track area. Can be converted to m/s by dividing speed by 280.
  - **enter_speed**: Speed of the vehicle when it entered the track area. Can be converted to m/s by dividing speed by 280.
  - **exit_speed**: Speed of vehicle when it exited the track area. Can be converted to m/s by dividing speed by 280.
  - **enter_bearing**: Direction the vehicle is facing when it enters the track area in centidegrees.
  - **exit_bearing**: Direction the vehicle is facing when it exits the track area in centidegrees.
  - **flags**:

```sh
{
  device_id:
  location:
  entries: [
    {
      internal_id:
      track_id:
      profile_id:
      profile_trigger_id:
      classification:
      start_timestamp:
      duration:
      min_speed:
      max_speed:
      avg_speed:
      enter_speed:
      exit_speed:
      enter_bearing:
      exit_bearing:
      flags:
    }
  ]
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

#### Checking output

Application log can be found directly at:

```sh
http://<axis_device_ip>/axis-cgi/admin/systemlog.cgi?appname=httpsUpload
```

### Setting Custom Parameters
  - ENDPOINT: https endpoint where the data will be sent.
    - Default: *blank*
  - AUTH: Authrization password which is attached to the header PARKSPLUS_AUTH (Specific to Parkspass).
    - Default: *blank*
  - INTERVAL: The frequency in seconds the data will be sent.
    - Default: 900
  - DAYS: Number of days worth of data being sent at one time to the endpoint.
    - Default: 7
  - DEVICE: A user given ID of the device.
    - Default: *blank*
  - LOCATION: User description of device location
    - Default: *blank* 

#### Setting Parameters in Source Code
1. Find `manifest.json` in `https-upload/app`.
2. Change the `default` field of each parameter. Do NOT change parameter names.
  - For more information on Axis Parameters customization check out the [AXParameter Documentation](https://axiscommunications.github.io/acap-documentation/docs/api/src/api/axparameter/html/index.html).
3. The app must be rebuilt and installed again. No need to uninstall the old app.

#### Setting Parameters using VAPIX API
