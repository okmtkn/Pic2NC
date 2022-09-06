#include "imageprocessing.h"

ImageProcessing::ImageProcessing(QString file_name, int median_ksize, int morphology_open, int morphology_close)
{
    //original_image_ = cv::imread(file_name.toStdString(), cv::IMREAD_GRAYSCALE);
    //OpenCVのimreadは日本語ファイル名に対応していない．一旦，QImageで開いてから，cv::Matに変換している．
    original_image_ = QimageToMat(QImage(file_name).convertToFormat(QImage::Format_Grayscale8), cv::IMREAD_GRAYSCALE);

    median_ksize_ = median_ksize;
    morphology_open_ = morphology_open;
    morphology_close_ = morphology_close;

    if(!original_image_.empty()){
        noise_removed_image_ = original_image_.clone();
        output_image_ = original_image_.clone();
    }
}


ImageProcessing::~ImageProcessing()
{

}


bool ImageProcessing::Empty()
{
    return original_image_.empty();
}


void ImageProcessing::ImageProcess()
{
    if(original_image_.empty()){
        return;
    }
    //まずは入力画像のコピーを作る．
    //以後，noise_removed_imageに対して種々の画像処理を行っていく
    if( !roi_rect_.isValid() || roi_rect_.isNull() || roi_rect_.isEmpty() ){
        noise_removed_image_ = original_image_.clone();
    } else {
        noise_removed_image_ = cv::Mat(original_image_.clone(), cv::Rect(roi_rect_.x(), roi_rect_.y(), roi_rect_.width(), roi_rect_.height()));
    }

    if(median_ksize_>=3){
        cv::medianBlur(noise_removed_image_,noise_removed_image_, median_ksize_);
    }

    cv::threshold(noise_removed_image_, noise_removed_image_, 0, 255, cv::THRESH_OTSU);

    //白点の除去：線のかすれを繋ぐ．
    if( morphology_open_ >= 2 ){
        cv::morphologyEx(
                    noise_removed_image_,
                    noise_removed_image_,
                    cv::MORPH_OPEN,
                    cv::getStructuringElement(
                        cv::MORPH_ELLIPSE, cv::Size(morphology_open_, morphology_open_)));
    }

    //黒点の除去：紙の汚れを除去
    if( morphology_close_ >= 2 ){
        cv::morphologyEx(
                    noise_removed_image_,
                    noise_removed_image_,
                    cv::MORPH_CLOSE,
                    cv::getStructuringElement(
                        cv::MORPH_ELLIPSE, cv::Size(morphology_close_, morphology_close_)));
    }

    output_image_ = noise_removed_image_.clone();
    //輪郭を検出
    //cv::Canny(noise_removed_image_, output_image_, canny_low_val_, canny_high_val_);
    //RemoveUnnecessaryPoints(output_image_);
}


//cv::Canny()は完全なエッジではなく，4近傍で接続している場合がある．これを選択的に削除する関数．
void ImageProcessing::RemoveUnnecessaryPoints(cv::Mat image)
{
    //cv::Mat からPixel値にアクセス
    for(int ix=1; ix<image.size().width-1; ix++){
        for(int iy=1; iy<image.size().height-1; iy++){
            if(image.at<unsigned char>(iy, ix) == 0){
                //孤立点を除去
                if(
                        image.at<unsigned char>(iy, ix+1) == 255 && //右
                        image.at<unsigned char>(iy-1, ix+1) == 255 && //右上
                        image.at<unsigned char>(iy-1, ix) == 255 && //上
                        image.at<unsigned char>(iy-1, ix-1) == 255 && //左上
                        image.at<unsigned char>(iy, ix-1) == 255 && //左
                        image.at<unsigned char>(iy+1, ix-1) == 255 && //左下
                        image.at<unsigned char>(iy+1, ix) == 255 && //下
                        image.at<unsigned char>(iy+1, ix+1) == 255 //右下
                        ){
                    image.at<unsigned char>(iy, ix) = 0;
                }
            }
        }
    }
}


cv::Mat ImageProcessing::QimageToMat(QImage const &img, int format) {
    return cv::Mat(img.height(), img.width(), format,
                   const_cast<uchar*>(img.bits()),
                   img.bytesPerLine()).clone();
}


QImage ImageProcessing::get_original_image_()
{
    return QImage(
                (unsigned char*) original_image_.data,
                original_image_.cols,
                original_image_.rows,
                original_image_.step,
                QImage::Format_Indexed8
                );
}

QImage ImageProcessing::get_noise_removed_image_()
{
    return QImage(
                (unsigned char*) noise_removed_image_.data,
                noise_removed_image_.cols,
                noise_removed_image_.rows,
                noise_removed_image_.step,
                QImage::Format_Indexed8
                );
}


QImage ImageProcessing::get_output_image_()
{
    return QImage(
                (unsigned char*) output_image_.data,
                output_image_.cols,
                output_image_.rows,
                output_image_.step,
                QImage::Format_Indexed8
                );
}


void ImageProcessing::set_roi_rect_(QRect rect)
{
    roi_rect_ = rect;

    if( roi_rect_.x() < 0 )
        roi_rect_.setX( 0 );
    if( roi_rect_.y() < 0 )
        roi_rect_.setY( 0 );

    if( roi_rect_.x() + roi_rect_.width() >= original_image_.cols )
        roi_rect_.setWidth( original_image_.cols-1 - roi_rect_.x() );

    if( roi_rect_.y() + roi_rect_.height() >= original_image_.rows )
        roi_rect_.setHeight( original_image_.rows-1 - roi_rect_.y() );
}


void ImageProcessing::set_median_ksize_(int value)
{
    median_ksize_ = value;
}


void ImageProcessing::set_morphology_open_(int value)
{
    morphology_open_ = value;
}


void ImageProcessing::set_morphology_close_(int value)
{
    morphology_close_ = value;
}
