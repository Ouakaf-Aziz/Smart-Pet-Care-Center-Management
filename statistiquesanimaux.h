#ifndef STATISTIQUESANIMAUX_H
#define STATISTIQUESANIMAUX_H

#include <QDialog>
#include <QVector>

#include "gestionanimaux.h"

// Page « Statistiques » des animaux :
//  - cercle : répartition par espèce et par sexe
//  - histogramme : animaux par tranche d'âge
//  - courbe : naissances par mois
class StatistiquesAnimaux : public QDialog
{
public:
    explicit StatistiquesAnimaux(const QVector<FicheAnimal> &animaux, QWidget *parent = nullptr);
};

#endif // STATISTIQUESANIMAUX_H
