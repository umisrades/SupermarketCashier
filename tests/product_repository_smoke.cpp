#include <QCoreApplication>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTextStream>

#include <vector>

#include "../src/data/product_repository.h"

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

const Product *findProduct(
    const std::vector<Product> &products,
    const QString &productId)
{
    for (const Product &product : products)
    {
        if (product.productId() == productId)
        {
            return &product;
        }
    }

    return nullptr;
}

bool runEmptyDatabaseTest()
{
    QString errorMessage;

    if (!require(
            ProductRepository::initialize(&errorMessage),
            errorMessage))
    {
        return false;
    }

    std::vector<Product> products;

    if (!require(
            ProductRepository::loadAllProducts(
                &products,
                &errorMessage),
            errorMessage)
        || !require(products.empty(), "新数据库应为空。"))
    {
        return false;
    }

    const Product onSaleProduct(
        "P001",
        "690000000001",
        "测试饮料",
        "饮料",
        "瓶",
        250,
        3,
        1,
        ProductStatus::OnSale);
    const Product pendingProduct(
        "P002",
        "690000000002",
        "待上架商品",
        "食品",
        "袋",
        450,
        5,
        2,
        ProductStatus::Pending);

    if (!require(
            ProductRepository::insertProduct(
                onSaleProduct,
                &errorMessage),
            errorMessage)
        || !require(
            ProductRepository::insertProduct(
                pendingProduct,
                &errorMessage),
            errorMessage))
    {
        return false;
    }

    const Product duplicateBarcode(
        "P003",
        "690000000001",
        "重复条码商品",
        "食品",
        "袋",
        500,
        1,
        0,
        ProductStatus::OnSale);

    if (!require(
            !ProductRepository::insertProduct(
                duplicateBarcode,
                &errorMessage),
            "重复条形码必须被拒绝。"))
    {
        return false;
    }

    std::vector<Product> onSaleProducts;

    if (!require(
            ProductRepository::loadOnSaleProducts(
                &onSaleProducts,
                &errorMessage),
            errorMessage)
        || !require(
            onSaleProducts.size() == 1,
            "收银台只能读取已上架商品。"))
    {
        return false;
    }

    QHash<QString, int> sale;
    sale.insert("P001", 2);

    if (!require(
            ProductRepository::commitSale(
                sale,
                &errorMessage),
            errorMessage)
        || !require(
            ProductRepository::loadAllProducts(
                &products,
                &errorMessage),
            errorMessage))
    {
        return false;
    }

    const Product *updatedProduct =
        findProduct(products, "P001");

    if (!require(
            updatedProduct != nullptr
                && updatedProduct->stock() == 1,
            "结算后库存应从 3 扣减到 1。"))
    {
        return false;
    }

    if (!require(
            !ProductRepository::commitSale(
                sale,
                &errorMessage),
            "库存不足的交易必须失败。")
        || !require(
            ProductRepository::loadAllProducts(
                &products,
                &errorMessage),
            errorMessage))
    {
        return false;
    }

    updatedProduct = findProduct(products, "P001");

    if (!require(
            updatedProduct != nullptr
                && updatedProduct->stock() == 1,
            "失败的交易不能改变库存。"))
    {
        return false;
    }

    QHash<QString, int> pendingSale;
    pendingSale.insert("P002", 1);

    if (!require(
            !ProductRepository::commitSale(
                pendingSale,
                &errorMessage),
            "待上架商品不能结算。")
        || !require(
            ProductRepository::updateWarningStock(
                "P001",
                4,
                &errorMessage),
            errorMessage)
        || !require(
            ProductRepository::updateStock(
                "P001",
                6,
                &errorMessage),
            errorMessage)
        || !require(
            ProductRepository::deleteProduct(
                "P002",
                &errorMessage),
            errorMessage)
        || !require(
            ProductRepository::loadAllProducts(
                &products,
                &errorMessage),
            errorMessage))
    {
        return false;
    }

    updatedProduct = findProduct(products, "P001");

    return require(
               products.size() == 1,
               "删除商品后数据库应只剩一个商品。")
        && require(
               updatedProduct != nullptr
                   && updatedProduct->stock() == 6
                   && updatedProduct->warningStock() == 4,
               "库存和预警值修改必须持久化。");
}

bool runLegacyMigrationTest()
{
    QSqlDatabase database =
        QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName("goods.db");

    if (!require(database.open(), "无法创建旧格式测试数据库。"))
    {
        return false;
    }

    QSqlQuery query(database);

    if (!require(
            query.exec(
                "CREATE TABLE products("
                "id TEXT PRIMARY KEY,"
                "name TEXT NOT NULL,"
                "price REAL NOT NULL,"
                "quantity INTEGER NOT NULL)"),
            query.lastError().text())
        || !require(
            query.exec(
                "INSERT INTO products(id, name, price, quantity) "
                "VALUES('1001', '旧商品', 3.50, 9)"),
            query.lastError().text()))
    {
        return false;
    }

    database.close();

    QString errorMessage;

    if (!require(
            ProductRepository::initialize(&errorMessage),
            errorMessage))
    {
        return false;
    }

    std::vector<Product> products;

    if (!require(
            ProductRepository::loadAllProducts(
                &products,
                &errorMessage),
            errorMessage)
        || !require(
            products.size() == 1,
            "迁移后旧商品必须保留。"))
    {
        return false;
    }

    const Product &product = products.front();

    return require(
               product.productId() == "1001"
                   && product.barcode() == "1001",
               "旧商品条形码应回填为商品编号。")
        && require(
               product.category() == "未分类"
                   && product.unit() == "件",
               "旧商品应补充默认分类和单位。")
        && require(
               product.priceInCent() == 350
                   && product.stock() == 9
                   && product.status() == ProductStatus::OnSale,
               "旧商品价格、库存和默认上架状态必须正确。 ");
}

bool runProductResourceImportTest()
{
    QString errorMessage;

    if (!require(
            ProductRepository::initialize(&errorMessage),
            errorMessage))
    {
        return false;
    }

    const Product userModifiedProduct(
        "1001",
        "CUSTOM-BARCODE",
        "用户已修改商品",
        "自定义分类",
        "箱",
        9999,
        7,
        3,
        ProductStatus::OffSale);

    if (!require(
            ProductRepository::insertProduct(
                userModifiedProduct,
                &errorMessage),
            errorMessage))
    {
        return false;
    }

    const QString sourceDatabasePath =
        QDir::current().filePath("default_goods.db");

    {
        QSqlDatabase sourceDatabase =
            QSqlDatabase::addDatabase(
                "QSQLITE",
                "resource_fixture");
        sourceDatabase.setDatabaseName(
            sourceDatabasePath);

        if (!require(
                sourceDatabase.open(),
                sourceDatabase.lastError().text()))
        {
            return false;
        }

        QSqlQuery query(sourceDatabase);

        if (!require(
                query.exec(
                    "CREATE TABLE products("
                    "id TEXT PRIMARY KEY,"
                    "name TEXT NOT NULL,"
                    "price REAL NOT NULL,"
                    "quantity INTEGER NOT NULL)"),
                query.lastError().text())
            || !require(
                query.exec(
                    "INSERT INTO products VALUES"
                    "('1001', '资源商品一', 2.50, 100),"
                    "('1002', '资源商品二', 4.50, 80)"),
                query.lastError().text()))
        {
            return false;
        }

        query.finish();
        sourceDatabase.close();
    }

    QSqlDatabase::removeDatabase("resource_fixture");

    if (!require(
            ProductRepository::importProductsFromDatabase(
                sourceDatabasePath,
                &errorMessage),
            errorMessage))
    {
        return false;
    }

    std::vector<Product> products;

    if (!require(
            ProductRepository::loadAllProducts(
                &products,
                &errorMessage),
            errorMessage)
        || !require(
            products.size() == 2,
            "资源导入后应包含两个商品。"))
    {
        return false;
    }

    const Product *preservedProduct =
        findProduct(products, "1001");
    const Product *importedProduct =
        findProduct(products, "1002");

    const bool importWasCorrect = require(
               preservedProduct != nullptr
                   && preservedProduct->name()
                       == "用户已修改商品"
                   && preservedProduct->barcode()
                       == "CUSTOM-BARCODE"
                   && preservedProduct->priceInCent()
                       == 9999
                   && preservedProduct->status()
                       == ProductStatus::OffSale,
               "资源导入不能覆盖用户已修改的商品字段。")
        && require(
               preservedProduct->imagePath()
                   == "images/products/1001.png",
               "已有商品缺少图片路径时应补充匹配路径。")
        && require(
               importedProduct != nullptr
                   && importedProduct->barcode() == "1002"
                   && importedProduct->name() == "资源商品二"
                   && importedProduct->category() == "未分类"
                   && importedProduct->unit() == "件"
                   && importedProduct->priceInCent() == 450
                   && importedProduct->stock() == 80
                   && importedProduct->warningStock() == 0
                   && importedProduct->status()
                       == ProductStatus::OnSale
                   && importedProduct->imagePath()
                       == "images/products/1002.png",
               "新资源商品必须使用明确数据和可编辑默认字段。");

    if (!importWasCorrect
        || !require(
            ProductRepository::deleteProduct(
                "1002",
                &errorMessage),
            errorMessage)
        || !require(
            ProductRepository::importProductsFromDatabase(
                sourceDatabasePath,
                &errorMessage),
            errorMessage)
        || !require(
            ProductRepository::loadAllProducts(
                &products,
                &errorMessage),
            errorMessage))
    {
        return false;
    }

    return require(
        findProduct(products, "1002") == nullptr,
        "首次导入后用户删除的资源商品不能在下次启动时被恢复。");
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

    const QString mode =
        argc > 1
        ? QString::fromLocal8Bit(argv[1])
        : "empty";
    const bool succeeded =
        mode == "legacy"
        ? runLegacyMigrationTest()
        : mode == "resource"
        ? runProductResourceImportTest()
        : runEmptyDatabaseTest();

    if (succeeded)
    {
        QTextStream(stdout)
            << "PASS: " << mode << Qt::endl;
        return 0;
    }

    return 1;
}
