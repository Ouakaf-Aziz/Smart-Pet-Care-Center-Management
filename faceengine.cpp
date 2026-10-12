#include "faceengine.h"

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

// Où chercher les modèles : à côté de l'exécutable, dans le dossier courant, puis dans les sources.
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

// Fichiers existants et plausibles, par ordre de préférence. Un fichier trop petit est
// presque toujours un « pointeur Git LFS » (téléchargement raté) : on le signale.
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

// ---------------------------------------------------------------------------
// FaceEngine
// ---------------------------------------------------------------------------
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

    // Tri du plus grand au plus petit (les deux tableaux restent alignés).
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

// ---------------------------------------------------------------------------
// Identification
// ---------------------------------------------------------------------------
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
