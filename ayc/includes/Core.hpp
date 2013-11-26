/* 
 * File:   Core.hpp
 * Author: stasstels
 *
 * Created on November 8, 2013, 11:38 PM
 */

#ifndef CORE_HPP
#define	CORE_HPP

#include <vector>

#include <boost/iterator/zip_iterator.hpp>
#include <boost/tuple/tuple.hpp>

#include "CImg.h"
using namespace cimg_library;

const float PI = 3.14159265f;

const float BETA_THRESHOLD = 0.1f;
const float BETA_THRESHOLD_INV = 1.0 / BETA_THRESHOLD;

const float GAMMA_THRESHOLD = 1.0f;


const int ROTATIONS_NUMBER = 36;
const int SCALES_NUMBER = 5;

const int CIRCLES_NUMBER = 10;
const int MIN_CIRCLE_RADIUS = 2;


struct Point {
    const int x;
    const int y;
    
    Point(int x, int y) : x(x), y(y) {}
};

typedef CImg<float> Image;
typedef std::vector<Point> Line;
typedef std::vector<float> Samples;

void convert2Gray(const CImg<u_char>& colorImage, Image& grayImage);

float evalSample(const Image& image, const Point& center, const Line& line);
void evalSamples(const Image& image, const Point& origin, int width, int height, const std::vector<Line>& lines, Samples& samples);

float evalCorrelation(const Samples& x, const Samples& y);

void generateQueries(const Image& pattern, std::vector<std::vector<Image> >& queries, int maxScale);
void generateRotations(const Image& pattern, std::vector<Image>& rotations);

void generateCirclesSet(const std::vector<std::vector<Image> >& queries, std::vector<std::vector<Line> >& circlesSet);

void generateRadiansSet(const std::vector<std::vector<Image> >& queries, std::vector<std::vector<Line> >& radiansSet);

void cascadeCiRaTe();
#endif	/* CORE_HPP */

