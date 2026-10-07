#include "gestionanimaux.h"
#include "ui_gsmartpetcare.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPaintEvent>
#include <QPainter>
#include <QPdfWriter>
#include <QPolygonF>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextDocument>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>
#include <QVector>
#include <QtMath>
#include <initializer_list>

namespace {
enum Colonne { ColId, ColNom, ColEspece, ColSexe, ColNaissance, ColProprietaire,
               ColTelephone, ColSuivi, ColActions, NbColonnes };

// Cellule de date (dd/MM/yyyy) triée chronologiquement et non alphabétiquement.
class ItemDate : public QTableWidgetItem
{
public:
    explicit ItemDate(const QString &texte) : QTableWidgetItem(texte) {}
    bool operator<(const QTableWidgetItem &autre) const override
    {
        const QDate a = QDate::fromString(text(), "dd/MM/yyyy");
        const QDate b = QDate::fromString(autre.text(), "dd/MM/yyyy");
        if (a.isValid() && b.isValid())
            return a < b;
        return QTableWidgetItem::operator<(autre);
    }
};
}

namespace {
enum class TypeGraphique { Barres, Camembert, Ligne, Histogramme };

class GraphiqueAnimalWidget : public QWidget
{
public:
    GraphiqueAnimalWidget(TypeGraphique type, const QString &titre,
                          const QStringList &labels, const QVector<int> &valeurs,
                          QWidget *parent = nullptr)
        : QWidget(parent), m_type(type), m_titre(titre), m_labels(labels), m_valeurs(valeurs)
    {
        setMinimumSize(360, 260);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), Qt::white);

        p.setPen(QColor("#1F2A37"));
        QFont titreFont = p.font(); titreFont.setBold(true); titreFont.setPointSize(11);
        p.setFont(titreFont);
        p.drawText(QRect(18, 12, width()-36, 28), Qt::AlignLeft|Qt::AlignVCenter, m_titre);

        if (m_valeurs.isEmpty()) {
            p.setPen(QColor("#6B7A78"));
            p.drawText(rect().adjusted(20,50,-20,-20), Qt::AlignCenter, "Aucune donnee disponible");
            return;
        }
        QRect zone = rect().adjusted(38, 55, -25, -38);
        if (m_type == TypeGraphique::Camembert) dessinerCamembert(p, zone);
        else if (m_type == TypeGraphique::Ligne) dessinerLigne(p, zone);
        else dessinerBarres(p, zone, m_type == TypeGraphique::Histogramme);
    }

private:
    TypeGraphique m_type;
    QString m_titre;
    QStringList m_labels;
    QVector<int> m_valeurs;

    QColor couleur(int i) const {
        static const QColor cs[] = {QColor("#0F766E"), QColor("#F28C28"), QColor("#2F80C1"),
                                    QColor("#5C6B69"), QColor("#0B4F4A"), QColor("#9AA6A4")};
        return cs[i % 6];
    }

    void dessinerBarres(QPainter &p, const QRect &z, bool histogramme)
    {
        int maxV = 1; for (int v : m_valeurs) maxV = qMax(maxV, v);
        p.setPen(QColor("#CBD5D3"));
        p.drawLine(z.bottomLeft(), z.bottomRight());
        int n=m_valeurs.size(); double slot=double(z.width())/qMax(1,n);
        for(int i=0;i<n;i++) {
            int h=int((z.height()-28)*double(m_valeurs[i])/maxV);
            int gap=histogramme ? 3 : qMax(8,int(slot*0.22));
            QRect b(int(z.left()+i*slot)+gap/2, z.bottom()-h, qMax(4,int(slot)-gap), h);
            p.fillRect(b, couleur(i));
            p.setPen(QColor("#1F2A37"));
            p.drawText(QRect(b.left()-5,b.top()-20,b.width()+10,18),Qt::AlignCenter,QString::number(m_valeurs[i]));
            QString lab=i<m_labels.size()?m_labels[i]:QString::number(i+1);
            p.drawText(QRect(int(z.left()+i*slot),z.bottom()+5,int(slot),25),Qt::AlignHCenter|Qt::AlignTop,lab.left(10));
        }
    }

    void dessinerCamembert(QPainter &p, const QRect &z)
    {
        int total=0; for(int v:m_valeurs) total+=v;
        if(total<=0) return;
        int d=qMin(z.height()-5, z.width()*2/3);
        QRect pie(z.left(), z.top(), d, d);
        int start=0;
        for(int i=0;i<m_valeurs.size();i++) {
            int span=qRound(5760.0*m_valeurs[i]/total);
            p.setBrush(couleur(i)); p.setPen(Qt::white); p.drawPie(pie,start,span); start+=span;
        }
        int lx=pie.right()+18, ly=pie.top()+12;
        p.setPen(QColor("#1F2A37"));
        for(int i=0;i<m_valeurs.size();i++) {
            p.fillRect(QRect(lx,ly+i*27,13,13),couleur(i));
            QString lab=(i<m_labels.size()?m_labels[i]:QString::number(i+1));
            double pc=100.0*m_valeurs[i]/total;
            p.drawText(lx+20,ly+12+i*27,QString("%1  %2% (%3)").arg(lab).arg(pc,0,'f',0).arg(m_valeurs[i]));
        }
    }

    void dessinerLigne(QPainter &p, const QRect &z)
    {
        int maxV=1; for(int v:m_valeurs) maxV=qMax(maxV,v);
        p.setPen(QColor("#CBD5D3")); p.drawLine(z.bottomLeft(),z.bottomRight()); p.drawLine(z.bottomLeft(),z.topLeft());
        if(m_valeurs.size()==1) return;
        QPolygonF pts;
        for(int i=0;i<m_valeurs.size();i++) {
            double x=z.left()+i*(double(z.width())/(m_valeurs.size()-1));
            double y=z.bottom()-(z.height()-25)*double(m_valeurs[i])/maxV;
            pts<<QPointF(x,y);
        }
        QPen pen(QColor("#0F766E"),3); p.setPen(pen); p.setBrush(Qt::NoBrush); p.drawPolyline(pts);
        for(int i=0;i<pts.size();i++) {
            p.setBrush(QColor("#F28C28")); p.setPen(Qt::white); p.drawEllipse(pts[i],5,5);
            p.setPen(QColor("#1F2A37"));
            QString lab=i<m_labels.size()?m_labels[i]:QString::number(i+1);
            p.drawText(QRectF(pts[i].x()-22,z.bottom()+5,44,20),Qt::AlignHCenter,lab.left(4));
            p.drawText(QRectF(pts[i].x()-18,pts[i].y()-23,36,18),Qt::AlignCenter,QString::number(m_valeurs[i]));
        }
    }
};

QFrame *creerCarteKpi(const QString &titre, const QString &valeur, QWidget *parent)
{
    QFrame *f=new QFrame(parent); f->setObjectName("carteKpiStats");
    f->setStyleSheet("QFrame#carteKpiStats{background:white;border:1px solid #DDE7E5;border-radius:12px;}"
                     "QLabel{border:none;background:transparent;}");
    QVBoxLayout *l=new QVBoxLayout(f); l->setContentsMargins(16,12,16,12);
    QLabel *t=new QLabel(titre,f); t->setStyleSheet("color:#6B7A78;font-size:11px;");
    QLabel *v=new QLabel(valeur,f); v->setStyleSheet("color:#0F766E;font-size:24px;font-weight:700;");
    l->addWidget(t); l->addWidget(v); return f;
}
}

GestionAnimaux::GestionAnimaux(Ui::GSmartPetCare *uiPrincipal, QMainWindow *fenetre, QObject *parent)
    : QObject(parent)
    , ui(uiPrincipal)
    , m_fenetre(fenetre)
{
    ui->pageAnimaux->setAttribute(Qt::WA_StyledBackground, true);   // fond de la feuille de style

    ui->dateEditNaissanceAnimal->setDisplayFormat("dd/MM/yyyy");
    ui->dateEditNaissanceAnimal->setCalendarPopup(true);

    connect(ui->pushButtonEnregistrerAnimal,  &QPushButton::clicked, this, &GestionAnimaux::enregistrerAnimal);
    connect(ui->pushButtonAnnulerAnimal,      &QPushButton::clicked, this, &GestionAnimaux::annulerFormulaire);
    connect(ui->pushButtonChoisirPhotoAnimal, &QPushButton::clicked, this, &GestionAnimaux::choisirPhoto);

    connect(ui->lineEditRechercheAnimal, &QLineEdit::textChanged, this, &GestionAnimaux::appliquerFiltres);
    connect(ui->comboBoxFiltreEspece, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GestionAnimaux::appliquerFiltres);
    connect(ui->comboBoxTriAnimal, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GestionAnimaux::trierAnimaux);

    connect(ui->pushButtonExporterPdfAnimal,   &QPushButton::clicked, this, &GestionAnimaux::exporterPdf);
    connect(ui->pushButtonStatistiquesAnimal,  &QPushButton::clicked, this, &GestionAnimaux::afficherStatistiques);
    connect(ui->pushButtonChatbotAnimal,       &QPushButton::clicked, this, &GestionAnimaux::ouvrirChatbotDiagnostic);
    connect(ui->pushButtonRappelsSanteAnimal,  &QPushButton::clicked, this, &GestionAnimaux::afficherRappelsSante);

    configurerTableau();
    remplirExemples();
    viderFormulaire();
}

void GestionAnimaux::setUtilisateur(const QString &nomComplet)
{
    m_utilisateur = nomComplet.trimmed();
}

// ---------------------------------------------------------------------
//  Tableau
// ---------------------------------------------------------------------

void GestionAnimaux::configurerTableau()
{
    QTableWidget *t = ui->tableWidgetAnimaux;   // colonnes et libellés : définis dans le .ui

    t->verticalHeader()->setVisible(false);
    t->verticalHeader()->setDefaultSectionSize(46);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setAlternatingRowColors(true);
    t->setShowGrid(true);

    // En-têtes longs sur deux lignes pour que chaque attribut reste lisible en entier.
    const QMap<int, QString> entetes = {
        { ColNaissance, "Date de\nnaissance" },
        { ColTelephone, "Telephone\nproprietaire" },
        { ColSuivi,     "Suivi\nsante" },
    };
    for (auto it = entetes.cbegin(); it != entetes.cend(); ++it)
        if (QTableWidgetItem *hi = t->horizontalHeaderItem(it.key()))
            hi->setText(it.value());

    QHeaderView *h = t->horizontalHeader();
    h->setStretchLastSection(false);
    h->setDefaultAlignment(Qt::AlignCenter);
    h->setMinimumSectionSize(60);
    h->setFixedHeight(52);

    // Colonnes courtes : largeur adaptée au contenu ET à l'en-tête.
    for (int col : { int(ColId), int(ColNom), int(ColEspece), int(ColSexe),
                     int(ColNaissance), int(ColTelephone) })
        h->setSectionResizeMode(col, QHeaderView::ResizeToContents);
    // Colonnes de texte libre : se partagent la place restante.
    h->setSectionResizeMode(ColProprietaire, QHeaderView::Stretch);
    h->setSectionResizeMode(ColSuivi, QHeaderView::Stretch);
    h->setSectionResizeMode(ColActions, QHeaderView::Fixed);  t->setColumnWidth(ColActions, 210);
}

void GestionAnimaux::remplirExemples()
{
    ui->tableWidgetAnimaux->setRowCount(0);
    const QList<QStringList> exemples = {
        { "A001", "Rex",   "Chien",  "Male",    "12/03/2020", "Ahmed Ben Salah", "20 123 456", "A jour" },
        { "A002", "Minou", "Chat",   "Femelle", "05/07/2022", "Sarra Trabelsi",  "55 987 321", "Vaccin a prevoir" },
        { "A003", "Coco",  "Oiseau", "Male",    "18/11/2023", "Mohamed Gharbi",  "98 456 123", "A jour" },
        { "A004", "Nala",  "Chat",   "Femelle", "02/02/2018", "Ines Mansour",    "22 654 987", "Vermifuge a prevoir" },
        { "A005", "Bunny", "Lapin",  "Femelle", "23/09/2024", "Karim Jlassi",    "50 111 222", "A jour" },
    };
    for (const QStringList &ligne : exemples) {
        const int row = ui->tableWidgetAnimaux->rowCount();
        ui->tableWidgetAnimaux->insertRow(row);
        remplirLigne(row, ligne);
    }
}

// valeurs : une valeur par colonne de données (ID ... Suivi santé), sans la colonne Actions.
// La photo et l'e-mail n'ont pas de colonne : ils sont rangés dans la cellule « Nom ».
void GestionAnimaux::remplirLigne(int row, const QStringList &valeurs,
                                  const QString &photo, const QString &email)
{
    QTableWidget *t = ui->tableWidgetAnimaux;
    for (int col = 0; col < ColActions; ++col) {
        const QString texte = valeurs.value(col);
        QTableWidgetItem *it = (col == ColNaissance) ? new ItemDate(texte)
                                                     : new QTableWidgetItem(texte);
        it->setTextAlignment(Qt::AlignCenter);
        it->setToolTip(texte);
        if (col == ColNom) {
            it->setData(Qt::UserRole, photo);
            it->setData(Qt::UserRole + 1, email);
        }
        t->setItem(row, col, it);
    }
    ajouterBoutonsAction(row);
}

void GestionAnimaux::ajouterBoutonsAction(int row)
{
    QTableWidget *t = ui->tableWidgetAnimaux;

    QWidget *conteneur = new QWidget(t);
    QHBoxLayout *layout = new QHBoxLayout(conteneur);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(6);

    QPushButton *btnModifier = new QPushButton("Modifier", conteneur);
    QPushButton *btnSupprimer = new QPushButton("Supprimer", conteneur);
    btnModifier->setCursor(Qt::PointingHandCursor);
    btnSupprimer->setCursor(Qt::PointingHandCursor);
    btnModifier->setFixedSize(92, 32);
    btnSupprimer->setFixedSize(102, 32);
    btnModifier->setStyleSheet(
        "QPushButton{background-color:#2F80C1;color:#FFFFFF;border:none;"
        "border-radius:4px;padding:5px 10px;font-weight:bold;font-size:9pt;}"
        "QPushButton:hover{background-color:#276FA8;}");
    btnSupprimer->setStyleSheet(
        "QPushButton{background-color:#C94F4F;color:#FFFFFF;border:none;"
        "border-radius:4px;padding:5px 10px;font-weight:bold;font-size:9pt;}"
        "QPushButton:hover{background-color:#B64242;}");

    layout->addStretch();
    layout->addWidget(btnModifier);
    layout->addWidget(btnSupprimer);
    layout->addStretch();
    t->setCellWidget(row, ColActions, conteneur);

    // La ligne est recalculée au clic : elle change après un tri ou une suppression.
    connect(btnModifier, &QPushButton::clicked, this, [this, conteneur]() {
        const int r = ui->tableWidgetAnimaux->indexAt(conteneur->pos()).row();
        if (r >= 0) chargerDansFormulaire(r);
    });
    connect(btnSupprimer, &QPushButton::clicked, this, [this, conteneur]() {
        const int r = ui->tableWidgetAnimaux->indexAt(conteneur->pos()).row();
        if (r >= 0) supprimerLigne(r);
    });
}

int GestionAnimaux::trouverLigne(const QString &id) const
{
    QTableWidget *t = ui->tableWidgetAnimaux;
    for (int r = 0; r < t->rowCount(); ++r) {
        const QTableWidgetItem *it = t->item(r, ColId);
        if (it && it->text() == id)
            return r;
    }
    return -1;
}

QString GestionAnimaux::prochainId() const
{
    int maxId = 0;
    for (int row = 0; row < ui->tableWidgetAnimaux->rowCount(); ++row) {
        QTableWidgetItem *item = ui->tableWidgetAnimaux->item(row, ColId);
        if (!item) continue;
        QString idText = item->text();
        idText.remove(QRegularExpression("[^0-9]"));
        bool ok = false;
        const int val = idText.toInt(&ok);
        if (ok && val > maxId) maxId = val;
    }
    return QString("A%1").arg(maxId + 1, 3, 10, QChar('0'));   // A001, A002, ...
}

// ---------------------------------------------------------------------
//  Formulaire : ajout / modification / suppression
// ---------------------------------------------------------------------

bool GestionAnimaux::validerFormulaire(QString *messageErreur) const
{
    if (ui->lineEditNomAnimal->text().trimmed().isEmpty()) {
        if (messageErreur) *messageErreur = "Le nom de l'animal est obligatoire.";
        return false;
    }
    if (ui->lineEditProprietaireAnimal->text().trimmed().isEmpty()) {
        if (messageErreur) *messageErreur = "Le nom du proprietaire est obligatoire.";
        return false;
    }
    const QString email = ui->lineEditEmailProprietaireAnimal->text().trimmed();
    if (!email.isEmpty() && !email.contains('@')) {
        if (messageErreur) *messageErreur = "L'email du proprietaire n'est pas valide.";
        return false;
    }
    return true;
}

void GestionAnimaux::viderFormulaire()
{
    ui->lineEditIdAnimal->setText(prochainId());
    ui->lineEditNomAnimal->clear();
    ui->comboBoxEspeceAnimal->setCurrentIndex(0);
    ui->comboBoxSexeAnimal->setCurrentIndex(0);
    ui->dateEditNaissanceAnimal->setDate(QDate::currentDate());
    ui->lineEditPhotoAnimal->clear();
    ui->lineEditProprietaireAnimal->clear();
    ui->lineEditTelephoneProprietaireAnimal->clear();
    ui->lineEditEmailProprietaireAnimal->clear();

    ui->tableWidgetAnimaux->clearSelection();
    m_enModification = false;
    definirModeFormulaire(false);
}

void GestionAnimaux::definirModeFormulaire(bool enModification)
{
    ui->labelDetailTitreAnimal->setText(enModification ? "Modifier l'animal" : "Ajouter un animal");
    ui->pushButtonEnregistrerAnimal->setText(enModification ? "Mettre a jour" : "Enregistrer");
}

void GestionAnimaux::chargerDansFormulaire(int row)
{
    QTableWidget *t = ui->tableWidgetAnimaux;
    if (row < 0 || row >= t->rowCount()) return;

    auto texte = [&](int col) -> QString {
        const QTableWidgetItem *it = t->item(row, col);
        return it ? it->text() : QString();
    };

    t->selectRow(row);
    m_enModification = true;

    ui->lineEditIdAnimal->setText(texte(ColId));
    ui->lineEditNomAnimal->setText(texte(ColNom));
    const int especeIndex = ui->comboBoxEspeceAnimal->findText(texte(ColEspece));
    ui->comboBoxEspeceAnimal->setCurrentIndex(especeIndex >= 0 ? especeIndex : 0);
    const int sexeIndex = ui->comboBoxSexeAnimal->findText(texte(ColSexe));
    ui->comboBoxSexeAnimal->setCurrentIndex(sexeIndex >= 0 ? sexeIndex : 0);
    const QDate date = QDate::fromString(texte(ColNaissance), "dd/MM/yyyy");
    ui->dateEditNaissanceAnimal->setDate(date.isValid() ? date : QDate::currentDate());
    ui->lineEditProprietaireAnimal->setText(texte(ColProprietaire));
    ui->lineEditTelephoneProprietaireAnimal->setText(texte(ColTelephone));

    const QTableWidgetItem *nom = t->item(row, ColNom);
    ui->lineEditPhotoAnimal->setText(nom ? nom->data(Qt::UserRole).toString() : QString());
    ui->lineEditEmailProprietaireAnimal->setText(nom ? nom->data(Qt::UserRole + 1).toString() : QString());

    definirModeFormulaire(true);
    ui->lineEditNomAnimal->setFocus();
}

void GestionAnimaux::supprimerLigne(int row)
{
    QTableWidget *t = ui->tableWidgetAnimaux;
    if (row < 0 || row >= t->rowCount()) return;

    const QTableWidgetItem *nomItem = t->item(row, ColNom);
    const QString nom = nomItem ? nomItem->text() : QString("cet animal");

    if (QMessageBox::question(m_fenetre, "Confirmer la suppression",
                              QString("Voulez-vous vraiment supprimer %1 ?").arg(nom),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        t->removeRow(row);
        viderFormulaire();
    }
}

void GestionAnimaux::enregistrerAnimal()
{
    QString erreur;
    if (!validerFormulaire(&erreur)) {
        QMessageBox::warning(m_fenetre, "Formulaire incomplet", erreur);
        return;
    }

    QTableWidget *t = ui->tableWidgetAnimaux;
    const QString nom = ui->lineEditNomAnimal->text().trimmed();
    const QString photo = ui->lineEditPhotoAnimal->text().trimmed();
    const QString email = ui->lineEditEmailProprietaireAnimal->text().trimmed();

    // --- Modification : la ligne est retrouvée par son ID (elle a pu bouger après un tri) ---
    if (m_enModification) {
        const int row = trouverLigne(ui->lineEditIdAnimal->text());
        if (row < 0) {
            QMessageBox::warning(m_fenetre, "Animal introuvable",
                                 "Cet animal n'existe plus dans la liste.");
            viderFormulaire();
            return;
        }
        const QString suivi = t->item(row, ColSuivi) ? t->item(row, ColSuivi)->text() : QString("A jour");
        const QStringList valeurs = {
            ui->lineEditIdAnimal->text(), nom,
            ui->comboBoxEspeceAnimal->currentText(),
            ui->comboBoxSexeAnimal->currentText(),
            ui->dateEditNaissanceAnimal->date().toString("dd/MM/yyyy"),
            ui->lineEditProprietaireAnimal->text().trimmed(),
            ui->lineEditTelephoneProprietaireAnimal->text().trimmed(),
            suivi
        };
        for (int col = 0; col < ColActions; ++col) {
            QTableWidgetItem *it = t->item(row, col);
            if (!it) continue;
            it->setText(valeurs.value(col));
            it->setToolTip(valeurs.value(col));
        }
        if (QTableWidgetItem *itNom = t->item(row, ColNom)) {
            itNom->setData(Qt::UserRole, photo);
            itNom->setData(Qt::UserRole + 1, email);
        }

        QMessageBox::information(m_fenetre, "Animal modifie", "Les informations ont ete mises a jour.");
        viderFormulaire();
        appliquerFiltres();
        return;
    }

    // --- Ajout ---
    const QStringList valeurs = {
        prochainId(), nom,
        ui->comboBoxEspeceAnimal->currentText(),
        ui->comboBoxSexeAnimal->currentText(),
        ui->dateEditNaissanceAnimal->date().toString("dd/MM/yyyy"),
        ui->lineEditProprietaireAnimal->text().trimmed(),
        ui->lineEditTelephoneProprietaireAnimal->text().trimmed(),
        "A jour"
    };
    const int row = t->rowCount();
    t->insertRow(row);
    remplirLigne(row, valeurs, photo, email);

    QMessageBox::information(m_fenetre, "Animal ajoute",
                             QString("%1 a ete ajoute avec succes.").arg(nom));
    viderFormulaire();
    appliquerFiltres();
}

void GestionAnimaux::annulerFormulaire()
{
    viderFormulaire();
}

void GestionAnimaux::choisirPhoto()
{
    const QString chemin = QFileDialog::getOpenFileName(
        m_fenetre, "Choisir une photo", QString(), "Images (*.png *.jpg *.jpeg)");
    if (!chemin.isEmpty())
        ui->lineEditPhotoAnimal->setText(chemin);
}

// ---------------------------------------------------------------------
//  Métiers de base : recherche / filtre / tri / export PDF / statistiques
// ---------------------------------------------------------------------

// Recherche et filtre par espèce se combinent : une ligne reste visible si elle
// satisfait les deux.
void GestionAnimaux::appliquerFiltres()
{
    QTableWidget *t = ui->tableWidgetAnimaux;
    const QString recherche = ui->lineEditRechercheAnimal->text().trimmed().toLower();
    const bool toutesEspeces = (ui->comboBoxFiltreEspece->currentIndex() <= 0);   // « Espece : Toutes »
    const QString espece = ui->comboBoxFiltreEspece->currentText();

    for (int row = 0; row < t->rowCount(); ++row) {
        bool visible = true;

        if (!toutesEspeces) {
            const QTableWidgetItem *it = t->item(row, ColEspece);
            visible = it && it->text().compare(espece, Qt::CaseInsensitive) == 0;
        }

        if (visible && !recherche.isEmpty()) {
            bool trouve = false;
            for (int col = 0; col < ColActions; ++col) {
                const QTableWidgetItem *it = t->item(row, col);
                if (it && it->text().toLower().contains(recherche)) {
                    trouve = true;
                    break;
                }
            }
            visible = trouve;
        }

        t->setRowHidden(row, !visible);
    }
}

void GestionAnimaux::trierAnimaux(int index)
{
    int colonne = -1;
    switch (index) {
    case 0: colonne = ColNom; break;
    case 1: colonne = ColEspece; break;
    case 2: colonne = ColNaissance; break;
    case 3: colonne = ColProprietaire; break;
    default: return;
    }
    ui->tableWidgetAnimaux->sortItems(colonne, Qt::AscendingOrder);
    appliquerFiltres();   // l'état « masqué » est lié à la position des lignes
}

void GestionAnimaux::exporterPdf()
{
    if (ui->tableWidgetAnimaux->rowCount() == 0) {
        QMessageBox::information(m_fenetre, "Exporter PDF", QString::fromUtf8("Aucun animal à exporter."));
        return;
    }

    const QString suggestion = QDir::homePath() + "/animaux_" +
                               QDate::currentDate().toString("yyyy-MM-dd") + ".pdf";

    QString chemin = QFileDialog::getSaveFileName(
        m_fenetre, QString::fromUtf8("Exporter la liste des animaux"),
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

bool GestionAnimaux::genererPdf(const QString &chemin)
{
    QTableWidget *t = ui->tableWidgetAnimaux;

    const QList<int> largeurs = { 8, 13, 11, 10, 14, 19, 14, 11 };   // en %, une par colonne exportée

    // Seules les lignes visibles (recherche / filtre en cours) sont exportées.
    int nbExportes = 0;
    for (int r = 0; r < t->rowCount(); ++r)
        if (!t->isRowHidden(r)) ++nbExportes;

    QString html;
    html += "<h1 style='color:#1B5E4B;'>PetNova - Liste des animaux</h1>";
    html += QString("<p style='color:#6B7280;'>")
            + QString::fromUtf8("Exporté le ")
            + QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm")
            + QString(" - %1 animaux</p>").arg(nbExportes);

    html += "<table width='100%' cellspacing='0' cellpadding='7'>";

    // En-tête vert
    html += "<tr>";
    for (int col = 0; col < ColActions; ++col) {
        const QTableWidgetItem *h = t->horizontalHeaderItem(col);
        html += QString("<td width='%1%' align='center' "
                        "style='background-color:#1B5E4B; color:white; font-weight:bold;'>%2</td>")
                    .arg(largeurs[col])
                    .arg((h ? h->text().replace('\n', ' ') : QString()).toHtmlEscaped());
    }
    html += "</tr>";

    // Lignes de données
    int numero = 0;
    for (int r = 0; r < t->rowCount(); ++r) {
        if (t->isRowHidden(r)) continue;
        const QString fond = (numero++ % 2 == 0) ? "#FFFFFF" : "#EEF3F1";
        html += "<tr>";
        for (int col = 0; col < ColActions; ++col) {
            const QTableWidgetItem *it = t->item(r, col);
            const QString texte = it ? it->text() : QString();

            QString style = QString("background-color:%1; color:#1F2A37;").arg(fond);
            if (col == ColSuivi && texte.compare("A jour", Qt::CaseInsensitive) != 0)
                style += " color:#B45309; font-weight:bold;";   // suivi de santé à prévoir
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
        writer.setTitle(QString::fromUtf8("Liste des animaux - PetNova"));

        QTextDocument doc;
        doc.setHtml(html);
        doc.print(&writer);
    }   // le fichier est finalisé ici

    const QFileInfo info(chemin);
    return info.exists() && info.size() > 0;
}

// ---------------------------------------------------------------------
//  Statistiques (page construite à chaque ouverture : données toujours à jour)
// ---------------------------------------------------------------------

void GestionAnimaux::afficherStatistiques()
{
    // Fenêtre recréée à chaque ouverture : les graphiques utilisent toujours les données actuelles.
    QMap<QString,int> especes, sexes;
    QVector<int> mois(12,0), ages(4,0);
    int total=0, suivis=0;
    QDate aujourdhui=QDate::currentDate();

    for(int r=0;r<ui->tableWidgetAnimaux->rowCount();++r) {
        QTableWidgetItem *e=ui->tableWidgetAnimaux->item(r,ColEspece);
        QTableWidgetItem *s=ui->tableWidgetAnimaux->item(r,ColSexe);
        QTableWidgetItem *d=ui->tableWidgetAnimaux->item(r,ColNaissance);
        QTableWidgetItem *suivi=ui->tableWidgetAnimaux->item(r,ColSuivi);
        if(e && !e->text().trimmed().isEmpty()) especes[e->text().trimmed()]++;
        if(s && !s->text().trimmed().isEmpty()) sexes[s->text().trimmed()]++;
        if(suivi && suivi->text().compare("A jour",Qt::CaseInsensitive)!=0) suivis++;
        if(d) {
            QDate naissance=QDate::fromString(d->text(),"dd/MM/yyyy");
            if(naissance.isValid()) {
                mois[naissance.month()-1]++;
                int age=naissance.daysTo(aujourdhui)/365;
                if(age<1) ages[0]++; else if(age<=3) ages[1]++; else if(age<=7) ages[2]++; else ages[3]++;
            }
        }
        total++;
    }

    // Fenêtre centrée sur l'application, comme les statistiques des services et des rendez-vous.
    QDialog dlg(m_fenetre);
    dlg.setWindowTitle("Statistiques des animaux");
    dlg.setMinimumSize(1100, 650);
    dlg.resize(1200, 720);
    dlg.setStyleSheet("QDialog{background:#F4F8F7;}");
    QWidget *page=&dlg;
    QVBoxLayout *racine=new QVBoxLayout(page); racine->setContentsMargins(24,20,24,20); racine->setSpacing(16);

    QHBoxLayout *entete=new QHBoxLayout;
    QLabel *titre=new QLabel("Statistiques des animaux",page); titre->setStyleSheet("font-size:22px;font-weight:700;color:#0B4F4A;");
    entete->addWidget(titre); entete->addStretch(); racine->addLayout(entete);

    QHBoxLayout *kpis=new QHBoxLayout; kpis->setSpacing(12);
    QString especeTop="-"; int top=0; for(auto it=especes.cbegin();it!=especes.cend();++it) if(it.value()>top){top=it.value();especeTop=it.key();}
    kpis->addWidget(creerCarteKpi("Total animaux",QString::number(total),page));
    kpis->addWidget(creerCarteKpi("Especes",QString::number(especes.size()),page));
    kpis->addWidget(creerCarteKpi("Espece dominante",especeTop,page));
    kpis->addWidget(creerCarteKpi("Suivis sante",QString::number(suivis),page));
    racine->addLayout(kpis);

    QScrollArea *scroll=new QScrollArea(page); scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame); scroll->setStyleSheet("QScrollArea{background:transparent;border:none;}");
    QWidget *contenu=new QWidget; contenu->setStyleSheet("background:transparent;");
    QGridLayout *grille=new QGridLayout(contenu); grille->setSpacing(14);

    QStringList le; QVector<int> ve; for(auto it=especes.cbegin();it!=especes.cend();++it){le<<it.key();ve<<it.value();}
    QStringList ls; QVector<int> vs; for(auto it=sexes.cbegin();it!=sexes.cend();++it){ls<<it.key();vs<<it.value();}
    QStringList lm={"Jan","Fev","Mar","Avr","Mai","Juin","Juil","Aout","Sep","Oct","Nov","Dec"};
    QStringList la={"< 1 an","1-3 ans","4-7 ans","8+ ans"};

    auto *g1=new GraphiqueAnimalWidget(TypeGraphique::Barres,"Animaux par espece",le,ve,contenu);
    auto *g2=new GraphiqueAnimalWidget(TypeGraphique::Camembert,"Repartition par sexe",ls,vs,contenu);
    auto *g3=new GraphiqueAnimalWidget(TypeGraphique::Ligne,"Naissances par mois",lm,mois,contenu);
    auto *g4=new GraphiqueAnimalWidget(TypeGraphique::Histogramme,"Repartition par tranche d'age",la,ages,contenu);
    for(QWidget *g:{static_cast<QWidget*>(g1),static_cast<QWidget*>(g2),static_cast<QWidget*>(g3),static_cast<QWidget*>(g4)})
        g->setStyleSheet("background:white;border:1px solid #DDE7E5;border-radius:12px;");
    grille->addWidget(g1,0,0); grille->addWidget(g2,0,1); grille->addWidget(g3,1,0); grille->addWidget(g4,1,1);
    scroll->setWidget(contenu); racine->addWidget(scroll,1);

    dlg.exec();
}

// ---------------------------------------------------------------------
//  Métiers innovants : chatbot de diagnostic / rappels de santé
// ---------------------------------------------------------------------

QString GestionAnimaux::contexteAnimalSelectionne() const
{
    int row = ui->tableWidgetAnimaux->currentRow();
    if (row < 0) return QString();

    auto valeur = [&](int col) -> QString {
        QTableWidgetItem *it = ui->tableWidgetAnimaux->item(row, col);
        return it ? it->text().trimmed() : QString();
    };

    QString naissance = valeur(ColNaissance);
    QDate d = QDate::fromString(naissance, "dd/MM/yyyy");
    QString age = "age inconnu";
    if (d.isValid()) {
        int ans = d.daysTo(QDate::currentDate()) / 365;
        age = QString::number(qMax(0, ans)) + " an(s)";
    }

    return QString("%1|%2|%3|%4|%5|%6")
        .arg(valeur(ColNom), valeur(ColEspece), valeur(ColSexe), age,
             valeur(ColProprietaire), valeur(ColSuivi));
}

QString GestionAnimaux::analyserSymptomes(const QString &question, const QString &espece, const QString &nomAnimal) const
{
    QString q = question.toLower().normalized(QString::NormalizationForm_D);
    q.remove(QRegularExpression("[\\x{0300}-\\x{036f}]"));
    QString animal = nomAnimal.isEmpty() ? "l'animal" : nomAnimal;
    QString type = espece.isEmpty() ? "animal" : espece.toLower();

    auto contient = [&](std::initializer_list<const char*> mots) {
        for (const char *m : mots) if (q.contains(QString::fromUtf8(m))) return true;
        return false;
    };

    // TRIAGE : signes qui doivent passer avant toute hypothese diagnostique.
    if (contient({"respire mal","difficulte a respirer","etouffe","convulsion","convulse","inconscient",
                  "ne se leve plus","paralys","saigne beaucoup","hemorrag","ventre gonfle","empoison",
                  "toxique","chocolat","raticide","antigel"})) {
        return QString(
            "🚨 URGENCE VETERINAIRE\n\n"
            "%1 presente un signe potentiellement grave. Ne donnez aucun medicament humain et ne faites pas vomir "
            "l'animal sans instruction veterinaire. Gardez-le au calme, limitez les manipulations et contactez/dirigez-vous "
            "vers un veterinaire ou un service d'urgence immediatement.\n\n"
            "Si une substance a ete ingeree, conservez l'emballage et notez l'heure et la quantite approximative."
        ).arg(animal);
    }

    if (contient({"vomit","vomissement","vomissements"})) {
        return QString(
            "Hypotheses possibles : irritation digestive, changement alimentaire, parasites, corps etranger ou autre maladie.\n\n"
            "A verifier : nombre de vomissements, presence de sang, diarrhee, douleur, ingestion d'un objet/produit, appetit et hydratation.\n\n"
            "Conduite a tenir : laissez de l'eau accessible en petites quantites et surveillez. Pas de medicament humain. "
            "Si les vomissements sont repetes, avec sang, douleur, grande fatigue, ventre gonfle, impossibilite de garder l'eau, "
            "ou si %1 est tres jeune/age, consultation veterinaire rapide. Le traitement depend de la cause et peut necessiter "
            "examen, rehydratation et traitement veterinaire adapte."
        ).arg(animal);
    }

    if (contient({"diarrhee","selles molles","sang dans les selles"})) {
        return QString(
            "Hypotheses possibles : trouble alimentaire, parasites, infection ou inflammation digestive.\n\n"
            "A verifier : duree, frequence, sang, vomissements, alimentation recente, vermifugation et etat general.\n\n"
            "Conduite a tenir : eau disponible, surveillance de l'hydratation et alimentation habituelle sans friandises ni nouveaux aliments. "
            "Sang abondant, abattement marque, vomissements associes, deshydratation ou diarrhee persistante = consultation. "
            "Le veterinaire choisira le traitement apres avoir identifie la cause; un antiparasitaire ou autre medicament ne doit pas etre donne au hasard."
        );
    }

    if (contient({"ne mange","mange pas","perte appetit","appetit","anorex"})) {
        QString urgenceChat = type.contains("chat") ?
            " Chez le chat, une absence d'alimentation prolongee est particulierement preoccupante." : "";
        return QString(
            "Une perte d'appetit est un symptome non specifique : douleur, probleme digestif/dentaire, fievre, stress ou autre maladie sont possibles.\n\n"
            "Verifiez : eau, vomissements/diarrhee, douleur, bouche/dents, temperature si vous savez la mesurer sans risque, et changement de comportement.%1\n\n"
            "Ne forcez pas de medicament ou de nourriture. Si %2 refuse toujours de manger, semble faible, vomit, souffre ou boit anormalement, "
            "faites evaluer l'animal par un veterinaire."
        ).arg(urgenceChat, animal);
    }

    if (contient({"fievre","temperature","chaud"})) {
        return QString(
            "La sensation au toucher ne suffit pas pour confirmer une fievre. Une temperature anormale peut accompagner une infection, inflammation ou autre probleme.\n\n"
            "Conduite a tenir : repos, eau disponible, environnement tempere. Ne donnez jamais paracetamol, ibuprofene ou aspirine sans prescription veterinaire : "
            "certains sont toxiques, notamment chez le chat. Si l'etat general de %1 est mauvais ou si la temperature mesuree est anormale/persistante, consultation veterinaire."
        ).arg(animal);
    }

    if (contient({"gratte","demange","puce","tiques","tique","perd ses poils","poils"})) {
        return QString(
            "Hypotheses possibles : puces/tiques, allergie, irritation cutanee, infection ou parasite de peau.\n\n"
            "Inspectez la peau et le pelage : rougeurs, plaies, parasites visibles, zones sans poils, localisation et intensite des demangeaisons. "
            "Evitez les produits antiparasitaires non adaptes a l'espece : certains produits pour chien sont dangereux pour le chat. "
            "Le traitement doit etre choisi selon le parasite/la cause et le poids de %1; une consultation est conseillee si lesions, infection, fortes demangeaisons ou persistance."
        ).arg(animal);
    }

    if (contient({"boite","boiterie","patte","douleur","blesse","plaie"})) {
        return QString(
            "Une boiterie/douleur peut venir d'une plaie, entorse, corps etranger, fracture ou probleme articulaire.\n\n"
            "Conduite a tenir : limitez l'activite, examinez visuellement sans forcer l'articulation. Pour une petite plaie superficielle, rincer doucement au serum physiologique/eau propre. "
            "Pas d'anti-inflammatoire humain. Appui impossible, deformation, douleur importante, morsure, plaie profonde ou saignement persistant = veterinaire rapidement."
        );
    }

    if (contient({"tousse","toux","eternue","eternuement","nez coule","ecoulement"})) {
        return QString(
            "Hypotheses possibles : irritation, infection respiratoire, allergie ou autre atteinte respiratoire.\n\n"
            "Observez frequence, ecoulement nasal/oculaire, appetit, energie et surtout la respiration au repos. "
            "Gardez %1 au calme et evitez fumee/aerosols. Respiration difficile, bouche ouverte au repos, coloration bleutee/pale ou aggravation = urgence veterinaire."
        ).arg(animal);
    }

    if (contient({"oreille","secoue la tete","mauvaise odeur"})) {
        return QString(
            "Une otite, des parasites, un corps etranger ou une irritation sont possibles. Ne mettez pas de produit, huile ou coton-tige profond dans l'oreille sans examen. "
            "Douleur, mauvaise odeur, ecoulement ou grattage persistant justifient une consultation : le traitement depend de l'etat du tympan et de la cause."
        );
    }

    if (contient({"urine","uriner","pipi","n'urine","sang urine"})) {
        QString noteChat = type.contains("chat") ?
            " Chez un chat, surtout male, des efforts repetes sans produire d'urine peuvent correspondre a une obstruction : urgence immediate." : "";
        return QString(
            "Des changements urinaires peuvent etre lies a une infection, inflammation, calculs ou obstruction.%1\n\n"
            "Notez la frequence, quantite, douleur, sang et consommation d'eau. Si %2 essaie d'uriner sans y arriver, semble douloureux ou abattu : urgence veterinaire."
        ).arg(noteChat, animal);
    }

    if (contient({"vaccin","vaccination","rappel"})) {
        return QString(
            "Pour %1, le calendrier vaccinal depend de l'espece, de l'age, du mode de vie et des vaccins deja recus. "
            "Consultez son dossier de sante et le module Rappels. Le veterinaire doit confirmer les vaccins necessaires et leurs dates."
        ).arg(animal);
    }

    return QString(
        "Je n'ai pas assez d'elements pour proposer une hypothese utile pour %1.\n\n"
        "Precisez : 1) symptome principal, 2) depuis quand, 3) frequence/intensite, 4) appetit et eau, "
        "5) vomissements/diarrhee, 6) comportement/energie, 7) exposition a un toxique ou traumatisme.\n\n"
        "Je pourrai alors faire un triage plus precis. En cas de respiration difficile, convulsions, inconscience, hemorragie, "
        "intoxication suspectee ou douleur intense, consultez en urgence."
    ).arg(animal);
}

void GestionAnimaux::ouvrirChatbotDiagnostic()
{
    // Tant que le vrai module Login n'est pas connecte, PetNova demande l'identite une seule fois.
    if (m_utilisateur.trimmed().isEmpty()) {
        bool okNom = false;
        QString nom = QInputDialog::getText(m_fenetre, "Identification PetNova",
            "Qui utilise l'assistant ? (nom de l'employe)", QLineEdit::Normal, QString(), &okNom);
        if (!okNom || nom.trimmed().isEmpty()) return;
        m_utilisateur = nom.trimmed();
    }

    int row = ui->tableWidgetAnimaux->currentRow();
    if (row < 0) {
        QMessageBox::information(m_fenetre, "Assistant diagnostic",
            "Selectionnez d'abord un animal dans le tableau. Le chatbot utilisera automatiquement sa fiche pour contextualiser l'analyse.");
        return;
    }

    QString contexte = contexteAnimalSelectionne();
    QStringList c = contexte.split('|');
    QString nomAnimal = c.value(0, "Animal");
    QString espece = c.value(1, "");
    QString sexe = c.value(2, "");
    QString age = c.value(3, "");
    QString proprietaire = c.value(4, "");
    QString suivi = c.value(5, "");

    QDialog *dlg = new QDialog(m_fenetre);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("PetNova Care Assistant - " + nomAnimal);
    dlg->resize(760, 650);
    dlg->setStyleSheet(
        "QDialog{background:#F4F8F7;}"
        "QLabel#titreChat{font-size:20px;font-weight:700;color:#0B4F4A;}"
        "QLabel#ficheChat{background:#E8F5F3;border:1px solid #B8DDD8;border-radius:10px;padding:10px;color:#1F2A37;}"
        "QTextEdit{background:white;border:1px solid #D7E3E1;border-radius:10px;padding:10px;font-size:12px;}"
        "QPushButton{background:#0F766E;color:white;border:none;border-radius:8px;padding:10px 18px;font-weight:700;}"
        "QPushButton:hover{background:#0B5F59;}"
    );

    QVBoxLayout *layout = new QVBoxLayout(dlg);
    layout->setContentsMargins(18,18,18,18);
    layout->setSpacing(10);

    QLabel *titre = new QLabel("Assistant de triage & diagnostic", dlg);
    titre->setObjectName("titreChat");
    layout->addWidget(titre);

    QLabel *fiche = new QLabel(
        QString("👤 Utilisateur : %1\n🐾 Patient : %2  •  %3  •  %4  •  %5\nProprietaire : %6  •  Suivi sante : %7")
            .arg(m_utilisateur, nomAnimal, espece, sexe, age, proprietaire, suivi), dlg);
    fiche->setObjectName("ficheChat");
    fiche->setWordWrap(true);
    layout->addWidget(fiche);

    QTextEdit *conversation = new QTextEdit(dlg);
    conversation->setReadOnly(true);
    conversation->setHtml(QString(
        "<b>PetNova Assistant :</b> Bonjour %1. Je connais la fiche de <b>%2</b>. "
        "Decrivez ses symptomes le plus precisement possible : depuis quand, frequence, appetit, eau, comportement, douleur, vomissements/diarrhee, etc.<br><br>"
        "<i>Assistant d'aide au triage : les diagnostics et prescriptions definitifs doivent etre valides par un veterinaire.</i>"
    ).arg(m_utilisateur.toHtmlEscaped(), nomAnimal.toHtmlEscaped()));
    layout->addWidget(conversation, 1);

    QTextEdit *saisie = new QTextEdit(dlg);
    saisie->setPlaceholderText("Exemple : Il vomit depuis ce matin, 3 fois, il ne mange plus mais il boit encore...");
    saisie->setMaximumHeight(95);
    layout->addWidget(saisie);

    QHBoxLayout *actions = new QHBoxLayout();
    QPushButton *fermer = new QPushButton("Fermer", dlg);
    QPushButton *envoyer = new QPushButton("Analyser les symptomes", dlg);
    actions->addWidget(fermer);
    actions->addStretch();
    actions->addWidget(envoyer);
    layout->addLayout(actions);

    connect(fermer, &QPushButton::clicked, dlg, &QDialog::close);
    connect(envoyer, &QPushButton::clicked, dlg, [this, saisie, conversation, espece, nomAnimal]() {
        QString question = saisie->toPlainText().trimmed();
        if (question.isEmpty()) return;
        conversation->append(QString("<p><b>%1 :</b> %2</p>")
            .arg(m_utilisateur.toHtmlEscaped(), question.toHtmlEscaped()));
        QString rep = analyserSymptomes(question, espece, nomAnimal);
        conversation->append(QString("<p><b>PetNova Assistant :</b><br>%1</p>")
            .arg(rep.toHtmlEscaped().replace("\n", "<br>")));
        saisie->clear();
    });

    dlg->show();
}

void GestionAnimaux::afficherRappelsSante()
{
    QStringList aSuivre;
    for (int row = 0; row < ui->tableWidgetAnimaux->rowCount(); ++row) {
        QTableWidgetItem *statut = ui->tableWidgetAnimaux->item(row, ColSuivi);
        QTableWidgetItem *nom = ui->tableWidgetAnimaux->item(row, ColNom);
        if (statut && nom && statut->text().compare("A jour", Qt::CaseInsensitive) != 0) {
            aSuivre << QString("%1 (%2)").arg(nom->text(), statut->text());
        }
    }

    if (aSuivre.isEmpty()) {
        QMessageBox::information(m_fenetre, "Rappels de sante",
            "Aucun rappel en attente : tous les animaux sont a jour.");
    } else {
        QMessageBox::warning(m_fenetre, "Rappels de sante",
            "Animaux necessitant un suivi :\n\n - " + aSuivre.join("\n - "));
    }
}
