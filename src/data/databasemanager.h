#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <QList>
#include <QSqlDatabase>
#include <QString>

#include "../models/product.h"
#include "../models/user.h"

class DatabaseManager
{
public:
    static DatabaseManager &instance();

    bool initDatabase(
        const QString &databasePath = "goods.db");
    bool importProductsFromDatabase(
        const QString &sourceDatabasePath);
    void closeDatabase();

    bool authenticateUser(
        const QString &username,
        const QString &password,
        User &user);

    bool addProduct(const Product &product);
    bool getProductById(
        const QString &productId,
        Product &product);
    bool getProductByBarcode(
        const QString &barcode,
        Product &product);
    QList<Product> getAllProducts();
    bool deleteProduct(const QString &productId);
    bool updateProductStock(
        const QString &productId,
        int newStock);

    bool addUser(const User &user);
    bool deleteUser(int userId);
    bool deleteUserByName(const QString &username);

    QString lastError() const;

private:
    DatabaseManager() = default;
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager &) = delete;
    DatabaseManager &operator=(
        const DatabaseManager &) = delete;

    bool createUserTable();
    bool ensureDefaultUsers();

    QSqlDatabase m_db;
    QString m_lastError;
};

#endif // DATABASE_MANAGER_H
