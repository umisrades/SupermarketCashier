#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

#include "src/data/databasemanager.h"
#include "src/ui/cashierwindow.h"
#include "src/ui/logindialog.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    DatabaseManager &database =
        DatabaseManager::instance();

    if (!database.initDatabase("goods.db"))
    {
        QMessageBox::critical(
            nullptr,
            "数据库错误",
            database.lastError());
        return 1;
    }

    const QString productResourceDatabase =
        QDir(QCoreApplication::applicationDirPath())
            .filePath("data/default_goods.db");

    if (QFileInfo::exists(productResourceDatabase)
        && !database.importProductsFromDatabase(
            productResourceDatabase))
    {
        QMessageBox::warning(
            nullptr,
            "商品资源导入提示",
            database.lastError());
    }

    LoginDialog loginDialog;

    if (loginDialog.exec() != QDialog::Accepted)
    {
        return 0;
    }

    CashierWindow cashierWindow;
    cashierWindow.setCurrentUser(
        loginDialog.authenticatedUser());
    cashierWindow.show();

    return app.exec();
}
