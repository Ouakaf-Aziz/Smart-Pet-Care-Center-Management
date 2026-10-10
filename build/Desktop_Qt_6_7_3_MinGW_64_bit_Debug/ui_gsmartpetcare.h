/********************************************************************************
** Form generated from reading UI file 'gsmartpetcare.ui'
**
** Created by: Qt User Interface Compiler version 6.7.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_GSMARTPETCARE_H
#define UI_GSMARTPETCARE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_GSmartPetCare
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayoutMain;
    QStackedWidget *stackedWidget;
    QWidget *pageLogin;
    QVBoxLayout *verticalLayoutLogin;
    QSpacerItem *spacerHaut;
    QHBoxLayout *horizontalLayoutCard;
    QSpacerItem *spacerGauche;
    QFrame *frameCard;
    QVBoxLayout *verticalLayoutCard;
    QLabel *labelLogo;
    QLabel *labelTitre;
    QSpacerItem *spacerTitre;
    QLabel *labelIdentifiant;
    QLineEdit *lineEditIdentifiant;
    QLabel *labelMotDePasse;
    QLineEdit *lineEditMotDePasse;
    QSpacerItem *spacerBouton;
    QPushButton *pushButtonConnexion;
    QLabel *labelInfo;
    QSpacerItem *spacerDroite;
    QSpacerItem *spacerBas;
    QWidget *pageAccueil;
    QVBoxLayout *verticalLayoutAccueilRoot;
    QFrame *frameHeaderAccueil;
    QHBoxLayout *horizontalLayoutHeader;
    QLabel *labelHeaderTitre;
    QLabel *labelHeaderSous;
    QSpacerItem *spacerHeader;
    QHBoxLayout *horizontalLayoutAccueilBody;
    QFrame *frameSidebar;
    QVBoxLayout *verticalLayoutSidebar;
    QPushButton *btnNavAccueil;
    QPushButton *btnNavServices;
    QPushButton *btnNavEmployes;
    QPushButton *btnNavRDV;
    QPushButton *btnNavAnimaux;
    QSpacerItem *spacerSidebar;
    QFrame *lineSidebar;
    QPushButton *btnDeconnexion;
    QStackedWidget *stackContenu;
    QWidget *widgetContenuAccueil;
    QVBoxLayout *verticalLayoutContenu;
    QLabel *labelAccueil;
    QLabel *labelBienvenue;
    QSpacerItem *spacerApresBienvenue;
    QGridLayout *gridLayoutModules;
    QFrame *cardService;
    QHBoxLayout *horizontalLayoutCardService;
    QFrame *accentService;
    QVBoxLayout *verticalLayoutCardServiceBody;
    QLabel *badgeService;
    QLabel *titleService;
    QSpacerItem *spacerCardService;
    QFrame *cardEmployes;
    QHBoxLayout *horizontalLayoutCardEmployes;
    QFrame *accentEmployes;
    QVBoxLayout *verticalLayoutCardEmployesBody;
    QLabel *badgeEmployes;
    QLabel *titleEmployes;
    QSpacerItem *spacerCardEmployes;
    QFrame *cardRDV;
    QHBoxLayout *horizontalLayoutCardRDV;
    QFrame *accentRDV;
    QVBoxLayout *verticalLayoutCardRDVBody;
    QLabel *badgeRDV;
    QLabel *titleRDV;
    QSpacerItem *spacerCardRDV;
    QFrame *cardAnimaux;
    QHBoxLayout *horizontalLayoutCardAnimaux;
    QFrame *accentAnimaux;
    QVBoxLayout *verticalLayoutCardAnimauxBody;
    QLabel *badgeAnimaux;
    QLabel *titleAnimaux;
    QSpacerItem *spacerCardAnimaux;
    QSpacerItem *spacerBasContenu;
    QScrollArea *scrollServices;
    QWidget *pageServices;
    QVBoxLayout *verticalLayoutRoot;
    QHBoxLayout *horizontalLayoutEntete;
    QLabel *labelTitrePage;
    QSpacerItem *spacerEntete;
    QHBoxLayout *horizontalLayoutFiltres;
    QLineEdit *lineEditRecherche;
    QComboBox *comboBoxTri;
    QComboBox *comboBoxFiltreType;
    QHBoxLayout *horizontalLayoutBoutons;
    QPushButton *pushButtonExporterPdf;
    QPushButton *pushButtonHistorique;
    QPushButton *pushButtonStatistiques;
    QSpacerItem *spacerBoutons;
    QHBoxLayout *horizontalLayoutContenu;
    QVBoxLayout *verticalLayoutGauche;
    QTableWidget *tableWidgetServices;
    QFrame *frameDetail;
    QVBoxLayout *verticalLayoutDetail;
    QLabel *labelDetailTitre;
    QHBoxLayout *horizontalLayoutIdPrix;
    QVBoxLayout *verticalLayoutId;
    QLabel *labelChampId;
    QLineEdit *lineEditIdService;
    QVBoxLayout *verticalLayoutPrix;
    QLabel *labelChampPrix;
    QLineEdit *lineEditPrix;
    QLabel *labelChampNomService;
    QLineEdit *lineEditNomService;
    QLabel *labelChampDescription;
    QPlainTextEdit *plainTextEditDescription;
    QHBoxLayout *horizontalLayoutDureeType;
    QVBoxLayout *verticalLayoutDuree;
    QLabel *labelChampDuree;
    QLineEdit *lineEditDuree;
    QVBoxLayout *verticalLayoutType;
    QLabel *labelChampType;
    QComboBox *comboBoxType;
    QCheckBox *checkBoxDisponible;
    QSpacerItem *spacerDetail;
    QHBoxLayout *horizontalLayoutActions;
    QPushButton *pushButtonEnregistrer;
    QPushButton *pushButtonAnnuler;

    void setupUi(QMainWindow *GSmartPetCare)
    {
        if (GSmartPetCare->objectName().isEmpty())
            GSmartPetCare->setObjectName("GSmartPetCare");
        GSmartPetCare->resize(1200, 750);
        GSmartPetCare->setMinimumSize(QSize(1000, 650));
        GSmartPetCare->setStyleSheet(QString::fromUtf8("QWidget {\n"
"    font-family: \"Arial\";\n"
"    font-size: 11pt;\n"
"    color: #1F2A37;\n"
"}\n"
"#centralwidget, #pageLogin, #pageAccueil {\n"
"    background-color: #F4F8F7;\n"
"}\n"
"#frameCard {\n"
"    background-color: #FFFFFF;\n"
"    border: 1px solid #CBD5D3;\n"
"    border-radius: 16px;\n"
"}\n"
"#labelTitre {\n"
"    font-size: 20pt;\n"
"    font-weight: bold;\n"
"    color: #0B4F4A;\n"
"}\n"
"#labelIdentifiant, #labelMotDePasse {\n"
"    font-size: 10pt;\n"
"    color: #6B7A78;\n"
"}\n"
"QLineEdit {\n"
"    background-color: #FFFFFF;\n"
"    border: 1px solid #CBD5D3;\n"
"    border-radius: 6px;\n"
"    padding: 6px 10px;\n"
"}\n"
"QLineEdit:focus {\n"
"    border: 2px solid #0F766E;\n"
"}\n"
"#pushButtonConnexion {\n"
"    background-color: #F28C28;\n"
"    color: #FFFFFF;\n"
"    font-size: 12pt;\n"
"    font-weight: bold;\n"
"    border: none;\n"
"    border-radius: 8px;\n"
"}\n"
"#pushButtonConnexion:hover {\n"
"    background-color: #E07B16;\n"
"}\n"
"#pushButtonConnexion:pressed {\n"
"    "
                        "background-color: #C96A0C;\n"
"}\n"
"#labelInfo {\n"
"    font-size: 9pt;\n"
"    color: #6B7A78;\n"
"}\n"
"#labelAccueil {\n"
"    font-size: 20pt;\n"
"    font-weight: bold;\n"
"    color: #0B4F4A;\n"
"}\n"
"#frameHeaderAccueil, #frameSidebar {\n"
"    background-color: #0F766E;\n"
"}\n"
"#labelHeaderTitre {\n"
"    color: #FFFFFF;\n"
"    font-size: 14pt;\n"
"    font-weight: bold;\n"
"}\n"
"#labelHeaderSous {\n"
"    color: #DCEEEC;\n"
"    font-size: 10pt;\n"
"}\n"
"#btnNavAccueil, #btnNavServices, #btnNavEmployes, #btnNavRDV, #btnNavAnimaux, #btnDeconnexion {\n"
"    background-color: transparent;\n"
"    color: #DCEEEC;\n"
"    font-size: 10pt;\n"
"    font-weight: bold;\n"
"    text-align: left;\n"
"    padding-left: 24px;\n"
"    border: none;\n"
"    border-left: 4px solid transparent;\n"
"}\n"
"#btnNavAccueil[actif=\"true\"], #btnNavServices[actif=\"true\"] {\n"
"    background-color: #0B5B55;\n"
"    color: #FFFFFF;\n"
"    border-left: 4px solid #F28C28;\n"
"}\n"
"#btnNavAccueil:hover, #btnNavServ"
                        "ices:hover, #btnNavEmployes:hover, #btnNavRDV:hover, #btnNavAnimaux:hover:hover, #btnDeconnexion:hover {\n"
"    background-color: #0B5B55;\n"
"    color: #FFFFFF;\n"
"}\n"
"#lineSidebar {\n"
"    background-color: #2E8B84;\n"
"    max-height: 1px;\n"
"    border: none;\n"
"}\n"
"#labelBienvenue {\n"
"    font-size: 11pt;\n"
"    color: #6B7A78;\n"
"}\n"
"#cardService, #cardEmployes, #cardRDV, #cardAnimaux {\n"
"    background-color: #FFFFFF;\n"
"    border: 1px solid #CBD5D3;\n"
"    border-radius: 12px;\n"
"}\n"
"#accentService, #accentEmployes, #accentAnimaux {\n"
"    background-color: #F28C28;\n"
"}\n"
"#accentRDV {\n"
"    background-color: #0F766E;\n"
"}\n"
"#badgeService, #badgeRDV {\n"
"    background-color: #F28C28;\n"
"    color: #FFFFFF;\n"
"    font-size: 16pt;\n"
"    font-weight: bold;\n"
"    border-radius: 22px;\n"
"}\n"
"#badgeEmployes, #badgeAnimaux {\n"
"    background-color: #0F766E;\n"
"    color: #FFFFFF;\n"
"    font-size: 16pt;\n"
"    font-weight: bold;\n"
"    border-radius: 22px;\n"
""
                        "}\n"
"#titleService, #titleEmployes, #titleRDV, #titleAnimaux {\n"
"    font-size: 13pt;\n"
"    font-weight: bold;\n"
"    color: #1F2A37;\n"
"}"));
        centralwidget = new QWidget(GSmartPetCare);
        centralwidget->setObjectName("centralwidget");
        verticalLayoutMain = new QVBoxLayout(centralwidget);
        verticalLayoutMain->setSpacing(0);
        verticalLayoutMain->setObjectName("verticalLayoutMain");
        verticalLayoutMain->setContentsMargins(0, 0, 0, 0);
        stackedWidget = new QStackedWidget(centralwidget);
        stackedWidget->setObjectName("stackedWidget");
        pageLogin = new QWidget();
        pageLogin->setObjectName("pageLogin");
        verticalLayoutLogin = new QVBoxLayout(pageLogin);
        verticalLayoutLogin->setObjectName("verticalLayoutLogin");
        spacerHaut = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayoutLogin->addItem(spacerHaut);

        horizontalLayoutCard = new QHBoxLayout();
        horizontalLayoutCard->setObjectName("horizontalLayoutCard");
        spacerGauche = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayoutCard->addItem(spacerGauche);

        frameCard = new QFrame(pageLogin);
        frameCard->setObjectName("frameCard");
        frameCard->setMinimumSize(QSize(480, 0));
        frameCard->setMaximumSize(QSize(480, 16777215));
        verticalLayoutCard = new QVBoxLayout(frameCard);
        verticalLayoutCard->setSpacing(10);
        verticalLayoutCard->setObjectName("verticalLayoutCard");
        verticalLayoutCard->setContentsMargins(50, 40, 50, 40);
        labelLogo = new QLabel(frameCard);
        labelLogo->setObjectName("labelLogo");
        labelLogo->setMinimumSize(QSize(300, 120));
        labelLogo->setMaximumSize(QSize(300, 120));
        labelLogo->setPixmap(QPixmap(QString::fromUtf8(":/images/logo_app.png")));
        labelLogo->setScaledContents(true);

        verticalLayoutCard->addWidget(labelLogo, 0, Qt::AlignmentFlag::AlignHCenter);

        labelTitre = new QLabel(frameCard);
        labelTitre->setObjectName("labelTitre");
        labelTitre->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayoutCard->addWidget(labelTitre);

        spacerTitre = new QSpacerItem(20, 8, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        verticalLayoutCard->addItem(spacerTitre);

        labelIdentifiant = new QLabel(frameCard);
        labelIdentifiant->setObjectName("labelIdentifiant");

        verticalLayoutCard->addWidget(labelIdentifiant);

        lineEditIdentifiant = new QLineEdit(frameCard);
        lineEditIdentifiant->setObjectName("lineEditIdentifiant");
        lineEditIdentifiant->setMinimumSize(QSize(0, 44));

        verticalLayoutCard->addWidget(lineEditIdentifiant);

        labelMotDePasse = new QLabel(frameCard);
        labelMotDePasse->setObjectName("labelMotDePasse");

        verticalLayoutCard->addWidget(labelMotDePasse);

        lineEditMotDePasse = new QLineEdit(frameCard);
        lineEditMotDePasse->setObjectName("lineEditMotDePasse");
        lineEditMotDePasse->setMinimumSize(QSize(0, 44));
        lineEditMotDePasse->setEchoMode(QLineEdit::EchoMode::Password);

        verticalLayoutCard->addWidget(lineEditMotDePasse);

        spacerBouton = new QSpacerItem(20, 12, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        verticalLayoutCard->addItem(spacerBouton);

        pushButtonConnexion = new QPushButton(frameCard);
        pushButtonConnexion->setObjectName("pushButtonConnexion");
        pushButtonConnexion->setMinimumSize(QSize(0, 48));
        pushButtonConnexion->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));

        verticalLayoutCard->addWidget(pushButtonConnexion);

        labelInfo = new QLabel(frameCard);
        labelInfo->setObjectName("labelInfo");
        labelInfo->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayoutCard->addWidget(labelInfo);


        horizontalLayoutCard->addWidget(frameCard);

        spacerDroite = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayoutCard->addItem(spacerDroite);


        verticalLayoutLogin->addLayout(horizontalLayoutCard);

        spacerBas = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayoutLogin->addItem(spacerBas);

        stackedWidget->addWidget(pageLogin);
        pageAccueil = new QWidget();
        pageAccueil->setObjectName("pageAccueil");
        verticalLayoutAccueilRoot = new QVBoxLayout(pageAccueil);
        verticalLayoutAccueilRoot->setSpacing(0);
        verticalLayoutAccueilRoot->setObjectName("verticalLayoutAccueilRoot");
        verticalLayoutAccueilRoot->setContentsMargins(0, 0, 0, 0);
        frameHeaderAccueil = new QFrame(pageAccueil);
        frameHeaderAccueil->setObjectName("frameHeaderAccueil");
        frameHeaderAccueil->setMinimumSize(QSize(0, 64));
        frameHeaderAccueil->setMaximumSize(QSize(16777215, 64));
        horizontalLayoutHeader = new QHBoxLayout(frameHeaderAccueil);
        horizontalLayoutHeader->setObjectName("horizontalLayoutHeader");
        horizontalLayoutHeader->setContentsMargins(24, 0, 24, 0);
        labelHeaderTitre = new QLabel(frameHeaderAccueil);
        labelHeaderTitre->setObjectName("labelHeaderTitre");

        horizontalLayoutHeader->addWidget(labelHeaderTitre);

        labelHeaderSous = new QLabel(frameHeaderAccueil);
        labelHeaderSous->setObjectName("labelHeaderSous");

        horizontalLayoutHeader->addWidget(labelHeaderSous);

        spacerHeader = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayoutHeader->addItem(spacerHeader);


        verticalLayoutAccueilRoot->addWidget(frameHeaderAccueil);

        horizontalLayoutAccueilBody = new QHBoxLayout();
        horizontalLayoutAccueilBody->setSpacing(0);
        horizontalLayoutAccueilBody->setObjectName("horizontalLayoutAccueilBody");
        frameSidebar = new QFrame(pageAccueil);
        frameSidebar->setObjectName("frameSidebar");
        frameSidebar->setMinimumSize(QSize(240, 0));
        frameSidebar->setMaximumSize(QSize(240, 16777215));
        verticalLayoutSidebar = new QVBoxLayout(frameSidebar);
        verticalLayoutSidebar->setSpacing(2);
        verticalLayoutSidebar->setObjectName("verticalLayoutSidebar");
        verticalLayoutSidebar->setContentsMargins(0, 16, 0, 16);
        btnNavAccueil = new QPushButton(frameSidebar);
        btnNavAccueil->setObjectName("btnNavAccueil");
        btnNavAccueil->setMinimumSize(QSize(0, 44));
        btnNavAccueil->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        btnNavAccueil->setFlat(true);

        verticalLayoutSidebar->addWidget(btnNavAccueil);

        btnNavServices = new QPushButton(frameSidebar);
        btnNavServices->setObjectName("btnNavServices");
        btnNavServices->setMinimumSize(QSize(0, 44));
        btnNavServices->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        btnNavServices->setFlat(true);

        verticalLayoutSidebar->addWidget(btnNavServices);

        btnNavEmployes = new QPushButton(frameSidebar);
        btnNavEmployes->setObjectName("btnNavEmployes");
        btnNavEmployes->setMinimumSize(QSize(0, 44));
        btnNavEmployes->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        btnNavEmployes->setFlat(true);

        verticalLayoutSidebar->addWidget(btnNavEmployes);

        btnNavRDV = new QPushButton(frameSidebar);
        btnNavRDV->setObjectName("btnNavRDV");
        btnNavRDV->setMinimumSize(QSize(0, 44));
        btnNavRDV->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        btnNavRDV->setFlat(true);

        verticalLayoutSidebar->addWidget(btnNavRDV);

        btnNavAnimaux = new QPushButton(frameSidebar);
        btnNavAnimaux->setObjectName("btnNavAnimaux");
        btnNavAnimaux->setMinimumSize(QSize(0, 44));
        btnNavAnimaux->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        btnNavAnimaux->setFlat(true);

        verticalLayoutSidebar->addWidget(btnNavAnimaux);

        spacerSidebar = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayoutSidebar->addItem(spacerSidebar);

        lineSidebar = new QFrame(frameSidebar);
        lineSidebar->setObjectName("lineSidebar");
        lineSidebar->setFrameShape(QFrame::Shape::HLine);
        lineSidebar->setFrameShadow(QFrame::Shadow::Sunken);

        verticalLayoutSidebar->addWidget(lineSidebar);

        btnDeconnexion = new QPushButton(frameSidebar);
        btnDeconnexion->setObjectName("btnDeconnexion");
        btnDeconnexion->setMinimumSize(QSize(0, 44));
        btnDeconnexion->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        btnDeconnexion->setFlat(true);

        verticalLayoutSidebar->addWidget(btnDeconnexion);


        horizontalLayoutAccueilBody->addWidget(frameSidebar);

        stackContenu = new QStackedWidget(pageAccueil);
        stackContenu->setObjectName("stackContenu");
        widgetContenuAccueil = new QWidget();
        widgetContenuAccueil->setObjectName("widgetContenuAccueil");
        verticalLayoutContenu = new QVBoxLayout(widgetContenuAccueil);
        verticalLayoutContenu->setSpacing(16);
        verticalLayoutContenu->setObjectName("verticalLayoutContenu");
        verticalLayoutContenu->setContentsMargins(40, 32, 40, 32);
        labelAccueil = new QLabel(widgetContenuAccueil);
        labelAccueil->setObjectName("labelAccueil");

        verticalLayoutContenu->addWidget(labelAccueil);

        labelBienvenue = new QLabel(widgetContenuAccueil);
        labelBienvenue->setObjectName("labelBienvenue");

        verticalLayoutContenu->addWidget(labelBienvenue);

        spacerApresBienvenue = new QSpacerItem(20, 10, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        verticalLayoutContenu->addItem(spacerApresBienvenue);

        gridLayoutModules = new QGridLayout();
        gridLayoutModules->setObjectName("gridLayoutModules");
        gridLayoutModules->setHorizontalSpacing(20);
        gridLayoutModules->setVerticalSpacing(20);
        cardService = new QFrame(widgetContenuAccueil);
        cardService->setObjectName("cardService");
        cardService->setMinimumSize(QSize(0, 140));
        horizontalLayoutCardService = new QHBoxLayout(cardService);
        horizontalLayoutCardService->setSpacing(0);
        horizontalLayoutCardService->setObjectName("horizontalLayoutCardService");
        horizontalLayoutCardService->setContentsMargins(0, 0, 0, 0);
        accentService = new QFrame(cardService);
        accentService->setObjectName("accentService");
        accentService->setMinimumSize(QSize(4, 0));
        accentService->setMaximumSize(QSize(4, 16777215));

        horizontalLayoutCardService->addWidget(accentService);

        verticalLayoutCardServiceBody = new QVBoxLayout();
        verticalLayoutCardServiceBody->setSpacing(10);
        verticalLayoutCardServiceBody->setObjectName("verticalLayoutCardServiceBody");
        verticalLayoutCardServiceBody->setContentsMargins(20, 20, 20, 20);
        badgeService = new QLabel(cardService);
        badgeService->setObjectName("badgeService");
        badgeService->setMinimumSize(QSize(44, 44));
        badgeService->setMaximumSize(QSize(44, 44));
        badgeService->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayoutCardServiceBody->addWidget(badgeService);

        titleService = new QLabel(cardService);
        titleService->setObjectName("titleService");

        verticalLayoutCardServiceBody->addWidget(titleService);

        spacerCardService = new QSpacerItem(20, 10, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayoutCardServiceBody->addItem(spacerCardService);


        horizontalLayoutCardService->addLayout(verticalLayoutCardServiceBody);


        gridLayoutModules->addWidget(cardService, 0, 0, 1, 1);

        cardEmployes = new QFrame(widgetContenuAccueil);
        cardEmployes->setObjectName("cardEmployes");
        cardEmployes->setMinimumSize(QSize(0, 140));
        horizontalLayoutCardEmployes = new QHBoxLayout(cardEmployes);
        horizontalLayoutCardEmployes->setSpacing(0);
        horizontalLayoutCardEmployes->setObjectName("horizontalLayoutCardEmployes");
        horizontalLayoutCardEmployes->setContentsMargins(0, 0, 0, 0);
        accentEmployes = new QFrame(cardEmployes);
        accentEmployes->setObjectName("accentEmployes");
        accentEmployes->setMinimumSize(QSize(4, 0));
        accentEmployes->setMaximumSize(QSize(4, 16777215));

        horizontalLayoutCardEmployes->addWidget(accentEmployes);

        verticalLayoutCardEmployesBody = new QVBoxLayout();
        verticalLayoutCardEmployesBody->setSpacing(10);
        verticalLayoutCardEmployesBody->setObjectName("verticalLayoutCardEmployesBody");
        verticalLayoutCardEmployesBody->setContentsMargins(20, 20, 20, 20);
        badgeEmployes = new QLabel(cardEmployes);
        badgeEmployes->setObjectName("badgeEmployes");
        badgeEmployes->setMinimumSize(QSize(44, 44));
        badgeEmployes->setMaximumSize(QSize(44, 44));
        badgeEmployes->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayoutCardEmployesBody->addWidget(badgeEmployes);

        titleEmployes = new QLabel(cardEmployes);
        titleEmployes->setObjectName("titleEmployes");

        verticalLayoutCardEmployesBody->addWidget(titleEmployes);

        spacerCardEmployes = new QSpacerItem(20, 10, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayoutCardEmployesBody->addItem(spacerCardEmployes);


        horizontalLayoutCardEmployes->addLayout(verticalLayoutCardEmployesBody);


        gridLayoutModules->addWidget(cardEmployes, 0, 1, 1, 1);

        cardRDV = new QFrame(widgetContenuAccueil);
        cardRDV->setObjectName("cardRDV");
        cardRDV->setMinimumSize(QSize(0, 140));
        horizontalLayoutCardRDV = new QHBoxLayout(cardRDV);
        horizontalLayoutCardRDV->setSpacing(0);
        horizontalLayoutCardRDV->setObjectName("horizontalLayoutCardRDV");
        horizontalLayoutCardRDV->setContentsMargins(0, 0, 0, 0);
        accentRDV = new QFrame(cardRDV);
        accentRDV->setObjectName("accentRDV");
        accentRDV->setMinimumSize(QSize(4, 0));
        accentRDV->setMaximumSize(QSize(4, 16777215));

        horizontalLayoutCardRDV->addWidget(accentRDV);

        verticalLayoutCardRDVBody = new QVBoxLayout();
        verticalLayoutCardRDVBody->setSpacing(10);
        verticalLayoutCardRDVBody->setObjectName("verticalLayoutCardRDVBody");
        verticalLayoutCardRDVBody->setContentsMargins(20, 20, 20, 20);
        badgeRDV = new QLabel(cardRDV);
        badgeRDV->setObjectName("badgeRDV");
        badgeRDV->setMinimumSize(QSize(44, 44));
        badgeRDV->setMaximumSize(QSize(44, 44));
        badgeRDV->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayoutCardRDVBody->addWidget(badgeRDV);

        titleRDV = new QLabel(cardRDV);
        titleRDV->setObjectName("titleRDV");

        verticalLayoutCardRDVBody->addWidget(titleRDV);

        spacerCardRDV = new QSpacerItem(20, 10, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayoutCardRDVBody->addItem(spacerCardRDV);


        horizontalLayoutCardRDV->addLayout(verticalLayoutCardRDVBody);


        gridLayoutModules->addWidget(cardRDV, 0, 2, 1, 1);

        cardAnimaux = new QFrame(widgetContenuAccueil);
        cardAnimaux->setObjectName("cardAnimaux");
        cardAnimaux->setMinimumSize(QSize(0, 140));
        horizontalLayoutCardAnimaux = new QHBoxLayout(cardAnimaux);
        horizontalLayoutCardAnimaux->setSpacing(0);
        horizontalLayoutCardAnimaux->setObjectName("horizontalLayoutCardAnimaux");
        horizontalLayoutCardAnimaux->setContentsMargins(0, 0, 0, 0);
        accentAnimaux = new QFrame(cardAnimaux);
        accentAnimaux->setObjectName("accentAnimaux");
        accentAnimaux->setMinimumSize(QSize(4, 0));
        accentAnimaux->setMaximumSize(QSize(4, 16777215));

        horizontalLayoutCardAnimaux->addWidget(accentAnimaux);

        verticalLayoutCardAnimauxBody = new QVBoxLayout();
        verticalLayoutCardAnimauxBody->setSpacing(10);
        verticalLayoutCardAnimauxBody->setObjectName("verticalLayoutCardAnimauxBody");
        verticalLayoutCardAnimauxBody->setContentsMargins(20, 20, 20, 20);
        badgeAnimaux = new QLabel(cardAnimaux);
        badgeAnimaux->setObjectName("badgeAnimaux");
        badgeAnimaux->setMinimumSize(QSize(44, 44));
        badgeAnimaux->setMaximumSize(QSize(44, 44));
        badgeAnimaux->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayoutCardAnimauxBody->addWidget(badgeAnimaux);

        titleAnimaux = new QLabel(cardAnimaux);
        titleAnimaux->setObjectName("titleAnimaux");

        verticalLayoutCardAnimauxBody->addWidget(titleAnimaux);

        spacerCardAnimaux = new QSpacerItem(20, 10, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayoutCardAnimauxBody->addItem(spacerCardAnimaux);


        horizontalLayoutCardAnimaux->addLayout(verticalLayoutCardAnimauxBody);


        gridLayoutModules->addWidget(cardAnimaux, 1, 0, 1, 1);


        verticalLayoutContenu->addLayout(gridLayoutModules);

        spacerBasContenu = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayoutContenu->addItem(spacerBasContenu);

        stackContenu->addWidget(widgetContenuAccueil);
        scrollServices = new QScrollArea();
        scrollServices->setObjectName("scrollServices");
        scrollServices->setFrameShape(QFrame::Shape::NoFrame);
        scrollServices->setWidgetResizable(true);
        pageServices = new QWidget();
        pageServices->setObjectName("pageServices");
        pageServices->setMinimumSize(QSize(1000, 0));
        pageServices->setStyleSheet(QString::fromUtf8("QWidget {\n"
"            font-family: \"Arial\";\n"
"            font-size: 11pt;\n"
"            color: #1F2A37;\n"
"        }\n"
"        #pageServices {\n"
"            background-color: #F4F8F7;\n"
"        }\n"
"        #labelTitrePage {\n"
"            font-size: 20pt;\n"
"            font-weight: bold;\n"
"            color: #0B4F4A;\n"
"        }\n"
"        QLineEdit, QComboBox, QPlainTextEdit {\n"
"            background-color: #FFFFFF;\n"
"            border: 1px solid #CBD5D3;\n"
"            border-radius: 6px;\n"
"            padding: 5px 8px;\n"
"        }\n"
"        QLineEdit:focus, QComboBox:focus, QPlainTextEdit:focus {\n"
"            border: 2px solid #0F766E;\n"
"        }\n"
"        QLineEdit:read-only {\n"
"            background-color: #EAF2F1;\n"
"            color: #5C6B69;\n"
"        }\n"
"        QComboBox::drop-down {\n"
"            border: none;\n"
"            width: 24px;\n"
"        }\n"
"        QComboBox QAbstractItemView {\n"
"            background-color: #FFFFFF;\n"
""
                        "            border: 1px solid #CBD5D3;\n"
"            selection-background-color: #CDEAE6;\n"
"            selection-color: #1F2A37;\n"
"            outline: none;\n"
"        }\n"
"        QCheckBox {\n"
"            spacing: 8px;\n"
"        }\n"
"        QCheckBox::indicator {\n"
"            width: 18px;\n"
"            height: 18px;\n"
"            border: 1px solid #CBD5D3;\n"
"            border-radius: 4px;\n"
"            background-color: #FFFFFF;\n"
"        }\n"
"        QCheckBox::indicator:checked {\n"
"            background-color: #0F766E;\n"
"            border: 1px solid #0F766E;\n"
"        }\n"
"        QScrollBar:vertical {\n"
"            background: #F4F8F7;\n"
"            width: 10px;\n"
"            margin: 2px;\n"
"        }\n"
"        QScrollBar::handle:vertical {\n"
"            background: #CBD5D3;\n"
"            border-radius: 5px;\n"
"            min-height: 24px;\n"
"        }\n"
"        QScrollBar::handle:vertical:hover {\n"
"            background: #9AA6A4;\n"
"        }\n"
""
                        "        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {\n"
"            height: 0px;\n"
"        }\n"
"        #btnModifierLigne {\n"
"            background-color: #2F6FB3;\n"
"            color: #FFFFFF;\n"
"            font-size: 10pt;\n"
"            font-weight: bold;\n"
"            border: none;\n"
"            border-radius: 6px;\n"
"            padding: 0 12px;\n"
"        }\n"
"        #btnModifierLigne:hover { background-color: #255A93; }\n"
"        #btnSupprimerLigne {\n"
"            background-color: #C0392B;\n"
"            color: #FFFFFF;\n"
"            font-size: 10pt;\n"
"            font-weight: bold;\n"
"            border: none;\n"
"            border-radius: 6px;\n"
"            padding: 0 12px;\n"
"        }\n"
"        #btnSupprimerLigne:hover { background-color: #A5311F; }\n"
"        #pushButtonStatistiques {\n"
"            background-color: #F28C28;\n"
"            color: #FFFFFF;\n"
"            font-weight: bold;\n"
"            border: none;\n"
"            borde"
                        "r-radius: 6px;\n"
"        }\n"
"        #pushButtonStatistiques:hover { background-color: #E07B16; }\n"
"        #pushButtonExporterPdf {\n"
"            background-color: #0B4F4A;\n"
"            color: #FFFFFF;\n"
"            font-weight: bold;\n"
"            border: none;\n"
"            border-radius: 6px;\n"
"        }\n"
"        #pushButtonExporterPdf:hover { background-color: #083E3A; }\n"
"        #pushButtonEnregistrer {\n"
"            background-color: #0F766E;\n"
"            color: #FFFFFF;\n"
"            font-size: 11pt;\n"
"            font-weight: bold;\n"
"            border: none;\n"
"            border-radius: 6px;\n"
"        }\n"
"        #pushButtonEnregistrer:hover { background-color: #0C5F58; }\n"
"        #pushButtonAnnuler {\n"
"            background-color: #C0392B;\n"
"            color: #FFFFFF;\n"
"            font-size: 11pt;\n"
"            font-weight: bold;\n"
"            border: none;\n"
"            border-radius: 6px;\n"
"        }\n"
"        #pushButtonAnnuler:hover"
                        " { background-color: #A5311F; }\n"
"        #pushButtonHistorique {\n"
"            background-color: #2F6FB3;\n"
"            color: #FFFFFF;\n"
"            font-weight: bold;\n"
"            border: none;\n"
"            border-radius: 6px;\n"
"        }\n"
"        #pushButtonHistorique:hover { background-color: #255A93; }\n"
"        #frameDetail {\n"
"            background-color: #FFFFFF;\n"
"            border: 1px solid #CBD5D3;\n"
"            border-radius: 12px;\n"
"        }\n"
"        #labelDetailTitre {\n"
"            font-size: 13pt;\n"
"            font-weight: bold;\n"
"            color: #0B4F4A;\n"
"        }\n"
"        #labelChampId, #labelChampPrix, #labelChampNomService, #labelChampDescription, #labelChampDuree, #labelChampType {\n"
"            font-size: 9pt;\n"
"            font-weight: bold;\n"
"            color: #6B7A78;\n"
"        }\n"
"        QTableWidget {\n"
"            background-color: #FFFFFF;\n"
"            border: 1px solid #CBD5D3;\n"
"            border-radius: 8p"
                        "x;\n"
"            gridline-color: #E3EAE9;\n"
"        }\n"
"        QHeaderView::section {\n"
"            background-color: #0F766E;\n"
"            color: #FFFFFF;\n"
"            font-weight: bold;\n"
"            padding: 6px 4px;\n"
"            border: none;\n"
"        }\n"
"        QTableWidget::item {\n"
"            padding: 4px;\n"
"        }\n"
"        QTableWidget::item:selected {\n"
"            background-color: #CDEAE6;\n"
"            color: #1F2A37;\n"
"        }"));
        verticalLayoutRoot = new QVBoxLayout(pageServices);
        verticalLayoutRoot->setSpacing(14);
        verticalLayoutRoot->setObjectName("verticalLayoutRoot");
        verticalLayoutRoot->setContentsMargins(28, 24, 28, 24);
        horizontalLayoutEntete = new QHBoxLayout();
        horizontalLayoutEntete->setSpacing(18);
        horizontalLayoutEntete->setObjectName("horizontalLayoutEntete");
        labelTitrePage = new QLabel(pageServices);
        labelTitrePage->setObjectName("labelTitrePage");

        horizontalLayoutEntete->addWidget(labelTitrePage);

        spacerEntete = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayoutEntete->addItem(spacerEntete);


        verticalLayoutRoot->addLayout(horizontalLayoutEntete);

        horizontalLayoutFiltres = new QHBoxLayout();
        horizontalLayoutFiltres->setSpacing(12);
        horizontalLayoutFiltres->setObjectName("horizontalLayoutFiltres");
        lineEditRecherche = new QLineEdit(pageServices);
        lineEditRecherche->setObjectName("lineEditRecherche");
        lineEditRecherche->setMinimumSize(QSize(0, 38));

        horizontalLayoutFiltres->addWidget(lineEditRecherche);

        comboBoxTri = new QComboBox(pageServices);
        comboBoxTri->addItem(QString());
        comboBoxTri->addItem(QString());
        comboBoxTri->addItem(QString());
        comboBoxTri->addItem(QString());
        comboBoxTri->setObjectName("comboBoxTri");
        comboBoxTri->setMinimumSize(QSize(170, 38));

        horizontalLayoutFiltres->addWidget(comboBoxTri);

        comboBoxFiltreType = new QComboBox(pageServices);
        comboBoxFiltreType->addItem(QString());
        comboBoxFiltreType->addItem(QString());
        comboBoxFiltreType->addItem(QString());
        comboBoxFiltreType->addItem(QString());
        comboBoxFiltreType->addItem(QString());
        comboBoxFiltreType->addItem(QString());
        comboBoxFiltreType->addItem(QString());
        comboBoxFiltreType->setObjectName("comboBoxFiltreType");
        comboBoxFiltreType->setMinimumSize(QSize(170, 38));

        horizontalLayoutFiltres->addWidget(comboBoxFiltreType);


        verticalLayoutRoot->addLayout(horizontalLayoutFiltres);

        horizontalLayoutBoutons = new QHBoxLayout();
        horizontalLayoutBoutons->setSpacing(10);
        horizontalLayoutBoutons->setObjectName("horizontalLayoutBoutons");
        pushButtonExporterPdf = new QPushButton(pageServices);
        pushButtonExporterPdf->setObjectName("pushButtonExporterPdf");
        pushButtonExporterPdf->setMinimumSize(QSize(130, 40));
        pushButtonExporterPdf->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));

        horizontalLayoutBoutons->addWidget(pushButtonExporterPdf);

        pushButtonHistorique = new QPushButton(pageServices);
        pushButtonHistorique->setObjectName("pushButtonHistorique");
        pushButtonHistorique->setMinimumSize(QSize(130, 40));
        pushButtonHistorique->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));

        horizontalLayoutBoutons->addWidget(pushButtonHistorique);

        pushButtonStatistiques = new QPushButton(pageServices);
        pushButtonStatistiques->setObjectName("pushButtonStatistiques");
        pushButtonStatistiques->setMinimumSize(QSize(130, 40));
        pushButtonStatistiques->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));

        horizontalLayoutBoutons->addWidget(pushButtonStatistiques);

        spacerBoutons = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayoutBoutons->addItem(spacerBoutons);


        verticalLayoutRoot->addLayout(horizontalLayoutBoutons);

        horizontalLayoutContenu = new QHBoxLayout();
        horizontalLayoutContenu->setSpacing(18);
        horizontalLayoutContenu->setObjectName("horizontalLayoutContenu");
        verticalLayoutGauche = new QVBoxLayout();
        verticalLayoutGauche->setSpacing(14);
        verticalLayoutGauche->setObjectName("verticalLayoutGauche");
        tableWidgetServices = new QTableWidget(pageServices);
        if (tableWidgetServices->columnCount() < 7)
            tableWidgetServices->setColumnCount(7);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        tableWidgetServices->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        tableWidgetServices->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        tableWidgetServices->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        tableWidgetServices->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
        tableWidgetServices->setHorizontalHeaderItem(4, __qtablewidgetitem4);
        QTableWidgetItem *__qtablewidgetitem5 = new QTableWidgetItem();
        tableWidgetServices->setHorizontalHeaderItem(5, __qtablewidgetitem5);
        QTableWidgetItem *__qtablewidgetitem6 = new QTableWidgetItem();
        tableWidgetServices->setHorizontalHeaderItem(6, __qtablewidgetitem6);
        tableWidgetServices->setObjectName("tableWidgetServices");
        tableWidgetServices->setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);
        tableWidgetServices->setAlternatingRowColors(true);
        tableWidgetServices->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
        tableWidgetServices->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
        tableWidgetServices->setShowGrid(true);
        tableWidgetServices->setSortingEnabled(false);
        tableWidgetServices->horizontalHeader()->setMinimumSectionSize(50);
        tableWidgetServices->horizontalHeader()->setDefaultSectionSize(120);
        tableWidgetServices->horizontalHeader()->setStretchLastSection(true);
        tableWidgetServices->verticalHeader()->setVisible(false);
        tableWidgetServices->verticalHeader()->setDefaultSectionSize(46);

        verticalLayoutGauche->addWidget(tableWidgetServices);


        horizontalLayoutContenu->addLayout(verticalLayoutGauche);

        frameDetail = new QFrame(pageServices);
        frameDetail->setObjectName("frameDetail");
        frameDetail->setMinimumSize(QSize(320, 0));
        frameDetail->setMaximumSize(QSize(340, 16777215));
        verticalLayoutDetail = new QVBoxLayout(frameDetail);
        verticalLayoutDetail->setSpacing(8);
        verticalLayoutDetail->setObjectName("verticalLayoutDetail");
        verticalLayoutDetail->setContentsMargins(18, 18, 18, 18);
        labelDetailTitre = new QLabel(frameDetail);
        labelDetailTitre->setObjectName("labelDetailTitre");

        verticalLayoutDetail->addWidget(labelDetailTitre);

        horizontalLayoutIdPrix = new QHBoxLayout();
        horizontalLayoutIdPrix->setSpacing(10);
        horizontalLayoutIdPrix->setObjectName("horizontalLayoutIdPrix");
        verticalLayoutId = new QVBoxLayout();
        verticalLayoutId->setSpacing(2);
        verticalLayoutId->setObjectName("verticalLayoutId");
        labelChampId = new QLabel(frameDetail);
        labelChampId->setObjectName("labelChampId");

        verticalLayoutId->addWidget(labelChampId);

        lineEditIdService = new QLineEdit(frameDetail);
        lineEditIdService->setObjectName("lineEditIdService");
        lineEditIdService->setMinimumSize(QSize(0, 36));
        lineEditIdService->setReadOnly(true);

        verticalLayoutId->addWidget(lineEditIdService);


        horizontalLayoutIdPrix->addLayout(verticalLayoutId);

        verticalLayoutPrix = new QVBoxLayout();
        verticalLayoutPrix->setSpacing(2);
        verticalLayoutPrix->setObjectName("verticalLayoutPrix");
        labelChampPrix = new QLabel(frameDetail);
        labelChampPrix->setObjectName("labelChampPrix");

        verticalLayoutPrix->addWidget(labelChampPrix);

        lineEditPrix = new QLineEdit(frameDetail);
        lineEditPrix->setObjectName("lineEditPrix");
        lineEditPrix->setMinimumSize(QSize(0, 36));

        verticalLayoutPrix->addWidget(lineEditPrix);


        horizontalLayoutIdPrix->addLayout(verticalLayoutPrix);


        verticalLayoutDetail->addLayout(horizontalLayoutIdPrix);

        labelChampNomService = new QLabel(frameDetail);
        labelChampNomService->setObjectName("labelChampNomService");
        labelChampNomService->setMinimumSize(QSize(0, 16));
        labelChampNomService->setWordWrap(false);

        verticalLayoutDetail->addWidget(labelChampNomService);

        lineEditNomService = new QLineEdit(frameDetail);
        lineEditNomService->setObjectName("lineEditNomService");
        lineEditNomService->setMinimumSize(QSize(0, 36));

        verticalLayoutDetail->addWidget(lineEditNomService);

        labelChampDescription = new QLabel(frameDetail);
        labelChampDescription->setObjectName("labelChampDescription");

        verticalLayoutDetail->addWidget(labelChampDescription);

        plainTextEditDescription = new QPlainTextEdit(frameDetail);
        plainTextEditDescription->setObjectName("plainTextEditDescription");
        plainTextEditDescription->setMinimumSize(QSize(0, 70));
        plainTextEditDescription->setMaximumSize(QSize(16777215, 80));

        verticalLayoutDetail->addWidget(plainTextEditDescription);

        horizontalLayoutDureeType = new QHBoxLayout();
        horizontalLayoutDureeType->setSpacing(10);
        horizontalLayoutDureeType->setObjectName("horizontalLayoutDureeType");
        verticalLayoutDuree = new QVBoxLayout();
        verticalLayoutDuree->setSpacing(2);
        verticalLayoutDuree->setObjectName("verticalLayoutDuree");
        labelChampDuree = new QLabel(frameDetail);
        labelChampDuree->setObjectName("labelChampDuree");

        verticalLayoutDuree->addWidget(labelChampDuree);

        lineEditDuree = new QLineEdit(frameDetail);
        lineEditDuree->setObjectName("lineEditDuree");
        lineEditDuree->setMinimumSize(QSize(0, 36));

        verticalLayoutDuree->addWidget(lineEditDuree);


        horizontalLayoutDureeType->addLayout(verticalLayoutDuree);

        verticalLayoutType = new QVBoxLayout();
        verticalLayoutType->setSpacing(2);
        verticalLayoutType->setObjectName("verticalLayoutType");
        labelChampType = new QLabel(frameDetail);
        labelChampType->setObjectName("labelChampType");

        verticalLayoutType->addWidget(labelChampType);

        comboBoxType = new QComboBox(frameDetail);
        comboBoxType->addItem(QString());
        comboBoxType->addItem(QString());
        comboBoxType->addItem(QString());
        comboBoxType->addItem(QString());
        comboBoxType->addItem(QString());
        comboBoxType->addItem(QString());
        comboBoxType->setObjectName("comboBoxType");
        comboBoxType->setMinimumSize(QSize(0, 36));

        verticalLayoutType->addWidget(comboBoxType);


        horizontalLayoutDureeType->addLayout(verticalLayoutType);


        verticalLayoutDetail->addLayout(horizontalLayoutDureeType);

        checkBoxDisponible = new QCheckBox(frameDetail);
        checkBoxDisponible->setObjectName("checkBoxDisponible");
        checkBoxDisponible->setChecked(true);

        verticalLayoutDetail->addWidget(checkBoxDisponible);

        spacerDetail = new QSpacerItem(20, 20, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayoutDetail->addItem(spacerDetail);

        horizontalLayoutActions = new QHBoxLayout();
        horizontalLayoutActions->setSpacing(10);
        horizontalLayoutActions->setObjectName("horizontalLayoutActions");
        pushButtonEnregistrer = new QPushButton(frameDetail);
        pushButtonEnregistrer->setObjectName("pushButtonEnregistrer");
        pushButtonEnregistrer->setMinimumSize(QSize(0, 42));
        pushButtonEnregistrer->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));

        horizontalLayoutActions->addWidget(pushButtonEnregistrer);

        pushButtonAnnuler = new QPushButton(frameDetail);
        pushButtonAnnuler->setObjectName("pushButtonAnnuler");
        pushButtonAnnuler->setMinimumSize(QSize(0, 42));
        pushButtonAnnuler->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));

        horizontalLayoutActions->addWidget(pushButtonAnnuler);


        verticalLayoutDetail->addLayout(horizontalLayoutActions);


        horizontalLayoutContenu->addWidget(frameDetail);


        verticalLayoutRoot->addLayout(horizontalLayoutContenu);

        scrollServices->setWidget(pageServices);
        stackContenu->addWidget(scrollServices);

        horizontalLayoutAccueilBody->addWidget(stackContenu);


        verticalLayoutAccueilRoot->addLayout(horizontalLayoutAccueilBody);

        stackedWidget->addWidget(pageAccueil);

        verticalLayoutMain->addWidget(stackedWidget);

        GSmartPetCare->setCentralWidget(centralwidget);

        retranslateUi(GSmartPetCare);

        stackedWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(GSmartPetCare);
    } // setupUi

    void retranslateUi(QMainWindow *GSmartPetCare)
    {
        GSmartPetCare->setWindowTitle(QCoreApplication::translate("GSmartPetCare", "Smart Pet Care Center Management", nullptr));
        labelLogo->setText(QString());
        labelTitre->setText(QCoreApplication::translate("GSmartPetCare", "Connexion", nullptr));
        labelIdentifiant->setText(QCoreApplication::translate("GSmartPetCare", "Identifiant (employ\303\251)", nullptr));
        lineEditIdentifiant->setPlaceholderText(QCoreApplication::translate("GSmartPetCare", "Saisissez votre identifiant", nullptr));
        labelMotDePasse->setText(QCoreApplication::translate("GSmartPetCare", "Mot de passe", nullptr));
        lineEditMotDePasse->setPlaceholderText(QCoreApplication::translate("GSmartPetCare", "Saisissez votre mot de passe", nullptr));
        pushButtonConnexion->setText(QCoreApplication::translate("GSmartPetCare", "Se connecter", nullptr));
        labelInfo->setText(QCoreApplication::translate("GSmartPetCare", "Acc\303\250s r\303\251serv\303\251 aux employ\303\251s du centre", nullptr));
        labelHeaderTitre->setText(QCoreApplication::translate("GSmartPetCare", "Pet Nova", nullptr));
        labelHeaderSous->setText(QCoreApplication::translate("GSmartPetCare", "  -  Gestion du centre de soins pour animaux", nullptr));
        btnNavAccueil->setText(QCoreApplication::translate("GSmartPetCare", "Accueil", nullptr));
        btnNavServices->setText(QCoreApplication::translate("GSmartPetCare", "Gestion des services", nullptr));
        btnNavEmployes->setText(QCoreApplication::translate("GSmartPetCare", "Gestion des employ\303\251s", nullptr));
        btnNavRDV->setText(QCoreApplication::translate("GSmartPetCare", "Gestion des RDV", nullptr));
        btnNavAnimaux->setText(QCoreApplication::translate("GSmartPetCare", "Gestion des animaux", nullptr));
        btnDeconnexion->setText(QCoreApplication::translate("GSmartPetCare", "D\303\251connexion", nullptr));
        labelAccueil->setText(QCoreApplication::translate("GSmartPetCare", "Accueil", nullptr));
        labelBienvenue->setText(QCoreApplication::translate("GSmartPetCare", "Bienvenue [Pr\303\251nom Nom]  -  R\303\264le : [Poste de l'employ\303\251]", nullptr));
        badgeService->setText(QCoreApplication::translate("GSmartPetCare", "1", nullptr));
        titleService->setText(QCoreApplication::translate("GSmartPetCare", "Module Services", nullptr));
        badgeEmployes->setText(QCoreApplication::translate("GSmartPetCare", "2", nullptr));
        titleEmployes->setText(QCoreApplication::translate("GSmartPetCare", "Module Employ\303\251s", nullptr));
        badgeRDV->setText(QCoreApplication::translate("GSmartPetCare", "3", nullptr));
        titleRDV->setText(QCoreApplication::translate("GSmartPetCare", "Module RDV", nullptr));
        badgeAnimaux->setText(QCoreApplication::translate("GSmartPetCare", "4", nullptr));
        titleAnimaux->setText(QCoreApplication::translate("GSmartPetCare", "Module Animaux", nullptr));
        labelTitrePage->setText(QCoreApplication::translate("GSmartPetCare", "Gestion des services", nullptr));
        lineEditRecherche->setPlaceholderText(QCoreApplication::translate("GSmartPetCare", "Rechercher un service\342\200\246", nullptr));
        comboBoxTri->setItemText(0, QCoreApplication::translate("GSmartPetCare", "Trier par : Nom", nullptr));
        comboBoxTri->setItemText(1, QCoreApplication::translate("GSmartPetCare", "Trier par : Prix", nullptr));
        comboBoxTri->setItemText(2, QCoreApplication::translate("GSmartPetCare", "Trier par : Dur\303\251e", nullptr));
        comboBoxTri->setItemText(3, QCoreApplication::translate("GSmartPetCare", "Trier par : Type", nullptr));

        comboBoxFiltreType->setItemText(0, QCoreApplication::translate("GSmartPetCare", "Type : Tous", nullptr));
        comboBoxFiltreType->setItemText(1, QCoreApplication::translate("GSmartPetCare", "Consultation", nullptr));
        comboBoxFiltreType->setItemText(2, QCoreApplication::translate("GSmartPetCare", "Vaccination", nullptr));
        comboBoxFiltreType->setItemText(3, QCoreApplication::translate("GSmartPetCare", "Traitement", nullptr));
        comboBoxFiltreType->setItemText(4, QCoreApplication::translate("GSmartPetCare", "Chirurgie", nullptr));
        comboBoxFiltreType->setItemText(5, QCoreApplication::translate("GSmartPetCare", "\303\211ducation", nullptr));
        comboBoxFiltreType->setItemText(6, QCoreApplication::translate("GSmartPetCare", "Comportement", nullptr));

        pushButtonExporterPdf->setText(QCoreApplication::translate("GSmartPetCare", "Exporter PDF", nullptr));
        pushButtonHistorique->setText(QCoreApplication::translate("GSmartPetCare", "Historique", nullptr));
        pushButtonStatistiques->setText(QCoreApplication::translate("GSmartPetCare", "Statistiques", nullptr));
        QTableWidgetItem *___qtablewidgetitem = tableWidgetServices->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("GSmartPetCare", "ID", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = tableWidgetServices->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("GSmartPetCare", "Nom du service", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = tableWidgetServices->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("GSmartPetCare", "Type", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = tableWidgetServices->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("GSmartPetCare", "Prix (DT)", nullptr));
        QTableWidgetItem *___qtablewidgetitem4 = tableWidgetServices->horizontalHeaderItem(4);
        ___qtablewidgetitem4->setText(QCoreApplication::translate("GSmartPetCare", "Dur\303\251e (min)", nullptr));
        QTableWidgetItem *___qtablewidgetitem5 = tableWidgetServices->horizontalHeaderItem(5);
        ___qtablewidgetitem5->setText(QCoreApplication::translate("GSmartPetCare", "Disponible", nullptr));
        QTableWidgetItem *___qtablewidgetitem6 = tableWidgetServices->horizontalHeaderItem(6);
        ___qtablewidgetitem6->setText(QCoreApplication::translate("GSmartPetCare", "Actions", nullptr));
        labelDetailTitre->setText(QCoreApplication::translate("GSmartPetCare", "Ajouter un service", nullptr));
        labelChampId->setText(QCoreApplication::translate("GSmartPetCare", "ID service", nullptr));
        lineEditIdService->setPlaceholderText(QCoreApplication::translate("GSmartPetCare", "G\303\251n\303\251r\303\251", nullptr));
        labelChampPrix->setText(QCoreApplication::translate("GSmartPetCare", "Prix (DT)", nullptr));
        lineEditPrix->setPlaceholderText(QCoreApplication::translate("GSmartPetCare", "0.0", nullptr));
        labelChampNomService->setText(QCoreApplication::translate("GSmartPetCare", "Nom du service", nullptr));
        lineEditNomService->setPlaceholderText(QCoreApplication::translate("GSmartPetCare", "Ex. : Consultation g\303\251n\303\251rale", nullptr));
        labelChampDescription->setText(QCoreApplication::translate("GSmartPetCare", "Description", nullptr));
        plainTextEditDescription->setPlaceholderText(QCoreApplication::translate("GSmartPetCare", "Description d\303\251taill\303\251e du service", nullptr));
        labelChampDuree->setText(QCoreApplication::translate("GSmartPetCare", "Dur\303\251e (min)", nullptr));
        lineEditDuree->setPlaceholderText(QCoreApplication::translate("GSmartPetCare", "30", nullptr));
        labelChampType->setText(QCoreApplication::translate("GSmartPetCare", "Type", nullptr));
        comboBoxType->setItemText(0, QCoreApplication::translate("GSmartPetCare", "Consultation", nullptr));
        comboBoxType->setItemText(1, QCoreApplication::translate("GSmartPetCare", "Vaccination", nullptr));
        comboBoxType->setItemText(2, QCoreApplication::translate("GSmartPetCare", "Traitement", nullptr));
        comboBoxType->setItemText(3, QCoreApplication::translate("GSmartPetCare", "Chirurgie", nullptr));
        comboBoxType->setItemText(4, QCoreApplication::translate("GSmartPetCare", "\303\211ducation", nullptr));
        comboBoxType->setItemText(5, QCoreApplication::translate("GSmartPetCare", "Comportement", nullptr));

        checkBoxDisponible->setText(QCoreApplication::translate("GSmartPetCare", "Service disponible", nullptr));
        pushButtonEnregistrer->setText(QCoreApplication::translate("GSmartPetCare", "Enregistrer", nullptr));
        pushButtonAnnuler->setText(QCoreApplication::translate("GSmartPetCare", "Annuler", nullptr));
    } // retranslateUi

};

namespace Ui {
    class GSmartPetCare: public Ui_GSmartPetCare {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_GSMARTPETCARE_H
