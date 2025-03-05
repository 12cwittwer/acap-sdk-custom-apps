TARGET = ParkspassQRScanner
C_SOURCES = send_event.c
CPP_SOURCES = $(wildcard *.cpp)
OBJECTS = $(C_SOURCES:.c=.o) $(CPP_SOURCES:.cpp=.o) 
PKGS = gio-2.0 gio-unix-2.0 vdostream libcurl axparameter axevent

CXXFLAGS += -Os -pipe -std=c17
CXXFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags-only-I $(PKGS))
CFLAGS += $(CXXFLAGS)  # Use same flags for C files
CFLAGS += -Wall \
          -Wextra \
          -Wformat=2 \
          -Wpointer-arith \
          -Wbad-function-cast \
          -Wstrict-prototypes \
          -Wmissing-prototypes \
          -Winline \
          -Wdisabled-optimization \
          -Wfloat-equal \
          -W \
          -Werror
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

OPENCV_INCLUDE ?= $(SDKTARGETSYSROOT)/usr/include/opencv4
CXXFLAGS += -I$(OPENCV_INCLUDE)
LDFLAGS = -L./lib -Wl,--no-as-needed,-rpath,'$$ORIGIN/lib'
LDLIBS += -lopencv_video -lopencv_imgproc -lopencv_core -lpthread \
          -lopencv_objdetect -lopencv_features2d -lopencv_calib3d \
          -lopencv_flann -lopencv_imgcodecs -lZXing

STRIP ?= strip

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(LDLIBS) $^ -o $@ ; \
	$(STRIP) --strip-unneeded $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) *.o $(TARGET) *.eap* *_LICENSE.txt pa*