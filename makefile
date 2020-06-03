SRC=$(wildcard *.cpp)
OBJ=$(patsubst %.cpp,%.o,$(SRC))

TARGET=server
CC=g++

$(TARGET):$(OBJ)
	$(CC) $^ -o $@ -lpthread

%.o:%.cpp
	$(CC)  -c $< -o $@ -std=c++11 -Wl,--no-as-needed  -lpthread

.PHONY:clean
clean:
	rm *.o