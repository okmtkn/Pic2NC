#ifndef IMAGEPROCESSING_H
#define IMAGEPROCESSING_H

#include <QObject>
#include <QImage>
#include <opencv2/opencv.hpp>


class ImageProcessing
{
public:
    ImageProcessing( QString file_name, int median_ksize, int morphology_open, int morphology_close );
    ~ImageProcessing();

public:
    bool Empty();
    void ImageProcess(void);

private:
    void RemoveUnnecessaryPoints(cv::Mat image);
    cv::Mat QimageToMat(QImage const &img, int format);

public:
    QImage get_original_image_();
    QImage get_noise_removed_image_();
    QImage get_output_image_();
    void set_roi_rect_(QRect rect);
    void set_median_ksize_(int value);
    void set_morphology_open_(int value);
    void set_morphology_close_(int value);

private:
    cv::Mat original_image_;
    cv::Mat noise_removed_image_;
    cv::Mat output_image_;

    QRect roi_rect_;

    int median_ksize_;
    int morphology_open_;
    int morphology_close_;
};

#endif // IMAGEPROCESSING_H
