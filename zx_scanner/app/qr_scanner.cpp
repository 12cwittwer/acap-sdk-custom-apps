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


 Modified by Christian Wittwer
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
#include <axsdk/axparameter.h>
#include <axsdk/axevent.h>
#include <glib-object.h>
#include <glib.h>
#include <glib-unix.h>

#include <ZXing/ReadBarcode.h>
#include "send_event.h"
#include "imgprovider.h"

#define APP_NAME "ParkspassQRScanner"

using namespace cv;

static AXEventHandler* event_handler = nullptr;
static guint qr_event_id = 0;
static ImgProvider_t* provider = nullptr;
static unsigned int streamWidth;
static unsigned int streamHeight;
static std::string endpoint;
static std::string auth;
static std::string location;
static std::string entrance;
static gboolean delay_in_progress = FALSE;

static int uploadRecentEntries(const std::string& json_data, const std::string& endpoint, const std::string& auth, const std::string location, const std::string entrance);
static bool retrieveAxParameters(std::string& endpoint, std::string& auth, std::string& location, std::string& entrance, AXParameter* handle);
static gboolean process_frame(AppData* app_data);
static gboolean reset_delay_flag(gpointer user_data);
static void toLowerCase(std::string& str);
static std::string extractValue(const std::string& json, const std::string& key);

int main(void) {
    GMainLoop* main_loop = NULL;
    GError* error = nullptr;

    // Open app logs
    openlog(APP_NAME, LOG_PID | LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "Running %s example with VDO as video source", APP_NAME);

    // Create AXParameter handle
    AXParameter* handle = ax_parameter_new(APP_NAME, &error);
    if (!handle) {
        syslog(LOG_ERR, "Failed to create AXParameter: %s", error->message);
        if (error) g_error_free(error); // Free error object
        return false;
    }

    // Cleanup AXParameter handle on exit
    auto paramCleanup = [&]() { ax_parameter_free(handle); };

    // Retrieve the AxParameters from manifest file
    if (!retrieveAxParameters(endpoint, auth, location, entrance, handle)) {
        return EXIT_FAILURE;
    }

    // The desired width and height of the BGR frame
    unsigned int width  = 1280;
    unsigned int height = 720;

    // chooseStreamResolution gets the least resource intensive stream
    // that exceeds or equals the desired resolution specified above
    streamWidth  = 0;
    streamHeight = 0;
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

    // Set up event
    AppData* app_data = create_event();
    syslog(LOG_INFO, "New event created with ID: %d", app_data->event_id);

    // Setup and start running main loop
    g_timeout_add(10, (GSourceFunc)process_frame, app_data);
    main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(main_loop);

    // Application cleanup
    event_cleanup();
    paramCleanup();
    destroyImgProvider(provider);

    g_main_loop_unref(main_loop);
    return EXIT_SUCCESS;
}

static gboolean process_frame(AppData* app_data) {
    // Do not process frame if delay is in progress
    if (delay_in_progress) {
        return TRUE;
    }
    // Get the latest NV12 image frame from VDO using the imageprovider
    VdoBuffer* buf = getLastFrameBlocking(provider);
    if (!buf) {
        syslog(LOG_INFO, "No more frames available, exiting");
        return FALSE;
    }

    // Extract raw NV12 image data
    Mat nv12_mat;
    nv12_mat = Mat(streamHeight  * 3 / 2, streamWidth, CV_8UC1);
    nv12_mat.data = static_cast<uint8_t*>(vdo_buffer_get_data(buf));

    // Convert NV12 to BGR
    Mat bgr_mat;
    bgr_mat  = Mat(streamHeight, streamWidth, CV_8UC3);
    cvtColor(nv12_mat, bgr_mat, COLOR_YUV2BGR_NV12, 3);

    // Convert BGR to grayscale
    Mat grey_mat;
    cvtColor(bgr_mat, grey_mat, COLOR_BGR2GRAY);

    // Crop to the region of interest (ROI) for QR detection
    cv::Rect roi;
    roi = cv::Rect(streamWidth * 3 / 8, streamHeight * 3 / 8, streamWidth * 2 / 8, streamHeight * 2 / 8);
    cv::Mat cropped = grey_mat(roi);

    // Apply CLAHE (Contrast Limited Adaptive Histogram Equalization)
    cv::Mat clahe_result;
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8,8));
    clahe->apply(cropped, clahe_result);

    // Resize the cropped image for better resolution
    cv::Mat resized;
    cv::resize(clahe_result, resized, cv::Size(), 2.0, 2.0, cv::INTER_CUBIC);  // 2x enlargement

    // Noise reduction (using median filter)
    cv::Mat denoised;
    cv::medianBlur(resized, denoised, 3);  // 3x3 kernel

    // Sharpening using Unsharp Mask
    cv::Mat blurred, sharpened;
    GaussianBlur(denoised, blurred, cv::Size(3, 3), 0);
    cv::addWeighted(denoised, 1.5, blurred, -0.5, 0, sharpened);

    // Adaptive thresholding (better for varying lighting)
    cv::Mat binary;
    cv::adaptiveThreshold(sharpened, binary, 255, 
                        cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 25, 2);

    // Optional Morphological Closing (removes gaps in QR patterns)
    cv::Mat morph;
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(binary, morph, cv::MORPH_CLOSE, kernel);

    // Use the processed image for ZXing QR detection
    auto image = ZXing::ImageView(morph.data, morph.cols, morph.rows, ZXing::ImageFormat::Lum);
    auto options = ZXing::ReaderOptions().setFormats(ZXing::BarcodeFormat::QRCode);
    auto barcodes = ZXing::ReadBarcodes(image, options);

    // Upload any barcode data to the endpoint
    for (const auto& b : barcodes) {
        syslog(LOG_INFO, "%s: %s", ZXing::ToString(b.format()).c_str(), b.text().c_str());

        int successValue = uploadRecentEntries(b.text(), endpoint, auth, location, entrance);

        // imwrite("final_img.png", morph);
        // syslog(LOG_INFO, "Final photo saved to final_img.png");

        if(successValue == 1) {
            app_data->value = 1;
            send_event(app_data);


            // Turn on delay
            // Set a timer for turning off the delay flag
            delay_in_progress = TRUE;
            g_timeout_add(3000, reset_delay_flag, NULL);
        } else {
            app_data->value = 2;
            send_event(app_data);

            delay_in_progress = TRUE;
            g_timeout_add(3000, reset_delay_flag, NULL);
        }
    }

    returnFrame(provider, buf);
    return TRUE;
}

// Write callback is called in uploadRecentEntries()
// Collects the response from the server
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t totalSize = size * nmemb;
    response->append((char*)contents, totalSize);
    return totalSize;
}

static int uploadRecentEntries(const std::string& json_data, const std::string& endpoint, const std::string& auth, const std::string location, const std::string entrance) {

    // Construct the URL with query parameters
    std::string url = endpoint + "?park_abbr=" + curl_easy_escape(nullptr, location.c_str(), 0) +
                      "&entrance=" + curl_easy_escape(nullptr, entrance.c_str(), 0) +
                      "&scandata=" + curl_easy_escape(nullptr, json_data.c_str(), 0);

    // initialize CURL and curl buffer
    CURL* curl;
    CURLcode res;
    char error_buffer[CURL_ERROR_SIZE];
    std::string response;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = NULL;

    // In the case that we add Authorization Headers
        // std::string auth_header = "PARKSPLUS_AUTH: " + auth;
        // headers = curl_slist_append(headers, auth_header.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());  // Use the URL with params
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  // Follow redirects
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);       // Limit the number of redirects

        // Set up the write function to capture response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Check the response from the server
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            syslog(LOG_INFO, "curl_easy_perform() failed");
        } else {
            // Get http server response code
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (http_code == 200) {
                int returnValue = 0;
                std::string result = extractValue(response, "result");
                std::string message = extractValue(response, "message");
                toLowerCase(result);
                toLowerCase(message);
                if (result == "success" || message == "pass found") {
                    std::string checkin = extractValue(response, "checkin");
                    syslog(LOG_INFO, "Result: %s; Message: %s; Check-In: %s", result.c_str(), message.c_str(), checkin.c_str());
                    returnValue = 1;
                } else {
                    syslog(LOG_INFO, "Result: %s; Message: %s", result.c_str(), message.c_str());
                    if (message == "pass not found") {
                        syslog(LOG_INFO, "Pass was not found");
                        returnValue = 2;
                    } else if (message == "invalid format") {
                        syslog(LOG_INFO, "QR Code data is not in a recongnizable format");
                        returnValue = 3;
                    } else if (message == "checkin failed") {
                        syslog(LOG_INFO, "Was not able to check the visitor in");
                        returnValue = 4;
                    } else if (message == "pass expired") {
                        syslog(LOG_INFO, "Pass is expired");
                        returnValue = 5;
                    } else {
                        syslog(LOG_INFO, "Unknown pass validation error");
                        returnValue = 6;
                    }
                }
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                curl_global_cleanup();
                return returnValue;
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
    return false;
}

// Collect the parameters defined in the manifest.json file of the application
static bool retrieveAxParameters(std::string& endpoint, std::string& auth, std::string& location, std::string& entrance, AXParameter* handle) {
    GError* error = nullptr;

    try {
        gchar *param_value = NULL;

        // Retrieve parameters
        if (ax_parameter_get(handle, "ENDPOINT", &param_value, &error)) {
            endpoint = param_value;
            g_free(param_value);
        } else {
            syslog(LOG_ERR, "Failed to retrieve ENDPOINT");
        }
        if (ax_parameter_get(handle, "AUTH", &param_value, &error)) {
            auth = param_value;
            g_free(param_value);
        } else {
            syslog(LOG_ERR, "Failed to retrieve AUTH");
        }
        if (ax_parameter_get(handle, "LOCATION", &param_value, &error)) {
            location = param_value;
            g_free(param_value);
        } else {
            syslog(LOG_ERR, "Failed to retrieve LOCATION");
        }
        if (ax_parameter_get(handle, "ENTRANCE", &param_value, &error)) {
            entrance = param_value;
            g_free(param_value);
        } else {
            syslog(LOG_ERR, "Failed to retrieve ENTRANCE");
        }

        // Log parameters for debugging
        syslog(LOG_INFO, "Endpoint: %s", endpoint.c_str());
        syslog(LOG_INFO, "Auth: %s", auth.c_str());
        syslog(LOG_INFO, "Location: %s", location.c_str());
        syslog(LOG_INFO, "Entrance: %s", entrance.c_str());

        if (error) g_error_free(error); // Free error object
    } catch (const std::exception& ex) {
        syslog(LOG_ERR, "%s", ex.what());
        if (error) g_error_free(error); // Free error object
        return false;
    }
    return true;
}

// Turn off delay, allowing passes to be scanned again
static gboolean reset_delay_flag(gpointer user_data) {
    delay_in_progress = FALSE;
    return FALSE;
}

static void toLowerCase(std::string& str) {
    for (char& c : str) {
        c = std::tolower(c);
    }
}

// Extract values from the json data returned by the server
static std::string extractValue(const std::string& json, const std::string& key) {
    // Format the key to match JSON format
    std::string searchKey = "\"" + key + "\":\""; 
    size_t start = json.find(searchKey);

    // Key not found
    if (start == std::string::npos) return ""; 

    // Move to the start of the value
    start += searchKey.length(); 
    // Find the closing quote
    size_t end = json.find("\"", start); 

    // Malformed JSON
    if (end == std::string::npos) return ""; 

    return json.substr(start, end - start);
}
