#include<stdio.h>
#include<libavcodec/avcodec.h>
#include<libavformat/avformat.h>
#include "ffmpegeditor.h"
#include "../wmsutils/wmslogger.h"

int open_file_ctx(AVFormatContext **ctx, char *filepath)
{
    int res = -1;

    if( (res = avformat_open_input(ctx, filepath, NULL, 0)) < 0)
    {
            wmslog_fatal("[ERROR OPENING FILE][%s][%s]", filepath, av_err2str(res));
            return -1;
    }

    if(avformat_find_stream_info(*ctx, 0) < 0)
    {
            wmslog_fatal("[ERROR WHILE READING STREAM INFO][%s]", filepath);
            return -1;
    }

    return 1;
}

int initialize_editorcontext(WMSEditorContext **ctx, char *outputfile, int n_files)
{
        (*ctx) = (WMSEditorContext *) calloc(1, sizeof(WMSEditorContext));
        WMSEditorOffset *video_offset = (WMSEditorOffset *) calloc(1, sizeof(WMSEditorOffset));
        WMSEditorOffset *audio_offset = (WMSEditorOffset *) calloc(1, sizeof(WMSEditorOffset));

        video_offset->pts = video_offset->dts = 0;
        audio_offset->pts = audio_offset->dts = 0;

        (*ctx)->video_offset = video_offset;
        (*ctx)->audio_offset = audio_offset;
        (*ctx)->out_filename = outputfile;
        (*ctx)->out_ctx = NULL;
        (*ctx)->init_streams = 0;
        (*ctx)->n_counter = 0;
        
        n_files = (n_files > 0) ? n_files : MAX_FILES;
        
        (*ctx)->in_files = (WMSEditorFile *) calloc(n_files, sizeof(WMSEditorFile));

        if(avformat_alloc_output_context2(&(*ctx)->out_ctx, NULL, NULL, (*ctx)->out_filename) < 0)
        {
                wmslog_info("[ERROR WHILE ALLOCATING OUTPUT CONTEXT]");
                return -1;
        }

        if(avio_open(&(*ctx)->out_ctx->pb, (*ctx)->out_filename, AVIO_FLAG_WRITE) < 0)
        {
                wmslog_info("[ERROR WHILE OPENING OUTPUT FILE FOR WRITING]");
                return -1;
        }
        
        fp = fopen("/home/sas/AdventNet/amserver/logs/jnilog.txt", "w+");
        setFP(fp);

        return 1;
}

AVCodecContext* open_decoder_ctx(AVCodecParameters *params)
{
        AVCodec *decoder = avcodec_find_decoder(params->codec_id);

        AVCodecContext *ctx = avcodec_alloc_context3(decoder);
        avcodec_parameters_to_context(ctx, params);
        avcodec_open2(ctx, decoder, NULL);

        return ctx;
}

AVCodecContext *open_encoder_ctx(AVCodecParameters *params, AVCodecContext *dec_ctx)
{

        AVCodec *encoder = avcodec_find_encoder(params->codec_id);
        AVCodecContext *enc_ctx = avcodec_alloc_context3(encoder);
        enc_ctx->bit_rate = 750000;
        enc_ctx->width = dec_ctx->width;
        enc_ctx->height = dec_ctx->height;
        enc_ctx->time_base = (AVRational){1, 25};
        enc_ctx->framerate = (AVRational){25, 1};

        enc_ctx->gop_size = 10;       
        enc_ctx->max_b_frames=1;
        enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
        
        avcodec_open2(enc_ctx, encoder, NULL);

        return enc_ctx;
}

void init_output_streams(AVFormatContext *in_ctx, WMSEditorContext *ctx)
{
        int i, n = in_ctx->nb_streams; 
        
        wmslog_debug("[INITIALING STREAMS][NO OF STREAMS - %d]", n);

        for(i=0; i<n; i++)
        {
                AVCodecParameters *params = in_ctx->streams[i]->codecpar;
                enum AVMediaType mtype = params->codec_type;

                if(mtype == AVMEDIA_TYPE_AUDIO)
                {
                        wmslog_debug("[INITIALIZING AUDIO STREAM][STREAM INDEX - %d]", i);
                }
                else if(mtype == AVMEDIA_TYPE_VIDEO)
                {
                        wmslog_debug("[INITIALIZING VIDEO STREAM][STREAM INDEX - %d]", i);
                        ctx->decode_ctx = open_decoder_ctx(params);
                        ctx->encode_ctx = open_encoder_ctx(params, ctx->decode_ctx);
                }
                else 
                {
                        wmslog_debug("[INITIALIZING UNKNOWN STREAM][STREAM INDEX - %d]", i);
                }

                AVStream *stream = avformat_new_stream(ctx->out_ctx, NULL);
                avcodec_parameters_copy(stream->codecpar, in_ctx->streams[i]->codecpar);
                stream->codecpar->codec_tag = 0;
        }
}

int64_t getTimeStamp(AVStream *stream, AVPacket pkt)
{
    int64_t time = av_q2d(stream->time_base) * pkt.pts;
    return time; 
}

int isKeyFrame(AVPacket pkt)
{
        if((pkt.flags & AV_PKT_FLAG_KEY))
        {
                return 1;
        }

        return 0;
}


int write_stream(WMSEditorContext *ctx)
{
        AVPacket pkt, last_video_pkt, last_audio_pkt;
        AVStream *in_stream = NULL;
        AVStream *out_stream = NULL;

        int audio_delta_flag = 1, video_delta_flag = 1;
        int i, n_counter = ctx->n_counter;
        int64_t time;

        if(n_counter <= 0)
        {
                wmslog_info("[NO FILES TO WRITE]");
                return -1;
        }

        wmslog_info("[NUMBER OF FILES TO BE WRITTEN][%d]", n_counter);
        
        for(i=0; i<n_counter; i++)
        {
                wmslog_info("[INPUT FILE %d : ][%s][NEAREST KEY FRAME][%d]", i+1, ctx->in_files[i].filename, ctx->in_files[i].nearest_keyframe.pts);
        }

        for(i=0; i<n_counter; i++)
        {
                AVPacket delta_audio_pkt, delta_video_pkt; 
                delta_audio_pkt.pts = delta_audio_pkt.dts = 0;
                delta_video_pkt.pts = delta_video_pkt.dts = 0;
                
                WMSEditorFile file = ctx->in_files[i];
  
                if(file.cut_video == 1 && file.start_time > 0)
                {
                        int ret = -1;

                        AVPacket keyframe = file.nearest_keyframe;
                        int stream_index = keyframe.stream_index;
                        int64_t seek_target = keyframe.pts;

                        if( (ret = av_seek_frame(file.in_ctx, keyframe.stream_index, seek_target, AVSEEK_FLAG_ANY)) >= 0)
                        {
                                wmslog_info("[AV SEEK FRAME DONE][%d][KEY FRAME][%d]", seek_target, isKeyFrame(keyframe));
                        }
                        else 
                        {
                                wmslog_error("[ERROR WHILE SEEKING][%d][%s]", seek_target, av_err2str(ret));
                        }
                }

                while(av_read_frame(file.in_ctx, &pkt) >= 0)
                {
                        in_stream = file.in_ctx->streams[pkt.stream_index];
                        out_stream = ctx->out_ctx->streams[pkt.stream_index];
                        enum AVMediaType mtype = in_stream->codecpar->codec_type;

                        //wmslog_info("[FRAME ENTER][%d][%d]", pkt.pts, mtype==AVMEDIA_TYPE_VIDEO);

                        AVFrame *frame = av_frame_alloc();

                        if(file.cut_video == 1)
                        {
                                if( file.end_time > 0 && (time = getTimeStamp(in_stream, pkt)) > file.end_time)
                                {
                                        wmslog_info("[CUT VIDEO - ENDING FRAME][TIME - %lld]", time);
                                        break;
                                }

                                if(mtype == AVMEDIA_TYPE_VIDEO && file.start_time > 0)
                                {
                                        //wmslog_debug("[DECODING FRAME][%d]", pkt.pts);
                                        //
                                        
                                        while(1)
                                        {
                                                int ret = avcodec_send_packet(ctx->decode_ctx, &pkt);

                                                if(ret == AVERROR(EAGAIN))
                                                {
                                                        continue;
                                                }

                                                if(ret < 0)
                                                {
                                                        wmslog_info("[ERROR SENDING PACKET FOR DECODING][%s]", av_err2str(ret));
                                                        return -1;
                                                }
                                                
                                                break;
                                        }
                                }

                                if(file.start_time > 0)
                                {
                                        time = getTimeStamp(in_stream, pkt);

                                        if(time <= file.start_time)
                                        {
                                                if(mtype == AVMEDIA_TYPE_AUDIO)
                                                {
                                                        delta_audio_pkt = pkt;
                                                }
                                                else if(mtype == AVMEDIA_TYPE_VIDEO)
                                                {
                                                        delta_video_pkt = pkt;
                                                }

                                                continue;
                                        }
                                }


                                if(mtype == AVMEDIA_TYPE_VIDEO && file.start_time > 0)
                                {
                                        int ret;

                                        while(1) 
                                        {
                                                ret = avcodec_send_frame(ctx->encode_ctx, frame);

                                                if(AVERROR(ret) == 0)
                                                {
                                                        break;
                                                }

                                                if(ret == AVERROR(EAGAIN))
                                                {
                                                        continue;
                                                }
                                                else 
                                                {
                                                        wmslog_info("[ERROR SENDING FRAME FOR ENCODING][%s][%d]", av_err2str(ret), AVERROR(ret));
                                                }

                                                wmslog_debug(".");
                                                break;
                                        }
                                        
                                        while(1)
                                        {
                                                ret = avcodec_receive_packet(ctx->encode_ctx, &pkt);

                                                if(AVERROR(ret) == 0)
                                                {
                                                        break;
                                                }

                                                if(ret == AVERROR(EAGAIN))
                                                {
                                                        continue;
                                                }
                                                else 
                                                {
                                                        wmslog_info("[ERROR RECEIVING PACKET AFTER ENCODING][%s][%d]", av_err2str(ret), AVERROR(ret));
                                                }

                                                wmslog_debug(".");

                                                break;
                                        }
                                }
                        }

                        if(mtype == AVMEDIA_TYPE_VIDEO)
                        {
                                pkt.dts += ctx->video_offset->dts - delta_video_pkt.dts;
                                pkt.pts += ctx->video_offset->pts - delta_video_pkt.pts;

                                last_video_pkt = pkt;
                        }
                        
                        if(mtype == AVMEDIA_TYPE_AUDIO)
                        {
                                pkt.dts += ctx->audio_offset->dts - delta_audio_pkt.dts;
                                pkt.pts += ctx->audio_offset->pts - delta_audio_pkt.pts;

                                last_audio_pkt = pkt;
                        }

                        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
                        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
                        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
                        pkt.pos = 1;

                        if(out_stream->cur_dts >= pkt.dts)
                        {
                                wmslog_info("[ANAMOLY FOUND][FRAME DTS - %lld][CURRENT DTS - %lld][FIXING ANAMOLY BY CHANGING DTS]", pkt.dts, out_stream->cur_dts);
                                pkt.dts = out_stream->cur_dts + 1;
                                pkt.pts = pkt.dts;
                        }

                        if(pkt.pts < pkt.dts)
                        {
                                //wmslog_info("[ERROR WHILE WRITING][PTS SHOULD BE GREATER THAN DTS][PTS - %lld][DTS - %lld]", pkt.pts, pkt.dts);
                                pkt.pts = pkt.dts;
                        }

                        if(av_write_frame(ctx->out_ctx, &pkt) < 0)
                        {
                                wmslog_info("[PROBLEM WHILE WRITING FRAME][IGNORED][CURRENT DTS - %lld]", out_stream->cur_dts);
                        }
                }

                ctx->audio_offset->dts = last_audio_pkt.dts;
                ctx->audio_offset->pts = last_audio_pkt.pts; 

                ctx->video_offset->dts = last_video_pkt.dts;
                ctx->video_offset->pts = last_video_pkt.pts; 

                wmslog_info("[WRITE COMPLETED SUCCESFULLY][FILE - %s]", file.filename);
        }
        
        fclose(fp);

        return 1;
}




void scan_keyframes(AVFormatContext *ctx, WMSEditorFile *file, int n)
{
        AVPacket pkt; 
        
        while(av_read_frame(ctx, &pkt) >= 0)
        {
                if(isKeyFrame(pkt))
                {
                        scan_keyframes(ctx, file, n+1);
                        file->keyframes[n] = pkt;
                        
                        AVStream *stream = ctx->streams[pkt.stream_index];
                        int mtype = stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO;
                        
                        if(mtype)
                        {
                                //wmslog_debug("[KEYFRAME][%d][%d][%d]", file->keyframes[n].pts, n, stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO);
                        }
                }
        } 

        if(file->keyframes == NULL)
        {
                file->keyframes = (AVPacket *) calloc(n+1, sizeof(AVPacket));
                file->no_keyframes = n+1;
        }
}

AVPacket find_nearest_keyframe(WMSEditorFile *file, AVFormatContext *fmtctx, int starttime)
{
        AVPacket *keyframes = file->keyframes;
        int i, n = file->no_keyframes;
        
        AVPacket startpts;

        for(i=0; i<n; i++)
        {
                AVPacket pkt = keyframes[i];
                AVStream *stream = fmtctx->streams[pkt.stream_index];
                int isVideoFrame = stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO;

                if(!isVideoFrame)
                {
                        continue;
                }

                int64_t time = getTimeStamp(stream,pkt);

                if(time < 0)
                {
                        continue;
                }

                if(time < starttime)
                {
                        startpts = pkt;
                }
                else 
                {
                        return startpts;
                }
        }
        
        return startpts;
}

int add_file(WMSEditorContext *ctx, WMSEditorFile *file)
{
        ctx->in_files[ctx->n_counter] = *file;
        ctx->n_counter += 1;
        
        if(ctx->init_streams == 0)
        {
                init_output_streams(file->in_ctx, ctx);
    
                if(avformat_write_header(ctx->out_ctx, NULL) < 0)
                {
                        wmslog_error("[ERROR WHILE WRITING HEADER OUTPUT FILE][FILE - %s]", ctx->out_ctx);
                        return 0;
                }

                ctx->init_streams = 1;
        }
        
        return ctx->n_counter - 1;
}

int merge_video(WMSEditorContext *ctx, char *filename)
{
        WMSEditorFile *file = (WMSEditorFile *) calloc(1, sizeof(WMSEditorFile));
        
        if(open_file_ctx(&file->in_ctx, filename) < 0)
        {
                wmslog_error("[ABANDONING MERGE DUE TO ERROR IN OPENING FILE]");
                return -1;
        }
        
        file->filename = filename; 
        file->cut_video = 0;
        file->start_time = file->end_time = -1;

        return add_file(ctx, file);
}

int cut_video(WMSEditorContext *ctx, char *filename, int start_time, int end_time)
{
        WMSEditorFile *file = (WMSEditorFile *) calloc(1, sizeof(WMSEditorFile));
        
        if(open_file_ctx(&file->in_ctx, filename) < 0)
        {
            wmslog_error("[ABANDONING CUT DUE TO ERROR IN OPENING FILE]");
            return -1;
        }

        file->filename = filename;
        file->cut_video = 1;
        file->start_time = start_time; 
        file->end_time = end_time;

        if(file->cut_video == 1 && file->start_time > 0)
        {
                AVFormatContext *fmtctx = NULL; 
                file->keyframes = NULL;
                
                if(open_file_ctx(&fmtctx, file->filename) < 0)
                {
                        wmslog_error("[ERROR DURING OPENING FOR KEYFRAME SCANNING]");
                }
                else 
                {
                        wmslog_info("[SUCCESSFULY OPENED FILE FOR SEEKING]");
                }
                
                scan_keyframes(fmtctx, file, 0);
                file->nearest_keyframe = find_nearest_keyframe(file, fmtctx, file->start_time);

                wmslog_info("[START TIME][%d][NEAREST KEYFRAME PTS VALUE][%d]", file->start_time, file->nearest_keyframe.pts);
        }

        return add_file(ctx, file);
}

int insert_video(WMSEditorContext *ctx, char *main_video_file, char *insert_video_file, int timestamp)
{
        cut_video(ctx, main_video_file, -1, timestamp);
        merge_video(ctx, insert_video_file);
        cut_video(ctx, main_video_file, timestamp+1, -1);

        return 1;
}

int replace_video(WMSEditorContext *ctx, char *main_video_file, char *insert_video_file, int timestamp)
{
        cut_video(ctx, main_video_file, -1, timestamp);

        int pos = merge_video(ctx, insert_video_file);
        
        int64_t duration = ctx->in_files[pos].in_ctx->duration / AV_TIME_BASE; 
        int offset = duration + timestamp;

        cut_video(ctx, main_video_file, offset, -1);

        return 1;
}

int perform_video_edit(WMSEditorContext *ctx)
{
         if(write_stream(ctx) > 0)
         {
                av_write_trailer(ctx->out_ctx);
         }

         return 1;
}
