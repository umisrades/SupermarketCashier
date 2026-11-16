#include "logindialog.h"
#include "ui_logindialog.h"

#include "../data/databasemanager.h"

#include <QLineEdit>
#include <QMessageBox>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    ui->usernameEdit->setFocus();

    connect(
        ui->passwordEdit,
        &QLineEdit::returnPressed,
        this,
        &LoginDialog::on_loginBtn_clicked);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

const User &LoginDialog::authenticatedUser() const
{
    return m_authenticatedUser;
}

void LoginDialog::on_loginBtn_clicked()
{
    const QString username =
        ui->usernameEdit->text().trimmed();
    const QString password =
        ui->passwordEdit->text().trimmed();

    if (username.isEmpty())
    {
        QMessageBox::warning(
            this,
            "提示",
            "请输入用户名");
        ui->usernameEdit->setFocus();
        return;
    }

    if (password.isEmpty())
    {
        QMessageBox::warning(
            this,
            "提示",
            "请输入密码");
        ui->passwordEdit->setFocus();
        return;
    }

    User authenticatedUser;
    DatabaseManager &database =
        DatabaseManager::instance();

    if (!database.authenticateUser(
            username,
            password,
            authenticatedUser))
    {
        QMessageBox::warning(
            this,
            "登录失败",
            database.lastError());
        ui->passwordEdit->clear();
        ui->passwordEdit->setFocus();
        return;
    }

    m_authenticatedUser = authenticatedUser;
    accept();
}

void LoginDialog::on_cancelBtn_clicked()
{
    reject();
}
