#include "LoginWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QMessageBox>
#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QFont>
#include <QFrame>
#include <QSizePolicy>

LoginWindow::LoginWindow(QWidget* parent)
    : QWidget(parent), userDB("ems_users.txt")
{
    setWindowTitle("EMS — Secure Gateway");
    setFixedSize(960, 600);
    buildUI();
}

void LoginWindow::buildUI() {
    // ── Root split layout ──────────────────────────────────────────────
    QHBoxLayout* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── LEFT PANEL: Brand / Illustration ──────────────────────────────
    QWidget* leftPanel = new QWidget(this);
    leftPanel->setFixedWidth(460);
    leftPanel->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        " stop:0 #0F172A, stop:0.5 #1E3A5F, stop:1 #0F172A);"
    );

    QVBoxLayout* leftLay = new QVBoxLayout(leftPanel);
    leftLay->setContentsMargins(50, 55, 50, 55);
    leftLay->setSpacing(0);

    // Logo mark
    QLabel* logoMark = new QLabel("⬡", leftPanel);
    logoMark->setStyleSheet("color: #38BDF8; font-size: 48px; font-weight: 900;");
    leftLay->addWidget(logoMark);
    leftLay->addSpacing(24);

    QLabel* brandTitle = new QLabel("Employee\nManagement\nSystem", leftPanel);
    brandTitle->setStyleSheet(
        "color: #F8FAFC; font-size: 38px; font-weight: 800; "
        "font-family: 'Georgia', serif; line-height: 1.15;"
    );
    leftLay->addWidget(brandTitle);
    leftLay->addSpacing(20);

    QLabel* brandSub = new QLabel("Workforce intelligence. Payroll precision.\nAttendance clarity.", leftPanel);
    brandSub->setStyleSheet("color: #94A3B8; font-size: 14px; line-height: 1.6; font-family: 'Segoe UI';");
    brandSub->setWordWrap(true);
    leftLay->addWidget(brandSub);

    leftLay->addStretch();

    // Feature bullets
    QStringList features = {"🔐  Role-based access control", "📊  Real-time analytics", "🧾  Automated payroll reports"};
    for (const auto& f : features) {
        QLabel* lbl = new QLabel(f, leftPanel);
        lbl->setStyleSheet("color: #CBD5E1; font-size: 13px; padding: 6px 0; font-family: 'Segoe UI';");
        leftLay->addWidget(lbl);
    }

    leftLay->addSpacing(30);

    // Default credentials hint
    QFrame* credsHint = new QFrame(leftPanel);
    credsHint->setStyleSheet("background: rgba(56,189,248,0.10); border: 1px solid rgba(56,189,248,0.25); border-radius: 8px;");
    QVBoxLayout* chLay = new QVBoxLayout(credsHint);
    chLay->setContentsMargins(16, 12, 16, 12);
    QLabel* chTitle = new QLabel("Default Credentials", credsHint);
    chTitle->setStyleSheet("color: #38BDF8; font-size: 11px; font-weight: 700; font-family: 'Segoe UI';");
    chLay->addWidget(chTitle);
    QStringList creds = {"admin / admin123 (Admin)", "hr / hr123 (HR Manager)", "viewer / viewer123 (Viewer)"};
    for (const auto& c : creds) {
        QLabel* cl = new QLabel(c, credsHint);
        cl->setStyleSheet("color: #94A3B8; font-size: 11px; font-family: 'Consolas', monospace;");
        chLay->addWidget(cl);
    }
    leftLay->addWidget(credsHint);

    root->addWidget(leftPanel);

    // ── RIGHT PANEL: Login Form ────────────────────────────────────────
    QWidget* rightPanel = new QWidget(this);
    rightPanel->setStyleSheet("background-color: #F8FAFC;");
    QVBoxLayout* rightLay = new QVBoxLayout(rightPanel);
    rightLay->setAlignment(Qt::AlignCenter);
    rightLay->setContentsMargins(60, 40, 60, 40);

    // Card
    cardFrame = new QWidget(rightPanel);
    cardFrame->setFixedSize(360, 420);
    cardFrame->setStyleSheet(
        "background: #FFFFFF; border-radius: 16px;"
    );
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(40);
    shadow->setColor(QColor(15, 23, 42, 40));
    shadow->setOffset(0, 8);
    cardFrame->setGraphicsEffect(shadow);

    QVBoxLayout* cardLay = new QVBoxLayout(cardFrame);
    cardLay->setContentsMargins(36, 40, 36, 40);
    cardLay->setSpacing(0);

    // Header
    QLabel* welcomeBack = new QLabel("Welcome back", cardFrame);
    welcomeBack->setStyleSheet("font-size: 24px; font-weight: 800; color: #0F172A; font-family: 'Georgia', serif;");
    cardLay->addWidget(welcomeBack);
    cardLay->addSpacing(6);

    QLabel* subHead = new QLabel("Sign in to your workspace", cardFrame);
    subHead->setStyleSheet("font-size: 13px; color: #64748B; font-family: 'Segoe UI';");
    cardLay->addWidget(subHead);
    cardLay->addSpacing(30);

    // Username field
    QLabel* lblUname = new QLabel("Username", cardFrame);
    lblUname->setStyleSheet("font-size: 12px; font-weight: 600; color: #374151; font-family: 'Segoe UI';");
    cardLay->addWidget(lblUname);
    cardLay->addSpacing(6);

    fieldUser = new QLineEdit(cardFrame);
    fieldUser->setPlaceholderText("Enter your username");
    fieldUser->setStyleSheet(
        "QLineEdit { padding: 11px 14px; border: 1.5px solid #E2E8F0; border-radius: 8px; "
        "font-size: 14px; background: #F8FAFC; color: #1E293B; font-family: 'Segoe UI'; }"
        "QLineEdit:focus { border-color: #3B82F6; background: #FFFFFF; }"
    );
    fieldUser->setFixedHeight(44);
    cardLay->addWidget(fieldUser);
    cardLay->addSpacing(16);

    // Password field
    QLabel* lblPwd = new QLabel("Password", cardFrame);
    lblPwd->setStyleSheet("font-size: 12px; font-weight: 600; color: #374151; font-family: 'Segoe UI';");
    cardLay->addWidget(lblPwd);
    cardLay->addSpacing(6);

    fieldPass = new QLineEdit(cardFrame);
    fieldPass->setPlaceholderText("Enter your password");
    fieldPass->setEchoMode(QLineEdit::Password);
    fieldPass->setStyleSheet(
        "QLineEdit { padding: 11px 14px; border: 1.5px solid #E2E8F0; border-radius: 8px; "
        "font-size: 14px; background: #F8FAFC; color: #1E293B; font-family: 'Segoe UI'; }"
        "QLineEdit:focus { border-color: #3B82F6; background: #FFFFFF; }"
    );
    fieldPass->setFixedHeight(44);
    cardLay->addWidget(fieldPass);
    cardLay->addSpacing(8);

    // Error label
    lblError = new QLabel("", cardFrame);
    lblError->setStyleSheet("color: #EF4444; font-size: 12px; font-family: 'Segoe UI';");
    lblError->setAlignment(Qt::AlignCenter);
    lblError->setFixedHeight(20);
    cardLay->addWidget(lblError);
    cardLay->addSpacing(10);

    // Login button
    btnLogin = new QPushButton("Sign In →", cardFrame);
    btnLogin->setFixedHeight(46);
    btnLogin->setStyleSheet(
        "QPushButton { background: #1E3A5F; color: #FFFFFF; font-size: 15px; font-weight: 700; "
        "border-radius: 8px; border: none; font-family: 'Segoe UI'; }"
        "QPushButton:hover { background: #2D5186; }"
        "QPushButton:pressed { background: #163059; }"
    );
    cardLay->addWidget(btnLogin);

    rightLay->addWidget(cardFrame, 0, Qt::AlignCenter);

    // Version
    lblVersion = new QLabel("Employee Management System v2.0", rightPanel);
    lblVersion->setStyleSheet("color: #94A3B8; font-size: 11px; font-family: 'Segoe UI'; margin-top: 20px;");
    lblVersion->setAlignment(Qt::AlignCenter);
    rightLay->addWidget(lblVersion);

    root->addWidget(rightPanel);

    // Connections
    connect(btnLogin,  &QPushButton::clicked,  this, &LoginWindow::handleLoginAttempt);
    connect(fieldUser, &QLineEdit::returnPressed, this, &LoginWindow::onUsernameReturn);
    connect(fieldPass, &QLineEdit::returnPressed, this, &LoginWindow::handleLoginAttempt);
}

void LoginWindow::onUsernameReturn() {
    fieldPass->setFocus();
}

void LoginWindow::handleLoginAttempt() {
    QString user = fieldUser->text().trimmed();
    QString pass = fieldPass->text();

    if (user.isEmpty() || pass.isEmpty()) {
        lblError->setText("Please fill in all fields.");
        return;
    }

    // Reload from file every time so accounts added during this session
    // (e.g. newly enrolled employees) are picked up without restarting.
    userDB.loadFromFile();

    QString role = userDB.authenticate(user, pass);
    if (!role.isEmpty()) {
        lblError->clear();
        emit portalAccessGranted(role, user);
    } else {
        lblError->setText("Invalid username or password.");
        fieldPass->clear();
        fieldPass->setFocus();
    }
}
