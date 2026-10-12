#ifndef PRIVILEGES_H
#define PRIVILEGES_H

#include <QObject>
#include <QStringList>
#include <QVector>

#include "donneesemployes.h"

class QWidget;
namespace Ui { class GSmartPetCare; }

// Page « Rôles et privilèges » (pagePrivileges de gsmartpetcare.ui), en CONSULTATION SEULE.
// Le rôle d'un employé est son poste ; chaque ligne = un poste, chaque colonne = un droit.
// Les privilèges sont attribués automatiquement selon le rôle (Emp::droitsParDefaut) : personne ne les modifie.
class Privileges : public QObject
{
    Q_OBJECT

public:
    Privileges(Ui::GSmartPetCare *ui, BaseEmployes &base, QWidget *fenetre);

    void ouvrir();                  // (re)construit le tableau avec les droits et effectifs actuels

signals:
    void termine();                 // bouton « Retour » : la fenêtre principale revient aux employés

private:
    enum Droit { Ajouter, Modifier, Supprimer, VoirSalaires, Exporter, Statistiques, NbDroits };


    Ui::GSmartPetCare *ui;
    QWidget *m_fenetre;
    BaseEmployes &m_base;
    QStringList m_postes;
};

#endif // PRIVILEGES_H
