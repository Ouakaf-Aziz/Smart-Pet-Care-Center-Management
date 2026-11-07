#ifndef RENDEZVOUS_H
#define RENDEZVOUS_H

#include <QString>
#include <QList>
#include <QStringList>
#include <QPair>
#include <QVariant>
#include <QDate>
#include <QTime>

// Accès à la table Oracle RENDEZ_VOUS
// (ID_RDV NUMBER, DATE_RDV DATE, HEURE VARCHAR2(5), TYPE_RDV, DUREE NUMBER, PRIORITE,
//  SALLE, ID_ANIMAL -> ANIMAL, ID_EMPLOYEE -> EMPLOYEE)
class RendezVous
{
public:
    RendezVous();

    // idAnimal : QVariant nul = pas d'animal (colonne ID_ANIMAL à NULL)
    bool ajouter(int id,
                 const QString &priorite,
                 int duree,
                 const QString &type,
                 const QString &salle,
                 const QTime &heure,
                 const QDate &date,
                 const QVariant &idAnimal = QVariant(),
                 const QVariant &idEmploye = QVariant());

    // Chaque ligne : ID_RDV, PRIORITE, DUREE, TYPE_RDV, SALLE, HEURE (HH:mm),
    //                DATE (dd/MM/yyyy), NOM de l'animal, ID_ANIMAL, ID_EMPLOYEE
    QList<QStringList> afficher();

    bool modifier(int id,
                  const QString &priorite,
                  int duree,
                  const QString &type,
                  const QString &salle,
                  const QTime &heure,
                  const QDate &date,
                  const QVariant &idAnimal = QVariant(),
                  const QVariant &idEmploye = QVariant());

    bool supprimer(int id);

    bool existe(int id);
    bool animalExiste(int id);
    bool employeExiste(int id);
    int  prochainId();                                  // MAX(ID_RDV) + 1
    QList<QPair<int, QString>> listerAnimaux();
    // Nombre de rendez-vous par valeur d'une colonne (TYPE_RDV, PRIORITE ou SALLE)
    QList<QPair<QString, int>> compterPar(const QString &colonne);         // (ID_ANIMAL, NOM) pour la liste déroulante

    QString getDerniereErreur() const;

private:
    QVariant entierOuNull(const QVariant &valeur) const;
    QString derniereErreur;
};

#endif // RENDEZVOUS_H