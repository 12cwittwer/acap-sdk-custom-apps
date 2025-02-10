# Axis Custom Apps for Parkspass

Welcome to the repository for custom apps developed for Axis communication devices, tailored to address specific needs for [Parkspass](https://parkspass.utah.gov/).

## Table of Contents

- [Overview](#overview)
- [Apps](#apps)
  - [HTTPS Upload](#https-upload-app)
  - [Parkspass QR Scanner](#parkspass-qr-scanner-app)
- [License](#license)
- [Contact](#contact)

## Overview

This repository aims to provide custom applications designed to enhance the functionality of Axis communication devices. These apps address niche requirements that cater specifically to the operations of Parkspass. The current offering includes:

- **HTTPS Upload App**: Sends data collected by the Speed Monitor App to a specified HTTPS endpoint.

## Apps

### [HTTPS Upload App](https://github.com/12cwittwer/acap-sdk-custom-apps/tree/main/https-upload)

- Sends the last specified number of days' worth of data collected by the Speed Monitor App.
- Secure data transmission via HTTPS.
- Configurable endpoint and data parameters.

### [Parkspass QR Scanner App](https://github.com/12cwittwer/acap-sdk-custom-apps/tree/main/zx_scanner)

- Repeatedly checks frames from a camera for QR Codes.
- Upon successfull scan:
    - QR code data is uploaded to an endpoint.
    - An AXEvent is sent.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contact

For any questions or support, please contact Christian Wittwer at [christian.wittwer@parkspass.org](mailto:christian.wittwer@parkspass.org).
