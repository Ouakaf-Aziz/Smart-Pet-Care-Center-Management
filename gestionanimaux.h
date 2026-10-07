#ifndef GESTIONANIMAUX_H
#define GESTIONANIMAUX_H

#include <QMainWindow>
#include <QObject>
#include <QString>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace Ui { class GSmartPetCare; }
QT_END_NAMESPACE

// Logique de la page « Gestion des animaux ».
// L'interface (tableau, formulaire, boutons...) est définie dans gsmartpetcare.ui
// (page « pageAnimaux ») : cette classe ne fait que la piloter, comme GestionServices
// et GestionRdv.
class GestionAnimaux : public QObject
{
    Q_OBJECT

public:
    // ui      : interface de la fenêtre principale (contient la page des animaux)
    // fenetre : fenêtre principale, utilisée comme parent des boîtes de dialogue
    GestionAnimaux(Ui::GSmartPetCare *ui, QMainWindow *fenetre, QObject *parent = nullptr);

    // Nom de l'employé connecté (affiché dans l'assistant de diagnostic).
    void setUtilisateur(const QString &nomComplet);

private slots:
    void enregistrerAnimal();
    void annulerFormulaire();
    void choisirPhoto();
    void appliquerFiltres();
    void trierAnimaux(int index);
    void exporterPdf();
    void afficherStatistiques();
    void ouvrirChatbotDiagnostic();
    void afficherRappelsSante();

private:
    void configurerTableau();
    void remplirExemples();
    void remplirLigne(int row, const QStringList &valeurs,
                      const QString &photo = QString(), const QString &email = QString());
    void ajouterBoutonsAction(int row);
    void chargerDansFormulaire(int row);
    void supprimerLigne(int row);
    void viderFormulaire();
    void definirModeFormulaire(bool enModification);
    bool validerFormulaire(QString *messageErreur) const;
    QString prochainId() const;
    int trouverLigne(const QString &id) const;
    QString contexteAnimalSelectionne() const;
    bool genererPdf(const QString &chemin);
    QString analyserSymptomes(const QString &question, const QString &espece,
                              const QString &nomAnimal) const;

    Ui::GSmartPetCare *ui;
    QMainWindow *m_fenetre;
    bool m_enModification = false;    // false = formulaire en mode « ajout »
    QString m_utilisateur;            // employé connecté
};

#endif // GESTIONANIMAUX_H
