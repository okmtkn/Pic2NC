#ifndef CUTTINGPOINT_H
#define CUTTINGPOINT_H


class CuttingPoint
{
public:
    CuttingPoint();
    CuttingPoint(int x, int y);
    ~CuttingPoint();

public:
    void set_x_(int value);
    void set_y_(int value);
    int  get_x_();
    int  get_y_();
    void set_cutting_code_(int value);
    int  get_cutting_code_();

private:
    int  x_;
    int  y_;
    int  cutting_code_;  //G0 or G1
};

#endif // CUTTINGPOINT_H
