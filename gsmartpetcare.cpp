#include "gsmartpetcare.h"
#include "ui_gsmartpetcare.h"
#include "gestionservices.h"
#include "gestionrdv.h"
#include "gestionemployes.h"
#include "gestionanimaux.h"
#include "statistiquesemployes.h"

#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QIcon>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QStyle>

GSmartPetCare::GSmartPetCare(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GSmartPetCare)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/images/icone.png"));
    resize(1366, 800);

    Emp::chargerDonneesDemo(m_baseEmployes);

    auto *ombre = new QGraphicsDropShadowEffect(this);
    ombre->setBlurRadius(30);
    ombre->setOffset(0, 4);
    ombre->setColor(QColor(0, 0, 0, 45));
    ui->frameCard->setGraphicsEffect(ombre);

    ui->stackedWidget->setCurrentIndex(PageLogin);
    ui->lineEditIdentifiant->setFocus();

    connect(ui->lineEditIdentifiant, &QLineEdit::returnPressed,
            ui->lineEditMotDePasse, qOverload<>(&QWidget::setFocus));
    connect(ui->lineEditMotDePasse, &QLineEdit::returnPressed,
            ui->pushButtonConnexion, &QPushButton::click);

    gestionServices = new GestionServices(ui, this, this);
    gestionRdv = new GestionRdv(ui, this, this);
    gestionEmployes = new GestionEmployes(ui, m_baseEmployes, this, this);
    gestionAnimaux = new GestionAnimaux(ui, this, this);

    connect(gestionEmployes, &GestionEmployes::statistiquesDemandees, this, [this]() {
        StatistiquesEmployes fenetre(m_baseEmployes.employes, this);
        fenetre.exec();
    });

    afficherContenu(ContenuAccueil);
    connect(ui->btnNavAccueil, &QPushButton::clicked, this, [this]() {
        afficherContenu(ContenuAccueil);
    });
    connect(ui->btnNavServices, &QPushButton::clicked, this, [this]() {
        afficherContenu(ContenuServices);
    });
    connect(ui->btnNavRDV, &QPushButton::clicked, this, [this]() {
        afficherContenu(ContenuRdv);
    });
    connect(ui->btnNavEmployes, &QPushButton::clicked, this, [this]() {
        afficherContenu(ContenuEmployes);
    });
    connect(ui->btnNavAnimaux, &QPushButton::clicked, this, [this]() {
        afficherContenu(ContenuAnimaux);
    });

    preparerCartesAccueil();

    connect(ui->btnDeconnexion, &QPushButton::clicked, this, [this]() {
        afficherContenu(ContenuAccueil);
        ui->stackedWidget->setCurrentIndex(PageLogin);
        ui->lineEditIdentifiant->setFocus();
    });
}

void GSmartPetCare::preparerCartesAccueil()
{
    const struct { QWidget *carte; int page; } cartes[] = {
        { ui->cardService,  ContenuServices },
        { ui->cardEmployes, ContenuEmployes },
        { ui->cardRDV,      ContenuRdv },
        { ui->cardAnimaux,  ContenuAnimaux },
    };
    for (const auto &c : cartes) {
        c.carte->setCursor(Qt::PointingHandCursor);
        c.carte->installEventFilter(this);
        c.carte->setProperty("pageContenu", c.page);
    }
}

bool GSmartPetCare::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() == Qt::LeftButton) {
            QWidget *w = qobject_cast<QWidget *>(watched);
            if (w && w->property("pageContenu").isValid()) {
                afficherContenu(w->property("pageContenu").toInt());
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void GSmartPetCare::afficherContenu(int index)
{
    ui->stackContenu->setCurrentIndex(index);

    const int highlight = index;
    const struct { QPushButton *bouton; int page; } liens[] = {
        { ui->btnNavAccueil,  ContenuAccueil },
        { ui->btnNavServices, ContenuServices },
        { ui->btnNavEmployes, ContenuEmployes },
        { ui->btnNavRDV,      ContenuRdv },
        { ui->btnNavAnimaux,  ContenuAnimaux },
    };
    for (const auto &l : liens) {
        l.bouton->setProperty("actif", l.page == highlight);
        l.bouton->style()->unpolish(l.bouton);
        l.bouton->style()->polish(l.bouton);
    }
}

void GSmartPetCare::appliquerSession(const Employe *u)
{
    if (u) {
        ui->labelBienvenue->setText(
            QString("Bienvenue %1 %2  -  Rôle : %3").arg(u->prenom, u->nom, u->poste));
        gestionEmployes->setUtilisateur(u->id);
        gestionAnimaux->setUtilisateur(QString("%1 %2").arg(u->prenom, u->nom));
    } else if (!m_baseEmployes.employes.isEmpty()) {
        const Employe *premier = nullptr;
        for (const Employe &e : m_baseEmployes.employes) {
            if (e.statut == "Actif") {
                premier = &e;
                break;
            }
        }
        if (!premier)
            premier = &m_baseEmployes.employes.first();
        ui->labelBienvenue->setText(
            QString("Bienvenue %1 %2  -  Rôle : %3")
                .arg(premier->prenom, premier->nom, premier->poste));
        gestionEmployes->setUtilisateur(premier->id);
        gestionAnimaux->setUtilisateur(QString("%1 %2").arg(premier->prenom, premier->nom));
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

    if (identifiant.isEmpty() || motDePasse.isEmpty()) {
        QMessageBox::warning(this, "Champs obligatoires",
                             "Veuillez saisir votre identifiant et votre mot de passe.");
        return;
    }

    const Employe *trouve = m_baseEmployes.trouverParEmail(identifiant);
    if (trouve) {
        if (trouve->statut != "Actif") {
            QMessageBox::warning(this, "Compte inactif",
                                 "Ce compte employé n'est pas actif.");
            return;
        }
        if (!Emp::verifier(motDePasse, trouve->motDePasse)) {
            QMessageBox::warning(this, "Connexion refusée",
                                 "Identifiant ou mot de passe incorrect.");
            return;
        }
        appliquerSession(trouve);
    } else {
        // Accès de démonstration (comme avant) : la session employé
        // reprend le premier compte actif.
        appliquerSession(nullptr);
    }

    ui->lineEditMotDePasse->clear();
    ui->stackedWidget->setCurrentIndex(PageAccueil);
}
