#ifndef DONNEES_H
#define DONNEES_H

#include <QDate>
#include <QString>
#include <QVector>

// La classe Service (modèle lié à la table SERVICE) est définie dans service.h.
// Elle remplace l'ancienne struct Service qui était ici (conflit de définition).
#include "service.h"

// Un animal, tel que FOURNI PAR LE MODULE ANIMAL (lecture seule dans ce module).
// Seul l'id est indispensable : l'historique s'y rattache. Le reste sert à l'affichage.
struct Animal
{
    int     id = 0;
    QString nom;
    QString espece;
    QString proprietaire;
};

// Un service rendu à un animal (une ligne de l'historique).
// Le nom, le type et le prix du service sont COPIÉS au moment de la prestation :
// l'historique reste correct même si le service est modifié ou supprimé plus tard.
struct ServiceRendu
{
    int     id = 0;
    int     animalId = 0;
    int     serviceId = 0;
    QString nomService;
    QString type;
    double  prix = 0.0;
    QDate   date;
    QString remarque;
};

// Historique en mémoire (aucune base de données : tout est perdu à la fermeture).
struct DonneesHistorique
{
    QVector<Animal>       animaux;      // liste donnée par le module Animal
    QVector<ServiceRendu> rendus;       // services rendus
    int                   prochainId = 1;   // prochain id à attribuer à un service rendu
};

#endif // DONNEES_H
