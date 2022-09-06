#include "cuttingpoint.h"

CuttingPoint::CuttingPoint()
{

}

CuttingPoint::~CuttingPoint()
{

}

void CuttingPoint::set_x_(int value)
{
    x_ = value;
}

void CuttingPoint::set_y_(int value)
{
    y_ = value;
}

int CuttingPoint::get_x_()
{
    return x_;
}

int CuttingPoint::get_y_()
{
    return y_;
}

void CuttingPoint::set_cutting_code_(int value)
{
    cutting_code_ = value;
}

int CuttingPoint::get_cutting_code_()
{
    return cutting_code_;
}
