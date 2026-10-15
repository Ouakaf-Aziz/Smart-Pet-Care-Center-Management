#ifndef GESTIONEMPLOYES_H
#define GESTIONEMPLOYES_H

#include <QObject>
#include <QVector>

#include "donneesemployes.h"

class QWidget;
namespace Ui { class GSmartPetCare; }

// Écran « Gestion des employés » : logique de la page pageEmployes de gsmartpetcare.ui
// (modifiable avec Qt Designer). Ce n'est plus une fenêtre : elle pilote les widgets de la page.
class GestionEmployes : public QObject
{
    Q_OBJECT

public:
    GestionEmployes(Ui::GSmartPetCare *ui, BaseEmployes &base, QWidget *fenetre);

    // À appeler après la connexion : applique les privilèges de l'employé connecté.
    void setUtilisateur(int employeId);

signals:
    void statistiquesDemandees(bool voirSalaires);   // la fenêtre principale affiche pageStats
    void privilegesDemandes();                       // ... ou pagePrivileges

private slots:
    void onAjouter();
    void onModifier(int employeId);
    void onSupprimer(int employeId);
    void onEnregistrer();
    void onAnnuler();
    void onSelectionChanged();
    void onExporterPdf();
    void onStatistiques();
    void onPrivileges();
    void onEnregistrerVisage();
    void onSupprimerVisage();
    void refreshTable();                // recherche + filtre + tri

private:
    struct DonneesExport {
        QStringList entetes;
        QVector<QStringList> lignes;
    };

    void configurerInterface();         // réglages que Designer ne sait pas faire (listes, colonnes, signaux)
    void appliquerDroits();
    void viderFormulaire();
    void remplirFormulaire(const Employe &e);
    void montrerChamp(QWidget *champ, bool visible);
    void majEtatVisage(int employeId);          // affiche l'état du visage de l'employé (-1 = aucun)
    QWidget *creerBoutonsActions(int employeId);
    DonneesExport donneesExport() const;
    QString htmlTableau() const;

    BaseEmployes &m_base;
    Droits m_droits;
    int m_userId = -1;
    int m_currentId = -1;               // -1 = nouvel employé
    QVector<Employe> m_vue;             // employés actuellement affichés (après filtre/tri)

    Ui::GSmartPetCare *ui;
    QWidget *m_fenetre;                 // fenêtre principale : parent des boîtes de dialogue
};

// Charge les employés de démonstration dans la base (à appeler une fois au démarrage).
void chargerEmployesDemo(BaseEmployes &base);

#endif // GESTIONEMPLOYES_H
