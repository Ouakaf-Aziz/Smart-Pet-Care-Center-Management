#ifndef BARCHART_H
#define BARCHART_H

#include <QColor>
#include <QPair>
#include <QString>
#include <QVector>
#include <QWidget>

// Diagramme en barres : vertical (histogramme) ou horizontal.
class BarChart : public QWidget
{
    Q_OBJECT
public:
    enum Orientation { Vertical, Horizontal };

    explicit BarChart(QWidget *parent = nullptr);

    // ex. { {"0 - 25", 3}, {"25 - 50", 4} }
    void setData(const QVector<QPair<QString, double>> &data,
                 Orientation orientation = Vertical,
                 const QString &suffixeValeur = QString(),
                 const QColor &couleur = QColor("#0F766E"));

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString formater(double v) const;

    QVector<QPair<QString, double>> m_data;
    Orientation m_orientation = Vertical;
    QString m_suffixe;
    QColor m_couleur;
};

#endif // BARCHART_H
