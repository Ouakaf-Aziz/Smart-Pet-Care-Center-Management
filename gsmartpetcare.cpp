#include "gsmartpetcare.h"
#include "ui_gsmartpetcare.h"
#include "gestionservices.h"
<<<<<<< Updated upstream
=======
#include "gestionrdv.h"
#include "connexion.h"
#include "facedialog.h"
#include "faceauth.h"
#include "gestionemployes.h"
#include "privileges.h"
#include "statistiquesemployes.h"
>>>>>>> Stashed changes

#include <QGraphicsDropShadowEffect>
#include <QIcon>
#include <QPushButton>
#include <QStyle>

GSmartPetCare::GSmartPetCare(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GSmartPetCare)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/images/icone.png"));
    resize(1366, 800);   // assez large pour la barre latérale + le tableau des services

    // Ombre légère sous la carte de connexion
    auto *ombre = new QGraphicsDropShadowEffect(this);
    ombre->setBlurRadius(30);
    ombre->setOffset(0, 4);
    ombre->setColor(QColor(0, 0, 0, 45));
    ui->frameCard->setGraphicsEffect(ombre);

    // La première page affichée est la page de connexion
    ui->stackedWidget->setCurrentIndex(PageLogin);
    ui->lineEditIdentifiant->setFocus();

    // Touche Entrée : identifiant -> mot de passe -> validation
    connect(ui->lineEditIdentifiant, &QLineEdit::returnPressed,
            ui->lineEditMotDePasse, qOverload<>(&QWidget::setFocus));
    connect(ui->lineEditMotDePasse, &QLineEdit::returnPressed,
            ui->pushButtonConnexion, &QPushButton::click);

    // Page « Gestion des services » : les widgets sont dans ce même .ui (pageServices),
    // GestionServices se charge de leur comportement.
    gestionServices = new GestionServices(ui, this, this);

<<<<<<< Updated upstream
=======
    // Page « Gestion des rendez-vous » : même principe (pageRDV dans ce même .ui).
    gestionRdv = new GestionRdv(ui, this, this);

    // Module « Gestion des employés » : même principe (pageEmployes, pageStatsEmployes, pagePrivileges
    // et pageVisage dans ce même .ui). Les employés servent aussi à l'authentification.
    chargerEmployesDemo(m_baseEmployes);
    FaceAuth::chargerGabarits(m_baseEmployes);      // visages enregistrés lors des sessions précédentes
    FaceDialog::preparerPage(ui);
    m_connexion     = new Connexion(ui, m_baseEmployes, this);
    m_employes      = new GestionEmployes(ui, m_baseEmployes, this);
    m_statsEmployes = new StatistiquesEmployes(ui, this);
    m_privileges    = new Privileges(ui, m_baseEmployes, this);

    connect(m_connexion, &Connexion::connecte, this, &GSmartPetCare::onConnecte);
    connect(m_connexion, &Connexion::abandon, this, &QWidget::close);      // 5 échecs : on ferme
    connect(m_employes, &GestionEmployes::statistiquesDemandees, this, &GSmartPetCare::afficherStatistiquesEmployes);
    connect(m_employes, &GestionEmployes::privilegesDemandes, this, &GSmartPetCare::afficherPrivileges);
    connect(m_statsEmployes, &StatistiquesEmployes::retour, this, [this]() { afficherContenu(ContenuEmployes); });
    connect(m_privileges, &Privileges::termine, this, [this]() { afficherContenu(ContenuEmployes); });

>>>>>>> Stashed changes
    // Navigation depuis le menu latéral (toujours visible : seule la zone de contenu change)
    afficherContenu(ContenuAccueil);
    connect(ui->btnNavAccueil, &QPushButton::clicked, this, [this]() {
        afficherContenu(ContenuAccueil);
    });
    connect(ui->btnNavServices, &QPushButton::clicked, this, [this]() {
        afficherContenu(ContenuServices);
    });
<<<<<<< Updated upstream
=======
    connect(ui->btnNavEmployes, &QPushButton::clicked, this, [this]() {
        afficherContenu(ContenuEmployes);
    });
    connect(ui->btnNavRDV, &QPushButton::clicked, this, [this]() {
        afficherContenu(ContenuRDV);
    });
>>>>>>> Stashed changes

    // Déconnexion : retour à la page de connexion
    connect(ui->btnDeconnexion, &QPushButton::clicked, this, [this]() {
        afficherContenu(ContenuAccueil);
        ui->stackedWidget->setCurrentIndex(PageLogin);
        ui->lineEditIdentifiant->clear();
        ui->labelErreurConnexion->hide();
        ui->lineEditIdentifiant->setFocus();
    });
}

void GSmartPetCare::afficherContenu(int index)
{
    ui->stackContenu->setCurrentIndex(index);

<<<<<<< Updated upstream
    // Le bouton actif est repéré par la propriété « actif » (voir la feuille de style du .ui)
    const struct { QPushButton *bouton; int page; } liens[] = {
        { ui->btnNavAccueil,  ContenuAccueil },
        { ui->btnNavServices, ContenuServices },
=======
    // Le bouton actif est repéré par la propriété « actif » (voir la feuille de style du .ui).
    // Les trois pages du module employés (liste, statistiques, privilèges) allument « Gestion des employés ».
    const bool pageEmployes = index == ContenuEmployes || index == ContenuStatsEmployes || index == ContenuPrivileges;
    const struct { QPushButton *bouton; bool actif; } liens[] = {
        { ui->btnNavAccueil,  index == ContenuAccueil },
        { ui->btnNavServices, index == ContenuServices },
        { ui->btnNavEmployes, pageEmployes },
        { ui->btnNavRDV,      index == ContenuRDV },
>>>>>>> Stashed changes
    };
    for (const auto &l : liens) {
        l.bouton->setProperty("actif", l.actif);
        l.bouton->style()->unpolish(l.bouton);
        l.bouton->style()->polish(l.bouton);
    }
}

GSmartPetCare::~GSmartPetCare()
{
    delete ui;
}

void GSmartPetCare::onConnecte(int employeId)
{
    m_employes->setUtilisateur(employeId);      // applique les privilèges de l'employé connecté
    if (const Employe *e = m_baseEmployes.trouver(employeId))
        ui->labelBienvenue->setText(QString("Bienvenue %1 %2  -  Rôle : %3").arg(e->prenom, e->nom, e->poste));
    afficherContenu(ContenuAccueil);
    ui->stackedWidget->setCurrentIndex(PageAccueil);
}

void GSmartPetCare::afficherStatistiquesEmployes(bool voirSalaires)
{
    m_statsEmployes->afficher(m_baseEmployes.employes, voirSalaires);
    afficherContenu(ContenuStatsEmployes);
}

void GSmartPetCare::afficherPrivileges()
{
    m_privileges->ouvrir();
    afficherContenu(ContenuPrivileges);
}
