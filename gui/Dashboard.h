#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QTabWidget>
#include <QStackedWidget>
#include <QTextEdit>
#include "../lib/Database.h"
#include "../lib/UserDatabase.h"

class Dashboard : public QWidget {
    Q_OBJECT

private:
    Database     db;
    UserDatabase userDB;
    QString      liveSessionRole;
    QString      liveSessionUser;

    // ── Main layout ──────────────────────────────────────────────
    QWidget*      navSidebar;
    QStackedWidget* pageStack;

    // ── Employees Page ───────────────────────────────────────────
    QTableWidget* mainGrid;
    QLineEdit*    barSearch;
    QComboBox*    cboDeptFilter;
    QComboBox*    cboStatusFilter;
    QComboBox*    cboSortBy;

    QLabel* metricStaff;
    QLabel* metricPayroll;
    QLabel* metricAttendance;
    QLabel* metricActive;

    QPushButton* btnOnboard;
    QPushButton* btnModify;
    QPushButton* btnPurge;
    QPushButton* btnBiometrics;
    QPushButton* btnFingerprintScan;   // NEW — fingerprint biometric
    QPushButton* btnSlipDisplay;
    QPushButton* btnExportCSV;
    QPushButton* btnPrintList;         // NEW — print employee list
    QPushButton* btnAdvFilter;

    // ── Reports Page ─────────────────────────────────────────────
    QTextEdit*   reportOutput;
    QTableWidget* deptStatsGrid;
    QPushButton* btnPrintReport;       // NEW — print report
    QPushButton* btnExportPDF;         // NEW — export report to PDF

    // ── Admin Users Page ─────────────────────────────────────────
    QTableWidget* usersGrid;
    QPushButton*  btnAddUser;
    QPushButton*  btnRemoveUser;
    QPushButton*  btnChangePass;

    // ── Navigation buttons ───────────────────────────────────────
    QPushButton* navBtnEmployees;
    QPushButton* navBtnReports;
    QPushButton* navBtnAdmin;
    QPushButton* navBtnLogout;

    void buildLayout();
    void buildSidebar();
    void buildEmployeesPage(QWidget* page);
    void buildReportsPage(QWidget* page);
    void buildAdminPage(QWidget* page);

    void populateGrid(const std::vector<Employee>& list);
    void applyFiltersAndRefresh();

    QString buildFullReport();
    QString buildDeptReport();
    QString buildAttendanceReport();
    QString buildPayrollReport();

    // NEW — shared print/PDF helpers
    void printTextContent(const QString& title, const QString& content);
    void printTableWidget(const QString& title, QTableWidget* table);
    void exportContentToPDF(const QString& title, const QString& content);

public:
    explicit Dashboard(const QString& role, const QString& username, QWidget* parent = nullptr);
    void reloadViewGrid();
    void computeMetrics();
    void reloadUsersGrid();
    void reloadDeptStats();

private slots:
    void executionAddWorkflow();
    void executionUpdateWorkflow();
    void executionDeleteWorkflow();
    void executionAttendanceWorkflow();
    void executionPaySlipWorkflow();
    void executionExportWorkflow();
    void executionLiveSearch(const QString& text);
    void executionAdvancedFilter();
    void executionSortChanged(int idx);

    void onNavEmployees();
    void onNavReports();
    void onNavAdmin();
    void onNavLogout();

    void onGenerateFullReport();
    void onGenerateDeptReport();
    void onGenerateAttendanceReport();
    void onGeneratePayrollReport();
    void onExportReport();

    void onAddUser();
    void onRemoveUser();
    void onChangePassword();

    // NEW slots
    void onFingerprintScan();        // fingerprint biometric attendance
    void onPrintEmployeeList();      // print the employee table
    void onPrintReport();            // print the current report text
    void onExportReportPDF();        // export report to PDF file
};

#endif // DASHBOARD_H
