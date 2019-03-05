#include<stdio.h>
#include "include/ffmpeg.h"

int main()
{
    char *outfile = {"/Users/dinesh-6810/Desktop/Output/Output.mp4"};

    ffmpeg_wrapper *wrapper;
    init_wrapper(&wrapper, outfile, 0);

    insert_video(wrapper, "/Users/dinesh-6810/Desktop/SonyTest.mp4", "/Users/dinesh-6810/Desktop/CostaRicaTest.mp4", 60);
    cut_video(wrapper, "/Users/dinesh-6810/Desktop/NewTest.mp4", -1, 60);

    execute_mux(wrapper);
}