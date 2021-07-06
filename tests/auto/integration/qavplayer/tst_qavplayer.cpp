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

    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::LoadedMedia, 10000);
}

void tst_QAVPlayer::playIncorrectSource()
{
    QAVPlayer p;
    QSignalSpy spy(&p, &QAVPlayer::stateChanged);

    p.play();

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::NoMedia, 10000);
    QVERIFY(!p.hasAudio());
    QVERIFY(!p.hasVideo());
    QCOMPARE(spy.count(), 0);

    p.setSource(QUrl(QLatin1String("unknown")));

    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::InvalidMedia, 10000);
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

    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::LoadedMedia, 10000);
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
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyMediaStatus.count(), 2);

    QTRY_VERIFY_WITH_TIMEOUT(p.position() != 0, 10000);

    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();

    p.play();

    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyMediaStatus.count(), 2); // EndOfMedia -> SeekingMedia -> LoadedMedia
    QCOMPARE(spyDuration.count(), 0);
    QCOMPARE(p.duration(), 999);
    QCOMPARE(p.error(), QAVPlayer::NoError);
    QVERIFY(p.errorString().isEmpty());

    QTRY_VERIFY_WITH_TIMEOUT(p.position() != 0, 10000);

    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());

    p.setSource({});

    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);

    p.play();

    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);

    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::LoadedMedia, 10000);
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

    QTRY_VERIFY_WITH_TIMEOUT(p.position() != 0, 10000);
    QCOMPARE(frame.format().sampleFormat(), QAVAudioFormat::Int32);

    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());
}

void tst_QAVPlayer::pauseAudio()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    p.play();

    QTest::qWait(50);
    p.pause();

    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::LoadedMedia, 10000);
    QCOMPARE(spyState.count(), 2); // Stopped -> Playing -> Paused
    QCOMPARE(spyMediaStatus.count(), 2); // NoMedia -> Loading -> Loaded

    p.play();

    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
}

void tst_QAVPlayer::stopAudio()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));
    p.play();

    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_VERIFY_WITH_TIMEOUT(p.position() != 0, 10000);

    p.stop();

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(p.position() != 0);
    QVERIFY(p.duration() != 0);

    p.play();

    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE_WITH_TIMEOUT(p.state(), QAVPlayer::StoppedState, 10000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
}

void tst_QAVPlayer::seekAudio()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    p.seek(500);
    QCOMPARE(p.position(), 500);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QVERIFY(p.position() >= 500);
    QTRY_COMPARE_WITH_TIMEOUT(p.state(), QAVPlayer::StoppedState, 10000);
    QCOMPARE(p.position(), p.duration());
    QCOMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);

    p.seek(100);
    QCOMPARE(p.mediaStatus(), QAVPlayer::SeekingMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), 100);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::LoadedMedia, 10000);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE_WITH_TIMEOUT(p.position(), p.duration(), 10000);

    p.seek(100000);
    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE_WITH_TIMEOUT(p.state(), QAVPlayer::StoppedState, 10000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.position(), p.duration());

    p.seek(200);
    p.play();
    QTRY_VERIFY_WITH_TIMEOUT(p.position() > 200, 10000);

    p.seek(100);
    QTRY_VERIFY_WITH_TIMEOUT(p.position() < 200, 10000);

    p.seek(p.duration());
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);
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
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);
}

void tst_QAVPlayer::playVideo()
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

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::LoadedMedia, 10000);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 2); // NoMedia -> Loading -> Loaded
    QCOMPARE(spyDuration.count(), 1);
    QCOMPARE(p.duration(), 15019);
    QCOMPARE(p.error(), QAVPlayer::NoError);
    QVERIFY(p.errorString().isEmpty());
    QVERIFY(p.hasAudio());
    QVERIFY(p.hasVideo());
    QVERIFY(p.isSeekable());
    QCOMPARE(p.position(), 0);

    p.play();

    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::LoadedMedia, 10000);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing

    QTRY_VERIFY_WITH_TIMEOUT(p.position() != 0, 10000);
}

void tst_QAVPlayer::pauseVideo()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::LoadedMedia, 10000);

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

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    p.seek(14500);
    p.play();

    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();

    p.play();

    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::LoadedMedia, 10000);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyMediaStatus.count(), 2); // EndOfMedia -> SeekingMedia -> LoadedMedia
    QCOMPARE(spyDuration.count(), 0);
    QCOMPARE(p.duration(), 15019);
    QCOMPARE(p.error(), QAVPlayer::NoError);
    QVERIFY(p.errorString().isEmpty());

    QTRY_VERIFY_WITH_TIMEOUT(p.position() < 5000 && p.position() > 1000, 10000);

    p.stop();

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(p.position() != p.duration());

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();

    p.setSource({});

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.duration(), 0);
    QCOMPARE(p.position(), 0);
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 1); // EndOfMedia -> NoMedia
    QCOMPARE(spyDuration.count(), 1);

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();

    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));
    p.play();
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadingMedia);
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::LoadedMedia, 10000);
    QCOMPARE(spyMediaStatus.count(), 2); // NoMedia -> Loading -> Loaded
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QTRY_COMPARE_WITH_TIMEOUT(spyDuration.count(), 1, 10000);

    p.seek(14500);
    QCOMPARE(p.mediaStatus(), QAVPlayer::SeekingMedia);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::LoadedMedia, 10000);
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY_WITH_TIMEOUT(p.position() != p.duration(), 10000);

    QTest::qWait(10);

    p.stop();
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

    p.seek(0);
    QCOMPARE(p.position(), 0);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::LoadedMedia, 10000);

    p.seek(-1);
    QCOMPARE(p.position(), 0);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

    p.seek(5000);
    p.play();

    QTRY_VERIFY_WITH_TIMEOUT(p.position() > 5000, 10000);

    p.seek(13000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::SeekingMedia);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::LoadedMedia, 10000);
    QVERIFY(p.position() > 13000);

    p.seek(2000);
    QCOMPARE(p.position(), 2000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::SeekingMedia);

    QTest::qWait(50);

    p.pause();
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::LoadedMedia, 10000);
    p.seek(14500);
    QCOMPARE(p.position(), 14500);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::SeekingMedia);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::LoadedMedia, 10000);

    p.seek(15000);
    QCOMPARE(p.position(), 15000);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::SeekingMedia);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::LoadedMedia, 10000);

    p.play();
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);
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
    QTRY_VERIFY_WITH_TIMEOUT(!frame.size().isEmpty(), 10000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
}

void tst_QAVPlayer::pauseSeekVideo()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));

    QAVVideoFrame frame;
    int count = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++count; });

    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));
    QTest::qWait(200);
    QCOMPARE(count, 0);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QVERIFY(frame.size().isEmpty());

    p.pause();
    QCOMPARE(count, 0);
    QTRY_VERIFY_WITH_TIMEOUT(!frame.size().isEmpty(), 10000);
    QCOMPARE(count, 1);

    QCOMPARE(p.state(), QAVPlayer::PausedState);
    frame = QAVVideoFrame();

    p.stop();
    QTest::qWait(100);
    QVERIFY(frame.size().isEmpty());

    p.pause();
    QTRY_VERIFY_WITH_TIMEOUT(!frame.size().isEmpty(), 10000);
    QTest::qWait(100);
    QCOMPARE(count, 2);
    QCOMPARE(p.state(), QAVPlayer::PausedState);

    frame = QAVVideoFrame();

    p.seek(1);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY_WITH_TIMEOUT(!frame.size().isEmpty(), 10000);
    QTest::qWait(100);
    QCOMPARE(count, 3);

    frame = QAVVideoFrame();

    p.stop();
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTest::qWait(100);
    QVERIFY(frame.size().isEmpty());
    QCOMPARE(count, 3);

    frame = QAVVideoFrame();

    p.seek(1);
    QTRY_VERIFY_WITH_TIMEOUT(!frame.size().isEmpty(), 10000);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTest::qWait(100);
    QCOMPARE(count, 4);

    frame = QAVVideoFrame();

    p.pause();
    QTRY_VERIFY_WITH_TIMEOUT(!frame.size().isEmpty(), 10000);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTest::qWait(100);
    QCOMPARE(count, 5);

    frame = QAVVideoFrame();

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_VERIFY_WITH_TIMEOUT(!frame.size().isEmpty(), 10000);

    frame = QAVVideoFrame();

    p.seek(1);
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::SeekingMedia);
    QVERIFY(frame.size().isEmpty());
    QTRY_VERIFY_WITH_TIMEOUT(!frame.size().isEmpty(), 10000);
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::LoadedMedia, 10000);

    count = 0;
    frame = QAVVideoFrame();

    p.stop();
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

    p.pause();
    QTRY_VERIFY_WITH_TIMEOUT(count > 0, 10000);

    count = 0;
    frame = QAVVideoFrame();

    p.stop();
    QCOMPARE(count, 0);
    QTest::qWait(100);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(count, 0);

    p.seek(1);
    QTRY_VERIFY_WITH_TIMEOUT(!frame.size().isEmpty(), 10000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTest::qWait(200);
    QCOMPARE(count, 1);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

    frame = QAVVideoFrame();

    p.pause();
    QVERIFY(frame.size().isEmpty());
    QTRY_VERIFY_WITH_TIMEOUT(!frame.size().isEmpty(), 10000);
    QTest::qWait(100);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QCOMPARE(count, 2);

    frame = QAVVideoFrame();
    p.seek(1);

    QTRY_VERIFY_WITH_TIMEOUT(!frame.size().isEmpty(), 10000);
    QTest::qWait(100);
    QCOMPARE(count, 3);
}

QTEST_MAIN(tst_QAVPlayer)
#include "tst_qavplayer.moc"
