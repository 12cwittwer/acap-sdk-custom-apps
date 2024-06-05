# Axis Custom Apps for Parkspass

Welcome to the repository for custom apps developed for Axis communication devices, tailored to address specific needs for Parkspass.

## Table of Contents

- [Overview](#overview)
- [HTTPS Upload App](#https-upload-app)
  - [Features](#features)
  - [Installation](#installation)
  - [Configuration](#configuration)
  - [Usage](#usage)
- [License](#license)
- [Contact](#contact)

## Overview

This repository aims to provide custom applications designed to enhance the functionality of Axis communication devices. These apps address niche requirements that cater specifically to the operations of Parkspass. The current offering includes:

- **HTTPS Upload App**: Sends data collected by the Speed Monitor App to a specified HTTPS endpoint.

## HTTPS Upload App

### Features

- Sends the last specified number of days' worth of data collected by the Speed Monitor App.
- Secure data transmission via HTTPS.
- Configurable endpoint and data parameters.

### Installation

1. **Clone the Repository**

    ```sh
    git clone https://github.com/12cwittwer/axis-custom-apps.git
    ```

2. **Navigate to the App Directory**

    ```sh
    cd axis-custom-apps/https-upload-app
    ```

3. **Upload to Axis Device**

   - Follow the standard procedure for uploading apps to your Axis device. This usually involves accessing the device's web interface and uploading the app package.

### Configuration

1. **Open Configuration File**

    - Locate the `config.json` file in the app directory.

2. **Edit Configuration Parameters**

    - `endpoint`: The HTTPS endpoint to which data will be uploaded.
    - `days`: Number of days' worth of data to be sent.
    - Example:

    ```json
    {
      "endpoint": "https://your-endpoint.com/upload",
      "days": 7
    }
    ```

3. **Save and Upload**

    - Save the changes to `config.json`.
    - Re-upload the app to the Axis device if necessary.

### Usage

1. **Activate the App**

    - Ensure the app is activated via the Axis device's web interface.

2. **Data Upload**

    - The app will automatically send data to the configured HTTPS endpoint based on the specified number of days.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contact

For any questions or support, please contact [yourname@yourdomain.com](mailto:yourname@yourdomain.com).
