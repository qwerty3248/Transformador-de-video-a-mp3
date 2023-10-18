#include <iostream>
#include <string>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>

int main(int argc, char* argv[]) {
    // Verificar que se proporcionó el nombre del archivo de video como argumento.
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <nombre_del_archivo_de_video>" << std::endl;
        return -1;
    }

    // Inicializar FFmpeg.
    av_register_all();
    avcodec_register_all();

    // Abrir el archivo de entrada de video.
    AVFormatContext* inputFormatContext = nullptr;
    if (avformat_open_input(&inputFormatContext, argv[1], nullptr, nullptr) != 0) {
        std::cerr << "Error: No se pudo abrir el archivo de entrada." << std::endl;
        return -1;
    }

    // Obtener información del formato del archivo.
    if (avformat_find_stream_info(inputFormatContext, nullptr) < 0) {
        std::cerr << "Error: No se pudo obtener información del formato del archivo." << std::endl;
        avformat_close_input(&inputFormatContext);
        return -1;
    }

    // Encontrar la corriente de audio.
    int audioStreamIndex = -1;
    for (unsigned int i = 0; i < inputFormatContext->nb_streams; i++) {
        if (inputFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            break;
        }
    }

    if (audioStreamIndex == -1) {
        std::cerr << "Error: No se encontró una corriente de audio en el archivo de entrada." << std::endl;
        avformat_close_input(&inputFormatContext);
        return -1;
    }

    // Abre el códec de audio.
    AVCodec* codec = avcodec_find_decoder(inputFormatContext->streams[audioStreamIndex]->codecpar->codec_id);
    AVCodecContext* codecContext = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codecContext, inputFormatContext->streams[audioStreamIndex]->codecpar) < 0) {
        std::cerr << "Error: No se pudo configurar el códec de audio." << std::endl;
        avformat_close_input(&inputFormatContext);
        return -1;
    }

    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        std::cerr << "Error: No se pudo abrir el códec de audio." << std::endl;
        avformat_close_input(&inputFormatContext);
        avcodec_free_context(&codecContext);
        return -1;
    }

    // Configurar un contexto de resample.
    SwrContext* resampleContext = swr_alloc_set_opts(nullptr,
                                                    AV_CH_LAYOUT_STEREO,  // Diseño de canales de salida
                                                    AV_SAMPLE_FMT_FLTP,   // Formato de muestras de salida
                                                    codecContext->sample_rate,
                                                    AV_CH_LAYOUT_STEREO,  // Diseño de canales de entrada
                                                    codecContext->sample_fmt,
                                                    codecContext->sample_rate,
                                                    0,
                                                    nullptr);

    if (!resampleContext || swr_init(resampleContext) < 0) {
        std::cerr << "Error: No se pudo configurar el contexto de resample." << std::endl;
        avformat_close_input(&inputFormatContext);
        avcodec_free_context(&codecContext);
        swr_free(&resampleContext);
        return -1;
    }

    // Crear un nombre de archivo de salida MP3 en la carpeta "mp3" con el mismo nombre del video.
    std::string videoFileName = argv[1];
    std::size_t found = videoFileName.find_last_of("/");
    std::string outputPath = "mp3/" + videoFileName.substr(found + 1) + ".mp3";

    // Abre el archivo de salida MP3.
    AVFormatContext* outputFormatContext = nullptr;
    if (avformat_alloc_output_context2(&outputFormatContext, nullptr, nullptr, outputPath.c_str()) < 0) {
        std::cerr << "Error: No se pudo abrir el archivo de salida MP3." << std::endl;
        avformat_close_input(&inputFormatContext);
        avcodec_free_context(&codecContext);
        swr_free(&resampleContext);
        return -1;
    }

    // Procesar los paquetes de audio y escribirlos en el archivo de salida MP3.
    AVPacket packet;
    av_init_packet(&packet);
    while (av_read_frame(inputFormatContext, &packet) >= 0) {
        if (packet.stream_index == audioStreamIndex) {
            AVFrame* frame = av_frame_alloc();
            if (avcodec_receive_frame(codecContext, frame) >= 0) {
                // Realizar la resample...
                int num_output_samples = av_rescale_rnd(swr_get_delay(resampleContext, frame->sample_rate) + frame->nb_samples,
                                                        codecContext->sample_rate, frame->sample_rate, AV_ROUND_UP);

                AVFrame* resampledFrame = av_frame_alloc();
                resampledFrame->channel_layout = AV_CH_LAYOUT_STEREO;
                resampledFrame->format = AV_SAMPLE_FMT_FLTP;
                resampledFrame->sample_rate = codecContext->sample_rate;
                av_frame_get_buffer(resampledFrame, 0);

                if (swr_convert(resampleContext, resampledFrame->data, num_output_samples,
                                (const uint8_t**)frame->data, frame->nb_samples) < 0) {
                    std::cerr << "Error: No se pudo realizar la resample." << std::endl;
                    av_frame_free(&frame);
                    av_frame_free(&resampledFrame);
                    av_packet_unref(&packet);
                    break;
                }

                // Crear un paquete de salida para el archivo MP3.
                AVPacket outputPacket;
                av_init_packet(&outputPacket);
                outputPacket.data = nullptr;
                outputPacket.size = 0;

                // Escribir el paquete en el archivo de salida MP3.
                if (av_write_frame(outputFormatContext, &outputPacket) < 0) {
                    std::cerr << "Error: No se pudo escribir el paquete en el archivo MP3." << std::endl;
                }
                av_packet_unref(&outputPacket);
                av_frame_free(&resampledFrame);
            }
            av_frame_free(&frame);
        }
        av_packet_unref(&packet);
    }

    // Finalizar la escritura y cerrar el archivo de salida MP3.
    if (avio_open(&outputFormatContext->pb, outputPath.c_str(), AVIO_FLAG_WRITE) < 0) {
        std::cerr << "Error: No se pudo abrir el archivo de salida MP3 para escritura." << std::endl;
    } else {
        if (avformat_write_header(outputFormatContext, nullptr) < 0) {
            std::cerr << "Error: No se pudo escribir la cabecera del archivo de salida MP3." << std::endl;
        }
        // Más operaciones de escritura y limpieza...
    }

    // Cerrar archivos de entrada y salida MP3.
    avio_closep(&outputFormatContext->pb);
    avformat_free_context(outputFormatContext);

    // Liberar recursos personalizados adicionales.
    // Ejemplo: Liberar memoria asignada dinámicamente.
    // Ejemplo: Cerrar otros archivos o recursos personalizados.

    // Finalizar y limpiar el contexto de resample.
    swr_free(&resampleContext);

    // Cerrar el códec de audio.
    avcodec_close(codecContext);
    avcodec_free_context(&codecContext);

    // Cerrar el archivo de entrada.
    avformat_close_input(&inputFormatContext);

    return 0;
}

