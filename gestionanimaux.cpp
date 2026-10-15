#include "gestionanimaux.h"
#include "ui_gsmartpetcare.h"
#include "statistiquesanimaux.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
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
#include <QTableWidgetItem>
#include <QTextDocument>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <initializer_list>

namespace {

enum Colonne { ColId = 0, ColNom, ColEspece, ColSexe, ColNaissance, ColProprietaire,
               ColTelephone, ColSuivi, ColActions, NbColonnes };

const QString kAJour = QString::fromUtf8("À jour");

void selectionnerCombo(QComboBox *combo, const QString &texte)
{
    const int i = combo->findText(texte, Qt::MatchFixedString);   // insensible à la casse
    combo->setCurrentIndex(i >= 0 ? i : 0);
}

QTableWidgetItem *cellule(const QString &texte, bool centre = true)
{
    auto *item = new QTableWidgetItem(texte);
    item->setToolTip(texte);
    const Qt::Alignment alignement = centre ? Qt::Alignment(Qt::AlignCenter)
                                            : Qt::Alignment(Qt::AlignVCenter | Qt::AlignLeft);
    item->setTextAlignment(alignement);
    return item;
}

// ---------------------------------------------------------------------------
// Assistant de triage : analyse de texte par mots-clés (aide au tri, pas un diagnostic)
// ---------------------------------------------------------------------------
QString analyserSymptomes(const QString &question, const QString &espece, const QString &nomAnimal)
{
    // On enlève accents et majuscules : les mots-clés ci-dessous sont écrits sans accent.
    QString q = question.toLower().normalized(QString::NormalizationForm_D);
    q.remove(QRegularExpression(QStringLiteral("[\\x{0300}-\\x{036f}]")));

    const QString animal = nomAnimal.isEmpty() ? QStringLiteral("l'animal") : nomAnimal;
    const QString type = espece.isEmpty() ? QStringLiteral("animal") : espece.toLower();

    auto contient = [&q](std::initializer_list<const char *> mots) {
        for (const char *m : mots)
            if (q.contains(QString::fromUtf8(m)))
                return true;
        return false;
    };

    // TRIAGE : signes graves, prioritaires sur toute hypothèse.
    if (contient({"respire mal", "difficulte a respirer", "etouffe", "convulsion", "convulse", "inconscient",
                  "ne se leve plus", "paralys", "saigne beaucoup", "hemorrag", "ventre gonfle", "empoison",
                  "toxique", "chocolat", "raticide", "antigel"})) {
        return QString::fromUtf8(
            "🚨 URGENCE VÉTÉRINAIRE\n\n"
            "%1 présente un signe potentiellement grave. Ne donnez aucun médicament humain et ne faites pas vomir "
            "l'animal sans instruction vétérinaire. Gardez-le au calme, limitez les manipulations et contactez un "
            "vétérinaire ou un service d'urgence immédiatement.\n\n"
            "Si une substance a été ingérée, conservez l'emballage et notez l'heure et la quantité approximative."
        ).arg(animal);
    }

    if (contient({"vomit", "vomissement", "vomissements"})) {
        return QString::fromUtf8(
            "Hypothèses possibles : irritation digestive, changement alimentaire, parasites, corps étranger ou autre maladie.\n\n"
            "À vérifier : nombre de vomissements, présence de sang, diarrhée, douleur, ingestion d'un objet ou produit, "
            "appétit et hydratation.\n\n"
            "Conduite à tenir : laissez de l'eau accessible en petites quantités et surveillez. Pas de médicament humain. "
            "Si les vomissements sont répétés, avec sang, douleur, grande fatigue, ventre gonflé, impossibilité de garder "
            "l'eau, ou si %1 est très jeune ou âgé, consultation vétérinaire rapide."
        ).arg(animal);
    }

    if (contient({"diarrhee", "selles molles", "sang dans les selles"})) {
        return QString::fromUtf8(
            "Hypothèses possibles : trouble alimentaire, parasites, infection ou inflammation digestive.\n\n"
            "À vérifier : durée, fréquence, sang, vomissements, alimentation récente, vermifugation et état général.\n\n"
            "Conduite à tenir : eau disponible, surveillance de l'hydratation, alimentation habituelle sans friandises ni "
            "nouveaux aliments. Sang abondant, abattement marqué, vomissements associés, déshydratation ou diarrhée "
            "persistante : consultation. Aucun antiparasitaire ou autre médicament ne doit être donné au hasard.");
    }

    if (contient({"ne mange", "mange pas", "perte appetit", "appetit", "anorex"})) {
        const QString urgenceChat = type.contains(QStringLiteral("chat"))
            ? QString::fromUtf8(" Chez le chat, une absence prolongée d'alimentation est particulièrement préoccupante.")
            : QString();
        return QString::fromUtf8(
            "Une perte d'appétit est un symptôme non spécifique : douleur, problème digestif ou dentaire, fièvre, stress "
            "ou autre maladie sont possibles.\n\n"
            "Vérifiez : eau, vomissements/diarrhée, douleur, bouche et dents, changement de comportement.%1\n\n"
            "Ne forcez ni médicament ni nourriture. Si %2 refuse toujours de manger, semble faible, vomit, souffre ou boit "
            "anormalement, faites-le examiner par un vétérinaire."
        ).arg(urgenceChat, animal);
    }

    if (contient({"fievre", "temperature", "chaud"})) {
        return QString::fromUtf8(
            "La sensation au toucher ne suffit pas pour confirmer une fièvre. Une température anormale peut accompagner "
            "une infection, une inflammation ou un autre problème.\n\n"
            "Conduite à tenir : repos, eau disponible, environnement tempéré. Ne donnez jamais paracétamol, ibuprofène ou "
            "aspirine sans prescription vétérinaire : certains sont toxiques, notamment chez le chat. Si l'état général de "
            "%1 est mauvais ou si la température mesurée est anormale ou persistante, consultation vétérinaire."
        ).arg(animal);
    }

    if (contient({"gratte", "demange", "puce", "tiques", "tique", "perd ses poils", "poils"})) {
        return QString::fromUtf8(
            "Hypothèses possibles : puces ou tiques, allergie, irritation cutanée, infection ou parasite de la peau.\n\n"
            "Inspectez la peau et le pelage : rougeurs, plaies, parasites visibles, zones sans poils, localisation et "
            "intensité des démangeaisons. Évitez les produits antiparasitaires non adaptés à l'espèce : certains produits "
            "pour chien sont dangereux pour le chat. Le traitement dépend de la cause et du poids de %1 ; consultation "
            "conseillée en cas de lésions, d'infection, de fortes démangeaisons ou de persistance."
        ).arg(animal);
    }

    if (contient({"boite", "boiterie", "patte", "douleur", "blesse", "plaie"})) {
        return QString::fromUtf8(
            "Une boiterie ou une douleur peut venir d'une plaie, d'une entorse, d'un corps étranger, d'une fracture ou d'un "
            "problème articulaire.\n\n"
            "Conduite à tenir : limitez l'activité, examinez sans forcer l'articulation. Pour une petite plaie superficielle, "
            "rincez doucement à l'eau propre ou au sérum physiologique. Pas d'anti-inflammatoire humain. Appui impossible, "
            "déformation, douleur importante, morsure, plaie profonde ou saignement persistant : vétérinaire rapidement.");
    }

    if (contient({"tousse", "toux", "eternue", "eternuement", "nez coule", "ecoulement"})) {
        return QString::fromUtf8(
            "Hypothèses possibles : irritation, infection respiratoire, allergie ou autre atteinte respiratoire.\n\n"
            "Observez la fréquence, les écoulements nasaux ou oculaires, l'appétit, l'énergie et surtout la respiration au "
            "repos. Gardez %1 au calme et évitez fumée et aérosols. Respiration difficile, bouche ouverte au repos, "
            "coloration bleutée ou pâle, ou aggravation : urgence vétérinaire."
        ).arg(animal);
    }

    if (contient({"oreille", "secoue la tete", "mauvaise odeur"})) {
        return QString::fromUtf8(
            "Une otite, des parasites, un corps étranger ou une irritation sont possibles. Ne mettez ni produit, ni huile, "
            "ni coton-tige profond dans l'oreille sans examen. Douleur, mauvaise odeur, écoulement ou grattage persistant "
            "justifient une consultation : le traitement dépend de l'état du tympan et de la cause.");
    }

    if (contient({"urine", "uriner", "pipi", "n'urine", "sang urine"})) {
        const QString noteChat = type.contains(QStringLiteral("chat"))
            ? QString::fromUtf8(" Chez un chat, surtout mâle, des efforts répétés sans produire d'urine peuvent "
                                "correspondre à une obstruction : urgence immédiate.")
            : QString();
        return QString::fromUtf8(
            "Des changements urinaires peuvent être liés à une infection, une inflammation, des calculs ou une "
            "obstruction.%1\n\n"
            "Notez la fréquence, la quantité, la douleur, le sang et la consommation d'eau. Si %2 essaie d'uriner sans y "
            "arriver, semble douloureux ou abattu : urgence vétérinaire."
        ).arg(noteChat, animal);
    }

    if (contient({"vaccin", "vaccination", "rappel"})) {
        return QString::fromUtf8(
            "Pour %1, le calendrier vaccinal dépend de l'espèce, de l'âge, du mode de vie et des vaccins déjà reçus. "
            "Consultez son dossier de santé et le bouton « Rappels de santé ». Le vétérinaire doit confirmer les vaccins "
            "nécessaires et leurs dates."
        ).arg(animal);
    }

    return QString::fromUtf8(
        "Je n'ai pas assez d'éléments pour proposer une hypothèse utile pour %1.\n\n"
        "Précisez : 1) symptôme principal, 2) depuis quand, 3) fréquence/intensité, 4) appétit et eau, "
        "5) vomissements/diarrhée, 6) comportement/énergie, 7) exposition à un toxique ou traumatisme.\n\n"
        "Je pourrai alors faire un triage plus précis. En cas de respiration difficile, convulsions, inconscience, "
        "hémorragie, intoxication suspectée ou douleur intense, consultez en urgence."
    ).arg(animal);
}

} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
GestionAnimaux::GestionAnimaux(Ui::GSmartPetCare *uiPrincipal, QWidget *fenetre, QObject *parent)
    : QObject(parent)
    , ui(uiPrincipal)
    , m_fenetre(fenetre)
{
    ui->pageAnimaux->setAttribute(Qt::WA_StyledBackground, true);   // fond de la feuille de style

    // --- Tableau ---
    QTableWidget *t = ui->tableWidgetAnimaux;
    QHeaderView *h = t->horizontalHeader();
    h->setMinimumSectionSize(50);
    h->setFixedHeight(44);                                         // même hauteur que les autres pages
    h->setDefaultAlignment(Qt::AlignCenter);
    h->setSectionResizeMode(ColId,           QHeaderView::Fixed);   h->resizeSection(ColId, 70);
    h->setSectionResizeMode(ColNom,          QHeaderView::Stretch);
    h->setSectionResizeMode(ColEspece,       QHeaderView::Fixed);   h->resizeSection(ColEspece, 90);
    h->setSectionResizeMode(ColSexe,         QHeaderView::Fixed);   h->resizeSection(ColSexe, 80);
    h->setSectionResizeMode(ColNaissance,    QHeaderView::Fixed);   h->resizeSection(ColNaissance, 130);
    h->setSectionResizeMode(ColProprietaire, QHeaderView::Stretch);
    h->setSectionResizeMode(ColTelephone,    QHeaderView::Fixed);   h->resizeSection(ColTelephone, 130);
    h->setSectionResizeMode(ColSuivi,        QHeaderView::Fixed);   h->resizeSection(ColSuivi, 170);
    h->setSectionResizeMode(ColActions, QHeaderView::Fixed); h->resizeSection(ColActions, 178);
    h->setStretchLastSection(false);
    t->verticalHeader()->setDefaultSectionSize(42);

    // --- Formulaire ---
    ui->dateEditNaissanceAnimal->setDisplayFormat("dd/MM/yyyy");
    ui->dateEditNaissanceAnimal->setMaximumDate(QDate::currentDate());
    ui->comboBoxSuiviAnimal->clear();
    ui->comboBoxSuiviAnimal->addItems({ kAJour,
                                        QString::fromUtf8("Vaccin à renouveler"),
                                        QString::fromUtf8("Vermifugation à faire"),
                                        QString::fromUtf8("Contrôle à planifier") });

    // --- Connexions ---
    connect(ui->pushButtonEnregistrerAnimal,    &QPushButton::clicked, this, &GestionAnimaux::onEnregistrer);
    connect(ui->pushButtonAnnulerAnimal,        &QPushButton::clicked, this, &GestionAnimaux::onAnnuler);
    connect(ui->pushButtonChoisirPhotoAnimal,   &QPushButton::clicked, this, &GestionAnimaux::onChoisirPhoto);
    connect(ui->pushButtonExporterPdfAnimal,    &QPushButton::clicked, this, &GestionAnimaux::onExporterPdf);
    connect(ui->pushButtonStatistiquesAnimal,   &QPushButton::clicked, this, &GestionAnimaux::onStatistiques);
    connect(ui->pushButtonChatbotAnimal,        &QPushButton::clicked, this, &GestionAnimaux::onChatbot);
    connect(ui->pushButtonRappelsSanteAnimal,   &QPushButton::clicked, this, &GestionAnimaux::onRappelsSante);

    connect(t, &QTableWidget::itemSelectionChanged, this, &GestionAnimaux::onSelectionChangee);
    connect(ui->lineEditRechercheAnimal, &QLineEdit::textChanged, this, &GestionAnimaux::refreshTable);
    connect(ui->comboBoxTriAnimal,    QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GestionAnimaux::refreshTable);
    connect(ui->comboBoxFiltreEspece, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GestionAnimaux::refreshTable);

    chargerExemples();                 // données de démonstration (en mémoire)
    nouveauFormulaire();               // le panneau de droite démarre en mode « Ajouter »
    refreshTable();
}

// Données de démonstration. Les animaux 1, 2 et 3 correspondent à l'historique de démonstration
// du module Services (Rex, Mimi, Coco).
void GestionAnimaux::chargerExemples()
{
    auto ajout = [this](const QString &nom, const QString &espece, const QString &sexe, const QDate &naissance,
                        const QString &proprietaire, const QString &tel, const QString &email,
                        const QString &suivi) {
        FicheAnimal a;
        a.id = m_prochainId++;
        a.nom = nom; a.espece = espece; a.sexe = sexe; a.naissance = naissance;
        a.proprietaire = proprietaire; a.telephone = tel; a.email = email; a.suiviSante = suivi;
        m_animaux.append(a);
    };

    const QString male = QString::fromUtf8("Mâle");
    ajout("Rex",        "Chien",  male,       QDate(2019, 3, 14),  "Ali Ben Salah",   "20 123 456", "ali.bensalah@mail.com",  kAJour);
    ajout("Mimi",       "Chat",   "Femelle",  QDate(2021, 7, 2),   "Sonia Trabelsi",  "22 345 678", "sonia.trabelsi@mail.com", kAJour);
    ajout("Coco",       "Oiseau", male,       QDate(2023, 1, 20),  "Mohamed Gharbi",  "50 987 654", "m.gharbi@mail.com",
          QString::fromUtf8("Contrôle à planifier"));
    ajout("Luna",       "Chat",   "Femelle",  QDate(2024, 5, 9),   "Yasmine Jaziri",  "98 456 123", "yasmine.jaziri@mail.com",
          QString::fromUtf8("Vaccin à renouveler"));
    ajout("Max",        "Chien",  male,       QDate(2016, 11, 30), "Karim Mansouri",  "25 741 852", "karim.mansouri@mail.com", kAJour);
    ajout("Choupette",  "Lapin",  "Femelle",  QDate(2022, 9, 5),   "Nour Hamdi",      "27 159 357", "nour.hamdi@mail.com",
          QString::fromUtf8("Vermifugation à faire"));
}

// ---------------------------------------------------------------------------
// Accès aux données
// ---------------------------------------------------------------------------
QVector<Animal> GestionAnimaux::animaux() const
{
    QVector<Animal> liste;
    for (const FicheAnimal &a : m_animaux) {
        Animal x;
        x.id = a.id;
        x.nom = a.nom;
        x.espece = a.espece;
        x.proprietaire = a.proprietaire;
        liste.append(x);
    }
    return liste;
}

FicheAnimal *GestionAnimaux::trouver(int id)
{
    for (FicheAnimal &a : m_animaux)
        if (a.id == id)
            return &a;
    return nullptr;
}

int GestionAnimaux::prochainId() const
{
    return m_prochainId;
}

QString GestionAnimaux::formaterId(int id)
{
    return QString("A%1").arg(id, 3, 10, QChar('0'));     // A001, A002, ...
}

int GestionAnimaux::idSelectionne() const
{
    const QList<QTableWidgetItem *> sel = ui->tableWidgetAnimaux->selectedItems();
    if (sel.isEmpty())
        return -1;
    const QTableWidgetItem *item = ui->tableWidgetAnimaux->item(sel.first()->row(), ColId);
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

// ---------------------------------------------------------------------------
// Tableau : recherche + filtre par espèce + tri
// ---------------------------------------------------------------------------
QVector<FicheAnimal> GestionAnimaux::vueCourante() const
{
    const QString q = ui->lineEditRechercheAnimal->text().trimmed();
    const QString especeFiltre = ui->comboBoxFiltreEspece->currentIndex() > 0
                                     ? ui->comboBoxFiltreEspece->currentText() : QString();

    QVector<FicheAnimal> vue;
    for (const FicheAnimal &a : m_animaux) {
        if (!especeFiltre.isEmpty() && a.espece.compare(especeFiltre, Qt::CaseInsensitive) != 0)
            continue;
        if (!q.isEmpty()
            && !a.nom.contains(q, Qt::CaseInsensitive)
            && !a.espece.contains(q, Qt::CaseInsensitive)
            && !a.proprietaire.contains(q, Qt::CaseInsensitive)
            && !a.telephone.contains(q, Qt::CaseInsensitive)
            && !a.sexe.contains(q, Qt::CaseInsensitive)
            && !formaterId(a.id).contains(q, Qt::CaseInsensitive))
            continue;
        vue.append(a);
    }

    switch (ui->comboBoxTriAnimal->currentIndex()) {
    case 1:  // Espèce
        std::sort(vue.begin(), vue.end(), [](const FicheAnimal &a, const FicheAnimal &b) {
            const int c = QString::localeAwareCompare(a.espece, b.espece);
            return c != 0 ? c < 0 : QString::localeAwareCompare(a.nom, b.nom) < 0;
        });
        break;
    case 2:  // Date de naissance (du plus âgé au plus jeune)
        std::sort(vue.begin(), vue.end(), [](const FicheAnimal &a, const FicheAnimal &b) {
            return a.naissance < b.naissance;
        });
        break;
    case 3:  // Propriétaire
        std::sort(vue.begin(), vue.end(), [](const FicheAnimal &a, const FicheAnimal &b) {
            const int c = QString::localeAwareCompare(a.proprietaire, b.proprietaire);
            return c != 0 ? c < 0 : QString::localeAwareCompare(a.nom, b.nom) < 0;
        });
        break;
    default: // Nom
        std::sort(vue.begin(), vue.end(), [](const FicheAnimal &a, const FicheAnimal &b) {
            return QString::localeAwareCompare(a.nom, b.nom) < 0;
        });
        break;
    }
    return vue;
}

void GestionAnimaux::refreshTable()
{
    const QVector<FicheAnimal> vue = vueCourante();
    QTableWidget *t = ui->tableWidgetAnimaux;

    {
        const QSignalBlocker blocker(t);       // pas de rechargement du formulaire pendant le remplissage
        t->clearContents();
        t->setRowCount(vue.size());

        int ligneAGarder = -1;
        for (int r = 0; r < vue.size(); ++r) {
            const FicheAnimal &a = vue[r];

            QTableWidgetItem *id = cellule(formaterId(a.id));
            id->setData(Qt::UserRole, a.id);                 // l'id numérique voyage avec la ligne
            t->setItem(r, ColId, id);
            t->setItem(r, ColNom, cellule(a.nom, false));
            t->setItem(r, ColEspece, cellule(a.espece));
            t->setItem(r, ColSexe, cellule(a.sexe));
            t->setItem(r, ColNaissance, cellule(a.naissance.toString("dd/MM/yyyy")));
            t->setItem(r, ColProprietaire, cellule(a.proprietaire, false));
            t->setItem(r, ColTelephone, cellule(a.telephone));

            QTableWidgetItem *suivi = cellule(a.suiviSante);
            const bool aJour = a.suiviSante.compare(kAJour, Qt::CaseInsensitive) == 0;
            suivi->setForeground(aJour ? QColor("#0F766E") : QColor("#B45309"));
            t->setItem(r, ColSuivi, suivi);
            auto *actions = new QWidget(t);
            auto *buttons = new QHBoxLayout(actions);
            buttons->setContentsMargins(2, 2, 2, 2);
            buttons->setSpacing(4);
            auto *modifier = new QPushButton(QString::fromUtf8("Modifier"), actions);
            auto *supprimer = new QPushButton(QString::fromUtf8("Supprimer"), actions);
            modifier->setStyleSheet("QPushButton{background:#2471A3;color:white;border:0;border-radius:5px;padding:5px;} QPushButton:hover{background:#1A5276;}");
            supprimer->setStyleSheet("QPushButton{background:#C0392B;color:white;border:0;border-radius:5px;padding:5px;} QPushButton:hover{background:#A5311F;}");
            buttons->addWidget(modifier);
            buttons->addWidget(supprimer);
            const int idAction = a.id;
            connect(modifier, &QPushButton::clicked, this, [this,idAction]() {
                for (int i=0;i<ui->tableWidgetAnimaux->rowCount();++i) {
                    auto *cell=ui->tableWidgetAnimaux->item(i, ColId);
                    if (cell && cell->data(Qt::UserRole).toInt()==idAction) {
                        ui->tableWidgetAnimaux->selectRow(i);
                        onModifier();
                        return;
                    }
                }
            });
            connect(supprimer, &QPushButton::clicked, this, [this,idAction]() {
                for (int i=0;i<ui->tableWidgetAnimaux->rowCount();++i) {
                    auto *cell=ui->tableWidgetAnimaux->item(i, ColId);
                    if (cell && cell->data(Qt::UserRole).toInt()==idAction) {
                        ui->tableWidgetAnimaux->selectRow(i);
                        onSupprimer();
                        return;
                    }
                }
            });
            t->setCellWidget(r, ColActions, actions);

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
void GestionAnimaux::definirMode(bool modification)
{
    ui->labelDetailTitreAnimal->setText(modification ? QString::fromUtf8("Modifier l'animal")
                                                     : QString::fromUtf8("Ajouter un animal"));
    ui->pushButtonEnregistrerAnimal->setText(modification ? QString::fromUtf8("Mettre à jour")
                                                          : QString::fromUtf8("Enregistrer"));
}

void GestionAnimaux::nouveauFormulaire()
{
    m_idEnModification = -1;

    ui->lineEditIdAnimal->setText(formaterId(prochainId()));
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
    definirMode(false);
}

void GestionAnimaux::remplirFormulaire(const FicheAnimal &a)
{
    ui->lineEditIdAnimal->setText(formaterId(a.id));
    ui->lineEditNomAnimal->setText(a.nom);
    selectionnerCombo(ui->comboBoxEspeceAnimal, a.espece);
    selectionnerCombo(ui->comboBoxSexeAnimal, a.sexe);
    ui->dateEditNaissanceAnimal->setDate(a.naissance.isValid() ? a.naissance : QDate::currentDate());
    ui->lineEditPhotoAnimal->setText(a.photo);
    ui->lineEditProprietaireAnimal->setText(a.proprietaire);
    ui->lineEditTelephoneProprietaireAnimal->setText(a.telephone);
    ui->lineEditEmailProprietaireAnimal->setText(a.email);
    selectionnerCombo(ui->comboBoxSuiviAnimal, a.suiviSante);
}

FicheAnimal GestionAnimaux::lireFormulaire() const
{
    FicheAnimal a;
    a.nom = ui->lineEditNomAnimal->text().trimmed();
    a.espece = ui->comboBoxEspeceAnimal->currentText();
    a.sexe = ui->comboBoxSexeAnimal->currentText();
    a.naissance = ui->dateEditNaissanceAnimal->date();
    a.photo = ui->lineEditPhotoAnimal->text().trimmed();
    a.proprietaire = ui->lineEditProprietaireAnimal->text().trimmed();
    a.telephone = ui->lineEditTelephoneProprietaireAnimal->text().trimmed();
    a.email = ui->lineEditEmailProprietaireAnimal->text().trimmed();
    a.suiviSante = ui->comboBoxSuiviAnimal->currentText();
    return a;
}

bool GestionAnimaux::valider(QString *erreur) const
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
// Boutons principaux
// ---------------------------------------------------------------------------
void GestionAnimaux::onSelectionChangee()
{
    const int id = idSelectionne();
    if (id < 0)
        return;                                   // rien de sélectionné : on ne touche pas au formulaire
    if (const FicheAnimal *a = trouver(id)) {
        m_idEnModification = id;
        remplirFormulaire(*a);
        definirMode(true);
    }
}

// « Ajouter » : prépare un formulaire vide ; si un nouvel animal est déjà saisi, l'enregistre.
void GestionAnimaux::onAjouter()
{
    if (m_idEnModification != -1) {
        nouveauFormulaire();                      // on quitte la modification d'une fiche existante
        ui->lineEditNomAnimal->setFocus();
        return;
    }
    if (ui->lineEditNomAnimal->text().trimmed().isEmpty()) {
        ui->lineEditNomAnimal->setFocus();        // formulaire vide : on invite à le remplir
        return;
    }
    onEnregistrer();
}

// « Modifier » : applique les changements saisis à l'animal sélectionné.
void GestionAnimaux::onModifier()
{
    if (m_idEnModification == -1) {
        QMessageBox::warning(m_fenetre, QString::fromUtf8("Aucune sélection"),
                             QString::fromUtf8("Sélectionnez d'abord un animal dans le tableau pour le modifier."));
        return;
    }
    onEnregistrer();
}

void GestionAnimaux::onEnregistrer()
{
    QString erreur;
    if (!valider(&erreur)) {
        QMessageBox::warning(m_fenetre, "Formulaire incomplet", erreur);
        return;
    }

    FicheAnimal saisie = lireFormulaire();

    if (m_idEnModification == -1) {
        saisie.id = m_prochainId++;
        m_animaux.append(saisie);
        QMessageBox::information(m_fenetre, QString::fromUtf8("Animal ajouté"),
                                 QString::fromUtf8("%1 a été ajouté avec succès.").arg(saisie.nom));
    } else {
        FicheAnimal *existant = trouver(m_idEnModification);
        if (!existant) {                          // ne devrait pas arriver
            nouveauFormulaire();
            refreshTable();
            return;
        }
        saisie.id = existant->id;
        *existant = saisie;
        QMessageBox::information(m_fenetre, QString::fromUtf8("Animal modifié"),
                                 QString::fromUtf8("Les informations de %1 ont été mises à jour.").arg(saisie.nom));
    }

    emit animauxChanges();
    nouveauFormulaire();
    refreshTable();
}

void GestionAnimaux::onSupprimer()
{
    const int id = idSelectionne();
    const FicheAnimal *a = id >= 0 ? trouver(id) : nullptr;
    if (!a) {
        QMessageBox::warning(m_fenetre, QString::fromUtf8("Aucune sélection"),
                             QString::fromUtf8("Sélectionnez d'abord un animal dans le tableau pour le supprimer."));
        return;
    }

    const QString nom = a->nom;
    const auto reponse = QMessageBox::question(
        m_fenetre, "Confirmer la suppression",
        QString::fromUtf8("Voulez-vous vraiment supprimer %1 ?\n\n"
                          "Son historique de services sera aussi supprimé.").arg(nom),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reponse != QMessageBox::Yes)
        return;

    for (int i = 0; i < m_animaux.size(); ++i) {
        if (m_animaux[i].id == id) {
            m_animaux.removeAt(i);
            break;
        }
    }

    emit animalSupprime(id);
    emit animauxChanges();
    nouveauFormulaire();
    refreshTable();
}

// « Annuler » (bouton rouge) : abandonne la saisie ou la modification.
void GestionAnimaux::onAnnuler()
{
    nouveauFormulaire();
}

void GestionAnimaux::onChoisirPhoto()
{
    const QString chemin = QFileDialog::getOpenFileName(
        m_fenetre, "Choisir une photo", QString(), "Images (*.png *.jpg *.jpeg)");
    if (!chemin.isEmpty())
        ui->lineEditPhotoAnimal->setText(chemin);
}

void GestionAnimaux::onHistorique()
{
    emit historiqueDemande(idSelectionne());
}

// ---------------------------------------------------------------------------
// Statistiques
// ---------------------------------------------------------------------------
void GestionAnimaux::onStatistiques()
{
    StatistiquesAnimaux dlg(m_animaux, m_fenetre);
    dlg.exec();
}

// ---------------------------------------------------------------------------
// Rappels de santé
// ---------------------------------------------------------------------------
void GestionAnimaux::onRappelsSante()
{
    QStringList resultats;
    for (const FicheAnimal &a : m_animaux) {
        const QString antecedent = a.suiviSante.trimmed();
        const bool suivi = !antecedent.isEmpty() && antecedent.compare(kAJour, Qt::CaseInsensitive) != 0;
        resultats << QString::fromUtf8("%1 (%2) — %3\n  Antécédent / suivi disponible : %4")
            .arg(a.nom, a.espece, suivi ? QString::fromUtf8("Suivi vétérinaire à envisager")
                                       : QString::fromUtf8("Aucune alerte connue"),
                 antecedent.isEmpty() ? QString::fromUtf8("Non renseigné") : antecedent);
    }
    QMessageBox::information(m_fenetre, QString::fromUtf8("Prédiction selon antécédents médicaux"),
        (resultats.isEmpty() ? QString::fromUtf8("Aucun animal enregistré.") : resultats.join("\n\n"))
        + QString::fromUtf8("\n\nÉvaluation indicative basée uniquement sur le suivi enregistré. "
                            "L'application ne dispose pas encore d'un historique médical détaillé : "
                            "ce résultat n'est pas une prédiction clinique ni un diagnostic vétérinaire."));
}

// ---------------------------------------------------------------------------
// Assistant de triage / diagnostic
// ---------------------------------------------------------------------------
void GestionAnimaux::onChatbot()
{
    const int id = idSelectionne();
    const FicheAnimal *fiche = id >= 0 ? trouver(id) : nullptr;
    if (!fiche) {
        QMessageBox::information(m_fenetre, "Assistant diagnostic",
                                 QString::fromUtf8("Sélectionnez d'abord un animal dans le tableau. L'assistant utilisera "
                                                   "automatiquement sa fiche pour contextualiser l'analyse."));
        return;
    }

    const QString utilisateur = m_utilisateur.trimmed().isEmpty() ? QString::fromUtf8("Employé") : m_utilisateur;
    const QString nomAnimal = fiche->nom;
    const QString espece = fiche->espece;
    const int ans = fiche->naissance.isValid() ? qMax(0, int(fiche->naissance.daysTo(QDate::currentDate()) / 365)) : -1;
    const QString age = ans >= 0 ? QString::fromUtf8("%1 an(s)").arg(ans) : QString::fromUtf8("âge inconnu");

    auto *dlg = new QDialog(m_fenetre);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("PetNova Care Assistant - " + nomAnimal);
    dlg->resize(760, 650);
    dlg->setStyleSheet(
        "QDialog{background:#F4F8F7;}"
        "QLabel#titreChat{font-size:20px;font-weight:700;color:#1F2A37;}"
        "QLabel#ficheChat{background:#E8F5F3;border:1px solid #B8DDD8;border-radius:10px;padding:10px;color:#1F2A37;}"
        "QTextEdit{background:white;border:1px solid #D7E3E1;border-radius:10px;padding:10px;font-size:12px;}"
        "QPushButton{background:#0F766E;color:white;border:none;border-radius:8px;padding:10px 18px;font-weight:700;}"
        "QPushButton:hover{background:#0B5F59;}");

    auto *layout = new QVBoxLayout(dlg);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(10);

    auto *titre = new QLabel(QString::fromUtf8("Assistant de triage & diagnostic"), dlg);
    titre->setObjectName("titreChat");
    layout->addWidget(titre);

    auto *ficheLabel = new QLabel(
        QString::fromUtf8("👤 Utilisateur : %1\n🐾 Patient : %2  •  %3  •  %4  •  %5\n"
                          "Propriétaire : %6  •  Suivi santé : %7")
            .arg(utilisateur, nomAnimal, espece, fiche->sexe, age, fiche->proprietaire, fiche->suiviSante),
        dlg);
    ficheLabel->setObjectName("ficheChat");
    ficheLabel->setWordWrap(true);
    layout->addWidget(ficheLabel);

    auto *conversation = new QTextEdit(dlg);
    conversation->setReadOnly(true);
    conversation->setHtml(QString::fromUtf8(
        "<b>PetNova Assistant :</b> Bonjour %1. Je connais la fiche de <b>%2</b>. "
        "Décrivez ses symptômes le plus précisément possible : depuis quand, fréquence, appétit, eau, comportement, "
        "douleur, vomissements/diarrhée, etc.<br><br>"
        "<i>Assistant d'aide au triage : les diagnostics et prescriptions définitifs doivent être validés par un "
        "vétérinaire.</i>").arg(utilisateur.toHtmlEscaped(), nomAnimal.toHtmlEscaped()));
    layout->addWidget(conversation, 1);

    auto *saisie = new QTextEdit(dlg);
    saisie->setPlaceholderText(QString::fromUtf8("Exemple : Il vomit depuis ce matin, 3 fois, il ne mange plus mais il boit encore..."));
    saisie->setMaximumHeight(95);
    layout->addWidget(saisie);

    auto *actions = new QHBoxLayout;
    auto *fermer = new QPushButton("Fermer", dlg);
    auto *envoyer = new QPushButton("Analyser les symptômes", dlg);
    actions->addWidget(fermer);
    actions->addStretch();
    actions->addWidget(envoyer);
    layout->addLayout(actions);

    connect(fermer, &QPushButton::clicked, dlg, &QDialog::close);
    connect(envoyer, &QPushButton::clicked, dlg, [conversation, saisie, espece, nomAnimal, utilisateur]() {
        const QString question = saisie->toPlainText().trimmed();
        if (question.isEmpty())
            return;
        conversation->append(QString("<p><b>%1 :</b> %2</p>")
                                 .arg(utilisateur.toHtmlEscaped(), question.toHtmlEscaped()));
        const QString reponse = analyserSymptomes(question, espece, nomAnimal);
        conversation->append(QString("<p><b>PetNova Assistant :</b><br>%1</p>")
                                 .arg(reponse.toHtmlEscaped().replace("\n", "<br>")));
        saisie->clear();
    });

    dlg->show();
}

// ---------------------------------------------------------------------------
// Export PDF
// ---------------------------------------------------------------------------
void GestionAnimaux::onExporterPdf()
{
    const QVector<FicheAnimal> lignes = vueCourante();       // ce qui est affiché (recherche, filtre, tri)
    if (lignes.isEmpty()) {
        QMessageBox::information(m_fenetre, "Exporter PDF", QString::fromUtf8("Aucun animal à exporter."));
        return;
    }

    const QString suggestion = QDir::homePath() + "/animaux_" +
                               QDate::currentDate().toString("yyyy-MM-dd") + ".pdf";
    QString chemin = QFileDialog::getSaveFileName(
        m_fenetre, "Exporter la liste des animaux", suggestion, "Fichiers PDF (*.pdf)");
    if (chemin.isEmpty())
        return;                                               // l'utilisateur a annulé
    if (!chemin.endsWith(".pdf", Qt::CaseInsensitive))
        chemin += ".pdf";

    if (!genererPdf(chemin, lignes)) {
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

bool GestionAnimaux::genererPdf(const QString &chemin, const QVector<FicheAnimal> &lignes) const
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
        const FicheAnimal &a = lignes[r];
        const QString fond = (r % 2 == 0) ? "#FFFFFF" : "#EEF3F1";
        const QStringList valeurs = { formaterId(a.id), a.nom, a.espece, a.sexe,
                                      a.naissance.toString("dd/MM/yyyy"), a.proprietaire, a.telephone, a.suiviSante };
        html += "<tr>";
        for (int col = 0; col < valeurs.size(); ++col) {
            QString style = QString("background-color:%1; color:#1F2A37;").arg(fond);
            if (col == ColSuivi) {
                const bool aJour = a.suiviSante.compare(kAJour, Qt::CaseInsensitive) == 0;
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
