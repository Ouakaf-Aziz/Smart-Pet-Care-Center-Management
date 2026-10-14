#include "connexion.h"
#include "ui_gsmartpetcare.h"

Connexion::Connexion(Ui::GSmartPetCare *ui, const BaseEmployes &base, QWidget *fenetre)
    : QObject(fenetre)
    , ui(ui)
    , m_fenetre(fenetre)
    , m_base(base)
{
    // Les widgets sont ceux de la page de connexion de gsmartpetcare.ui (pageLogin) :
    // l'identifiant est l'email de l'employé. La touche Entrée est gérée par GSmartPetCare.
    connect(ui->pushButtonConnexion, &QPushButton::clicked, this, &Connexion::onConnecter);
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
        afficherErreur("Saisissez votre identifiant et votre mot de passe.");
        return;
    }

    // MODE TEMPORAIRE : n'importe quel identifiant / mot de passe est accepté (seuls les champs vides sont refusés).
    // On ouvre la session avec un compte « Responsable RH » actif (accès total), sinon le premier compte actif.
    const Employe *e = nullptr;
    for (const Employe &emp : m_base.employes) {
        if (emp.statut != "Actif")
            continue;
        if (emp.poste == "Responsable RH") { e = &emp; break; }
        if (!e)
            e = &emp;
    }
    if (!e) {
        afficherErreur("Aucun compte actif disponible.");
        return;
    }

    reussir(e->id);
}
