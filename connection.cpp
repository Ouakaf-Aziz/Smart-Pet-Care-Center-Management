#include "connection.h"

// Constructeur : prépare l'objet de connexion avec le driver ODBC.
// Fait une seule fois, car l'instance est unique.
Connection::Connection()
{
    db = QSqlDatabase::addDatabase("QODBC");
}

// Destructeur : ferme la connexion proprement à la fin du programme
Connection::~Connection()
{
    if (db.isOpen())
        db.close();
}

// Accès à l'instance unique (technique « Meyers Singleton »)
Connection& Connection::createInstance()
{
    static Connection instance;   // créée au 1er appel, puis réutilisée
    return instance;              // retourne une référence à cette instance
}

// Ouvre la connexion avec vos paramètres
bool Connection::createConnection()
{
    db.setDatabaseName("Source_Projet2A");   // ← nom de VOTRE source ODBC
    db.setUserName("monuser");               // ← votre utilisateur Oracle
    db.setPassword("Monpass2026");         // ← son mot de passe

    if (db.open()) {
        qDebug() << "Connexion établie";
        return true;
    }
    qDebug() << "Échec de la connexion :" << db.lastError().text();
    return false;
}