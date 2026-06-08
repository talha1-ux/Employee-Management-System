#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "../lib/UserDatabase.h"

class LoginWindow : public QWidget {
    Q_OBJECT

private:
    UserDatabase userDB;

    QLineEdit*   fieldUser;
    QLineEdit*   fieldPass;
    QPushButton* btnLogin;
    QLabel*      lblError;
    QLabel*      lblVersion;

    // Animation helpers
    QWidget*     cardFrame;

    void buildUI();
    void applyStyles();

public:
    explicit LoginWindow(QWidget* parent = nullptr);

signals:
    void portalAccessGranted(QString confirmedRole, QString username);

private slots:
    void handleLoginAttempt();
    void onUsernameReturn();
};

#endif // LOGINWINDOW_H
