#ifndef GSMARTPETCARE_H
#define GSMARTPETCARE_H

#include <QMainWindow>

#include "donneesemployes.h"

QT_BEGIN_NAMESPACE
namespace Ui { class GSmartPetCare; }
QT_END_NAMESPACE

class GestionServices;
class GestionRdv;
class Connexion;
class GestionEmployes;
class StatistiquesEmployes;
class Privileges;

class GSmartPetCare : public QMainWindow
{
    Q_OBJECT

public:
    explicit GSmartPetCare(QWidget *parent = nullptr);
    ~GSmartPetCare();

private slots:
    // Connexion réussie (email + mot de passe, ou reconnaissance faciale) : ouvre l'accueil
    void onConnecte(int employeId);
    // Pages du module employés
    void afficherStatistiquesEmployes(bool voirSalaires);
    void afficherPrivileges();

private:
    Ui::GSmartPetCare *ui;

    // Indices des pages du QStackedWidget principal « stackedWidget » du .ui
    enum Page { PageLogin = 0, PageAccueil = 1, PageVisage = 2 };

    // Pages de la zone de contenu (QStackedWidget « stackContenu » du .ui)
    enum Contenu {
        ContenuAccueil = 0, ContenuServices = 1, ContenuRDV = 2,
        ContenuEmployes = 3, ContenuStatsEmployes = 4, ContenuPrivileges = 5
    };

    // Affiche une page du contenu et met en surbrillance le bouton du menu correspondant
    void afficherContenu(int index);

    // Logique de la page « Gestion des services » (widgets définis dans le .ui)
    GestionServices *gestionServices = nullptr;

    // Logique de la page « Gestion des rendez-vous » (widgets définis dans le même .ui)
    GestionRdv *gestionRdv = nullptr;

    // ----- Module employés (pages pageEmployes, pageStatsEmployes, pagePrivileges, pageVisage du même .ui) -----
    BaseEmployes m_baseEmployes;                 // employés en mémoire (+ visages enregistrés)
    Connexion *m_connexion = nullptr;            // authentification des employés
    GestionEmployes *m_employes = nullptr;       // gestion des employés (CRUD, tri, recherche, PDF)
    StatistiquesEmployes *m_statsEmployes = nullptr;
    Privileges *m_privileges = nullptr;          // rôles et privilèges (consultation)
};

#endif // GSMARTPETCARE_H
