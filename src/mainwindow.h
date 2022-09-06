#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <QFileDialog>
#include <QResizeEvent> //QMainWindowがリサイズされたときの処理
#include <QProgressBar>
#include <QStyle> //標準アイコン
#include <QStandardPaths> //Pictureフォルダを参照するため
#include <QDir> //ユーザディレクトリを参照するため
#include <QFileInfo> //ファイル名だけ取得して保存ファイル名にするため
#include <QDesktopServices> //エクスプローラーを開くため
#include <QMessageBox>  //about画面の表示
#include <QRubberBand>
#include <QMimeData>

#include <QGraphicsScene>

#include <QTextStream>


#include <opencv2/opencv.hpp>
#include "imageprocessing.h"
#include "ncdata.h"
#include "scannertwain.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void OpenImageFileDialog(); //[Open]ボタンを押したときと，Menu->Openを選択したとき
    void SelectOutputDirectory();
    void MessageBoxAbout();
    void MessageBoxAboutQt();

private:
    void ShowImage(void);
    void OpenImageFile(QString file_name); //image_をdeleteしてからnewする関数のため，image_ではなくMainWindowのメンバとした．
    std::string MakeTimeString();

public slots:
    void SetProgressBarValue(int value);

private slots:
    void on_pushButtonOpen_clicked();
    void on_pushButtonScan_clicked();
    void on_pushButtonResetTrim_clicked();

    void on_horizontalSliderMedian_valueChanged(int value);
    void on_horizontalSliderMorphologyOpen_valueChanged(int value);
    void on_horizontalSliderMorphologyClose_valueChanged(int value);

    void on_toolButtonSelectDirectory_clicked();
    void on_pushButtonOpenDirectory_clicked();
    void on_pushButtonGenrerate_clicked();

    void on_pushButtonOpenNcData_clicked();


private:
    Ui::MainWindow *ui;

    ImageProcessing *image_ = std::nullptr_t();

    QString input_file_name_;
    QString output_file_name_;

    QString input_directory_name_ = QDir::homePath() + "/" + QStandardPaths::displayName(QStandardPaths::PicturesLocation);
    QString output_directory_name_ = QDir::homePath() + "/" + QStandardPaths::displayName(QStandardPaths::DesktopLocation);

    QRubberBand *rubber_band_;
    QPoint mouse_clicked_point_;
    QRect selected_rect_;

    QGraphicsScene scene_nc_;

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent* event) override;
};

#endif // MAINWINDOW_H
