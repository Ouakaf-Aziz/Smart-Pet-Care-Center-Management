#include "historiqueservices.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPageSize>
#include <QPdfWriter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTextDocument>
#include <QVBoxLayout>

#include <algorithm>

namespace {

const char *kStyleDialogue = R"(
QDialog { background-color: #F4F8F7; }
QWidget { font-family: "Arial"; font-size: 11pt; color: #1F2A37; }
QLineEdit, QComboBox, QDateEdit, QPlainTextEdit {
    background-color: #FFFFFF; border: 1px solid #CBD5D3; border-radius: 6px; padding: 5px 8px;
}
QLineEdit:focus, QComboBox:focus, QDateEdit:focus, QPlainTextEdit:focus { border: 2px solid #0F766E; }
QComboBox::drop-down, QDateEdit::drop-down { border: none; width: 24px; }
QComboBox QAbstractItemView {
    background-color: #FFFFFF; border: 1px solid #CBD5D3;
    selection-background-color: #CDEAE6; selection-color: #1F2A37; outline: none;
}
#carteAnimaux, #carteHistorique {
    background-color: #FFFFFF; border: 1px solid #CBD5D3; border-radius: 12px;
}
#titrePage { font-size: 20pt; font-weight: bold; color: #0B4F4A; }
#titreCarte { font-size: 13pt; font-weight: bold; color: #0B4F4A; }
#statBox { background-color: #EAF2F1; border-radius: 8px; padding: 8px; }
QTableWidget {
    background-color: #FFFFFF; border: 1px solid #CBD5D3; border-radius: 8px; gridline-color: #E3EAE9;
}
QHeaderView::section {
    background-color: #0F766E; color: #FFFFFF; font-weight: bold; padding: 6px; border: none;
}
QTableWidget::item { padding: 4px; }
QTableWidget::item:selected { background-color: #CDEAE6; color: #1F2A37; }
QScrollBar:vertical { background: #F4F8F7; width: 10px; margin: 2px; }
QScrollBar::handle:vertical { background: #CBD5D3; border-radius: 5px; min-height: 24px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
)";

QPushButton *creerBouton(const QString &texte, const QString &fond, const QString &survol, QWidget *parent)
{
    auto *b = new QPushButton(texte, parent);
    b->setMinimumHeight(38);
    b->setCursor(Qt::PointingHandCursor);
    b->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: #FFFFFF; font-weight: bold; border: none;"
        " border-radius: 6px; padding: 0 14px; }"
        "QPushButton:hover { background-color: %2; }"
        "QPushButton:disabled { background-color: #C9D2D0; color: #F4F8F7; }")
        .arg(fond, survol));
    return b;
}

void configurerTable(QTableWidget *t)
{
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setAlternatingRowColors(true);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->verticalHeader()->setVisible(false);
    t->horizontalHeader()->setStretchLastSection(false);
}

QString formatDate(const QDate &d) { return d.toString("dd/MM/yyyy"); }

} // namespace

// ---------------------------------------------------------------------------
HistoriqueServices::HistoriqueServices(const QVector<Service> &services,
                                       DonneesHistorique &donnees,
                                       int animalIdInitial,
                                       QWidget *parent)
    : QDialog(parent)
    , m_services(services)
    , m_d(donnees)
{
    setWindowTitle("Historique des services rendus");
    setMinimumSize(1100, 650);
    resize(1200, 720);
    setStyleSheet(kStyleDialogue);

    construireInterface();
    remplirFiltreType();
    refreshAnimaux();

    if (animalIdInitial >= 0) {
        for (int r = 0; r < m_tableAnimaux->rowCount(); ++r)
            if (m_tableAnimaux->item(r, 0)->data(Qt::UserRole).toInt() == animalIdInitial) {
                m_tableAnimaux->selectRow(r);
                break;
            }
    }
}

void HistoriqueServices::construireInterface()
{
    auto *racine = new QVBoxLayout(this);
    racine->setContentsMargins(28, 24, 28, 24);
    racine->setSpacing(14);

    auto *titre = new QLabel("Historique des services rendus", this);
    titre->setObjectName("titrePage");
    racine->addWidget(titre);

    auto *contenu = new QHBoxLayout;
    contenu->setSpacing(16);
    racine->addLayout(contenu, 1);

    // ------------------------- Colonne gauche : animaux -------------------------
    auto *carteAnimaux = new QFrame(this);
    carteAnimaux->setObjectName("carteAnimaux");
    carteAnimaux->setFixedWidth(400);
    auto *lg = new QVBoxLayout(carteAnimaux);
    lg->setContentsMargins(16, 16, 16, 16);
    lg->setSpacing(10);

    auto *titreAnimaux = new QLabel("Animaux", carteAnimaux);
    titreAnimaux->setObjectName("titreCarte");
    lg->addWidget(titreAnimaux);

    m_rechercheAnimal = new QLineEdit(carteAnimaux);
    m_rechercheAnimal->setPlaceholderText("Rechercher un animal ou un propriétaire...");
    m_rechercheAnimal->setClearButtonEnabled(true);
    lg->addWidget(m_rechercheAnimal);

    m_tableAnimaux = new QTableWidget(0, 3, carteAnimaux);
    m_tableAnimaux->setHorizontalHeaderLabels({"Nom", "Espèce", "Propriétaire"});
    configurerTable(m_tableAnimaux);
    QHeaderView *ha = m_tableAnimaux->horizontalHeader();
    ha->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ha->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ha->setSectionResizeMode(2, QHeaderView::Stretch);
    lg->addWidget(m_tableAnimaux, 1);

    auto *noteAnimaux = new QLabel("Liste fournie par le module Animal.", carteAnimaux);
    noteAnimaux->setStyleSheet("font-size: 9pt; color: #6B7A78;");
    lg->addWidget(noteAnimaux);

    contenu->addWidget(carteAnimaux);

    // ------------------------- Colonne droite : historique ----------------------
    auto *carteHisto = new QFrame(this);
    carteHisto->setObjectName("carteHistorique");
    auto *ld = new QVBoxLayout(carteHisto);
    ld->setContentsMargins(16, 16, 16, 16);
    ld->setSpacing(10);

    m_labelFiche = new QLabel(carteHisto);
    m_labelFiche->setObjectName("titreCarte");
    m_labelFiche->setTextFormat(Qt::RichText);
    ld->addWidget(m_labelFiche);

    auto *stats = new QHBoxLayout;
    stats->setSpacing(10);
    m_labelNbServices = new QLabel(carteHisto);
    m_labelTotal = new QLabel(carteHisto);
    m_labelDernier = new QLabel(carteHisto);
    for (QLabel *l : {m_labelNbServices, m_labelTotal, m_labelDernier}) {
        l->setObjectName("statBox");
        l->setTextFormat(Qt::RichText);
        stats->addWidget(l, 1);
    }
    ld->addLayout(stats);

    auto *filtre = new QHBoxLayout;
    auto *lblFiltre = new QLabel("Filtrer par type :", carteHisto);
    m_filtreType = new QComboBox(carteHisto);
    m_filtreType->setMinimumWidth(200);
    filtre->addWidget(lblFiltre);
    filtre->addWidget(m_filtreType);
    filtre->addStretch(1);
    ld->addLayout(filtre);

    m_tableHistorique = new QTableWidget(0, 5, carteHisto);
    m_tableHistorique->setHorizontalHeaderLabels({"Date", "Service", "Type", "Prix (DT)", "Remarque"});
    configurerTable(m_tableHistorique);
    QHeaderView *hh = m_tableHistorique->horizontalHeader();
    hh->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    hh->setSectionResizeMode(1, QHeaderView::Stretch);
    hh->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    hh->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    hh->setSectionResizeMode(4, QHeaderView::Stretch);
    ld->addWidget(m_tableHistorique, 1);

    auto *boutonsHisto = new QHBoxLayout;
    boutonsHisto->setSpacing(10);
    m_btnAjouterRendu = creerBouton("Ajouter un service rendu", "#0F766E", "#0C5F58", carteHisto);
    m_btnModifierRendu = creerBouton("Modifier", "#F28C28", "#E07B16", carteHisto);
    m_btnSupprimerRendu = creerBouton("Supprimer", "#C0392B", "#A5311F", carteHisto);
    m_btnPdf = creerBouton("Exporter PDF", "#0B4F4A", "#083E3A", carteHisto);
    boutonsHisto->addWidget(m_btnAjouterRendu);
    boutonsHisto->addWidget(m_btnModifierRendu);
    boutonsHisto->addWidget(m_btnSupprimerRendu);
    boutonsHisto->addWidget(m_btnPdf);
    boutonsHisto->addStretch(1);
    ld->addLayout(boutonsHisto);

    contenu->addWidget(carteHisto, 1);

    // ------------------------------ Connexions ---------------------------------
    connect(m_rechercheAnimal, &QLineEdit::textChanged, this, &HistoriqueServices::refreshAnimaux);
    connect(m_tableAnimaux, &QTableWidget::itemSelectionChanged, this, &HistoriqueServices::refreshHistorique);
    connect(m_filtreType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HistoriqueServices::refreshHistorique);
    connect(m_tableHistorique, &QTableWidget::itemSelectionChanged, this, &HistoriqueServices::majBoutons);

    connect(m_btnAjouterRendu, &QPushButton::clicked, this, &HistoriqueServices::onAjouterRendu);
    connect(m_btnModifierRendu, &QPushButton::clicked, this, &HistoriqueServices::onModifierRendu);
    connect(m_btnSupprimerRendu, &QPushButton::clicked, this, &HistoriqueServices::onSupprimerRendu);
    connect(m_btnPdf, &QPushButton::clicked, this, &HistoriqueServices::onExporterPdf);
}

void HistoriqueServices::remplirFiltreType()
{
    QSet<QString> types;
    for (const Service &s : m_services)
        types.insert(s.type);
    for (const ServiceRendu &r : m_d.rendus)
        types.insert(r.type);
    QStringList liste = types.values();
    liste.sort(Qt::CaseInsensitive);

    const QSignalBlocker b(m_filtreType);
    m_filtreType->clear();
    m_filtreType->addItem("Tous les types");
    m_filtreType->addItems(liste);
}

// ---------------------------------------------------------------------------
// Accès aux données
// ---------------------------------------------------------------------------
Animal *HistoriqueServices::trouverAnimal(int id)
{
    for (Animal &a : m_d.animaux)
        if (a.id == id)
            return &a;
    return nullptr;
}

ServiceRendu *HistoriqueServices::trouverRendu(int id)
{
    for (ServiceRendu &r : m_d.rendus)
        if (r.id == id)
            return &r;
    return nullptr;
}

const Service *HistoriqueServices::trouverService(int id) const
{
    for (const Service &s : m_services)
        if (s.id == id)
            return &s;
    return nullptr;
}

int HistoriqueServices::animalCourantId() const
{
    const QList<QTableWidgetItem *> sel = m_tableAnimaux->selectedItems();
    if (sel.isEmpty())
        return -1;
    return m_tableAnimaux->item(sel.first()->row(), 0)->data(Qt::UserRole).toInt();
}

int HistoriqueServices::renduCourantId() const
{
    const QList<QTableWidgetItem *> sel = m_tableHistorique->selectedItems();
    if (sel.isEmpty())
        return -1;
    return m_tableHistorique->item(sel.first()->row(), 0)->data(Qt::UserRole).toInt();
}

// ---------------------------------------------------------------------------
// Affichage
// ---------------------------------------------------------------------------
void HistoriqueServices::refreshAnimaux()
{
    const int idAvant = animalCourantId();
    const QString q = m_rechercheAnimal->text().trimmed();

    QVector<Animal> vue;
    for (const Animal &a : m_d.animaux) {
        if (!q.isEmpty()
            && !a.nom.contains(q, Qt::CaseInsensitive)
            && !a.espece.contains(q, Qt::CaseInsensitive)
            && !a.proprietaire.contains(q, Qt::CaseInsensitive))
            continue;
        vue.append(a);
    }
    std::sort(vue.begin(), vue.end(), [](const Animal &x, const Animal &y) {
        return QString::localeAwareCompare(x.nom, y.nom) < 0;
    });

    {
        const QSignalBlocker blocker(m_tableAnimaux);
        m_tableAnimaux->setRowCount(vue.size());
        int ligne = -1;
        for (int r = 0; r < vue.size(); ++r) {
            const Animal &a = vue[r];
            auto *itNom = new QTableWidgetItem(a.nom);
            itNom->setData(Qt::UserRole, a.id);
            m_tableAnimaux->setItem(r, 0, itNom);
            m_tableAnimaux->setItem(r, 1, new QTableWidgetItem(a.espece));
            m_tableAnimaux->setItem(r, 2, new QTableWidgetItem(a.proprietaire));
            if (a.id == idAvant)
                ligne = r;
        }
        if (ligne < 0 && !vue.isEmpty())
            ligne = 0;                       // sélection par défaut : premier animal
        if (ligne >= 0)
            m_tableAnimaux->selectRow(ligne);
        else
            m_tableAnimaux->clearSelection();
    }
    refreshHistorique();
}

void HistoriqueServices::refreshHistorique()
{
    const Animal *a = trouverAnimal(animalCourantId());

    {
        const QSignalBlocker blocker(m_tableHistorique);
        m_tableHistorique->setRowCount(0);
    }

    if (!a) {
        m_labelFiche->setText(m_d.animaux.isEmpty()
                                  ? "Aucun animal (liste fournie par le module Animal)"
                                  : "Sélectionnez un animal");
        m_labelNbServices->setText("<span style='font-size:9pt;color:#6B7A78;'>SERVICES RENDUS</span><br><b style='font-size:16pt;color:#0B4F4A;'>—</b>");
        m_labelTotal->setText("<span style='font-size:9pt;color:#6B7A78;'>TOTAL FACTURÉ</span><br><b style='font-size:16pt;color:#0B4F4A;'>—</b>");
        m_labelDernier->setText("<span style='font-size:9pt;color:#6B7A78;'>DERNIER SERVICE</span><br><b style='font-size:16pt;color:#0B4F4A;'>—</b>");
        majBoutons();
        return;
    }

    QString fiche = QString("%1 — %2").arg(a->nom.toHtmlEscaped(), a->espece.toHtmlEscaped());
    fiche += QString("<br><span style='font-size:10pt;font-weight:normal;color:#5C6B69;'>Propriétaire : %1</span>")
                 .arg(a->proprietaire.isEmpty() ? "—" : a->proprietaire.toHtmlEscaped());
    m_labelFiche->setText(fiche);

    // Tous les services de cet animal (les statistiques ne dépendent pas du filtre)
    QVector<ServiceRendu> tous;
    for (const ServiceRendu &r : m_d.rendus)
        if (r.animalId == a->id)
            tous.append(r);
    std::sort(tous.begin(), tous.end(), [](const ServiceRendu &x, const ServiceRendu &y) {
        return x.date != y.date ? x.date > y.date : x.id > y.id;     // le plus récent d'abord
    });

    double total = 0.0;
    for (const ServiceRendu &r : tous)
        total += r.prix;

    m_labelNbServices->setText(QString("<span style='font-size:9pt;color:#6B7A78;'>SERVICES RENDUS</span><br><b style='font-size:16pt;color:#0B4F4A;'>%1</b>").arg(tous.size()));
    m_labelTotal->setText(QString("<span style='font-size:9pt;color:#6B7A78;'>TOTAL FACTURÉ</span><br><b style='font-size:16pt;color:#0B4F4A;'>%1 DT</b>").arg(total, 0, 'f', 2));
    m_labelDernier->setText(QString("<span style='font-size:9pt;color:#6B7A78;'>DERNIER SERVICE</span><br><b style='font-size:16pt;color:#0B4F4A;'>%1</b>")
                                .arg(tous.isEmpty() ? QString("—") : formatDate(tous.first().date)));

    const QString typeFiltre = m_filtreType->currentIndex() > 0 ? m_filtreType->currentText() : QString();

    {
        const QSignalBlocker blocker(m_tableHistorique);
        for (const ServiceRendu &r : tous) {
            if (!typeFiltre.isEmpty() && r.type != typeFiltre)
                continue;
            const int ligne = m_tableHistorique->rowCount();
            m_tableHistorique->insertRow(ligne);

            auto *itDate = new QTableWidgetItem(formatDate(r.date));
            itDate->setData(Qt::UserRole, r.id);
            itDate->setTextAlignment(Qt::AlignCenter);
            auto *itPrix = new QTableWidgetItem(QString::number(r.prix, 'f', 2));
            itPrix->setTextAlignment(Qt::AlignCenter);

            m_tableHistorique->setItem(ligne, 0, itDate);
            m_tableHistorique->setItem(ligne, 1, new QTableWidgetItem(r.nomService));
            m_tableHistorique->setItem(ligne, 2, new QTableWidgetItem(r.type));
            m_tableHistorique->setItem(ligne, 3, itPrix);
            m_tableHistorique->setItem(ligne, 4, new QTableWidgetItem(r.remarque));
        }
        m_tableHistorique->clearSelection();
    }
    majBoutons();
}

void HistoriqueServices::majBoutons()
{
    const bool animal = animalCourantId() >= 0;
    const bool rendu = renduCourantId() >= 0;
    m_btnAjouterRendu->setEnabled(animal);
    m_btnPdf->setEnabled(animal);
    m_btnModifierRendu->setEnabled(rendu);
    m_btnSupprimerRendu->setEnabled(rendu);
}

// ---------------------------------------------------------------------------
// Boîtes de saisie
// ---------------------------------------------------------------------------
namespace {
void styliserBoutonsOkAnnuler(QDialogButtonBox *box)
{
    QPushButton *ok = box->button(QDialogButtonBox::Ok);
    QPushButton *an = box->button(QDialogButtonBox::Cancel);
    ok->setText("Enregistrer");
    an->setText("Annuler");
    ok->setCursor(Qt::PointingHandCursor);
    an->setCursor(Qt::PointingHandCursor);
    ok->setMinimumSize(110, 36);
    an->setMinimumSize(110, 36);
    ok->setStyleSheet("QPushButton { background-color: #0F766E; color: #FFFFFF; font-weight: bold; border: none; border-radius: 6px; }"
                      "QPushButton:hover { background-color: #0C5F58; }");
    an->setStyleSheet("QPushButton { background-color: #9AA6A4; color: #FFFFFF; font-weight: bold; border: none; border-radius: 6px; }"
                      "QPushButton:hover { background-color: #808C8A; }");
}
} // namespace

bool HistoriqueServices::saisirRendu(ServiceRendu &r, const QString &titre)
{
    const bool edition = r.id != 0;

    QDialog dlg(this);
    dlg.setWindowTitle(titre);
    dlg.setMinimumWidth(480);

    auto *comboService = new QComboBox(&dlg);
    for (const Service &s : m_services)
        if (s.disponible)
            comboService->addItem(QString("%1 — %2").arg(s.nom, s.type), s.id);

    if (edition && comboService->findData(r.serviceId) < 0)
        comboService->insertItem(0, QString("%1 (plus proposé)").arg(r.nomService), r.serviceId);

    if (comboService->count() == 0) {
        QMessageBox::information(this, "Aucun service",
                                 "Aucun service disponible. Ajoutez d'abord un service dans la gestion des services.");
        return false;
    }
    if (edition)
        comboService->setCurrentIndex(comboService->findData(r.serviceId));

    auto *date = new QDateEdit(r.date.isValid() ? r.date : QDate::currentDate(), &dlg);
    date->setCalendarPopup(true);
    date->setDisplayFormat("dd/MM/yyyy");
    date->setMaximumDate(QDate::currentDate());

    auto *prix = new QLineEdit(&dlg);
    auto prixDuService = [&]() {
        if (const Service *s = trouverService(comboService->currentData().toInt()))
            prix->setText(QString::number(s->prix, 'f', 2));
    };
    if (edition)
        prix->setText(QString::number(r.prix, 'f', 2));
    else
        prixDuService();
    // En changeant de service, le prix proposé suit (il reste modifiable).
    connect(comboService, QOverload<int>::of(&QComboBox::currentIndexChanged), &dlg,
            [&](int) { prixDuService(); });

    auto *remarque = new QPlainTextEdit(r.remarque, &dlg);
    remarque->setMaximumHeight(90);
    remarque->setPlaceholderText("Observations, vétérinaire, traitement prescrit...");

    auto *form = new QFormLayout;
    form->setSpacing(10);
    form->addRow("Service *", comboService);
    form->addRow("Date *", date);
    form->addRow("Prix appliqué (DT) *", prix);
    form->addRow("Remarque", remarque);

    auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    styliserBoutonsOkAnnuler(box);

    auto *lay = new QVBoxLayout(&dlg);
    lay->setContentsMargins(20, 20, 20, 20);
    lay->setSpacing(16);
    lay->addLayout(form);
    lay->addWidget(box);

    connect(box, &QDialogButtonBox::accepted, &dlg, [&]() {
        bool ok = false;
        QString txt = prix->text().trimmed();
        txt.replace(',', '.');
        const double v = txt.toDouble(&ok);
        if (!ok || v < 0) {
            QMessageBox::warning(&dlg, "Prix invalide", "Saisissez un prix valide (ex. 30 ou 30.50).");
            prix->setFocus();
            return;
        }
        dlg.accept();
    });
    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted)
        return false;

    QString txt = prix->text().trimmed();
    txt.replace(',', '.');

    const int nouveauServiceId = comboService->currentData().toInt();
    // On (re)copie le nom et le type seulement si le service a changé.
    if (!edition || nouveauServiceId != r.serviceId) {
        if (const Service *s = trouverService(nouveauServiceId)) {
            r.nomService = s->nom;
            r.type = s->type;
        }
    }
    r.serviceId = nouveauServiceId;
    r.date = date->date();
    r.prix = txt.toDouble();
    r.remarque = remarque->toPlainText().trimmed();
    return true;
}

// ---------------------------------------------------------------------------
// Actions : services rendus
// ---------------------------------------------------------------------------
void HistoriqueServices::onAjouterRendu()
{
    const int animalId = animalCourantId();
    if (animalId < 0)
        return;

    ServiceRendu r;
    r.animalId = animalId;
    if (!saisirRendu(r, "Ajouter un service rendu"))
        return;
    r.id = m_d.prochainId++;
    m_d.rendus.append(r);

    remplirFiltreType();                      // au cas où un nouveau type apparaît
    refreshHistorique();
}

void HistoriqueServices::onModifierRendu()
{
    ServiceRendu *r = trouverRendu(renduCourantId());
    if (!r) {
        QMessageBox::information(this, "Modifier", "Sélectionnez d'abord une ligne de l'historique.");
        return;
    }
    ServiceRendu copie = *r;
    if (!saisirRendu(copie, "Modifier le service rendu"))
        return;
    *r = copie;
    refreshHistorique();
}

void HistoriqueServices::onSupprimerRendu()
{
    const ServiceRendu *r = trouverRendu(renduCourantId());
    if (!r) {
        QMessageBox::information(this, "Supprimer", "Sélectionnez d'abord une ligne de l'historique.");
        return;
    }
    const int id = r->id;
    if (QMessageBox::question(this, "Supprimer",
                              QString("Supprimer « %1 » du %2 de l'historique ?")
                                  .arg(r->nomService, formatDate(r->date)))
        != QMessageBox::Yes)
        return;

    m_d.rendus.erase(std::remove_if(m_d.rendus.begin(), m_d.rendus.end(),
                                    [id](const ServiceRendu &x) { return x.id == id; }),
                     m_d.rendus.end());
    refreshHistorique();
}

// ---------------------------------------------------------------------------
// Export PDF de la fiche de l'animal
// ---------------------------------------------------------------------------
void HistoriqueServices::onExporterPdf()
{
    const Animal *a = trouverAnimal(animalCourantId());
    if (!a) {
        QMessageBox::information(this, "Export PDF", "Sélectionnez d'abord un animal.");
        return;
    }

    QString nomFichier = "historique_" + a->nom;
    nomFichier.replace(' ', '_');
    QString chemin = QFileDialog::getSaveFileName(this, "Exporter l'historique en PDF",
                                                  nomFichier + ".pdf", "Fichiers PDF (*.pdf)");
    if (chemin.isEmpty())
        return;
    if (!chemin.endsWith(".pdf", Qt::CaseInsensitive))
        chemin += ".pdf";

    QVector<ServiceRendu> liste;
    for (const ServiceRendu &r : m_d.rendus)
        if (r.animalId == a->id)
            liste.append(r);
    std::sort(liste.begin(), liste.end(), [](const ServiceRendu &x, const ServiceRendu &y) {
        return x.date != y.date ? x.date > y.date : x.id > y.id;
    });

    double total = 0.0;
    for (const ServiceRendu &r : liste)
        total += r.prix;

    QString html = QString("<h2 style='color:#0B4F4A;'>Historique des services rendus</h2>"
                           "<p><b>Animal :</b> %1 (%2)<br>"
                           "<b>Propriétaire :</b> %3<br>"
                           "<b>Édité le :</b> %4</p>")
                       .arg(a->nom.toHtmlEscaped(),
                            a->espece.toHtmlEscaped(),
                            a->proprietaire.isEmpty() ? QString("—") : a->proprietaire.toHtmlEscaped(),
                            formatDate(QDate::currentDate()));

    html += "<table border='1' cellspacing='0' cellpadding='5' width='100%'>"
            "<tr style='background-color:#0F766E; color:white;'>"
            "<th>Date</th><th>Service</th><th>Type</th><th>Prix (DT)</th><th>Remarque</th></tr>";
    for (const ServiceRendu &r : liste) {
        html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td align='right'>%4</td><td>%5</td></tr>")
                    .arg(formatDate(r.date),
                         r.nomService.toHtmlEscaped(),
                         r.type.toHtmlEscaped(),
                         QString::number(r.prix, 'f', 2),
                         r.remarque.toHtmlEscaped());
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

    QMessageBox::information(this, "Export PDF", "Le fichier PDF a été créé :\n" + chemin);
}
