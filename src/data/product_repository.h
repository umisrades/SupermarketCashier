#ifndef PRODUCT_REPOSITORY_H
#define PRODUCT_REPOSITORY_H

#include <QHash>
#include <QString>

#include <vector>

#include "../models/product.h"

struct DailySalesStats
{
    QString date;
    int orderCount;
    double totalAmount;
    double averageOrder;
};

struct MonthlySalesStats
{
    QString month;
    int orderCount;
    double totalAmount;
    double averageOrder;
    int customerCount;
};

struct SalesRankingItem
{
    QString productId;
    QString productName;
    QString barcode;
    int totalQuantity;
    double totalRevenue;
    double unitPrice;
};

class ProductRepository
{
public:
    static bool initialize(QString *errorMessage = nullptr);

    static bool initializeAtPath(
        const QString &databasePath,
        QString *errorMessage = nullptr);

    static bool importProductsFromDatabase(
        const QString &sourceDatabasePath,
        QString *errorMessage = nullptr);

    static bool loadAllProducts(
        std::vector<Product> *products,
        QString *errorMessage = nullptr);

    static bool loadOnSaleProducts(
        std::vector<Product> *products,
        QString *errorMessage = nullptr);

    static bool getProductById(
        const QString &productId,
        Product *product,
        QString *errorMessage = nullptr);

    static bool insertProduct(
        const Product &product,
        QString *errorMessage = nullptr);

    static bool updateStock(
        const QString &productId,
        int stock,
        QString *errorMessage = nullptr);

    static bool updateWarningStock(
        const QString &productId,
        int warningStock,
        QString *errorMessage = nullptr);

    static bool deleteProduct(
        const QString &productId,
        QString *errorMessage = nullptr);

    static bool updateStocks(
        const QHash<QString, int> &stockByProductId,
        QString *errorMessage = nullptr);

    static bool commitSale(
        const QHash<QString, int> &quantityByProductId,
        QString *errorMessage = nullptr);

    static bool commitSaleWithRecord(
        const QHash<QString, int> &quantityByProductId,
        int cashierId,
        double totalAmount,
        double paidAmount,
        QString *errorMessage = nullptr);

    static bool getDailySalesStats(
        const QString &date,
        DailySalesStats *stats,
        QString *errorMessage = nullptr);

    static bool getSalesStatsForRange(
        const QString &dateFrom,
        const QString &dateTo,
        DailySalesStats *stats,
        QString *errorMessage = nullptr);

    static bool getMonthlySalesStats(
        int year,
        int month,
        MonthlySalesStats *stats,
        QString *errorMessage = nullptr);

    static bool getSalesRanking(
        const QString &dateFrom,
        const QString &dateTo,
        std::vector<SalesRankingItem> *ranking,
        QString *errorMessage = nullptr);

    static bool getTodaySalesStats(
        DailySalesStats *stats,
        QString *errorMessage = nullptr);

    static bool getMonthToDateSalesStats(
        MonthlySalesStats *stats,
        QString *errorMessage = nullptr);
};

#endif // PRODUCT_REPOSITORY_H
