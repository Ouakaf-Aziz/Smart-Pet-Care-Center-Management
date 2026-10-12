#ifndef STATISTIQUESEMPLOYES_H
#define STATISTIQUESEMPLOYES_H

#include <QObject>
#include <QVector>

#include "donneesemployes.h"

namespace Ui { class GSmartPetCare; }

// Page « Statistiques des employés » (pageStatsEmployes de gsmartpetcare.ui) :
//  - cercle (anneau) : répartition par statut (actif / inactif)
//  - barres          : répartition par poste
//  - histogramme     : analyse des salaires (moyen, minimum, maximum), si l'utilisateur voit les salaires
//  - courbe          : évolution des recrutements par année d'embauche
class StatistiquesEmployes : public QObject
{
    Q_OBJECT

public:
    StatistiquesEmployes(Ui::GSmartPetCare *ui, QObject *parent = nullptr);

    // Calcule et dessine les 4 graphiques (à appeler à chaque ouverture de la page).
    void afficher(const QVector<Employe> &employes, bool voirSalaires);

signals:
    void retour();                  // bouton « Retour » : la fenêtre principale revient aux employés

private:
    void calculer();

    Ui::GSmartPetCare *ui;
    QVector<Employe> m_employes;
    bool m_voirSalaires;
};

#endif // STATISTIQUESEMPLOYES_H
