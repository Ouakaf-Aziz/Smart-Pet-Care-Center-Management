#include "gsmartpetcare.h"
#include "ui_gsmartpetcare.h"
#include "gestionservices.h"

#include <QGraphicsDropShadowEffect>
#include <QIcon>
#include <QMessageBox>
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

    // Navigation depuis le menu latéral (toujours visible : seule la zone de contenu change)
    afficherContenu(ContenuAccueil);
    connect(ui->btnNavAccueil, &QPushButton::clicked, this, [this]() {
        afficherContenu(ContenuAccueil);
    });
    connect(ui->btnNavServices, &QPushButton::clicked, this, [this]() {
        afficherContenu(ContenuServices);
    });

    // Déconnexion : retour à la page de connexion
    connect(ui->btnDeconnexion, &QPushButton::clicked, this, [this]() {
        afficherContenu(ContenuAccueil);
        ui->stackedWidget->setCurrentIndex(PageLogin);
        ui->lineEditIdentifiant->setFocus();
    });
}

void GSmartPetCare::afficherContenu(int index)
{
    ui->stackContenu->setCurrentIndex(index);

    // Le bouton actif est repéré par la propriété « actif » (voir la feuille de style du .ui)
    const struct { QPushButton *bouton; int page; } liens[] = {
        { ui->btnNavAccueil,  ContenuAccueil },
        { ui->btnNavServices, ContenuServices },
    };
    for (const auto &l : liens) {
        l.bouton->setProperty("actif", l.page == index);
        l.bouton->style()->unpolish(l.bouton);
        l.bouton->style()->polish(l.bouton);
    }
}

GSmartPetCare::~GSmartPetCare()
{
    delete ui;
}

void GSmartPetCare::on_pushButtonConnexion_clicked()
{
    const QString identifiant = ui->lineEditIdentifiant->text().trimmed();
    const QString motDePasse  = ui->lineEditMotDePasse->text();

    // Contrôle de saisie
    if (identifiant.isEmpty() || motDePasse.isEmpty()) {
        QMessageBox::warning(this, "Champs obligatoires",
                             "Veuillez saisir votre identifiant et votre mot de passe.");
        return;
    }

    // TODO : vérifier l'identifiant et le mot de passe (haché) dans la base de données,
    //        puis récupérer le rôle de l'employé pour gérer ses privilèges.
    //        Pour l'instant, on passe directement à la page d'accueil.
    ui->lineEditMotDePasse->clear();
    ui->stackedWidget->setCurrentIndex(PageAccueil);
}
