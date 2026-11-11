#include "employee.h"
#include "ui_gsmartpetcare.h"

#include <QCryptographicHash>
#include <QRandomGenerator>

Employe *BaseEmployes::trouver(int id)
{
    for (Employe &e : employes)
        if (e.id == id)
            return &e;
    return nullptr;
}

const Employe *BaseEmployes::trouver(int id) const
{
    for (const Employe &e : employes)
        if (e.id == id)
            return &e;
    return nullptr;
}

const Employe *BaseEmployes::trouverParEmail(const QString &email) const
{
    for (const Employe &e : employes)
        if (e.email.compare(email.trimmed(), Qt::CaseInsensitive) == 0)
            return &e;
    return nullptr;
}

int BaseEmployes::nombreResponsablesRH() const
{
    int n = 0;
    for (const Employe &e : employes)
        if (e.poste == QLatin1String("Responsable RH"))
            ++n;
    return n;
}

Droits BaseEmployes::droits(const QString &poste) const
{
    return Emp::droitsParDefaut(poste);
}

QStringList Emp::postes()
{
    return { QStringLiteral("Responsable RH"), QStringLiteral("Comptable"),
             QStringLiteral("Vétérinaire"), QStringLiteral("Assistant vétérinaire"),
             QStringLiteral("Réceptionniste"), QStringLiteral("Toiletteur") };
}

QStringList Emp::statuts()
{
    return { QStringLiteral("Actif"), QStringLiteral("Inactif") };
}

Droits Emp::droitsParDefaut(const QString &poste)
{
    Droits d;
    if (poste == QLatin1String("Responsable RH")) {
        d.ajouter = d.modifier = d.supprimer = true;
        d.voirSalaires = d.exporter = d.statistiques = true;
        d.gererPrivileges = true;
    } else if (poste == QLatin1String("Comptable")) {
        d.voirSalaires = d.exporter = d.statistiques = true;
    } else {
        d.statistiques = true;
    }
    return d;
}

QString Emp::hacherAvecSel(const QString &sel, const QString &mdp)
{
    return QString::fromLatin1(
        QCryptographicHash::hash((sel + mdp).toUtf8(), QCryptographicHash::Sha256).toHex());
}

QString Emp::hacher(const QString &mdp)
{
    const QString sel = QString::number(QRandomGenerator::global()->generate64(), 16);
    return sel + QLatin1Char('$') + hacherAvecSel(sel, mdp);
}

bool Emp::verifier(const QString &mdp, const QString &stocke)
{
    const int i = stocke.indexOf(QLatin1Char('$'));
    if (i <= 0)
        return false;
    return hacherAvecSel(stocke.left(i), mdp) == stocke.mid(i + 1);
}



#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include <algorithm>
#include <cmath>
#include <vector>

#ifdef HAVE_OPENCV
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/objdetect/face.hpp>
#include <opencv2/videoio.hpp>
#endif

namespace {

QStringList dossiersModeles()
{
    QStringList d;
    const QString app = QCoreApplication::applicationDirPath();
    d << app + "/models" << app + "/../models" << QDir::currentPath() + "/models";
#ifdef FACE_MODELS_DIR
    d << QString::fromUtf8(FACE_MODELS_DIR);
#endif
    return d;
}

QStringList chercherModeles(const QStringList &noms, qint64 tailleMin, QString *probleme)
{
    QStringList trouves;
    for (const QString &nom : noms) {
        for (const QString &dossier : dossiersModeles()) {
            const QFileInfo fi(dossier + "/" + nom);
            if (!fi.exists())
                continue;
            if (fi.size() < tailleMin) {
                if (probleme)
                    *probleme = QString("Le fichier %1 est incomplet (%2 octets) : c'est probablement un "
                                        "pointeur Git LFS. Relancez le script de téléchargement.")
                                    .arg(fi.absoluteFilePath()).arg(fi.size());
                continue;
            }
            const QString chemin = fi.absoluteFilePath();
            if (!trouves.contains(chemin))
                trouves << chemin;
        }
    }
    return trouves;
}

} // namespace

struct FaceEngine::Impl
{
#ifdef HAVE_OPENCV
    cv::VideoCapture camera;
    cv::Ptr<cv::FaceDetectorYN> detecteur;
    cv::Ptr<cv::FaceRecognizerSF> reconnaisseur;
    cv::Mat image;                          // dernière image lue (BGR)
    std::vector<cv::Mat> boites;            // lignes 1x15 de detect(), du plus grand au plus petit
    cv::Size tailleDetecteur;
#endif
};

FaceEngine::FaceEngine() : d(new Impl) {}
FaceEngine::~FaceEngine() { fermerCamera(); }

bool FaceEngine::compileAvecOpenCV()
{
#ifdef HAVE_OPENCV
    return true;
#else
    return false;
#endif
}

double FaceEngine::similarite(const QVector<float> &a, const QVector<float> &b)
{
    if (a.isEmpty() || a.size() != b.size())
        return -1.0;
    double ab = 0, aa = 0, bb = 0;
    for (int i = 0; i < a.size(); ++i) {
        ab += double(a[i]) * b[i];
        aa += double(a[i]) * a[i];
        bb += double(b[i]) * b[i];
    }
    const double n = std::sqrt(aa) * std::sqrt(bb);
    return n > 0.0 ? ab / n : -1.0;
}

#ifdef HAVE_OPENCV

bool FaceEngine::charger(QString *erreur)
{
    if (d->detecteur && d->reconnaisseur)
        return true;

    QString probleme;
    const QStringList detecteurs = chercherModeles(
        {"face_detection_yunet_2023mar.onnx", "face_detection_yunet_2022mar.onnx"}, 100000, &probleme);
    const QStringList reconnaisseurs = chercherModeles({"face_recognition_sface_2021dec.onnx"}, 1000000, &probleme);

    if (detecteurs.isEmpty() || reconnaisseurs.isEmpty()) {
        if (erreur) {
            *erreur = probleme.isEmpty()
                          ? QString("Modèles de reconnaissance faciale introuvables.\n\n"
                                    "Placez face_detection_yunet_2023mar.onnx et face_recognition_sface_2021dec.onnx "
                                    "dans le dossier « models » (script : models/telecharger_modeles).\n"
                                    "Dossiers cherchés :\n%1").arg(dossiersModeles().join("\n"))
                          : probleme;
        }
        return false;
    }

    QString derniere;
    for (const QString &chemin : detecteurs) {      // 2023mar d'abord ; 2022mar si OpenCV < 4.8
        try {
            d->detecteur = cv::FaceDetectorYN::create(chemin.toStdString(), "", cv::Size(320, 320),
                                                      0.9f, 0.3f, 5000);
            if (d->detecteur)
                break;
        } catch (const cv::Exception &ex) {
            derniere = QString::fromStdString(ex.what());
        }
    }
    if (!d->detecteur) {
        if (erreur)
            *erreur = "Impossible de charger le détecteur de visages (OpenCV " CV_VERSION ").\n" + derniere;
        return false;
    }

    try {
        d->reconnaisseur = cv::FaceRecognizerSF::create(reconnaisseurs.first().toStdString(), "");
    } catch (const cv::Exception &ex) {
        if (erreur)
            *erreur = "Impossible de charger le modèle de reconnaissance (OpenCV " CV_VERSION ").\n"
                      + QString::fromStdString(ex.what());
        d->detecteur.release();
        return false;
    }
    d->tailleDetecteur = cv::Size();
    return true;
}

bool FaceEngine::ouvrirCamera(QString *erreur)
{
    if (d->camera.isOpened())
        return true;
    bool ok = false;
    try {
#ifdef Q_OS_WIN
        ok = d->camera.open(0, cv::CAP_DSHOW);
#else
        ok = d->camera.open(0);
#endif
        if (ok) {
            d->camera.set(cv::CAP_PROP_FRAME_WIDTH, 640);
            d->camera.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
        }
    } catch (const cv::Exception &) {
        ok = false;
    }
    if (!ok && erreur)
        *erreur = "Impossible d'ouvrir la caméra.\nVérifiez qu'une webcam est branchée, "
                  "qu'aucune autre application ne l'utilise et que l'accès caméra est autorisé.";
    return ok;
}

void FaceEngine::fermerCamera()
{
    if (d->camera.isOpened())
        d->camera.release();
}

bool FaceEngine::lire(QImage *image, QVector<VisageDetecte> *visages)
{
    visages->clear();
    d->boites.clear();
    if (!d->camera.isOpened() || !d->detecteur)
        return false;

    cv::Mat f;
    try {
        if (!d->camera.read(f) || f.empty())
            return false;
        if (f.channels() == 1)
            cv::cvtColor(f, f, cv::COLOR_GRAY2BGR);
        else if (f.channels() == 4)
            cv::cvtColor(f, f, cv::COLOR_BGRA2BGR);
        d->image = f;

        if (f.size() != d->tailleDetecteur) {
            d->detecteur->setInputSize(f.size());
            d->tailleDetecteur = f.size();
        }
        cv::Mat faces;
        d->detecteur->detect(f, faces);

        for (int r = 0; r < faces.rows; ++r) {
            VisageDetecte v;
            v.cadre = QRect(int(faces.at<float>(r, 0)), int(faces.at<float>(r, 1)),
                            int(faces.at<float>(r, 2)), int(faces.at<float>(r, 3)));
            v.score = faces.at<float>(r, 14);
            visages->append(v);
            d->boites.push_back(faces.row(r).clone());
        }
    } catch (const cv::Exception &) {
        return false;
    }

    std::vector<int> ordre(visages->size());
    for (int i = 0; i < int(ordre.size()); ++i)
        ordre[i] = i;
    std::sort(ordre.begin(), ordre.end(), [&](int a, int b) {
        return (*visages)[a].cadre.width() * (*visages)[a].cadre.height()
               > (*visages)[b].cadre.width() * (*visages)[b].cadre.height();
    });
    QVector<VisageDetecte> v2;
    std::vector<cv::Mat> b2;
    for (int i : ordre) {
        v2.append((*visages)[i]);
        b2.push_back(d->boites[i]);
    }
    *visages = v2;
    d->boites = b2;

    *image = QImage(f.data, f.cols, f.rows, int(f.step), QImage::Format_BGR888).copy();
    return true;
}

bool FaceEngine::calculerGabarit(int index, QVector<float> *gabarit, QString *erreur)
{
    if (!d->reconnaisseur || index < 0 || index >= int(d->boites.size())) {
        if (erreur)
            *erreur = "Aucun visage disponible.";
        return false;
    }
    try {
        cv::Mat aligne, feature;
        d->reconnaisseur->alignCrop(d->image, d->boites[index], aligne);
        d->reconnaisseur->feature(aligne, feature);
        feature = feature.reshape(1, 1);
        feature.convertTo(feature, CV_32F);
        const double n = cv::norm(feature);
        if (n <= 0.0 || feature.total() != size_t(kDimension)) {
            if (erreur)
                *erreur = "Empreinte invalide.";
            return false;
        }
        feature /= n;
        gabarit->resize(kDimension);
        for (int i = 0; i < kDimension; ++i)
            (*gabarit)[i] = feature.at<float>(0, i);
        return true;
    } catch (const cv::Exception &ex) {
        if (erreur)
            *erreur = QString::fromStdString(ex.what());
        return false;
    }
}

#else   // ----- compilé sans OpenCV : le module reste présent mais inactif -----

bool FaceEngine::charger(QString *erreur)
{
    if (erreur)
        *erreur = "Cette version a été compilée sans OpenCV : la reconnaissance faciale est désactivée.";
    return false;
}
bool FaceEngine::ouvrirCamera(QString *erreur) { return charger(erreur); }
void FaceEngine::fermerCamera() {}
bool FaceEngine::lire(QImage *, QVector<VisageDetecte> *visages) { visages->clear(); return false; }
bool FaceEngine::calculerGabarit(int, QVector<float> *, QString *erreur) { return charger(erreur); }

#endif

FaceMatch::Resultat FaceMatch::identifier(const QVector<float> &g, const QVector<Employe> &employes)
{
    Resultat r;
    double meilleur = -1.0, second = -1.0;
    int idMeilleur = -1;

    for (const Employe &e : employes) {
        double score = -1.0;                                    // meilleur échantillon de cet employé
        for (const QVector<float> &echantillon : e.gabaritsVisage)
            score = std::max(score, FaceEngine::similarite(g, echantillon));
        if (score < 0.0 && e.gabaritsVisage.isEmpty())
            continue;
        if (score > meilleur) {
            second = meilleur;
            meilleur = score;
            idMeilleur = e.id;
        } else if (score > second) {
            second = score;
        }
    }

    r.employeId = idMeilleur;
    r.score = meilleur;
    r.second = second;
    r.accepte = idMeilleur >= 0 && meilleur >= kSeuil && (meilleur - std::max(second, 0.0) >= kMarge);
    return r;
}


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


#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>


EmployeSQL::EmployeSQL()
{
    id = 0;
    salaire = 0.0;
}


EmployeSQL::EmployeSQL(int id,
                 QString nom,
                 QString prenom,
                 QString email,
                 QString adresse,
                 QString telephone,
                 double salaire,
                 QDate dateEmbauche,
                 QString statut,
                 QString poste,
                 QString motDePasse)
{
    this->id = id;
    this->nom = nom;
    this->prenom = prenom;
    this->email = email;
    this->adresse = adresse;
    this->telephone = telephone;
    this->salaire = salaire;
    this->dateEmbauche = dateEmbauche;
    this->statut = statut;
    this->poste = poste;
    this->motDePasse = motDePasse;
}


bool EmployeSQL::ajouter()
{
    QSqlQuery query;

    query.prepare(
        "INSERT INTO EMPLOYE "
        "(ID_EMPLOYR, NOM, PRENOM, EMAIL, ADRESSE, TELEPHONE, "
        "SALAIRE, DATE_EMBAUCHE, STATUT, POSTE, MOY_DE_PASSE) "
        "VALUES "
        "(:id, :nom, :prenom, :email, :adresse, :telephone, "
        ":salaire, :date_embauche, :statut, :poste, :mot_de_passe)"
        );

    query.bindValue(":id", id);
    query.bindValue(":nom", nom);
    query.bindValue(":prenom", prenom);
    query.bindValue(":email", email);
    query.bindValue(":adresse", adresse);
    query.bindValue(":telephone", telephone);
    query.bindValue(":salaire", salaire);

    query.bindValue(":date_embauche", dateEmbauche);

    query.bindValue(":statut", statut);
    query.bindValue(":poste", poste);
    query.bindValue(":mot_de_passe", motDePasse);

    if (query.exec())
    {
        qDebug() << "Employé ajouté avec succès.";
        return true;
    }
    else
    {
        qDebug() << "Erreur ajout :" << query.lastError().text();
        return false;
    }
}


bool EmployeSQL::supprimer()
{
    QSqlQuery query;

    query.prepare(
        "DELETE FROM EMPLOYE "
        "WHERE ID_EMPLOYR = :id"
        );

    query.bindValue(":id", id);

    if (query.exec())
    {
        qDebug() << "Employé supprimé avec succès.";
        return true;
    }
    else
    {
        qDebug() << "Erreur suppression :"
                 << query.lastError().text();
        return false;
    }
}


bool EmployeSQL::modifier()
{
    QSqlQuery query;

    query.prepare(
        "UPDATE EMPLOYE SET "
        "NOM = :nom, "
        "PRENOM = :prenom, "
        "EMAIL = :email, "
        "ADRESSE = :adresse, "
        "TELEPHONE = :telephone, "
        "SALAIRE = :salaire, "
        "DATE_EMBAUCHE = :date_embauche, "
        "STATUT = :statut, "
        "POSTE = :poste, "
        "MOY_DE_PASSE = :mot_de_passe "
        "WHERE ID_EMPLOYR = :id"
        );

    query.bindValue(":id", id);
    query.bindValue(":nom", nom);
    query.bindValue(":prenom", prenom);
    query.bindValue(":email", email);
    query.bindValue(":adresse", adresse);
    query.bindValue(":telephone", telephone);
    query.bindValue(":salaire", salaire);
    query.bindValue(":date_embauche", dateEmbauche);
    query.bindValue(":statut", statut);
    query.bindValue(":poste", poste);
    query.bindValue(":mot_de_passe", motDePasse);

    if (query.exec())
    {
        qDebug() << "Employé modifié avec succès.";
        return true;
    }
    else
    {
        qDebug() << "Erreur modification :"
                 << query.lastError().text();
        return false;
    }
}


QSqlQueryModel* EmployeSQL::afficher()
{
    QSqlQueryModel* model = new QSqlQueryModel();

    QSqlQuery query;

    query.prepare(
        "SELECT ID_EMPLOYR, NOM, PRENOM, EMAIL, ADRESSE, "
        "TELEPHONE, SALAIRE, DATE_EMBAUCHE, STATUT, POSTE, "
        "MOY_DE_PASSE "
        "FROM EMPLOYE"
        );

    if (!query.exec())
    {
        qDebug() << "Erreur affichage :"
                 << query.lastError().text();

        return model;
    }

    model->setQuery(std::move(query));

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Prénom"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Email"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Adresse"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Téléphone"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Salaire"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("Date embauche"));
    model->setHeaderData(8, Qt::Horizontal, QObject::tr("Statut"));
    model->setHeaderData(9, Qt::Horizontal, QObject::tr("Poste"));
    model->setHeaderData(10, Qt::Horizontal, QObject::tr("Mot de passe"));

    return model;
}


#include <QBoxLayout>
#include <QFrame>
#include <QPushButton>
#include <QSqlQueryModel>

namespace {

QBoxLayout *trouverLayout(QLayout *l, QWidget *w, int *index)
{
    if (!l)
        return nullptr;
    const int i = l->indexOf(w);
    if (i >= 0) {
        *index = i;
        return qobject_cast<QBoxLayout *>(l);
    }
    for (int k = 0; k < l->count(); ++k) {
        if (QLayout *sous = l->itemAt(k)->layout())
            if (QBoxLayout *r = trouverLayout(sous, w, index))
                return r;
    }
    return nullptr;
}

} // namespace

Connexion::Connexion(Ui::GSmartPetCare *ui, BaseEmployes &base, QWidget *fenetre)
    : QObject(fenetre)
    , ui(ui)
    , m_fenetre(fenetre)
    , m_base(base)
{
    connect(ui->pushButtonConnexion, &QPushButton::clicked, this, &Connexion::onConnecter);

    QWidget *parent = ui->pushButtonConnexion->parentWidget();
    m_btnVisage = new QPushButton("Se connecter avec le visage", parent);
    m_btnVisage->setObjectName("btnConnexionVisage");
    m_btnVisage->setCursor(Qt::PointingHandCursor);
    m_btnVisage->setMinimumHeight(52);
    m_btnVisage->setStyleSheet(
        "QPushButton { background: #FFFFFF; color: #0F766E; border: 2px solid #0F766E;"
        "              border-radius: 8px; font-weight: bold; font-size: 14px; }"
        "QPushButton:hover { background: #E6F4F1; }");

    int index = -1;
    QBoxLayout *lay = trouverLayout(parent ? parent->layout() : nullptr, ui->pushButtonConnexion, &index);
    if (lay)
        lay->insertWidget(index + 1, m_btnVisage);
    else if (auto *carte = qobject_cast<QBoxLayout *>(ui->frameCard->layout()))
        carte->addWidget(m_btnVisage);

    connect(m_btnVisage, &QPushButton::clicked, this, &Connexion::onConnecterVisage);
}

void Connexion::afficherErreur(const QString &message)
{
    ui->labelErreurConnexion->setText(message);
    ui->labelErreurConnexion->show();
}

void Connexion::echec(const QString &message)
{
    ++m_essais;
    if (m_essais >= 5) {
        emit abandon();                                   // 5 échecs : l'application se ferme
        return;
    }
    afficherErreur(QString("%1 (essai %2/5)").arg(message).arg(m_essais));
}

void Connexion::reussir(int employeId)
{
    ui->lineEditMotDePasse->clear();              // rien ne reste affiché une fois connecté
    ui->labelErreurConnexion->hide();
    m_essais = 0;
    emit connecte(employeId);
}

void Connexion::chargerEmployes()
{
    QSqlQueryModel *modele = EmployeSQL::afficher();
    if (!modele || modele->rowCount() == 0)
        return;

    m_base.employes.clear();
    int idMax = 0;
    for (int i = 0; i < modele->rowCount(); ++i) {
        Employe e;
        e.id = modele->data(modele->index(i, 0)).toInt();
        e.nom = modele->data(modele->index(i, 1)).toString();
        e.prenom = modele->data(modele->index(i, 2)).toString();
        e.email = modele->data(modele->index(i, 3)).toString();
        e.adresse = modele->data(modele->index(i, 4)).toString();
        e.telephone = modele->data(modele->index(i, 5)).toString();
        e.salaire = modele->data(modele->index(i, 6)).toDouble();
        e.dateEmbauche = modele->data(modele->index(i, 7)).toDate();
        e.statut = modele->data(modele->index(i, 8)).toString();
        e.poste = modele->data(modele->index(i, 9)).toString();
        e.motDePasse = modele->data(modele->index(i, 10)).toString();
        m_base.employes.append(e);
        if (e.id > idMax)
            idMax = e.id;
    }
    if (idMax >= m_base.prochainId)
        m_base.prochainId = idMax + 1;

    FaceAuth::chargerGabarits(m_base);
}

const Employe *Connexion::trouverEmploye(const QString &identifiant) const
{
    const QString id = identifiant.trimmed();
    if (id.contains('@'))
        return m_base.trouverParEmail(id);
    bool ok = false;
    const int n = id.toInt(&ok);
    return ok ? m_base.trouver(n) : nullptr;
}

void Connexion::onConnecter()
{
    const QString identifiant = ui->lineEditIdentifiant->text().trimmed();
    const QString mdp = ui->lineEditMotDePasse->text();
    if (identifiant.isEmpty() || mdp.isEmpty()) {
        afficherErreur("Saisissez votre identifiant et votre mot de passe.");
        return;
    }

    chargerEmployes();

    const Employe *e = trouverEmploye(identifiant);
    if (!e || e->motDePasse.isEmpty() || !(e->motDePasse == mdp || Emp::verifier(mdp, e->motDePasse))) {
        echec("Identifiant ou mot de passe incorrect.");
        return;
    }
    if (e->statut != "Actif") {
        echec("Ce compte est inactif.");
        return;
    }
    reussir(e->id);
}

void Connexion::onConnecterVisage()
{
    chargerEmployes();

    const int id = FaceAuth::authentifier(ui, m_fenetre, m_base.employes);
    if (id < 0)
        return;

    const Employe *e = m_base.trouver(id);
    if (!e) {
        echec("Visage non reconnu.");
        return;
    }
    if (e->statut != "Actif") {
        echec("Ce compte est inactif.");
        return;
    }
    reussir(e->id);
}


#include <QAbstractItemView>
#include <QColor>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>

Privileges::Privileges(Ui::GSmartPetCare *ui, BaseEmployes &base, QWidget *fenetre)
    : QObject(fenetre)
    , ui(ui)
    , m_fenetre(fenetre)
    , m_base(base)
    , m_postes(Emp::postes())
{
    QTableWidget *table = ui->tableRoles;
    table->horizontalHeader()->setFixedHeight(44);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    connect(ui->btnAnnulerPrivileges, &QPushButton::clicked, this, &Privileges::termine);
}

void Privileges::ouvrir()
{
    QTableWidget *table = ui->tableRoles;
    table->clearContents();
    table->setRowCount(m_postes.size());
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);      // consultation seule

    const char *infobulles[NbDroits] = {
        "Créer de nouveaux employés (et enregistrer leur visage)",
        "Modifier les fiches employés (et enregistrer leur visage)",
        "Supprimer des employés",
        "Voir les salaires et trier par salaire ; sans ce droit la colonne et le champ sont masqués",
        "Exporter la liste en PDF",
        "Ouvrir les statistiques",
    };

    for (int r = 0; r < m_postes.size(); ++r) {
        const QString &poste = m_postes[r];
        int nb = 0;
        for (const Employe &e : std::as_const(m_base.employes))
            if (e.poste == poste)
                ++nb;

        table->setItem(r, 0, new QTableWidgetItem(poste));
        auto *itNb = new QTableWidgetItem(QString::number(nb));
        itNb->setTextAlignment(Qt::AlignCenter);
        table->setItem(r, 1, itNb);

        const Droits d = Emp::droitsParDefaut(poste);
        const bool valeurs[NbDroits] = {d.ajouter, d.modifier, d.supprimer, d.voirSalaires, d.exporter, d.statistiques};
        for (int c = 0; c < NbDroits; ++c) {
            auto *it = new QTableWidgetItem(valeurs[c] ? QStringLiteral("✔") : QStringLiteral("✘"));
            it->setTextAlignment(Qt::AlignCenter);
            it->setForeground(QColor(valeurs[c] ? "#0F766E" : "#C0392B"));
            it->setToolTip(QString::fromUtf8(infobulles[c]));
            table->setItem(r, 2 + c, it);
        }
    }
}


#include "barchart.h"
#include "donutchart.h"
#include "linechart.h"

#include <QMap>

#include <algorithm>

StatistiquesEmployes::StatistiquesEmployes(Ui::GSmartPetCare *ui, QObject *parent)
    : QObject(parent)
    , ui(ui)
    , m_voirSalaires(false)
{
    ui->donutStatut->setUnite("employés");
    connect(ui->btnRetourStats, &QPushButton::clicked, this, &StatistiquesEmployes::retour);
}

void StatistiquesEmployes::afficher(const QVector<Employe> &employes, bool voirSalaires)
{
    m_employes = employes;
    m_voirSalaires = voirSalaires;

    ui->carteSalaire->setVisible(m_voirSalaires);
    ui->gridCartes->removeWidget(ui->carteRecrutement);
    if (m_voirSalaires)
        ui->gridCartes->addWidget(ui->carteRecrutement, 1, 1);
    else
        ui->gridCartes->addWidget(ui->carteRecrutement, 1, 0, 1, 2);
    calculer();
}

void StatistiquesEmployes::calculer()
{
    QMap<QString, int> parStatut;
    QMap<QString, int> parPoste;
    QMap<int, int> parAnnee;

    for (const Employe &e : m_employes) {
        parStatut[e.statut]++;
        parPoste[e.poste]++;
        if (e.dateEmbauche.isValid())
            parAnnee[e.dateEmbauche.year()]++;
    }

    ui->donutStatut->setData(parStatut);

    QVector<QPair<QString, double>> postes;
    for (auto it = parPoste.constBegin(); it != parPoste.constEnd(); ++it)
        postes.append({it.key(), double(it.value())});
    ui->barresPoste->setData(postes, BarChart::Horizontal, QString(), QColor("#F28C28"));

    if (m_voirSalaires && !m_employes.isEmpty()) {
        double somme = 0, mini = m_employes.first().salaire, maxi = mini;
        for (const Employe &e : std::as_const(m_employes)) {
            somme += e.salaire;
            mini = std::min(mini, e.salaire);
            maxi = std::max(maxi, e.salaire);
        }
        QVector<QPair<QString, double>> salaires = {
            {QStringLiteral("Moyen"), somme / m_employes.size()},
            {QStringLiteral("Minimum"), mini},
            {QStringLiteral("Maximum"), maxi}
        };
        ui->barresSalaire->setData(salaires, BarChart::Vertical, "DT", QColor("#0F766E"));
    }

    QVector<QPair<QString, double>> annees;
    if (!parAnnee.isEmpty()) {
        for (int a = parAnnee.firstKey(); a <= parAnnee.lastKey(); ++a)
            annees.append({QString::number(a), double(parAnnee.value(a, 0))});
    }
    ui->courbeRecrutement->setData(annees, QColor("#0F766E"));
}


#include <QComboBox>
#include <QDateEdit>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QFont>
#include <QTextDocument>
#include <QVBoxLayout>

#include <algorithm>

namespace {

enum Colonne { ColId = 0, ColNom, ColPrenom, ColEmail, ColPoste, ColStatut, ColSalaire, ColDate, ColMdp, ColActions };

QVector<QVector<float>> g_gabaritsAttente;

bool capturerVisage(Ui::GSmartPetCare *ui, QWidget *parent, const QVector<Employe> &tous,
                    const QString &titre, QVector<QVector<float>> &sortie)
{
    if (!FaceEngine::compileAvecOpenCV()) {
        QMessageBox::information(parent, "Reconnaissance faciale",
                                 "Cette version a été compilée sans OpenCV : la reconnaissance faciale "
                                 "est désactivée.");
        return false;
    }

    FaceDialog dlg(ui, FaceDialog::Enregistrement, nullptr, titre, parent);
    QString erreur;
    if (!dlg.demarrer(&erreur)) {
        QMessageBox::critical(parent, "Reconnaissance faciale", erreur);
        return false;
    }
    if (!dlg.exec())
        return false;

    for (const QVector<float> &g : dlg.gabarits()) {
        const FaceMatch::Resultat r = FaceMatch::identifier(g, tous);
        if (r.score >= FaceMatch::kSeuil) {
            QMessageBox::warning(parent, "Visage déjà enregistré",
                                 "Ce visage ressemble trop à celui d'un autre employé : enregistrement refusé.");
            return false;
        }
    }
    sortie = dlg.gabarits();
    return true;
}

} // namespace

GestionEmployes::GestionEmployes(Ui::GSmartPetCare *ui, BaseEmployes &base, QWidget *fenetre)
    : QObject(fenetre)
    , m_base(base)
    , ui(ui)
    , m_fenetre(fenetre)
{
    configurerInterface();
}

void GestionEmployes::configurerInterface()
{
    const char *criteres[] = {"tout", "id", "nom", "prenom"};      // recherche : ID, nom, prénom
    for (int i = 0; i < 4; ++i)
        ui->comboCritere->setItemData(i, criteres[i]);
    while (ui->comboTri->count() > 3)
        ui->comboTri->removeItem(ui->comboTri->count() - 1);
    const char *tris[] = {"nom", "salaire", "date"};
    for (int i = 0; i < 3; ++i)
        ui->comboTri->setItemData(i, tris[i]);

    ui->editTelephone->setMaxLength(8);
    ui->editTelephone->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]{0,8}"), ui->editTelephone));

    ui->comboFiltreStatut->addItems(Emp::statuts());
    ui->comboStatut->addItems(Emp::statuts());
    ui->comboPoste->addItems(Emp::postes());
    ui->dateEmbauche->setDate(QDate::currentDate());
    ui->dateEmbauche->setMaximumDate(QDate::currentDate());

    QHeaderView *h = ui->tableEmployes->horizontalHeader();
    h->setMinimumSectionSize(38);
    h->setStretchLastSection(false);
    h->setFixedHeight(44);
    ui->tableEmployes->setColumnCount(10);
    ui->tableEmployes->setHorizontalHeaderLabels({"ID", "Nom", "Prénom", "Email", "Poste", "Statut",
                                                  "Salaire (DT)", "Embauche", "Mot de passe", "Actions"});
    h->setSectionResizeMode(ColEmail, QHeaderView::Stretch);
    h->setSectionResizeMode(ColId, QHeaderView::Fixed);        h->resizeSection(ColId, 38);
    h->setSectionResizeMode(ColNom, QHeaderView::Stretch);
    h->setSectionResizeMode(ColPrenom, QHeaderView::Stretch);
    h->setSectionResizeMode(ColPoste, QHeaderView::Fixed);     h->resizeSection(ColPoste, 140);
    h->setSectionResizeMode(ColStatut, QHeaderView::Fixed);    h->resizeSection(ColStatut, 66);
    h->setSectionResizeMode(ColSalaire, QHeaderView::Fixed);   h->resizeSection(ColSalaire, 84);
    h->setSectionResizeMode(ColDate, QHeaderView::Fixed);      h->resizeSection(ColDate, 92);
    h->setSectionResizeMode(ColMdp, QHeaderView::Fixed);       h->resizeSection(ColMdp, 92);
    h->setSectionResizeMode(ColActions, QHeaderView::Fixed);   h->resizeSection(ColActions, 175);

    ui->btnAjouter->hide();                     // le bouton « Ajouter » est supprimé : le formulaire est toujours prêt
    connect(ui->editRecherche, &QLineEdit::textChanged, this, &GestionEmployes::refreshTable);
    connect(ui->comboCritere, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GestionEmployes::refreshTable);
    connect(ui->comboTri, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GestionEmployes::refreshTable);
    connect(ui->comboFiltreStatut, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GestionEmployes::refreshTable);
    connect(ui->comboOrdre, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GestionEmployes::refreshTable);
    connect(ui->tableEmployes, &QTableWidget::itemSelectionChanged, this, &GestionEmployes::onSelectionChanged);
    connect(ui->btnEnregistrer, &QPushButton::clicked, this, &GestionEmployes::onEnregistrer);
    connect(ui->btnAnnuler, &QPushButton::clicked, this, &GestionEmployes::onAnnuler);
    connect(ui->btnPdf, &QPushButton::clicked, this, &GestionEmployes::onExporterPdf);
    connect(ui->btnStats, &QPushButton::clicked, this, &GestionEmployes::onStatistiques);
    connect(ui->btnPrivileges, &QPushButton::clicked, this, &GestionEmployes::onPrivileges);
    connect(ui->btnVisage, &QPushButton::clicked, this, &GestionEmployes::onEnregistrerVisage);
    connect(ui->btnSupprVisage, &QPushButton::clicked, this, &GestionEmployes::onSupprimerVisage);
}

void GestionEmployes::montrerChamp(QWidget *champ, bool visible)
{
    champ->setVisible(visible);
    if (QWidget *etiquette = ui->formLayout->labelForField(champ))
        etiquette->setVisible(visible);
}

void GestionEmployes::setUtilisateur(int employeId)
{
    m_userId = employeId;
    const Employe *u = m_base.trouver(employeId);
    m_droits = u ? m_base.droits(u->poste) : Droits();
    ui->labelUtilisateur->setText(u ? QString("Connecté : %1 %2 (%3)").arg(u->prenom, u->nom, u->poste)
                                    : QString());

    appliquerDroits();
    m_currentId = -1;
    viderFormulaire();
    refreshTable();
}

void GestionEmployes::appliquerDroits()
{
    const bool editable = m_droits.ajouter || m_droits.modifier;

    ui->btnAjouter->setVisible(false);
    ui->btnPdf->setVisible(m_droits.exporter);
    ui->btnStats->setVisible(m_droits.statistiques);
    ui->btnPrivileges->setVisible(m_droits.gererPrivileges);

    ui->tableEmployes->setColumnHidden(ColSalaire, !m_droits.voirSalaires);
    ui->tableEmployes->setColumnHidden(ColMdp, !m_droits.modifier);
    ui->tableEmployes->setColumnHidden(ColActions, !(m_droits.modifier || m_droits.supprimer));
    montrerChamp(ui->editSalaire, m_droits.voirSalaires);
    montrerChamp(ui->editMdp, editable);

    {
        const QSignalBlocker blocker(ui->comboTri);
        const int iSalaire = ui->comboTri->findData("salaire");
        if (m_droits.voirSalaires && iSalaire < 0)
            ui->comboTri->insertItem(1, "Trier par : salaire", "salaire");
        else if (!m_droits.voirSalaires && iSalaire >= 0)
            ui->comboTri->removeItem(iSalaire);
        ui->comboTri->setCurrentIndex(0);                   // retour au tri par nom
    }
    {
        const QSignalBlocker blocker(ui->comboOrdre);
        ui->comboOrdre->setCurrentIndex(0);                 // croissant
    }

    for (QLineEdit *l : {ui->editNom, ui->editPrenom, ui->editEmail, ui->editAdresse, ui->editTelephone, ui->editSalaire})
        l->setReadOnly(!editable);
    for (QWidget *w : {static_cast<QWidget *>(ui->dateEmbauche), static_cast<QWidget *>(ui->comboStatut),
                       static_cast<QWidget *>(ui->comboPoste)})
        w->setEnabled(editable);
    ui->btnEnregistrer->setVisible(editable);
    ui->btnAnnuler->setVisible(editable);
}

void GestionEmployes::refreshTable()
{
    QSqlQueryModel *modelSQL = EmployeSQL::afficher();

    qDebug() << "Nombre de lignes Oracle :" << modelSQL->rowCount();
    m_base.employes.clear();

    for (int i = 0; i < modelSQL->rowCount(); ++i)
    {
        Employe e;

        e.id = modelSQL->data(modelSQL->index(i, 0)).toInt();
        e.nom = modelSQL->data(modelSQL->index(i, 1)).toString();
        e.prenom = modelSQL->data(modelSQL->index(i, 2)).toString();
        e.email = modelSQL->data(modelSQL->index(i, 3)).toString();
        e.adresse = modelSQL->data(modelSQL->index(i, 4)).toString();
        e.telephone = modelSQL->data(modelSQL->index(i, 5)).toString();
        e.salaire = modelSQL->data(modelSQL->index(i, 6)).toDouble();
        e.dateEmbauche = modelSQL->data(modelSQL->index(i, 7)).toDate();
        e.statut = modelSQL->data(modelSQL->index(i, 8)).toString();
        e.poste = modelSQL->data(modelSQL->index(i, 9)).toString();
        e.motDePasse = modelSQL->data(modelSQL->index(i, 10)).toString();

        m_base.employes.append(e);
    }

    FaceAuth::chargerGabarits(m_base);

    const QString q = ui->editRecherche->text().trimmed();
    const QString critere = ui->comboCritere->currentData().toString();
    const QString statutFiltre = ui->comboFiltreStatut->currentIndex() > 0 ? ui->comboFiltreStatut->currentText() : QString();

    auto contient = [&q](const QString &s) { return s.startsWith(q, Qt::CaseInsensitive); };
    auto correspond = [&](const Employe &e) {
        if (q.isEmpty())
            return true;
        if (critere == "id")     return contient(QString::number(e.id));
        if (critere == "nom")    return contient(e.nom);
        if (critere == "prenom") return contient(e.prenom);
        return contient(QString::number(e.id)) || contient(e.nom) || contient(e.prenom);   // « tout » = ID, nom, prénom
    };

    QVector<Employe> vue;
    for (const Employe &e : std::as_const(m_base.employes)) {
        if (!statutFiltre.isEmpty() && e.statut != statutFiltre)
            continue;
        if (correspond(e))
            vue.append(e);
    }

    const QString cle = ui->comboTri->currentData().toString();
    const bool decroissant = ui->comboOrdre->currentIndex() == 1;
    std::stable_sort(vue.begin(), vue.end(), [&](const Employe &a, const Employe &b) {
        int c = 0;
        if (cle == "salaire")
            c = a.salaire < b.salaire ? -1 : (a.salaire > b.salaire ? 1 : 0);
        else if (cle == "date")
            c = a.dateEmbauche < b.dateEmbauche ? -1 : (a.dateEmbauche > b.dateEmbauche ? 1 : 0);
        else if (cle == "poste")
            c = QString::localeAwareCompare(a.poste, b.poste);
        else if (cle == "statut")
            c = QString::localeAwareCompare(a.statut, b.statut);
        else
            c = QString::localeAwareCompare(a.nom, b.nom);
        if (c == 0 && cle != "nom")
            c = QString::localeAwareCompare(a.nom, b.nom);          // départage par nom
        if (c == 0)
            c = a.id < b.id ? -1 : (a.id > b.id ? 1 : 0);
        return decroissant ? c > 0 : c < 0;
    });
    m_vue = vue;

    QTableWidget *t = ui->tableEmployes;
    {
        const QSignalBlocker blocker(t);
        t->clearSpans();
        t->clearContents();
        t->setRowCount(vue.size());
        int ligneSelection = -1;

        for (int r = 0; r < vue.size(); ++r) {
            const Employe &e = vue[r];

            auto centre = [](const QString &txt) {
                auto *it = new QTableWidgetItem(txt);
                it->setTextAlignment(Qt::AlignCenter);
                return it;
            };

            auto *itId = centre(QString::number(e.id));
            itId->setData(Qt::UserRole, e.id);

            auto *itStatut = centre(e.statut);
            itStatut->setForeground(QColor(e.statut == "Actif" ? "#0F766E" : "#C0392B"));

            t->setItem(r, ColId, itId);
            t->setItem(r, ColNom, new QTableWidgetItem(e.nom));
            t->setItem(r, ColPrenom, new QTableWidgetItem(e.prenom));
            t->setItem(r, ColEmail, new QTableWidgetItem(e.email));
            t->setItem(r, ColPoste, new QTableWidgetItem(e.poste));
            t->setItem(r, ColStatut, itStatut);
            t->setItem(r, ColSalaire, centre(QString::number(e.salaire, 'f', 2)));
            t->setItem(r, ColDate, centre(e.dateEmbauche.toString("dd/MM/yyyy")));
            t->setItem(r, ColMdp, centre(e.motDePasse.isEmpty() ? QString("Non défini")
                                                                : QString(8, QChar(0x2022))));
            t->setItem(r, ColActions, new QTableWidgetItem());
            t->setCellWidget(r, ColActions, creerBoutonsActions(e.id));

            if (e.id == m_currentId)
                ligneSelection = r;
        }

        if (ligneSelection >= 0)
            t->selectRow(ligneSelection);
        else
            t->clearSelection();
    }
}

QWidget *GestionEmployes::creerBoutonsActions(int employeId)
{
    auto *conteneur = new QWidget;
    auto *lay = new QHBoxLayout(conteneur);
    lay->setContentsMargins(6, 4, 6, 4);
    lay->setSpacing(8);

    if (m_droits.modifier) {
        auto *btn = new QPushButton("Modifier", conteneur);
        btn->setObjectName("btnModifierLigne");
        btn->setFixedHeight(28);
        btn->setCursor(Qt::PointingHandCursor);
        lay->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, [this, employeId]() { onModifier(employeId); });
    }
    if (m_droits.supprimer) {
        auto *btn = new QPushButton("Supprimer", conteneur);
        btn->setObjectName("btnSupprimerLigne");
        btn->setFixedHeight(28);
        btn->setCursor(Qt::PointingHandCursor);
        lay->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, [this, employeId]() { onSupprimer(employeId); });
    }
    return conteneur;
}

void GestionEmployes::viderFormulaire()
{
    ui->labelDetailTitreEmp->setText(m_droits.ajouter ? "Nouvel employé" : "Détails de l'employé");
    ui->editId->clear();
    ui->editNom->clear();
    ui->editPrenom->clear();
    ui->editEmail->clear();
    ui->editAdresse->clear();
    ui->editTelephone->clear();
    ui->editSalaire->clear();
    ui->dateEmbauche->setDate(QDate::currentDate());
    ui->comboStatut->setCurrentIndex(0);
    ui->comboPoste->setCurrentIndex(0);
    ui->editMdp->clear();
    ui->editMdp->setPlaceholderText("Obligatoire (6 caractères min.)");
    g_gabaritsAttente.clear();                              // pas de visage en attente pour un formulaire vide
    majEtatVisage(-1);
}

void GestionEmployes::remplirFormulaire(const Employe &e)
{
    ui->labelDetailTitreEmp->setText("Détails de l'employé");
    ui->editId->setText(QString::number(e.id));
    ui->editNom->setText(e.nom);
    ui->editPrenom->setText(e.prenom);
    ui->editEmail->setText(e.email);
    ui->editAdresse->setText(e.adresse);
    ui->editTelephone->setText(e.telephone);
    ui->editSalaire->setText(QString::number(e.salaire, 'f', 2));
    ui->dateEmbauche->setDate(e.dateEmbauche);
    ui->comboStatut->setCurrentText(e.statut);
    ui->comboPoste->setCurrentText(e.poste);
    ui->editMdp->clear();                                   // le mot de passe n'est jamais réaffiché
    ui->editMdp->setPlaceholderText("Laisser vide pour le conserver");
    g_gabaritsAttente.clear();
    majEtatVisage(e.id);
}

void GestionEmployes::majEtatVisage(int employeId)
{
    const Employe *e = m_base.trouver(employeId);
    const bool nouveau = (employeId < 0) && m_droits.ajouter;
    const bool visible = nouveau || (e && (m_droits.modifier || e->id == m_userId));
    ui->labelVisage->setVisible(visible);
    ui->btnVisage->setVisible(visible);
    ui->btnSupprVisage->setVisible(visible);
    if (!visible)
        return;
    const int n = nouveau ? int(g_gabaritsAttente.size()) : int(e->gabaritsVisage.size());
    ui->labelVisage->setText(n > 0 ? QString("Visage : enregistré (%1 échantillons)").arg(n)
                                   : QString("Visage : non enregistré"));
    ui->btnVisage->setText(n > 0 ? "Réenregistrer le visage" : "Enregistrer le visage");
    ui->btnSupprVisage->setEnabled(n > 0);
}

void GestionEmployes::onSelectionChanged()
{
}

void GestionEmployes::onAjouter()
{
    if (!m_droits.ajouter)
        return;
    m_currentId = -1;
    ui->tableEmployes->clearSelection();
    viderFormulaire();
    ui->editNom->setFocus();
}

void GestionEmployes::onModifier(int employeId)
{
    if (!m_droits.modifier)
        return;
    const Employe *e = m_base.trouver(employeId);
    if (!e)
        return;
    m_currentId = employeId;
    remplirFormulaire(*e);

    for (int r = 0; r < ui->tableEmployes->rowCount(); ++r) {
        if (ui->tableEmployes->item(r, 0) && ui->tableEmployes->item(r, 0)->data(Qt::UserRole).toInt() == employeId) {
            ui->tableEmployes->selectRow(r);
            break;
        }
    }
    ui->editNom->setFocus();
    ui->editNom->selectAll();
}

void GestionEmployes::onSupprimer(int employeId)
{
    if (!m_droits.supprimer)
        return;

    const Employe *e = m_base.trouver(employeId);

    if (!e)
        return;


    if (employeId == m_userId)
    {
        QMessageBox::warning(
            m_fenetre,
            "Suppression impossible",
            "Vous ne pouvez pas supprimer votre propre compte."
            );
        return;
    }

    if (e->poste == "Responsable RH"
        && m_base.nombreResponsablesRH() <= 1)
    {
        QMessageBox::warning(
            m_fenetre,
            "Suppression impossible",
            "Il doit toujours rester au moins un Responsable RH."
            );
        return;
    }


    if (QMessageBox::question(
            m_fenetre,
            "Supprimer",
            QString("Supprimer l'employé « %1 %2 » ?")
                .arg(e->prenom, e->nom)
            ) != QMessageBox::Yes)
    {
        return;
    }


    EmployeSQL sql;

    sql.id = employeId;

    if (!sql.supprimer())
    {
        QMessageBox::critical(
            m_fenetre,
            "Erreur",
            "Impossible de supprimer l'employé de la base de données."
            );

        return;
    }


    m_base.employes.erase(
        std::remove_if(
            m_base.employes.begin(),
            m_base.employes.end(),
            [employeId](const Employe &x)
            {
                return x.id == employeId;
            }
            ),
        m_base.employes.end()
        );


    m_currentId = -1;
    viderFormulaire();

    FaceAuth::sauvegarderGabarits(m_base);

    refreshTable();
}

void GestionEmployes::onEnregistrer()
{
    const bool nouveau = (m_currentId < 0);

    if (nouveau ? !m_droits.ajouter : !m_droits.modifier)
        return;

    auto erreur = [this](const QString &titre,
                         const QString &msg,
                         QWidget *champ)
    {
        QMessageBox::warning(m_fenetre, titre, msg);
        champ->setFocus();
    };

    const QString nom = ui->editNom->text().trimmed();
    const QString prenom = ui->editPrenom->text().trimmed();
    const QString email = ui->editEmail->text().trimmed();
    const QString tel = ui->editTelephone->text().trimmed();
    const QString mdp = ui->editMdp->text();


    static const QRegularExpression reNom("^[A-Z][a-z]+$");

    if (nom.isEmpty())
        return erreur(
            "Champ manquant",
            "Le nom est obligatoire.",
            ui->editNom
            );

    if (!reNom.match(nom).hasMatch())
        return erreur(
            "Nom invalide",
            "Le nom doit contenir uniquement des lettres (a-z) et commencer par une majuscule (ex. Kadhefi).",
            ui->editNom
            );

    if (prenom.isEmpty())
        return erreur(
            "Champ manquant",
            "Le prénom est obligatoire.",
            ui->editPrenom
            );

    if (!reNom.match(prenom).hasMatch())
        return erreur(
            "Prénom invalide",
            "Le prénom doit contenir uniquement des lettres (a-z) et commencer par une majuscule (ex. Elaa).",
            ui->editPrenom
            );

    static const QRegularExpression reEmail(
        "^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$"
        );

    if (!reEmail.match(email).hasMatch())
        return erreur(
            "Email invalide",
            "Saisissez un email valide (ex. prenom.nom@petnova.tn).",
            ui->editEmail
            );

    const Employe *memeEmail = m_base.trouverParEmail(email);

    if (memeEmail && memeEmail->id != m_currentId)
        return erreur(
            "Email déjà utilisé",
            "Un autre employé utilise déjà cet email.",
            ui->editEmail
            );

    static const QRegularExpression reTel(
        "^[0-9]{8}$"
        );

    if (!reTel.match(tel).hasMatch())
        return erreur(
            "Téléphone invalide",
            "Le numéro de téléphone doit contenir exactement 8 chiffres.",
            ui->editTelephone
            );

    double salaire = 0.0;

    if (m_droits.voirSalaires)
    {
        bool ok = false;

        QString txt = ui->editSalaire->text().trimmed();
        txt.replace(',', '.');

        salaire = txt.toDouble(&ok);

        if (!ok || salaire < 0)
            return erreur(
                "Salaire invalide",
                "Saisissez un salaire valide (ex. 1800 ou 1800.50).",
                ui->editSalaire
                );
    }


    if (nouveau && mdp.length() < 6)
        return erreur(
            "Mot de passe invalide",
            "Le mot de passe doit contenir au moins 6 caractères.",
            ui->editMdp
            );

    if (!nouveau && !mdp.isEmpty() && mdp.length() < 6)
        return erreur(
            "Mot de passe invalide",
            "Le mot de passe doit contenir au moins 6 caractères.",
            ui->editMdp
            );


    Employe *existant = nouveau
                            ? nullptr
                            : m_base.trouver(m_currentId);

    if (existant)
    {
        if (existant->id == m_userId
            && (ui->comboPoste->currentText() != existant->poste
                || ui->comboStatut->currentText() != existant->statut))
        {
            QMessageBox::warning(
                m_fenetre,
                "Action refusée",
                "Vous ne pouvez pas modifier votre propre poste ou votre propre statut."
                );
            return;
        }

        if (existant->poste == "Responsable RH"
            && ui->comboPoste->currentText() != "Responsable RH"
            && m_base.nombreResponsablesRH() <= 1)
        {
            QMessageBox::warning(
                m_fenetre,
                "Action refusée",
                "Il doit toujours rester au moins un Responsable RH."
                );
            return;
        }
    }


    Employe e = existant ? *existant : Employe();

    e.nom = nom;
    e.prenom = prenom;
    e.email = email;
    e.adresse = ui->editAdresse->text().trimmed();
    e.telephone = tel;

    if (m_droits.voirSalaires)
        e.salaire = salaire;

    e.dateEmbauche = ui->dateEmbauche->date();
    e.statut = ui->comboStatut->currentText();
    e.poste = ui->comboPoste->currentText();


    if (!mdp.isEmpty())
    {
        e.motDePasse = Emp::hacher(mdp);
    }
    else if (!nouveau && existant)
    {
        e.motDePasse = existant->motDePasse;
    }


    if (nouveau)
    {
        e.id = m_base.prochainId++;

        e.gabaritsVisage = g_gabaritsAttente;

        EmployeSQL sql(
            e.id,
            e.nom,
            e.prenom,
            e.email,
            e.adresse,
            e.telephone,
            e.salaire,
            e.dateEmbauche,
            e.statut,
            e.poste,
            e.motDePasse
            );

        if (!sql.ajouter())
        {
            QMessageBox::critical(
                m_fenetre,
                "Erreur",
                "Impossible d'ajouter l'employé dans la base de données."
                );

            return;
        }

        m_base.employes.append(e);

        m_currentId = e.id;
    }
    else
    {
        EmployeSQL sql(
            e.id,
            e.nom,
            e.prenom,
            e.email,
            e.adresse,
            e.telephone,
            e.salaire,
            e.dateEmbauche,
            e.statut,
            e.poste,
            e.motDePasse
            );

        if (!sql.modifier())
        {
            QMessageBox::critical(
                m_fenetre,
                "Erreur",
                "Impossible de modifier l'employé dans la base de données."
                );

            return;
        }

        *existant = e;
    }


    FaceAuth::sauvegarderGabarits(m_base);

    QMessageBox::information(
        m_fenetre,
        nouveau ? "Employé ajouté" : "Employé modifié",
        nouveau ? "L'employé a été ajouté avec succès."
                : "L'employé a été modifié avec succès."
        );

    m_currentId = -1;
    viderFormulaire();

    refreshTable();
}

void GestionEmployes::onAnnuler()
{
    m_currentId = -1;
    ui->tableEmployes->clearSelection();
    viderFormulaire();
}

GestionEmployes::DonneesExport GestionEmployes::donneesExport() const
{
    DonneesExport d;
    d.entetes = {"ID", "Nom", "Prénom", "Email", "Adresse", "Téléphone", "Poste", "Statut", "Date d'embauche"};
    if (m_droits.voirSalaires)
        d.entetes << "Salaire (DT)";

    for (const Employe &e : m_vue) {
        QStringList l = {QString::number(e.id), e.nom, e.prenom, e.email, e.adresse, e.telephone,
                         e.poste, e.statut, e.dateEmbauche.toString("dd/MM/yyyy")};
        if (m_droits.voirSalaires)
            l << QString::number(e.salaire, 'f', 2);
        d.lignes.append(l);
    }
    return d;
}

QString GestionEmployes::htmlTableau() const
{
    const DonneesExport d = donneesExport();
    const QString texte = QStringLiteral("#1F2A37");
    const QString turquoise = QStringLiteral("#0F766E");

    QString html = QStringLiteral("<div style='font-family:Arial; color:%1;'>").arg(texte);
    html += QStringLiteral("<h1 style='font-family:Arial; font-size:20pt; font-weight:bold; color:%1;'>"
                           "Liste des employés - Pet Nova</h1>").arg(turquoise);
    html += QStringLiteral("<p style='font-size:11pt;'>Date d'exportation : <b>%1</b><br>"
                           "Nombre total d'employés : <b>%2</b></p>")
                .arg(QDate::currentDate().toString("dd/MM/yyyy"))
                .arg(d.lignes.size());

    html += QStringLiteral("<table border='0' cellspacing='0' cellpadding='5' width='100%' "
                           "style='font-family:Arial; font-size:9pt; border-collapse:collapse;'>"
                           "<tr style='background-color:%1; color:#FFFFFF;'>").arg(turquoise);
    for (const QString &en : d.entetes)
        html += "<th align='left'>" + en.toHtmlEscaped() + "</th>";
    html += "</tr>";
    int n = 0;
    for (const QStringList &l : d.lignes) {
        html += QStringLiteral("<tr style='background-color:%1;'>").arg(n++ % 2 ? "#F4F8F7" : "#FFFFFF");
        for (const QString &c : l)
            html += QStringLiteral("<td style='border-bottom:1px solid #CBD5D3;'>") + c.toHtmlEscaped() + "</td>";
        html += "</tr>";
    }
    html += "</table></div>";
    return html;
}

void GestionEmployes::onExporterPdf()
{
    if (!m_droits.exporter)
        return;
    QString chemin = QFileDialog::getSaveFileName(m_fenetre, "Exporter en PDF", "employes.pdf", "Fichiers PDF (*.pdf)");
    if (chemin.isEmpty())
        return;
    if (!chemin.endsWith(".pdf", Qt::CaseInsensitive))
        chemin += ".pdf";

    QTextDocument doc;
    doc.setDefaultFont(QFont("Arial", 9));
    doc.setHtml(htmlTableau());

    QPdfWriter writer(chemin);
    writer.setPageLayout(QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Landscape,
                                     QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter));
    doc.print(&writer);

    QMessageBox::information(m_fenetre, "Export PDF", "Le fichier PDF a été créé :\n" + chemin);
}

void GestionEmployes::onStatistiques()
{
    if (m_droits.statistiques)
        emit statistiquesDemandees(m_droits.voirSalaires);
}

void GestionEmployes::onPrivileges()
{
    if (!m_droits.gererPrivileges)
        return;
    emit privilegesDemandes();
}

void GestionEmployes::onEnregistrerVisage()
{
    if (m_currentId < 0) {
        if (!m_droits.ajouter)
            return;
        if (!g_gabaritsAttente.isEmpty()
            && QMessageBox::question(m_fenetre, "Visage", "Un visage est déjà capturé. Le remplacer ?")
                   != QMessageBox::Yes)
            return;
        const QString prenom = ui->editPrenom->text().trimmed();
        const QString nom = ui->editNom->text().trimmed();
        const QString titre = (prenom + " " + nom).trimmed().isEmpty() ? QString("le nouvel employé")
                                                                       : (prenom + " " + nom).trimmed();
        QVector<QVector<float>> capture;
        if (capturerVisage(ui, m_fenetre, m_base.employes, titre, capture)) {
            g_gabaritsAttente = capture;
            majEtatVisage(-1);
            QMessageBox::information(m_fenetre, "Visage",
                                     "Le visage a été capturé.\n"
                                     "Il sera enregistré avec l'employé quand vous cliquerez sur « Enregistrer ».");
        }
        return;
    }

    const Employe *e = m_base.trouver(m_currentId);
    if (!e || !(m_droits.modifier || e->id == m_userId))
        return;
    if (!e->gabaritsVisage.isEmpty()
        && QMessageBox::question(m_fenetre, "Visage", "Un visage est déjà enregistré pour cet employé. Le remplacer ?")
               != QMessageBox::Yes)
        return;

    if (FaceAuth::enregistrer(ui, m_fenetre, m_base, m_currentId)) {
        majEtatVisage(m_currentId);
        QMessageBox::information(m_fenetre, "Visage", "Le visage a été enregistré.\n"
                                                      "Il peut maintenant servir à se connecter.");
    }
}

void GestionEmployes::onSupprimerVisage()
{
    if (m_currentId < 0) {
        g_gabaritsAttente.clear();
        majEtatVisage(-1);
        return;
    }

    Employe *e = m_base.trouver(m_currentId);
    if (!e || !(m_droits.modifier || e->id == m_userId) || e->gabaritsVisage.isEmpty())
        return;
    if (QMessageBox::question(m_fenetre, "Visage", "Retirer le visage enregistré de cet employé ?") != QMessageBox::Yes)
        return;
    e->gabaritsVisage.clear();
    FaceAuth::sauvegarderGabarits(m_base);
    majEtatVisage(m_currentId);
}

void chargerEmployesDemo(BaseEmployes &base)
{
    auto ajout = [&base](const QString &nom, const QString &prenom, const QString &email,
                         const QString &adresse, const QString &tel, double salaire,
                         const QDate &date, const QString &statut, const QString &poste,
                         const QString &mdp) {
        Employe e;
        e.id = base.prochainId++;
        e.nom = nom; e.prenom = prenom; e.email = email; e.adresse = adresse;
        e.telephone = tel; e.salaire = salaire; e.dateEmbauche = date;
        e.statut = statut; e.poste = poste; e.motDePasse = Emp::hacher(mdp);
        base.employes.append(e);
    };

    ajout("Ben Salah", "Ali",     "rh@petnova.tn",        "12 rue de la Liberté, Tunis",   "21650123456", 3200, QDate(2019, 3, 4),  "Actif",    "Responsable RH",        "admin123");
    ajout("Trabelsi",  "Sonia",   "sonia.trabelsi@petnova.tn", "5 avenue Habib Bourguiba, Tunis", "21698234567", 2800, QDate(2020, 9, 14), "Actif",    "Comptable",             "petnova123");
    ajout("Gharbi",    "Mohamed", "mohamed.gharbi@petnova.tn", "8 rue Ibn Khaldoun, Ariana",      "21655345678", 3900, QDate(2018, 1, 22), "Actif",    "Vétérinaire",           "petnova123");
    ajout("Jlassi",    "Ines",    "ines.jlassi@petnova.tn",    "20 rue des Jasmins, La Marsa",    "21622456789", 3700, QDate(2021, 5, 3),  "Actif", "Vétérinaire",           "petnova123");
    ajout("Mansour",   "Karim",   "karim.mansour@petnova.tn",  "3 rue de Carthage, Tunis",        "21697567890", 1500, QDate(2022, 2, 7),  "Actif",    "Assistant vétérinaire", "petnova123");
    ajout("Dridi",     "Yasmine", "yasmine.dridi@petnova.tn",  "17 avenue de la République, Ben Arous", "21652678901", 1450, QDate(2023, 6, 19), "Actif", "Assistant vétérinaire", "petnova123");
    ajout("Hamdi",     "Nour",    "nour.hamdi@petnova.tn",     "9 rue Mongi Slim, Tunis",         "21623789012", 1300, QDate(2023, 10, 2), "Actif",    "Réceptionniste",        "petnova123");
    ajout("Ayari",     "Sami",    "sami.ayari@petnova.tn",     "14 rue Farhat Hached, Radès",     "21699890123", 1250, QDate(2024, 1, 15), "Inactif", "Réceptionniste",        "petnova123");
    ajout("Chaabane",  "Rim",     "rim.chaabane@petnova.tn",   "6 rue du Lac, Les Berges du Lac", "21653901234", 1400, QDate(2022, 8, 29), "Actif",    "Toiletteur",            "petnova123");
    ajout("Bouzid",    "Hatem",   "hatem.bouzid@petnova.tn",   "11 rue de Marseille, Tunis",      "21624012345", 1350, QDate(2021, 11, 8), "Inactif", "Toiletteur",      "petnova123");
}
