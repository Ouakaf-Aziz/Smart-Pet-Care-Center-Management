#ifndef STATISTIQUESEMPLOYES_H
#define STATISTIQUESEMPLOYES_H

#include <QDialog>
#include <QVector>

#include "donnees.h"

// Fenêtre « Statistiques des employés » : s'ouvre au centre de l'application,
// comme les statistiques des services et des rendez-vous (croix de fermeture
// de la barre de titre).
class StatistiquesEmployes : public QDialog
{
    Q_OBJECT

public:
    explicit StatistiquesEmployes(const QVector<Employe> &employes, QWidget *parent = nullptr);

private:
    void construireInterface();
    void calculer();

    QVector<Employe> m_employes;
    class DonutChart *m_donutStatut = nullptr;
    class BarChart   *m_barresPoste = nullptr;
    class BarChart   *m_barresSalaire = nullptr;
    class LineChart  *m_courbeRecrutement = nullptr;
};

#endif // STATISTIQUESEMPLOYES_H
