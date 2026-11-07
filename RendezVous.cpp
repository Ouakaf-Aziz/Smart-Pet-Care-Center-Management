#include "RendezVous.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QMetaType>
#include <QDebug>

RendezVous::RendezVous()
{
    derniereErreur = "";
}

// NULL typé (entier) : évite l'erreur ODBC « type de paramètre invalide » avec un QVariant() vide
QVariant RendezVous::entierOuNull(const QVariant &valeur) const
{
    if (valeur.isValid() && !valeur.isNull() && valeur.toInt() > 0)
        return valeur.toInt();
    return QVariant(QMetaType::fromType<int>());
}

// ==================== AJOUTER ====================
bool RendezVous::ajouter(int id, const QString &priorite, int duree,
                         const QString &type, const QString &salle,
                         const QTime &heure, const QDate &date, const QVariant &idAnimal,
                         const QVariant &idEmploye)
{
    QSqlQuery query;

    // HEURE est un VARCHAR2(5) -> "HH:mm" (avec les secondes, Oracle refuse : ORA-12899)
    // La date est passée en texte ISO puis convertie par TO_DATE : indépendant des réglages NLS.
    query.prepare("INSERT INTO RENDEZ_VOUS "
                  "(ID_RDV, PRIORITE, DUREE, TYPE_RDV, SALLE, HEURE, DATE_RDV, ID_ANIMAL, ID_EMPLOYEE) "
                  "VALUES (:id, :priorite, :duree, :type, :salle, :heure, "
                  "TO_DATE(:date, 'YYYY-MM-DD'), :animal, :employe)");

    query.bindValue(":id", id);
    query.bindValue(":priorite", priorite);
    query.bindValue(":duree", duree);
    query.bindValue(":type", type);
    query.bindValue(":salle", salle);
    query.bindValue(":heure", heure.toString("HH:mm"));
    query.bindValue(":date", date.toString("yyyy-MM-dd"));
    query.bindValue(":animal", entierOuNull(idAnimal));
    query.bindValue(":employe", entierOuNull(idEmploye));

    if (query.exec())
        return true;

    derniereErreur = query.lastError().text();
    qDebug() << "INSERT RENDEZ_VOUS :" << derniereErreur;
    return false;
}

// ==================== AFFICHER ====================
QList<QStringList> RendezVous::afficher()
{
    QList<QStringList> liste;
    QSqlQuery query;

    if (!query.exec("SELECT r.ID_RDV, r.PRIORITE, r.DUREE, r.TYPE_RDV, r.SALLE, "
                    "r.HEURE, r.DATE_RDV, a.NOM, r.ID_ANIMAL, r.ID_EMPLOYEE "
                    "FROM RENDEZ_VOUS r LEFT JOIN ANIMAL a ON a.ID_ANIMAL = r.ID_ANIMAL "
                    "ORDER BY r.ID_RDV")) {
        derniereErreur = query.lastError().text();
        qDebug() << "SELECT RENDEZ_VOUS :" << derniereErreur;
        return liste;
    }

    while (query.next()) {
        // 0 ID_RDV, 1 PRIORITE, 2 DUREE, 3 TYPE_RDV, 4 SALLE, 5 HEURE, 6 DATE_RDV, 7 NOM, 8 ID_ANIMAL, 9 ID_EMPLOYEE
        QDate d = query.value(6).toDate();
        if (!d.isValid())
            d = query.value(6).toDateTime().date();

        QStringList ligne;
        ligne << query.value(0).toString()
              << query.value(1).toString()
              << query.value(2).toString()
              << query.value(3).toString()
              << query.value(4).toString()
              << query.value(5).toString()
              << d.toString("dd/MM/yyyy")
              << query.value(7).toString()
              << (query.value(8).isNull() ? QString() : query.value(8).toString())
              << (query.value(9).isNull() ? QString() : query.value(9).toString());
        liste.append(ligne);
    }
    return liste;
}

// ==================== MODIFIER ====================
bool RendezVous::modifier(int id, const QString &priorite, int duree,
                          const QString &type, const QString &salle,
                          const QTime &heure, const QDate &date, const QVariant &idAnimal,
                          const QVariant &idEmploye)
{
    QSqlQuery query;

    query.prepare("UPDATE RENDEZ_VOUS SET "
                  "PRIORITE = :priorite, DUREE = :duree, "
                  "TYPE_RDV = :type, SALLE = :salle, HEURE = :heure, "
                  "DATE_RDV = TO_DATE(:date, 'YYYY-MM-DD'), ID_ANIMAL = :animal, ID_EMPLOYEE = :employe "
                  "WHERE ID_RDV = :id");

    query.bindValue(":priorite", priorite);
    query.bindValue(":duree", duree);
    query.bindValue(":type", type);
    query.bindValue(":salle", salle);
    query.bindValue(":heure", heure.toString("HH:mm"));
    query.bindValue(":date", date.toString("yyyy-MM-dd"));
    query.bindValue(":animal", entierOuNull(idAnimal));
    query.bindValue(":employe", entierOuNull(idEmploye));
    query.bindValue(":id", id);

    if (!query.exec()) {
        derniereErreur = query.lastError().text();
        qDebug() << "UPDATE RENDEZ_VOUS :" << derniereErreur;
        return false;
    }
    if (query.numRowsAffected() == 0) {
        derniereErreur = "Rendez-vous introuvable.";
        return false;
    }
    return true;
}

// ==================== SUPPRIMER ====================
bool RendezVous::supprimer(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM RENDEZ_VOUS WHERE ID_RDV = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        derniereErreur = query.lastError().text();
        qDebug() << "DELETE RENDEZ_VOUS :" << derniereErreur;
        return false;
    }
    if (query.numRowsAffected() == 0) {
        derniereErreur = "Rendez-vous introuvable.";
        return false;
    }
    return true;
}

// ==================== OUTILS ====================
bool RendezVous::existe(int id)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM RENDEZ_VOUS WHERE ID_RDV = :id");
    query.bindValue(":id", id);
    if (query.exec() && query.next())
        return query.value(0).toInt() > 0;
    derniereErreur = query.lastError().text();
    return false;
}

bool RendezVous::animalExiste(int id)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM ANIMAL WHERE ID_ANIMAL = :id");
    query.bindValue(":id", id);
    if (query.exec() && query.next())
        return query.value(0).toInt() > 0;
    derniereErreur = query.lastError().text();
    return false;
}

bool RendezVous::employeExiste(int id)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM EMPLOYEE WHERE ID_EMPLOYEE = :id");
    query.bindValue(":id", id);
    if (query.exec() && query.next())
        return query.value(0).toInt() > 0;
    derniereErreur = query.lastError().text();
    return false;
}

int RendezVous::prochainId()
{
    QSqlQuery query;
    if (query.exec("SELECT NVL(MAX(ID_RDV), 0) + 1 FROM RENDEZ_VOUS") && query.next())
        return query.value(0).toInt();
    derniereErreur = query.lastError().text();
    return 1;
}

QList<QPair<int, QString>> RendezVous::listerAnimaux()
{
    QList<QPair<int, QString>> liste;
    QSqlQuery query;
    if (!query.exec("SELECT ID_ANIMAL, NOM FROM ANIMAL ORDER BY NOM")) {
        derniereErreur = query.lastError().text();
        qDebug() << "SELECT ANIMAL :" << derniereErreur;
        return liste;
    }
    while (query.next())
        liste.append({ query.value(0).toInt(), query.value(1).toString() });
    return liste;
}

QList<QPair<QString, int>> RendezVous::compterPar(const QString &colonne)
{
    QList<QPair<QString, int>> liste;

    // Le nom de colonne ne peut pas être un paramètre lié : liste blanche pour éviter toute injection SQL
    static const QStringList autorisees = { "TYPE_RDV", "PRIORITE", "SALLE" };
    if (!autorisees.contains(colonne)) {
        derniereErreur = "Colonne non autorisée : " + colonne;
        return liste;
    }

    QSqlQuery query;
    if (!query.exec("SELECT NVL(" + colonne + ", '(vide)'), COUNT(*) FROM RENDEZ_VOUS "
                                              "GROUP BY " + colonne + " ORDER BY COUNT(*) DESC")) {
        derniereErreur = query.lastError().text();
        qDebug() << "STATS RENDEZ_VOUS :" << derniereErreur;
        return liste;
    }
    while (query.next())
        liste.append({ query.value(0).toString(), query.value(1).toInt() });
    return liste;
}

QString RendezVous::getDerniereErreur() const
{
    return derniereErreur;
}