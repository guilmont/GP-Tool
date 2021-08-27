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
            EOI_CODE = 257,   // for lzw encoder-decoder

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
            Read(const fs::path& movie_path);

            bool successful(void) const { return success; }

            const fs::path& getMoviePath(void) const { return movie_path; }
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



        class Write
        {
        public:
            template <typename T>
            Write(const std::vector<Image<T>>& vImg, std::string metadata = "", bool lzw = false);

            GP_API void save(const fs::path &path);
            GP_API void funcLZW(const uint32_t tid, uint32_t nThreads, uint32_t nStrips, IFD* ifd);

        private:
            bool lzw = false;      // if the file lzw compressed
            std::vector<IFD> vIFD; // To organize the bytes into good information

            uint32_t globalOffset; // globa offset for guidance
            std::string metadata;  // holder for metadata

            template <typename A>
            void writeValue(Buffer* vOut, A val);


            template <typename T>
            void createTable(const std::vector<Image<T>>& vImg, std::string metadata, bool lzw);

        }; // class

    }

    /*******************************************************************************/
    /*******************************************************************************/
    // READ TEMPLATE IMPLEMENTATION

    template <typename T>
    Image<T> Tiffer::Read::getImage(const uint32_t id)
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

    /***********************************************************************************/
    /***********************************************************************************/
    // WRITE TEMPLATE IMPLEMENTATION


    template <typename T>
    Tiffer::Write::Write(const std::vector<Image<T>>& vImg, std::string metadata, bool lzw) { createTable(vImg, metadata, lzw); }

    template <typename A>
    void Tiffer::Write::writeValue(Buffer* vOut, A val)
    {
        for (int k = 0; k < sizeof(A); k++)
        {
            uint8_t oi = val >> (8 * k) & 0xff; // little endian
            vOut->push_back(oi);
        }
    }

    template <typename T>
    void Tiffer::Write::createTable(const std::vector<Image<T>>& vImg, std::string metadata, bool lzw)
    {
        const uint32_t numIFD = uint32_t(vImg.size());

        this->lzw = lzw;           // checking if to be compressed
        this->metadata = metadata; // in case we need to insert metadata

        globalOffset = 8;
        vIFD.resize(numIFD);

        for (uint32_t ct = 0; ct < numIFD; ct++)
        {
            IFD ifd;

            // ImageWidth -- ImageHeight
            uint32_t
                width = static_cast<uint32_t>(vImg.at(ct).cols()),
                height = static_cast<uint32_t>(vImg.at(ct).rows());

            uint32_t RPS = 32;
            while (height % RPS != 0 || RPS != 1)
                RPS = RPS >> 1;

            uint32_t nStrip = (uint32_t)ceil(float(height) / float(RPS));

            ifd.field[IMAGEWIDTH] = { SHORT, 1, width };
            ifd.field[IMAGEHEIGHT] = { SHORT, 1, height };

            //BitsPerSample
            uint16_t bits = 8 * sizeof(T);
            ifd.field[BITSPERSAMPLE] = { SHORT, 1, bits };

            // SamplesPerPixel
            ifd.field[SAMPLESPERPIXEL] = { SHORT, 1, 1 };

            // Compress
            ifd.field[COMPRESSION] = { SHORT, 1, uint16_t(lzw ? 5 : 1) };

            // PhotometricInterpretation
            ifd.field[PHOTOMETRIC] = { SHORT, 1, 1 };

            // StripOffsets
            ifd.field[STRIPOFFSETS] = { LONG, nStrip, 0 }; // offset comes later

            // RowsperStrip -- by default 32 row per strip, but can drop to 1 if needed
            ifd.field[ROWSPERSTRIP] = { SHORT, RPS, 1 };

            // StripByteCount
            ifd.field[STRIPBYTECOUNTS] = { LONG, nStrip, 0 }; // value comes later

            // Metadata -- comes only at first ifd if exists
            if (ct == 0 && metadata.size() > 0)
                ifd.field[DESCRIPTION] = { ASCII, uint32_t(metadata.size()), 0 }; // offset later

            ///////////////////////////
            // Handling images
            const Image<T>& img = vImg.at(ct);
            ifd.vData.resize(nStrip);

            for (uint32_t h = 0; h < nStrip; h++)
            {
                uint32_t nCols = h * RPS;
                nCols = (nCols + RPS > height ? (height - nCols) : RPS);

                uint32_t size = nCols * width,
                    offset = RPS * width * h;

                ifd.vData.at(h).resize(size * sizeof(T));
                memcpy(ifd.vData.at(h).data(), img.data() + uint64_t(offset), size * sizeof(T));
            }

            if (lzw) // if we need to compress image
            {
                const uint32_t nThreads = std::thread::hardware_concurrency();
                std::vector<std::thread> vThr(nThreads);
                for (uint32_t k = 0; k < nThreads; k++)
                    vThr[k] = std::thread(&Write::funcLZW, this, k, nThreads, nStrip, &ifd);

                for (std::thread& thr : vThr)
                    thr.join();

            } // if-compressed

            ///////////////////////////
            // Final setup
            ifd.dir_count = uint32_t(ifd.field.size());
            globalOffset += 12 * uint32_t(ifd.field.size()) + 6;

            ifd.offsetNext = globalOffset;

            vIFD.at(ct) = ifd;

        } // loop-ifd

        // last ifd has offset zero
        vIFD.at(numIFD - 1).offsetNext = 0x0000;

        // Handling metadata
        if (metadata.size() > 0)
        {
            vIFD.at(0).field[DESCRIPTION].value = globalOffset;
            globalOffset += uint32_t(metadata.size());
        }

    } 

}