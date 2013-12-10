#include <cassert>
#include <cstdlib>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/numeric.hpp>
#include <boost/bind.hpp>
#include <boost/range/algorithm/sort.hpp>

#include "mkl.h"


#define cimg_OS 0
#include "CImg.h"

#include "Core.hpp"

using namespace cimg_library;

Image readGrayImage(const char* filename) {
    CImg<u_char> image(filename);
    Image gray(image.width(), image.height(), 1, 1);
    convert2Gray(image, gray);
    return std::move(gray);
}

struct Result {
    int queryID;
    int x;
    int y;
    float corel;

    Result(int queryID, int x, int y, float corel) : queryID(queryID), x(x), y(y), corel(corel) {};
    
    bool operator<(const Result& r) const {
        return queryID < r.queryID;
    } 
};

int main(int argc, char** argv) {
    if (argc < 5) return 0;

    int maxThreads = std::atoi(argv[1]);
    float maxScale = std::atof(argv[2]);
    CImg<u_char> image(argv[3]);
    Image gray = readGrayImage(argv[3]).blur(BLUR);
    std::ofstream out("out.txt");
    std::vector<Result> thirdGrade;
    std::vector<Result> finalResult;

    std::vector<Image> queryScales;
    boost::for_each(boost::irange(4, argc), [&](int i) {
        Image query = readGrayImage(argv[i]);
        generateScales(query, maxScale, std::back_inserter(queryScales));
    });
    auto QUERY_NUMBER = argc - 4;
    auto QUERY_SCALES_NUMBER = queryScales.size();

    std::vector < std::vector<Image> > queryRotations(QUERY_SCALES_NUMBER);

    FeaturesVector circleSet(QUERY_SCALES_NUMBER, Features(CIRCLES_NUMBER));
    FeaturesVector radianSet(QUERY_SCALES_NUMBER, Features(ROTATIONS_NUMBER));
    FeaturesVector pointSet(QUERY_SCALES_NUMBER, Features(ROTATIONS_NUMBER));

    Descriptors queryCircleDescriptors(QUERY_SCALES_NUMBER, Descriptor(CIRCLES_NUMBER));
    std::vector<Descriptors> queryRadianDescriptorVector(QUERY_SCALES_NUMBER, Descriptors(ROTATIONS_NUMBER));
    std::vector<Descriptors> queryTemplateDescriptorVector(QUERY_SCALES_NUMBER, Descriptors(ROTATIONS_NUMBER));

    generateQueryRotations(queryScales, std::begin(queryRotations));
    boost::for_each(queryScales, [](Image & q) {
        q = q.blur(BLUR);
    });

    generateCirclesSet(queryScales, std::begin(circleSet));
    generateRadianSet(queryScales, std::begin(radianSet));
    generatePointSet(queryRotations, std::begin(pointSet));

    evalQueryDescriptors(circleSet, std::begin(queryScales), std::begin(queryCircleDescriptors));
    evalQueryRadianDescriptorsVector(radianSet, std::begin(queryScales), std::begin(queryRadianDescriptorVector));
    evalQueryTemplateDescriptors(pointSet, std::begin(queryRotations), std::begin(queryTemplateDescriptorVector));

    auto TemplateFilter = [&](const Point& center, int probableScale, int probableRotation) {
        auto i = queryRotations[probableScale][probableRotation];
        auto r = std::max(i.width(), i.height()) / 2;
        if (!isFitImage(r, gray, center)) {
            return;
        }
        Descriptor templateDescriptor;
        evalPointDescriptor(pointSet[probableScale][probableRotation], center, gray, std::back_inserter(templateDescriptor));
        auto cor = evalCorrelation(templateDescriptor, queryTemplateDescriptorVector[probableScale][probableRotation]);
        if (cor > TEMPLATE_FILTER_THRESHOLD) {
            thirdGrade.push_back(Result(probableScale, center.x, center.y, cor));
        }
    };

    auto RadianFilter = [&](const Point& center, int probableScale) {
        Descriptor radianDescriptor(ROTATIONS_NUMBER);
        std::vector<float> correlations(ROTATIONS_NUMBER);
        evalFeaturesSampleDescriptor(radianSet[probableScale], gray, center, std::begin(radianDescriptor));
        boost::transform(queryRadianDescriptorVector[probableScale], std::begin(correlations), boost::bind(evalCorrelation, _1, boost::cref(radianDescriptor)));
        auto min = boost::max_element(correlations);
        if (*min > RADIAN_FILTER_THRESHOLD) {
            TemplateFilter(center, probableScale, min - std::begin(correlations));
        }
    };

    auto CircleFilter = [&](const Point & center) {
        Descriptors circleDescriptors(QUERY_SCALES_NUMBER, Descriptor(CIRCLES_NUMBER));
        std::vector<float> correlations(QUERY_SCALES_NUMBER);
        evalDescriptors(circleSet, gray, center, std::begin(circleDescriptors));
        boost::transform(queryCircleDescriptors, circleDescriptors, std::begin(correlations), boost::bind(evalCorrelation, _1, _2));
        auto min = boost::max_element(correlations);
        if (*min > CIRCLE_FILTER_THRESHOLD) {
            RadianFilter(center, min - std::begin(correlations));
        }
    };

    cimg_forXY(gray, x, y) {
        CircleFilter(Point(x, y));
    }

    boost::for_each(thirdGrade, [&](const Result & candidate) {
        auto f = boost::find_if(finalResult, boost::bind<bool>([&](const Result & result) {
            return (std::abs(candidate.x - result.x) < (queryScales[candidate.queryID].width() + queryScales[result.queryID].width()) / 2) &&
                   (std::abs(candidate.y - result.y) < (queryScales[candidate.queryID].height() + queryScales[result.queryID].height()) / 2);
        }, _1));
        if(f != finalResult.end() && f -> corel > candidate.corel) {
            *f = candidate;
        }
        if(f == finalResult.end()) {
            finalResult.push_back(candidate);
        }
    });
    boost::sort(finalResult);
    boost::for_each(finalResult, [&](const Result& r) {
       out << 1 + r.queryID / QUERY_SCALES_NUMBER << "\t" << r.x << "\t" << r.y << std::endl;
    });
}
