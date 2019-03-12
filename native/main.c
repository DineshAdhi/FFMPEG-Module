#include<stdio.h>
#include "include/ffmpeg.h"

int main()
{
    char *outfile = {"/Users/dinesh-6810/Desktop/Output/Output.mp4"};

    ffmpeg_wrapper *wrapper;
    init_wrapper(&wrapper, outfile, 10);

    replace_video(wrapper, "/Users/dinesh-6810/Desktop/TestVideos/EarthTest.mp4", "/Users/dinesh-6810/Desktop/TestVideos/SonyTest.mp4", 120);

    // merge_video(wrapper, "/Users/dinesh-6810/Desktop/TestVideos/SonyTest.mp4");
    // merge_video(wrapper, "/Users/dinesh-6810/Desktop/TestVideos/SonyTest.mp4");

    execute_mux(wrapper);
}