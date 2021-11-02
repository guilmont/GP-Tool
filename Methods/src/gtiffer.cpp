#include "gtiffer.h"

#include <fstream>

namespace GPT::Tiffer
{
    static Buffer lzw_decoder(const uint8_t *vInput, const uint64_t SZ)
    {
        uint8_t B = 9;
        uint32_t bCounter = 0;
        uint16_t oldCode = 0, code = 0;

            // Creating table
        std::vector<std::string> table(258);
        table.reserve(5012);

        for (uint32_t k = 0; k < 256; k++)
            table[k] = std::string(1, char(k));

        table[256] = "start";
        table[257] = "EOI";

        // lambda functions
        auto make_number = [&](uint32_t B) -> uint16_t {
           uint64_t 
               bit = bCounter % 8,
               id = bCounter >> 3;

           uint8_t aux = vInput[id];

           uint16_t val = 0;
           for (int32_t k = B - 1; k >= 0; k--)
           {
               val |= ((aux >> (7 - bit)) & 0x01) << k;

               if (++bit == 8)
               {
                   aux = vInput[++id];
                   bit = 0;
               }
           }
           
            bCounter += B;
            return val;
        };

        auto clear_table = [&](void) -> void {
            B = 9;
            table.resize(258);
        };

        /////////////////////////////////////////////////
        // loop over all bits
        Buffer arr;
        clear_table();

        while (true)
        {
            code = make_number(B);
            if (code == EOI_CODE)
                break;

            if (code == CLEAR_CODE)
            {
                clear_table();

                code = make_number(B);
                if (code == EOI_CODE)
                    break;

                for (char oi : table[code])
                    arr.push_back(oi);
            }
            else
            {
                // is code in table
                if (code < table.size())
                {
                    for (uint8_t oi : table[code])
                        arr.push_back(oi);

                    table.push_back(table[oldCode] + table[code].at(0));
                }
                else //not in table
                {
                    std::string outStr = table[oldCode] + table[oldCode].at(0);

                    for (uint8_t oi : outStr)
                        arr.push_back(oi);

                    table.push_back(outStr);
                }

            } // else - clear_code

            uint16_t s = uint16_t(table.size());
            if (s == 511 || s == 1023 || s == 2047)
                B++;

            oldCode = code;

        } // while-true

        return arr;

    } // decoder

    static Buffer lzw_encoder(Buffer vInput)
    {

        uint8_t B;
        uint16_t code;
        std::string Omega, K;
        std::unordered_map<std::string, uint16_t> table;
        std::vector<std::pair<uint8_t, uint16_t>> encoded;

        auto clear_table = [&](void) -> void
        {
            std::string ch;
            B = 9;
            code = 258;
            table.clear();
            for (uint16_t k = 0; k < 256; k++)
            {
                ch = char(k);
                table[ch] = k;
            }
        };

        // starting initial sequence
        clear_table();
        Omega = "";
        encoded.push_back({ B, CLEAR_CODE });

        for (uint32_t id = 0; id < vInput.size(); id++)
        {
            K = char(vInput.at(id));

            // if present in table
            if (table.find(Omega + K) != table.end())
            {
                Omega = Omega + K;
            }
            else
            {
                encoded.push_back({ B, table[Omega] });
                table[Omega + K] = code;
                Omega = K;
                code++;

                if (code == 512 || code == 1024 || code == 2048)
                    B++;

                if (code == 4094)
                {
                    encoded.push_back({ 12, CLEAR_CODE });
                    clear_table();
                }
            }

        } // loop-numbers

        encoded.push_back({ B, table[Omega] }); // Omega must be in the table
        code++;
        if (code == 512 || code == 1024 || code == 2048)
            B++;

        encoded.push_back({ B, EOI_CODE });

        //  converting to binary --> little endian by default
        std::vector<bool> vec;
        for (auto num : encoded)
            for (int k = num.first - 1; k >= 0; k--)
                vec.push_back(num.second >> k & 0x01);

        // padding end for multiple of 8
        uint32_t dif = 8 * uint32_t(ceil(float(vec.size()) / 8.0f)) - uint32_t(vec.size());
        for (uint32_t l = 0; l < dif; l++)
            vec.push_back(true);

        // Creating bytes array
        Buffer arr;
        for (uint32_t k = 0; k < vec.size(); k += 8)
        {
            uint8_t val = 0;
            for (uint32_t l = 0; l < 8; l++)
                val |= vec.at(k + l) << (7 - l); // little endian

            arr.push_back(val);
        } // loop-bytes

        return arr;
    }

    uint8_t Read::get_uint8(const uint32_t pos) { return buffer.at(pos); }

    uint16_t Read::get_uint16(const uint32_t pos)
    {
        uint16_t val = reinterpret_cast<uint16_t *>(buffer.data() + pos)[0];

        if (bigEndian)
            return (val << 8) | (val >> 8);
        else
            return val;

    }

    uint32_t Read::get_uint32(const uint32_t pos)
    {
        uint32_t val = reinterpret_cast<uint32_t *>(buffer.data() + pos)[0];

        if (bigEndian)
        {
            val = (val & 0x0000FFFF) << 16 | (val & 0xFFFF0000) >> 16;
            return (val & 0x00FF00FF) << 8 | (val & 0xFF00FF00) >> 8;
        }
        else
            return val;

    }

}

/***************************************************************************************/
/***************************************************************************************/
// READ API IMPLEMENTATION

GPT::Tiffer::Read::Read(const fs::path &movie_path) : movie_path(movie_path)
{
    // Reading binary data
    std::ifstream arq(movie_path, std::ios::binary);

    if (arq.fail())
    {
        success = false;
        pout("ERROR (GPT::Tiffer::Read) ==> Cannot read file:", movie_path);
        return;
    }

    arq.seekg(0, std::ios::end);
    size_t len = size_t(arq.tellg());
    buffer.resize(len);
    arq.seekg(0, std::ios::beg);
    arq.read((char *)buffer.data(), len);
    arq.close();

    // Little or big endian
    this->bigEndian = (get_uint8(0) == 'I' ? false : true);

    // Is it tiff?
    if (get_uint16(2) != 42)
    {
        success = false;
        pout("ERROR (GPT::Tiffer::Read) ==> Not tiff file:", movie_path);
        return;
    }

    // Let's check where the first IFD begins
    uint32_t offset = get_uint32(4);

    // Reading all IFDs
    while (offset != 0)
    {
        IFD ifd;

        // Get number of tags
        ifd.dir_count = get_uint16(offset);

        // Running tags
        for (uint32_t k = 0; k < ifd.dir_count; k++)
        {
            uint32_t ct = offset + 12 * k + 2; // 12 bytes from tiff + 2 bytes from count

            uint16_t tag = get_uint16(ct),
                     type = get_uint16(ct + 2);

            if (type > RATIONAL)
            {
                success = false;
                pout("ERROR (GPT::Tiffer::Read) ==> Tiff file might be corrupted:", movie_path);
                return;
            }

            uint32_t count = get_uint32(ct + 4);

            uint32_t value;
            if (type == 3 && count == 1)
                value = get_uint16(ct + 8);
            else
                value = get_uint32(ct + 8);

            // Some basic check
            if (tag == COMPRESSION) // Image is compressed somehow
            {
                if (value == 1)
                    this->lzw = false;
                else if (value == 5)
                    this->lzw = true;
                else
                {
                    success = false;
                    pout("ERROR (GPT::Tiffer::Read) ==> Compression format is not supported! ::", movie_path);
                    return;
                }

            } // compression

            if (tag == SAMPLESPERPIXEL && value != 1)
            {
                success = false;
                pout("ERROR (GPT::Tiffer::Read::load) ==> Only grayscale format is supported! ::", movie_path);
                return;
            }

            if (tag == BITSPERSAMPLE)
            {
                if (count > 1)
                {
                    if (type == 1)
                        value = get_uint8(value);
                    else if (type == 3)
                        value = get_uint16(value);
                    else if (type == 4)
                        value = get_uint32(value);
                    else
                        value = -1;
                }

                if (value < 8 || value > 32)
                {
                    success = false;
                    pout("ERROR (GPT::Tiffer::Read::load) ==> Only 8/16/32 bits grayscale images are accepted! ::", movie_path);
                    return;
                }

            } // bitsPerSample

            ifd.field[tag] = {type, count, value};

        } // loop tags

        // Searching more directories
        uint32_t next = offset + 12 * ifd.dir_count + 2;
        offset = get_uint32(next);

        // Append this directory to class
        ifd.offsetNext = offset;
        vIFD.push_back(ifd);

    } // while - next IFD

    // to simplify verifications later
    this->numDir = uint32_t(vIFD.size());

} // constructor

uint32_t GPT::Tiffer::Read::getBitCount(void) { return vIFD.at(0).field[BITSPERSAMPLE].value; }
uint32_t GPT::Tiffer::Read::getWidth(void) { return vIFD.at(0).field[IMAGEWIDTH].value; }
uint32_t GPT::Tiffer::Read::getHeight(void) { return vIFD.at(0).field[IMAGEHEIGHT].value; }

std::string GPT::Tiffer::Read::getDateTime(void)
{
    if (vIFD.at(0).field.find(DATETIME) == vIFD.at(0).field.end())
    {
        pout("WARN (GPT::Tiffer::Read::getDateTime) ==> Movie doesn't contain a time stamp ::", movie_path);
        return "";
    }
    else
    {
        uint32_t count = vIFD.at(0).field[DATETIME].count;
        uint32_t pos = vIFD.at(0).field[DATETIME].value;
        std::string out((char *)buffer.data() + pos, count);
        return out;
    }
} // getDateTime

std::string GPT::Tiffer::Read::getMetadata(void)
{
    auto it = vIFD.at(0).field.find(DESCRIPTION);

    if (it == vIFD.at(0).field.end())
        return "";

    uint32_t count = it->second.count;
    uint32_t pos = it->second.value;

    std::string out((char *)buffer.data() + pos, count);

    return out;
} // getMetadata

std::string GPT::Tiffer::Read::getIJMetadata(void)
{
    auto it = vIFD[0].field.find(IJ_META_DATA);

    if (it == vIFD[0].field.end())
        return "";

    uint32_t count = it->second.count;
    uint32_t pos = it->second.value;

    std::string out; // imagej metada has utf16 format

    for (size_t k = 0; k < count; k++)
        if (buffer[pos + k] != 0)
            out += (char)buffer[pos + k];

    return out;
} // getIJMetadata

GPT::Tiffer::ImData GPT::Tiffer::Read::getImageData(const uint32_t id)
{
    // pointer to directory
    auto &dir = vIFD.at(id).field;

    uint32_t count = dir[STRIPOFFSETS].count;
    std::vector<std::pair<uint32_t, size_t>> vStrip(count);

    // Setting up strips to read
    if (count == 1)
    {
        vStrip.at(0) = {dir[STRIPOFFSETS].value,
                        dir[STRIPBYTECOUNTS].value};
    }
    else
    {
        uint32_t start = dir[STRIPOFFSETS].value,
                 type_start = dir[STRIPOFFSETS].type,
                 size = dir[STRIPBYTECOUNTS].value,
                 type_size = dir[STRIPBYTECOUNTS].type;

        for (uint32_t k = 0; k < count; k++)
        {
            uint32_t a1, a2;
            if (type_start == 4)
                a1 = get_uint32(start + 4 * k);
            else
                a1 = get_uint16(start + 2 * k);

            if (type_size == 4)
                a2 = get_uint32(size + 4 * k);
            else
                a2 = get_uint16(size + 2 * k);

            vStrip.at(k) = {a1, a2};
        }
    } // else

    // Creating image
    uint32_t width = dir[IMAGEWIDTH].value,
             height = dir[IMAGEHEIGHT].value,
             bytes = dir[BITSPERSAMPLE].value >> 3;

    Buffer output;
    output.reserve(uint64_t(width) * uint64_t(height) * uint64_t(bytes));

    for (auto &var : vStrip)
    {
        uint8_t *loc = buffer.data() + var.first;

        // run decoding if needed
        if (lzw)
        {
            Buffer aux = lzw_decoder(loc, var.second);
            var.second = aux.size(); // size changes after decoding, so we update

            std::move(aux.begin(), aux.end(), std::back_inserter(output));
        }
        else
            output.insert(output.end(), loc, loc + var.second);

    } // loop-over-strips

    return {width, height, output};

} // getImageData


/***************************************************************************************/
/***************************************************************************************/
// WRITE API IMPLEMENTATION

void GPT::Tiffer::Write::funcLZW(const uint32_t tid, const uint32_t nThreads, uint32_t nStrips, IFD* ifd)
{
    for (uint32_t k = tid; k < nStrips; k += nThreads)
    {
        Buffer hi = lzw_encoder(ifd->vData.at(k));
        ifd->vData.at(k).resize(hi.size());
        std::copy(hi.data(), hi.data() + hi.size(), ifd->vData.at(k).data());
    }
}

void GPT::Tiffer::Write::save(const fs::path& filename)
{
    // initialize specifying file is in little endian notation, it is a tif file
    // and the offset to first ifd is 8
    Buffer vOut = { 'I', 'I', 0x2A, 0x00, 0x08, 0x00, 0x00, 0x00 };

    for (IFD& ifd : vIFD)
    {
        ifd.field[STRIPOFFSETS].value = globalOffset;
        globalOffset += uint32_t(4 * ifd.vData.size());
        ifd.field[STRIPBYTECOUNTS].value = globalOffset;
        globalOffset += uint32_t(4 * ifd.vData.size());

        writeValue(&vOut, uint16_t(ifd.dir_count));

        for (auto it = ifd.field.begin(); it != ifd.field.end(); it++)
        {
            writeValue(&vOut, uint16_t(it->first));
            writeValue(&vOut, uint16_t(it->second.type));
            writeValue(&vOut, uint32_t(it->second.count));
            writeValue(&vOut, uint32_t(it->second.value));
        }

        writeValue(&vOut, uint32_t(ifd.offsetNext));
    } // loop -- ifd

    // wrting metadata
    for (uint32_t k = 0; k < metadata.size(); k++)
        vOut.push_back(metadata[k]);

    // writing location of images
    for (IFD& ifd : vIFD)
    {
        for (auto v : ifd.vData)
        {
            writeValue(&vOut, uint32_t(globalOffset));
            globalOffset += uint32_t(v.size());
        }

        for (auto v : ifd.vData)
            writeValue(&vOut, uint32_t(v.size()));
    }

    // writting strips accordingly
    for (IFD& ifd : vIFD)
        for (Buffer v : ifd.vData)
            vOut.insert(vOut.end(), v.begin(), v.end());

    std::ofstream outfile(filename, std::ios::binary);
    outfile.write((const char*)vOut.data(), vOut.size());
    outfile.close();

}