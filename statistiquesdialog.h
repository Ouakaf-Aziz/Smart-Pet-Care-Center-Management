#ifndef STATISTIQUESDIALOG_H
#define STATISTIQUESDIALOG_H

#include <QDate>
#include <QDialog>
#include <QString>
#include <QVector>

// Une ligne de donnees utilisee pour calculer les statistiques
struct RdvStat
{
    QString type;       // vaccination, consultation, ...
    QString statut;     // confirme, annule
    QString priorite;   // urgent, tres urgent, ou vide (= normal)
    QString salle;      // 1, 2, ...
    int duree = 0;      // en minutes
    QDate date;         // date du rendez-vous
};

class StatistiquesDialog : public QDialog
{
public:
    explicit StatistiquesDialog(const QVector<RdvStat> &donnees, QWidget *parent = nullptr);
};

#endif // STATISTIQUESDIALOG_H