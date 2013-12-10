#include "Core.hpp"
#include <cmath>

#include <iostream>
#include <boost/range/numeric.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/bind.hpp>

#include <mkl_cblas.h>


void convert2Gray(const CImg<u_char>& colorImage, Image& grayImage) {
    static const float R = 0.299f;
    static const float G = 0.587f;
    static const float B = 0.114f;

    cimg_forXY(colorImage, x, y) {
        grayImage(x, y) = ((colorImage(x, y, 0, 1) * R) + (colorImage(x, y, 0, 1) * G) + (colorImage(x, y, 0, 2) * B)) / 255;
    }
}

float getMaxRadius(const Features& f) {
    float r = f.back().size() / (2 * PI);
    r += (2 * r / f.size());
    return r;
}

bool isFitImage(float rad, const Image& i, const Point& c) {
    return (c.x > rad) && (c.x + rad < i.width()) && (c.y > rad) && (c.y + rad < i.height());
}

float point2float(const Point& p, const Point& c, const Image& i) {
    return i(p.x + c.x, p.y + c.y);
}

float evalSample(const Points& points, const Point& center, const Image& image) {
    return boost::accumulate(points | boost::adaptors::transformed(boost::bind<float>(point2float, _1, center, boost::cref(image))), 0.0f);
}

float evalCorrelation(const Descriptor& x, const Descriptor& y) {
    assert(x.size() == y.size());

    const int size = x.size();
    Descriptor xMeanCorrected(size, 1.0f);
    Descriptor yMeanCorrected(size, 1.0f);

    float xMean = cblas_sasum(size, x.data(), 1) / size;
    float yMean = cblas_sasum(size, y.data(), 1) / size;

    cblas_saxpby(size, 1.0f, x.data(), 1, -xMean, xMeanCorrected.data(), 1);
    cblas_saxpby(size, 1.0f, y.data(), 1, -yMean, yMeanCorrected.data(), 1);

    float xMeanCorrectedSquare = cblas_sdot(size, xMeanCorrected.data(), 1, xMeanCorrected.data(), 1);

    float beta = cblas_sdot(size, xMeanCorrected.data(), 1, yMeanCorrected.data(), 1) / xMeanCorrectedSquare;
    float gamma = yMean - beta * xMean;

    float betaAbs = std::abs(beta);
    float gammaAbs = std::abs(gamma);

    if (betaAbs < BETA_THRESHOLD || betaAbs > BETA_THRESHOLD_INV || gammaAbs > GAMMA_THRESHOLD) {
        return 0;
    }

    float xMeanCorrectedNorm = cblas_snrm2(size, xMeanCorrected.data(), 1);
    float yMeanCorrectedNorm = cblas_snrm2(size, yMeanCorrected.data(), 1);

    return (beta * xMeanCorrectedSquare) / (xMeanCorrectedNorm * yMeanCorrectedNorm);
}

void generateCircle(int radius, const Point& center, Points& circle) {
    int x0 = center.x;
    int y0 = center.y;
    int error = 1 - radius;
    int errorY = 1;
    int errorX = -2 * radius;
    int x = radius, y = 0;

    circle.push_back(Point(x0, y0 + radius));
    circle.push_back(Point(x0, y0 - radius));
    circle.push_back(Point(x0 + radius, y0));
    circle.push_back(Point(x0 - radius, y0));

    while (y < x) {
        if (error > 0) 
        {
            x--;
            errorX += 2;
            error += errorX;
        }
        y++;
        errorY += 2;
        error += errorY;
        circle.push_back(Point(x0 + x, y0 + y));
        circle.push_back(Point(x0 - x, y0 + y));
        circle.push_back(Point(x0 + x, y0 - y));
        circle.push_back(Point(x0 - x, y0 - y));
        circle.push_back(Point(x0 + y, y0 + x));
        circle.push_back(Point(x0 - y, y0 + x));
        circle.push_back(Point(x0 + y, y0 - x));
        circle.push_back(Point(x0 - y, y0 - x));
    }
}

void generateRadianLine(int angle, int radius, const Point& center, Points& radianLine) {
    int x0 = center.x;
    int y0 = center.y;

    int x1 = std::cos(-angle * PI / 180.0) * radius + x0;
    int y1 = std::sin(-angle * PI / 180.0) * radius + y0;



    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2; 

    for (;;) { /* loop */
        radianLine.push_back(Point(x0, y0));
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        } /* e_xy+e_x > 0 */
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        } /* e_xy+e_y < 0 */
    }
}

