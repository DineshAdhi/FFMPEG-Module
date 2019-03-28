#include<stdio.h>
#include "include/ffmpeg.h"

int main()
{
    char *outfile = {"/Users/dinesh-6810/Desktop/Output/Output.webm"};

    char *main = "/Users/dinesh-6810/Desktop/TestVideos/main.webm";
    char *edit = "/Users/dinesh-6810/Desktop/TestVideos/edit.webm";


    /*char *outfile = {"/Users/dinesh-6810/Desktop/Output/Output.mp4"};

    char *main = "/Users/dinesh-6810/Desktop/TestVideos/SonyTest.mp4";
    char *edit = "/Users/dinesh-6810/Desktop/TestVideos/NewTest.mp4";*/


    ffmpeg_wrapper *wrapper;
    init_wrapper(&wrapper, outfile, 10);

    //insert_video(wrapper, "/Users/dinesh-6810/Desktop/TestVideos/EarthTest.mp4", "/Users/dinesh-6810/Desktop/TestVideos/SonyTest.mp4", 120);

    // merge_video(wrapper, "/Users/dinesh-6810/Desktop/main.webm");
    // merge_video(wrapper, "/Users/dinesh-6810/Desktop/edit.webm");
    
   
    merge_video(wrapper, main);
    merge_video(wrapper, edit);

    //insert_video(wrapper, main, edit, 5);

    //cut_video(wrapper, main, 10, 25);

    execute_mux(wrapper);
}
