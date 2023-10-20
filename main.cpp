#include <iostream>

#include <vector>

#include <fstream>

#include <stdexcept>

#include <sstream>

#include <algorithm>


// External libraries

#include <ffmpeg/ffmpeg.h>


// Function prototypes

bool open_input_file(const std::string& input_file, AVFormatContext** input_format_context);

bool open_output_file(const std::string& output_file, AVFormatContext** output_format_context);

bool init_input_stream(AVFormatContext* input_format_context, int& stream_index);

bool init_output_stream(AVFormatContext* output_format_context, AVCodec* output_codec, AVStream* input_stream, AVStream** output_stream);

bool encode_audio_frame(AVFrame* frame, AVPacket* packet, AVCodecContext* codec_context);

bool decode_audio_frame(AVPacket* packet, AVFrame* frame, AVCodecContext* codec_context);

bool init_filters(const std::string& filters_descr);

bool filter_encode_audio_frame(AVFrame* frame, AVPacket* packet, AVFilterContext* buffersink_ctx, AVFilterGraph* filter_graph);


// Global variables

AVFormatContext* input_format_context = nullptr;

AVFormatContext* output_format_context = nullptr;

AVCodec* output_codec = nullptr;

AVStream* input_stream = nullptr;

AVStream* output_stream = nullptr;

AVFilterGraph* filter_graph = nullptr;

AVFilterContext* buffersink_ctx = nullptr;

AVFilterContext* buffersrc_ctx = nullptr;

// Function definitions

bool open_input_file(const std::string& input_file, AVFormatContext** input_format_context) {

    if (avformat_open_input(input_format_context, input_file.c_str(), nullptr, nullptr) < 0) {

        std::cerr << "Error opening input file: " << input_file << std::endl;

        return false;

    }


    if (avformat_find_stream_info(*input_format_context, nullptr) < 0) {

        std::cerr << "Error finding stream information for input file: " << input_file << std::endl;

        return false;

    }


    return true;

}


bool open_output_file(const std::string& output_file, AVFormatContext** output_format_context) {

    AVOutputFormat* output_format = av_guess_format(nullptr, output_file.c_str(), nullptr);

    if (!output_format) {

        std::cerr << "Error guessing output format for output file: " << output_file << std::endl;

        return false;

    }


    if (avformat_alloc_output_context2(output_format_context, output_format, nullptr, output_file.c_str()) < 0) {

        std::cerr << "Error allocating output context for output file: " << output_file << std::endl;

        return false;

    }


    return true;

}


bool init_input_stream(AVFormatContext* input_format_context, int& stream_index) {

    for (int i = 0; i < input_format_context->nb_streams; i++) {

        if (input_format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {

            stream_index = i;

            break;

        }

    }


    if (stream_index == -1) {

        std::cerr << "Error finding audio stream in input file" << std::endl;

        return false;

    }


    return true;

}


bool init_output_stream(AVFormatContext* output_format_context, AVCodec* output_codec, AVStream* input_stream, AVStream** output_stream) {

    *output_stream = avformat_new_stream(output_format_context, output_codec);

    if (!*output_stream) {

        std::cerr << "Error creating new output stream" << std::endl;

        return false;

    }


    if (avcodec_parameters_copy((*output_stream)->codecpar, input_stream->codecpar) < 0) {

        std::cerr << "Error copying codec parameters to output stream" << std::endl;

        return false;

    }


    (*output_stream)->codecpar->codec_tag = 0;


    return true;

}


bool encode_audio_frame(AVFrame* frame, AVPacket* packet, AVCodecContext* codec_context) {

    int ret = avcodec_send_frame(codec_context, frame);

    if (ret < 0) {

        std::cerr << "Error sending audio frame to encoder" << std::endl;

        return false;

    }


    while (ret >= 0) {

        ret = avcodec_receive_packet(codec_context, packet);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {

            return true;

        } else if (ret < 0) {

            std::cerr << "Error receiving packet from encoder" << std::endl;

            return false;

        }


        av_packet_rescale_ts(packet, codec_context->time_base, (*output_stream)->time_base);

        packet->stream_index = (*output_stream)->index;


        if (av_interleaved_write_frame(output_format_context, packet) < 0) {

            std::cerr << "Error writing packet to output file" << std::endl;

            return false;

        }


        av_packet_unref(packet);

    }


    return true;

}


bool decode_audio_frame(AVPacket* packet, AVFrame* frame, AVCodecContext* codec_context) {

    int ret = avcodec_send_packet(codec_context, packet);

    if (ret < 0) {

        std::cerr << "Error sending packet to decoder" << std::endl;

        return false;

    }


    while (ret >= 0) {

        ret = avcodec_receive_frame(codec_context, frame);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {

            return true;

        } else if (ret < 0) {

            std::cerr << "Error receiving frame from decoder" << std::endl;

            return false;

        }


        if (!encode_audio_frame(frame, output_packet, output_codec_context)) {

            return false;

        }


        av_frame_unref(frame);

    }


    return true;

}


int main(int argc, char* argv[]) {

    if (argc != 3) {

        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;

        return 1;

    }


    std::string input_file = argv[1];

    std::string output_file = argv[2];


    AVFormatContext* input_format_context = nullptr;

    AVFormatContext* output_format_context = nullptr;

    AVCodec* output_codec = nullptr;

    AVCodecContext* input_codec_context = nullptr;

    AVCodecContext* output_codec_context = nullptr;

    AVFrame* frame = nullptr;

    AVPacket* packet = nullptr;

    AVPacket* output_packet = nullptr;

    int stream_index = -1;


    av_register_all();

    avformat_network_init();


    if (!open_input_file(input_file, &input_format_context)) {

        return 1;

    }


    if (!open_output_file(output_file, &output_format_context)) {

        return 1;

    }


    output_codec = avcodec_find_encoder(AV_CODEC_ID_AAC);

    if (!output_codec) {

        std::cerr << "Error finding output codec" << std::endl;

        return 1;

    }


    if (!init_decoder(input_format_context, &input_codec_context, &stream_index)) {

        return 1;

    }


    if (!init_encoder(output_format_context, output_codec, &output_codec_context, &frame, &packet, &output_packet)) {

        return 1;

    }


    while (av_read_frame(input_format_context, packet) >= 0) {

        if (packet->stream_index == stream_index) {

            if (!decode_audio_frame(packet, frame, input_codec_context)) {

                return 1;

            }

        }


        av_packet_unref(packet);

    }


    av_write_trailer(output_format_context);


    if (!close_input_file(input_format_context)) {

        return 1;

    }


    if (!close_output_file(output_format_context)) {

        return 1;

    }


    av_frame_free(&frame);

    av_packet_free(&packet);

    av_packet_free(&output_packet);

    avcodec_free_context(&input_codec_context);

    avcodec_free_context(&output_codec_context);


    return 0;

}
