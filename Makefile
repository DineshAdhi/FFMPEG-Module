CC = gcc

default: main
	make clean

main : log.o ffmpeg.o
	CC main.c log.o ffmpeg.o -lavformat -lavcodec -o main


log.o : include/ffmpeg.h log.c
	CC -c log.c


ffmpeg.o : include/ffmpeg.h ffmpeg.c
	CC -c ffmpeg.c

clean : 
	rm *.o


