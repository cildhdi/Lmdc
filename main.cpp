#include <QtWidgets/QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  QApplication::setStyle("Fusion");
  MainWindow w;
  w.show();
  return a.exec();
}
