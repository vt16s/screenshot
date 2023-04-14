#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWindow>
#include <QScreen>
#include <QMessageBox>
#include <QDir>
#include <QDateTime>
#include <QCryptographicHash>
#include <QDebug>
#include <QtConcurrentRun>
#include <QFutureWatcher>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // Database creating
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("./screenshots.db");
    if(!db.open())
        QMessageBox::warning(this, "Error", "Could not open database");
    else
    {
        query = new QSqlQuery(db);
        model = new QSqlTableModel(this, db);
        if(!query->exec("CREATE TABLE IF NOT EXISTS Screenshots(Filename TEXT, `Hash (MD5)` TEXT, `Similarity \nwith previous, %` INT);"))
            QMessageBox::warning(this, "Error", "Could not create table");
        else
        {
            model->setTable("Screenshots");
            model->setSort(0, Qt::DescendingOrder);
            model->select();
            ui->DBView->setModel(model);
            ui->DBView->setColumnWidth(0, 100);
            ui->DBView->setColumnWidth(1, 220);
        }
    }

    if(model->rowCount() <= 1)
        ui->delBtn->setEnabled(false);

    timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, &MainWindow::newScreenshot);
    connect(this, &MainWindow::fileCreated, &MainWindow::addRecordToDatabase);

    qDebug() << "Thread MAIN_APP is " << QThread::currentThreadId();
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::calcMD5(const QString &fileName)
{
    QString result = "no data";
    QByteArray data;
    QCryptographicHash cryp(QCryptographicHash::Md5);
    QFile file(fileName);
    if(file.open(QIODevice::ReadOnly))
    {
        data = file.readAll();
        cryp.addData(data);
        result = cryp.result().toHex().data();
        file.close();
    }
    return result;
}

int MainWindow::compareFiles(const QString &fileName1, const QString &fileName2)
{
    QImage img1, img2;
    int globalCount = 0, ecvivaletCount = 0;

    img1.load(fileName1);
    img2.load(fileName2);

    if((img1.width() != img2.width()) || (img1.height() != img2.height()))
        return 0;
    globalCount = img1.width() * img1.height();
    for(int i = 0; i < img1.width(); i++)
        for(int j = 0; j < img1.height(); j++)
        {
            if(img1.pixel(i, j) == img2.pixel(i, j))
                ecvivaletCount++;
        }
    int percent = 100 * ecvivaletCount / globalCount;
    qDebug() << "Thread COMPARE is " << QThread::currentThreadId();
    return percent;
}

void MainWindow::newScreenshot()
{
    if(ui->hideChBox->isChecked())
        hide();
    QTimer::singleShot(1000, this, &MainWindow::saveScreen);
}

void MainWindow::saveScreen()
{
/*************** Creating screenshot ***************/
    QScreen *screen = QGuiApplication::primaryScreen();
    if(const QWindow *window = windowHandle())
        screen = window->screen();
    if(!screen)
    {
        QMessageBox::warning(this, "Error", "Could not create screenshot");
        return;
    }
    pixmap = screen->grabWindow(0);
    if(ui->hideChBox->isChecked())
        show();

/*************** Saving screenshot ***************/
  //  const QString format = ".png";
    // Generation a unique filename
    const QString fileName = QString::number(QDateTime::currentDateTime().toTime_t());
    QDir dirPath("./");
    if(!dirPath.exists(imageDir))
        dirPath.mkdir(imageDir);
    const QString totalFileName = imageDir + fileName + fileFormat;
    if(!pixmap.save(totalFileName))
        QMessageBox::warning(this, "Save Error", "The image could not be saved");
    else
        emit fileCreated(fileName + fileFormat);
}

void MainWindow::on_start_stop_btn_clicked()
{
    if(timer->isActive())
    {
        timer->stop();
        ui->start_stop_btn->setText("Start");
    }
    else
    {
        timer->start(60000);  // timer period
        ui->start_stop_btn->setText("Stop");
    }
}

void MainWindow::addRecordToDatabase(const QString &fileName)
{
    if(db.isOpen() && query->isActive())
    {
        model->insertRows(0, 1);
        model->setData(model->index(0, 0), fileName);
        QString hashSum = calcMD5(imageDir + fileName);
        model->setData(model->index(0, 1), hashSum);
        if(model->rowCount() > 1)
        {
            QString fileName2 = model->data(model->index(1, 0)).toString();
            // Running the file comparison function in a separate thread
            QEventLoop loop;
            QFutureWatcher<int> watcher;
            watcher.setFuture(QtConcurrent::run(compareFiles, QString(imageDir + fileName), QString(imageDir + fileName2)));
            connect(&watcher, &QFutureWatcher<int>::finished,  &loop, &QEventLoop::quit);
            loop.exec();
            model->setData(model->index(0, 2), watcher.result());
        }
        model->submitAll();
        if(model->rowCount() > 1 && !ui->delBtn->isEnabled())
            ui->delBtn->setEnabled(true);
    }
}

void MainWindow::on_delBtn_clicked()
{
    if(db.isOpen() && query->isActive())
    {
        if(model->rowCount() > 1)
        {
            QString fileName = model->data(model->index(model->rowCount() - 1, 0)).toString();
            QFile file(imageDir + fileName);
            if(file.exists())
            {
                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(this, this->windowTitle(), "Remove file " + fileName + " from hard disk?", QMessageBox::Yes|QMessageBox::No);
                if(reply == QMessageBox::Yes)
                {
                    file.remove();
                }
            }
            model->removeRows(model->rowCount() - 1, 1);
            model->select();
            if(model->rowCount() <= 1)
                ui->delBtn->setEnabled(false);
        }
    }
}
