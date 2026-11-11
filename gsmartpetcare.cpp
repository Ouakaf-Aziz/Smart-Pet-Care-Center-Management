#include "gsmartpetcare.h"
#include "ui_gsmartpetcare.h"
#include "employe.h"

#include "animal.h"
#include <QColor>
#include <QDoubleSpinBox>
#include <QFontMetrics>
#include <QFrame>
#include <QScrollArea>
#include <QTextEdit>
#include <initializer_list>
#include <utility>

#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QGraphicsDropShadowEffect>
#include <QIcon>
#include <QStyle>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QLineEdit>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QLayoutItem>
#include <array>
#include <functional>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QResizeEvent>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QTextDocument>
#include <QTime>
#include <QUrl>
#include <QDateEdit>
#include <QDebug>
#include <QDialog>
#include <QFont>
#include <QFormLayout>
#include <QGuiApplication>
#include <QMap>
#include <QMarginsF>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QScreen>
#include <QSet>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <algorithm>
#include <cmath>

namespace {

// Colonnes de données (la colonne « Animal » éventuelle et « Actions » sont gérées dynamiquement)
// Ordre = celui des en-têtes du tableau du .ui : Id, Priorité, Durée, Type, Salle, Heure, Date, Actions
enum Colonne { ColId, ColPriorite, ColDuree, ColType, ColSalle, ColHeure, ColDate };

const int RoleIdAnimal  = Qt::UserRole + 1;   // ID_ANIMAL mémorisé dans la cellule « Id »
const int RoleIdEmploye = Qt::UserRole + 2;   // ID_EMPLOYE mémorisé dans la cellule « Id »

// Grille (QGridLayout) qui contient le widget w
QGridLayout *trouverGrille(QWidget *w)
{
    std::function<QGridLayout *(QLayout *)> chercher = [&](QLayout *l) -> QGridLayout * {
        if (!l) return nullptr;
        if (auto *gr = qobject_cast<QGridLayout *>(l))
            if (gr->indexOf(w) >= 0) return gr;
        for (int i = 0; i < l->count(); ++i)
            if (QLayout *sub = l->itemAt(i)->layout())
                if (auto *r = chercher(sub)) return r;
        return nullptr;
    };
    return w->parentWidget() ? chercher(w->parentWidget()->layout()) : nullptr;
}

// Texte (en majuscules) des étiquettes situées sur la ligne juste au-dessus du widget w :
// permet de reconnaître « ID EMPLOYÉ » / « ID ANIMAL » même si l'objectName est différent.
QString etiquetteDe(QWidget *w)
{
    QGridLayout *g = trouverGrille(w);
    if (!g) return QString();
    int r, c, rs, cs;
    g->getItemPosition(g->indexOf(w), &r, &c, &rs, &cs);
    QString texte;
    for (int i = 0; i < g->count(); ++i) {
        int ri, ci, rsi, csi;
        g->getItemPosition(i, &ri, &ci, &rsi, &csi);
        if (ri + rsi - 1 == r - 1)
            if (auto *lbl = qobject_cast<QLabel *>(g->itemAt(i)->widget()))
                texte += lbl->text().toUpper() + " ";
    }
    return texte;
}

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

// ===========================================================================
// === CONSTRUCTEUR & DESTRUCTEUR ============================================
// ===========================================================================
GSmartPetCare::GSmartPetCare(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GSmartPetCare)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/images/icone.png"));
    resize(1366, 800);

    // Ombre légère sous la carte de connexion
    auto *ombre = new QGraphicsDropShadowEffect(this);
    ombre->setBlurRadius(30);
    ombre->setOffset(0, 4);
    ombre->setColor(QColor(0, 0, 0, 45));
    ui->frameCard->setGraphicsEffect(ombre);

    // La connexion est gérée par le module Employés.
    ui->stackedWidget->setCurrentIndex(PageLogin);
    ui->lineEditIdentifiant->setFocus();
    connect(ui->lineEditIdentifiant, &QLineEdit::returnPressed,
            ui->lineEditMotDePasse, qOverload<>(&QWidget::setFocus));
    connect(ui->lineEditMotDePasse, &QLineEdit::returnPressed,
            ui->pushButtonConnexion, &QPushButton::click);
    chargerEmployesDemo(m_baseEmployes);
    FaceAuth::chargerGabarits(m_baseEmployes);
    FaceDialog::preparerPage(ui);
    m_connexion = new Connexion(ui, m_baseEmployes, this);
    m_employes = new GestionEmployes(ui, m_baseEmployes, this);
    m_statsEmployes = new StatistiquesEmployes(ui, this);
    m_privileges = new Privileges(ui, m_baseEmployes, this);

    connect(m_connexion, &Connexion::connecte, this, &GSmartPetCare::onConnecte);
    connect(m_connexion, &Connexion::abandon, this, &QWidget::close);
    connect(m_employes, &GestionEmployes::statistiquesDemandees,
            this, &GSmartPetCare::afficherStatistiquesEmployes);
    connect(m_employes, &GestionEmployes::privilegesDemandes,
            this, &GSmartPetCare::afficherPrivileges);
    connect(m_statsEmployes, &StatistiquesEmployes::retour, this, [this]() {
        afficherContenu(ContenuEmployes);
    });
    connect(m_privileges, &Privileges::termine, this, [this]() {
        afficherContenu(ContenuEmployes);
    });

    // Modules intégrés (lisent et écrivent dans Oracle) :
    initialiserRdv();                    // MODULE RENDEZ-VOUS
    new GestionServices(ui, this, this); // MODULE SERVICE (+ historique et statistiques)
    configurerPageAnimaux();               // MODULE ANIMAL

    // Navigation depuis le menu latéral (employés et animaux : interface seulement pour l'instant)
    afficherContenu(ContenuAccueil);
    connect(ui->btnNavAccueil,  &QPushButton::clicked, this, [this]() { afficherContenu(ContenuAccueil); });
    connect(ui->btnNavServices, &QPushButton::clicked, this, [this]() { afficherContenu(ContenuServices); });
    connect(ui->btnNavEmployes, &QPushButton::clicked, this, [this]() { afficherContenu(ContenuEmployes); });
    connect(ui->btnNavRDV,      &QPushButton::clicked, this, [this]() {
        rafraichirRdv();                    // recharge les rendez-vous et les animaux depuis la base
        afficherContenu(ContenuRDV);
    });
    connect(ui->btnNavAnimaux,  &QPushButton::clicked, this, [this]() {
        rafraichirTableAnimaux();
        afficherContenu(ContenuAnimaux);
    });

    // Déconnexion : retour à la page de connexion
    connect(ui->btnDeconnexion, &QPushButton::clicked, this, [this]() {
        afficherContenu(ContenuAccueil);
        ui->stackedWidget->setCurrentIndex(PageLogin);
        ui->lineEditIdentifiant->clear();
        ui->lineEditMotDePasse->clear();
        ui->labelErreurConnexion->hide();
        ui->lineEditIdentifiant->setFocus();
    });
}

GSmartPetCare::~GSmartPetCare()
{
    delete ui;
}

void GSmartPetCare::onConnecte(int employeId)
{
    m_employes->setUtilisateur(employeId);
    if (const Employe *employe = m_baseEmployes.trouver(employeId)) {
        ui->labelBienvenue->setText(
            QStringLiteral("Bienvenue %1 %2  -  Rôle : %3")
                .arg(employe->prenom, employe->nom, employe->poste));
    }
    afficherContenu(ContenuAccueil);
    ui->stackedWidget->setCurrentIndex(PageAccueil);
}

void GSmartPetCare::afficherStatistiquesEmployes(bool voirSalaires)
{
    m_statsEmployes->afficher(m_baseEmployes.employes, voirSalaires);
    afficherContenu(ContenuStatsEmployes);
}

void GSmartPetCare::afficherPrivileges()
{
    m_privileges->ouvrir();
    afficherContenu(ContenuPrivileges);
}

// ===========================================================================
// === SLOTS GÉNÉRAUX (NAVIGATION ENTRE PAGES) ===============================
// ===========================================================================
void GSmartPetCare::afficherContenu(int index)
{
    ui->stackContenu->setCurrentIndex(index);

    // Le bouton actif est repéré par la propriété « actif » (voir la feuille de style du .ui).
    const bool pageEmployes = index == ContenuEmployes || index == ContenuStatsEmployes || index == ContenuPrivileges;
    const struct { QPushButton *bouton; bool actif; } liens[] = {
                  { ui->btnNavAccueil,  index == ContenuAccueil },
                  { ui->btnNavServices, index == ContenuServices },
                  { ui->btnNavEmployes, pageEmployes },
                  { ui->btnNavRDV,      index == ContenuRDV },
                  { ui->btnNavAnimaux,  index == ContenuAnimaux },
                  };
    for (const auto &l : liens) {
        l.bouton->setProperty("actif", l.actif);
        l.bouton->style()->unpolish(l.bouton);
        l.bouton->style()->polish(l.bouton);
    }
}

// ===========================================================================
// === MODULE RENDEZ-VOUS ====================================================
//                       PAGE « GESTION DES RENDEZ-VOUS »
// ===========================================================================
void GSmartPetCare::initialiserRdv()
{
    ui->pageRDV->setAttribute(Qt::WA_StyledBackground, true);   // fond de la feuille de style

    // --- Connexions ---
    connect(ui->pushButtonEnregistrerRdv,  &QPushButton::clicked, this, &GSmartPetCare::enregistrerRdv);
    connect(ui->pushButtonAnnulerRdv,      &QPushButton::clicked, this, &GSmartPetCare::annulerRdv);
    connect(ui->pushButtonStatistiquesRdv, &QPushButton::clicked, this, &GSmartPetCare::afficherStatistiques);
    connect(ui->pushButtonExporterPdfRdv,  &QPushButton::clicked, this, &GSmartPetCare::exporterPdf);

    m_checkEmail = ui->pageRDV->findChild<QCheckBox *>("checkBoxEmailRdv");   // facultatif

    // Widgets « Animal » (présents dans votre .ui : adaptez les noms si besoin)
    // Liste « Animal » : par son nom, sinon n'importe quelle liste dont le nom contient « animal »
    m_comboAnimal = ui->pageRDV->findChild<QComboBox *>("comboBoxAnimalRdv");
    if (!m_comboAnimal) {
        for (QComboBox *cb : ui->pageRDV->findChildren<QComboBox *>())
            if (cb->objectName().contains("animal", Qt::CaseInsensitive)) { m_comboAnimal = cb; break; }
    }
    // Champs « ID ANIMAL » et « ID EMPLOYÉ » : retrouvés par leur nom (lineEditIdAnimalRdv,
    // lineEditIdEmployeRdv...) ou, à défaut, par l'étiquette placée juste au-dessus.
    m_editIdAnimal  = ui->pageRDV->findChild<QLineEdit *>("lineEditIdAnimalRdv");
    m_editIdEmploye = ui->pageRDV->findChild<QLineEdit *>("lineEditIdEmployeRdv");
    for (QLineEdit *le : ui->pageRDV->findChildren<QLineEdit *>()) {
        const QString n = le->objectName();
        if (le == ui->lineEditIdRdv || le == m_editIdAnimal || le == m_editIdEmploye
            || n.contains("recherche", Qt::CaseInsensitive) || n.startsWith("qt_"))
            continue;
        const QString et = etiquetteDe(le);
        const bool estEmploye = n.contains("employ", Qt::CaseInsensitive)
                                || n.contains("idemp", Qt::CaseInsensitive) || et.contains("EMPLOY");
        const bool estAnimal  = n.contains("animal", Qt::CaseInsensitive) || et.contains("ANIMAL");
        if (estEmploye && !m_editIdEmploye)      m_editIdEmploye = le;
        else if (estAnimal && !m_editIdAnimal)   m_editIdAnimal = le;
    }
    ajouterChampsManquants();     // crée « Animal » / « ID employé » si votre .ui ne les contient pas
    if (m_comboAnimal && m_editIdAnimal) {
        connect(m_comboAnimal, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            const QVariant id = m_comboAnimal->currentData();
            m_editIdAnimal->setText(id.isValid() ? id.toString() : QString());
        });
    }

    // L'ID du rendez-vous est géré automatiquement : jamais modifiable à la main
    ui->lineEditIdRdv->setReadOnly(true);
    ui->lineEditIdRdv->setFocusPolicy(Qt::NoFocus);
    ui->lineEditIdRdv->setPlaceholderText(QString());

    configurerTableau();
    // Un clic sur une ligne charge tout le rendez-vous dans le formulaire (mode « Modifier »)
    connect(ui->tableWidgetRdv, &QTableWidget::cellClicked, this, [this](int row, int) {
        modifierLigne(row);
    });
    remplirComboAnimaux();
    chargerDepuisBase();              // les rendez-vous viennent de la base, plus du code
    nouveauFormulaire();              // le panneau de droite démarre en mode « Ajouter »
}


// ---------------------------------------------------------------------------
// Champs ajoutés par le code si le .ui ne les contient pas
// ---------------------------------------------------------------------------
void GSmartPetCare::ajouterChampsManquants()
{
    if (m_comboAnimal && m_editIdEmploye)
        return;

    // Grille du formulaire = celle qui contient le champ « ID RDV »
    QWidget *ref = ui->lineEditIdRdv;
    QGridLayout *g = trouverGrille(ref);
    if (!g)
        return;

    QWidget *parent = ref->parentWidget();
    const QString styleLabel = "font-size:9pt; font-weight:bold; color:#6B7A78; padding-top:8px;";

    if (!m_comboAnimal) {
        // Juste après la ligne « ID RDV » : on décale les lignes suivantes de 2 rangées
        int rId = 0, cId = 0, rs = 1, cs = 1;
        g->getItemPosition(g->indexOf(ref), &rId, &cId, &rs, &cs);
        const int base = rId + rs;

        struct Pos { QLayoutItem *it; int r, c, rs, cs; Qt::Alignment al; };
        QList<Pos> aDecaler;
        for (int i = 0; i < g->count(); ++i) {
            int r, c, rsp, csp;
            g->getItemPosition(i, &r, &c, &rsp, &csp);
            if (r >= base)
                aDecaler.append({ g->itemAt(i), r, c, rsp, csp, g->itemAt(i)->alignment() });
        }
        for (const Pos &p : aDecaler) g->removeItem(p.it);
        for (const Pos &p : aDecaler) g->addItem(p.it, p.r + 2, p.c, p.rs, p.cs, p.al);

        auto *lbl = new QLabel("ANIMAL", parent);
        lbl->setObjectName("labelChampAnimalRdv");
        lbl->setStyleSheet(styleLabel);

        m_comboAnimal = new QComboBox(parent);
        m_comboAnimal->setObjectName("comboBoxAnimalRdv");
        m_comboAnimal->setMinimumHeight(34);

        g->addWidget(lbl, base, 0, 1, 2);
        g->addWidget(m_comboAnimal, base + 1, 0, 1, 2);
    }

    if (!m_editIdEmploye) {
        const int row = g->rowCount();
        auto *lbl = new QLabel(QString::fromUtf8("ID EMPLOYÉ"), parent);
        lbl->setObjectName("labelChampEmployeRdv");
        lbl->setStyleSheet(styleLabel);

        m_editIdEmploye = new QLineEdit(parent);
        m_editIdEmploye->setObjectName("lineEditIdEmployeRdv");
        m_editIdEmploye->setMinimumHeight(34);
        m_editIdEmploye->setPlaceholderText("ex. 12");

        g->addWidget(lbl, row, 0, 1, 2);
        g->addWidget(m_editIdEmploye, row + 1, 0, 1, 2);
    }
}

// ---------------------------------------------------------------------------
// Tableau
// ---------------------------------------------------------------------------
void GSmartPetCare::configurerTableau()
{
    QTableWidget *t = ui->tableWidgetRdv;

    // Colonnes repérées par leur en-tête ; « Actions » est la dernière.
    auto trouverCol = [t](const QString &motif) {
        for (int i = 0; i < t->columnCount(); ++i) {
            QTableWidgetItem *hi = t->horizontalHeaderItem(i);
            if (!hi) continue;
            const QString ht = hi->text().trimmed().toLower();
            if (ht == motif || (motif == "id employe" && ht.startsWith("id employ"))) return i;
        }
        return -1;
    };
    // Si votre .ui n'a pas ces colonnes, on les crée juste avant « Actions »
    auto assurerCol = [t, trouverCol](const QString &titre) {
        int c = trouverCol(titre.toLower());
        if (c < 0) {
            c = t->columnCount() - 1;                 // position de « Actions »
            t->insertColumn(c);
            t->setHorizontalHeaderItem(c, new QTableWidgetItem(titre));
        }
        return c;
    };
    m_colAnimal    = assurerCol("Animal");
    m_colIdEmploye = assurerCol("id employe");
    m_colIdAnimal  = assurerCol("Id animal");
    m_colAction    = t->columnCount() - 1;

    QHeaderView *h = t->horizontalHeader();
    h->setMinimumSectionSize(40);
    h->setStretchLastSection(false);
    h->setFixedHeight(44);                                   // même hauteur que la page des services
    h->setDefaultAlignment(Qt::AlignCenter);

    h->setSectionResizeMode(ColId,       QHeaderView::Fixed);    h->resizeSection(ColId, 68);
    h->setSectionResizeMode(ColPriorite, QHeaderView::Fixed);    h->resizeSection(ColPriorite, 90);
    h->setSectionResizeMode(ColDuree,    QHeaderView::Fixed);    h->resizeSection(ColDuree, 58);
    h->setSectionResizeMode(ColType,     QHeaderView::Stretch);                       // colonne qui s'étire
    h->setSectionResizeMode(ColSalle,    QHeaderView::Fixed);    h->resizeSection(ColSalle, 60);
    h->setSectionResizeMode(ColHeure,    QHeaderView::Fixed);    h->resizeSection(ColHeure, 54);
    h->setSectionResizeMode(ColDate,     QHeaderView::Fixed);    h->resizeSection(ColDate, 95);
    h->setSectionResizeMode(m_colAnimal,    QHeaderView::Fixed);  h->resizeSection(m_colAnimal, 100);
    h->setSectionResizeMode(m_colIdEmploye, QHeaderView::Fixed);  h->resizeSection(m_colIdEmploye, 100);
    h->setSectionResizeMode(m_colIdAnimal,  QHeaderView::Fixed);  h->resizeSection(m_colIdAnimal, 92);
    h->setSectionResizeMode(m_colAction, QHeaderView::Fixed);    h->resizeSection(m_colAction, 172);
}

void GSmartPetCare::remplirLigne(int row, const QStringList &valeurs)
{
    QTableWidget *t = ui->tableWidgetRdv;

    // valeurs : 7 colonnes de données, puis (facultatif) le nom de l'animal
    for (int col = 0; col < valeurs.size() && col < m_colAction; ++col) {
        auto *item = new QTableWidgetItem(valeurs[col]);
        item->setTextAlignment(Qt::AlignCenter);
        item->setToolTip(valeurs[col]);
        t->setItem(row, col, item);
    }
}

int GSmartPetCare::idNumerique(const QString &texte)
{
    QString chiffres;
    for (const QChar ch : texte)
        if (ch.isDigit()) chiffres += ch;
    return chiffres.toInt();          // 0 si aucun chiffre
}

// Liste déroulante des animaux (table ANIMAL)
void GSmartPetCare::remplirComboAnimaux()
{
    if (!m_comboAnimal)
        return;

    const QVariant courant = m_comboAnimal->currentData();
    m_comboAnimal->blockSignals(true);
    m_comboAnimal->clear();
    m_comboAnimal->addItem(QString::fromUtf8("— Choisir un animal —"), QVariant());
    for (const auto &a : m_rdv.listerAnimaux())
        m_comboAnimal->addItem(a.second, a.first);
    const int i = m_comboAnimal->findData(courant);
    m_comboAnimal->setCurrentIndex(i >= 0 ? i : 0);
    m_comboAnimal->blockSignals(false);
}

// ID_ANIMAL choisi dans le formulaire (liste déroulante, sinon champ « ID ANIMAL »), nul si aucun
QVariant GSmartPetCare::idAnimalSaisi() const
{
    if (m_comboAnimal && m_comboAnimal->currentData().isValid())
        return m_comboAnimal->currentData().toInt();
    if (m_editIdAnimal) {
        const int id = idNumerique(m_editIdAnimal->text());
        if (id > 0) return id;
    }
    return QVariant();
}

// ID_EMPLOYE saisi dans le formulaire (nul si vide ou invalide)
QVariant GSmartPetCare::idEmployeSaisi() const
{
    if (m_editIdEmploye) {
        const int id = idNumerique(m_editIdEmploye->text());
        if (id > 0) return id;
    }
    return QVariant();
}

void GSmartPetCare::rafraichirRdv()
{
    remplirComboAnimaux();
    chargerDepuisBase();
}

// Remplit le tableau avec les rendez-vous de la table RENDEZ_VOUS
void GSmartPetCare::chargerDepuisBase()
{
    QTableWidget *t = ui->tableWidgetRdv;
    const QList<QStringList> lignes = m_rdv.afficher();

    if (lignes.isEmpty() && !m_rdv.getDerniereErreur().isEmpty()) {
        QMessageBox::warning(this, "Base de données",
                             "Impossible de lire les rendez-vous :\n" + m_rdv.getDerniereErreur());
    }

    t->setRowCount(0);
    for (const QStringList &l : lignes) {
        // l : ID, priorité, durée, type, salle, heure, date, nom animal, id animal, id employé
        const int idNum = l[0].toInt();
        QStringList valeurs = l.mid(0, 7);
        valeurs[ColId] = QString("%1").arg(idNum, 2, 10, QChar('0'));   // 1 -> 01, 12 -> 12
        valeurs << l[7];                                                     // nom de l'animal (colonne 7)

        const int row = t->rowCount();
        t->insertRow(row);
        remplirLigne(row, valeurs);
        // id employé et id animal affichés dans leurs colonnes (l[9] = ID_EMPLOYE, l[8] = ID_ANIMAL)
        for (const auto &cv : { qMakePair(m_colIdEmploye, l[9]), qMakePair(m_colIdAnimal, l[8]) }) {
            auto *it = new QTableWidgetItem(cv.second);
            it->setTextAlignment(Qt::AlignCenter);
            it->setToolTip(cv.second);
            t->setItem(row, cv.first, it);
        }
        t->item(row, ColId)->setData(RoleIdAnimal,  l[8].isEmpty()  ? QVariant() : QVariant(l[8].toInt()));
        t->item(row, ColId)->setData(RoleIdEmploye, l[9].isEmpty() ? QVariant() : QVariant(l[9].toInt()));
        ajouterBoutonsAction(row);
    }
}

// Boutons « Modifier » / « Supprimer » de la colonne Actions.
// Leur style vient de la feuille de style de pageRDV (btnModifierLigne / btnSupprimerLigne).
void GSmartPetCare::ajouterBoutonsAction(int row)
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
    t->setCellWidget(row, m_colAction, conteneur);

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
void GSmartPetCare::nouveauFormulaire()
{
    m_ligneEnModification = -1;
    ui->labelDetailTitreRdv->setText("Ajouter un rendez-vous");
    ui->pushButtonEnregistrerRdv->setText("Enregistrer");

    // ID automatique : prochain numéro libre (01, 02, 03...), non modifiable
    ui->lineEditIdRdv->setText(QString("%1").arg(m_rdv.prochainId(), 2, 10, QChar('0')));
    ui->dateEditRdv->setDate(QDate::currentDate());
    ui->timeEditRdv->setTime(QTime(9, 0));
    ui->comboBoxTypeRdv->setCurrentIndex(0);
    ui->spinBoxDureeRdv->setValue(0);
    ui->comboBoxPrioriteRdv->setCurrentIndex(0);
    ui->comboBoxSalleRdv->setCurrentIndex(0);
    if (m_checkEmail) m_checkEmail->setChecked(false);
    if (m_comboAnimal)  m_comboAnimal->setCurrentIndex(0);
    if (m_editIdAnimal) m_editIdAnimal->clear();
    if (m_editIdEmploye) m_editIdEmploye->clear();
}

// Mode « Modifier » : formulaire rempli avec le rendez-vous choisi.
void GSmartPetCare::modifierLigne(int r)
{
    QTableWidget *t = ui->tableWidgetRdv;
    auto texte = [t, r](int col) { return t->item(r, col) ? t->item(r, col)->text() : QString(); };

    ui->lineEditIdRdv->setText(texte(ColId));
    selectionnerCombo(ui->comboBoxPrioriteRdv, texte(ColPriorite));
    ui->spinBoxDureeRdv->setValue(texte(ColDuree).toInt());
    selectionnerCombo(ui->comboBoxTypeRdv,     texte(ColType));
    selectionnerCombo(ui->comboBoxSalleRdv,    texte(ColSalle));
    ui->timeEditRdv->setTime(QTime::fromString(texte(ColHeure), "HH:mm"));
    ui->dateEditRdv->setDate(QDate::fromString(texte(ColDate), "dd/MM/yyyy"));

    const QVariant idAnimal = t->item(r, ColId) ? t->item(r, ColId)->data(RoleIdAnimal) : QVariant();
    if (m_comboAnimal) {
        const int i = idAnimal.isValid() ? m_comboAnimal->findData(idAnimal.toInt()) : 0;
        m_comboAnimal->setCurrentIndex(i >= 0 ? i : 0);
    }
    if (m_editIdAnimal)
        m_editIdAnimal->setText(idAnimal.isValid() ? idAnimal.toString() : QString());
    if (m_editIdEmploye) {
        const QVariant idEmp = t->item(r, ColId) ? t->item(r, ColId)->data(RoleIdEmploye) : QVariant();
        m_editIdEmploye->setText(idEmp.isValid() ? idEmp.toString() : QString());
    }

    m_ligneEnModification = r;
    ui->labelDetailTitreRdv->setText("Modifier le rendez-vous");
    ui->pushButtonEnregistrerRdv->setText(QString::fromUtf8("Mettre à jour"));
    t->selectRow(r);
}

// Bouton « Annuler » : abandonne la saisie (ou la modification) en cours.
void GSmartPetCare::annulerRdv()
{
    nouveauFormulaire();
    ui->tableWidgetRdv->clearSelection();
}

// Bouton « Enregistrer » : INSERT (ou UPDATE en modification) dans la base Oracle,
// puis relecture du tableau depuis la base.
void GSmartPetCare::enregistrerRdv()
{
    QTableWidget *t = ui->tableWidgetRdv;
    const bool modification = (m_ligneEnModification >= 0);

    if (ui->spinBoxDureeRdv->value() <= 0) {
        QMessageBox::warning(this, "Champ obligatoire",
                             QString::fromUtf8("Veuillez saisir une durée supérieure à 0."));
        ui->spinBoxDureeRdv->setFocus();
        return;
    }

    // ID_ANIMAL est obligatoire dans la table RENDEZ_VOUS (NOT NULL)
    if (!idAnimalSaisi().isValid()) {
        QMessageBox::warning(this, "Champ obligatoire",
                             (m_comboAnimal || m_editIdAnimal)
                                 ? QString::fromUtf8("Veuillez choisir un animal pour ce rendez-vous.")
                                 : QString::fromUtf8("Aucun champ « Animal » n'a été trouvé dans le formulaire "
                                                     "(comboBoxAnimalRdv / lineEditIdAnimalRdv). Vérifiez leurs noms dans le .ui."));
        if (m_comboAnimal) m_comboAnimal->setFocus();
        else if (m_editIdAnimal) m_editIdAnimal->setFocus();
        return;
    }

    // ID_EMPLOYE est obligatoire lui aussi : l'employé doit exister dans la table EMPLOYE
    if (!idEmployeSaisi().isValid()) {
        QMessageBox::warning(this, "Champ obligatoire",
                             QString::fromUtf8("Veuillez saisir l'ID de l'employé (nombre)."));
        if (m_editIdEmploye) m_editIdEmploye->setFocus();
        return;
    }
    if (!m_rdv.employeExiste(idEmployeSaisi().toInt())) {
        QMessageBox::warning(this, "Employé introuvable",
                             QString::fromUtf8("Aucun employé n'a l'ID %1 dans la base.").arg(idEmployeSaisi().toInt()));
        if (m_editIdEmploye) m_editIdEmploye->setFocus();
        return;
    }
    if (!m_rdv.animalExiste(idAnimalSaisi().toInt())) {
        QMessageBox::warning(this, "Animal introuvable",
                             QString::fromUtf8("Aucun animal n'a l'ID %1 dans la base.").arg(idAnimalSaisi().toInt()));
        if (m_editIdAnimal) m_editIdAnimal->setFocus();
        return;
    }

    // ID_RDV : automatique. Modification -> on garde l'ID de la ligne ; ajout -> prochain numéro libre.
    const int id = modification
                       ? idNumerique(t->item(m_ligneEnModification, ColId)->text())
                       : m_rdv.prochainId();

    const QString priorite = ui->comboBoxPrioriteRdv->currentText().trimmed();
    const int     duree    = ui->spinBoxDureeRdv->value();
    const QString type     = ui->comboBoxTypeRdv->currentText().trimmed();
    const QString salle    = ui->comboBoxSalleRdv->currentText().trimmed();
    const QTime   heure    = ui->timeEditRdv->time();
    const QDate   date     = ui->dateEditRdv->date();
    const QVariant animal  = idAnimalSaisi();
    const QVariant employe = idEmployeSaisi();

    const bool ok = modification
                        ? m_rdv.modifier(id, priorite, duree, type, salle, heure, date, animal, employe)
                        : m_rdv.ajouter (id, priorite, duree, type, salle, heure, date, animal, employe);

    if (!ok) {
        QMessageBox::critical(this, "Base de données",
                              QString::fromUtf8("L'enregistrement a échoué :\n") + m_rdv.getDerniereErreur());
        return;                      // on garde le formulaire tel quel pour corriger
    }

    chargerDepuisBase();             // le tableau reflète maintenant la base
    if (!modification)
        nouveauFormulaire();         // après un ajout : formulaire vide

    const QString idAffiche = QString("%1").arg(id, 2, 10, QChar('0'));
    for (int r = 0; r < t->rowCount(); ++r) {
        if (t->item(r, ColId) && t->item(r, ColId)->text() == idAffiche) {
            t->scrollToItem(t->item(r, ColId));
            if (modification)
                modifierLigne(r);    // après une modification : le formulaire garde toutes les valeurs
            else
                t->selectRow(r);
            break;
        }
    }
}

void GSmartPetCare::supprimerLigne(int r)
{
    QTableWidget *t = ui->tableWidgetRdv;
    const QString idAffiche = t->item(r, ColId)->text();

    const auto rep = QMessageBox::question(
        this, "Supprimer",
        "Voulez-vous vraiment supprimer le rendez-vous " + idAffiche + " ?");
    if (rep != QMessageBox::Yes)
        return;

    if (!m_rdv.supprimer(idNumerique(idAffiche))) {
        QMessageBox::critical(this, "Base de données",
                              QString::fromUtf8("La suppression a échoué :\n") + m_rdv.getDerniereErreur());
        return;
    }

    chargerDepuisBase();
    nouveauFormulaire();             // retour au mode « Ajouter »
}

// ---------------------------------------------------------------------------
// Statistiques (calculées par des requêtes SQL, dans la classe RendezVous)
// ---------------------------------------------------------------------------
void GSmartPetCare::afficherStatistiques()
{
    const struct { const char *titre; const char *colonne; } blocs[] = {
                  { "Par type",     "TYPE_RDV" },
                  { "Par priorité", "PRIORITE" },
                  { "Par salle",    "SALLE"    },
                  };

    QString texte;
    int total = 0;
    for (const auto &b : blocs) {
        const auto lignes = m_rdv.compterPar(b.colonne);
        if (lignes.isEmpty() && !m_rdv.getDerniereErreur().isEmpty()) {
            QMessageBox::warning(this, "Statistiques",
                                 QString::fromUtf8("Impossible de calculer les statistiques :\n") + m_rdv.getDerniereErreur());
            return;
        }
        texte += QString::fromUtf8(b.titre) + " :\n";
        int somme = 0;
        for (const auto &l : lignes) {
            texte += QString("   - %1 : %2\n").arg(l.first).arg(l.second);
            somme += l.second;
        }
        texte += "\n";
        total = somme;
    }

    QMessageBox::information(this, "Statistiques des rendez-vous",
                             QString::fromUtf8("Nombre total de rendez-vous : %1\n\n").arg(total) + texte);
}

// ---------------------------------------------------------------------------
// Export PDF
// ---------------------------------------------------------------------------

// Bouton « Exporter PDF » : demande où enregistrer le fichier puis génère le PDF
void GSmartPetCare::exporterPdf()
{
    if (ui->tableWidgetRdv->rowCount() == 0) {
        QMessageBox::information(this, "Exporter PDF", "Aucun rendez-vous à exporter.");
        return;
    }

    const QString suggestion = QDir::homePath() + "/rendez-vous_" +
                               QDate::currentDate().toString("yyyy-MM-dd") + ".pdf";

    QString chemin = QFileDialog::getSaveFileName(
        this, QString::fromUtf8("Exporter la liste des rendez-vous"),
        suggestion, "Fichiers PDF (*.pdf)");

    if (chemin.isEmpty())
        return;   // l'utilisateur a annulé

    if (!chemin.endsWith(".pdf", Qt::CaseInsensitive))
        chemin += ".pdf";

    if (!genererPdf(chemin)) {
        QMessageBox::critical(this, "Exporter PDF",
                              "Impossible de créer le fichier PDF.\n"
                              "Vérifiez que le fichier n'est pas ouvert dans un autre programme.");
        return;
    }

    const auto rep = QMessageBox::question(
        this, "Exporter PDF",
        QString::fromUtf8("Le PDF a été créé avec succès.\n\nVoulez-vous l'ouvrir ?"));
    if (rep == QMessageBox::Yes)
        QDesktopServices::openUrl(QUrl::fromLocalFile(chemin));
}

// Construit le document (titre + tableau) et l'écrit dans le fichier PDF
bool GSmartPetCare::genererPdf(const QString &chemin)
{
    QTableWidget *t = ui->tableWidgetRdv;

    // Colonnes exportées (la colonne Actions est ignorée)
    const QStringList entetes = {
        "Id", QString::fromUtf8("Priorité"), QString::fromUtf8("Durée (min)"),
        "Type", "Salle", "Heure", "Date"
    };
    const QList<int> largeurs = { 13, 16, 14, 21, 10, 12, 14 };   // en %

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

// ===========================================================================
// === MODULE EMPLOYÉ  (interface seulement : à intégrer) ====================
// ===========================================================================


// ===========================================================================
// === MODULE ANIMAL   (interface seulement : à intégrer) ====================
// ===========================================================================


// ===========================================================================
// === MODULE SERVICE ========================================================
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
    // Si la requête a échoué (modèle sans requête active), canFetchMore() resterait vrai : on ne boucle que si la requête est active.
    while (vue->query().isActive() && vue->canFetchMore())
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
    m_animaux = Animal::rechercherParTexte(ui->lineEditRechercheAnimalHisto->text());

    {
        const QSignalBlocker blocker(ui->tableAnimauxHisto);
        ui->tableAnimauxHisto->setRowCount(m_animaux.size());
        int ligne = -1;
        for (int r = 0; r < m_animaux.size(); ++r) {
            const Animal &a = m_animaux[r];
            auto *itNom = new QTableWidgetItem(a.nom);
            itNom->setData(Qt::UserRole, a.id);
            ui->tableAnimauxHisto->setItem(r, 0, itNom);
            ui->tableAnimauxHisto->setItem(r, 1, new QTableWidgetItem(a.espece));
            ui->tableAnimauxHisto->setItem(r, 2, new QTableWidgetItem(a.proprietaire));
            if (a.id == idAvant)
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
    const Animal *a = trouverAnimal(animalCourantId());

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

    QString fiche = QString("%1 — %2").arg(a->nom.toHtmlEscaped(), a->espece.toHtmlEscaped());
    fiche += QString("<br><span style='font-size:10pt;font-weight:normal;color:#5C6B69;'>Propriétaire : %1</span>")
                 .arg(a->proprietaire.isEmpty() ? "—" : a->proprietaire.toHtmlEscaped());
    ui->labelFicheHisto->setText(fiche);

    // Statistiques (COUNT / SUM / MAX en SQL) : elles ne dépendent pas du filtre
    const QVariantMap st = Animal::statistiquesServices(a->id);

    ui->labelNbServicesHisto->setText(QString("<span style='font-size:9pt;color:#6B7A78;'>SERVICES RENDUS</span><br><b style='font-size:16pt;color:#0B4F4A;'>%1</b>").arg(st.value("nombre").toInt()));
    ui->labelTotalHisto->setText(QString("<span style='font-size:9pt;color:#6B7A78;'>TOTAL FACTURÉ</span><br><b style='font-size:16pt;color:#0B4F4A;'>%1 DT</b>").arg(st.value("total").toDouble(), 0, 'f', 2));
    ui->labelDernierHisto->setText(QString("<span style='font-size:9pt;color:#6B7A78;'>DERNIER SERVICE</span><br><b style='font-size:16pt;color:#0B4F4A;'>%1</b>")
                                .arg(st.value("dernier").toDate().isValid() ? formatDate(st.value("dernier").toDate()) : QString("—")));

    // Filtre par type + tri exécutés par Oracle
    const QString typeFiltre = ui->comboFiltreTypeHisto->currentIndex() > 0 ? ui->comboFiltreTypeHisto->currentText() : QString();
    m_rendus = Service::historiqueAnimal(a->id, typeFiltre, ui->comboTriHisto->currentIndex());

    {
        const QSignalBlocker blocker(ui->tableHistorique);
        ui->tableHistorique->setRowCount(m_rendus.size());
        for (int ligne = 0; ligne < m_rendus.size(); ++ligne) {
            const Service &r = m_rendus[ligne];

            auto *itDate = new QTableWidgetItem(formatDate(r.getRdv().getDateRdv()));
            itDate->setData(Qt::UserRole, r.getRdv().getIdRdv());
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
    const QVector<RendezVous> rdvs = RendezVous::rendezVousAnimal(animalId);
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
                              .arg(r->getNom(), formatDate(r->getRdv().getDateRdv()));
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
        RendezVous rv;
        rv.setIdRdv(ui->comboRdv->currentData().toInt());
        r.setRdv(rv);
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
    const Animal *a = trouverAnimal(animalCourantId());
    if (!a) {
        QMessageBox::information(m_fenetre, "Export PDF", "Sélectionnez d'abord un animal.");
        return;
    }

    QString nomFichier = "historique_" + a->nom;
    nomFichier.replace(' ', '_');
    QString chemin = QFileDialog::getSaveFileName(m_fenetre, "Exporter l'historique en PDF",
                                                  nomFichier + ".pdf", "Fichiers PDF (*.pdf)");
    if (chemin.isEmpty())
        return;
    if (!chemin.endsWith(".pdf", Qt::CaseInsensitive))
        chemin += ".pdf";

    // Tous les services de l'animal, du plus récent au plus ancien (lus en base)
    const QVector<Service> liste = Service::historiqueAnimal(a->id, QString(), 0);

    double total = 0.0;
    for (const Service &r : liste)
        total += r.getPrixApplique();

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
    for (const Service &r : liste) {
        html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td align='right'>%4</td><td>%5</td></tr>")
                    .arg(formatDate(r.getRdv().getDateRdv()),
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
void HistoriqueServices::ouvrirSaisie(const Service &r, const QVector<RendezVous> &rdvs)
{
    const bool edition = r.getRdv().getIdRdv() > 0;

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
        ui->labelRdv->setText(QString("RDV n°%1 — %2").arg(r.getRdv().getIdRdv()).arg(formatDate(r.getRdv().getDateRdv())));
        ui->labelTitreSaisieRendu->setText("Modifier le service rendu");
        ui->lineEditPrixRendu->setText(QString::number(r.getPrixApplique(), 'f', 2));
    } else {
        for (const RendezVous &v : rdvs)
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

const Animal *HistoriqueServices::trouverAnimal(int id) const
{
    for (const Animal &a : m_animaux)
        if (a.id == id)
            return &a;
    return nullptr;
}

const Service *HistoriqueServices::trouverRendu(int idRdv, int idService) const
{
    for (const Service &r : m_rendus)
        if (r.getRdv().getIdRdv() == idRdv && r.getID() == idService)
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


namespace {

enum AnimalColonne { AnimalColId = 0, AnimalColNom, AnimalColEspece, AnimalColSexe, AnimalColNaissance, AnimalColProprietaire,
               AnimalColTelephone, AnimalColSuivi, AnimalColActions, AnimalNbColonnes };

const QString kAJourAnimal = QString::fromUtf8("À jour");

void selectionnerComboAnimal(QComboBox *combo, const QString &texte)
{
    const int i = combo->findText(texte, Qt::MatchFixedString);   // insensible à la casse
    combo->setCurrentIndex(i >= 0 ? i : 0);
}

QTableWidgetItem *celluleAnimal(const QString &texte, bool centre = true)
{
    auto *item = new QTableWidgetItem(texte);
    item->setToolTip(texte);
    item->setTextAlignment(centre ? Qt::Alignment(Qt::AlignCenter)
                                  : Qt::Alignment(Qt::AlignVCenter | Qt::AlignLeft));
    return item;
}


// Texte sans accents et en minuscules (comparaison des questions du chatbot)
QString normaliser(const QString &texte)
{
    QString n = texte.toLower().normalized(QString::NormalizationForm_D);
    n.remove(QRegularExpression(QStringLiteral("[\\x{0300}-\\x{036f}]")));
    return n;
}

// ---------------------------------------------------------------------------
// Les graphiques réutilisent les widgets communs du module Employés.

const char *kStyleStats = R"(
QDialog { background-color: #F4F8F7; }
QWidget { font-family: "Arial"; font-size: 11pt; color: #1F2A37; }
#carteStat { background-color: #FFFFFF; border: 1px solid #CBD5D3; border-radius: 12px; }
#titrePage { font-size: 20pt; font-weight: bold; color: #0B4F4A; }
#titreCarte { font-size: 13pt; font-weight: bold; color: #0B4F4A; }
#sousTitreCarte { font-size: 9pt; color: #6B7A78; }
#kpiTitre { font-size: 9pt; color: #6B7A78; }
#kpiValeur { font-size: 20pt; font-weight: bold; color: #0F766E; }
)";

// Carte blanche avec un titre, un sous-titre et le graphique donné.
QFrame *creerCarte(const QString &titre, const QString &sousTitre, QWidget *graphique, QWidget *parent)
{
    auto *carte = new QFrame(parent);
    carte->setObjectName("carteStat");
    auto *lay = new QVBoxLayout(carte);
    lay->setContentsMargins(18, 14, 18, 14);
    lay->setSpacing(4);

    auto *lblTitre = new QLabel(titre, carte);
    lblTitre->setObjectName("titreCarte");
    auto *lblSous = new QLabel(sousTitre, carte);
    lblSous->setObjectName("sousTitreCarte");

    lay->addWidget(lblTitre);
    lay->addWidget(lblSous);
    lay->addWidget(graphique, 1);
    return carte;
}

// Petite carte d'indicateur : un libellé et une grande valeur.
QFrame *creerKpi(const QString &titre, const QString &valeur, QWidget *parent)
{
    auto *carte = new QFrame(parent);
    carte->setObjectName("carteStat");
    auto *lay = new QVBoxLayout(carte);
    lay->setContentsMargins(18, 10, 18, 10);
    lay->setSpacing(0);

    auto *lblTitre = new QLabel(titre, carte);
    lblTitre->setObjectName("kpiTitre");
    auto *lblValeur = new QLabel(valeur, carte);
    lblValeur->setObjectName("kpiValeur");

    lay->addWidget(lblTitre);
    lay->addWidget(lblValeur);
    return carte;
}

} // namespace

// ===========================================================================
// MODULE ANIMAL — intégré au projet principal
// ===========================================================================

// ===========================================================================
// MODULE ANIMAUX — vue (les requêtes Oracle sont dans la classe Animal)
// ===========================================================================
void GSmartPetCare::configurerPageAnimaux()
{
    ui->pageAnimaux->setAttribute(Qt::WA_StyledBackground, true);   // fond de la feuille de style

    // --- Tableau ---
    QTableWidget *t = ui->tableWidgetAnimaux;
    QHeaderView *h = t->horizontalHeader();
    // Colonnes larges et aérées : largeurs de base confortables (voir ajusterColonnesAnimaux),
    // barre de défilement horizontale si la fenêtre est trop étroite plutôt que des colonnes écrasées.
    h->setMinimumSectionSize(70);
    h->setFixedHeight(48);
    h->setDefaultAlignment(Qt::AlignCenter);
    h->setSectionResizeMode(QHeaderView::Interactive);
    h->setStretchLastSection(false);
    t->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    t->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    t->setWordWrap(false);
    t->setTextElideMode(Qt::ElideRight);
    t->verticalHeader()->setDefaultSectionSize(56);
    t->setStyleSheet(QStringLiteral(
        "QHeaderView::section { padding: 8px 16px; }"
        "QTableWidget::item { padding: 6px 14px; }"));
    ajusterColonnesAnimaux();

    // --- Formulaire ---
    ui->dateEditNaissanceAnimal->setDisplayFormat("dd/MM/yyyy");
    ui->dateEditNaissanceAnimal->setMaximumDate(QDate::currentDate());
    ui->comboBoxSuiviAnimal->clear();
    ui->comboBoxSuiviAnimal->addItems({ kAJourAnimal,
                                        QString::fromUtf8("Vaccin à renouveler"),
                                        QString::fromUtf8("Vermifugation à faire"),
                                        QString::fromUtf8("Contrôle à planifier") });

    // --- Connexions ---
    connect(ui->pushButtonEnregistrerAnimal,  &QPushButton::clicked, this, &GSmartPetCare::onEnregistrerAnimal);
    connect(ui->pushButtonAnnulerAnimal,      &QPushButton::clicked, this, &GSmartPetCare::onAnnulerAnimal);
    connect(ui->pushButtonChoisirPhotoAnimal, &QPushButton::clicked, this, &GSmartPetCare::onChoisirPhotoAnimal);
    connect(ui->pushButtonExporterPdfAnimal,  &QPushButton::clicked, this, &GSmartPetCare::onExporterPdfAnimaux);
    connect(ui->pushButtonRappelsSanteAnimal, &QPushButton::clicked, this, &GSmartPetCare::onDossierMedicalAnimal);
    connect(ui->pushButtonStatistiquesAnimal, &QPushButton::clicked, this, &GSmartPetCare::onStatistiquesAnimaux);
    connect(ui->pushButtonChatbotAnimal,      &QPushButton::clicked, this, &GSmartPetCare::onChatbotAnimal);

    connect(t, &QTableWidget::itemSelectionChanged, this, &GSmartPetCare::onSelectionAnimalChangee);
    connect(ui->lineEditRechercheAnimal, &QLineEdit::textChanged, this, &GSmartPetCare::rafraichirTableAnimaux);
    connect(ui->comboBoxTriAnimal,    QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GSmartPetCare::rafraichirTableAnimaux);
    connect(ui->comboBoxFiltreEspece, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GSmartPetCare::rafraichirTableAnimaux);

    nouveauFormulaireAnimal();           // le panneau de droite démarre en mode « Ajouter »
    rafraichirTableAnimaux();
    majBadgeAnimaux();
}

// Largeurs de base (px) de chaque colonne ; l'espace libre éventuel est réparti sur Nom, Propriétaire et Suivi.
void GSmartPetCare::ajusterColonnesAnimaux()
{
    QTableWidget *t = ui->tableWidgetAnimaux;
    QHeaderView *h = t->horizontalHeader();
    int largeur[AnimalNbColonnes];
    largeur[AnimalColId]           = 90;
    largeur[AnimalColNom]          = 170;
    largeur[AnimalColEspece]       = 130;
    largeur[AnimalColSexe]         = 110;
    largeur[AnimalColNaissance]    = 160;
    largeur[AnimalColProprietaire] = 190;
    largeur[AnimalColTelephone]    = 160;
    largeur[AnimalColSuivi]        = 200;
    largeur[AnimalColActions]      = 230;

    int total = 0;
    for (int c = 0; c < AnimalNbColonnes; ++c)
        total += largeur[c];
    const int libre = t->viewport()->width() - total;
    if (libre > 0) {
        largeur[AnimalColNom]          += libre / 3;
        largeur[AnimalColProprietaire] += libre / 3;
        largeur[AnimalColSuivi]        += libre - 2 * (libre / 3);
    }
    for (int c = 0; c < AnimalNbColonnes; ++c)
        h->resizeSection(c, largeur[c]);
}

void GSmartPetCare::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (ui && ui->tableWidgetAnimaux)
        ajusterColonnesAnimaux();
}

QString GSmartPetCare::formaterIdAnimal(int id)
{
    return QString("A%1").arg(id, 3, 10, QChar('0'));     // A001, A002, ...
}

int GSmartPetCare::idAnimalSelectionne() const
{
    const QList<QTableWidgetItem *> sel = ui->tableWidgetAnimaux->selectedItems();
    if (sel.isEmpty())
        return -1;
    const QTableWidgetItem *item = ui->tableWidgetAnimaux->item(sel.first()->row(), AnimalColId);
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

void GSmartPetCare::majBadgeAnimaux()
{
    QVector<Animal> tous;
    if (Animal::afficher(tous))
        ui->badgeAnimaux->setText(QString::number(tous.size()));
}

// ---------------------------------------------------------------------------
// Tableau : recherche + filtre par espèce + tri (exécutés par Oracle via Animal::rechercher)
// ---------------------------------------------------------------------------
void GSmartPetCare::rafraichirTableAnimaux()
{
    QString erreur;
    const QString espece = ui->comboBoxFiltreEspece->currentIndex() > 0
        ? ui->comboBoxFiltreEspece->currentText() : QString();
    if (!Animal::rechercher(m_vueAnimaux, ui->lineEditRechercheAnimal->text(),
                            espece, ui->comboBoxTriAnimal->currentIndex(), &erreur)) {
        QMessageBox::warning(this, "Oracle - Recherche animaux", erreur);
        m_vueAnimaux.clear();
    }

    QTableWidget *t = ui->tableWidgetAnimaux;
    {
        const QSignalBlocker blocker(t);       // pas de rechargement du formulaire pendant le remplissage
        t->clearContents();
        t->setRowCount(m_vueAnimaux.size());

        int ligneAGarder = -1;
        for (int r = 0; r < m_vueAnimaux.size(); ++r) {
            const Animal &a = m_vueAnimaux[r];

            QTableWidgetItem *id = celluleAnimal(formaterIdAnimal(a.id));
            id->setData(Qt::UserRole, a.id);                 // l'id numérique voyage avec la ligne
            t->setItem(r, AnimalColId, id);
            t->setItem(r, AnimalColNom, celluleAnimal(a.nom, false));
            t->setItem(r, AnimalColEspece, celluleAnimal(a.espece));
            t->setItem(r, AnimalColSexe, celluleAnimal(a.sexe));
            t->setItem(r, AnimalColNaissance, celluleAnimal(a.naissance.toString("dd/MM/yyyy")));
            t->setItem(r, AnimalColProprietaire, celluleAnimal(a.proprietaire, false));
            t->setItem(r, AnimalColTelephone, celluleAnimal(a.telephone));

            QTableWidgetItem *suivi = celluleAnimal(a.suiviSante);
            const bool aJour = a.suiviSante.compare(kAJourAnimal, Qt::CaseInsensitive) == 0;
            suivi->setForeground(aJour ? QColor("#0F766E") : QColor("#B45309"));
            t->setItem(r, AnimalColSuivi, suivi);

            // Boutons « Modifier » / « Supprimer » de la ligne
            auto *actions = new QWidget(t);
            auto *buttons = new QHBoxLayout(actions);
            buttons->setContentsMargins(8, 6, 8, 6);
            buttons->setSpacing(10);
            auto *modifier  = new QPushButton("Modifier", actions);
            auto *supprimer = new QPushButton("Supprimer", actions);
            modifier->setStyleSheet("QPushButton{background:#2471A3;color:white;border:0;border-radius:5px;padding:6px 12px;} QPushButton:hover{background:#1A5276;}");
            supprimer->setStyleSheet("QPushButton{background:#C0392B;color:white;border:0;border-radius:5px;padding:6px 12px;} QPushButton:hover{background:#A5311F;}");
            buttons->addWidget(modifier);
            buttons->addWidget(supprimer);
            const int idAction = a.id;
            connect(modifier,  &QPushButton::clicked, this, [this, idAction]() { onModifierAnimal(idAction); });
            connect(supprimer, &QPushButton::clicked, this, [this, idAction]() { onSupprimerAnimal(idAction); });
            t->setCellWidget(r, AnimalColActions, actions);

            if (a.id == m_idEnModification)
                ligneAGarder = r;
        }
        if (ligneAGarder >= 0)
            t->selectRow(ligneAGarder);                       // la fiche en cours reste sélectionnée
    }
}

// ---------------------------------------------------------------------------
// Formulaire
// ---------------------------------------------------------------------------
void GSmartPetCare::definirModeAnimal(bool modification)
{
    ui->labelDetailTitreAnimal->setText(modification ? QString::fromUtf8("Modifier l'animal")
                                                     : QString::fromUtf8("Ajouter un animal"));
    ui->pushButtonEnregistrerAnimal->setText(modification ? QString::fromUtf8("Mettre à jour")
                                                          : QString::fromUtf8("Enregistrer"));
}

void GSmartPetCare::nouveauFormulaireAnimal()
{
    m_idEnModification = -1;

    ui->lineEditIdAnimal->setText(formaterIdAnimal(Animal::prochainId()));
    ui->lineEditNomAnimal->clear();
    ui->comboBoxEspeceAnimal->setCurrentIndex(0);
    ui->comboBoxSexeAnimal->setCurrentIndex(0);
    ui->dateEditNaissanceAnimal->setDate(QDate::currentDate());
    ui->lineEditPhotoAnimal->clear();
    ui->lineEditProprietaireAnimal->clear();
    ui->lineEditTelephoneProprietaireAnimal->clear();
    ui->lineEditEmailProprietaireAnimal->clear();
    ui->comboBoxSuiviAnimal->setCurrentIndex(0);

    {
        const QSignalBlocker blocker(ui->tableWidgetAnimaux);
        ui->tableWidgetAnimaux->clearSelection();
    }
    definirModeAnimal(false);
}

void GSmartPetCare::remplirFormulaireAnimal(const Animal &a)
{
    ui->lineEditIdAnimal->setText(formaterIdAnimal(a.id));
    ui->lineEditNomAnimal->setText(a.nom);
    selectionnerComboAnimal(ui->comboBoxEspeceAnimal, a.espece);
    selectionnerComboAnimal(ui->comboBoxSexeAnimal, a.sexe);
    ui->dateEditNaissanceAnimal->setDate(a.naissance.isValid() ? a.naissance : QDate::currentDate());
    ui->lineEditPhotoAnimal->setText(a.photo);
    ui->lineEditProprietaireAnimal->setText(a.proprietaire);
    ui->lineEditTelephoneProprietaireAnimal->setText(a.telephone);
    ui->lineEditEmailProprietaireAnimal->setText(a.email);
    selectionnerComboAnimal(ui->comboBoxSuiviAnimal, a.suiviSante);
}

Animal GSmartPetCare::lireFormulaireAnimal() const
{
    Animal a;
    a.nom          = ui->lineEditNomAnimal->text().trimmed();
    a.espece       = ui->comboBoxEspeceAnimal->currentText();
    a.sexe         = ui->comboBoxSexeAnimal->currentText();
    a.naissance    = ui->dateEditNaissanceAnimal->date();
    a.photo        = ui->lineEditPhotoAnimal->text().trimmed();
    a.proprietaire = ui->lineEditProprietaireAnimal->text().trimmed();
    a.telephone    = ui->lineEditTelephoneProprietaireAnimal->text().trimmed();
    a.email        = ui->lineEditEmailProprietaireAnimal->text().trimmed();
    a.suiviSante   = ui->comboBoxSuiviAnimal->currentText();
    return a;
}

bool GSmartPetCare::validerFormulaireAnimal(QString *erreur) const
{
    auto echec = [erreur](const QString &message) {
        if (erreur)
            *erreur = message;
        return false;
    };

    if (ui->lineEditNomAnimal->text().trimmed().isEmpty())
        return echec(QString::fromUtf8("Le nom de l'animal est obligatoire."));
    if (ui->lineEditProprietaireAnimal->text().trimmed().isEmpty())
        return echec(QString::fromUtf8("Le nom du propriétaire est obligatoire."));

    const QString tel = ui->lineEditTelephoneProprietaireAnimal->text().trimmed();
    if (!tel.isEmpty()) {
        QString chiffres = tel;
        chiffres.remove(QRegularExpression(QStringLiteral("[^0-9]")));
        const bool caracteresOk = !tel.contains(QRegularExpression(QStringLiteral("[^0-9 +()\\-.]")));
        if (!caracteresOk || chiffres.size() < 8)
            return echec(QString::fromUtf8("Le téléphone du propriétaire n'est pas valide (au moins 8 chiffres)."));
    }

    const QString email = ui->lineEditEmailProprietaireAnimal->text().trimmed();
    if (!email.isEmpty()
        && !QRegularExpression(QStringLiteral("^[^@\\s]+@[^@\\s]+\\.[^@\\s]+$")).match(email).hasMatch())
        return echec(QString::fromUtf8("L'email du propriétaire n'est pas valide."));

    return true;
}

// ---------------------------------------------------------------------------
// Actions
// ---------------------------------------------------------------------------
void GSmartPetCare::onSelectionAnimalChangee()
{
    const int id = idAnimalSelectionne();
    if (id < 0)
        return;                                   // rien de sélectionné : on ne touche pas au formulaire
    for (const Animal &a : std::as_const(m_vueAnimaux)) {
        if (a.id == id) {
            m_idEnModification = id;
            remplirFormulaireAnimal(a);
            definirModeAnimal(true);
            return;
        }
    }
}

// Bouton « Modifier » d'une ligne : charge la fiche dans le formulaire ; l'enregistrement se fait
// ensuite avec « Mettre à jour ».
void GSmartPetCare::onModifierAnimal(int id)
{
    for (int i = 0; i < ui->tableWidgetAnimaux->rowCount(); ++i) {
        const QTableWidgetItem *cell = ui->tableWidgetAnimaux->item(i, AnimalColId);
        if (cell && cell->data(Qt::UserRole).toInt() == id) {
            ui->tableWidgetAnimaux->selectRow(i);
            ui->lineEditNomAnimal->setFocus();
            return;
        }
    }
}

void GSmartPetCare::onEnregistrerAnimal()
{
    QString erreur;
    if (!validerFormulaireAnimal(&erreur)) {
        QMessageBox::warning(this, "Formulaire incomplet", erreur);
        return;
    }

    Animal saisie = lireFormulaireAnimal();
    QString erreurOracle;

    if (m_idEnModification == -1) {
        if (!saisie.ajouter(&erreurOracle)) {
            QMessageBox::critical(this, "Ajout Oracle impossible", erreurOracle);
            return;
        }
        QMessageBox::information(this, "Animal ajouté", "Animal enregistré dans Oracle.");
    } else {
        saisie.id = m_idEnModification;
        if (!saisie.modifier(&erreurOracle)) {
            QMessageBox::critical(this, "Modification Oracle impossible", erreurOracle);
            return;
        }
        QMessageBox::information(this, "Animal modifié", "Modification enregistrée dans Oracle.");
    }

    nouveauFormulaireAnimal();
    rafraichirTableAnimaux();
    majBadgeAnimaux();
}

void GSmartPetCare::onSupprimerAnimal(int id)
{
    QString nom = QString::number(id);
    for (const Animal &a : std::as_const(m_vueAnimaux))
        if (a.id == id) { nom = a.nom; break; }

    const auto reponse = QMessageBox::question(
        this, "Confirmer la suppression",
        QString::fromUtf8("Voulez-vous vraiment supprimer %1 ?").arg(nom),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reponse != QMessageBox::Yes)
        return;

    QString erreurOracle;
    if (!Animal::supprimer(id, &erreurOracle)) {
        QMessageBox::critical(this, "Suppression Oracle impossible", erreurOracle);
        return;
    }

    nouveauFormulaireAnimal();
    rafraichirTableAnimaux();
    majBadgeAnimaux();
}

// « Annuler » (bouton rouge) : abandonne la saisie ou la modification.
void GSmartPetCare::onAnnulerAnimal()
{
    nouveauFormulaireAnimal();
}

void GSmartPetCare::onChoisirPhotoAnimal()
{
    const QString chemin = QFileDialog::getOpenFileName(
        this, "Choisir une photo", QString(), "Images (*.png *.jpg *.jpeg)");
    if (!chemin.isEmpty())
        ui->lineEditPhotoAnimal->setText(chemin);
}

// ---------------------------------------------------------------------------
// Statistiques : 4 indicateurs + cercles (espèce, sexe) + histogramme (âge) + courbe (naissances)
// ---------------------------------------------------------------------------
void GSmartPetCare::onStatistiquesAnimaux()
{
    QVector<Animal> animaux;
    QString erreur;
    if (!Animal::afficher(animaux, &erreur)) {          // requête Oracle (animal.cpp)
        QMessageBox::warning(this, "Oracle - Statistiques", erreur);
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle("Statistiques des animaux");
    dlg.setMinimumSize(1100, 700);
    dlg.resize(1200, 760);
    dlg.setStyleSheet(kStyleStats);

    // ----- Calculs -----
    QMap<QString, int> parEspece;
    QMap<QString, int> parSexe;
    QVector<double> parMois(12, 0.0);
    int tranches[4] = { 0, 0, 0, 0 };                 // < 1 an, 1-3 ans, 4-7 ans, 8 ans et +
    int suivisAFaire = 0;
    const QDate aujourdhui = QDate::currentDate();

    for (const Animal &a : std::as_const(animaux)) {
        if (!a.espece.isEmpty()) parEspece[a.espece]++;
        if (!a.sexe.isEmpty())   parSexe[a.sexe]++;
        if (a.suiviSante.compare(kAJourAnimal, Qt::CaseInsensitive) != 0)
            suivisAFaire++;
        if (a.naissance.isValid()) {
            parMois[a.naissance.month() - 1] += 1.0;
            const int age = int(a.naissance.daysTo(aujourdhui) / 365);
            if (age < 1)       tranches[0]++;
            else if (age <= 3) tranches[1]++;
            else if (age <= 7) tranches[2]++;
            else               tranches[3]++;
        }
    }

    QString especeDominante = "-";
    int max = 0;
    for (auto it = parEspece.constBegin(); it != parEspece.constEnd(); ++it) {
        if (it.value() > max) {
            max = it.value();
            especeDominante = it.key();
        }
    }

    // ----- Interface -----
    auto *racine = new QVBoxLayout(&dlg);
    racine->setContentsMargins(28, 24, 28, 24);
    racine->setSpacing(14);

    auto *titre = new QLabel("Statistiques des animaux", &dlg);
    titre->setObjectName("titrePage");
    racine->addWidget(titre);

    auto *kpis = new QHBoxLayout;
    kpis->setSpacing(14);
    kpis->addWidget(creerKpi("Total animaux", QString::number(animaux.size()), &dlg));
    kpis->addWidget(creerKpi(QString::fromUtf8("Espèces"), QString::number(parEspece.size()), &dlg));
    kpis->addWidget(creerKpi(QString::fromUtf8("Espèce dominante"), especeDominante, &dlg));
    kpis->addWidget(creerKpi(QString::fromUtf8("Suivis santé à faire"), QString::number(suivisAFaire), &dlg));
    racine->addLayout(kpis);

    auto *donutEspece = new DonutChart(&dlg);
    donutEspece->setUnite("animaux");
    donutEspece->setData(parEspece);

    auto *donutSexe = new DonutChart(&dlg);
    donutSexe->setUnite("animaux");
    donutSexe->setData(parSexe);

    auto *histoAge = new BarChart(&dlg);
    QVector<QPair<QString, double>> ages;
    ages.append({ QString::fromUtf8("< 1 an"),     double(tranches[0]) });
    ages.append({ QString::fromUtf8("1-3 ans"),    double(tranches[1]) });
    ages.append({ QString::fromUtf8("4-7 ans"),    double(tranches[2]) });
    ages.append({ QString::fromUtf8("8 ans et +"), double(tranches[3]) });
    histoAge->setData(ages, BarChart::Vertical, QString(), QColor("#F28C28"));

    auto *courbe = new LineChart(&dlg);
    const QStringList mois = { "Jan", QString::fromUtf8("Fév"), "Mar", "Avr", "Mai", "Juin",
                               "Juil", QString::fromUtf8("Août"), "Sep", "Oct", "Nov", QString::fromUtf8("Déc") };
    QVector<QPair<QString, double>> naissances;
    for (int i = 0; i < 12; ++i)
        naissances.append({ mois[i], parMois[i] });
    courbe->setData(naissances);

    auto *grille = new QGridLayout;
    grille->setHorizontalSpacing(18);
    grille->setVerticalSpacing(18);
    grille->addWidget(creerCarte(QString::fromUtf8("Animaux par espèce"),
                                 QString::fromUtf8("Répartition du nombre d'animaux selon leur espèce"),
                                 donutEspece, &dlg), 0, 0);
    grille->addWidget(creerCarte(QString::fromUtf8("Répartition par sexe"),
                                 QString::fromUtf8("Mâles et femelles suivis par la clinique"),
                                 donutSexe, &dlg), 0, 1);
    grille->addWidget(creerCarte(QString::fromUtf8("Tranches d'âge"),
                                 QString::fromUtf8("Nombre d'animaux par tranche d'âge"),
                                 histoAge, &dlg), 1, 0);
    grille->addWidget(creerCarte("Naissances par mois",
                                 QString::fromUtf8("Mois de naissance des animaux enregistrés"),
                                 courbe, &dlg), 1, 1);
    grille->setColumnStretch(0, 1);
    grille->setColumnStretch(1, 1);
    grille->setRowStretch(0, 1);
    grille->setRowStretch(1, 1);
    racine->addLayout(grille, 1);

    dlg.exec();
}

// ---------------------------------------------------------------------------
// Chatbot : questions sur les animaux et leur dossier médical (rendez-vous et services : non inclus)
// ---------------------------------------------------------------------------
void GSmartPetCare::onChatbotAnimal()
{
    const int idChoisi = idAnimalSelectionne();

    auto *dlg = new QDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(QString::fromUtf8("PetNova — Assistant animaux"));
    dlg->resize(780, 640);
    dlg->setStyleSheet("QDialog{background:#F4F8F7;} QTextEdit{background:white;border:1px solid #D7E3E1;"
                       "border-radius:8px;padding:8px;} QPushButton{background:#0F766E;color:white;"
                       "border-radius:7px;padding:9px;} QLabel{color:#1F2A37;}");
    auto *layout = new QVBoxLayout(dlg);
    auto *titre = new QLabel(QString::fromUtf8("Assistant PetNova — animaux (données Oracle)"), dlg);
    QFont f = titre->font(); f.setPointSize(16); f.setBold(true); titre->setFont(f);
    layout->addWidget(titre);

    auto *mode = new QComboBox(dlg);
    mode->addItem(QString::fromUtf8("Tous les animaux / Mode global"), -1);
    if (idChoisi >= 0)
        mode->addItem(QString::fromUtf8("Animal sélectionné (ID %1)").arg(idChoisi), idChoisi);
    layout->addWidget(mode);

    auto *conversation = new QTextEdit(dlg);
    conversation->setReadOnly(true);
    conversation->setPlainText(QString::fromUtf8(
        "PetNova : Bonjour. Je peux donner le nombre d'animaux, leur liste, la fiche d'un animal et son "
        "dossier médical (allergies, traitements, vaccination, poids, antécédents...).\n"
        "Pour un animal précis, indiquez son ID (ex. : animal 12) ou son nom exact.\n"
        "Les rendez-vous et les services ne sont pas inclus dans cette version."));
    layout->addWidget(conversation, 1);

    auto *saisie = new QTextEdit(dlg);
    saisie->setMaximumHeight(85);
    saisie->setPlaceholderText(QString::fromUtf8("Ex. Combien d'animaux ? / Allergies animal 12 / Dossier médical de Max"));
    layout->addWidget(saisie);

    auto *actions = new QHBoxLayout;
    auto *fermer = new QPushButton("Fermer", dlg);
    auto *envoyer = new QPushButton("Interroger Oracle", dlg);
    actions->addWidget(fermer); actions->addStretch(); actions->addWidget(envoyer);
    layout->addLayout(actions);

    connect(fermer, &QPushButton::clicked, dlg, &QDialog::close);
    connect(envoyer, &QPushButton::clicked, dlg, [this, mode, conversation, saisie]() {
        const QString question = saisie->toPlainText().trimmed();
        if (question.isEmpty())
            return;
        conversation->append(QString::fromUtf8("\nVous : %1").arg(question));
        conversation->append(QString::fromUtf8("PetNova : %1\n").arg(reponseChatbotAnimal(question, mode->currentData().toInt())));
        saisie->clear();
    });
    dlg->show();
}

// idMode : -1 = mode global, sinon id de l'animal sélectionné. Toutes les données viennent de la classe Animal.
QString GSmartPetCare::reponseChatbotAnimal(const QString &question, int idMode) const
{
    const QString q = normaliser(question);
    auto contient = [&q](std::initializer_list<const char *> mots) {
        for (const char *m : mots)
            if (q.contains(QLatin1String(m)))
                return true;
        return false;
    };
    const bool compte  = contient({"combien", "nombre", "total"});
    const bool liste   = contient({"liste", "tous les animaux", "afficher les animaux"});
    const bool medical = contient({"antecedent", "allerg", "medical", "sante", "traitement", "vaccin",
                                   "poids", "maladie", "chronique", "derniere consultation"});
    const bool autreModule = !medical && contient({"rendez", "rdv", "consultation", "service", "prestation"});
    QString erreur;

    if (autreModule)
        return QString::fromUtf8("Les rendez-vous, consultations et services ne sont pas inclus dans cette version "
                                 "(module Animaux seulement).");

    // --- Identification de l'animal : mode sélectionné, sinon « animal 12 » / « id 12 », sinon nom exact ---
    int id = idMode;
    if (id < 0) {
        static const QRegularExpression reId(QString::fromUtf8(
            R"((?:\b(?:animal\s*(?:id|n[°o.]|numero)?|id)\s*[:#-]?\s*|#)(\d+))"));
        const QRegularExpressionMatch m = reId.match(q);
        if (m.hasMatch()) {
            id = m.captured(1).toInt();
        } else {
            QVector<Animal> tous;
            if (!Animal::afficher(tous, &erreur))
                return QString::fromUtf8("Erreur Oracle : %1").arg(erreur);
            QVector<Animal> trouves;
            for (const Animal &a : std::as_const(tous)) {
                const QString nom = normaliser(a.nom).trimmed();
                if (nom.isEmpty())
                    continue;
                const QRegularExpression mot("(?<![a-z0-9])" + QRegularExpression::escape(nom) + "(?![a-z0-9])");
                if (mot.match(q).hasMatch())
                    trouves.append(a);
            }
            if (trouves.size() == 1) {
                id = trouves.first().id;
            } else if (trouves.size() > 1) {
                QStringList options;
                for (const Animal &a : std::as_const(trouves))
                    options << QString("%1 (ID %2)").arg(a.nom).arg(a.id);
                return QString::fromUtf8("Plusieurs animaux correspondent : %1. Précisez l'ID.").arg(options.join(", "));
            }
        }
    }

    // --- Dossier médical d'un animal (champ précisé, sinon dossier complet) ---
    if (id >= 0 && medical) {
        Animal a;
        if (!Animal::obtenirMedical(id, a, &erreur))
            return QString::fromUtf8("Impossible de charger le dossier médical : %1").arg(erreur);
        auto valeur = [](const QString &v) { return v.trimmed().isEmpty() ? QString::fromUtf8("Non renseigné") : v; };
        QString vaccin = QString::fromUtf8("Inconnu");
        if (a.statutVaccinal == "A_JOUR") vaccin = QString::fromUtf8("À jour");
        else if (a.statutVaccinal == "INCOMPLET") vaccin = "Incomplet";

        const QVector<QPair<QStringList, QString>> champs = {
            { {"antecedent"},            QString::fromUtf8("Antécédents : ") + valeur(a.antecedentsMedicaux) },
            { {"maladie", "chronique"},  QString::fromUtf8("Maladies chroniques : ") + valeur(a.maladiesChroniques) },
            { {"allerg"},                QString::fromUtf8("Allergies : ") + valeur(a.allergies) },
            { {"traitement"},            QString::fromUtf8("Traitements en cours : ") + valeur(a.traitementsEnCours) },
            { {"vaccin"},                QString::fromUtf8("Vaccination : ") + vaccin },
            { {"poids"},                 QString::fromUtf8("Poids : ") + (a.poids > 0 ? QString::number(a.poids, 'f', 2) + " kg"
                                                                                       : QString::fromUtf8("Non renseigné")) },
            { {"derniere consultation"}, QString::fromUtf8("Dernière consultation : ")
                                         + (a.derniereConsultation.isValid() ? a.derniereConsultation.toString("dd/MM/yyyy")
                                                                             : QString::fromUtf8("Non renseignée")) },
            { {"suivi"},                 QString::fromUtf8("Suivi santé : ") + valeur(a.suiviSante) },
        };
        QStringList lignes;
        for (const auto &c : champs)
            for (const QString &mot : c.first)
                if (q.contains(mot)) { lignes << c.second; break; }
        if (lignes.isEmpty())                       // question générale : dossier complet
            for (const auto &c : champs)
                lignes << c.second;
        return QString::fromUtf8("Animal : %1 (ID %2)\n").arg(a.nom).arg(a.id) + lignes.join("\n");
    }

    // --- Nombre d'animaux (avec répartition par espèce) ---
    if (compte && contient({"animal", "animaux"})) {
        int total = 0;
        Animal::Comptage parEspece;
        if (!Animal::compter(total, &erreur) || !Animal::repartitionParEspece(parEspece, &erreur))
            return QString::fromUtf8("Erreur Oracle : %1").arg(erreur);
        QStringList details;
        for (const auto &e : std::as_const(parEspece))
            details << QString("%1 : %2").arg(e.first).arg(e.second);
        return QString::fromUtf8("Nombre d'animaux enregistrés : %1.").arg(total)
               + (details.isEmpty() ? QString() : "\n" + details.join("\n"));
    }

    // --- Liste des animaux ---
    if (liste && id < 0) {
        QVector<Animal> tous;
        if (!Animal::afficher(tous, &erreur))
            return QString::fromUtf8("Erreur Oracle : %1").arg(erreur);
        if (tous.isEmpty())
            return QString::fromUtf8("Aucun animal enregistré.");
        QStringList lignes;
        for (int i = 0; i < tous.size() && i < 20; ++i)
            lignes << QString::fromUtf8("%1 — %2 — %3 — %4").arg(formaterIdAnimal(tous[i].id), tous[i].nom,
                                                                tous[i].espece, tous[i].proprietaire);
        if (tous.size() > 20)
            lignes << QString::fromUtf8("... et %1 autres animaux.").arg(tous.size() - 20);
        return lignes.join("\n");
    }

    // --- Fiche d'un animal ---
    if (id >= 0) {
        Animal a;
        if (!Animal::obtenir(id, a, &erreur))
            return QString::fromUtf8("Erreur Oracle : %1").arg(erreur);
        return QString::fromUtf8("Animal : %1 (ID %2)\nEspèce : %3\nSexe : %4\nNaissance : %5\n"
                                 "Propriétaire : %6\nTéléphone : %7\nSuivi santé : %8")
            .arg(a.nom).arg(a.id).arg(a.espece, a.sexe, a.naissance.toString("dd/MM/yyyy"),
                                      a.proprietaire, a.telephone, a.suiviSante);
    }

    if (medical)
        return QString::fromUtf8("Indiquez le nom exact ou l'ID de l'animal, ou sélectionnez le mode Animal sélectionné.");
    return QString::fromUtf8("Question non reconnue. Essayez : « Combien d'animaux ? », « Liste des animaux », "
                             "« Animal 12 », « Allergies animal 12 » ou « Dossier médical de Max ».");
}

// ---------------------------------------------------------------------------
// Dossier médical (7 listes déroulantes) : page enfant de pageAnimaux
// ---------------------------------------------------------------------------
void GSmartPetCare::onDossierMedicalAnimal()
{
    const int id = idAnimalSelectionne();
    if (id < 0) {
        QMessageBox::information(this, "Prédiction santé", "Sélectionnez d'abord un animal dans le tableau.");
        return;
    }
    Animal a;
    QString erreur;
    if (!Animal::obtenirMedical(id, a, &erreur)) {
        QMessageBox::warning(this, "Dossier médical",
            "Impossible de charger le dossier médical local.\n" + erreur);
        return;
    }

    // Widget enfant de pageAnimaux : aucune nouvelle fenêtre et aucun deuxième .ui.
    auto *page = new QFrame(ui->pageAnimaux);
    page->setObjectName("dossierMedicalAnimaux");
    page->setGeometry(ui->pageAnimaux->rect());
    page->setStyleSheet("QFrame#dossierMedicalAnimaux{background:#F4F8F7;}"
                        "QLabel{color:#1F2A37;} QTextEdit,QDoubleSpinBox,QDateEdit{"
                        "background:white;border:1px solid #C8DAD7;border-radius:6px;padding:5px;}"
                        "QComboBox{background:#FFFFFF;border:1px solid #A7C8C3;border-radius:7px;"
                        "padding:5px 32px 5px 12px;min-height:32px;max-height:32px;color:#173A36;}"
                        "QComboBox::drop-down{border-left:1px solid #C8DAD7;width:30px;"
                        "background:#DDEFEA;border-top-right-radius:6px;border-bottom-right-radius:6px;}"
                        "QComboBox::down-arrow{image:none;border-left:5px solid transparent;"
                        "border-right:5px solid transparent;border-top:6px solid #0F766E;"
                        "width:0px;height:0px;}"
                        "QComboBox QAbstractItemView{background:white;selection-background-color:#DDEFEA;"
                        "selection-color:#173A36;outline:0px;}"
                        "QPushButton{background:#0F766E;color:white;border-radius:7px;padding:9px;font-weight:bold;}"
                        "QPushButton:hover{background:#0B5F59;}");
    auto *outer = new QVBoxLayout(page);
    auto *header = new QHBoxLayout;
    auto *title = new QLabel(QString::fromUtf8("DOSSIER MÉDICAL — 7 LISTES DÉROULANTES (S7) — %1 (#%2)").arg(a.nom).arg(a.id), page);
    QFont titleFont = title->font(); titleFont.setPointSize(16); titleFont.setBold(true); title->setFont(titleFont);
    auto *retour = new QPushButton("Retour aux animaux", page);
    header->addWidget(title, 1); header->addWidget(retour);
    outer->addLayout(header);

    auto *scroll = new QScrollArea(page); scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
    auto *content = new QWidget(scroll); auto *form = new QFormLayout(content);
    form->setSpacing(12);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setFormAlignment(Qt::AlignTop);

    // Tous les champs du formulaire sont des QComboBox.
    auto medicalCombo = [&](const QString &label, const QString &value, const QStringList &choices) {
        auto *w = new QComboBox(content);
        w->setEditable(false);
        w->setFixedHeight(42);
        w->setMinimumWidth(340);
        w->setStyleSheet("QComboBox { background: #ffffff; color: #183333; border: 1px solid #a9c8c6; border-radius: 5px; padding: 7px 12px; } "
                         "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 34px; border-left: 1px solid #a9c8c6; background: #e4f1ef; } "
                         "QComboBox::down-arrow { image: none; border-left: 5px solid transparent; border-right: 5px solid transparent; border-top: 7px solid #075a55; width: 0; height: 0; } "
                         "QComboBox QAbstractItemView { background: white; color: #183333; selection-background-color: #b9dfdb; }");
        w->addItem("Non renseigné");
        for (const QString &choice : choices) w->addItem(choice);
        if (!value.trimmed().isEmpty() && w->findText(value) < 0) w->addItem(value);
        w->setCurrentText(value.trimmed().isEmpty() ? "Non renseigné" : value);
        w->setInsertPolicy(QComboBox::NoInsert);
        form->addRow(label, w);
        return w;
    };
    auto *ante = medicalCombo("Antécédents médicaux", a.antecedentsMedicaux,
        {"Aucun", "Opération chirurgicale", "Fracture / blessure", "Infection antérieure",
         "Trouble digestif", "Maladie respiratoire", "Autre (à préciser)"});
    auto *chron = medicalCombo("Maladies chroniques", a.maladiesChroniques,
        {"Aucune", "Diabète", "Maladie cardiaque", "Maladie rénale",
         "Arthrose", "Épilepsie", "Asthme", "Autre (à préciser)"});
    auto *allerg = medicalCombo("Allergies connues", a.allergies,
        {"Aucune connue", "Allergie alimentaire", "Allergie médicamenteuse",
         "Allergie environnementale", "Autre (à préciser)"});
    auto *trait = medicalCombo("Traitements en cours", a.traitementsEnCours,
        {"Aucun", "Traitement antiparasitaire", "Traitement dermatologique",
         "Traitement chronique prescrit", "Autre traitement (à préciser)"});

    auto *poids = new QComboBox(content);
    poids->setEditable(true);
    poids->setFixedHeight(42);
    poids->setInsertPolicy(QComboBox::NoInsert);
    poids->addItem("Non renseigné", 0.0);
    for (int i = 1; i <= 100; i++) poids->addItem(QString::number(i) + " kg", double(i));
    poids->setCurrentText(a.poids > 0 ? QString::number(a.poids, 'f', 2) + " kg" : "Non renseigné");
    form->addRow("Poids (kg)", poids);

    auto *vacc = new QComboBox(content);
    vacc->setFixedHeight(42);
    vacc->addItem("Inconnu", "INCONNU"); vacc->addItem("À jour", "A_JOUR"); vacc->addItem("Incomplet", "INCOMPLET");
    const int vi = vacc->findData(a.statutVaccinal); vacc->setCurrentIndex(vi < 0 ? 0 : vi);
    form->addRow("Statut vaccinal", vacc);

    auto *consult = new QComboBox(content);
    consult->setEditable(true);
    consult->setFixedHeight(42);
    consult->setInsertPolicy(QComboBox::NoInsert);
    consult->addItem("Non renseignée");
    consult->addItem("Aujourd'hui");
    consult->addItem("Il y a 1 mois");
    consult->addItem("Il y a 6 mois");
    consult->addItem("Il y a 1 an");
    consult->setCurrentText(a.derniereConsultation.isValid()
        ? a.derniereConsultation.toString("dd/MM/yyyy") : "Non renseignée");
    form->addRow("Dernière consultation", consult);

    scroll->setWidget(content); outer->addWidget(scroll, 1);
    auto *resultat = new QTextEdit(page); resultat->setReadOnly(true); resultat->setMaximumHeight(170);
    resultat->setPlaceholderText("L'analyse des risques apparaîtra ici."); outer->addWidget(resultat);
    auto *actions = new QHBoxLayout; auto *save = new QPushButton("Enregistrer le dossier médical", page);
    auto *analyser = new QPushButton("Analyser les risques", page);
    actions->addWidget(save); actions->addWidget(analyser); outer->addLayout(actions);

    // Lecture des 7 champs vers un Animal (retourne false si une valeur est invalide)
    auto lire = [=](Animal &b) -> bool {
        b.antecedentsMedicaux = (ante->currentIndex() == 0 ? QString() : ante->currentText().trimmed());
        b.maladiesChroniques  = (chron->currentIndex() == 0 ? QString() : chron->currentText().trimmed());
        b.allergies           = (allerg->currentIndex() == 0 ? QString() : allerg->currentText().trimmed());
        b.traitementsEnCours  = (trait->currentIndex() == 0 ? QString() : trait->currentText().trimmed());
        QString poidsTexte = poids->currentText().trimmed();
        if (poidsTexte == "Non renseigné" || poidsTexte.isEmpty()) b.poids = 0;
        else {
            poidsTexte.remove("kg", Qt::CaseInsensitive);
            poidsTexte = poidsTexte.trimmed().replace(',', '.');
            bool ok = false; const double p = poidsTexte.toDouble(&ok);
            if (!ok || p <= 0 || p > 9999.99) {
                QMessageBox::warning(page, "Validation", "Poids invalide. Indiquez une valeur positive en kg.");
                return false;
            }
            b.poids = p;
        }
        b.statutVaccinal = vacc->currentData().toString();
        const QString dateTexte = consult->currentText().trimmed();
        if (dateTexte.isEmpty() || dateTexte == "Non renseignée") b.derniereConsultation = QDate();
        else if (dateTexte == "Aujourd'hui") b.derniereConsultation = QDate::currentDate();
        else if (dateTexte == "Il y a 1 mois") b.derniereConsultation = QDate::currentDate().addMonths(-1);
        else if (dateTexte == "Il y a 6 mois") b.derniereConsultation = QDate::currentDate().addMonths(-6);
        else if (dateTexte == "Il y a 1 an") b.derniereConsultation = QDate::currentDate().addYears(-1);
        else {
            b.derniereConsultation = QDate::fromString(dateTexte, "dd/MM/yyyy");
            if (!b.derniereConsultation.isValid())
                b.derniereConsultation = QDate::fromString(dateTexte, "yyyy-MM-dd");
            if (!b.derniereConsultation.isValid()) {
                QMessageBox::warning(page, "Validation", "Date invalide. Utilisez JJ/MM/AAAA.");
                return false;
            }
        }
        if (b.derniereConsultation.isValid() && b.derniereConsultation > QDate::currentDate()) {
            QMessageBox::warning(page, "Validation", "La dernière consultation ne peut pas être future.");
            return false;
        }
        return true;
    };

    connect(save, &QPushButton::clicked, page, [=]() {
        Animal b = a; if (!lire(b)) return;
        if (b.antecedentsMedicaux.size() > 1000 || b.maladiesChroniques.size() > 500 ||
            b.allergies.size() > 500 || b.traitementsEnCours.size() > 1000) {
            QMessageBox::warning(page, "Validation", "Un champ médical dépasse la longueur autorisée."); return;
        }
        QString e;
        if (!Animal::enregistrerMedical(b, &e)) QMessageBox::warning(page, "Fichier médical", e);
        else QMessageBox::information(page, "Dossier médical", "Informations enregistrées dans dossiers_medicaux.txt.");
    });
    connect(analyser, &QPushButton::clicked, page, [=]() {
        Animal b = a; if (!lire(b)) return;
        QStringList alertes, points;
        if (!b.maladiesChroniques.isEmpty()) alertes << "Maladie chronique déclarée : suivi vétérinaire régulier conseillé.";
        if (!b.allergies.isEmpty()) alertes << "Allergies déclarées : signaler ces informations avant tout traitement.";
        if (b.statutVaccinal == "INCOMPLET") alertes << "Vaccination incomplète : vérifier les rappels avec un vétérinaire.";
        if (b.statutVaccinal == "INCONNU") points << "Statut vaccinal à confirmer.";
        if (!b.derniereConsultation.isValid()) points << "Dernière consultation non renseignée.";
        else if (b.derniereConsultation.daysTo(QDate::currentDate()) > 365) points << "Dernière consultation datant de plus de 12 mois : contrôle préventif à envisager.";
        if (b.poids <= 0) points << "Poids non renseigné : impossible d'évaluer une tendance pondérale.";
        if (!b.traitementsEnCours.isEmpty()) points << "Traitement en cours : ne pas le modifier sans avis vétérinaire.";
        if (!b.antecedentsMedicaux.isEmpty()) points << "Antécédents présents : à examiner avec le vétérinaire.";
        if (alertes.isEmpty()) alertes << "Aucun facteur d'alerte détecté parmi les champs renseignés.";
        resultat->setPlainText("ANALYSE INDICATIVE — " + b.nom + "\n\n" +
            alertes.join("\n") + "\n\nPoints de vigilance :\n" + points.join("\n") +
            "\n\nCe résultat est un repérage par règles, pas une prédiction clinique. "
            "Les données absentes ne signifient pas absence de risque. Aucun traitement ne doit être prescrit automatiquement.");
    });
    connect(retour, &QPushButton::clicked, page, &QWidget::deleteLater);
    page->show(); page->raise();
}

// ---------------------------------------------------------------------------
// Export PDF de la liste affichée (filtres SQL en cours)
// ---------------------------------------------------------------------------
void GSmartPetCare::onExporterPdfAnimaux()
{
    if (m_vueAnimaux.isEmpty()) {
        QMessageBox::information(this, "Exporter PDF", QString::fromUtf8("Aucun animal à exporter."));
        return;
    }

    const QString suggestion = QDir::homePath() + "/animaux_" +
                               QDate::currentDate().toString("yyyy-MM-dd") + ".pdf";
    QString chemin = QFileDialog::getSaveFileName(
        this, "Exporter la liste des animaux", suggestion, "Fichiers PDF (*.pdf)");
    if (chemin.isEmpty())
        return;                                               // l'utilisateur a annulé
    if (!chemin.endsWith(".pdf", Qt::CaseInsensitive))
        chemin += ".pdf";

    if (!genererPdfAnimaux(chemin, m_vueAnimaux)) {
        QMessageBox::critical(this, "Exporter PDF",
                              QString::fromUtf8("Impossible de créer le fichier PDF.\n"
                                                "Vérifiez que le fichier n'est pas ouvert dans un autre programme."));
        return;
    }

    const auto rep = QMessageBox::question(
        this, "Exporter PDF",
        QString::fromUtf8("Le PDF a été créé avec succès.\n\nVoulez-vous l'ouvrir ?"));
    if (rep == QMessageBox::Yes)
        QDesktopServices::openUrl(QUrl::fromLocalFile(chemin));
}

bool GSmartPetCare::genererPdfAnimaux(const QString &chemin, const QVector<Animal> &lignes) const
{
    const QStringList entetes = {
        "ID", "Nom", QString::fromUtf8("Espèce"), "Sexe", "Naissance",
        QString::fromUtf8("Propriétaire"), QString::fromUtf8("Téléphone"), QString::fromUtf8("Suivi santé")
    };
    const QList<int> largeurs = { 7, 13, 10, 9, 12, 19, 13, 17 };   // en %

    QString html;
    html += "<h1 style='color:#1B5E4B;'>PetNova - Liste des animaux</h1>";
    html += QString("<p style='color:#6B7280;'>")
            + QString::fromUtf8("Exporté le ")
            + QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm")
            + QString(" - %1 animaux</p>").arg(lignes.size());

    html += "<table width='100%' cellspacing='0' cellpadding='7'><tr>";
    for (int i = 0; i < entetes.size(); ++i) {
        html += QString("<td width='%1%' align='center' "
                        "style='background-color:#1B5E4B; color:white; font-weight:bold;'>%2</td>")
                    .arg(largeurs[i]).arg(entetes[i].toHtmlEscaped());
    }
    html += "</tr>";

    for (int r = 0; r < lignes.size(); ++r) {
        const Animal &a = lignes[r];
        const QString fond = (r % 2 == 0) ? "#FFFFFF" : "#EEF3F1";
        const QStringList valeurs = { formaterIdAnimal(a.id), a.nom, a.espece, a.sexe,
                                      a.naissance.toString("dd/MM/yyyy"), a.proprietaire, a.telephone, a.suiviSante };
        html += "<tr>";
        for (int col = 0; col < valeurs.size(); ++col) {
            QString style = QString("background-color:%1; color:#1F2A37;").arg(fond);
            if (col == AnimalColSuivi) {
                const bool aJour = a.suiviSante.compare(kAJourAnimal, Qt::CaseInsensitive) == 0;
                style += QString(" color:%1; font-weight:bold;").arg(aJour ? "#0F766E" : "#B45309");
            }
            html += QString("<td align='center' style='%1'>%2</td>").arg(style, valeurs[col].toHtmlEscaped());
        }
        html += "</tr>";
    }
    html += "</table>";

    {
        QPdfWriter writer(chemin);
        writer.setPageSize(QPageSize(QPageSize::A4));
        writer.setPageOrientation(QPageLayout::Landscape);
        writer.setPageMargins(QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter);
        writer.setTitle(QString::fromUtf8("Liste des animaux - PetNova"));

        QTextDocument doc;
        doc.setHtml(html);
        doc.print(&writer);
    }   // le fichier est finalisé ici

    const QFileInfo info(chemin);
    return info.exists() && info.size() > 0;
}
