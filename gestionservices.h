#ifndef GESTIONSERVICES_H
#define GESTIONSERVICES_H

#include <QObject>
#include <QString>
#include <QVector>

#include "donnees.h"
#include "service.h"

QT_BEGIN_NAMESPACE
namespace Ui { class GSmartPetCare; }
QT_END_NAMESPACE

// VUE de la page « Gestion des services ».
// L'interface (tableau, formulaire, boutons...) est définie dans gsmartpetcare.ui
// (page « pageServices ») : cette classe ne fait que la piloter.
// AUCUNE requête SQL ici : tout passe par la classe Service (modèle).
class GestionServices : public QObject
{
    Q_OBJECT

public:
    // ui      : interface de la fenêtre principale (contient la page des services)
    // fenetre : fenêtre principale, utilisée comme parent des boîtes de dialogue
    GestionServices(Ui::GSmartPetCare *ui, QWidget *fenetre, QObject *parent = nullptr);

    // ----- Points d'entrée utilisés par le MODULE ANIMAL (GestionAnimaux) -----
    // Donne (ou met à jour) la liste des animaux. Appelé au démarrage et à chaque changement.
    void setAnimaux(const QVector<Animal> &animaux);
    // À appeler quand un animal est supprimé : efface aussi son historique de services.
    void animalSupprime(int animalId);
    // Ouvre l'historique (sur cet animal si animalId >= 0).
    void afficherHistorique(int animalId = -1);

private slots:
    void onModifier(int serviceId);      // bouton « Modifier » de la colonne Actions
    void onSupprimer(int serviceId);     // bouton « Supprimer » de la colonne Actions
    void onEnregistrer();
    void onAnnuler();
    void onExporterPdf();
    void onHistorique();
    void onStatistiques();               // ouvre la page des statistiques
    void refreshTable();                 // recherche + filtre + tri

private:
    void chargerHistoriqueExemple();     // historique de démonstration (en mémoire)
    void chargerDepuisBase();            // Read : demande la liste au modèle Service
    void viderFormulaire();
    void remplirFormulaire(const Service &s);
    QWidget *creerBoutonsActions(int serviceId);
    const Service *trouver(int id) const;

    Ui::GSmartPetCare *ui;
    QWidget *m_fenetre;
    QVector<Service> m_services;         // copie en mémoire de la table SERVICE
    Service m_etmp;                      // objet du modèle pour appeler supprimer(int)
    DonneesHistorique m_histo;           // animaux + services rendus
    int m_currentId = -1;                // -1 = mode « Ajouter » ; sinon id du service en cours de modification
};

#endif // GESTIONSERVICES_H
