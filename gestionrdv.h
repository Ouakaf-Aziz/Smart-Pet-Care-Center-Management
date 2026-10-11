#ifndef GESTIONRDV_H
#define GESTIONRDV_H

#include <QObject>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace Ui { class GSmartPetCare; }
class QComboBox;
QT_END_NAMESPACE

// Logique de la page « Gestion des rendez-vous ».
// L'interface (tableau, formulaire, boutons...) est définie dans gsmartpetcare.ui
// (page « pageRDV ») : cette classe ne fait que la piloter, comme GestionServices.
class GestionRdv : public QObject
{
    Q_OBJECT

public:
    // ui      : interface de la fenêtre principale (contient la page des rendez-vous)
    // fenetre : fenêtre principale, utilisée comme parent des boîtes de dialogue
    GestionRdv(Ui::GSmartPetCare *ui, QWidget *fenetre, QObject *parent = nullptr);

private slots:
    void enregistrerRdv();        // bouton « Enregistrer » / « Mettre à jour »
    void annuler();               // bouton « Annuler » : abandonne la saisie ou la modification
    void afficherStatistiques();  // bouton « Statistiques »
    void exporterPdf();           // bouton « Exporter PDF »

private:
    void configurerTableau();
    void remplirExemples();       // données de démonstration (en mémoire)
    void remplirLigne(int row, const QStringList &valeurs);
    void ajouterBoutonsAction(int row);
    void modifierLigne(int row);  // charge la ligne dans le formulaire
    void supprimerLigne(int row);
    void nouveauFormulaire();     // formulaire vide, mode « Ajouter »
    bool genererPdf(const QString &chemin);

    Ui::GSmartPetCare *ui;
    QWidget *m_fenetre;
    int m_ligneEnModification = -1;   // -1 = mode « Ajouter »
};

#endif // GESTIONRDV_H
