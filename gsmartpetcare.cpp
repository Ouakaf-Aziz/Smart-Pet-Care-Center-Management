#include "gsmartpetcare.h"
#include "ui_gsmartpetcare.h"

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

    // Page de connexion : seulement l'interface (aucune vérification) -> le bouton ouvre l'accueil
    ui->stackedWidget->setCurrentIndex(PageLogin);
    ui->lineEditIdentifiant->setFocus();
    connect(ui->lineEditIdentifiant, &QLineEdit::returnPressed,
            ui->lineEditMotDePasse, qOverload<>(&QWidget::setFocus));
    connect(ui->lineEditMotDePasse, &QLineEdit::returnPressed,
            ui->pushButtonConnexion, &QPushButton::click);
    connect(ui->pushButtonConnexion, &QPushButton::clicked, this, [this]() {
        afficherContenu(ContenuAccueil);
        ui->stackedWidget->setCurrentIndex(PageAccueil);
    });

    // Modules intégrés (lisent et écrivent dans Oracle) :
    initialiserRdv();                    // MODULE RENDEZ-VOUS
    new GestionServices(ui, this, this); // MODULE SERVICE (+ historique et statistiques)

    // Navigation depuis le menu latéral (employés et animaux : interface seulement pour l'instant)
    afficherContenu(ContenuAccueil);
    connect(ui->btnNavAccueil,  &QPushButton::clicked, this, [this]() { afficherContenu(ContenuAccueil); });
    connect(ui->btnNavServices, &QPushButton::clicked, this, [this]() { afficherContenu(ContenuServices); });
    connect(ui->btnNavEmployes, &QPushButton::clicked, this, [this]() { afficherContenu(ContenuEmployes); });
    connect(ui->btnNavRDV,      &QPushButton::clicked, this, [this]() {
        rafraichirRdv();                    // recharge les rendez-vous et les animaux depuis la base
        afficherContenu(ContenuRDV);
    });
    connect(ui->btnNavAnimaux,  &QPushButton::clicked, this, [this]() { afficherContenu(ContenuAnimaux); });

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
