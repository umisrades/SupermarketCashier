#ifndef LOGIN_DIALOG_H
#define LOGIN_DIALOG_H

#include <QDialog>

#include "../models/user.h"

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog() override;

    const User &authenticatedUser() const;

private slots:
    void on_loginBtn_clicked();
    void on_cancelBtn_clicked();

private:
    Ui::LoginDialog *ui;
    User m_authenticatedUser;
};

#endif // LOGIN_DIALOG_H
