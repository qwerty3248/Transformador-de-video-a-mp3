CFLAGS = -std=c++11 -Wall
LDFLAGS = $(shell pkg-config --cflags --libs libavformat libavcodec libswresample)

all: PasarVideoMp3

PasarVideoMp3: PasarVideoMp3.cpp
	g++ -std=c++11 -Wall -o PasarVideoMp3 PasarVideoMp3.cpp\
	 -I/usr/include/x86_64-linux-gnu -L/usr/lib/x86_64-linux-gnu -lavformat -lavcodec -lswresample


clean:
	rm -f PasarVideoMp3

