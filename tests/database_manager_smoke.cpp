#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>
#include <QTextStream>

#include "../src/data/databasemanager.h"

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
    QCoreApplication application(argc, argv);
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

    User admin;

    if (!require(
            database.authenticateUser(
                "admin",
                "admin123",
                admin),
            database.lastError())
        || !require(
            admin.isAdministrator(),
            "默认 admin 必须是管理员。"))
    {
        return 1;
    }

    User cashier;

    if (!require(
            database.authenticateUser(
                "cashier",
                "cashier123",
                cashier),
            database.lastError())
        || !require(
            !cashier.isAdministrator(),
            "默认 cashier 必须是收银员。"))
    {
        return 1;
    }

    User reservedAdmin;
    User reservedCashier;

    if (!require(
            database.authenticateUser(
                "boss",
                "boss123",
                reservedAdmin),
            database.lastError())
        || !require(
            reservedAdmin.isAdministrator(),
            "预留 boss 账号必须是管理员。")
        || !require(
            database.authenticateUser(
                "people",
                "people123",
                reservedCashier),
            database.lastError())
        || !require(
            !reservedCashier.isAdministrator(),
            "预留 people 账号必须是收银员。"))
    {
        return 1;
    }

    const User newUser(
        0,
        "operator",
        "test123",
        UserRole::Cashier,
        AccountStatus::Enabled);

    if (!require(
            database.addUser(newUser),
            database.lastError())
        || !require(
            !database.addUser(newUser),
            "重复用户名必须被拒绝。"))
    {
        return 1;
    }

    User operatorUser;

    if (!require(
            database.authenticateUser(
                "operator",
                "test123",
                operatorUser),
            database.lastError())
        || !require(
            database.deleteUserByName("operator"),
            database.lastError())
        || !require(
            !database.authenticateUser(
                "operator",
                "test123",
                operatorUser),
            "删除的用户不能继续登录。"))
    {
        return 1;
    }

    const Product product(
        "C001",
        "880000000001",
        "控制台商品",
        1290,
        8,
        2,
        ProductStatus::Active,
        "2026-07-13 15:30:00");

    if (!require(
            database.addProduct(product),
            database.lastError()))
    {
        return 1;
    }

    Product loadedById;
    Product loadedByBarcode;

    if (!require(
            database.getProductById(
                "C001",
                loadedById),
            database.lastError())
        || !require(
            database.getProductByBarcode(
                "880000000001",
                loadedByBarcode),
            database.lastError())
        || !require(
            loadedById.category() == "未分类"
                && loadedById.unit() == "件"
                && loadedById.status()
                    == ProductStatus::OnSale,
            "控制台商品必须映射到共享商品模型。"))
    {
        return 1;
    }

    if (!require(
            database.updateProductStock("C001", 5),
            database.lastError())
        || !require(
            database.getProductById(
                "C001",
                loadedById),
            database.lastError())
        || !require(
            loadedById.stock() == 5
                && !loadedById.updateTime().isEmpty(),
            "控制台库存修改必须同步库存和更新时间。")
        || !require(
            database.deleteProduct("C001"),
            database.lastError())
        || !require(
            database.getAllProducts().isEmpty(),
            "删除控制台商品后共享商品表应为空。"))
    {
        return 1;
    }

    QTextStream(stdout)
        << "PASS: DatabaseManager" << Qt::endl;
    return 0;
}
