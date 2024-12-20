/**
 * Copyright (C) 2021 Axis Communications AB, Lund, Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#include <opencv2/imgproc.hpp>
#pragma GCC diagnostic pop
#include <opencv2/video.hpp>
#include <stdlib.h>
#include <syslog.h>
#include <opencv2/imgcodecs.hpp>

#include <ZXing/ReadBarcode.h>

#include "imgprovider.h"

using namespace cv;

int main(void) {
    openlog("opencv_app", LOG_PID | LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "Running OpenCV example with VDO as video source");
    ImgProvider_t* provider = NULL;

    // The desired width and height of the BGR frame
    unsigned int width  = 1280;
    unsigned int height = 720;

    // chooseStreamResolution gets the least resource intensive stream
    // that exceeds or equals the desired resolution specified above
    unsigned int streamWidth  = 0;
    unsigned int streamHeight = 0;
    if (!chooseStreamResolution(width, height, &streamWidth, &streamHeight)) {
        syslog(LOG_ERR, "%s: Failed choosing stream resolution", __func__);
        exit(1);
    }

    syslog(LOG_INFO,
           "Creating VDO image provider and creating stream %d x %d",
           streamWidth,
           streamHeight);
    provider = createImgProvider(streamWidth, streamHeight, 2, VDO_FORMAT_YUV);
    if (!provider) {
        syslog(LOG_ERR, "%s: Failed to create ImgProvider", __func__);
        exit(2);
    }

    syslog(LOG_INFO, "Start fetching video frames from VDO");
    if (!startFrameFetch(provider)) {
        syslog(LOG_ERR, "%s: Failed to fetch frames from VDO", __func__);
        exit(3);
    }

    // Create OpenCV Mats for the camera frame (nv12), the converted frame (bgr)
    // and the foreground frame that is outputted by the background subtractor
    Mat bgr_mat  = Mat(streamHeight, streamWidth, CV_8UC3);
    Mat nv12_mat = Mat(streamHeight  * 3 / 2, streamWidth, CV_8UC1);
    Mat grey_mat;

    // Crop area being scanned
    cv::Rect roi(streamWidth * 1 / 3, streamHeight * 1 / 4, streamWidth * 1 / 3, streamHeight * 1 / 2);

    while (true) {
        // Get the latest NV12 image frame from VDO using the imageprovider
        VdoBuffer* buf = getLastFrameBlocking(provider);
        if (!buf) {
            syslog(LOG_INFO, "No more frames available, exiting");
            exit(0);
        }

        nv12_mat.data = static_cast<uint8_t*>(vdo_buffer_get_data(buf));

        // imwrite("nv12.png", nv12_mat);
        // syslog(LOG_INFO, "nv12 image saved to nv12.png");

        // Convert the NV12 data to BGR
        cvtColor(nv12_mat, bgr_mat, COLOR_YUV2BGR_NV12, 3);

        // imwrite("bgr_frame.png", bgr_mat);
        // syslog(LOG_INFO, "BGR image saved to bgr_frame.png");

        cvtColor(bgr_mat, grey_mat, COLOR_BGR2GRAY);

        // imwrite("greyscale_img.png", grey_mat);
        // syslog(LOG_INFO, "Greyscale image saved to greyscale_img.png");

        cv::Mat cropped = grey_mat(roi);

        // Resize the cropped image to enhance resolution
        cv::Mat resized;
        cv::resize(cropped, resized, cv::Size(), 2.0, 2.0, cv::INTER_CUBIC); // 2x enlargement

        cv::Mat result = resized.clone();

        // Increase black and white contrast
        cv::Mat high_contrast;
        resized.convertTo(high_contrast, -1, 2.5, -200); // Increase contrast

        // Set all pixels with a brightness greater than 100 to white
        double threshold_value = 100;
        for (int y = 0; y < high_contrast.rows; ++y) {
            for (int x = 0; x < high_contrast.cols; ++x) {
                if (high_contrast.at<uchar>(y, x) >= threshold_value) {
                    result.at<uchar>(y, x) = 255; // Set pixel to white
                } else if (high_contrast.at<uchar>(y,x) < threshold_value) {
                    result.at<uchar>(y,x) = 0; // Set pixel to black
                }
            }
        }

        // imwrite("altered_img.png", result);
        // syslog(LOG_INFO, "Final photo saved to altered_img.png");

        auto image = ZXing::ImageView(result.data, result.cols, result.rows, ZXing::ImageFormat::Lum);
        auto options = ZXing::ReaderOptions().setFormats(ZXing::BarcodeFormat::QRCode);
        auto barcodes = ZXing::ReadBarcodes(image, options);

        for (const auto& b : barcodes)
            syslog(LOG_INFO, "%s: %s", ZXing::ToString(b.format()).c_str(), b.text().c_str());

        returnFrame(provider, buf);
        break;
    }
    return EXIT_SUCCESS;
}
