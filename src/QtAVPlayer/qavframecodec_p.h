/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFRAMECODEC_P_H
#define QAVFRAMECODEC_P_H

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

#include "qavcodec_p.h"

QT_BEGIN_NAMESPACE

struct AVPacket;
struct AVFrame;
class Q_AVPLAYER_EXPORT QAVFrameCodec : public QAVCodec
{
public:
    // Returns true if frame is received
    //         false to try next packet
    bool decode(const AVPacket *pkt, AVFrame *frame) const;

protected:
    QAVFrameCodec(QObject *parent = nullptr);
    QAVFrameCodec(QAVCodecPrivate &d, QObject *parent = nullptr);
};

QT_END_NAMESPACE

#endif
