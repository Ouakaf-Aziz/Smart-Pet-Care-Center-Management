#include "service.h"

#include <QSqlError>
#include <QObject>
#include <QDebug>

// ---------------------------------------------------------------------------
// Constructeurs
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

// ---------------------------------------------------------------------------
// AJOUT (Create) : requête préparée + variables liées
// L'ID_SERVICE est attribué par Oracle (séquence + trigger ou colonne IDENTITY).
// ---------------------------------------------------------------------------
bool Service::ajouter()
{
    QSqlQuery query;

    // prepare() prend la requête en paramètre pour la préparer à l'exécution.
    query.prepare("INSERT INTO SERVICE (NOM_SERVICE, DESCRIPTION_SERVICE, PRIX_SERVICE, "
                  "DUREE_SERVICE, TYPE_SERVICE, DISPONIBILITE) "
                  "VALUES (:nom, :desc, :prix, :duree, :type, :dispo)");

    // Création des variables liées
    query.bindValue(":nom",   nom);
    query.bindValue(":desc",  description);
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
// AFFICHAGE (Read) : modèle Qt à brancher avec ui->tableView->setModel(...)
// Remarque : l'appelant doit supprimer l'ancien modèle s'il en recrée un.
// ---------------------------------------------------------------------------
QSqlQueryModel * Service::afficher()
{
    QSqlQueryModel * model = new QSqlQueryModel();

    model->setQuery("SELECT ID_SERVICE, NOM_SERVICE, TYPE_SERVICE, PRIX_SERVICE, "
                    "DUREE_SERVICE, DISPONIBILITE FROM SERVICE ORDER BY ID_SERVICE");
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom du service"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Type"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Prix (DT)"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Durée (min)"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Disponible"));

    return model;
}

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
// MODIFICATION (Update)
// ---------------------------------------------------------------------------
bool Service::modifier()
{
    QSqlQuery query;

    query.prepare("UPDATE SERVICE SET NOM_SERVICE=:nom, DESCRIPTION_SERVICE=:desc, "
                  "PRIX_SERVICE=:prix, DUREE_SERVICE=:duree, TYPE_SERVICE=:type, "
                  "DISPONIBILITE=:dispo WHERE ID_SERVICE=:id");

    query.bindValue(":nom",   nom);
    query.bindValue(":desc",  description);
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
    QSqlQuery query;

    query.prepare("DELETE FROM SERVICE WHERE ID_SERVICE = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "DELETE SERVICE :" << query.lastError().text();
        return false;
    }
    return true;
}
