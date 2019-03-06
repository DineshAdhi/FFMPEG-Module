#include<stdio.h>
#include "include/ffmpeg.h"

int main()
{
    char *outfile = {"/Users/dinesh-6810/Desktop/Output/Output.mp4"};

    ffmpeg_wrapper *wrapper;
    init_wrapper(&wrapper, outfile, 10);

    insert_video(wrapper, "/Users/dinesh-6810/Desktop/SonyTest.mp4", "/Users/dinesh-6810/Desktop/CostaRicaTest.mp4", 10);
    cut_video(wrapper, "/Users/dinesh-6810/Desktop/4KNewTest.mp4", -1, 60);
    merge_video(wrapper, "/Users/dinesh-6810/Desktop/SonyTest.mp4");

    execute_mux(wrapper);
}