#ifndef GESTIONANIMAUX_H
#define GESTIONANIMAUX_H

#include <QDate>
#include <QObject>
#include <QString>
#include <QVector>

#include "donnees.h"

QT_BEGIN_NAMESPACE
namespace Ui { class GSmartPetCare; }
QT_END_NAMESPACE

// Fiche complète d'un animal (le module Services n'en voit qu'une version réduite : struct Animal).
struct FicheAnimal
{
    int     id = 0;
    QString nom;
    QString espece;
    QString sexe;
    QDate   naissance;
    QString photo;               // chemin de l'image (facultatif)
    QString proprietaire;
    QString telephone;
    QString email;
    QString suiviSante;          // « À jour » ou texte du suivi à faire
};

// Logique de la page « Gestion des animaux ».
// L'interface (tableau, formulaire, boutons...) est définie dans gsmartpetcare.ui
// (page « pageAnimaux ») : cette classe ne fait que la piloter, comme GestionServices et GestionRdv.
class GestionAnimaux : public QObject
{
    Q_OBJECT

public:
    // ui      : interface de la fenêtre principale (contient la page des animaux)
    // fenetre : fenêtre principale, utilisée comme parent des boîtes de dialogue
    GestionAnimaux(Ui::GSmartPetCare *ui, QWidget *fenetre, QObject *parent = nullptr);

    // Liste réduite fournie au module Services (historique des services rendus).
    QVector<Animal> animaux() const;
    int nombreAnimaux() const { return m_animaux.size(); }

    // Nom de l'employé connecté (utilisé par l'assistant de diagnostic).
    void setUtilisateur(const QString &nomComplet) { m_utilisateur = nomComplet; }

signals:
    // La liste des animaux a changé (ajout, modification, suppression).
    void animauxChanges();
    // Un animal a été supprimé : le module Services efface son historique.
    void animalSupprime(int animalId);
    // Demande d'ouverture de l'historique des services (animalId = -1 : aucun animal précis).
    void historiqueDemande(int animalId);

private slots:
    void onAjouter();
    void onModifier();
    void onSupprimer();
    void onEnregistrer();
    void onAnnuler();
    void onChoisirPhoto();
    void onSelectionChangee();
    void refreshTable();                 // recherche + filtre espèce + tri
    void onExporterPdf();
    void onStatistiques();
    void onChatbot();
    void onRappelsSante();
    void onHistorique();

private:
    void chargerExemples();              // données de démonstration (en mémoire)
    void nouveauFormulaire();            // formulaire vide, mode « Ajouter »
    void remplirFormulaire(const FicheAnimal &a);
    void definirMode(bool modification);
    bool valider(QString *erreur) const;
    FicheAnimal lireFormulaire() const;
    FicheAnimal *trouver(int id);
    int idSelectionne() const;           // -1 si aucune ligne sélectionnée
    int prochainId() const;
    QVector<FicheAnimal> vueCourante() const;   // animaux affichés (recherche + filtre + tri)
    static QString formaterId(int id);   // 3 -> « A003 »
    bool genererPdf(const QString &chemin, const QVector<FicheAnimal> &lignes) const;

    Ui::GSmartPetCare *ui;
    QWidget *m_fenetre;
    QVector<FicheAnimal> m_animaux;
    int m_idEnModification = -1;         // -1 = mode « Ajouter »
    int m_prochainId = 1;
    QString m_utilisateur;
};

#endif // GESTIONANIMAUX_H
