#ifndef ANIMAL_H
#define ANIMAL_H

#include <QDate>
#include <QPair>
#include <QString>
#include <QVariantMap>
#include <QVector>

class QSqlQuery;

// Classe métier ANIMAL : contient les attributs d'un animal ET toutes les requêtes Oracle
// (ajouter, modifier, supprimer, afficher, rechercher). Aucune dépendance envers les widgets :
// l'interface est entièrement gérée par GSmartPetCare (gsmartpetcare.h / .cpp).
class Animal
{
public:
    // ----- Attributs (table ANIMAL) -----
    int     id = 0;
    QString nom;
    QString espece;
    QString sexe;                // « Mâle » ou « Femelle » (converti en M / F pour Oracle)
    QDate   naissance;
    QString photo;               // chemin de l'image (facultatif)
    QString proprietaire;
    QString telephone;
    QString email;
    QString suiviSante;          // « À jour » ou texte du suivi à faire

    // ----- Dossier médical (fichier texte local dossiers_medicaux.txt, pas Oracle) -----
    QString antecedentsMedicaux, maladiesChroniques, allergies, traitementsEnCours;
    double  poids = 0.0;
    QString statutVaccinal;      // INCONNU / A_JOUR / INCOMPLET
    QDate   derniereConsultation;

    Animal() = default;

    // ----- Requêtes Oracle -----
    bool ajouter(QString *erreur = nullptr);                 // INSERT : attribue aussi l'id
    bool modifier(QString *erreur = nullptr) const;          // UPDATE
    static bool supprimer(int id, QString *erreur = nullptr);// DELETE
    static bool afficher(QVector<Animal> &liste, QString *erreur = nullptr);   // SELECT *
    static bool rechercher(QVector<Animal> &liste, const QString &texte,
                           const QString &espece, int tri, QString *erreur = nullptr);
    static bool obtenir(int id, Animal &animal, QString *erreur = nullptr);    // SELECT par id
    static int  prochainId();                                // MAX(ID_ANIMAL)+1

    // ----- Recherche / statistiques pour l'historique des services rendus (module Service) -----
    // Recherche par nom, espèce ou propriétaire (contient, insensible à la casse), triée par nom.
    static QVector<Animal> rechercherParTexte(const QString &recherche);
    // Nombre, total facturé et date du dernier service d'un animal (COUNT, SUM, MAX en SQL)
    static QVariantMap statistiquesServices(int idAnimal);

    // ----- Comptages (chatbot) -----
    using Comptage = QVector<QPair<QString, int>>;           // (libellé, effectif)
    static bool compter(int &total, QString *erreur = nullptr);
    static bool repartitionParEspece(Comptage &resultat, QString *erreur = nullptr);

    // ----- Dossier médical (fichier texte local) -----
    static bool obtenirMedical(int id, Animal &animal, QString *erreur = nullptr);
    static bool enregistrerMedical(const Animal &animal, QString *erreur = nullptr);

private:
    static Animal depuisRequete(const QSqlQuery &q);
};

#endif // ANIMAL_H
