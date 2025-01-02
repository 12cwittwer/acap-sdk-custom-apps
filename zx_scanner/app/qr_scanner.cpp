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
#include <curl/curl.h>
#include <glib.h>

#include <ZXing/ReadBarcode.h>

#include "imgprovider.h"

using namespace cv;

static void uploadRecentEntries(const std::string& json_data);

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

        result = high_contrast; // Inserted to allow removal of thresholding. Needs to be cleaned up later.

        // Set all pixels with a brightness greater than 100 to white
        // Thresholding does not work properly in low light environments
        // double threshold_value = 100;
        // for (int y = 0; y < high_contrast.rows; ++y) {
        //     for (int x = 0; x < high_contrast.cols; ++x) {
        //         if (high_contrast.at<uchar>(y, x) >= threshold_value) {
        //             result.at<uchar>(y, x) = 255; // Set pixel to white
        //         } else if (high_contrast.at<uchar>(y,x) < threshold_value) {
        //             result.at<uchar>(y,x) = 0; // Set pixel to black
        //         }
        //     }
        // }

        imwrite("altered_img.png", result);
        // syslog(LOG_INFO, "Final photo saved to altered_img.png");

        auto image = ZXing::ImageView(result.data, result.cols, result.rows, ZXing::ImageFormat::Lum);
        auto options = ZXing::ReaderOptions().setFormats(ZXing::BarcodeFormat::QRCode);
        auto barcodes = ZXing::ReadBarcodes(image, options);

        for (const auto& b : barcodes) {
            syslog(LOG_INFO, "%s: %s", ZXing::ToString(b.format()).c_str(), b.text().c_str());
            // Uncomment when QR scanner is working effectively
            uploadRecentEntries(b.text());
        }

        returnFrame(provider, buf);
    }
    return EXIT_SUCCESS;
}


static void uploadRecentEntries(const std::string& json_data) {
    std::string endpoint = "https://5487-144-38-176-40.ngrok-free.app/messages";
    std::string auth = "My authorization";
    std::string location = "UTSAND";
    std::string device_id = "SAND_SOUTH";

    std::string json_body = "{"
        "\"location\": \"" + location + "\","
        "\"device_id\": \"" + device_id + "\","
        "\"data\": \"" + json_data +
    "\"}";
    
    CURL* curl;
    CURLcode res;
    char error_buffer[CURL_ERROR_SIZE];

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        std::string auth_header = "PARKSPLUS_AUTH: " + auth;
        headers = curl_slist_append(headers, auth_header.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  // Follow redirects
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);       // Limit the number of redirects to follow

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            syslog(LOG_INFO, "curl_easy_perform() failed");
        } else {
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (http_code == 200) {
                syslog(LOG_INFO, "Data uploaded succesfully");
            } else {
                syslog(LOG_INFO, "Data was not successfully uploaded");
            }
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
        syslog(LOG_ERR, "Failed to initialize CURL");
    }

    curl_global_cleanup();
}