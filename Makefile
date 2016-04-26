TARGET=run
SOURCEDIR=./sources
CXXSRCS:=$(shell find $(SOURCEDIR) -name "*.cpp") main.cpp

CXXFLAGS=-std=c++11 -O2 -Wall -Wextra -Weffc++
LFLAGS=-lpthread


all:
	$(CXX) $(CXXFLAGS) $(CXXSRCS) $(LFLAGS) -o  $(TARGET)
