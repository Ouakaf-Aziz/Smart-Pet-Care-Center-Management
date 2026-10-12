#include "faceauth.h"

#include "facedialog.h"
#include "faceengine.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QSaveFile>
#include <QStandardPaths>

namespace {

QString cheminGabarits()
{
    const QString dossier = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dossier);
    return dossier + "/visages.json";
}

bool moduleDisponible(QWidget *parent)
{
    if (FaceEngine::compileAvecOpenCV())
        return true;
    QMessageBox::information(parent, "Reconnaissance faciale",
                             "Cette version a été compilée sans OpenCV : la reconnaissance faciale "
                             "est désactivée.\n\nUtilisez votre email et votre mot de passe.");
    return false;
}

} // namespace

int FaceAuth::authentifier(Ui::GSmartPetCare *ui, QWidget *parent, const QVector<Employe> &employes)
{
    if (!moduleDisponible(parent))
        return -1;

    bool aucunVisage = true;
    for (const Employe &e : employes)
        if (!e.gabaritsVisage.isEmpty())
            aucunVisage = false;
    if (aucunVisage) {
        QMessageBox::information(parent, "Reconnaissance faciale",
                                 "Aucun visage n'est encore enregistré.\n\n"
                                 "Connectez-vous avec votre email, puis enregistrez votre visage "
                                 "(fiche employé > « Enregistrer le visage »).");
        return -1;
    }

    FaceDialog dlg(ui, FaceDialog::Authentification, &employes, QString(), parent);
    QString erreur;
    if (!dlg.demarrer(&erreur)) {
        QMessageBox::critical(parent, "Reconnaissance faciale", erreur);
        return -1;
    }
    if (!dlg.exec()) {
        if (dlg.expire())
            QMessageBox::information(parent, "Reconnaissance faciale",
                                     "Visage non reconnu.\n\nRéessayez ou utilisez votre email et votre mot de passe.");
        return -1;
    }
    return dlg.employeId();
}

bool FaceAuth::enregistrer(Ui::GSmartPetCare *ui, QWidget *parent, BaseEmployes &base, int employeId)
{
    Employe *e = base.trouver(employeId);
    if (!e || !moduleDisponible(parent))
        return false;

    FaceDialog dlg(ui, FaceDialog::Enregistrement, nullptr, QString("%1 %2").arg(e->prenom, e->nom), parent);
    QString erreur;
    if (!dlg.demarrer(&erreur)) {
        QMessageBox::critical(parent, "Reconnaissance faciale", erreur);
        return false;
    }
    if (!dlg.exec())
        return false;

    // Ce visage ne doit pas déjà appartenir à un autre employé (sinon la connexion serait ambiguë).
    QVector<Employe> autres;
    for (const Employe &x : std::as_const(base.employes))
        if (x.id != employeId)
            autres.append(x);
    for (const QVector<float> &g : dlg.gabarits()) {
        const FaceMatch::Resultat r = FaceMatch::identifier(g, autres);
        if (r.score >= FaceMatch::kSeuil) {
            const Employe *autre = base.trouver(r.employeId);
            QMessageBox::warning(parent, "Visage déjà enregistré",
                                 QString("Ce visage ressemble trop à celui de %1 : enregistrement refusé.")
                                     .arg(autre ? autre->prenom + " " + autre->nom : QString("un autre employé")));
            return false;
        }
    }

    e->gabaritsVisage = dlg.gabarits();
    if (!sauvegarderGabarits(base))
        QMessageBox::warning(parent, "Reconnaissance faciale",
                             "Visage enregistré, mais la sauvegarde sur disque a échoué : "
                             "il sera perdu à la fermeture.");
    return true;
}

bool FaceAuth::sauvegarderGabarits(const BaseEmployes &base)
{
    QJsonObject racine;
    for (const Employe &e : base.employes) {
        if (e.gabaritsVisage.isEmpty())
            continue;
        QJsonArray echantillons;
        for (const QVector<float> &g : e.gabaritsVisage) {
            QJsonArray valeurs;
            for (float v : g)
                valeurs.append(double(v));
            echantillons.append(valeurs);
        }
        racine.insert(e.email.trimmed().toLower(), echantillons);
    }

    QSaveFile f(cheminGabarits());                  // écriture atomique
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(QJsonDocument(racine).toJson(QJsonDocument::Compact));
    f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return f.commit();
}

void FaceAuth::chargerGabarits(BaseEmployes &base)
{
    QFile f(cheminGabarits());
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QJsonObject racine = QJsonDocument::fromJson(f.readAll()).object();

    for (Employe &e : base.employes) {
        const QJsonArray echantillons = racine.value(e.email.trimmed().toLower()).toArray();
        QVector<QVector<float>> gabarits;
        for (const QJsonValue &ev : echantillons) {
            const QJsonArray valeurs = ev.toArray();
            if (valeurs.size() != FaceEngine::kDimension)       // fichier altéré : on ignore
                continue;
            QVector<float> g;
            g.reserve(valeurs.size());
            for (const QJsonValue &v : valeurs)
                g.append(float(v.toDouble()));
            gabarits.append(g);
        }
        e.gabaritsVisage = gabarits;
    }
}
