/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavplayer.h"

#include <QDebug>
#include <QtTest/QtTest>

QT_USE_NAMESPACE

class tst_QAVPlayer : public QObject
{
    Q_OBJECT
public slots:
    void initTestCase();

private slots:
    void construction();
    void sourceChanged();
    void speedChanged();
    void quitAudio();
    void playIncorrectSource();
    void playAudio();
    void playAudioOutput();
    void pauseAudio();
    void stopAudio();
    void seekAudio();
    void speedAudio();
    void playVideo();
    void pauseVideo();
    void seekVideo();
    void speedVideo();
    void videoFrame();
    void pauseSeekVideo();
    void stepForward();
};

void tst_QAVPlayer::initTestCase()
{
    QThreadPool::globalInstance()->setMaxThreadCount(20);
}

void tst_QAVPlayer::construction()
{
    QAVPlayer p;
    QVERIFY(p.source().isEmpty());
    QVERIFY(!p.hasAudio());
    QVERIFY(!p.hasVideo());
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.duration(), 0);
    QCOMPARE(p.position(), 0);
    QCOMPARE(p.speed(), 1.0);
    QVERIFY(!p.isSeekable());
    QCOMPARE(p.error(), QAVPlayer::NoError);
    QVERIFY(p.errorString().isEmpty());
}

void tst_QAVPlayer::sourceChanged()
{
    QAVPlayer p;
    QSignalSpy spy(&p, &QAVPlayer::sourceChanged);
    p.setSource(QUrl(QLatin1String("unknown.mp4")));
    QCOMPARE(spy.count(), 1);
    p.setSource(QUrl(QLatin1String("unknown.mp4")));
    QCOMPARE(spy.count(), 1);
}

void tst_QAVPlayer::speedChanged()
{
    QAVPlayer p;
    QSignalSpy spy(&p, &QAVPlayer::speedChanged);
    QCOMPARE(p.speed(), 1.0);
    p.setSpeed(0);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(p.speed(), 0.0);
    p.setSpeed(2);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(p.speed(), 2.0);
}

void tst_QAVPlayer::quitAudio()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
}

void tst_QAVPlayer::playIncorrectSource()
{
    QAVPlayer p;
    QSignalSpy spy(&p, &QAVPlayer::stateChanged);

    p.play();

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QVERIFY(!p.hasAudio());
    QVERIFY(!p.hasVideo());
    QCOMPARE(spy.count(), 0);

    p.setSource(QUrl(QLatin1String("unknown")));

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::InvalidMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.error(), QAVPlayer::ResourceError);
    QVERIFY(!p.errorString().isEmpty());

    p.play();

    QCOMPARE(p.mediaStatus(), QAVPlayer::InvalidMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.error(), QAVPlayer::ResourceError);
    QVERIFY(!p.errorString().isEmpty());
    QVERIFY(!p.hasAudio());
    QVERIFY(!p.hasVideo());
    QCOMPARE(spy.count(), 0);

    p.setSource(QUrl(QLatin1String("unknown")));
    p.play();
    QCOMPARE(p.mediaStatus(), QAVPlayer::InvalidMedia);
    QCOMPARE(spy.count(), 0);
}

void tst_QAVPlayer::playAudio()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);
    QSignalSpy spyDuration(&p, &QAVPlayer::durationChanged);

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QVERIFY(!p.hasAudio());
    QVERIFY(!p.hasVideo());
    QVERIFY(!p.isSeekable());
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 0);
    QCOMPARE(spyDuration.count(), 0);

    p.setSource({});

    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 2); // NoMedia -> Loading -> Loaded
    QCOMPARE(spyDuration.count(), 1);
    QCOMPARE(p.duration(), 999);
    QCOMPARE(p.position(), 0);
    QCOMPARE(p.error(), QAVPlayer::NoError);
    QVERIFY(p.errorString().isEmpty());
    QVERIFY(p.hasAudio());
    QVERIFY(!p.hasVideo());
    QVERIFY(p.isSeekable());

    p.play();

    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyMediaStatus.count(), 2);

    QTRY_VERIFY(p.position() != 0);

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();

    p.play();

    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyMediaStatus.count(), 2); // EndOfMedia -> SeekingMedia -> LoadedMedia
    QCOMPARE(spyDuration.count(), 0);
    QCOMPARE(p.duration(), 999);
    QCOMPARE(p.error(), QAVPlayer::NoError);
    QVERIFY(p.errorString().isEmpty());

    QTRY_VERIFY(p.position() != 0);

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());

    p.setSource({});

    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);

    p.play();

    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);

    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
}

void tst_QAVPlayer::playAudioOutput()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);
    QSignalSpy spyDuration(&p, &QAVPlayer::durationChanged);

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));
    QAVAudioFrame frame;
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &f) { frame = f; });
    p.play();

    QTRY_VERIFY(p.position() != 0);
    QCOMPARE(frame.format().sampleFormat(), QAVAudioFormat::Int32);

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());
}

void tst_QAVPlayer::pauseAudio()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);
    QSignalSpy spyPaused(&p, &QAVPlayer::paused);

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadingMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyPaused.count(), 0);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);

    QTest::qWait(50);
    QCOMPARE(spyPaused.count(), 0);

    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QCOMPARE(spyState.count(), 2); // Stopped -> Playing -> Paused
    QCOMPARE(spyMediaStatus.count(), 4); // NoMedia -> Loading -> Loaded -> Pausing -> Loaded
    QTRY_COMPARE(spyPaused.count(), 1);

    p.play();

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyPaused.count(), 1);
}

void tst_QAVPlayer::stopAudio()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));
    p.play();

    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_VERIFY(p.position() != 0);

    p.stop();

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(p.position() != 0);
    QVERIFY(p.duration() != 0);

    p.play();

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
}

void tst_QAVPlayer::seekAudio()
{
    QAVPlayer p;
    QSignalSpy spyPaused(&p, &QAVPlayer::paused);

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    p.seek(500);
    QCOMPARE(p.position(), 500);
    p.play();

    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QVERIFY(p.position() >= 500);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());
    QCOMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);

    p.seek(100);
    QVERIFY(p.mediaStatus() == QAVPlayer::SeekingMedia || p.mediaStatus() == QAVPlayer::LoadedMedia);
    QCOMPARE(p.position(), 100);

    p.play();

    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(p.position(), p.duration());
    QCOMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);

    p.seek(100000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    p.play();

    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.position(), p.duration());

    p.seek(200);
    p.play();
    QVERIFY(p.mediaStatus() == QAVPlayer::SeekingMedia || p.mediaStatus() == QAVPlayer::LoadedMedia);
    QTRY_VERIFY(p.position() > 200);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

    p.seek(100);
    QVERIFY(p.mediaStatus() == QAVPlayer::SeekingMedia || p.mediaStatus() == QAVPlayer::LoadedMedia);
    QTRY_VERIFY(p.position() < 200);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

    p.seek(p.duration());
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(spyPaused.count(), 0);
}

void tst_QAVPlayer::speedAudio()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    p.setSpeed(0.5);
    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QCOMPARE(p.speed(), 0.5);
    p.setSpeed(2.0);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
}

void tst_QAVPlayer::playVideo()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);
    QSignalSpy spyDuration(&p, &QAVPlayer::durationChanged);
    QSignalSpy spyPaused(&p, &QAVPlayer::paused);
    QSignalSpy spyVideoFrameRateChanged(&p, &QAVPlayer::videoFrameRateChanged);

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QVERIFY(!p.hasAudio());
    QVERIFY(!p.hasVideo());
    QVERIFY(!p.isSeekable());
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 0);
    QCOMPARE(spyDuration.count(), 0);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(spyVideoFrameRateChanged.count(), 0);
    QCOMPARE(p.videoFrameRate(), 0.0);

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 2); // NoMedia -> Loading -> Loaded
    QCOMPARE(spyDuration.count(), 1);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(spyVideoFrameRateChanged.count(), 1);
    QCOMPARE(p.videoFrameRate(), 0.04);
    QCOMPARE(p.duration(), 15019);
    QCOMPARE(p.error(), QAVPlayer::NoError);
    QVERIFY(p.errorString().isEmpty());
    QVERIFY(p.hasAudio());
    QVERIFY(p.hasVideo());
    QVERIFY(p.isSeekable());
    QCOMPARE(p.position(), 0);

    p.play();

    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(spyVideoFrameRateChanged.count(), 1);

    QTRY_VERIFY(p.position() != 0);
}

void tst_QAVPlayer::pauseVideo()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);

    QTest::qWait(50);
    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
}

void tst_QAVPlayer::seekVideo()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);
    QSignalSpy spyDuration(&p, &QAVPlayer::durationChanged);
    QSignalSpy spySeeked(&p, &QAVPlayer::seeked);
    QSignalSpy spyPaused(&p, &QAVPlayer::paused);

    qint64 seekPosition = -1;
    QObject::connect(&p, &QAVPlayer::seeked, &p, [&](qint64 pos) { seekPosition = pos; });

    qint64 pausePosition = -1;
    QObject::connect(&p, &QAVPlayer::paused, &p, [&](qint64 pos) { pausePosition = pos; });

    int framesCount = 0;
    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&frame, &framesCount](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    p.seek(14500);
    p.play();

    if (p.mediaStatus() != QAVPlayer::EndOfMedia)
        QTRY_VERIFY(frame == true);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QTRY_VERIFY(frame == false);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());
    QVERIFY(p.hasVideo());
    QVERIFY(p.hasAudio());
    QCOMPARE(spySeeked.count(), 1);
    QCOMPARE(spyPaused.count(), 0);
    QVERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 14500) < 500);
    seekPosition = -1;
    QCOMPARE(pausePosition, -1);

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();
    spySeeked.clear();

    p.play();

    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(frame == true);
    QVERIFY(framesCount > 0);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyMediaStatus.count(), 2); // EndOfMedia -> SeekingMedia -> LoadedMedia
    QCOMPARE(spyDuration.count(), 0);
    QCOMPARE(spySeeked.count(), 0);
    QCOMPARE(seekPosition, -1);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);
    QCOMPARE(p.duration(), 15019);
    QCOMPARE(p.error(), QAVPlayer::NoError);
    QVERIFY(p.errorString().isEmpty());

    QTRY_VERIFY(p.position() < 5000 && p.position() > 1000);

    spyMediaStatus.clear();
    framesCount = 0;

    p.stop();

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(frame == false);
    QCOMPARE(framesCount, 1);
    QVERIFY(p.position() != p.duration());
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 0);

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();
    spySeeked.clear();
    framesCount = 0;

    p.setSource({});

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.duration(), 0);
    QCOMPARE(p.position(), 0);
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 1); // EndOfMedia -> NoMedia
    QCOMPARE(spyDuration.count(), 1);
    QCOMPARE(spySeeked.count(), 0);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();
    spyState.clear();

    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));
    p.play();

    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(spyMediaStatus.count(), 2); // NoMedia -> Loading -> Loaded
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyDuration.count(), 1);
    QCOMPARE(spySeeked.count(), 0);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);
    QTRY_VERIFY(frame == true);
    QVERIFY(framesCount > 0);

    p.seek(14500);
    QVERIFY(p.mediaStatus() == QAVPlayer::SeekingMedia || p.mediaStatus() == QAVPlayer::LoadedMedia);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spySeeked.count(), 1);
    QVERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 14500) < 500);
    seekPosition = -1;
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_VERIFY(p.position() != p.duration());
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);

    framesCount = 0;
    QTest::qWait(10);
    p.stop();
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);
    QTRY_VERIFY(frame == false);
    QCOMPARE(framesCount, 1);
    spySeeked.clear();
    QCOMPARE(seekPosition, -1);

    p.seek(0);
    QVERIFY(p.mediaStatus() == QAVPlayer::SeekingMedia || p.mediaStatus() == QAVPlayer::LoadedMedia);
    QCOMPARE(p.position(), 0);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(seekPosition < 10);
    seekPosition = -1;
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);

    const int pos = p.position();
    QTest::qWait(50);
    QCOMPARE(p.position(), pos);
    p.seek(-1);
    QCOMPARE(p.position(), pos);
    QTest::qWait(50);
    QCOMPARE(p.position(), pos);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(spySeeked.count(), 1);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);

    spySeeked.clear();

    p.seek(5000);
    QVERIFY(p.mediaStatus() == QAVPlayer::SeekingMedia || p.mediaStatus() == QAVPlayer::LoadedMedia);
    p.play();

    QTRY_VERIFY_WITH_TIMEOUT(p.position() > 5000, 10000);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 5000) < 500);
    seekPosition = -1;

    p.seek(13000);
    QVERIFY(p.mediaStatus() == QAVPlayer::SeekingMedia || p.mediaStatus() == QAVPlayer::LoadedMedia);
    QTRY_VERIFY_WITH_TIMEOUT(p.position() > 13000, 10000);
    QTRY_VERIFY(p.mediaStatus() == QAVPlayer::LoadedMedia || p.mediaStatus() == QAVPlayer::EndOfMedia);
    QCOMPARE(spySeeked.count(), 2);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 13000) < 500);
    seekPosition = -1;

    p.seek(2000);
    QVERIFY(p.mediaStatus() == QAVPlayer::SeekingMedia || p.mediaStatus() == QAVPlayer::LoadedMedia);
    QCOMPARE(p.position(), 2000);
    QTRY_COMPARE(spySeeked.count(), 3);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 2000) < 500);
    QTRY_VERIFY(frame == true);
    seekPosition = -1;
    framesCount = 0;

    p.stop();
    QTest::qWait(50);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);
    QTRY_VERIFY(frame == false);
    QCOMPARE(framesCount, 1);
    spyPaused.clear();

    QTest::qWait(50);
    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_COMPARE(spyPaused.count(), 1);
    QTRY_COMPARE(pausePosition, p.position());
    pausePosition = -1;
    spyPaused.clear();
    QCOMPARE(p.state(), QAVPlayer::PausedState);

    p.pause();
    QTest::qWait(50);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);
    QCOMPARE(p.state(), QAVPlayer::PausedState);

    p.seek(14500);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 14500) < 500);
    seekPosition = -1;
    QCOMPARE(spyPaused.count(), 0);

    p.play();
    QVERIFY(p.mediaStatus() == QAVPlayer::SeekingMedia || p.mediaStatus() == QAVPlayer::LoadedMedia);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);
    QCOMPARE(spySeeked.count(), 4);
    QCOMPARE(seekPosition, -1);
    QCOMPARE(spyPaused.count(), 0);
    p.play();
}

void tst_QAVPlayer::speedVideo()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    p.setSpeed(5);
    p.play();

    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);

    p.setSpeed(0.5);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.speed(), 0.5);

    p.play();

    QTest::qWait(100);
    p.setSpeed(5);
    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);

    p.play();
}

void tst_QAVPlayer::videoFrame()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&frame](const QAVVideoFrame &f) { frame = f; });

    p.play();
    QTRY_VERIFY(!frame.size().isEmpty());
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

    p.stop();
    QTRY_VERIFY(frame.size().isEmpty());
}

void tst_QAVPlayer::pauseSeekVideo()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));

    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; if (f) ++framesCount; });
    qint64 seekPosition = -1;
    QObject::connect(&p, &QAVPlayer::seeked, &p, [&](qint64 pos) { seekPosition = pos; });
    qint64 pausePosition = -1;
    QObject::connect(&p, &QAVPlayer::paused, &p, [&](qint64 pos) { pausePosition = pos; });

    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));
    QTest::qWait(200);
    QCOMPARE(framesCount, 0);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QVERIFY(frame.size().isEmpty());
    QCOMPARE(pausePosition, -1);

    p.pause();
    QCOMPARE(framesCount, 0);
    QTRY_VERIFY(!frame.size().isEmpty());
    QVERIFY(framesCount > 0);
    QTRY_COMPARE(pausePosition, p.position());
    pausePosition = -1;

    QCOMPARE(p.state(), QAVPlayer::PausedState);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.stop();
    QTest::qWait(100);
    QVERIFY(frame.size().isEmpty());
    QCOMPARE(framesCount, 0);
    QCOMPARE(pausePosition, -1);

    p.pause();
    QTRY_VERIFY(!frame.size().isEmpty());
    QVERIFY(framesCount > 0);
    int count = framesCount;
    QTRY_COMPARE(pausePosition, p.position());
    pausePosition = -1;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 2);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QVERIFY(p.mediaStatus() == QAVPlayer::SeekingMedia || p.mediaStatus() == QAVPlayer::LoadedMedia);
    QTRY_VERIFY(!frame.size().isEmpty());
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(pausePosition, -1);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 2);
    QVERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 1) < 100);
    seekPosition = -1;

    frame = QAVVideoFrame();
    framesCount = 0;

    p.stop();
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTest::qWait(100);
    QVERIFY(frame.size().isEmpty());
    QCOMPARE(framesCount, 0);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1);
    QVERIFY(p.mediaStatus() == QAVPlayer::SeekingMedia || p.mediaStatus() == QAVPlayer::LoadedMedia);
    QTRY_VERIFY(!frame.size().isEmpty());
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(pausePosition, -1);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 2);
    QVERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 1) < 100);
    seekPosition = -1;
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.pause();
    QTRY_VERIFY(!frame.size().isEmpty());
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTRY_COMPARE(pausePosition, p.position());
    pausePosition = -1;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 2);
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_VERIFY(!frame.size().isEmpty());
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTest::qWait(100);
    QVERIFY(count != framesCount);
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1);
    QVERIFY(p.mediaStatus() == QAVPlayer::SeekingMedia || p.mediaStatus() == QAVPlayer::LoadedMedia);
    QVERIFY(frame.size().isEmpty());
    QTRY_VERIFY(!frame.size().isEmpty());
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTest::qWait(100);
    QVERIFY(count != framesCount);
    QVERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 1) < 100);
    seekPosition = -1;
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.stop();
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(framesCount, 0);
    QTest::qWait(100);
    QVERIFY(framesCount >= 0);
    count = framesCount;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 2);
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1);
    QVERIFY(p.mediaStatus() == QAVPlayer::SeekingMedia || p.mediaStatus() == QAVPlayer::LoadedMedia);
    QTRY_VERIFY(!frame.size().isEmpty());
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 2);
    QVERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 1) < 100);
    seekPosition = -1;
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.pause();
    QVERIFY(frame.size().isEmpty());
    QTRY_VERIFY(!frame.size().isEmpty());
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTRY_COMPARE(pausePosition, p.position());
    pausePosition = -1;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 2);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1);
    QCOMPARE(pausePosition, -1);
    QVERIFY(p.mediaStatus() == QAVPlayer::SeekingMedia || p.mediaStatus() == QAVPlayer::LoadedMedia);
    QTRY_VERIFY(!frame.size().isEmpty());
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 2);
    QVERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 1) < 100);
    seekPosition = -1;
    QCOMPARE(pausePosition, -1);
}

void tst_QAVPlayer::stepForward()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));

    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });
    qint64 pausePosition = -1;
    QObject::connect(&p, &QAVPlayer::paused, &p, [&](qint64 pos) { pausePosition = pos; });
    qint64 stepPosition = -1;
    QObject::connect(&p, &QAVPlayer::stepped, &p, [&](qint64 pos) { stepPosition = pos; });
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);

    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));
    QTest::qWait(200);
    QCOMPARE(framesCount, 0);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QVERIFY(frame == false);
    QCOMPARE(pausePosition, -1);

    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame == true);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QVERIFY(frame == false);
    QCOMPARE(framesCount, 0);
    QTest::qWait(50);

    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QVERIFY(frame == false);
    QCOMPARE(framesCount, 0);
    QCOMPARE(stepPosition, -1);

    spyMediaStatus.clear();
    qint64 prev = -1;

    p.stepForward();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame == true);
    QCOMPARE(framesCount, 1);
    QVERIFY(stepPosition > prev);
    QCOMPARE(spyMediaStatus.count(), 2); // Loaded -> Stepping -> Loaded

    spyMediaStatus.clear();
    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;

    p.stepForward();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame == true);
    QCOMPARE(framesCount, 1);
    QVERIFY(stepPosition > prev);
    QCOMPARE(spyMediaStatus.count(), 2); // Loaded -> Stepping -> Loaded

    spyMediaStatus.clear();
    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;

    p.stepForward();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame == true);
    QCOMPARE(framesCount, 1);
    QVERIFY(stepPosition > prev);
    QCOMPARE(spyMediaStatus.count(), 2); // Loaded -> Stepping -> Loaded

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTest::qWait(50);

    spyMediaStatus.clear();
    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    
    p.stepForward();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTest::qWait(50);
    QTRY_VERIFY(frame == true);
    QCOMPARE(framesCount, 1);
    QVERIFY(stepPosition > prev);
    QCOMPARE(spyMediaStatus.count(), 2); // Loaded -> Stepping -> Loaded

    spyMediaStatus.clear();
    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;

    p.stepForward();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame == true);
    QCOMPARE(framesCount, 1);
    QVERIFY(stepPosition > prev);
    QCOMPARE(spyMediaStatus.count(), 2); // Loaded -> Stepping -> Loaded

    spyMediaStatus.clear();
    frame = QAVVideoFrame();
    framesCount = 0;
    stepPosition = -1;

    p.stop();
    QVERIFY(frame == false);
    QCOMPARE(framesCount, 1);
    QCOMPARE(stepPosition, -1);
    QCOMPARE(spyMediaStatus.count(), 0);

    spyMediaStatus.clear();
    frame = QAVVideoFrame();
    framesCount = 0;
    stepPosition = -1;

    QTest::qWait(50);
    p.stop();
    QVERIFY(frame == false);
    QCOMPARE(framesCount, 0);
    QCOMPARE(stepPosition, -1);
    QCOMPARE(spyMediaStatus.count(), 0);

    spyMediaStatus.clear();
    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;

    p.stepForward();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame == true);
    QCOMPARE(framesCount, 1);
    QVERIFY(stepPosition > prev);
    QCOMPARE(spyMediaStatus.count(), 2); // Loaded -> Stepping -> Loaded
}

QTEST_MAIN(tst_QAVPlayer)
#include "tst_qavplayer.moc"
