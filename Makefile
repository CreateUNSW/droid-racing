
## List of all cpp files
CPP_FILES = $(wildcard *.cpp)

## Optimisation flag
SPEC_FLAG=-O2

## From the list of cpp files, generate a list of .o files
OBJ_FILES=$(CPP_FILES:.cpp=.o)

## remove main.cpp and some testing source from the list of C++ files
empty=
CPP_FILES := $(subst main.cpp,$(empty),$(CPP_FILES))

## get the libs together
LIBS = -ljpeg `pkg-config opencv --cflags --libs` -ldl -lstdc++ -lwiringPi -lraspicam -lraspicam_cv

all: run

run: $(OBJ_FILES) main.cpp
	g++ $(SPEC_FLAG) -o run.exe main.cpp $(INCL_DIRS) $(OBJ_FILES) --std=c++11 -g $(LIBS) -pthread

%.o: %.cpp %.hpp
	g++ $(SPEC_FLAG) -c $< --std=c++11 -pthread


clean:
	rm -rf $(OBJ_FILES) run.exe
