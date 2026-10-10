#ifndef GSMARTPETCARE_H
#define GSMARTPETCARE_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class GSmartPetCare; }
QT_END_NAMESPACE

class GestionServices;

class GSmartPetCare : public QMainWindow
{
    Q_OBJECT

public:
    explicit GSmartPetCare(QWidget *parent = nullptr);
    ~GSmartPetCare();

private slots:
    // Clic sur le bouton « Se connecter » (connexion automatique par le nom du slot)
    void on_pushButtonConnexion_clicked();

private:
    Ui::GSmartPetCare *ui;

    // Indices des pages du QStackedWidget
    enum Page { PageLogin = 0, PageAccueil = 1 };

    // Pages de la zone de contenu (QStackedWidget « stackContenu » du .ui)
    enum Contenu { ContenuAccueil = 0, ContenuServices = 1 };

    // Affiche une page du contenu et met en surbrillance le bouton du menu correspondant
    void afficherContenu(int index);

    // Logique de la page « Gestion des services » (widgets définis dans le .ui)
    GestionServices *gestionServices = nullptr;
};

#endif // GSMARTPETCARE_H
