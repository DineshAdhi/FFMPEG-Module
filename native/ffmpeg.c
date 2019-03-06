#include<stdio.h>
#include<libavcodec/avcodec.h>
#include<libavformat/avformat.h>

#include "include/ffmpeg.h"
#include "include/log.h"

int temp = 0;

// Opens AVFormatContext and extracts Stream info using the filename
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

// Initializes the output context with Streams that are present in the Input video files. 
// i,e,. The Input Video file may have number of streams(video, audio, subtitles), we have to create each stream seperately 
// in the output context. In case of multiple files, the file with large number of streams must to used to initialize

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
            
        AVStream *stream = avformat_new_stream(out_ctx, NULL);                      // creates new stream in the output context
        avcodec_parameters_copy(stream->codecpar, in_ctx->streams[i]->codecpar);
        stream->codecpar->codec_tag = 0;
    }
}


// A wrapper function to initialize ffmpeg_wrapper. It needs n_files variable which 
// denotes the number of video files we are currently working on. If given zero, 
// the default value is MAX_FILES

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

// Fetches all the input video files declared in the wrapper. 
// Writes all the input file frames to the output context. 
// Keeps track of the offset at the end of each routine. 
// Rescales the video file duration and playback with that of the original file

// int write_stream(ffmpeg_wrapper *wrapper)
// {
//     int i;

//     AVStream *in_stream = NULL, *out_stream = NULL;
//     AVPacket pkt, last_audio_packet, last_video_packet;
//     ffmpeg_offset prev_a_offset, prev_v_offset;

//     log_info("[NUMBER OF FILES : %d]", wrapper->n_files);

//     for(i=0; i<wrapper->n_files; i++)
//     {
//         log_info("[INPUT FILE : %s]", wrapper->in_files[i].filename);
//     }

//     for(i=0; i<wrapper->n_files; i++)
//     {
//         ffmpeg_file file = wrapper->in_files[i];

//         if(file.cut_video == 1 && file.start_time != -1)        // Seek frame to the start_position if cut is enabled
//         {
//             log_info("[SEEKING FRAME][START TIME - %d]", file.start_time);
//             if(av_seek_frame(file.in_ctx, -1, file.start_time * AV_TIME_BASE, AVSEEK_FLAG_FRAME) < 0)
//             {
//                 return ERROR_SEEKING_FLAG;
//             }
//         }

//         if(file.delta_offset == 1)
//         {
//             wrapper->audio_offset->dts -= wrapper->delta_audio_offset->dts;
//             wrapper->audio_offset->pts -= wrapper->delta_audio_offset->pts;

//             wrapper->video_offset->dts -= wrapper->delta_video_offset->dts;
//             wrapper->video_offset->pts -= wrapper->delta_video_offset->pts;
//         }

//         // Storing a previous temp offset in order to compute the delta offset later
//         prev_a_offset = *wrapper->audio_offset;
//         prev_v_offset = *wrapper->video_offset;

//         while(av_read_frame(file.in_ctx, &pkt) >= 0)
//         {
//                 in_stream = file.in_ctx->streams[pkt.stream_index];
//                 out_stream = wrapper->out_ctx->streams[pkt.stream_index];

//                 if(file.cut_video == 1 && file.end_time != -1)          
//                 {
//                     if(av_q2d(in_stream->time_base) * pkt.pts >= file.end_time)
//                     {
//                         log_info("[ENDING FRAME][END TIME - %d]", file.end_time);
//                         break;
//                     }
//                 }

//                 if(in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
//                 {
//                     pkt.dts += wrapper->audio_offset->dts;
//                     pkt.pts += wrapper->video_offset->pts;

//                     last_audio_packet = pkt;
//                 }

//                 if(in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
//                 {
//                     pkt.dts += wrapper->video_offset->dts;
//                     pkt.pts += wrapper->video_offset->pts;

//                     last_video_packet = pkt;
//                 }

//                 pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
//                 pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
//                 pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
//                 pkt.pos = -1;

//                 if(out_stream->cur_dts >= pkt.dts)
//                 {
//                     log_info("[ANAMOLY FOUND][FRAME DTS - %lld][CURRENT DTS - %lld][FIXING ANAMOLY BY CHANGING DTS]", pkt.dts, out_stream->cur_dts);
//                     pkt.dts = out_stream->cur_dts + 1;
//                     pkt.pts = pkt.dts;
//                 }

//                 // if(pkt.pts < pkt.dts)
//                 // {
//                 //     pkt.pts = pkt.dts;
//                 // }

//                 if( av_write_frame(wrapper->out_ctx, &pkt) < 0 )
//                 {
//                     log_info("[PROBLEM WRITING FRAME][IGNORED][CURRENT DTS - %d]", out_stream->cur_dts);
//                 }                
//         }
        
//         wrapper->audio_offset->dts = last_audio_packet.dts;
//         wrapper->audio_offset->pts = last_audio_packet.pts;

//         wrapper->video_offset->dts = last_video_packet.dts;
//         wrapper->video_offset->pts = last_audio_packet.pts;

//         if(file.cut_video == 1 && file.delta_offset == 0)       // Each time when we cut a video, store the offset. Next time when we insert the remaining part of the video include the offset to nullify time jump.
//         {                                          
//             wrapper->delta_audio_offset->dts = wrapper->audio_offset->dts - prev_a_offset.dts;
//             wrapper->delta_audio_offset->pts = wrapper->audio_offset->pts - prev_a_offset.dts;

//             wrapper->delta_video_offset->dts = wrapper->video_offset->dts - prev_v_offset.dts;
//             wrapper->delta_video_offset->pts = wrapper->video_offset->pts - prev_v_offset.pts;
//         }

//         log_info("[WRITE DONE][%s]", file.filename);
//     }

//     return 1;
// }



int write_stream(ffmpeg_wrapper *wrapper)
{
    int i;

    AVStream *in_stream = NULL, *out_stream = NULL;
    AVPacket pkt, last_audio_packet, last_video_packet;
    ffmpeg_offset prev_a_offset, prev_v_offset;

    log_info("[NUMBER OF FILES : %d]", wrapper->n_files);

    for(i=0; i<wrapper->n_files; i++)
    {
        log_info("[INPUT FILE : %s]", wrapper->in_files[i].filename);
    }

    for(i=0; i<wrapper->n_files; i++)
    {
        ffmpeg_file file = wrapper->in_files[i];

        AVPacket delta_audio_pkt, delta_video_pkt;
        delta_audio_pkt.dts = delta_audio_pkt.pts = delta_video_pkt.pts = delta_video_pkt.dts = 0;

        while(av_read_frame(file.in_ctx, &pkt) >= 0)
        {
                in_stream = file.in_ctx->streams[pkt.stream_index];
                out_stream = wrapper->out_ctx->streams[pkt.stream_index];

                if(file.cut_video == 1 && file.end_time != -1)          
                {
                    if(av_q2d(in_stream->time_base) * pkt.pts >= file.end_time)
                    {
                        log_info("[ENDING FRAME][END TIME - %d]", av_q2d(in_stream->time_base) * pkt.pts);
                        break;
                    }
                }

                if(file.cut_video == 1 && file.start_time != -1)
                {
                    if(av_q2d(in_stream->time_base) * pkt.pts <= file.start_time)
                    {
                            if(in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
                            {
                                    delta_audio_pkt = pkt;
                            }
                            else if(in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
                            {
                                    delta_video_pkt = pkt;
                            }
                            continue;
                    }
                }

                if(in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
                {
                    pkt.dts += wrapper->audio_offset->dts - delta_video_pkt.dts;
                    pkt.pts += wrapper->audio_offset->pts - delta_video_pkt.pts;

                    last_audio_packet = pkt;
                }

                if(in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
                {
                    pkt.dts += wrapper->video_offset->dts - delta_audio_pkt.dts;;
                    pkt.pts += wrapper->video_offset->pts - delta_audio_pkt.pts;;

                    last_video_packet = pkt;
                }

                pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
                pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
                pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
                pkt.pos = -1;

                // if(out_stream->cur_dts >= pkt.dts)
                // {
                //     log_info("[ANAMOLY FOUND][FRAME DTS - %lld][CURRENT DTS - %lld][FIXING ANAMOLY BY CHANGING DTS]", pkt.dts, out_stream->cur_dts);
                //     pkt.dts = out_stream->cur_dts + 1;
                //     pkt.pts = pkt.dts;
                // }

                if( av_write_frame(wrapper->out_ctx, &pkt) < 0 )
                {
                    log_info("[PROBLEM WRITING FRAME][IGNORED][CURRENT DTS - %d]", out_stream->cur_dts);
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

// ----------------------------------------- Insert Strategy -------------------------------------------------------------

// () ----> Denotes Frame Inserted in to the output context
// [] ----> Video Context and each number enclosed in it represents PTS (or) Timestamp

// Main Stream -  [ 1 2 3 4 5 6 7 8 9 10 ]
// Insert Stream - [ 1 2 3 ]
// OutStream = []

// Insert at pos 4 
// Main Stream - [ (1) (2) (3) (4) 5 6 7 8 9 10 ]   // Cut the video at position 4 and insert. Now the current_offset = 4. We store the delta_offset = 4 every time we execute cut.
// Insert Stream - [ (1) (2) (3) ]                    // Inserted 3 elements from the insert video. Now Currrent_offset = 7;
// Out Stream - [ 1 2 3 4 5 6 7 ]           // 4 + 3 = 7 Frames are filled in output context
//
// Before we fill the remaining frames from the main stream. We need to adjust the offset with the delta_offset; 
// Each value of the remaining frames must be subtracted with delta_offset
// Main Stream - [ (1) (2) (3) (4) 1 2 3 4 5 6]    //  current_offset = 7
// OutStream = [1 2 3 4 5 6 7]
// We then insert the frames as usual.
// Main Stream = [ (1) (2) (3) (4) (1) (2) (3) (4) (5) (6)]
// OutStream = [ 1 2 3 4 5 6 7 8 9 10 11 12 13 ] // offset = 13

// ------------------------------------------------------------------------------------------------------------------------

// A wrapper function to add file in to the ffmpeg_wrapper. 
// We initialize streams when the first video that is added
// If a input file contains more than two streams, it must be added first.

int add_file(ffmpeg_wrapper *wrapper, ffmpeg_file *file)
{
    wrapper->in_files[wrapper->n_files] = *file;
    wrapper->n_files += 1;
    //wrapper->in_files = (ffmpeg_file *) realloc(wrapper->in_files, wrapper->n_files + 1);

    if(wrapper->init_streams == 0)    
    {
        init_streams(file->in_ctx, wrapper->out_ctx);       // We initialize streams when the first video that is added

        if(avformat_write_header(wrapper->out_ctx, NULL) < 0)   // Adds header only once 
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

// A delta_offset_cut is used when the video that is to be merged with the output context doesn't
// start with timestamp 0. 

int cut_video_with_delta_offset(ffmpeg_wrapper *wrapper, char *filename, int start_time, int end_time)
{
    ffmpeg_file *file = (ffmpeg_file *) calloc(1, sizeof(ffmpeg_file));
    open_file(&file->in_ctx, filename);
    file->filename = filename;
    file->cut_video = 1;
    file->start_time = start_time;
    file->end_time = end_time;

    return add_file(wrapper, file);
}

// Insert adds three files to the ffmpeg_wrapper

int insert_video(ffmpeg_wrapper *wrapper, char *main_video_file, char *insert_video_file, int timestamp)
{
    cut_video(wrapper, main_video_file, -1, timestamp);
    merge_video(wrapper, insert_video_file);
    cut_video(wrapper, main_video_file, timestamp+1, -1);

    return 1;
}

// Invokes write_stream and closes the file after writing trailer

int execute_mux(ffmpeg_wrapper *wrapper)
{
    write_stream(wrapper);
    av_write_trailer(wrapper->out_ctx);

    return 1;
}



