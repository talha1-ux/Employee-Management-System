#include "FingerprintDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QRandomGenerator>

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor — Verify mode (original signature preserved for compatibility)
// ─────────────────────────────────────────────────────────────────────────────
FingerprintDialog::FingerprintDialog(int employeeId,
                                     const QString& employeeName,
                                     QWidget* parent)
    : QDialog(parent),
      m_employeeId(employeeId),
      m_employeeName(employeeName),
      m_mode(Mode::Verify)
{
    setWindowTitle("🔐 Fingerprint Biometric — Attendance Verification");
    setFixedSize(440, 580);
    setStyleSheet("background:#F8FAFC; font-family:'Segoe UI';");

    scanTimer = new QTimer(this);
    connect(scanTimer, &QTimer::timeout, this, &FingerprintDialog::onScanTick);

    buildUI();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor — Enroll mode (used when adding a new employee)
// ─────────────────────────────────────────────────────────────────────────────
FingerprintDialog::FingerprintDialog(int employeeId,
                                     const QString& employeeName,
                                     Mode mode,
                                     QWidget* parent)
    : QDialog(parent),
      m_employeeId(employeeId),
      m_employeeName(employeeName),
      m_mode(mode)
{
    if (m_mode == Mode::Enroll) {
        setWindowTitle("🖐 Fingerprint Enrollment — Register Biometric");
    } else {
        setWindowTitle("🔐 Fingerprint Biometric — Attendance Verification");
    }
    setFixedSize(440, 580);
    setStyleSheet("background:#F8FAFC; font-family:'Segoe UI';");

    scanTimer = new QTimer(this);
    connect(scanTimer, &QTimer::timeout, this, &FingerprintDialog::onScanTick);

    buildUI();
}

// ─────────────────────────────────────────────────────────────────────────────
void FingerprintDialog::buildUI() {
    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(28, 24, 28, 20);
    lay->setSpacing(14);

    // ── Mode badge ───────────────────────────────────────────────────────────
    QString modeColor  = (m_mode == Mode::Enroll) ? "#7C3AED" : "#1E3A5F";
    QString modeLabel  = (m_mode == Mode::Enroll) ? "ENROLLMENT MODE" : "VERIFICATION MODE";

    QLabel* modeBadge = new QLabel(modeLabel, this);
    modeBadge->setStyleSheet(
        QString("background:%1; color:#FFFFFF; font-size:10px; font-weight:700; "
                "letter-spacing:1.5px; padding:4px 12px; border-radius:12px;").arg(modeColor));
    modeBadge->setAlignment(Qt::AlignCenter);
    modeBadge->setFixedHeight(26);
    lay->addWidget(modeBadge);

    // ── Title ────────────────────────────────────────────────────────────────
    lblTitle = new QLabel(
        m_mode == Mode::Enroll ? "Fingerprint Enrollment" : "Fingerprint Verification", this);
    lblTitle->setStyleSheet(
        "font-size:20px; font-weight:800; color:#0F172A; font-family:'Georgia',serif;");
    lay->addWidget(lblTitle);

    lblEmployeeInfo = new QLabel(
        QString("Employee: <b>%1</b>  |  ID: <b>%2</b>")
        .arg(m_employeeName).arg(m_employeeId), this);
    lblEmployeeInfo->setStyleSheet("color:#64748B; font-size:13px;");
    lay->addWidget(lblEmployeeInfo);

    // ── Separator ────────────────────────────────────────────────────────────
    QFrame* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color:#E2E8F0;");
    lay->addWidget(sep);

    // ── Fingerprint scanner visual ────────────────────────────────────────────
    QWidget* scannerFrame = new QWidget(this);
    scannerFrame->setFixedHeight(160);
    scannerFrame->setStyleSheet(
        QString("background:%1; border-radius:14px;").arg(
            m_mode == Mode::Enroll ? "#1A0938" : "#0F172A"));
    QVBoxLayout* sfLay = new QVBoxLayout(scannerFrame);
    sfLay->setAlignment(Qt::AlignCenter);

    lblFingerIcon = new QLabel("🖐", scannerFrame);
    lblFingerIcon->setAlignment(Qt::AlignCenter);
    lblFingerIcon->setStyleSheet("font-size:60px;");
    sfLay->addWidget(lblFingerIcon);

    QString placeholderText = (m_mode == Mode::Enroll)
        ? "Place finger to enroll biometric"
        : "Place finger on scanner to verify";
    lblStatus = new QLabel(placeholderText, scannerFrame);
    lblStatus->setAlignment(Qt::AlignCenter);
    lblStatus->setStyleSheet(
        "color:#94A3B8; font-size:13px; font-family:'Segoe UI';");
    sfLay->addWidget(lblStatus);

    lay->addWidget(scannerFrame);

    // ── Progress bar ─────────────────────────────────────────────────────────
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setTextVisible(false);
    progressBar->setFixedHeight(8);
    progressBar->setStyleSheet(
        "QProgressBar { background:#E2E8F0; border-radius:4px; border:none; }"
        "QProgressBar::chunk { background:#3B82F6; border-radius:4px; }");
    lay->addWidget(progressBar);

    // ── Mark-type selector — only shown in Verify mode ───────────────────────
    cboMarkType = nullptr;
    if (m_mode == Mode::Verify) {
        QHBoxLayout* markRow = new QHBoxLayout();
        QLabel* lblMark = new QLabel("Mark as:", this);
        lblMark->setStyleSheet("font-size:12px; font-weight:600; color:#374151;");
        markRow->addWidget(lblMark);

        cboMarkType = new QComboBox(this);
        cboMarkType->addItems({"Present", "Absent", "On Leave"});
        cboMarkType->setStyleSheet(
            "QComboBox { padding:8px 12px; border:1.5px solid #E2E8F0; border-radius:8px;"
            "font-size:13px; background:#FFFFFF; }"
            "QComboBox::drop-down { border:none; }");
        cboMarkType->setFixedHeight(36);
        markRow->addWidget(cboMarkType, 1);
        lay->addLayout(markRow);

        connect(cboMarkType, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &FingerprintDialog::onMarkTypeChanged);
    } else {
        // In enroll mode show an info note instead
        QLabel* enrollNote = new QLabel(
            "ℹ  The captured template will be saved with the employee record.\n"
            "   Use the Fingerprint button on the main screen for attendance.", this);
        enrollNote->setStyleSheet(
            "color:#7C3AED; font-size:11px; background:#F3EEFF; padding:10px; "
            "border-radius:8px; border:1px solid #DDD6FE;");
        enrollNote->setWordWrap(true);
        lay->addWidget(enrollNote);
    }

    // ── Activity log ─────────────────────────────────────────────────────────
    logBox = new QTextEdit(this);
    logBox->setReadOnly(true);
    logBox->setFixedHeight(90);
    logBox->setStyleSheet(
        "QTextEdit { background:#1E293B; color:#94A3B8; font-family:'Consolas',monospace;"
        "font-size:11px; border-radius:8px; padding:10px; border:none; }");
    logBox->append(QString("[%1]  Scanner ready — %2")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(m_mode == Mode::Enroll ? "Enrollment session" : "Verification session"));
    logBox->append(QString("[%1]  Employee #%2 — %3")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(m_employeeId)
        .arg(m_employeeName));
    lay->addWidget(logBox);

    // ── Action buttons ───────────────────────────────────────────────────────
    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(10);

    btnCancel = new QPushButton("Cancel", this);
    btnCancel->setStyleSheet(
        "QPushButton { background:#FFFFFF; color:#334155; padding:11px 18px;"
        "font-weight:700; border-radius:6px; border:1px solid #CBD5E1; font-size:13px; }"
        "QPushButton:hover { background:#F1F5F9; }");

    QString scanLabel = (m_mode == Mode::Enroll) ? "🖐  Start Enrollment" : "🖐  Start Scan";
    btnScan = new QPushButton(scanLabel, this);
    QString scanBg = (m_mode == Mode::Enroll) ? "#7C3AED" : "#1E3A5F";
    btnScan->setStyleSheet(
        QString("QPushButton { background:%1; color:#FFFFFF; padding:11px 22px;"
                "font-weight:700; border-radius:6px; border:none; font-size:13px; }"
                "QPushButton:hover { opacity:0.85; }"
                "QPushButton:disabled { background:#E2E8F0; color:#94A3B8; }").arg(scanBg));

    btnRow->addWidget(btnCancel);
    btnRow->addStretch();
    btnRow->addWidget(btnScan);
    lay->addLayout(btnRow);

    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnScan,   &QPushButton::clicked, this, &FingerprintDialog::onStartScan);
}

// ─────────────────────────────────────────────────────────────────────────────
void FingerprintDialog::onMarkTypeChanged(int idx) {
    if (cboMarkType) m_markType = cboMarkType->itemText(idx);
}

// ─────────────────────────────────────────────────────────────────────────────
void FingerprintDialog::onStartScan() {
    btnScan->setEnabled(false);
    if (cboMarkType) cboMarkType->setEnabled(false);
    m_scanStep = 0;
    progressBar->setValue(0);
    lblFingerIcon->setText("🖐");

    QString scanningMsg = (m_mode == Mode::Enroll) ? "Capturing fingerprint..." : "Scanning...";
    lblStatus->setText(scanningMsg);
    lblStatus->setStyleSheet("color:#38BDF8; font-size:13px;");
    progressBar->setStyleSheet(
        "QProgressBar { background:#E2E8F0; border-radius:4px; border:none; }"
        "QProgressBar::chunk { background:#3B82F6; border-radius:4px; }");

    QString logMsg = (m_mode == Mode::Enroll)
        ? QString("[%1]  Enrollment initiated for Employee #%2")
              .arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(m_employeeId)
        : QString("[%1]  Scan initiated — Mark: %2")
              .arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(m_markType);
    logBox->append(logMsg);

    // Tick every 80ms × 25 ticks ≈ 2 seconds
    scanTimer->start(80);
}

// ─────────────────────────────────────────────────────────────────────────────
void FingerprintDialog::onScanTick() {
    m_scanStep++;
    int pct = (m_scanStep * 100) / 25;
    progressBar->setValue(qMin(pct, 100));

    static const char* icons[] = {"🖐","✋","👋","✋","🖐"};
    lblFingerIcon->setText(icons[m_scanStep % 5]);

    if (m_scanStep < 25) return;   // still scanning

    scanTimer->stop();

    if (m_mode == Mode::Enroll)
        simulateEnroll();
    else
        simulateScan();
}

// ─────────────────────────────────────────────────────────────────────────────
//  simulateEnroll()
//  ─────────────────────────────────────────────────────────────────────────
//  HARDWARE INTEGRATION: Replace this function body with your SDK's enroll call.
//
//  Example (ZKTeco ZKFP SDK):
//      int ret = ZKFP2_AcquireFingerprint(m_hDevice, m_fpBuffer, &fpLen,
//                                         m_templateBuffer, &tplLen);
//      if (ret == ZKFP_ERR_OK) {
//          m_capturedTemplate = QString(QByteArray((char*)m_templateBuffer, tplLen).toHex());
//          ...mark success...
//      }
//
//  Example (DigitalPersona):
//      DPFPSample sample = reader.GetSample(timeout);
//      DPFPFeatureSet features = processor.CreateFeatureSet(sample, DataPurpose.Enrollment);
//      enrollment.AddFeatures(features);
//      if (enrollment.TemplateStatus == Feedback.TemplateCreated) {
//          m_capturedTemplate = QString::fromLatin1(
//              enrollment.Template.Serialize().toBase64());
//          ...mark success...
//      }
//
//  The 95% probability below is only for offline demonstration.
// ─────────────────────────────────────────────────────────────────────────────
void FingerprintDialog::simulateEnroll() {
    bool success = (QRandomGenerator::global()->bounded(100) < 95);

    if (success) {
        // Generate a fake hex template (256 random bytes) for demo purposes.
        // In production, replace this with the real SDK template bytes.
        QByteArray fakeTemplate(256, 0);
        for (auto& b : fakeTemplate)
            b = static_cast<char>(QRandomGenerator::global()->bounded(256));
        m_capturedTemplate = fakeTemplate.toHex();

        lblFingerIcon->setText("✅");
        lblStatus->setText("Fingerprint Enrolled!");
        lblStatus->setStyleSheet("color:#10B981; font-size:14px; font-weight:700;");
        progressBar->setStyleSheet(
            "QProgressBar { background:#E2E8F0; border-radius:4px; border:none; }"
            "QProgressBar::chunk { background:#7C3AED; border-radius:4px; }");
        progressBar->setValue(100);

        logBox->append(QString("[%1]  ✅  Template captured (%2 bytes) — enrollment complete.")
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
            .arg(m_capturedTemplate.length() / 2));

        // Auto-close after 1.2 seconds
        QTimer::singleShot(1200, this, &QDialog::accept);

    } else {
        lblFingerIcon->setText("❌");
        lblStatus->setText("Enrollment Failed — Try Again");
        lblStatus->setStyleSheet("color:#EF4444; font-size:13px; font-weight:700;");
        progressBar->setStyleSheet(
            "QProgressBar { background:#E2E8F0; border-radius:4px; border:none; }"
            "QProgressBar::chunk { background:#EF4444; border-radius:4px; }");

        logBox->append(QString("[%1]  ❌  Capture failed. Please retry.")
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));

        btnScan->setEnabled(true);
        btnScan->setText("🖐  Retry Enrollment");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  simulateScan()
//  ─────────────────────────────────────────────────────────────────────────
//  HARDWARE INTEGRATION: Replace this function body with your SDK's verify call.
//  The 90% probability below is only for offline demonstration.
// ─────────────────────────────────────────────────────────────────────────────
void FingerprintDialog::simulateScan() {
    bool matched = (QRandomGenerator::global()->bounded(100) < 90);

    if (matched) {
        m_verified = true;
        lblFingerIcon->setText("✅");
        lblStatus->setText("Identity Verified!");
        lblStatus->setStyleSheet("color:#10B981; font-size:14px; font-weight:700;");
        progressBar->setStyleSheet(
            "QProgressBar { background:#E2E8F0; border-radius:4px; border:none; }"
            "QProgressBar::chunk { background:#10B981; border-radius:4px; }");
        progressBar->setValue(100);

        logBox->append(QString("[%1]  ✅  Match confirmed — attendance marked as: %2")
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
            .arg(m_markType));

        QTimer::singleShot(1000, this, &QDialog::accept);

    } else {
        m_verified = false;
        lblFingerIcon->setText("❌");
        lblStatus->setText("Scan Failed — Try Again");
        lblStatus->setStyleSheet("color:#EF4444; font-size:13px; font-weight:700;");
        progressBar->setStyleSheet(
            "QProgressBar { background:#E2E8F0; border-radius:4px; border:none; }"
            "QProgressBar::chunk { background:#EF4444; border-radius:4px; }");

        logBox->append(QString("[%1]  ❌  Match failed. Please retry.")
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));

        btnScan->setEnabled(true);
        if (cboMarkType) cboMarkType->setEnabled(true);
        btnScan->setText("🖐  Retry Scan");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void FingerprintDialog::resetScanState() {
    m_scanStep = 0;
    m_verified = false;
    m_capturedTemplate.clear();
    progressBar->setValue(0);
    lblFingerIcon->setText("🖐");
    QString placeholderText = (m_mode == Mode::Enroll)
        ? "Place finger to enroll biometric"
        : "Place finger on scanner to verify";
    lblStatus->setText(placeholderText);
    lblStatus->setStyleSheet("color:#94A3B8; font-size:13px;");
    btnScan->setEnabled(true);
    QString scanLabel = (m_mode == Mode::Enroll) ? "🖐  Start Enrollment" : "🖐  Start Scan";
    btnScan->setText(scanLabel);
    if (cboMarkType) cboMarkType->setEnabled(true);
}
