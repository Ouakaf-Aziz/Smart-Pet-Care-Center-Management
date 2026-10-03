#ifndef GESTIONSERVICES_H
#define GESTIONSERVICES_H

#include <QObject>
#include <QString>
#include <QVector>

#include "donnees.h"

QT_BEGIN_NAMESPACE
namespace Ui { class GSmartPetCare; }
QT_END_NAMESPACE

// Logique de la page « Gestion des services ».
// L'interface (tableau, formulaire, boutons...) est définie dans gsmartpetcare.ui
// (page « pageServices ») : cette classe ne fait que la piloter.
class GestionServices : public QObject
{
    Q_OBJECT

public:
    // ui      : interface de la fenêtre principale (contient la page des services)
    // fenetre : fenêtre principale, utilisée comme parent des boîtes de dialogue
    GestionServices(Ui::GSmartPetCare *ui, QWidget *fenetre, QObject *parent = nullptr);

    // ----- Points d'entrée pour le MODULE ANIMAL -----
    // Donne (ou met à jour) la liste des animaux. À appeler au démarrage et à chaque changement.
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
    void onSelectionChanged();
    void refreshTable();                 // recherche + filtre + tri

private:
    void chargerDonneesExemple();        // données de démonstration (en mémoire)
    void chargerAnimauxExemple();        // TEMPORAIRE : à remplacer par le module Animal
    void viderFormulaire();
    void remplirFormulaire(const Service &s);
    QWidget *creerBoutonsActions(int serviceId);
    Service *trouver(int id);

    Ui::GSmartPetCare *ui;
    QWidget *m_fenetre;
    QVector<Service> m_services;
    DonneesHistorique m_histo;           // animaux + services rendus
    int m_currentId = -1;                // -1 = nouveau service
    int m_prochainIdService = 1;         // prochain id à attribuer à un service
};

#endif // GESTIONSERVICES_H
