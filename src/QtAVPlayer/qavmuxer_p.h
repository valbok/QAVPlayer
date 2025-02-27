/*********************************************************
 * Copyright (C) 2025, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVMUXER_H
#define QAVMUXER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qavpacket_p.h"
#include "qavstream.h"
#include "qavframe.h"
#include "qavsubtitleframe.h"
#include <memory>

struct AVFormatContext;
QT_BEGIN_NAMESPACE

class QAVMuxerPrivate;
/**
 * Allows to mux and write the input streams to output file.
 */
class QAVMuxer
{
public:
    QAVMuxer();
    ~QAVMuxer();

    /**
     * @param ictx Input format context to use streams from
     * @param filename Output filename to write the packets to
     * @see QAVDemuxer::load
     */
    int load(const AVFormatContext *ictx, const QString &filename);
    /**
     * Transfers the ownership of the packet
     */
    int write(const QAVPacket &packet);
    /**
     * Closes the opened stream if any
     */
    void close();

private:
    void reset();

    Q_DISABLE_COPY(QAVMuxer)
    Q_DECLARE_PRIVATE(QAVMuxer)
    std::unique_ptr<QAVMuxerPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif
