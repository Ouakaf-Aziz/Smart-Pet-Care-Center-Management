#ifndef DONUTCHART_H
#define DONUTCHART_H

#include <QWidget>
#include <QMap>
#include <QString>
#include <QVector>
#include <QColor>

// Cercle (anneau) de statistiques avec le total au centre et une légende à droite.
class DonutChart : public QWidget
{
    Q_OBJECT
public:
    explicit DonutChart(QWidget *parent = nullptr);

    // ex. { {"Consultation", 5}, {"Vaccination", 3} }
    void setData(const QMap<QString, int> &data);
    void clear();

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QMap<QString, int> m_data;
    QVector<QColor> m_colors;
};

#endif // DONUTCHART_H
