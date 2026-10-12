#include "gestionemployes.h"
#include "ui_gsmartpetcare.h"
#include "faceauth.h"

#include <QComboBox>
#include <QDateEdit>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTextDocument>
#include <QVBoxLayout>

#include <algorithm>

namespace {

enum Colonne { ColId = 0, ColNom, ColPrenom, ColPoste, ColStatut, ColSalaire, ColDate, ColMdp, ColActions };

} // namespace

GestionEmployes::GestionEmployes(Ui::GSmartPetCare *ui, BaseEmployes &base, QWidget *fenetre)
    : QObject(fenetre)
    , m_base(base)
    , ui(ui)
    , m_fenetre(fenetre)
{
    configurerInterface();
}

// ---------------------------------------------------------------------------
// Réglages de la page pageEmployes (définie dans gsmartpetcare.ui)
// ---------------------------------------------------------------------------
void GestionEmployes::configurerInterface()
{
    // Valeurs associées aux entrées des listes (le texte affiché vient du .ui).
    const char *criteres[] = {"tout", "id", "nom", "prenom"};      // recherche : ID, nom, prénom
    for (int i = 0; i < 4; ++i)
        ui->comboCritere->setItemData(i, criteres[i]);
    const char *tris[] = {"nom", "salaire", "date", "poste", "statut"};
    for (int i = 0; i < 5; ++i)
        ui->comboTri->setItemData(i, tris[i]);

    // Listes alimentées par les données de l'application.
    ui->comboFiltreStatut->addItems(Emp::statuts());
    ui->comboStatut->addItems(Emp::statuts());
    ui->comboPoste->addItems(Emp::postes());
    ui->dateEmbauche->setDate(QDate::currentDate());
    ui->dateEmbauche->setMaximumDate(QDate::currentDate());

    // Largeur des colonnes du tableau (compactes : la page est affichée à côté du menu latéral).
    QHeaderView *h = ui->tableEmployes->horizontalHeader();
    h->setMinimumSectionSize(38);
    h->setStretchLastSection(false);
    h->setFixedHeight(44);
    if (QTableWidgetItem *entete = ui->tableEmployes->horizontalHeaderItem(ColDate)) {
        entete->setToolTip(entete->text());         // « Date d'embauche » en infobulle
        entete->setText("Embauche");
    }
    h->setSectionResizeMode(ColId, QHeaderView::Fixed);        h->resizeSection(ColId, 38);
    h->setSectionResizeMode(ColNom, QHeaderView::Stretch);
    h->setSectionResizeMode(ColPrenom, QHeaderView::Stretch);
    h->setSectionResizeMode(ColPoste, QHeaderView::Fixed);     h->resizeSection(ColPoste, 140);
    h->setSectionResizeMode(ColStatut, QHeaderView::Fixed);    h->resizeSection(ColStatut, 66);
    h->setSectionResizeMode(ColSalaire, QHeaderView::Fixed);   h->resizeSection(ColSalaire, 84);
    h->setSectionResizeMode(ColDate, QHeaderView::Fixed);      h->resizeSection(ColDate, 92);
    h->setSectionResizeMode(ColMdp, QHeaderView::Fixed);       h->resizeSection(ColMdp, 92);
    h->setSectionResizeMode(ColActions, QHeaderView::Fixed);   h->resizeSection(ColActions, 175);

    // --- Connexions ---
    connect(ui->btnAjouter, &QPushButton::clicked, this, &GestionEmployes::onAjouter);
    connect(ui->editRecherche, &QLineEdit::textChanged, this, &GestionEmployes::refreshTable);
    connect(ui->comboCritere, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GestionEmployes::refreshTable);
    connect(ui->comboTri, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GestionEmployes::refreshTable);
    connect(ui->comboFiltreStatut, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GestionEmployes::refreshTable);
    connect(ui->comboOrdre, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GestionEmployes::refreshTable);
    connect(ui->tableEmployes, &QTableWidget::itemSelectionChanged, this, &GestionEmployes::onSelectionChanged);
    connect(ui->btnEnregistrer, &QPushButton::clicked, this, &GestionEmployes::onEnregistrer);
    connect(ui->btnAnnuler, &QPushButton::clicked, this, &GestionEmployes::onAnnuler);
    connect(ui->btnPdf, &QPushButton::clicked, this, &GestionEmployes::onExporterPdf);
    connect(ui->btnStats, &QPushButton::clicked, this, &GestionEmployes::onStatistiques);
    connect(ui->btnPrivileges, &QPushButton::clicked, this, &GestionEmployes::onPrivileges);
    connect(ui->btnVisage, &QPushButton::clicked, this, &GestionEmployes::onEnregistrerVisage);
    connect(ui->btnSupprVisage, &QPushButton::clicked, this, &GestionEmployes::onSupprimerVisage);
}

// ---------------------------------------------------------------------------
// Privilèges d'accès
// ---------------------------------------------------------------------------
void GestionEmployes::montrerChamp(QWidget *champ, bool visible)
{
    champ->setVisible(visible);
    if (QWidget *etiquette = ui->formLayout->labelForField(champ))
        etiquette->setVisible(visible);
}

void GestionEmployes::setUtilisateur(int employeId)
{
    m_userId = employeId;
    const Employe *u = m_base.trouver(employeId);
    m_droits = u ? m_base.droits(u->poste) : Droits();
    ui->labelUtilisateur->setText(u ? QString("Connecté : %1 %2 (%3)").arg(u->prenom, u->nom, u->poste)
                                  : QString());

    appliquerDroits();
    m_currentId = -1;
    viderFormulaire();
    refreshTable();
    if (ui->tableEmployes->rowCount() > 0)
        ui->tableEmployes->selectRow(0);
}

void GestionEmployes::appliquerDroits()
{
    const bool editable = m_droits.ajouter || m_droits.modifier;

    ui->btnAjouter->setVisible(m_droits.ajouter);
    ui->btnPdf->setVisible(m_droits.exporter);
    ui->btnStats->setVisible(m_droits.statistiques);
    ui->btnPrivileges->setVisible(m_droits.gererPrivileges);

    ui->tableEmployes->setColumnHidden(ColSalaire, !m_droits.voirSalaires);
    // Le mot de passe est stocké haché : on ne peut donc afficher que son état (••••••••), jamais sa valeur.
    ui->tableEmployes->setColumnHidden(ColMdp, !m_droits.modifier);
    ui->tableEmployes->setColumnHidden(ColActions, !(m_droits.modifier || m_droits.supprimer));
    montrerChamp(ui->editSalaire, m_droits.voirSalaires);
    montrerChamp(ui->editMdp, editable);

    // Sans le privilège « voir les salaires », le tri par salaire n'existe pas.
    {
        const QSignalBlocker blocker(ui->comboTri);
        const int iSalaire = ui->comboTri->findData("salaire");
        if (m_droits.voirSalaires && iSalaire < 0)
            ui->comboTri->insertItem(1, "Trier par : salaire", "salaire");
        else if (!m_droits.voirSalaires && iSalaire >= 0)
            ui->comboTri->removeItem(iSalaire);
        ui->comboTri->setCurrentIndex(0);                   // retour au tri par nom
    }
    {
        const QSignalBlocker blocker(ui->comboOrdre);
        ui->comboOrdre->setCurrentIndex(0);                 // croissant
    }

    // Formulaire en lecture seule pour ceux qui ne peuvent rien modifier.
    for (QLineEdit *l : {ui->editNom, ui->editPrenom, ui->editEmail, ui->editAdresse, ui->editTelephone, ui->editSalaire})
        l->setReadOnly(!editable);
    for (QWidget *w : {static_cast<QWidget *>(ui->dateEmbauche), static_cast<QWidget *>(ui->comboStatut),
                       static_cast<QWidget *>(ui->comboPoste)})
        w->setEnabled(editable);
    ui->btnEnregistrer->setVisible(editable);
    ui->btnAnnuler->setVisible(editable);
}

// ---------------------------------------------------------------------------
// Tableau : recherche + filtre + tri
// ---------------------------------------------------------------------------
void GestionEmployes::refreshTable()
{
    const QString q = ui->editRecherche->text().trimmed();
    const QString critere = ui->comboCritere->currentData().toString();
    const QString statutFiltre = ui->comboFiltreStatut->currentIndex() > 0 ? ui->comboFiltreStatut->currentText() : QString();

    auto contient = [&q](const QString &s) { return s.contains(q, Qt::CaseInsensitive); };
    auto correspond = [&](const Employe &e) {
        if (q.isEmpty())
            return true;
        if (critere == "id")     return QString::number(e.id) == q;       // ID : valeur exacte
        if (critere == "nom")    return contient(e.nom);
        if (critere == "prenom") return contient(e.prenom);
        return QString::number(e.id) == q || contient(e.nom) || contient(e.prenom);       // « tout » = ID, nom, prénom
    };

    QVector<Employe> vue;
    for (const Employe &e : std::as_const(m_base.employes)) {
        if (!statutFiltre.isEmpty() && e.statut != statutFiltre)
            continue;
        if (correspond(e))
            vue.append(e);
    }

    const QString cle = ui->comboTri->currentData().toString();
    const bool decroissant = ui->comboOrdre->currentIndex() == 1;
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
            c = QString::localeAwareCompare(a.nom, b.nom);          // départage par nom
        if (c == 0)
            c = a.id < b.id ? -1 : (a.id > b.id ? 1 : 0);
        return decroissant ? c > 0 : c < 0;
    });
    m_vue = vue;

    QTableWidget *t = ui->tableEmployes;
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

    if (m_droits.modifier) {
        auto *btn = new QPushButton("Modifier", conteneur);
        btn->setObjectName("btnModifierLigne");
        btn->setFixedHeight(28);
        btn->setCursor(Qt::PointingHandCursor);
        lay->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, [this, employeId]() { onModifier(employeId); });
    }
    if (m_droits.supprimer) {
        auto *btn = new QPushButton("Supprimer", conteneur);
        btn->setObjectName("btnSupprimerLigne");
        btn->setFixedHeight(28);
        btn->setCursor(Qt::PointingHandCursor);
        lay->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, [this, employeId]() { onSupprimer(employeId); });
    }
    return conteneur;
}

// ---------------------------------------------------------------------------
// Formulaire
// ---------------------------------------------------------------------------
void GestionEmployes::viderFormulaire()
{
    ui->labelDetailTitreEmp->setText(m_droits.ajouter ? "Nouvel employé" : "Détails de l'employé");
    ui->editId->clear();
    ui->editNom->clear();
    ui->editPrenom->clear();
    ui->editEmail->clear();
    ui->editAdresse->clear();
    ui->editTelephone->clear();
    ui->editSalaire->clear();
    ui->dateEmbauche->setDate(QDate::currentDate());
    ui->comboStatut->setCurrentIndex(0);
    ui->comboPoste->setCurrentIndex(0);
    ui->editMdp->clear();
    ui->editMdp->setPlaceholderText("Obligatoire (6 caractères min.)");
    majEtatVisage(-1);
}

void GestionEmployes::remplirFormulaire(const Employe &e)
{
    ui->labelDetailTitreEmp->setText("Détails de l'employé");
    ui->editId->setText(QString::number(e.id));
    ui->editNom->setText(e.nom);
    ui->editPrenom->setText(e.prenom);
    ui->editEmail->setText(e.email);
    ui->editAdresse->setText(e.adresse);
    ui->editTelephone->setText(e.telephone);
    ui->editSalaire->setText(QString::number(e.salaire, 'f', 2));
    ui->dateEmbauche->setDate(e.dateEmbauche);
    ui->comboStatut->setCurrentText(e.statut);
    ui->comboPoste->setCurrentText(e.poste);
    ui->editMdp->clear();                                   // le mot de passe n'est jamais réaffiché
    ui->editMdp->setPlaceholderText("Laisser vide pour le conserver");
    majEtatVisage(e.id);
}

void GestionEmployes::majEtatVisage(int employeId)
{
    const Employe *e = m_base.trouver(employeId);
    // Visible pour qui peut modifier les fiches, et pour chacun sur sa propre fiche.
    const bool visible = e && (m_droits.modifier || e->id == m_userId);
    ui->labelVisage->setVisible(visible);
    ui->btnVisage->setVisible(visible);
    ui->btnSupprVisage->setVisible(visible);
    if (!visible)
        return;
    const int n = e->gabaritsVisage.size();
    ui->labelVisage->setText(n > 0 ? QString("Visage : enregistré (%1 échantillons)").arg(n)
                                 : QString("Visage : non enregistré"));
    ui->btnVisage->setText(n > 0 ? "Réenregistrer le visage" : "Enregistrer le visage");
    ui->btnSupprVisage->setEnabled(n > 0);
}

void GestionEmployes::onSelectionChanged()
{
    const QList<QTableWidgetItem *> sel = ui->tableEmployes->selectedItems();
    if (sel.isEmpty())
        return;

    const QTableWidgetItem *premier = ui->tableEmployes->item(sel.first()->row(), 0);
    if (!premier)
        return;
    const int id = premier->data(Qt::UserRole).toInt();
    if (const Employe *e = m_base.trouver(id)) {
        m_currentId = id;
        remplirFormulaire(*e);
    }
}

// ---------------------------------------------------------------------------
// Actions CRUD
// ---------------------------------------------------------------------------
void GestionEmployes::onAjouter()
{
    if (!m_droits.ajouter)
        return;
    m_currentId = -1;
    ui->tableEmployes->clearSelection();
    viderFormulaire();
    ui->editNom->setFocus();
}

void GestionEmployes::onModifier(int employeId)
{
    if (!m_droits.modifier)
        return;
    const Employe *e = m_base.trouver(employeId);
    if (!e)
        return;
    m_currentId = employeId;
    remplirFormulaire(*e);

    for (int r = 0; r < ui->tableEmployes->rowCount(); ++r) {
        if (ui->tableEmployes->item(r, 0) && ui->tableEmployes->item(r, 0)->data(Qt::UserRole).toInt() == employeId) {
            ui->tableEmployes->selectRow(r);
            break;
        }
    }
    ui->editNom->setFocus();
    ui->editNom->selectAll();
}

void GestionEmployes::onSupprimer(int employeId)
{
    if (!m_droits.supprimer)
        return;
    const Employe *e = m_base.trouver(employeId);
    if (!e)
        return;

    if (employeId == m_userId) {
        QMessageBox::warning(m_fenetre, "Suppression impossible", "Vous ne pouvez pas supprimer votre propre compte.");
        return;
    }
    if (e->poste == "Responsable RH" && m_base.nombreResponsablesRH() <= 1) {
        QMessageBox::warning(m_fenetre, "Suppression impossible",
                             "Il doit toujours rester au moins un Responsable RH.");
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
    FaceAuth::sauvegarderGabarits(m_base);          // retire aussi son visage du fichier
    refreshTable();
}

void GestionEmployes::onEnregistrer()
{
    const bool nouveau = (m_currentId < 0);
    if (nouveau ? !m_droits.ajouter : !m_droits.modifier)
        return;

    auto erreur = [this](const QString &titre, const QString &msg, QWidget *champ) {
        QMessageBox::warning(m_fenetre, titre, msg);
        champ->setFocus();
    };

    const QString nom = ui->editNom->text().trimmed();
    const QString prenom = ui->editPrenom->text().trimmed();
    const QString email = ui->editEmail->text().trimmed();
    const QString tel = ui->editTelephone->text().trimmed();
    const QString mdp = ui->editMdp->text();

    if (nom.isEmpty())    return erreur("Champ manquant", "Le nom est obligatoire.", ui->editNom);
    if (prenom.isEmpty()) return erreur("Champ manquant", "Le prénom est obligatoire.", ui->editPrenom);

    static const QRegularExpression reEmail("^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$");
    if (!reEmail.match(email).hasMatch())
        return erreur("Email invalide", "Saisissez un email valide (ex. prenom.nom@petnova.tn).", ui->editEmail);

    const Employe *memeEmail = m_base.trouverParEmail(email);
    if (memeEmail && memeEmail->id != m_currentId)
        return erreur("Email déjà utilisé", "Un autre employé utilise déjà cet email.", ui->editEmail);

    static const QRegularExpression reTel("^\\+?[0-9 ]{8,15}$");
    if (!reTel.match(tel).hasMatch())
        return erreur("Téléphone invalide", "Saisissez un numéro valide (8 à 15 chiffres).", ui->editTelephone);

    double salaire = 0.0;
    if (m_droits.voirSalaires) {
        bool ok = false;
        QString txt = ui->editSalaire->text().trimmed();
        txt.replace(',', '.');
        salaire = txt.toDouble(&ok);
        if (!ok || salaire < 0)
            return erreur("Salaire invalide", "Saisissez un salaire valide (ex. 1800 ou 1800.50).", ui->editSalaire);
    }

    if (nouveau && mdp.length() < 6)
        return erreur("Mot de passe invalide", "Le mot de passe doit contenir au moins 6 caractères.", ui->editMdp);
    if (!nouveau && !mdp.isEmpty() && mdp.length() < 6)
        return erreur("Mot de passe invalide", "Le mot de passe doit contenir au moins 6 caractères.", ui->editMdp);

    Employe *existant = nouveau ? nullptr : m_base.trouver(m_currentId);

    if (existant) {
        // Règles de sécurité sur les comptes sensibles
        if (existant->id == m_userId
            && (ui->comboPoste->currentText() != existant->poste || ui->comboStatut->currentText() != existant->statut)) {
            QMessageBox::warning(m_fenetre, "Action refusée",
                                 "Vous ne pouvez pas modifier votre propre poste ou votre propre statut.");
            return;
        }
        if (existant->poste == "Responsable RH" && ui->comboPoste->currentText() != "Responsable RH"
            && m_base.nombreResponsablesRH() <= 1) {
            QMessageBox::warning(m_fenetre, "Action refusée", "Il doit toujours rester au moins un Responsable RH.");
            return;
        }
    }

    Employe e = existant ? *existant : Employe();
    e.nom = nom;
    e.prenom = prenom;
    e.email = email;
    e.adresse = ui->editAdresse->text().trimmed();
    e.telephone = tel;
    if (m_droits.voirSalaires)
        e.salaire = salaire;
    e.dateEmbauche = ui->dateEmbauche->date();
    e.statut = ui->comboStatut->currentText();
    e.poste = ui->comboPoste->currentText();
    if (!mdp.isEmpty())
        e.motDePasse = Emp::hacher(mdp);

    if (nouveau) {
        e.id = m_base.prochainId++;
        m_base.employes.append(e);
        m_currentId = e.id;
    } else {
        *existant = e;
    }

    ui->editId->setText(QString::number(m_currentId));
    ui->labelDetailTitreEmp->setText("Détails de l'employé");
    ui->editMdp->clear();
    ui->editMdp->setPlaceholderText("Laisser vide pour le conserver");
    FaceAuth::sauvegarderGabarits(m_base);          // les visages sont indexés par email
    majEtatVisage(m_currentId);
    refreshTable();
}

void GestionEmployes::onAnnuler()
{
    if (const Employe *e = m_base.trouver(m_currentId))
        remplirFormulaire(*e);
    else
        viderFormulaire();
}

// ---------------------------------------------------------------------------
// Export PDF (la liste exportée = ce qui est affiché : filtre + tri appliqués)
// ---------------------------------------------------------------------------
GestionEmployes::DonneesExport GestionEmployes::donneesExport() const
{
    DonneesExport d;
    d.entetes = {"ID", "Nom", "Prénom", "Email", "Adresse", "Téléphone", "Poste", "Statut", "Date d'embauche"};
    if (m_droits.voirSalaires)
        d.entetes << "Salaire (DT)";

    for (const Employe &e : m_vue) {
        QStringList l = {QString::number(e.id), e.nom, e.prenom, e.email, e.adresse, e.telephone,
                         e.poste, e.statut, e.dateEmbauche.toString("dd/MM/yyyy")};
        if (m_droits.voirSalaires)
            l << QString::number(e.salaire, 'f', 2);
        d.lignes.append(l);
    }
    return d;
}

QString GestionEmployes::htmlTableau() const
{
    const DonneesExport d = donneesExport();
    const QString texte = QStringLiteral("#1F2A37");
    const QString turquoise = QStringLiteral("#0F766E");

    // Titre, date d'exportation et nombre total d'employés (Arial ; titre 20 pt gras).
    QString html = QStringLiteral("<div style='font-family:Arial; color:%1;'>").arg(texte);
    html += QStringLiteral("<h1 style='font-family:Arial; font-size:20pt; font-weight:bold; color:%1;'>"
                           "Liste des employés - Pet Nova</h1>").arg(turquoise);
    html += QStringLiteral("<p style='font-size:11pt;'>Date d'exportation : <b>%1</b><br>"
                           "Nombre total d'employés : <b>%2</b></p>")
                .arg(QDate::currentDate().toString("dd/MM/yyyy"))
                .arg(d.lignes.size());

    // Tableau structuré : en-tête turquoise, lignes alternées.
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
    if (!m_droits.exporter)
        return;
    QString chemin = QFileDialog::getSaveFileName(m_fenetre, "Exporter en PDF", "employes.pdf", "Fichiers PDF (*.pdf)");
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
    if (m_droits.statistiques)
        emit statistiquesDemandees(m_droits.voirSalaires);
}

// ---------------------------------------------------------------------------
// Rôles / privilèges et reconnaissance faciale
// ---------------------------------------------------------------------------
void GestionEmployes::onPrivileges()
{
    if (!m_droits.gererPrivileges)
        return;
    emit privilegesDemandes();
}

void GestionEmployes::onEnregistrerVisage()
{
    const Employe *e = m_base.trouver(m_currentId);
    if (!e || !(m_droits.modifier || e->id == m_userId))
        return;
    if (!e->gabaritsVisage.isEmpty()
        && QMessageBox::question(m_fenetre, "Visage", "Un visage est déjà enregistré pour cet employé. Le remplacer ?")
               != QMessageBox::Yes)
        return;

    if (FaceAuth::enregistrer(ui, m_fenetre, m_base, m_currentId)) {
        majEtatVisage(m_currentId);
        QMessageBox::information(m_fenetre, "Visage", "Le visage a été enregistré.\n"
                                                 "Il peut maintenant servir à se connecter.");
    }
}

void GestionEmployes::onSupprimerVisage()
{
    Employe *e = m_base.trouver(m_currentId);
    if (!e || !(m_droits.modifier || e->id == m_userId) || e->gabaritsVisage.isEmpty())
        return;
    if (QMessageBox::question(m_fenetre, "Visage", "Retirer le visage enregistré de cet employé ?") != QMessageBox::Yes)
        return;
    e->gabaritsVisage.clear();
    FaceAuth::sauvegarderGabarits(m_base);
    majEtatVisage(m_currentId);
}

// ---------------------------------------------------------------------------
// Données de démonstration des employés (en mémoire). Mot de passe de tous les comptes : petnova123,
// sauf le Responsable RH (rh@petnova.tn) : admin123.
void chargerEmployesDemo(BaseEmployes &base)
{
    auto ajout = [&base](const QString &nom, const QString &prenom, const QString &email,
                         const QString &adresse, const QString &tel, double salaire,
                         const QDate &date, const QString &statut, const QString &poste,
                         const QString &mdp) {
        Employe e;
        e.id = base.prochainId++;
        e.nom = nom; e.prenom = prenom; e.email = email; e.adresse = adresse;
        e.telephone = tel; e.salaire = salaire; e.dateEmbauche = date;
        e.statut = statut; e.poste = poste; e.motDePasse = Emp::hacher(mdp);
        base.employes.append(e);
    };

    ajout("Ben Salah", "Ali",     "rh@petnova.tn",        "12 rue de la Liberté, Tunis",   "21650123456", 3200, QDate(2019, 3, 4),  "Actif",    "Responsable RH",        "admin123");
    ajout("Trabelsi",  "Sonia",   "sonia.trabelsi@petnova.tn", "5 avenue Habib Bourguiba, Tunis", "21698234567", 2800, QDate(2020, 9, 14), "Actif",    "Comptable",             "petnova123");
    ajout("Gharbi",    "Mohamed", "mohamed.gharbi@petnova.tn", "8 rue Ibn Khaldoun, Ariana",      "21655345678", 3900, QDate(2018, 1, 22), "Actif",    "Vétérinaire",           "petnova123");
    ajout("Jlassi",    "Ines",    "ines.jlassi@petnova.tn",    "20 rue des Jasmins, La Marsa",    "21622456789", 3700, QDate(2021, 5, 3),  "Actif", "Vétérinaire",           "petnova123");
    ajout("Mansour",   "Karim",   "karim.mansour@petnova.tn",  "3 rue de Carthage, Tunis",        "21697567890", 1500, QDate(2022, 2, 7),  "Actif",    "Assistant vétérinaire", "petnova123");
    ajout("Dridi",     "Yasmine", "yasmine.dridi@petnova.tn",  "17 avenue de la République, Ben Arous", "21652678901", 1450, QDate(2023, 6, 19), "Actif", "Assistant vétérinaire", "petnova123");
    ajout("Hamdi",     "Nour",    "nour.hamdi@petnova.tn",     "9 rue Mongi Slim, Tunis",         "21623789012", 1300, QDate(2023, 10, 2), "Actif",    "Réceptionniste",        "petnova123");
    ajout("Ayari",     "Sami",    "sami.ayari@petnova.tn",     "14 rue Farhat Hached, Radès",     "21699890123", 1250, QDate(2024, 1, 15), "Inactif", "Réceptionniste",        "petnova123");
    ajout("Chaabane",  "Rim",     "rim.chaabane@petnova.tn",   "6 rue du Lac, Les Berges du Lac", "21653901234", 1400, QDate(2022, 8, 29), "Actif",    "Toiletteur",            "petnova123");
    ajout("Bouzid",    "Hatem",   "hatem.bouzid@petnova.tn",   "11 rue de Marseille, Tunis",      "21624012345", 1350, QDate(2021, 11, 8), "Inactif", "Toiletteur",      "petnova123");
}
