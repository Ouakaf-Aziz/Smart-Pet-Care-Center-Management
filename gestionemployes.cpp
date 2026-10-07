#include "gestionemployes.h"
#include "ui_gsmartpetcare.h"

#include <QColor>
#include <QComboBox>
#include <QDateEdit>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTextDocument>

#include <algorithm>

namespace {

enum Colonne { ColId = 0, ColNom, ColPrenom, ColPoste, ColStatut, ColSalaire, ColDate, ColMdp, ColActions };

} // namespace

GestionEmployes::GestionEmployes(Ui::GSmartPetCare *ui, BaseEmployes &base, QMainWindow *fenetre,
                                 QObject *parent)
    : QObject(parent)
    , m_base(base)
    , ui(ui)
    , m_fenetre(fenetre)
{
    configurerInterface();
}

void GestionEmployes::configurerInterface()
{
    ui->pageEmployes->setAttribute(Qt::WA_StyledBackground, true);

    const char *criteres[] = {"tout", "id", "nom", "prenom", "email", "poste", "statut"};
    for (int i = 0; i < 7; ++i)
        ui->comboBoxEmpCritere->setItemData(i, criteres[i]);
    const char *tris[] = {"nom", "salaire", "date", "poste", "statut"};
    for (int i = 0; i < 5; ++i)
        ui->comboBoxEmpTri->setItemData(i, tris[i]);

    ui->comboBoxEmpPoste->clear();
    ui->comboBoxEmpPoste->addItems(Emp::postes());
    ui->comboBoxEmpStatut->clear();
    ui->comboBoxEmpStatut->addItems(Emp::statuts());
    ui->dateEditEmpEmbauche->setDate(QDate::currentDate());
    ui->dateEditEmpEmbauche->setMaximumDate(QDate::currentDate());
    ui->dateEditEmpEmbauche->setCalendarPopup(true);
    ui->dateEditEmpEmbauche->setDisplayFormat("dd/MM/yyyy");

    QHeaderView *h = ui->tableWidgetEmployes->horizontalHeader();
    h->setMinimumSectionSize(50);
    h->setStretchLastSection(false);
    h->setFixedHeight(44);
    h->setSectionResizeMode(ColId, QHeaderView::Fixed);        h->resizeSection(ColId, 50);
    h->setSectionResizeMode(ColNom, QHeaderView::Stretch);
    h->setSectionResizeMode(ColPrenom, QHeaderView::Stretch);
    h->setSectionResizeMode(ColPoste, QHeaderView::Fixed);     h->resizeSection(ColPoste, 160);
    h->setSectionResizeMode(ColStatut, QHeaderView::Fixed);    h->resizeSection(ColStatut, 105);
    h->setSectionResizeMode(ColSalaire, QHeaderView::Fixed);   h->resizeSection(ColSalaire, 105);
    h->setSectionResizeMode(ColDate, QHeaderView::Fixed);      h->resizeSection(ColDate, 140);
    h->setSectionResizeMode(ColMdp, QHeaderView::Fixed);       h->resizeSection(ColMdp, 160);
    h->setSectionResizeMode(ColActions, QHeaderView::Fixed);   h->resizeSection(ColActions, 215);

    connect(ui->pushButtonEmpAjouter, &QPushButton::clicked, this, &GestionEmployes::onAjouter);
    connect(ui->lineEditEmpRecherche, &QLineEdit::textChanged, this, &GestionEmployes::refreshTable);
    connect(ui->comboBoxEmpCritere, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GestionEmployes::refreshTable);
    connect(ui->comboBoxEmpTri, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GestionEmployes::refreshTable);
    connect(ui->comboBoxEmpFiltreStatut, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GestionEmployes::refreshTable);
    connect(ui->pushButtonEmpOrdre, &QPushButton::toggled, this, [this](bool decroissant) {
        ui->pushButtonEmpOrdre->setText(decroissant ? "Décroissant" : "Croissant");
        refreshTable();
    });
    connect(ui->tableWidgetEmployes, &QTableWidget::itemSelectionChanged,
            this, &GestionEmployes::onSelectionChanged);
    connect(ui->pushButtonEmpEnregistrer, &QPushButton::clicked, this, &GestionEmployes::onEnregistrer);
    connect(ui->pushButtonEmpAnnuler, &QPushButton::clicked, this, &GestionEmployes::onAnnuler);
    connect(ui->pushButtonEmpExporterPdf, &QPushButton::clicked, this, &GestionEmployes::onExporterPdf);
    connect(ui->pushButtonEmpStatistiques, &QPushButton::clicked, this, &GestionEmployes::onStatistiques);
}

void GestionEmployes::setUtilisateur(int employeId)
{
    m_userId = employeId;
    const Employe *u = m_base.trouver(employeId);
    ui->labelEmpUtilisateur->setText(u ? QString("Connecté : %1 %2 (%3)").arg(u->prenom, u->nom, u->poste)
                                       : QString());

    m_currentId = -1;
    viderFormulaire();
    refreshTable();
    if (ui->tableWidgetEmployes->rowCount() > 0)
        ui->tableWidgetEmployes->selectRow(0);
}

void GestionEmployes::refreshTable()
{
    const QString q = ui->lineEditEmpRecherche->text().trimmed();
    const QString critere = ui->comboBoxEmpCritere->currentData().toString();
    const QString statutFiltre = ui->comboBoxEmpFiltreStatut->currentIndex() > 0
            ? ui->comboBoxEmpFiltreStatut->currentText() : QString();

    auto contient = [&q](const QString &s) { return s.contains(q, Qt::CaseInsensitive); };
    auto correspond = [&](const Employe &e) {
        if (q.isEmpty())
            return true;
        if (critere == "id")     return QString::number(e.id) == q;
        if (critere == "nom")    return contient(e.nom);
        if (critere == "prenom") return contient(e.prenom);
        if (critere == "email")  return contient(e.email);
        if (critere == "poste")  return contient(e.poste);
        if (critere == "statut") return contient(e.statut);
        return QString::number(e.id) == q || contient(e.nom) || contient(e.prenom) || contient(e.email)
               || contient(e.poste) || contient(e.statut) || contient(e.telephone) || contient(e.adresse);
    };

    QVector<Employe> vue;
    for (const Employe &e : std::as_const(m_base.employes)) {
        if (!statutFiltre.isEmpty() && e.statut != statutFiltre)
            continue;
        if (correspond(e))
            vue.append(e);
    }

    const QString cle = ui->comboBoxEmpTri->currentData().toString();
    const bool decroissant = ui->pushButtonEmpOrdre->isChecked();
    std::stable_sort(vue.begin(), vue.end(), [&](const Employe &a, const Employe &b) {
        int c = 0;
        if (cle == "salaire")
            c = a.salaire < b.salaire ? -1 : (a.salaire > b.salaire ? 1 : 0);
        else if (cle == "date")
            c = a.dateEmbauche < b.dateEmbauche ? -1 : (a.dateEmbauche > b.dateEmbauche ? 1 : 0);
        else if (cle == "poste")
            c = QString::localeAwareCompare(a.poste, b.poste);
        else if (cle == "statut")
            c = QString::localeAwareCompare(a.statut, b.statut);
        else
            c = QString::localeAwareCompare(a.nom, b.nom);
        if (c == 0 && cle != "nom")
            c = QString::localeAwareCompare(a.nom, b.nom);
        if (c == 0)
            c = a.id < b.id ? -1 : (a.id > b.id ? 1 : 0);
        return decroissant ? c > 0 : c < 0;
    });
    m_vue = vue;

    QTableWidget *t = ui->tableWidgetEmployes;
    {
        const QSignalBlocker blocker(t);
        t->clearSpans();
        t->clearContents();
        t->setRowCount(vue.size());
        int ligneSelection = -1;

        for (int r = 0; r < vue.size(); ++r) {
            const Employe &e = vue[r];

            auto centre = [](const QString &txt) {
                auto *it = new QTableWidgetItem(txt);
                it->setTextAlignment(Qt::AlignCenter);
                return it;
            };

            auto *itId = centre(QString::number(e.id));
            itId->setData(Qt::UserRole, e.id);

            auto *itStatut = centre(e.statut);
            itStatut->setForeground(QColor(e.statut == "Actif" ? "#0F766E" : "#C0392B"));

            t->setItem(r, ColId, itId);
            t->setItem(r, ColNom, new QTableWidgetItem(e.nom));
            t->setItem(r, ColPrenom, new QTableWidgetItem(e.prenom));
            t->setItem(r, ColPoste, new QTableWidgetItem(e.poste));
            t->setItem(r, ColStatut, itStatut);
            t->setItem(r, ColSalaire, centre(QString::number(e.salaire, 'f', 2)));
            t->setItem(r, ColDate, centre(e.dateEmbauche.toString("dd/MM/yyyy")));
            t->setItem(r, ColMdp, centre(e.motDePasse.isEmpty() ? QString("Non défini")
                                                                : QString(8, QChar(0x2022))));
            t->setItem(r, ColActions, new QTableWidgetItem());
            t->setCellWidget(r, ColActions, creerBoutonsActions(e.id));

            if (e.id == m_currentId)
                ligneSelection = r;
        }

        if (ligneSelection >= 0)
            t->selectRow(ligneSelection);
        else
            t->clearSelection();
    }
}

QWidget *GestionEmployes::creerBoutonsActions(int employeId)
{
    auto *conteneur = new QWidget;
    auto *lay = new QHBoxLayout(conteneur);
    lay->setContentsMargins(6, 4, 6, 4);
    lay->setSpacing(8);

    auto *btnModifier = new QPushButton("Modifier", conteneur);
    btnModifier->setObjectName("btnModifierLigne");
    btnModifier->setFixedHeight(32);
    btnModifier->setCursor(Qt::PointingHandCursor);
    lay->addWidget(btnModifier);
    connect(btnModifier, &QPushButton::clicked, this, [this, employeId]() { onModifier(employeId); });

    auto *btnSupprimer = new QPushButton("Supprimer", conteneur);
    btnSupprimer->setObjectName("btnSupprimerLigne");
    btnSupprimer->setFixedHeight(32);
    btnSupprimer->setCursor(Qt::PointingHandCursor);
    lay->addWidget(btnSupprimer);
    connect(btnSupprimer, &QPushButton::clicked, this, [this, employeId]() { onSupprimer(employeId); });
    return conteneur;
}

void GestionEmployes::viderFormulaire()
{
    ui->labelEmpDetailTitre->setText("Nouvel employé");
    ui->lineEditEmpId->clear();
    ui->lineEditEmpNom->clear();
    ui->lineEditEmpPrenom->clear();
    ui->lineEditEmpEmail->clear();
    ui->lineEditEmpAdresse->clear();
    ui->lineEditEmpTelephone->clear();
    ui->lineEditEmpSalaire->clear();
    ui->dateEditEmpEmbauche->setDate(QDate::currentDate());
    ui->comboBoxEmpStatut->setCurrentIndex(0);
    ui->comboBoxEmpPoste->setCurrentIndex(0);
    ui->lineEditEmpMdp->clear();
    ui->lineEditEmpMdp->setPlaceholderText("Obligatoire (6 caractères min.)");
}

void GestionEmployes::remplirFormulaire(const Employe &e)
{
    ui->labelEmpDetailTitre->setText("Détails de l'employé");
    ui->lineEditEmpId->setText(QString::number(e.id));
    ui->lineEditEmpNom->setText(e.nom);
    ui->lineEditEmpPrenom->setText(e.prenom);
    ui->lineEditEmpEmail->setText(e.email);
    ui->lineEditEmpAdresse->setText(e.adresse);
    ui->lineEditEmpTelephone->setText(e.telephone);
    ui->lineEditEmpSalaire->setText(QString::number(e.salaire, 'f', 2));
    ui->dateEditEmpEmbauche->setDate(e.dateEmbauche);
    ui->comboBoxEmpStatut->setCurrentText(e.statut);
    ui->comboBoxEmpPoste->setCurrentText(e.poste);
    ui->lineEditEmpMdp->clear();
    ui->lineEditEmpMdp->setPlaceholderText("Laisser vide pour le conserver");
}

void GestionEmployes::onSelectionChanged()
{
    const QList<QTableWidgetItem *> sel = ui->tableWidgetEmployes->selectedItems();
    if (sel.isEmpty())
        return;

    const QTableWidgetItem *premier = ui->tableWidgetEmployes->item(sel.first()->row(), 0);
    if (!premier)
        return;
    const int id = premier->data(Qt::UserRole).toInt();
    if (const Employe *e = m_base.trouver(id)) {
        m_currentId = id;
        remplirFormulaire(*e);
    }
}

void GestionEmployes::onAjouter()
{
    m_currentId = -1;
    ui->tableWidgetEmployes->clearSelection();
    viderFormulaire();
    ui->lineEditEmpNom->setFocus();
}

void GestionEmployes::onModifier(int employeId)
{
    const Employe *e = m_base.trouver(employeId);
    if (!e)
        return;
    m_currentId = employeId;
    remplirFormulaire(*e);

    for (int r = 0; r < ui->tableWidgetEmployes->rowCount(); ++r) {
        if (ui->tableWidgetEmployes->item(r, 0)
            && ui->tableWidgetEmployes->item(r, 0)->data(Qt::UserRole).toInt() == employeId) {
            ui->tableWidgetEmployes->selectRow(r);
            break;
        }
    }
    ui->lineEditEmpNom->setFocus();
    ui->lineEditEmpNom->selectAll();
}

void GestionEmployes::onSupprimer(int employeId)
{
    const Employe *e = m_base.trouver(employeId);
    if (!e)
        return;

    if (employeId == m_userId) {
        QMessageBox::warning(m_fenetre, "Suppression impossible",
                             "Vous ne pouvez pas supprimer votre propre compte.");
        return;
    }
    if (QMessageBox::question(m_fenetre, "Supprimer",
                              QString("Supprimer l'employé « %1 %2 » ?").arg(e->prenom, e->nom))
        != QMessageBox::Yes)
        return;

    m_base.employes.erase(std::remove_if(m_base.employes.begin(), m_base.employes.end(),
                                         [employeId](const Employe &x) { return x.id == employeId; }),
                          m_base.employes.end());
    if (m_currentId == employeId) {
        m_currentId = -1;
        viderFormulaire();
    }
    refreshTable();
}

void GestionEmployes::onEnregistrer()
{
    const bool nouveau = (m_currentId < 0);

    auto erreur = [this](const QString &titre, const QString &msg, QWidget *champ) {
        QMessageBox::warning(m_fenetre, titre, msg);
        champ->setFocus();
    };

    const QString nom = ui->lineEditEmpNom->text().trimmed();
    const QString prenom = ui->lineEditEmpPrenom->text().trimmed();
    const QString email = ui->lineEditEmpEmail->text().trimmed();
    const QString tel = ui->lineEditEmpTelephone->text().trimmed();
    const QString mdp = ui->lineEditEmpMdp->text();

    if (nom.isEmpty())    return erreur("Champ manquant", "Le nom est obligatoire.", ui->lineEditEmpNom);
    if (prenom.isEmpty()) return erreur("Champ manquant", "Le prénom est obligatoire.", ui->lineEditEmpPrenom);

    static const QRegularExpression reEmail("^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$");
    if (!reEmail.match(email).hasMatch())
        return erreur("Email invalide", "Saisissez un email valide (ex. prenom.nom@petnova.tn).",
                      ui->lineEditEmpEmail);

    const Employe *memeEmail = m_base.trouverParEmail(email);
    if (memeEmail && memeEmail->id != m_currentId)
        return erreur("Email déjà utilisé", "Un autre employé utilise déjà cet email.",
                      ui->lineEditEmpEmail);

    static const QRegularExpression reTel("^\\+?[0-9 ]{8,15}$");
    if (!reTel.match(tel).hasMatch())
        return erreur("Téléphone invalide", "Saisissez un numéro valide (8 à 15 chiffres).",
                      ui->lineEditEmpTelephone);

    bool ok = false;
    QString txt = ui->lineEditEmpSalaire->text().trimmed();
    txt.replace(',', '.');
    const double salaire = txt.toDouble(&ok);
    if (!ok || salaire < 0)
        return erreur("Salaire invalide", "Saisissez un salaire valide (ex. 1800 ou 1800.50).",
                      ui->lineEditEmpSalaire);

    if (nouveau && mdp.length() < 6)
        return erreur("Mot de passe invalide", "Le mot de passe doit contenir au moins 6 caractères.",
                      ui->lineEditEmpMdp);
    if (!nouveau && !mdp.isEmpty() && mdp.length() < 6)
        return erreur("Mot de passe invalide", "Le mot de passe doit contenir au moins 6 caractères.",
                      ui->lineEditEmpMdp);

    Employe *existant = nouveau ? nullptr : m_base.trouver(m_currentId);

    if (existant && existant->id == m_userId
        && (ui->comboBoxEmpPoste->currentText() != existant->poste
            || ui->comboBoxEmpStatut->currentText() != existant->statut)) {
        QMessageBox::warning(m_fenetre, "Action refusée",
                             "Vous ne pouvez pas modifier votre propre poste ou votre propre statut.");
        return;
    }

    Employe e = existant ? *existant : Employe();
    e.nom = nom;
    e.prenom = prenom;
    e.email = email;
    e.adresse = ui->lineEditEmpAdresse->text().trimmed();
    e.telephone = tel;
    e.salaire = salaire;
    e.dateEmbauche = ui->dateEditEmpEmbauche->date();
    e.statut = ui->comboBoxEmpStatut->currentText();
    e.poste = ui->comboBoxEmpPoste->currentText();
    if (!mdp.isEmpty())
        e.motDePasse = Emp::hacher(mdp);

    if (nouveau) {
        e.id = m_base.prochainId++;
        m_base.employes.append(e);
        m_currentId = e.id;
    } else {
        *existant = e;
    }

    ui->lineEditEmpId->setText(QString::number(m_currentId));
    ui->labelEmpDetailTitre->setText("Détails de l'employé");
    ui->lineEditEmpMdp->clear();
    ui->lineEditEmpMdp->setPlaceholderText("Laisser vide pour le conserver");
    refreshTable();
}

void GestionEmployes::onAnnuler()
{
    if (const Employe *e = m_base.trouver(m_currentId))
        remplirFormulaire(*e);
    else
        viderFormulaire();
}

GestionEmployes::DonneesExport GestionEmployes::donneesExport() const
{
    DonneesExport d;
    d.entetes = {"ID", "Nom", "Prénom", "Email", "Adresse", "Téléphone", "Poste", "Statut", "Date d'embauche"};
    d.entetes << "Salaire (DT)";

    for (const Employe &e : m_vue) {
        QStringList l = {QString::number(e.id), e.nom, e.prenom, e.email, e.adresse, e.telephone,
                         e.poste, e.statut, e.dateEmbauche.toString("dd/MM/yyyy"),
                         QString::number(e.salaire, 'f', 2)};
        d.lignes.append(l);
    }
    return d;
}

QString GestionEmployes::htmlTableau() const
{
    const DonneesExport d = donneesExport();
    const QString texte = QStringLiteral("#1F2A37");
    const QString turquoise = QStringLiteral("#0F766E");

    QString html = QStringLiteral("<div style='font-family:Arial; color:%1;'>").arg(texte);
    html += QStringLiteral("<h1 style='font-family:Arial; font-size:20pt; font-weight:bold; color:%1;'>"
                           "Liste des employés - Smart Pet Care</h1>").arg(turquoise);
    html += QStringLiteral("<p style='font-size:11pt;'>Date d'exportation : <b>%1</b><br>"
                           "Nombre total d'employés : <b>%2</b></p>")
                .arg(QDate::currentDate().toString("dd/MM/yyyy"))
                .arg(d.lignes.size());

    html += QStringLiteral("<table border='0' cellspacing='0' cellpadding='5' width='100%' "
                           "style='font-family:Arial; font-size:9pt; border-collapse:collapse;'>"
                           "<tr style='background-color:%1; color:#FFFFFF;'>").arg(turquoise);
    for (const QString &en : d.entetes)
        html += "<th align='left'>" + en.toHtmlEscaped() + "</th>";
    html += "</tr>";
    int n = 0;
    for (const QStringList &l : d.lignes) {
        html += QStringLiteral("<tr style='background-color:%1;'>").arg(n++ % 2 ? "#F4F8F7" : "#FFFFFF");
        for (const QString &c : l)
            html += QStringLiteral("<td style='border-bottom:1px solid #CBD5D3;'>") + c.toHtmlEscaped() + "</td>";
        html += "</tr>";
    }
    html += "</table></div>";
    return html;
}

void GestionEmployes::onExporterPdf()
{
    QString chemin = QFileDialog::getSaveFileName(m_fenetre, "Exporter en PDF", "employes.pdf",
                                                  "Fichiers PDF (*.pdf)");
    if (chemin.isEmpty())
        return;
    if (!chemin.endsWith(".pdf", Qt::CaseInsensitive))
        chemin += ".pdf";

    QTextDocument doc;
    doc.setDefaultFont(QFont("Arial", 9));
    doc.setHtml(htmlTableau());

    QPdfWriter writer(chemin);
    writer.setPageLayout(QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Landscape,
                                     QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter));
    doc.print(&writer);

    QMessageBox::information(m_fenetre, "Export PDF", "Le fichier PDF a été créé :\n" + chemin);
}

void GestionEmployes::onStatistiques()
{
    emit statistiquesDemandees();
}
