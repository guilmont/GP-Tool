#pragma once

#include "header.h"

namespace GPT
{

    namespace Tiffer
    {
        // Basic sizes in bytes
        constexpr uint32_t
            BYTE = 1,
            ASCII = 2,
            SHORT = 3,
            LONG = 4,
            RATIONAL = 5,
            CLEAR_CODE = 256, // for lzw encoder-decoder
            EOI_CODE = 257,   // for lz encoder-decoder

            // Defining important tags for grayscale images
            IMAGEWIDTH = 256,
            IMAGEHEIGHT = 257,
            BITSPERSAMPLE = 258, // 8-bits -- 16-bits ...
            COMPRESSION = 259,
            PHOTOMETRIC = 262,
            DESCRIPTION = 270,
            STRIPOFFSETS = 273,    // In which byte the image begins
            SAMPLESPERPIXEL = 277, // 1 grayscale -- 3 rgb -- 4 rgba
            ROWSPERSTRIP = 278,
            STRIPBYTECOUNTS = 279,
            RESOLUTIONUNIT = 296,
            XRESOLUTION = 282,
            YRESOLUTION = 283,
            DATETIME = 306,       //When the image was created
            IJ_META_DATA = 50839; // registered id for imagej metadata

        using Buffer = std::vector<uint8_t>;
        using ImData = std::tuple<uint32_t, uint32_t, Buffer>;

        struct IFD
        {
            struct Tag
            {
                uint16_t type;
                uint32_t count, value;
            };

            uint16_t dir_count = 0;
            uint32_t offsetNext = 0;                 // Zero for last IFD
            std::unordered_map<uint16_t, Tag> field; // To store all the tags

            std::vector<Buffer> vData;
        };

        class Read
        {
        public:
            Read(const fs::path &movie_path);

            bool successful(void) const { return success; }

            const fs::path &getMoviePath(void) const { return movie_path; }
            uint32_t getNumDirectories() { return numDir; }
            uint32_t getBitCount(void);
            uint32_t getWidth(void);
            uint32_t getHeight(void);
            std::string getDateTime(void);
            std::string getMetadata(void);
            std::string getIJMetadata(void);

            template <typename T>
            Image<T> getImage(const uint32_t id = 0);

        private:
            fs::path movie_path;
            bool success = true;

            bool bigEndian = false; //  true = big | false = little
            bool lzw = false;       // if image was compressed used lzw algorithm

            uint32_t numDir = 0;
            Buffer buffer;

            std::vector<IFD> vIFD; // To organize the bytes into good information

            uint8_t get_uint8(const uint32_t pos);
            uint16_t get_uint16(const uint32_t pos);
            uint32_t get_uint32(const uint32_t pos);

            ImData getImageData(const uint32_t id);

        }; // class

        template <typename T>
        Image<T> Read::getImage(const uint32_t id)
        {
            if (id >= numDir)
            {
                pout("ERROR (GPT::Tiffer::Read::getImage) ==> Number of directories exceeded!");
                return Image<T>(0, 0);
            }

            auto [width, height, buf] = getImageData(id);

            size_t check = buf.size() / (width * height);
            if (check != sizeof(T))
            {
                pout("ERROR(GPT::Tiffer::Read::getImage) : Movie expects ", check, " bytes!!");
                return Image<T>(0, 0);
            }

            Image<T> img = Image<T>::Zero(height, width);

            T *data = reinterpret_cast<T *>(buf.data());
            size_t row = 0, col = 0, SZ = buf.size() / sizeof(T);

            for (size_t k = 0; k < SZ; k++)
            {
                T val = data[k];

                if (bigEndian)
                {
                    if (sizeof(T) == 1)
                        img(row, col++) = val;
                    else if (sizeof(T) == 2)
                        img(row, col++) = (val & 0x00FF) << 8 | (val & 0xFF00) >> 8;
                    else if (sizeof(T) == 4)
                    {
                        val = (val & 0x0000FFFF) << 16 | (val & 0xFFFF0000) >> 16;
                        img(row, col++) = (val & 0x00FF00FF) << 8 | (val & 0xFF00FF00) >> 8;
                    }
                }
                else
                    img(row, col++) = val;

                if (col == width)
                {
                    row++;
                    col = 0;
                }

            } //loop-buffer

            return img;
        } // getImage

    }
}