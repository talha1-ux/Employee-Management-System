#include <QApplication>
#include "gui/LoginWindow.h"
#include "gui/Dashboard.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("Employee Management System");
    app.setApplicationVersion("2.0");
    app.setOrganizationName("Employee Management System");

    // Set application font
    QFont appFont("Segoe UI", 10);
    app.setFont(appFont);

    LoginWindow loginUI;
    Dashboard*  dashboard = nullptr;

    QObject::connect(&loginUI, &LoginWindow::portalAccessGranted,
        [&](const QString& role, const QString& username) {
            dashboard = new Dashboard(role, username);
            dashboard->showMaximized();
            loginUI.close();
        });

    loginUI.show();
    return app.exec();
}
