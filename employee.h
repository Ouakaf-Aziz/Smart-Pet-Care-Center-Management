#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include <QDate>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QImage>
#include <QObject>
#include <QRect>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVector>

#include <memory>

class QPushButton;
class QWidget;
namespace Ui { class GSmartPetCare; }

struct Employe
{
    int id = 0;
    QString nom;
    QString prenom;
    QString email;
    QString adresse;
    QString telephone;
    double salaire = 0.0;
    QDate dateEmbauche;
    QString statut;
    QString poste;
    QString motDePasse;
    QVector<QVector<float>> gabaritsVisage;
};

struct Droits
{
    bool ajouter = false;
    bool modifier = false;
    bool supprimer = false;
    bool voirSalaires = false;
    bool exporter = false;
    bool statistiques = true;
    bool gererPrivileges = false;
};

struct BaseEmployes
{
    QVector<Employe> employes;
    int prochainId = 1;

    Employe *trouver(int id);
    const Employe *trouver(int id) const;
    const Employe *trouverParEmail(const QString &email) const;
    Droits droits(const QString &poste) const;
    int nombreResponsablesRH() const;
};

namespace Emp {
QStringList postes();
QStringList statuts();
Droits droitsParDefaut(const QString &poste);
QString hacherAvecSel(const QString &sel, const QString &mdp);
QString hacher(const QString &mdp);
bool verifier(const QString &mdp, const QString &stocke);
}

class EmployeSQL
{
public:
    int id;
    QString nom;
    QString prenom;
    QString email;
    QString adresse;
    QString telephone;
    double salaire;
    QDate dateEmbauche;
    QString statut;
    QString poste;
    QString motDePasse;

    EmployeSQL();
    EmployeSQL(int id, QString nom, QString prenom, QString email, QString adresse,
               QString telephone, double salaire, QDate dateEmbauche, QString statut,
               QString poste, QString motDePasse);

    bool ajouter();
    bool supprimer();
    bool modifier();
    static QSqlQueryModel *afficher();
};

struct VisageDetecte
{
    QRect cadre;
    float score = 0.f;
};

class FaceEngine
{
public:
    static constexpr int kDimension = 128;

    FaceEngine();
    ~FaceEngine();
    FaceEngine(const FaceEngine &) = delete;
    FaceEngine &operator=(const FaceEngine &) = delete;

    static bool compileAvecOpenCV();
    bool charger(QString *erreur);
    bool ouvrirCamera(QString *erreur);
    void fermerCamera();
    bool lire(QImage *image, QVector<VisageDetecte> *visages);
    bool calculerGabarit(int index, QVector<float> *gabarit, QString *erreur = nullptr);
    static double similarite(const QVector<float> &a, const QVector<float> &b);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

namespace FaceMatch {
constexpr double kSeuil = 0.40;
constexpr double kMarge = 0.05;

struct Resultat
{
    int employeId = -1;
    double score = -1.0;
    double second = -1.0;
    bool accepte = false;
};

Resultat identifier(const QVector<float> &gabarit, const QVector<Employe> &employes);
}

namespace FaceAuth {
int authentifier(Ui::GSmartPetCare *ui, QWidget *parent, const QVector<Employe> &employes);
bool enregistrer(Ui::GSmartPetCare *ui, QWidget *parent, BaseEmployes &base, int employeId);
void chargerGabarits(BaseEmployes &base);
bool sauvegarderGabarits(const BaseEmployes &base);
}

class FaceDialog : public QObject
{
    Q_OBJECT
public:
    enum Mode { Enregistrement, Authentification };
    static constexpr int kEchantillons = 5;

    static void preparerPage(Ui::GSmartPetCare *ui);
    FaceDialog(Ui::GSmartPetCare *ui, Mode mode, const QVector<Employe> *employes,
               const QString &titre, QWidget *fenetre = nullptr);
    ~FaceDialog() override;

    bool demarrer(QString *erreur);
    bool exec();
    int employeId() const { return m_employeId; }
    const QVector<QVector<float>> &gabarits() const { return m_gabarits; }
    bool expire() const { return m_expire; }

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
    QEventLoop m_boucle;
    QElapsedTimer m_chrono;
    QElapsedTimer m_derniereCapture;
    QVector<QVector<float>> m_gabarits;
    int m_employeId = -1;
    int m_candidat = -1;
    int m_stable = 0;
    int m_tick = 0;
    int m_echecsLecture = 0;
    bool m_expire = false;
    bool m_termine = false;
    bool m_reussi = false;
};

class Connexion : public QObject
{
    Q_OBJECT
public:
    Connexion(Ui::GSmartPetCare *ui, BaseEmployes &base, QWidget *fenetre);

signals:
    void connecte(int employeId);
    void abandon();

private slots:
    void onConnecter();
    void onConnecterVisage();

private:
    void chargerEmployes();
    const Employe *trouverEmploye(const QString &identifiant) const;
    void afficherErreur(const QString &message);
    void echec(const QString &message);
    void reussir(int employeId);

    Ui::GSmartPetCare *ui;
    QWidget *m_fenetre;
    BaseEmployes &m_base;
    QPushButton *m_btnVisage = nullptr;
    int m_essais = 0;
};

class GestionEmployes : public QObject
{
    Q_OBJECT
public:
    GestionEmployes(Ui::GSmartPetCare *ui, BaseEmployes &base, QWidget *fenetre);
    void setUtilisateur(int employeId);

signals:
    void statistiquesDemandees(bool voirSalaires);
    void privilegesDemandes();

private slots:
    void onAjouter();
    void onModifier(int employeId);
    void onSupprimer(int employeId);
    void onEnregistrer();
    void onAnnuler();
    void onSelectionChanged();
    void onExporterPdf();
    void onStatistiques();
    void onPrivileges();
    void onEnregistrerVisage();
    void onSupprimerVisage();
    void refreshTable();

private:
    struct DonneesExport {
        QStringList entetes;
        QVector<QStringList> lignes;
    };

    void configurerInterface();
    void appliquerDroits();
    void viderFormulaire();
    void remplirFormulaire(const Employe &e);
    void montrerChamp(QWidget *champ, bool visible);
    void majEtatVisage(int employeId);
    QWidget *creerBoutonsActions(int employeId);
    DonneesExport donneesExport() const;
    QString htmlTableau() const;

    BaseEmployes &m_base;
    Droits m_droits;
    int m_userId = -1;
    int m_currentId = -1;
    QVector<Employe> m_vue;
    Ui::GSmartPetCare *ui;
    QWidget *m_fenetre;
};

class StatistiquesEmployes : public QObject
{
    Q_OBJECT
public:
    StatistiquesEmployes(Ui::GSmartPetCare *ui, QObject *parent = nullptr);
    void afficher(const QVector<Employe> &employes, bool voirSalaires);

signals:
    void retour();

private:
    void calculer();
    Ui::GSmartPetCare *ui;
    QVector<Employe> m_employes;
    bool m_voirSalaires;
};

class Privileges : public QObject
{
    Q_OBJECT
public:
    Privileges(Ui::GSmartPetCare *ui, BaseEmployes &base, QWidget *fenetre);
    void ouvrir();

signals:
    void termine();

private:
    enum Droit { Ajouter, Modifier, Supprimer, VoirSalaires, Exporter, Statistiques, NbDroits };

    Ui::GSmartPetCare *ui;
    QWidget *m_fenetre;
    BaseEmployes &m_base;
    QStringList m_postes;
};

void chargerEmployesDemo(BaseEmployes &base);

#endif
