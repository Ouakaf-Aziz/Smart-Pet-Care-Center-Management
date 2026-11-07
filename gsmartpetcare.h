#ifndef GSMARTPETCARE_H
#define GSMARTPETCARE_H

#include "service.h"

#include <QMainWindow>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QStringList>
#include <QVector>
#include <QWidget>

class QDialog;

QT_BEGIN_NAMESPACE
namespace Ui { class GSmartPetCare; }
QT_END_NAMESPACE

// Fenêtre principale + page de connexion (pageLogin de gsmartpetcare.ui : email + mot de passe).
// Les anciennes classes GSmartPetCare et Connexion sont fusionnées ici.
class GSmartPetCare : public QMainWindow
{
    Q_OBJECT

public:
    explicit GSmartPetCare(QWidget *parent = nullptr);
    ~GSmartPetCare();

private slots:
    void onConnecter();                 // clic sur « Connexion » (ex-Connexion::onConnecter)
    void onDeconnecter();               // clic sur « Déconnexion »

private:
    enum Page { PageLogin = 0, PageAccueil = 1, PageVisage = 2 };
    enum Contenu { ContenuAccueil = 0, ContenuServices = 1, ContenuRDV = 2,
                   ContenuEmployes = 3, ContenuStatsEmployes = 4, ContenuPrivileges = 5,
                   ContenuAnimaux = 6 };

    void afficherContenu(int index);
    void afficherErreur(const QString &message);   // ex-Connexion::afficherErreur
    void reussir(int employeId);                   // ex-Connexion::reussir

    Ui::GSmartPetCare *ui;
};


// ===========================================================================
// VUES DU MODULE SERVICES : l'interface est entièrement définie dans gsmartpetcare.ui (Qt Designer).
// Ces classes ne font que piloter les widgets (connexions, remplissage, appels au modèle).
// AUCUNE requête SQL ici : tout passe par la classe Service (modèle).
// ===========================================================================

class HistoriqueServices;
class StatistiquesServices;

// Page « Gestion des services » (pageServices).
class GestionServices : public QObject
{
    Q_OBJECT

public:
    // ui      : interface de la fenêtre principale (contient la page des services)
    // fenetre : fenêtre principale, utilisée comme parent des boîtes de message
    GestionServices(Ui::GSmartPetCare *ui, QWidget *fenetre, QObject *parent = nullptr);

private slots:
    void onEnregistrer();
    void onAnnuler();
    void onExporterPdf();
    void onHistorique();
    void onStatistiques();               // ouvre la page des statistiques
    void refreshTable();                 // recherche + filtre + tri

private:
    void afficherHistorique(int animalId = -1);   // ouvre la fenêtre Historique (sur cet animal si animalId >= 0)
    void chargerDepuisBase();            // Read : demande la liste au modèle Service
    void viderFormulaire();
    void remplirFormulaire(const Service &s);
    void modifierService(int serviceId);
    void supprimerService(int serviceId);
    void ajouterBoutonsLigne(int ligne, int serviceId);   // boutons Modifier / Supprimer de la colonne « Actions »
    const Service *trouver(int id) const;

    Ui::GSmartPetCare *ui;
    QWidget *m_fenetre;
    QVector<Service> m_services;         // copie en mémoire de la table SERVICE
    Service m_etmp;                      // objet du modèle pour appeler supprimer(int)
    int m_currentId = -1;                // -1 = mode « Ajouter » ; sinon id du service en cours de modification
    HistoriqueServices *m_historique = nullptr;
    StatistiquesServices *m_statistiques = nullptr;
};


// Page « Historique des services rendus à chaque animal » (pageHistoriqueServices).
// Les données viennent d'Oracle (ANIMAL, RENDEZ_VOUS, INCLURE, SERVICE) via la classe Service :
// recherche, filtre, tri et statistiques sont calculés en SQL.
// Le formulaire d'ajout / modification d'un service rendu est le panneau « frameSaisieRendu ».
class HistoriqueServices : public QObject
{
    Q_OBJECT

public:
    HistoriqueServices(Ui::GSmartPetCare *ui, const QVector<Service> &services,
                       QWidget *fenetre, QObject *parent = nullptr);

    // Affiche la page. animalIdInitial : animal à afficher d'emblée (-1 = le premier de la liste)
    void ouvrir(int animalIdInitial = -1);

private slots:
    void refreshAnimaux();
    void refreshHistorique();

    void onAjouterRendu();
    void onModifierRendu();
    void onSupprimerRendu();
    void onEnregistrerRendu();           // bouton « Enregistrer » du panneau de saisie
    void onServiceRenduChange();         // le prix proposé suit le service choisi
    void onExporterPdf();

private:
    void configurerColonnes();           // modes de redimensionnement des colonnes
    void remplirFiltreType();
    void majBoutons();
    void ouvrirSaisie(const Service &r, const QVector<Service> &rdvs);
    void fermerSaisie();
    void basculerSaisie(bool ouverte);

    const Service *trouverAnimal(int id) const;
    const Service *trouverRendu(int idRdv, int idService) const;
    const Service *trouverService(int id) const;
    int animalCourantId() const;
    bool renduCourant(int &idRdv, int &idService) const;   // false si aucune ligne sélectionnée

    Ui::GSmartPetCare *ui;
    const QVector<Service> &m_services;
    QWidget *m_fenetre;
    QVector<Service> m_animaux;                 // dernier résultat de la recherche (champs animal)
    QVector<Service> m_rendus;                  // dernier historique affiché (services rendus)

    QDialog *m_dialogue = nullptr;              // fenêtre centrée (fermée par la croix)

    // état du panneau de saisie
    bool m_edition = false;
    Service m_renduEnCours;
    int m_ancienIdService = 0;
};


// Page « Statistiques » (pageStatistiquesServices) :
//  - anneau : répartition des services par type
//  - histogramme : nombre de services par tranche de prix
//  - barres horizontales : durée moyenne par type de service
// Les données sont lues par Service::statistiques() (SQL) ; cette classe ne fait que les afficher.
class StatistiquesServices : public QObject
{
    Q_OBJECT

public:
    StatistiquesServices(Ui::GSmartPetCare *ui, QWidget *fenetre, QObject *parent = nullptr);
    void ouvrir();

private:
    void calculer();

    Ui::GSmartPetCare *ui;
    QWidget *m_fenetre;
    QDialog *m_dialogue = nullptr;              // fenêtre centrée (fermée par la croix)
};


// Widget de dessin des graphiques. Il est placé dans gsmartpetcare.ui comme « widget promu » :
// position, taille et cadre sont définis dans Qt Designer ; le code ne fait que dessiner.
class GraphiqueService : public QWidget
{
    Q_OBJECT

public:
    explicit GraphiqueService(QWidget *parent = nullptr);

    void setAnneau(const QMap<QString, int> &data);
    void setBarresVerticales(const QVector<QPair<QString, double>> &data);
    void setBarresHorizontales(const QVector<QPair<QString, double>> &data);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    enum Mode { Anneau, BarresVerticales, BarresHorizontales };
    void setBarres(Mode mode, const QVector<QPair<QString, double>> &data);

    Mode m_mode = Anneau;
    QStringList m_labels;
    QVector<double> m_values;
};

#endif // GSMARTPETCARE_H
