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

namespace {

// Colonnes de données (la colonne « Animal » éventuelle et « Actions » sont gérées dynamiquement)
// Ordre = celui des en-têtes du tableau du .ui : Id, Priorité, Durée, Type, Salle, Heure, Date, Actions
enum Colonne { ColId, ColPriorite, ColDuree, ColType, ColSalle, ColHeure, ColDate };

const int RoleIdAnimal  = Qt::UserRole + 1;   // ID_ANIMAL mémorisé dans la cellule « Id »
const int RoleIdEmploye = Qt::UserRole + 2;   // ID_EMPLOYEE mémorisé dans la cellule « Id »

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

    // Page « Gestion des rendez-vous » (seule page qui fonctionne : elle lit et écrit dans Oracle)
    initialiserRdv();

    // Navigation depuis le menu latéral : les autres pages sont accessibles mais sans fonctionnement
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

// ID_EMPLOYEE saisi dans le formulaire (nul si vide ou invalide)
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
        // id employé et id animal affichés dans leurs colonnes (l[9] = ID_EMPLOYEE, l[8] = ID_ANIMAL)
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

    // ID_EMPLOYEE est obligatoire lui aussi : l'employé doit exister dans la table EMPLOYEE
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