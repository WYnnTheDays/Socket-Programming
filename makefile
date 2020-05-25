SRC=$(wildcard *.cpp)
OBJ=$(patsubst %.cpp,%.o,$(SRC))

TARGET=server
CC=g++

$(TARGET):$(OBJ)
	$(CC) $^ -o $@

%.o:%.cpp
	$(CC)  -c $< -o $@ -std=c++11