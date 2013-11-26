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
    float white = 0;

    std::vector<std::vector<Image> > queries;
    std::vector<std::vector<Line> > circles, radians;
    generateQueries(gray, queries, 3);
    generateCirclesSet(queries, circles);
    generateRadiansSet(queries, radians);

    Point center(0, 0);
    auto foo = boost::bind<float>([&](const Point& p) {
        return gray(p.x + center.x, p.y + center.y) / 255;
    }, _1);  
    float f = boost::accumulate(circles[0][0] | boost::adaptors::transformed(foo), 0.0f);
    
    std::cout << f << std::endl;

    for (int i = 0; i < SCALES_NUMBER; ++i) {
        auto circle = circles[i];
        auto rad = radians[i];
        for (auto c : circle) {
            for (auto p : c) {
                queries[i][0](p.x, p.y) = 0;
            }
        }
        for (auto r : rad) {
            for (auto p : r) {
                queries[i][0](p.x, p.y) = 0;
            }
        }
    }

    std::cout << "Circle sizes: " << circles.size() << " By scale: " << circles.front().size() << std::endl;
    std::cout << "Radians sizes: " << radians.size() << " By scale: " << radians.front().size() << std::endl;


    for (auto v : queries) {
        auto i = v[0];
        CImgDisplay main_disp(i, "Click a point");
        while (!main_disp.is_closed()) {
            main_disp.wait();
        }
    }
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