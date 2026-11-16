#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>

#include <cstdlib>
#include <iostream>
#include <string>

#include "../data/databasemanager.h"

namespace {

void clearInputBuffer()
{
    std::cin.clear();
    std::cin.ignore(32767, '\n');
}

} // namespace

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    std::system("chcp 65001 > nul");
#endif

    QCoreApplication application(argc, argv);

    DatabaseManager &database =
        DatabaseManager::instance();

    if (!database.initDatabase("goods.db"))
    {
        std::cout
            << "数据库初始化失败："
            << database.lastError().toStdString()
            << std::endl;
        return -1;
    }

    const QString productResourceDatabase =
        QDir(QCoreApplication::applicationDirPath())
            .filePath("data/default_goods.db");

    if (QFileInfo::exists(productResourceDatabase)
        && !database.importProductsFromDatabase(
            productResourceDatabase))
    {
        std::cout
            << "商品资源导入失败："
            << database.lastError().toStdString()
            << std::endl;
    }

    std::cout
        << "========================================\n"
        << "       欢迎使用超市库存管理系统\n"
        << "========================================"
        << std::endl;

    User loggedInUser;
    bool loginSuccess = false;

    while (!loginSuccess)
    {
        std::string username;
        std::string password;

        std::cout << "\n[系统登录]" << std::endl;
        std::cout << "请输入用户名: ";
        std::cin >> username;
        std::cout << "请输入密码  : ";
        std::cin >> password;

        if (database.authenticateUser(
                QString::fromStdString(username),
                QString::fromStdString(password),
                loggedInUser))
        {
            loginSuccess = true;
            std::cout
                << "\n登录成功！欢迎您，"
                << loggedInUser.username().toStdString()
                << " ["
                << (loggedInUser.isAdministrator()
                        ? "系统管理员"
                        : "收银员")
                << "]"
                << std::endl;
        }
        else
        {
            std::cout
                << "登录失败："
                << database.lastError().toStdString()
                << " 请重新输入！"
                << std::endl;
        }
    }

    int choice = 0;

    while (true)
    {
        std::cout
            << "\n----------------------------------------\n"
            << "  【系统管理菜单】\n"
            << "  1. 查看所有商品 (查)\n"
            << "  2. 录入新商品   (增)\n"
            << "  3. 修改商品库存 (改)\n"
            << "  4. 删除商品数据 (删)\n"
            << "  5. 注册/添加新用户 (增)\n"
            << "  6. 删除用户账号    (删)\n"
            << "  7. 退出系统\n"
            << "----------------------------------------\n"
            << "请选择操作 (1-7): ";

        std::cin >> choice;

        if (std::cin.fail())
        {
            clearInputBuffer();
            std::cout
                << "输入无效！请输入 1 至 7 之间的数字。"
                << std::endl;
            continue;
        }

        if (choice == 7)
        {
            std::cout
                << "谢谢使用，系统已安全退出。"
                << std::endl;
            break;
        }

        switch (choice)
        {
        case 1:
        {
            const QList<Product> products =
                database.getAllProducts();

            std::cout
                << "\n[商品列表] 共 "
                << products.size()
                << " 件商品："
                << std::endl;
            std::cout
                << "ID\t条码\t\t名称\t价格(元)\t库存\t更新时间"
                << std::endl;
            std::cout
                << "----------------------------------------------------------------"
                << std::endl;

            for (const Product &product : products)
            {
                std::cout
                    << product.productId().toStdString()
                    << "\t"
                    << product.barcode().toStdString()
                    << "\t"
                    << product.name().toStdString()
                    << "\t"
                    << product.priceInCent() / 100.0
                    << "\t"
                    << product.stock()
                    << "\t"
                    << product.updateTime().toStdString()
                    << std::endl;
            }
            break;
        }

        case 2:
        {
            std::string productId;
            std::string barcode;
            std::string name;
            double priceInYuan = 0.0;
            int stock = 0;
            int warningStock = 0;

            std::cout << "\n[录入新商品]" << std::endl;
            std::cout << "请输入商品ID (如 P001): ";
            std::cin >> productId;
            std::cout << "请输入条形码 (如 690123): ";
            std::cin >> barcode;
            std::cout << "请输入商品名称: ";
            std::cin >> name;
            std::cout << "请输入售价 (元): ";
            std::cin >> priceInYuan;
            std::cout << "请输入初始库存: ";
            std::cin >> stock;
            std::cout << "请输入库存预警线: ";
            std::cin >> warningStock;

            if (std::cin.fail())
            {
                clearInputBuffer();
                std::cout
                    << "输入格式有误，录入取消！"
                    << std::endl;
                break;
            }

            const qint64 priceInCent =
                static_cast<qint64>(
                    priceInYuan * 100);
            const QString updateTime =
                QDateTime::currentDateTime().toString(
                    "yyyy-MM-dd hh:mm:ss");
            const Product product(
                QString::fromStdString(productId),
                QString::fromStdString(barcode),
                QString::fromStdString(name),
                priceInCent,
                stock,
                warningStock,
                ProductStatus::Active,
                updateTime);

            if (database.addProduct(product))
            {
                std::cout
                    << "商品 ["
                    << name
                    << "] 录入成功！"
                    << std::endl;
            }
            else
            {
                std::cout
                    << "录入失败："
                    << database.lastError().toStdString()
                    << std::endl;
            }
            break;
        }

        case 3:
        {
            std::string productId;
            int newStock = 0;

            std::cout << "\n[修改商品库存]" << std::endl;
            std::cout << "请输入要修改的商品ID: ";
            std::cin >> productId;
            std::cout << "请输入新的库存数量: ";
            std::cin >> newStock;

            if (std::cin.fail())
            {
                clearInputBuffer();
                std::cout
                    << "输入无效，修改失败！"
                    << std::endl;
                break;
            }

            if (database.updateProductStock(
                    QString::fromStdString(productId),
                    newStock))
            {
                std::cout
                    << "商品 "
                    << productId
                    << " 库存已成功更新为 "
                    << newStock
                    << std::endl;
            }
            else
            {
                std::cout
                    << "更新库存失败："
                    << database.lastError().toStdString()
                    << std::endl;
            }
            break;
        }

        case 4:
        {
            std::string productId;
            char confirmation = 'n';

            std::cout << "\n[删除商品数据]" << std::endl;
            std::cout << "请输入要删除的商品ID: ";
            std::cin >> productId;
            std::cout << "确定要删除吗？(y/n): ";
            std::cin >> confirmation;

            if (confirmation == 'y'
                || confirmation == 'Y')
            {
                if (database.deleteProduct(
                        QString::fromStdString(productId)))
                {
                    std::cout
                        << "商品 "
                        << productId
                        << " 已成功从系统删除！"
                        << std::endl;
                }
                else
                {
                    std::cout
                        << "删除失败："
                        << database.lastError().toStdString()
                        << std::endl;
                }
            }
            else
            {
                std::cout << "操作已取消。" << std::endl;
            }
            break;
        }

        case 5:
        {
            std::string username;
            std::string password;
            int roleChoice = 2;

            std::cout
                << "\n[注册/添加新用户]"
                << std::endl;
            std::cout << "请输入新账号的用户名: ";
            std::cin >> username;
            std::cout << "请输入新账号的密码  : ";
            std::cin >> password;
            std::cout
                << "选择新账号的角色权限 "
                << "(1. 系统管理员 / 2. 收银员): ";
            std::cin >> roleChoice;

            if (std::cin.fail()
                || (roleChoice != 1
                    && roleChoice != 2))
            {
                clearInputBuffer();
                std::cout
                    << "输入无效，注册取消！"
                    << std::endl;
                break;
            }

            const UserRole role =
                roleChoice == 1
                ? UserRole::Administrator
                : UserRole::Cashier;
            const User newUser(
                0,
                QString::fromStdString(username),
                QString::fromStdString(password),
                role,
                AccountStatus::Enabled);

            if (database.addUser(newUser))
            {
                std::cout
                    << "用户 ["
                    << username
                    << "] 注册成功！"
                    << std::endl;
            }
            else
            {
                std::cout
                    << "用户注册失败："
                    << database.lastError().toStdString()
                    << std::endl;
            }
            break;
        }

        case 6:
        {
            std::string username;

            std::cout
                << "\n[删除用户账号]"
                << std::endl;
            std::cout << "请输入要删除的用户名: ";
            std::cin >> username;

            if (username == "admin")
            {
                std::cout
                    << "安全保护：超级管理员账号 "
                    << "[admin] 为系统内置账号，禁止删除！"
                    << std::endl;
                break;
            }

            if (username
                == loggedInUser.username().toStdString())
            {
                std::cout
                    << "安全保护：不能在登录状态下删除"
                    << "自己当前正在使用的账号！"
                    << std::endl;
                break;
            }

            char confirmation = 'n';
            std::cout
                << "确定要永久删除用户账号 ["
                << username
                << "] 吗？(y/n): ";
            std::cin >> confirmation;

            if (confirmation == 'y'
                || confirmation == 'Y')
            {
                if (database.deleteUserByName(
                        QString::fromStdString(username)))
                {
                    std::cout
                        << "用户账号 ["
                        << username
                        << "] 已被成功删除！"
                        << std::endl;
                }
                else
                {
                    std::cout
                        << "删除失败："
                        << database.lastError().toStdString()
                        << std::endl;
                }
            }
            else
            {
                std::cout << "操作已取消。" << std::endl;
            }
            break;
        }

        default:
            std::cout
                << "选项无效，请输入 1 至 7。"
                << std::endl;
            break;
        }
    }

    return 0;
}
