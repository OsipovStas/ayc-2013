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
//#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/ref.hpp>

#include "CImg.h"
using namespace cimg_library;

const float PI = 3.14159265f;

const float BLUR = 1.2f;

const float BETA_THRESHOLD = 0.1f;
const float BETA_THRESHOLD_INV = 1.0 / BETA_THRESHOLD;

const float GAMMA_THRESHOLD = 1.0f;

const int FULL_DEGREES = 360;
const int ROTATIONS_NUMBER = 20;
const int ROTATION_ANGLE = FULL_DEGREES / ROTATIONS_NUMBER;

const int CIRCLES_NUMBER = 10;
const int MIN_CIRCLE_RADIUS = 2;
const int MAX_CIRCLE_RADIUS = 50;

const float CIRCLE_FILTER_THRESHOLD = 0.95;
const float RADIAN_FILTER_THRESHOLD = 0.9;
const float TEMPLATE_FILTER_THRESHOLD = 0.9;

struct Point {
    const int x;
    const int y;

    Point(int x, int y) : x(x), y(y) {
    }
};

//struct Log {
//
//    Log(const char* msg) : start(boost::posix_time::microsec_clock::local_time()) {
//        std::cout << "Log begin: " << msg << std::endl;
//    }
//
//    ~Log() {
//        std::cout << "Log end. Time: " << (boost::posix_time::microsec_clock::local_time() - start).total_microseconds() << std::endl;
//    }
//
//    boost::posix_time::ptime start;
//};

typedef CImg<float> Image;
typedef std::vector<Point> Points;
typedef std::vector<Points> Features;
typedef std::vector<Features> FeaturesVector;
typedef std::vector<float> Descriptor;
typedef std::vector<Descriptor> Descriptors;
typedef std::vector<Descriptors> DescriptorsVector;


float point2float(const Point& p, const Point& c, const Image& i);


template<class OutputIterator>
void generateScales(const Image& query, int maxScale, OutputIterator out) {
    boost::for_each(boost::irange(1, maxScale + 1), [&](int i) {
        *out++ = query.get_resize(-i * 100, -i * 100);
    });
}

template<class SinglePassRange, class OutputIterator>
void generateQueryRotations(const SinglePassRange& queries, OutputIterator out) {
    boost::for_each(queries, [&](const Image & image) {
        auto inserter = std::back_inserter(*out++);
        boost::for_each(boost::irange(0, FULL_DEGREES, ROTATION_ANGLE), [&](int angle) {
            inserter = image.get_rotate(angle).blur(BLUR);
        });
    });
}

void generateCircle(int radius, const Point& center, Points& circle);

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

void generateRadianLine(int angle, int radius, const Point& center, Points& radianLine);

template <class SinglePassRange, class OutputIterator>
void generateRadianSet(const SinglePassRange& queries, OutputIterator out) {
    boost::for_each(queries, [&](const Image & image) {
        auto featuresOut = std::begin(*out++);
        auto radius = std::min(std::min(image.height(), image.width()) / 2, MAX_CIRCLE_RADIUS);
                radius = (radius / CIRCLES_NUMBER) * CIRCLES_NUMBER;
                boost::for_each(boost::irange(0, FULL_DEGREES, ROTATION_ANGLE), [&](int angle) {
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
            auto c = Point(image.width() / 2, image.height() / 2);
            cimg_forXY(image, x, y) {
                if (image(x, y) != 0) {
                    inserter = Point(x - c.x, y - c.y);
                }
            }
        });
    });
}

void convert2Gray(const CImg<u_char>& colorImage, Image& grayImage);

float evalSample(const Points& points, const Point& center, const Image& image);

template <class OutputIterator>
void evalQueryFeaturesSampleDescriptor(const Features& points, const Image& i, OutputIterator out) {
    boost::transform(points, out, boost::bind<float>(evalSample, _1, Point(i.width() / 2, i.height() / 2), boost::cref(i)));
}

template<class OutputIterator> 
void evalPointDescriptor(const Points& points, const Point& p, const Image& image, OutputIterator out) {
    boost::transform(points, out, boost::bind<float>(point2float, _1, boost::cref(p), boost::cref(image)));
}


template <class ImageInputIterator, class OutputIterator>
void evalQueryFeaturesPointDescriptor(const Features& features, ImageInputIterator images, OutputIterator out) {
    boost::for_each(features, [&](const Points& points) {
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

template<class InputImageIterator, class OutputIterator>
void evalQueryTemplateDescriptors(const FeaturesVector& featureSet, InputImageIterator images, OutputIterator out) {
    boost::for_each(featureSet, [&](const Features & features) {
        evalQueryFeaturesPointDescriptor(features, std::begin(*images++),  std::begin(*out++));
    });
}


bool isFitImage(float r, const Image& i, const Point& c);
float getMaxRadius(const Features& f);

template<class OutputIterator>
void evalFeaturesSampleDescriptor(const Features& features, const Image& i, const Point& c, OutputIterator out) {
    if (isFitImage(getMaxRadius(features), i, c)) {
        boost::transform(features, out, boost::bind<float>(evalSample, _1, c, boost::cref(i)));
    }

}

template<class OutputIterator>
void evalDescriptors(const FeaturesVector& featureSet, const Image& image, const Point& center, OutputIterator out) {
    boost::for_each(featureSet, [&](const Features & features) {
        evalFeaturesSampleDescriptor(features, image, center, std::begin(*out++));
    });
}

template<class InputImagesIterator, class OutputIterator>
void evalQueryRadianDescriptorsVector(const FeaturesVector& featureSet, InputImagesIterator images, OutputIterator out) {
    Descriptors radianDescriptors(featureSet.size(), Descriptor(ROTATIONS_NUMBER));
    evalQueryDescriptors(featureSet, images, std::begin(radianDescriptors));
    boost::for_each(radianDescriptors, [&](const Descriptor & d) {
        auto DescriptorsOut = std::begin(*out++);
        boost::for_each(boost::irange(0, ROTATIONS_NUMBER), [&](int r) {
            boost::rotate_copy(d, std::begin(d) + r, std::back_inserter(*DescriptorsOut++));
        });
    });
}

float evalCorrelation(const Descriptor& x, const Descriptor& y);

void generateQueries(const Image& pattern, std::vector<std::vector<Image> >& queries, int maxScale);
void generateRotations(const Image& pattern, std::vector<Image>& rotations);

void generateRadiansSet(const std::vector<std::vector<Image> >& queries, std::vector<std::vector<Points> >& radiansSet);

#endif	/* CORE_HPP */

