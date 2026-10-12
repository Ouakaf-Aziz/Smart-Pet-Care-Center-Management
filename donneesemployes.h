#ifndef DONNEESEMPLOYES_H
#define DONNEESEMPLOYES_H

#include <QCryptographicHash>
#include <QDate>
#include <QMap>
#include <QRandomGenerator>
#include <QString>
#include <QStringList>
#include <QVector>

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

#endif // DONNEESEMPLOYES_H
