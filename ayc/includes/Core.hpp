/* 
 * File:   Core.hpp
 * Author: stasstels
 *
 * Created on November 8, 2013, 11:38 PM
 */

#ifndef CORE_HPP
#define	CORE_HPP

#include <vector>

#include <boost/bind.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/irange.hpp>
#include <boost/range/any_range.hpp>

#include "CImg.h"
using namespace cimg_library;

const float PI = 3.14159265f;

const float BETA_THRESHOLD = 0.1f;
const float BETA_THRESHOLD_INV = 1.0 / BETA_THRESHOLD;

const float GAMMA_THRESHOLD = 1.0f;


const int ROTATIONS_NUMBER = 12;
const int SCALES_NUMBER = 5;

const int CIRCLES_NUMBER = 17;
const int MIN_CIRCLE_RADIUS = 2;

struct Point {
    const int x;
    const int y;

    Point(int x, int y) : x(x), y(y) {
    }
};

typedef CImg<float> Image;
typedef std::vector<Point> Points;
typedef std::vector<Points> Features;
typedef std::vector<Features> FeatureSet;
typedef std::vector<float> Descriptor;
typedef std::vector<Descriptor> DescriptorSet;


template<class OutputIterator>
void generateScales(const Image& query, int maxScale, OutputIterator out) {
    boost::for_each(boost::irange(1, maxScale + 1), [&](int i) {
        *out++ = query.get_resize(-i * 100, -i * 100);
    });
}

template<class SinglePassRange, class OutputIterator>
void generateQueryRotations(const SinglePassRange& queries, int rotationNumber, OutputIterator out) {
    boost::for_each(queries, [&](const Image & image) {
        auto inserter = std::back_inserter(*out++);
        boost::for_each(boost::irange(0, rotationNumber * 10, 10), [&](int angle) {
            inserter = image.get_rotate(angle);
        });
    });
}

void generateCircle(int radius, const Point& center, Points& circle);

template <class SinglePassRange, class OutputIterator>
void generateCirclesSet(const SinglePassRange& queries, int circlesNumber, OutputIterator out) {
    boost::for_each(queries, [&](const Image & image) {
        auto featuresOut = std::begin(*out++);
        auto maxRadius = std::min(image.height(), image.width()) / 2;
                auto diff = std::max((maxRadius - MIN_CIRCLE_RADIUS) / circlesNumber, 1);
                boost::for_each(boost::irange(0, circlesNumber), [&](int i) {
                    int radius = std::min(i * diff + MIN_CIRCLE_RADIUS, maxRadius);
                    generateCircle(radius, Point(0, 0), *featuresOut++);
                });
    });
}

void generateRadianLine(int angle, int radius, const Point& center, Points& radianLine);

template <class SinglePassRange, class OutputIterator>
void generateRadianSet(const SinglePassRange& queries, int rotationNumber, OutputIterator out) {
    boost::for_each(queries, [&](const Image & image) {
        auto featuresOut = std::begin(*out++);
        auto radius = std::min(image.height(), image.width()) / 2;
                boost::for_each(boost::irange(0, rotationNumber * 10, 10), [&](int angle) {
                    generateRadianLine(angle, radius, Point(0, 0), *featuresOut++);
                });
    });
}

template<class SinglePassRange, class OutputIterator>
void generatePointSet(const SinglePassRange& queriesRotations, OutputIterator out) {
    boost::for_each(queriesRotations, [&](const std::vector<Image>& rotations) {
        auto featuresOut = std::begin(*out++);
        boost::for_each(rotations, [&](const Image & image) {
            auto inserter = std::back_inserter(*featuresOut++);
            cimg_forXY(image, x, y) {
                if (image(x, y) != 0) {
                    inserter = Point(x, y);
                }
            }
        });
    });
}

void convert2Gray(const CImg<u_char>& colorImage, Image& grayImage);

float evalSample(const Points& line, const Point& center, const Image& image);

template <class OutputIterator>
void evalFeaturesDescriptor(const Features& points, const Image& i, const Point& c, OutputIterator out) {
    boost::transform(points, out, boost::bind<float>(evalSample, _1, c, i));
}


template<class InputImageIterator, class OutputIterator>
void evalDescriptorSet(const FeatureSet& featureSet, const Point& c, InputImageIterator images, OutputIterator out) {
    boost::for_each(featureSet, [&](const Features& features) {
        evalFeaturesDescriptor(features, *images++, c, std::begin(*out++));
    });
}


float evalCorrelation(const Descriptor& x, const Descriptor& y);

void generateQueries(const Image& pattern, std::vector<std::vector<Image> >& queries, int maxScale);
void generateRotations(const Image& pattern, std::vector<Image>& rotations);

void generateRadiansSet(const std::vector<std::vector<Image> >& queries, std::vector<std::vector<Points> >& radiansSet);

void cascadeCiRaTe();
#endif	/* CORE_HPP */

