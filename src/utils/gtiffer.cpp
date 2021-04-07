#include "utils/gtiffer.h"

namespace Tiffer
{
    static Buffer lzw_decoder(const uint8_t *vInput, const size_t SZ)
    {
        uint8_t B = 9;
        uint32_t bCounter = 0;
        uint16_t oldCode = 0, code = 0;

        // Convert buffer to bool array
        std::vector<bool> vBool;
        vBool.reserve(8 * SZ);

        for (size_t k = 0; k < SZ; k++)
        {
            uint8_t val = vInput[k];
            for (int l = 7; l >= 0; l--)
                vBool.emplace_back((val >> l) & 0x01);
        }

        // Creating table
        std::vector<std::string> table(258);
        for (uint32_t k = 0; k < 256; k++)
            table[k] = std::string(1, char(k));

        table[256] = "start";
        table[257] = "EOI";

        // lambda functions
        auto make_number = [&](uint32_t B) -> uint16_t {
            uint16_t val = 0;
            for (uint32_t k = 0; k < B; k++)
                if (vBool[bCounter + k])
                    val |= 1 << (B - k - 1);

            bCounter += B;
            return val;
        };

        auto clear_table = [&](void) -> void {
            B = 9;
            table.resize(258); };

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

            uint16_t s = table.size();
            if (s == 511 || s == 1023 || s == 2047)
                B++;

            oldCode = code;

            std::fflush(stdout);
        } // while-true

        return arr;

    } // decoder

    uint8_t Read::get_uint8(const uint32_t pos)
    {
        return buffer.at(pos);
    } // get_uint8

    uint16_t Read::get_uint16(const uint32_t pos)
    {
        uint16_t val = reinterpret_cast<uint16_t *>(buffer.data() + pos)[0];

        if (bigEndian)
            return __builtin_bswap16(val);
        else
            return val;

    } // get_uint16

    uint32_t Read::get_uint32(const uint32_t pos)
    {
        uint32_t val = reinterpret_cast<uint32_t *>(buffer.data() + pos)[0];

        if (bigEndian)
            return __builtin_bswap32(val);
        else
            return val;

    } // get_uint32

}; // namespace Tiffer

Tiffer::Read::Read(const std::string &movie_path, Mailbox *mail)
    : movie_path(movie_path), mbox(mail)
{
    // Reading binary data
    std::ifstream arq(movie_path, std::ios::binary);

    if (arq.fail())
    {
        success = false;
        if (mbox)
            mbox->create<Message::Error>("Cannot open file: " + movie_path);
        else
            std::cerr << "ERROR (Tiffer::Read) ==> Cannot read file: " << movie_path << std::endl;

        return;
    }

    arq.seekg(0, std::ios::end);
    size_t len = arq.tellg();
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
        if (mbox)
            mbox->create<Message::Error>("Not tiff file: " + movie_path);
        else
            std::cerr << "ERROR (Tiffer::Read) ==> Not tiff file: " + movie_path << std::endl;

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
                    if (mbox)
                        mbox->create<Message::Error>("Compression format is not supported!");
                    else
                        std::cerr << "ERROR (Tiffer::Read): Compression format is not supported! - "
                                  << movie_path << std::endl;

                    return;
                }
            } // compression

            if (tag == SAMPLESPERPIXEL && value != 1)
            {
                success = false;
                if (mbox)
                    mbox->create<Message::Error>("Only grayscale format is supported!");
                else
                    std::cerr << "ERROR (Tiffer::Read::load) ==> "
                              << "Only grayscale format is supported! - "
                              << movie_path << std::endl;

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
                    if (mbox)
                        mbox->create<Message::Error>("Only 8/16/32 bits grayscale "
                                                     "images are accepted!");
                    else
                        std::cerr << "ERROR (Tiffer::Read::load) ==> "
                                  << "Only 8/16/32 bits grayscale images are accepted! - "
                                  << movie_path << std::endl;
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
    this->numDir = vIFD.size();

} // constructor

uint32_t Tiffer::Read::getBitCount(void) { return vIFD.at(0).field[BITSPERSAMPLE].value; }
uint32_t Tiffer::Read::getWidth(void) { return vIFD.at(0).field[IMAGEWIDTH].value; }
uint32_t Tiffer::Read::getHeight(void) { return vIFD.at(0).field[IMAGEHEIGHT].value; }

std::string Tiffer::Read::getDateTime(void)
{
    if (vIFD.at(0).field.find(DATETIME) == vIFD.at(0).field.end())
    {
        if (mbox)
            mbox->create<Message::Warn>("Movie doesn't contain a time stamp!");
        else
            std::cout << "WARN (Tiffer::Read::getDateTime): Movie doesn't contain a time stamp - "
                      << movie_path << std::endl;

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

std::string Tiffer::Read::getMetadata(void)
{
    auto it = vIFD.at(0).field.find(DESCRIPTION);

    if (it == vIFD.at(0).field.end())
        return "";

    uint32_t count = it->second.count;
    uint32_t pos = it->second.value;

    std::string out((char *)buffer.data() + pos, count);

    return out;
} // getMetadata

std::string Tiffer::Read::getIJMetadata(void)
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

Tiffer::ImData Tiffer::Read::getImageData(const uint32_t id)
{
    // pointer to directory
    auto &dir = vIFD.at(id).field;

    uint32_t count = dir[STRIPOFFSETS].count;
    std::vector<std::pair<uint32_t, uint32_t>> vStrip(count);

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
             height = dir[IMAGEHEIGHT].value;

    Buffer output;

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

// void Tiffer::Read::printAll(void)
// {
//     std::cout << "Types: 1 - byte :: 2 - ASCII ::  3 - SHORT :: 4 - LONG :: 5 - RATIONAL\n\n";

//     for (auto &[val, tag] : vIFD[0].field)
//     {
//         std::cout << "ID: " << val
//                   << " :: Byte count: " << tag.count
//                   << " :: Type: " << tag.type << std::endl;

//         if (tag.type == ASCII || tag.type == BYTE)
//         {
//             uint32_t count = tag.count;
//             uint32_t pos = tag.value;

//             std::string out((char *)buffer.data() + pos, count);
//             std::cout << out << std::endl;
//         }
//         else if (tag.type == RATIONAL)
//         {
//             uint32_t *loc = reinterpret_cast<uint32_t *>(buffer.data() + tag.value);

//             std::cout << float(loc[0] / loc[1]) << std::endl;
//         }
//         else
//         {
//             if (tag.count == 1)
//                 std::cout << tag.value << std::endl;
//             else
//             {
//                 uint32_t *loc = reinterpret_cast<uint32_t *>(buffer.data() + tag.value);

//                 for (uint32_t k = 0; k < tag.count; k++)
//                     std::cout << "  -> " << loc[k] << std::endl;
//             }
//         }

//         std::cout << "\n-----------------\n\n"
//                   << std::endl;
//     }
// }