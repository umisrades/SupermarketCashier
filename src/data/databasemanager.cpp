#include "databasemanager.h"

#include "product_repository.h"

#include <QSqlError>
#include <QSqlQuery>

DatabaseManager &DatabaseManager::instance()
{
    static DatabaseManager manager;
    return manager;
}

DatabaseManager::~DatabaseManager()
{
    closeDatabase();
}

bool DatabaseManager::initDatabase(
    const QString &databasePath)
{
    m_lastError.clear();

    if (!ProductRepository::initializeAtPath(
            databasePath,
            &m_lastError))
    {
        return false;
    }

    m_db = QSqlDatabase::database();

    if (!createUserTable()
        || !ensureDefaultUsers())
    {
        return false;
    }

    return true;
}

bool DatabaseManager::importProductsFromDatabase(
    const QString &sourceDatabasePath)
{
    const bool succeeded =
        ProductRepository::importProductsFromDatabase(
            sourceDatabasePath,
            &m_lastError);

    if (succeeded)
    {
        m_lastError.clear();
    }

    return succeeded;
}

void DatabaseManager::closeDatabase()
{
    if (m_db.isValid() && m_db.isOpen())
    {
        m_db.close();
    }
}

bool DatabaseManager::authenticateUser(
    const QString &username,
    const QString &password,
    User &user)
{
    QSqlQuery query(m_db);
    query.prepare(
        "SELECT user_id, username, password, role, status "
        "FROM users "
        "WHERE username = :name AND password = :password");
    query.bindValue(":name", username);
    query.bindValue(":password", password);

    if (!query.exec())
    {
        m_lastError = query.lastError().text();
        return false;
    }

    if (!query.next())
    {
        m_lastError = "用户名或密码错误，请重试！";
        return false;
    }

    const UserRole role =
        query.value("role").toString()
            == "Administrator"
        ? UserRole::Administrator
        : UserRole::Cashier;
    const AccountStatus status =
        query.value("status").toInt() == 1
        ? AccountStatus::Enabled
        : AccountStatus::Disabled;

    if (status == AccountStatus::Disabled)
    {
        m_lastError =
            "该账号已被停用，请联系系统管理员！";
        return false;
    }

    user = User(
        query.value("user_id").toInt(),
        query.value("username").toString(),
        query.value("password").toString(),
        role,
        status);
    m_lastError.clear();
    return true;
}

bool DatabaseManager::addProduct(
    const Product &product)
{
    const bool succeeded =
        ProductRepository::insertProduct(
            product,
            &m_lastError);

    if (succeeded)
    {
        m_lastError.clear();
    }

    return succeeded;
}

bool DatabaseManager::getProductById(
    const QString &productId,
    Product &product)
{
    const QList<Product> products = getAllProducts();

    for (const Product &item : products)
    {
        if (item.productId() == productId)
        {
            product = item;
            m_lastError.clear();
            return true;
        }
    }

    m_lastError = "未找到商品编号：" + productId;
    return false;
}

bool DatabaseManager::getProductByBarcode(
    const QString &barcode,
    Product &product)
{
    const QList<Product> products = getAllProducts();

    for (const Product &item : products)
    {
        if (item.barcode() == barcode)
        {
            product = item;
            m_lastError.clear();
            return true;
        }
    }

    m_lastError = "未找到条形码：" + barcode;
    return false;
}

QList<Product> DatabaseManager::getAllProducts()
{
    std::vector<Product> products;

    if (!ProductRepository::loadAllProducts(
            &products,
            &m_lastError))
    {
        return {};
    }

    QList<Product> result;
    result.reserve(
        static_cast<qsizetype>(products.size()));

    for (const Product &product : products)
    {
        result.append(product);
    }

    m_lastError.clear();
    return result;
}

bool DatabaseManager::deleteProduct(
    const QString &productId)
{
    const bool succeeded =
        ProductRepository::deleteProduct(
            productId,
            &m_lastError);

    if (succeeded)
    {
        m_lastError.clear();
    }

    return succeeded;
}

bool DatabaseManager::updateProductStock(
    const QString &productId,
    int newStock)
{
    const bool succeeded =
        ProductRepository::updateStock(
            productId,
            newStock,
            &m_lastError);

    if (succeeded)
    {
        m_lastError.clear();
    }

    return succeeded;
}

bool DatabaseManager::addUser(const User &user)
{
    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO users(username, password, role, status) "
        "VALUES(:name, :password, :role, :status)");
    query.bindValue(":name", user.username());
    query.bindValue(":password", user.password());
    query.bindValue(
        ":role",
        user.role() == UserRole::Administrator
        ? "Administrator"
        : "Cashier");
    query.bindValue(
        ":status",
        user.status() == AccountStatus::Enabled
        ? 1
        : 0);

    if (!query.exec())
    {
        m_lastError = query.lastError().text();
        return false;
    }

    m_lastError.clear();
    return true;
}

bool DatabaseManager::deleteUser(int userId)
{
    QSqlQuery query(m_db);
    query.prepare(
        "DELETE FROM users WHERE user_id = :id");
    query.bindValue(":id", userId);

    if (!query.exec())
    {
        m_lastError = query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() != 1)
    {
        m_lastError = "未找到需要删除的用户。";
        return false;
    }

    m_lastError.clear();
    return true;
}

bool DatabaseManager::deleteUserByName(
    const QString &username)
{
    QSqlQuery query(m_db);
    query.prepare(
        "DELETE FROM users WHERE username = :name");
    query.bindValue(":name", username);

    if (!query.exec())
    {
        m_lastError = query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() != 1)
    {
        m_lastError = "未找到需要删除的用户。";
        return false;
    }

    m_lastError.clear();
    return true;
}

QString DatabaseManager::lastError() const
{
    return m_lastError;
}

bool DatabaseManager::createUserTable()
{
    QSqlQuery query(m_db);

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS users("
            "user_id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "username TEXT UNIQUE NOT NULL,"
            "password TEXT NOT NULL,"
            "role TEXT NOT NULL,"
            "status INTEGER NOT NULL)"))
    {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::ensureDefaultUsers()
{
    if (!m_db.transaction())
    {
        m_lastError = m_db.lastError().text();
        return false;
    }

    QSqlQuery insertQuery(m_db);
    insertQuery.prepare(
        "INSERT OR IGNORE INTO users("
        "username, password, role, status) "
        "VALUES(?, ?, ?, 1)");

    const QList<QList<QVariant>> defaultUsers = {
        {"admin", "admin123", "Administrator"},
        {"boss", "boss123", "Administrator"},
        {"cashier", "cashier123", "Cashier"},
        {"people", "people123", "Cashier"}
    };

    for (const QList<QVariant> &defaultUser : defaultUsers)
    {
        insertQuery.bindValue(0, defaultUser.at(0));
        insertQuery.bindValue(1, defaultUser.at(1));
        insertQuery.bindValue(2, defaultUser.at(2));

        if (!insertQuery.exec())
        {
            m_lastError = insertQuery.lastError().text();
            m_db.rollback();
            return false;
        }
    }

    if (!m_db.commit())
    {
        m_lastError = m_db.lastError().text();
        m_db.rollback();
        return false;
    }

    return true;
}
