#include "service.h"
#include <QDate>
#include <QDebug>
#include <QObject>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QVariantMap>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QSslSocket>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

#include <algorithm>
#include <cmath>
#include <utility>

// ===========================================================================
// SOMMAIRE DU FICHIER : MODELE - classe Service (toutes les requêtes SQL)
//   1. Contrôle de saisie         : controleSaisie() (une seule fonction pour tous les champs)
//   2. CRUD                       : ajouter(), afficher(), modifier(), supprimer()
//   3. Recherches                 : lister(), rechercher() (+ filtre + tri)
//   4. Autres
//        4.1 Statistiques         : statistiques()
//        4.2 Historique des services rendus
//             - recherche d'animaux      : Animal::rechercherParTexte()   (animal.cpp)
//             - affichage + filtre + tri : historiqueAnimal()
//             - rendez-vous d'un animal  : RendezVous::rendezVousAnimal() (RendezVous.cpp)
//             - statistiques d'un animal : Animal::statistiquesServices() (animal.cpp)
//             - CRUD (table INCLURE)     : ajouterRendu(), modifierRendu(), supprimerRendu()
//   5. Rappel automatique par SMS : Service::demarrerRappels() (en fin de fichier, appelée par main.cpp)
//
// Les VUES (GestionServices, HistoriqueServices, StatistiquesServices, GraphiqueService)
// sont dans gsmartpetcare.cpp / gsmartpetcare.h, à côté de gsmartpetcare.ui.
// ===========================================================================

namespace {
// Motif LIKE « commence par » : échappe le backslash, % et _ puis ajoute % à la fin.
// (recherche par lettre : « d » ne garde que les noms qui COMMENCENT par d)
QString motifDebut(const QString &texte)
{
    QString m = texte.trimmed();
    m.replace("\\", "\\\\");
    m.replace("%", "\\%");
    m.replace("_", "\\_");
    return m + "%";
}

// INCLURE.OBSERVATION est NOT NULL et Oracle traite '' comme NULL : on stocke "-" si vide.
QString observationPourBase(const QString &texte)
{
    const QString s = texte.trimmed();
    return s.isEmpty() ? QString("-") : s;
}

// Prépare la requête « recherche + filtre + tri » sur SERVICE (partagée par rechercher() et afficher()).
//  - recherche : sur le NOM uniquement, « commence par », insensible à la casse
//    (UPPER ... LIKE UPPER). Ni la description ni le type ne sont consultés : taper « d »
//    ne ramène donc que les services dont le nom commence par d.
//  - valeurs passées par bindValue (pas de concaténation => pas d'injection SQL)
//  - ORDER BY ne peut pas être une variable liée : clause choisie dans une liste fixe (« liste blanche »)
void preparerRechercheService(QSqlQuery &query, const QString &colonnes, const QString &recherche,
                              const QString &type, int tri, bool decroissant)
{
    QString sql = "SELECT " + colonnes + " FROM SERVICE WHERE 1 = 1 ";

    const QString q = recherche.trimmed();
    const bool avecRecherche = !q.isEmpty();
    const bool avecType = !type.trimmed().isEmpty();

    if (avecRecherche)
        sql += "AND UPPER(NOM_SERVICE) LIKE UPPER(:q) ESCAPE '\\' ";
    if (avecType)
        sql += "AND TYPE_SERVICE = :type ";

    const QString direction = decroissant ? " DESC" : " ASC";
    switch (tri) {
    case 1:  sql += "ORDER BY DUREE_SERVICE" + direction + ", UPPER(NOM_SERVICE)"; break;   // Durée
    default: sql += "ORDER BY PRIX_SERVICE" + direction + ", UPPER(NOM_SERVICE)";  break;   // Prix
    }

    query.prepare(sql);

    if (avecRecherche)
        query.bindValue(":q", motifDebut(q));
    if (avecType)
        query.bindValue(":type", type.trimmed());
}
} // namespace

// ===========================================================================
// MODELE - classe Service (requêtes SQL préparées + bindValue)
// ===========================================================================

// ---------------------------------------------------------------------------
// CONSTRUCTEURS
// ---------------------------------------------------------------------------
Service::Service()
    : id(0), prix(0.0), duree(0), disponible(true)
{
}

Service::Service(int id, QString nom, QString description, QString type,
                 double prix, int duree, bool disponible)
{
    this->id = id;
    this->nom = nom;
    this->description = description;
    this->type = type;
    this->prix = prix;
    this->duree = duree;
    this->disponible = disponible;
}

// ===========================================================================
// 1. CONTRÔLE DE SAISIE : UNE SEULE fonction pour tous les champs
//    (utilisée par ajouter(), modifier() et par la vue)
// ===========================================================================

// ---------------------------------------------------------------------------
// Règles adaptées à un centre de soins pour animaux (consultation, vaccination,
// toilettage, chirurgie, éducation...). Les limites se changent ici, en un seul endroit.
//  - nom         : 2 à 50 caractères, commence par une lettre ; lettres (accents compris),
//                  chiffres, espaces et . , ' - & / ( ) autorisés (ex. « Vaccin 2 en 1 »)
//  - description : 5 à 255 caractères, au moins une lettre
//  - type        : obligatoire
//  - prix        : de 30 à 5000 DT, 2 décimales au plus
//  - durée       : entier de 5 à 480 minutes (8 h maximum)
//
// prixSeulement = true : ne contrôle que le prix, avec 0 permis (service offert).
// C'est le mode du formulaire de l'historique (gsmartpetcare.cpp, onEnregistrerRendu).
// ---------------------------------------------------------------------------
namespace {
const int    NOM_MIN = 2,         NOM_MAX = 50;
const int    DESCRIPTION_MIN = 5, DESCRIPTION_MAX = 255;
const double PRIX_MIN = 30.0,      PRIX_MAX = 5000.0;     // en dinars (DT)
const int    DUREE_MIN = 5,       DUREE_MAX = 480;       // en minutes
}

bool Service::controleSaisie(QString &erreur, Champ *champ, bool prixSeulement) const
{
    if (champ)
        *champ = ChampAucun;
    auto echec = [&](Champ fautif, const QString &message) {
        erreur = message;
        if (champ)
            *champ = fautif;
        return false;
    };

    if (!prixSeulement) {
        const QString n = nom.trimmed();
        const QString d = description.trimmed();

        // Nom
        static const QRegularExpression reNom(QStringLiteral("^\\p{L}[\\p{L}\\p{N} .,'\\x{2019}\\-&/()]*$"));
        if (n.isEmpty())
            return echec(ChampNom, QObject::tr("Le nom du service est obligatoire."));
        if (n.length() < NOM_MIN || n.length() > NOM_MAX || !reNom.match(n).hasMatch())
            return echec(ChampNom, QObject::tr("Le nom doit faire %1 à %2 caractères, commencer par une lettre et ne "
                                               "contenir que des lettres, chiffres, espaces ou . , ' - & / ( ).")
                                       .arg(NOM_MIN).arg(NOM_MAX));

        // Description
        static const QRegularExpression reLettre(QStringLiteral("\\p{L}"));
        if (d.isEmpty())
            return echec(ChampDescription, QObject::tr("La description est obligatoire."));
        if (d.length() < DESCRIPTION_MIN || d.length() > DESCRIPTION_MAX || !reLettre.match(d).hasMatch())
            return echec(ChampDescription, QObject::tr("La description doit faire %1 à %2 caractères et contenir du texte.")
                                               .arg(DESCRIPTION_MIN).arg(DESCRIPTION_MAX));

        // Type
        if (type.trimmed().isEmpty())
            return echec(ChampType, QObject::tr("Choisissez un type de service."));
    }

    // Prix : nombre fini, dans l'intervalle autorisé, avec au plus 2 décimales
    const double prixMin = prixSeulement ? 0.0 : PRIX_MIN;
    if (!std::isfinite(prix) || prix < prixMin || prix > PRIX_MAX)
        return echec(ChampPrix, QObject::tr("Le prix doit être compris entre %1 et %2 DT.")
                                    .arg(QString::number(prixMin, 'f', 0), QString::number(PRIX_MAX, 'f', 0)));
    if (std::fabs(prix * 100.0 - std::round(prix * 100.0)) > 1e-6)
        return echec(ChampPrix, QObject::tr("Le prix ne peut pas avoir plus de 2 chiffres après la virgule (ex. 30.50)."));

    if (prixSeulement)
        return true;

    // Durée
    if (duree < DUREE_MIN || duree > DUREE_MAX)
        return echec(ChampDuree, QObject::tr("La durée doit être un nombre entier de minutes compris entre %1 et %2.")
                                     .arg(DUREE_MIN).arg(DUREE_MAX));

    // Vérifications liées au CRUD, centralisées ici :
    // id == 0 : création ; id > 0 : modification.
    // Le mode prixSeulement (historique des services rendus) ne concerne pas
    // le CRUD de la table SERVICE.
    if (id > 0) {
        QSqlQuery verifierId;
        verifierId.prepare("SELECT COUNT(*) FROM SERVICE WHERE ID_SERVICE = :id");
        verifierId.bindValue(":id", id);

        if (!verifierId.exec() || !verifierId.next())
            return echec(ChampAucun,
                         QObject::tr("Impossible de vérifier l'existence du service dans la base de données."));

        if (verifierId.value(0).toInt() == 0)
            return echec(ChampAucun,
                         QObject::tr("Le service à modifier n'existe plus dans la base de données."));
    }

    // Empêche deux services d'avoir le même nom (sans tenir compte des majuscules
    // et des espaces au début/à la fin). En modification, le service courant est exclu.
    QSqlQuery verifierNom;
    if (id > 0) {
        verifierNom.prepare(
            "SELECT COUNT(*) FROM SERVICE "
            "WHERE UPPER(TRIM(NOM_SERVICE)) = UPPER(:nom) AND ID_SERVICE <> :id");
        verifierNom.bindValue(":id", id);
    } else {
        verifierNom.prepare(
            "SELECT COUNT(*) FROM SERVICE WHERE UPPER(TRIM(NOM_SERVICE)) = UPPER(:nom)");
    }
    verifierNom.bindValue(":nom", nom.trimmed());

    if (!verifierNom.exec() || !verifierNom.next())
        return echec(ChampAucun,
                     QObject::tr("Impossible de vérifier si le nom du service est déjà utilisé."));

    if (verifierNom.value(0).toInt() > 0)
        return echec(ChampNom,
                     QObject::tr("Un autre service possède déjà ce nom. Choisissez un nom différent."));

    return true;
}

// ===========================================================================
// 2. CRUD : ajouter (Create) / afficher (Read) / modifier (Update) / supprimer (Delete)
// ===========================================================================

// ---------------------------------------------------------------------------
// AJOUT (Create) : contrôle de saisie + requête préparée + variables liées
// L'ID_SERVICE est attribué ici, dans le code (prochaine valeur de la séquence
// SEQ_SERVICE) : plus besoin du trigger TRG_SERVICE_ID.
// ---------------------------------------------------------------------------
bool Service::ajouter()
{
    // Contrôle de saisie avant toute requête
    QString erreur;
    if (!controleSaisie(erreur)) {
        qDebug() << "Saisie invalide (ajout) :" << erreur;
        return false;
    }

    // Attribution de l'identifiant par le code : on lit le prochain numéro de la séquence.
    QSqlQuery seq;
    if (!seq.exec("SELECT SEQ_SERVICE.NEXTVAL FROM DUAL") || !seq.next()) {
        qDebug() << "SEQ_SERVICE :" << seq.lastError().text();
        return false;
    }
    id = seq.value(0).toInt();

    QSqlQuery query;

    // prepare() prend la requête en paramètre pour la préparer à l'exécution.
    query.prepare("INSERT INTO SERVICE (ID_SERVICE, NOM_SERVICE, DESCRIPTION_SERVICE, PRIX_SERVICE, "
                  "DUREE_SERVICE, TYPE_SERVICE, DISPONIBILITE) "
                  "VALUES (:id, :nom, :desc, :prix, :duree, :type, :dispo)");

    // Création des variables liées
    query.bindValue(":id",    id);
    query.bindValue(":nom",   nom.trimmed());
    query.bindValue(":desc",  description.trimmed());
    query.bindValue(":prix",  prix);
    query.bindValue(":duree", duree);
    query.bindValue(":type",  type);
    query.bindValue(":dispo", disponible ? 1 : 0);

    // exec() envoie la requête pour l'exécuter
    if (!query.exec()) {
        qDebug() << "INSERT SERVICE :" << query.lastError().text();
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// AFFICHAGE (Read) : modèle Qt (QSqlQueryModel) renvoyé à la vue.
// Les paramètres sont facultatifs : afficher() sans argument renvoie toute la table ;
// avec recherche / type / tri, Oracle exécute le WHERE ... LIKE et le ORDER BY
// (voir « 3. RECHERCHES »).
// Colonnes du modèle : 0 ID, 1 Nom, 2 Type, 3 Prix, 4 Durée, 5 Disponible.
// Remarque : l'appelant est propriétaire du modèle retourné (il doit le supprimer).
// ---------------------------------------------------------------------------
QSqlQueryModel * Service::afficher(const QString &recherche, const QString &type, int tri, bool decroissant)
{
    QSqlQueryModel * model = new QSqlQueryModel();

    QSqlQuery query;
    preparerRechercheService(query, "ID_SERVICE, NOM_SERVICE, TYPE_SERVICE, PRIX_SERVICE, "
                                    "DUREE_SERVICE, DISPONIBILITE", recherche, type, tri, decroissant);
    if (!query.exec())
        qDebug() << "AFFICHER SERVICE :" << query.lastError().text();
    else
        model->setQuery(std::move(query));

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom du service"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Type"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Prix (DT)"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Durée (min)"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Disponible"));

    return model;
}

// ---------------------------------------------------------------------------
// MODIFICATION (Update) : contrôle de saisie + requête préparée
// ---------------------------------------------------------------------------
bool Service::modifier()
{
    // Contrôle de saisie avant toute requête
    QString erreur;
    if (!controleSaisie(erreur)) {
        qDebug() << "Saisie invalide (modification) :" << erreur;
        return false;
    }

    QSqlQuery query;

    query.prepare("UPDATE SERVICE SET NOM_SERVICE=:nom, DESCRIPTION_SERVICE=:desc, "
                  "PRIX_SERVICE=:prix, DUREE_SERVICE=:duree, TYPE_SERVICE=:type, "
                  "DISPONIBILITE=:dispo WHERE ID_SERVICE=:id");

    query.bindValue(":nom",   nom.trimmed());
    query.bindValue(":desc",  description.trimmed());
    query.bindValue(":prix",  prix);
    query.bindValue(":duree", duree);
    query.bindValue(":type",  type);
    query.bindValue(":dispo", disponible ? 1 : 0);
    query.bindValue(":id",    id);

    if (!query.exec()) {
        qDebug() << "UPDATE SERVICE :" << query.lastError().text();
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// SUPPRESSION (Delete)
// ---------------------------------------------------------------------------
bool Service::supprimer(int id)
{
    // Vérifier que l'identifiant est valide.
    if (id <= 0) {
        qDebug() << "DELETE SERVICE : identifiant invalide.";
        return false;
    }

    // Vérifier que le service existe avant de le supprimer.
    QSqlQuery verifier;
    verifier.prepare("SELECT COUNT(*) FROM SERVICE WHERE ID_SERVICE = :id");
    verifier.bindValue(":id", id);

    if (!verifier.exec() || !verifier.next()) {
        qDebug() << "VERIFICATION SERVICE AVANT DELETE :" << verifier.lastError().text();
        return false;
    }

    if (verifier.value(0).toInt() == 0) {
        qDebug() << "DELETE SERVICE : aucun service ne correspond à cet identifiant.";
        return false;
    }

    QSqlQuery query;
    query.prepare("DELETE FROM SERVICE WHERE ID_SERVICE = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "DELETE SERVICE :" << query.lastError().text();
        return false;
    }

    // Confirmer qu'une seule ligne a été supprimée.
    if (query.numRowsAffected() != 1) {
        qDebug() << "DELETE SERVICE : le nombre de lignes supprimées est inattendu."
                 << query.numRowsAffected();
        return false;
    }

    return true;
}

// ===========================================================================
// 3. RECHERCHES : lister() et rechercher() (recherche + filtre + tri)
// ===========================================================================

// Read (version « liste d'objets ») : utilisée par la vue (QTableWidget)
QVector<Service> Service::lister()
{
    QVector<Service> liste;
    QSqlQuery query;

    query.prepare("SELECT ID_SERVICE, NOM_SERVICE, DESCRIPTION_SERVICE, PRIX_SERVICE, "
                  "DUREE_SERVICE, TYPE_SERVICE, DISPONIBILITE FROM SERVICE ORDER BY ID_SERVICE");

    if (!query.exec()) {
        qDebug() << "SELECT SERVICE :" << query.lastError().text();
        return liste;
    }

    while (query.next()) {
        Service s(query.value(0).toInt(),        // id
                  query.value(1).toString(),     // nom
                  query.value(2).toString(),     // description
                  query.value(5).toString(),     // type
                  query.value(3).toDouble(),     // prix
                  query.value(4).toInt(),        // durée
                  query.value(6).toInt() == 1);  // disponibilité
        liste.append(s);
    }
    return liste;
}

// ---------------------------------------------------------------------------
// RECHERCHE + FILTRE + TRI (Read avec WHERE / ORDER BY) : tout est fait par Oracle.
//  - La recherche porte sur le NOM seulement et vaut « commence par » : taper « d »
//    ne renvoie que les noms qui commencent par d (pas ceux dont la description ou le
//    type contiennent un d). Insensible à la casse (UPPER ... LIKE UPPER).
//  - Les valeurs saisies sont passées par bindValue (pas de concaténation => pas
//    d'injection SQL). Les caractères % et _ tapés par l'utilisateur sont échappés.
//  - ORDER BY ne peut pas être une variable liée : on choisit la clause dans une
//    liste fixe (« liste blanche ») selon le numéro de tri.
// ---------------------------------------------------------------------------
QVector<Service> Service::rechercher(const QString &recherche, const QString &type, int tri, bool decroissant)
{
    QVector<Service> liste;
    QSqlQuery query;

    preparerRechercheService(query, "ID_SERVICE, NOM_SERVICE, DESCRIPTION_SERVICE, PRIX_SERVICE, "
                                    "DUREE_SERVICE, TYPE_SERVICE, DISPONIBILITE", recherche, type, tri, decroissant);

    if (!query.exec()) {
        qDebug() << "RECHERCHE SERVICE :" << query.lastError().text();
        return liste;
    }

    while (query.next()) {
        liste.append(Service(query.value(0).toInt(),        // id
                             query.value(1).toString(),     // nom
                             query.value(2).toString(),     // description
                             query.value(5).toString(),     // type
                             query.value(3).toDouble(),     // prix
                             query.value(4).toInt(),        // durée
                             query.value(6).toInt() == 1)); // disponibilité
    }
    return liste;
}

// ===========================================================================
// 4. AUTRES
//   4.1 STATISTIQUES des services
// ===========================================================================

// ---------------------------------------------------------------------------
// STATISTIQUES : agrégats calculés par Oracle (COUNT, SUM, AVG, GROUP BY)
// ---------------------------------------------------------------------------
QVariantMap Service::statistiques()
{
    QVariantMap st;
    QMap<QString, int> parType;
    QVector<QPair<QString, double>> tranchesPrix, dureeMoyenneParType;
    QSqlQuery query;

    // 1) Nombre de services et durée moyenne par type
    if (!query.exec("SELECT TYPE_SERVICE, COUNT(*), AVG(DUREE_SERVICE) "
                    "FROM SERVICE GROUP BY TYPE_SERVICE ORDER BY TYPE_SERVICE")) {
        qDebug() << "STATS SERVICE (par type) :" << query.lastError().text();
        return st;
    }
    while (query.next()) {
        QString type = query.value(0).toString().trimmed();
        if (type.isEmpty())
            type = QObject::tr("Non défini");
        parType[type] += query.value(1).toInt();
        dureeMoyenneParType.append(
            qMakePair(type, qRound(query.value(2).toDouble() * 10) / 10.0));
    }

    // 2) Nombre de services par tranche de prix (borne min incluse, borne max exclue)
    if (!query.exec("SELECT "
                    "NVL(SUM(CASE WHEN PRIX_SERVICE < 25 THEN 1 ELSE 0 END), 0), "
                    "NVL(SUM(CASE WHEN PRIX_SERVICE >= 25  AND PRIX_SERVICE < 50  THEN 1 ELSE 0 END), 0), "
                    "NVL(SUM(CASE WHEN PRIX_SERVICE >= 50  AND PRIX_SERVICE < 100 THEN 1 ELSE 0 END), 0), "
                    "NVL(SUM(CASE WHEN PRIX_SERVICE >= 100 AND PRIX_SERVICE < 200 THEN 1 ELSE 0 END), 0), "
                    "NVL(SUM(CASE WHEN PRIX_SERVICE >= 200 THEN 1 ELSE 0 END), 0) "
                    "FROM SERVICE")) {
        qDebug() << "STATS SERVICE (tranches de prix) :" << query.lastError().text();
        return st;
    }
    if (query.next()) {
        tranchesPrix.append(qMakePair(QString("0 - 25"),    query.value(0).toDouble()));
        tranchesPrix.append(qMakePair(QString("25 - 50"),   query.value(1).toDouble()));
        tranchesPrix.append(qMakePair(QString("50 - 100"),  query.value(2).toDouble()));
        tranchesPrix.append(qMakePair(QString("100 - 200"), query.value(3).toDouble()));
        tranchesPrix.append(qMakePair(QString("200 et +"),  query.value(4).toDouble()));
    }

    st.insert("ok", true);
    st.insert("parType", QVariant::fromValue(parType));
    st.insert("tranchesPrix", QVariant::fromValue(tranchesPrix));
    st.insert("dureeMoyenneParType", QVariant::fromValue(dureeMoyenneParType));
    return st;
}

// ===========================================================================
//   4.2 HISTORIQUE DES SERVICES RENDUS (jointures ANIMAL / RENDEZ_VOUS / INCLURE / SERVICE)
// ===========================================================================

// ----- 4.2.1 Recherche d'animaux : voir Animal::rechercherParTexte() (animal.cpp) -----

// ----- 4.2.2 Affichage de l'historique d'un animal (filtre + tri en SQL) -----
// Services rendus à un animal (jointure INCLURE / RENDEZ_VOUS / SERVICE) : filtre + tri en SQL
QVector<Service> Service::historiqueAnimal(int idAnimal, const QString &type, int tri)
{
    QVector<Service> liste;
    QSqlQuery query;

    const bool avecType = !type.trimmed().isEmpty();

    QString sql = "SELECT i.ID_RDV, i.ID_SERVICE, s.NOM_SERVICE, s.TYPE_SERVICE, "
                  "i.PRIX_APPLIQUE, r.DATE_RDV, i.OBSERVATION "
                  "FROM INCLURE i "
                  "JOIN RENDEZ_VOUS r ON r.ID_RDV = i.ID_RDV "
                  "JOIN SERVICE s ON s.ID_SERVICE = i.ID_SERVICE "
                  "WHERE r.ID_ANIMAL = :animal ";
    if (avecType)
        sql += "AND s.TYPE_SERVICE = :type ";

    // ORDER BY ne peut pas être une variable liée : liste blanche
    switch (tri) {
    case 1:  sql += "ORDER BY r.DATE_RDV, r.HEURE_RDV, i.ID_RDV"; break;
    case 2:  sql += "ORDER BY UPPER(s.NOM_SERVICE), r.DATE_RDV DESC"; break;
    case 3:  sql += "ORDER BY UPPER(s.TYPE_SERVICE), UPPER(s.NOM_SERVICE), r.DATE_RDV DESC"; break;
    case 4:  sql += "ORDER BY i.PRIX_APPLIQUE, r.DATE_RDV DESC"; break;
    default: sql += "ORDER BY r.DATE_RDV DESC, r.HEURE_RDV DESC, i.ID_RDV DESC"; break;
    }

    query.prepare(sql);
    query.bindValue(":animal", idAnimal);
    if (avecType)
        query.bindValue(":type", type.trimmed());

    if (!query.exec()) {
        qDebug() << "HISTORIQUE ANIMAL :" << query.lastError().text();
        return liste;
    }
    while (query.next()) {
        Service r;
        RendezVous rv;
        rv.setIdRdv(query.value(0).toInt());
        rv.setDateRdv(query.value(5).toDate());
        r.rdv = rv;
        r.id = query.value(1).toInt();                 // id du service
        r.nom = query.value(2).toString();             // nom du service
        r.type = query.value(3).toString();            // type du service
        r.prixApplique = query.value(4).toDouble();
        r.remarque = query.value(6).toString();
        if (r.remarque == "-")           // valeur de remplacement (voir observationPourBase)
            r.remarque.clear();
        liste.append(r);
    }
    return liste;
}

// Rendez-vous d'un animal : voir RendezVous::rendezVousAnimal() (RendezVous.cpp)

// ----- 4.2.3 Statistiques d'un animal : voir Animal::statistiquesServices() (animal.cpp) -----

// ----- 4.2.4 CRUD des services rendus (table INCLURE) -----
// Create : rattache un service à un rendez-vous existant
bool Service::ajouterRendu() const
{
    if (rdv.getIdRdv() <= 0 || id <= 0 || prixApplique < 0 || remarque.trimmed().length() > 500) {
        qDebug() << "Saisie invalide (ajout service rendu)";
        return false;
    }

    QSqlQuery query;
    query.prepare("INSERT INTO INCLURE (ID_RDV, ID_SERVICE, PRIX_APPLIQUE, OBSERVATION) "
                  "VALUES (:rdv, :service, :prix, :obs)");
    query.bindValue(":rdv", rdv.getIdRdv());
    query.bindValue(":service", id);
    query.bindValue(":prix", prixApplique);
    query.bindValue(":obs", observationPourBase(remarque));

    if (!query.exec()) {
        qDebug() << "INSERT INCLURE :" << query.lastError().text();
        return false;
    }
    return true;
}

// Update : la clé de INCLURE est (ID_RDV, ID_SERVICE) ; on retrouve la ligne par l'ancien service
bool Service::modifierRendu(int ancienIdService) const
{
    if (rdv.getIdRdv() <= 0 || id <= 0 || prixApplique < 0 || remarque.trimmed().length() > 500) {
        qDebug() << "Saisie invalide (modification service rendu)";
        return false;
    }

    QSqlQuery query;
    query.prepare("UPDATE INCLURE SET ID_SERVICE = :service, PRIX_APPLIQUE = :prix, "
                  "OBSERVATION = :obs WHERE ID_RDV = :rdv AND ID_SERVICE = :ancien");
    query.bindValue(":service", id);
    query.bindValue(":prix", prixApplique);
    query.bindValue(":obs", observationPourBase(remarque));
    query.bindValue(":rdv", rdv.getIdRdv());
    query.bindValue(":ancien", ancienIdService);

    if (!query.exec()) {
        qDebug() << "UPDATE INCLURE :" << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

// Delete : retire le service du rendez-vous (le rendez-vous lui-même reste)
bool Service::supprimerRendu(int idRdv, int idService)
{
    QSqlQuery query;
    query.prepare("DELETE FROM INCLURE WHERE ID_RDV = :rdv AND ID_SERVICE = :service");
    query.bindValue(":rdv", idRdv);
    query.bindValue(":service", idService);

    if (!query.exec()) {
        qDebug() << "DELETE INCLURE :" << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}


// ===========================================================================
// RAPPEL AUTOMATIQUE PAR SMS (TextBee) : méthodes statiques de la classe Service
//
// Principe : toutes les X minutes, l'application cherche dans Oracle les rendez-vous
// non annulés qui commencent dans moins de "rappel_avant_heures" heures, puis envoie
// un SMS au propriétaire de l'animal via l'API TextBee (téléphone Android + SIM).
// Chaque RDV n'est notifié qu'une seule fois (rappels_sms.ini). Journal : rappels_sms.log.
//
// Configuration : variables d'environnement TEXTBEE_API_KEY / TEXTBEE_DEVICE_ID
// OU fichier textbee.ini (cherché dans le dossier de données, puis à côté de l'exe et
// dans ses dossiers parents). Dossier de données :
//   C:/Users/<vous>/AppData/Roaming/PetNova/GestionEmployes
//
// Démarrage : une seule ligne dans main() -> Service::demarrerRappels(&a);
// ===========================================================================

// Membres statiques du rappel SMS (définitions)
QTimer *Service::s_timer = nullptr;
QNetworkAccessManager *Service::s_manager = nullptr;
QSettings *Service::s_settings = nullptr;
QSettings *Service::s_config = nullptr;
QString Service::s_dossier;
QString Service::s_cheminConfig;
QString Service::s_apiKey;
QString Service::s_deviceId;
int  Service::s_avantHeures = 2;
int  Service::s_intervalleMin = 5;
bool Service::s_simulation = false;
QSet<QString> Service::s_enCours;
QSet<QString> Service::s_simules;

void Service::demarrerRappels(QObject *parent)
{
    if (s_timer)
        return;                       // déjà démarré : un seul rappel pour toute l'application

    s_timer = new QTimer(parent);
    s_manager = new QNetworkAccessManager(parent);
    s_dossier = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(s_dossier);
    s_settings = new QSettings(s_dossier + "/rappels_sms.ini", QSettings::IniFormat, parent);
    s_cheminConfig = trouverConfiguration();
    s_config   = new QSettings(s_cheminConfig, QSettings::IniFormat, parent);

    creerModeleConfiguration();
    lireConfiguration();

    if (s_apiKey.isEmpty() || s_deviceId.isEmpty()) {
        journaliser(QStringLiteral("Rappels SMS DÉSACTIVÉS : api_key et/ou device_id manquants. "
                                   "Renseignez-les dans %1").arg(s_cheminConfig));
        return;
    }
    if (!QSslSocket::supportsSsl())
        journaliser(QStringLiteral("ATTENTION : SSL indisponible, l'envoi HTTPS vers TextBee échouera "
                                   "(Qt5 : copier libssl/libcrypto OpenSSL à côté de l'exécutable)."));

    journaliser(QStringLiteral("Configuration lue : %1").arg(s_cheminConfig));
    journaliser(QStringLiteral("Rappels SMS actifs (rappel %1 h avant le RDV, vérification toutes les %2 min%3).")
                .arg(s_avantHeures).arg(s_intervalleMin)
                .arg(s_simulation ? ", MODE SIMULATION" : ""));

    QObject::connect(s_timer, &QTimer::timeout, s_timer, []() { verifierRendezVous(); });
    s_timer->start(s_intervalleMin * 60 * 1000);
    verifierRendezVous(); // première vérification au démarrage
}

void Service::journaliser(const QString &texte)
{
    qInfo().noquote() << texte;
    QFile f(s_dossier + "/rappels_sms.log");
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        const QString ligne = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")
                              + "  " + texte + "\n";
        f.write(ligne.toUtf8());
    }
}

// Cherche textbee.ini : dossier de données, dossier de l'exe, puis dossiers parents
// (permet de le placer à la racine du projet même avec un build séparé de Qt Creator).
QString Service::trouverConfiguration()
{
    QStringList candidats;
    candidats << s_dossier + "/textbee.ini";
    QDir d(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 5; ++i) {
        candidats << d.filePath("textbee.ini");
        if (!d.cdUp()) break;
    }
    candidats << QDir::current().filePath("textbee.ini");
    for (const QString &c : candidats)
        if (QFile::exists(c))
            return QDir::cleanPath(c);
    return s_dossier + "/textbee.ini";
}

void Service::creerModeleConfiguration()
{
    if (QFile::exists(s_cheminConfig))
        return;
    s_config->setValue("textbee/api_key", "");
    s_config->setValue("textbee/device_id", "");
    s_config->setValue("textbee/rappel_avant_heures", 2);
    s_config->setValue("textbee/intervalle_minutes", 5);
    s_config->setValue("textbee/simulation", false);
    s_config->sync();
}

void Service::lireConfiguration()
{
    s_config->sync();
    s_apiKey = qEnvironmentVariable("TEXTBEE_API_KEY").trimmed();
    if (s_apiKey.isEmpty())
        s_apiKey = s_config->value("textbee/api_key").toString().trimmed();
    s_deviceId = qEnvironmentVariable("TEXTBEE_DEVICE_ID").trimmed();
    if (s_deviceId.isEmpty())
        s_deviceId = s_config->value("textbee/device_id").toString().trimmed();

    if (s_apiKey.startsWith("COLLER_ICI"))   s_apiKey.clear();   // valeurs d'exemple non remplies
    if (s_deviceId.startsWith("COLLER_ICI")) s_deviceId.clear();

    s_avantHeures    = qBound(1, s_config->value("textbee/rappel_avant_heures", 2).toInt(), 168);
    s_intervalleMin  = qBound(1, s_config->value("textbee/intervalle_minutes", 5).toInt(), 60);
    s_simulation     = s_config->value("textbee/simulation", false).toBool();
}

// Format international E.164 (numéros tunisiens à 8 chiffres complétés avec +216).
QString Service::normaliserTelephone(QString telephone)
{
    const bool plus = telephone.trimmed().startsWith('+');
    telephone.remove(QRegularExpression("[^0-9]"));
    if (!plus && telephone.startsWith("00"))
        telephone.remove(0, 2);
    if (telephone.size() == 8)
        telephone.prepend("216");
    return telephone.isEmpty() ? QString() : "+" + telephone;
}

bool Service::telephoneValide(const QString &tel)
{
    static const QRegularExpression re("^\\+[1-9][0-9]{9,14}$");
    return re.match(tel).hasMatch();
}

void Service::verifierRendezVous()
{
    // Début du RDV = DATE_RDV (jour) + heure de HEURE_RDV.
    // Rappel : RDV pas encore commencé et qui débute dans moins de s_avantHeures heures.
    const QString debut = QStringLiteral(
        "(TRUNC(r.DATE_RDV) + (r.HEURE_RDV - TRUNC(r.HEURE_RDV)))");
    const QString sql = QStringLiteral(
        "SELECT r.ID_RDV, r.DATE_RDV, r.TYPE_RDV, a.NOM_ANIMAL, a.TEL_PROPRIETAIRE, %1 AS DEBUT_RDV, "
        "(SELECT LISTAGG(sv.NOM_SERVICE, ', ') WITHIN GROUP (ORDER BY sv.NOM_SERVICE) "
        " FROM INCLURE i JOIN SERVICE sv ON sv.ID_SERVICE = i.ID_SERVICE "
        " WHERE i.ID_RDV = r.ID_RDV) AS SERVICES, a.PROPRIETAIRE "
        "FROM RENDEZ_VOUS r "
        "JOIN ANIMAL a ON a.ID_ANIMAL = r.ID_ANIMAL "
        "WHERE UPPER(NVL(r.STATUT_RDV, ' ')) NOT LIKE 'ANNUL%' "
        "AND r.DATE_RDV IS NOT NULL AND r.HEURE_RDV IS NOT NULL "
        "AND %1 > SYSDATE "
        "AND %1 <= SYSDATE + %2 / 24")
        .arg(debut).arg(s_avantHeures);

    QSqlQuery query;
    if (!query.exec(sql)) {
        journaliser(QStringLiteral("Erreur SQL rappel SMS : %1").arg(query.lastError().text()));
        return;
    }

    int index = 0;
    while (query.next()) {
        const QString id   = query.value(0).toString();
        const QString date = query.value(1).toDate().toString(Qt::ISODate);
        const QString cle  = id + "_" + date + "_avant";
        if (s_settings->value(cle, false).toBool() || s_enCours.contains(cle) || s_simules.contains(cle))
            continue;

        const QString telephone = normaliserTelephone(query.value(4).toString());
        if (!telephoneValide(telephone)) {
            journaliser(QStringLiteral("RDV %1 : numéro invalide (%2), SMS ignoré.")
                        .arg(id, query.value(4).toString()));
            continue;
        }
        if (index >= MAX_SMS_PAR_CYCLE)
            break; // le reste sera traité au prochain cycle

        const QString type = query.value(2).toString().trimmed();
        const QString animal = query.value(3).toString().trimmed();
        const QDateTime debutRdv = query.value(5).toDateTime();
        const QString services = query.value(6).toString().trimmed();
        const QString proprietaire = query.value(7).toString().trimmed();

        QString message = proprietaire.isEmpty() ? QStringLiteral("Bonjour,")
                                                 : QStringLiteral("Bonjour %1,").arg(proprietaire);
        message += QStringLiteral("\nRappel de votre rendez-vous au Smart Pet Care Center :");
        message += QStringLiteral("\nAnimal : %1").arg(animal);
        if (!type.isEmpty())
            message += QStringLiteral("\nType : %1").arg(type);
        if (!services.isEmpty())
            message += QStringLiteral("\nService : %1").arg(services);
        message += QStringLiteral("\nDate : %1 à %2")
                       .arg(debutRdv.toString("dd/MM/yyyy"), debutRdv.toString("HH:mm"));
        message += QStringLiteral("\nMerci d'arriver à l'heure.");

        if (s_simulation) {
            s_simules.insert(cle);
            journaliser(QStringLiteral("[SIMULATION] SMS vers %1 : %2").arg(telephone, message));
            continue;
        }

        // Étale les envois (3 s d'écart) pour ne pas saturer le téléphone Android.
        s_enCours.insert(cle);
        QTimer::singleShot(index * 3000, s_timer, [telephone, message, cle]() {
            envoyerSms(telephone, message, cle);
        });
        ++index;
    }
}

void Service::envoyerSms(const QString &telephone, const QString &message, const QString &cle)
{
    // Endpoint officiel TextBee : POST /gateway/devices/{deviceId}/send-sms
    const QUrl url(QStringLiteral("https://api.textbee.dev/api/v1/gateway/devices/%1/send-sms")
                   .arg(QString::fromLatin1(QUrl::toPercentEncoding(s_deviceId))));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("x-api-key", s_apiKey.toUtf8());
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(30000);
#endif

    QJsonArray destinataires;
    destinataires.append(telephone);
    QJsonObject body;
    body.insert("recipients", destinataires);
    body.insert("message", message);

    s_enCours.insert(cle);
    QNetworkReply *reply = s_manager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    QObject::connect(reply, &QNetworkReply::finished, s_timer, [reply, cle, telephone]() {
        const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QString reponse = QString::fromUtf8(reply->readAll()).left(300);
        if (reply->error() == QNetworkReply::NoError && code >= 200 && code < 300) {
            s_settings->setValue(cle, true); // mémorisé : ne sera plus jamais renvoyé
            s_settings->sync();
            journaliser(QStringLiteral("SMS accepté par TextBee (%1) pour %2").arg(cle, telephone));
        } else {
            QString aide;
            if (code == 401 || code == 403) aide = " -> clé API invalide ou non autorisée";
            else if (code == 404)           aide = " -> device_id introuvable";
            journaliser(QStringLiteral("ÉCHEC TextBee (%1) HTTP %2 : %3%4 %5")
                        .arg(cle).arg(code).arg(reply->errorString(), aide, reponse));
        }
        s_enCours.remove(cle);
        reply->deleteLater();
    });
}
