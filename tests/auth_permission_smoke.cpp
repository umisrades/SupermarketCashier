#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTableWidget>
#include <QToolButton>

#include "../src/data/databasemanager.h"
#include "../src/ui/cashierwindow.h"
#include "../src/ui/inventory_widget.h"
#include "../src/ui/logindialog.h"

namespace {

bool require(
    bool condition,
    const QString &message)
{
    if (!condition)
    {
        QTextStream(stderr)
            << "FAILED: " << message << Qt::endl;
        return false;
    }

    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QTemporaryDir temporaryDirectory;

    if (!require(
            temporaryDirectory.isValid(),
            "无法创建临时测试目录。")
        || !require(
            QDir::setCurrent(temporaryDirectory.path()),
            "无法切换到临时测试目录。"))
    {
        return 1;
    }

    DatabaseManager &database =
        DatabaseManager::instance();

    if (!require(
            database.initDatabase("goods.db"),
            database.lastError()))
    {
        return 1;
    }

    const QString resourceDatabasePath =
        QDir(QCoreApplication::applicationDirPath())
            .filePath("data/default_goods.db");

    if (!require(
            database.importProductsFromDatabase(
                resourceDatabasePath),
            database.lastError()))
    {
        return 1;
    }

    LoginDialog administratorLogin;
    QLineEdit *administratorNameEdit =
        administratorLogin.findChild<QLineEdit *>(
            "usernameEdit");
    QLineEdit *administratorPasswordEdit =
        administratorLogin.findChild<QLineEdit *>(
            "passwordEdit");

    if (!require(
            administratorNameEdit != nullptr
                && administratorPasswordEdit != nullptr,
            "登录窗口缺少用户名或密码输入框。"))
    {
        return 1;
    }

    administratorNameEdit->setText("boss");
    administratorPasswordEdit->setText("boss123");
    QMetaObject::invokeMethod(
        &administratorLogin,
        "on_loginBtn_clicked",
        Qt::DirectConnection);

    if (!require(
            administratorLogin.result()
                == QDialog::Accepted,
            "预留管理员账号必须能通过登录窗口。"))
    {
        return 1;
    }

    const User administrator =
        administratorLogin.authenticatedUser();

    LoginDialog cashierLogin;
    QLineEdit *cashierNameEdit =
        cashierLogin.findChild<QLineEdit *>(
            "usernameEdit");
    QLineEdit *cashierPasswordEdit =
        cashierLogin.findChild<QLineEdit *>(
            "passwordEdit");
    cashierNameEdit->setText("people");
    cashierPasswordEdit->setText("people123");
    QMetaObject::invokeMethod(
        &cashierLogin,
        "on_loginBtn_clicked",
        Qt::DirectConnection);

    if (!require(
            cashierLogin.result() == QDialog::Accepted,
            "预留收银员账号必须能通过登录窗口。"))
    {
        return 1;
    }

    const User cashier =
        cashierLogin.authenticatedUser();

    if (!require(
            !cashier.isAdministrator(),
            "登录窗口返回的 people 必须是收银员。")
        || !require(
            administrator.isAdministrator(),
            "登录窗口返回的 boss 必须是管理员。"))
    {
        return 1;
    }

    const Product barcodeProduct(
        "B001",
        "6901234567890",
        "条形码测试商品",
        "测试分类",
        "件",
        560,
        3,
        1,
        ProductStatus::OnSale);

    if (!require(
            database.addProduct(barcodeProduct),
            database.lastError()))
    {
        return 1;
    }

    CashierWindow window;
    QAction *inventoryAction =
        window.findChild<QAction *>(
            "actionInventoryManagement");
    QLabel *statusLabel =
        window.findChild<QLabel *>(
            "userStatusLabel");
    QLineEdit *barcodeInput =
        window.findChild<QLineEdit *>("priceInput");
    QTableWidget *cartTable =
        window.findChild<QTableWidget *>("tableWidget");
    QToolButton *resourceProductButton = nullptr;

    for (QToolButton *button
         : window.findChildren<QToolButton *>())
    {
        if (button->text() == "农夫山泉矿泉水")
        {
            resourceProductButton = button;
            break;
        }
    }

    if (!require(
            inventoryAction != nullptr,
            "未找到库存管理菜单动作。")
        || !require(
            statusLabel != nullptr,
            "未找到当前用户状态标签。")
        || !require(
            barcodeInput != nullptr
                && cartTable != nullptr,
            "未找到条形码输入框或购物车表格。")
        || !require(
            resourceProductButton != nullptr
                && !resourceProductButton->icon().isNull(),
            "资源商品按钮必须加载与商品编号匹配的图片。"))
    {
        return 1;
    }

    barcodeInput->setText("6901234567890");
    QMetaObject::invokeMethod(
        &window,
        "on_SearchandAdd_clicked",
        Qt::DirectConnection);

    if (!require(
            cartTable->rowCount() == 1,
            "输入有效条形码后必须加入一个购物车项目。")
        || !require(
            cartTable->item(0, 0) != nullptr
                && cartTable->item(0, 0)->text()
                    == "B001",
            "条形码查询后购物车必须保留对应商品编号。"))
    {
        return 1;
    }

    window.setCurrentUser(cashier);

    if (!require(
            !inventoryAction->isEnabled(),
            "收银员的库存管理菜单必须禁用。")
        || !require(
            statusLabel->text().contains("收银员"),
            "收银员登录状态必须显示在收银台。"))
    {
        return 1;
    }

    window.setCurrentUser(administrator);

    if (!require(
            inventoryAction->isEnabled(),
            "管理员的库存管理菜单必须启用。")
        || !require(
            statusLabel->text().contains("管理员"),
            "管理员登录状态必须显示在收银台。"))
    {
        return 1;
    }

    inventoryAction->trigger();
    application.processEvents();

    InventoryWidget *inventoryWindow =
        window.findChild<InventoryWidget *>();

    if (!require(
            inventoryWindow != nullptr,
            "管理员必须能够打开库存管理窗口。"))
    {
        return 1;
    }

    inventoryWindow->close();
    application.processEvents();

    QTextStream(stdout)
        << "PASS: AuthenticationAndPermissions"
        << Qt::endl;
    return 0;
}
