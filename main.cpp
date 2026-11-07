#include "gsmartpetcare.h"
#include "connection.h"
#include "service.h"

#include <QApplication>
#include <QCoreApplication>
#include <QPalette>
#include <QStyleFactory>
#include <QMessageBox>

// Force un thème clair.
static void appliquerThemeClair(QApplication &app)
{
    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette p;
    p.setColor(QPalette::Window, QColor("#FFFFFF"));
    p.setColor(QPalette::WindowText, QColor("#1F2A37"));
    p.setColor(QPalette::Base, QColor("#FFFFFF"));
    p.setColor(QPalette::AlternateBase, QColor("#F3F4F6"));
    p.setColor(QPalette::Text, QColor("#1F2A37"));
    p.setColor(QPalette::PlaceholderText, QColor("#9CA3AF"));
    p.setColor(QPalette::Button, QColor("#F3F4F6"));
    p.setColor(QPalette::ButtonText, QColor("#1F2A37"));
    p.setColor(QPalette::BrightText, QColor("#FFFFFF"));
    p.setColor(QPalette::ToolTipBase, QColor("#FFFFFF"));
    p.setColor(QPalette::ToolTipText, QColor("#1F2A37"));
    p.setColor(QPalette::Highlight, QColor("#0F766E"));
    p.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    p.setColor(QPalette::Link, QColor("#1D4ED8"));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor("#9CA3AF"));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#9CA3AF"));
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#9CA3AF"));
    app.setPalette(p);
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "windows:darkmode=0");
#endif

    QCoreApplication::setOrganizationName("PetNova");
    QCoreApplication::setApplicationName("GestionEmployes");

    QApplication a(argc, argv);
    appliquerThemeClair(a);

    Connection &c = Connection::createInstance();
    if (!c.createConnection()) {
        QMessageBox::critical(nullptr, QObject::tr("Base de données"),
                              QObject::tr("Échec de la connexion à la base de données.\n"
                                          "Vérifiez la source ODBC, l'utilisateur et le mot de passe."),
                              QMessageBox::Cancel);
        return -1;
    }

    Service::demarrerRappels(&a); // rappels automatiques par SMS (dans service.h / service.cpp)
    GSmartPetCare w;
    w.showMaximized();
    return a.exec();
}
