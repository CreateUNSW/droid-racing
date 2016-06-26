.SUFFIXES: .cpp .c .o

# Tool names
SHELL = /bin/sh
CROSS_COMPILE = $(TARGET)-
CC            = $(CROSS_COMPILE)gcc
CPP           = $(CROSS_COMPILE)g++
ARM_CC        = $(CROSS_PREFIX)$(CPP)
LD            = $(CROSS_PREFIX)ld
OBJDUMP       = $(CROSS_PREFIX)objdump

export CC CPP LD OBJDUMP

MAKEDEPEND    = $(CPP) -M
INSTALL       = install

WEB_DIR       = /etc/iot
CONFIG_DIR    = /etc/iot

# Source files
CAPTURE_SOURCES = $(BASE_DIR)capture/capture.cpp \
$(BASE_DIR)capture/webcam.cpp \
$(BASE_DIR)capture/imgserver.cpp \
$(BASE_DIR)common/jpeglive.cpp \
$(BASE_DIR)common/jpeg.cpp \
$(BASE_DIR)common/buffer.cpp \
$(BASE_DIR)common/utils.cpp \
$(BASE_DIR)common/args.cpp \
$(BASE_DIR)common/status.cpp \
$(BASE_DIR)common/pipe.cpp \
$(BASE_DIR)common/lock.cpp \
$(BASE_DIR)common/sighandler.cpp \
$(BASE_DIR)common/wininc.cpp
CAPTURE_OBJECTS = $(CAPTURE_SOURCES:.cpp=.o)

# Source files
CVTEST_SOURCES = $(BASE_DIR)cvtest/cvtest.cpp \
$(BASE_DIR)capture/imgserver.cpp \
$(BASE_DIR)common/jpeg.cpp \
$(BASE_DIR)common/buffer.cpp \
$(BASE_DIR)common/utils.cpp \
$(BASE_DIR)common/args.cpp \
$(BASE_DIR)common/status.cpp \
$(BASE_DIR)common/pipe.cpp \
$(BASE_DIR)common/lock.cpp \
$(BASE_DIR)common/sighandler.cpp \
$(BASE_DIR)common/wininc.cpp
CVTEST_OBJECTS = $(CVTEST_SOURCES:.cpp=.o)

DEPENDENCIES = .depend

CPPFLAGS = $(CFLAGS) -O3 -Wall -Wfatal-errors -Wno-reorder -g -I/usr/local/include -I/usr/include -I$(BASE_DIR)common \
-DSOCKLEN_T=socklen_t -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -DBSD=1 -D__STDC_CONSTANT_MACROS

LDFLAGS = -L/usr/local/lib -L/usr/lib -ldl -lstdc++ -lpthread -ljpeg `pkg-config opencv --cflags --libs`

CAPTURE_TARGET = $(BASE_DIR)bin/capture
CVTEST_TARGET = $(BASE_DIR)bin/cvtest

capture: $(CAPTURE_OBJECTS)
	$(CPP) $(CAPTURE_OBJECTS) $(LDFLAGS) -o $(CAPTURE_TARGET)

cvtest: $(CVTEST_OBJECTS)
	$(CPP) $(CVTEST_OBJECTS) $(LDFLAGS) -o $(CVTEST_TARGET)

OBJECTS = $(CAPTURE_OBJECTS) $(CVTEST_OBJECTS)
TARGETS = $(CAPTURE_TARGET) $(CVTEST_TARGET)


dep: $(SOURCES)
	$(CPP) -M $(CPPFLAGS) $(DEFS) $(SOURCES) $^ > .$@

.cpp.o:
	$(CPP) $(CPPFLAGS) $(PROJECT_FLAGS) $(DEFS) $< -c -o $@

# Cleanup
clean:
	@rm -rf $(TARGETS) $(OBJECTS) $(DEPENDENCIES)

cleanall: clean
	@echo "not implemeted"

distclean:
	@echo "not implemeted"

-include .depend
