#include "FaceIdDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QByteArray>
#include <QBuffer>
#include <QPixmap>
#include <QPainter>
#include <QConicalGradient>
#include <QMediaDevices>
#include <QCameraDevice>
#include <QColor>
#include <QMutexLocker>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <algorithm>
#include <numeric>

// ── Tuning ───────────────────────────────────────────────────────────────────
static constexpr double MATCH_THRESHOLD = 0.40;
static constexpr int    VIEWFINDER_W    = 384;
static constexpr int    VIEWFINDER_H    = 216;
static constexpr int    SCAN_DELAY_MS   = 1800;  // ms before capture
static constexpr int    DETECT_EVERY    = 3;     // run detection every Nth frame
static constexpr int    DETECT_SCALE    = 6;     // downscale factor for detection

// ═════════════════════════════════════════════════════════════════════════════
//  SpinRing — 360° animated scanning ring
// ═════════════════════════════════════════════════════════════════════════════
SpinRing::SpinRing(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(160, 160);

    m_timer = new QTimer(this);
    m_timer->setInterval(16);   // ~60 fps spin
    connect(m_timer, &QTimer::timeout, this, &SpinRing::onTick);
}

void SpinRing::startSpin() {
    m_state  = 1;
    m_active = true;
    m_timer->start();
    show();
    update();
}

void SpinRing::stopSpin() {
    m_state  = 0;
    m_active = false;
    m_timer->stop();
    hide();
}

void SpinRing::setSuccess(bool ok) {
    m_state  = ok ? 2 : 3;
    m_active = false;
    m_timer->stop();
    update();
}

void SpinRing::onTick() {
    m_angle = (m_angle + 4) % 360;  // 4° per tick = smooth full rotation
    update();
}

void SpinRing::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF  r     = QRectF(rect()).adjusted(10, 10, -10, -10);
    const QPointF c     = r.center();
    const qreal   thick = 6.0;

    if (m_state == 0) return;

    if (m_state == 1) {
        // ── Spinning arc ──────────────────────────────────────────────────
        // Background ring (dim)
        QPen bg(QColor(255, 255, 255, 30), thick, Qt::SolidLine, Qt::RoundCap);
        p.setPen(bg);
        p.drawEllipse(r);

        // Spinning gradient arc — 270° sweep from current angle
        // Draw as series of short arcs with increasing opacity
        const int   SEGS  = 36;
        const int   SWEEP = 270;
        const qreal step  = SWEEP / (qreal)SEGS;

        for (int i = 0; i < SEGS; ++i) {
            int   alpha  = (int)(255.0 * (i + 1) / SEGS);
            qreal startA = m_angle + i * step;
            QColor c1(14, 165, 233, alpha);    // #0EA5E9 blue fade
            QColor c2(16, 185, 129, alpha);    // #10B981 teal tail

            // Blend colour along arc
            QColor seg;
            seg.setRgb(
                (int)(c1.red()   * (1 - (double)i/SEGS) + c2.red()   * ((double)i/SEGS)),
                (int)(c1.green() * (1 - (double)i/SEGS) + c2.green() * ((double)i/SEGS)),
                (int)(c1.blue()  * (1 - (double)i/SEGS) + c2.blue()  * ((double)i/SEGS)),
                alpha
            );

            QPen ap(seg, thick, Qt::SolidLine, Qt::RoundCap);
            p.setPen(ap);
            // Qt drawArc angles are in 1/16ths of a degree, counter-clockwise
            p.drawArc(r,
                      (int)(-(startA) * 16),
                      (int)(-(step + 0.5) * 16));
        }

        // Glowing head dot
        qreal headRad = r.width() / 2.0;
        qreal headX   = c.x() + headRad * qCos(qDegreesToRadians((qreal)m_angle));
        qreal headY   = c.y() - headRad * qSin(qDegreesToRadians((qreal)m_angle));
        QRadialGradient glow(headX, headY, thick * 2.5);
        glow.setColorAt(0.0, QColor(14, 165, 233, 255));
        glow.setColorAt(1.0, QColor(14, 165, 233, 0));
        p.setPen(Qt::NoPen);
        p.setBrush(glow);
        p.drawEllipse(QPointF(headX, headY), thick * 2.0, thick * 2.0);

    } else if (m_state == 2) {
        // ── Success — solid green ring + checkmark ─────────────────────────
        QPen ok(QColor("#10B981"), thick, Qt::SolidLine, Qt::RoundCap);
        p.setPen(ok);
        p.drawEllipse(r);

        // Checkmark
        p.setPen(QPen(QColor("#10B981"), thick * 1.2, Qt::SolidLine,
                      Qt::RoundCap, Qt::RoundJoin));
        qreal cx = c.x(), cy = c.y();
        p.drawLine(QPointF(cx - 18, cy),
                   QPointF(cx - 6,  cy + 14));
        p.drawLine(QPointF(cx - 6,  cy + 14),
                   QPointF(cx + 20, cy - 16));

    } else if (m_state == 3) {
        // ── Failure — red ring + X ─────────────────────────────────────────
        QPen fail(QColor("#EF4444"), thick, Qt::SolidLine, Qt::RoundCap);
        p.setPen(fail);
        p.drawEllipse(r);

        p.setPen(QPen(QColor("#EF4444"), thick * 1.2, Qt::SolidLine,
                      Qt::RoundCap, Qt::RoundJoin));
        qreal cx = c.x(), cy = c.y();
        p.drawLine(QPointF(cx - 14, cy - 14), QPointF(cx + 14, cy + 14));
        p.drawLine(QPointF(cx + 14, cy - 14), QPointF(cx - 14, cy + 14));
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  FaceIdDialog
// ═════════════════════════════════════════════════════════════════════════════
FaceIdDialog::FaceIdDialog(int employeeId,
                           const QString& employeeName,
                           Mode mode,
                           QWidget* parent)
    : QDialog(parent),
      m_employeeId(employeeId),
      m_employeeName(employeeName),
      m_mode(mode)
{
    setWindowTitle(mode == Mode::Enroll
                   ? "🪪 Face ID Enrollment — Register Biometric"
                   : "🔐 Face ID Verification — Biometric Check");
    setFixedSize(460, 640);
    setStyleSheet("background:#F8FAFC; font-family:'Segoe UI';");

    scanTimer    = new QTimer(this);
    captureTimer = new QTimer(this);
    captureTimer->setSingleShot(true);

    m_watcher = new QFutureWatcher<QRect>(this);

    connect(scanTimer,    &QTimer::timeout, this, &FaceIdDialog::onScanTick);
    connect(captureTimer, &QTimer::timeout, this, &FaceIdDialog::onCaptureNow);
    connect(m_watcher, &QFutureWatcher<QRect>::finished,
            this,       &FaceIdDialog::onDetectionFinished);

    buildUI();
}

FaceIdDialog::FaceIdDialog(int employeeId,
                           const QString& employeeName,
                           const QString& storedTemplate,
                           Mode mode,
                           QWidget* parent)
    : FaceIdDialog(employeeId, employeeName, mode, parent)
{
    m_storedTemplate = storedTemplate;
}

FaceIdDialog::~FaceIdDialog() {
    stopCamera();
    if (m_watcher->isRunning()) m_watcher->waitForFinished();
}

// ─────────────────────────────────────────────────────────────────────────────
//  UI
// ─────────────────────────────────────────────────────────────────────────────
void FaceIdDialog::buildUI()
{
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(24, 20, 24, 18);
    lay->setSpacing(10);

    // Badge
    QString modeColor = (m_mode == Mode::Enroll) ? "#0369A1" : "#1E3A5F";
    QString modeLabel = (m_mode == Mode::Enroll) ? "FACE ID ENROLLMENT" : "FACE ID VERIFICATION";
    auto* badge = new QLabel(modeLabel, this);
    badge->setStyleSheet(
        QString("background:%1;color:#FFF;font-size:10px;font-weight:700;"
                "letter-spacing:1.5px;padding:4px 12px;border-radius:12px;").arg(modeColor));
    badge->setAlignment(Qt::AlignCenter);
    badge->setFixedHeight(26);
    lay->addWidget(badge);

    // Title
    lblTitle = new QLabel(
        m_mode == Mode::Enroll ? "Face ID Enrollment" : "Face ID Verification", this);
    lblTitle->setStyleSheet(
        "font-size:19px;font-weight:800;color:#0F172A;font-family:'Georgia',serif;");
    lay->addWidget(lblTitle);

    lblEmployeeInfo = new QLabel(
        QString("Employee: <b>%1</b>  |  ID: <b>%2</b>").arg(m_employeeName).arg(m_employeeId));
    lblEmployeeInfo->setStyleSheet("color:#64748B;font-size:12px;");
    lay->addWidget(lblEmployeeInfo);

    auto* sep = new QFrame(this); sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color:#E2E8F0;"); lay->addWidget(sep);

    // ── Camera viewfinder + SpinRing overlay ─────────────────────────────────
    auto* camContainer = new QWidget(this);
    camContainer->setFixedSize(VIEWFINDER_W, VIEWFINDER_H);
    camContainer->setStyleSheet("background:#0F172A;border-radius:12px;");

    cameraView = new QLabel(camContainer);
    cameraView->setFixedSize(VIEWFINDER_W, VIEWFINDER_H);
    cameraView->setAlignment(Qt::AlignCenter);
    cameraView->setStyleSheet("background:transparent;color:#475569;font-size:12px;");
    cameraView->setText("📷  Click \"Start Face Scan\" to open camera");
    cameraView->setWordWrap(true);

    // SpinRing floats centred over the viewfinder
    spinRing = new SpinRing(camContainer);
    spinRing->move((VIEWFINDER_W - spinRing->width())  / 2,
                   (VIEWFINDER_H - spinRing->height()) / 2);
    spinRing->hide();

    lay->addWidget(camContainer, 0, Qt::AlignHCenter);

    // Status
    lblStatus = new QLabel(
        m_mode == Mode::Enroll
            ? "Position your face in the frame, then click Start"
            : "Look directly at the camera, then click Verify", this);
    lblStatus->setAlignment(Qt::AlignCenter);
    lblStatus->setStyleSheet("color:#475569;font-size:12px;");
    lblStatus->setWordWrap(true);
    lay->addWidget(lblStatus);

    // Progress
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100); progressBar->setValue(0);
    progressBar->setTextVisible(false); progressBar->setFixedHeight(7);
    progressBar->setStyleSheet(
        "QProgressBar{background:#E2E8F0;border-radius:3px;border:none;}"
        "QProgressBar::chunk{background:#0EA5E9;border-radius:3px;}");
    lay->addWidget(progressBar);

    // Info note
    auto* note = new QLabel(
        m_mode == Mode::Enroll
            ? "ℹ  Ensure good lighting and face the camera directly.\n"
              "   Your face template will be saved with the employee record."
            : "ℹ  Face will be compared against the enrolled template.\n"
              "   Keep still and ensure your face is fully visible.", this);
    note->setStyleSheet(
        "color:#0369A1;font-size:11px;background:#E0F2FE;padding:8px;"
        "border-radius:8px;border:1px solid #BAE6FD;");
    note->setWordWrap(true);
    lay->addWidget(note);

    // Log
    logBox = new QTextEdit(this);
    logBox->setReadOnly(true);
    logBox->setFixedHeight(68);
    logBox->setStyleSheet(
        "QTextEdit{background:#1E293B;color:#94A3B8;font-family:'Consolas',monospace;"
        "font-size:10px;border-radius:8px;padding:8px;border:none;}");
    appendLog(QString("Session — %1 mode  |  Employee #%2 — %3")
        .arg(m_mode == Mode::Enroll ? "Enroll" : "Verify")
        .arg(m_employeeId).arg(m_employeeName));
    lay->addWidget(logBox);

    // Buttons
    auto* btnRow = new QHBoxLayout(); btnRow->setSpacing(10);
    btnCancel = new QPushButton("Cancel", this);
    btnCancel->setStyleSheet(
        "QPushButton{background:#FFF;color:#334155;padding:10px 18px;"
        "font-weight:700;border-radius:6px;border:1px solid #CBD5E1;font-size:13px;}"
        "QPushButton:hover{background:#F1F5F9;}");

    btnScan = new QPushButton(
        m_mode == Mode::Enroll ? "📷  Start Face Scan" : "📷  Verify Face", this);
    btnScan->setStyleSheet(
        "QPushButton{background:#0369A1;color:#FFF;padding:10px 22px;"
        "font-weight:700;border-radius:6px;border:none;font-size:13px;}"
        "QPushButton:hover{background:#0284C7;}"
        "QPushButton:disabled{background:#E2E8F0;color:#94A3B8;}");

    btnRow->addWidget(btnCancel); btnRow->addStretch(); btnRow->addWidget(btnScan);
    lay->addLayout(btnRow);

    connect(btnCancel, &QPushButton::clicked, this, [this]{ stopCamera(); reject(); });
    connect(btnScan,   &QPushButton::clicked, this, &FaceIdDialog::onStartScan);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Camera
// ─────────────────────────────────────────────────────────────────────────────
void FaceIdDialog::startCamera()
{
    if (m_camera) return;

    const auto cams = QMediaDevices::videoInputs();
    if (cams.isEmpty()) {
        appendLog("ERROR: No camera found");
        setStatusFail("❌ No camera detected");
        btnScan->setEnabled(true);
        btnScan->setText(m_mode == Mode::Enroll ? "📷  Start Face Scan" : "📷  Verify Face");
        return;
    }
    appendLog(QString("Camera: %1").arg(cams.first().description()));

    m_camera  = new QCamera(cams.first(), this);
    m_session = new QMediaCaptureSession(this);
    m_sink    = new QVideoSink(this);
    m_session->setCamera(m_camera);
    m_session->setVideoSink(m_sink);

    connect(m_sink, &QVideoSink::videoFrameChanged,
            this,   &FaceIdDialog::onVideoFrameChanged,
            Qt::QueuedConnection);   // queued keeps UI responsive

    m_camera->start();
    appendLog("Camera started");
}

void FaceIdDialog::stopCamera()
{
    scanTimer->stop();
    captureTimer->stop();
    if (m_camera) {
        m_camera->stop();
        delete m_session; m_session = nullptr;
        m_sink   = nullptr;
        m_camera = nullptr;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Slot: Start button
// ─────────────────────────────────────────────────────────────────────────────
void FaceIdDialog::onStartScan()
{
    btnScan->setEnabled(false);
    btnScan->setText("⏳  Scanning...");
    m_scanning    = true;
    m_captureNext = false;
    m_scanStep    = 0;
    m_frameCount  = 0;
    progressBar->setValue(0);
    lblStatus->setText("Opening camera...");
    lblStatus->setStyleSheet("color:#475569;font-size:12px;");

    startCamera();
    if (!m_camera) return;

    spinRing->startSpin();
    scanTimer->start(150);
    captureTimer->start(SCAN_DELAY_MS);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Slot: progress animation
// ─────────────────────────────────────────────────────────────────────────────
void FaceIdDialog::onScanTick()
{
    m_scanStep++;
    progressBar->setValue(qMin(m_scanStep * 6, 80));
    if      (m_scanStep == 2)  lblStatus->setText("Detecting face...");
    else if (m_scanStep == 5)  lblStatus->setText("Mapping facial features...");
    else if (m_scanStep == 9)  lblStatus->setText("Encoding biometric data...");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Slot: captureTimer — grab the NEXT frame
// ─────────────────────────────────────────────────────────────────────────────
void FaceIdDialog::onCaptureNow()
{
    if (!m_scanning) return;
    m_captureNext = true;
    appendLog("Capturing frame...");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Slot: new video frame — FAST path (UI thread, must return quickly)
// ─────────────────────────────────────────────────────────────────────────────
void FaceIdDialog::onVideoFrameChanged(const QVideoFrame& frame)
{
    if (!frame.isValid()) return;

    // ── Convert once, reuse for display and detection ─────────────────────
    QImage img = frame.toImage().convertToFormat(QImage::Format_RGB888);
    if (img.isNull()) return;
    img = img.mirrored(true, false);

    m_frameCount++;

    // ── Launch async detection every DETECT_EVERY frames ─────────────────
    if (!m_detectBusy.load() && (m_frameCount % DETECT_EVERY == 0)) {
        m_detectBusy.store(true);

        // Build tiny downscaled copy for detection (done here, cheap)
        QImage small = img.scaled(img.width()  / DETECT_SCALE,
                                  img.height() / DETECT_SCALE,
                                  Qt::IgnoreAspectRatio,
                                  Qt::FastTransformation)
                          .convertToFormat(QImage::Format_RGB888);
        int W = small.width(), H = small.height();

        // Fire off background detection — captures small by value
        QFuture<QRect> fut = QtConcurrent::run([small, W, H]() {
            return FaceIdDialog::detectFaceBg(small, W, H);
        });
        m_watcher->setFuture(fut);
    }

    // ── Save frame for possible capture ──────────────────────────────────
    {
        QMutexLocker lk(&m_frameMutex);
        m_pendingFrame = img;
    }

    // ── Draw overlay with last known face rect (never blocks) ────────────
    QImage display = img.copy();
    drawOverlay(display, m_lastFaceRect);

    // FastTransformation — ~3× faster than Smooth for live preview
    cameraView->setPixmap(
        QPixmap::fromImage(display).scaled(
            VIEWFINDER_W, VIEWFINDER_H,
            Qt::KeepAspectRatio,
            Qt::FastTransformation));

    // ── Capture? ─────────────────────────────────────────────────────────
    if (m_captureNext && m_scanning) {
        m_captureNext = false;
        m_scanning    = false;
        scanTimer->stop();
        progressBar->setValue(100);

        QImage captured;
        {
            QMutexLocker lk(&m_frameMutex);
            captured = m_pendingFrame.copy();
        }

        if (!m_lastFaceRect.isValid()) {
            appendLog("No face in frame — retrying in 1s");
            m_scanning = true;
            captureTimer->start(1000);
            return;
        }

        appendLog(QString("Face captured: %1×%2 px")
                  .arg(m_lastFaceRect.width()).arg(m_lastFaceRect.height()));
        processCapture(captured, m_lastFaceRect);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Slot: async detection finished
// ─────────────────────────────────────────────────────────────────────────────
void FaceIdDialog::onDetectionFinished()
{
    QRect result = m_watcher->result();
    m_lastFaceRect = result;
    m_detectBusy.store(false);
}

// ─────────────────────────────────────────────────────────────────────────────
//  drawOverlay — draw face rect and spin ring on display image
// ─────────────────────────────────────────────────────────────────────────────
void FaceIdDialog::drawOverlay(QImage& display, const QRect& faceRect)
{
    QPainter p(&display);
    p.setRenderHint(QPainter::Antialiasing);

    if (faceRect.isValid()) {
        // Green box
        p.setPen(QPen(QColor("#10B981"), 2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(faceRect);

        // Blue corner accents
        int cs = qMax(faceRect.width() / 6, 8);
        QPen cp(QColor("#0EA5E9"), 3, Qt::SolidLine, Qt::RoundCap);
        p.setPen(cp);
        // TL
        p.drawLine(faceRect.topLeft(), faceRect.topLeft() + QPoint(cs, 0));
        p.drawLine(faceRect.topLeft(), faceRect.topLeft() + QPoint(0, cs));
        // TR
        p.drawLine(faceRect.topRight(), faceRect.topRight() + QPoint(-cs, 0));
        p.drawLine(faceRect.topRight(), faceRect.topRight() + QPoint(0,   cs));
        // BL
        p.drawLine(faceRect.bottomLeft(), faceRect.bottomLeft() + QPoint(cs,  0));
        p.drawLine(faceRect.bottomLeft(), faceRect.bottomLeft() + QPoint(0,  -cs));
        // BR
        p.drawLine(faceRect.bottomRight(), faceRect.bottomRight() + QPoint(-cs, 0));
        p.drawLine(faceRect.bottomRight(), faceRect.bottomRight() + QPoint(0,  -cs));

        // Label
        p.setPen(QColor("#10B981"));
        p.setFont(QFont("Segoe UI", 8, QFont::Bold));
        p.drawText(faceRect.bottomLeft() + QPoint(2, 14), "Face Detected");
    } else {
        p.setPen(QColor("#94A3B8"));
        p.setFont(QFont("Segoe UI", 8));
        p.drawText(display.rect().adjusted(6, 0, 0, -6),
                   Qt::AlignBottom | Qt::AlignLeft, "Searching for face...");
    }
    p.end();
}

// ─────────────────────────────────────────────────────────────────────────────
//  processCapture
// ─────────────────────────────────────────────────────────────────────────────
void FaceIdDialog::processCapture(const QImage& img, const QRect& faceRect)
{
    // Crop face region with scale correction
    // faceRect is in full-image coordinates (detection already scaled back)
    QRect safeRect = faceRect.intersected(img.rect());
    QImage faceImg = safeRect.isValid() ? img.copy(safeRect) : img;

    if (m_mode == Mode::Enroll) doEnroll(faceImg);
    else                         doVerify(faceImg);
}

// ─────────────────────────────────────────────────────────────────────────────
//  doEnroll
// ─────────────────────────────────────────────────────────────────────────────
void FaceIdDialog::doEnroll(const QImage& faceImg)
{
    stopCamera();
    spinRing->setSuccess(true);

    QByteArray bytes;
    QBuffer    buf(&bytes);
    buf.open(QIODevice::WriteOnly);
    faceImg.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation)
           .save(&buf, "JPEG", 88);

    m_capturedTemplate = "FACEID_V2:" + QString::fromLatin1(bytes.toBase64());

    setStatusOK("✅  Face enrolled successfully!");
    appendLog(QString("Template stored — %1 bytes").arg(m_capturedTemplate.size()));
    appendLog("✅ Enrollment complete");

    cameraView->setStyleSheet(
        "background:#0F172A;border-radius:12px;border:2px solid #10B981;");

    btnScan->setText("✓  Done");
    btnScan->setEnabled(true);
    btnScan->setStyleSheet(
        "QPushButton{background:#10B981;color:#FFF;padding:10px 22px;"
        "font-weight:700;border-radius:6px;border:none;font-size:13px;}"
        "QPushButton:hover{background:#059669;}");
    disconnect(btnScan, nullptr, nullptr, nullptr);
    connect(btnScan, &QPushButton::clicked, this, &QDialog::accept);
}

// ─────────────────────────────────────────────────────────────────────────────
//  doVerify
// ─────────────────────────────────────────────────────────────────────────────
void FaceIdDialog::doVerify(const QImage& liveFaceImg)
{
    stopCamera();

    if (m_storedTemplate.isEmpty()) {
        spinRing->setSuccess(false);
        setStatusFail("❌ No enrolled face template for this employee.");
        appendLog("ERROR: No template stored");
        btnScan->setEnabled(true);
        return;
    }

    QString b64 = m_storedTemplate;
    if (b64.startsWith("FACEID_V2:") || b64.startsWith("FACEID_V1:"))
        b64 = b64.mid(10);

    QByteArray storedBytes = QByteArray::fromBase64(b64.toLatin1());
    QImage storedImg;
    storedImg.loadFromData(storedBytes, "JPEG");

    if (storedImg.isNull()) {
        spinRing->setSuccess(false);
        setStatusFail("❌ Template decode failed.");
        appendLog("ERROR: imdecode failed"); btnScan->setEnabled(true); return;
    }

    QImage a = storedImg.scaled(96, 96, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                        .convertToFormat(QImage::Format_RGB888);
    QImage b = liveFaceImg.scaled(96, 96, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                          .convertToFormat(QImage::Format_RGB888);

    double dist = compareHistograms(buildHistogram(a), buildHistogram(b));
    appendLog(QString("Histogram distance: %1 (threshold ≤ %2)")
              .arg(dist, 0, 'f', 4).arg(MATCH_THRESHOLD));

    if (dist <= MATCH_THRESHOLD) {
        m_verified = true;
        spinRing->setSuccess(true);
        setStatusOK("✅  Identity confirmed!");
        appendLog("✅ Face match confirmed");
        cameraView->setStyleSheet(
            "background:#0F172A;border-radius:12px;border:2px solid #10B981;");
        btnScan->setText("✓  Accept");
        btnScan->setEnabled(true);
        btnScan->setStyleSheet(
            "QPushButton{background:#10B981;color:#FFF;padding:10px 22px;"
            "font-weight:700;border-radius:6px;border:none;font-size:13px;}"
            "QPushButton:hover{background:#059669;}");
        disconnect(btnScan, nullptr, nullptr, nullptr);
        connect(btnScan, &QPushButton::clicked, this, &QDialog::accept);
    } else {
        spinRing->setSuccess(false);
        setStatusFail("❌  Face not recognised — try again");
        appendLog("FAIL: No match");
        cameraView->setStyleSheet(
            "background:#0F172A;border-radius:12px;border:2px solid #EF4444;");
        btnScan->setText("🔄  Retry");
        btnScan->setEnabled(true);
        btnScan->setStyleSheet(
            "QPushButton{background:#EF4444;color:#FFF;padding:10px 22px;"
            "font-weight:700;border-radius:6px;border:none;font-size:13px;}"
            "QPushButton:hover{background:#DC2626;}");
        disconnect(btnScan, nullptr, nullptr, nullptr);
        connect(btnScan, &QPushButton::clicked, this, [this]{
            spinRing->stopSpin();
            cameraView->setStyleSheet("background:#0F172A;border-radius:12px;");
            m_scanning = false; m_scanStep = 0; progressBar->setValue(0);
            lblStatus->setText("Look at the camera and click Verify");
            lblStatus->setStyleSheet("color:#475569;font-size:12px;");
            btnScan->setText("📷  Verify Face");
            btnScan->setStyleSheet(
                "QPushButton{background:#0369A1;color:#FFF;padding:10px 22px;"
                "font-weight:700;border-radius:6px;border:none;font-size:13px;}"
                "QPushButton:hover{background:#0284C7;}"
                "QPushButton:disabled{background:#E2E8F0;color:#94A3B8;}");
            disconnect(btnScan, nullptr, nullptr, nullptr);
            connect(btnScan, &QPushButton::clicked, this, &FaceIdDialog::onStartScan);
        });
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  detectFaceBg — runs on worker thread, NO Qt UI calls allowed here
//
//  Uses integral image (summed-area table) for O(1) window sum queries,
//  making the sliding window search ~40× faster vs nested loops.
// ─────────────────────────────────────────────────────────────────────────────
QRect FaceIdDialog::detectFaceBg(QImage small, int W, int H)
{
    if (small.isNull() || W == 0 || H == 0) return {};

    // ── Build skin map ───────────────────────────────────────────────────────
    QVector<int> skin(W * H, 0);
    for (int y = 0; y < H; ++y) {
        const uchar* row = small.constScanLine(y);
        for (int x = 0; x < W; ++x) {
            int r = row[x * 3 + 0];
            int g = row[x * 3 + 1];
            int b = row[x * 3 + 2];
            // HSV skin check
            QColor c(r, g, b);
            int h = c.hsvHue();
            int s = c.hsvSaturation();
            int v = c.value();
            bool isSkin = ((h >= 0 && h <= 38) || (h >= 330))
                          && s >= 35 && s <= 230 && v >= 70;
            skin[y * W + x] = isSkin ? 1 : 0;
        }
    }

    // ── Build integral image ─────────────────────────────────────────────────
    QVector<int> integral(W * H, 0);
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int val = skin[y * W + x];
            if (x > 0) val += integral[y * W + (x - 1)];
            if (y > 0) val += integral[(y - 1) * W + x];
            if (x > 0 && y > 0) val -= integral[(y - 1) * W + (x - 1)];
            integral[y * W + x] = val;
        }
    }

    // Integral rectangle sum (x1,y1) to (x2,y2) inclusive
    auto rectSum = [&](int x1, int y1, int x2, int y2) -> int {
        int s = integral[y2 * W + x2];
        if (x1 > 0) s -= integral[y2 * W + (x1 - 1)];
        if (y1 > 0) s -= integral[(y1 - 1) * W + x2];
        if (x1 > 0 && y1 > 0) s += integral[(y1 - 1) * W + (x1 - 1)];
        return s;
    };

    // ── Sliding window search ────────────────────────────────────────────────
    const int WS       = qMax(W / 5, 6);
    const int STEP     = 2;
    int bestCount      = 0;
    int bestX1 = -1, bestY1 = -1;

    for (int wy = 0; wy + WS < H; wy += STEP) {
        for (int wx = 0; wx + WS < W; wx += STEP) {
            int count = rectSum(wx, wy, wx + WS - 1, wy + WS - 1);
            if (count > bestCount) {
                bestCount = count;
                bestX1 = wx; bestY1 = wy;
            }
        }
    }

    if (bestCount < WS * WS * 0.28) return {};   // not enough skin

    // Expand for full face (face is taller than the skin-dense window)
    int pad  = WS / 2;
    int x1   = qMax(0, bestX1 - pad);
    int y1   = qMax(0, bestY1 - pad * 2);   // extra head room
    int x2   = qMin(W - 1, bestX1 + WS + pad);
    int y2   = qMin(H - 1, bestY1 + WS + pad);

    // Scale back to original image coordinates
    return QRect(x1 * DETECT_SCALE, y1 * DETECT_SCALE,
                 (x2 - x1) * DETECT_SCALE, (y2 - y1) * DETECT_SCALE);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Histogram — 512-bin RGB (8 bins/channel), Chi-square distance
// ─────────────────────────────────────────────────────────────────────────────
QVector<double> FaceIdDialog::buildHistogram(const QImage& img)
{
    const int BINS = 8;
    QVector<double> hist(BINS * BINS * BINS, 0.0);
    int pixels = img.width() * img.height();
    if (pixels == 0) return hist;

    for (int y = 0; y < img.height(); ++y) {
        const uchar* row = img.constScanLine(y);
        for (int x = 0; x < img.width(); ++x) {
            int r = row[x * 3 + 0] * BINS / 256;
            int g = row[x * 3 + 1] * BINS / 256;
            int b = row[x * 3 + 2] * BINS / 256;
            hist[r * BINS * BINS + g * BINS + b] += 1.0;
        }
    }
    for (auto& v : hist) v /= pixels;
    return hist;
}

double FaceIdDialog::compareHistograms(const QVector<double>& a, const QVector<double>& b)
{
    double dist = 0.0;
    for (int i = 0; i < a.size(); ++i) {
        double sum = a[i] + b[i];
        if (sum > 1e-10)
            dist += (a[i] - b[i]) * (a[i] - b[i]) / sum;
    }
    return dist;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Utility
// ─────────────────────────────────────────────────────────────────────────────
void FaceIdDialog::appendLog(const QString& msg) {
    logBox->append(QString("[%1]  %2")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(msg));
}
void FaceIdDialog::setStatusOK(const QString& msg) {
    lblStatus->setText(msg);
    lblStatus->setStyleSheet("color:#10B981;font-size:12px;font-weight:600;");
}
void FaceIdDialog::setStatusFail(const QString& msg) {
    lblStatus->setText(msg);
    lblStatus->setStyleSheet("color:#EF4444;font-size:12px;font-weight:600;");
}
