#ifndef CASHIERWINDOW_H
#define CASHIERWINDOW_H

#include <QHash>
#include <QMainWindow>
#include <QPointer>
#include <QString>
#include <QVector>

#include "../models/user.h"

class InventoryWidget;
class QLabel;
class QTableWidgetItem;
class StatisticsDialog;

QT_BEGIN_NAMESPACE
namespace Ui {
class CashierWindow;
}
QT_END_NAMESPACE

class CashierWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit CashierWindow(QWidget *parent = nullptr);
    ~CashierWindow() override;

    void createData();
    void loadData();
    void saveData();
    void setCurrentUser(const User &user);
    const User &currentUser() const;

private slots:
    void on_SearchandAdd_clicked();
    void on_priceInput_returnPressed();
    void on_payButton_clicked();
    void on_Hangup_clicked();
    void on_deleteItem_clicked();
    void onInventoryManagementTriggered();
    void onLogoutTriggered();
    void onStatisticsTriggered();

private:

    //商品临时存储（运行时放到内存）的结构体
    struct CashierProduct{
        QString id;
        QString barcode;
        QString imagePath;
        QString name;
        double price;
        int quantity;
    };

    void loadProduct();
    int cartQuantityForProduct(const QString &productId) const;
    void updateTotalMoney();
    void updateUserStatus();

    Ui::CashierWindow *ui;

    QHash<QString,CashierProduct> productHash;//商品数据缓存（按条形码索引）

    //挂起时用来临时存储数据
    struct SuspendedItem {
        QString id, name;
        double price, subtotal;
        int quantity;
    };
    QVector<SuspendedItem> suspendedCart;
    bool isSuspended = false;//布尔值存储具体是否挂起

    QPointer<InventoryWidget> inventoryWindow;
    QPointer<StatisticsDialog> statisticsDialog;
    User curUser;
    QLabel *userStatusLabel;
    bool isUpdating = false;
};

#endif // CASHIERWINDOW_H
