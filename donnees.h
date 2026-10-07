#ifndef DONNEES_H
#define DONNEES_H

#include <QCryptographicHash>
#include <QDate>
#include <QRandomGenerator>
#include <QString>
#include <QStringList>
#include <QVector>

// Un service proposé par la clinique.
struct Service
{
    int     id = 0;
    QString nom;
    QString description;
    QString type;
    double  prix = 0.0;
    int     duree = 0;          // minutes
    bool    disponible = true;
};

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

// Un employé de la clinique (module Gestion des employés).
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
    QString poste;
    QString motDePasse;     // JAMAIS en clair : "sel$hachage" (voir Emp::hacher)
};

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

inline void chargerDonneesDemo(BaseEmployes &base)
{
    auto ajout = [&base](const QString &nom, const QString &prenom, const QString &email,
                         const QString &adresse, const QString &tel, double salaire,
                         const QDate &date, const QString &statut, const QString &poste,
                         const QString &mdp) {
        Employe e;
        e.id = base.prochainId++;
        e.nom = nom; e.prenom = prenom; e.email = email; e.adresse = adresse;
        e.telephone = tel; e.salaire = salaire; e.dateEmbauche = date;
        e.statut = statut; e.poste = poste; e.motDePasse = Emp::hacher(mdp);
        base.employes.append(e);
    };

    ajout("Ben Salah", "Ali",     "rh@petnova.tn",        "12 rue de la Liberté, Tunis",   "21650123456", 3200, QDate(2019, 3, 4),  "Actif",    "Responsable RH",        "admin123");
    ajout("Trabelsi",  "Sonia",   "sonia.trabelsi@petnova.tn", "5 avenue Habib Bourguiba, Tunis", "21698234567", 2800, QDate(2020, 9, 14), "Actif",    "Comptable",             "petnova123");
    ajout("Gharbi",    "Mohamed", "mohamed.gharbi@petnova.tn", "8 rue Ibn Khaldoun, Ariana",      "21655345678", 3900, QDate(2018, 1, 22), "Actif",    "Vétérinaire",           "petnova123");
    ajout("Jlassi",    "Ines",    "ines.jlassi@petnova.tn",    "20 rue des Jasmins, La Marsa",    "21622456789", 3700, QDate(2021, 5, 3),  "Actif", "Vétérinaire",           "petnova123");
    ajout("Mansour",   "Karim",   "karim.mansour@petnova.tn",  "3 rue de Carthage, Tunis",        "21697567890", 1500, QDate(2022, 2, 7),  "Actif",    "Assistant vétérinaire", "petnova123");
    ajout("Dridi",     "Yasmine", "yasmine.dridi@petnova.tn",  "17 avenue de la République, Ben Arous", "21652678901", 1450, QDate(2023, 6, 19), "Actif", "Assistant vétérinaire", "petnova123");
    ajout("Hamdi",     "Nour",    "nour.hamdi@petnova.tn",     "9 rue Mongi Slim, Tunis",         "21623789012", 1300, QDate(2023, 10, 2), "Actif",    "Réceptionniste",        "petnova123");
    ajout("Ayari",     "Sami",    "sami.ayari@petnova.tn",     "14 rue Farhat Hached, Radès",     "21699890123", 1250, QDate(2024, 1, 15), "Inactif", "Réceptionniste",        "petnova123");
    ajout("Chaabane",  "Rim",     "rim.chaabane@petnova.tn",   "6 rue du Lac, Les Berges du Lac", "21653901234", 1400, QDate(2022, 8, 29), "Actif",    "Toiletteur",            "petnova123");
    ajout("Bouzid",    "Hatem",   "hatem.bouzid@petnova.tn",   "11 rue de Marseille, Tunis",      "21624012345", 1350, QDate(2021, 11, 8), "Inactif", "Toiletteur",      "petnova123");
}

} // namespace Emp

#endif // DONNEES_H
