#ifndef FACEENGINE_H
#define FACEENGINE_H

#include <QImage>
#include <QRect>
#include <QString>
#include <QVector>

#include <memory>

#include "donneesemployes.h"

// Un visage détecté dans l'image de la caméra.
struct VisageDetecte
{
    QRect cadre;
    float score = 0.f;      // confiance du détecteur (0..1)
};

// Moteur de reconnaissance faciale, basé sur OpenCV (modules principaux uniquement, pas de contrib) :
//   - détection : YuNet  (face_detection_yunet_*.onnx)
//   - empreinte : SFace  (face_recognition_sface_2021dec.onnx) -> vecteur de 128 valeurs
// Les deux fichiers .onnx sont dans le dossier models/ (voir models/LISEZMOI.md).
// Si le projet est compilé sans OpenCV (HAVE_OPENCV absent), compileAvecOpenCV() renvoie false
// et tout le reste échoue proprement avec un message.
class FaceEngine
{
public:
    static constexpr int kDimension = 128;

    FaceEngine();
    ~FaceEngine();
    FaceEngine(const FaceEngine &) = delete;
    FaceEngine &operator=(const FaceEngine &) = delete;

    static bool compileAvecOpenCV();

    bool charger(QString *erreur);          // charge les deux modèles
    bool ouvrirCamera(QString *erreur);
    void fermerCamera();

    // Lit une image et détecte les visages (triés du plus grand au plus petit).
    bool lire(QImage *image, QVector<VisageDetecte> *visages);

    // Empreinte (normalisée) du visage n° `index` de la dernière image lue.
    bool calculerGabarit(int index, QVector<float> *gabarit, QString *erreur = nullptr);

    // Similarité cosinus entre deux empreintes : 1 = identiques, ~0 = personnes différentes.
    static double similarite(const QVector<float> &a, const QVector<float> &b);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

// Décision d'identification (indépendante d'OpenCV, donc testable seule).
namespace FaceMatch {

constexpr double kSeuil = 0.40;     // similarité minimale (0.363 = seuil officiel de SFace, on est plus strict)
constexpr double kMarge = 0.05;     // écart minimal avec le 2e meilleur employé (évite les ambiguïtés)

struct Resultat
{
    int    employeId = -1;
    double score = -1.0;            // meilleure similarité
    double second = -1.0;           // meilleure similarité parmi les AUTRES employés
    bool   accepte = false;         // score >= seuil ET écart >= marge
};

Resultat identifier(const QVector<float> &gabarit, const QVector<Employe> &employes);

} // namespace FaceMatch

#endif // FACEENGINE_H
