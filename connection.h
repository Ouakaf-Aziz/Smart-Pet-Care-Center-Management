#ifndef CONNECTION_H
#define CONNECTION_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>

class Connection
{
public:
    static Connection& createInstance();   // (1) point d'accès unique
    bool createConnection();               // ouvre la base

private:
    QSqlDatabase db;                       // la connexion, gardée dans le singleton

    Connection();                          // (2) constructeur privé
    ~Connection();                         // (3) destructeur privé

    Connection(const Connection&) = delete;             // (4) copie interdite
    Connection& operator=(const Connection&) = delete;  // (5) affectation interdite
};

#endif // CONNECTION_H