#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->lineEditSelectDirectory->setText(output_directory_name_);
    ui->pushButtonGenrerate->setDisabled(true);
    ui->pushButtonOpenNcData->setDisabled(true);

    rubber_band_ = new QRubberBand(QRubberBand::Rectangle, ui->labelImageOriginal);
    ui->labelImageOriginal->installEventFilter(this); //MouseEventのため

    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setScene(&scene_nc_);
    ui->graphicsView->setRenderHint(QPainter::RenderHint(QPainter::Antialiasing));

    setAcceptDrops(true);   //Drag & Dropを全てのwidgetで有効化する

    ui->graphicsViewDrawing->setScene(&scene_drawing_);

    //タブにアイコンをつける
    ui->tabWidgetImageDrawing->setTabIcon(0, QApplication::style()->standardIcon( QStyle::SP_FileIcon ));
    ui->tabWidgetImageDrawing->setTabIcon(1, QApplication::style()->standardIcon( QStyle::SP_ComputerIcon ));
    ui->tabWidget->setTabIcon(0, QApplication::style()->standardIcon( QStyle::SP_DialogApplyButton ));
    ui->tabWidget->setTabIcon(1, QApplication::style()->standardIcon( QStyle::SP_FileDialogDetailedView ));
    ui->tabWidget->setTabIcon(2, QApplication::style()->standardIcon( QStyle::SP_FileDialogInfoView  ));
    ui->tabWidget->setTabIcon(3, QApplication::style()->standardIcon( QStyle::SP_FileDialogInfoView ));


    //ボタンにアイコンをつける
    ui->toolButtonSelectDirectory->setIcon(QApplication::style()->standardIcon( QStyle::SP_DialogOpenButton ));
    ui->pushButtonGenrerate->setIcon(QApplication::style()->standardIcon( QStyle::SP_DialogApplyButton ));
    ui->pushButtonOpen->setIcon(QApplication::style()->standardIcon( QStyle::SP_DialogOpenButton ));
    ui->pushButtonScan->setIcon(QApplication::style()->standardIcon( QStyle::SP_FileIcon ));
    ui->pushButtonResetTrim->setIcon(QApplication::style()->standardIcon( QStyle::SP_DialogResetButton ));
    ui->pushButtonOpenNcData->setIcon(QApplication::style()->standardIcon( QStyle::SP_DialogOpenButton ));


    //設定データを読み込む
    LoadSettings();

    ui->statusBar->showMessage("Ready. Open your image file.");
}


MainWindow::~MainWindow()
{
    delete ui;
    delete image_;
    image_ = std::nullptr_t();
    delete rubber_band_;
}


void MainWindow::OpenImageFileDialog()
{
    //画像読み込み
    QString file_name = QString::fromLocal8Bit(
                QFileDialog::getOpenFileName(this, tr("Open Image"),
                                             input_directory_name_,
                                             tr("Image Files (*.bmp *.png *.jpg *jpeg *jpe *jfif *gif *tif *tiff *webp);;All Files (*.*)")
                                             ).toLocal8Bit()
                );

    OpenImageFile(file_name);
}


void MainWindow::OpenImageFile(QString file_name)
{
    //エラーチェック
    if (file_name.isEmpty() || file_name.isNull()) {
        return;
    } else {
        input_file_name_ = file_name;

        //今回読み取ったディレクトリ名を記録しておく．次回もこのディレクトリをDialogで開きに行くため．
        input_directory_name_ = QDir(input_file_name_).absolutePath();

        //image_をnewするまえに，前回newされていることを想定してdeleteしておく．あらかじめnullptrが代入されているはずだから，二重解放にはならない．
        delete image_;
        image_ = std::nullptr_t();
    }

    image_ = new ImageProcessing(input_file_name_,
                                 (ui->horizontalSliderMedian->value()+1)*2-1,
                                 ui->horizontalSliderMorphologyOpen->value()+1,
                                 ui->horizontalSliderMorphologyClose->value()+1
                                 );

    //画像ファイル以外が選択された場合
    if(image_->Empty()){
        ui->statusBar->showMessage("Invalid image file.");
        delete image_;
        image_ = std::nullptr_t();

        ui->pushButtonGenrerate->setDisabled(true);
        ui->pushButtonOpenNcData->setDisabled(true);
        //return;
    } else { //ちゃんと画像ファイルだった場合
        ui->statusBar->showMessage("Adjust any options. Click [Generate] to process.");
        ui->pushButtonGenrerate->setDisabled(false);
        //前回処理済だとProgressバーが100%だから，画像を開きなおしたら0に戻しておく
        ui->progressBar->setValue(0);
        ui->pushButtonOpenNcData->setDisabled(true);

        //タブを切り替える
        ui->tabWidgetImageDrawing->setCurrentIndex(0);

        //画像処理と表示
        image_->ImageProcess();
    }

    //表示エリアをすべてクリア
    ShowImage();
    rubber_band_->hide();
    ui->textEdit->clear();
    ui->lineEditNumOfBlock->clear();
    ui->lineEditG00Length->clear();
    ui->lineEditG01Length->clear();
    ui->lineEditTotalLength->clear();
    ui->lineEditTotalTime->clear();
    scene_nc_.clear();
}


void MainWindow::on_pushButtonScan_clicked()
{
    ScannerTwain *scan = new ScannerTwain((HWND)this->winId());

    scan->Scan("temp.bmp");
    OpenImageFile("temp.bmp");
    input_file_name_ = QString::fromStdString(MakeTimeString()) + ".bmp";

    delete scan;
}


void MainWindow::on_pushButtonResetTrim_clicked()
{
    if(image_ == std::nullptr_t()){
        return;
    }

    rubber_band_->hide();
    QRect roi_rect = QRect(0,0,0,0);
    image_->set_roi_rect_(roi_rect);
    image_->ImageProcess();
    ShowImage();
}


void MainWindow::SelectOutputDirectory()
{
    QFileDialog::Options options = QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly;

    QString tmp_directory_name = QFileDialog::getExistingDirectory(
                this,
                tr("Select Output Directory"),
                output_directory_name_,
                options);

    if ( !tmp_directory_name.isEmpty() )
    {
        output_directory_name_ = tmp_directory_name;
        ui->lineEditSelectDirectory->setText(output_directory_name_);
        ui->statusBar->showMessage("Output directory: " + output_directory_name_);
    } else {
        ui->statusBar->showMessage("Invalid directory name.");
        return;
    }
}


void MainWindow::ShowImage(void)
{
    if(image_ == std::nullptr_t() ){
        ui->labelImageOriginal->clear();
        ui->labelImageNoiseRemoved->clear();
        scene_nc_.clear();
        return;
    }

    int w = ui->labelImageOriginal->width();
    int h = ui->labelImageOriginal->height();
    ui->labelImageOriginal->setPixmap(
                QPixmap::fromImage(
                    image_->get_original_image_()
                    ).scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                );

    w = ui->labelImageNoiseRemoved->width();
    h = ui->labelImageNoiseRemoved->height();
    ui->labelImageNoiseRemoved->setPixmap(
                QPixmap::fromImage(
                    image_->get_noise_removed_image_()
                    ).scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                );
}


void MainWindow::SetProgressBarValue(int value)
{
    ui->progressBar->setValue(value);
}


void MainWindow::resizeEvent(QResizeEvent *event)
{
    rubber_band_->hide();
    ShowImage();
    ui->graphicsView->fitInView( scene_nc_.itemsBoundingRect(), Qt::KeepAspectRatio );
    ui->graphicsView->setSceneRect(scene_nc_.itemsBoundingRect());
}


//trueを返したeventは棄却される
//MainWindowのコンストラクタでinstallEventFilterをして使う
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if(image_ == std::nullptr_t()){
        return false;   //なにもせず返す
    }

    if(event->type() == QEvent::MouseButtonPress){
        QMouseEvent *mouse_event = static_cast<QMouseEvent*>(event);

        mouse_clicked_point_ = mouse_event->pos();
        selected_rect_ = QRect(mouse_clicked_point_,
                               mouse_event->pos()
                               ).normalized();
        rubber_band_->setGeometry(selected_rect_);
        rubber_band_->show();

        return false;
    }
    if(event->type() == QEvent::MouseMove){
        QMouseEvent *mouse_event = static_cast<QMouseEvent*>(event);

        selected_rect_ = QRect(mouse_clicked_point_, mouse_event->pos()
                               ).normalized();
        rubber_band_->setGeometry(selected_rect_);

        return false;
    }
    if(event->type() == QEvent::MouseButtonRelease){
        int w = ui->labelImageOriginal->width();
        int h = ui->labelImageOriginal->height();

        double scale_w = image_->get_original_image_().width() / double(w);
        double scale_h = image_->get_original_image_().height() / double(h);
        double scale = (scale_w > scale_h) ? scale_w : scale_h;

        QPixmap scaled_image = QPixmap(QPixmap::fromImage(image_->get_original_image_()).scaled(w, h, Qt::KeepAspectRatio));
        int rubber_band_offset_x = (ui->labelImageOriginal->width() - scaled_image.width())/2.;
        int rubber_band_offset_y = (ui->labelImageOriginal->height() - scaled_image.height())/2.;

        QRect roi_rect = QRect((rubber_band_->x() - rubber_band_offset_x) * scale ,
                               (rubber_band_->y() - rubber_band_offset_y) * scale,
                               rubber_band_->width()*scale, rubber_band_->height()*scale );

        image_->set_roi_rect_(roi_rect);
        image_->ImageProcess();
        ShowImage();

        return false;
    }
    return false;
}


void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasText()){   //デスクトップからドロップされた場合，file:///C:/**　というようなplain textがdropされる
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    QString file_name = QString::fromLocal8Bit( event->mimeData()->text().toLocal8Bit() );
    file_name.remove(0,8);  // file:///という冒頭の余計な8文字を除去
    OpenImageFile(file_name);
}


void MainWindow::on_pushButtonOpen_clicked()
{
    OpenImageFileDialog();
}


void MainWindow::on_horizontalSliderMedian_valueChanged(int value)
{
    if(image_ != std::nullptr_t()){
        image_->set_median_ksize_((value+1)*2-1);
        image_->ImageProcess();
        ShowImage();
    }
}


void MainWindow::on_horizontalSliderMorphologyOpen_valueChanged(int value)
{
    if(image_ != std::nullptr_t()){
        image_->set_morphology_open_( value + 1 );
        image_->ImageProcess();
        ShowImage();
    }
}


void MainWindow::on_horizontalSliderMorphologyClose_valueChanged(int value)
{
    if(image_ != std::nullptr_t()){
        image_->set_morphology_close_( value + 1 );
        image_->ImageProcess();
        ShowImage();
    }
}


void MainWindow::on_toolButtonSelectDirectory_clicked()
{
    SelectOutputDirectory();
}


void MainWindow::on_pushButtonOpenDirectory_clicked()
{
    QUrl url = QString("file:///").append(output_directory_name_);
    QDesktopServices::openUrl(url);
}


void MainWindow::on_pushButtonGenrerate_clicked()
{
    //Drawing領域に手描きした場合
    if(ui->tabWidgetImageDrawing->currentIndex() == 1){
        QString file_name = output_directory_name_ + "/tmp.png";
        QPixmap(ui->graphicsViewDrawing->grab()).save(file_name);
        OpenImageFile(file_name);
    }

    if(image_ == std::nullptr_t()){
        return;
    }

    ui->statusBar->showMessage("Processing...");
    ui->pushButtonGenrerate->setText("Processing...");
    ui->pushButtonGenrerate->setDisabled(true);
    ui->pushButtonOpenNcData->setDisabled(true);

    ui->progressBar->setValue(1);

    //長い処理中にGUIの応答がなくなって固まるので対策
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    NcData *nc_data;

    //出力ファイル名を設定する．
    output_file_name_ = output_directory_name_ + "/" + QFileInfo(input_file_name_).baseName() + ui->comboBoxFormat->currentText();

    nc_data = new NcData(image_->get_output_image_(),
                         output_file_name_,
                         ui->comboBoxFormat->currentText(),
                         ui->doubleSpinBoxWorkWidth->value(),
                         ui->doubleSpinBoxWorkHeight->value(),
                         ui->doubleSpinBoxRetract->value(),
                         ui->doubleSpinBoxCuttingDepth->value(),
                         ui->spinBoxSpindle->value(),
                         ui->spinBoxFeedRate->value(),
                         ui->doubleSpinBoxOffsetX->value(),
                         ui->doubleSpinBoxOffsetY->value(),
                         ui->doubleSpinBoxOffsetZ->value(),
                         ui->spinBoxNumOfIgnore->value(),
                         ui->doubleSpinBoxTolerance->value(),
                         ui->groupBoxOffsetPocketMilling->isChecked(),
                         ui->checkBoxCutterCompensation->isChecked(),
                         ui->doubleSpinBoxToolR->value(),
                         ui->doubleSpinBoxPickfeed->value(),
                         ui->doubleSpinBox_StlHeight->value()
                         );

    connect(nc_data, SIGNAL(ProgressBarValueChanged(int)), this, SLOT(SetProgressBarValue(int)));

    nc_data->GenerateNcData();

    ui->statusBar->showMessage("Drawing tool path...");
    nc_data->DrawNcView( &scene_nc_, ui->graphicsView->rect() );    //scene_nc_をnc_dataに渡して描画してもらう
    ui->graphicsView->fitInView(scene_nc_.itemsBoundingRect(), Qt::KeepAspectRatio);
    ui->graphicsView->setSceneRect(scene_nc_.itemsBoundingRect());
    
    int num_of_block = nc_data->get_cutting_point_length_();
    float g0_length = nc_data->GetG0Length();
    float g1_length = nc_data->GetG1Length();
    float minF = g0_length/ui->spinBoxG00Speed->value() +
                 g1_length/ui->spinBoxFeedRate->value() +
                 num_of_block * ui->spinBoxTimePerBlock->value()*0.001/60.;
    int min = int(minF);
    int sec = 60*(minF - min);
    ui->lineEditNumOfBlock->setText(QString::number(num_of_block));
    ui->lineEditG00Length->setText(QString::number(int(g0_length)));
    ui->lineEditG01Length->setText(QString::number(int(g1_length)));
    ui->lineEditTotalLength->setText(QString::number(int(g0_length+g1_length)));
    if(sec<10){
        ui->lineEditTotalTime->setText(QString::number(min) + ":0" + QString::number(sec));
    } else {
        ui->lineEditTotalTime->setText(QString::number(min) + ":" + QString::number(sec));
    }


    //出力ファイルの文字列を表示する．
    //ただし，文字数が多いと表示が重くなるので，先頭の100行だけ表示する．
    ui->textEdit->clear();
    QString display_string = nc_data->GetOutputString();
    int num_of_line = display_string.count("\n");
    if(num_of_line > 100){
        display_string = display_string.section("\n", 0, 99);
        display_string.append("\n...\n");
    }
    ui->textEdit->setText(display_string);


    //ui->textEdit->setText(nc_data->GetOutputString());

    delete nc_data;

    ui->pushButtonGenrerate->setDisabled(false);
    ui->pushButtonOpenNcData->setDisabled(false);
    ui->progressBar->setValue(100);
    ui->pushButtonGenrerate->setText("Generate");
    ui->statusBar->showMessage("NC data output completed!  File neme: " + output_file_name_);
    QApplication::beep();
}


void MainWindow::on_pushButtonOpenNcData_clicked()
{
    QUrl url = QString("file:///").append(output_file_name_);
    QDesktopServices::openUrl(url);
}


void MainWindow::MessageBoxAbout()
{
    QString text =
            "<span style=\"font-size:xx-large; font-weight:bold\">Pic2NC</span><br>"
            "Version 2.0.29<br><br>"
            "Copyright (C) 2023 Nanshin Institute of Technology. Ken OKAMOTO.<br>"
            "<a href=\"https://nanshinkotan.ac.jp/\">"
            "https://nanshinkotan.ac.jp/</a><br><br><br>"

            "Licensed under the Apache License, Version 2.0 (the “License”);<br>"
            "you may not use this file except in compliance with the License.<br>"
            "You may obtain a copy of the License at<br>"
            "<a href=\"http://www.apache.org/licenses/LICENSE-2.0\">"
            "http://www.apache.org/licenses/LICENSE-2.0</a><br><br>"

            "Unless required by applicable law or agreed to in writing, software<br>"
            "distributed under the License is distributed on an “AS IS” BASIS,<br>"
            "WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.<br><br>"

            "See the License for the specific language governing permissions and<br>"
            "limitations under the License.<br><br><br>"


            "The TWAIN Toolkit is distributed as is. The developer and distributors of the "
            "TWAIN Toolkit expressly disclaim all implied, express or statutory warranties "
            "including, without limitation, the implied warranties of merchantability, "
            "noninfringement of third party rights and fitness for a particular purpose. "
            "Neither the developers nor the distributors will be liable for damages, "
            "whether direct, indirect, special, incidental, or consequential, as a result "
            "of the reproduction, modification, distribution or other use of the TWAIN "
            "Toolkit.";



    QMessageBox msgbox;
    //msgbox.about(this, tr("About Pic2NC V2"), text);
    msgbox.setText(text);
    msgbox.setWindowTitle(tr("About Pic2NC"));
    msgbox.setIconPixmap(QPixmap("resource/64.png"));
    //msgbox.setIconPixmap(QPixmap("resource/pic2nc.ico"));
    msgbox.exec();
}


std::string MainWindow::MakeTimeString()
{
    time_t t = time(nullptr);
    const tm* localTime = localtime(&t);
    std::stringstream s;
    s << localTime->tm_year + 1900;
    // setw(),setfill()で0詰め
    s << std::setw(2) << std::setfill('0') << localTime->tm_mon + 1;
    s << std::setw(2) << std::setfill('0') << localTime->tm_mday;
    s << std::setw(2) << std::setfill('0') << localTime->tm_hour;
    s << std::setw(2) << std::setfill('0') << localTime->tm_min;
    s << std::setw(2) << std::setfill('0') << localTime->tm_sec;
    // std::stringにして値を返す
    return s.str();
}

void MainWindow::SaveSettings()
{
    //iniファイルのファイル名を設定する．
    QSettings settings("user_settings.ini", QSettings::IniFormat);
    //settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    settings.beginGroup("Noise_Reduction");
    settings.setValue("Median", ui->spinBoxMedian->value());
    settings.setValue("White", ui->spinBoxMorphologyOpen->value());
    settings.setValue("Black", ui->spinBoxMorphologyClose->value());
    settings.endGroup();

    settings.beginGroup("Save_Directory");
    settings.setValue("Directory", ui->lineEditSelectDirectory->text());
    settings.setValue("Filetype", ui->comboBoxFormat->currentIndex());
    settings.endGroup();

    settings.beginGroup("Cutting_Conditions");
    settings.setValue("WorkWidth", ui->doubleSpinBoxWorkWidth->value());
    settings.setValue("WorkHeight", ui->doubleSpinBoxWorkHeight->value());
    settings.setValue("Retract", ui->doubleSpinBoxRetract->value());
    settings.setValue("CuttingDepth", ui->doubleSpinBoxCuttingDepth->value());
    settings.setValue("Spindle", ui->spinBoxSpindle->value());
    settings.setValue("FeedRate", ui->spinBoxFeedRate->value());
    settings.endGroup();
    settings.beginGroup("Origin_Offset");
    settings.setValue("OffsetX", ui->doubleSpinBoxOffsetX->value());
    settings.setValue("OffsetY", ui->doubleSpinBoxOffsetY->value());
    settings.setValue("OffsetZ", ui->doubleSpinBoxOffsetZ->value());
    settings.endGroup();
    settings.beginGroup("Pocket_Milling");
    settings.setValue("GroupBoxOffsetPocketMilling", ui->groupBoxOffsetPocketMilling->isChecked());
    settings.setValue("CheckBoxCutterCompensation", ui->checkBoxCutterCompensation->isChecked());
    settings.setValue("ToolR", ui->doubleSpinBoxToolR->value());
    settings.setValue("Pickfeed", ui->doubleSpinBoxPickfeed->value());
    settings.endGroup();
    settings.beginGroup("Polygonal_Approximation");
    settings.setValue("MinPathLength", ui->spinBoxNumOfIgnore->value());
    settings.setValue("Tolerance", ui->doubleSpinBoxTolerance->value());
    settings.endGroup();
    settings.beginGroup("Machine_Processing Speed");
    settings.setValue("G00Speed", ui->spinBoxG00Speed->value());
    settings.setValue("TimePerBlock", ui->spinBoxTimePerBlock->value());
    settings.endGroup();
    settings.beginGroup("3D_Options");
    settings.setValue("StlHeight", ui->doubleSpinBox_StlHeight->value());
    settings.endGroup();
    settings.sync();
}

void MainWindow::LoadSettings()
{
    //user_settings.iniが存在するか確認する．
    //存在しない場合は，デフォルト設定を読み込む．
    QFile file("user_settings.ini");
    if(!file.exists()){
        LoadDefaultSettings();
        return;
    }

    QSettings settings("user_settings.ini", QSettings::IniFormat);
    //settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    settings.beginGroup("Noise_Reduction");
    ui->spinBoxMedian->setValue(settings.value("Median").toInt());
    ui->spinBoxMorphologyOpen->setValue(settings.value("White").toInt());
    ui->spinBoxMorphologyClose->setValue(settings.value("Black").toInt());
    settings.endGroup();

    settings.beginGroup("Save_Directory");
    ui->lineEditSelectDirectory->setText(settings.value("Directory").toString());
    output_directory_name_ = ui->lineEditSelectDirectory->text();
    //ディレクトリが存在しない場合は，デスクトップを設定する．
    QDir dir(ui->lineEditSelectDirectory->text());
    if(!dir.exists()){
        ui->lineEditSelectDirectory->setText(QDir::homePath() + "/" + QStandardPaths::displayName(QStandardPaths::DesktopLocation));
    }

    ui->comboBoxFormat->setCurrentIndex(settings.value("Filetype").toInt());
    settings.endGroup();

    settings.beginGroup("Cutting_Conditions");
    ui->doubleSpinBoxWorkWidth->setValue(settings.value("WorkWidth").toDouble());
    ui->doubleSpinBoxWorkHeight->setValue(settings.value("WorkHeight").toDouble());
    ui->doubleSpinBoxRetract->setValue(settings.value("Retract").toDouble());
    ui->doubleSpinBoxCuttingDepth->setValue(settings.value("CuttingDepth").toDouble());
    ui->spinBoxSpindle->setValue(settings.value("Spindle").toInt());
    ui->spinBoxFeedRate->setValue(settings.value("FeedRate").toInt());
    settings.endGroup();
    settings.beginGroup("Origin_Offset");
    ui->doubleSpinBoxOffsetX->setValue(settings.value("OffsetX").toDouble());
    ui->doubleSpinBoxOffsetY->setValue(settings.value("OffsetY").toDouble());
    ui->doubleSpinBoxOffsetZ->setValue(settings.value("OffsetZ").toDouble());
    settings.endGroup();
    settings.beginGroup("Pocket_Milling");
    ui->groupBoxOffsetPocketMilling->setChecked(settings.value("GroupBoxOffsetPocketMilling").toBool());
    ui->checkBoxCutterCompensation->setChecked(settings.value("CheckBoxCutterCompensation").toBool());
    ui->doubleSpinBoxToolR->setValue(settings.value("ToolR").toDouble());
    ui->doubleSpinBoxPickfeed->setValue(settings.value("Pickfeed").toDouble());
    settings.endGroup();
    settings.beginGroup("Polygonal_Approximation");
    ui->spinBoxNumOfIgnore->setValue(settings.value("MinPathLength").toInt());
    ui->doubleSpinBoxTolerance->setValue(settings.value("Tolerance").toDouble());
    settings.endGroup();
    settings.beginGroup("Machine_Processing Speed");
    ui->spinBoxG00Speed->setValue(settings.value("G00Speed").toInt());
    ui->spinBoxTimePerBlock->setValue(settings.value("TimePerBlock").toInt());
    settings.endGroup();
    settings.beginGroup("3D_Options");
    ui->doubleSpinBox_StlHeight->setValue(settings.value("StlHeight").toDouble());
    settings.endGroup();
}

void MainWindow::LoadDefaultSettings()
{
    ui->spinBoxMedian->setValue(2);
    ui->spinBoxMorphologyOpen->setValue(2);
    ui->spinBoxMorphologyClose->setValue(2);

    ui->comboBoxFormat->setCurrentIndex(0);

    ui->lineEditSelectDirectory->setText(QDir::homePath() + "/" + QStandardPaths::displayName(QStandardPaths::DesktopLocation));

    ui->doubleSpinBoxWorkWidth->setValue(40);
    ui->doubleSpinBoxWorkHeight->setValue(40);
    ui->doubleSpinBoxRetract->setValue(2);
    ui->doubleSpinBoxCuttingDepth->setValue(0);
    ui->spinBoxSpindle->setValue(3000);
    ui->spinBoxFeedRate->setValue(360);
    ui->doubleSpinBoxOffsetX->setValue(0);
    ui->doubleSpinBoxOffsetY->setValue(0);
    ui->doubleSpinBoxOffsetZ->setValue(0);
    ui->doubleSpinBoxToolR->setValue(0);
    ui->doubleSpinBoxPickfeed->setValue(0.15);
    ui->spinBoxNumOfIgnore->setValue(3);
    ui->doubleSpinBoxTolerance->setValue(2);
    ui->spinBoxG00Speed->setValue(1000);
    ui->spinBoxTimePerBlock->setValue(25);
    ui->doubleSpinBox_StlHeight->setValue(1.5);
}

void MainWindow::SlotSaveSettings()
{
    //設定ファイルを上書きしても良いか，確認する．
    QMessageBox::StandardButton ret;
    ret = QMessageBox::question(this, tr("Pic2NC"),
                                tr("Do you want to overwrite the settings file?"),
                                QMessageBox::Yes | QMessageBox::No);
    if(ret == QMessageBox::No){
        return;
    }

    SaveSettings();
}

void MainWindow::SlotLoadSettings()
{
    //設定ファイルを読み込んでよいか，確認する．
    QMessageBox::StandardButton ret;
    ret = QMessageBox::question(this, tr("Pic2NC"),
                                tr("Do you want to load the settings file?"),
                                QMessageBox::Yes | QMessageBox::No);
    if(ret == QMessageBox::No){
        return;
    }

    LoadSettings();
}

void MainWindow::SlotLoadDefaultSettings()
{
    //デフォルト設定を復元しても良いか，確認する．
    QMessageBox::StandardButton ret;
    ret = QMessageBox::question(this, tr("Pic2NC"),
                                tr("Do you want to restore the default settings?"),
                                QMessageBox::Yes | QMessageBox::No);
    if(ret == QMessageBox::No){
        return;
    }

    LoadDefaultSettings();
}


void MainWindow::on_radioButton_toggled(bool checked)
{
    if(checked){
        scene_drawing_.pen_.setColor(Qt::black);
    } else {
        scene_drawing_.pen_.setColor(Qt::white);
    }
}


void MainWindow::on_tabWidgetImageDrawing_currentChanged(int index)
{
    if(index == 1){
        ui->pushButtonGenrerate->setEnabled(true);
    } else if(image_ == std::nullptr_t()) {
        ui->pushButtonGenrerate->setEnabled(false);
    }
}


void MainWindow::on_pushButtonClear_clicked()
{
    ui->graphicsViewDrawing->scene()->clear();
}


void MainWindow::on_horizontalSliderPenWidth_valueChanged(int value)
{
    scene_drawing_.pen_.setWidth(value);
}

