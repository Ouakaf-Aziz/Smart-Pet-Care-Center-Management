#ifndef STATISTIQUESSERVICES_H
#define STATISTIQUESSERVICES_H

#include <QDialog>
#include <QVector>

#include "donnees.h"

// Page « Statistiques » :
//  - cercle (anneau) : répartition des services par type
//  - histogramme     : nombre de services par tranche de prix
//  - barres horizontales : durée moyenne par type de service
class StatistiquesServices : public QDialog
{
    Q_OBJECT

public:
    explicit StatistiquesServices(const QVector<Service> &services, QWidget *parent = nullptr);

private:
    void construireInterface();
    void calculer();

    const QVector<Service> &m_services;
    class DonutChart *m_donut = nullptr;
    class BarChart   *m_histoPrix = nullptr;
    class BarChart   *m_barresDuree = nullptr;
};

#endif // STATISTIQUESSERVICES_H
