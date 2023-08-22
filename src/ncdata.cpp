#include "ncdata.h"

/*
NcData::NcData(cv::Mat image)
{
    image_width_ = image.size().width;
    image_height_ = image.size().height;
    //cutting_pointの数は，最大でimage_width_*image_height_個あればいい
    cutting_point_all_ = new QPointF[image_width_*image_height_];

    //cv::Mat からPixel値にアクセス
    for(int ix=0; ix<image_width_; ix++){
        for(int iy=0; iy<image_height_; iy++){
            //輝度値があるピクセルを，切削点として登録していく
            if(image.at<unsigned char>(iy, ix) == 255){
                cutting_point_all_[length_all_].setX(ix);
                cutting_point_all_[length_all_].setY(iy);
                length_all_++;
            }
        }
    }
}
*/

NcData::NcData(QImage image,
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
               )
{
    image_ = image.copy();  //コピーして保持しておくことで，NcDataをdeleteしたときにimage本体が開放されないようにする
    image_ = image_.convertToFormat(QImage::Format_Indexed8);
    //image = image.convertToFormat(QImage::Format_Mono);

    output_file_name_ = output_file_name;
    work_width_ = work_width;
    work_height_= work_height;
    retract_height_ = retract_height;
    cutting_depth_ = cutting_depth;
    spindle_speed_ = spindle_speed;
    feed_rate_ = feed_rate;
    offset_x_ = offset_x;
    offset_y_ = offset_y;
    offset_z_ = offset_z;
    num_of_ignore_ = num_of_ignore;
    tolerance_ = tolerance;
    enable_offset_pocket_milling_ = enable_offset_pocket_milling;
    enable_cutter_compensation_ = enable_cutter_compensation;
    cutter_r_ = cutter_r;
    pickfeed_ = pickfeed;

    //NCデータのスケール,原点位置を設定
    float scale_x = work_width_ / image_.width();
    float scale_y = work_height_ / image_.height();
    if(scale_x > scale_y){
        scale_ = scale_y;
    } else {
        scale_ = scale_x;
    }
    //image_.save("demo.png");
}


NcData::~NcData()
{

}

void NcData::GenerateNcData()
{
    //ui->statusBar->showMessage("Searching for cutting point...");
    if(enable_offset_pocket_milling_){
        OffsetPocketMilling();
    } else {
        SearchCuttingPoint();
    }
    set_progress_bar_value_(20);

    //ui->statusBar->showMessage("2-opt...");
    RouteOptimization2Opt();
    set_progress_bar_value_(40);

    MakeCuttingCode();

    //ui->statusBar->showMessage("Removing Isolated Cutting Points...");
    RemoveIsolatedCuttingPoint();
    set_progress_bar_value_(60);

    //ui->statusBar->showMessage("2-opt again...");
    RouteOptimization2Opt();
    set_progress_bar_value_(70);

    MakeCuttingCode();

    //ui->statusBar->showMessage("Remove Unnesessary Cutting Points...");
    RemoveLinearCuttingPoint();
    set_progress_bar_value_(80);

    //ui->statusBar->showMessage("Output NC data...");
    OutputNcData();
    set_progress_bar_value_(90);
}


// 輪郭追跡法のように切削点を探してみている．
// なんちゃって輪郭追跡法であっても，ある程度は順番が整列されるから，後の順番最適計算が少しだけ高速になる．
// 切削点：0
// 探索済点：128
void NcData::SearchCuttingPoint()
{
    for(int ix=0; ix<image_.width(); ix++){
        for(int iy=0; iy<image_.height(); iy++){
            if( IsEdge4(ix, iy) ){
                SetCuttingPoint(ix, iy);
                SearchCuttingPointNeighbor( ix, iy );
            }
        }
    }
}


bool NcData::OffsetPocketMilling()
{
    int progress = 0;

    int cutter_r_pixel = cutter_r_ / scale_/2.;
    int pickfeed_pixel = pickfeed_ / scale_ / 2.;  //1回で4近傍と8近傍の2回を実行するから，1/2倍にしている


    //工具径補正
    if( enable_cutter_compensation_ && cutter_r_ > 0 ){
        Closing( cutter_r_pixel );
    }

    SearchCuttingPoint();   //収縮処理を実行する前に，輪郭を検出しておく
    while( Closing( pickfeed_pixel ) ){  //pixel回の収縮処理を行う．黒色がなくなるまで繰り返す
        //長い処理中にGUIの応答がなくなって固まるので対策
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        SearchCuttingPoint();
        progress++;
        set_progress_bar_value_(progress);
    }
    return true;
}


void NcData::SearchCuttingPointNeighbor(int x, int y)
{
    //8近傍もまた，切削点である可能性が高い．
    //順序最適計算の高速化のため，8近傍を探索して，ある程度順序を整列しておいて，高速化する．
    //このコードは，無くても動作には差し支えない

    bool neighbor_is_cutting_point = true;
    while(neighbor_is_cutting_point){
        neighbor_is_cutting_point = false;
        // 右
        if( x < image_.width()-1 ){
            x++;
            if( IsEdge4(x, y) ){
                SetCuttingPoint(x, y);
                neighbor_is_cutting_point = true;
                continue;
            }
        }
        // 右下
        if( y < image_.height()-1 ){
            y++;
            if( IsEdge4(x, y) ){
                SetCuttingPoint(x, y);
                neighbor_is_cutting_point = true;
                continue;
            }
        }
        // 下
        if( x > 0 ) {
            x--;
            if( IsEdge4(x, y) ){
                SetCuttingPoint(x, y);
                neighbor_is_cutting_point = true;
                continue;
            }
        }
        // 左下
        if( x > 0 ) {
            x--;
            if( IsEdge4(x, y) ){
                SetCuttingPoint(x, y);
                neighbor_is_cutting_point = true;
                continue;
            }
        }
        // 左
        if( y > 0 ) {
            y--;
            if( IsEdge4(x, y) ){
                SetCuttingPoint(x, y);
                neighbor_is_cutting_point = true;
                continue;
            }
        }
        // 左上
        if( y > 0 ) {
            y--;
            if( IsEdge4(x, y) ){
                SetCuttingPoint(x, y);
                neighbor_is_cutting_point = true;
                continue;
            }
        }
        // 上
        if( x < image_.width()-1 ) {
            x++;
            if( IsEdge4(x, y) ){
                SetCuttingPoint(x, y);
                neighbor_is_cutting_point = true;
                continue;
            }
        }
        // 右上
        if( x < image_.width()-1 ) {
            x++;
            if( IsEdge4(x, y) ){
                SetCuttingPoint(x, y);
                neighbor_is_cutting_point = true;
                continue;
            }
        }
    }
}


//length_all_をカウントしながら，座標・切削コードの設定を行う関数
void NcData::SetCuttingPoint(int x, int y)
{
    cutting_point_all_.append(CuttingPoint(x,y));

    //探索済の点は，255と0以外の数値に設定しておく．これが探索済: 足跡となる．
    ((uchar *)image_.bits())[x*image_.depth()/8 + y*image_.bytesPerLine()] = 128;
}


//4近傍と8近傍を交互に繰り返すことで，4方向ではなく8方向に収縮させている
bool NcData::Closing(int pixel)
{
    if(pixel<=0){
        pixel = 1;
    }

    bool still_black = false;

    for(int i=0; i<pixel; i++){

        //128と記した場所はすでに切削点として登録済なので白色に変えて除去する
        for(int ix=0; ix<image_.width(); ix++){
            for(int iy=0; iy<image_.height(); iy++){
                if( ((uchar *)image_.bits())[ix*image_.depth()/8 + iy*image_.bytesPerLine()] == 128){
                    ((uchar *)image_.bits())[ix*image_.depth()/8 + iy*image_.bytesPerLine()] = 255;
                }
            }
        }

        //4近傍をclosingする
        for(int ix=0; ix<image_.width(); ix++){
            for(int iy=0; iy<image_.height(); iy++){
                if( IsEdge4(ix, iy) ){
                    ((uchar *)image_.bits())[ix*image_.depth()/8 + iy*image_.bytesPerLine()] = 128;
                    still_black = true;
                }
            }
        }

        //128と記した場所を除去する
        for(int ix=0; ix<image_.width(); ix++){
            for(int iy=0; iy<image_.height(); iy++){
                if( ((uchar *)image_.bits())[ix*image_.depth()/8 + iy*image_.bytesPerLine()] == 128){
                    ((uchar *)image_.bits())[ix*image_.depth()/8 + iy*image_.bytesPerLine()] = 255;
                }
            }
        }

        //8近傍をclosingする
        for(int ix=0; ix<image_.width(); ix++){
            for(int iy=0; iy<image_.height(); iy++){
                if( IsEdge8(ix, iy) ){
                    ((uchar *)image_.bits())[ix*image_.depth()/8 + iy*image_.bytesPerLine()] = 128;
                    still_black = true;
                }
            }
        }

        //128と記した場所を除去する
        for(int ix=0; ix<image_.width(); ix++){
            for(int iy=0; iy<image_.height(); iy++){
                if( ((uchar *)image_.bits())[ix*image_.depth()/8 + iy*image_.bytesPerLine()] == 128){
                    ((uchar *)image_.bits())[ix*image_.depth()/8 + iy*image_.bytesPerLine()] = 255;
                }
            }
        }
        if(!still_black){
            break;
        }
    }

    return still_black;
}


//4近傍を調べて，エッジかどうかを返す関数
bool NcData::IsEdge4(int x, int y)
{
    if( ((uchar *)image_.bits())[x*image_.depth()/8 + y*image_.bytesPerLine()] != 0 ){
        return false;
    }
    //画像の端部の場合は，黒色ならばエッジと判定する
    if(x==0 || y==0 || x==image_.width()-1 || y==image_.height()-1){
        if( ((uchar *)image_.bits())[x*image_.depth()/8 + y*image_.bytesPerLine()] == 0 ){
            return true;
        } else {
            return false;
        }
    } else if( ((uchar *)image_.bits())[x*image_.depth()/8 + y*image_.bytesPerLine()] == 0 &&   //端部以外のすべてのピクセルの場合，4近傍が255ならエッジと判定する
               (
                   ((uchar *)image_.bits())[(x+1)*image_.depth()/8 + (y  )*image_.bytesPerLine()] == 255 || //右
                   ((uchar *)image_.bits())[(x  )*image_.depth()/8 + (y-1)*image_.bytesPerLine()] == 255 || //上
                   ((uchar *)image_.bits())[(x-1)*image_.depth()/8 + (y  )*image_.bytesPerLine()] == 255 || //左
                   ((uchar *)image_.bits())[(x  )*image_.depth()/8 + (y+1)*image_.bytesPerLine()] == 255    //下
                   )
               ){
        return true;
    } else {
        return false;
    }
}


bool NcData::IsEdge8(int x, int y)
{
    if( ((uchar *)image_.bits())[x*image_.depth()/8 + y*image_.bytesPerLine()] != 0 ){
        return false;
    }
    //画像の端部の場合は，黒色ならばエッジと判定する
    if(x==0 || y==0 || x==image_.width()-1 || y==image_.height()-1){
        if( ((uchar *)image_.bits())[x*image_.depth()/8 + y*image_.bytesPerLine()] == 0 ){
            return true;
        } else {
            return false;
        }
    } else if( ((uchar *)image_.bits())[x*image_.depth()/8 + y*image_.bytesPerLine()] == 0 &&   //端部以外のすべてのピクセルの場合，4近傍が255ならエッジと判定する
               (
                   ((uchar *)image_.bits())[(x+1)*image_.depth()/8 + (y  )*image_.bytesPerLine()] == 255 || //右
                   ((uchar *)image_.bits())[(x  )*image_.depth()/8 + (y-1)*image_.bytesPerLine()] == 255 || //上
                   ((uchar *)image_.bits())[(x-1)*image_.depth()/8 + (y  )*image_.bytesPerLine()] == 255 || //左
                   ((uchar *)image_.bits())[(x  )*image_.depth()/8 + (y+1)*image_.bytesPerLine()] == 255 || //下
                   ((uchar *)image_.bits())[(x+1)*image_.depth()/8 + (y+1)*image_.bytesPerLine()] == 255 || //右下
                   ((uchar *)image_.bits())[(x+1)*image_.depth()/8 + (y-1)*image_.bytesPerLine()] == 255 || //右上
                   ((uchar *)image_.bits())[(x-1)*image_.depth()/8 + (y-1)*image_.bytesPerLine()] == 255 || //左上
                   ((uchar *)image_.bits())[(x-1)*image_.depth()/8 + (y+1)*image_.bytesPerLine()] == 255    //左下
                   )
               ){
        return true;
    } else {
        return false;
    }
}


void NcData::RemoveIsolatedCuttingPoint()
{
    int one_tool_path_length = 1;

    for(int i=1; i<cutting_point_all_.size(); i++){   //i=0はG0する点だから，確認する必要はない
        if(cutting_point_all_[i].get_cutting_code_() == 1){
            one_tool_path_length ++;    //G1が連続する数をカウントする
        } else {    //G0のとき
            if(one_tool_path_length <= num_of_ignore_){  //ここでG0する前回の切削経路で，連続するG1が少なければ
                for(int j=0; j<one_tool_path_length; j++){
                    cutting_point_all_.removeAt(i-j-1);
                }
                i -= one_tool_path_length;
            }
            one_tool_path_length = 1;
        }
    }

    //最後のG0が余計な経路ならば除去する．
    if(one_tool_path_length <= num_of_ignore_){
        for(int j=0; j<one_tool_path_length; j++){
            cutting_point_all_.removeAt(cutting_point_all_.size()-j-1);
        }
    }
}


void NcData::RemoveLinearCuttingPoint()
{
    float a, b, c; //直線の方程式 ax + by + c = 0
    float denominator;

    float tmp_distance;
    float max_distance = 0;
    int   output_point_num;

    cutting_point_output_.append(cutting_point_all_[0]);
    cutting_point_output_[0].set_cutting_code_(0);

    //前回G1を出力した点No.
    int G1_i = 0;
    int G0_i = 0;

    for(int i=2; i<cutting_point_all_.size(); i++){
        if(cutting_point_all_[i].get_cutting_code_() == 1){ //G1する候補ならば
            //前回の送り方向から，大きく方向が変わる点のみをG1出力する．
            a = cutting_point_all_[G1_i].get_y_() - cutting_point_all_[i].get_y_();
            b = cutting_point_all_[i].get_x_() - cutting_point_all_[G1_i].get_x_();
            c = cutting_point_all_[G1_i].get_x_() * cutting_point_all_[i].get_y_() -
                    cutting_point_all_[i].get_x_() * cutting_point_all_[G1_i].get_y_();
            denominator = sqrt(a*a+b*b);
            max_distance = 0;
            output_point_num = 0;
            for(int j=G1_i+1; j<i; j++){
                tmp_distance = abs(
                            a*cutting_point_all_[j].get_x_() +
                            b*cutting_point_all_[j].get_y_() + c
                            ) / denominator;
                if(max_distance < tmp_distance){
                    max_distance = tmp_distance;
                    output_point_num = j;
                }
            }
            if( max_distance > tolerance_ ){
                cutting_point_output_.append(cutting_point_all_[output_point_num]);
                cutting_point_output_.last().set_cutting_code_(1);
                G1_i = output_point_num;
                i = G1_i + 2;
            }
        } else if(cutting_point_all_[i].get_cutting_code_() == 0) {
            //前回リトラクトした点とi-1点の距離がちかければ，一周して戻ってきたということ．前回リトラクトした点をG1として出力する．
            if(CalcDistanceLite(cutting_point_all_[i-1], cutting_point_all_[G0_i]) <= retract_threshold_){
                cutting_point_output_.append(cutting_point_all_[G0_i]);
                cutting_point_output_.last().set_cutting_code_(1);
            } else if(G1_i != i-1){  //前回値を出力していなかった場合は，出力しておく．この点は，非ループ形状の終点だ．
                cutting_point_output_.append(cutting_point_all_[i-1]);
                cutting_point_output_.last().set_cutting_code_(1);
            }
            cutting_point_output_.append(cutting_point_all_[i]);
            cutting_point_output_.last().set_cutting_code_(0);
            G0_i = i;
            G1_i = i;
        }
    }
    //切削点データの最後の点がループ形状なら，次の点を出力しておく．
    if(CalcDistanceLite(cutting_point_all_[cutting_point_all_.size()-1], cutting_point_all_[G0_i]) < retract_threshold_){
        cutting_point_output_.append(cutting_point_all_[G0_i]);
        cutting_point_output_.last().set_cutting_code_(1);
    } else if(G1_i != cutting_point_all_.size()-1){  //最後の点を出力していなかった場合は，出力しておく
        cutting_point_output_.append(cutting_point_all_[cutting_point_all_.size()-1]);
        cutting_point_output_.last().set_cutting_code_(1);
    }
}


void NcData::RouteOptimization2Opt()
{
    if(cutting_point_all_.size()<=2){
        return;
    }

    int max_distance;
    int tmp_distance;
    int max_j;

    int distance_ij;
    int distance_i1j1;

    CuttingPoint tmp_point; //切削点の入れ替えに使うtmp

    int *distance = new int[cutting_point_all_.size()-1];

    //リトラクトするコストを与えることで，総切削距離が短い経路を選ぶようになる
    //この値は，距離の近似計算方式に合わせて計算する
    int retract_cost = retract_height_ * 2;


    bool opt = true;
    //while(opt){
    for(int k=0; k<100; k++){
        //長い処理中にGUIの応答がなくなって固まるので対策
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        opt = false;

        //高速化のため，あらかじめ点間の距離を配列に格納しておく．
        for(int i=0; i<cutting_point_all_.size()-1; i++){
            distance[i] = CalcDistanceLite(cutting_point_all_[i], cutting_point_all_[i+1]);
            if(distance[i] > retract_threshold_ ){  //リトラクトするコストを与えることで，総切削距離が短い経路を選ぶようになる
                distance[i] += retract_cost;
            }
        }

        for(int i=0; i<cutting_point_all_.size()-3; i++){
            //すでに十分近い点は，最適化の計算から除外する
            if(distance[i] == 1) continue;
            max_distance = 0;
            max_j = 0;
            for(int j=i+2; j<cutting_point_all_.size()-2; j++){
                //すでに十分近い点は，経路の交換先から除外する
                if(distance[j] == 1) continue;
                distance_ij = CalcDistanceLite(cutting_point_all_[i],cutting_point_all_[j]);
                if(distance_ij > retract_threshold_ ){  //リトラクトするコストを与えることで，総切削距離が短い経路を選ぶようになる
                    distance_ij += retract_cost;
                }
                distance_i1j1 = CalcDistanceLite(cutting_point_all_[i+1],cutting_point_all_[j+1]);
                if(distance_i1j1 > retract_threshold_ ){
                    distance_i1j1 += retract_cost;
                }
                tmp_distance =
                        distance[i] + distance[j] -
                        distance_ij - distance_i1j1;
                if(tmp_distance - max_distance > 0){
                    max_distance = tmp_distance;
                    max_j = j;
                }
            }
            if( max_j != 0 && max_distance > 0 ){    //もし，j番目の方が距離が短ければ
                //入れ替える
                for(int n=0; n<int((max_j-i)/2.); n++){
                    tmp_point = cutting_point_all_[i+1+n];
                    cutting_point_all_[i+1+n] = cutting_point_all_[max_j-n];
                    cutting_point_all_[max_j-n] = tmp_point;
                }
                //入れ替えた部分だけ距離を計算しなおす
                for(int k=i; k<=max_j; k++){
                    distance[k] = CalcDistanceLite(cutting_point_all_[k], cutting_point_all_[k+1]);
                    if(distance[k] > retract_threshold_ ){
                        distance[k] += retract_cost;
                    }
                }
                opt = true;
            }
        }
        if(!opt) break;
    }

    delete[] distance;

    return;
}


void NcData::MakeCuttingCode()
{
    //データ最初の点は，G0する点だ
    cutting_point_all_[0].set_cutting_code_(0);

    for(int i=0; i<cutting_point_all_.size()-1; i++){
        //次の点がG0する点なのか，G1する点なのか設定する
        if(CalcDistanceLite(cutting_point_all_[i], cutting_point_all_[i+1]) <= retract_threshold_){
            cutting_point_all_[i+1].set_cutting_code_(1);
        } else {
            cutting_point_all_[i+1].set_cutting_code_(0);
        }
    }
}


void NcData::OutputNcData()
{
    FILE *fp = fopen(output_file_name_.toLocal8Bit(),"w");

    fprintf(fp,"%%\n(GENERATED BY PIC2NC);\n");
    fprintf(fp,"G17G90G40G80;\n");
    fprintf(fp,"N100(CONTOUR CUT);\n");
    //fprintf(fp,"T10M6;\n");
    fprintf(fp,"G90G54S%dF%d;\n", spindle_speed_, feed_rate_);

    //fprintf(fp,"G43H10Z100.M1;\n");
    fprintf(fp,"G0Z%.3lf;\n", retract_height_ + offset_z_);
    fprintf(fp,"M3;\n");
    //fprintf(fp,"/M8;\n");

    fprintf(fp,"X%.3lfY%.3lf;\n", cutting_point_output_[0].get_x_()*scale_ + offset_x_, - cutting_point_output_[0].get_y_()*scale_ + offset_y_);
    fprintf(fp,"G1Z%.3lf;\n", cutting_depth_ + offset_z_);

    for(int i=1; i<cutting_point_output_.size(); i++){
        if(cutting_point_output_[i].get_cutting_code_() == 1){ //G1する候補ならば
            fprintf(fp,"X%.3lfY%.3lf;\n", cutting_point_output_[i].get_x_()*scale_ + offset_x_, - cutting_point_output_[i].get_y_()*scale_ + offset_y_);
        } else {
            fprintf(fp,"Z%.3lf;\n", retract_height_ + offset_z_);
            fprintf(fp,"G0X%.3lfY%.3lf;\n", cutting_point_output_[i].get_x_()*scale_ + offset_x_, - cutting_point_output_[i].get_y_()*scale_ + offset_y_);
            fprintf(fp,"G1Z%.3lf;\n", cutting_depth_ + offset_z_);   //これが，one tool pathの1点目となる．
        }
    }

    //最後のリトラクト
    fprintf(fp,"G1Z%.3lf;\n", retract_height_ + offset_z_);
    fprintf(fp,"M5;\n");
    //fprintf(fp,"M9;\n");
    fprintf(fp,"M30;\n");
    fprintf(fp,"%%\n");

    fclose(fp);
}

void NcData::DrawNcView(QGraphicsScene *scene, QRect view_size)
{
    float scale_h = float(view_size.height()) / float(image_.height());
    float scale_w = float(view_size.width()) / float(image_.width());
    float scale = scale_h < scale_w ? scale_h : scale_w;

    //scene->setBackgroundBrush(Qt::black);
    scene->clear();

    for(int i=0; i<cutting_point_output_.size()-1; i++){
        if(cutting_point_output_[i+1].get_cutting_code_() == 0){
            scene->addLine(cutting_point_output_[i].get_x_() * scale,
                           cutting_point_output_[i].get_y_() * scale,
                           cutting_point_output_[i+1].get_x_() * scale,
                    cutting_point_output_[i+1].get_y_() * scale,
                    QPen(QColor(255,0,0), 2, Qt::DashLine)
                    );
        } else {
            scene->addLine(cutting_point_output_[i].get_x_() * scale,
                           cutting_point_output_[i].get_y_() * scale,
                           cutting_point_output_[i+1].get_x_() * scale,
                    cutting_point_output_[i+1].get_y_() * scale,
                    QPen(QColor(0,0,0), 1)
                    );
        }
    }
}

float NcData::GetG0Length()
{
    float g0_length = 0;
    //i=0は最初の点なので計算から除外する
    for (int i=1; i<cutting_point_output_.size(); i++){
        if(cutting_point_output_[i].get_cutting_code_() == 0){
            g0_length += CalcDistance(cutting_point_output_[i-1], cutting_point_output_[i]) + retract_height_;
        }
    }
    g0_length *= scale_;
    return g0_length;
}


float NcData::GetG1Length()
{
    float g1_length = 0;
    //i=0は最初の点なので計算から除外する
    for (int i=1; i<cutting_point_output_.size(); i++){
        if(cutting_point_output_[i].get_cutting_code_() == 0){
            g1_length += retract_height_;
        } else {
            g1_length += CalcDistance(cutting_point_output_[i-1], cutting_point_output_[i]);
        }
    }
    g1_length *= scale_;
    return g1_length;
}

int NcData::get_length_output_()
{
    return cutting_point_output_.size();
}



//厳密な距離
float NcData::CalcDistance(CuttingPoint p1, CuttingPoint p2)
{
    return sqrt(( p1.get_x_() - p2.get_x_() ) * ( p1.get_x_() - p2.get_x_() ) +
           ( p1.get_y_() - p2.get_y_() ) * ( p1.get_y_() - p2.get_y_() ));
}



//平方根無しのざっくり距離
//X-Yの2軸が主に動マシニングセンタでは，ユークリッド距離よりもこの距離のほうが切削時間の評価には有効だ．
//しかも計算コストが低い
int NcData::CalcDistanceLite(CuttingPoint p1, CuttingPoint p2)
{
    //return ( p1.get_x_() - p2.get_x_() ) * ( p1.get_x_() - p2.get_x_() ) +
    //       ( p1.get_y_() - p2.get_y_() ) * ( p1.get_y_() - p2.get_y_() );

    return std::max( abs( p1.get_x_() - p2.get_x_() ), abs( p1.get_y_() - p2.get_y_() ) );
}


void NcData::set_progress_bar_value_(int value)
{
    if (progress_bar_value_ == value) return;
    progress_bar_value_ = value;
    emit ProgressBarValueChanged(progress_bar_value_);
}
