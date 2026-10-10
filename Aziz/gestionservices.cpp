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
    h->setSectionResizeMode(3, QHeaderView::Fixed);   h->resizeSection(3, 80);   // Prix (DT)
    h->setSectionResizeMode(4, QHeaderView::Fixed);   h->resizeSection(4, 90);   // Durée (min)
    h->setSectionResizeMode(5, QHeaderView::Fixed);   h->resizeSection(5, 88);   // Disponible
    h->setSectionResizeMode(6, QHeaderView::Fixed);   h->resizeSection(6, 205);   // Actions

    ui->frameDetail->setMaximumWidth(320);                 // panneau de détails un peu plus étroit

    // --- Connexions ---
    connect(ui->pushButtonExporterPdf,&QPushButton::clicked, this, &GestionServices::onExporterPdf);
    connect(ui->pushButtonEnregistrer,&QPushButton::clicked, this, &GestionServices::onEnregistrer);
    connect(ui->pushButtonAnnuler,    &QPushButton::clicked, this, &GestionServices::onAnnuler);
    connect(ui->pushButtonHistorique, &QPushButton::clicked, this, &GestionServices::onHistorique);
    connect(ui->pushButtonStatistiques, &QPushButton::clicked, this, &GestionServices::onStatistiques);

    connect(ui->lineEditRecherche, &QLineEdit::textChanged, this, &GestionServices::refreshTable);
    connect(ui->comboBoxTri,        QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GestionServices::refreshTable);
    connect(ui->comboBoxFiltreType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GestionServices::refreshTable);

    chargerDonneesExemple();                 // données de démonstration (en mémoire)
    chargerAnimauxExemple();                 // TEMPORAIRE (le module Animal fournira la vraie liste)
    viderFormulaire();                       // le panneau de droite démarre en mode « Ajouter »
    refreshTable();
}

// ---------------------------------------------------------------------------
// Données de démonstration (gardées en mémoire, sans base de données)
// ---------------------------------------------------------------------------
void GestionServices::chargerDonneesExemple()
{
    auto ajout = [this](const QString &nom, const QString &desc, const QString &type,
                        double prix, int duree, bool dispo) {
        Service s;
        s.nom = nom; s.description = desc; s.type = type;
        s.prix = prix; s.duree = duree; s.disponible = dispo;
        s.id = m_prochainIdService++;
        m_services.append(s);
    };

    ajout("Consultation générale", "Examen clinique complet de l'animal.", "Consultation", 30.0, 30, true);
    ajout("Consultation de suivi", "Contrôle après traitement.",           "Consultation", 20.0, 20, true);
    ajout("Vaccin antirabique",    "Vaccination annuelle contre la rage.", "Vaccination",  45.0, 15, true);
    ajout("Vaccin polyvalent",     "Protection contre les maladies courantes.", "Vaccination", 60.0, 20, true);
    ajout("Traitement antiparasitaire", "Traitement contre puces et vers.", "Traitement",  35.0, 25, true);
    ajout("Soin de plaie",         "Nettoyage et pansement.",              "Traitement",   40.0, 30, false);
    ajout("Stérilisation",         "Intervention chirurgicale sous anesthésie.", "Chirurgie", 250.0, 90, true);
    ajout("Chirurgie dentaire",    "Détartrage et extraction.",            "Chirurgie",   180.0, 75, true);
    ajout("Séance d'éducation",    "Apprentissage des ordres de base.",    "Éducation",    50.0, 60, true);
    ajout("Bilan comportemental",  "Analyse et conseils personnalisés.",   "Comportement", 70.0, 45, true);

    // Historique de démonstration (animaux 1, 2, 3 = ceux de chargerAnimauxExemple)
    auto rendu = [this](int animalId, int serviceId, int joursAvant, const QString &remarque) {
        const Service *s = trouver(serviceId);
        if (!s)
            return;
        ServiceRendu r;
        r.animalId = animalId;
        r.serviceId = s->id;
        r.nomService = s->nom;
        r.type = s->type;
        r.prix = s->prix;
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

Service *GestionServices::trouver(int id)
{
    for (Service &s : m_services)
        if (s.id == id)
            return &s;
    return nullptr;
}

// ---------------------------------------------------------------------------
// TEMPORAIRE : animaux de test.
// Quand le module Animal sera prêt, supprime cette fonction (et son appel dans le
// constructeur) et appelle setAnimaux() avec sa vraie liste.
// ---------------------------------------------------------------------------
void GestionServices::chargerAnimauxExemple()
{
    QVector<Animal> animaux;
    animaux.append({1, "Rex",  "Chien",  "Ali Ben Salah"});
    animaux.append({2, "Mimi", "Chat",   "Sonia Trabelsi"});
    animaux.append({3, "Coco", "Oiseau", "Mohamed Gharbi"});
    setAnimaux(animaux);
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
        if (!typeFiltre.isEmpty() && s.type != typeFiltre)
            continue;
        if (!q.isEmpty()
            && !s.nom.contains(q, Qt::CaseInsensitive)
            && !s.description.contains(q, Qt::CaseInsensitive)
            && !s.type.contains(q, Qt::CaseInsensitive))
            continue;
        vue.append(s);
    }

    switch (ui->comboBoxTri->currentIndex()) {
    case 1:  // Prix
        std::sort(vue.begin(), vue.end(), [](const Service &a, const Service &b) { return a.prix < b.prix; });
        break;
    case 2:  // Durée
        std::sort(vue.begin(), vue.end(), [](const Service &a, const Service &b) { return a.duree < b.duree; });
        break;
    case 3:  // Type
        std::sort(vue.begin(), vue.end(), [](const Service &a, const Service &b) {
            const int c = QString::localeAwareCompare(a.type, b.type);
            return c != 0 ? c < 0 : QString::localeAwareCompare(a.nom, b.nom) < 0;
        });
        break;
    default: // Nom
        std::sort(vue.begin(), vue.end(), [](const Service &a, const Service &b) {
            return QString::localeAwareCompare(a.nom, b.nom) < 0;
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

            auto *itId = new QTableWidgetItem(QString::number(s.id));
            itId->setData(Qt::UserRole, s.id);
            itId->setTextAlignment(Qt::AlignCenter);

            auto *itPrix = new QTableWidgetItem(QString::number(s.prix, 'f', 2));
            itPrix->setTextAlignment(Qt::AlignCenter);
            auto *itDuree = new QTableWidgetItem(QString::number(s.duree));
            itDuree->setTextAlignment(Qt::AlignCenter);
            auto *itDispo = new QTableWidgetItem(s.disponible ? "Oui" : "Non");
            itDispo->setTextAlignment(Qt::AlignCenter);

            t->setItem(r, 0, itId);
            t->setItem(r, 1, new QTableWidgetItem(s.nom));
            t->setItem(r, 2, new QTableWidgetItem(s.type));
            t->setItem(r, 3, itPrix);
            t->setItem(r, 4, itDuree);
            t->setItem(r, 5, itDispo);
            t->setItem(r, 6, new QTableWidgetItem());      // colonne « Actions » (boutons ci-dessous)
            t->setCellWidget(r, 6, creerBoutonsActions(s.id));

            if (s.id == m_currentId)
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
    ui->lineEditIdService->setText(QString::number(m_prochainIdService));   // prochain id (lecture seule)
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
    ui->lineEditIdService->setText(QString::number(s.id));
    ui->lineEditNomService->setText(s.nom);
    ui->lineEditPrix->setText(QString::number(s.prix, 'f', 2));
    ui->lineEditDuree->setText(QString::number(s.duree));
    ui->plainTextEditDescription->setPlainText(s.description);
    ui->comboBoxType->setCurrentText(s.type);
    ui->checkBoxDisponible->setChecked(s.disponible);
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

    // Même style que le bouton « Historique » (fond plein, texte blanc en gras, coins arrondis)
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

void GestionServices::onSupprimer(int serviceId)
{
    const Service *s = trouver(serviceId);
    if (!s)
        return;

    if (QMessageBox::question(m_fenetre, "Supprimer",
                              QString("Supprimer le service « %1 » ?").arg(s->nom))
        != QMessageBox::Yes)
        return;

    m_services.erase(std::remove_if(m_services.begin(), m_services.end(),
                                    [serviceId](const Service &x) { return x.id == serviceId; }),
                     m_services.end());
    if (m_currentId == serviceId)
        viderFormulaire();                   // retour au mode « Ajouter »
    refreshTable();
}

void GestionServices::onEnregistrer()
{
    const QString nom = ui->lineEditNomService->text().trimmed();
    if (nom.isEmpty()) {
        QMessageBox::warning(m_fenetre, "Champ manquant", "Le nom du service est obligatoire.");
        ui->lineEditNomService->setFocus();
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

    Service s;
    s.nom = nom;
    s.description = ui->plainTextEditDescription->toPlainText().trimmed();
    s.type = ui->comboBoxType->currentText();
    s.prix = prix;
    s.duree = duree;
    s.disponible = ui->checkBoxDisponible->isChecked();

    if (m_currentId < 0) {
        s.id = m_prochainIdService++;
        m_services.append(s);
    } else if (Service *existant = trouver(m_currentId)) {
        s.id = existant->id;
        *existant = s;
    }

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
