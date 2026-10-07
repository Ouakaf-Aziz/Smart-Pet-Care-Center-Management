#ifndef GESTIONRDV_H
#define GESTIONRDV_H

#include <QMainWindow>
#include <QObject>
#include <QString>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace Ui { class GSmartPetCare; }
QT_END_NAMESPACE

// Logique de la page « Gestion des rendez-vous ».
// L'interface (tableau, formulaire, boutons...) est définie dans gsmartpetcare.ui
// (page « pageRdv ») : cette classe ne fait que la piloter, comme GestionServices.
class GestionRdv : public QObject
{
    Q_OBJECT

public:
    // ui      : interface de la fenêtre principale (contient la page des rendez-vous)
    // fenetre : fenêtre principale, utilisée comme parent des boîtes de dialogue
    GestionRdv(Ui::GSmartPetCare *ui, QMainWindow *fenetre, QObject *parent = nullptr);

private slots:
    void enregistrerRdv();
    void viderFormulaire();
    void afficherStatistiques();
    void exporterPdf();

private:
    void configurerTableau();
    void remplirExemples();
    void remplirLigne(int row, const QStringList &valeurs);
    void ajouterBoutonsAction(int row);
    bool genererPdf(const QString &chemin);

    Ui::GSmartPetCare *ui;
    QMainWindow *m_fenetre;
    int m_ligneEnModification = -1;   // -1 = nouveau rendez-vous
};

#endif // GESTIONRDV_H
