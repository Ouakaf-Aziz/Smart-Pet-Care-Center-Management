#include "gestionrdv.h"
#include "ui_gsmartpetcare.h"
#include "statistiquesdialog.h"

#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QTextDocument>
#include <QTime>
#include <QUrl>

namespace {

enum Colonne { ColId, ColStatut, ColPriorite, ColDuree, ColType, ColSalle,
               ColHeure, ColDate, ColAction, NbColonnes };

void selectionnerCombo(QComboBox *combo, const QString &texte)
{
    for (int i = 0; i < combo->count(); ++i) {
        if (combo->itemText(i).trimmed().compare(texte.trimmed(), Qt::CaseInsensitive) == 0) {
            combo->setCurrentIndex(i);
            return;
        }
    }
}

} // namespace

GestionRdv::GestionRdv(Ui::GSmartPetCare *uiPrincipal, QWidget *fenetre, QObject *parent)
    : QObject(parent)
    , ui(uiPrincipal)
    , m_fenetre(fenetre)
{
    ui->pageRDV->setAttribute(Qt::WA_StyledBackground, true);   // fond de la feuille de style

    // --- Connexions ---
    connect(ui->pushButtonEnregistrerRdv,  &QPushButton::clicked, this, &GestionRdv::enregistrerRdv);
    connect(ui->pushButtonAnnulerRdv,      &QPushButton::clicked, this, &GestionRdv::annuler);
    connect(ui->pushButtonStatistiquesRdv, &QPushButton::clicked, this, &GestionRdv::afficherStatistiques);
    connect(ui->pushButtonExporterPdfRdv,  &QPushButton::clicked, this, &GestionRdv::exporterPdf);

    configurerTableau();
    remplirExemples();
    nouveauFormulaire();              // le panneau de droite démarre en mode « Ajouter »
}

// ---------------------------------------------------------------------------
// Tableau
// ---------------------------------------------------------------------------
void GestionRdv::configurerTableau()
{
    QTableWidget *t = ui->tableWidgetRdv;

    QHeaderView *h = t->horizontalHeader();
    h->setMinimumSectionSize(40);
    h->setStretchLastSection(false);
    h->setFixedHeight(44);                                   // même hauteur que la page des services
    h->setDefaultAlignment(Qt::AlignCenter);

    h->setSectionResizeMode(ColId,       QHeaderView::Fixed);    h->resizeSection(ColId, 68);
    h->setSectionResizeMode(ColStatut,   QHeaderView::Fixed);    h->resizeSection(ColStatut, 72);
    h->setSectionResizeMode(ColPriorite, QHeaderView::Fixed);    h->resizeSection(ColPriorite, 80);
    h->setSectionResizeMode(ColDuree,    QHeaderView::Fixed);    h->resizeSection(ColDuree, 58);
    h->setSectionResizeMode(ColType,     QHeaderView::Stretch);                       // colonne qui s'étire
    h->setSectionResizeMode(ColSalle,    QHeaderView::Fixed);    h->resizeSection(ColSalle, 50);
    h->setSectionResizeMode(ColHeure,    QHeaderView::Fixed);    h->resizeSection(ColHeure, 54);
    h->setSectionResizeMode(ColDate,     QHeaderView::Fixed);    h->resizeSection(ColDate, 80);
    h->setSectionResizeMode(ColAction,   QHeaderView::Fixed);    h->resizeSection(ColAction, 172);
}

void GestionRdv::remplirLigne(int row, const QStringList &valeurs)
{
    QTableWidget *t = ui->tableWidgetRdv;

    for (int col = 0; col < valeurs.size() && col < ColAction; ++col) {
        auto *item = new QTableWidgetItem(valeurs[col]);
        item->setTextAlignment(Qt::AlignCenter);
        item->setToolTip(valeurs[col]);
        t->setItem(row, col, item);
    }

    const bool annule = valeurs[ColStatut].startsWith(QString::fromUtf8("annul"));
    t->item(row, ColStatut)->setForeground(annule ? QColor("#B91C1C") : QColor("#0F766E"));
}

void GestionRdv::remplirExemples()
{
    const QList<QStringList> exemples = {
        { "RDV001", QString::fromUtf8("confirmé"), QString::fromUtf8("urgent"),      "30", "consultation", "1", "09:00", "05/10/2026" },
        { "RDV002", QString::fromUtf8("confirmé"), QString::fromUtf8("trés urgent"), "45", "controle",     "2", "09:45", "05/10/2026" },
        { "RDV003", QString::fromUtf8("annulé"),   "",                               "20", "vaccination",  "1", "10:30", "06/10/2026" },
        { "RDV004", QString::fromUtf8("confirmé"), QString::fromUtf8("urgent"),      "60", "toilettage",   "2", "11:00", "06/10/2026" },
        { "RDV005", QString::fromUtf8("confirmé"), "",                               "30", "consultation", "1", "14:00", "07/10/2026" },
        { "RDV006", QString::fromUtf8("annulé"),   QString::fromUtf8("urgent"),      "25", "vaccination",  "2", "15:15", "08/10/2026" },
        { "RDV007", QString::fromUtf8("confirmé"), QString::fromUtf8("trés urgent"), "40", "controle",     "1", "16:00", "09/10/2026" },
    };

    ui->tableWidgetRdv->setRowCount(0);

    for (const QStringList &valeurs : exemples) {
        const int row = ui->tableWidgetRdv->rowCount();
        ui->tableWidgetRdv->insertRow(row);
        remplirLigne(row, valeurs);
        ajouterBoutonsAction(row);
    }
}

// Boutons « Modifier » / « Supprimer » de la colonne Actions.
// Leur style vient de la feuille de style de pageRDV (btnModifierLigne / btnSupprimerLigne).
void GestionRdv::ajouterBoutonsAction(int row)
{
    QTableWidget *t = ui->tableWidgetRdv;

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
    t->setCellWidget(row, ColAction, conteneur);

    // La ligne peut changer (suppression d'une ligne au-dessus) : on la retrouve à chaque clic.
    auto ligneCourante = [this, conteneur]() {
        return ui->tableWidgetRdv->indexAt(conteneur->pos()).row();
    };
    connect(btnModifier,  &QPushButton::clicked, this, [this, ligneCourante]() {
        const int r = ligneCourante();
        if (r >= 0) modifierLigne(r);
    });
    connect(btnSupprimer, &QPushButton::clicked, this, [this, ligneCourante]() {
        const int r = ligneCourante();
        if (r >= 0) supprimerLigne(r);
    });
}

// ---------------------------------------------------------------------------
// Formulaire
// ---------------------------------------------------------------------------

// Mode « Ajouter » : formulaire vide, prêt pour un nouveau rendez-vous.
void GestionRdv::nouveauFormulaire()
{
    m_ligneEnModification = -1;
    ui->labelDetailTitreRdv->setText("Ajouter un rendez-vous");
    ui->pushButtonEnregistrerRdv->setText("Enregistrer");

    ui->lineEditIdRdv->clear();
    ui->dateEditRdv->setDate(QDate::currentDate());
    ui->timeEditRdv->setTime(QTime(9, 0));
    ui->comboBoxTypeRdv->setCurrentIndex(0);
    ui->spinBoxDureeRdv->setValue(0);
    ui->comboBoxPrioriteRdv->setCurrentIndex(0);
    ui->comboBoxSalleRdv->setCurrentIndex(0);
    ui->comboBoxStatutRdv->setCurrentIndex(0);
    ui->checkBoxEmailRdv->setChecked(false);
}

// Mode « Modifier » : formulaire rempli avec le rendez-vous choisi.
void GestionRdv::modifierLigne(int r)
{
    QTableWidget *t = ui->tableWidgetRdv;
    auto texte = [t, r](int col) { return t->item(r, col) ? t->item(r, col)->text() : QString(); };

    ui->lineEditIdRdv->setText(texte(ColId));
    selectionnerCombo(ui->comboBoxStatutRdv,   texte(ColStatut));
    selectionnerCombo(ui->comboBoxPrioriteRdv, texte(ColPriorite));
    ui->spinBoxDureeRdv->setValue(texte(ColDuree).toInt());
    selectionnerCombo(ui->comboBoxTypeRdv,     texte(ColType));
    selectionnerCombo(ui->comboBoxSalleRdv,    texte(ColSalle));
    ui->timeEditRdv->setTime(QTime::fromString(texte(ColHeure), "HH:mm"));
    ui->dateEditRdv->setDate(QDate::fromString(texte(ColDate), "dd/MM/yyyy"));

    m_ligneEnModification = r;
    ui->labelDetailTitreRdv->setText("Modifier le rendez-vous");
    ui->pushButtonEnregistrerRdv->setText(QString::fromUtf8("Mettre à jour"));
    t->selectRow(r);
}

// Bouton « Annuler » : abandonne la saisie (ou la modification) en cours.
void GestionRdv::annuler()
{
    nouveauFormulaire();
    ui->tableWidgetRdv->clearSelection();
}

// Bouton « Enregistrer » : ajoute une ligne (ou met à jour la ligne en modification)
void GestionRdv::enregistrerRdv()
{
    QTableWidget *t = ui->tableWidgetRdv;

    const QString id = ui->lineEditIdRdv->text().trimmed();
    if (id.isEmpty()) {
        QMessageBox::warning(m_fenetre, "Champ obligatoire", "Veuillez saisir l'ID du rendez-vous.");
        ui->lineEditIdRdv->setFocus();
        return;
    }
    if (ui->spinBoxDureeRdv->value() <= 0) {
        QMessageBox::warning(m_fenetre, "Champ obligatoire",
                             QString::fromUtf8("Veuillez saisir une durée supérieure à 0."));
        ui->spinBoxDureeRdv->setFocus();
        return;
    }

    for (int r = 0; r < t->rowCount(); ++r) {
        if (r != m_ligneEnModification &&
            t->item(r, ColId)->text().compare(id, Qt::CaseInsensitive) == 0) {
            QMessageBox::warning(m_fenetre, "ID existant",
                                 QString::fromUtf8("Un rendez-vous avec cet ID existe déjà."));
            ui->lineEditIdRdv->setFocus();
            return;
        }
    }

    const QStringList valeurs = {
        id,
        ui->comboBoxStatutRdv->currentText().trimmed(),
        ui->comboBoxPrioriteRdv->currentText().trimmed(),
        QString::number(ui->spinBoxDureeRdv->value()),
        ui->comboBoxTypeRdv->currentText().trimmed(),
        ui->comboBoxSalleRdv->currentText().trimmed(),
        ui->timeEditRdv->time().toString("HH:mm"),
        ui->dateEditRdv->date().toString("dd/MM/yyyy")
    };

    int ligne;
    if (m_ligneEnModification >= 0) {
        ligne = m_ligneEnModification;
        remplirLigne(ligne, valeurs);
    } else {
        ligne = t->rowCount();
        t->insertRow(ligne);
        remplirLigne(ligne, valeurs);
        ajouterBoutonsAction(ligne);
        t->scrollToBottom();
    }

    nouveauFormulaire();
    t->selectRow(ligne);
}

void GestionRdv::supprimerLigne(int r)
{
    QTableWidget *t = ui->tableWidgetRdv;

    const auto rep = QMessageBox::question(
        m_fenetre, "Supprimer",
        "Voulez-vous vraiment supprimer le rendez-vous " + t->item(r, ColId)->text() + " ?");
    if (rep != QMessageBox::Yes)
        return;

    t->removeRow(r);

    if (r == m_ligneEnModification)
        nouveauFormulaire();                 // retour au mode « Ajouter »
    else if (r < m_ligneEnModification)
        --m_ligneEnModification;
}

// ---------------------------------------------------------------------------
// Statistiques
// ---------------------------------------------------------------------------
void GestionRdv::afficherStatistiques()
{
    QTableWidget *t = ui->tableWidgetRdv;
    QVector<RdvStat> donnees;

    for (int r = 0; r < t->rowCount(); ++r) {
        if (!t->item(r, ColType) || !t->item(r, ColStatut) || !t->item(r, ColDuree))
            continue;

        auto texte = [t, r](int col) { return t->item(r, col) ? t->item(r, col)->text() : QString(); };

        RdvStat s;
        s.type = texte(ColType);
        s.statut = texte(ColStatut);
        s.priorite = texte(ColPriorite);
        s.salle = texte(ColSalle);
        s.duree = texte(ColDuree).toInt();
        s.date = QDate::fromString(texte(ColDate), "dd/MM/yyyy");
        donnees.append(s);
    }

    StatistiquesDialog dlg(donnees, m_fenetre);
    dlg.exec();
}

// ---------------------------------------------------------------------------
// Export PDF
// ---------------------------------------------------------------------------

// Bouton « Exporter PDF » : demande où enregistrer le fichier puis génère le PDF
void GestionRdv::exporterPdf()
{
    if (ui->tableWidgetRdv->rowCount() == 0) {
        QMessageBox::information(m_fenetre, "Exporter PDF", "Aucun rendez-vous à exporter.");
        return;
    }

    const QString suggestion = QDir::homePath() + "/rendez-vous_" +
                               QDate::currentDate().toString("yyyy-MM-dd") + ".pdf";

    QString chemin = QFileDialog::getSaveFileName(
        m_fenetre, QString::fromUtf8("Exporter la liste des rendez-vous"),
        suggestion, "Fichiers PDF (*.pdf)");

    if (chemin.isEmpty())
        return;   // l'utilisateur a annulé

    if (!chemin.endsWith(".pdf", Qt::CaseInsensitive))
        chemin += ".pdf";

    if (!genererPdf(chemin)) {
        QMessageBox::critical(m_fenetre, "Exporter PDF",
                              "Impossible de créer le fichier PDF.\n"
                              "Vérifiez que le fichier n'est pas ouvert dans un autre programme.");
        return;
    }

    const auto rep = QMessageBox::question(
        m_fenetre, "Exporter PDF",
        QString::fromUtf8("Le PDF a été créé avec succès.\n\nVoulez-vous l'ouvrir ?"));
    if (rep == QMessageBox::Yes)
        QDesktopServices::openUrl(QUrl::fromLocalFile(chemin));
}

// Construit le document (titre + tableau) et l'écrit dans le fichier PDF
bool GestionRdv::genererPdf(const QString &chemin)
{
    QTableWidget *t = ui->tableWidgetRdv;

    // Colonnes exportées (la colonne Actions est ignorée)
    const QStringList entetes = {
        "Id", "Statut", QString::fromUtf8("Priorité"), QString::fromUtf8("Durée (min)"),
        "Type", "Salle", "Heure", "Date"
    };
    const QList<int> largeurs = { 11, 13, 14, 12, 16, 8, 11, 15 };   // en %

    QString html;
    html += "<h1 style='color:#1B5E4B;'>PetNova - Liste des rendez-vous</h1>";
    html += QString("<p style='color:#6B7280;'>")
            + QString::fromUtf8("Exporté le ")
            + QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm")
            + QString(" - %1 rendez-vous</p>").arg(t->rowCount());

    html += "<table width='100%' cellspacing='0' cellpadding='7'>";

    // En-tête vert
    html += "<tr>";
    for (int i = 0; i < entetes.size(); ++i) {
        html += QString("<td width='%1%' align='center' "
                        "style='background-color:#1B5E4B; color:white; font-weight:bold;'>%2</td>")
                    .arg(largeurs[i]).arg(entetes[i].toHtmlEscaped());
    }
    html += "</tr>";

    // Lignes de données
    for (int r = 0; r < t->rowCount(); ++r) {
        const QString fond = (r % 2 == 0) ? "#FFFFFF" : "#EEF3F1";
        html += "<tr>";
        for (int col = 0; col < entetes.size(); ++col) {
            const QTableWidgetItem *it = t->item(r, col);
            const QString texte = it ? it->text() : QString();

            QString style = QString("background-color:%1; color:#1F2A37;").arg(fond);
            if (col == ColStatut) {
                const bool annule = texte.startsWith(QString::fromUtf8("annul"));
                style += QString(" color:%1; font-weight:bold;").arg(annule ? "#B91C1C" : "#0F766E");
            }
            html += QString("<td align='center' style='%1'>%2</td>")
                        .arg(style, texte.toHtmlEscaped());
        }
        html += "</tr>";
    }
    html += "</table>";

    {
        QPdfWriter writer(chemin);
        writer.setPageSize(QPageSize(QPageSize::A4));
        writer.setPageOrientation(QPageLayout::Landscape);
        writer.setPageMargins(QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter);
        writer.setTitle(QString::fromUtf8("Liste des rendez-vous - PetNova"));

        QTextDocument doc;
        doc.setHtml(html);
        doc.print(&writer);
    }   // le fichier est finalisé ici

    const QFileInfo info(chemin);
    return info.exists() && info.size() > 0;
}
