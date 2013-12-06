#include <cassert>

#include <iostream>
#include <string>
#include <list>

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/numeric.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "mkl.h"

#include "CImg.h"

#include "BitmapImporter.h"
#include "Core.hpp"

using namespace cimg_library;

int main(int argc, char** argv) {
    if (argc <= 1) return 0;
    if (argv == NULL) return 0;

    //    Samples x = {1, 2, 3};
    //    Samples y = {2, 4, 6};
    //    evalCorrelation(x, y);

    std::string image_name(argv[1]);
    CImg<u_char> image(image_name.c_str());
    CImg<float> gray(image.width(), image.height(), 1, 1);
    convert2Gray(image, gray);
    //    float white = 0;

    std::vector<Image> queryScales;
    std::vector < std::vector<Image> > queryRotations(SCALES_NUMBER);

    FeatureSet circleSet(SCALES_NUMBER, Features(CIRCLES_NUMBER));
    FeatureSet radianSet(SCALES_NUMBER, Features(ROTATIONS_NUMBER));
    FeatureSet pointSet(SCALES_NUMBER, Features(ROTATIONS_NUMBER));

    generateScales(gray, SCALES_NUMBER, std::back_inserter(queryScales));
    generateQueryRotations(queryScales, ROTATIONS_NUMBER, std::begin(queryRotations));
    generateCirclesSet(queryScales, CIRCLES_NUMBER, std::begin(circleSet));
    generateRadianSet(queryScales, ROTATIONS_NUMBER, std::begin(radianSet));
    generatePointSet(queryRotations, std::begin(pointSet));
    std::cout << "Хуй" << std::endl;

    //    
    //    
    //    
    //    
    //    std::vector<std::vector<Image> > queries;
    //    std::vector<std::vector<Points> > circles, radians;
    //    generateQueries(gray, queries, 3);
    //    generateCirclesSet(queries, circles);
    //    generateRadiansSet(queries, radians);
    //
    //    std::vector<Descriptor> circleSamples(circles.size(), Descriptor(CIRCLES_NUMBER));
    //    evalSamplesSet(circles, Point(0, 0), std::begin(queries), std::begin(circleSamples));


    //draw circles and radians
    for (int i = 0; i < SCALES_NUMBER; ++i) {
        auto circles = circleSet[i];
        auto rad = radianSet[i];
        int w = queryScales[i].width() / 2;
        int h = queryScales[i].height() / 2;
        for (auto c : circles) {
            for (auto p : c) {
                queryScales[i](p.x + w, p.y + h) = 0;
            }
        }
        for (auto r : rad) {
            for (auto p : r) {
                queryScales[i](p.x + w, p.y + h) = 0;
            }
        }
    }
    
    //draw point set on rotations
    for(int i = 0; i < SCALES_NUMBER; ++i) {
        for(int j = 0; j < 1; ++j) {
            std::cout << pointSet[i][j].size() << std::endl;
            boost::for_each(pointSet[i][j], [&](const Point& p) {
//                std::cout << queryRotations[i][j](p.x, p.y) << std::endl;
            });
        }
    }
    
    //
    //    std::cout << "Circle sizes: " << circles.size() << " By scale: " << circles.front().size() << std::endl;
    //    std::cout << "Radians sizes: " << radians.size() << " By scale: " << radians.front().size() << std::endl;

    //Show scales
    for (auto s : queryScales) {
        CImgDisplay main_disp(s, "Click a point");
        while (!main_disp.is_closed()) {
            main_disp.wait();
        }
    }
//    //Show rotations
//    for (auto s : queryRotations) {
//        for (auto r : s) {
//            CImgDisplay main_disp(r, "Click a point");
//            while (!main_disp.is_closed()) {
//                main_disp.wait();
//            }
//        }
//    }
}

//Test generate point sets
//std::vector<std::vector<Image> > queries;
//std::vector<std::vector<Line> > circles, radians;
//generateQueries(gray, queries, 3);
//generateCirclesSet(queries, circles);
//generateRadiansSet(queries, radians);
//for (int i = 0; i < SCALES_NUMBER; ++i) {
//    auto circle = circles[i];
//    auto rad = radians[i];
//    for (auto c : circle) {
//        for (auto p : c) {
//            queries[i][0](p.x, p.y) = 0;
//        }
//    }
//    for (auto r : rad) {
//        for (auto p : r) {
//            queries[i][0](p.x, p.y) = 0;
//        }
//    }
//}
//
//for (auto v : queries) {
//    auto i = v[0];
//    CImgDisplay main_disp(i, "Click a point");
//    while (!main_disp.is_closed()) {
//        main_disp.wait();
//    }
//}