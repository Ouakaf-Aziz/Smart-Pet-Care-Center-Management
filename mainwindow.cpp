#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "statistiquesdialog.h"

#include <QApplication>
#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QPalette>
#include <QPushButton>
#include <QScreen>
#include <QStatusBar>
#include <QStyleFactory>
#include <QTableWidgetItem>
#include <QTextDocument>
#include <QTime>
#include <QUrl>
#include <QVBoxLayout>

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

// Force un theme clair (fond blanc), meme si Windows est en mode sombre
void appliquerThemeClair()
{
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QPalette p;
    p.setColor(QPalette::Window,          QColor("#FFFFFF"));
    p.setColor(QPalette::WindowText,      QColor("#1F2A37"));
    p.setColor(QPalette::Base,            QColor("#FFFFFF"));
    p.setColor(QPalette::AlternateBase,   QColor("#F3F4F6"));
    p.setColor(QPalette::Text,            QColor("#1F2A37"));
    p.setColor(QPalette::PlaceholderText, QColor("#9CA3AF"));
    p.setColor(QPalette::Button,          QColor("#F3F4F6"));
    p.setColor(QPalette::ButtonText,      QColor("#1F2A37"));
    p.setColor(QPalette::BrightText,      QColor("#FFFFFF"));
    p.setColor(QPalette::ToolTipBase,     QColor("#FFFFFF"));
    p.setColor(QPalette::ToolTipText,     QColor("#1F2A37"));
    p.setColor(QPalette::Highlight,       QColor("#0F766E"));
    p.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    p.setColor(QPalette::Link,            QColor("#1D4ED8"));

    p.setColor(QPalette::Disabled, QPalette::Text,       QColor("#9CA3AF"));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#9CA3AF"));
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#9CA3AF"));

    qApp->setPalette(p);
}

enum Colonne { ColId, ColStatut, ColPriorite, ColDuree, ColType, ColSalle,
               ColHeure, ColDate, ColAction, NbColonnes };
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    appliquerThemeClair();            // fond blanc impose
    setPalette(qApp->palette());      // et applique a cette fenetre et a tous ses widgets
    ui->setupUi(this);
    configurerMiseEnPage();           // interface adaptable (plein ecran)

    ui->dateEdit->setDisplayFormat("dd/MM/yyyy");
    ui->dateEdit->setCalendarPopup(true);
    ui->dateEdit->setDate(QDate::currentDate());
    ui->timeEdit->setDisplayFormat("HH:mm");
    ui->spinBox->setRange(0, 600);

    connect(ui->pushButton_22, &QPushButton::clicked, this, &MainWindow::enregistrerRdv);
    connect(ui->pushButton_21, &QPushButton::clicked, this, &MainWindow::viderFormulaire);
    connect(ui->pushButton_17, &QPushButton::clicked, this, &MainWindow::afficherStatistiques);
    connect(ui->pushButton_24, &QPushButton::clicked, this, &MainWindow::exporterPdf);

    configurerTableau();
    remplirExemples();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Met les widgets dans des layouts : l'interface s'adapte a la taille de la fenetre
// (le tableau s'agrandit en plein ecran, le formulaire garde sa taille)
void MainWindow::configurerMiseEnPage()
{
    // La page n'a plus de taille fixe
    ui->contentPage->setMinimumSize(0, 0);

    auto *principal = new QVBoxLayout(ui->contentPage);
    principal->setContentsMargins(20, 6, 20, 12);
    principal->setSpacing(8);

    // Ligne 1 : titre
    principal->addWidget(ui->label_5);

    // Ligne 2 : recherche + tri + filtre
    auto *ligneRecherche = new QHBoxLayout;
    ligneRecherche->setSpacing(10);
    ui->lineEdit_2->setMinimumHeight(28);
    ui->comboBox_5->setMinimumSize(181, 28);
    ui->comboBox_4->setMinimumSize(181, 28);
    ligneRecherche->addWidget(ui->lineEdit_2, 1);
    ligneRecherche->addWidget(ui->comboBox_5);
    ligneRecherche->addWidget(ui->comboBox_4);
    principal->addLayout(ligneRecherche);

    // Ligne 3 : boutons
    ui->pushButton_24->setMinimumWidth(151);   // Exporter PDF
    ui->pushButton_17->setMinimumWidth(151);   // Statistique
    ui->pushButton_18->setMinimumWidth(181);   // Confirmer RDV
    ui->pushButton_25->setMinimumWidth(201);   // Calendrier intelligent
    auto *ligneBoutons = new QHBoxLayout;
    ligneBoutons->setSpacing(14);
    ligneBoutons->addWidget(ui->pushButton_24);
    ligneBoutons->addWidget(ui->pushButton_17);
    ligneBoutons->addWidget(ui->pushButton_18);
    ligneBoutons->addWidget(ui->pushButton_25);
    ligneBoutons->addStretch(1);
    principal->addLayout(ligneBoutons);

    // Ligne 4 : tableau (s'etire) + formulaire (taille fixe, en haut)
    auto *corps = new QHBoxLayout;
    corps->setSpacing(14);
    ui->tableWidget->setMinimumWidth(720);
    ui->groupBox->setFixedSize(411, 451);
    corps->addWidget(ui->tableWidget, 1);
    corps->addWidget(ui->groupBox, 0, Qt::AlignTop);
    principal->addLayout(corps, 1);

    // Taille de depart (sans depasser l'ecran)
    const QRect ecran = QGuiApplication::primaryScreen()->availableGeometry();
    resize(qMin(1485, ecran.width()), qMin(680, ecran.height()));
}

void MainWindow::configurerTableau()
{
    QTableWidget *t = ui->tableWidget;

    t->setColumnCount(NbColonnes);
    t->setHorizontalHeaderLabels({
        "Id",
        QString::fromUtf8("Statut"),
        QString::fromUtf8("Priorité"),
        QString::fromUtf8("Durée"),
        "Type",
        "Salle",
        "Heure",
        "Date",
        "Actions"
    });

    t->verticalHeader()->setVisible(false);
    t->verticalHeader()->setDefaultSectionSize(52);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setAlternatingRowColors(true);
    t->setShowGrid(true);

    QHeaderView *h = t->horizontalHeader();
    h->setSectionResizeMode(QHeaderView::Stretch);
    h->setDefaultAlignment(Qt::AlignCenter);

    h->setSectionResizeMode(ColId, QHeaderView::Fixed);      t->setColumnWidth(ColId, 70);
    h->setSectionResizeMode(ColDuree, QHeaderView::Fixed);   t->setColumnWidth(ColDuree, 55);
    h->setSectionResizeMode(ColSalle, QHeaderView::Fixed);   t->setColumnWidth(ColSalle, 55);
    h->setSectionResizeMode(ColHeure, QHeaderView::Fixed);   t->setColumnWidth(ColHeure, 60);
    h->setSectionResizeMode(ColAction, QHeaderView::Fixed);  t->setColumnWidth(ColAction, 250);

    t->setStyleSheet(
        "QTableWidget { alternate-background-color: #EEF3F1; background: white;"
        "               gridline-color: #DDE5E2; border: 1px solid #D9E2DF; }"
        "QTableWidget::item { padding: 4px; }"
        "QTableWidget::item:selected { background: #CFE8E0; color: black; }"
        "QHeaderView::section { background-color: #1B5E4B; color: white;"
        "                       font-weight: bold; padding: 8px; border: none; }");
}

void MainWindow::remplirLigne(int row, const QStringList &valeurs)
{
    QTableWidget *t = ui->tableWidget;

    for (int col = 0; col < valeurs.size() && col < ColAction; ++col) {
        auto *item = new QTableWidgetItem(valeurs[col]);
        item->setTextAlignment(Qt::AlignCenter);
        item->setToolTip(valeurs[col]);
        t->setItem(row, col, item);
    }

    const bool annule = valeurs[ColStatut].startsWith(QString::fromUtf8("annul"));
    t->item(row, ColStatut)->setForeground(annule ? QColor("#B91C1C") : QColor("#0F766E"));
}

void MainWindow::remplirExemples()
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

    ui->tableWidget->setRowCount(0);

    for (const QStringList &valeurs : exemples) {
        const int row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);
        remplirLigne(row, valeurs);
        ajouterBoutonsAction(row);
    }
}

// Bouton "enregistrer" : ajoute une ligne (ou met a jour la ligne en modification)
void MainWindow::enregistrerRdv()
{
    QTableWidget *t = ui->tableWidget;

    const QString id = ui->lineEdit->text().trimmed();
    if (id.isEmpty()) {
        QMessageBox::warning(this, "Champ obligatoire", "Veuillez saisir l'ID du rendez-vous.");
        ui->lineEdit->setFocus();
        return;
    }
    if (ui->spinBox->value() <= 0) {
        QMessageBox::warning(this, "Champ obligatoire", "Veuillez saisir une duree superieure a 0.");
        ui->spinBox->setFocus();
        return;
    }

    for (int r = 0; r < t->rowCount(); ++r) {
        if (r != m_ligneEnModification &&
            t->item(r, ColId)->text().compare(id, Qt::CaseInsensitive) == 0) {
            QMessageBox::warning(this, "ID existant", "Un rendez-vous avec cet ID existe deja.");
            ui->lineEdit->setFocus();
            return;
        }
    }

    const QStringList valeurs = {
        id,
        ui->comboBox->currentText().trimmed(),
        ui->comboBox_2->currentText().trimmed(),
        QString::number(ui->spinBox->value()),
        ui->comboBox_6->currentText().trimmed(),
        ui->comboBox_3->currentText().trimmed(),
        ui->timeEdit->time().toString("HH:mm"),
        ui->dateEdit->date().toString("dd/MM/yyyy")
    };

    if (m_ligneEnModification >= 0) {
        remplirLigne(m_ligneEnModification, valeurs);
        t->selectRow(m_ligneEnModification);
        statusBar()->showMessage("Rendez-vous modifie", 3000);
    } else {
        const int row = t->rowCount();
        t->insertRow(row);
        remplirLigne(row, valeurs);
        ajouterBoutonsAction(row);
        t->selectRow(row);
        t->scrollToBottom();
        statusBar()->showMessage("Rendez-vous ajoute", 3000);
    }

    viderFormulaire();
}

void MainWindow::viderFormulaire()
{
    m_ligneEnModification = -1;
    ui->pushButton_22->setText("enregistrer");

    ui->lineEdit->clear();
    ui->dateEdit->setDate(QDate::currentDate());
    ui->timeEdit->setTime(QTime(9, 0));
    ui->comboBox_6->setCurrentIndex(0);
    ui->spinBox->setValue(0);
    ui->comboBox_2->setCurrentIndex(0);
    ui->comboBox_3->setCurrentIndex(0);
    ui->comboBox->setCurrentIndex(0);
    ui->checkBox->setChecked(false);
}

void MainWindow::ajouterBoutonsAction(int row)
{
    QTableWidget *t = ui->tableWidget;

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
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(8);
    layout->addWidget(btnModifier);
    layout->addWidget(btnSupprimer);

    t->setCellWidget(row, ColAction, conteneur);

    auto ligneCourante = [this, conteneur]() {
        return ui->tableWidget->indexAt(conteneur->pos()).row();
    };

    connect(btnModifier, &QPushButton::clicked, this, [this, ligneCourante]() {
        const int r = ligneCourante();
        if (r < 0) return;
        auto texte = [this, r](int col) { return ui->tableWidget->item(r, col)->text(); };

        ui->lineEdit->setText(texte(ColId));
        selectionnerCombo(ui->comboBox, texte(ColStatut));
        selectionnerCombo(ui->comboBox_2, texte(ColPriorite));
        ui->spinBox->setValue(texte(ColDuree).toInt());
        selectionnerCombo(ui->comboBox_6, texte(ColType));
        selectionnerCombo(ui->comboBox_3, texte(ColSalle));
        ui->timeEdit->setTime(QTime::fromString(texte(ColHeure), "HH:mm"));
        ui->dateEdit->setDate(QDate::fromString(texte(ColDate), "dd/MM/yyyy"));

        m_ligneEnModification = r;
        ui->pushButton_22->setText(QString::fromUtf8("mettre à jour"));
    });

    connect(btnSupprimer, &QPushButton::clicked, this, [this, ligneCourante]() {
        const int r = ligneCourante();
        if (r < 0) return;

        const auto rep = QMessageBox::question(
            this, "Supprimer",
            "Voulez-vous vraiment supprimer le rendez-vous " +
                ui->tableWidget->item(r, ColId)->text() + " ?");
        if (rep != QMessageBox::Yes)
            return;

        ui->tableWidget->removeRow(r);

        if (r == m_ligneEnModification)
            viderFormulaire();
        else if (r < m_ligneEnModification)
            --m_ligneEnModification;
    });
}

// Bouton "Statistique" : ouvre la fenetre des statistiques a partir des lignes du tableau
void MainWindow::afficherStatistiques()
{
    QTableWidget *t = ui->tableWidget;
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

    StatistiquesDialog dlg(donnees, this);
    dlg.exec();
}

// Bouton "Exporter PDF" : demande ou enregistrer le fichier puis genere le PDF
void MainWindow::exporterPdf()
{
    if (ui->tableWidget->rowCount() == 0) {
        QMessageBox::information(this, "Exporter PDF", "Aucun rendez-vous a exporter.");
        return;
    }

    const QString suggestion = QDir::homePath() + "/rendez-vous_" +
                               QDate::currentDate().toString("yyyy-MM-dd") + ".pdf";

    QString chemin = QFileDialog::getSaveFileName(
        this, QString::fromUtf8("Exporter la liste des rendez-vous"),
        suggestion, "Fichiers PDF (*.pdf)");

    if (chemin.isEmpty())
        return;   // l'utilisateur a clique sur Annuler

    if (!chemin.endsWith(".pdf", Qt::CaseInsensitive))
        chemin += ".pdf";

    if (!genererPdf(chemin)) {
        QMessageBox::critical(this, "Exporter PDF",
                              "Impossible de creer le fichier PDF.\n"
                              "Verifiez que le fichier n'est pas ouvert dans un autre programme.");
        return;
    }

    const auto rep = QMessageBox::question(
        this, "Exporter PDF",
        QString::fromUtf8("Le PDF a été créé avec succès.\n\nVoulez-vous l'ouvrir ?"));
    if (rep == QMessageBox::Yes)
        QDesktopServices::openUrl(QUrl::fromLocalFile(chemin));
}

// Construit le document (titre + tableau) et l'ecrit dans le fichier PDF
bool MainWindow::genererPdf(const QString &chemin)
{
    QTableWidget *t = ui->tableWidget;

    // Colonnes exportees (la colonne Actions est ignoree)
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

    // En-tete vert
    html += "<tr>";
    for (int i = 0; i < entetes.size(); ++i) {
        html += QString("<td width='%1%' align='center' "
                        "style='background-color:#1B5E4B; color:white; font-weight:bold;'>%2</td>")
                    .arg(largeurs[i]).arg(entetes[i].toHtmlEscaped());
    }
    html += "</tr>";

    // Lignes de donnees
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
    }   // le fichier est finalise ici

    const QFileInfo info(chemin);
    return info.exists() && info.size() > 0;
}