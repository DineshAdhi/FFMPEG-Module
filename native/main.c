#include<stdio.h>
#include "include/ffmpeg.h"

int main()
{
    char *outfile = {"/Users/dinesh-6810/Desktop/Output/Output.mp4"};

    ffmpeg_wrapper *wrapper;
    init_wrapper(&wrapper, outfile, 10);


    cut_video(wrapper, "/Users/dinesh-6810/Desktop/JobsTest.mp4", 60, -1);

    execute_mux(wrapper);
}