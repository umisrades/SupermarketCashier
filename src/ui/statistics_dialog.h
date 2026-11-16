#ifndef STATISTICS_DIALOG_H
#define STATISTICS_DIALOG_H

#include <QDialog>

#include "../data/product_repository.h"

namespace Ui {
class StatisticsDialog;
}

class StatisticsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StatisticsDialog(QWidget *parent = nullptr);
    ~StatisticsDialog() override;

private slots:
    void on_todayStatsBtn_clicked();
    void on_monthStatsBtn_clicked();
    void on_customDateBtn_clicked();
    void on_rankingBtn_clicked();
    void on_refreshBtn_clicked();

private:
    void loadTodayStats();
    void loadMonthStats();
    void loadCustomDateStats();
    void loadSalesRanking();
    void displayDailyStats(const DailySalesStats &stats);
    void displayMonthlyStats(const MonthlySalesStats &stats);
    void displayRanking(
        const std::vector<SalesRankingItem> &ranking);

    Ui::StatisticsDialog *ui;
};

#endif // STATISTICS_DIALOG_H