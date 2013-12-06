/*!
 * \file BitmapImporter.cpp
 * \brief Contains the implementation of the functions declared in BitmapImporter.h
 * \author Cédric Andreolli (Intel)
 * \date 10 April 2013
 */
#include <utility>
#include <sys/types.h>

#include "BitmapImporter.h" 



using namespace std;

ayc::BitMapImage::~BitMapImage() {
};

void ayc::BitMapImage::swap(BitMapImage& other) {
    std::swap(width, other.width);
    std::swap(height, other.height);
    std::swap(imageHeader, other.imageHeader);
    std::swap(colorData, other.colorData);
    std::swap(greyData, other.greyData);
}

ayc::BitMapImage& ayc::BitMapImage::operator=(const ayc::BitMapImage& other) {

    if (this == &other) {
        return *this;
    }

    ayc::BitMapImage(other).swap(*this);
    return *this;

}

ayc::BitMapImage ayc::BitMapImage::imageRead(const std::string& filename) {
    ayc::BitMapImage result;
    std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    if (in.is_open()) {
        std::shared_ptr<ayc::BitMapHeader> header(new BitMapHeader);
        in.seekg(0, std::ios::beg);
        in.read((char*) (header.get()), sizeof (ayc::BitMapHeader));

        result.width = header -> width;
        result.height = header -> height;

        in.seekg(header -> offset, std::ios::beg);
        int bytesPerPixel = (header -> bitPerPixel) / 8;
        int padding = (((header -> width) * bytesPerPixel) % 4 == 0) ? 0 : 4 - (((header -> width) * bytesPerPixel) % 4);

        u_int size = sizeof (u_char) * ((header -> height) * ((header -> width) + padding)) * bytesPerPixel;
        std::shared_ptr<u_char> data(new u_char[size], [] (u_char * p) {
            delete[] p; });
        in.read((char*) (data.get()), size);

        result.imageHeader = header;
        result.colorData = std::shared_ptr<PixelStr>(new PixelStr[(header -> width) * (header -> height) * sizeof (PixelStr)], [] (PixelStr * p) {
            delete[] p; });

        unsigned int offset = 0;
        //In the Bitmap format, pixels are in a reversed order
        for (int i = result.height - 1; i >= 0; --i) {
            for (int j = 0; j < result.width; ++j) {
                (result.colorData.get() + i * result.width + j) -> b = *(data.get() + offset++);
                (result.colorData.get() + i * result.width + j) -> g = *(data.get() + offset++);
                (result.colorData.get() + i * result.width + j) -> r = *(data.get() + offset++);
            }
            offset += padding;
        }
        in.close();
    }

    return result;
}

bool ayc::BitMapImage::imageWrite(const std::string& filename) const {
    std::ofstream out(filename.c_str(), std::ios::out | std::ios::binary);
    if (out.is_open()) {
        out.write((char*) (imageHeader.get()), sizeof (ayc::BitMapHeader));

        int bytesPerPixel = (imageHeader -> bitPerPixel) / 8;
        int padding = (((imageHeader -> width) * bytesPerPixel) % 4 == 0) ? 0 : 4 - (((imageHeader -> width) * bytesPerPixel) % 4);

        u_int size = ((imageHeader -> width) * bytesPerPixel);
        for (int i = 0; i < (imageHeader -> height); ++i) {
            out.write((char*) (colorData.get() + i * size), size);
            out.write("777", padding);
        }
        out.close();
    }
    return true;
}

//void ayc::BitMapImage::convertToGrey() {
//    if (greyData) {
//        return;
//    }
//
//    static const float BLUE = 0.114f;
//    static const float GREEN = 0.587f;
//    static const float RED = 0.299f;
//
//
//    u_int width = imageHeader -> width;
//    u_int height = imageHeader -> height;
//    u_int bytesPerPixel = (imageHeader -> bitPerPixel) / 8;
//    u_int padding = ((width * bytesPerPixel) % 4 == 0) ? 0 : 4 - ((width * bytesPerPixel) % 4);
//
//    u_int size = width * height;
//    std::shared_ptr<float> data(new float[size], [] (float* p) {
//        delete[] p; });
//    float* greyPtr = data.get();
//    u_char* colorPtr = colorData.get();
//    u_int offset = 0;
//
//    for (u_int i = 0; i < height; ++i) {
//        for (u_int j = 0; j < width; ++j) {
//            greyPtr[i * width + j] = colorPtr[offset++] * BLUE + colorPtr[offset++] * GREEN + colorPtr[offset++] * RED;
//            greyPtr[i * width + j] /= 255;
//        }
//        offset += padding;
//    }
//
//    greyData = data;
//}
//
//void ayc::BitMapImage::convertToBGR() {
//    u_int width = imageHeader -> width;
//    u_int height = imageHeader -> height;
//    u_int bytesPerPixel = (imageHeader -> bitPerPixel) / 8;
//    u_int padding = ((width * bytesPerPixel) % 4 == 0) ? 0 : 4 - ((width * bytesPerPixel) % 4);
//
//    u_int size = sizeof (u_char) * (height * (width + padding)) * bytesPerPixel;
//    std::shared_ptr<u_char> data(new u_char[size], [] (u_char * p) {
//        delete[] p; });
//    u_char* colorPtr = data.get();
//    float* greyPtr = greyData.get();
//
//    u_int offset = 0;
//
//    for (u_int i = 0; i < height; ++i) {
//        for (u_int j = 0; j < width; ++j) {
//            float fi = greyPtr[i * width + j] * 255;
//            u_char i = (u_char) fi;
//            colorPtr[offset++] = i;
//            colorPtr[offset++] = i;
//            colorPtr[offset++] = i;
//        }
//        offset += padding;
//    }
//
//    colorData = data;
//}

ayc::PixelStr ayc::BitMapImage::getPixel(unsigned int i, unsigned int j) const {
    PixelStr* ptr = colorData.get();
    return ptr[width * i + j];
}

void ayc::BitMapImage::setPixel(u_int i, u_int j, const ayc::PixelStr& color) {
    PixelStr* ptr = colorData.get();
    ptr[width * i + j] = color;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ayc::Image ayc::Image::create_image_from_bitmap(const std::string file_name) {
    ayc::Image result;
    ifstream file(file_name.c_str(), ios::in | ios::binary | ios::ate);
    if (file.is_open()) {
        ayc::HeaderStr header;
        file.seekg(0, ios::beg);
        //Read the header first
        file.read((char*) &header, sizeof (HeaderStr));

        result.width = header.width;
        result.height = header.height;

        //Put the cursor on the BMP data
        file.seekg(header.offset, ios::beg);
        int bytes_per_pixel = header.bit_per_pixel / 8;
        int padding = ((header.width * bytes_per_pixel) % 4 == 0) ? 0 : 4 - ((header.width * bytes_per_pixel) % 4);
        //Allocate the size needed to read the BMP data
        int size = sizeof (char)*(header.height * (header.width + padding)) * bytes_per_pixel;
        unsigned char* data = (unsigned char*) malloc(size);
        //Read the data
        file.read((char*) data, size);
        //Create the Accelerate::Bitmap object
        result.pixel_data = (PixelStr*) malloc(header.width * header.height * sizeof (PixelStr));
        unsigned int offset = 0;
        //In the Bitmap format, pixels are in a reversed order
        for (int i = header.height - 1; i >= 0; i--) {
            for (int j = 0; j < header.width; j++) {
                result.pixel_data[i * header.width + j].b = data[offset++];
                result.pixel_data[i * header.width + j].g = data[offset++];
                result.pixel_data[i * header.width + j].r = data[offset++];
            }
            offset += padding;
        }
        file.close();
        free(data);
    }
    return result;
}

ayc::Image::Image() {

}

ayc::Image::~Image() {
    free(pixel_data);
}

ayc::PixelStr ayc::Image::get_pixel(unsigned int i, unsigned int j) const {
    return this->pixel_data[this->width * i + j];
}


ayc::Image ayc::Image::scale_image(unsigned int scale) const {
    ayc::Image result;

    result.width = width * scale;
    result.height = height * scale;

    result.pixel_data = (PixelStr*) malloc(result.width * result.height * sizeof (PixelStr));

    for (unsigned int w = 0; w<this->width; w++) {
        for (unsigned int i = 0; i < scale; i++)
            copy_column(result, w, scale);
    }

    return result;
}

void ayc::Image::copy_column(ayc::Image& result, unsigned int column_number, unsigned int scale) const {
    //retrieve the column indice in the result image
    unsigned int first_column_indice = column_number * scale;
    for (unsigned int i = 0; i < scale; i++) {
        for (unsigned int row = 0; row < height; row++) {
            for (unsigned int j = 0; j < scale; j++) {
                result.pixel_data[(row * scale + j) * result.width + first_column_indice + i] = this->pixel_data[row * this->width + column_number];
            }
        }
    }
}

std::ostream& operator<<(std::ostream &o, const ayc::PixelStr& pixel) {
    return o << "[" << (int) pixel.r << ", " << (int) pixel.g << ", " << (int) pixel.b << "] ";
}

std::ostream& ayc::operator<<(std::ostream &o, ayc::BitMapImage& im) {
    ayc::PixelStr* pixels = im.getPixels();
    for (unsigned int i = 0; i < im.getHeight(); ++i) {
        for (unsigned int j = 0; j < im.getWidth(); ++j) {
            ayc::PixelStr pixel = pixels[i * im.getWidth() + j];
            o << "[" << (int) pixel.r << ", " << (int) pixel.g << ", " << (int) pixel.b << "] ";
        }
        o << std::endl;
    }

    return o;
}

std::ostream& operator<<(std::ostream &o, ayc::Image& im) {
    ayc::PixelStr* pixels = im.get_pixels();
    for (unsigned int i = 0; i < im.get_height(); i++) {
        for (unsigned int j = 0; j < im.get_width(); j++) {
            ayc::PixelStr pixel = pixels[i * im.get_width() + j];
            o << pixel << " ";
        }
        o << std::endl;
    }

    return o;
}
