#include "gestionservices.h"
#include "ui_gsmartpetcare.h"
#include "historiqueservices.h"
#include "statistiquesservices.h"

#include <QComboBox>
#include <QDate>
#include <QFileDialog>
#include <QHeaderView>
#include <QMap>
#include <QMessageBox>
#include <QPageSize>
#include <QPdfWriter>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTextDocument>

#include <algorithm>

// ===========================================================================
// VUE : aucune requête SQL dans ce fichier.
// Toutes les opérations sur la base passent par la classe Service (modèle).
// ===========================================================================

GestionServices::GestionServices(Ui::GSmartPetCare *uiPrincipal, QWidget *fenetre, QObject *parent)
    : QObject(parent)
    , ui(uiPrincipal)
    , m_fenetre(fenetre)
{
    ui->pageServices->setAttribute(Qt::WA_StyledBackground, true);   // fond de la feuille de style

    // --- Colonnes du tableau ---
    QHeaderView *h = ui->tableWidgetServices->horizontalHeader();
    h->setMinimumSectionSize(50);
    h->setStretchLastSection(false);
    h->setFixedHeight(44);                                 // barre des noms de colonnes plus haute
    h->setSectionResizeMode(0, QHeaderView::Fixed);   h->resizeSection(0, 45);    // ID
    h->setSectionResizeMode(1, QHeaderView::Stretch);                              // Nom du service
    h->setSectionResizeMode(2, QHeaderView::Fixed);   h->resizeSection(2, 100);   // Type
    h->setSectionResizeMode(3, QHeaderView::Fixed);   h->resizeSection(3, 80);    // Prix (DT)
    h->setSectionResizeMode(4, QHeaderView::Fixed);   h->resizeSection(4, 90);    // Durée (min)
    h->setSectionResizeMode(5, QHeaderView::Fixed);   h->resizeSection(5, 88);    // Disponible
    h->setSectionResizeMode(6, QHeaderView::Fixed);   h->resizeSection(6, 205);   // Actions

    ui->frameDetail->setMaximumWidth(320);                 // panneau de détails un peu plus étroit

    // --- Connexions ---
    connect(ui->pushButtonExporterPdf,  &QPushButton::clicked, this, &GestionServices::onExporterPdf);
    connect(ui->pushButtonEnregistrer,  &QPushButton::clicked, this, &GestionServices::onEnregistrer);
    connect(ui->pushButtonAnnuler,      &QPushButton::clicked, this, &GestionServices::onAnnuler);
    connect(ui->pushButtonHistorique,   &QPushButton::clicked, this, &GestionServices::onHistorique);
    connect(ui->pushButtonStatistiques, &QPushButton::clicked, this, &GestionServices::onStatistiques);

    connect(ui->lineEditRecherche, &QLineEdit::textChanged, this, &GestionServices::refreshTable);
    connect(ui->comboBoxTri,        QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GestionServices::refreshTable);
    connect(ui->comboBoxFiltreType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GestionServices::refreshTable);

    chargerDepuisBase();                     // READ : les services viennent d'Oracle (via Service)
    chargerHistoriqueExemple();              // historique de démo (ignoré si les services n'existent pas)
    viderFormulaire();                       // le panneau de droite démarre en mode « Ajouter »
    refreshTable();
}

// READ : la vue demande la liste au modèle ; elle ne connaît pas le SQL.
void GestionServices::chargerDepuisBase()
{
    m_services = Service::lister();
}

// ---------------------------------------------------------------------------
// Historique de démonstration (gardé en mémoire)
// ---------------------------------------------------------------------------
void GestionServices::chargerHistoriqueExemple()
{
    // Animaux 1, 2, 3 = Rex, Mimi, Coco du module Animal
    auto rendu = [this](int animalId, int serviceId, int joursAvant, const QString &remarque) {
        const Service *s = trouver(serviceId);
        if (!s)
            return;
        ServiceRendu r;
        r.animalId = animalId;
        r.serviceId = s->getID();
        r.nomService = s->getNom();
        r.type = s->getType();
        r.prix = s->getPrix();
        r.date = QDate::currentDate().addDays(-joursAvant);
        r.remarque = remarque;
        r.id = m_histo.prochainId++;
        m_histo.rendus.append(r);
    };
    rendu(1, 1, 200, "Premier examen, bon état général.");
    rendu(1, 3, 180, "Rappel dans 12 mois.");
    rendu(1, 5,  90, "Traitement contre puces et vers.");
    rendu(1, 2,  15, "Contrôle : tout est normal.");
    rendu(2, 1, 120, "Perte d'appétit, examen complet.");
    rendu(2, 7,  60, "Stérilisation sans complication.");
    rendu(2, 2,  30, "Cicatrisation correcte.");
    rendu(3, 1,  45, "Plumage à surveiller.");
}

const Service *GestionServices::trouver(int id) const
{
    for (const Service &s : m_services)
        if (s.getID() == id)
            return &s;
    return nullptr;
}

// ---------------------------------------------------------------------------
// Interface avec le module Animal
// ---------------------------------------------------------------------------
void GestionServices::setAnimaux(const QVector<Animal> &animaux)
{
    m_histo.animaux = animaux;
}

void GestionServices::animalSupprime(int animalId)
{
    m_histo.rendus.erase(std::remove_if(m_histo.rendus.begin(), m_histo.rendus.end(),
                                        [animalId](const ServiceRendu &r) { return r.animalId == animalId; }),
                         m_histo.rendus.end());
}

void GestionServices::afficherHistorique(int animalId)
{
    HistoriqueServices fenetre(m_services, m_histo, animalId, m_fenetre);
    fenetre.exec();
}

// ---------------------------------------------------------------------------
// Tableau : recherche + filtre par type + tri
// ---------------------------------------------------------------------------
void GestionServices::refreshTable()
{
    const QString q = ui->lineEditRecherche->text().trimmed();
    const QString typeFiltre = ui->comboBoxFiltreType->currentIndex() > 0
                                   ? ui->comboBoxFiltreType->currentText() : QString();

    QVector<Service> vue;
    for (const Service &s : m_services) {
        if (!typeFiltre.isEmpty() && s.getType() != typeFiltre)
            continue;
        if (!q.isEmpty()
            && !s.getNom().contains(q, Qt::CaseInsensitive)
            && !s.getDescription().contains(q, Qt::CaseInsensitive)
            && !s.getType().contains(q, Qt::CaseInsensitive))
            continue;
        vue.append(s);
    }

    switch (ui->comboBoxTri->currentIndex()) {
    case 1:  // Prix
        std::sort(vue.begin(), vue.end(), [](const Service &a, const Service &b) { return a.getPrix() < b.getPrix(); });
        break;
    case 2:  // Durée
        std::sort(vue.begin(), vue.end(), [](const Service &a, const Service &b) { return a.getDuree() < b.getDuree(); });
        break;
    case 3:  // Type
        std::sort(vue.begin(), vue.end(), [](const Service &a, const Service &b) {
            const int c = QString::localeAwareCompare(a.getType(), b.getType());
            return c != 0 ? c < 0 : QString::localeAwareCompare(a.getNom(), b.getNom()) < 0;
        });
        break;
    default: // Nom
        std::sort(vue.begin(), vue.end(), [](const Service &a, const Service &b) {
            return QString::localeAwareCompare(a.getNom(), b.getNom()) < 0;
        });
        break;
    }

    QTableWidget *t = ui->tableWidgetServices;
    {
        const QSignalBlocker blocker(t);
        t->clearSpans();
        t->setRowCount(vue.size());
        int ligneSelection = -1;

        for (int r = 0; r < vue.size(); ++r) {
            const Service &s = vue[r];

            auto *itId = new QTableWidgetItem(QString::number(s.getID()));
            itId->setData(Qt::UserRole, s.getID());
            itId->setTextAlignment(Qt::AlignCenter);

            auto *itPrix = new QTableWidgetItem(QString::number(s.getPrix(), 'f', 2));
            itPrix->setTextAlignment(Qt::AlignCenter);
            auto *itDuree = new QTableWidgetItem(QString::number(s.getDuree()));
            itDuree->setTextAlignment(Qt::AlignCenter);
            auto *itDispo = new QTableWidgetItem(s.isDisponible() ? "Oui" : "Non");
            itDispo->setTextAlignment(Qt::AlignCenter);

            t->setItem(r, 0, itId);
            t->setItem(r, 1, new QTableWidgetItem(s.getNom()));
            t->setItem(r, 2, new QTableWidgetItem(s.getType()));
            t->setItem(r, 3, itPrix);
            t->setItem(r, 4, itDuree);
            t->setItem(r, 5, itDispo);
            t->setItem(r, 6, new QTableWidgetItem());      // colonne « Actions » (boutons ci-dessous)
            t->setCellWidget(r, 6, creerBoutonsActions(s.getID()));

            if (s.getID() == m_currentId)
                ligneSelection = r;
        }

        // La ligne du service en cours de modification reste surlignée
        if (ligneSelection >= 0)
            t->selectRow(ligneSelection);
        else
            t->clearSelection();
    }
}

// ---------------------------------------------------------------------------
// Formulaire
// ---------------------------------------------------------------------------
// Mode « Ajouter » : formulaire vide, prêt pour un nouveau service.
void GestionServices::viderFormulaire()
{
    m_currentId = -1;
    ui->labelDetailTitre->setText("Ajouter un service");
    ui->lineEditIdService->setText("Auto");   // id attribué par Oracle (lecture seule)
    ui->lineEditNomService->clear();
    ui->lineEditPrix->clear();
    ui->lineEditDuree->clear();
    ui->plainTextEditDescription->clear();
    ui->comboBoxType->setCurrentIndex(0);
    ui->checkBoxDisponible->setChecked(true);
    ui->tableWidgetServices->clearSelection();
}

// Mode « Modifier » : formulaire rempli avec le service choisi.
void GestionServices::remplirFormulaire(const Service &s)
{
    ui->labelDetailTitre->setText("Modifier le service");
    ui->lineEditIdService->setText(QString::number(s.getID()));
    ui->lineEditNomService->setText(s.getNom());
    ui->lineEditPrix->setText(QString::number(s.getPrix(), 'f', 2));
    ui->lineEditDuree->setText(QString::number(s.getDuree()));
    ui->plainTextEditDescription->setPlainText(s.getDescription());
    ui->comboBoxType->setCurrentText(s.getType());
    ui->checkBoxDisponible->setChecked(s.isDisponible());
}

// ---------------------------------------------------------------------------
// Actions
// ---------------------------------------------------------------------------
// Boutons « Modifier » / « Supprimer » de la colonne Actions (un couple par ligne)
QWidget *GestionServices::creerBoutonsActions(int serviceId)
{
    auto *conteneur = new QWidget;
    auto *lay = new QHBoxLayout(conteneur);
    lay->setContentsMargins(6, 4, 6, 4);
    lay->setSpacing(8);

    auto *btnModifier = new QPushButton("Modifier", conteneur);
    btnModifier->setObjectName("btnModifierLigne");
    btnModifier->setFixedHeight(32);
    btnModifier->setCursor(Qt::PointingHandCursor);

    auto *btnSupprimer = new QPushButton("Supprimer", conteneur);
    btnSupprimer->setObjectName("btnSupprimerLigne");
    btnSupprimer->setFixedHeight(32);
    btnSupprimer->setCursor(Qt::PointingHandCursor);

    lay->addWidget(btnModifier);
    lay->addWidget(btnSupprimer);

    connect(btnModifier,  &QPushButton::clicked, this, [this, serviceId]() { onModifier(serviceId); });
    connect(btnSupprimer, &QPushButton::clicked, this, [this, serviceId]() { onSupprimer(serviceId); });
    return conteneur;
}

void GestionServices::onModifier(int serviceId)
{
    const Service *s = trouver(serviceId);
    if (!s)
        return;
    m_currentId = serviceId;
    remplirFormulaire(*s);

    // sélectionne la ligne correspondante
    QTableWidget *t = ui->tableWidgetServices;
    for (int r = 0; r < t->rowCount(); ++r) {
        if (t->item(r, 0) && t->item(r, 0)->data(Qt::UserRole).toInt() == serviceId) {
            t->selectRow(r);
            break;
        }
    }
    ui->lineEditNomService->setFocus();
    ui->lineEditNomService->selectAll();
}

// DELETE : la vue appelle Service::supprimer(), puis rafraîchit l'affichage.
void GestionServices::onSupprimer(int serviceId)
{
    const Service *s = trouver(serviceId);
    if (!s)
        return;

    if (QMessageBox::question(m_fenetre, "Supprimer",
                              QString("Supprimer le service « %1 » ?").arg(s->getNom()))
        != QMessageBox::Yes)
        return;

    bool test = m_etmp.supprimer(serviceId);

    if (!test) {
        QMessageBox::critical(m_fenetre, "Suppression impossible",
            "Ce service est peut-être utilisé dans un rendez-vous (table INCLURE).");
        return;
    }

    chargerDepuisBase();                     // Refresh (Actualiser) après la suppression
    if (m_currentId == serviceId)
        viderFormulaire();                   // retour au mode « Ajouter »
    refreshTable();
}

// CREATE / UPDATE : contrôle de saisie en C++, puis appel du modèle.
void GestionServices::onEnregistrer()
{
    // ----- Contrôle de saisie -----
    const QString nom = ui->lineEditNomService->text().trimmed();
    if (nom.isEmpty()) {
        QMessageBox::warning(m_fenetre, "Champ manquant", "Le nom du service est obligatoire.");
        ui->lineEditNomService->setFocus();
        return;
    }

    const QString description = ui->plainTextEditDescription->toPlainText().trimmed();
    if (description.isEmpty()) {
        QMessageBox::warning(m_fenetre, "Champ manquant", "La description est obligatoire.");
        ui->plainTextEditDescription->setFocus();
        return;
    }

    bool okPrix = false, okDuree = false;
    QString prixTxt = ui->lineEditPrix->text().trimmed();
    prixTxt.replace(',', '.');
    const double prix = prixTxt.toDouble(&okPrix);
    const int duree = ui->lineEditDuree->text().trimmed().toInt(&okDuree);

    if (!okPrix || prix < 0) {
        QMessageBox::warning(m_fenetre, "Prix invalide", "Saisissez un prix valide (ex. 30 ou 30.50).");
        ui->lineEditPrix->setFocus();
        return;
    }
    if (!okDuree || duree <= 0) {
        QMessageBox::warning(m_fenetre, "Durée invalide", "Saisissez une durée en minutes (entier positif).");
        ui->lineEditDuree->setFocus();
        return;
    }

    // ----- Récupération des informations saisies et instanciation de l'objet du modèle -----
    Service s(m_currentId < 0 ? 0 : m_currentId,
              nom,
              description,
              ui->comboBoxType->currentText(),
              prix,
              duree,
              ui->checkBoxDisponible->isChecked());

    // ----- Appel du modèle (aucune requête ici) -----
    bool test = (m_currentId < 0) ? s.ajouter() : s.modifier();

    if (!test) {
        QMessageBox::critical(m_fenetre, "Base de données",
                              "L'enregistrement a échoué (voir la sortie de l'application).");
        return;
    }

    chargerDepuisBase();                     // Refresh (Actualiser) après l'ajout / la modification
    viderFormulaire();                       // enregistré : retour au mode « Ajouter »
    refreshTable();
}

void GestionServices::onAnnuler()
{
    viderFormulaire();                       // abandonne la modification, retour au mode « Ajouter »
    refreshTable();                          // retire le surlignage de la ligne
}

void GestionServices::onExporterPdf()
{
    QString chemin = QFileDialog::getSaveFileName(m_fenetre, "Exporter en PDF",
                                                  "services.pdf", "Fichiers PDF (*.pdf)");
    if (chemin.isEmpty())
        return;
    if (!chemin.endsWith(".pdf", Qt::CaseInsensitive))
        chemin += ".pdf";

    QTableWidget *t = ui->tableWidgetServices;

    QString html = "<h2 style='color:#0B4F4A;'>Liste des services</h2>"
                   "<table border='1' cellspacing='0' cellpadding='5' width='100%'>"
                   "<tr style='background-color:#0F766E; color:white;'>";
    const int nbCols = t->columnCount() - 1;                // sans la colonne « Actions »
    for (int c = 0; c < nbCols; ++c)
        html += "<th>" + t->horizontalHeaderItem(c)->text().toHtmlEscaped() + "</th>";
    html += "</tr>";

    for (int r = 0; r < t->rowCount(); ++r) {
        html += "<tr>";
        for (int c = 0; c < nbCols; ++c)
            html += "<td>" + (t->item(r, c) ? t->item(r, c)->text().toHtmlEscaped() : QString()) + "</td>";
        html += "</tr>";
    }
    html += "</table>";

    QTextDocument doc;
    doc.setHtml(html);

    QPdfWriter writer(chemin);
    writer.setPageSize(QPageSize(QPageSize::A4));
    doc.print(&writer);

    QMessageBox::information(m_fenetre, "Export PDF", "Le fichier PDF a été créé :\n" + chemin);
}

void GestionServices::onHistorique()
{
    afficherHistorique(-1);
}

void GestionServices::onStatistiques()
{
    StatistiquesServices fenetre(m_services, m_fenetre);
    fenetre.exec();
}
