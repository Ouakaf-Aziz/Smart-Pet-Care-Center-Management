#include "gsmartpetcare.h"
#include "ui_gsmartpetcare.h"
#include "service.h"
#include <QGraphicsDropShadowEffect>
#include <QIcon>
#include <QPushButton>
#include <QStyle>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDebug>
#include <QDialog>
#include <QFileDialog>
#include <QFont>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QMarginsF>
#include <QMessageBox>
#include <QObject>
#include <QPageLayout>
#include <QPageSize>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPdfWriter>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QScreen>
#include <QSet>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>
#include <QSqlQueryModel>
#include <algorithm>
#include <cmath>

GSmartPetCare::GSmartPetCare(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::GSmartPetCare)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/images/icone.png"));
    resize(1366, 800);
    auto *ombre = new QGraphicsDropShadowEffect(this);
    ombre->setBlurRadius(30); ombre->setOffset(0, 4); ombre->setColor(QColor(0, 0, 0, 45));
    ui->frameCard->setGraphicsEffect(ombre);

    // ===== Page de connexion =====
    ui->stackedWidget->setCurrentIndex(PageLogin);
    ui->lineEditIdentifiant->setFocus();
    connect(ui->lineEditIdentifiant, &QLineEdit::returnPressed, ui->lineEditMotDePasse, qOverload<>(&QWidget::setFocus));
    connect(ui->lineEditMotDePasse, &QLineEdit::returnPressed, ui->pushButtonConnexion, &QPushButton::click);
    connect(ui->pushButtonConnexion, &QPushButton::clicked, this, &GSmartPetCare::onConnecter);
    connect(ui->btnDeconnexion, &QPushButton::clicked, this, &GSmartPetCare::onDeconnecter);

    // Seul le module Service conserve ses fonctionnalités métier.
    // Les autres pages restent présentes dans gsmartpetcare.ui, mais sans logique métier.
    new GestionServices(ui, this, this);

    // ===== Navigation =====
    afficherContenu(ContenuAccueil);
    connect(ui->btnNavAccueil, &QPushButton::clicked, this, [this]{ afficherContenu(ContenuAccueil); });
    connect(ui->btnNavServices, &QPushButton::clicked, this, [this]{ afficherContenu(ContenuServices); });
    connect(ui->btnNavEmployes, &QPushButton::clicked, this, [this]{ afficherContenu(ContenuEmployes); });
    connect(ui->btnNavRDV, &QPushButton::clicked, this, [this]{ afficherContenu(ContenuRDV); });
    connect(ui->btnNavAnimaux, &QPushButton::clicked, this, [this]{ afficherContenu(ContenuAnimaux); });
}

GSmartPetCare::~GSmartPetCare() { delete ui; }

// ---------------------------------------------------------------------------
// Authentification
// ---------------------------------------------------------------------------
void GSmartPetCare::afficherErreur(const QString &message)
{
    ui->labelErreurConnexion->setText(message);
    ui->labelErreurConnexion->show();
}

void GSmartPetCare::onConnecter()
{
    const QString email = ui->lineEditIdentifiant->text().trimmed();
    if (email.isEmpty() || ui->lineEditMotDePasse->text().isEmpty()) {
        afficherErreur("Saisissez votre identifiant et votre mot de passe.");
        return;
    }

    // MODE TEMPORAIRE : n'importe quel identifiant / mot de passe est accepté (seuls les champs vides sont refusés).
    // On ouvre la session avec le compte n°1 (« Responsable RH », accès total).
    // La connexion Oracle est gérée séparément (voir main.cpp / connection.cpp).
    reussir(1);
}

void GSmartPetCare::reussir(int employeId)
{
    Q_UNUSED(employeId);
    ui->lineEditMotDePasse->clear();              // rien ne reste affiché une fois connecté
    ui->labelErreurConnexion->hide();
    ui->labelBienvenue->setText(QString("Bienvenue"));
    afficherContenu(ContenuAccueil);
    ui->stackedWidget->setCurrentIndex(PageAccueil);
}

void GSmartPetCare::onDeconnecter()
{
    ui->stackedWidget->setCurrentIndex(PageLogin);
    ui->lineEditIdentifiant->clear();
    ui->lineEditMotDePasse->clear();
    ui->labelErreurConnexion->hide();
    ui->lineEditIdentifiant->setFocus();
}

// ---------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------
void GSmartPetCare::afficherContenu(int index)
{
    ui->stackContenu->setCurrentIndex(index);
    const struct { QPushButton *bouton; bool actif; } liens[] = {
        {ui->btnNavAccueil, index == ContenuAccueil}, {ui->btnNavServices, index == ContenuServices},
        {ui->btnNavEmployes, index == ContenuEmployes}, {ui->btnNavRDV, index == ContenuRDV},
        {ui->btnNavAnimaux, index == ContenuAnimaux}
    };
    for (const auto &l : liens) { l.bouton->setProperty("actif", l.actif); l.bouton->style()->unpolish(l.bouton); l.bouton->style()->polish(l.bouton); }
}


// ===========================================================================
// VUES DU MODULE SERVICES - aucune requête SQL ici.
// Toutes les opérations sur la base passent par la classe Service (modèle).
// ===========================================================================

// ===========================================================================
// GestionServices : page « Gestion des services »
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
    h->setSectionResizeMode(6, QHeaderView::Fixed);   h->resizeSection(6, 190);   // Actions (Modifier / Supprimer)

    // --- Connexions ---
    connect(ui->pushButtonExporterPdf,  &QPushButton::clicked, this, &GestionServices::onExporterPdf);
    connect(ui->pushButtonEnregistrer,  &QPushButton::clicked, this, &GestionServices::onEnregistrer);
    connect(ui->pushButtonAnnuler,      &QPushButton::clicked, this, &GestionServices::onAnnuler);
    connect(ui->pushButtonHistorique,   &QPushButton::clicked, this, &GestionServices::onHistorique);
    connect(ui->pushButtonStatistiques, &QPushButton::clicked, this, &GestionServices::onStatistiques);

    connect(ui->lineEditRecherche, &QLineEdit::textChanged, this, &GestionServices::refreshTable);
    connect(ui->comboBoxTri,        QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GestionServices::refreshTable);
    connect(ui->comboBoxOrdre,      QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GestionServices::refreshTable);
    connect(ui->comboBoxFiltreType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GestionServices::refreshTable);

    // Pages « Historique » et « Statistiques » (définies dans gsmartpetcare.ui)
    m_historique   = new HistoriqueServices(ui, m_services, fenetre, this);
    m_statistiques = new StatistiquesServices(ui, fenetre, this);

    chargerDepuisBase();                     // READ : les services viennent d'Oracle (via Service)
    viderFormulaire();                       // le panneau de droite démarre en mode « Ajouter »
    refreshTable();
}

// ---------------------------------------------------------------------------
// 1. CRUD (Create / Update / Delete)
// ---------------------------------------------------------------------------

// CREATE / UPDATE : contrôle de saisie en C++, puis appel du modèle.
void GestionServices::onEnregistrer()
{
    // ----- Lecture des champs (conversion du texte en nombres) -----
    const QString nom = ui->lineEditNomService->text().trimmed();
    const QString description = ui->plainTextEditDescription->toPlainText().trimmed();

    bool okPrix = false, okDuree = false;
    QString prixTxt = ui->lineEditPrix->text().trimmed();
    prixTxt.replace(',', '.');
    const double prix = prixTxt.toDouble(&okPrix);
    const int duree = ui->lineEditDuree->text().trimmed().toInt(&okDuree);

    // ----- Instanciation de l'objet du modèle -----
    Service s(m_currentId < 0 ? 0 : m_currentId,
              nom,
              description,
              ui->comboBoxType->currentText(),
              prix,
              duree,
              ui->checkBoxDisponible->isChecked());

    // ----- Contrôle de saisie : les règles sont dans le modèle (Service::controleSaisie) -----
    QString erreur;
    Service::Champ champ = Service::ChampAucun;
    if (!s.controleSaisie(erreur, &champ)) {
        // Un texte non numérique est converti en 0 : on remplace alors le message par un message adapté
        if (champ == Service::ChampPrix && !okPrix)
            erreur = "Saisissez un prix en dinars (ex. 30 ou 30.50).";
        if (champ == Service::ChampDuree && !okDuree)
            erreur = "Saisissez une durée en minutes (nombre entier, ex. 30).";

        QMessageBox::warning(m_fenetre, "Saisie invalide", erreur);
        switch (champ) {
        case Service::ChampNom:         ui->lineEditNomService->setFocus();      break;
        case Service::ChampDescription: ui->plainTextEditDescription->setFocus(); break;
        case Service::ChampType:        ui->comboBoxType->setFocus();            break;
        case Service::ChampPrix:        ui->lineEditPrix->setFocus();            break;
        case Service::ChampDuree:       ui->lineEditDuree->setFocus();           break;
        default: break;
        }
        return;
    }

    // ----- Appel du modèle (aucune requête ici) -----
    const bool nouvelAjout = (m_currentId < 0);
    bool test = nouvelAjout ? s.ajouter() : s.modifier();

    if (!test) {
        QMessageBox::critical(m_fenetre, "Base de données",
                              "L'enregistrement a échoué (voir la sortie de l'application).");
        return;
    }

    chargerDepuisBase();                     // Refresh (Actualiser) après l'ajout / la modification
    if (nouvelAjout)
        QMessageBox::information(m_fenetre, "Service ajouté",
                                 QString("Le service « %1 » a été ajouté avec succès.").arg(nom));
    viderFormulaire();                       // enregistré : retour au mode « Ajouter »
    refreshTable();
}

void GestionServices::modifierService(int serviceId)
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
void GestionServices::supprimerService(int serviceId)
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

void GestionServices::onAnnuler()
{
    viderFormulaire();                       // abandonne la modification, retour au mode « Ajouter »
    refreshTable();                          // retire le surlignage de la ligne
}

// ---------------------------------------------------------------------------
// 2. AFFICHAGE + RECHERCHE + FILTRE + TRI
// ---------------------------------------------------------------------------

// READ : la vue demande la liste au modèle ; elle ne connaît pas le SQL.
void GestionServices::chargerDepuisBase()
{
    m_services = Service::lister();
}

const Service *GestionServices::trouver(int id) const
{
    for (const Service &s : m_services)
        if (s.getID() == id)
            return &s;
    return nullptr;
}

// ---------------------------------------------------------------------------
// Tableau : recherche + filtre par type + tri (faits en SQL par Service::afficher)
// ---------------------------------------------------------------------------
void GestionServices::refreshTable()
{
    const QString q = ui->lineEditRecherche->text().trimmed();
    const QString typeFiltre = ui->comboBoxFiltreType->currentIndex() > 0
                                   ? ui->comboBoxFiltreType->currentText() : QString();

    // Recherche + filtre + tri exécutés par Oracle : la vue demande le modèle à Service::afficher()
    // (colonnes : 0 ID, 1 Nom, 2 Type, 3 Prix, 4 Durée, 5 Disponible)
    QSqlQueryModel *vue = m_etmp.afficher(q, typeFiltre, ui->comboBoxTri->currentIndex(),
                                          ui->comboBoxOrdre->currentIndex() == 1);
    while (vue->canFetchMore())
        vue->fetchMore();                    // Oracle renvoie les lignes par paquets : on les charge toutes
    const int nbLignes = vue->rowCount();

    QTableWidget *t = ui->tableWidgetServices;
    {
        const QSignalBlocker blocker(t);
        t->clearSpans();
        t->setRowCount(nbLignes);
        int ligneSelection = -1;

        for (int r = 0; r < nbLignes; ++r) {
            const int id = vue->data(vue->index(r, 0)).toInt();

            auto *itId = new QTableWidgetItem(QString::number(id));
            itId->setData(Qt::UserRole, id);
            itId->setTextAlignment(Qt::AlignCenter);

            auto *itPrix = new QTableWidgetItem(QString::number(vue->data(vue->index(r, 3)).toDouble(), 'f', 2));
            itPrix->setTextAlignment(Qt::AlignCenter);
            auto *itDuree = new QTableWidgetItem(QString::number(vue->data(vue->index(r, 4)).toInt()));
            itDuree->setTextAlignment(Qt::AlignCenter);
            auto *itDispo = new QTableWidgetItem(vue->data(vue->index(r, 5)).toInt() == 1 ? "Oui" : "Non");
            itDispo->setTextAlignment(Qt::AlignCenter);

            t->setItem(r, 0, itId);
            t->setItem(r, 1, new QTableWidgetItem(vue->data(vue->index(r, 1)).toString()));
            t->setItem(r, 2, new QTableWidgetItem(vue->data(vue->index(r, 2)).toString()));
            t->setItem(r, 3, itPrix);
            t->setItem(r, 4, itDuree);
            t->setItem(r, 5, itDispo);
            ajouterBoutonsLigne(r, id);                      // colonne « Actions »

            if (id == m_currentId)
                ligneSelection = r;
        }

        // La ligne du service en cours de modification reste surlignée
        if (ligneSelection >= 0)
            t->selectRow(ligneSelection);
        else
            t->clearSelection();
    }

    delete vue;                              // le modèle appartient à l'appelant
}

// Boutons « Modifier » / « Supprimer » de la colonne Actions : un couple devant chaque service.
// Ils agissent sur le service de leur propre ligne (id capturé), sans sélection préalable.
void GestionServices::ajouterBoutonsLigne(int ligne, int serviceId)
{
    auto *conteneur = new QWidget;
    auto *lay = new QHBoxLayout(conteneur);
    lay->setContentsMargins(6, 4, 6, 4);
    lay->setSpacing(6);

    auto *btnModifier = new QPushButton("Modifier", conteneur);
    btnModifier->setObjectName("btnModifierLigne");          // styles définis dans gsmartpetcare.ui
    btnModifier->setCursor(Qt::PointingHandCursor);
    btnModifier->setMinimumHeight(30);

    auto *btnSupprimer = new QPushButton("Supprimer", conteneur);
    btnSupprimer->setObjectName("btnSupprimerLigne");
    btnSupprimer->setCursor(Qt::PointingHandCursor);
    btnSupprimer->setMinimumHeight(30);

    lay->addWidget(btnModifier);
    lay->addWidget(btnSupprimer);

    // Exécution différée : l'action reconstruit le tableau, donc détruit ce bouton.
    connect(btnModifier, &QPushButton::clicked, this, [this, serviceId] {
        QTimer::singleShot(0, this, [this, serviceId] { modifierService(serviceId); });
    });
    connect(btnSupprimer, &QPushButton::clicked, this, [this, serviceId] {
        QTimer::singleShot(0, this, [this, serviceId] { supprimerService(serviceId); });
    });

    ui->tableWidgetServices->setCellWidget(ligne, 6, conteneur);
}

// ---------------------------------------------------------------------------
// 3. STATISTIQUES
// ---------------------------------------------------------------------------

void GestionServices::onStatistiques()
{
    m_statistiques->ouvrir();
}

// ---------------------------------------------------------------------------
// 4. HISTORIQUE
// ---------------------------------------------------------------------------

void GestionServices::onHistorique()
{
    afficherHistorique(-1);
}

// Ouvre la fenêtre Historique (sur cet animal si animalId >= 0)
void GestionServices::afficherHistorique(int animalId)
{
    m_historique->ouvrir(animalId);          // l'historique lit Oracle (via Service)
}

// ---------------------------------------------------------------------------
// 5. EXPORT PDF
// ---------------------------------------------------------------------------

// Export PDF : les données sont relues directement dans Oracle (table SERVICE) au moment
// de l'export, avec la recherche / le filtre / le tri actuellement choisis dans la page.
void GestionServices::onExporterPdf()
{
    QString chemin = QFileDialog::getSaveFileName(m_fenetre, "Exporter en PDF",
                                                  "services.pdf", "Fichiers PDF (*.pdf)");
    if (chemin.isEmpty())
        return;
    if (!chemin.endsWith(".pdf", Qt::CaseInsensitive))
        chemin += ".pdf";

    const QString q = ui->lineEditRecherche->text().trimmed();
    const QString typeFiltre = ui->comboBoxFiltreType->currentIndex() > 0
                                   ? ui->comboBoxFiltreType->currentText() : QString();
    const QVector<Service> liste = Service::rechercher(q, typeFiltre, ui->comboBoxTri->currentIndex(),
                                                        ui->comboBoxOrdre->currentIndex() == 1);

    QString html = QString("<h2 style='color:#0B4F4A;'>Liste des services</h2>"
                           "<p><b>Date d'export :</b> %1<br>"
                           "<b>Nombre de services :</b> %2</p>")
                       .arg(QDate::currentDate().toString("dd/MM/yyyy"))
                       .arg(liste.size());

    html += "<table border='1' cellspacing='0' cellpadding='5' width='100%'>"
            "<tr style='background-color:#0F766E; color:white;'>"
            "<th>ID</th><th>Nom du service</th><th>Type</th><th>Prix (DT)</th>"
            "<th>Durée (min)</th><th>Disponible</th></tr>";
    for (const Service &s : liste) {
        html += QString("<tr><td align='center'>%1</td><td>%2</td><td>%3</td>"
                        "<td align='right'>%4</td><td align='center'>%5</td><td align='center'>%6</td></tr>")
                    .arg(s.getID())
                    .arg(s.getNom().toHtmlEscaped(),
                         s.getType().toHtmlEscaped(),
                         QString::number(s.getPrix(), 'f', 2),
                         QString::number(s.getDuree()),
                         s.isDisponible() ? "Oui" : "Non");
    }
    html += "</table>";

    QTextDocument doc;
    doc.setHtml(html);

    QPdfWriter writer(chemin);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
    doc.print(&writer);

    QMessageBox::information(m_fenetre, "Export PDF", "Le fichier PDF a été créé :\n" + chemin);
}


// ---------------------------------------------------------------------------
// 6. FORMULAIRE (mode Ajouter / mode Modifier)
// ---------------------------------------------------------------------------

// Mode « Ajouter » : formulaire vide, prêt pour un nouveau service.
void GestionServices::viderFormulaire()
{
    m_currentId = -1;
    ui->labelDetailTitre->setText("Ajouter un service");
    ui->lineEditIdService->setText("Auto");   // id attribué par le code à l'enregistrement (lecture seule)
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

// ===========================================================================
// Utilitaires communs aux fenêtres Historique et Statistiques
// ===========================================================================

namespace {
QString formatDate(const QDate &d) { return d.toString("dd/MM/yyyy"); }

// Crée une fenêtre (avec la seule croix de fermeture) qui reçoit une page de gsmartpetcare.ui.
// Le widget est retiré du QStackedWidget de la fenêtre principale et placé dans la fenêtre.
QDialog *creerFenetre(QWidget *parent, const QString &titre, QWidget *contenu)
{
    auto *d = new QDialog(parent);
    d->setWindowTitle(titre);
    d->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    d->setWindowFlag(Qt::WindowCloseButtonHint, true);
    auto *lay = new QVBoxLayout(d);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(contenu);
    contenu->show();                         // la page était masquée dans la pile
    return d;
}

// Affiche la fenêtre au milieu de l'écran puis attend sa fermeture.
void afficherCentree(QDialog *d, QWidget *reference, int largeur, int hauteur)
{
    QScreen *ecran = reference ? reference->screen() : QGuiApplication::primaryScreen();
    const QRect zone = ecran->availableGeometry();
    const QSize taille(qMin(largeur, zone.width() - 60), qMin(hauteur, zone.height() - 80));
    d->resize(taille);
    d->move(zone.center() - QPoint(taille.width() / 2, taille.height() / 2));
    d->exec();
}
} // namespace

// ===========================================================================
// HistoriqueServices : page « Historique des services rendus à chaque animal »
// ===========================================================================

HistoriqueServices::HistoriqueServices(Ui::GSmartPetCare *uiPrincipal, const QVector<Service> &services,
                                       QWidget *fenetre, QObject *parent)
    : QObject(parent)
    , ui(uiPrincipal)
    , m_services(services)
    , m_fenetre(fenetre)
{
    ui->pageHistoriqueServices->setAttribute(Qt::WA_StyledBackground, true);   // fond de la feuille de style
    configurerColonnes();

    // Fenêtre centrée au milieu de l'écran : seule la croix la ferme.
    m_dialogue = creerFenetre(fenetre, "Historique des services rendus", ui->scrollHistoriqueServices);
    m_fenetre = m_dialogue;                  // les messages s'affichent au-dessus de cette fenêtre
    connect(m_dialogue, &QDialog::finished, this, [this] { fermerSaisie(); });

    // ------------------------------ Connexions ---------------------------------
    connect(ui->lineEditRechercheAnimalHisto, &QLineEdit::textChanged, this, &HistoriqueServices::refreshAnimaux);
    connect(ui->tableAnimauxHisto, &QTableWidget::itemSelectionChanged, this, &HistoriqueServices::refreshHistorique);
    connect(ui->comboFiltreTypeHisto, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HistoriqueServices::refreshHistorique);
    connect(ui->comboTriHisto, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HistoriqueServices::refreshHistorique);
    connect(ui->tableHistorique, &QTableWidget::itemSelectionChanged, this, &HistoriqueServices::majBoutons);

    connect(ui->btnAjouterRendu,   &QPushButton::clicked, this, &HistoriqueServices::onAjouterRendu);
    connect(ui->btnModifierRendu,  &QPushButton::clicked, this, &HistoriqueServices::onModifierRendu);
    connect(ui->btnSupprimerRendu, &QPushButton::clicked, this, &HistoriqueServices::onSupprimerRendu);
    connect(ui->btnPdfHisto,       &QPushButton::clicked, this, &HistoriqueServices::onExporterPdf);

    // Panneau de saisie (frameSaisieRendu)
    connect(ui->btnEnregistrerRendu, &QPushButton::clicked, this, &HistoriqueServices::onEnregistrerRendu);
    connect(ui->btnAnnulerRendu,     &QPushButton::clicked, this, &HistoriqueServices::fermerSaisie);
    connect(ui->comboServiceRendu, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HistoriqueServices::onServiceRenduChange);
}

// Affiche la page Historique (les données sont lues à chaque ouverture).
void HistoriqueServices::ouvrir(int animalIdInitial)
{
    fermerSaisie();
    remplirFiltreType();
    refreshAnimaux();

    if (animalIdInitial >= 0) {
        for (int r = 0; r < ui->tableAnimauxHisto->rowCount(); ++r)
            if (ui->tableAnimauxHisto->item(r, 0)->data(Qt::UserRole).toInt() == animalIdInitial) {
                ui->tableAnimauxHisto->selectRow(r);
                break;
            }
    }
    afficherCentree(m_dialogue, m_dialogue->parentWidget(), 1180, 780);
}

// ---------------------------------------------------------------------------
// 1. AFFICHAGE + RECHERCHE + FILTRE + TRI
// ---------------------------------------------------------------------------

void HistoriqueServices::refreshAnimaux()
{
    const int idAvant = animalCourantId();

    // Recherche + tri par nom exécutés par Oracle
    m_animaux = Service::rechercherAnimaux(ui->lineEditRechercheAnimalHisto->text());

    {
        const QSignalBlocker blocker(ui->tableAnimauxHisto);
        ui->tableAnimauxHisto->setRowCount(m_animaux.size());
        int ligne = -1;
        for (int r = 0; r < m_animaux.size(); ++r) {
            const Service &a = m_animaux[r];
            auto *itNom = new QTableWidgetItem(a.getNomAnimal());
            itNom->setData(Qt::UserRole, a.getIdAnimal());
            ui->tableAnimauxHisto->setItem(r, 0, itNom);
            ui->tableAnimauxHisto->setItem(r, 1, new QTableWidgetItem(a.getEspece()));
            ui->tableAnimauxHisto->setItem(r, 2, new QTableWidgetItem(a.getProprietaire()));
            if (a.getIdAnimal() == idAvant)
                ligne = r;
        }
        if (ligne < 0 && !m_animaux.isEmpty())
            ligne = 0;                       // sélection par défaut : premier animal
        if (ligne >= 0)
            ui->tableAnimauxHisto->selectRow(ligne);
        else
            ui->tableAnimauxHisto->clearSelection();
    }
    refreshHistorique();
}

void HistoriqueServices::refreshHistorique()
{
    const Service *a = trouverAnimal(animalCourantId());

    {
        const QSignalBlocker blocker(ui->tableHistorique);
        ui->tableHistorique->setRowCount(0);
    }
    m_rendus.clear();

    if (!a) {
        ui->labelFicheHisto->setText(m_animaux.isEmpty() ? "Aucun animal trouvé" : "Sélectionnez un animal");
        ui->labelNbServicesHisto->setText("<span style='font-size:9pt;color:#6B7A78;'>SERVICES RENDUS</span><br><b style='font-size:16pt;color:#0B4F4A;'>—</b>");
        ui->labelTotalHisto->setText("<span style='font-size:9pt;color:#6B7A78;'>TOTAL FACTURÉ</span><br><b style='font-size:16pt;color:#0B4F4A;'>—</b>");
        ui->labelDernierHisto->setText("<span style='font-size:9pt;color:#6B7A78;'>DERNIER SERVICE</span><br><b style='font-size:16pt;color:#0B4F4A;'>—</b>");
        majBoutons();
        return;
    }

    QString fiche = QString("%1 — %2").arg(a->getNomAnimal().toHtmlEscaped(), a->getEspece().toHtmlEscaped());
    fiche += QString("<br><span style='font-size:10pt;font-weight:normal;color:#5C6B69;'>Propriétaire : %1</span>")
                 .arg(a->getProprietaire().isEmpty() ? "—" : a->getProprietaire().toHtmlEscaped());
    ui->labelFicheHisto->setText(fiche);

    // Statistiques (COUNT / SUM / MAX en SQL) : elles ne dépendent pas du filtre
    const QVariantMap st = Service::statistiquesAnimal(a->getIdAnimal());

    ui->labelNbServicesHisto->setText(QString("<span style='font-size:9pt;color:#6B7A78;'>SERVICES RENDUS</span><br><b style='font-size:16pt;color:#0B4F4A;'>%1</b>").arg(st.value("nombre").toInt()));
    ui->labelTotalHisto->setText(QString("<span style='font-size:9pt;color:#6B7A78;'>TOTAL FACTURÉ</span><br><b style='font-size:16pt;color:#0B4F4A;'>%1 DT</b>").arg(st.value("total").toDouble(), 0, 'f', 2));
    ui->labelDernierHisto->setText(QString("<span style='font-size:9pt;color:#6B7A78;'>DERNIER SERVICE</span><br><b style='font-size:16pt;color:#0B4F4A;'>%1</b>")
                                .arg(st.value("dernier").toDate().isValid() ? formatDate(st.value("dernier").toDate()) : QString("—")));

    // Filtre par type + tri exécutés par Oracle
    const QString typeFiltre = ui->comboFiltreTypeHisto->currentIndex() > 0 ? ui->comboFiltreTypeHisto->currentText() : QString();
    m_rendus = Service::historiqueAnimal(a->getIdAnimal(), typeFiltre, ui->comboTriHisto->currentIndex());

    {
        const QSignalBlocker blocker(ui->tableHistorique);
        ui->tableHistorique->setRowCount(m_rendus.size());
        for (int ligne = 0; ligne < m_rendus.size(); ++ligne) {
            const Service &r = m_rendus[ligne];

            auto *itDate = new QTableWidgetItem(formatDate(r.getDateRdv()));
            itDate->setData(Qt::UserRole, r.getIdRdv());
            itDate->setData(Qt::UserRole + 1, r.getID());
            itDate->setTextAlignment(Qt::AlignCenter);
            auto *itPrix = new QTableWidgetItem(QString::number(r.getPrixApplique(), 'f', 2));
            itPrix->setTextAlignment(Qt::AlignCenter);

            ui->tableHistorique->setItem(ligne, 0, itDate);
            ui->tableHistorique->setItem(ligne, 1, new QTableWidgetItem(r.getNom()));
            ui->tableHistorique->setItem(ligne, 2, new QTableWidgetItem(r.getType()));
            ui->tableHistorique->setItem(ligne, 3, itPrix);
            ui->tableHistorique->setItem(ligne, 4, new QTableWidgetItem(r.getRemarque()));
        }
        ui->tableHistorique->clearSelection();
    }
    majBoutons();
}

void HistoriqueServices::majBoutons()
{
    int idRdv = 0, idService = 0;
    const bool animal = animalCourantId() >= 0;
    const bool rendu = renduCourant(idRdv, idService);
    ui->btnAjouterRendu->setEnabled(animal);
    ui->btnPdfHisto->setEnabled(animal);
    ui->btnModifierRendu->setEnabled(rendu);
    ui->btnSupprimerRendu->setEnabled(rendu);
}

void HistoriqueServices::remplirFiltreType()
{
    QSet<QString> types;
    for (const Service &s : m_services)
        types.insert(s.getType());
    QStringList liste = types.values();
    liste.sort(Qt::CaseInsensitive);

    // Le premier élément « Tous les types » est défini dans gsmartpetcare.ui ;
    // seuls les types lus en base sont ajoutés ici.
    const QSignalBlocker b(ui->comboFiltreTypeHisto);
    while (ui->comboFiltreTypeHisto->count() > 1)
        ui->comboFiltreTypeHisto->removeItem(ui->comboFiltreTypeHisto->count() - 1);
    ui->comboFiltreTypeHisto->addItems(liste);
}

// Modes de redimensionnement par colonne (non disponibles dans Qt Designer).
void HistoriqueServices::configurerColonnes()
{
    QHeaderView *ha = ui->tableAnimauxHisto->horizontalHeader();
    ha->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ha->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ha->setSectionResizeMode(2, QHeaderView::Stretch);

    QHeaderView *hh = ui->tableHistorique->horizontalHeader();
    hh->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    hh->setSectionResizeMode(1, QHeaderView::Stretch);
    hh->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    hh->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    hh->setSectionResizeMode(4, QHeaderView::Stretch);
}

// ---------------------------------------------------------------------------
// 2. CRUD des services rendus (écrits dans la table INCLURE via Service)
// ---------------------------------------------------------------------------

void HistoriqueServices::onAjouterRendu()
{
    const int animalId = animalCourantId();
    if (animalId < 0)
        return;

    // Ajout : le service est rattaché à l'un des rendez-vous de l'animal.
    const QVector<Service> rdvs = Service::rendezVousAnimal(animalId);
    if (rdvs.isEmpty()) {
        QMessageBox::information(m_fenetre, "Aucun rendez-vous",
                                 "Cet animal n'a aucun rendez-vous. Créez d'abord un rendez-vous "
                                 "(module Rendez-vous) pour pouvoir y ajouter un service.");
        return;
    }
    ouvrirSaisie(Service(), rdvs);
}

void HistoriqueServices::onModifierRendu()
{
    int idRdv = 0, idService = 0;
    const Service *r = renduCourant(idRdv, idService) ? trouverRendu(idRdv, idService) : nullptr;
    if (!r) {
        QMessageBox::information(m_fenetre, "Modifier", "Sélectionnez d'abord une ligne de l'historique.");
        return;
    }
    ouvrirSaisie(*r, {});
}

void HistoriqueServices::onSupprimerRendu()
{
    int idRdv = 0, idService = 0;
    const Service *r = renduCourant(idRdv, idService) ? trouverRendu(idRdv, idService) : nullptr;
    if (!r) {
        QMessageBox::information(m_fenetre, "Supprimer", "Sélectionnez d'abord une ligne de l'historique.");
        return;
    }
    const QString texte = QString("Supprimer « %1 » du %2 de l'historique ?\n"
                                  "(le rendez-vous lui-même n'est pas supprimé)")
                              .arg(r->getNom(), formatDate(r->getDateRdv()));
    if (QMessageBox::question(m_fenetre, "Supprimer", texte) != QMessageBox::Yes)
        return;

    if (!Service::supprimerRendu(idRdv, idService)) {
        QMessageBox::critical(m_fenetre, "Suppression impossible",
                              "La suppression a échoué (voir la sortie de l'application).");
        return;
    }
    refreshHistorique();
}

// Bouton « Enregistrer » : contrôle de saisie, puis appel du modèle.
void HistoriqueServices::onEnregistrerRendu()
{
    bool ok = false;
    QString txt = ui->lineEditPrixRendu->text().trimmed();
    txt.replace(',', '.');
    const double prix = txt.toDouble(&ok);
    QString erreurPrix;
    if (!ok)
        erreurPrix = "Saisissez un prix en dinars (ex. 30 ou 30.50).";
    Service verif;
    verif.setPrix(prix);
    if (!ok || !verif.controleSaisie(erreurPrix, nullptr, true)) {   // prix seul, 0 permis (service offert)
        QMessageBox::warning(m_fenetre, "Prix invalide", erreurPrix);
        ui->lineEditPrixRendu->setFocus();
        return;
    }
    if (ui->plainTextEditRemarqueRendu->toPlainText().trimmed().length() > 500) {
        QMessageBox::warning(m_fenetre, "Remarque trop longue", "La remarque ne doit pas dépasser 500 caractères.");
        ui->plainTextEditRemarqueRendu->setFocus();
        return;
    }

    Service r = m_renduEnCours;
    if (!m_edition) {
        r.setIdRdv(ui->comboRdv->currentData().toInt());
        r.setDateRdv(QDate());
    }
    const int nouveauServiceId = ui->comboServiceRendu->currentData().toInt();
    if (const Service *s = trouverService(nouveauServiceId)) {
        r.setNom(s->getNom());
        r.setType(s->getType());
    }
    r.setID(nouveauServiceId);
    r.setPrixApplique(prix);
    r.setRemarque(ui->plainTextEditRemarqueRendu->toPlainText().trimmed());

    const bool reussi = m_edition ? r.modifierRendu(m_ancienIdService)
                                  : r.ajouterRendu();
    if (!reussi) {
        QMessageBox::critical(m_fenetre, m_edition ? "Modification impossible" : "Ajout impossible",
                              "Ce service est peut-être déjà enregistré pour ce rendez-vous "
                              "(voir aussi la sortie de l'application).");
        return;
    }
    fermerSaisie();
    refreshHistorique();
}

// En changeant de service, le prix proposé suit (il reste modifiable).
void HistoriqueServices::onServiceRenduChange()
{
    if (const Service *s = trouverService(ui->comboServiceRendu->currentData().toInt()))
        ui->lineEditPrixRendu->setText(QString::number(s->getPrix(), 'f', 2));
}

// ---------------------------------------------------------------------------
// 3. EXPORT PDF de la fiche de l'animal
// ---------------------------------------------------------------------------

void HistoriqueServices::onExporterPdf()
{
    const Service *a = trouverAnimal(animalCourantId());
    if (!a) {
        QMessageBox::information(m_fenetre, "Export PDF", "Sélectionnez d'abord un animal.");
        return;
    }

    QString nomFichier = "historique_" + a->getNomAnimal();
    nomFichier.replace(' ', '_');
    QString chemin = QFileDialog::getSaveFileName(m_fenetre, "Exporter l'historique en PDF",
                                                  nomFichier + ".pdf", "Fichiers PDF (*.pdf)");
    if (chemin.isEmpty())
        return;
    if (!chemin.endsWith(".pdf", Qt::CaseInsensitive))
        chemin += ".pdf";

    // Tous les services de l'animal, du plus récent au plus ancien (lus en base)
    const QVector<Service> liste = Service::historiqueAnimal(a->getIdAnimal(), QString(), 0);

    double total = 0.0;
    for (const Service &r : liste)
        total += r.getPrixApplique();

    QString html = QString("<h2 style='color:#0B4F4A;'>Historique des services rendus</h2>"
                           "<p><b>Animal :</b> %1 (%2)<br>"
                           "<b>Propriétaire :</b> %3<br>"
                           "<b>Édité le :</b> %4</p>")
                       .arg(a->getNomAnimal().toHtmlEscaped(),
                            a->getEspece().toHtmlEscaped(),
                            a->getProprietaire().isEmpty() ? QString("—") : a->getProprietaire().toHtmlEscaped(),
                            formatDate(QDate::currentDate()));

    html += "<table border='1' cellspacing='0' cellpadding='5' width='100%'>"
            "<tr style='background-color:#0F766E; color:white;'>"
            "<th>Date</th><th>Service</th><th>Type</th><th>Prix (DT)</th><th>Remarque</th></tr>";
    for (const Service &r : liste) {
        html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td align='right'>%4</td><td>%5</td></tr>")
                    .arg(formatDate(r.getDateRdv()),
                         r.getNom().toHtmlEscaped(),
                         r.getType().toHtmlEscaped(),
                         QString::number(r.getPrixApplique(), 'f', 2),
                         r.getRemarque().toHtmlEscaped());
    }
    html += "</table>";
    html += QString("<p><b>%1 service(s) — Total : %2 DT</b></p>")
                .arg(liste.size())
                .arg(total, 0, 'f', 2);

    QTextDocument doc;
    doc.setHtml(html);

    QPdfWriter writer(chemin);
    writer.setPageSize(QPageSize(QPageSize::A4));
    doc.print(&writer);

    QMessageBox::information(m_fenetre, "Export PDF", "Le fichier PDF a été créé :\n" + chemin);
}

// ---------------------------------------------------------------------------
// 4. PANNEAU DE SAISIE (frameSaisieRendu, défini dans gsmartpetcare.ui)
// ---------------------------------------------------------------------------

// Active / désactive le reste de la page pendant la saisie.
void HistoriqueServices::basculerSaisie(bool ouverte)
{
    ui->frameSaisieRendu->setVisible(ouverte);
    const QList<QWidget *> reste = { ui->tableAnimauxHisto, ui->tableHistorique, ui->lineEditRechercheAnimalHisto,
                                     ui->comboFiltreTypeHisto, ui->comboTriHisto,
                                     ui->btnAjouterRendu, ui->btnModifierRendu, ui->btnSupprimerRendu, ui->btnPdfHisto };
    for (QWidget *w : reste)
        w->setEnabled(!ouverte);
    if (!ouverte)
        majBoutons();
}

void HistoriqueServices::fermerSaisie()
{
    basculerSaisie(false);
}

// r : nouveau service rendu (idRdv == 0) ou service rendu à modifier.
// rdvs : rendez-vous proposés (ajout uniquement).
void HistoriqueServices::ouvrirSaisie(const Service &r, const QVector<Service> &rdvs)
{
    const bool edition = r.getIdRdv() > 0;

    // Services proposés : uniquement les services disponibles
    {
        const QSignalBlocker bloc(ui->comboServiceRendu);
        ui->comboServiceRendu->clear();
        for (const Service &s : m_services)
            if (s.isDisponible())
                ui->comboServiceRendu->addItem(QString("%1 — %2").arg(s.getNom(), s.getType()), s.getID());
        if (edition && ui->comboServiceRendu->findData(r.getID()) < 0)
            ui->comboServiceRendu->insertItem(0, QString("%1 (plus proposé)").arg(r.getNom()), r.getID());
        if (ui->comboServiceRendu->count() == 0) {
            QMessageBox::information(m_fenetre, "Aucun service",
                                     "Aucun service disponible. Ajoutez d'abord un service dans la gestion des services.");
            return;
        }
        if (edition)
            ui->comboServiceRendu->setCurrentIndex(ui->comboServiceRendu->findData(r.getID()));
    }

    m_edition = edition;
    m_renduEnCours = r;
    m_ancienIdService = r.getID();

    // Ajout : liste des rendez-vous ; modification : rendez-vous affiché en lecture seule.
    ui->formLayoutSaisieRendu->setRowVisible(ui->comboRdv, !edition);
    ui->formLayoutSaisieRendu->setRowVisible(ui->labelRdv, edition);
    ui->comboRdv->clear();
    if (edition) {
        ui->labelRdv->setText(QString("RDV n°%1 — %2").arg(r.getIdRdv()).arg(formatDate(r.getDateRdv())));
        ui->labelTitreSaisieRendu->setText("Modifier le service rendu");
        ui->lineEditPrixRendu->setText(QString::number(r.getPrixApplique(), 'f', 2));
    } else {
        for (const Service &v : rdvs)
            ui->comboRdv->addItem(QString("%1 — %2 (RDV n°%3)").arg(formatDate(v.getDateRdv()), v.getTypeRdv()).arg(v.getIdRdv()), v.getIdRdv());
        ui->labelTitreSaisieRendu->setText("Ajouter un service rendu");
        if (const Service *s = trouverService(ui->comboServiceRendu->currentData().toInt()))
            ui->lineEditPrixRendu->setText(QString::number(s->getPrix(), 'f', 2));
    }
    ui->plainTextEditRemarqueRendu->setPlainText(r.getRemarque());

    basculerSaisie(true);
    ui->lineEditPrixRendu->setFocus();
}

// ---------------------------------------------------------------------------
// 5. ACCÈS AUX DONNÉES AFFICHÉES (dernier résultat lu en base)
// ---------------------------------------------------------------------------

const Service *HistoriqueServices::trouverAnimal(int id) const
{
    for (const Service &a : m_animaux)
        if (a.getIdAnimal() == id)
            return &a;
    return nullptr;
}

const Service *HistoriqueServices::trouverRendu(int idRdv, int idService) const
{
    for (const Service &r : m_rendus)
        if (r.getIdRdv() == idRdv && r.getID() == idService)
            return &r;
    return nullptr;
}

const Service *HistoriqueServices::trouverService(int id) const
{
    for (const Service &s : m_services)
        if (s.getID() == id)
            return &s;
    return nullptr;
}

int HistoriqueServices::animalCourantId() const
{
    const QList<QTableWidgetItem *> sel = ui->tableAnimauxHisto->selectedItems();
    if (sel.isEmpty())
        return -1;
    return ui->tableAnimauxHisto->item(sel.first()->row(), 0)->data(Qt::UserRole).toInt();
}

// La clé d'un service rendu est (ID_RDV, ID_SERVICE) : stockée dans la 1re cellule de la ligne.
bool HistoriqueServices::renduCourant(int &idRdv, int &idService) const
{
    const QList<QTableWidgetItem *> sel = ui->tableHistorique->selectedItems();
    if (sel.isEmpty())
        return false;
    const QTableWidgetItem *it = ui->tableHistorique->item(sel.first()->row(), 0);
    idRdv = it->data(Qt::UserRole).toInt();
    idService = it->data(Qt::UserRole + 1).toInt();
    return true;
}

// ===========================================================================
// StatistiquesServices : page « Statistiques des services »
// ===========================================================================
// L'interface (titres, cartes, graphiques) est définie dans gsmartpetcare.ui ;
// les graphiques sont des widgets promus de type GraphiqueService.

StatistiquesServices::StatistiquesServices(Ui::GSmartPetCare *uiPrincipal, QWidget *fenetre, QObject *parent)
    : QObject(parent)
    , ui(uiPrincipal)
    , m_fenetre(fenetre)
{
    ui->pageStatistiquesServices->setAttribute(Qt::WA_StyledBackground, true);   // fond de la feuille de style

    // Fenêtre centrée au milieu de l'écran : seule la croix la ferme.
    m_dialogue = creerFenetre(fenetre, "Statistiques des services", ui->scrollStatistiquesServices);
}

void StatistiquesServices::ouvrir()
{
    calculer();                              // données relues dans Oracle à chaque ouverture
    afficherCentree(m_dialogue, m_fenetre, 1100, 720);
}

void StatistiquesServices::calculer()
{
    const QVariantMap st = Service::statistiques();
    if (!st.value("ok").toBool())
        return;
    ui->graphiqueTypeServ->setAnneau(st.value("parType").value<QMap<QString, int>>());
    ui->graphiquePrixServ->setBarresVerticales(st.value("tranchesPrix").value<QVector<QPair<QString, double>>>());
    ui->graphiqueDureeServ->setBarresHorizontales(st.value("dureeMoyenneParType").value<QVector<QPair<QString, double>>>());
}

// ===========================================================================
// GraphiqueService : widget de dessin des graphiques (promu dans gsmartpetcare.ui)
// ===========================================================================

GraphiqueService::GraphiqueService(QWidget *parent)
    : QWidget(parent)
{
    // Taille minimale et politique d'expansion : définies dans gsmartpetcare.ui
}

void GraphiqueService::setAnneau(const QMap<QString, int> &data)
{
    m_mode = Anneau;
    m_labels.clear();
    m_values.clear();
    for (auto it = data.cbegin(); it != data.cend(); ++it) {
        m_labels << it.key();
        m_values << double(it.value());
    }
    update();
}

void GraphiqueService::setBarresVerticales(const QVector<QPair<QString, double>> &data)
{
    setBarres(BarresVerticales, data);
}

void GraphiqueService::setBarresHorizontales(const QVector<QPair<QString, double>> &data)
{
    setBarres(BarresHorizontales, data);
}

void GraphiqueService::setBarres(Mode mode, const QVector<QPair<QString, double>> &data)
{
    m_mode = mode;
    m_labels.clear();
    m_values.clear();
    for (const auto &v : data) {
        m_labels << v.first;
        m_values << v.second;
    }
    update();
}

void GraphiqueService::paintEvent(QPaintEvent *)
{
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing); p.fillRect(rect(), Qt::white);
        if (m_values.isEmpty()) { p.setPen(QColor("#6B7A78")); p.drawText(rect(), Qt::AlignCenter, QObject::tr("Aucune donnée disponible")); return; }
        const QVector<QColor> colors={QColor("#0B7A75"),QColor("#F28C28"),QColor("#3B82F6"),QColor("#A855F7"),QColor("#E45756"),QColor("#14B8A6"),QColor("#64748B")};
        if (m_mode==Anneau) {
            double total=0; for(double v:m_values) total+=qMax(0.0,v); if(total<=0) total=1;
            int side=qMin(width()-130,height()-20); side=qMax(80,side); QRectF r((width()-side)/2.0-40,(height()-side)/2.0,side,side);
            int start=0; for(int i=0;i<m_values.size();++i){int span=qRound(360.0* qMax(0.0,m_values[i])/total*16); p.setBrush(colors[i%colors.size()]);p.setPen(Qt::white);p.drawPie(r,start,span);start+=span;}
            QRectF inner=r.adjusted(side*.28,side*.28,-side*.28,-side*.28);p.setBrush(Qt::white);p.setPen(Qt::NoPen);p.drawEllipse(inner);
            int y=20; int x=qMin(width()-120, int(r.right()+12)); p.setFont(QFont("Arial",9));
            for(int i=0;i<m_labels.size();++i){p.setPen(Qt::NoPen);p.setBrush(colors[i%colors.size()]);p.drawRoundedRect(x,y+2,10,10,2,2);p.setPen(QColor("#334155"));p.drawText(x+15,y+12,QString("%1 (%2)").arg(m_labels[i]).arg(m_values[i],0,'g',4));y+=22;if(y>height()-12)break;}
            return;
        }
        const int left=m_mode==BarresHorizontales?105:42, right=18, top=14, bottom=m_mode==BarresVerticales?50:24;
        QRect plot(left,top,qMax(20,width()-left-right),qMax(20,height()-top-bottom));
        double maxv=1;for(double v:m_values)maxv=qMax(maxv,v);
        p.setFont(QFont("Arial",8));p.setPen(QColor("#94A3B8"));
        if(m_mode==BarresVerticales){
            p.drawLine(plot.left(),plot.bottom(),plot.right(),plot.bottom());int n=m_values.size();double slot=double(plot.width())/n;int bw=qMax(8,int(slot*.58));
            for(int i=0;i<n;++i){int bh=int((qMax(0.0,m_values[i])/maxv)*(plot.height()-8));int x=int(plot.left()+i*slot+(slot-bw)/2);QRect b(x,plot.bottom()-bh,bw,bh);p.fillRect(b,colors[i%colors.size()]);p.setPen(QColor("#334155"));p.drawText(x-10,plot.bottom()+5,bw+20,35,Qt::AlignHCenter|Qt::AlignTop,m_labels.value(i));p.setPen(QColor("#64748B"));p.drawText(x-8,b.top()-17,bw+16,15,Qt::AlignHCenter,QString::number(m_values[i],'f',0));}
        } else {
            int n=m_values.size();double slot=double(plot.height())/n;int bh=qMax(8,int(slot*.55));
            for(int i=0;i<n;++i){int bw=int((qMax(0.0,m_values[i])/maxv)*(plot.width()-8));int y=int(plot.top()+i*slot+(slot-bh)/2);QRect b(plot.left(),y,bw,bh);p.fillRect(b,colors[i%colors.size()]);p.setPen(QColor("#334155"));p.drawText(5,y-1,left-12,bh+3,Qt::AlignVCenter|Qt::AlignRight,m_labels.value(i));p.drawText(qMin(plot.right()-35,b.right()+5),y-1,38,bh+3,Qt::AlignVCenter,QString::number(m_values[i],'f',1));}
        }
}
