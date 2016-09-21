CXX=g++
CXXFLAGS=-g -std=c++11 -Wall -pedantic 
BIN=main

SRC=$(wildcard *.cpp)
OBJ=$(SRC:%.cpp=%.o)

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) -o $(BIN) $^ -ludev

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

.PHONY : clean
clean:
	-rm -- *.o $(BIN) 
