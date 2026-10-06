#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
private slots:
    void enregistrerRdv();
    void viderFormulaire();
    void afficherStatistiques();
    void exporterPdf();
private:
    Ui::MainWindow *ui;
    int m_ligneEnModification = -1;
    void configurerMiseEnPage();
    void configurerTableau();
    void remplirExemples();
    void remplirLigne(int row, const QStringList &valeurs);
    void ajouterBoutonsAction(int row);
    bool genererPdf(const QString &chemin);
};
#endif