/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavplayer.h"
#include "qavdemuxer_p.h"
#include "qavvideocodec_p.h"
#include "qavaudiocodec_p.h"
#include "qavvideoframe.h"
#include "qavaudioframe.h"
#include "qavpacketqueue_p.h"
#include <QtConcurrent/qtconcurrentrun.h>
#include <QLoggingCategory>

extern "C" {
#include <libavformat/avformat.h>
}

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcAVPlayer, "qt.QtAVPlayer")

class QAVPlayerPrivate
{
    Q_DECLARE_PUBLIC(QAVPlayer)
public:
    QAVPlayerPrivate(QAVPlayer *q)
        : q_ptr(q)
    {
        threadPool.setMaxThreadCount(4);
    }

    void setMediaStatus(QAVPlayer::MediaStatus status);
    bool setState(QAVPlayer::State s);
    void setSeekable(bool seekable);
    void setError(QAVPlayer::Error err, const QString &str);
    void setDuration(double d);
    void updatePosition(double p);
    bool isSeeking() const;
    void setVideoFrameRate(double v);

    void terminate();

    void doWait();
    void setWait(bool v);
    void doLoad(const QUrl &url);
    void doDemux();
    void doPlayVideo();
    void doPlayAudio();

    template <class T>
    void dispatch(T fn);

    QAVPlayer *q_ptr = nullptr;
    QUrl url;
    QAVPlayer::MediaStatus mediaStatus = QAVPlayer::NoMedia;
    QAVPlayer::State state = QAVPlayer::StoppedState;
    mutable QMutex stateMutex;

    bool seekable = false;
    qreal speed = 1.0;
    mutable QMutex speedMutex;
    double videoFrameRate = 0.0;

    QAVPlayer::Error error = QAVPlayer::NoError;
    QString errorString;

    double duration = 0;
    double position = 0;
    double pendingPosition = -1;
    mutable QMutex positionMutex;

    bool pendingPlay = false;

    QAVDemuxer demuxer;

    QThreadPool threadPool;
    QFuture<void> loaderFuture;
    QFuture<void> demuxerFuture;

    QFuture<void> videoPlayFuture;
    QAVPacketQueue videoQueue;

    QFuture<void> audioPlayFuture;
    QAVPacketQueue audioQueue;
    double audioPts = 0;

    bool quit = 0;
    bool wait = false;
    mutable QMutex waitMutex;
    QWaitCondition waitCond;
};

static QString err_str(int err)
{
    char errbuf[128];
    const char *errbuf_ptr = errbuf;
    if (av_strerror(err, errbuf, sizeof(errbuf)) < 0)
        errbuf_ptr = strerror(AVUNERROR(err));

    return QString::fromUtf8(errbuf_ptr);
}

void QAVPlayerPrivate::setMediaStatus(QAVPlayer::MediaStatus status)
{
    {
        QMutexLocker locker(&stateMutex);
        if (mediaStatus == status)
            return;

        qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << mediaStatus << "->" << status;
        mediaStatus = status;
    }

    emit q_ptr->mediaStatusChanged(status);
}

bool QAVPlayerPrivate::setState(QAVPlayer::State s)
{
    Q_Q(QAVPlayer);
    bool result = false;
    {
        QMutexLocker locker(&stateMutex);
        if (state == s)
            return result;

        qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << state << "->" << s;
        state = s;
        result = true;
    }

    emit q->stateChanged(s);
    return result;
}

void QAVPlayerPrivate::setSeekable(bool s)
{
    Q_Q(QAVPlayer);
    if (seekable == s)
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << seekable << "->" << s;
    seekable = s;
    emit q->seekableChanged(seekable);
}

void QAVPlayerPrivate::setDuration(double d)
{
    Q_Q(QAVPlayer);
    if (qFuzzyCompare(duration, d))
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << duration << "->" << d;
    duration = d;
    emit q->durationChanged(q->duration());
}

void QAVPlayerPrivate::updatePosition(double p)
{
    QMutexLocker locker(&positionMutex);
    position = p;
    // If the demuxer reported that seeking is finished
    if (pendingPosition < 0) {
        locker.unlock();
        switch (q_ptr->mediaStatus()) {
            case QAVPlayer::SeekingMedia:
                qCDebug(lcAVPlayer) << "Seeked to pos:" << q_ptr->position();
                setMediaStatus(QAVPlayer::LoadedMedia);
                emit q_ptr->seeked(q_ptr->position());
                break;
            case QAVPlayer::PausingMedia:
                qCDebug(lcAVPlayer) << "Paused to pos:" << q_ptr->position();
                setMediaStatus(QAVPlayer::LoadedMedia);
                emit q_ptr->paused(q_ptr->position());
                break;
            case QAVPlayer::SteppingMedia:
                qCDebug(lcAVPlayer) << "Stepped to pos:" << q_ptr->position();
                setMediaStatus(QAVPlayer::LoadedMedia);
                emit q_ptr->stepped(q_ptr->position());
                break;
            default:
                break;
        }

        // Show only first decoded frame on seek/pause
        QAVPlayer::State currState = q_ptr->state();
        if (!quit && (currState == QAVPlayer::PausedState || currState == QAVPlayer::StoppedState)) {
            setWait(true);
        }
    }
}

bool QAVPlayerPrivate::isSeeking() const
{
    QMutexLocker locker(&positionMutex);
    return pendingPosition >= 0;
}

void QAVPlayerPrivate::setVideoFrameRate(double v)
{
    Q_Q(QAVPlayer);
    if (qFuzzyCompare(videoFrameRate, v))
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << videoFrameRate << "->" << v;
    videoFrameRate = v;
    emit q->videoFrameRateChanged(v);
}

template <class T>
void QAVPlayerPrivate::dispatch(T fn)
{
    QMetaObject::invokeMethod(q_ptr, fn, nullptr);
}

void QAVPlayerPrivate::setError(QAVPlayer::Error err, const QString &str)
{
    Q_Q(QAVPlayer);
    if (error == err)
        return;

    qWarning() << "Error:" << url << ":"<< str;
    error = err;
    errorString = str;
    emit q->errorOccurred(err, str);
    setMediaStatus(QAVPlayer::InvalidMedia);
}

void QAVPlayerPrivate::terminate()
{
    qCDebug(lcAVPlayer) << __FUNCTION__;
    setState(QAVPlayer::StoppedState);
    demuxer.abort();
    quit = true;
    pendingPlay = false;
    pendingPosition = -1;
    setWait(false);
    videoFrameRate = 0.0;
    videoQueue.clear();
    videoQueue.abort();
    audioQueue.clear();
    audioQueue.abort();
    loaderFuture.waitForFinished();
    demuxerFuture.waitForFinished();
    videoPlayFuture.waitForFinished();
    audioPlayFuture.waitForFinished();
}

void QAVPlayerPrivate::doWait()
{
    QMutexLocker lock(&waitMutex);
    if (wait)
        waitCond.wait(&waitMutex);
}

void QAVPlayerPrivate::setWait(bool v)
{
    {
        QMutexLocker locker(&waitMutex);
        wait = v;
    }

    if (v) {
        videoQueue.wakeAll();
        audioQueue.wakeAll();
    } else {
        waitCond.wakeAll();
    }
}

void QAVPlayerPrivate::doLoad(const QUrl &url)
{
    demuxer.abort(false);
    demuxer.unload();
    int ret = demuxer.load(url);
    if (ret < 0) {
        dispatch([this, ret] { setError(QAVPlayer::ResourceError, err_str(ret)); });
        return;
    }

    if (demuxer.videoStream() < 0 && demuxer.audioStream() < 0) {
        dispatch([this] { setError(QAVPlayer::ResourceError, QLatin1String("No codecs found")); });
        return;
    }

    double d = demuxer.duration();
    bool seekable = demuxer.seekable();
    double frameRate = demuxer.frameRate();
    dispatch([this, d, seekable, frameRate] {
        qCDebug(lcAVPlayer) << "[" << this->url << "]: Loaded, seekable:" << seekable << ", duration:" << d;
        setSeekable(seekable);
        setDuration(d);
        setVideoFrameRate(frameRate);
        if (!isSeeking())
            setMediaStatus(QAVPlayer::LoadedMedia);
        if (pendingPlay)
            q_ptr->play();
    });

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    demuxerFuture = QtConcurrent::run(&threadPool, this, &QAVPlayerPrivate::doDemux);
    videoPlayFuture = QtConcurrent::run(&threadPool, this, &QAVPlayerPrivate::doPlayVideo);
    audioPlayFuture = QtConcurrent::run(&threadPool, this, &QAVPlayerPrivate::doPlayAudio);
#else
    demuxerFuture = QtConcurrent::run(&threadPool, &QAVPlayerPrivate::doDemux, this);
    videoPlayFuture = QtConcurrent::run(&threadPool, &QAVPlayerPrivate::doPlayVideo, this);
    audioPlayFuture = QtConcurrent::run(&threadPool, &QAVPlayerPrivate::doPlayAudio, this);
#endif
}

void QAVPlayerPrivate::doDemux()
{
    const int maxQueueBytes = 15 * 1024 * 1024;
    QMutex waiterMutex;
    QWaitCondition waiter;

    while (!quit) {
        doWait();
        if (videoQueue.bytes() + audioQueue.bytes() > maxQueueBytes
            || (videoQueue.enough() && audioQueue.enough()))
        {
            QMutexLocker locker(&waiterMutex);
            waiter.wait(&waiterMutex, 10);
            continue;
        }

        {
            QMutexLocker locker(&positionMutex);
            if (pendingPosition >= 0) {
                const double pos = pendingPosition;
                locker.unlock();
                qCDebug(lcAVPlayer) << "Seeking to pos:" << pos * 1000;
                const int ret = demuxer.seek(pos);
                if (ret >= 0) {
                    videoQueue.clear();
                    audioQueue.clear();
                    qCDebug(lcAVPlayer) << "Waiting before all packets are handled";
                    videoQueue.waitForFinished();
                    audioQueue.waitForFinished();
                } else {
                    qWarning() << "Could not seek:" << err_str(ret);
                }
                locker.relock();
                if (qFuzzyCompare(pendingPosition, pos))
                    pendingPosition = -1;
            }
        }

        auto packet = demuxer.read();
        if (!packet) {
            if (demuxer.eof() && videoQueue.isEmpty() && audioQueue.isEmpty()) {
                dispatch([this] {
                    qCDebug(lcAVPlayer) << "EndOfMedia";
                    q_ptr->stop();
                    setMediaStatus(QAVPlayer::EndOfMedia);
                });
            }

            QMutexLocker locker(&waiterMutex);
            waiter.wait(&waiterMutex, 10);
            continue;
        }

        if (packet.streamIndex() == demuxer.videoStream()) {
            videoQueue.enqueue(packet);
        } else if (packet.streamIndex() == demuxer.audioStream()) {
            audioQueue.enqueue(packet);
        }
    }
}

void QAVPlayerPrivate::doPlayVideo()
{
    videoQueue.setFrameRate(demuxer.frameRate());

    while (!quit) {
        doWait();
        QAVVideoFrame frame = videoQueue.sync(q_ptr->speed(), audioQueue.pts());
        if (!frame)
            continue;

        emit q_ptr->videoFrame(frame);

        updatePosition(frame.pts());
        videoQueue.pop();
    }

    emit q_ptr->videoFrame({});
    videoQueue.clear();
}

void QAVPlayerPrivate::doPlayAudio()
{
    QAVAudioFrame frame;
    volatile const bool hasVideo = q_ptr->hasVideo();

    while (!quit) {
        doWait();
        const qreal currSpeed = q_ptr->speed();
        frame = audioQueue.sync(currSpeed);
        if (!frame)
            continue;

        frame.frame()->sample_rate *= currSpeed;
        emit q_ptr->audioFrame(frame);

        if (!hasVideo)
            updatePosition(frame.pts());
        audioQueue.pop();
    }

    audioQueue.clear();
}

QAVPlayer::QAVPlayer(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVPlayerPrivate(this))
{
}

QAVPlayer::~QAVPlayer()
{
    Q_D(QAVPlayer);
    d->terminate();
}

void QAVPlayer::setSource(const QUrl &url)
{
    Q_D(QAVPlayer);
    if (d->url == url)
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << url;
    d->terminate();
    d->url = url;
    emit sourceChanged(url);
    if (d->url.isEmpty()) {
        d->setMediaStatus(QAVPlayer::NoMedia);
        d->setDuration(0);
        d->updatePosition(0);
        return;
    }

    d->setWait(true);
    d->quit = false;
    d->setMediaStatus(QAVPlayer::LoadingMedia);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    d->loaderFuture = QtConcurrent::run(&d->threadPool, d, &QAVPlayerPrivate::doLoad, d->url);
#else
    d->loaderFuture = QtConcurrent::run(&d->threadPool, &QAVPlayerPrivate::doLoad, d, d->url);
#endif
}

QUrl QAVPlayer::source() const
{
    return d_func()->url;
}

bool QAVPlayer::hasAudio() const
{
    return d_func()->demuxer.audioStream() >= 0;
}

bool QAVPlayer::hasVideo() const
{
    return d_func()->demuxer.videoStream() >= 0;
}

QAVPlayer::State QAVPlayer::state() const
{
    Q_D(const QAVPlayer);
    QMutexLocker locker(&d->stateMutex);
    return d->state;
}

QAVPlayer::MediaStatus QAVPlayer::mediaStatus() const
{
    Q_D(const QAVPlayer);
    QMutexLocker locker(&d->stateMutex);
    return d->mediaStatus;
}

void QAVPlayer::play()
{
    Q_D(QAVPlayer);
    if (d->url.isEmpty() || mediaStatus() == QAVPlayer::InvalidMedia)
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__;
    d->setState(QAVPlayer::PlayingState);
    if (mediaStatus() == QAVPlayer::EndOfMedia) {
        if (!d->isSeeking()) {
            qCDebug(lcAVPlayer) << "Playing from beginning";
            seek(0);
            d->setMediaStatus(QAVPlayer::LoadedMedia);
        }
    } else if (mediaStatus() != QAVPlayer::LoadedMedia) {
        qCDebug(lcAVPlayer) << "Postponing playing until loaded";
        d->pendingPlay = true;
        return;
    }

    d->setWait(false);
    d->pendingPlay = false;
}

void QAVPlayer::pause()
{
    Q_D(QAVPlayer);
    qCDebug(lcAVPlayer) << __FUNCTION__;
    d->setWait(!d->setState(QAVPlayer::PausedState));
    d->setMediaStatus(QAVPlayer::PausingMedia);
    d->pendingPlay = false;
}

void QAVPlayer::stepForward()
{
    Q_D(QAVPlayer);
    qCDebug(lcAVPlayer) << __FUNCTION__;
    d->setState(QAVPlayer::PausedState);
    d->setMediaStatus(QAVPlayer::SteppingMedia);
    d->setWait(false);
    d->pendingPlay = false;
}

void QAVPlayer::stop()
{
    Q_D(QAVPlayer);
    qCDebug(lcAVPlayer) << __FUNCTION__;
    if (d->setState(QAVPlayer::StoppedState)) {
        if (hasVideo())
            d->dispatch([this] { emit videoFrame({}); });
    }
    d->setWait(true);
    d->pendingPlay = false;
}

bool QAVPlayer::isSeekable() const
{
    return d_func()->seekable;
}

void QAVPlayer::seek(qint64 pos)
{
    Q_D(QAVPlayer);
    if (pos < 0 || (duration() > 0 && pos > duration()))
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << "pos:" << pos;
    {
        QMutexLocker locker(&d->positionMutex);
        d->pendingPosition = pos / 1000.0;
        d->position = d->pendingPosition;
    }

    d->setMediaStatus(QAVPlayer::SeekingMedia);
    d->setWait(false);
}

qint64 QAVPlayer::duration() const
{
    return d_func()->duration * 1000;
}

qint64 QAVPlayer::position() const
{
    Q_D(const QAVPlayer);

    if (mediaStatus() == QAVPlayer::EndOfMedia)
        return duration();

    QMutexLocker locker(&d->positionMutex);
    if (d->pendingPosition >= 0)
        return d->pendingPosition * 1000;

    return d->position * 1000;
}

void QAVPlayer::setSpeed(qreal r)
{
    Q_D(QAVPlayer);

    {
        QMutexLocker locker(&d->speedMutex);
        if (qFuzzyCompare(d->speed, r))
            return;

        qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << d->speed << "->" << r;
        d->speed = r;
    }
    emit speedChanged(r);
}

qreal QAVPlayer::speed() const
{
    Q_D(const QAVPlayer);

    QMutexLocker locker(&d->speedMutex);
    return d->speed;
}

double QAVPlayer::videoFrameRate() const
{
    return d_func()->videoFrameRate;
}

QAVPlayer::Error QAVPlayer::error() const
{
    return d_func()->error;
}

QString QAVPlayer::errorString() const
{
    return d_func()->errorString;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, QAVPlayer::State state)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (state) {
        case QAVPlayer::StoppedState:
            return dbg << "StoppedState";
        case QAVPlayer::PlayingState:
            return dbg << "PlayingState";
        case QAVPlayer::PausedState:
            return dbg << "PausedState";
        default:
            return dbg << QString(QLatin1String("UserType(%1)" )).arg(int(state)).toLatin1().constData();
    }
}

QDebug operator<<(QDebug dbg, QAVPlayer::MediaStatus status)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (status) {
        case QAVPlayer::NoMedia:
            return dbg << "NoMedia";
        case QAVPlayer::LoadingMedia:
            return dbg << "LoadingMedia";
        case QAVPlayer::PausingMedia:
            return dbg << "PausingMedia";
        case QAVPlayer::SteppingMedia:
            return dbg << "SteppingMedia";
        case QAVPlayer::SeekingMedia:
            return dbg << "SeekingMedia";
        case QAVPlayer::LoadedMedia:
            return dbg << "LoadedMedia";
        case QAVPlayer::EndOfMedia:
            return dbg << "EndOfMedia";
        case QAVPlayer::InvalidMedia:
            return dbg << "InvalidMedia";
        default:
            return dbg << QString(QLatin1String("UserType(%1)" )).arg(int(status)).toLatin1().constData();
    }
}
#endif

QT_END_NAMESPACE
