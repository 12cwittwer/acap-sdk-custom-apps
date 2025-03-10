ARG ARCH=armv7hf
ARG REPO=axisecp
ARG SDK=acap-native-sdk
ARG UBUNTU_VERSION=24.04
ARG VERSION=12.1.0

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Set general arguments
ARG ARCH
ARG SDK_LIB_PATH_BASE=/opt/axis/acapsdk/sysroots/${ARCH}/usr
ARG BUILD_DIR=/opt/build

#-------------------------------------------------------------------------------
# Prepare build environment
#-------------------------------------------------------------------------------

# Install build dependencies for cross compiling OpenCV
RUN DEBIAN_FRONTEND=noninteractive \
    apt-get update && apt-get install -y -f --no-install-recommends \
    cmake && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

#-------------------------------------------------------------------------------
# Build OpenCV libraries
#-------------------------------------------------------------------------------

ARG OPENCV_VERSION=4.10.0
ARG OPENCV_DIR=${BUILD_DIR}/opencv
ARG OPENCV_SRC_DIR=${OPENCV_DIR}/opencv-${OPENCV_VERSION}
ARG OPENCV_BUILD_DIR=${OPENCV_DIR}/build

WORKDIR ${OPENCV_DIR}
SHELL ["/bin/bash", "-o", "pipefail", "-c"]
RUN curl -fsSL https://github.com/opencv/opencv/archive/$OPENCV_VERSION.tar.gz | tar -xz

WORKDIR ${OPENCV_BUILD_DIR}
ENV COMMON_CMAKE_FLAGS="-S $OPENCV_SRC_DIR \
        -B $OPENCV_BUILD_DIR \
        -D CMAKE_INSTALL_PREFIX=$SDK_LIB_PATH_BASE \
        -D CMAKE_BUILD_TYPE=RELEASE \
        -D WITH_OPENEXR=OFF \
        -D WITH_GTK=OFF \
        -D WITH_V4L=OFF \
        -D WITH_FFMPEG=OFF \
        -D WITH_GSTREAMER=OFF \
        -D WITH_GSTREAMER_0_10=OFF \
        -D BUILD_LIST=core,imgproc,video,objdetect,features2d,flann,calib3d,imgcodecs \
        -D BUILD_EXAMPLES=OFF \
        -D BUILD_OPENCV_APPS=OFF \
        -D BUILD_DOCS=OFF \
        -D BUILD_JPEG=ON \
        -D BUILD_PNG=OFF \
        -D WITH_JASPER=OFF \
        -D BUILD_PROTOBUF=OFF \
        -D OPENCV_GENERATE_PKGCONFIG=ON "

# hadolint ignore=SC2086
RUN if [ "$ARCH" = armv7hf ]; then \
        # Source SDK environment to get cross compilation tools
        . /opt/axis/acapsdk/environment-setup* && \
        # Configure build with CMake
        cmake \
        -D CMAKE_CXX_COMPILER=${CXX%-g++*}-g++ \
        -D CMAKE_CXX_FLAGS="${CXX#*-g++}" \
        -D CMAKE_C_COMPILER=${CC%-gcc*}-gcc \
        -D CMAKE_C_FLAGS="${CC#*-gcc}" \
        -D CMAKE_TOOLCHAIN_FILE=${OPENCV_SRC_DIR}/platforms/linux/arm-gnueabi.toolchain.cmake \
        -D CPU_BASELINE=NEON,VFPV3 \
        -D ENABLE_NEON=ON \
        -D ENABLE_VFPV3=ON \
        $COMMON_CMAKE_FLAGS && \
        # Build and install OpenCV
        make -j "$(nproc)" install ; \
    elif [ "$ARCH" = aarch64 ]; then \
        # Source SDK environment to get cross compilation tools
        . /opt/axis/acapsdk/environment-setup* && \
        # Configure build with CMake
        # No need to set NEON and VFP for aarch64 since they are implicitly
        # present in an any standard armv8-a implementation.
        cmake \
        -D CMAKE_CXX_COMPILER=${CXX%-g++*}-g++ \
        -D CMAKE_CXX_FLAGS="${CXX#*-g++}" \
        -D CMAKE_C_COMPILER=${CC%-gcc*}-gcc \
        -D CMAKE_C_FLAGS="${CC#*-gcc}" \
        -D CMAKE_TOOLCHAIN_FILE=${OPENCV_SRC_DIR}/platforms/linux/aarch64-gnu.toolchain.cmake \
        $COMMON_CMAKE_FLAGS && \
        # Build and install OpenCV
        make -j "$(nproc)" install ; \
    else \
        printf "Error: '%s' is not a valid value for the ARCH variable\n", "$ARCH"; \
        exit 1; \
    fi

#-------------------------------------------------------------------------------
# Copy the built library files to application directory
#-------------------------------------------------------------------------------

# !All library files are copied below!
# WORKDIR /opt/app
# COPY ./app .
# RUN mkdir lib && \
#     cp -P ${OPENCV_BUILD_DIR}/lib/lib*.so* ./lib/

#-------------------------------------------------------------------------------
# Build ZXing Libraries
#-------------------------------------------------------------------------------

ARG ZXING_VERSION=2.2.0

# Add a section to build ZXing libraries
ARG ZXING_DIR=${BUILD_DIR}/zxing
ARG ZXING_SRC_DIR=${ZXING_DIR}/zxing-cpp-${ZXING_VERSION}
ARG ZXING_BUILD_DIR=${ZXING_DIR}/build

# Fetch ZXing source code
WORKDIR ${ZXING_DIR}
RUN curl -fsSL https://github.com/zxing-cpp/zxing-cpp/archive/refs/tags/v${ZXING_VERSION}.tar.gz | tar -xz
RUN ls -l /opt/build/zxing

# Build and install ZXing library
WORKDIR ${ZXING_BUILD_DIR}
RUN if [ "$ARCH" = armv7hf ]; then \
        # Source SDK environment to get cross compilation tools
        . /opt/axis/acapsdk/environment-setup* && \
        # Configure build with CMake for armv7hf
        cmake \
        -D CMAKE_INSTALL_PREFIX=$SDK_LIB_PATH_BASE \
        -D CMAKE_BUILD_TYPE=Release \
        -D BUILD_SHARED_LIBS=ON \
        -D CMAKE_TOOLCHAIN_FILE=${OPENCV_SRC_DIR}/platforms/linux/arm-gnueabi.toolchain.cmake \
        -S $ZXING_SRC_DIR \
        -B $ZXING_BUILD_DIR && \
        # Build and install ZXing
        make -j "$(nproc)" install ; \
    elif [ "$ARCH" = aarch64 ]; then \
        # Source SDK environment to get cross compilation tools
        . /opt/axis/acapsdk/environment-setup* && \
        # Configure build with CMake for aarch64
        cmake \
        -D CMAKE_INSTALL_PREFIX=$SDK_LIB_PATH_BASE \
        -D CMAKE_BUILD_TYPE=Release \
        -D BUILD_SHARED_LIBS=ON \
        -D CMAKE_TOOLCHAIN_FILE=${OPENCV_SRC_DIR}/platforms/linux/aarch64-gnu.toolchain.cmake \
        -S $ZXING_SRC_DIR \
        -B $ZXING_BUILD_DIR && \
        # Build and install ZXing
        make -j "$(nproc)" install ; \
    else \
        printf "Error: '%s' is not a valid value for the ARCH variable\n", "$ARCH"; \
        exit 1; \
    fi

#-------------------------------------------------------------------------------
# Copy the built ZXing library files to application directory
#-------------------------------------------------------------------------------

COPY ./app /opt/app/
WORKDIR /opt/app
RUN mkdir lib && \
    cp -P ${OPENCV_BUILD_DIR}/lib/lib*.so* ./lib/ && \
    cp -P ${ZXING_BUILD_DIR}/core/libZXing* ./lib/

#-------------------------------------------------------------------------------
# Finally build the ACAP application
#-------------------------------------------------------------------------------

RUN . /opt/axis/acapsdk/environment-setup* && acap-build .
