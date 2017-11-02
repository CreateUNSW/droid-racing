
## List of all cpp files
CPP_FILES = $(wildcard src/*.cpp)

## Include directories
INCL_DIRS = -I./include/ 

## Executable directories
EXEC_DIRS = ./bin/

## Optimisation flag
#SPEC_FLAG = -std=c++11

## From the list of cpp files, generate a list of .o files
OBJ_FILES=$(CPP_FILES:.cpp=.o)

## get the libs together
LIBS = -ljpeg `pkg-config opencv --cflags` -L/usr/local/lib -lopencv_video -lopencv_features2d -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_imgproc -lopencv_core -ldl -lstdc++

all: run

run: $(OBJ_FILES) main.cpp
	g++ $(SPEC_FLAG) -o $(EXEC_DIRS)run.exe main.cpp $(INCL_DIRS) $(OBJ_FILES) --std=c++11 -g $(LIBS) -pthread

src/%.o: src/%.cpp
	g++ $(SPEC_FLAG) -c -o $@ $< $(INCL_DIRS) --std=c++11 -pthread

clean:
	rm -rf $(OBJ_FILES) $(EXEC_DIRS)run.exe
