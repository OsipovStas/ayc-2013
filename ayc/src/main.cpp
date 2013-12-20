#include <cassert>
#include <cstdlib>

#include <iostream>
#include <string>
#include <vector>

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/numeric.hpp>
#include <boost/bind.hpp>
#include <boost/range/algorithm/sort.hpp>
//#include <boost/date_time/posix_time/posix_time.hpp>

#include "mkl.h"
#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>
#include <tbb/concurrent_vector.h>
#include <tbb/task_scheduler_init.h>

#define cimg_OS 0
#include "CImg.h"

#include "Core.hpp"

using namespace cimg_library;

//struct Log {
//    boost::posix_time::ptime start;
//    Log(const char* msg) : start(boost::posix_time::microsec_clock::local_time()) {
//        std::cout << "Log started: " << msg << std::endl;
//    }
//    
//    ~Log() {
//        std::cout << "Log end: Time - " << (boost::posix_time::microsec_clock::local_time() - start).total_microseconds() << " mcs" << std::endl; 
//    }
//};

Image readGrayImage(const char* filename) {
    CImg<u_char> image(filename);
    Image gray(image.width(), image.height(), 1, 1);
    convert2Gray(image, gray);
    return gray;
}

float getScaleRatio(const Image& i) {
    float imageSize = i.width() * i.height();
    if (imageSize < MAX_IMAGE_SIZE) {
        return 1.000f;
    }
    return 1 / std::sqrt(imageSize / MAX_IMAGE_SIZE);
}

struct Result {
    int queryID;
    int x;
    int y;
    float corel;

    Result(int queryID, int x, int y, float corel) : queryID(queryID), x(x), y(y), corel(corel) {
    };

    bool operator<(const Result& r) const {
        return queryID < r.queryID;
    }
};

const int GRAIN_SIZE = 256;

int main(int argc, char** argv) {
    if (argc < 5) return 0;
    int maxThreads = std::atoi(argv[1]);
    float maxScale = std::atof(argv[2]);
    CImg<u_char> image(argv[3]);
    float ratio = getScaleRatio(image);
    int scaleRatio = std::floor(ratio * 100);
    Image gray = readGrayImage(argv[3]).resize(-scaleRatio, -scaleRatio).blur(BLUR);
    tbb::task_scheduler_init tsch(tbb::task_scheduler_init::deferred);
    if (maxThreads > 0) {
        tsch.initialize(maxThreads);
    } else {
        tsch.initialize();
    }
    tbb::concurrent_vector<Result> thirdGrade;
    std::vector<Result> finalResult;

    std::vector<Image> queryScales;
    boost::for_each(boost::irange(4, argc), [&](int i) {
        Image query = readGrayImage(argv[i]);
        generateScales(query, 0.5 * ratio, maxScale * ratio, std::back_inserter(queryScales));
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

    auto avg = [](const Points& points, const Point& center, const Image & image) {
        return evalSample(points, center, image);
    };

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
        evalFeaturesSampleDescriptor(radianSet[probableScale], gray, center, std::begin(radianDescriptor), avg);
        boost::transform(queryRadianDescriptorVector[probableScale], std::begin(correlations), boost::bind(evalCorrelation, _1, boost::cref(radianDescriptor)));
        auto min = boost::max_element(correlations);
        if (*min > RADIAN_FILTER_THRESHOLD) {
            TemplateFilter(center, probableScale, min - std::begin(correlations));
        }
    };

    auto CircleFilter = [&](const Point & center) {
        Descriptors circleDescriptors(QUERY_SCALES_NUMBER, Descriptor(CIRCLES_NUMBER));
        std::vector<float> correlations(QUERY_SCALES_NUMBER);
        evalDescriptors(circleSet, gray, center, std::begin(circleDescriptors), avg);
        boost::transform(queryCircleDescriptors, circleDescriptors, std::begin(correlations), boost::bind(evalCorrelation, _1, _2));
        auto min = boost::max_element(correlations);
        if (*min > CIRCLE_FILTER_THRESHOLD) {
            RadianFilter(center, min - std::begin(correlations));
        }
    };

    tbb::parallel_for(tbb::blocked_range2d<size_t>(0, gray.width(), GRAIN_SIZE, 0, gray.height(), GRAIN_SIZE),
            [&](const tbb::blocked_range2d<size_t>& rng) {
                boost::for_each(boost::irange(rng.rows().begin(), rng.rows().end()), [&](size_t i) {
                    boost::for_each(boost::irange(rng.cols().begin(), rng.cols().end()), [&](size_t j) {
                        CircleFilter(Point(i, j));
                    });
                });
            });
    boost::for_each(thirdGrade, [&](const Result & candidate) {
        auto f = boost::find_if(finalResult, boost::bind<bool>([&](const Result & result) {
            return (std::abs(candidate.x - result.x) < (queryScales[candidate.queryID].width() + queryScales[result.queryID].width()) / 2) &&
                    (std::abs(candidate.y - result.y) < (queryScales[candidate.queryID].height() + queryScales[result.queryID].height()) / 2);
        }, _1));
        if (f != finalResult.end() && f -> corel > candidate.corel) {
            *f = candidate;
        }
        if (f == finalResult.end()) {
            finalResult.push_back(candidate);
        }
    });
    boost::sort(finalResult);
    boost::for_each(finalResult, [&](const Result & r) {
        std::cout << 1 + r.queryID / QUERY_SCALES_NUMBER << "\t" << (int) std::floor(r.x / ratio) << "\t" << (int) std::floor(r.y / ratio) << std::endl;
    });
}

