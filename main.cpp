#include "auth.h"
#include "mainwindow.h"
#include "auth.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    Auth auth;
    auth.show();
    auth.exec();
    if (auth.getUsername().size() > 3) {
        w.setUsername(auth.getUsername());
        w.show();
    } else {
        a.exit(0);
        return 0;
    }
    return a.exec();
}
