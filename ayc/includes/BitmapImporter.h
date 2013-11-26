/**
 * \file BitmapImporter.h  
 * \brief Contains declarations used to parse and hold the .bmp data.
 * \author Cédric Andreolli (Intel)
 * \date 10 April 2013
 */
#ifndef BITMAP_IMPORTER_H
#define BITMAP_IMPORTER_H

#include <cstdlib>
#include <stdint.h>

#include <string>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>

namespace ayc {

    /*!
     * \struct HeaderStr
     * \brief This structure holds the .bmp header informations
     * 
     * The HeaderStr structure contains:<br />
     * <ul>
     *	<li>The size of the file</li>
     *	<li>The offset of the data</li>
     *	<li>The width of the image</li>
     *	<li>The height of the image</li>
     *	<li>The number of bits per pixel</li>
     *	<li>Etc.</li>
     * </ul>
     * You don't have to use this structure and you can write/use your own \
     * structure if you want.
     */
    typedef struct __attribute__((__packed__)) {
        //BMP header
        uint16_t magic_number;
        uint32_t size;
        uint32_t reserved;
        uint32_t offset;
        //DIB header
        uint32_t dibSize;
        uint32_t width;
        uint32_t height;
        uint16_t plane;
        uint16_t bit_per_pixel;
        uint32_t compression;
        uint32_t data_size;
        uint32_t hor_res;
        uint32_t vert_res;
        uint32_t color_number;
        uint32_t important;
    }
    HeaderStr;

    /*!
     * \struct PixelStr
     * \brief This structure holds a pixel
     * 
     * In the program, a pixel is represented by 3 elements that can define its color.
     * <ul>
     *	<li><b>r: </b>The red value of the pixel</li>
     *	<li><b>g: </b>The green value of the pixel</li>
     *	<li><b>b: </b>The blue value of the pixel</li>
     * </ul>
     */
    typedef struct __attribute__((__packed__)) {
        unsigned char b;
        unsigned char g;
        unsigned char r;
    }
    PixelStr;

    typedef struct __attribute__((__packed__)) {
        //BMP header
        uint16_t magicNumber;
        uint32_t size;
        uint32_t reserved;
        uint32_t offset;
        //DIB header
        uint32_t dibSize;
        uint32_t width;
        uint32_t height;
        uint16_t plane;
        uint16_t bitPerPixel;
        uint32_t compression;
        uint32_t dataSize;
        uint32_t horRes;
        uint32_t vertRes;
        uint32_t colorNumber;
        uint32_t important;
    }
    BitMapHeader;

    class BitMapImage {
    public:

        BitMapImage(const BitMapImage& other) :
        width(other.width),
        height(other.height),
        imageHeader(other.imageHeader),
        colorData(other.colorData),
        greyData(other.greyData) {
        };

        BitMapImage& operator=(const BitMapImage& other);

        void swap(BitMapImage& other);

        bool imageWrite(const std::string& filename) const;

        PixelStr* getPixels() {
            return colorData.get();
        }

        //        void convertToGrey();
        //
        //        void convertToBGR();

        int getHeight() {
            return imageHeader -> height;
        }

        int getWidth() {
            return imageHeader -> width;
        }

        PixelStr getPixel(unsigned int i, unsigned int j) const;

        void setPixel(unsigned int i, unsigned int j, const PixelStr& color);

        ~BitMapImage();

        static BitMapImage imageRead(const std::string& filename);

    private:

        friend std::ostream& operator<<(std::ostream &o, const ayc::BitMapImage& image);

        BitMapImage() : width(0), height(0), imageHeader(), colorData(), greyData() {
        };

        uint width;
        uint height;

        std::shared_ptr<BitMapHeader> imageHeader;
        std::shared_ptr<PixelStr> colorData;
        std::shared_ptr<float> greyData;
    };

    std::ostream& operator<<(std::ostream &o, ayc::BitMapImage& image);

    /*!
     * \class Image
     * \brief The Image class holds the data of the image. 
     *
     * To instantiate an image from a bitmap, you can use the static function:<br />
     * Image::create_image_from_bitmap(const std::string file_name)<br />
     * 
     */
    class Image {
    protected:
        PixelStr* pixel_data; /*!< The image data */
        unsigned int width; /*!< The width of the image*/
        unsigned int height; /*!< The height of the image*/

        /*!
         * \brief Default constructor of an image
         *
         * This constructor doesn't need to be public.
         *
         */
        Image();

        /*!
         * \brief Copy the column represented by the variable column_number at the right place in a scaled image
         * \param result The scaled image based on this
         * \param column_number The column indice to copy in the current image
         * \param scale The scale factor
         */
        void copy_column(ayc::Image& result, unsigned int column_number, unsigned int scale) const;
    public:
        /*!
         * \brief The destructor
         */
        ~Image();

        /*!
         * \brief Returns the pixel at the position [i,j] in the image
         * 
         * This function helps you to retrieve the value of a single pixel in the image.
         * As the image is represented by a single dimension array, to retrieve the pixel
         * at the position [i,j] you must compute i*image_width + j.
         *
         * \param i The rown indice
         * \param j the column indice
         * \return The PixelStr at position [i,j]
         */
        PixelStr get_pixel(unsigned int i, unsigned int j) const;

        /*!
         * \brief Create a new scaled image from the current image
         * \param scale The scale factor.
         * \return A new scaled image. 
         */
        Image scale_image(unsigned int scale) const;

        void set_pixels(PixelStr* data) {
            this->pixel_data = data;
        }

        PixelStr* get_pixels() {
            return pixel_data;
        }

        void set_width(const unsigned int width) {
            this->width = width;
        }

        unsigned int get_width()const {
            return this->width;
        }

        void set_height(const unsigned int height) {
            this->height = height;
        }

        unsigned int get_height()const {
            return this->height;
        }

        /*!
         * \brief Creates an image from a bitmap file
         * \param file_name The path of the bitmap file
         * \return A new image
         */
        static Image create_image_from_bitmap(const std::string file_name);
    };

}

std::ostream& operator<<(std::ostream &o, const ayc::PixelStr& p);
std::ostream& operator<<(std::ostream &o, ayc::Image& im);
#endif
