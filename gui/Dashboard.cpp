#include "Dashboard.h"
#include "FingerprintDialog.h"
#include "FaceIdDialog.h"
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>
#include <QPrintPreviewDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QInputDialog>
#include <QDialog>
#include <QRandomGenerator>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QScrollArea>
#include <QDateTime>
#include <QApplication>
#include <QSplitter>

// ──────────────────────────────────────────────────────────────────────
//  Shared Style Constants
// ──────────────────────────────────────────────────────────────────────
static const QString NAV_BTN_NORMAL =
    "QPushButton { text-align:left; padding:11px 20px; background:transparent; "
    "color:#94A3B8; border:none; font-size:13px; font-weight:600; border-radius:6px; "
    "font-family:'Segoe UI'; }"
    "QPushButton:hover { background:#1E293B; color:#F8FAFC; }";

static const QString NAV_BTN_ACTIVE =
    "QPushButton { text-align:left; padding:11px 20px; background:#1E3A5F; "
    "color:#38BDF8; border:none; font-size:13px; font-weight:700; border-radius:6px; "
    "font-family:'Segoe UI'; }";

static const QString TABLE_STYLE =
    "QTableWidget { background:#FFFFFF; border:1px solid #E2E8F0; border-radius:10px; "
    "  gridline-color:#F1F5F9; selection-background-color:#EFF6FF; }"
    "QHeaderView::section { background:#F8FAFC; color:#475569; font-weight:700; "
    "  border:none; padding:12px; border-bottom:2px solid #E2E8F0; font-size:11px; "
    "  text-transform:uppercase; font-family:'Segoe UI'; }"
    "QTableWidget::item { padding:12px; border-bottom:1px solid #F1F5F9; "
    "  color:#334155; font-family:'Segoe UI'; }";

static QPushButton* makeActionBtn(const QString& text, const QString& bg,
                                  const QString& fg, const QString& border,
                                  QWidget* parent) {
    auto* b = new QPushButton(text, parent);
    b->setStyleSheet(
        QString("QPushButton { background:%1; color:%2; padding:11px 18px; "
                "font-weight:700; border-radius:6px; border:%3; font-size:13px; "
                "font-family:'Segoe UI'; }"
                "QPushButton:hover { opacity:0.85; }"
                "QPushButton:disabled { background:#E2E8F0; color:#94A3B8; border:none; }")
        .arg(bg).arg(fg).arg(border)
    );
    return b;
}

// ──────────────────────────────────────────────────────────────────────
Dashboard::Dashboard(const QString& role, const QString& username, QWidget* parent)
    : QWidget(parent),
      db("ems_datastore.txt"),
      userDB("ems_users.txt"),
      liveSessionRole(role),
      liveSessionUser(username)
{
    db.readDataFromFile();
    buildLayout();
    reloadViewGrid();
    computeMetrics();
    reloadUsersGrid();
    reloadDeptStats();
    onNavEmployees();
}

// ──────────────────────────────────────────────────────────────────────
void Dashboard::buildLayout() {
    setStyleSheet("background-color:#F8FAFC; font-family:'Segoe UI', sans-serif;");

    QHBoxLayout* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Sidebar
    navSidebar = new QWidget(this);
    navSidebar->setFixedWidth(260);
    buildSidebar();
    root->addWidget(navSidebar);

    // Page stack
    pageStack = new QStackedWidget(this);

    auto* empPage     = new QWidget(this);
    auto* reportsPage = new QWidget(this);
    auto* adminPage   = new QWidget(this);

    buildEmployeesPage(empPage);
    buildReportsPage(reportsPage);
    buildAdminPage(adminPage);

    pageStack->addWidget(empPage);       // index 0
    pageStack->addWidget(reportsPage);   // index 1
    pageStack->addWidget(adminPage);     // index 2

    root->addWidget(pageStack, 1);
}

// ──────────────────────────────────────────────────────────────────────
void Dashboard::buildSidebar() {
    navSidebar->setStyleSheet("background:#0F172A;");
    QVBoxLayout* lay = new QVBoxLayout(navSidebar);
    lay->setContentsMargins(20, 40, 20, 30);
    lay->setSpacing(4);

    // Brand
    QLabel* brand = new QLabel("⬡ EMS CORE", navSidebar);
    brand->setStyleSheet("color:#F8FAFC; font-size:18px; font-weight:800; "
                         "letter-spacing:1px; margin-bottom:30px; font-family:'Georgia',serif;");
    lay->addWidget(brand);

    QLabel* navHeader = new QLabel("NAVIGATION", navSidebar);
    navHeader->setStyleSheet("color:#475569; font-size:10px; font-weight:700; "
                             "letter-spacing:1.5px; padding:0 4px; margin-bottom:6px;");
    lay->addWidget(navHeader);

    navBtnEmployees = new QPushButton("👥  Employees", navSidebar);
    navBtnEmployees->setStyleSheet(NAV_BTN_NORMAL);
    lay->addWidget(navBtnEmployees);

    navBtnReports = new QPushButton("📊  Reports", navSidebar);
    navBtnReports->setStyleSheet(NAV_BTN_NORMAL);
    lay->addWidget(navBtnReports);

    // Admin panel only for admin role
    if (liveSessionRole == "admin") {
        navBtnAdmin = new QPushButton("⚙️  Admin Panel", navSidebar);
        navBtnAdmin->setStyleSheet(NAV_BTN_NORMAL);
        lay->addWidget(navBtnAdmin);
        connect(navBtnAdmin, &QPushButton::clicked, this, &Dashboard::onNavAdmin);
    } else {
        navBtnAdmin = nullptr;
    }

    lay->addStretch();

    // User badge
    QWidget* userBadge = new QWidget(navSidebar);
    userBadge->setStyleSheet("background:#1E293B; border-radius:8px;");
    QVBoxLayout* badgeLay = new QVBoxLayout(userBadge);
    badgeLay->setContentsMargins(14, 12, 14, 12);
    badgeLay->setSpacing(4);

    QLabel* lUname = new QLabel("🔐 " + liveSessionUser.toUpper(), userBadge);
    lUname->setStyleSheet("color:#F8FAFC; font-size:13px; font-weight:700;");
    badgeLay->addWidget(lUname);

    QString roleBadgeColor = liveSessionRole == "admin" ? "#F59E0B"
                           : liveSessionRole == "hr"    ? "#10B981" : "#38BDF8";
    QLabel* lRole = new QLabel(liveSessionRole.toUpper(), userBadge);
    lRole->setStyleSheet(QString("color:%1; font-size:10px; font-weight:700; "
                                 "letter-spacing:1px;").arg(roleBadgeColor));
    badgeLay->addWidget(lRole);
    lay->addWidget(userBadge);

    lay->addSpacing(10);

    navBtnLogout = new QPushButton("↩  Logout", navSidebar);
    navBtnLogout->setStyleSheet(
        "QPushButton { text-align:left; padding:10px 20px; background:transparent; "
        "color:#EF4444; border:none; font-size:13px; font-weight:600; border-radius:6px; }"
        "QPushButton:hover { background:#1E293B; }");
    lay->addWidget(navBtnLogout);

    connect(navBtnEmployees, &QPushButton::clicked, this, &Dashboard::onNavEmployees);
    connect(navBtnReports,   &QPushButton::clicked, this, &Dashboard::onNavReports);
    connect(navBtnLogout,    &QPushButton::clicked, this, &Dashboard::onNavLogout);
}

// ──────────────────────────────────────────────────────────────────────
void Dashboard::buildEmployeesPage(QWidget* page) {
    QVBoxLayout* lay = new QVBoxLayout(page);
    lay->setContentsMargins(36, 32, 36, 24);
    lay->setSpacing(22);

    // Page title
    QHBoxLayout* titleRow = new QHBoxLayout();
    QLabel* pageTitle = new QLabel("Employee Directory", page);
    pageTitle->setStyleSheet("font-size:24px; font-weight:800; color:#0F172A; font-family:'Georgia',serif;");
    titleRow->addWidget(pageTitle);
    titleRow->addStretch();
    QLabel* ts = new QLabel(QDateTime::currentDateTime().toString("ddd, MMM d yyyy"), page);
    ts->setStyleSheet("color:#94A3B8; font-size:13px;");
    titleRow->addWidget(ts);
    lay->addLayout(titleRow);

    // ── KPI Cards ──────────────────────────────────────────────────────
    QHBoxLayout* kpiRow = new QHBoxLayout();
    kpiRow->setSpacing(16);

    auto makeKPI = [&](const QString& label, const QString& accent, QLabel*& valLabel) {
        QWidget* card = new QWidget(page);
        card->setStyleSheet(QString("background:#FFFFFF; border-radius:12px; "
                                    "border-left:4px solid %1;").arg(accent));
        card->setFixedHeight(90);
        QVBoxLayout* cl = new QVBoxLayout(card);
        cl->setContentsMargins(18, 14, 18, 14);
        QLabel* lbl = new QLabel(label, card);
        lbl->setStyleSheet("color:#64748B; font-size:10px; font-weight:700; letter-spacing:1px;");
        cl->addWidget(lbl);
        valLabel = new QLabel("—", card);
        valLabel->setStyleSheet(QString("font-size:24px; font-weight:800; color:%1;").arg(accent));
        cl->addWidget(valLabel);
        return card;
    };

    if (liveSessionRole == "viewer") {
        kpiRow->addWidget(makeKPI("MY NAME",        "#3B82F6", metricStaff));
        kpiRow->addWidget(makeKPI("MY NET SALARY",  "#10B981", metricPayroll));
        kpiRow->addWidget(makeKPI("MY ATTENDANCE",  "#F59E0B", metricAttendance));
        kpiRow->addWidget(makeKPI("MY STATUS",      "#8B5CF6", metricActive));
    } else {
        kpiRow->addWidget(makeKPI("TOTAL EMPLOYEES", "#3B82F6", metricStaff));
        kpiRow->addWidget(makeKPI("MONTHLY PAYROLL", "#10B981", metricPayroll));
        kpiRow->addWidget(makeKPI("AVG ATTENDANCE",  "#F59E0B", metricAttendance));
        kpiRow->addWidget(makeKPI("ACTIVE STAFF",    "#8B5CF6", metricActive));
    }
    lay->addLayout(kpiRow);

    // ── Search / Filter Bar ────────────────────────────────────────────
    QHBoxLayout* searchRow = new QHBoxLayout();
    searchRow->setSpacing(10);

    barSearch = new QLineEdit(page);
    barSearch->setPlaceholderText("Search by name, department, ID, CNIC, email...");
    barSearch->setStyleSheet(
        "QLineEdit { padding:10px 16px; background:#FFFFFF; border:1.5px solid #E2E8F0; "
        "border-radius:8px; font-size:13px; }"
        "QLineEdit:focus { border-color:#3B82F6; }");
    barSearch->setMinimumWidth(260);

    // Search icon button
    auto* btnSearch = new QPushButton("🔍", page);
    btnSearch->setFixedSize(40, 40);
    btnSearch->setToolTip("Search");
    btnSearch->setStyleSheet(
        "QPushButton { background:#3B82F6; color:#FFFFFF; border-radius:8px; "
        "font-size:16px; border:none; }"
        "QPushButton:hover { background:#2563EB; }"
        "QPushButton:pressed { background:#1D4ED8; }");
    connect(btnSearch, &QPushButton::clicked, this, [this]{ applyFiltersAndRefresh(); });

    // Clear button (appears when there is text)
    auto* btnClear = new QPushButton("✕", page);
    btnClear->setFixedSize(34, 34);
    btnClear->setToolTip("Clear search");
    btnClear->setStyleSheet(
        "QPushButton { background:#F1F5F9; color:#64748B; border-radius:6px; "
        "font-size:12px; font-weight:700; border:1px solid #E2E8F0; }"
        "QPushButton:hover { background:#E2E8F0; color:#334155; }");
    btnClear->setVisible(false);
    connect(btnClear, &QPushButton::clicked, this, [this, btnClear]{
        barSearch->clear();
        btnClear->setVisible(false);
        applyFiltersAndRefresh();
    });
    connect(barSearch, &QLineEdit::textChanged, this, [this, btnClear](const QString& t){
        btnClear->setVisible(!t.isEmpty());
        applyFiltersAndRefresh();
    });

    searchRow->addWidget(barSearch, 3);
    searchRow->addWidget(btnClear);
    searchRow->addWidget(btnSearch);

    cboDeptFilter = new QComboBox(page);
    cboDeptFilter->addItem("All Departments");
    cboDeptFilter->setStyleSheet(
        "QComboBox { padding:9px 12px; background:#FFFFFF; border:1.5px solid #E2E8F0; "
        "border-radius:8px; font-size:13px; }"
        "QComboBox::drop-down { border:none; }"
    );
    searchRow->addWidget(cboDeptFilter, 2);

    cboStatusFilter = new QComboBox(page);
    cboStatusFilter->addItems({"All Status", "Active", "On Leave", "Resigned"});
    cboStatusFilter->setStyleSheet(cboDeptFilter->styleSheet());
    searchRow->addWidget(cboStatusFilter, 1);

    cboSortBy = new QComboBox(page);
    cboSortBy->addItems({"Sort: Default (ID)", "Sort: Name A-Z", "Sort: Salary High-Low",
                         "Sort: Attendance", "Sort: Department"});
    cboSortBy->setStyleSheet(cboDeptFilter->styleSheet());
    searchRow->addWidget(cboSortBy, 2);

    btnAdvFilter = makeActionBtn("⚙ Advanced Filter", "#FFFFFF", "#334155",
                                 "1px solid #CBD5E1", page);
    searchRow->addWidget(btnAdvFilter, 1);

    lay->addLayout(searchRow);

    // ── Data Table ─────────────────────────────────────────────────────
    mainGrid = new QTableWidget(page);
    mainGrid->setColumnCount(9);
    mainGrid->setHorizontalHeaderLabels(
        {"ID", "Full Name", "CNIC", "Department", "Designation", "Email",
         "Net Salary", "Status", "Attendance"});
    mainGrid->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainGrid->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainGrid->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainGrid->setFocusPolicy(Qt::StrongFocus);
    mainGrid->setAlternatingRowColors(true);
    mainGrid->setStyleSheet(TABLE_STYLE +
        "QTableWidget { alternate-background-color: #FAFBFC; }");
    lay->addWidget(mainGrid, 1);

    // ── Action Buttons ─────────────────────────────────────────────────
    QHBoxLayout* cmdRow = new QHBoxLayout();
    cmdRow->setSpacing(10);

    btnOnboard    = makeActionBtn("➕ Add Employee",     "#1E3A5F", "#FFFFFF", "none", page);
    btnModify     = makeActionBtn("✏️ Edit Profile",     "#FFFFFF", "#334155", "1px solid #CBD5E1", page);
    btnBiometrics = makeActionBtn("🗓️ Log Attendance",   "#FFFFFF", "#334155", "1px solid #CBD5E1", page);
    btnSlipDisplay= makeActionBtn("🧾 Pay Slip",         "#FFFFFF", "#334155", "1px solid #CBD5E1", page);
    btnPurge      = makeActionBtn("🗑️ Delete",           "#FEF2F2", "#EF4444", "1px solid #FCA5A5", page);
    btnExportCSV  = makeActionBtn("📥 Export CSV",       "#10B981", "#FFFFFF", "none", page);

    // NEW buttons
    btnFingerprintScan = makeActionBtn("🗳️ Fingerprint", "#7C3AED", "#FFFFFF", "none", page);
    btnPrintList       = makeActionBtn("🖨️ Print List",  "#FFFFFF", "#334155", "1px solid #CBD5E1", page);

    cmdRow->addWidget(btnOnboard);
    cmdRow->addWidget(btnModify);
    cmdRow->addWidget(btnBiometrics);
    cmdRow->addWidget(btnFingerprintScan);
    cmdRow->addWidget(btnSlipDisplay);
    cmdRow->addStretch();
    cmdRow->addWidget(btnPurge);
    cmdRow->addWidget(btnPrintList);
    cmdRow->addWidget(btnExportCSV);
    lay->addLayout(cmdRow);

    // Role-based guards
    bool canEdit = (liveSessionRole == "admin" || liveSessionRole == "hr");
    if (!canEdit) {
        btnOnboard->setEnabled(false);
        btnModify->setEnabled(false);
        btnPurge->setEnabled(false);
        btnBiometrics->setEnabled(false);
        btnFingerprintScan->setEnabled(false);
    }
    if (liveSessionRole != "admin") {
        btnPurge->setEnabled(false);
    }

    // Connections
    connect(btnOnboard,       &QPushButton::clicked, this, &Dashboard::executionAddWorkflow);
    connect(btnModify,        &QPushButton::clicked, this, &Dashboard::executionUpdateWorkflow);
    connect(btnPurge,         &QPushButton::clicked, this, &Dashboard::executionDeleteWorkflow);
    connect(btnBiometrics,    &QPushButton::clicked, this, &Dashboard::executionAttendanceWorkflow);
    connect(btnFingerprintScan,&QPushButton::clicked,this, &Dashboard::onFingerprintScan);
    connect(btnSlipDisplay,   &QPushButton::clicked, this, &Dashboard::executionPaySlipWorkflow);
    connect(btnExportCSV,     &QPushButton::clicked, this, &Dashboard::executionExportWorkflow);
    connect(btnPrintList,     &QPushButton::clicked, this, &Dashboard::onPrintEmployeeList);
    connect(btnAdvFilter,     &QPushButton::clicked, this, &Dashboard::executionAdvancedFilter);
    connect(cboSortBy, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Dashboard::executionSortChanged);
    connect(cboDeptFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){ applyFiltersAndRefresh(); });
    connect(cboStatusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){ applyFiltersAndRefresh(); });
}

// ──────────────────────────────────────────────────────────────────────
void Dashboard::buildReportsPage(QWidget* page) {
    QVBoxLayout* lay = new QVBoxLayout(page);
    lay->setContentsMargins(36, 32, 36, 24);
    lay->setSpacing(20);

    QLabel* title = new QLabel("Reports & Analytics", page);
    title->setStyleSheet("font-size:24px; font-weight:800; color:#0F172A; font-family:'Georgia',serif;");
    lay->addWidget(title);

    // Report generation buttons
    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(10);

    auto* btnFull  = makeActionBtn("📋 Full Summary",     "#1E3A5F", "#FFFFFF", "none", page);
    auto* btnDept  = makeActionBtn("🏢 By Department",    "#FFFFFF", "#334155", "1px solid #CBD5E1", page);
    auto* btnAtt   = makeActionBtn("🗓️ Attendance Report", "#FFFFFF", "#334155", "1px solid #CBD5E1", page);
    auto* btnPay   = makeActionBtn("💰 Payroll Report",   "#FFFFFF", "#334155", "1px solid #CBD5E1", page);
    auto* btnExp   = makeActionBtn("📥 Export Report",    "#10B981", "#FFFFFF", "none", page);
    btnPrintReport = makeActionBtn("🖨️ Print Report",     "#FFFFFF", "#334155", "1px solid #CBD5E1", page);
    btnExportPDF   = makeActionBtn("📄 Export PDF",       "#7C3AED", "#FFFFFF", "none", page);

    btnRow->addWidget(btnFull);
    btnRow->addWidget(btnDept);
    btnRow->addWidget(btnAtt);
    btnRow->addWidget(btnPay);
    btnRow->addStretch();
    btnRow->addWidget(btnPrintReport);
    btnRow->addWidget(btnExportPDF);
    btnRow->addWidget(btnExp);
    lay->addLayout(btnRow);

    // Split: text on left, dept table on right
    QHBoxLayout* splitRow = new QHBoxLayout();
    splitRow->setSpacing(16);

    // Report text box
    reportOutput = new QTextEdit(page);
    reportOutput->setReadOnly(true);
    reportOutput->setStyleSheet(
        "QTextEdit { background:#0F172A; color:#E2E8F0; font-family:'Consolas',monospace; "
        "font-size:12px; border-radius:10px; padding:18px; border:none; }");
    splitRow->addWidget(reportOutput, 2);

    // Dept stats table
    QWidget* rightBlock = new QWidget(page);
    QVBoxLayout* rbLay = new QVBoxLayout(rightBlock);
    rbLay->setContentsMargins(0,0,0,0);
    rbLay->setSpacing(8);

    QLabel* deptTitle = new QLabel("Department Statistics", rightBlock);
    deptTitle->setStyleSheet("font-size:14px; font-weight:700; color:#1E293B;");
    rbLay->addWidget(deptTitle);

    deptStatsGrid = new QTableWidget(rightBlock);
    deptStatsGrid->setColumnCount(4);
    deptStatsGrid->setHorizontalHeaderLabels({"Department", "Headcount", "Avg Salary", "Avg Attend."});
    deptStatsGrid->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    deptStatsGrid->setEditTriggers(QAbstractItemView::NoEditTriggers);
    deptStatsGrid->setSelectionBehavior(QAbstractItemView::SelectRows);
    deptStatsGrid->setStyleSheet(TABLE_STYLE);
    rbLay->addWidget(deptStatsGrid, 1);
    splitRow->addWidget(rightBlock, 1);

    lay->addLayout(splitRow, 1);

    connect(btnFull,        &QPushButton::clicked, this, &Dashboard::onGenerateFullReport);
    connect(btnDept,        &QPushButton::clicked, this, &Dashboard::onGenerateDeptReport);
    connect(btnAtt,         &QPushButton::clicked, this, &Dashboard::onGenerateAttendanceReport);
    connect(btnPay,         &QPushButton::clicked, this, &Dashboard::onGeneratePayrollReport);
    connect(btnExp,         &QPushButton::clicked, this, &Dashboard::onExportReport);
    connect(btnPrintReport, &QPushButton::clicked, this, &Dashboard::onPrintReport);
    connect(btnExportPDF,   &QPushButton::clicked, this, &Dashboard::onExportReportPDF);

    // Default report
    onGenerateFullReport();
}

// ──────────────────────────────────────────────────────────────────────
void Dashboard::buildAdminPage(QWidget* page) {
    QVBoxLayout* lay = new QVBoxLayout(page);
    lay->setContentsMargins(36, 32, 36, 24);
    lay->setSpacing(20);

    QLabel* title = new QLabel("Admin Panel — User Management", page);
    title->setStyleSheet("font-size:24px; font-weight:800; color:#0F172A; font-family:'Georgia',serif;");
    lay->addWidget(title);

    QLabel* desc = new QLabel("Manage system users, roles, and access permissions.", page);
    desc->setStyleSheet("color:#64748B; font-size:13px;");
    lay->addWidget(desc);

    // Action buttons
    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(10);
    btnAddUser    = makeActionBtn("➕ Add User",        "#1E3A5F", "#FFFFFF", "none", page);
    btnRemoveUser = makeActionBtn("🗑️ Remove User",     "#FEF2F2", "#EF4444", "1px solid #FCA5A5", page);
    btnChangePass = makeActionBtn("🔑 Change Password", "#FFFFFF", "#334155", "1px solid #CBD5E1", page);
    btnRow->addWidget(btnAddUser);
    btnRow->addWidget(btnRemoveUser);
    btnRow->addWidget(btnChangePass);
    btnRow->addStretch();
    lay->addLayout(btnRow);

    usersGrid = new QTableWidget(page);
    usersGrid->setColumnCount(5);
    usersGrid->setHorizontalHeaderLabels({"Username", "Full Name", "Role", "Email", "Access Rights"});
    usersGrid->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    usersGrid->setEditTriggers(QAbstractItemView::NoEditTriggers);
    usersGrid->setSelectionBehavior(QAbstractItemView::SelectRows);
    usersGrid->setStyleSheet(TABLE_STYLE);
    lay->addWidget(usersGrid, 1);

    connect(btnAddUser,    &QPushButton::clicked, this, &Dashboard::onAddUser);
    connect(btnRemoveUser, &QPushButton::clicked, this, &Dashboard::onRemoveUser);
    connect(btnChangePass, &QPushButton::clicked, this, &Dashboard::onChangePassword);
}

// ──────────────────────────────────────────────────────────────────────
//  Data Helpers
// ──────────────────────────────────────────────────────────────────────
void Dashboard::populateGrid(const std::vector<Employee>& list) {
    mainGrid->setRowCount(0);
    for (const auto& emp : list) {
        int r = mainGrid->rowCount();
        mainGrid->insertRow(r);

        auto setCell = [&](int col, const QString& val, Qt::AlignmentFlag align = Qt::AlignLeft) {
            auto* item = new QTableWidgetItem(val);
            item->setTextAlignment(align | Qt::AlignVCenter);
            mainGrid->setItem(r, col, item);
        };

        setCell(0, QString::number(emp.getId()), Qt::AlignCenter);
        setCell(1, emp.getName());
        setCell(2, emp.getCnic().isEmpty() ? "—" : emp.getCnic());
        setCell(3, emp.getDepartment());
        setCell(4, emp.getDesignation());
        setCell(5, emp.getEmail());
        setCell(6, QString("Rs. %1").arg(emp.calculateNetPay(), 0, 'f', 0), Qt::AlignRight);

        // Status with color
        auto* statusItem = new QTableWidgetItem(emp.getStatus());
        statusItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        if (emp.getStatus() == "Active")
            statusItem->setForeground(QColor("#10B981"));
        else if (emp.getStatus() == "On Leave")
            statusItem->setForeground(QColor("#F59E0B"));
        else
            statusItem->setForeground(QColor("#EF4444"));
        mainGrid->setItem(r, 7, statusItem);

        double att = emp.getAttendancePercent();
        auto* attItem = new QTableWidgetItem(QString("%1%").arg(att, 0, 'f', 1));
        attItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        if (att >= 90) attItem->setForeground(QColor("#10B981"));
        else if (att >= 75) attItem->setForeground(QColor("#F59E0B"));
        else attItem->setForeground(QColor("#EF4444"));
        mainGrid->setItem(r, 8, attItem);
    }
    computeMetrics();
}

void Dashboard::reloadViewGrid() {
    // Refresh dept filter combo
    QString curDept = cboDeptFilter->currentText();
    cboDeptFilter->clear();
    cboDeptFilter->addItem("All Departments");
    for (const auto& d : db.getUniqueDepartments())
        cboDeptFilter->addItem(d);
    int idx = cboDeptFilter->findText(curDept);
    if (idx >= 0) cboDeptFilter->setCurrentIndex(idx);

    applyFiltersAndRefresh();
}

void Dashboard::applyFiltersAndRefresh() {
    FilterCriteria fc;

    QString searchText = barSearch ? barSearch->text().trimmed() : "";
    if (!searchText.isEmpty()) fc.nameQuery = searchText;

    QString dept = cboDeptFilter ? cboDeptFilter->currentText() : "";
    if (dept != "All Departments") fc.department = dept;

    QString status = cboStatusFilter ? cboStatusFilter->currentText() : "";
    if (status != "All Status") fc.status = status;

    std::vector<Employee> filtered = db.searchDatabase(fc);

    // ── Viewer access ──────────────────────────────────────────────────────
    // A viewer logs in as emp_<id> (e.g. emp_5).
    // Extract the numeric ID from the username and show only that employee's
    // record — they cannot see any other employee's data.
    if (liveSessionRole == "viewer") {
        std::vector<Employee> ownRecord;

        // Username format is "emp_<id>" — extract the id part
        QString uname = liveSessionUser;
        bool idOk = false;
        int empId = -1;

        if (uname.startsWith("emp_", Qt::CaseInsensitive)) {
            empId = uname.mid(4).toInt(&idOk);  // "emp_5" → 5
        }

        if (idOk && empId >= 0) {
            // Direct lookup by ID — fast and exact, no name-matching
            Employee* emp = db.lookupRecord(empId);
            if (emp) ownRecord.push_back(*emp);
        } else {
            // Fallback: username doesn't follow emp_<id> pattern
            // Try matching by the user's full name stored in UserDatabase
            User* u = userDB.findUser(liveSessionUser);
            QString fullName = u ? u->getFullName().toLower().trimmed()
                                 : liveSessionUser.toLower().trimmed();
            for (const auto& emp : db.pullAllRecords()) {
                if (emp.getName().toLower().trimmed() == fullName) {
                    ownRecord.push_back(emp);
                    break;
                }
            }
        }

        populateGrid(ownRecord);
        return;
    }

    // Apply sort
    int sortIdx = cboSortBy ? cboSortBy->currentIndex() : 0;
    if (sortIdx == 1)
        std::sort(filtered.begin(), filtered.end(),
            [](const Employee& a, const Employee& b){ return a.getName().toLower() < b.getName().toLower(); });
    else if (sortIdx == 2)
        std::sort(filtered.begin(), filtered.end(),
            [](const Employee& a, const Employee& b){ return a.calculateNetPay() > b.calculateNetPay(); });
    else if (sortIdx == 3)
        std::sort(filtered.begin(), filtered.end(),
            [](const Employee& a, const Employee& b){ return a.getAttendancePercent() > b.getAttendancePercent(); });
    else if (sortIdx == 4)
        std::sort(filtered.begin(), filtered.end(),
            [](const Employee& a, const Employee& b){ return a.getDepartment().toLower() < b.getDepartment().toLower(); });
    else
        std::sort(filtered.begin(), filtered.end(),
            [](const Employee& a, const Employee& b){ return a.getId() < b.getId(); });

    populateGrid(filtered);
}

void Dashboard::computeMetrics() {
    // ── Viewer: show only their own employee's data ───────────────────────
    if (liveSessionRole == "viewer") {
        bool idOk = false;
        int empId = -1;
        if (liveSessionUser.startsWith("emp_", Qt::CaseInsensitive))
            empId = liveSessionUser.mid(4).toInt(&idOk);

        Employee* emp = (idOk && empId >= 0) ? db.lookupRecord(empId) : nullptr;

        if (emp) {
            metricStaff->setText(emp->getName());
            metricPayroll->setText(QString("Rs.%1").arg(emp->calculateNetPay(), 0, 'f', 0));
            metricAttendance->setText(QString("%1%").arg(emp->getAttendancePercent(), 0, 'f', 1));
            metricActive->setText(emp->getStatus());
        } else {
            metricStaff->setText("—");
            metricPayroll->setText("—");
            metricAttendance->setText("—");
            metricActive->setText("—");
        }
        return;
    }

    // ── Admin / HR: show company-wide totals ──────────────────────────────
    auto& all = db.pullAllRecords();
    metricStaff->setText(QString::number(all.size()));
    metricPayroll->setText(QString("Rs.%1").arg(db.getTotalPayroll(), 0, 'f', 0));
    metricAttendance->setText(QString("%1%").arg(db.getAverageAttendance(), 0, 'f', 1));
    metricActive->setText(QString::number(db.getActiveCount()));
}

void Dashboard::reloadUsersGrid() {
    if (!usersGrid) return;
    usersGrid->setRowCount(0);
    for (const auto& u : userDB.getAllUsers()) {
        int r = usersGrid->rowCount();
        usersGrid->insertRow(r);
        usersGrid->setItem(r, 0, new QTableWidgetItem(u->getUsername()));
        usersGrid->setItem(r, 1, new QTableWidgetItem(u->getFullName()));
        auto* roleItem = new QTableWidgetItem(u->getActiveRole().toUpper());
        if (u->getActiveRole() == "admin")
            roleItem->setForeground(QColor("#F59E0B"));
        else if (u->getActiveRole() == "hr")
            roleItem->setForeground(QColor("#10B981"));
        else
            roleItem->setForeground(QColor("#3B82F6"));
        usersGrid->setItem(r, 2, roleItem);
        usersGrid->setItem(r, 3, new QTableWidgetItem(u->getEmail()));
        usersGrid->setItem(r, 4, new QTableWidgetItem(u->getSystemAccessRights()));
    }
}

void Dashboard::reloadDeptStats() {
    if (!deptStatsGrid) return;
    deptStatsGrid->setRowCount(0);
    for (const auto& ds : db.getDepartmentBreakdown()) {
        int r = deptStatsGrid->rowCount();
        deptStatsGrid->insertRow(r);
        deptStatsGrid->setItem(r, 0, new QTableWidgetItem(ds.dept));
        deptStatsGrid->setItem(r, 1, new QTableWidgetItem(QString::number(ds.headCount)));
        deptStatsGrid->setItem(r, 2, new QTableWidgetItem(
            QString("Rs. %1").arg(ds.avgSalary, 0, 'f', 0)));
        auto* attItem = new QTableWidgetItem(QString("%1%").arg(ds.avgAttendance, 0, 'f', 1));
        if (ds.avgAttendance >= 90) attItem->setForeground(QColor("#10B981"));
        else if (ds.avgAttendance >= 75) attItem->setForeground(QColor("#F59E0B"));
        else attItem->setForeground(QColor("#EF4444"));
        deptStatsGrid->setItem(r, 3, attItem);
    }
}

// ──────────────────────────────────────────────────────────────────────
//  Navigation
// ──────────────────────────────────────────────────────────────────────
void Dashboard::onNavEmployees() {
    pageStack->setCurrentIndex(0);
    navBtnEmployees->setStyleSheet(NAV_BTN_ACTIVE);
    navBtnReports->setStyleSheet(NAV_BTN_NORMAL);
    if (navBtnAdmin) navBtnAdmin->setStyleSheet(NAV_BTN_NORMAL);
    reloadViewGrid();
}

void Dashboard::onNavReports() {
    pageStack->setCurrentIndex(1);
    navBtnEmployees->setStyleSheet(NAV_BTN_NORMAL);
    navBtnReports->setStyleSheet(NAV_BTN_ACTIVE);
    if (navBtnAdmin) navBtnAdmin->setStyleSheet(NAV_BTN_NORMAL);
    reloadDeptStats();
    onGenerateFullReport();
}

void Dashboard::onNavAdmin() {
    pageStack->setCurrentIndex(2);
    navBtnEmployees->setStyleSheet(NAV_BTN_NORMAL);
    navBtnReports->setStyleSheet(NAV_BTN_NORMAL);
    if (navBtnAdmin) navBtnAdmin->setStyleSheet(NAV_BTN_ACTIVE);
    reloadUsersGrid();
}

void Dashboard::onNavLogout() {
    if (QMessageBox::question(this, "Logout", "Are you sure you want to logout?",
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        close();
    }
}

// ──────────────────────────────────────────────────────────────────────
//  Employee CRUD
// ──────────────────────────────────────────────────────────────────────
void Dashboard::executionAddWorkflow() {
    QDialog dlg(this);
    dlg.setWindowTitle("Add New Employee");
    dlg.setMinimumSize(560, 680);
    dlg.resize(560, 720);
    dlg.setStyleSheet("background:#F8FAFC; font-family:'Segoe UI';");

    // Outer layout holds scroll area + button row
    QVBoxLayout* outerLay = new QVBoxLayout(&dlg);
    outerLay->setContentsMargins(0, 0, 0, 0);
    outerLay->setSpacing(0);

    // ── Scroll area so content doesn't clip on small screens ────────────────
    QScrollArea* scroll = new QScrollArea(&dlg);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background:#F8FAFC; border:none; }");

    QWidget* inner = new QWidget(scroll);
    QVBoxLayout* lay = new QVBoxLayout(inner);
    lay->setContentsMargins(30, 28, 30, 16);
    lay->setSpacing(12);

    QLabel* dlgTitle = new QLabel("Onboard New Employee", inner);
    dlgTitle->setStyleSheet(
        "font-size:20px; font-weight:800; color:#0F172A; font-family:'Georgia',serif;");
    lay->addWidget(dlgTitle);

    QLabel* dlgSubtitle = new QLabel("Fields marked * are required", inner);
    dlgSubtitle->setStyleSheet("color:#94A3B8; font-size:12px;");
    lay->addWidget(dlgSubtitle);

    // ── Field factory ────────────────────────────────────────────────────────
    auto makeField = [&](QWidget* parent, const QString& label,
                         const QString& placeholder = "") -> QLineEdit* {
        QLabel* lbl = new QLabel(label, parent);
        lbl->setStyleSheet("font-size:12px; font-weight:600; color:#374151;");
        lay->addWidget(lbl);
        auto* le = new QLineEdit(parent);
        le->setPlaceholderText(placeholder);
        le->setStyleSheet(
            "QLineEdit { padding:9px 12px; border:1.5px solid #E2E8F0; border-radius:6px; "
            "font-size:13px; background:#FFFFFF; }"
            "QLineEdit:focus { border-color:#3B82F6; }");
        le->setFixedHeight(38);
        lay->addWidget(le);
        return le;
    };

    // ── Row 1: Name + Department ─────────────────────────────────────────────
    QHBoxLayout* r1 = new QHBoxLayout(); r1->setSpacing(12);

    auto makeHalf = [&](QWidget* parent, const QString& label,
                         const QString& ph, QLineEdit*& outField) {
        QWidget* w = new QWidget(parent);
        QVBoxLayout* l = new QVBoxLayout(w); l->setContentsMargins(0,0,0,0); l->setSpacing(4);
        QLabel* lb = new QLabel(label, w);
        lb->setStyleSheet("font-size:12px; font-weight:600; color:#374151;");
        outField = new QLineEdit(w);
        outField->setPlaceholderText(ph);
        outField->setStyleSheet(
            "QLineEdit { padding:9px 12px; border:1.5px solid #E2E8F0; border-radius:6px; "
            "font-size:13px; background:#FFFFFF; }"
            "QLineEdit:focus { border-color:#3B82F6; }");
        outField->setFixedHeight(38);
        l->addWidget(lb); l->addWidget(outField);
        return w;
    };

    // ── Department dropdown (editable — can type a custom value too) ──────────
    static const QStringList DEPARTMENTS = {
        "Engineering",        "Software Development",  "Information Technology",
        "Human Resources",    "Finance & Accounts",    "Marketing",
        "Sales",              "Operations",            "Administration",
        "Customer Support",   "Research & Development","Legal",
        "Procurement",        "Quality Assurance",     "Security",
        "Management",         "Data Analytics",        "Design & UX"
    };
    static const QStringList DESIGNATIONS = {
        // Engineering / Tech
        "Software Engineer",          "Senior Software Engineer",
        "Junior Software Engineer",   "Lead Software Engineer",
        "Full Stack Developer",       "Frontend Developer",
        "Backend Developer",          "Mobile App Developer",
        "DevOps Engineer",            "QA Engineer",
        "Data Scientist",             "Data Analyst",
        "UI/UX Designer",             "System Architect",
        "Database Administrator",     "Network Engineer",
        "Cybersecurity Analyst",      "IT Support Specialist",
        "Technical Lead",             "CTO",
        // Management
        "Project Manager",            "Product Manager",
        "Engineering Manager",        "Operations Manager",
        "General Manager",            "CEO",
        "COO",                        "CFO",
        "Team Lead",                  "Department Head",
        "Director",                   "Vice President",
        // HR / Admin
        "HR Manager",                 "HR Officer",
        "Recruitment Specialist",     "Payroll Officer",
        "Administrative Officer",     "Office Manager",
        "Executive Assistant",        "Receptionist",
        // Finance
        "Accountant",                 "Senior Accountant",
        "Finance Manager",            "Financial Analyst",
        "Audit Officer",              "Tax Consultant",
        // Sales / Marketing
        "Sales Executive",            "Senior Sales Executive",
        "Sales Manager",              "Marketing Executive",
        "Marketing Manager",          "Brand Manager",
        "Content Writer",             "SEO Specialist",
        "Business Development Manager",
        // Support / Operations
        "Customer Support Agent",     "Customer Support Manager",
        "Operations Executive",       "Procurement Officer",
        "Supply Chain Manager",       "Quality Assurance Officer",
        "Security Officer",           "Intern"
    };

    QString cboCss =
        "QComboBox { padding:9px 12px; border:1.5px solid #E2E8F0; border-radius:6px; "
        "font-size:13px; background:#FFFFFF; color:#0F172A; }"
        "QComboBox:focus { border-color:#3B82F6; }"
        "QComboBox:hover { border-color:#93C5FD; }"
        "QComboBox::drop-down { border:none; width:28px; }"
        "QComboBox::down-arrow { width:12px; height:12px; }"
        "QComboBox QAbstractItemView { "
        "border:1px solid #CBD5E1; border-radius:6px; background:#FFFFFF; "
        "selection-background-color:#EFF6FF; selection-color:#1D4ED8; "
        "font-size:13px; padding:4px; outline:none; }"
        "QComboBox QAbstractItemView::item { padding:7px 12px; min-height:28px; }"
        "QComboBox QAbstractItemView::item:hover { background:#F0F9FF; color:#0369A1; }";

    // Department widget
    QWidget* wDept = new QWidget(inner);
    QVBoxLayout* lDept = new QVBoxLayout(wDept); lDept->setContentsMargins(0,0,0,0); lDept->setSpacing(4);
    QLabel* lbDept = new QLabel("Department *", wDept);
    lbDept->setStyleSheet("font-size:12px; font-weight:600; color:#374151;");
    auto* cboDept = new QComboBox(wDept);
    cboDept->setEditable(false);
    cboDept->addItem("— Select Department —");
    cboDept->addItems(DEPARTMENTS);
    cboDept->setStyleSheet(cboCss);
    cboDept->setFixedHeight(38);
    lDept->addWidget(lbDept); lDept->addWidget(cboDept);

    QLineEdit *fName = nullptr;
    r1->addWidget(makeHalf(inner, "Full Name *", "e.g. Ahmed Ali", fName));
    r1->addWidget(wDept);
    lay->addLayout(r1);

    // Designation dropdown (non-editable, click to pick)
    QWidget* wPost = new QWidget(inner);
    QVBoxLayout* lPost = new QVBoxLayout(wPost); lPost->setContentsMargins(0,0,0,0); lPost->setSpacing(4);
    QLabel* lbPost = new QLabel("Designation *", wPost);
    lbPost->setStyleSheet("font-size:12px; font-weight:600; color:#374151;");
    auto* cboPost = new QComboBox(wPost);
    cboPost->setEditable(false);
    cboPost->addItem("— Select Designation —");
    cboPost->addItems(DESIGNATIONS);
    cboPost->setStyleSheet(cboCss);
    cboPost->setFixedHeight(38);
    lPost->addWidget(lbPost); lPost->addWidget(cboPost);
    lay->addWidget(wPost);
    auto* fEmail   = makeField(inner, "Email", "e.g. name@company.com");
    auto* fPhone   = makeField(inner, "Phone", "e.g. +92-300-1234567");

    // ── CNIC Field ───────────────────────────────────────────────────────────
    auto* fCnic = makeField(inner, "CNIC", "e.g. 35202-1234567-1");
    fCnic->setInputMask("99999-9999999-9;_");   // enforce format at input level
    fCnic->setPlaceholderText("35202-1234567-1");

    // ── Row 2: Salary + Gender ───────────────────────────────────────────────
    QHBoxLayout* r2 = new QHBoxLayout(); r2->setSpacing(12);

    QWidget* wSal = new QWidget(inner);
    QVBoxLayout* lSal = new QVBoxLayout(wSal); lSal->setContentsMargins(0,0,0,0); lSal->setSpacing(4);
    QLabel* lbSal = new QLabel("Basic Salary (Rs.) *", inner);
    lbSal->setStyleSheet("font-size:12px; font-weight:600; color:#374151;");
    auto* fSalary = new QDoubleSpinBox(inner);
    fSalary->setRange(0, 5000000); fSalary->setValue(45000); fSalary->setDecimals(2);
    fSalary->setStyleSheet(
        "QDoubleSpinBox { padding:9px; border:1.5px solid #E2E8F0; border-radius:6px; font-size:13px; }");
    fSalary->setFixedHeight(38);
    lSal->addWidget(lbSal); lSal->addWidget(fSalary);
    r2->addWidget(wSal);

    QWidget* wGender = new QWidget(inner);
    QVBoxLayout* lGender = new QVBoxLayout(wGender); lGender->setContentsMargins(0,0,0,0); lGender->setSpacing(4);
    QLabel* lbGender = new QLabel("Gender", inner);
    lbGender->setStyleSheet("font-size:12px; font-weight:600; color:#374151;");
    auto* cboGender = new QComboBox(inner);
    cboGender->addItems({"Male", "Female", "Other"});
    cboGender->setStyleSheet(
        "QComboBox { padding:9px; border:1.5px solid #E2E8F0; border-radius:6px; font-size:13px; }");
    cboGender->setFixedHeight(38);
    lGender->addWidget(lbGender); lGender->addWidget(cboGender);
    r2->addWidget(wGender);
    lay->addLayout(r2);

    auto* fJoinDate = makeField(inner, "Join Date", QDate::currentDate().toString("yyyy-MM-dd"));
    fJoinDate->setText(QDate::currentDate().toString("yyyy-MM-dd"));

    // ── Fingerprint Enrollment Section ───────────────────────────────────────
    QFrame* fpFrame = new QFrame(inner);
    fpFrame->setStyleSheet(
        "QFrame { background:#F3EEFF; border:1.5px solid #DDD6FE; border-radius:8px; }");
    QHBoxLayout* fpLay = new QHBoxLayout(fpFrame);
    fpLay->setContentsMargins(14, 12, 14, 12);
    fpLay->setSpacing(12);

    QLabel* fpIcon = new QLabel("🖐", fpFrame);
    fpIcon->setStyleSheet("font-size:28px;");
    fpLay->addWidget(fpIcon);

    QVBoxLayout* fpTextLay = new QVBoxLayout();
    fpTextLay->setSpacing(2);
    QLabel* fpTitle = new QLabel("Fingerprint Biometric", fpFrame);
    fpTitle->setStyleSheet("font-size:13px; font-weight:700; color:#5B21B6;");
    fpTextLay->addWidget(fpTitle);

    // Track enrollment state
    QString capturedTemplate;   // filled when enrolled
    QLabel* fpStatusLbl = new QLabel("Not enrolled — click Enroll to register fingerprint", fpFrame);
    fpStatusLbl->setStyleSheet("font-size:11px; color:#7C3AED;");
    fpTextLay->addWidget(fpStatusLbl);
    fpLay->addLayout(fpTextLay, 1);

    auto* btnEnrollFP = new QPushButton("🖐 Enroll Fingerprint", fpFrame);
    btnEnrollFP->setStyleSheet(
        "QPushButton { background:#7C3AED; color:#FFFFFF; padding:8px 14px; "
        "font-weight:700; border-radius:6px; border:none; font-size:12px; }"
        "QPushButton:hover { background:#6D28D9; }");
    btnEnrollFP->setFixedHeight(36);
    fpLay->addWidget(btnEnrollFP);
    lay->addWidget(fpFrame);

    // ── Face ID Enrollment Section ────────────────────────────────────────────
    QFrame* faceFrame = new QFrame(inner);
    faceFrame->setStyleSheet(
        "QFrame { background:#E0F2FE; border:1.5px solid #BAE6FD; border-radius:8px; }");
    QHBoxLayout* faceLay = new QHBoxLayout(faceFrame);
    faceLay->setContentsMargins(14, 12, 14, 12);
    faceLay->setSpacing(12);

    QLabel* faceIcon = new QLabel("\U0001FABA", faceFrame);  // 🪪
    faceIcon->setStyleSheet("font-size:28px;");
    faceLay->addWidget(faceIcon);

    QVBoxLayout* faceTextLay = new QVBoxLayout();
    faceTextLay->setSpacing(2);
    QLabel* faceTitle = new QLabel("Face ID Biometric", faceFrame);
    faceTitle->setStyleSheet("font-size:13px; font-weight:700; color:#0369A1;");
    faceTextLay->addWidget(faceTitle);

    QString capturedFaceTemplate;
    QLabel* faceStatusLbl = new QLabel("Not enrolled — click Enroll to register Face ID", faceFrame);
    faceStatusLbl->setStyleSheet("font-size:11px; color:#0369A1;");
    faceTextLay->addWidget(faceStatusLbl);
    faceLay->addLayout(faceTextLay, 1);

    auto* btnEnrollFace = new QPushButton("📷 Enroll Face ID", faceFrame);
    btnEnrollFace->setStyleSheet(
        "QPushButton { background:#0369A1; color:#FFFFFF; padding:8px 14px; "
        "font-weight:700; border-radius:6px; border:none; font-size:12px; }"
        "QPushButton:hover { background:#0284C7; }");
    btnEnrollFace->setFixedHeight(36);
    faceLay->addWidget(btnEnrollFace);
    lay->addWidget(faceFrame);

    scroll->setWidget(inner);
    outerLay->addWidget(scroll, 1);

    // ── Sticky bottom button row (outside scroll) ────────────────────────────
    QWidget* btnContainer = new QWidget(&dlg);
    btnContainer->setStyleSheet(
        "background:#F8FAFC; border-top:1px solid #E2E8F0;");
    QHBoxLayout* btnRow = new QHBoxLayout(btnContainer);
    btnRow->setContentsMargins(30, 14, 30, 14);
    btnRow->setSpacing(10);
    auto* btnCancel = makeActionBtn("Cancel", "#FFFFFF", "#334155", "1px solid #CBD5E1", &dlg);
    auto* btnSave   = makeActionBtn("✓ Save Employee", "#1E3A5F", "#FFFFFF", "none", &dlg);
    btnRow->addStretch();
    btnRow->addWidget(btnCancel);
    btnRow->addWidget(btnSave);
    outerLay->addWidget(btnContainer);

    // ── Fingerprint enroll button connection ─────────────────────────────────
    int newEmpId = db.getNextId();
    connect(btnEnrollFP, &QPushButton::clicked, [&](){
        QString nameForFP = fName->text().trimmed();
        if (nameForFP.isEmpty()) nameForFP = "New Employee";

        FingerprintDialog fpDlg(newEmpId, nameForFP,
                                FingerprintDialog::Mode::Enroll, &dlg);
        if (fpDlg.exec() == QDialog::Accepted && !fpDlg.enrolledTemplate().isEmpty()) {
            capturedTemplate = fpDlg.enrolledTemplate();
            fpStatusLbl->setText("✅  Fingerprint enrolled successfully");
            fpStatusLbl->setStyleSheet("font-size:11px; color:#10B981; font-weight:600;");
            btnEnrollFP->setText("🔄 Re-enroll");
            fpFrame->setStyleSheet(
                "QFrame { background:#ECFDF5; border:1.5px solid #A7F3D0; border-radius:8px; }");
        }
    });

    // ── Face ID enroll button connection ─────────────────────────────────────
    connect(btnEnrollFace, &QPushButton::clicked, [&](){
        QString nameForFace = fName->text().trimmed();
        if (nameForFace.isEmpty()) nameForFace = "New Employee";

        FaceIdDialog faceDlg(newEmpId, nameForFace, FaceIdDialog::Mode::Enroll, &dlg);
        if (faceDlg.exec() == QDialog::Accepted && !faceDlg.enrolledTemplate().isEmpty()) {
            capturedFaceTemplate = faceDlg.enrolledTemplate();
            faceStatusLbl->setText("✅  Face ID enrolled successfully");
            faceStatusLbl->setStyleSheet("font-size:11px; color:#10B981; font-weight:600;");
            btnEnrollFace->setText("🔄 Re-enroll Face");
            faceFrame->setStyleSheet(
                "QFrame { background:#ECFDF5; border:1.5px solid #A7F3D0; border-radius:8px; }");
        }
    });

    connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(btnSave, &QPushButton::clicked, [&](){
        QString deptVal = cboDept->currentText().trimmed();
        QString postVal = cboPost->currentText().trimmed();
        if (deptVal == "— Select Department —") deptVal = "";
        if (postVal == "— Select Designation —") postVal = "";
        if (fName->text().trimmed().isEmpty() || deptVal.isEmpty() || postVal.isEmpty()) {
            QMessageBox::warning(&dlg, "Validation", "Please fill required fields (*).");
            return;
        }
        double sal = fSalary->value();
        Employee emp(newEmpId,
                     fName->text().trimmed(),
                     deptVal,
                     postVal,
                     sal, sal * 0.15, sal * 0.06,
                     fEmail->text().trimmed(),
                     fPhone->text().trimmed(),
                     fJoinDate->text().trimmed(),
                     "Active",
                     cboGender->currentText(),
                     fCnic->text().trimmed(),
                     capturedTemplate);
        db.insertRecord(emp);

        // ── Auto-generate viewer credentials for the new employee ─────────
        // Username : emp_<id>   e.g.  emp_5
        // Password : EMP@<id><4 random digits>  e.g.  EMP@5_3842
        QString autoUser = QString("emp_%1").arg(newEmpId);
        QString autoPass = QString("EMP@%1_%2")
                           .arg(newEmpId)
                           .arg(QRandomGenerator::global()->bounded(1000, 9999));

        userDB.addUser(autoUser, autoPass, "viewer",
                       fName->text().trimmed(),
                       fEmail->text().trimmed());

        dlg.accept();

        // ── Show credentials dialog ───────────────────────────────────────
        QDialog credDlg(this);
        credDlg.setWindowTitle("Employee Account Created");
        credDlg.setFixedSize(460, 320);
        credDlg.setStyleSheet("background:#F8FAFC; font-family:'Segoe UI';");

        QVBoxLayout* cl = new QVBoxLayout(&credDlg);
        cl->setContentsMargins(28, 24, 28, 20);
        cl->setSpacing(12);

        // Header
        QLabel* icon = new QLabel("🔐", &credDlg);
        icon->setAlignment(Qt::AlignCenter);
        icon->setStyleSheet("font-size:36px;");
        cl->addWidget(icon);

        QLabel* hdr = new QLabel("Login Credentials Generated", &credDlg);
        hdr->setAlignment(Qt::AlignCenter);
        hdr->setStyleSheet("font-size:16px; font-weight:800; color:#0F172A;");
        cl->addWidget(hdr);

        QLabel* sub = new QLabel(
            QString("Account created for  <b>%1</b>").arg(fName->text().trimmed()),
            &credDlg);
        sub->setAlignment(Qt::AlignCenter);
        sub->setStyleSheet("font-size:12px; color:#64748B;");
        cl->addWidget(sub);

        // Credentials box
        QFrame* box = new QFrame(&credDlg);
        box->setStyleSheet(
            "QFrame { background:#1E293B; border-radius:10px; padding:4px; }");
        QVBoxLayout* bl = new QVBoxLayout(box);
        bl->setSpacing(10);
        bl->setContentsMargins(16, 14, 16, 14);

        auto credRow = [&](const QString& label, const QString& value) {
            QHBoxLayout* row = new QHBoxLayout();
            QLabel* lbl = new QLabel(label, box);
            lbl->setStyleSheet("color:#94A3B8; font-size:11px; font-weight:600; min-width:80px;");
            QLabel* val = new QLabel(value, box);
            val->setStyleSheet(
                "color:#F1F5F9; font-size:14px; font-weight:700; font-family:'Consolas';");
            val->setTextInteractionFlags(Qt::TextSelectableByMouse);
            row->addWidget(lbl);
            row->addWidget(val);
            row->addStretch();
            bl->addLayout(row);
        };
        credRow("Username :", autoUser);
        credRow("Password :", autoPass);
        credRow("Role     :", "Viewer  (read-only)");
        cl->addWidget(box);

        QLabel* note = new QLabel(
            "ℹ  Share these credentials with the employee.\n"
            "   They can log in to view their own profile.", &credDlg);
        note->setStyleSheet(
            "color:#0369A1; font-size:10px; background:#E0F2FE; padding:8px; "
            "border-radius:6px; border:1px solid #BAE6FD;");
        note->setWordWrap(true);
        cl->addWidget(note);

        QPushButton* okBtn = new QPushButton("✓  Got it", &credDlg);
        okBtn->setStyleSheet(
            "QPushButton { background:#0369A1; color:#FFFFFF; padding:10px 28px; "
            "font-weight:700; border-radius:6px; border:none; font-size:13px; }"
            "QPushButton:hover { background:#0284C7; }");
        okBtn->setFixedHeight(40);

        // ── Export button ─────────────────────────────────────────────────
        QPushButton* exportBtn = new QPushButton("📄  Export", &credDlg);
        exportBtn->setStyleSheet(
            "QPushButton { background:#10B981; color:#FFFFFF; padding:10px 22px; "
            "font-weight:700; border-radius:6px; border:none; font-size:13px; }"
            "QPushButton:hover { background:#059669; }");
        exportBtn->setFixedHeight(40);

        QHBoxLayout* btnRowCred = new QHBoxLayout();
        btnRowCred->setSpacing(10);
        btnRowCred->addWidget(exportBtn);
        btnRowCred->addStretch();
        btnRowCred->addWidget(okBtn);
        cl->addLayout(btnRowCred);

        // Copy values so the lambda owns them (dialog may close before lambda runs)
        QString expUser = autoUser;
        QString expPass = autoPass;
        QString expName = fName->text().trimmed();
        QString expEmail= fEmail->text().trimmed();

        QObject::connect(exportBtn, &QPushButton::clicked, &credDlg,
            [&credDlg, expUser, expPass, expName, expEmail]() {
                QString path = QFileDialog::getSaveFileName(
                    &credDlg,
                    "Save Credentials",
                    QString("credentials_%1.txt").arg(expUser),
                    "Text Files (*.txt);;All Files (*)");
                if (path.isEmpty()) return;
                if (!path.endsWith(".txt", Qt::CaseInsensitive)) path += ".txt";

                QFile f(path);
                if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QMessageBox::critical(&credDlg, "Error",
                        "Could not write file. Check permissions.");
                    return;
                }
                QTextStream out(&f);
                out << "========================================\n";
                out << "   Employee Management System\n";
                out << "   Login Credentials\n";
                out << "========================================\n\n";
                out << "Employee  : " << expName  << "\n";
                out << "Email     : " << (expEmail.isEmpty() ? "N/A" : expEmail) << "\n\n";
                out << "Username  : " << expUser  << "\n";
                out << "Password  : " << expPass  << "\n";
                out << "Role      : Viewer (read-only)\n\n";
                out << "----------------------------------------\n";
                out << "Log in using the Employee Management\n";
                out << "System desktop application.\n";
                out << "========================================\n";
                f.close();
                QMessageBox::information(&credDlg, "Exported",
                    QString("Credentials saved to:\n%1").arg(path));
            });

        QObject::connect(okBtn, &QPushButton::clicked, &credDlg, &QDialog::accept);
        credDlg.exec();
    });

    if (dlg.exec() == QDialog::Accepted) {
        reloadViewGrid();
    }
}

void Dashboard::executionUpdateWorkflow() {
    int row = mainGrid->currentRow();
    if (row < 0) { QMessageBox::warning(this, "No Selection", "Select an employee row to edit."); return; }

    int id = mainGrid->item(row, 0)->text().toInt();
    Employee* emp = db.lookupRecord(id);
    if (!emp) return;

    QDialog dlg(this);
    dlg.setWindowTitle("Edit Employee Profile");
    dlg.setMinimumSize(540, 560);
    dlg.resize(540, 580);
    dlg.setStyleSheet("background:#F8FAFC; font-family:'Segoe UI';");

    QVBoxLayout* outerLay = new QVBoxLayout(&dlg);
    outerLay->setContentsMargins(0, 0, 0, 0);
    outerLay->setSpacing(0);

    QScrollArea* scroll = new QScrollArea(&dlg);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background:#F8FAFC; border:none; }");

    QWidget* inner = new QWidget(scroll);
    QVBoxLayout* lay = new QVBoxLayout(inner);
    lay->setContentsMargins(30, 28, 30, 16);
    lay->setSpacing(12);

    QLabel* t = new QLabel("Edit Employee: " + emp->getName(), inner);
    t->setStyleSheet("font-size:18px; font-weight:800; color:#0F172A; font-family:'Georgia',serif;");
    lay->addWidget(t);

    auto makeRow = [&](const QString& label, const QString& val, QLineEdit*& field) {
        QLabel* lbl = new QLabel(label, inner);
        lbl->setStyleSheet("font-size:12px; font-weight:600; color:#374151;");
        lay->addWidget(lbl);
        field = new QLineEdit(inner);
        field->setText(val);
        field->setStyleSheet("QLineEdit { padding:9px 12px; border:1.5px solid #E2E8F0; border-radius:6px; font-size:13px; }"
                             "QLineEdit:focus { border-color:#3B82F6; }");
        field->setFixedHeight(38);
        lay->addWidget(field);
    };

    QLineEdit *fName, *fDept, *fPost, *fEmail, *fPhone, *fCnic;
    makeRow("Full Name", emp->getName(), fName);
    makeRow("Department", emp->getDepartment(), fDept);
    makeRow("Designation", emp->getDesignation(), fPost);
    makeRow("Email", emp->getEmail(), fEmail);
    makeRow("Phone", emp->getPhone(), fPhone);

    // CNIC field
    QLabel* lbCnic = new QLabel("CNIC", inner);
    lbCnic->setStyleSheet("font-size:12px; font-weight:600; color:#374151;");
    lay->addWidget(lbCnic);
    fCnic = new QLineEdit(inner);
    fCnic->setText(emp->getCnic());
    fCnic->setInputMask("99999-9999999-9;_");
    fCnic->setStyleSheet("QLineEdit { padding:9px 12px; border:1.5px solid #E2E8F0; border-radius:6px; font-size:13px; }"
                         "QLineEdit:focus { border-color:#3B82F6; }");
    fCnic->setFixedHeight(38);
    lay->addWidget(fCnic);

    QLabel* lbSt = new QLabel("Status", inner); lbSt->setStyleSheet("font-size:12px; font-weight:600; color:#374151;");
    lay->addWidget(lbSt);
    auto* cboStatus = new QComboBox(inner);
    cboStatus->addItems({"Active", "On Leave", "Resigned"});
    cboStatus->setCurrentText(emp->getStatus());
    cboStatus->setStyleSheet("QComboBox { padding:9px; border:1.5px solid #E2E8F0; border-radius:6px; font-size:13px; }");
    cboStatus->setFixedHeight(38);
    lay->addWidget(cboStatus);

    QLabel* lbSal = new QLabel("Basic Salary (Rs.)", inner); lbSal->setStyleSheet("font-size:12px; font-weight:600; color:#374151;");
    lay->addWidget(lbSal);
    auto* fSal = new QDoubleSpinBox(inner);
    fSal->setRange(0, 5000000); fSal->setValue(emp->getBasicSalary()); fSal->setDecimals(2);
    fSal->setStyleSheet("QDoubleSpinBox { padding:9px; border:1.5px solid #E2E8F0; border-radius:6px; font-size:13px; }");
    fSal->setFixedHeight(38);
    lay->addWidget(fSal);

    // Fingerprint status indicator
    QFrame* fpFrame = new QFrame(inner);
    fpFrame->setStyleSheet(emp->hasFingerprintEnrolled()
        ? "QFrame { background:#ECFDF5; border:1.5px solid #A7F3D0; border-radius:8px; }"
        : "QFrame { background:#F3EEFF; border:1.5px solid #DDD6FE; border-radius:8px; }");
    QHBoxLayout* fpLay = new QHBoxLayout(fpFrame);
    fpLay->setContentsMargins(12, 10, 12, 10);
    QLabel* fpLbl = new QLabel(
        emp->hasFingerprintEnrolled()
            ? "🖐  Fingerprint enrolled"
            : "🖐  No fingerprint enrolled",
        fpFrame);
    fpLbl->setStyleSheet(emp->hasFingerprintEnrolled()
        ? "font-size:12px; color:#10B981; font-weight:600;"
        : "font-size:12px; color:#7C3AED;");
    fpLay->addWidget(fpLbl, 1);

    auto* btnReEnroll = new QPushButton(
        emp->hasFingerprintEnrolled() ? "🔄 Re-enroll" : "🖐 Enroll Now", fpFrame);
    btnReEnroll->setStyleSheet(
        "QPushButton { background:#7C3AED; color:#FFFFFF; padding:7px 12px; "
        "font-weight:700; border-radius:6px; border:none; font-size:12px; }");
    btnReEnroll->setFixedHeight(34);
    fpLay->addWidget(btnReEnroll);
    lay->addWidget(fpFrame);

    scroll->setWidget(inner);
    outerLay->addWidget(scroll, 1);

    // Sticky bottom buttons
    QWidget* btnContainer = new QWidget(&dlg);
    btnContainer->setStyleSheet("background:#F8FAFC; border-top:1px solid #E2E8F0;");
    QHBoxLayout* br = new QHBoxLayout(btnContainer);
    br->setContentsMargins(30, 14, 30, 14);
    br->setSpacing(10);
    auto* bCancel = makeActionBtn("Cancel", "#FFFFFF", "#334155", "1px solid #CBD5E1", &dlg);
    auto* bSave   = makeActionBtn("✓ Save Changes", "#1E3A5F", "#FFFFFF", "none", &dlg);
    br->addStretch(); br->addWidget(bCancel); br->addWidget(bSave);
    outerLay->addWidget(btnContainer);

    // Re-enroll connection
    connect(btnReEnroll, &QPushButton::clicked, [&](){
        FingerprintDialog fpDlg(emp->getId(), emp->getName(),
                                FingerprintDialog::Mode::Enroll, &dlg);
        if (fpDlg.exec() == QDialog::Accepted && !fpDlg.enrolledTemplate().isEmpty()) {
            emp->setFingerprintTemplate(fpDlg.enrolledTemplate());
            db.modifyRecord(emp->getId(), *emp);
            fpLbl->setText("🖐  Fingerprint enrolled");
            fpLbl->setStyleSheet("font-size:12px; color:#10B981; font-weight:600;");
            fpFrame->setStyleSheet("QFrame { background:#ECFDF5; border:1.5px solid #A7F3D0; border-radius:8px; }");
            btnReEnroll->setText("🔄 Re-enroll");
        }
    });

    connect(bCancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(bSave, &QPushButton::clicked, [&](){
        emp->setName(fName->text().trimmed());
        emp->setDepartment(fDept->text().trimmed());
        emp->setDesignation(fPost->text().trimmed());
        emp->setEmail(fEmail->text().trimmed());
        emp->setPhone(fPhone->text().trimmed());
        emp->setCnic(fCnic->text().trimmed());
        emp->setStatus(cboStatus->currentText());
        double sal = fSal->value();
        emp->setBasicSalary(sal);
        emp->setAllowance(sal * 0.15);
        emp->setDeductions(sal * 0.06);
        db.modifyRecord(id, *emp);
        dlg.accept();
    });

    if (dlg.exec() == QDialog::Accepted) {
        reloadViewGrid();
        QMessageBox::information(this, "Updated", "Employee profile updated.");
    }
}

void Dashboard::executionDeleteWorkflow() {
    int row = mainGrid->currentRow();
    if (row < 0) return;
    int id = mainGrid->item(row, 0)->text().toInt();
    QString name = mainGrid->item(row, 1)->text();

    if (QMessageBox::question(this, "Confirm Delete",
        QString("Permanently delete employee:\n\n  %1 (ID: %2)\n\nThis cannot be undone.")
        .arg(name).arg(id),
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        db.removeRecord(id);
        reloadViewGrid();
    }
}

void Dashboard::executionAttendanceWorkflow() {
    int row = mainGrid->currentRow();
    if (row < 0) { QMessageBox::warning(this, "No Selection", "Select an employee first."); return; }

    int id = mainGrid->item(row, 0)->text().toInt();
    Employee* emp = db.lookupRecord(id);
    if (!emp) return;

    QDialog dlg(this);
    dlg.setWindowTitle("Log Attendance: " + emp->getName());
    dlg.setFixedSize(360, 260);
    dlg.setStyleSheet("background:#F8FAFC; font-family:'Segoe UI';");

    QVBoxLayout* lay = new QVBoxLayout(&dlg);
    lay->setContentsMargins(28, 24, 28, 20);
    lay->setSpacing(14);

    QLabel* t = new QLabel("Log Attendance", &dlg);
    t->setStyleSheet("font-size:18px; font-weight:800; color:#0F172A; font-family:'Georgia',serif;");
    lay->addWidget(t);

    QLabel* empInfo = new QLabel(emp->getName() + "  |  " + emp->getDepartment(), &dlg);
    empInfo->setStyleSheet("color:#64748B; font-size:13px;");
    lay->addWidget(empInfo);

    // Current stats
    QLabel* stats = new QLabel(
        QString("Current: Present %1 | Absent %2 | Leave %3")
        .arg(emp->getPresentDays()).arg(emp->getAbsentDays()).arg(emp->getLeaveDays()), &dlg);
    stats->setStyleSheet("color:#3B82F6; font-size:12px; background:#EFF6FF; "
                         "padding:8px; border-radius:6px;");
    lay->addWidget(stats);

    QLabel* lbl = new QLabel("Mark as:", &dlg);
    lbl->setStyleSheet("font-size:12px; font-weight:600; color:#374151;");
    lay->addWidget(lbl);

    auto* cbo = new QComboBox(&dlg);
    cbo->addItems({"Present", "Absent", "On Leave"});
    cbo->setStyleSheet("QComboBox { padding:9px; border:1.5px solid #E2E8F0; border-radius:6px; font-size:13px; }");
    cbo->setFixedHeight(38);
    lay->addWidget(cbo);

    QHBoxLayout* br = new QHBoxLayout();
    auto* bCancel = makeActionBtn("Cancel", "#FFFFFF", "#334155", "1px solid #CBD5E1", &dlg);
    auto* bSave   = makeActionBtn("✓ Log Entry", "#1E3A5F", "#FFFFFF", "none", &dlg);
    br->addStretch(); br->addWidget(bCancel); br->addWidget(bSave);
    lay->addLayout(br);

    connect(bCancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(bSave, &QPushButton::clicked, [&](){
        QString sel = cbo->currentText();
        if (sel == "Present")  emp->logPresent();
        else if (sel == "Absent")  emp->logAbsent();
        else  emp->logLeave();
        db.modifyRecord(id, *emp);
        dlg.accept();
    });

    if (dlg.exec() == QDialog::Accepted)
        reloadViewGrid();
}

void Dashboard::executionPaySlipWorkflow() {
    int row = mainGrid->currentRow();
    if (row < 0) return;
    int id = mainGrid->item(row, 0)->text().toInt();
    Employee* emp = db.lookupRecord(id);
    if (!emp) return;

    QDialog dlg(this);
    dlg.setWindowTitle("Pay Slip — " + emp->getName());
    dlg.setFixedSize(480, 500);
    dlg.setStyleSheet("background:#F8FAFC; font-family:'Segoe UI';");

    QVBoxLayout* lay = new QVBoxLayout(&dlg);
    lay->setContentsMargins(24, 20, 24, 20);

    QTextEdit* slip = new QTextEdit(&dlg);
    slip->setReadOnly(true);
    slip->setText(emp->generateSlipPayload());
    slip->setStyleSheet(
        "QTextEdit { background:#0F172A; color:#E2E8F0; font-family:'Consolas',monospace; "
        "font-size:12px; border-radius:8px; padding:16px; border:none; }");
    lay->addWidget(slip);

    auto* btnClose = makeActionBtn("Close", "#1E3A5F", "#FFFFFF", "none", &dlg);
    lay->addWidget(btnClose, 0, Qt::AlignRight);
    connect(btnClose, &QPushButton::clicked, &dlg, &QDialog::accept);
    dlg.exec();
}

void Dashboard::executionLiveSearch(const QString& text) {
    Q_UNUSED(text);
    applyFiltersAndRefresh();
}

void Dashboard::executionSortChanged(int) {
    applyFiltersAndRefresh();
}

void Dashboard::executionAdvancedFilter() {
    QDialog dlg(this);
    dlg.setWindowTitle("Advanced Filter");
    dlg.setFixedSize(400, 380);
    dlg.setStyleSheet("background:#F8FAFC; font-family:'Segoe UI';");

    QVBoxLayout* lay = new QVBoxLayout(&dlg);
    lay->setContentsMargins(28, 24, 28, 20);
    lay->setSpacing(14);

    QLabel* t = new QLabel("Advanced Filter", &dlg);
    t->setStyleSheet("font-size:18px; font-weight:800; color:#0F172A; font-family:'Georgia',serif;");
    lay->addWidget(t);

    QFormLayout* form = new QFormLayout();
    form->setSpacing(10);

    auto* fMinSal = new QDoubleSpinBox(&dlg);
    fMinSal->setRange(0, 5000000); fMinSal->setValue(0); fMinSal->setDecimals(0);
    fMinSal->setStyleSheet("QDoubleSpinBox { padding:8px; border:1.5px solid #E2E8F0; border-radius:6px; }");
    form->addRow("Min Net Salary (Rs.):", fMinSal);

    auto* fMaxSal = new QDoubleSpinBox(&dlg);
    fMaxSal->setRange(0, 5000000); fMaxSal->setValue(5000000); fMaxSal->setDecimals(0);
    fMaxSal->setStyleSheet(fMinSal->styleSheet());
    form->addRow("Max Net Salary (Rs.):", fMaxSal);

    auto* fMinAtt = new QDoubleSpinBox(&dlg);
    fMinAtt->setRange(0, 100); fMinAtt->setValue(0); fMinAtt->setSuffix("%");
    fMinAtt->setStyleSheet(fMinSal->styleSheet());
    form->addRow("Min Attendance (%):", fMinAtt);

    auto* cboGender = new QComboBox(&dlg);
    cboGender->addItems({"All Genders", "Male", "Female", "Other"});
    cboGender->setStyleSheet("QComboBox { padding:8px; border:1.5px solid #E2E8F0; border-radius:6px; }");
    form->addRow("Gender:", cboGender);

    lay->addLayout(form);
    lay->addStretch();

    QHBoxLayout* br = new QHBoxLayout();
    auto* bReset  = makeActionBtn("Reset Filters", "#FFFFFF", "#334155", "1px solid #CBD5E1", &dlg);
    auto* bApply  = makeActionBtn("Apply", "#1E3A5F", "#FFFFFF", "none", &dlg);
    br->addStretch(); br->addWidget(bReset); br->addWidget(bApply);
    lay->addLayout(br);

    connect(bReset, &QPushButton::clicked, [&](){
        fMinSal->setValue(0); fMaxSal->setValue(5000000);
        fMinAtt->setValue(0); cboGender->setCurrentIndex(0);
        barSearch->clear();
        cboDeptFilter->setCurrentIndex(0);
        cboStatusFilter->setCurrentIndex(0);
        applyFiltersAndRefresh();
        dlg.accept();
    });

    connect(bApply, &QPushButton::clicked, [&](){
        FilterCriteria fc;
        fc.minSalary = fMinSal->value();
        fc.maxSalary = fMaxSal->value();
        fc.minAttendance = fMinAtt->value();
        if (cboGender->currentIndex() > 0) fc.gender = cboGender->currentText();
        QString dept = cboDeptFilter->currentText();
        if (dept != "All Departments") fc.department = dept;
        QString status = cboStatusFilter->currentText();
        if (status != "All Status") fc.status = status;

        auto filtered = db.searchDatabase(fc);
        populateGrid(filtered);
        dlg.accept();
    });

    dlg.exec();
}

void Dashboard::executionExportWorkflow() {
    QString path = QFileDialog::getSaveFileName(this, "Export Employee Data", "",
                                                "CSV Files (*.csv)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&f);
    out << "ID,Name,Department,Designation,Email,Phone,Join Date,Status,Gender,"
           "Basic Salary,Allowance,Deductions,Net Pay,Present,Absent,Leave,Attendance%\n";

    for (const auto& emp : db.pullAllRecords()) {
        out << emp.getId() << "," << emp.getName() << "," << emp.getDepartment() << ","
            << emp.getDesignation() << "," << emp.getEmail() << "," << emp.getPhone() << ","
            << emp.getJoinDate() << "," << emp.getStatus() << "," << emp.getGender() << ","
            << emp.getBasicSalary() << "," << emp.getAllowance() << "," << emp.getDeductions() << ","
            << emp.calculateNetPay() << ","
            << emp.getPresentDays() << "," << emp.getAbsentDays() << "," << emp.getLeaveDays() << ","
            << emp.getAttendancePercent() << "\n";
    }
    f.close();
    QMessageBox::information(this, "Exported", "Data exported successfully.");
}

// ──────────────────────────────────────────────────────────────────────
//  Reports
// ──────────────────────────────────────────────────────────────────────
QString Dashboard::buildFullReport() {
    auto& all = db.pullAllRecords();
    QString r;
    r += "╔══════════════════════════════════════════════════════╗\n";
    r += "        ENTERPRISE EMS — FULL WORKFORCE REPORT\n";
    r += QString("        Generated: %1\n").arg(QDateTime::currentDateTime().toString("dd-MMM-yyyy hh:mm AP"));
    r += "╚══════════════════════════════════════════════════════╝\n\n";

    r += QString("  Total Employees    : %1\n").arg(all.size());
    r += QString("  Active Employees   : %1\n").arg(db.getActiveCount());
    r += QString("  On Leave           : %1\n").arg(db.getOnLeaveCount());
    r += QString("  Total Payroll      : Rs. %1\n").arg(db.getTotalPayroll(), 0, 'f', 2);
    r += QString("  Average Salary     : Rs. %1\n").arg(db.getAverageSalary(), 0, 'f', 2);
    r += QString("  Avg Attendance     : %1%%\n\n").arg(db.getAverageAttendance(), 0, 'f', 1);

    r += "──────────────────────────────────────────────────────\n";
    r += "  DEPARTMENT BREAKDOWN\n";
    r += "──────────────────────────────────────────────────────\n";
    for (const auto& ds : db.getDepartmentBreakdown()) {
        r += QString("  %-22s  %2 staff   Avg: Rs.%3   Att: %4%%\n")
             .arg(ds.dept).arg(ds.headCount)
             .arg(ds.avgSalary, 0, 'f', 0)
             .arg(ds.avgAttendance, 0, 'f', 1);
    }
    return r;
}

QString Dashboard::buildDeptReport() {
    QString r = "╔══════════════════════════════════════╗\n"
                "        DEPARTMENT REPORT\n"
                "╚══════════════════════════════════════╝\n\n";
    for (const auto& ds : db.getDepartmentBreakdown()) {
        r += QString("  Department: %1\n"
                     "  Headcount : %2 employees\n"
                     "  Payroll   : Rs. %3\n"
                     "  Avg Salary: Rs. %4\n"
                     "  Avg Attend: %5%%\n\n")
             .arg(ds.dept).arg(ds.headCount)
             .arg(ds.totalPayroll, 0, 'f', 2)
             .arg(ds.avgSalary, 0, 'f', 2)
             .arg(ds.avgAttendance, 0, 'f', 1);
        r += "  ──────────────────────────────────\n";
    }
    return r;
}

QString Dashboard::buildAttendanceReport() {
    auto& all = db.pullAllRecords();
    QString r = "╔══════════════════════════════════════╗\n"
                "         ATTENDANCE REPORT\n"
                "╚══════════════════════════════════════╝\n\n";
    r += QString("  Average Attendance : %1%%\n\n").arg(db.getAverageAttendance(), 0, 'f', 1);

    r += "  LOW ATTENDANCE (< 75%)\n";
    r += "  ──────────────────────────────────\n";
    bool anyLow = false;
    for (const auto& emp : all) {
        if (emp.getAttendancePercent() < 75) {
            r += QString("  %-22s  %1%%  (P:%2 A:%3 L:%4)\n")
                 .arg(emp.getName())
                 .arg(emp.getAttendancePercent(), 0, 'f', 1)
                 .arg(emp.getPresentDays())
                 .arg(emp.getAbsentDays())
                 .arg(emp.getLeaveDays());
            anyLow = true;
        }
    }
    if (!anyLow) r += "  No employees below 75% attendance.\n";

    r += "\n  HIGH ATTENDANCE (>= 95%)\n";
    r += "  ──────────────────────────────────\n";
    bool anyHigh = false;
    for (const auto& emp : all) {
        if (emp.getAttendancePercent() >= 95) {
            r += QString("  %-22s  %1%%\n")
                 .arg(emp.getName()).arg(emp.getAttendancePercent(), 0, 'f', 1);
            anyHigh = true;
        }
    }
    if (!anyHigh) r += "  No employees above 95% attendance.\n";
    return r;
}

QString Dashboard::buildPayrollReport() {
    auto& all = db.pullAllRecords();
    QString r = "╔══════════════════════════════════════╗\n"
                "           PAYROLL REPORT\n"
                "╚══════════════════════════════════════╝\n\n";
    r += QString("  Total Payroll  : Rs. %1\n"
                 "  Average Salary : Rs. %2\n\n")
         .arg(db.getTotalPayroll(), 0, 'f', 2)
         .arg(db.getAverageSalary(), 0, 'f', 2);

    r += "  ID     NAME                   NET PAY\n";
    r += "  ─────────────────────────────────────────\n";
    std::vector<Employee> sorted(all.begin(), all.end());
    std::sort(sorted.begin(), sorted.end(),
        [](const Employee& a, const Employee& b){ return a.calculateNetPay() > b.calculateNetPay(); });
    for (const auto& emp : sorted) {
        r += QString("  %1  %-22s  Rs. %2\n")
             .arg(emp.getId(), 6)
             .arg(emp.getName())
             .arg(emp.calculateNetPay(), 0, 'f', 2);
    }
    return r;
}

void Dashboard::onGenerateFullReport()       { reportOutput->setText(buildFullReport()); }
void Dashboard::onGenerateDeptReport()       { reportOutput->setText(buildDeptReport()); }
void Dashboard::onGenerateAttendanceReport() { reportOutput->setText(buildAttendanceReport()); }
void Dashboard::onGeneratePayrollReport()    { reportOutput->setText(buildPayrollReport()); }

void Dashboard::onExportReport() {
    QString path = QFileDialog::getSaveFileName(this, "Export Report", "", "Text Files (*.txt)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&f);
    out << reportOutput->toPlainText();
    f.close();
    QMessageBox::information(this, "Exported", "Report exported successfully.");
}

// ──────────────────────────────────────────────────────────────────────
//  Admin: User Management
// ──────────────────────────────────────────────────────────────────────
void Dashboard::onAddUser() {
    QDialog dlg(this);
    dlg.setWindowTitle("Add System User");
    dlg.setFixedSize(400, 380);
    dlg.setStyleSheet("background:#F8FAFC; font-family:'Segoe UI';");

    QVBoxLayout* lay = new QVBoxLayout(&dlg);
    lay->setContentsMargins(28, 24, 28, 20);
    lay->setSpacing(12);

    QLabel* t = new QLabel("Add New User", &dlg);
    t->setStyleSheet("font-size:18px; font-weight:800; color:#0F172A; font-family:'Georgia',serif;");
    lay->addWidget(t);

    QFormLayout* form = new QFormLayout(); form->setSpacing(10);

    auto* fUser = new QLineEdit(&dlg); fUser->setPlaceholderText("e.g. john.doe");
    fUser->setStyleSheet("QLineEdit { padding:9px; border:1.5px solid #E2E8F0; border-radius:6px; }");
    form->addRow("Username *:", fUser);

    auto* fPwd = new QLineEdit(&dlg); fPwd->setPlaceholderText("Minimum 6 characters");
    fPwd->setEchoMode(QLineEdit::Password);
    fPwd->setStyleSheet(fUser->styleSheet());
    form->addRow("Password *:", fPwd);

    auto* fFull = new QLineEdit(&dlg); fFull->setPlaceholderText("Full display name");
    fFull->setStyleSheet(fUser->styleSheet());
    form->addRow("Full Name:", fFull);

    auto* fEmail = new QLineEdit(&dlg); fEmail->setPlaceholderText("user@company.com");
    fEmail->setStyleSheet(fUser->styleSheet());
    form->addRow("Email:", fEmail);

    auto* cboRole = new QComboBox(&dlg);
    cboRole->addItems({"viewer", "hr", "admin"});
    cboRole->setStyleSheet("QComboBox { padding:8px; border:1.5px solid #E2E8F0; border-radius:6px; }");
    form->addRow("Role:", cboRole);

    lay->addLayout(form);
    lay->addStretch();

    QHBoxLayout* br = new QHBoxLayout();
    auto* bCancel = makeActionBtn("Cancel", "#FFFFFF", "#334155", "1px solid #CBD5E1", &dlg);
    auto* bSave   = makeActionBtn("Add User", "#1E3A5F", "#FFFFFF", "none", &dlg);
    br->addStretch(); br->addWidget(bCancel); br->addWidget(bSave);
    lay->addLayout(br);

    connect(bCancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(bSave, &QPushButton::clicked, [&](){
        if (fUser->text().trimmed().isEmpty() || fPwd->text().length() < 6) {
            QMessageBox::warning(&dlg, "Validation", "Username required, password min 6 chars.");
            return;
        }
        bool ok = userDB.addUser(fUser->text().trimmed(), fPwd->text(),
                                 cboRole->currentText(),
                                 fFull->text().trimmed(),
                                 fEmail->text().trimmed());
        if (!ok) {
            QMessageBox::warning(&dlg, "Error", "Username already exists.");
            return;
        }
        dlg.accept();
    });

    if (dlg.exec() == QDialog::Accepted) {
        reloadUsersGrid();
        QMessageBox::information(this, "Success", "User added successfully.");
    }
}

void Dashboard::onRemoveUser() {
    int row = usersGrid->currentRow();
    if (row < 0) { QMessageBox::warning(this, "No Selection", "Select a user to remove."); return; }

    QString username = usersGrid->item(row, 0)->text();

    // Prevent removing own account
    if (username == liveSessionUser) {
        QMessageBox::warning(this, "Error", "You cannot remove your own account.");
        return;
    }

    if (QMessageBox::question(this, "Confirm",
        QString("Remove user: %1?").arg(username),
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        userDB.removeUser(username);
        reloadUsersGrid();
    }
}

void Dashboard::onChangePassword() {
    int row = usersGrid->currentRow();
    if (row < 0) { QMessageBox::warning(this, "No Selection", "Select a user."); return; }
    QString username = usersGrid->item(row, 0)->text();

    bool ok;
    QString newPwd = QInputDialog::getText(this, "Change Password",
        QString("New password for '%1':").arg(username),
        QLineEdit::Password, "", &ok);

    if (!ok || newPwd.isEmpty()) return;
    if (newPwd.length() < 6) { QMessageBox::warning(this, "Error", "Password must be 6+ characters."); return; }

    userDB.changePassword(username, newPwd);
    QMessageBox::information(this, "Success", "Password updated.");
}

// ══════════════════════════════════════════════════════════════════════════════
//  NEW FEATURES — Print, PDF Export, Fingerprint Biometric
// ══════════════════════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: print any plain-text content via Qt PrintDialog
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: print any plain-text content via Qt PrintDialog
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::printTextContent(const QString& title, const QString& content) {
    QPrinter printer(QPrinter::HighResolution);
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);

    QPrintDialog dlg(&printer, this);
    dlg.setWindowTitle("Print — " + title);
    if (dlg.exec() != QDialog::Accepted) return;

    QPainter painter(&printer);
    if (!painter.isActive()) return;

    // ── Correct DPI-aware font sizing ─────────────────────────────────────
    // Use DevicePixel coordinates and convert pt → px using printer DPI.
    // This is the only reliable way: point-size fonts scale by DPI, so
    // setPixelSize() pins them to the exact physical size we want.
    const qreal dpiY = printer.logicalDpiY();
    const qreal dpiX = printer.logicalDpiX();
    auto pxY = [&](qreal pt) { return pt * dpiY / 72.0; };
    auto pxX = [&](qreal pt) { return pt * dpiX / 72.0; };

    const QRectF page = QRectF(printer.pageRect(QPrinter::DevicePixel));

    // Header font (9pt physical)
    QFont headerFont("Segoe UI", 1, QFont::Bold);
    headerFont.setPixelSize(qRound(pxY(9)));
    painter.setFont(headerFont);
    QFontMetricsF hfm(headerFont);
    qreal hLineH = hfm.height() + pxY(2);

    QString headerText = QString("%1   |   Page %2   |   %3")
        .arg(title).arg(1)
        .arg(QDateTime::currentDateTime().toString("dd MMM yyyy  hh:mm AP"));

    painter.setPen(QColor("#334155"));
    painter.drawText(QRectF(page.left(), page.top(), page.width(), hLineH),
                     Qt::AlignLeft | Qt::AlignVCenter, headerText);

    // Divider line
    painter.setPen(QPen(QColor("#CBD5E1"), pxX(0.5)));
    painter.drawLine(QPointF(page.left(),  page.top() + hLineH + pxY(2)),
                     QPointF(page.right(), page.top() + hLineH + pxY(2)));

    // Body font (8pt Courier New physical)
    QFont bodyFont("Courier New", 1);
    bodyFont.setPixelSize(qRound(pxY(8)));
    painter.setFont(bodyFont);
    QFontMetricsF bfm(bodyFont);

    qreal lineH    = bfm.height() + pxY(2);
    qreal bodyTop  = page.top() + hLineH + pxY(8);
    qreal footH    = pxY(20);
    qreal bodyH    = page.height() - hLineH - pxY(8) - footH;
    int linesPerPg = static_cast<int>(bodyH / lineH);
    if (linesPerPg < 1) linesPerPg = 1;

    QStringList lines = content.split('\n');
    int lineIdx = 0;
    int pageNum = 1;

    while (lineIdx < lines.size()) {
        if (pageNum > 1) {
            printer.newPage();
            painter.setFont(headerFont);
            QString hdr = QString("%1   |   Page %2   |   %3")
                .arg(title).arg(pageNum)
                .arg(QDateTime::currentDateTime().toString("dd MMM yyyy  hh:mm AP"));
            painter.setPen(QColor("#334155"));
            painter.drawText(QRectF(page.left(), page.top(), page.width(), hLineH),
                             Qt::AlignLeft | Qt::AlignVCenter, hdr);
            painter.setPen(QPen(QColor("#CBD5E1"), pxX(0.5)));
            painter.drawLine(QPointF(page.left(),  page.top() + hLineH + pxY(2)),
                             QPointF(page.right(), page.top() + hLineH + pxY(2)));
            painter.setFont(bodyFont);
        }
        pageNum++;

        painter.setPen(QColor("#334155"));
        qreal y = bodyTop;
        for (int i = 0; i < linesPerPg && lineIdx < lines.size(); ++i, ++lineIdx) {
            painter.drawText(QPointF(page.left(), y + bfm.ascent()), lines[lineIdx]);
            y += lineH;
        }

        // Footer
        QFont footFont("Segoe UI", 1);
        footFont.setPixelSize(qRound(pxY(7)));
        painter.setFont(footFont);
        painter.setPen(QColor("#94A3B8"));
        qreal footY = page.bottom() - footH;
        painter.drawLine(QPointF(page.left(), footY), QPointF(page.right(), footY));
        painter.drawText(QRectF(page.left(), footY + pxY(2), page.width(), footH - pxY(2)),
                         Qt::AlignCenter,
                         QString("EMS Core — Confidential | Page %1").arg(pageNum - 1));
        painter.setFont(bodyFont);
        painter.setPen(QColor("#334155"));
    }
    painter.end();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: print QTableWidget
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::printTableWidget(const QString& title, QTableWidget* table) {
    if (!table) return;

    QPrinter printer(QPrinter::HighResolution);
    printer.setPageOrientation(QPageLayout::Landscape);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(10, 10, 10, 10), QPageLayout::Millimeter);

    QPrintDialog dlg(&printer, this);
    dlg.setWindowTitle("Print — " + title);
    if (dlg.exec() != QDialog::Accepted) return;

    QPainter painter(&printer);
    if (!painter.isActive()) return;

    // ── Correct DPI-aware font sizing ─────────────────────────────────────
    const qreal dpiY = printer.logicalDpiY();
    const qreal dpiX = printer.logicalDpiX();
    auto pxY = [&](qreal pt) { return pt * dpiY / 72.0; };
    auto pxX = [&](qreal pt) { return pt * dpiX / 72.0; };

    const QRectF pageRect = QRectF(printer.pageRect(QPrinter::DevicePixel));

    // ── Collect data first ────────────────────────────────────────────────
    QStringList headers;
    for (int c = 0; c < table->columnCount(); c++)
        headers << (table->horizontalHeaderItem(c) ? table->horizontalHeaderItem(c)->text() : "");

    QVector<int> colWidths(table->columnCount(), 0);
    for (int c = 0; c < table->columnCount(); c++) {
        colWidths[c] = headers[c].length();
        for (int r = 0; r < table->rowCount(); r++) {
            int len = table->item(r, c) ? table->item(r, c)->text().length() : 0;
            colWidths[c] = qMax(colWidths[c], len);
        }
    }
    int totalChars = 0;
    for (int w : colWidths) totalChars += w;
    if (totalChars == 0) totalChars = 1;

    // ── Fonts ─────────────────────────────────────────────────────────────
    QFont titleFont("Segoe UI", 1, QFont::Bold);
    titleFont.setPixelSize(qRound(pxY(11)));

    QFont headerFont("Segoe UI", 1, QFont::Bold);
    headerFont.setPixelSize(qRound(pxY(8)));

    QFont bodyFont("Courier New", 1);
    bodyFont.setPixelSize(qRound(pxY(7)));

    QFont subFont("Segoe UI", 1);
    subFont.setPixelSize(qRound(pxY(7)));

    painter.setFont(titleFont);
    QFontMetricsF tfm(titleFont);
    qreal titleH = tfm.height() + pxY(4);

    painter.setFont(bodyFont);
    QFontMetricsF bfm(bodyFont);
    qreal rowH = bfm.height() + pxY(6);

    qreal x0 = pageRect.left();
    qreal y  = pageRect.top();

    // ── Title ─────────────────────────────────────────────────────────────
    painter.setFont(titleFont);
    painter.setPen(QColor("#0F172A"));
    painter.drawText(QRectF(x0, y, pageRect.width(), titleH),
                     Qt::AlignLeft | Qt::AlignVCenter, title);
    y += titleH;

    painter.setFont(subFont);
    painter.setPen(QColor("#64748B"));
    painter.drawText(QRectF(x0, y, pageRect.width(), pxY(14)),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     "Generated: " + QDateTime::currentDateTime()
                         .toString("dd MMM yyyy  hh:mm AP"));
    y += pxY(16);

    // Divider
    painter.setPen(QPen(QColor("#CBD5E1"), pxX(0.5)));
    painter.drawLine(QPointF(x0, y), QPointF(pageRect.right(), y));
    y += pxY(6);

    // ── Calculate column x-positions ──────────────────────────────────────
    QVector<qreal> colX(table->columnCount());
    QVector<qreal> colW(table->columnCount());
    qreal usableW = pageRect.width();
    qreal cur = x0;
    for (int c = 0; c < table->columnCount(); c++) {
        colW[c] = usableW * colWidths[c] / static_cast<qreal>(totalChars);
        colX[c] = cur;
        cur += colW[c];
    }

    // ── Header row ────────────────────────────────────────────────────────
    painter.setBrush(QColor("#F1F5F9"));
    painter.setPen(Qt::NoPen);
    painter.drawRect(QRectF(x0, y, usableW, rowH));

    painter.setFont(headerFont);
    painter.setPen(QColor("#334155"));
    for (int c = 0; c < table->columnCount(); c++) {
        painter.drawText(
            QRectF(colX[c] + pxX(3), y, colW[c] - pxX(6), rowH),
            Qt::AlignLeft | Qt::AlignVCenter,
            headers[c]);
    }
    y += rowH;

    // ── Data rows ─────────────────────────────────────────────────────────
    painter.setFont(bodyFont);
    int page = 1;
    qreal footH = pxY(20);

    for (int r = 0; r < table->rowCount(); r++) {
        if (y + rowH > pageRect.bottom() - footH) {
            // Footer
            QFont fFont("Segoe UI", 1);
            fFont.setPixelSize(qRound(pxY(7)));
            painter.setFont(fFont);
            painter.setPen(QColor("#94A3B8"));
            painter.drawLine(QPointF(x0, pageRect.bottom() - footH),
                             QPointF(pageRect.right(), pageRect.bottom() - footH));
            painter.drawText(
                QRectF(x0, pageRect.bottom() - footH + pxY(2), usableW, footH - pxY(2)),
                Qt::AlignCenter,
                QString("EMS Core — %1 | Page %2 | Confidential").arg(title).arg(page));

            printer.newPage();
            page++;
            y = pageRect.top();

            // Repeat column header
            painter.setBrush(QColor("#F1F5F9"));
            painter.setPen(Qt::NoPen);
            painter.drawRect(QRectF(x0, y, usableW, rowH));
            painter.setFont(headerFont);
            painter.setPen(QColor("#334155"));
            for (int c = 0; c < table->columnCount(); c++) {
                painter.drawText(
                    QRectF(colX[c] + pxX(3), y, colW[c] - pxX(6), rowH),
                    Qt::AlignLeft | Qt::AlignVCenter, headers[c]);
            }
            y += rowH;
            painter.setFont(bodyFont);
        }

        if (r % 2 == 0) {
            painter.setBrush(QColor("#F8FAFC"));
            painter.setPen(Qt::NoPen);
            painter.drawRect(QRectF(x0, y, usableW, rowH));
        }

        painter.setPen(QColor("#334155"));
        for (int c = 0; c < table->columnCount(); c++) {
            QString cellText = table->item(r, c) ? table->item(r, c)->text() : "";
            painter.drawText(
                QRectF(colX[c] + pxX(3), y, colW[c] - pxX(6), rowH),
                Qt::AlignLeft | Qt::AlignVCenter,
                cellText);
        }

        painter.setPen(QPen(QColor("#F1F5F9"), pxX(0.5)));
        painter.drawLine(QPointF(x0, y + rowH), QPointF(pageRect.right(), y + rowH));
        y += rowH;
    }

    // Final footer
    QFont fFont("Segoe UI", 1);
    fFont.setPixelSize(qRound(pxY(7)));
    painter.setFont(fFont);
    painter.setPen(QColor("#94A3B8"));
    painter.drawLine(QPointF(x0, pageRect.bottom() - footH),
                     QPointF(pageRect.right(), pageRect.bottom() - footH));
    painter.drawText(
        QRectF(x0, pageRect.bottom() - footH + pxY(2), usableW, footH - pxY(2)),
        Qt::AlignCenter,
        QString("EMS Core — %1 | Page %2 | Confidential").arg(title).arg(page));

    painter.end();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: export text content to PDF
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::exportContentToPDF(const QString& title, const QString& content) {
    QString path = QFileDialog::getSaveFileName(
        this, "Save PDF — " + title, title + ".pdf", "PDF Files (*.pdf)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".pdf", Qt::CaseInsensitive)) path += ".pdf";

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);

    QPainter painter(&printer);
    if (!painter.isActive()) {
        QMessageBox::critical(this, "PDF Error",
            "Could not create PDF file.\nCheck the file path and permissions.");
        return;
    }

    // ── Correct DPI-aware font sizing ─────────────────────────────────────
    // QPrinter in PdfFormat reports logicalDpi of 1200. Using setPixelSize()
    // converted from pt pins fonts to their correct physical size regardless
    // of the printer's internal DPI. Never use painter.scale() + point sizes
    // together — that was the root cause of the oversized/garbled output.
    const qreal dpiY = printer.logicalDpiY();
    const qreal dpiX = printer.logicalDpiX();
    auto pxY = [&](qreal pt) { return pt * dpiY / 72.0; };
    auto pxX = [&](qreal pt) { return pt * dpiX / 72.0; };

    const QRectF pageRect = QRectF(printer.pageRect(QPrinter::DevicePixel));

    // ── Styled header block ───────────────────────────────────────────────
    qreal headerH = pxY(50);
    painter.setBrush(QColor(0x0F, 0x17, 0x2A));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(QRectF(pageRect.left(), pageRect.top(),
                                   pageRect.width(), headerH), pxX(5), pxY(5));

    QFont titleFont("Segoe UI", 1, QFont::Bold);
    titleFont.setPixelSize(qRound(pxY(13)));
    painter.setFont(titleFont);
    painter.setPen(Qt::white);
    painter.drawText(
        QRectF(pageRect.left() + pxX(12), pageRect.top() + pxY(6),
               pageRect.width() - pxX(24), pxY(28)),
        Qt::AlignLeft | Qt::AlignVCenter,
        "Employee Management System — " + title);

    QFont subFont("Segoe UI", 1);
    subFont.setPixelSize(qRound(pxY(8)));
    painter.setFont(subFont);
    painter.setPen(QColor(0x94, 0xA3, 0xB8));
    painter.drawText(
        QRectF(pageRect.left() + pxX(12), pageRect.top() + pxY(34),
               pageRect.width() - pxX(24), pxY(14)),
        Qt::AlignLeft | Qt::AlignVCenter,
        "Generated: " + QDateTime::currentDateTime()
            .toString("dd MMM yyyy  hh:mm AP") + "   |   Confidential");

    // ── Content area ─────────────────────────────────────────────────────
    QFont bodyFont("Courier New", 1);
    bodyFont.setPixelSize(qRound(pxY(8)));
    painter.setFont(bodyFont);
    painter.setPen(QColor(0x33, 0x41, 0x55));

    QFontMetricsF bfm(bodyFont);
    qreal lineH    = bfm.height() + pxY(3);
    qreal footH    = pxY(20);
    qreal bodyTop  = pageRect.top() + headerH + pxY(10);
    qreal bodyH    = pageRect.height() - headerH - pxY(10) - footH;
    int linesPerPg = static_cast<int>(bodyH / lineH);
    if (linesPerPg < 1) linesPerPg = 1;

    QStringList lines = content.split('\n');
    int lineIdx = 0, pageNum = 0;

    auto drawFooter = [&](int pg) {
        QFont fFont("Segoe UI", 1);
        fFont.setPixelSize(qRound(pxY(7)));
        painter.setFont(fFont);
        painter.setPen(QColor(0x94, 0xA3, 0xB8));
        qreal footY = pageRect.bottom() - footH;
        painter.drawLine(QPointF(pageRect.left(), footY),
                         QPointF(pageRect.right(), footY));
        painter.drawText(
            QRectF(pageRect.left(), footY + pxY(4), pageRect.width(), footH - pxY(4)),
            Qt::AlignCenter,
            QString("EMS Core  |  Page %1  |  Confidential").arg(pg));
        painter.setFont(bodyFont);
        painter.setPen(QColor(0x33, 0x41, 0x55));
    };

    while (lineIdx < lines.size()) {
        if (pageNum > 0) {
            printer.newPage();
            bodyTop = pageRect.top() + pxY(10);
        }
        pageNum++;

        qreal y = bodyTop;
        for (int i = 0; i < linesPerPg && lineIdx < lines.size(); ++i, ++lineIdx) {
            painter.drawText(QPointF(pageRect.left(), y + bfm.ascent()), lines[lineIdx]);
            y += lineH;
        }
        drawFooter(pageNum);
    }

    painter.end();
    QMessageBox::information(this, "PDF Saved",
        QString("Report exported:\n%1").arg(path));
}

void Dashboard::onPrintEmployeeList() {
    printTableWidget("Employee Directory", mainGrid);
}

// ─────────────────────────────────────────────────────────────────────────────
//  SLOT: Print current Report
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::onPrintReport() {
    printTextContent("EMS Report", reportOutput->toPlainText());
}

// ─────────────────────────────────────────────────────────────────────────────
//  SLOT: Export current Report to PDF
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::onExportReportPDF() {
    exportContentToPDF("EMS Report", reportOutput->toPlainText());
}

// ─────────────────────────────────────────────────────────────────────────────
//  SLOT: Fingerprint Biometric Attendance
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::onFingerprintScan() {
    int row = mainGrid->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "No Selection",
            "Please select an employee row before scanning fingerprint.");
        return;
    }

    int id = mainGrid->item(row, 0)->text().toInt();
    QString name = mainGrid->item(row, 1)->text();

    Employee* emp = db.lookupRecord(id);
    if (!emp) return;

    FingerprintDialog scanDlg(id, name, this);
    if (scanDlg.exec() == QDialog::Accepted && scanDlg.wasVerified()) {
        QString mark = scanDlg.markType();
        if (mark == "Present")       emp->logPresent();
        else if (mark == "Absent")   emp->logAbsent();
        else                         emp->logLeave();

        db.modifyRecord(id, *emp);
        reloadViewGrid();

        QMessageBox::information(this, "Biometric Success",
            QString("✅  Fingerprint verified!\n\nEmployee: %1\nAttendance marked as: %2")
            .arg(name).arg(mark));
    }
}
