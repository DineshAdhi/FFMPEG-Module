#include<stdio.h>
#include<libavcodec/avcodec.h>
#include<libavformat/avformat.h>

#include "include/ffmpeg.h"
#include "include/log.h"

int64_t video_offset_dts = 0;
int64_t audio_offset_pts = 0;
int64_t audio_offset_dts = 0;
int64_t video_offset_pts = 0;

int open_file(AVFormatContext **ctx, char *filename)
{
    if(avformat_open_input(ctx, filename, 0, 0) < 0)
    {
        log_fatal("[CANNOT OPEN FILE][avformat_open_input][%s]", filename);
        return FILE_IO_ERROR;
    }

    if(avformat_find_stream_info(*ctx, 0) < 0)
    {
        log_fatal("[CANNOT READ STREAM INFO][avformat_find_stream_info][%s]", filename);
        return FILE_STREAM_IO_ERROR;
    }

    return 1;
}

void init_streams(AVFormatContext *in_ctx, AVFormatContext *out_ctx)
{
    int i = 0;
    int n_streams = in_ctx->nb_streams;

    log_info("[INITIALIZING STREAMS][NUMBER OF STREAMS - %d]", n_streams);

    if(n_streams > 2)
    {
        log_info("[CONTAINER CONTAINS MORE THAN 2 STREAMS]");
    }

    for(i=0; i<n_streams; i++)
    {
        if(in_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            log_info("[INITIALIZING AUDIO STREAM][STREAM INDEX - %d]", i);
        }
        else 
        {
            log_info("[INITIALIZING VIDEO STREAM][STREAM INDEX - %d]", i);
        }
            
        AVStream *stream = avformat_new_stream(out_ctx, NULL);
        avcodec_parameters_copy(stream->codecpar, in_ctx->streams[i]->codecpar);
        stream->codecpar->codec_tag = 0;
    }
}

int *init_wrapper(ffmpeg_wrapper **wrapper, char *out_file, int n_files)
{
    int ret, i;

    *wrapper = (ffmpeg_wrapper *) calloc(1, sizeof(ffmpeg_wrapper));
    ffmpeg_offset *a_offset = (ffmpeg_offset *) calloc(1, sizeof(ffmpeg_offset));
    ffmpeg_offset *v_offset = (ffmpeg_offset *) calloc(1, sizeof(ffmpeg_offset));

    a_offset->dts = 0;
    a_offset->pts = 0;
    v_offset->dts = 0;
    v_offset->pts = 0;

    (*wrapper)->out_file = out_file;
    (*wrapper)->audio_offset = a_offset;
    (*wrapper)->video_offset = v_offset;
    (*wrapper)->n_files = 0;
    (*wrapper)->init_streams = 0;
    (*wrapper)->out_ctx = NULL;
    if(n_files == 0)
    {
        (*wrapper)->in_files = (ffmpeg_file *)calloc(MAX_FILES, sizeof(ffmpeg_file));
    }
    else 
    {
        (*wrapper)->in_files = (ffmpeg_file *)calloc(n_files, sizeof(ffmpeg_file));
    }

    if( avformat_alloc_output_context2(&(*wrapper)->out_ctx, NULL, NULL, (*wrapper)->out_file) < 0)
    {
        log_info("[ERROR WHILE ALLOCATING OUTPUT CONTEXT]");
        return NULL;
    }
    
    if(avio_open(&(*wrapper)->out_ctx->pb, (*wrapper)->out_file, AVIO_FLAG_WRITE) < 0)
    {
        log_info("[ERROR WHILE OPENING OUTPUT FILE][%d]", (*wrapper)->out_file);
        return NULL;
    }

    return NULL;
}

int write_stream(ffmpeg_wrapper *wrapper)
{
    int i;

    AVStream *in_stream = NULL, *out_stream = NULL;
    AVPacket pkt, last_audio_packet, last_video_packet;

    for(i=0; i<wrapper->n_files; i++)
    {
        log_info("[INPUT FILE : %s]", wrapper->in_files[i].filename);
    }

    for(i=0; i<wrapper->n_files; i++)
    {
        ffmpeg_file file = wrapper->in_files[i];

        if(file.cut_video == 1 && file.start_time != -1)
        {
            log_info("[SEEKING FRAME][START TIME - %d]", file.start_time);
            if(av_seek_frame(file.in_ctx, -1, file.start_time * AV_TIME_BASE, AVSEEK_FLAG_FRAME) < 0)
            {
                return ERROR_SEEKING_FLAG;
            }
        }

        while(av_read_frame(file.in_ctx, &pkt) >= 0)
        {
                in_stream = file.in_ctx->streams[pkt.stream_index];
                out_stream = wrapper->out_ctx->streams[pkt.stream_index];


                if(file.cut_video == 1 && file.end_time != -1)
                {
                    if(av_q2d(in_stream->time_base) * pkt.pts >= file.end_time)
                    {
                        log_info("[ENDING FRAME][END TIME - %d]", file.end_time);
                        break;
                    }
                }

                if(in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
                {
                    pkt.dts += wrapper->audio_offset->dts;
                    pkt.pts += wrapper->video_offset->pts;
                    last_audio_packet = pkt;
                }

                if(in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
                {
                    pkt.dts += wrapper->video_offset->dts;
                    pkt.pts += wrapper->video_offset->pts;
                    last_video_packet = pkt;
                }

                pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
                pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
                pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
                pkt.pos = -1;

                if( av_write_frame(wrapper->out_ctx, &pkt) < 0 )
                {
                    log_info("[PROBLEM WRITING FRAME][IGNORED]");
                }
        }

        wrapper->audio_offset->dts = last_audio_packet.dts;
        wrapper->audio_offset->pts = last_audio_packet.pts;

        wrapper->video_offset->dts = last_video_packet.dts;
        wrapper->video_offset->pts = last_audio_packet.pts;

        log_info("[WRITE DONE][%s]", file.filename);
    }

    return 1;
}

int add_file(ffmpeg_wrapper *wrapper, ffmpeg_file *file)
{
    wrapper->in_files[wrapper->n_files] = *file;
    wrapper->n_files += 1;
    //wrapper->in_files = (ffmpeg_file *) realloc(wrapper->in_files, wrapper->n_files + 1);

    if(wrapper->init_streams == 0)
    {
        init_streams(file->in_ctx, wrapper->out_ctx);

        if(avformat_write_header(wrapper->out_ctx, NULL) < 0)
        {
            log_info("[ERROR WHILE WRITING HEADER OUTPUT FILE][%d]", wrapper->out_file);
            return 0;
        } 
        wrapper->init_streams = 1;
    }

    return 1;
}

int merge_video(ffmpeg_wrapper *wrapper, char *filename)
{
    ffmpeg_file *file = (ffmpeg_file *) calloc(1, sizeof(ffmpeg_file));
    open_file(&file->in_ctx, filename);
    file->filename = filename;
    file->cut_video = 0;
    file->start_time = file->end_time = -1;

    return add_file(wrapper, file);
}

int cut_video(ffmpeg_wrapper *wrapper, char *filename, int start_time, int end_time)
{
    ffmpeg_file *file = (ffmpeg_file *) calloc(1, sizeof(ffmpeg_file));
    open_file(&file->in_ctx, filename);
    file->filename = filename;
    file->cut_video = 1;
    file->start_time = start_time;
    file->end_time = end_time;

    return add_file(wrapper, file);
}

int insert_video(ffmpeg_wrapper *wrapper, char *main_video_file, char *insert_video_file, int timestamp)
{
    cut_video(wrapper, main_video_file, -1, timestamp);
    merge_video(wrapper, insert_video_file);
    cut_video(wrapper, main_video_file, timestamp+1, -1);

    return 1;
}

int execute_mux(ffmpeg_wrapper *wrapper)
{
    write_stream(wrapper);
    av_write_trailer(wrapper->out_ctx);

    return 1;
}



