#include "product.h"

Product::Product()
    : m_priceInCent(0),
    m_stock(0),
    m_warningStock(0),
    m_status(ProductStatus::Pending)
{
}

Product::Product(const QString &productId,
                 const QString &barcode,
                 const QString &name,
                 const QString &category,
                 const QString &unit,
                 qint64 priceInCent,
                 int stock,
                 int warningStock,
                 ProductStatus status)
    : m_productId(productId),
    m_barcode(barcode),
    m_name(name),
    m_category(category),
    m_unit(unit),
    m_priceInCent(priceInCent),
    m_stock(stock),
    m_warningStock(warningStock),
    m_status(status)
{
}

Product::Product(const QString &productId,
                 const QString &barcode,
                 const QString &name,
                 qint64 priceInCent,
                 int stock,
                 int warningStock,
                 ProductStatus status,
                 const QString &updateTime)
    : m_productId(productId),
    m_barcode(barcode),
    m_name(name),
    m_category("未分类"),
    m_unit("件"),
    m_priceInCent(priceInCent),
    m_stock(stock),
    m_warningStock(warningStock),
    m_status(status),
    m_updateTime(updateTime)
{
}

QString Product::productId() const
{
    return m_productId;
}

QString Product::barcode() const
{
    return m_barcode;
}

QString Product::name() const
{
    return m_name;
}

QString Product::category() const
{
    return m_category;
}

QString Product::unit() const
{
    return m_unit;
}

qint64 Product::priceInCent() const
{
    return m_priceInCent;
}

int Product::stock() const
{
    return m_stock;
}

int Product::warningStock() const
{
    return m_warningStock;
}

ProductStatus Product::status() const
{
    return m_status;
}

QString Product::updateTime() const
{
    return m_updateTime;
}

QString Product::imagePath() const
{
    return m_imagePath;
}

void Product::setProductId(const QString &productId)
{
    m_productId = productId;
}

void Product::setBarcode(const QString &barcode)
{
    m_barcode = barcode;
}

void Product::setName(const QString &name)
{
    m_name = name;
}

void Product::setCategory(const QString &category)
{
    m_category = category;
}

void Product::setUnit(const QString &unit)
{
    m_unit = unit;
}

void Product::setPriceInCent(qint64 priceInCent)
{
    if (priceInCent >= 0)
    {
        m_priceInCent = priceInCent;
    }
}

void Product::setStock(int stock)
{
    if (stock >= 0)
    {
        m_stock = stock;
    }
}

void Product::setWarningStock(int warningStock)
{
    if (warningStock >= 0)
    {
        m_warningStock = warningStock;
    }
}

void Product::setStatus(ProductStatus status)
{
    m_status = status;
}

void Product::setUpdateTime(const QString &updateTime)
{
    m_updateTime = updateTime;
}

void Product::setImagePath(const QString &imagePath)
{
    m_imagePath = imagePath;
}

QString Product::statusToString(ProductStatus status)
{
    switch (status)
    {
    case ProductStatus::OnSale:
        return "已上架";
    case ProductStatus::OffSale:
        return "已下架";
    case ProductStatus::Pending:
        return "待上架";
    }

    return "待上架";
}

ProductStatus Product::stringToStatus(const QString &text)
{
    if (text == "已上架")
    {
        return ProductStatus::OnSale;
    }

    if (text == "已下架")
    {
        return ProductStatus::OffSale;
    }

    return ProductStatus::Pending;
}

bool Product::isOnSale() const
{
    return m_status == ProductStatus::OnSale;
}

bool Product::isLowStock() const
{
    return m_stock <= m_warningStock;
}

bool Product::hasEnoughStock(int quantity) const
{
    return quantity > 0 && m_stock >= quantity;
}

bool Product::increaseStock(int quantity)
{
    if (quantity <= 0)
    {
        return false;
    }

    m_stock += quantity;
    return true;
}

bool Product::decreaseStock(int quantity)
{
    if (quantity <= 0 || quantity > m_stock)
    {
        return false;
    }

    m_stock -= quantity;
    return true;
}
