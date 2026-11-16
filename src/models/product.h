#ifndef PRODUCT_H
#define PRODUCT_H
#include <QString>
#include <QtGlobal>

#include "system_enums.h"

class Product
{
public:
    Product();

    Product(const QString &productId,
            const QString &barcode,
            const QString &name,
            const QString &category,
            const QString &unit,
            qint64 priceInCent,
            int stock,
            int warningStock,
            ProductStatus status);

    // 兼容管理控制台原有商品构造方式。
    // 控制台未提供分类和单位时使用默认值。
    Product(const QString &productId,
            const QString &barcode,
            const QString &name,
            qint64 priceInCent,
            int stock,
            int warningStock,
            ProductStatus status,
            const QString &updateTime);

    QString productId() const;
    QString barcode() const;
    QString name() const;
    QString category() const;
    QString unit() const;

    qint64 priceInCent() const;
    int stock() const;
    int warningStock() const;
    ProductStatus status() const;
    QString updateTime() const;
    QString imagePath() const;

    void setProductId(const QString &productId);
    void setBarcode(const QString &barcode);
    void setName(const QString &name);
    void setCategory(const QString &category);
    void setUnit(const QString &unit);
    void setPriceInCent(qint64 priceInCent);
    void setStock(int stock);
    void setWarningStock(int warningStock);
    void setStatus(ProductStatus status);
    void setUpdateTime(const QString &updateTime);
    void setImagePath(const QString &imagePath);

    static QString statusToString(ProductStatus status);
    static ProductStatus stringToStatus(const QString &text);

    bool isOnSale() const;
    bool isLowStock() const;
    bool hasEnoughStock(int quantity) const;

    bool increaseStock(int quantity);
    bool decreaseStock(int quantity);

private:
    QString m_productId;
    QString m_barcode;
    QString m_name;
    QString m_category;
    QString m_unit;

    qint64 m_priceInCent;
    int m_stock;
    int m_warningStock;

    ProductStatus m_status;
    QString m_updateTime;
    QString m_imagePath;
};
#endif // PRODUCT_H
