#include "gsmartpetcare.h"

#include <QApplication>
<<<<<<< Updated upstream

int main(int argc, char *argv[])
{
=======
#include <QCoreApplication>
#include <QPalette>
#include <QStyleFactory>

// Force un thème clair (fond blanc), même si Windows est en mode sombre
// (boîtes de dialogue, calendrier déroulant, etc. restent lisibles)
static void appliquerThemeClair(QApplication &app)
{
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette p;
    p.setColor(QPalette::Window,          QColor("#FFFFFF"));
    p.setColor(QPalette::WindowText,      QColor("#1F2A37"));
    p.setColor(QPalette::Base,            QColor("#FFFFFF"));
    p.setColor(QPalette::AlternateBase,   QColor("#F3F4F6"));
    p.setColor(QPalette::Text,            QColor("#1F2A37"));
    p.setColor(QPalette::PlaceholderText, QColor("#9CA3AF"));
    p.setColor(QPalette::Button,          QColor("#F3F4F6"));
    p.setColor(QPalette::ButtonText,      QColor("#1F2A37"));
    p.setColor(QPalette::BrightText,      QColor("#FFFFFF"));
    p.setColor(QPalette::ToolTipBase,     QColor("#FFFFFF"));
    p.setColor(QPalette::ToolTipText,     QColor("#1F2A37"));
    p.setColor(QPalette::Highlight,       QColor("#0F766E"));
    p.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    p.setColor(QPalette::Link,            QColor("#1D4ED8"));

    p.setColor(QPalette::Disabled, QPalette::Text,       QColor("#9CA3AF"));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#9CA3AF"));
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#9CA3AF"));

    app.setPalette(p);
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // Barre de titre claire aussi (Qt 6.5+), sans effet sur les versions plus anciennes
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "windows:darkmode=0");
#endif

    // Dossier de données de l'application (visages.json du module employés : empreintes de visage).
    // Mêmes noms que l'ancienne application « GestionEmployes » : les visages déjà enregistrés sont conservés.
    QCoreApplication::setOrganizationName("PetNova");
    QCoreApplication::setApplicationName("GestionEmployes");

>>>>>>> Stashed changes
    QApplication a(argc, argv);
    GSmartPetCare w;
    w.showMaximized();      // plein écran : la page des employés est large (tableau + fiche)
    return a.exec();
}
