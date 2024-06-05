ARG ARCH=aarch64
ARG VERSION=1.14
ARG UBUNTU_VERSION=22.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION} as sdk

# Set arguments used in build of both libraries
ARG ARCH=aarch64
ARG SDK_LIB_PATH_BASE=/opt/axis/acapsdk/sysroots/${ARCH}/usr
ARG APP_RPATH=/usr/local/packages/sqlite_example
ARG BUILD_DIR=/opt/build

# Install build dependencies for cross compiling sqlite3
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    git \
    build-essential \
    autoconf \
    libtool \
    automake \
    tcl \
    ca-certificates \
    pkg-config \
    libglib2.0-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

#-------------------------------------------------------------------------------
# Build sqlite3 library
#-------------------------------------------------------------------------------

ARG SQLITE_BUILD_PATH=${BUILD_DIR}/sqlite
ARG SQLITE_BUILD_DIR=${SQLITE_BUILD_PATH}/sqlite

# Clone sqlite3 source code
WORKDIR ${SQLITE_BUILD_PATH}
RUN git clone https://github.com/sqlite/sqlite.git

WORKDIR ${SQLITE_BUILD_DIR}
# Source the SDK environment to get correct cross compiler
RUN . /opt/axis/acapsdk/environment-setup* && \
    ./configure --prefix=${PWD}/build ${CONFIGURE_FLAGS} && \
    make && \
    make install

#-------------------------------------------------------------------------------
# Copy the built library files to application directory
#-------------------------------------------------------------------------------

WORKDIR /opt/app
COPY ./app .
RUN mkdir lib && \
    cp -r ${SQLITE_BUILD_DIR}/build/lib/libsqlite3.so* lib

#-------------------------------------------------------------------------------
# Finally build the ACAP application
#-------------------------------------------------------------------------------

RUN . /opt/axis/acapsdk/environment-setup* && \
    acap-build ./
