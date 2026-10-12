#ifndef FACEAUTH_H
#define FACEAUTH_H

#include <QVector>
#include <QWidget>

#include "donneesemployes.h"

namespace Ui { class GSmartPetCare; }

// Authentification et enregistrement par reconnaissance faciale (OpenCV : YuNet + SFace).
//
// Prérequis : OpenCV >= 4.5.4 à la compilation (sinon le module reste inactif et le bouton
// affiche un message) et les deux fichiers .onnx du dossier models/ (voir models/LISEZMOI.md).
//
// Ce qui est stocké : pour chaque employé, 5 empreintes numériques de 128 valeurs. Aucune photo.
// Elles sont sauvegardées dans le dossier de données de l'application (visages.json, lisible
// uniquement par l'utilisateur du système), indexées par email.
namespace FaceAuth {

// Affiche la page caméra (pageVisage de gsmartpetcare.ui), ouvre la caméra et retourne l'id de l'employé reconnu, ou -1 (échec / annulation / non disponible).
int authentifier(Ui::GSmartPetCare *ui, QWidget *parent, const QVector<Employe> &employes);

// Capture 5 échantillons du visage de l'employé et les enregistre. Refuse si ce visage ressemble
// trop à celui d'un autre employé. Retourne true si l'enregistrement a eu lieu.
bool enregistrer(Ui::GSmartPetCare *ui, QWidget *parent, BaseEmployes &base, int employeId);

// Persistance des empreintes (à appeler après chargement des employés, puis après chaque changement).
void chargerGabarits(BaseEmployes &base);
bool sauvegarderGabarits(const BaseEmployes &base);

} // namespace FaceAuth

#endif // FACEAUTH_H
