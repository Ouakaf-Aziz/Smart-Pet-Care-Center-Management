#ifndef CONNEXION_H
#define CONNEXION_H

#include <QObject>

#include "donneesemployes.h"

class QWidget;
namespace Ui { class GSmartPetCare; }

// Page d'authentification (pageLogin de gsmartpetcare.ui) : email + mot de passe.
// Émet connecte(id) quand l'employé est identifié, abandon() après 5 échecs.
class Connexion : public QObject
{
    Q_OBJECT

public:
    Connexion(Ui::GSmartPetCare *ui, const BaseEmployes &base, QWidget *fenetre);

signals:
    void connecte(int employeId);
    void abandon();

private slots:
    void onConnecter();

private:
    void afficherErreur(const QString &message);
    void reussir(int employeId);

    Ui::GSmartPetCare *ui;
    QWidget *m_fenetre;
    const BaseEmployes &m_base;
    int m_essais = 0;
};

#endif // CONNEXION_H
