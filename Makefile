TARGET=run
SOURCEDIR=./sources
CXXSRCS=$(shell find $(SOURCEDIR) -name *.cpp) main.cpp

CXXFLAGS=-std=c++11 -O0 -g -Wall -Wextra -Weffc++
LFLAGS=-lpthread


all:
	$(CXX) $(CXXFLAGS) $(CXXSRCS) $(LFLAGS) -o  $(TARGET)
