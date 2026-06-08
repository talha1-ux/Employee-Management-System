#ifndef FACEIDIALOG_H
#define FACEIDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTimer>
#include <QTextEdit>
#include <QImage>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include <QVideoFrame>
#include <QFutureWatcher>
#include <QWidget>
#include <QPainter>
#include <QMutex>
#include <QRect>
#include <QVector>
#include <atomic>

// ─────────────────────────────────────────────────────────────────────────────
//  SpinRing — animated 360° scanning ring overlay widget
// ─────────────────────────────────────────────────────────────────────────────
class SpinRing : public QWidget {
    Q_OBJECT
public:
    explicit SpinRing(QWidget* parent = nullptr);
    void startSpin();
    void stopSpin();
    void setSuccess(bool ok);   // freezes ring, turns green/red

protected:
    void paintEvent(QPaintEvent*) override;

private slots:
    void onTick();

private:
    QTimer* m_timer  = nullptr;
    int     m_angle  = 0;
    bool    m_active = false;
    int     m_state  = 0;   // 0=idle, 1=spinning, 2=success, 3=fail
};

// ─────────────────────────────────────────────────────────────────────────────
//  FaceIdDialog — Fast Qt6-only face capture with 360° scan ring
//
//  Speed improvements over v1:
//   • Face detection runs on a worker thread (QtConcurrent::run) — UI never
//     blocks; viewfinder stays at full 30fps regardless.
//   • Integral image (summed-area table) reduces sliding-window cost from
//     O(W×H×WS²) → O(W×H) — ~40× faster on a 160×120 skin map.
//   • Detection skips every 2nd frame; display skips nothing.
//   • FastTransformation for viewfinder scaling.
//   • skinMap buffer reused across frames (no heap alloc per frame).
// ─────────────────────────────────────────────────────────────────────────────
class FaceIdDialog : public QDialog {
    Q_OBJECT

public:
    enum class Mode { Enroll, Verify };

    explicit FaceIdDialog(int employeeId,
                          const QString& employeeName,
                          Mode mode = Mode::Enroll,
                          QWidget* parent = nullptr);

    explicit FaceIdDialog(int employeeId,
                          const QString& employeeName,
                          const QString& storedTemplate,
                          Mode mode = Mode::Verify,
                          QWidget* parent = nullptr);

    ~FaceIdDialog();

    QString enrolledTemplate() const { return m_capturedTemplate; }
    bool    wasVerified()      const { return m_verified; }

private slots:
    void onStartScan();
    void onVideoFrameChanged(const QVideoFrame& frame);
    void onDetectionFinished();
    void onScanTick();
    void onCaptureNow();

signals:
    void detectionDone(QRect faceRect);

private:
    void buildUI();
    void startCamera();
    void stopCamera();
    void processCapture(const QImage& img, const QRect& faceRect);
    void doEnroll(const QImage& faceImg);
    void doVerify(const QImage& faceImg);
    void appendLog(const QString& msg);
    void setStatusOK(const QString& msg);
    void setStatusFail(const QString& msg);
    void drawOverlay(QImage& display, const QRect& faceRect);

    // Face detection — called on worker thread
    static QRect detectFaceBg(QImage small, int W, int H);

    // Histogram
    static QVector<double> buildHistogram(const QImage& img);
    static double compareHistograms(const QVector<double>& a, const QVector<double>& b);

    // State
    int     m_employeeId;
    QString m_employeeName;
    QString m_storedTemplate;
    Mode    m_mode;
    bool    m_verified       = false;
    bool    m_scanning       = false;
    bool    m_captureNext    = false;
    int     m_scanStep       = 0;
    QString m_capturedTemplate;

    // Frame pipeline
    std::atomic<bool>  m_detectBusy { false };  // worker thread in flight?
    int                m_frameCount = 0;        // total frames received
    QImage             m_pendingFrame;          // latest full-res frame
    QRect              m_lastFaceRect;          // last result from worker
    QMutex             m_frameMutex;

    // Async detection watcher
    QFutureWatcher<QRect>* m_watcher = nullptr;

    // Camera
    QCamera*              m_camera  = nullptr;
    QMediaCaptureSession* m_session = nullptr;
    QVideoSink*           m_sink    = nullptr;

    // UI
    QLabel*       cameraView;
    SpinRing*     spinRing;
    QLabel*       lblTitle;
    QLabel*       lblEmployeeInfo;
    QLabel*       lblStatus;
    QProgressBar* progressBar;
    QPushButton*  btnScan;
    QPushButton*  btnCancel;
    QTextEdit*    logBox;

    QTimer* scanTimer    = nullptr;
    QTimer* captureTimer = nullptr;
};

#endif // FACEIDIALOG_H
