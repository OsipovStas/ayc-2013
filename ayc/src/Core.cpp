#include "Core.hpp"
#include <cmath>

#include <iostream>
#include <boost/range/numeric.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/bind.hpp>

#include <mkl_cblas.h>

bool Point::operator<(const Point& o) const {
    if(y >= 0) {
        if(o.y < 0) {
            return true;
        }
        return o.x < x;
    } else {
        if(o.y == 0 && o.x > 0) {
            return false;
        }
        if(o.y > 0) {
            return false;
        }
        return x < o.x;
    }
}



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


float hammingDistance(const Descriptor& d1, const Descriptor& d2) {
    int size = std::min(d1.size(), d2.size()) / 2;
    int hamming = 0;
    boost::for_each(boost::irange(0, size), [&](int i) {
       int pair = i + size;
       if((d1[i] < d1[pair]) ^ (d2[i] < d2[pair])) {
           ++hamming;
       }
    });
    return hamming / (float)(size);
}


float evalCorrelation(const Descriptor& x, const Descriptor& y) {
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
        if (error > 0) {
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
    boost::sort(circle);
}


float maxIntensityReducer(const Descriptor& sample) {
    Descriptor tmp(sample);
    float length = sample.size();
    int kSize = tmp.size() / KERNEL_SIZE;
    std::copy_n(tmp.begin(), kSize, std::back_inserter(tmp));
    float init = std::accumulate(std::begin(tmp), std::begin(tmp) + kSize, 0.0f);
    Descriptor kernelSum;
    std::transform(std::begin(tmp) + kSize, std::end(tmp), std::begin(tmp), std::back_inserter(kernelSum), boost::bind<float>([&](float f2, float f1) {
        init += (f2 - f1);
        return init;
    }, _1, _2));
    return (boost::max_element(kernelSum) - std::begin(kernelSum)) / length;
}

int chooseProbableRotation(const Descriptors& analyzed, const Descriptor& query, size_t directionNumber) {
    Descriptor directions;
    boost::transform(analyzed
            | boost::adaptors::transformed(maxIntensityReducer),
            query,
            std::back_inserter(directions),
            std::minus<float>());
    int rotation = static_cast<int> (std::floor(directionNumber * boost::accumulate(directions, 0.0f) / directions.size()));
    rotation = rotation < 0 ? (directionNumber + rotation) : rotation;
    return rotation;
}


float sumDescriptorReducer(const Descriptor& d) {
    return boost::accumulate(d, 0.0f);
}
