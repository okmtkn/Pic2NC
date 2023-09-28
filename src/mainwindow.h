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


#include <QSettings> //設定の保存と読み込み
#include <QTextCodec> //iniファイルのファイル名指定のため

#include <opencv2/opencv.hpp>
#include "imageprocessing.h"
#include "ncdata.h"
#include "scannertwain.h"
#include "drawingmousescene.h"


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

    // Function: SaveSettings
    // 設定を保存する．
    void SlotSaveSettings();

    // Function: LoadSettings
    // 設定を読み込む．
    void SlotLoadSettings();

    // Function: LoadDefaultSettings
    // デフォルトの設定を読み込む．
    void SlotLoadDefaultSettings();

private:
    // Function: ShowImage
    // 画像をUIに表示する．
    void ShowImage(void);

    // Function: OpenImageFile
    // 画像ファイルを開く．
    void OpenImageFile(QString file_name); //image_をdeleteしてからnewする関数のため，image_ではなくMainWindowのメンバとした．

    // Function: MakeTimeString
    // 現在時刻を文字列で返す．
    std::string MakeTimeString();

    // Function: SaveSettings
    // 設定を保存する．
    void SaveSettings();

    // Function: LoadSettings
    // 設定を読み込む．
    void LoadSettings();

    // Function: LoadDefaultSettings
    // デフォルトの設定を読み込む．
    void LoadDefaultSettings();

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

    void on_radioButton_toggled(bool checked);

    void on_tabWidgetImageDrawing_currentChanged(int index);

    void on_pushButtonClear_clicked();

    void on_horizontalSliderPenWidth_valueChanged(int value);

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

    DrawingMouseScene scene_drawing_;

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent* event) override;
};

#endif // MAINWINDOW_H
