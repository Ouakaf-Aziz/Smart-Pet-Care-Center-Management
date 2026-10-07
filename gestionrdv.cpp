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
#include <QStatusBar>
#include <QTableWidgetItem>
#include <QTextDocument>
#include <QTime>
#include <QUrl>
#include <QVector>

namespace {
void selectionnerCombo(QComboBox *combo, const QString &texte)
{
    for (int i = 0; i < combo->count(); ++i) {
        if (combo->itemText(i).trimmed().compare(texte.trimmed(), Qt::CaseInsensitive) == 0) {
            combo->setCurrentIndex(i);
            return;
        }
    }
}

enum Colonne { ColId, ColStatut, ColPriorite, ColDuree, ColType, ColSalle,
               ColHeure, ColDate, ColAction, NbColonnes };
}

GestionRdv::GestionRdv(Ui::GSmartPetCare *uiPrincipal, QMainWindow *fenetre, QObject *parent)
    : QObject(parent)
    , ui(uiPrincipal)
    , m_fenetre(fenetre)
{
    ui->pageRdv->setAttribute(Qt::WA_StyledBackground, true);   // fond de la feuille de style

    ui->dateEditRdv->setDisplayFormat("dd/MM/yyyy");
    ui->dateEditRdv->setCalendarPopup(true);
    ui->dateEditRdv->setDate(QDate::currentDate());
    ui->timeEditRdv->setDisplayFormat("HH:mm");
    ui->spinBoxRdvDuree->setRange(0, 600);

    connect(ui->pushButtonRdvEnregistrer,  &QPushButton::clicked, this, &GestionRdv::enregistrerRdv);
    connect(ui->pushButtonRdvVider,        &QPushButton::clicked, this, &GestionRdv::viderFormulaire);
    connect(ui->pushButtonRdvStatistiques, &QPushButton::clicked, this, &GestionRdv::afficherStatistiques);
    connect(ui->pushButtonRdvExporterPdf,  &QPushButton::clicked, this, &GestionRdv::exporterPdf);

    configurerTableau();
    remplirExemples();
}

void GestionRdv::configurerTableau()
{
    QTableWidget *t = ui->tableWidgetRdv;   // colonnes et libellés : définis dans le .ui

    t->verticalHeader()->setVisible(false);
    t->verticalHeader()->setDefaultSectionSize(52);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setAlternatingRowColors(true);
    t->setShowGrid(true);

    QHeaderView *h = t->horizontalHeader();
    h->setDefaultAlignment(Qt::AlignCenter);
    h->setMinimumSectionSize(60);
    h->setFixedHeight(46);

    // Chaque colonne s'adapte à son en-tête et à son contenu : les attributs restent lisibles.
    h->setSectionResizeMode(QHeaderView::ResizeToContents);
    h->setSectionResizeMode(ColType, QHeaderView::Stretch);   // prend la place restante
    h->setSectionResizeMode(ColAction, QHeaderView::Fixed);   t->setColumnWidth(ColAction, 190);
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

// Données de démonstration (gardées en mémoire, sans base de données)
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

// Bouton « enregistrer » : ajoute une ligne (ou met à jour la ligne en modification)
void GestionRdv::enregistrerRdv()
{
    QTableWidget *t = ui->tableWidgetRdv;

    const QString id = ui->lineEditRdvId->text().trimmed();
    if (id.isEmpty()) {
        QMessageBox::warning(m_fenetre, "Champ obligatoire", "Veuillez saisir l'ID du rendez-vous.");
        ui->lineEditRdvId->setFocus();
        return;
    }
    if (ui->spinBoxRdvDuree->value() <= 0) {
        QMessageBox::warning(m_fenetre, "Champ obligatoire", "Veuillez saisir une durée supérieure à 0.");
        ui->spinBoxRdvDuree->setFocus();
        return;
    }

    for (int r = 0; r < t->rowCount(); ++r) {
        if (r != m_ligneEnModification &&
            t->item(r, ColId)->text().compare(id, Qt::CaseInsensitive) == 0) {
            QMessageBox::warning(m_fenetre, "ID existant", "Un rendez-vous avec cet ID existe déjà.");
            ui->lineEditRdvId->setFocus();
            return;
        }
    }

    const QStringList valeurs = {
        id,
        ui->comboBoxRdvStatut->currentText().trimmed(),
        ui->comboBoxRdvPriorite->currentText().trimmed(),
        QString::number(ui->spinBoxRdvDuree->value()),
        ui->comboBoxRdvType->currentText().trimmed(),
        ui->comboBoxRdvSalle->currentText().trimmed(),
        ui->timeEditRdv->time().toString("HH:mm"),
        ui->dateEditRdv->date().toString("dd/MM/yyyy")
    };

    if (m_ligneEnModification >= 0) {
        remplirLigne(m_ligneEnModification, valeurs);
        t->selectRow(m_ligneEnModification);
        m_fenetre->statusBar()->showMessage(QString::fromUtf8("Rendez-vous modifié"), 3000);
    } else {
        const int row = t->rowCount();
        t->insertRow(row);
        remplirLigne(row, valeurs);
        ajouterBoutonsAction(row);
        t->selectRow(row);
        t->scrollToBottom();
        m_fenetre->statusBar()->showMessage(QString::fromUtf8("Rendez-vous ajouté"), 3000);
    }

    viderFormulaire();
}

void GestionRdv::viderFormulaire()
{
    m_ligneEnModification = -1;
    ui->pushButtonRdvEnregistrer->setText("enregistrer");

    ui->lineEditRdvId->clear();
    ui->dateEditRdv->setDate(QDate::currentDate());
    ui->timeEditRdv->setTime(QTime(9, 0));
    ui->comboBoxRdvType->setCurrentIndex(0);
    ui->spinBoxRdvDuree->setValue(0);
    ui->comboBoxRdvPriorite->setCurrentIndex(0);
    ui->comboBoxRdvSalle->setCurrentIndex(0);
    ui->comboBoxRdvStatut->setCurrentIndex(0);
    ui->checkBoxRdvEmail->setChecked(false);
}

void GestionRdv::ajouterBoutonsAction(int row)
{
    QTableWidget *t = ui->tableWidgetRdv;

    auto *btnModifier = new QPushButton("Modifier");
    auto *btnSupprimer = new QPushButton("Supprimer");

    for (QPushButton *b : { btnModifier, btnSupprimer }) {
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(34);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    btnModifier->setStyleSheet(
        "QPushButton {"
        "    background-color: #1F6FBF;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-family: Arial;"
        "    font-size: 10pt;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover   { background-color: #1858A0; }"
        "QPushButton:pressed { background-color: #134780; }");

    btnSupprimer->setStyleSheet(
        "QPushButton {"
        "    background-color: #C62828;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-family: Arial;"
        "    font-size: 10pt;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover   { background-color: #A81F1F; }"
        "QPushButton:pressed { background-color: #8B1818; }");

    auto *conteneur = new QWidget;
    auto *layout = new QHBoxLayout(conteneur);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);
    layout->addWidget(btnModifier);
    layout->addWidget(btnSupprimer);

    t->setCellWidget(row, ColAction, conteneur);

    // La ligne peut changer (suppression d'une ligne au-dessus) : on la retrouve à chaque clic
    auto ligneCourante = [this, conteneur]() {
        return ui->tableWidgetRdv->indexAt(conteneur->pos()).row();
    };

    connect(btnModifier, &QPushButton::clicked, this, [this, ligneCourante]() {
        const int r = ligneCourante();
        if (r < 0) return;
        auto texte = [this, r](int col) { return ui->tableWidgetRdv->item(r, col)->text(); };

        ui->lineEditRdvId->setText(texte(ColId));
        selectionnerCombo(ui->comboBoxRdvStatut, texte(ColStatut));
        selectionnerCombo(ui->comboBoxRdvPriorite, texte(ColPriorite));
        ui->spinBoxRdvDuree->setValue(texte(ColDuree).toInt());
        selectionnerCombo(ui->comboBoxRdvType, texte(ColType));
        selectionnerCombo(ui->comboBoxRdvSalle, texte(ColSalle));
        ui->timeEditRdv->setTime(QTime::fromString(texte(ColHeure), "HH:mm"));
        ui->dateEditRdv->setDate(QDate::fromString(texte(ColDate), "dd/MM/yyyy"));

        m_ligneEnModification = r;
        ui->pushButtonRdvEnregistrer->setText(QString::fromUtf8("mettre à jour"));
    });

    connect(btnSupprimer, &QPushButton::clicked, this, [this, ligneCourante]() {
        const int r = ligneCourante();
        if (r < 0) return;

        const auto rep = QMessageBox::question(
            m_fenetre, "Supprimer",
            "Voulez-vous vraiment supprimer le rendez-vous " +
                ui->tableWidgetRdv->item(r, ColId)->text() + " ?");
        if (rep != QMessageBox::Yes)
            return;

        ui->tableWidgetRdv->removeRow(r);

        if (r == m_ligneEnModification)
            viderFormulaire();
        else if (r < m_ligneEnModification)
            --m_ligneEnModification;
    });
}

// Bouton « Statistique » : ouvre la fenêtre des statistiques à partir des lignes du tableau
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

// Bouton « Exporter PDF » : demande où enregistrer le fichier puis génère le PDF
void GestionRdv::exporterPdf()
{
    if (ui->tableWidgetRdv->rowCount() == 0) {
        QMessageBox::information(m_fenetre, "Exporter PDF", QString::fromUtf8("Aucun rendez-vous à exporter."));
        return;
    }

    const QString suggestion = QDir::homePath() + "/rendez-vous_" +
                               QDate::currentDate().toString("yyyy-MM-dd") + ".pdf";

    QString chemin = QFileDialog::getSaveFileName(
        m_fenetre, QString::fromUtf8("Exporter la liste des rendez-vous"),
        suggestion, "Fichiers PDF (*.pdf)");

    if (chemin.isEmpty())
        return;   // l'utilisateur a cliqué sur Annuler

    if (!chemin.endsWith(".pdf", Qt::CaseInsensitive))
        chemin += ".pdf";

    if (!genererPdf(chemin)) {
        QMessageBox::critical(m_fenetre, "Exporter PDF",
                              QString::fromUtf8("Impossible de créer le fichier PDF.\n"
                                                "Vérifiez que le fichier n'est pas ouvert dans un autre programme."));
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
