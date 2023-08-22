#ifndef NCDATA_H
#define NCDATA_H

#include <iostream>
#include <iomanip>
#include <cmath>
#include <QObject>
#include <QPixmap>
#include <QApplication> //長い処理中にGUIの応答がなくなって固まるので， QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);を適宜実行している．

#include <QGraphicsScene>
#include <QGraphicsItem>

#include "cuttingpoint.h"


class NcData : public QObject
{
    Q_OBJECT

public:
    //NcData(cv::Mat image);    //将来的にQImageにしたい．廃止予定．
    NcData(QImage image,
           QString output_file_name,
           float work_width,
           float work_height,
           float retract_height,
           float cutting_depth,
           int   spindle_speed,
           int   feed_rate,
           float offset_x,
           float offset_y,
           float offset_z,
           int   num_of_ignore,
           float tolerance,
           bool  enable_offset_pocket_milling,
           bool  enable_cutter_compensation,
           float cutter_r,
           float pickfeed
           );
    ~NcData();

public:
    void  GenerateNcData();
    void  DrawNcView(QGraphicsScene *scene, QRect view_size);
    float GetG0Length();
    float GetG1Length();
    float GetTotalTime();
    int   get_length_output_();

public:
    //SearchCuttingPoint()の冒頭で1%に設定し，
    //2optで98%にして，
    //保存まで完了しきったら100%にしている
    void set_progress_bar_value_(int value);

signals:
    void ProgressBarValueChanged(int value);

private:
    void SearchCuttingPoint(); //画像全域から切削点を探して格納する
    bool OffsetPocketMilling();   //ポケット加工をoffset[mm]で実行する関数
    void RouteOptimization2Opt();
    void MakeCuttingCode(); //各切削点にG0 or G1を設定する関数
    void RemoveIsolatedCuttingPoint();
    void RemoveLinearCuttingPoint();
    void OutputNcData(); //送り方向を計算しながら，切削点を削減する

    float  CalcDistance(CuttingPoint p1, CuttingPoint p2);
    int    CalcDistanceLite(CuttingPoint p1, CuttingPoint p2);
    void   SearchCuttingPointNeighbor(int x, int y);
    void   SetCuttingPoint(int x, int y);
    bool   Closing(int pixel);   //収縮処理をpixel回だけ実行する関数．
    bool   IsEdge4(int x, int y);
    bool   IsEdge8(int x, int y);

private:
    QList<CuttingPoint> cutting_point_all_;
    QList<CuttingPoint> cutting_point_output_;
    float retract_threshold_ = 3; //このピクセル数以下が，連続しているとみなす．

    int   progress_bar_value_ = 0;

    QString output_file_name_;

    QImage image_;
    float work_width_;
    float work_height_;
    float retract_height_;
    float cutting_depth_;
    int   spindle_speed_;
    int   feed_rate_;
    float offset_x_;
    float offset_y_;
    float offset_z_;
    float scale_;
    int   num_of_ignore_;
    float tolerance_;
    bool  enable_offset_pocket_milling_;
    bool  enable_cutter_compensation_;
    float cutter_r_;
    float pickfeed_;
};

#endif // NCDATA_H
