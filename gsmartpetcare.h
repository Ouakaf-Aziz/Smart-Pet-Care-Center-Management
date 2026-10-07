#ifndef GSMARTPETCARE_H
#define GSMARTPETCARE_H

#include <QMainWindow>

#include "donnees.h"

QT_BEGIN_NAMESPACE
namespace Ui { class GSmartPetCare; }
QT_END_NAMESPACE

class GestionServices;
class GestionRdv;
class GestionEmployes;
class GestionAnimaux;

class GSmartPetCare : public QMainWindow
{
    Q_OBJECT

public:
    explicit GSmartPetCare(QWidget *parent = nullptr);
    ~GSmartPetCare();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void on_pushButtonConnexion_clicked();

private:
    Ui::GSmartPetCare *ui;

    enum Page { PageLogin = 0, PageAccueil = 1 };

    enum Contenu {
        ContenuAccueil = 0,
        ContenuServices = 1,
        ContenuRdv = 2,
        ContenuEmployes = 3,
        ContenuAnimaux = 4
    };

    void afficherContenu(int index);
    void preparerCartesAccueil();
    void appliquerSession(const Employe *u);

    GestionServices *gestionServices = nullptr;
    GestionRdv *gestionRdv = nullptr;
    GestionEmployes *gestionEmployes = nullptr;
    GestionAnimaux *gestionAnimaux = nullptr;
    BaseEmployes m_baseEmployes;
};

#endif // GSMARTPETCARE_H
