#include "inventory_widget.h"
#include "ui_inventory_widget.h"

#include "../data/product_repository.h"

#include <algorithm>

#include <QAbstractItemView>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidgetItem>
#include <QColor>
#include <QVBoxLayout>
#include <QtMath>


static QTableWidgetItem *createTableItem(
    const QString &text)
{
    QTableWidgetItem *item =
        new QTableWidgetItem(text);

    item->setTextAlignment(Qt::AlignCenter);

    return item;
}

static QTableWidgetItem *createStatusItem(
    const QString &text,
    const QString &statusType)
{
    QTableWidgetItem *item =
        new QTableWidgetItem(text);

    item->setTextAlignment(Qt::AlignCenter);

    if (statusType == "低库存" || statusType == "库存不足")
    {
        item->setBackground(QColor(248, 113, 113));
        item->setForeground(QColor(255, 255, 255));
    }
    else if (statusType == "无库存")
    {
        item->setBackground(QColor(156, 163, 175));
        item->setForeground(QColor(255, 255, 255));
    }

    return item;
}


InventoryWidget::InventoryWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::InventoryWidget)
{
    ui->setupUi(this);

    initUi();

    QString databaseError;

    if (!ProductRepository::loadAllProducts(
            &m_products,
            &databaseError))
    {
        QMessageBox::critical(
            this,
            "数据库错误",
            databaseError);
    }

    rebuildProductIndexes();

    refreshInventoryTable(m_products);
    updateSummary();



    //建立对应的槽函数对应关系
    connect(ui->searchButton,
            &QPushButton::clicked,
            this,
            &InventoryWidget::onSearchClicked);

    connect(ui->resetButton,
            &QPushButton::clicked,
            this,
            &InventoryWidget::onResetClicked);

    connect(ui->refreshButton,
            &QPushButton::clicked,
            this,
            &InventoryWidget::onRefreshClicked);

    connect(ui->confirmButton,
            &QPushButton::clicked,
            this,
            &InventoryWidget::onConfirmClicked);

    connect(ui->setWarningButton,
            &QPushButton::clicked,
            this,
            &InventoryWidget::onSetWarningClicked);

    connect(ui->addProductButton,
            &QPushButton::clicked,
            this,
            &InventoryWidget::onAddProductClicked);

    connect(ui->inventoryTable,
            &QTableWidget::itemSelectionChanged,
            this,
            &InventoryWidget::onTableSelectionChanged);
}


InventoryWidget::~InventoryWidget()
{
    delete ui;
}


void InventoryWidget::initUi()
{
    ui->inventoryTable->setColumnCount(10);

    ui->inventoryTable->setHorizontalHeaderLabels({
        "商品编号",
        "条形码",
        "商品名称",
        "商品分类",
        "单位",
        "单价（元）",
        "当前库存",
        "预警值",
        "商品状态",
        "库存状态"
    });

    ui->inventoryTable->setRowHeight(0, 36);
    ui->inventoryTable->verticalHeader()->setDefaultSectionSize(32);

    QHeaderView *header = ui->inventoryTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::Fixed);
    header->setSectionResizeMode(4, QHeaderView::Fixed);
    header->setSectionResizeMode(5, QHeaderView::Fixed);
    header->setSectionResizeMode(6, QHeaderView::Fixed);
    header->setSectionResizeMode(7, QHeaderView::Fixed);
    header->setSectionResizeMode(8, QHeaderView::Fixed);
    header->setSectionResizeMode(9, QHeaderView::Fixed);

    ui->inventoryTable->setColumnWidth(0, 90);
    ui->inventoryTable->setColumnWidth(1, 90);
    ui->inventoryTable->setColumnWidth(3, 90);
    ui->inventoryTable->setColumnWidth(4, 60);
    ui->inventoryTable->setColumnWidth(5, 90);
    ui->inventoryTable->setColumnWidth(6, 90);
    ui->inventoryTable->setColumnWidth(7, 70);
    ui->inventoryTable->setColumnWidth(8, 80);
    ui->inventoryTable->setColumnWidth(9, 80);

    header->setStyleSheet(
        "QHeaderView::section { "
        "    background-color: #1a1a1a; "
        "    color: white; "
        "    font-weight: bold; "
        "    padding: 8px; "
        "    border: 1px solid #333333; "
        "}");

    ui->inventoryTable->setStyleSheet(
        "QTableWidget { "
        "    gridline-color: #333333; "
        "    alternate-background-color: #1a1a1a; "
        "    background-color: black; "
        "    color: white; "
        "}"
        "QTableWidget::item { "
        "    color: white; "
        "}"
        "QTableWidget::item:selected { "
        "    background-color: #667eea; "
        "    color: white; "
        "}");

    ui->inventoryTable->verticalHeader()->setStyleSheet(
        "QHeaderView::section { "
        "    background-color: #1a1a1a; "
        "    color: white; "
        "}");

    ui->inventoryTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);

    ui->inventoryTable->setSelectionMode(
        QAbstractItemView::SingleSelection);

    ui->inventoryTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    // 初始化商品状态下拉框
    ui->statusComboBox->clear();
    ui->statusComboBox->addItem("全部");
    ui->statusComboBox->addItem("待上架");
    ui->statusComboBox->addItem("已上架");
    ui->statusComboBox->addItem("已下架");

    // 初始化库存操作下拉框
    ui->operationTypeComboBox->clear();
    ui->operationTypeComboBox->addItem("商品入库");
    ui->operationTypeComboBox->addItem("商品报损");
    ui->operationTypeComboBox->addItem("盘点调整");
    ui->operationTypeComboBox->addItem("删除商品");

    // 数量范围
    ui->quantitySpinBox->setMinimum(1);
    ui->quantitySpinBox->setMaximum(99999);

    // 预警值范围
    ui->warningValueSpinBox->setMinimum(0);
    ui->warningValueSpinBox->setMaximum(99999);

    ui->selectedProductLabel->setText(
        "当前选择：未选择商品");

    ui->statusTipLabel->setText(
        "可新增商品；选择已有商品后可入库、报损、盘点或随时删除");
}


void InventoryWidget::refreshInventoryTable(
    const std::vector<Product> &products)
{
    // 填充数据时暂时关闭排序
    // 避免插入数据过程中行位置发生改变
    ui->inventoryTable->setSortingEnabled(false);

    ui->inventoryTable->setRowCount(
        static_cast<int>(products.size()));

    for (int row = 0;
         row < static_cast<int>(products.size());
         ++row)
    {
        const Product &product = products[row];

        // 商品编号
        ui->inventoryTable->setItem(
            row,
            0,
            createTableItem(product.productId()));

        // 条形码
        ui->inventoryTable->setItem(
            row,
            1,
            createTableItem(product.barcode()));

        // 商品名称
        ui->inventoryTable->setItem(
            row,
            2,
            createTableItem(product.name()));

        // 商品分类
        ui->inventoryTable->setItem(
            row,
            3,
            createTableItem(product.category()));

        // 单位
        ui->inventoryTable->setItem(
            row,
            4,
            createTableItem(product.unit()));

        // 单价
        ui->inventoryTable->setItem(
            row,
            5,
            createTableItem(
                QString::number(
                    product.priceInCent() / 100.0,
                    'f',
                    2)));

        // 当前库存
        ui->inventoryTable->setItem(
            row,
            6,
            createTableItem(
                QString::number(product.stock())));

        // 预警值
        ui->inventoryTable->setItem(
            row,
            7,
            createTableItem(
                QString::number(
                    product.warningStock())));

        // 商品状态
        ui->inventoryTable->setItem(
            row,
            8,
            createTableItem(
                productStatusToString(
                    product.status())));

        // 库存状态
        const QString inventoryStatus = inventoryStatusText(product);
        ui->inventoryTable->setItem(
            row,
            9,
            createStatusItem(inventoryStatus, inventoryStatus));
    }

    // 数据填充完成后恢复排序功能
    ui->inventoryTable->setSortingEnabled(true);
}


void InventoryWidget::onAddProductClicked()
{
    QDialog dialog(this);

    dialog.setWindowTitle("新增商品入库");
    dialog.setMinimumWidth(430);

    // 商品编号输入框
    QLineEdit *productIdEdit =
        new QLineEdit(&dialog);

    // 条形码输入框
    QLineEdit *barcodeEdit =
        new QLineEdit(&dialog);

    // 商品名称输入框
    QLineEdit *nameEdit =
        new QLineEdit(&dialog);

    // 商品分类输入框
    QLineEdit *categoryEdit =
        new QLineEdit(&dialog);

    // 单位输入框
    QLineEdit *unitEdit =
        new QLineEdit(&dialog);

    productIdEdit->setPlaceholderText(
        "例如：P001");

    barcodeEdit->setPlaceholderText(
        "请输入唯一条形码");

    nameEdit->setPlaceholderText(
        "请输入商品名称");

    categoryEdit->setPlaceholderText(
        "例如：饮料、食品");

    unitEdit->setPlaceholderText(
        "例如：瓶、袋、盒");

    // 单价输入框
    QDoubleSpinBox *priceSpinBox =
        new QDoubleSpinBox(&dialog);

    priceSpinBox->setRange(
        0.01,
        999999.99);

    priceSpinBox->setDecimals(2);
    priceSpinBox->setValue(1.00);
    priceSpinBox->setSuffix(" 元");

    // 初始库存输入框
    QSpinBox *stockSpinBox =
        new QSpinBox(&dialog);

    stockSpinBox->setRange(0, 99999);
    stockSpinBox->setValue(1);

    // 库存预警值输入框
    QSpinBox *warningSpinBox =
        new QSpinBox(&dialog);

    warningSpinBox->setRange(0, 99999);
    warningSpinBox->setValue(10);

    // 商品状态下拉框
    QComboBox *productStatusComboBox =
        new QComboBox(&dialog);

    productStatusComboBox->addItem("待上架");
    productStatusComboBox->addItem("已上架");
    productStatusComboBox->addItem("已下架");

    // 默认选择“已上架”
    productStatusComboBox->setCurrentIndex(1);

    // 表单布局
    QFormLayout *formLayout =
        new QFormLayout;

    formLayout->addRow(
        "商品编号：",
        productIdEdit);

    formLayout->addRow(
        "条形码：",
        barcodeEdit);

    formLayout->addRow(
        "商品名称：",
        nameEdit);

    formLayout->addRow(
        "商品分类：",
        categoryEdit);

    formLayout->addRow(
        "单位：",
        unitEdit);

    formLayout->addRow(
        "单价：",
        priceSpinBox);

    formLayout->addRow(
        "初始库存：",
        stockSpinBox);

    formLayout->addRow(
        "库存预警值：",
        warningSpinBox);

    formLayout->addRow(
        "商品状态：",
        productStatusComboBox);

    // 确认、取消按钮
    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(
            QDialogButtonBox::Ok
                | QDialogButtonBox::Cancel,
            &dialog);

    buttonBox
        ->button(QDialogButtonBox::Ok)
        ->setText("确认入库");

    buttonBox
        ->button(QDialogButtonBox::Cancel)
        ->setText("取消");

    // 弹窗总布局
    QVBoxLayout *dialogLayout =
        new QVBoxLayout(&dialog);

    dialogLayout->addLayout(formLayout);
    dialogLayout->addWidget(buttonBox);

    // 取消按钮
    connect(
        buttonBox->button(
            QDialogButtonBox::Cancel),
        &QPushButton::clicked,
        &dialog,
        &QDialog::reject);

    // 确认按钮
    connect(
        buttonBox->button(
            QDialogButtonBox::Ok),
        &QPushButton::clicked,
        &dialog,
        [&]()
        {
            const QString productId =
                productIdEdit->text().trimmed();

            const QString barcode =
                barcodeEdit->text().trimmed();

            const QString name =
                nameEdit->text().trimmed();

            const QString category =
                categoryEdit->text().trimmed();

            const QString unit =
                unitEdit->text().trimmed();

            // 检查必填内容
            if (productId.isEmpty()
                || barcode.isEmpty()
                || name.isEmpty()
                || category.isEmpty()
                || unit.isEmpty())
            {
                QMessageBox::warning(
                    &dialog,
                    "提示",
                    "商品编号、条形码、商品名称、分类和单位不能为空。");

                return;
            }

            // 使用商品编号哈希表检查编号是否重复
            if (findProductById(productId) != nullptr)
            {
                QMessageBox::warning(
                    &dialog,
                    "提示",
                    "该商品编号已经存在，请更换商品编号。");

                return;
            }

            // 使用条形码哈希表检查条形码是否重复
            if (barcodeExists(barcode))
            {
                QMessageBox::warning(
                    &dialog,
                    "提示",
                    "该条形码已经存在，请更换条形码。");

                return;
            }

            dialog.accept();
        });

    productIdEdit->setFocus();

    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    // 根据下拉框取得商品状态
    ProductStatus status =
        ProductStatus::OnSale;

    if (productStatusComboBox->currentIndex() == 0)
    {
        status = ProductStatus::Pending;
    }
    else if (
        productStatusComboBox->currentIndex() == 2)
    {
        status = ProductStatus::OffSale;
    }

    // 将元转换成分
    const qint64 priceInCent =
        qRound64(
            priceSpinBox->value() * 100.0);

    const Product newProduct(
        productIdEdit->text().trimmed(),
        barcodeEdit->text().trimmed(),
        nameEdit->text().trimmed(),
        categoryEdit->text().trimmed(),
        unitEdit->text().trimmed(),
        priceInCent,
        stockSpinBox->value(),
        warningSpinBox->value(),
        status);

    QString databaseError;

    if (!ProductRepository::insertProduct(
            newProduct,
            &databaseError))
    {
        QMessageBox::critical(
            this,
            "入库失败",
            databaseError);
        return;
    }

    // 向 vector 中加入新商品
    m_products.push_back(newProduct);

    /*
     * vector 增加商品后：
     * 1. 新商品有了新的下标
     * 2. vector 还可能发生内存重新分配
     * 因此重新建立哈希索引。
     */
    rebuildProductIndexes();

    refreshInventoryTable(m_products);
    updateSummary();

    QMessageBox::information(
        this,
        "成功",
        "新商品入库成功。");
}


void InventoryWidget::onSearchClicked()
{
    const QString keyword =
        ui->keywordEdit->text().trimmed();

    const QString statusText =
        ui->statusComboBox->currentText();

    const bool onlyLowStock =
        ui->lowStockCheckBox->isChecked();

    std::vector<Product> result;

    /*
     * 判断一个商品是否满足：
     * 1. 商品状态条件
     * 2. 低库存条件
     *
     * 关键词匹配在后面单独处理。
     */
    auto matchesOtherConditions =
        [&](const Product &product)
    {
        const bool matchStatus =
            statusText == "全部"
            || productStatusToString(
                   product.status()) == statusText;

        const bool matchLowStock =
            !onlyLowStock
            || product.isLowStock();

        return matchStatus && matchLowStock;
    };

    /*
     * 没有输入关键词：
     * 遍历全部商品，只检查状态和低库存条件。
     */
    if (keyword.isEmpty())
    {
        for (const Product &product : m_products)
        {
            if (matchesOtherConditions(product))
            {
                result.push_back(product);
            }
        }

        refreshInventoryTable(result);
        return;
    }

    /*
     * 第一种情况：输入的是完整商品编号。
     * 使用商品编号哈希表进行平均 O(1) 查找。
     */
    Product *productById =
        findProductById(keyword);

    /*
     * 第二种情况：输入的是完整条形码。
     * 使用条形码哈希表进行平均 O(1) 查找。
     */
    Product *productByBarcode =
        findProductByBarcode(keyword);

    const bool foundExactProduct =
        productById != nullptr
        || productByBarcode != nullptr;

    if (foundExactProduct)
    {
        // 商品编号精确匹配
        if (productById != nullptr
            && matchesOtherConditions(
                *productById))
        {
            result.push_back(*productById);
        }

        /*
         * 条形码精确匹配。
         *
         * 如果商品编号和条形码恰好找到了同一个商品，
         * 不重复加入查询结果。
         */
        if (productByBarcode != nullptr)
        {
            const bool sameProduct =
                productById != nullptr
                && productById->productId()
                       == productByBarcode->productId();

            if (!sameProduct
                && matchesOtherConditions(
                    *productByBarcode))
            {
                result.push_back(
                    *productByBarcode);
            }
        }

        refreshInventoryTable(result);
        return;
    }

    /*
     * 没有精确匹配时，说明用户可能输入了：
     * 1. 商品名称
     * 2. 商品分类
     * 3. 部分商品编号
     * 4. 部分条形码
     *
     * 普通哈希表不支持 contains 模糊查找，
     * 所以这里继续遍历 vector。
     */
    for (const Product &product : m_products)
    {
        const bool matchKeyword =
            product.productId().contains(
                keyword,
                Qt::CaseInsensitive)

            || product.barcode().contains(
                keyword,
                Qt::CaseInsensitive)

            || product.name().contains(
                keyword,
                Qt::CaseInsensitive)

            || product.category().contains(
                keyword,
                Qt::CaseInsensitive);

        if (matchKeyword
            && matchesOtherConditions(product))
        {
            result.push_back(product);
        }
    }

    refreshInventoryTable(result);
}


void InventoryWidget::onResetClicked()
{
    ui->keywordEdit->clear();

    ui->statusComboBox->setCurrentIndex(0);

    ui->lowStockCheckBox->setChecked(false);

    ui->reasonEdit->clear();

    ui->inventoryTable->clearSelection();

    ui->selectedProductLabel->setText(
        "当前选择：未选择商品");

    refreshInventoryTable(m_products);
    updateSummary();
}


void InventoryWidget::onRefreshClicked()
{
    ui->inventoryTable->clearSelection();

    ui->selectedProductLabel->setText(
        "当前选择：未选择商品");

    refreshInventoryTable(m_products);
    updateSummary();
}


void InventoryWidget::onConfirmClicked()
{
    const int row = selectedRow();

    if (row < 0)
    {
        QMessageBox::warning(
            this,
            "提示",
            "请先选择一个商品。");

        return;
    }

    QTableWidgetItem *idItem =
        ui->inventoryTable->item(row, 0);

    if (idItem == nullptr)
    {
        QMessageBox::warning(
            this,
            "错误",
            "无法读取选中的商品编号。");

        return;
    }

    const QString productId =
        idItem->text();

    /*
     * 使用商品编号哈希表查找商品。
     * 不再遍历整个 vector。
     */
    Product *product =
        findProductById(productId);

    if (product == nullptr)
    {
        QMessageBox::warning(
            this,
            "错误",
            "未找到该商品数据。");

        return;
    }

    const QString operation =
        ui->operationTypeComboBox->currentText();

    /*
     * 删除商品：
     * 和库存数量没有关系，任何库存都可以删除。
     */
    if (operation == "删除商品")
    {
        const QString productName =
            product->name();

        const QMessageBox::StandardButton answer =
            QMessageBox::question(
                this,
                "确认删除",
                "确定要删除商品“"
                    + productName
                    + "”吗？\n"
                    + "该操作会把商品从当前商品列表中移除。",
                QMessageBox::Yes
                    | QMessageBox::No,
                QMessageBox::No);

        if (answer != QMessageBox::Yes)
        {
            return;
        }

        QString databaseError;

        if (!ProductRepository::deleteProduct(
                productId,
                &databaseError))
        {
            QMessageBox::critical(
                this,
                "删除失败",
                databaseError);
            return;
        }

        // 根据商品编号删除 vector 中的商品
        const auto newEnd =
            std::remove_if(
                m_products.begin(),
                m_products.end(),
                [&productId](
                    const Product &item)
                {
                    return item.productId()
                    == productId;
                });

        m_products.erase(
            newEnd,
            m_products.end());

        /*
         * 删除商品后，后面的 vector 下标会前移，
         * 因此必须重新建立哈希索引。
         */
        rebuildProductIndexes();

        ui->reasonEdit->clear();

        ui->inventoryTable->clearSelection();

        ui->selectedProductLabel->setText(
            "当前选择：未选择商品");

        refreshInventoryTable(m_products);
        updateSummary();

        QMessageBox::information(
            this,
            "成功",
            "商品已删除。");

        return;
    }

    const int quantity =
        ui->quantitySpinBox->value();

    const QString reason =
        ui->reasonEdit
            ->toPlainText()
            .trimmed();

    if ((operation == "商品报损"
         || operation == "盘点调整")
        && reason.isEmpty())
    {
        QMessageBox::warning(
            this,
            "提示",
            "报损或盘点调整必须填写原因。");

        return;
    }

    const int originalStock = product->stock();

    if (operation == "商品入库")
    {
        if (!product->increaseStock(quantity))
        {
            QMessageBox::warning(
                this,
                "错误",
                "入库数量不合法。");

            return;
        }
    }
    else if (operation == "商品报损")
    {
        if (!product->decreaseStock(quantity))
        {
            QMessageBox::warning(
                this,
                "错误",
                "库存不足，不能报损这么多。");

            return;
        }
    }
    else if (operation == "盘点调整")
    {
        product->setStock(quantity);
    }
    else
    {
        QMessageBox::warning(
            this,
            "错误",
            "未知库存操作类型。");

        return;
    }

    QString databaseError;

    if (!ProductRepository::updateStock(
            productId,
            product->stock(),
            &databaseError))
    {
        product->setStock(originalStock);

        QMessageBox::critical(
            this,
            "库存操作失败",
            databaseError);
        return;
    }

    /*
     * 入库、报损、盘点只修改库存，
     * 不会改变商品编号和条形码，
     * 所以不需要重建哈希索引。
     */
    ui->reasonEdit->clear();

    ui->inventoryTable->clearSelection();

    ui->selectedProductLabel->setText(
        "当前选择：未选择商品");

    refreshInventoryTable(m_products);
    updateSummary();

    QMessageBox::information(
        this,
        "成功",
        "库存操作成功。");
}


void InventoryWidget::onTableSelectionChanged()
{
    const int row = selectedRow();

    if (row < 0)
    {
        ui->selectedProductLabel->setText(
            "当前选择：未选择商品");

        return;
    }

    QTableWidgetItem *idItem =
        ui->inventoryTable->item(row, 0);

    QTableWidgetItem *nameItem =
        ui->inventoryTable->item(row, 2);

    QTableWidgetItem *stockItem =
        ui->inventoryTable->item(row, 6);

    QTableWidgetItem *warningItem =
        ui->inventoryTable->item(row, 7);

    if (idItem == nullptr
        || nameItem == nullptr
        || stockItem == nullptr
        || warningItem == nullptr)
    {
        ui->selectedProductLabel->setText(
            "当前选择：未选择商品");

        return;
    }

    ui->selectedProductLabel->setText(
        "当前选择："
        + nameItem->text()
        + "，商品编号："
        + idItem->text()
        + "，库存："
        + stockItem->text()
        + "，预警值："
        + warningItem->text());

    ui->warningValueSpinBox->setValue(
        warningItem->text().toInt());
}


void InventoryWidget::onSetWarningClicked()
{
    const int row = selectedRow();

    if (row < 0)
    {
        QMessageBox::warning(
            this,
            "提示",
            "请先选择一个商品。");

        return;
    }

    QTableWidgetItem *idItem =
        ui->inventoryTable->item(row, 0);

    if (idItem == nullptr)
    {
        QMessageBox::warning(
            this,
            "错误",
            "无法读取选中的商品编号。");

        return;
    }

    /*
     * 根据商品编号进行哈希查找。
     */
    Product *product =
        findProductById(idItem->text());

    if (product == nullptr)
    {
        QMessageBox::warning(
            this,
            "错误",
            "未找到该商品。");

        return;
    }

    const int originalWarningStock =
        product->warningStock();
    const int newWarningStock =
        ui->warningValueSpinBox->value();

    product->setWarningStock(newWarningStock);

    QString databaseError;

    if (!ProductRepository::updateWarningStock(
            product->productId(),
            newWarningStock,
            &databaseError))
    {
        product->setWarningStock(
            originalWarningStock);

        QMessageBox::critical(
            this,
            "设置失败",
            databaseError);
        return;
    }

    /*
     * 这里只修改预警值，
     * 不修改编号和条形码，
     * 因此不需要重建哈希索引。
     */
    refreshInventoryTable(m_products);
    updateSummary();

    ui->inventoryTable->clearSelection();

    ui->selectedProductLabel->setText(
        "当前选择：未选择商品");

    QMessageBox::information(
        this,
        "成功",
        "库存预警值设置成功。");
}


void InventoryWidget::updateSummary()
{
    const int totalProducts =
        static_cast<int>(m_products.size());

    int totalStock = 0;
    int lowStockCount = 0;
    int outOfStockCount = 0;

    for (const Product &product : m_products)
    {
        totalStock += product.stock();

        if (product.stock() == 0)
        {
            ++outOfStockCount;
        }

        if (product.isLowStock())
        {
            ++lowStockCount;
        }
    }

    ui->totalProductsValueLabel->setText(
        QString::number(totalProducts));

    ui->totalStockValueLabel->setText(
        QString::number(totalStock));

    ui->lowStockValueLabel->setText(
        QString::number(lowStockCount));

    ui->outOfStockValueLabel->setText(
        QString::number(outOfStockCount));
}


int InventoryWidget::selectedRow() const
{
    const QList<QTableWidgetItem *> selectedItems =
        ui->inventoryTable->selectedItems();

    if (selectedItems.isEmpty())
    {
        return -1;
    }

    return selectedItems.first()->row();
}


void InventoryWidget::rebuildProductIndexes()
{
    // 清空原来的哈希索引
    m_productIdIndex.clear();
    m_barcodeIndex.clear();

    /*
     * 遍历 vector，将每个商品的：
     * 1. 商品编号
     * 2. 条形码
     *
     * 与它在 vector 中的下标建立对应关系。
     */
    for (int index = 0;
         index < static_cast<int>(
             m_products.size());
         ++index)
    {
        const Product &product =
            m_products[index];

        if (!product.productId().isEmpty())
        {
            m_productIdIndex.insert(
                product.productId(),
                index);
        }

        if (!product.barcode().isEmpty())
        {
            m_barcodeIndex.insert(
                product.barcode(),
                index);
        }
    }
}


Product *InventoryWidget::findProductById(
    const QString &productId)
{
    /*
     * constFind 不会因为查询不存在的 key，
     * 自动向 QHash 中插入新数据。
     */
    const auto iterator =
        m_productIdIndex.constFind(productId);

    if (iterator
        == m_productIdIndex.constEnd())
    {
        return nullptr;
    }

    const int index = iterator.value();

    // 防止哈希索引中的下标越界
    if (index < 0
        || index >= static_cast<int>(
               m_products.size()))
    {
        return nullptr;
    }

    /*
     * 再检查一次编号是否对应，
     * 可以避免索引意外没有及时更新时访问错误商品。
     */
    if (m_products[index].productId()
        != productId)
    {
        return nullptr;
    }

    return &m_products[index];
}


Product *InventoryWidget::findProductByBarcode(
    const QString &barcode)
{
    const auto iterator =
        m_barcodeIndex.constFind(barcode);

    if (iterator
        == m_barcodeIndex.constEnd())
    {
        return nullptr;
    }

    const int index = iterator.value();

    // 防止下标越界
    if (index < 0
        || index >= static_cast<int>(
               m_products.size()))
    {
        return nullptr;
    }

    // 检查条形码是否对应
    if (m_products[index].barcode()
        != barcode)
    {
        return nullptr;
    }

    return &m_products[index];
}


bool InventoryWidget::barcodeExists(
    const QString &barcode) const
{
    /*
     * 使用条形码哈希表判断。
     * 不再遍历所有商品。
     */
    return m_barcodeIndex.contains(barcode);
}


QString InventoryWidget::productStatusToString(
    ProductStatus status) const
{
    switch (status)
    {
    case ProductStatus::Pending:
        return "待上架";

    case ProductStatus::OnSale:
        return "已上架";

    case ProductStatus::OffSale:
        return "已下架";
    }

    return "未知";
}


QString InventoryWidget::inventoryStatusText(
    const Product &product) const
{
    if (product.stock() == 0)
    {
        return "无库存";
    }

    if (product.stock()
        <= product.warningStock())
    {
        return "库存不足";
    }

    return "正常";
}
