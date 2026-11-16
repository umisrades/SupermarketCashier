#ifndef INVENTORY_WIDGET_H
#define INVENTORY_WIDGET_H

#include <QHash>
#include <QString>
#include <QWidget>

#include <vector>

#include "../models/product.h"

namespace Ui {
class InventoryWidget;
}

class InventoryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit InventoryWidget(QWidget *parent = nullptr);
    ~InventoryWidget();

private slots:
    // 查询按钮
    void onSearchClicked();

    // 重置按钮
    void onResetClicked();

    // 刷新按钮
    void onRefreshClicked();

    // 确认库存操作：
    // 已有商品入库、报损、盘点调整、删除商品
    void onConfirmClicked();

    // 设置库存预警值
    void onSetWarningClicked();

    // 新增商品入库
    void onAddProductClicked();

    // 表格选中行变化
    void onTableSelectionChanged();

private:
    // 初始化界面
    void initUi();

    // 刷新库存表格
    void refreshInventoryTable(
        const std::vector<Product> &products);

    // 更新顶部统计信息
    void updateSummary();

    // 获取当前选中的表格行号
    // 没有选中时返回 -1
    int selectedRow() const;

    // 根据 m_products 重新建立哈希索引
    void rebuildProductIndexes();

    // 根据商品编号进行哈希查找
    Product *findProductById(
        const QString &productId);

    // 根据条形码进行哈希查找
    Product *findProductByBarcode(
        const QString &barcode);

    // 判断条形码是否已经存在
    bool barcodeExists(
        const QString &barcode) const;

    // 商品状态转成界面文字
    QString productStatusToString(
        ProductStatus status) const;

    // 判断库存状态
    QString inventoryStatusText(
        const Product &product) const;

private:
    Ui::InventoryWidget *ui;

    // vector 负责保存、遍历和表格显示
    std::vector<Product> m_products;

    // 商品编号哈希索引
    // key：商品编号
    // value：商品在 m_products 中的下标
    QHash<QString, int> m_productIdIndex;

    // 条形码哈希索引
    // key：条形码
    // value：商品在 m_products 中的下标
    QHash<QString, int> m_barcodeIndex;
};

#endif // INVENTORY_WIDGET_H