#include<stdio.h>
#include<libavcodec/avcodec.h>
#include<libavformat/avformat.h>

#define MAX_FILES 20

#define FILE_IO_ERROR -11
#define FILE_STREAM_IO_ERROR -2
#define ERROR_WRITING_HEADER -3
#define ERROR_SEEKING_FLAG -4

typedef struct
{
    int64_t dts;
    int64_t pts;
} ffmpeg_offset;

typedef struct 
{
    AVFormatContext *in_ctx;
    char *filename;
    int start_time; 
    int end_time;
    int cut_video; 
} ffmpeg_file;

typedef struct
{
    char *out_file;
    int n_files;
    int init_streams;
    ffmpeg_file *in_files;
    AVFormatContext *out_ctx;
    AVPacket *pkt;
    ffmpeg_offset *video_offset;
    ffmpeg_offset *audio_offset;
    int video_stream_index;
    int audio_stream_index;
    AVCodecContext *encode_ctx;
    AVCodecContext *decode_ctx;
    AVFrame *frame;
} ffmpeg_wrapper;

int merge_video(ffmpeg_wrapper *wrapper, char *filename);
int *init_wrapper(ffmpeg_wrapper **wrapper, char *out_file, int n_files);
int execute_mux(ffmpeg_wrapper *wrapper);
int insert_video(ffmpeg_wrapper *wrapper, char *main_video_file, char *insert_video_file, int timestamp);
int cut_video(ffmpeg_wrapper *wrapper, char *filename, int start_time, int end_time);
int replace_video(ffmpeg_wrapper *wrapper, char *main_video_file, char *insert_video_file, int timestamp);
