#include "Core.hpp"

#include <cassert>
#include <iostream>
#include <cmath>
#include <mkl_cblas.h>

void convert2Gray(const CImg<u_char>& colorImage, Image& grayImage) {
    assert(colorImage.width() == grayImage.width());
    assert(colorImage.height() == grayImage.height());
    static const float R = 0.299f;
    static const float G = 0.587f;
    static const float B = 0.114f;

    cimg_forXY(colorImage, x, y) {
        grayImage(x, y) = ((colorImage(x, y, 0, 1) * R) + (colorImage(x, y, 0, 1) * G) + (colorImage(x, y, 0, 2) * B)) / 255;
        grayImage(x, y) *= 255;

    }
}

float evalSample(const Image& image, const Point& center, const Line& line) {
    float res = 0.0;
    for (const Point& p : line) {
        res += image(center.x + p.x, center.y + p.y);
    }
    return res;
}

void evalSamples(const Image& image, const Point& origin, int width, int height, const std::vector<Line>& lines, Samples& samples) {
    Point center = {origin.x + width / 2, origin.y + height / 2};
    for (const Line& c : lines) {
        samples.push_back(evalSample(image, center, c));
    }
}

float evalCorrelation(const Samples& x, const Samples& y) {
    assert(x.size() == y.size());

    const int size = x.size();
    Samples xMeanCorrected(size, 1.0f);
    Samples yMeanCorrected(size, 1.0f);

    float xMean = cblas_sasum(size, x.data(), 1) / size;
    float yMean = cblas_sasum(size, y.data(), 1) / size;

    cblas_saxpby(size, 1.0f, x.data(), 1, -xMean, xMeanCorrected.data(), 1);
    cblas_saxpby(size, 1.0f, y.data(), 1, -yMean, yMeanCorrected.data(), 1);

    float xMeanCorrectedSquare = cblas_sdot(size, xMeanCorrected.data(), 1, xMeanCorrected.data(), 1);

    float beta = cblas_sdot(size, xMeanCorrected.data(), 1, yMeanCorrected.data(), 1) / xMeanCorrectedSquare;
    float gamma = yMean - beta * xMean;

    float betaAbs = std::abs(beta);
    float gammaAbs = std::abs(gamma);

    //    std::cout << "xMean: " << xMean << " yMean: " << yMean << " xMeanCorrected: " << xMeanCorrectedSquare 
    //            << " x*y: " << cblas_sdot(size, xMeanCorrected.data(), 1, yMeanCorrected.data(), 1) << std::endl;

    if (betaAbs < BETA_THRESHOLD || betaAbs > BETA_THRESHOLD_INV || gammaAbs > GAMMA_THRESHOLD) {
        return 0;
    }

    float xMeanCorrectedNorm = cblas_snrm2(size, xMeanCorrected.data(), 1);
    float yMeanCorrectedNorm = cblas_snrm2(size, yMeanCorrected.data(), 1);

    //    std::cout << "Xnorm " << xMeanCorrectedNorm << " Ynorm " << yMeanCorrectedNorm << std::endl;

    return (beta * xMeanCorrectedSquare) / (xMeanCorrectedNorm * yMeanCorrectedNorm);
}

void generateRotations(const Image& pattern, std::vector<Image>& rotations) {
    int diff = 360 / ROTATIONS_NUMBER;
    for (int i = 0; i < 360; i += diff) {
        rotations.push_back(pattern.get_rotate(i));
    }
}

void generateQueries(const Image& pattern, std::vector<std::vector<Image> >& queries, int maxScale) {
    int diff = (maxScale * 100 - 100) / SCALES_NUMBER;
    for (int i = 100; i < maxScale * 100; i += diff) {
        queries.push_back(std::vector<Image>());
        generateRotations(pattern.get_resize(-i, -i), queries.back());
    }
}

void generateCircle(const Image& image, int radius, Line& circle) {
    int x0 = image.width() / 2;
    int y0 = image.height() / 2;
    int error = 1 - radius;
    int errorY = 1;
    int errorX = -2 * radius;
    int x = radius, y = 0;

    circle.push_back(Point(x0, y0 + radius));
    circle.push_back(Point(x0, y0 - radius));
    circle.push_back(Point(x0 + radius, y0));
    circle.push_back(Point(x0 - radius, y0));

    while (y < x) {
        if (error > 0) // >= 0 produces a slimmer circle. =0 produces the circle picture at radius 11 above
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

void generateCircles(const Image& image, std::vector<Line>& circles) {
    const int MAX_CIRCLE_RADIUS = std::min(image.width(), image.height()) / 2;
    int diff = MAX_CIRCLE_RADIUS / CIRCLES_NUMBER;
    for (int r = MAX_CIRCLE_RADIUS; r > 0; r -= diff) {
        circles.push_back(Line());
        generateCircle(image, r, circles.back());
    }
}

void generateCirclesSet(const std::vector<std::vector<Image> >& queries, std::vector<std::vector<Line> >& circlesSet) {
    for (auto q : queries) {
        circlesSet.push_back(std::vector<Line>());
        generateCircles(q.front(), circlesSet.back());
    }
}

void generateRadianLine(const Image& image, int angle, int radius, Line& radianLine) {
    int x0 = image.width() / 2;
    int y0 = image.height() / 2;

    int x1 = std::cos(-angle * PI / 180.0) * radius + x0;
    int y1 = std::sin(-angle * PI / 180.0) * radius + y0;



    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2; /* error value e_xy */

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

void generateRadians(const Image& image, std::vector<Line>& radians) {
    const int radius = std::min(image.width(), image.height()) / 2;
    int diff = 360 / ROTATIONS_NUMBER;
    for (int a = 0; a < 360; a += diff) {
        radians.push_back(Line());
        generateRadianLine(image, a, radius, radians.back());
    }
}

void generateRadiansSet(const std::vector<std::vector<Image> >& queries, std::vector<std::vector<Line> >& radiansSet) {
    for (auto q : queries) {
        radiansSet.push_back(std::vector<Line>());
        generateRadians(q.front(), radiansSet.back());
    }
}

//////////////Circle Filter
