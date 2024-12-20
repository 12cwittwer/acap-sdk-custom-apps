#include <opencv2/imgproc.hpp>
#include <opencv2/video.hpp>
#include <opencv2/objdetect.hpp>
#include <stdlib.h>
#include <syslog.h>
#include <opencv2/imgcodecs.hpp>

#include "imgprovider.h"

using namespace cv;

int main(int argc, char* argv[]) {
    openlog("opencv_app", LOG_PID | LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "Starting up QR scanner app");

    unsigned int width  = 1024;
    unsigned int height = 576;

    unsigned int stream_width = 0;
    unsigned int stream_height = 0;
    if (!chooseStreamResolution(width, height, &stream_width, &stream_height)) {
        syslog(LOG_ERR, "%s: Failed choosing stream resolution", __func__);
        exit(1);
    }

    syslog(LOG_INFO, "Starting Image Provider %d x %d", stream_width, stream_height);
    ImgProvider_t* provider = createImgProvider(1024, 576, 2, VDO_FORMAT_YUV);
    if (!provider) {
        syslog(LOG_ERR, "Failed to create ImgProvider");
        return EXIT_FAILURE;
    }

    syslog(LOG_INFO, "Starting Frame Fetching");
    if (!startFrameFetch(provider)) {
        syslog(LOG_ERR, "Failed to Fetch Frames");
        return EXIT_FAILURE;
    }

    QRCodeDetector qr_detector;

    Mat bgr_mat  = Mat(height, width, CV_8UC3);
    Mat nv12_mat = Mat(height * 3 / 2, width, CV_8UC1);

    while (true) {
        VdoBuffer* buf = getLastFrameBlocking(provider);
        if (!buf) {
            syslog(LOG_INFO, "No more frames, exiting");
            break;
        }

        nv12_mat.data = static_cast<uint8_t*>(vdo_buffer_get_data(buf));

        imwrite("original_frame.png", nv12_mat);
        syslog(LOG_INFO, "Original frame saved to original_frame.png");

        cvtColor(nv12_mat, bgr_mat, COLOR_YUV2BGR_NV12, 3);

        imwrite("bgr_frame.png", bgr_mat);
        syslog(LOG_INFO, "BGR frame saved to bgr_frame.png");

        // Detect and decode multiple QR codes
        std::vector<std::string> decodedTexts;
        std::vector<std::vector<Point>> points;

        bool detected = qr_detector.detectAndDecodeMulti(bgr_mat, decodedTexts, points);

        if (detected && !decodedTexts.empty()) {
            for (size_t i = 0; i < decodedTexts.size(); ++i) {
                syslog(LOG_INFO, "QR Code %zu detected: %s", i + 1, decodedTexts[i].c_str());

                // Draw the QR code quadrilateral
                const std::vector<Point>& qr_points = points[i];
                if (qr_points.size() == 4) {
                    for (size_t j = 0; j < 4; j++) {
                        line(bgr_mat, qr_points[j], qr_points[(j + 1) % 4], Scalar(0, 255, 0), 2);
                    }
                }
            }
        }

        returnFrame(provider, buf);
        break;
    }

    stopFrameFetch(provider);
    return EXIT_SUCCESS;
}
