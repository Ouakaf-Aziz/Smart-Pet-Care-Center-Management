#ifndef GSMARTPETCARE_H
#define GSMARTPETCARE_H

#include <QMainWindow>
#include <QStringList>
#include <QVariant>

#include "RendezVous.h"

QT_BEGIN_NAMESPACE
namespace Ui { class GSmartPetCare; }
class QComboBox;
class QLineEdit;
class QCheckBox;
QT_END_NAMESPACE

// Fenêtre principale : navigation entre les pages + page « Gestion des rendez-vous ».
// Les autres pages (services, employés, animaux) sont seulement accessibles (interface du .ui),
// leur fonctionnement a été retiré. Toutes les requêtes SQL des rendez-vous sont dans la classe RendezVous.
class GSmartPetCare : public QMainWindow
{
    Q_OBJECT

public:
    explicit GSmartPetCare(QWidget *parent = nullptr);
    ~GSmartPetCare();

private slots:
    // ----- Page « Gestion des rendez-vous » -----
    void enregistrerRdv();        // bouton « Enregistrer » / « Mettre à jour »
    void annulerRdv();            // bouton « Annuler » : abandonne la saisie ou la modification
    void afficherStatistiques();  // bouton « Statistiques »
    void exporterPdf();           // bouton « Exporter PDF »

private:
    Ui::GSmartPetCare *ui;

    // Indices des pages du QStackedWidget principal « stackedWidget » du .ui
    enum Page { PageLogin = 0, PageAccueil = 1, PageVisage = 2 };

    // Pages de la zone de contenu (QStackedWidget « stackContenu » du .ui)
    enum Contenu {
        ContenuAccueil = 0, ContenuServices = 1, ContenuRDV = 2,
        ContenuEmployes = 3, ContenuStatsEmployes = 4, ContenuPrivileges = 5,
        ContenuAnimaux = 6
    };

    // Affiche une page du contenu et met en surbrillance le bouton du menu correspondant
    void afficherContenu(int index);

    // ----- Page « Gestion des rendez-vous » -----
    void initialiserRdv();
    void configurerTableau();
    void ajouterChampsManquants(); // crée « Animal » et « ID employé » dans le formulaire s'ils n'existent pas
    void chargerDepuisBase();      // remplit le tableau avec la table RENDEZ_VOUS
    void remplirComboAnimaux();    // liste déroulante « Animal » depuis la table ANIMAL
    void rafraichirRdv();          // relit les animaux et les rendez-vous depuis la base
    QVariant idAnimalSaisi() const;
    QVariant idEmployeSaisi() const;
    static int idNumerique(const QString &texte);   // « RDV008 » -> 8
    void remplirLigne(int row, const QStringList &valeurs);
    void ajouterBoutonsAction(int row);
    void modifierLigne(int row);   // charge la ligne dans le formulaire
    void supprimerLigne(int row);
    void nouveauFormulaire();      // formulaire vide, mode « Ajouter »
    bool genererPdf(const QString &chemin);

    RendezVous m_rdv;                 // accès à la base de données (toutes les requêtes SQL)
    int m_ligneEnModification = -1;   // -1 = mode « Ajouter »
    int m_colIdEmploye = -1;          // colonne « id employe »
    int m_colIdAnimal  = -1;          // colonne « Id animal »
    int m_colAnimal = -1;             // colonne « Animal » du tableau (-1 si absente)
    int m_colAction = 7;              // colonne « Actions » (toujours la dernière)
    QComboBox *m_comboAnimal = nullptr;
    QLineEdit *m_editIdAnimal = nullptr;
    QLineEdit *m_editIdEmploye = nullptr;
    QCheckBox *m_checkEmail  = nullptr;
};

#endif // GSMARTPETCARE_H