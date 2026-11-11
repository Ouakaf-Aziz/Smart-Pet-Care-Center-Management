#include "animal.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

namespace {

const QString kColonnes = QStringLiteral(
    "ID_ANIMAL, NOM_ANIMAL, ESPECE, SEXE, DATE_NAISSANCE, PHOTO, PROPRIETAIRE, "
    "TEL_PROPRIETAIRE, EMAIL_PROPRIETAIRE, SUIVI_SANTE");

// Interface -> Oracle : CHAR(1)
QString sexeVersOracle(QString sexe)
{
    sexe = sexe.trimmed().toLower();
    if (sexe == "m" || sexe == "male" || sexe == QString::fromUtf8("mâle"))
        return "M";
    if (sexe == "f" || sexe == "femelle")
        return "F";
    return QString();
}

// Oracle -> interface : « M » -> « Mâle », « F » -> « Femelle »
QString sexeVersLibelle(const QString &code)
{
    const QString c = code.trimmed().toUpper();
    if (c == "M") return QString::fromUtf8("Mâle");
    if (c == "F") return "Femelle";
    return code;
}

void remplirErreur(QString *erreur, const QString &message)
{
    if (erreur)
        *erreur = message;
}

QString cheminDossiersMedicaux()
{
    const QString dossier = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(dossier).filePath(QStringLiteral("dossiers_medicaux.txt"));
}

} // namespace

// Construit un Animal à partir de la ligne courante d'une requête SELECT kColonnes.
Animal Animal::depuisRequete(const QSqlQuery &q)
{
    Animal a;
    a.id           = q.value(0).toInt();
    a.nom          = q.value(1).toString();
    a.espece       = q.value(2).toString();
    a.sexe         = sexeVersLibelle(q.value(3).toString());
    a.naissance    = q.value(4).toDate();
    a.photo        = q.value(5).toString();
    a.proprietaire = q.value(6).toString();
    a.telephone    = q.value(7).toString();
    a.email        = q.value(8).toString();
    a.suiviSante   = q.value(9).toString();
    return a;
}

// ---------------------------------------------------------------------------
// Prochain identifiant : MAX(ID_ANIMAL)+1 (démonstration mono-utilisateur ;
// préférer une séquence Oracle en production)
// ---------------------------------------------------------------------------
int Animal::prochainId()
{
    QSqlQuery q;
    if (q.exec("SELECT NVL(MAX(ID_ANIMAL), 0) + 1 FROM ANIMAL") && q.next())
        return q.value(0).toInt();
    return 1;
}

// ---------------------------------------------------------------------------
// AJOUTER
// ---------------------------------------------------------------------------
bool Animal::ajouter(QString *erreur)
{
    const QString sexeOracle = sexeVersOracle(sexe);
    if (sexeOracle.isEmpty()) {
        remplirErreur(erreur, "Sexe invalide : choisir Male ou Femelle.");
        return false;
    }
    if (nom.trimmed().isEmpty()) {
        remplirErreur(erreur, "Le nom de l'animal est obligatoire.");
        return false;
    }

    QSqlQuery idQuery;
    if (!idQuery.exec("SELECT NVL(MAX(ID_ANIMAL), 0) + 1 FROM ANIMAL") || !idQuery.next()) {
        remplirErreur(erreur, idQuery.lastError().text());
        return false;
    }
    const int nouvelId = idQuery.value(0).toInt();

    QSqlQuery query;
    query.prepare(
        "INSERT INTO ANIMAL "
        "(ID_ANIMAL, NOM_ANIMAL, ESPECE, SEXE, DATE_NAISSANCE, PHOTO, PROPRIETAIRE, "
        "TEL_PROPRIETAIRE, EMAIL_PROPRIETAIRE, SUIVI_SANTE) "
        "VALUES (:id, :nom, :espece, :sexe, :naissance, :photo, :proprietaire, "
        ":telephone, :email, :suivi)");
    query.bindValue(":id", nouvelId);
    query.bindValue(":nom", nom);
    query.bindValue(":espece", espece);
    query.bindValue(":sexe", sexeOracle);
    query.bindValue(":naissance", naissance);
    query.bindValue(":photo", photo);
    query.bindValue(":proprietaire", proprietaire);
    query.bindValue(":telephone", telephone);
    query.bindValue(":email", email);
    query.bindValue(":suivi", suiviSante);

    if (!query.exec()) {
        remplirErreur(erreur, query.lastError().text());
        qDebug() << "Erreur ajout animal:" << query.lastError().text();
        return false;
    }
    id = nouvelId;
    remplirErreur(erreur, QString());
    return true;
}

// ---------------------------------------------------------------------------
// MODIFIER
// ---------------------------------------------------------------------------
bool Animal::modifier(QString *erreur) const
{
    const QString sexeOracle = sexeVersOracle(sexe);
    if (sexeOracle.isEmpty()) {
        remplirErreur(erreur, "Sexe invalide.");
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "UPDATE ANIMAL SET NOM_ANIMAL = :nom, ESPECE = :espece, SEXE = :sexe, "
        "DATE_NAISSANCE = :naissance, PHOTO = :photo, PROPRIETAIRE = :proprietaire, "
        "TEL_PROPRIETAIRE = :telephone, EMAIL_PROPRIETAIRE = :email, SUIVI_SANTE = :suivi "
        "WHERE ID_ANIMAL = :id");
    query.bindValue(":id", id);
    query.bindValue(":nom", nom);
    query.bindValue(":espece", espece);
    query.bindValue(":sexe", sexeOracle);
    query.bindValue(":naissance", naissance);
    query.bindValue(":photo", photo);
    query.bindValue(":proprietaire", proprietaire);
    query.bindValue(":telephone", telephone);
    query.bindValue(":email", email);
    query.bindValue(":suivi", suiviSante);

    if (!query.exec()) {
        remplirErreur(erreur, query.lastError().text());
        qDebug() << "Erreur modification animal:" << query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        remplirErreur(erreur, "Aucun animal trouve avec cet ID.");
        return false;
    }
    remplirErreur(erreur, QString());
    return true;
}

// ---------------------------------------------------------------------------
// SUPPRIMER (peut échouer si un rendez-vous référence l'animal : contrainte FK)
// ---------------------------------------------------------------------------
bool Animal::supprimer(int id, QString *erreur)
{
    QSqlQuery query;
    query.prepare("DELETE FROM ANIMAL WHERE ID_ANIMAL = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        remplirErreur(erreur, query.lastError().text());
        qDebug() << "Erreur suppression animal:" << query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        remplirErreur(erreur, "Aucun animal trouve avec cet ID.");
        return false;
    }
    remplirErreur(erreur, QString());
    return true;
}

// ---------------------------------------------------------------------------
// AFFICHER : tous les animaux
// ---------------------------------------------------------------------------
bool Animal::afficher(QVector<Animal> &liste, QString *erreur)
{
    liste.clear();
    QSqlQuery query;
    if (!query.exec(QString("SELECT %1 FROM ANIMAL ORDER BY ID_ANIMAL").arg(kColonnes))) {
        remplirErreur(erreur, query.lastError().text());
        return false;
    }
    while (query.next())
        liste.append(depuisRequete(query));
    remplirErreur(erreur, QString());
    return true;
}

// ---------------------------------------------------------------------------
// RECHERCHER : recherche + filtre par espèce + tri, exécutés par Oracle
// tri : 0 = nom, 1 = espèce, 2 = date de naissance, 3 = propriétaire
// ---------------------------------------------------------------------------
bool Animal::rechercher(QVector<Animal> &liste, const QString &texte,
                        const QString &espece, int tri, QString *erreur)
{
    liste.clear();
    QString sql = QString("SELECT %1 FROM ANIMAL WHERE 1=1").arg(kColonnes);
    if (!espece.trimmed().isEmpty())
        sql += " AND UPPER(ESPECE) = UPPER(:espece)";
    if (!texte.trimmed().isEmpty())
        sql += " AND (UPPER(NOM_ANIMAL) LIKE :texte OR UPPER(ESPECE) LIKE :texte "
               "OR UPPER(PROPRIETAIRE) LIKE :texte OR UPPER(TEL_PROPRIETAIRE) LIKE :texte "
               "OR UPPER(SEXE) LIKE :texte OR TO_CHAR(ID_ANIMAL) LIKE :texte)";
    switch (tri) {
    case 1:  sql += " ORDER BY UPPER(ESPECE), UPPER(NOM_ANIMAL)"; break;
    case 2:  sql += " ORDER BY DATE_NAISSANCE ASC NULLS LAST"; break;
    case 3:  sql += " ORDER BY UPPER(PROPRIETAIRE), UPPER(NOM_ANIMAL)"; break;
    default: sql += " ORDER BY UPPER(NOM_ANIMAL)"; break;
    }

    QSqlQuery q;
    if (!q.prepare(sql)) {
        remplirErreur(erreur, q.lastError().text());
        return false;
    }
    if (!espece.trimmed().isEmpty())
        q.bindValue(":espece", espece.trimmed());
    if (!texte.trimmed().isEmpty())
        q.bindValue(":texte", "%" + texte.trimmed().toUpper() + "%");
    if (!q.exec()) {
        remplirErreur(erreur, q.lastError().text());
        return false;
    }
    while (q.next())
        liste.append(depuisRequete(q));
    remplirErreur(erreur, QString());
    return true;
}

// ---------------------------------------------------------------------------
// OBTENIR : un animal par son id
// ---------------------------------------------------------------------------
bool Animal::obtenir(int id, Animal &animal, QString *erreur)
{
    QSqlQuery q;
    q.prepare(QString("SELECT %1 FROM ANIMAL WHERE ID_ANIMAL = :id").arg(kColonnes));
    q.bindValue(":id", id);
    if (!q.exec()) {
        remplirErreur(erreur, q.lastError().text());
        return false;
    }
    if (!q.next()) {
        remplirErreur(erreur, "Animal introuvable dans Oracle.");
        return false;
    }
    animal = depuisRequete(q);
    remplirErreur(erreur, QString());
    return true;
}

// ---------------------------------------------------------------------------
// DOSSIER MÉDICAL : un seul fichier texte INI (dossiers_medicaux.txt) pour tous les animaux,
// dans le dossier de données de l'application. L'identité et le suivi santé restent dans Oracle.
// ---------------------------------------------------------------------------
bool Animal::obtenirMedical(int id, Animal &a, QString *erreur)
{
    if (!obtenir(id, a, erreur))
        return false;

    QSettings fichier(cheminDossiersMedicaux(), QSettings::IniFormat);
    fichier.beginGroup(QStringLiteral("ANIMAL_%1").arg(id));
    a.antecedentsMedicaux  = fichier.value("ANTECEDENTS_MEDICAUX").toString();
    a.maladiesChroniques   = fichier.value("MALADIES_CHRONIQUES").toString();
    a.allergies            = fichier.value("ALLERGIES").toString();
    a.traitementsEnCours   = fichier.value("TRAITEMENTS_EN_COURS").toString();
    a.poids                = fichier.value("POIDS", 0.0).toDouble();
    a.statutVaccinal       = fichier.value("STATUT_VACCINAL", "INCONNU").toString();
    a.derniereConsultation = QDate::fromString(fichier.value("DERNIERE_CONSULTATION").toString(), Qt::ISODate);
    fichier.endGroup();

    if (fichier.status() != QSettings::NoError) {
        remplirErreur(erreur, QStringLiteral("Impossible de lire %1").arg(cheminDossiersMedicaux()));
        return false;
    }
    remplirErreur(erreur, QString());
    return true;
}

bool Animal::enregistrerMedical(const Animal &a, QString *erreur)
{
    Animal existant;                       // ne pas écrire si l'animal n'existe plus dans Oracle
    if (!obtenir(a.id, existant, erreur))
        return false;

    const QString chemin = cheminDossiersMedicaux();
    if (!QDir().mkpath(QFileInfo(chemin).absolutePath())) {
        remplirErreur(erreur, QStringLiteral("Impossible de creer le dossier des donnees medicales."));
        return false;
    }

    QSettings fichier(chemin, QSettings::IniFormat);
    fichier.beginGroup(QStringLiteral("ANIMAL_%1").arg(a.id));
    fichier.setValue("ANTECEDENTS_MEDICAUX", a.antecedentsMedicaux);
    fichier.setValue("MALADIES_CHRONIQUES", a.maladiesChroniques);
    fichier.setValue("ALLERGIES", a.allergies);
    fichier.setValue("TRAITEMENTS_EN_COURS", a.traitementsEnCours);
    fichier.setValue("POIDS", a.poids);
    fichier.setValue("STATUT_VACCINAL", a.statutVaccinal);
    fichier.setValue("DERNIERE_CONSULTATION",
                     a.derniereConsultation.isValid() ? a.derniereConsultation.toString(Qt::ISODate) : QString());
    fichier.endGroup();
    fichier.sync();

    if (fichier.status() != QSettings::NoError) {
        remplirErreur(erreur, QStringLiteral("Echec de l'enregistrement dans %1").arg(chemin));
        return false;
    }
    remplirErreur(erreur, QString());
    return true;
}

// ---------------------------------------------------------------------------
// COMPTAGES : regroupements faits par Oracle (GROUP BY), utilisés par le chatbot
// ---------------------------------------------------------------------------
static bool executerComptage(const QString &sql, QVector<QPair<QString, int>> &brut, QString *erreur)
{
    brut.clear();
    QSqlQuery q;
    if (!q.exec(sql)) {
        remplirErreur(erreur, q.lastError().text());
        return false;
    }
    while (q.next())
        brut.append(qMakePair(q.value(0).toString(), q.value(1).toInt()));
    remplirErreur(erreur, QString());
    return true;
}

bool Animal::compter(int &total, QString *erreur)
{
    QSqlQuery q;
    if (!q.exec("SELECT COUNT(*) FROM ANIMAL") || !q.next()) {
        remplirErreur(erreur, q.lastError().text());
        return false;
    }
    total = q.value(0).toInt();
    remplirErreur(erreur, QString());
    return true;
}

bool Animal::repartitionParEspece(Comptage &resultat, QString *erreur)
{
    Comptage brut;
    if (!executerComptage("SELECT ESPECE, COUNT(*) FROM ANIMAL GROUP BY ESPECE "
                          "ORDER BY COUNT(*) DESC, ESPECE", brut, erreur))
        return false;
    resultat.clear();
    for (const auto &b : brut)
        resultat.append(qMakePair(b.first.trimmed().isEmpty() ? QString::fromUtf8("Non renseignée") : b.first,
                                  b.second));
    return true;
}
