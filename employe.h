#ifndef EMPLOYE_H
#define EMPLOYE_H

// Module Employés : tout le code du module est regroupé dans employe.h / employe.cpp
// (requêtes SQL, données, gestion, connexion, privilèges, statistiques, graphiques, reconnaissance faciale).

#include <QColor>
#include <QCryptographicHash>
#include <QDate>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QImage>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QRandomGenerator>
#include <QRect>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVector>
#include <QWidget>
#include <memory>

// ============================================================================
// donneesemployes
// ============================================================================
// Un employé de la clinique Pet Nova.
struct Employe
{
    int     id = 0;
    QString nom;
    QString prenom;
    QString email;
    QString adresse;
    QString telephone;
    double  salaire = 0.0;
    QDate   dateEmbauche;
    QString statut;         // Actif ou Inactif
    QString poste;          // détermine aussi le rôle / les privilèges
    QString motDePasse;     // JAMAIS en clair : "sel$hachage" (voir Emp::hacher)

    // Reconnaissance faciale : un gabarit (empreinte numérique de 128 valeurs) par échantillon
    // capturé. Aucune photo n'est conservée. Vide = visage non enregistré.
    QVector<QVector<float>> gabaritsVisage;
};

// Ce qu'un employé a le droit de faire, selon son poste.
struct Droits
{
    bool ajouter       = false;
    bool modifier      = false;
    bool supprimer     = false;
    bool voirSalaires  = false;
    bool exporter      = false;
    bool statistiques  = true;
    bool gererPrivileges = false;   // réservé au Responsable RH, jamais modifiable
};

// Données en mémoire (aucune base de données : tout est perdu à la fermeture).
struct BaseEmployes
{
    QVector<Employe> employes;
    int prochainId = 1;

    Employe *trouver(int id)
    {
        for (Employe &e : employes)
            if (e.id == id)
                return &e;
        return nullptr;
    }
    const Employe *trouver(int id) const
    {
        for (const Employe &e : employes)
            if (e.id == id)
                return &e;
        return nullptr;
    }
    const Employe *trouverParEmail(const QString &email) const
    {
        for (const Employe &e : employes)
            if (e.email.compare(email.trimmed(), Qt::CaseInsensitive) == 0)
                return &e;
        return nullptr;
    }
    // Privilèges par rôle (= poste) modifiés depuis l'interface. Un poste absent de cette table
    // garde ses droits par défaut (Emp::droitsParDefaut).

    Droits droits(const QString &poste) const;                  // droits effectifs d'un poste

    int nombreResponsablesRH() const
    {
        int n = 0;
        for (const Employe &e : employes)
            if (e.poste == QLatin1String("Responsable RH"))
                ++n;
        return n;
    }
};

namespace Emp {

inline QStringList postes()
{
    return { QStringLiteral("Responsable RH"), QStringLiteral("Comptable"),
             QStringLiteral("Vétérinaire"),    QStringLiteral("Assistant vétérinaire"),
             QStringLiteral("Réceptionniste"), QStringLiteral("Toiletteur") };
}

inline QStringList statuts()
{
    return { QStringLiteral("Actif"), QStringLiteral("Inactif") };
}

// Droits PAR DÉFAUT d'un rôle (= le poste de l'employé). Ils sont attribués automatiquement
// selon le poste (personne ne les modifie : l'écran « Rôles et privilèges » est en consultation seule).
inline Droits droitsParDefaut(const QString &poste)
{
    Droits d;
    if (poste == QLatin1String("Responsable RH")) {        // accès total
        d.ajouter = d.modifier = d.supprimer = true;
        d.voirSalaires = d.exporter = d.statistiques = true;
        d.gererPrivileges = true;
    } else if (poste == QLatin1String("Comptable")) {      // lecture + salaires + exports
        d.voirSalaires = d.exporter = d.statistiques = true;
    } else {                                               // consultation seule, sans salaires
        d.statistiques = true;
    }
    return d;
}

// Mot de passe : SHA-256 avec un sel aléatoire. Format stocké : "sel$hachage".
inline QString hacherAvecSel(const QString &sel, const QString &mdp)
{
    return QString::fromLatin1(
        QCryptographicHash::hash((sel + mdp).toUtf8(), QCryptographicHash::Sha256).toHex());
}

inline QString hacher(const QString &mdp)
{
    const QString sel = QString::number(QRandomGenerator::global()->generate64(), 16);
    return sel + QLatin1Char('$') + hacherAvecSel(sel, mdp);
}

inline bool verifier(const QString &mdp, const QString &stocke)
{
    const int i = stocke.indexOf(QLatin1Char('$'));
    if (i <= 0)
        return false;
    return hacherAvecSel(stocke.left(i), mdp) == stocke.mid(i + 1);
}

} // namespace Emp

// Les droits d'un employé sont déterminés automatiquement par son poste.
inline Droits BaseEmployes::droits(const QString &poste) const
{
    return Emp::droitsParDefaut(poste);
}


// ============================================================================
// employe
// ============================================================================
class EmployeSQL
{
public:
    // Données de l'employé
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

    // Constructeurs
    EmployeSQL();

    EmployeSQL(int id,
            QString nom,
            QString prenom,
            QString email,
            QString adresse,
            QString telephone,
            double salaire,
            QDate dateEmbauche,
            QString statut,
            QString poste,
            QString motDePasse);

    // CRUD Oracle
    bool ajouter();
    bool supprimer();
    bool modifier();

    // Affichage depuis Oracle
    static QSqlQueryModel* afficher();
};


// ============================================================================
// faceengine
// ============================================================================
// Un visage détecté dans l'image de la caméra.
struct VisageDetecte
{
    QRect cadre;
    float score = 0.f;      // confiance du détecteur (0..1)
};

// Moteur de reconnaissance faciale, basé sur OpenCV (modules principaux uniquement, pas de contrib) :
//   - détection : YuNet  (face_detection_yunet_*.onnx)
//   - empreinte : SFace  (face_recognition_sface_2021dec.onnx) -> vecteur de 128 valeurs
// Les deux fichiers .onnx sont dans le dossier models/ (voir models/LISEZMOI.md).
// Si le projet est compilé sans OpenCV (HAVE_OPENCV absent), compileAvecOpenCV() renvoie false
// et tout le reste échoue proprement avec un message.
class FaceEngine
{
public:
    static constexpr int kDimension = 128;

    FaceEngine();
    ~FaceEngine();
    FaceEngine(const FaceEngine &) = delete;
    FaceEngine &operator=(const FaceEngine &) = delete;

    static bool compileAvecOpenCV();

    bool charger(QString *erreur);          // charge les deux modèles
    bool ouvrirCamera(QString *erreur);
    void fermerCamera();

    // Lit une image et détecte les visages (triés du plus grand au plus petit).
    bool lire(QImage *image, QVector<VisageDetecte> *visages);

    // Empreinte (normalisée) du visage n° `index` de la dernière image lue.
    bool calculerGabarit(int index, QVector<float> *gabarit, QString *erreur = nullptr);

    // Similarité cosinus entre deux empreintes : 1 = identiques, ~0 = personnes différentes.
    static double similarite(const QVector<float> &a, const QVector<float> &b);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

// Décision d'identification (indépendante d'OpenCV, donc testable seule).
namespace FaceMatch {

constexpr double kSeuil = 0.40;     // similarité minimale (0.363 = seuil officiel de SFace, on est plus strict)
constexpr double kMarge = 0.05;     // écart minimal avec le 2e meilleur employé (évite les ambiguïtés)

struct Resultat
{
    int    employeId = -1;
    double score = -1.0;            // meilleure similarité
    double second = -1.0;           // meilleure similarité parmi les AUTRES employés
    bool   accepte = false;         // score >= seuil ET écart >= marge
};

Resultat identifier(const QVector<float> &gabarit, const QVector<Employe> &employes);

} // namespace FaceMatch


// ============================================================================
// barchart
// ============================================================================
// Diagramme en barres : vertical (histogramme) ou horizontal.
class BarChart : public QWidget
{
    Q_OBJECT
public:
    enum Orientation { Vertical, Horizontal };

    explicit BarChart(QWidget *parent = nullptr);

    // ex. { {"0 - 25", 3}, {"25 - 50", 4} }
    void setData(const QVector<QPair<QString, double>> &data,
                 Orientation orientation = Vertical,
                 const QString &suffixeValeur = QString(),
                 const QColor &couleur = QColor("#0F766E"));

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString formater(double v) const;

    QVector<QPair<QString, double>> m_data;
    Orientation m_orientation = Vertical;
    QString m_suffixe;
    QColor m_couleur;
};


// ============================================================================
// donutchart
// ============================================================================
// Cercle (anneau) de statistiques avec le total au centre et une légende à droite.
class DonutChart : public QWidget
{
    Q_OBJECT
public:
    explicit DonutChart(QWidget *parent = nullptr);

    // ex. { {"Consultation", 5}, {"Vaccination", 3} }
    void setData(const QMap<QString, int> &data);
    void clear();
    void setUnite(const QString &unite);   // texte sous le total (par défaut « services », « employés » pour les employés)

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QMap<QString, int> m_data;
    QVector<QColor> m_colors;
    QString m_unite = QStringLiteral("services");
};


// ============================================================================
// linechart
// ============================================================================
// Courbe d'évolution (ex. recrutements par année d'embauche).
class LineChart : public QWidget
{
    Q_OBJECT
public:
    explicit LineChart(QWidget *parent = nullptr);

    // ex. { {"2019", 1}, {"2020", 2}, {"2021", 3} }
    void setData(const QVector<QPair<QString, double>> &data,
                 const QColor &couleur = QColor("#0F766E"));

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<QPair<QString, double>> m_data;
    QColor m_couleur;
};


// ============================================================================
// faceauth
// ============================================================================
namespace Ui { class GSmartPetCare; }

// Authentification et enregistrement par reconnaissance faciale (OpenCV : YuNet + SFace).
//
// Prérequis : OpenCV >= 4.5.4 à la compilation (sinon le module reste inactif et le bouton
// affiche un message) et les deux fichiers .onnx du dossier models/ (voir models/LISEZMOI.md).
//
// Ce qui est stocké : pour chaque employé, 5 empreintes numériques de 128 valeurs. Aucune photo.
// Elles sont sauvegardées dans le dossier de données de l'application (visages.json, lisible
// uniquement par l'utilisateur du système), indexées par email.
namespace FaceAuth {

// Affiche la page caméra (pageVisage de gsmartpetcare.ui), ouvre la caméra et retourne l'id de l'employé reconnu, ou -1 (échec / annulation / non disponible).
int authentifier(Ui::GSmartPetCare *ui, QWidget *parent, const QVector<Employe> &employes);

// Capture 5 échantillons du visage de l'employé et les enregistre. Refuse si ce visage ressemble
// trop à celui d'un autre employé. Retourne true si l'enregistrement a eu lieu.
bool enregistrer(Ui::GSmartPetCare *ui, QWidget *parent, BaseEmployes &base, int employeId);

// Persistance des empreintes (à appeler après chargement des employés, puis après chaque changement).
void chargerGabarits(BaseEmployes &base);
bool sauvegarderGabarits(const BaseEmployes &base);

} // namespace FaceAuth


// ============================================================================
// facedialog
// ============================================================================
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


// ============================================================================
// connexion
// ============================================================================
class QPushButton;
class QWidget;
namespace Ui { class GSmartPetCare; }

// Page d'authentification (pageLogin de gsmartpetcare.ui).
// Deux façons de se connecter, réservées aux employés de la liste (Oracle) au statut « Actif » :
//  - identifiant (ID de l'employé, ou son email) + mot de passe ;
//  - reconnaissance faciale (bouton « Se connecter avec le visage », créé ici dans le code).
// Émet connecte(id) quand l'employé est identifié, abandon() après 5 échecs.
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
    void chargerEmployes();                                   // employés Oracle + empreintes de visage
    const Employe *trouverEmploye(const QString &identifiant) const;
    void afficherErreur(const QString &message);
    void echec(const QString &message);                       // compte les essais ratés
    void reussir(int employeId);

    Ui::GSmartPetCare *ui;
    QWidget *m_fenetre;
    BaseEmployes &m_base;
    QPushButton *m_btnVisage = nullptr;
    int m_essais = 0;
};


// ============================================================================
// privileges
// ============================================================================
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


// ============================================================================
// statistiquesemployes
// ============================================================================
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


// ============================================================================
// gestionemployes
// ============================================================================
class QWidget;
namespace Ui { class GSmartPetCare; }

// Écran « Gestion des employés » : logique de la page pageEmployes de gsmartpetcare.ui
// (modifiable avec Qt Designer). Ce n'est plus une fenêtre : elle pilote les widgets de la page.
class GestionEmployes : public QObject
{
    Q_OBJECT

public:
    GestionEmployes(Ui::GSmartPetCare *ui, BaseEmployes &base, QWidget *fenetre);

    // À appeler après la connexion : applique les privilèges de l'employé connecté.
    void setUtilisateur(int employeId);

signals:
    void statistiquesDemandees(bool voirSalaires);   // la fenêtre principale affiche pageStats
    void privilegesDemandes();                       // ... ou pagePrivileges

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
    void refreshTable();                // recherche + filtre + tri

private:
    struct DonneesExport {
        QStringList entetes;
        QVector<QStringList> lignes;
    };

    void configurerInterface();         // réglages que Designer ne sait pas faire (listes, colonnes, signaux)
    void appliquerDroits();
    void viderFormulaire();
    void remplirFormulaire(const Employe &e);
    void montrerChamp(QWidget *champ, bool visible);
    void majEtatVisage(int employeId);          // affiche l'état du visage de l'employé (-1 = aucun)
    QWidget *creerBoutonsActions(int employeId);
    DonneesExport donneesExport() const;
    QString htmlTableau() const;

    BaseEmployes &m_base;
    Droits m_droits;
    int m_userId = -1;
    int m_currentId = -1;               // -1 = nouvel employé
    QVector<Employe> m_vue;             // employés actuellement affichés (après filtre/tri)

    Ui::GSmartPetCare *ui;
    QWidget *m_fenetre;                 // fenêtre principale : parent des boîtes de dialogue
};

// Charge les employés de démonstration dans la base (à appeler une fois au démarrage).
void chargerEmployesDemo(BaseEmployes &base);

#endif // EMPLOYE_H
