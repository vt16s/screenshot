#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QString calcMD5(const QString &fileName);
    static int compareFiles(const QString &fileName1, const QString &fileName2);

private:
    Ui::MainWindow *ui;
    QSqlDatabase db;
    QSqlQuery *query;
    QSqlTableModel *model;
    QTimer *timer;
    QPixmap pixmap;
    const QString fileFormat = ".png";
    const QString imageDir   = "./ScreenShots/";

private slots:
    void saveScreen();
    void newScreenshot();
    void on_start_stop_btn_clicked();
    void addRecordToDatabase(const QString &fileName);
    void on_delBtn_clicked();

signals:
    void fileCreated(const QString& /*fileName*/);
};
#endif // MAINWINDOW_H
