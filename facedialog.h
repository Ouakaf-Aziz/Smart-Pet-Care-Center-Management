#ifndef FACEDIALOG_H
#define FACEDIALOG_H

#include <QElapsedTimer>
#include <QEventLoop>
#include <QObject>
#include <QTimer>
#include <QVector>

#include "donneesemployes.h"
#include "faceengine.h"

class QWidget;
namespace Ui { class GSmartPetCare; }

// Page d'aperçu caméra (pageVisage de gsmartpetcare.ui), en deux modes :
//  - Enregistrement : capture kEchantillons empreintes du visage d'un employé ;
//  - Authentification : identifie la personne devant la caméra parmi les employés enregistrés.
class FaceDialog : public QObject
{
    Q_OBJECT

public:
    enum Mode { Enregistrement, Authentification };
    static constexpr int kEchantillons = 5;

    // Réglages de mise en page de la page, à faire une seule fois au démarrage.
    static void preparerPage(Ui::GSmartPetCare *ui);

    // `employes` n'est utilisé qu'en mode Authentification (et doit survivre à l'objet).
    FaceDialog(Ui::GSmartPetCare *ui, Mode mode, const QVector<Employe> *employes, const QString &titre,
               QWidget *fenetre = nullptr);
    ~FaceDialog() override;

    bool demarrer(QString *erreur);                 // charge les modèles + ouvre la caméra
    bool exec();                                    // affiche la page caméra jusqu'à la fin ; true = réussi

    int employeId() const { return m_employeId; }   // Authentification : id reconnu
    const QVector<QVector<float>> &gabarits() const { return m_gabarits; }
    bool expire() const { return m_expire; }        // Authentification : temps écoulé sans succès

private slots:
    void onTick();

private:
    void terminer(bool reussi);
    void afficher(QImage image, const QVector<VisageDetecte> &visages, bool ok);

    Ui::GSmartPetCare *ui;
    Mode m_mode;
    const QVector<Employe> *m_employes;
    FaceEngine m_moteur;
    QTimer m_timer;
    QEventLoop m_boucle;                // exec() attend ici la fin de la capture
    QElapsedTimer m_chrono;             // durée totale
    QElapsedTimer m_derniereCapture;

    QVector<QVector<float>> m_gabarits;
    int m_employeId = -1;
    int m_candidat = -1;
    int m_stable = 0;                   // images consécutives avec le même candidat
    int m_tick = 0;
    int m_echecsLecture = 0;
    bool m_expire = false;
    bool m_termine = false;
    bool m_reussi = false;
};

#endif // FACEDIALOG_H
