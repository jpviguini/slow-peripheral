CXX=g++
CXXFLAGS=-std=c++17 -Wall -Iinclude
SRC=src/main.cpp src/slow_client.cpp src/janela_envio.cpp src/slow_threads.cpp
OUT=slow_client
LDFLAGS = -pthread

all:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

run:
	./slow_client

clean:
	rm -f $(OUT)
