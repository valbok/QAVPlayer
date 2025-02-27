/*********************************************************
 * Copyright (C) 2025, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavmuxer_p.h"
#include "qavvideocodec_p.h"
#include "qavaudiocodec_p.h"
#include "qavsubtitlecodec_p.h"
#include "qavhwdevice_p.h"
#include "qaviodevice.h"
#include <QtAVPlayer/qtavplayerglobal.h>

#include <QDir>
#include <QSharedPointer>
#include <QMutexLocker>
#include <atomic>
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libavutil/timestamp.h>
}

// Workaround to use av_err2str
#ifdef av_err2str
#undef av_err2str
av_always_inline char *av_err2str(int errnum)
{
    thread_local char str[AV_ERROR_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}
#endif

QT_BEGIN_NAMESPACE

class QAVMuxerPrivate
{
    Q_DECLARE_PUBLIC(QAVMuxer)
public:
    QAVMuxerPrivate(QAVMuxer *q)
        : q_ptr(q)
    {
    }

    QAVMuxer *q_ptr = nullptr;
    AVFormatContext *ctx = nullptr;
    int *stream_mapping = nullptr;
    int stream_mapping_size = 0;
    bool loaded = false;
};

QAVMuxer::QAVMuxer()
    : d_ptr(new QAVMuxerPrivate(this))
{
}

QAVMuxer::~QAVMuxer()
{
    close();
    reset();
}

void QAVMuxer::reset()
{
    Q_D(QAVMuxer);
    if (d->ctx && !(d->ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&d->ctx->pb);
    avformat_free_context(d->ctx);
    d->ctx = nullptr;
    av_freep(&d->stream_mapping);
    d->stream_mapping = nullptr;
}

void QAVMuxer::close()
{
    Q_D(QAVMuxer);
    if (d->ctx && d->loaded)
        av_write_trailer(d->ctx);
    d->loaded = false;
}

int QAVMuxer::load(const AVFormatContext *ictx, const QString &filename)
{
    Q_D(QAVMuxer);
    reset();
    int ret = avformat_alloc_output_context2(&d->ctx, nullptr, nullptr, filename.toUtf8().constData());
    if (ret < 0) {
        qWarning() << "Could not create output context:" << av_err2str(ret);
        return ret;
    }
    d->stream_mapping_size = ictx->nb_streams;
    d->stream_mapping = (int *) av_calloc(d->stream_mapping_size, sizeof(*d->stream_mapping));

    int stream_index = 0;
    for (unsigned i = 0; i < ictx->nb_streams; ++i) {
        AVStream *out_stream;
        AVStream *in_stream = ictx->streams[i];
        AVCodecParameters *in_codecpar = in_stream->codecpar;

        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            d->stream_mapping[i] = -1;
            continue;
        }

        d->stream_mapping[i] = stream_index++;

        out_stream = avformat_new_stream(d->ctx, nullptr);
        if (!out_stream) {
            qWarning() << "Failed allocating output stream";
            return AVERROR_UNKNOWN;
        }

        ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        if (ret < 0) {
            qWarning() << "Failed to copy codec parameters" << av_err2str(ret);
            return ret;
        }
        out_stream->codecpar->codec_tag = 0;
    }
    av_dump_format(d->ctx, 0, filename.toUtf8().constData(), 1);

    if (!(d->ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&d->ctx->pb, filename.toUtf8().constData(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            qWarning() << "Could not open output file:" << filename << ":" << av_err2str(ret);
            return ret;
        }
    }

    ret = avformat_write_header(d->ctx, nullptr);
    if (ret < 0) {
        qWarning() << "Error occurred when opening output file:" << av_err2str(ret);
        return ret;
    }
    d->loaded = true;
    return 0;
}

int QAVMuxer::write(const QAVPacket &packet)
{
    Q_D(QAVMuxer);
    Q_ASSERT(d->ctx);
    auto pkt = packet.packet();
    auto in_stream = packet.stream().stream();
    if (pkt->stream_index >= d->stream_mapping_size ||
        d->stream_mapping[pkt->stream_index] < 0) {
        return AVERROR_UNKNOWN;
    }
    pkt->stream_index = d->stream_mapping[pkt->stream_index];
    auto out_stream = d->ctx->streams[pkt->stream_index];

    // copy packet
    av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);
    pkt->pos = -1;

    auto ret = av_interleaved_write_frame(d->ctx, pkt);
    // pkt is now blank (av_interleaved_write_frame() takes ownership of
    // its contents and resets pkt), so that no unreferencing is necessary.
    // This would be different if one used av_write_frame().
    if (ret < 0) {
        qWarning() << "Error muxing packet:" << av_err2str(ret);
        return ret;
    }
    return 0;
}

QT_END_NAMESPACE
