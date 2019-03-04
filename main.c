#include<stdio.h>
#include "include/ffmpeg.h"

int main()
{
    char **in_files = (char **) calloc(3, sizeof(char *));
    in_files[0] = "/Users/dinesh-6810/Desktop/SonyTest.mp4";
    //in_files[1] = "/Users/dinesh-6810/Desktop/4KTest.mp4";
    in_files[1] = "/Users/dinesh-6810/Desktop/4KNewTest.mp4";
    in_files[2] = "/Users/dinesh-6810/Desktop/4KNewTest.mp4";
    int n = 3;

    ffmpeg_file *file = {"/Users/dinesh-6810/Desktop/SonyTest.mp4", 0, 10, 1};

    copy_video(in_files, n, "/Users/dinesh-6810/Desktop/Dummy.mp4");
}