#include "connexion.h"
#include "ui_gsmartpetcare.h"

#include "faceauth.h"

Connexion::Connexion(Ui::GSmartPetCare *ui, const BaseEmployes &base, QWidget *fenetre)
    : QObject(fenetre)
    , ui(ui)
    , m_fenetre(fenetre)
    , m_base(base)
{
    // Les widgets sont ceux de la page de connexion de gsmartpetcare.ui (pageLogin) :
    // l'identifiant est l'email de l'employé. La touche Entrée est gérée par GSmartPetCare.
    connect(ui->pushButtonConnexion, &QPushButton::clicked, this, &Connexion::onConnecter);
    connect(ui->pushButtonVisageConnexion, &QPushButton::clicked, this, &Connexion::onVisage);
}

void Connexion::afficherErreur(const QString &message)
{
    ui->labelErreurConnexion->setText(message);
    ui->labelErreurConnexion->show();
}

void Connexion::reussir(int employeId)
{
    ui->lineEditMotDePasse->clear();              // rien ne reste affiché une fois connecté
    ui->labelErreurConnexion->hide();
    m_essais = 0;
    emit connecte(employeId);
}

void Connexion::onConnecter()
{
    const QString email = ui->lineEditIdentifiant->text().trimmed();
    if (email.isEmpty() || ui->lineEditMotDePasse->text().isEmpty()) {
        afficherErreur("Saisissez votre email et votre mot de passe.");
        return;
    }

    // Même message dans tous les cas d'échec : on ne révèle pas si l'email existe.
    const Employe *e = m_base.trouverParEmail(email);
    if (!e || !Emp::verifier(ui->lineEditMotDePasse->text(), e->motDePasse)) {
        if (++m_essais >= 5) {
            emit abandon();                     // trop d'échecs : on ferme l'application
            return;
        }
        afficherErreur(QString("Email ou mot de passe incorrect (%1/5).").arg(m_essais));
        ui->lineEditMotDePasse->clear();
        return;
    }

    if (e->statut != "Actif") {
        afficherErreur("Ce compte n'a pas accès à l'application.");
        return;
    }

    reussir(e->id);
}

void Connexion::onVisage()
{
    const int id = FaceAuth::authentifier(ui, m_fenetre, m_base.employes);
    if (id < 0)
        return;
    const Employe *e = m_base.trouver(id);
    if (!e || e->statut != "Actif") {
        afficherErreur("Ce compte n'a pas accès à l'application.");
        return;
    }
    reussir(id);
}
