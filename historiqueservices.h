#ifndef HISTORIQUESERVICES_H
#define HISTORIQUESERVICES_H

#include <QDialog>
#include <QVector>

#include "donnees.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

// Fenêtre « Historique des services rendus à chaque animal ».
// Les données appartiennent à GestionServices ; cette fenêtre les modifie directement.
class HistoriqueServices : public QDialog
{
    Q_OBJECT

public:
    // animalIdInitial : animal à afficher d'emblée (-1 = le premier de la liste)
    HistoriqueServices(const QVector<Service> &services,
                       DonneesHistorique &donnees,
                       int animalIdInitial = -1,
                       QWidget *parent = nullptr);

private slots:
    void refreshAnimaux();
    void refreshHistorique();

    void onAjouterRendu();
    void onModifierRendu();
    void onSupprimerRendu();

    void onExporterPdf();

private:
    void construireInterface();
    void remplirFiltreType();
    void majBoutons();

    Animal       *trouverAnimal(int id);
    ServiceRendu *trouverRendu(int id);
    const Service *trouverService(int id) const;
    int animalCourantId() const;
    int renduCourantId() const;

    bool saisirRendu(ServiceRendu &r, const QString &titre);

    const QVector<Service> &m_services;
    DonneesHistorique      &m_d;

    QLineEdit    *m_rechercheAnimal = nullptr;
    QTableWidget *m_tableAnimaux = nullptr;
    QLabel       *m_labelFiche = nullptr;
    QLabel       *m_labelNbServices = nullptr;
    QLabel       *m_labelTotal = nullptr;
    QLabel       *m_labelDernier = nullptr;
    QComboBox    *m_filtreType = nullptr;
    QTableWidget *m_tableHistorique = nullptr;
    QPushButton  *m_btnAjouterRendu = nullptr;
    QPushButton  *m_btnModifierRendu = nullptr;
    QPushButton  *m_btnSupprimerRendu = nullptr;
    QPushButton  *m_btnPdf = nullptr;
};

#endif // HISTORIQUESERVICES_H
