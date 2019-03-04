#include<stdio.h>
#include<libavcodec/avcodec.h>
#include<libavformat/avformat.h>

#define FILE_IO_ERROR -11;
#define FILE_STREAM_IO_ERROR -2;
#define ERROR_WRITING_HEADER -3;
#define ERROR_SEEKING_FLAG -4;

typedef struct
{
    int64_t dts;
    int64_t pts;
} ffmpeg_offset;

typedef struct 
{
    char *filename;
    int start_time; 
    int end_time;
    int cut_video; 
}ffmpeg_file;

typedef struct
{
    char **in_files;
    char *out_file;
    int n_files;
    int start_time, end_time;
    AVFormatContext **in_ctx;
    AVFormatContext *out_ctx;
    AVPacket *pkt;
    ffmpeg_offset *video_offset;
    ffmpeg_offset *audio_offset;
} ffmpeg_wrapper;

int copy_video(char **in_file, int n, char *out_file);
