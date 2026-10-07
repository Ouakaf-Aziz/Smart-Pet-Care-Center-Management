#ifndef LINECHART_H
#define LINECHART_H

#include <QColor>
#include <QPair>
#include <QString>
#include <QVector>
#include <QWidget>

// Courbe d'évolution (ex. recrutements par année d'embauche).
class LineChart : public QWidget
{
    Q_OBJECT
public:
    explicit LineChart(QWidget *parent = nullptr);

    void setData(const QVector<QPair<QString, double>> &data,
                 const QColor &couleur = QColor("#0F766E"));

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<QPair<QString, double>> m_data;
    QColor m_couleur;
};

#endif // LINECHART_H
