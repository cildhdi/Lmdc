#pragma once

#include <QtWebSockets/qwebsocket.h>
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
  }

 private:
  Ui::MainWindowClass ui;
  QWebSocket wsocket;
  QTimer timer;
  enum State { Stop, Prepare, Collect } state;

  void resetUrl() { ui.editUrl->setText("ws://127.0.0.1:6437/v6.json"); }

  void setEditsEnabled(bool enabled) {
    ui.editUrl->setEnabled(enabled);
    ui.btnReset->setEnabled(enabled);
    ui.btnConnect->setEnabled(enabled);
    ui.btnDisconnect->setEnabled(enabled);

    ui.editAction->setEnabled(enabled);
    ui.spinRepeat->setEnabled(enabled);
    ui.spinTime->setEnabled(enabled);
    ui.btnStart->setEnabled(enabled);
    ui.btnStop->setEnabled(!enabled);
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
        timerStart(5);
        break;
      case State::Collect:
        ui.textLog->append(QStringLiteral("数据收集开始....\n\n"));
        timerStart(ui.spinTime->value());
        break;
      default:
        break;
    }
    state = s;
  }

  void stop() { changeState(Stop); }

  void timerStart(int s) {
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
          case State::Collect:
            changeState(Prepare);
            break;
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
    int repeatTime = ui.spinRepeat->value();
    ui.listFilename->clear();

    QStringList filenames;
    for (int i = 0; i < repeatTime; i++)
      filenames.append("data/" + actionName + "/" + QString::number(i) +
                       ".csv");
    ui.listFilename->addItems(filenames);

    ui.listFilename->setItemSelected(ui.listFilename->item(0), true);
  }

  void lmClose() { wsocket.close(); }

  void onFrame(QString const& msg) {
    auto doc = QJsonDocument::fromJson(msg.toLatin1());
    mxt::Frame frame(doc.object());
    if (frame.hands.size() != 0) {
      ui.labelCurrentFrame->setText(
          QStringLiteral("当前帧：id: %1, timestamp:%2, handnum:%3\n\n")
              .arg(frame.id)
              .arg(frame.timestamp)
              .arg(frame.hands.size()));
    } else if (frame.id == 0) {
      ui.textLog->append(msg + "\n\n");
    }
  }
};
