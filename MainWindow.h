#pragma once

#include <QtWebSockets/qwebsocket.h>
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
    connect(ui.btnStart, &QPushButton::clicked,
            [this]() { setEditsEnabled(false); });
    connect(ui.btnStop, &QPushButton::clicked,
            [this]() { setEditsEnabled(true); });

    connect(ui.btnClear, &QPushButton::clicked,
            [this]() { ui.textLog->clear(); });
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
    ui.btnStop->setEnabled(!enabled);
  }

 private slots:
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
      filenames.append("data/" + actionName + "/" + QString::number(i) + ".csv");
    ui.listFilename->addItems(filenames);

    ui.listFilename->setItemSelected(ui.listFilename->item(0), true);
  }

  void lmClose() { wsocket.close(); }

  void onFrame(QString const& msg) {
    auto doc = QJsonDocument::fromJson(msg.toLatin1());
    mxt::Frame frame(doc.object());
    if (frame.hands.size() != 0) {
      ui.textLog->append(QString("id: %1, timestamp:%2, handnum:%3\n\n")
                             .arg(frame.id)
                             .arg(frame.timestamp)
                             .arg(frame.hands.size()));
    } else if (frame.id == 0) {
      ui.textLog->append(msg + "\n\n");
    }
  }
};
