#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Embed GameCanvas into the canvasContainer
    gameCanvas = new GameCanvas(this);
    QVBoxLayout *layout = new QVBoxLayout(ui->canvasContainer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(gameCanvas);

    connect(gameCanvas, &GameCanvas::statsUpdated, this, &MainWindow::onStatsUpdated);
    connect(ui->btnLevel1, &QPushButton::clicked, this, &MainWindow::on_btnLevel1_clicked);
    connect(ui->btnLevel2, &QPushButton::clicked, this, &MainWindow::on_btnLevel2_clicked);
    connect(ui->btnLevel3, &QPushButton::clicked, this, &MainWindow::on_btnLevel3_clicked);
    connect(ui->btnReset, &QPushButton::clicked, this, &MainWindow::on_btnReset_clicked);
    connect(ui->btnCelebration, &QPushButton::clicked, this, &MainWindow::on_btnCelebration_clicked);

    // Give focus to game canvas immediately for keyboard controls
    gameCanvas->setFocus();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnLevel1_clicked()
{
    gameCanvas->setTrackLevel(TrackLevel::DONUT_PLAINS);
    gameCanvas->setFocus();
}

void MainWindow::on_btnLevel2_clicked()
{
    gameCanvas->setTrackLevel(TrackLevel::CHOCO_VALLEY);
    gameCanvas->setFocus();
}

void MainWindow::on_btnLevel3_clicked()
{
    gameCanvas->setTrackLevel(TrackLevel::BOWSER_CASTLE);
    gameCanvas->setFocus();
}

void MainWindow::on_btnReset_clicked()
{
    gameCanvas->resetKart();
    gameCanvas->setFocus();
}

void MainWindow::on_btnCelebration_clicked()
{
    gameCanvas->triggerCelebration("★ FINISH! ★", "COURSE CLEAR!");
    gameCanvas->setFocus();
}

void MainWindow::onStatsUpdated(double speed, int lap, double x, double z, double angle, QString surface)
{
    ui->lblSpeed->setText(QString("Speed: %1 km/h").arg(QString::number(speed, 'f', 1)));
    ui->lblLap->setText(QString("Lap: %1 / 3").arg(lap));
    ui->lblPos->setText(QString("Pos: (%1, %2)").arg(static_cast<int>(x)).arg(static_cast<int>(z)));
    ui->lblHeading->setText(QString("Heading: %1°").arg(static_cast<int>(angle)));
    ui->lblSurface->setText(QString("Surface: %1").arg(surface));
}
