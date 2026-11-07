#ifndef SERVICE_H
#define SERVICE_H

#include <QDate>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>
#include <QTimer>
#include <QMap>
#include <QPair>
#include <QSqlQueryModel>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariantMap>

// ---------------------------------------------------------------------------
class QSettings;

// MODELE : classe purement C++ liée à l'entité Service (table SERVICE).
// Toutes les requêtes SQL de l'entité sont ici (requêtes préparées + bindValue).
//
// IMPORTANT : cette classe est l'UNIQUE définition de « Service » dans le projet.
// Les VUES (GestionServices, ...) sont dans gsmartpetcare.h / gsmartpetcare.cpp.
class Service
{
public:
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

    // Champ fautif signalé par le contrôle de saisie (pour placer le curseur dans la vue)
    enum Champ { ChampAucun, ChampNom, ChampDescription, ChampType, ChampPrix, ChampDuree };

    // CONTRÔLE DE SAISIE : UNE SEULE fonction pour tous les champs (nom, description, type,
    // prix, durée). Renvoie false, remplit "erreur" (message prêt à afficher) et, si "champ"
    // est fourni, indique le champ fautif. Appelée par ajouter(), modifier() et par la vue.
    // Règles : nom 2-50 car. (commence par une lettre), description 5-255 car.,
    //          type obligatoire, prix 30-5000 DT (2 décimales max), durée 5-480 min.
    // prixSeulement = true : contrôle uniquement le prix, avec 0 permis (formulaire de l'historique).
    bool controleSaisie(QString &erreur, Champ *champ = nullptr, bool prixSeulement = false) const;

    // CRUD : Fonctionnalités de base relatives à l'entité Service
    bool ajouter();                      // Create
    // Read : modèle Qt renvoyé à la vue. Sans argument = toute la table ; avec recherche / type / tri,
    // le WHERE ... LIKE et le ORDER BY sont exécutés par Oracle (mêmes paramètres que rechercher()).
    // Colonnes : 0 ID, 1 Nom, 2 Type, 3 Prix, 4 Durée, 5 Disponible. L'appelant supprime le modèle.
    QSqlQueryModel * afficher(const QString &recherche = QString(),
                              const QString &type = QString(),
                              int tri = 0,
                              bool decroissant = false);
    bool modifier();                     // Update
    bool supprimer(int);                 // Delete

    // -----------------------------------------------------------------------
    // LECTURE EN LISTE D'OBJETS (toute la table) : utilisée par la vue (QTableWidget)
    // -----------------------------------------------------------------------
    static QVector<Service> lister();

    // -----------------------------------------------------------------------
    // RECHERCHE + FILTRE + TRI : exécutés par Oracle (WHERE ... LIKE, ORDER BY)
    //   recherche : début du NOM, insensible à la casse (vide = pas de recherche)
    //   type      : filtre exact sur TYPE_SERVICE (vide = tous les types)
    //   tri       : 0 = Prix, 1 = Durée (puis Nom)
    // -----------------------------------------------------------------------
    static QVector<Service> rechercher(const QString &recherche,
                                       const QString &type = QString(),
                                       int tri = 0,
                                       bool decroissant = false);

    // -----------------------------------------------------------------------
    // STATISTIQUES : calculées directement en SQL (COUNT, AVG, GROUP BY...)
    // sur la table SERVICE, comme les fonctions du CRUD.
    // -----------------------------------------------------------------------
    static QVariantMap statistiques();

    // -----------------------------------------------------------------------
    // HISTORIQUE DES SERVICES RENDUS
    // Tables : ANIMAL  <-  RENDEZ_VOUS  ->  INCLURE  ->  SERVICE
    // Un « service rendu » = une ligne de INCLURE (service inclus dans un
    // rendez-vous d'un animal) : prix appliqué + observation. La date est celle
    // du rendez-vous.
    //
    // Il n'y a plus de classes séparées (AnimalInfo, RendezVousInfo, ServiceRendu) :
    // leurs attributs sont directement dans Service (voir la section « private »).
    //  - objet « animal »       : idAnimal, nomAnimal, espece, proprietaire
    //  - objet « rendez-vous »  : idRdv, dateRdv, typeRdv
    //  - objet « service rendu »: id/nom/type du service + idRdv, dateRdv, prixApplique, remarque
    // PROVISOIRE (jusqu'à l'intégration des modules Animaux et Rendez-vous) :
    // les champs animal / rendez-vous sont en lecture seule : le module Service ne
    // les ajoute, ne les modifie et ne les supprime jamais.
    // -----------------------------------------------------------------------

    // Getters / setters des attributs « animal »
    int getIdAnimal() const { return idAnimal; }
    QString getNomAnimal() const { return nomAnimal; }
    QString getEspece() const { return espece; }
    QString getProprietaire() const { return proprietaire; }
    void setIdAnimal(int a) { idAnimal = a; }
    void setNomAnimal(QString n) { nomAnimal = n; }
    void setEspece(QString e) { espece = e; }
    void setProprietaire(QString p) { proprietaire = p; }

    // Getters / setters des attributs « rendez-vous » et « service rendu »
    int getIdRdv() const { return idRdv; }
    QDate getDateRdv() const { return dateRdv; }
    QString getTypeRdv() const { return typeRdv; }
    double getPrixApplique() const { return prixApplique; }
    QString getRemarque() const { return remarque; }
    void setIdRdv(int r) { idRdv = r; }
    void setDateRdv(QDate d) { dateRdv = d; }
    void setTypeRdv(QString t) { typeRdv = t; }
    void setPrixApplique(double p) { prixApplique = p; }
    void setRemarque(QString r) { remarque = r; }

    // Recherche d'animaux (nom, espèce ou propriétaire), triés par nom : faite par Oracle.
    // Chaque Service renvoyé ne porte que les champs « animal ».
    static QVector<Service> rechercherAnimaux(const QString &recherche);

    // Nombre, total facturé et date du dernier service (COUNT, SUM, MAX en SQL)
    static QVariantMap statistiquesAnimal(int idAnimal);

    // Rendez-vous d'un animal (pour rattacher un nouveau service rendu).
    // Chaque Service renvoyé ne porte que les champs « rendez-vous » (idRdv, dateRdv, typeRdv).
    static QVector<Service> rendezVousAnimal(int idAnimal);

    // Services rendus à un animal (jointure INCLURE / RENDEZ_VOUS / SERVICE) : filtre + tri en SQL
    static QVector<Service> historiqueAnimal(int idAnimal,
                                             const QString &type = QString(),
                                             int tri = 0);

    // CRUD d'un service rendu (table INCLURE) : l'objet courant porte id (service), idRdv,
    // prixApplique et remarque.
    bool ajouterRendu() const;                      // Create
    bool modifierRendu(int ancienIdService) const;  // Update
    static bool supprimerRendu(int idRdv, int idService); // Delete

    // -----------------------------------------------------------------------
    // RAPPEL AUTOMATIQUE PAR SMS (TextBee) : fonctionnalité commune à tout le projet.
    // Démarré une seule fois dans main() : Service::demarrerRappels(&a);
    // Toutes les X minutes : cherche les rendez-vous qui commencent bientôt (RENDEZ_VOUS,
    // ANIMAL, INCLURE, SERVICE) et envoie un SMS au propriétaire via l'API TextBee.
    // Configuration : textbee.ini (api_key, device_id, rappel_avant_heures, ...).
    // Les attributs du rappel (minuteur, clé API, ...) sont des membres statiques de Service.
    // -----------------------------------------------------------------------
    static void demarrerRappels(QObject *parent = nullptr);

private:
    // Attributs privés : accès uniquement par les getters / setters ci-dessus.
    // --- Service (table SERVICE) ---
    int id;
    QString nom, description, type;
    double prix;
    int duree;
    bool disponible;

    // --- « Animal » (table ANIMAL, lecture seule) ---
    int idAnimal = 0;
    QString nomAnimal, espece, proprietaire;

    // --- « Rendez-vous » (table RENDEZ_VOUS, lecture seule) + « service rendu » (table INCLURE) ---
    int idRdv = 0;
    QDate dateRdv;
    QString typeRdv;
    double prixApplique = 0.0;   // prix appliqué lors du rendez-vous (INCLURE.PRIX_APPLIQUE)
    QString remarque;            // observation (INCLURE.OBSERVATION)

    // --- Rappel SMS TextBee (un seul exemplaire pour toute l'application : membres statiques) ---
    static QTimer *s_timer;
    static QNetworkAccessManager *s_manager;
    static QSettings *s_settings;   // rendez-vous déjà notifiés (rappels_sms.ini)
    static QSettings *s_config;     // textbee.ini
    static QString s_dossier;
    static QString s_cheminConfig;
    static QString s_apiKey;
    static QString s_deviceId;
    static int  s_avantHeures;      // SMS envoyé X heures AVANT le début du rendez-vous
    static int  s_intervalleMin;    // fréquence de vérification (minutes)
    static bool s_simulation;       // true : écrit dans le journal sans envoyer de SMS
    static QSet<QString> s_enCours;
    static QSet<QString> s_simules;
    static constexpr int MAX_SMS_PAR_CYCLE = 10;

    static void journaliser(const QString &texte);
    static QString trouverConfiguration();
    static void creerModeleConfiguration();
    static void lireConfiguration();
    static QString normaliserTelephone(QString telephone);
    static bool telephoneValide(const QString &tel);
    static void verifierRendezVous();
    static void envoyerSms(const QString &telephone, const QString &message, const QString &cle);
};

#endif // SERVICE_H
