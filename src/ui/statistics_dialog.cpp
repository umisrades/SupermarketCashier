#include "statistics_dialog.h"
#include "ui_statistics_dialog.h"

#include <QDate>
#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidgetItem>

StatisticsDialog::StatisticsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::StatisticsDialog)
{
    ui->setupUi(this);
    setWindowTitle("销售统计与销量排行");

    ui->dateFromEdit->setDate(QDate::currentDate());
    ui->dateToEdit->setDate(QDate::currentDate());

    ui->rankingTable->setColumnCount(6);
    ui->rankingTable->setHorizontalHeaderLabels({
        "排名", "商品名称", "条码", "单价", "销量", "销售额"});
    ui->rankingTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);
    ui->rankingTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);
    ui->rankingTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);
}

StatisticsDialog::~StatisticsDialog()
{
    delete ui;
}

void StatisticsDialog::on_todayStatsBtn_clicked()
{
    loadTodayStats();
}

void StatisticsDialog::on_monthStatsBtn_clicked()
{
    loadMonthStats();
}

void StatisticsDialog::on_customDateBtn_clicked()
{
    loadCustomDateStats();
}

void StatisticsDialog::on_rankingBtn_clicked()
{
    loadSalesRanking();
}

void StatisticsDialog::on_refreshBtn_clicked()
{
    loadTodayStats();
    loadSalesRanking();
}

void StatisticsDialog::loadTodayStats()
{
    DailySalesStats stats;
    QString errorMessage;

    if (!ProductRepository::getTodaySalesStats(
            &stats,
            &errorMessage))
    {
        QMessageBox::warning(
            this,
            "查询失败",
            errorMessage);
        return;
    }

    displayDailyStats(stats);
}

void StatisticsDialog::loadMonthStats()
{
    MonthlySalesStats stats;
    QString errorMessage;

    if (!ProductRepository::getMonthToDateSalesStats(
            &stats,
            &errorMessage))
    {
        QMessageBox::warning(
            this,
            "查询失败",
            errorMessage);
        return;
    }

    displayMonthlyStats(stats);
}

void StatisticsDialog::loadCustomDateStats()
{
    const QString dateFrom =
        ui->dateFromEdit->date().toString("yyyy-MM-dd");
    const QString dateTo =
        ui->dateToEdit->date().toString("yyyy-MM-dd");

    if (dateFrom > dateTo)
    {
        QMessageBox::warning(
            this,
            "日期错误",
            "开始日期不能晚于结束日期");
        return;
    }

    DailySalesStats stats;
    QString errorMessage;

    if (!ProductRepository::getSalesStatsForRange(
            dateFrom,
            dateTo,
            &stats,
            &errorMessage))
    {
        QMessageBox::warning(
            this,
            "查询失败",
            errorMessage);
        return;
    }

    displayDailyStats(stats);
}

void StatisticsDialog::loadSalesRanking()
{
    const QString dateFrom =
        ui->dateFromEdit->date().toString("yyyy-MM-dd");
    const QString dateTo =
        ui->dateToEdit->date().toString("yyyy-MM-dd");

    if (dateFrom > dateTo)
    {
        QMessageBox::warning(
            this,
            "日期错误",
            "开始日期不能晚于结束日期");
        return;
    }

    std::vector<SalesRankingItem> ranking;
    QString errorMessage;

    if (!ProductRepository::getSalesRanking(
            dateFrom,
            dateTo,
            &ranking,
            &errorMessage))
    {
        QMessageBox::warning(
            this,
            "查询失败",
            errorMessage);
        return;
    }

    displayRanking(ranking);
}

void StatisticsDialog::displayDailyStats(const DailySalesStats &stats)
{
    ui->dateLabel->setText("日期：" + stats.date);
    ui->orderCountLabel->setText(
        QString("订单数：%1 单").arg(stats.orderCount));
    ui->totalAmountLabel->setText(
        QString("销售额：%1 元").arg(stats.totalAmount, 0, 'f', 2));
    ui->avgOrderLabel->setText(
        QString("客单价：%1 元").arg(stats.averageOrder, 0, 'f', 2));
}

void StatisticsDialog::displayMonthlyStats(const MonthlySalesStats &stats)
{
    ui->dateLabel->setText("月份：" + stats.month);
    ui->orderCountLabel->setText(
        QString("订单数：%1 单").arg(stats.orderCount));
    ui->totalAmountLabel->setText(
        QString("销售额：%1 元").arg(stats.totalAmount, 0, 'f', 2));
    ui->avgOrderLabel->setText(
        QString("客单价：%1 元").arg(stats.averageOrder, 0, 'f', 2));
}

void StatisticsDialog::displayRanking(
    const std::vector<SalesRankingItem> &ranking)
{
    ui->rankingTable->setRowCount(0);

    for (size_t i = 0; i < ranking.size(); ++i)
    {
        const SalesRankingItem &item = ranking[i];
        const int row = ui->rankingTable->rowCount();
        ui->rankingTable->insertRow(row);

        ui->rankingTable->setItem(
            row,
            0,
            new QTableWidgetItem(QString::number(i + 1)));
        ui->rankingTable->setItem(
            row,
            1,
            new QTableWidgetItem(item.productName));
        ui->rankingTable->setItem(
            row,
            2,
            new QTableWidgetItem(item.barcode));
        ui->rankingTable->setItem(
            row,
            3,
            new QTableWidgetItem(
                QString::number(item.unitPrice, 'f', 2)));
        ui->rankingTable->setItem(
            row,
            4,
            new QTableWidgetItem(QString::number(item.totalQuantity)));
        ui->rankingTable->setItem(
            row,
            5,
            new QTableWidgetItem(
                QString::number(item.totalRevenue, 'f', 2)));

        if (i == 0)
        {
            QTableWidgetItem *rankItem = ui->rankingTable->item(row, 0);
            rankItem->setText("第1名");
        }
        else if (i == 1)
        {
            QTableWidgetItem *rankItem = ui->rankingTable->item(row, 0);
            rankItem->setText("第2名");
        }
        else if (i == 2)
        {
            QTableWidgetItem *rankItem = ui->rankingTable->item(row, 0);
            rankItem->setText("第3名");
        }
    }

    if (ranking.empty())
    {
        ui->rankingTable->insertRow(0);
        ui->rankingTable->setItem(
            0,
            0,
            new QTableWidgetItem("暂无销售数据"));
        ui->rankingTable->setSpan(0, 0, 1, 6);
    }
}