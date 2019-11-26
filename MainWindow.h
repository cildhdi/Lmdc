#pragma once

#include <QtWebSockets/qwebsocket.h>
#include <qdir.h>
#include <qfile.h>
#include <qtimer.h>
#include <QMessageBox>
#include <QtWidgets/QMainWindow>
#include <functional>
#include <memory>
#include "LmType.h"
#include "ui_MainWindow.h"

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(QWidget* parent = Q_NULLPTR) : QMainWindow(parent) {
    ui.setupUi(this);
    resetUrl();
    connect(&wsocket, &QWebSocket::textMessageReceived, this,
            &MainWindow::onFrame);
    connect(ui.btnConnect, &QPushButton::clicked, this, &MainWindow::lmOpen);
    connect(ui.btnDisconnect, &QPushButton::clicked, this,
            &MainWindow::lmClose);
    connect(ui.btnReset, &QPushButton::clicked, this, &MainWindow::resetUrl);

    connect(ui.editAction, &QLineEdit::textChanged, this,
            &MainWindow::filenameChanged);
    connect(ui.spinRepeat, SIGNAL(valueChanged(int)), this,
            SLOT(filenameChanged()));

    setEditsEnabled(true);
    connect(ui.btnStart, &QPushButton::clicked, this, &MainWindow::start);
    connect(ui.btnStop, &QPushButton::clicked, this, &MainWindow::stop);

    connect(ui.btnClear, &QPushButton::clicked,
            [this]() { ui.textLog->clear(); });

    connect(&timer, &QTimer::timeout, this, &MainWindow::onTimeOut);
    ui.pgrTime->setRange(0, 100);

    csv.reserve(50000);
  }

 private:
  Ui::MainWindowClass ui;
  QWebSocket wsocket;
  QTimer timer;
  enum State { Stop, Prepare, Collect } state;
  QString csv;

  void resetUrl() { ui.editUrl->setText("ws://127.0.0.1:6437/v6.json"); }

  void setEditsEnabled(bool enabled) {
    ui.editUrl->setEnabled(enabled);
    ui.btnReset->setEnabled(enabled);
    ui.btnConnect->setEnabled(enabled);
    ui.btnDisconnect->setEnabled(enabled);

    ui.editAction->setEnabled(enabled);
    ui.spinRepeat->setEnabled(enabled);
    ui.spinPrepare->setEnabled(enabled);
    ui.spinTime->setEnabled(enabled);
    ui.btnStart->setEnabled(enabled);
    ui.btnStop->setEnabled(!enabled);

    ui.listFilename->setDisabled(!enabled);
  }

 private slots:
  void start() {
    if (ui.editAction->text().isEmpty()) {
      QMessageBox::critical(
          this, QStringLiteral("错误"),
          ui.editAction->toolTip() + QStringLiteral("不能为空"));
      return;
    }
    setEditsEnabled(false);
    changeState(State::Prepare);
  }

  void changeState(State s) {
    switch (s) {
      case State::Stop:
        ui.textLog->append(QStringLiteral("已停止....\n\n"));
        timerStop();
        setEditsEnabled(true);
        break;
      case State::Prepare:
        ui.textLog->append(QStringLiteral("请准备....\n\n"));
        timerStart(ui.spinPrepare->value());
        break;
      case State::Collect:
        ui.textLog->append(QStringLiteral("数据收集开始 ->") +
                           ui.listFilename->currentItem()->text() + "\n\n");
        csv.clear();
        timerStart(ui.spinTime->value());
        break;
      default:
        break;
    }
    state = s;
  }

  void stop() { changeState(Stop); }

  void timerStart(double s) {
    if (timer.isActive()) {
      timer.stop();
    }
    timer.start(s * 10);
    ui.pgrTime->setValue(0);
  }

  void timerStop() {
    if (timer.isActive()) {
      timer.stop();
      ui.pgrTime->setValue(0);
    }
  }

  void onTimeOut() {
    if (state != Stop) {
      int v = ui.pgrTime->value();
      if (v < 100) {
        ui.pgrTime->setValue(v + 1);
      } else {
        switch (state) {
          case State::Prepare:
            changeState(Collect);
            break;
          case State::Collect: {
            {
              auto filename = ui.listFilename->currentItem()->text();
              QDir().mkpath(QApplication::applicationDirPath() +
                            QFileInfo(filename).dir().path());
              QFile file(QApplication::applicationDirPath() + filename);
              if (file.open(QFile::WriteOnly | QIODevice::Truncate)) {
                QTextStream out(&file);
                out << csv;
                file.close();
                ui.textLog->append(QStringLiteral("已保存到 ") +
                                   file.fileName() + "\n\n");
              } else {
                ui.textLog->append(QStringLiteral("保存失败， ") +
                                   file.errorString() + "\n\n");
              }
            }

            auto index = ui.listFilename->currentRow();
            if (index < ui.listFilename->count() - 1) {
              ui.listFilename->setCurrentRow(index + 1);
              changeState(Prepare);
            } else {
              changeState(Stop);
            }

          } break;
          default:
            break;
        }
      }
    }
  }

  void lmOpen() {
    if (ui.editUrl->text().isEmpty()) {
      QMessageBox::critical(this, QStringLiteral("错误"),
                            ui.editUrl->toolTip() + QStringLiteral("不能为空"));
      return;
    }
    wsocket.close();
    wsocket.open(QUrl{ui.editUrl->text()});
  }

  void filenameChanged() {
    QString actionName = ui.editAction->text();
    if (actionName.isEmpty()) return;
    int repeatTime = ui.spinRepeat->value();
    ui.listFilename->clear();

    QStringList filenames;
    for (int i = 0; i < repeatTime; i++)
      filenames.append("/data/" + actionName + "/" + QString::number(i) +
                       ".csv");
    ui.listFilename->addItems(filenames);

    ui.listFilename->setCurrentRow(0);
  }

  void lmClose() { wsocket.close(); }

  void onFrame(QString const& msg) {
    mxt::Frame frame(msg);
    if (frame.hands.size() != 0) {
      ui.labelCurrentFrame->setText(
          QStringLiteral("当前帧：id: %1, timestamp:%2, handnum:%3")
              .arg(frame.id)
              .arg(frame.timestamp)
              .arg(frame.hands.size()));
      if (state == Collect) {
        if (csv.isEmpty()) csv.append(frame.toCsvLine(true));
        csv.append(frame.toCsvLine());
      }
    } else if (frame.id == 0) {
      ui.textLog->append(msg + "\n\n");
    }
  }
};
