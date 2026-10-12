#include "facedialog.h"
#include "ui_gsmartpetcare.h"

#include <QPainter>

namespace {

constexpr int kDelaiAuthMs = 20000;         // abandon de l'authentification après 20 s
constexpr int kImagesStables = 3;           // même employé reconnu sur 3 analyses de suite
constexpr int kIntervalleCaptureMs = 1200;  // laisse le temps de bouger légèrement la tête
constexpr double kCoherenceEnregistrement = 0.45;   // les échantillons doivent être de la même personne

const char *kPoses[FaceDialog::kEchantillons] = {
    "Regardez la caméra, visage de face.",
    "Tournez très légèrement la tête vers la gauche.",
    "Tournez très légèrement la tête vers la droite.",
    "Relevez un peu le menton.",
    "De face, avec un léger sourire.",
};

// Le visage doit être seul, entier, ni trop loin ni trop près.
bool qualiteOk(const QVector<VisageDetecte> &v, const QSize &image, QString *conseil)
{
    if (v.isEmpty()) {
        *conseil = "Aucun visage détecté : placez-vous face à la caméra, dans un endroit bien éclairé.";
        return false;
    }
    if (v.size() > 1) {
        *conseil = "Un seul visage à la fois, s'il vous plaît.";
        return false;
    }
    const QRect r = v.first().cadre;
    if (r.left() < 0 || r.top() < 0 || r.right() >= image.width() || r.bottom() >= image.height()) {
        *conseil = "Centrez votre visage dans l'image.";
        return false;
    }
    if (r.width() < image.width() * 0.18) {
        *conseil = "Rapprochez-vous de la caméra.";
        return false;
    }
    if (r.width() > image.width() * 0.75) {
        *conseil = "Reculez un peu.";
        return false;
    }
    return true;
}

} // namespace

void FaceDialog::preparerPage(Ui::GSmartPetCare *ui)
{
    // Plein écran : l'aperçu garde un format raisonnable et reste centré.
    ui->labelApercu->setMaximumSize(960, 720);
    ui->progressBar->setMinimumWidth(640);
    ui->progressBar->setMaximumWidth(960);
    ui->labelConsigne->setMaximumWidth(960);
    for (QWidget *w : {static_cast<QWidget *>(ui->labelTitreVisage), static_cast<QWidget *>(ui->labelApercu),
                       static_cast<QWidget *>(ui->labelConsigne), static_cast<QWidget *>(ui->progressBar)})
        ui->layoutPageVisage->setAlignment(w, Qt::AlignHCenter);
    ui->spacerBoutonsVisage->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->layoutPageVisage->setAlignment(ui->layoutBoutonsVisage, Qt::AlignHCenter);
    ui->layoutPageVisage->insertStretch(0, 1);
    ui->layoutPageVisage->addStretch(1);
}

FaceDialog::FaceDialog(Ui::GSmartPetCare *ui, Mode mode, const QVector<Employe> *employes, const QString &titre,
                       QWidget *fenetre)
    : QObject(fenetre)
    , ui(ui)
    , m_mode(mode)
    , m_employes(employes)
{
    ui->pageVisage->setWindowTitle(mode == Enregistrement ? "Enregistrement du visage" : "Reconnaissance faciale");
    ui->labelTitreVisage->setText(mode == Enregistrement ? QString("Enregistrer le visage de %1").arg(titre)
                                                         : QString("Regardez la caméra pour vous identifier"));
    ui->labelApercu->clear();
    ui->progressBar->setRange(0, kEchantillons);
    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(mode == Enregistrement);

    connect(ui->btnAnnulerVisage, &QPushButton::clicked, this, [this]() { terminer(false); });
    connect(&m_timer, &QTimer::timeout, this, &FaceDialog::onTick);
}

FaceDialog::~FaceDialog()
{
    m_timer.stop();
    m_moteur.fermerCamera();
}

bool FaceDialog::demarrer(QString *erreur)
{
    if (!m_moteur.charger(erreur) || !m_moteur.ouvrirCamera(erreur))
        return false;
    m_chrono.start();
    m_derniereCapture.start();
    ui->labelConsigne->setText(m_mode == Enregistrement ? QString(kPoses[0]) : QString("Recherche du visage..."));
    m_timer.start(40);
    return true;
}

bool FaceDialog::exec()
{
    QWidget *precedente = ui->stackedWidget->currentWidget();
    ui->stackedWidget->setCurrentWidget(ui->pageVisage);
    if (!m_termine)
        m_boucle.exec();                        // l'interface reste active : annuler, fermer la fenêtre...
    if (precedente)
        ui->stackedWidget->setCurrentWidget(precedente);
    return m_reussi;
}

void FaceDialog::terminer(bool reussi)
{
    if (m_termine)
        return;
    m_termine = true;
    m_reussi = reussi;
    m_timer.stop();
    m_moteur.fermerCamera();
    m_boucle.quit();
}

void FaceDialog::afficher(QImage image, const QVector<VisageDetecte> &visages, bool ok)
{
    QPainter p(&image);
    p.setPen(QPen(ok ? QColor("#0F766E") : QColor("#F28C28"), 3));
    for (const VisageDetecte &v : visages)
        p.drawRect(v.cadre);
    p.end();
    // Aperçu en miroir (comme un miroir), uniquement pour l'affichage.
    ui->labelApercu->setPixmap(QPixmap::fromImage(image.mirrored(true, false))
                            .scaled(ui->labelApercu->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void FaceDialog::onTick()
{
    if (m_termine)
        return;
    QImage image;
    QVector<VisageDetecte> visages;
    if (!m_moteur.lire(&image, &visages)) {
        if (++m_echecsLecture > 50) {                       // ~2 s sans image
            ui->labelConsigne->setText("La caméra ne répond plus.");
            terminer(false);
        }
        return;
    }
    m_echecsLecture = 0;

    QString conseil;
    const bool ok = qualiteOk(visages, image.size(), &conseil);
    afficher(image, visages, ok);

    if (m_mode == Authentification && m_chrono.elapsed() > kDelaiAuthMs) {
        m_expire = true;
        terminer(false);
        return;
    }
    if (!ok) {
        m_stable = 0;
        m_candidat = -1;
        ui->labelConsigne->setText(conseil);
        return;
    }

    if (m_mode == Enregistrement) {
        const int n = m_gabarits.size();
        if (m_derniereCapture.elapsed() < kIntervalleCaptureMs) {
            ui->labelConsigne->setText(QString(kPoses[n]));
            return;
        }
        QVector<float> g;
        if (!m_moteur.calculerGabarit(0, &g))
            return;
        if (!m_gabarits.isEmpty() && FaceEngine::similarite(g, m_gabarits.first()) < kCoherenceEnregistrement) {
            ui->labelConsigne->setText("Le visage a changé pendant l'enregistrement : une seule personne, de face.");
            return;
        }
        m_gabarits.append(g);
        m_derniereCapture.restart();
        ui->progressBar->setValue(m_gabarits.size());
        if (m_gabarits.size() >= kEchantillons) {
            ui->labelConsigne->setText("Enregistrement terminé.");
            terminer(true);
        } else {
            ui->labelConsigne->setText(QString(kPoses[m_gabarits.size()]));
        }
        return;
    }

    // Authentification : une analyse toutes les 3 images (~8 par seconde).
    if (++m_tick % 3 != 0)
        return;
    QVector<float> g;
    if (!m_moteur.calculerGabarit(0, &g))
        return;
    const FaceMatch::Resultat r = FaceMatch::identifier(g, *m_employes);
    if (!r.accepte) {
        m_stable = 0;
        m_candidat = -1;
        ui->labelConsigne->setText("Visage non reconnu...");
        return;
    }
    if (r.employeId == m_candidat) {
        ++m_stable;
    } else {
        m_candidat = r.employeId;
        m_stable = 1;
    }
    ui->labelConsigne->setText("Vérification...");
    if (m_stable >= kImagesStables) {
        m_employeId = m_candidat;
        terminer(true);
    }
}
