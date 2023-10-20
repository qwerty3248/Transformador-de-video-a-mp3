#include <iostream>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <algorithm>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}

// Function prototypes
bool open_input_file(const std::string& input_file, AVFormatContext** input_format_context);
bool open_output_file(const std::string& output_file, AVFormatContext** output_format_context);
bool init_input_stream(AVFormatContext* input_format_context, int& stream_index);
bool init_output_stream(AVFormatContext* output_format_context, AVCodec* output_codec, AVStream* input_stream, AVStream** output_stream);
bool encode_audio_frame(AVFrame* frame, AVPacket* packet, AVCodecContext* codec_context, AVFormatContext* output_format_context);
bool decode_audio_frame(AVPacket* packet, AVFrame* frame, AVCodecContext* codec_context);
bool init_filters(const std::string& filters_descr, AVFormatContext* input_format_context, AVFilterGraph** filter_graph, AVFilterContext** buffersink_ctx, AVFilterContext** buffersrc_ctx);

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
        std::cerr << "Error finding audio stream in the input file" << std::endl;
        return false;
    }

    return true;
}

bool init_output_stream(AVFormatContext* output_format_context, AVCodec* output_codec, AVStream* input_stream, AVStream** output_stream) {
    *output_stream = avformat_new_stream(output_format_context, output_codec);
    if (!*output_stream) {
        std::cerr << "Error creating a new output stream" << std::endl;
        return false;
    }

    if (avcodec_parameters_copy((*output_stream)->codecpar, input_stream->codecpar) < 0) {
        std::cerr << "Error copying codec parameters to the output stream" << std::endl;
        return false;
    }

    (*output_stream)->codecpar->codec_tag = 0;

    return true;
}

bool encode_audio_frame(AVFrame* frame, AVPacket* packet, AVCodecContext* codec_context, AVFormatContext* output_format_context) {
    int ret = avcodec_send_frame(codec_context, frame);
    if (ret < 0) {
        std::cerr << "Error sending an audio frame to the encoder" << std::endl;
        return false;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(codec_context, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return true;
        } else if (ret < 0) {
            std::cerr << "Error receiving a packet from the encoder" << std::endl;
            return false;
        }

        av_packet_rescale_ts(packet, codec_context->time_base, output_stream->time_base);
	packet->stream_index = output_stream->index;


        if (av_interleaved_write_frame(output_format_context, packet) < 0) {
            std::cerr << "Error writing a packet to the output file" << std::endl;
            return false;
        }

        av_packet_unref(packet);
    }

    return true;
}

bool decode_audio_frame(AVPacket* packet, AVFrame* frame, AVCodecContext* codec_context) {
    int ret = avcodec_send_packet(codec_context, packet);
    if (ret < 0) {
        std::cerr << "Error sending a packet to the decoder" << std::endl;
        return false;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(codec_context, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return true;
        } else if (ret < 0) {
            std::cerr << "Error receiving a frame from the decoder" << std::endl;
            return false;
        }

        if (!encode_audio_frame(frame, packet, codec_context, output_format_context)) {
            return false;
        }

        av_frame_unref(frame);
    }

    return true;
}


bool init_filters(const std::string& filters_descr, AVFormatContext* input_format_context, AVFilterGraph** filter_graph, AVFilterContext** buffersink_ctx, AVFilterContext** buffersrc_ctx) {
    *filter_graph = avfilter_graph_alloc();
    if (!*filter_graph) {
        std::cerr << "Error allocating the filter graph" << std::endl;
        return false;
    }

    const AVFilter* buffersrc = avfilter_get_by_name("abuffer");
    const AVFilter* buffersink = avfilter_get_by_name("abuffersink");


    if (!buffersrc || !buffersink) {
        std::cerr << "Error locating filter 'abuffer' or 'abuffersink'" << std::endl;
        return false;
    }

    AVFilterInOut* inputs = avfilter_inout_alloc();
    AVFilterInOut* outputs = avfilter_inout_alloc();
    if (!inputs || !outputs) {
        std::cerr << "Error allocating filter inputs and outputs" << std::endl;
        return false;
    }

    AVFilterContext* buffersrc_ctx_tmp = nullptr;
    AVFilterContext* buffersink_ctx_tmp = nullptr;

    if (avfilter_graph_create_filter(&buffersrc_ctx_tmp, buffersrc, "in", nullptr, nullptr, 
        *filter_graph) < 0 ||
    avfilter_graph_create_filter(&buffersink_ctx_tmp, buffersink, "out", nullptr, nullptr, 
        *filter_graph) < 0) {
    std::cerr << "Error creating filter contexts" << std::endl;
    return false;
}


    AVFilterContext* buffersrc_ctx = const_cast<AVFilterContext*>(buffersrc_ctx_tmp);
    AVFilterContext* buffersink_ctx = const_cast<AVFilterContext*>(buffersink_ctx_tmp);

    char args[512];
    AVPixelFormat pix_fmts[] = { AV_PIX_FMT_BGR0, AV_PIX_FMT_NONE };
    std::ostringstream args_stream;
    args_stream << "time_base=" << input_format_context->streams[input_stream->index]->time_base.num << '/'
                << input_format_context->streams[input_stream->index]->time_base.den << ":sample_rate="
                << input_format_context->streams[input_stream->index]->codecpar->sample_rate << ":sample_fmt="
                << av_get_sample_fmt_name(input_format_context->streams[input_stream->index]->codecpar->format)
                << ":channel_layout=0x" << std::hex
                << input_format_context->streams[input_stream->index]->codecpar->channel_layout << std::dec;

    avfilter_graph_create_filter(buffersrc_ctx, buffersrc, "in", args_stream.str().c_str(), 
                                 nullptr, *filter_graph);
    avfilter_graph_create_filter(buffersink_ctx, buffersink, "out", nullptr, nullptr, 
                                 *filter_graph);


    AVFilterInOut* inputs_tmp = inputs;
    AVFilterInOut* outputs_tmp = outputs;

    inputs->name = av_strdup("in");
    inputs->filter_ctx = buffersrc_ctx;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    outputs->name = av_strdup("out");
    outputs->filter_ctx = buffersink_ctx;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    if (avfilter_graph_parse_ptr(*filter_graph, filters_descr.c_str(), &inputs_tmp, &outputs_tmp, nullptr) < 0) {
        std::cerr << "Error parsing filters description" << std::endl;
        return false;
    }

    if (avfilter_graph_config(*filter_graph, nullptr) < 0) {
        std::cerr << "Error configuring the filter graph" << std::endl;
        return false;
    }

    buffersrc_ctx = const_cast<AVFilterContext*>(buffersrc_ctx_tmp);
    buffersink_ctx = const_cast<AVFilterContext*>(buffersink_ctx_tmp);


    return true;
}

int main(int argc, char* argv[]) {
    // Check for proper command line arguments, and set input and output file names
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " input_file output_file" << std::endl;
        return 1;
    }

    const std::string input_file = argv[1];
    const std::string output_file = argv[2];

    av_register_all();

    // Open the input file
    if (!open_input_file(input_file, &input_format_context)) {
        return 1;
    }

    // Open the output file
    if (!open_output_file(output_file, &output_format_context)) {
        return 1;
    }

    // Initialize the input stream
    int stream_index = -1;
    if (!init_input_stream(input_format_context, stream_index)) {
        return 1;
    }

    // Initialize the output codec and stream
    output_codec = avcodec_find_encoder(AV_CODEC_ID_MP3);
    if (!output_codec) {
        std::cerr << "Error finding the MP3 encoder" << std::endl;
        return 1;
    }

    if (!init_output_stream(output_format_context, output_codec, input_format_context->streams[stream_index], &output_stream)) {
        return 1;
    }

    // Initialize audio filters
    if (!init_filters("anull", input_format_context, &filter_graph, &buffersink_ctx, &buffersrc_ctx)) {
        return 1;
    }

    // Create audio frame and packet
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "Error allocating audio frame" << std::endl;
        return 1;
    }

    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        std::cerr << "Error allocating packet" << std::endl;
        return 1;
    }

    // Main processing loop
    while (true) {
        AVPacket packet;
        av_init_packet(&packet);
        packet.data = nullptr;
        packet.size = 0;

        // Read a packet from the input file
        if (av_read_frame(input_format_context, &packet) < 0) {
            break;
        }

        if (packet.stream_index == stream_index) {
            // Decode the audio frame
            if (!decode_audio_frame(&packet, frame, input_format_context->streams[stream_index]->codec)) {
                return 1;
            }
        }

        av_packet_unref(&packet);
    }

    // Flush the encoder
    if (!encode_audio_frame(nullptr, packet, output_stream->codec, output_format_context)) {
        return 1;
    }

    // Clean up
    avformat_free_context(input_format_context);
    avformat_free_context(output_format_context);
    avcodec_free_context(&output_stream->codec);
    av_frame_free(&frame);
    av_packet_free(&packet);

    // Clean up audio filters
    avfilter_free(buffersrc_ctx);
    avfilter_free(buffersink_ctx);
    avfilter_graph_free(&filter_graph);

    return 0;
}

