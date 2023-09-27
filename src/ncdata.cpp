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
               )
{
    image_ = image.copy();  //コピーして保持しておくことで，NcDataをdeleteしたときにimage本体が開放されないようにする
    image_ = image_.convertToFormat(QImage::Format_Indexed8);
    //image = image.convertToFormat(QImage::Format_Mono);

    output_file_name_ = output_file_name;
    file_type_ = file_type;
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
    stl_height_ = stl_height;

    //NCデータのスケール,原点位置を設定
    double scale_x = work_width_ / image_.width();
    double scale_y = work_height_ / image_.height();
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
    if(enable_offset_pocket_milling_ && file_type_ != ".stl"){
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
    if(file_type_ == ".nc" || file_type_ == ".ncd"){
        GenerateNcString();
    }
    if(file_type_ == ".dxf"){
        GenerateDxfString();
    }
    if(file_type_ ==".stl"){
        GenerateStlString();
    }
    set_progress_bar_value_(100);

    OutputFile();
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
    if(tolerance_ == 0){
        cutting_point_output_ = cutting_point_all_;
        return;
    }

    double a, b, c; //直線の方程式 ax + by + c = 0
    double denominator;

    double tmp_distance;
    double max_distance = 0;
    int   output_point_num;

    cutting_point_output_.append(cutting_point_all_[0]);

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


void NcData::GenerateNcString()
{
    double x, y;
    output_ascii_data_.append("%%\n(GENERATED BY PIC2NC);\n"
                             "G17G90G40G80;\n"
                             "N100(CONTOUR CUT);\n"
                             //"T10M6;\n"
                             );
    output_ascii_data_.append(QString::asprintf("G90G54S%dF%d;\n", spindle_speed_, feed_rate_));
    //output_text_data_.append("G43H10Z100.M1;\n");
    output_ascii_data_.append(QString::asprintf("G0Z%.3lf;\n", retract_height_ + offset_z_));

    output_ascii_data_.append("M3;\n");
    //output_text_data_.append("/M8;\n");

    x =   cutting_point_output_[0].get_x_()*scale_ + offset_x_;
    y = - cutting_point_output_[0].get_y_()*scale_ + offset_y_;
    output_ascii_data_.append(QString::asprintf("X%.3lfY%.3lf;\n", x, y));
    output_ascii_data_.append(QString::asprintf("G1Z%.3lf;\n", cutting_depth_ + offset_z_));

    for(int i=1; i<cutting_point_output_.size(); i++){
        x =   cutting_point_output_[i].get_x_()*scale_ + offset_x_;
        y = - cutting_point_output_[i].get_y_()*scale_ + offset_y_;

        if(cutting_point_output_[i].get_cutting_code_() == 1){ //G1する候補ならば
            output_ascii_data_.append(QString::asprintf("X%.3lfY%.3lf;\n", x, y));
        } else {
            output_ascii_data_.append(QString::asprintf("Z%.3lf;\n", retract_height_ + offset_z_));
            output_ascii_data_.append(QString::asprintf("G0X%.3lfY%.3lf;\n", x, y));
            output_ascii_data_.append(QString::asprintf("G1Z%.3lf;\n", cutting_depth_ + offset_z_));   //これが，one tool pathの1点目となる．
        }
    }

    //最後のリトラクト
    output_ascii_data_.append(QString::asprintf("G1Z%.3lf;\n", retract_height_ + offset_z_));
    output_ascii_data_.append("M5;\n");
    output_ascii_data_.append("G0X0.Y0.;\n");
    //output_text_data_.append("M9;\n");
    output_ascii_data_.append("M30;\n");
    output_ascii_data_.append("%%\n");
}

void NcData::GenerateDxfString()
{
    double x1, y1, x2, y2;
    output_ascii_data_.append("0\n"
                              "SECTION\n"
                              "2\n"
                              "HEADER\n"
                              "9\n"
                              "$INSUNITS\n"
                              "70\n"
                              "4\n"
                              "0\n"
                              "ENDSEC\n"
                              "0\n"
                              "SECTION\n"
                              "2\n"
                              "ENTITIES\n");
    for(int i=1; i<cutting_point_output_.size(); i++){
        if(cutting_point_output_[i].get_cutting_code_() == 1){
            x1 =   cutting_point_output_[i-1].get_x_()*scale_ + offset_x_;
            y1 = - cutting_point_output_[i-1].get_y_()*scale_ + offset_y_ + work_height_;
            x2 =   cutting_point_output_[i  ].get_x_()*scale_ + offset_x_;
            y2 = - cutting_point_output_[i  ].get_y_()*scale_ + offset_y_ + work_height_;
            output_ascii_data_.append("0\nLINE\n8\n0\n");
            output_ascii_data_.append(QString::asprintf("10\n%.3lf\n20\n%.3lf\n", x1, y1));
            output_ascii_data_.append(QString::asprintf("11\n%.3lf\n21\n%.3lf\n", x2, y2));
        }
    }

    output_ascii_data_.append("0\nENDSEC\n0\nEOF\n");
}


void NcData::OutputFile()
{
    QFileInfo file_info(output_file_name_);

    //ファイルがすでに存在する場合は，上書きするかどうかを確認する
    if (file_info.exists()) {
        //ビープ音を鳴らす
        QApplication::beep();
        QMessageBox msgBox;
        msgBox.setText("File already exist.");
        msgBox.setInformativeText("Do you want to overwite?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        if(msgBox.exec() == QMessageBox::Cancel){
            return;
        }
    }

    QFile file(output_file_name_);
    if(!file.open(QIODevice::WriteOnly))    //ファイルオープン
    {
        QMessageBox msgBox;
        msgBox.setText("Could not open the file.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        return;
    }
    QTextStream out_stream(&file);
    out_stream << output_ascii_data_;
    file.close();
}

int NcData::GetTriangleRotationDirection(cv::Point2d vertex0, cv::Point2d vertex1, cv::Point2d vertex2)
{
    double cross_product = (vertex1.x - vertex0.x)*(vertex2.y - vertex0.y) - (vertex1.y - vertex0.y)*(vertex2.x - vertex0.x);
    if(cross_product > 0){
        return 1;
    } else if(cross_product < 0){
        return -1;
    } else {
        return 0;
    }
}

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
cv::Point2d NcData::GetIntersection(cv::Point2d A, cv::Point2d B, cv::Point2d C, cv::Point2d D)
{
    //線分ABと線分CDの交点を求める
    //まずは，線分ABの方程式を求める
    //ax + by = c
    //a = y2 - y1
    //b = x1 - x2
    //c = ax1 + by1
    double a1 = B.y - A.y;
    double b1 = A.x - B.x;
    double c1 = a1*A.x + b1*A.y;

    //次に，線分CDの方程式を求める
    double a2 = D.y - C.y;
    double b2 = C.x - D.x;
    double c2 = a2*C.x + b2*C.y;

    //線分ABと線分CDの交点を求める
    double det = a1*b2 - a2*b1;
    if(det<0.000001 && det>-0.000001){
        //交点がない場合
        return cv::Point2d(-1,-1);
    } else {
        double x = (b2*c1 - b1*c2) / det;
        double y = (a1*c2 - a2*c1) / det;
        return cv::Point2d(x, y);
    }
}


void NcData::DrawNcView(QGraphicsScene *scene, QRect view_size)
{
    double scale_h = double(view_size.height()) / double(image_.height());
    double scale_w = double(view_size.width()) / double(image_.width());
    double scale = scale_h < scale_w ? scale_h : scale_w;

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

double NcData::GetG0Length()
{
    double g0_length = 0;
    //i=0は最初の点なので計算から除外する
    for (int i=1; i<cutting_point_output_.size(); i++){
        if(cutting_point_output_[i].get_cutting_code_() == 0){
            g0_length += CalcDistance(cutting_point_output_[i-1], cutting_point_output_[i]) + retract_height_;
        }
    }
    g0_length *= scale_;
    return g0_length;
}


double NcData::GetG1Length()
{
    double g1_length = 0;
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

QString NcData::GetOutputString()
{
    return output_ascii_data_;
}

int NcData::get_cutting_point_length_()
{
    return cutting_point_output_.size();
}



//厳密な距離
double NcData::CalcDistance(CuttingPoint p1, CuttingPoint p2)
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


void NcData::GenerateStlString()
{
    // Phase.1: STLファイルのヘッダを出力する
    output_ascii_data_.append("solid\n");


    // Phase.2: OpenCVによるドロネー分割を行う．
    cv::Rect rect(0, 0, image_.width(), image_.height());
    cv::Subdiv2D *subdiv;
    subdiv = new cv::Subdiv2D(rect);

    for(int i=0; i<cutting_point_output_.size(); i++){
        subdiv->insert(cv::Point2f(cutting_point_output_[i].get_x_(), cutting_point_output_[i].get_y_()));
    }
    std::vector<cv::Vec6f> triangle_list;    //OpenCVのドロネー分割によって生成された三角形のリスト
    //ドロネー分割を行う
    subdiv->getTriangleList(triangle_list);

    //triangle_listをQListに変換する
    QList<cv::Vec6d> triangle_list_q;
    for(auto& triangle : triangle_list){
        triangle_list_q.append(triangle);
    }
    //triangle_list_qは，とても大きな領域を必要とするため，十分な量のメモリを確保する
    //三角形の数は，cutting_point_output_.size()と比較して，十分な量を確保しておく
    //triangle_list_q.reserve(cutting_point_output_.size()*100);

    //triangle_listはもう使わないので，メモリを開放する
    subdiv->initDelaunay(rect);
    delete subdiv;
    triangle_list.clear();


    // Phase.3: ドロネー分割の三角形の修正
    //         OpenCVによるドロネー分割では，画像のエッジに相当する部分に交差する三角形が生成されることがある．
    //         交差する三角形を，総当たりで交差しているかどうかを確認し，交差している場合は交点を追加する．
    //         交点を頂点とする新しい三角形を生成する．
    //         交差する三角形は，頂点の位置を変更することで，交差しないようにする．

    cv::Point2d p0, p1;
    cv::Point2d vertex0, vertex1, vertex2;
    cv::Point2d intersection;

    for(int i=0; i<cutting_point_output_.size()-1; i++){
        //UIが固まらないようにする
        QCoreApplication::processEvents();

        //早送り経路の場合は，交差判定を行わない
        if(cutting_point_output_[i+1].get_cutting_code_() == 0) continue;

        //多角形の辺を，線分の始点と終点の2点で表す
        p0 = cv::Point2d(cutting_point_output_[i].get_x_(), cutting_point_output_[i].get_y_());
        p1 = cv::Point2d(cutting_point_output_[i+1].get_x_(), cutting_point_output_[i+1].get_y_());

        //交差判定を行う
        //FIXME:三角形の総当たりが効率が悪い．さらに高速な方法を考える．
        bool intersection_was_found = true;
        //while(intersection_was_found){    //while文で囲むと，計算がループから抜け出せないことがある
        for(int j=0; j<50 && intersection_was_found; j++){ //何度か繰り返さないと，三角形の交差を全て検出できないことがある
            intersection_was_found = false;
            for(auto& triangle : triangle_list_q){
                //三角形の各辺について，多角形の辺と交差しているかを確認
                vertex0 = cv::Point2d(triangle[0], triangle[1]);
                vertex1 = cv::Point2d(triangle[2], triangle[3]);
                vertex2 = cv::Point2d(triangle[4], triangle[5]);

                //三角形の辺①について，もし交差していたら，交点を追加して三角形を分割する
                if(intersect(p0, p1, vertex0, vertex1)){
                    intersection = GetIntersection(p0, p1, vertex0, vertex1);
                    if(intersection.x != -1 && intersection.y != -1){   //無効な値であった場合，交点を追加しない
                        intersection_was_found = true;
                        //交点を追加して三角形を分割する
                        triangle_list_q.append(cv::Vec6d(intersection.x, intersection.y, triangle[0], triangle[1], triangle[4], triangle[5]));
                        //三角形の頂点を変更する
                        triangle[0] = intersection.x;
                        triangle[1] = intersection.y;
                        continue;
                    }
                }
                //三角形の辺②について，もし交差していたら，交点を追加して三角形を分割する
                if(intersect(p0, p1, vertex1, vertex2)){
                    intersection = GetIntersection(p0, p1, vertex1, vertex2);
                    if(intersection.x != -1 && intersection.y != -1){   //無効な値であった場合，交点を追加しない
                        intersection_was_found = true;
                        //交点を追加して三角形を分割する
                        triangle_list_q.append(cv::Vec6d(intersection.x, intersection.y, triangle[0], triangle[1], triangle[2], triangle[3]));
                        //三角形の頂点を変更する
                        triangle[2] = intersection.x;
                        triangle[3] = intersection.y;
                        continue;
                    }
                }
                //三角形の辺③について，もし交差していたら，交点を追加して三角形を分割する
                if(intersect(p0, p1, vertex2, vertex0)){
                    intersection = GetIntersection(p0, p1, vertex2, vertex0);
                    if(intersection.x != -1 && intersection.y != -1){   //無効な値であった場合，交点を追加しない
                        intersection_was_found = true;
                        //交点を追加して三角形を分割する
                        triangle_list_q.append(cv::Vec6d(intersection.x, intersection.y, triangle[2], triangle[3], triangle[4], triangle[5]));
                        //三角形の頂点を変更する
                        triangle[4] = intersection.x;
                        triangle[5] = intersection.y;
                    }
                }
            }
        }
    }

    QList<cv::Vec6d> triangle_list_output;  //最終的に出力する三角形のリスト

    //triangle_list_qのうち，重心位置の画素が黒色の三角形のみをtriangle_list_outputに追加する
    for(auto& triangle : triangle_list_q){
        //三角形の重心を算出
        int mass_x = round((triangle[0] + triangle[2] + triangle[4]) / 3.);
        int mass_y = round((triangle[1] + triangle[3] + triangle[5]) / 3.);
        //重心位置の画素が黒色の場合，三角形をtriangle_list_outputに追加する
        if( ((uchar *)image_.bits())[mass_x*image_.depth()/8 + mass_y*image_.bytesPerLine()] != 255 ){    //重心の画素が白色の場合
            triangle_list_output.append(triangle);
        }
    }

    //三角形の頂点の回転方向を確認し，反時計回りの場合は頂点の位置を入れ替える
    for(auto& triangle : triangle_list_output){
        cv::Point2d vertex0 = cv::Point2d(triangle[0], triangle[1]);
        cv::Point2d vertex1 = cv::Point2d(triangle[2], triangle[3]);
        cv::Point2d vertex2 = cv::Point2d(triangle[4], triangle[5]);
        if(GetTriangleRotationDirection(vertex0, vertex1, vertex2) == -1){
            triangle[2] = vertex2.x;
            triangle[3] = vertex2.y;
            triangle[4] = vertex1.x;
            triangle[5] = vertex1.y;
        }
    }

    // Phase.4: 三角形の出力

    double x1, y1, x2, y2, x3, y3;
    for (auto &triangle : triangle_list_output) {
        x1 = triangle[0];
        y1 = triangle[1];
        x2 = triangle[2];
        y2 = triangle[3];
        x3 = triangle[4];
        y3 = triangle[5];

        x1 =  x1 * scale_ + offset_x_;
        y1 = -y1 * scale_ + offset_y_ + work_height_;
        x2 =  x2 * scale_ + offset_x_;
        y2 = -y2 * scale_ + offset_y_ + work_height_;
        x3 =  x3 * scale_ + offset_x_;
        y3 = -y3 * scale_ + offset_y_ + work_height_;

        //底面
        output_ascii_data_.append("facet normal 0.0 0.0 -1.0\nouter loop\n");
        output_ascii_data_.append(QString::asprintf("vertex %.6lf %.6lf 0.0\n", x1, y1));
        output_ascii_data_.append(QString::asprintf("vertex %.6lf %.6lf 0.0\n", x2, y2));
        output_ascii_data_.append(QString::asprintf("vertex %.6lf %.6lf 0.0\n", x3, y3));
        output_ascii_data_.append("endloop\nendfacet\n");

        //天面
        output_ascii_data_.append("facet normal 0.0 0.0 1.0\nouter loop\n");
        output_ascii_data_.append(QString::asprintf("vertex %.6lf %.6lf %.6lf\n", x2, y2, stl_height_));
        output_ascii_data_.append(QString::asprintf("vertex %.6lf %.6lf %.6lf\n", x1, y1, stl_height_));
        output_ascii_data_.append(QString::asprintf("vertex %.6lf %.6lf %.6lf\n", x3, y3, stl_height_));
        output_ascii_data_.append("endloop\nendfacet\n");
    }


    //側面
    int g0_itr = 0; //ループの先頭のインデックス
    int direction = 0;  //正の値：右回り，負の値：左回り
    for(int i=0; i<cutting_point_output_.size()-1; i++){
        if(cutting_point_output_[i+1].get_cutting_code_() == 1 && i!=cutting_point_output_.size()-2){
            //TODO:画像の端部の場合の処理を別の場所に記述すること

            //TODO:多角形の外向き法線ベクトルを求める
            //現在の切削点と，次の切削点の平均座標を求める

            double mean_x = (cutting_point_output_[i].get_x_() + cutting_point_output_[i+1].get_x_()) / 2;
            double mean_y = (cutting_point_output_[i].get_y_() + cutting_point_output_[i+1].get_y_()) / 2;
            //現在の切削点と，次の切削点の差分ベクトルに対して直交する方向に1ピクセル進めた座標値を求める．
            //この座標値は，多角形の外向き法線ベクトルとなる
            double diff_x = cutting_point_output_[i+1].get_x_() - cutting_point_output_[i].get_x_();
            double diff_y = cutting_point_output_[i+1].get_y_() - cutting_point_output_[i].get_y_();
            double diff_length = sqrt(diff_x*diff_x + diff_y*diff_y);
            int normal_x = round(mean_x + diff_y / diff_length * 2);    //2ピクセル先の画素を見に行っている．
            int normal_y = round(mean_y - diff_x / diff_length * 2);

            //normal_xとnormal_yが，画像の範囲内かどうかを確認する
            if(normal_x < 0 || normal_x >= image_.width() || normal_y < 0 || normal_y >= image_.height()){
                //何もしない
            } else if( ((uchar *)image_.bits())[normal_x*image_.depth()/8 + normal_y*image_.bytesPerLine()] != 255 ){
                direction--;
            } else {
                direction++;
            }
        } else {    //１経路分を一気に出力する
            for(int j=g0_itr; j<i || j==cutting_point_output_.size()-2; j++){
                x1 = cutting_point_output_[j].get_x_() * scale_ + offset_x_;
                y1 = - cutting_point_output_[j].get_y_() * scale_ + offset_y_ + work_height_;
                x2 = cutting_point_output_[j+1].get_x_() * scale_ + offset_x_;
                y2 = - cutting_point_output_[j+1].get_y_() * scale_ + offset_y_ + work_height_;
                x3 = x1;
                y3 = y1;

                //三角形の法線ベクトルを三角形の辺ベクトルの外積として求める
                double vector1_x = x2 - x1;
                double vector1_y = y2 - y1;
                double vector1_z = 0.0;
                double vector2_x = x3 - x1;
                double vector2_y = y3 - y1;
                double vector2_z = stl_height_;
                double normal_vector_x = vector1_y * vector2_z - vector1_z * vector2_y;
                double normal_vector_y = vector1_z * vector2_x - vector1_x * vector2_z;
                double normal_vector_length = sqrt(normal_vector_x*normal_vector_x + normal_vector_y*normal_vector_y);
                normal_vector_x /= normal_vector_length;
                normal_vector_y /= normal_vector_length;


                if(direction < 0){ //三角形の向きを正回転に記述する

                    //側面三角形の下半分
                    output_ascii_data_.append(QString::asprintf("facet normal %.6lf %.6lf 0.0\nouter loop\n", normal_vector_x, normal_vector_y));
                    output_ascii_data_.append(QString::asprintf("vertex %.6lf %.6lf 0.0\n", x1, y1));
                    output_ascii_data_.append(QString::asprintf("vertex %.6lf %.6lf 0.0\n", x2, y2));
                    output_ascii_data_.append(QString::asprintf("vertex %.6lf %.6lf %.6lf\n", x3, y3, stl_height_));
                    output_ascii_data_.append("endloop\nendfacet\n");

                    //側面三角形の上半分
                    x3 = x2;
                    y3 = y2;
                    output_ascii_data_.append(QString::asprintf("facet normal %.6lf %.6lf 0.0\nouter loop\n", normal_vector_x, normal_vector_y));
                    output_ascii_data_.append(QString::asprintf("vertex %.6lf %.6lf %.6lf\n", x2, y2, stl_height_));
                    output_ascii_data_.append(QString::asprintf("vertex %.6lf %.6lf %.6lf\n", x1, y1, stl_height_));
                    output_ascii_data_.append(QString::asprintf("vertex %.6lf %.6lf 0.0\n", x3, y3));
                    output_ascii_data_.append("endloop\nendfacet\n");
                } else { //三角形の向きを逆回転に記述する
                    //法線ベクトルの向きを反転させる
                    normal_vector_x *= -1;
                    normal_vector_y *= -1;

                    //側面三角形の下半分
                    output_ascii_data_.append(QString::asprintf("facet normal %.6lf %.6lf 0.0\nouter loop\n", normal_vector_x, normal_vector_y));
                    output_ascii_data_.append(QString::asprintf("vertex %.6lf %.6lf 0.0\n", x2, y2));
                    output_ascii_data_.append(QString::asprintf("vertex %.6lf %.6lf 0.0\n", x1, y1));
                    output_ascii_data_.append(QString::asprintf("vertex %.6lf %.6lf %.6lf\n", x3, y3, stl_height_));
                    output_ascii_data_.append("endloop\nendfacet\n");

                    //側面三角形の上半分
                    x3 = x2;
                    y3 = y2;
                    output_ascii_data_.append(QString::asprintf("facet normal %.6lf %.6lf 0.0\nouter loop\n", normal_vector_x, normal_vector_y));
                    output_ascii_data_.append(QString::asprintf("vertex %.6lf %.6lf %.6lf\n", x1, y1, stl_height_));
                    output_ascii_data_.append(QString::asprintf("vertex %.6lf %.6lf %.6lf\n", x2, y2, stl_height_));
                    output_ascii_data_.append(QString::asprintf("vertex %.6lf %.6lf 0.0\n", x3, y3));
                    output_ascii_data_.append("endloop\nendfacet\n");
                }
            }
            g0_itr = i+1;
            direction = 0;
        }
    }

    output_ascii_data_.append("endsolid\n");
}

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
bool NcData::intersect(cv::Point2d A, cv::Point2d B, cv::Point2d C, cv::Point2d D)
{
    //線分の端の点が一致している場合は，交差していないと判定する
    if(A==C || A==D || B==C || B==D) return false;

    //線分の端の点が十分近い場合は，交差していないと判定する
    //if(cv::norm(A-C) < 1e-10 || cv::norm(A-D) < 1e-10 || cv::norm(B-C) < 1e-10 || cv::norm(B-D) < 1e-10) return false;

    //(Cが線分ABの上または下にある) != (Dが線分ABの上または下にある)
    //数学的にはゼロ以下であれば良いのだが，誤差を考慮して-0.5とする
    if(((A.y-C.y)*(B.x-A.x) - (A.x-C.x)*(B.y-A.y)) * ((A.y-D.y)*(B.x-A.x) - (A.x-D.x)*(B.y-A.y)) < -0.5){
        //(Aが線分CDの上または下にある) != (Bが線分CDの上または下にある)
        if(((C.y-A.y)*(D.x-C.x) - (C.x-A.x)*(D.y-C.y)) * ((C.y-B.y)*(D.x-C.x) - (C.x-B.x)*(D.y-C.y)) < -0.5){
            return true;
        }
    }

    return false;
}
