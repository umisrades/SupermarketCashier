#include "product_repository.h"

#include <QDateTime>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtMath>

namespace {

void setErrorMessage(
    QString *errorMessage,
    const QString &message)
{
    if (errorMessage != nullptr)
    {
        *errorMessage = message;
    }
}

bool ensureDatabaseOpen(QString *errorMessage)
{
    if (!QSqlDatabase::contains())
    {
        setErrorMessage(
            errorMessage,
            "数据库尚未初始化。");
        return false;
    }

    QSqlDatabase database = QSqlDatabase::database();

    if (database.isOpen())
    {
        return true;
    }

    if (!database.open())
    {
        setErrorMessage(
            errorMessage,
            "无法打开商品数据库："
                + database.lastError().text());
        return false;
    }

    return true;
}

bool executeSql(
    const QString &sql,
    QString *errorMessage)
{
    QSqlQuery query;

    if (!query.exec(sql))
    {
        setErrorMessage(
            errorMessage,
            "数据库初始化失败："
                + query.lastError().text());
        return false;
    }

    return true;
}

QHash<QString, bool> productColumns(
    QString *errorMessage)
{
    QHash<QString, bool> columns;
    QSqlQuery query("PRAGMA table_info(products)");

    if (query.lastError().isValid())
    {
        setErrorMessage(
            errorMessage,
            "无法读取商品表结构："
                + query.lastError().text());
        return {};
    }

    while (query.next())
    {
        columns.insert(
            query.value(1).toString(),
            true);
    }

    return columns;
}

ProductStatus statusFromDatabase(int value)
{
    switch (value)
    {
    case 0:
        return ProductStatus::Pending;
    case 1:
        return ProductStatus::OnSale;
    case 2:
        return ProductStatus::OffSale;
    default:
        return ProductStatus::Pending;
    }
}

int statusToDatabase(ProductStatus status)
{
    switch (status)
    {
    case ProductStatus::Pending:
        return 0;
    case ProductStatus::OnSale:
        return 1;
    case ProductStatus::OffSale:
        return 2;
    }

    return 0;
}

bool loadProducts(
    const QString &whereClause,
    std::vector<Product> *products,
    QString *errorMessage)
{
    if (products == nullptr)
    {
        setErrorMessage(
            errorMessage,
            "商品结果容器不能为空。");
        return false;
    }

    if (!ensureDatabaseOpen(errorMessage))
    {
        return false;
    }

    QSqlQuery query;
    const QString sql =
        "SELECT id, barcode, name, category, unit, "
        "price, quantity, warning_stock, status, update_time, image_path "
        "FROM products "
        + whereClause
        + " ORDER BY id";

    if (!query.exec(sql))
    {
        setErrorMessage(
            errorMessage,
            "无法读取商品数据："
                + query.lastError().text());
        return false;
    }

    std::vector<Product> loadedProducts;

    while (query.next())
    {
        Product product(
            query.value(0).toString(),
            query.value(1).toString(),
            query.value(2).toString(),
            query.value(3).toString(),
            query.value(4).toString(),
            qRound64(query.value(5).toDouble() * 100.0),
            query.value(6).toInt(),
            query.value(7).toInt(),
            statusFromDatabase(query.value(8).toInt()));
        product.setUpdateTime(query.value(9).toString());
        product.setImagePath(query.value(10).toString());
        loadedProducts.push_back(product);
    }

    *products = std::move(loadedProducts);
    return true;
}

bool rollbackWithError(
    QSqlDatabase &database,
    QString *errorMessage,
    const QString &message)
{
    database.rollback();
    setErrorMessage(errorMessage, message);
    return false;
}

} // namespace

bool ProductRepository::initialize(QString *errorMessage)
{
    return initializeAtPath("goods.db", errorMessage);
}

bool ProductRepository::initializeAtPath(
    const QString &databasePath,
    QString *errorMessage)
{
    QSqlDatabase database;

    if (QSqlDatabase::contains())
    {
        database = QSqlDatabase::database();
    }
    else
    {
        database = QSqlDatabase::addDatabase("QSQLITE");
    }

    database.setDatabaseName(databasePath);

    if (!database.open())
    {
        setErrorMessage(
            errorMessage,
            "无法连接商品数据库："
                + database.lastError().text());
        return false;
    }

    if (!executeSql(
            "CREATE TABLE IF NOT EXISTS products("
            "id TEXT PRIMARY KEY,"
            "barcode TEXT,"
            "name TEXT NOT NULL,"
            "category TEXT NOT NULL DEFAULT '未分类',"
            "unit TEXT NOT NULL DEFAULT '件',"
            "price REAL NOT NULL,"
            "quantity INTEGER NOT NULL DEFAULT 0,"
            "warning_stock INTEGER NOT NULL DEFAULT 0,"
            "status INTEGER NOT NULL DEFAULT 1,"
            "update_time TEXT NOT NULL DEFAULT '',"
            "image_path TEXT NOT NULL DEFAULT '')",
            errorMessage))
    {
        return false;
    }

    if (!executeSql(
            "CREATE TABLE IF NOT EXISTS app_metadata("
            "key TEXT PRIMARY KEY,"
            "value TEXT NOT NULL)",
            errorMessage))
    {
        return false;
    }

    if (!executeSql(
            "CREATE TABLE IF NOT EXISTS sales("
            "sale_id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "cashier_id INTEGER NOT NULL,"
            "total_amount REAL NOT NULL,"
            "paid_amount REAL NOT NULL,"
            "change_amount REAL NOT NULL,"
            "sale_time TEXT NOT NULL)",
            errorMessage))
    {
        return false;
    }

    if (!executeSql(
            "CREATE TABLE IF NOT EXISTS order_items("
            "item_id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "sale_id INTEGER NOT NULL,"
            "product_id TEXT NOT NULL,"
            "product_name TEXT NOT NULL,"
            "barcode TEXT NOT NULL,"
            "unit_price REAL NOT NULL,"
            "quantity INTEGER NOT NULL,"
            "subtotal REAL NOT NULL,"
            "FOREIGN KEY(sale_id) REFERENCES sales(sale_id))",
            errorMessage))
    {
        return false;
    }

    const QHash<QString, bool> columns =
        productColumns(errorMessage);

    if (columns.isEmpty())
    {
        return false;
    }

    const QList<QPair<QString, QString>> migrations = {
        {"barcode", "ALTER TABLE products ADD COLUMN barcode TEXT"},
        {"category", "ALTER TABLE products ADD COLUMN category TEXT NOT NULL DEFAULT '未分类'"},
        {"unit", "ALTER TABLE products ADD COLUMN unit TEXT NOT NULL DEFAULT '件'"},
        {"warning_stock", "ALTER TABLE products ADD COLUMN warning_stock INTEGER NOT NULL DEFAULT 0"},
        {"status", "ALTER TABLE products ADD COLUMN status INTEGER NOT NULL DEFAULT 1"},
        {"update_time", "ALTER TABLE products ADD COLUMN update_time TEXT NOT NULL DEFAULT ''"},
        {"image_path", "ALTER TABLE products ADD COLUMN image_path TEXT NOT NULL DEFAULT ''"}
    };

    for (const auto &migration : migrations)
    {
        if (!columns.contains(migration.first)
            && !executeSql(migration.second, errorMessage))
        {
            return false;
        }
    }

    if (!executeSql(
            "UPDATE products SET barcode = id "
            "WHERE barcode IS NULL OR TRIM(barcode) = ''",
            errorMessage)
        || !executeSql(
            "UPDATE products SET category = '未分类' "
            "WHERE category IS NULL OR TRIM(category) = ''",
            errorMessage)
        || !executeSql(
            "UPDATE products SET unit = '件' "
            "WHERE unit IS NULL OR TRIM(unit) = ''",
            errorMessage)
        || !executeSql(
            "UPDATE products SET warning_stock = 0 "
            "WHERE warning_stock IS NULL OR warning_stock < 0",
            errorMessage)
        || !executeSql(
            "UPDATE products SET status = 1 "
            "WHERE status IS NULL OR status NOT IN (0, 1, 2)",
            errorMessage)
        || !executeSql(
            "CREATE UNIQUE INDEX IF NOT EXISTS "
            "products_barcode_unique "
            "ON products(barcode) "
            "WHERE barcode IS NOT NULL AND TRIM(barcode) <> ''",
            errorMessage))
    {
        return false;
    }

    return true;
}

bool ProductRepository::importProductsFromDatabase(
    const QString &sourceDatabasePath,
    QString *errorMessage)
{
    if (!QFileInfo::exists(sourceDatabasePath))
    {
        setErrorMessage(
            errorMessage,
            "商品资源数据库不存在："
                + sourceDatabasePath);
        return false;
    }

    if (!ensureDatabaseOpen(errorMessage))
    {
        return false;
    }

    const QString resourceVersion =
        "product_resources_1001_1150_v1";
    QSqlQuery metadataQuery;
    metadataQuery.prepare(
        "SELECT value FROM app_metadata WHERE key = ?");
    metadataQuery.addBindValue(resourceVersion);

    if (!metadataQuery.exec())
    {
        setErrorMessage(
            errorMessage,
            "无法检查商品资源导入状态："
                + metadataQuery.lastError().text());
        return false;
    }

    if (metadataQuery.next())
    {
        return true;
    }

    struct ImportedProduct
    {
        QString id;
        QString name;
        double price;
        int quantity;
    };

    std::vector<ImportedProduct> importedProducts;
    const QString connectionName =
        "product_resource_import";

    if (QSqlDatabase::contains(connectionName))
    {
        QSqlDatabase::removeDatabase(connectionName);
    }

    {
        QSqlDatabase sourceDatabase =
            QSqlDatabase::addDatabase(
                "QSQLITE",
                connectionName);
        sourceDatabase.setDatabaseName(
            sourceDatabasePath);

        if (!sourceDatabase.open())
        {
            setErrorMessage(
                errorMessage,
                "无法打开商品资源数据库："
                    + sourceDatabase.lastError().text());
            sourceDatabase = QSqlDatabase();
            QSqlDatabase::removeDatabase(connectionName);
            return false;
        }

        QSqlQuery sourceQuery(sourceDatabase);

        if (!sourceQuery.exec(
                "SELECT id, name, price, quantity "
                "FROM products ORDER BY id"))
        {
            setErrorMessage(
                errorMessage,
                "无法读取商品资源数据库："
                    + sourceQuery.lastError().text());
            sourceQuery.finish();
            sourceDatabase.close();
            sourceDatabase = QSqlDatabase();
            QSqlDatabase::removeDatabase(connectionName);
            return false;
        }

        while (sourceQuery.next())
        {
            ImportedProduct product;
            product.id =
                sourceQuery.value(0).toString().trimmed();
            product.name =
                sourceQuery.value(1).toString();
            product.price =
                sourceQuery.value(2).toDouble();
            product.quantity =
                sourceQuery.value(3).toInt();

            if (!product.id.isEmpty())
            {
                importedProducts.push_back(product);
            }
        }

        sourceQuery.finish();
        sourceDatabase.close();
    }

    QSqlDatabase::removeDatabase(connectionName);

    QSqlDatabase database = QSqlDatabase::database();

    if (!database.transaction())
    {
        setErrorMessage(
            errorMessage,
            "无法开始商品资源导入事务："
                + database.lastError().text());
        return false;
    }

    QSqlQuery insertQuery(database);
    insertQuery.prepare(
        "INSERT OR IGNORE INTO products("
        "id, barcode, name, category, unit, price, "
        "quantity, warning_stock, status, update_time, image_path) "
        "VALUES(?, ?, ?, '未分类', '件', ?, ?, 0, 1, '', ?)");
    QSqlQuery imagePathQuery(database);
    imagePathQuery.prepare(
        "UPDATE products SET image_path = ? "
        "WHERE id = ? "
        "AND (image_path IS NULL OR TRIM(image_path) = '')");

    for (const ImportedProduct &product : importedProducts)
    {
        insertQuery.bindValue(0, product.id);
        insertQuery.bindValue(1, product.id);
        insertQuery.bindValue(2, product.name);
        insertQuery.bindValue(3, product.price);
        insertQuery.bindValue(4, product.quantity);
        insertQuery.bindValue(
            5,
            "images/products/"
                + product.id
                + ".png");

        if (!insertQuery.exec())
        {
            return rollbackWithError(
                database,
                errorMessage,
                "导入商品“" + product.id + "”失败："
                    + insertQuery.lastError().text());
        }

        imagePathQuery.bindValue(
            0,
            "images/products/"
                + product.id
                + ".png");
        imagePathQuery.bindValue(1, product.id);

        if (!imagePathQuery.exec())
        {
            return rollbackWithError(
                database,
                errorMessage,
                "关联商品“" + product.id + "”图片失败："
                    + imagePathQuery.lastError().text());
        }
    }

    QSqlQuery markImportedQuery(database);
    markImportedQuery.prepare(
        "INSERT INTO app_metadata(key, value) VALUES(?, ?)");
    markImportedQuery.addBindValue(resourceVersion);
    markImportedQuery.addBindValue(
        QDateTime::currentDateTime().toString(
            Qt::ISODate));

    if (!markImportedQuery.exec())
    {
        return rollbackWithError(
            database,
            errorMessage,
            "无法记录商品资源导入状态："
                + markImportedQuery.lastError().text());
    }

    if (!database.commit())
    {
        return rollbackWithError(
            database,
            errorMessage,
            "无法提交商品资源导入事务："
                + database.lastError().text());
    }

    return true;
}

bool ProductRepository::loadAllProducts(
    std::vector<Product> *products,
    QString *errorMessage)
{
    return loadProducts("", products, errorMessage);
}

bool ProductRepository::loadOnSaleProducts(
    std::vector<Product> *products,
    QString *errorMessage)
{
    return loadProducts(
        "WHERE status = 1 ",
        products,
        errorMessage);
}

bool ProductRepository::getProductById(
    const QString &productId,
    Product *product,
    QString *errorMessage)
{
    if (product == nullptr)
    {
        setErrorMessage(errorMessage, "商品结果容器不能为空。");
        return false;
    }

    if (!ensureDatabaseOpen(errorMessage))
    {
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "SELECT id, barcode, name, category, unit, "
        "price, quantity, warning_stock, status, update_time, image_path "
        "FROM products WHERE id = ?");
    query.addBindValue(productId);

    if (!query.exec())
    {
        setErrorMessage(
            errorMessage,
            "无法读取商品数据："
                + query.lastError().text());
        return false;
    }

    if (!query.next())
    {
        setErrorMessage(errorMessage, "未找到商品编号：" + productId);
        return false;
    }

    *product = Product(
        query.value(0).toString(),
        query.value(1).toString(),
        query.value(2).toString(),
        query.value(3).toString(),
        query.value(4).toString(),
        qRound64(query.value(5).toDouble() * 100.0),
        query.value(6).toInt(),
        query.value(7).toInt(),
        statusFromDatabase(query.value(8).toInt()));
    product->setUpdateTime(query.value(9).toString());
    product->setImagePath(query.value(10).toString());

    return true;
}

bool ProductRepository::insertProduct(
    const Product &product,
    QString *errorMessage)
{
    if (!ensureDatabaseOpen(errorMessage))
    {
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "INSERT INTO products("
        "id, barcode, name, category, unit, price, "
        "quantity, warning_stock, status, update_time, image_path) "
        "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(product.productId());
    query.addBindValue(product.barcode());
    query.addBindValue(product.name());
    query.addBindValue(product.category());
    query.addBindValue(product.unit());
    query.addBindValue(product.priceInCent() / 100.0);
    query.addBindValue(product.stock());
    query.addBindValue(product.warningStock());
    query.addBindValue(statusToDatabase(product.status()));
    query.addBindValue(
        product.updateTime().isEmpty()
        ? QDateTime::currentDateTime().toString(
              "yyyy-MM-dd hh:mm:ss")
        : product.updateTime());
    query.addBindValue(
        product.imagePath().isNull()
        ? QString("")
        : product.imagePath());

    if (!query.exec())
    {
        setErrorMessage(
            errorMessage,
            "无法新增商品："
                + query.lastError().text());
        return false;
    }

    return true;
}

bool ProductRepository::updateStock(
    const QString &productId,
    int stock,
    QString *errorMessage)
{
    if (stock < 0)
    {
        setErrorMessage(errorMessage, "库存不能小于零。");
        return false;
    }

    if (!ensureDatabaseOpen(errorMessage))
    {
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "UPDATE products "
        "SET quantity = ?, update_time = ? "
        "WHERE id = ?");
    query.addBindValue(stock);
    query.addBindValue(
        QDateTime::currentDateTime().toString(
            "yyyy-MM-dd hh:mm:ss"));
    query.addBindValue(productId);

    if (!query.exec())
    {
        setErrorMessage(
            errorMessage,
            "无法更新库存："
                + query.lastError().text());
        return false;
    }

    if (query.numRowsAffected() != 1)
    {
        setErrorMessage(errorMessage, "未找到需要更新的商品。");
        return false;
    }

    return true;
}

bool ProductRepository::updateWarningStock(
    const QString &productId,
    int warningStock,
    QString *errorMessage)
{
    if (warningStock < 0)
    {
        setErrorMessage(errorMessage, "库存预警值不能小于零。");
        return false;
    }

    if (!ensureDatabaseOpen(errorMessage))
    {
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "UPDATE products "
        "SET warning_stock = ?, update_time = ? "
        "WHERE id = ?");
    query.addBindValue(warningStock);
    query.addBindValue(
        QDateTime::currentDateTime().toString(
            "yyyy-MM-dd hh:mm:ss"));
    query.addBindValue(productId);

    if (!query.exec())
    {
        setErrorMessage(
            errorMessage,
            "无法更新库存预警值："
                + query.lastError().text());
        return false;
    }

    if (query.numRowsAffected() != 1)
    {
        setErrorMessage(errorMessage, "未找到需要更新的商品。");
        return false;
    }

    return true;
}

bool ProductRepository::deleteProduct(
    const QString &productId,
    QString *errorMessage)
{
    if (!ensureDatabaseOpen(errorMessage))
    {
        return false;
    }

    QSqlQuery query;
    query.prepare("DELETE FROM products WHERE id = ?");
    query.addBindValue(productId);

    if (!query.exec())
    {
        setErrorMessage(
            errorMessage,
            "无法删除商品："
                + query.lastError().text());
        return false;
    }

    if (query.numRowsAffected() != 1)
    {
        setErrorMessage(errorMessage, "未找到需要删除的商品。");
        return false;
    }

    return true;
}

bool ProductRepository::updateStocks(
    const QHash<QString, int> &stockByProductId,
    QString *errorMessage)
{
    if (!ensureDatabaseOpen(errorMessage))
    {
        return false;
    }

    QSqlDatabase database = QSqlDatabase::database();

    if (!database.transaction())
    {
        setErrorMessage(
            errorMessage,
            "无法开始库存保存事务："
                + database.lastError().text());
        return false;
    }

    QSqlQuery query(database);
    query.prepare(
        "UPDATE products "
        "SET quantity = ?, update_time = ? "
        "WHERE id = ?");

    for (auto iterator = stockByProductId.constBegin();
         iterator != stockByProductId.constEnd();
         ++iterator)
    {
        if (iterator.value() < 0)
        {
            return rollbackWithError(
                database,
                errorMessage,
                "商品“" + iterator.key() + "”的库存不能小于零。");
        }

        query.bindValue(0, iterator.value());
        query.bindValue(
            1,
            QDateTime::currentDateTime().toString(
                "yyyy-MM-dd hh:mm:ss"));
        query.bindValue(2, iterator.key());

        if (!query.exec() || query.numRowsAffected() != 1)
        {
            return rollbackWithError(
                database,
                errorMessage,
                "无法保存商品“" + iterator.key() + "”的库存："
                    + query.lastError().text());
        }
    }

    if (!database.commit())
    {
        return rollbackWithError(
            database,
            errorMessage,
            "无法提交库存保存事务："
                + database.lastError().text());
    }

    return true;
}

bool ProductRepository::commitSale(
    const QHash<QString, int> &quantityByProductId,
    QString *errorMessage)
{
    if (quantityByProductId.isEmpty())
    {
        setErrorMessage(errorMessage, "本次交易没有商品。");
        return false;
    }

    if (!ensureDatabaseOpen(errorMessage))
    {
        return false;
    }

    QSqlDatabase database = QSqlDatabase::database();

    if (!database.transaction())
    {
        setErrorMessage(
            errorMessage,
            "无法开始结算事务："
                + database.lastError().text());
        return false;
    }

    QSqlQuery query(database);
    query.prepare(
        "UPDATE products "
        "SET quantity = quantity - ?, update_time = ? "
        "WHERE id = ? AND status = 1 AND quantity >= ?");

    for (auto iterator = quantityByProductId.constBegin();
         iterator != quantityByProductId.constEnd();
         ++iterator)
    {
        if (iterator.value() <= 0)
        {
            return rollbackWithError(
                database,
                errorMessage,
                "商品“" + iterator.key() + "”的购买数量不合法。");
        }

        query.bindValue(0, iterator.value());
        query.bindValue(
            1,
            QDateTime::currentDateTime().toString(
                "yyyy-MM-dd hh:mm:ss"));
        query.bindValue(2, iterator.key());
        query.bindValue(3, iterator.value());

        if (!query.exec())
        {
            return rollbackWithError(
                database,
                errorMessage,
                "扣减商品“" + iterator.key() + "”库存失败："
                    + query.lastError().text());
        }

        if (query.numRowsAffected() != 1)
        {
            return rollbackWithError(
                database,
                errorMessage,
                "商品“" + iterator.key()
                    + "”不存在、未上架或库存不足，请刷新后重试。");
        }
    }

    if (!database.commit())
    {
        return rollbackWithError(
            database,
            errorMessage,
            "无法提交结算事务："
                + database.lastError().text());
    }

    return true;
}

bool ProductRepository::commitSaleWithRecord(
    const QHash<QString, int> &quantityByProductId,
    int cashierId,
    double totalAmount,
    double paidAmount,
    QString *errorMessage)
{
    if (quantityByProductId.isEmpty())
    {
        setErrorMessage(errorMessage, "本次交易没有商品。");
        return false;
    }

    if (!ensureDatabaseOpen(errorMessage))
    {
        return false;
    }

    QSqlDatabase database = QSqlDatabase::database();

    if (!database.transaction())
    {
        setErrorMessage(
            errorMessage,
            "无法开始结算事务："
                + database.lastError().text());
        return false;
    }

    QSqlQuery updateStockQuery(database);
    updateStockQuery.prepare(
        "UPDATE products "
        "SET quantity = quantity - ?, update_time = ? "
        "WHERE id = ? AND status = 1 AND quantity >= ?");

    for (auto iterator = quantityByProductId.constBegin();
         iterator != quantityByProductId.constEnd();
         ++iterator)
    {
        if (iterator.value() <= 0)
        {
            return rollbackWithError(
                database,
                errorMessage,
                "商品“" + iterator.key() + "”的购买数量不合法。");
        }

        updateStockQuery.bindValue(0, iterator.value());
        updateStockQuery.bindValue(
            1,
            QDateTime::currentDateTime().toString(
                "yyyy-MM-dd hh:mm:ss"));
        updateStockQuery.bindValue(2, iterator.key());
        updateStockQuery.bindValue(3, iterator.value());

        if (!updateStockQuery.exec())
        {
            return rollbackWithError(
                database,
                errorMessage,
                "扣减商品“" + iterator.key() + "”库存失败："
                    + updateStockQuery.lastError().text());
        }

        if (updateStockQuery.numRowsAffected() != 1)
        {
            return rollbackWithError(
                database,
                errorMessage,
                "商品“" + iterator.key()
                    + "”不存在、未上架或库存不足，请刷新后重试。");
        }
    }

    QSqlQuery insertSaleQuery(database);
    insertSaleQuery.prepare(
        "INSERT INTO sales("
        "cashier_id, total_amount, paid_amount, "
        "change_amount, sale_time) "
        "VALUES(?, ?, ?, ?, ?)");
    insertSaleQuery.addBindValue(cashierId);
    insertSaleQuery.addBindValue(totalAmount);
    insertSaleQuery.addBindValue(paidAmount);
    insertSaleQuery.addBindValue(paidAmount - totalAmount);
    insertSaleQuery.addBindValue(
        QDateTime::currentDateTime().toString(
            "yyyy-MM-dd hh:mm:ss"));

    if (!insertSaleQuery.exec())
    {
        return rollbackWithError(
            database,
            errorMessage,
            "记录销售订单失败："
                + insertSaleQuery.lastError().text());
    }

    const int saleId = insertSaleQuery.lastInsertId().toInt();

    QSqlQuery insertItemQuery(database);
    insertItemQuery.prepare(
        "INSERT INTO order_items("
        "sale_id, product_id, product_name, barcode, "
        "unit_price, quantity, subtotal) "
        "VALUES(?, ?, ?, ?, ?, ?, ?)");

    for (auto iterator = quantityByProductId.constBegin();
         iterator != quantityByProductId.constEnd();
         ++iterator)
    {
        const QString productId = iterator.key();
        const int quantity = iterator.value();

        Product product;
        QString innerError;

        if (!getProductById(productId, &product, &innerError))
        {
            return rollbackWithError(
                database,
                errorMessage,
                "获取商品“" + productId + "”信息失败：" + innerError);
        }

        const double unitPrice = product.priceInCent() / 100.0;
        const double subtotal = unitPrice * quantity;

        insertItemQuery.bindValue(0, saleId);
        insertItemQuery.bindValue(1, productId);
        insertItemQuery.bindValue(2, product.name());
        insertItemQuery.bindValue(3, product.barcode());
        insertItemQuery.bindValue(4, unitPrice);
        insertItemQuery.bindValue(5, quantity);
        insertItemQuery.bindValue(6, subtotal);

        if (!insertItemQuery.exec())
        {
            return rollbackWithError(
                database,
                errorMessage,
                "记录订单明细失败："
                    + insertItemQuery.lastError().text());
        }
    }

    if (!database.commit())
    {
        return rollbackWithError(
            database,
            errorMessage,
            "无法提交结算事务："
                + database.lastError().text());
    }

    return true;
}

bool ProductRepository::getDailySalesStats(
    const QString &date,
    DailySalesStats *stats,
    QString *errorMessage)
{
    if (stats == nullptr)
    {
        setErrorMessage(errorMessage, "统计结果容器不能为空。");
        return false;
    }

    if (!ensureDatabaseOpen(errorMessage))
    {
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "SELECT COUNT(*), SUM(total_amount), AVG(total_amount) "
        "FROM sales "
        "WHERE sale_time LIKE ?");
    query.addBindValue(date + "%");

    if (!query.exec())
    {
        setErrorMessage(
            errorMessage,
            "查询日销售统计失败："
                + query.lastError().text());
        return false;
    }

    stats->date = date;
    stats->orderCount = 0;
    stats->totalAmount = 0.0;
    stats->averageOrder = 0.0;

    if (query.next())
    {
        stats->orderCount = query.value(0).toInt();
        stats->totalAmount = query.value(1).toDouble();
        stats->averageOrder = stats->orderCount > 0
            ? query.value(2).toDouble()
            : 0.0;
    }

    return true;
}

bool ProductRepository::getSalesStatsForRange(
    const QString &dateFrom,
    const QString &dateTo,
    DailySalesStats *stats,
    QString *errorMessage)
{
    if (stats == nullptr)
    {
        setErrorMessage(errorMessage, "统计结果容器不能为空。");
        return false;
    }

    if (!ensureDatabaseOpen(errorMessage))
    {
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "SELECT COUNT(*), SUM(total_amount), AVG(total_amount) "
        "FROM sales "
        "WHERE sale_time >= ? AND sale_time <= ?");
    query.addBindValue(dateFrom + " 00:00:00");
    query.addBindValue(dateTo + " 23:59:59");

    if (!query.exec())
    {
        setErrorMessage(
            errorMessage,
            "查询销售统计失败："
                + query.lastError().text());
        return false;
    }

    stats->date = dateFrom + " 至 " + dateTo;
    stats->orderCount = 0;
    stats->totalAmount = 0.0;
    stats->averageOrder = 0.0;

    if (query.next())
    {
        stats->orderCount = query.value(0).toInt();
        stats->totalAmount = query.value(1).toDouble();
        stats->averageOrder = stats->orderCount > 0
            ? query.value(2).toDouble()
            : 0.0;
    }

    return true;
}

bool ProductRepository::getMonthlySalesStats(
    int year,
    int month,
    MonthlySalesStats *stats,
    QString *errorMessage)
{
    if (stats == nullptr)
    {
        setErrorMessage(errorMessage, "统计结果容器不能为空。");
        return false;
    }

    if (!ensureDatabaseOpen(errorMessage))
    {
        return false;
    }

    const QString monthPrefix =
        QString("%1-%2").arg(year).arg(month, 2, 10, QChar('0'));

    QSqlQuery query;
    query.prepare(
        "SELECT COUNT(*), SUM(total_amount), AVG(total_amount), "
        "COUNT(DISTINCT cashier_id) "
        "FROM sales "
        "WHERE sale_time LIKE ?");
    query.addBindValue(monthPrefix + "%");

    if (!query.exec())
    {
        setErrorMessage(
            errorMessage,
            "查询月销售统计失败："
                + query.lastError().text());
        return false;
    }

    stats->month = monthPrefix;
    stats->orderCount = 0;
    stats->totalAmount = 0.0;
    stats->averageOrder = 0.0;
    stats->customerCount = 0;

    if (query.next())
    {
        stats->orderCount = query.value(0).toInt();
        stats->totalAmount = query.value(1).toDouble();
        stats->averageOrder = stats->orderCount > 0
            ? query.value(2).toDouble()
            : 0.0;
        stats->customerCount = query.value(3).toInt();
    }

    return true;
}

bool ProductRepository::getSalesRanking(
    const QString &dateFrom,
    const QString &dateTo,
    std::vector<SalesRankingItem> *ranking,
    QString *errorMessage)
{
    if (ranking == nullptr)
    {
        setErrorMessage(errorMessage, "排行结果容器不能为空。");
        return false;
    }

    if (!ensureDatabaseOpen(errorMessage))
    {
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "SELECT oi.product_id, oi.product_name, oi.barcode, "
        "SUM(oi.quantity), SUM(oi.subtotal), AVG(oi.unit_price) "
        "FROM order_items oi "
        "JOIN sales s ON oi.sale_id = s.sale_id "
        "WHERE s.sale_time >= ? AND s.sale_time <= ? "
        "GROUP BY oi.product_id, oi.product_name, oi.barcode "
        "ORDER BY SUM(oi.quantity) DESC");
    query.addBindValue(dateFrom + " 00:00:00");
    query.addBindValue(dateTo + " 23:59:59");

    if (!query.exec())
    {
        setErrorMessage(
            errorMessage,
            "查询销量排行失败："
                + query.lastError().text());
        return false;
    }

    std::vector<SalesRankingItem> result;

    while (query.next())
    {
        SalesRankingItem item;
        item.productId = query.value(0).toString();
        item.productName = query.value(1).toString();
        item.barcode = query.value(2).toString();
        item.totalQuantity = query.value(3).toInt();
        item.totalRevenue = query.value(4).toDouble();
        item.unitPrice = query.value(5).toDouble();
        result.push_back(item);
    }

    *ranking = std::move(result);
    return true;
}

bool ProductRepository::getTodaySalesStats(
    DailySalesStats *stats,
    QString *errorMessage)
{
    const QString today =
        QDateTime::currentDateTime().toString("yyyy-MM-dd");
    return getDailySalesStats(today, stats, errorMessage);
}

bool ProductRepository::getMonthToDateSalesStats(
    MonthlySalesStats *stats,
    QString *errorMessage)
{
    const QDate today = QDate::currentDate();
    return getMonthlySalesStats(
        today.year(),
        today.month(),
        stats,
        errorMessage);
}
