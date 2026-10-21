#ifndef SERVICE_H
#define SERVICE_H

#include <QString>
#include <QVector>
#include <QSqlQuery>
#include <QSqlQueryModel>

// MODELE : classe purement C++ liée à l'entité Service (table SERVICE).
// Toutes les requêtes SQL de l'entité sont ici (requêtes préparées + bindValue).
//
// IMPORTANT : cette classe est l'UNIQUE définition de « Service » dans le projet.
// Il ne doit plus y avoir de « struct Service » dans donnees.h (voir explications).
class Service
{
public:
    // Attributs publics : les autres fichiers (historiqueservices, statistiquesservices...)
    // qui utilisaient l'ancienne struct (s.nom, s.prix, ...) continuent de compiler.
    // Les getters/setters ci-dessous restent disponibles pour le reste du code.
    int id;
    QString nom, description, type;
    double prix;
    int duree;
    bool disponible;

    // Constructeurs
    Service();
    Service(int, QString, QString, QString, double, int, bool);

    // Getters (const : la vue les appelle sur des Service const)
    int getID() const { return id; }
    QString getNom() const { return nom; }
    QString getDescription() const { return description; }
    QString getType() const { return type; }
    double getPrix() const { return prix; }
    int getDuree() const { return duree; }
    bool isDisponible() const { return disponible; }

    // Setters
    void setID(int id) { this->id = id; }
    void setNom(QString n) { nom = n; }
    void setDescription(QString d) { description = d; }
    void setType(QString t) { type = t; }
    void setPrix(double p) { prix = p; }
    void setDuree(int d) { duree = d; }
    void setDisponible(bool d) { disponible = d; }

    // Fonctionnalités de Base relatives à l'entité Service (CRUD)
    bool ajouter();                      // Create
    QSqlQueryModel * afficher();         // Read (modèle pour un QTableView)
    bool modifier();                     // Update
    bool supprimer(int);                 // Delete

    // Read (version « liste d'objets ») : utilisée par la vue actuelle (QTableWidget)
    static QVector<Service> lister();
};

#endif // SERVICE_H
