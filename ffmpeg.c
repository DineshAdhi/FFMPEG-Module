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

int *init_wrapper(ffmpeg_wrapper **wrapper, char **in_file, int n, char *out_file)
{
    int ret, i;

    for(i=0; i<n; i++)
    {
        log_info("[INPUT FILE - %s][OUTPUT FILE - %s]", in_file[i], out_file);
    }

    *wrapper = (ffmpeg_wrapper *) calloc(1, sizeof(ffmpeg_wrapper));
    ffmpeg_offset *a_offset = (ffmpeg_offset *) calloc(1, sizeof(ffmpeg_offset));
    ffmpeg_offset *v_offset = (ffmpeg_offset *) calloc(1, sizeof(ffmpeg_offset));

    a_offset->dts = 0;
    a_offset->pts = 0;
    v_offset->dts = 0;
    v_offset->pts = 0;

    (*wrapper)->in_files = in_file;
    (*wrapper)->out_file = out_file;
    (*wrapper)->audio_offset = a_offset;
    (*wrapper)->video_offset = v_offset;
    (*wrapper)->cut_video = 0;
    (*wrapper)->start_time = -1;
    (*wrapper)->end_time = -1;
    (*wrapper)->n_files = n;
    (*wrapper)->in_ctx = (AVFormatContext **) calloc(1, sizeof(AVFormatContext *));
    (*wrapper)->out_ctx = NULL;

    for(i=0; i<n; i++)
    {
        (*wrapper)->in_ctx[i] = NULL;
        if((ret = open_file(&(*wrapper)->in_ctx[i],in_file[i])) < 0)
        {
            log_error("[CAN'T OPEN FILE]");
            return NULL;
        }
    }

    if( avformat_alloc_output_context2(&(*wrapper)->out_ctx, NULL, NULL, (*wrapper)->out_file) < 0)
    {
        log_info("[ERROR WHILE ALLOCATING OUTPUT CONTEXT]");
        return NULL;
    }

    init_streams((*wrapper)->in_ctx[0], (*wrapper)->out_ctx);
    
    if(avio_open(&(*wrapper)->out_ctx->pb, (*wrapper)->out_file, AVIO_FLAG_WRITE) < 0)
    {
        log_info("[ERROR WHILE OPENING OUTPUT FILE][%d]", (*wrapper)->out_file);
        return NULL;
    }

    if(avformat_write_header((*wrapper)->out_ctx, NULL) < 0)
    {
        log_info("[ERROR WHILE WRITING HEADER OUTPUT FILE][%d]", (*wrapper)->out_file);
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
        // if(wrapper->cut_video == 1 && wrapper->start_time != -1)
        // {
        //     if(av_seek_frame(wrapper->in_ctx[i], -1, wrapper->start_time * AV_TIME_BASE, AVSEEK_FLAG_FRAME) < 0)
        //     {
        //         log_info("[SEEKING FRAME][START TIME - %d]", wrapper->start_time);
        //         return ERROR_SEEKING_FLAG;
        //     }
        // }

        while(av_read_frame(wrapper->in_ctx[i], &pkt) >= 0)
        {
                in_stream = wrapper->in_ctx[i]->streams[pkt.stream_index];
                out_stream = wrapper->out_ctx->streams[pkt.stream_index];

                // if(wrapper->cut_video == 1 && wrapper->end_time != -1)
                // {
                //     if(av_q2d(in_stream->time_base) * pkt.pts > wrapper->end_time)
                //     {
                //         log_info("[ENDING FRAME][START TIME - %d]", wrapper->end_time);
                //         break;
                //     }
                // }

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

        log_info("[WRITE DONE]");
    }

    return 1;
}

int copy_video(char **in_files, int n, char *out_file)
{
    ffmpeg_wrapper *wrapper;
    init_wrapper(&wrapper, in_files, n, out_file);
    write_stream(wrapper);

    av_write_trailer(wrapper->out_ctx);

    return 1;
}



