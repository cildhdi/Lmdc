#pragma once

#include <QtWebSockets/qwebsocket.h>
#include <QMessageBox>
#include <QtWidgets/QMainWindow>
#include <functional>
#include <memory>
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

    setEditsEnabled(true);
    connect(ui.btnStart, &QPushButton::clicked,
            [this]() { setEditsEnabled(false); });
    connect(ui.btnStop, &QPushButton::clicked,
            [this]() { setEditsEnabled(true); });
  }

 private:
  Ui::MainWindowClass ui;
  QWebSocket wsocket;

  void resetUrl() { ui.editUrl->setText("ws://127.0.0.1:6437/v6.json"); }

  void setEditsEnabled(bool enabled) {
    if (!enabled && ui.editAction->text().isEmpty()) {
      QMessageBox::critical(
          this, QStringLiteral("错误"),
          ui.editAction->toolTip() + QStringLiteral("不能为空"));
      return;
    }

    ui.editUrl->setEnabled(enabled);
    ui.btnReset->setEnabled(enabled);
    ui.btnConnect->setEnabled(enabled);
    ui.btnDisconnect->setEnabled(enabled);

    ui.editAction->setEnabled(enabled);
    ui.spinRepeat->setEnabled(enabled);
    ui.spinTime->setEnabled(enabled);
    ui.btnStart->setEnabled(enabled);
    ui.btnPause->setEnabled(!enabled);
    ui.btnStop->setEnabled(!enabled);
  }

  void lmOpen() {
    if (ui.editUrl->text().isEmpty()) {
      QMessageBox::critical(this, QStringLiteral("错误"),
                            ui.editUrl->toolTip() + QStringLiteral("不能为空"));
      return;
    }
    wsocket.open(QUrl{ui.editUrl->text()});
  }

  void lmClose() { wsocket.close(); }

  void onFrame(QString const& msg) {
    ui.listLog->addItem(msg);
    if (ui.listLog->count() > ui.spinLogCount->value()) {
      ui.listLog->removeItemWidget(ui.listLog->item(0));
    }
  }
};
