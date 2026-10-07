#ifndef GESTIONEMPLOYES_H
#define GESTIONEMPLOYES_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include "donnees.h"

QT_BEGIN_NAMESPACE
namespace Ui { class GSmartPetCare; }
QT_END_NAMESPACE

class QMainWindow;
class QWidget;

// Logique de la page « Gestion des employés ».
// L'interface (tableau, formulaire, boutons...) est définie dans gsmartpetcare.ui
// (page « pageEmployes ») : cette classe ne fait que la piloter, comme GestionServices.
class GestionEmployes : public QObject
{
    Q_OBJECT

public:
    GestionEmployes(Ui::GSmartPetCare *ui, BaseEmployes &base, QMainWindow *fenetre,
                    QObject *parent = nullptr);

    void setUtilisateur(int employeId);
    const QVector<Employe> &employes() const { return m_base.employes; }

signals:
    void statistiquesDemandees();

private slots:
    void onAjouter();
    void onModifier(int employeId);
    void onSupprimer(int employeId);
    void onEnregistrer();
    void onAnnuler();
    void onSelectionChanged();
    void onExporterPdf();
    void onStatistiques();
    void refreshTable();

private:
    struct DonneesExport {
        QStringList entetes;
        QVector<QStringList> lignes;
    };

    void configurerInterface();
    void viderFormulaire();
    void remplirFormulaire(const Employe &e);
    QWidget *creerBoutonsActions(int employeId);
    DonneesExport donneesExport() const;
    QString htmlTableau() const;

    BaseEmployes &m_base;
    int m_userId = -1;
    int m_currentId = -1;
    QVector<Employe> m_vue;

    Ui::GSmartPetCare *ui;
    QMainWindow *m_fenetre;
};

#endif // GESTIONEMPLOYES_H
