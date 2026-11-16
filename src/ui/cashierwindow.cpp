#include "cashierwindow.h"
#include "ui_cashierwindow.h"

#include "inventory_widget.h"
#include "logindialog.h"
#include "statistics_dialog.h"
#include "../data/product_repository.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QGridLayout>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLayoutItem>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QStatusBar>

CashierWindow::CashierWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CashierWindow)
{
    ui->setupUi(this);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows); //点击时能一次选中整行，为后续删除商品做铺垫
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    userStatusLabel = new QLabel(this);
    userStatusLabel->setObjectName("userStatusLabel");

    connect(ui->actionInventoryManagement,&QAction::triggered,this,&CashierWindow::onInventoryManagementTriggered);
    connect(ui->actionStatistics,&QAction::triggered,this,&CashierWindow::onStatisticsTriggered);
    connect(ui->actionLogout,&QAction::triggered,this,&CashierWindow::onLogoutTriggered);
    connect(ui->actionExit,&QAction::triggered,this,&QWidget::close);

    statusBar()->addPermanentWidget(userStatusLabel);
    updateUserStatus();

    loadData();
    loadProduct();
}

CashierWindow::~CashierWindow()
{
    if(!inventoryWindow.isNull())
    {
        disconnect(inventoryWindow,nullptr,this,nullptr);
        delete inventoryWindow.data();
        inventoryWindow = nullptr;
    }
    if(!statisticsDialog.isNull())
    {
        delete statisticsDialog.data();
        statisticsDialog = nullptr;
    }
    delete ui;
}

//创建数据库
void CashierWindow::createData()
{
    QString errorMessage;
    if(!ProductRepository::initialize(&errorMessage))
        QMessageBox::critical(this,"数据库错误",errorMessage);
}

//加载数据
void CashierWindow::loadData()
{
    std::vector<Product> products;
    QString errorMessage;
    if(!ProductRepository::loadOnSaleProducts(&products,&errorMessage))
    {
        QMessageBox::critical(this,"数据库错误",errorMessage);
        return;
    }

    productHash.clear();
    for(const Product &product : products)
    {
        CashierProduct p;
        p.id = product.productId();
        p.barcode = product.barcode();
        p.imagePath = product.imagePath();
        p.name = product.name();
        p.price = product.priceInCent() / 100.0;
        p.quantity = product.stock();
        productHash.insert(p.barcode,p);
    }
}

//保存数据
void CashierWindow::saveData()
{
    QHash<QString,int> stockByProductId;
    for(auto it = productHash.constBegin();it!=productHash.constEnd();++it)
        stockByProductId.insert(it.value().id,it.value().quantity);

    QString errorMessage;
    if(!ProductRepository::updateStocks(stockByProductId,&errorMessage))
        QMessageBox::critical(this,"保存失败",errorMessage);
}

void CashierWindow::setCurrentUser(const User &user)
{
    curUser = user;
    updateUserStatus();
}

const User &CashierWindow::currentUser() const
{
    return curUser;
}

//按钮点击
void CashierWindow::on_SearchandAdd_clicked()
{
    //输入
    QString inputBarcode = ui->priceInput->text().trimmed();
    if(inputBarcode.isEmpty())
    {
        QMessageBox::information(this,"提示","请输入商品条形码");
        return;
    }

    //查找商品
    if(!productHash.contains(inputBarcode))
    {
        QMessageBox::warning(this,"提示","未找到该条形码对应的已上架商品");
        return;
    }
    CashierProduct productFound = productHash.value(inputBarcode);
    double price = productFound.price;
    int quantity = 1;
    double subtotal = price*quantity;
    QString name = productFound.name;

    if(productFound.quantity <= 0)
    {
        QMessageBox::warning(this,"库存不足","该商品当前没有库存。");
        return;
    }

    if(cartQuantityForProduct(productFound.id) >= productFound.quantity)
    {
        QMessageBox::warning(this,"库存不足","购物车中的该商品数量已经达到当前库存上限。");
        return;
    }

    //先检查购物车是否已经添加这个商品了
    int existIndex = -1;
    for(int i = 0;i<ui->tableWidget->rowCount();++i)
    {
        if(ui->tableWidget->item(i,0)->text() == productFound.id)
        {
            existIndex = i;
            break;
        }
    }

    isUpdating = true;

    //假如有了，再添加就只增加该商品的数量，不用新建一行
    if(existIndex != -1)
    {
        QTableWidgetItem *qtyItem = ui->tableWidget->item(existIndex,3);
        QTableWidgetItem *subItem = ui->tableWidget->item(existIndex,4);
        int newQuantity = qtyItem->text().toInt() + 1;
        double newSubtotal = price * newQuantity;
        qtyItem->setText(QString::number(newQuantity));
        subItem->setText(QString::number(newSubtotal,'f',2));
    }
    else
    {
        //假如没有这个商品就新建一行，然后设置购物车具体的数据的格式并创建数据
        int currentRow = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(currentRow);
        ui->tableWidget->setItem(currentRow,0,new QTableWidgetItem(productFound.id));
        ui->tableWidget->setItem(currentRow,1,new QTableWidgetItem(name));
        ui->tableWidget->setItem(currentRow,2,new QTableWidgetItem(QString::number(price,'f',2)));
        ui->tableWidget->setItem(currentRow,3,new QTableWidgetItem(QString::number(quantity)));
        ui->tableWidget->setItem(currentRow,4,new QTableWidgetItem(QString::number(subtotal,'f',2)));
    }

    isUpdating = false;
    updateTotalMoney();
    ui->priceInput->clear();
}

//输入框按enter之后重新渲染右侧
void CashierWindow::on_priceInput_returnPressed()
{
    loadProduct();
}

//商品预览界面
void CashierWindow::loadProduct()
{
    QWidget *container = ui->widget;
    QLayout *layout = container->layout();
    QGridLayout *gridLayout = qobject_cast<QGridLayout *>(layout);

    if(!gridLayout)
    {
        if(layout) delete layout;
        gridLayout = new QGridLayout(container);
    }
    else
    {
        QLayoutItem *item;
        while((item = gridLayout->takeAt(0)) != nullptr)
        {
            if(item->widget()) delete item->widget();
            delete item;
        }
    }

    //设置间距与格式
    gridLayout->setSpacing(10);
    gridLayout->setContentsMargins(10,10,10,10);

    //根据输入框里的内容对图标做限制，假如输入完整条形码并按enter，将会显示唯一的那个图标，从而实现"查询"功能
    QString inputPrefix = ui->priceInput->text().trimmed();
    int row = 0,col = 0,count = 0;

    for(auto it = productHash.begin();it!=productHash.end();++it)
    {
        CashierProduct p = it.value();

        if(!inputPrefix.isEmpty() && !p.barcode.startsWith(inputPrefix,Qt::CaseInsensitive))
            continue;
        count++;
        /*QToolButton *btn = new QToolButton();
        btn->setFixedSize(140,120);
        btn->setText(p.name);
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setIcon(QIcon("image/"+p.id+".png"));
        btn->setIconSize(QSize(90,90));
        connect(btn,&QToolButton::clicked,[this,p](){
            ui->priceInput->setText(p.barcode);
            on_SearchandAdd_clicked();
        });
        static_cast<QGridLayout*>(container->layout())->addWidget(btn,row,col);
        col++;
        if(col>=3)
        {
            col=0;
            row++;
        }*/
    }

    //解决滚动条不存在问题，通过硬编码定义滚动区域大小
    int totalItems = count;
    int cols = 3;
    int rows = (totalItems + cols - 1) / cols;
    int buttonHeight = 120;
    int spacing = 10;
    int margins = 10;
    int needHeight = (rows-1)*(buttonHeight+spacing)+margins;

    ui->scrollAreaWidgetContents->setMinimumSize(550,needHeight);
    ui->scrollAreaWidgetContents->setMaximumSize(550,needHeight+150);

    for(auto it = productHash.begin();it!=productHash.end();++it)
    {
        CashierProduct p = it.value();

        if(!inputPrefix.isEmpty() && !p.barcode.startsWith(inputPrefix,Qt::CaseInsensitive))
            continue;

        QToolButton *btn = new QToolButton();
        btn->setFixedSize(140,120);
        btn->setText(p.name);
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setIcon(QIcon("image/"+p.id+".png"));
        btn->setIconSize(QSize(90,90));

        connect(btn,&QToolButton::clicked,[this,p](){
            ui->priceInput->setText(p.barcode);
            on_SearchandAdd_clicked();
        });

        gridLayout->addWidget(btn,row,col);
        col++;
        if(col>=3)
        {
            col=0;
            row++;
        }
    }

    //让布局自动计算理想尺寸，并强制内容部件刚好这么大(以下内容解决了搜索时按钮位置渲染问题）
    gridLayout->setSizeConstraint(QLayout::SetFixedSize);
    ui->scrollAreaWidgetContents->adjustSize();
    ui->scrollArea->setWidgetResizable(false);
}

//付款按钮与付款逻辑的实现
void CashierWindow::on_payButton_clicked()
{
    if(ui->tableWidget->rowCount() == 0)
    {
        QMessageBox::information(this,"提示","购物车为空，请先添加商品！");
        return;
    }

    //调用总金额
    double total = ui->label_totalMoney->text().replace(" 元","").toDouble();

    //模拟收款的输入框（由于具体的支付宝微信支付等支付手段牵扯到很多其他方面的东西，所以暂时用手动输入来模拟包括现金和网上支付在内的所有支付手段）
    bool ok;
    double paid = QInputDialog::getDouble(this,"交易结算",
                                          QString("本次合计：%1 元\n请输入顾客付款金额：").arg(total,0,'f',2),
                                          0.0,0.0,999999.0,2,&ok);
    if(!ok) return;//用户按了取消

    QHash<QString,int> saleQuantities;
    for(int row = 0;row<ui->tableWidget->rowCount();++row)
    {
        QString id = ui->tableWidget->item(row,0)->text();
        int qty = ui->tableWidget->item(row,3)->text().toInt();
        saleQuantities[id] += qty;
    }

    //防止找零为负数，当付款小于总价的时候弹出警告窗口
    double change = paid - total;
    if(change < 0)
    {
        QMessageBox::warning(this,"付款不足",
                             QString("顾客付款金额 %1 元小于总价 %2 元，请重新输入！")
                                 .arg(paid,0,'f',2).arg(total,0,'f',2));
        return;
    }

    //生成收银小票
    QString receipt;
    receipt.append("==============================\n");
    receipt.append("             🛒 快乐购超市 收银小票\n");
    receipt.append("==============================\n");
    receipt.append("时间：" + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "\n");
    receipt.append("----------------------------\n");
    receipt.append("商品名称    数量  单价   小计\n");
    receipt.append("----------------------------\n");

    for(int i = 0;i<ui->tableWidget->rowCount();++i)
    {
        QString name = ui->tableWidget->item(i,1)->text();
        QString qty  = ui->tableWidget->item(i,3)->text();
        QString price= ui->tableWidget->item(i,2)->text();
        QString sub  = ui->tableWidget->item(i,4)->text();

        receipt.append(QString("%1  x%2  %3  %4\n")
                           .arg(name.leftJustified(8,' '))
                           .arg(qty.rightJustified(2,' '))
                           .arg(price.rightJustified(6,' '))
                           .arg(sub.rightJustified(7,' ')));
    }

    receipt.append("----------------------------\n");
    receipt.append(QString("合计：     %1 元\n").arg(total,0,'f',2));
    receipt.append(QString("实收：     %1 元\n").arg(paid,0,'f',2));
    receipt.append(QString("找零：     %1 元\n").arg(change,0,'f',2));
    receipt.append("==============================\n");
    receipt.append("       谢谢惠顾，欢迎下次光临   \n");

    //调用数据库记录交易和扣减库存
    QString errorMessage;
    if(!ProductRepository::commitSaleWithRecord(saleQuantities,curUser.userId(),total,paid,&errorMessage))
    {
        QMessageBox::warning(this,"结算失败",errorMessage);
        loadData();
        loadProduct();
        return;
    }

    QMessageBox::information(this,"交易成功，小票生成",receipt);

    isUpdating = true;
    ui->tableWidget->setRowCount(0);
    isUpdating = false;
    ui->label_totalMoney->setText("0.00 元");

    loadData();
    loadProduct();
}

//挂起恢复
void CashierWindow::on_Hangup_clicked()
{
    if(!isSuspended)
    {
        if(ui->tableWidget->rowCount() == 0)
        {
            QMessageBox::information(this,"提示","购物车为空，没有可以挂起的商品。");
            return;
        }

        suspendedCart.clear();//清空旧的缓存

        //将当前表格里的所有商品存入缓存
        for(int i = 0;i<ui->tableWidget->rowCount();++i)
        {
            SuspendedItem item;
            item.id       = ui->tableWidget->item(i,0)->text();
            item.name     = ui->tableWidget->item(i,1)->text();
            item.price    = ui->tableWidget->item(i,2)->text().toDouble();
            item.quantity = ui->tableWidget->item(i,3)->text().toInt();
            item.subtotal = ui->tableWidget->item(i,4)->text().toDouble();
            suspendedCart.append(item);
        }

        //清空界面，重置金额
        isUpdating = true;
        ui->tableWidget->setRowCount(0);
        isUpdating = false;
        updateTotalMoney();
        ui->Hangup->setText("恢复");//点击完挂起按钮变成"恢复"
        isSuspended = true;
        return;
    }

    //假如此时已经挂起过商品了，那点击就是恢复
    isUpdating = true;
    ui->tableWidget->setRowCount(0);
    isUpdating = false;

    for(const SuspendedItem &item : suspendedCart)
    {
        int row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);
        ui->tableWidget->setItem(row,0,new QTableWidgetItem(item.id));
        ui->tableWidget->setItem(row,1,new QTableWidgetItem(item.name));
        ui->tableWidget->setItem(row,2,new QTableWidgetItem(QString::number(item.price,'f',2)));
        ui->tableWidget->setItem(row,3,new QTableWidgetItem(QString::number(item.quantity)));
        ui->tableWidget->setItem(row,4,new QTableWidgetItem(QString::number(item.subtotal,'f',2)));
    }

    updateTotalMoney();
    suspendedCart.clear();
    ui->Hangup->setText("挂起");//点击完恢复按钮变成"挂起"
    isSuspended = false;
}

//删除购物车内的商品
void CashierWindow::on_deleteItem_clicked()
{
    //记录用户选中的是哪一行
    int currentRow = ui->tableWidget->currentRow();

    //异常查询，没有选中行的情况：
    if(currentRow < 0)
    {
        QMessageBox::information(this,"提示","请先点击表格选中要删除的商品！");
        return;
    }

    isUpdating = true;
    ui->tableWidget->removeRow(currentRow);
    isUpdating = false;
    updateTotalMoney();
}

void CashierWindow::onInventoryManagementTriggered()
{
    if(!curUser.isEnabled() || !curUser.isAdministrator())
    {
        QMessageBox::warning(this,"权限不足","只有管理员账号可以打开库存管理界面。");
        return;
    }

    if(!inventoryWindow.isNull())
    {
        inventoryWindow->show();
        inventoryWindow->raise();
        inventoryWindow->activateWindow();
        return;
    }

    InventoryWidget *w = new InventoryWidget(this);
    w->setWindowFlag(Qt::Window,true);
    w->setWindowModality(Qt::WindowModal);
    w->setAttribute(Qt::WA_DeleteOnClose,true);
    inventoryWindow = w;

    connect(w,&QObject::destroyed,this,[this](){
        inventoryWindow = nullptr;
        loadData();
        loadProduct();
    });

    w->show();
}

void CashierWindow::onStatisticsTriggered()
{
    if(!curUser.isEnabled())
    {
        QMessageBox::warning(this,"权限不足","请先登录系统。");
        return;
    }

    if(!statisticsDialog.isNull())
    {
        statisticsDialog->show();
        statisticsDialog->raise();
        statisticsDialog->activateWindow();
        return;
    }

    StatisticsDialog *d = new StatisticsDialog(this);
    d->setWindowFlag(Qt::Window,true);
    d->setAttribute(Qt::WA_DeleteOnClose,true);
    statisticsDialog = d;

    connect(d,&QObject::destroyed,this,[this](){
        statisticsDialog = nullptr;
    });

    d->show();
}

void CashierWindow::onLogoutTriggered()
{
    isUpdating = true;
    ui->tableWidget->setRowCount(0);
    isUpdating = false;
    ui->label_totalMoney->setText("0.00 元");
    ui->priceInput->clear();
    suspendedCart.clear();
    isSuspended = false;
    ui->Hangup->setText("挂起");

    curUser = User();
    updateUserStatus();

    LoginDialog loginDialog(this);
    if(loginDialog.exec() != QDialog::Accepted)
    {
        close();
        return;
    }

    setCurrentUser(loginDialog.authenticatedUser());
    loadData();
    loadProduct();
}

int CashierWindow::cartQuantityForProduct(const QString &productId) const
{
    int quantity = 0;
    for(int row = 0;row<ui->tableWidget->rowCount();++row)
    {
        QTableWidgetItem *idItem = ui->tableWidget->item(row,0);
        QTableWidgetItem *qtyItem = ui->tableWidget->item(row,3);
        if(idItem != nullptr && qtyItem != nullptr && idItem->text() == productId)
            quantity += qtyItem->text().toInt();
    }
    return quantity;
}

void CashierWindow::updateTotalMoney()
{
    double totalPrice = 0.0;
    for(int i = 0;i<ui->tableWidget->rowCount();i++)
    {
        QTableWidgetItem *item = ui->tableWidget->item(i,4);//第4列是"小计"，总金额等于小计之和
        if(item) totalPrice+=item->text().toDouble();
    }
    ui->label_totalMoney->setText(QString::number(totalPrice,'f',2)+" 元");
}

void CashierWindow::updateUserStatus()
{
    bool loggedIn = curUser.isEnabled() && !curUser.username().isEmpty();
    bool isAdmin = loggedIn && curUser.isAdministrator();

    ui->actionInventoryManagement->setEnabled(isAdmin);
    ui->actionStatistics->setEnabled(loggedIn);
    ui->actionLogout->setEnabled(loggedIn);

    if(!loggedIn)
    {
        userStatusLabel->setText("未登录");
        return;
    }

    userStatusLabel->setText(QString("已登录 - %1 (%2)")
                                 .arg(curUser.username())
                                 .arg(isAdmin ? "管理员" : "收银员"));
}