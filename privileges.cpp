#include "privileges.h"
#include "ui_gsmartpetcare.h"

#include <QAbstractItemView>
#include <QColor>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>

Privileges::Privileges(Ui::GSmartPetCare *ui, BaseEmployes &base, QWidget *fenetre)
    : QObject(fenetre)
    , ui(ui)
    , m_fenetre(fenetre)
    , m_base(base)
    , m_postes(Emp::postes())
{
    QTableWidget *table = ui->tableRoles;
    table->horizontalHeader()->setFixedHeight(44);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    connect(ui->btnAnnulerPrivileges, &QPushButton::clicked, this, &Privileges::termine);
}

void Privileges::ouvrir()
{
    QTableWidget *table = ui->tableRoles;
    table->clearContents();
    table->setRowCount(m_postes.size());
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);      // consultation seule

    const char *infobulles[NbDroits] = {
        "Créer de nouveaux employés (et enregistrer leur visage)",
        "Modifier les fiches employés (et enregistrer leur visage)",
        "Supprimer des employés",
        "Voir les salaires et trier par salaire ; sans ce droit la colonne et le champ sont masqués",
        "Exporter la liste en PDF",
        "Ouvrir les statistiques",
    };

    // Une ligne par rôle (poste), une colonne par droit : ✔ accordé, ✘ refusé. Valeurs automatiques.
    for (int r = 0; r < m_postes.size(); ++r) {
        const QString &poste = m_postes[r];
        int nb = 0;
        for (const Employe &e : std::as_const(m_base.employes))
            if (e.poste == poste)
                ++nb;

        table->setItem(r, 0, new QTableWidgetItem(poste));
        auto *itNb = new QTableWidgetItem(QString::number(nb));
        itNb->setTextAlignment(Qt::AlignCenter);
        table->setItem(r, 1, itNb);

        const Droits d = Emp::droitsParDefaut(poste);
        const bool valeurs[NbDroits] = {d.ajouter, d.modifier, d.supprimer, d.voirSalaires, d.exporter, d.statistiques};
        for (int c = 0; c < NbDroits; ++c) {
            auto *it = new QTableWidgetItem(valeurs[c] ? QStringLiteral("✔") : QStringLiteral("✘"));
            it->setTextAlignment(Qt::AlignCenter);
            it->setForeground(QColor(valeurs[c] ? "#0F766E" : "#C0392B"));
            it->setToolTip(QString::fromUtf8(infobulles[c]));
            table->setItem(r, 2 + c, it);
        }
    }
}
