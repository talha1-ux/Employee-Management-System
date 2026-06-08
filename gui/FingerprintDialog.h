#ifndef FINGERPRINTDIALOG_H
#define FINGERPRINTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTimer>
#include <QComboBox>
#include <QTextEdit>

// ─────────────────────────────────────────────────────────────────────────────
//  FingerprintDialog
//
//  Two operating modes:
//    Mode::Enroll  — called during Add Employee: captures & stores a template.
//                    After accept(), call enrolledTemplate() to get the hex blob.
//    Mode::Verify  — called for attendance marking: matches against stored template.
//                    After accept(), call wasVerified() and markType().
//
//  Hardware integration:
//    Replace the bodies of simulateEnroll() and simulateVerify() with your SDK
//    calls (e.g. ZKTeco ZKFP SDK, DigitalPersona U.are.U, SecuGen Hamster).
//    The public API surface (Mode, enrolledTemplate, wasVerified, markType) stays
//    unchanged so the rest of the application does not need modification.
// ─────────────────────────────────────────────────────────────────────────────
class FingerprintDialog : public QDialog {
    Q_OBJECT

public:
    enum class Mode { Enroll, Verify };

    // Verify mode (attendance — original constructor signature preserved)
    explicit FingerprintDialog(int employeeId,
                               const QString& employeeName,
                               QWidget* parent = nullptr);

    // Enroll mode (called from Add Employee dialog)
    explicit FingerprintDialog(int employeeId,
                               const QString& employeeName,
                               Mode mode,
                               QWidget* parent = nullptr);

    // Verify mode results
    bool    wasVerified()      const { return m_verified; }
    QString markType()         const { return m_markType; }   // "Present"|"Absent"|"On Leave"

    // Enroll mode result — call after accept()
    QString enrolledTemplate() const { return m_capturedTemplate; }

private slots:
    void onStartScan();
    void onScanTick();
    void onMarkTypeChanged(int idx);

private:
    void buildUI();
    void resetScanState();

    // ── Hardware SDK integration points ──────────────────────────────────────
    //  Replace these two methods with real SDK calls in a hardware deployment.
    void simulateEnroll();   // captures template → m_capturedTemplate
    void simulateScan();     // verifies finger   → m_verified

    int     m_employeeId;
    QString m_employeeName;
    Mode    m_mode          = Mode::Verify;
    bool    m_verified      = false;
    int     m_scanStep      = 0;
    QString m_markType      = "Present";
    QString m_capturedTemplate;   // hex/base64 biometric blob stored on enroll

    // UI
    QLabel*       lblTitle;
    QLabel*       lblEmployeeInfo;
    QLabel*       lblFingerIcon;
    QLabel*       lblStatus;
    QProgressBar* progressBar;
    QPushButton*  btnScan;
    QPushButton*  btnCancel;
    QComboBox*    cboMarkType;
    QTextEdit*    logBox;
    QTimer*       scanTimer;
};

#endif // FINGERPRINTDIALOG_H
