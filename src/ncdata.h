#ifndef NCDATA_H
#define NCDATA_H

#include <iostream>
#include <iomanip>
#include <cmath>
#include <QObject>
#include <QPixmap>
#include <QApplication> //長い処理中にGUIの応答がなくなって固まるので， QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);を適宜実行している．

#include <QGraphicsScene>   //描画用
#include <QGraphicsItem>    //描画用
#include <QFile>        //ファイル入出力用
#include <QTextStream>  //ファイル入出力用
#include <QFileInfo>    //ファイル入出力用
#include <QMessageBox>  //メッセージボックス用

#include "cuttingpoint.h"

#include <opencv2/opencv.hpp>   //ドロネー三角形分割のために使用．
#include <vector>               //ドロネー三角形分割のために使用．


class NcData : public QObject
{
    Q_OBJECT

public:
    // Function: NcData
    // コンストラクタ
    //
    // Parameters:
    // image - 画像データ
    // output_file_name - 出力ファイル名
    // file_type - 出力ファイルの種類
    // work_width - 加工領域の幅[mm]
    // work_height - 加工領域の高さ[mm]
    // retract_height - リトラクト高さ[mm]
    // cutting_depth - 切削深さ[mm]
    // spindle_speed - スピンドル回転数[rpm]
    // feed_rate - 送り速度[mm/min]
    // offset_x - X軸オフセット[mm]
    // offset_y - Y軸オフセット[mm]
    // offset_z - Z軸オフセット[mm]
    // num_of_ignore - 画像の端から無視するピクセル数
    // tolerance - トレランス[mm]
    // enable_offset_pocket_milling - ポケット加工を有効にするか
    // enable_cutter_compensation - カッタ補正を有効にするか
    // cutter_r - 工具半径[mm]
    // pickfeed - ピックフィード[mm/min]
    // stl_height - STLファイルとして出力するモデルの高さ方向寸法[mm]
    NcData(QImage image,
           QString output_file_name,
           QString file_type,
           double work_width,
           double work_height,
           double retract_height,
           double cutting_depth,
           int   spindle_speed,
           int   feed_rate,
           double offset_x,
           double offset_y,
           double offset_z,
           int   num_of_ignore,
           double tolerance,
           bool  enable_offset_pocket_milling,
           bool  enable_cutter_compensation,
           double cutter_r,
           double pickfeed,
           double stl_height
           );
    ~NcData();

public:
    // Function: GenerateNcData
    // NCデータを生成する関数
    void  GenerateNcData();

    // Function: DrawNcView
    // NCデータを描画する関数
    //
    // Parameters:
    // scene - 描画するシーン
    // view_size - 描画する領域の大きさ
    void  DrawNcView(QGraphicsScene *scene, QRect view_size);

    // Function: GetG0Length
    // G0の移動距離を取得する関数
    //
    // Returns:
    // G0の移動距離[mm]
    double GetG0Length();

    // Function: GetG1Length
    // G1の移動距離を取得する関数
    double GetG1Length();

    // Function: GetTotalTime
    // NC処理にかかる時間を取得する関数
    double GetTotalTime();

    // Function: GetOutputString
    // 出力文字列を取得する関数
    //
    // Returns:
    // 出力文字列
    QString GetOutputString();

    // Function: GetOutputFileName
    // 切削点の数を取得する関数
    //
    // Returns:
    // 切削点の数
    int   get_cutting_point_length_();


public:
    // Function: set_progress_bar_value_
    // プログレスバーの値を設定する関数
    // SearchCuttingPoint()の冒頭で1%に設定し，
    // NC処理でで90%にして，
    // 保存まで完了しきったら100%にしている
    void set_progress_bar_value_(int value);

signals:
    // Function: ProgressBarValueChanged
    // プログレスバーの値を変更するシグナル
    void ProgressBarValueChanged(int value);

private:
    // Function: SearchCuttingPoint
    // 画像全域から切削点を探して格納する関数
    void SearchCuttingPoint();

    // Function: OffsetPocketMilling
    // ポケット加工をoffset[mm]で実行する関数
    bool OffsetPocketMilling();

    // Function: CutterCompensation
    // 2-opt法によるルート最適化を実行する関数
    void RouteOptimization2Opt();

    // Function: RouteOptimization2Opt
    // 各切削点にG0 or G1を設定する関数
    void MakeCuttingCode();

    // Function: RemoveIsolatedCuttingPoint
    // 孤立した工具経路を削除する．
    void RemoveIsolatedCuttingPoint();

    // Function: RemoveLinearCuttingPoint
    // トレランスの範囲内に収まる余分な切削点を削除する．
    void RemoveLinearCuttingPoint();

    // Function: GenerateNcString
    // Gコードフォーマットの文字列を生成する．
    void GenerateNcString();

    // Function: GenerateDxfString
    // DXFフォーマットの文字列を生成する．
    void GenerateDxfString();

    // Function: GenerateStlString
    // ドロネー三角形分割を利用して，黒色部分を三角形に分割して，
    // STLフォーマットの文字列を生成する．
    void GenerateStlString();

    // Function: intersect
    // ドロネー三角形分割の修正のため．線分の交差を判定する関数．
    //
    // Parameters:
    // A - 線分1の始点
    // B - 線分1の終点
    // C - 線分2の始点
    // D - 線分2の終点
    //
    // Returns:
    // 交差していればtrue，していなければfalse
    bool intersect(cv::Point2d A, cv::Point2d B, cv::Point2d C, cv::Point2d D);

    // Function: CalcDistance
    // 2点間の距離を計算する関数．
    //
    // Parameters:
    // p1 - 切削点1
    // p2 - 切削点2
    //
    // Returns:
    // 2点間の距離
    double  CalcDistance(CuttingPoint p1, CuttingPoint p2);

    // Function: CalcDistanceLite
    // 2点間の距離を簡易的に計算する関数．ルート計算を省いた距離．
    //
    // Parameters:
    // p1 - 切削点1
    // p2 - 切削点2
    //
    // Returns:
    // 2点間の距離
    int    CalcDistanceLite(CuttingPoint p1, CuttingPoint p2);

    // Function: SearchCuttingPointNeighbor
    // ある切削点の近傍にある切削点を探す関数．
    //
    // Parameters:
    // x - 切削点のx座標
    // y - 切削点のy座標
    void   SearchCuttingPointNeighbor(int x, int y);

    // Function: SetCuttingPoint
    // 切削点を新たに設定する関数．
    //
    // Parameters:
    // x - 切削点のx座標
    // y - 切削点のy座標
    void   SetCuttingPoint(int x, int y);

    // Function: Closing
    // 収縮処理をpixel回だけ実行する関数．
    //
    // Parameters:
    // pixel - 収縮処理を行う回数
    //
    // Returns:
    // 収縮処理を行えたかどうか．行えたらtrue，行えなかったらfalse
    bool   Closing(int pixel);

    // Function: IsEdge4
    // 4近傍のエッジを判定する関数．
    //
    // Parameters:
    // x - 切削点のx座標
    // y - 切削点のy座標
    //
    // Returns:
    // エッジならtrue，エッジでなければfalse
    bool   IsEdge4(int x, int y);

    // Function: IsEdge8
    // 8近傍のエッジを判定する関数．
    //
    // Parameters:
    // x - 切削点のx座標
    // y - 切削点のy座標
    //
    // Returns:
    // エッジならtrue，エッジでなければfalse
    bool   IsEdge8(int x, int y);

    // Function: OutputFile
    // 出力ファイルを生成する関数．
    void   OutputFile();

    // Function: GetIntersection
    // 2つの線分の交点を計算する関数．
    //
    // Parameters:
    // A - 線分1の始点
    // B - 線分1の終点
    // C - 線分2の始点
    // D - 線分2の終点
    //
    // Returns:
    // 交点
    cv::Point2d GetIntersection(cv::Point2d A, cv::Point2d B, cv::Point2d C, cv::Point2d D);

    // Function: GetTriangleRotationDirection
    // 三角形の回転方向を計算する関数．
    //
    // Parameters:
    // vertex0 - 三角形の頂点0
    // vertex1 - 三角形の頂点1
    // vertex2 - 三角形の頂点2
    //
    // Returns:
    // 回転方向．反時計回りなら1，時計回りなら-1，直線状など無効な三角形なら0
    int GetTriangleRotationDirection(cv::Point2d vertex0, cv::Point2d vertex1, cv::Point2d vertex2);

private:
    // Variable: cutting_point_all_
    // 画像全域から探した切削点を格納するリスト
    QList<CuttingPoint> cutting_point_all_;

    // Variable: cutting_point_output_
    // 出力する切削点を格納するリスト
    QList<CuttingPoint> cutting_point_output_;

    // Variable: retract_threshold_
    // リトラクトするピクセル数の閾値．このピクセル数以下が，連続しているとみなす．
    double retract_threshold_ = 3;

    // Variable: progress_bar_value_
    // プログレスバーの値
    int   progress_bar_value_ = 0;

    // Variable: output_file_name_
    // 出力ファイル名
    QString output_file_name_;

    // Variable: file_type_
    // 出力ファイルの種類
    QString file_type_;

    // Variable: output_ascii_data_
    // 出力する文字列
    QString output_ascii_data_;

    // Variable: image_
    // 入力画像
    QImage image_;

    // Variable: image_width_
    // 入力画像の幅
    double work_width_;

    // Variable: image_height_
    // 入力画像の高さ
    double work_height_;

    // Variable: retract_height_
    // リトラクト高さ
    double retract_height_;

    // Variable: cutting_depth_
    // 切削深さ
    double cutting_depth_;

    // Variable: spindle_speed_
    // 主軸回転数[rpm]
    int   spindle_speed_;

    // Variable: feed_rate_
    // 送り速度[mm/min]
    int   feed_rate_;

    // Variable: offset_x_
    // X方向のオフセット[mm]
    double offset_x_;

    // Variable: offset_y_
    // Y方向のオフセット[mm]
    double offset_y_;

    // Variable: offset_z_
    // Z方向のオフセット[mm]
    double offset_z_;

    // Variable: scale_
    // 画像のスケール
    double scale_;

    // Variable: num_of_ignore_
    // 無視する切削点の数
    int   num_of_ignore_;

    // Variable: tolerance_
    // トレランス[px]
    double tolerance_;

    // Variable: enable_offset_pocket_milling_
    // ポケット加工を行うかどうか true:行う false:行わない
    bool  enable_offset_pocket_milling_;

    // Variable: enable_cutter_compensation_
    // 工具径補正を行うかどうか true:行う false:行わない
    bool  enable_cutter_compensation_;

    // Variable: cutter_r_
    // 工具半径[mm]
    double cutter_r_;

    // Variable: pickfeed_
    // ピックフィード[px]
    double pickfeed_;

    // Variable: stl_width_
    // STL形式で出力するモデルの高さ寸法[mm]
    double stl_height_;
};

#endif // NCDATA_H
