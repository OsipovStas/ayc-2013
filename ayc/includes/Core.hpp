/* 
 * File:   Core.hpp
 * Author: stasstels
 *
 * Created on November 8, 2013, 11:38 PM
 */

#ifndef CORE_HPP
#define	CORE_HPP

#include <cmath>

#include <vector>
#include <numeric>
#include <random>
#include <functional>

#include <boost/bind.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/irange.hpp>
#include <boost/ref.hpp>
#include "boost/range/numeric.hpp"


#include <mkl_cblas.h>


#define cimg_OS 0
#include "CImg.h"
using namespace cimg_library;

const float PI = 3.14159265f;
const float MAX_IMAGE_SIZE = 4000000.0f;
const float BLUR = 2.1f;

const float BETA_THRESHOLD = 0.1f;
const float BETA_THRESHOLD_INV = 1.0 / BETA_THRESHOLD;

const float GAMMA_THRESHOLD = 1.0f;

const int SCALES_NUMBER = 8;

const int FULL_DEGREES = 360;

const int CIRCLES_NUMBER = 15;
const int MIN_CIRCLE_RADIUS = 2;
const int MAX_CIRCLE_RADIUS = 150;

const float CIRCLE_FILTER_THRESHOLD = 0.95;
const float BRIEF_FILTER_THRESHOLD = 0.25;

const int KERNEL_SIZE = 8;
const int DIRECTION_NUMBER = 36;

const float COS = std::cos(2 * PI / DIRECTION_NUMBER);
const float SIN = std::sin(2 * PI / DIRECTION_NUMBER);

struct Point {
    int x;
    int y;

    Point(int x, int y) : x(x), y(y) {
    }

    bool operator<(const Point& o) const;

};

typedef CImg<float> Image;
typedef std::vector<Point> Points;
typedef std::vector<Points> Features;
typedef std::vector<Features> FeaturesVector;
typedef std::vector<float> Descriptor;
typedef std::vector<Descriptor> Descriptors;
typedef std::vector<Descriptors> DescriptorsVector;


float sumDescriptorReducer(const Descriptor& d);

bool isFitImage(float r, const Image& i, const Point& c);

float getMaxRadius(const Features& f);

float point2float(const Point& p, const Point& c, const Image& i);

void generateCircle(int radius, const Point& center, Points& circle);

void convert2Gray(const CImg<u_char>& colorImage, Image& grayImage);

float maxIntensityReducer(const Descriptor& circleSample);

int chooseProbableRotation(const Descriptors& analyzed, const Descriptor& query, size_t directionNumber);

float evalCorrelation(const Descriptor& x, const Descriptor& y);

float hammingDistance(const Descriptor& d1, const Descriptor& d2);

template<class OutputIterator>
void generateScales(const Image& query, float minScale, float maxScale, OutputIterator out) {
    float diff = (maxScale - minScale) / (SCALES_NUMBER - 1);
    boost::for_each(boost::irange(0, SCALES_NUMBER), [&](int i) {
        int factor = -std::floor((minScale + diff * i) * 100);
        *out++ = query.get_resize(factor, factor).blur(BLUR);
    });
}

template <class SinglePassRange, class OutputIterator>
void generateCirclesSet(const SinglePassRange& queries, OutputIterator out) {
    boost::for_each(queries, [&](const Image & image) {
        auto featuresOut = std::begin(*out++);
        auto maxRadius = std::min(std::min(image.height(), image.width()) / 2, MAX_CIRCLE_RADIUS);
                auto diff = std::max(maxRadius / CIRCLES_NUMBER, 1);
                boost::for_each(boost::irange(1, CIRCLES_NUMBER + 1), [&](int i) {
                    int radius = std::min(i * diff, maxRadius);
                    generateCircle(radius, Point(0, 0), *featuresOut++);
                });
    });
}

template<class OutputIterator>
void evalPointDescriptor(const Points& points, const Point& c, const Image& image, OutputIterator out) {
    boost::transform(points, out, boost::bind<float>(point2float, _1, boost::cref(c), boost::cref(image)));
}

template <class ImageInputIterator, class OutputIterator>
void evalQueryFeaturesPointDescriptor(const Features& features, ImageInputIterator images, OutputIterator out) {
    boost::for_each(features, [&](const Points & points) {
        auto image = *images++;
        evalPointDescriptor(points, Point(image.width() / 2, image.height() / 2), image, std::back_inserter(*out++));
    });
}

template<class InputImageIterator, class OutputIterator>
void evalQueryDescriptors(const FeaturesVector& featureSet, InputImageIterator images, OutputIterator out) {
    boost::for_each(featureSet, [&](const Features & features) {
        evalQueryFeaturesSampleDescriptor(features, *images++, std::begin(*out++));
    });
}

template<class OutputIterator>
void evalPointDescriptors(const Features& features, const Image& i, const Point& c, OutputIterator out) {
    if (isFitImage(getMaxRadius(features), i, c)) {
        boost::for_each(features, [&](const Points & points) {
            evalPointDescriptor(points, c, i, std::back_inserter(*out++));
        });
    }
}

template<class OutputIterator>
void evalPointDescriptorsVector(const FeaturesVector& featuresVector, const Image& i, const Point& c, OutputIterator out) {
    boost::for_each(featuresVector, [&](const Features & features) {
        evalPointDescriptors(features, i, c, std::begin(*out++));
    });
}

template<class InputImageIterator, class OutputIterator>
void evalQueryDescriptorsVector(const FeaturesVector& featureSet, InputImageIterator images, OutputIterator out) {
    boost::for_each(featureSet, [&](const Features & features) {
        auto image = *images++;
        evalPointDescriptors(features, image, Point(image.width() / 2, image.height() / 2), std::begin(*out++));
    });
}

template<class OutputIterator, class DescriptorReducer>
void reduceDescriptorsVector(const DescriptorsVector& descriptorsVector, OutputIterator out, DescriptorReducer reducer) {
    boost::for_each(descriptorsVector, [&](const Descriptors & descriptors) {
        boost::transform(descriptors, std::back_inserter(*out++), reducer);
    });
}

template<class Out>
void generateRandomPoints(size_t number, Out out) {
    std::default_random_engine generator;
    std::normal_distribution<float> distribution(0, number * number / 25.0f);
    for (size_t i = 0; i < number * 32; ++i) {
        *out++ = distribution(generator);
    }
}

template<class Out>
void generateRotations(const std::vector<float>& pts, Out out) {
    std::vector<float> tmp(pts);
    for (size_t i = 0; i < DIRECTION_NUMBER; ++i) {
        boost::copy(tmp, std::begin(*out++));
        cblas_srot(tmp.size() / 2, tmp.data(), 1, tmp.data() + tmp.size() / 2, 1, COS, SIN);
    }
}

template<class OutPointsIterator>
void generateBriefPoints(const std::vector<std::vector<float> >& rPts, OutPointsIterator out) {
    boost::for_each(rPts, [&](const std::vector<float>& pts) {
        std::transform(std::begin(pts), std::begin(pts) + pts.size() / 2, std::begin(pts) + pts.size() / 2, std::back_inserter(*out++), boost::bind<Point>([](float xf, float yf) {
            int x = static_cast<int> (std::floor(xf));
            int y = static_cast<int> (std::floor(yf));
            return Point(x, y);
        }, _1, _2));
    });
}

template<class OutPointsIterator>
void generateRotatedPoints(const std::vector<float>& pts, OutPointsIterator out) {
    std::vector<std::vector<float> > rPts(DIRECTION_NUMBER, std::vector<float>(pts.size()));
    generateRotations(pts, std::begin(rPts));
    generateBriefPoints(rPts, out);
}

template<class OutPointsIterator>
void generateBriefFeatures(const Image& i, OutPointsIterator out) {
    auto featuresNumber = std::min(i.width(), i.height());
    std::vector<float> pts;
    generateRandomPoints(featuresNumber, std::back_inserter(pts));
    generateRotatedPoints(pts, out);
}

template <class SinglePassRange, class OutFeaturesIterator>
void generateBriefSet(const SinglePassRange& queries, OutFeaturesIterator out) {
    boost::for_each(queries, [&](const Image & i) {
        generateBriefFeatures(i, std::begin(*out++));
    });
}

#endif	/* CORE_HPP */

