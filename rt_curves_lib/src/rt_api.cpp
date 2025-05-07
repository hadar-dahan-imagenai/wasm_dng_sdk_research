#include "../include/rt_api.h"


void mini_rt::firstAnalysis(dng_pixel_buffer& pixel_buf, int width, int hight, LUTu & histogram)
{
    double lumimul[3];
    const double wprof[3][3] = {{0.79767489999999996, 0.1351917, 0.031353399999999997},
        {0.28804020000000002,0.71187409999999995,8.5699999999999996e-05 },
        {0,0,0.82521}}; //i took it from "ProPhoto"
    lumimul[0] = wprof[1][0];
    lumimul[1] = wprof[1][1];
    lumimul[2] = wprof[1][2];

    int W = width;
    int H = hight;
    int TS =  W;
    float lumimulf[3] = {static_cast<float>(lumimul[0]), static_cast<float>(lumimul[1]), static_cast<float>(lumimul[2])};

    // calculate histogram of the y channel needed for contrast curve calculation in exposure adjustments
    histogram.clear();

        for (int i = 0; i < H; i++) {
            for (int j = 0; j < W; j++) {

                float r = (*pixel_buf.ConstPixel_uint16(i, j, 0));
                float g =(*pixel_buf.ConstPixel_uint16(i, j, 1));
                float b = (*pixel_buf.ConstPixel_uint16(i, j, 2));

                int y = (lumimulf[0] * r + lumimulf[1] * g + lumimulf[2] * b);
                histogram[y]++;
            }
        }
}

int mini_rt::do_contrast(dng_pixel_buffer& pixel_buf, size_t hight, size_t width, double contrast, LUTf& contrast_curve) {



    LUTf curve1;

    LUTf curve2;
    // LUTf contrast_curve;
    LUTf satcurve;
    LUTf lhskcurve;
    LUTf lumacurve;
    LUTf clcurve;
    LUTf clToningcurve;
    LUTf cl2Toningcurve;
    LUTf wavclCurve;

    LUTf rCurve;
    LUTf gCurve;
    LUTf bCurve;
    LUTu dummy;
    LUTu hist16;
    hist16(65536);

    curve1(65536);
    curve2(65536);
    contrast_curve(65536, 0);
    satcurve(65536, 0);
    lhskcurve(65536, 0);
    lumacurve(32770, 0);  // lumacurve[32768] and lumacurve[32769] will be set to 32768 and 32769 later to allow linear interpolation
    clcurve(65536, 0);
    wavclCurve(65536, 0);
    rtengine::ToneCurve customToneCurve1, customToneCurve2;


    double expcomp = 0;
    double black = 0;
    double hlcompr = 0;
    double hlcomprthresh = 0;
    double shcompr = 50; //by default in pp3 file
    double br = 0;
    double contr = contrast;
    std::vector<double> curvePoints;
    curvePoints.push_back(0);
    std::vector<double> curvePoints2;
    curvePoints2.push_back(0);

    rtengine::Color::init ();
    firstAnalysis(pixel_buf,width,hight,hist16);
    rtengine::CurveFactory::complexCurve(expcomp, black / 65535.0, hlcompr, hlcomprthresh, shcompr, br, contr,
                                         curvePoints, curvePoints2,
                                         hist16, curve1, curve2, contrast_curve,
                                         dummy, customToneCurve1, customToneCurve2);

    return 0;
}