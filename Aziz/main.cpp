#include "gsmartpetcare.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    GSmartPetCare w;
    w.show();
    return a.exec();
}
