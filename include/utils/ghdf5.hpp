#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <cstring>

#include <H5Cpp.h>

using String = std::string;

struct Shape
{
    size_t x, y;
};

enum TypeID : uint32_t
{
    G5_GROUP,
    G5_BOOL,
    G5_UINT8,
    G5_UINT16,
    G5_UINT32,
    G5_CHAR,
    G5_INT,
    G5_FLOAT,
    G5_DOUBLE,
    G5_STRING,
    G5_ARRAY_BOOL,
    G5_ARRAY_UINT8,
    G5_ARRAY_UINT16,
    G5_ARRAY_UINT32,
    G5_ARRAY_INT,
    G5_ARRAY_FLOAT,
    G5_ARRAY_DOUBLE
};

template <typename T>
using GHDF5_ptr = std::tuple<T *, size_t, size_t>;

class GHDF5
{
public:
    GHDF5(String name = "root") : nameID(name) {}

    // utilities
    uint32_t getType(void) const { return typeID; }
    size_t getSize(void) const { return shape.x * shape.y; }

    const String &getName(void) const { return nameID; }
    const Shape &getShape(void) const { return shape; }

    void reshape(size_t width, size_t height) { shape = {width, height}; }
    void save(const String &filename);
    void open(const String &filename);

    bool contains(const String &name) const;

    // access element
    GHDF5 &operator[](const String &name);
    GHDF5 &operator[](const uint32_t index);
    const GHDF5 &operator[](const String &name) const;
    const GHDF5 &operator[](const uint32_t index) const;

    // assignment operator
    template <typename T>
    void operator=(T value);

    template <typename T>
    void operator=(const GHDF5_ptr<T> ptrDim);

    template <typename T>
    void createFromPointer(const T *ptr, size_t width, size_t height = 1);

    // retrieving data
    template <typename T>
    T getValue(void) const;

    template <typename T>
    std::vector<T> getArray(void) const;

    template <typename T, typename SP>
    SP getTemplatedMatrix(void) const;

    const uint8_t *getRawPointer(void) const { return buffer.data(); }

    // attributes management
    template <typename T>
    void setAttribute(const String &name, T attrib);

    template <typename T>
    T getAttribute(const String &name) const;

    // iterators
    std::map<String, GHDF5> &children(void) { return objs; }
    std::map<String, GHDF5> &attributes(void) { return attribs; }

    const std::map<String, GHDF5> &children(void) const { return objs; }
    const std::map<String, GHDF5> &attributes(void) const { return attribs; }

private:
    String nameID;

    uint32_t typeID = G5_GROUP;
    size_t nBytes = 1;
    Shape shape = {1, 1};

    std::map<String, GHDF5> objs;
    std::map<String, GHDF5> attribs;
    std::vector<uint8_t> buffer;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void g_assert(bool trigger, const char *msg)
{
    if (!trigger)
    {
        std::cerr << msg << std::endl;
        exit(-1);
    }
}

GHDF5 &GHDF5::operator[](const String &name)
{
    if (objs.find(name) == objs.end())
    {
        objs[name] = GHDF5(name);
        return objs.at(name);
    }

    else
        return objs.at(name);

} // operator[]

const GHDF5 &GHDF5::operator[](const String &name) const
{
    if (objs.find(name) == objs.end())
    {
        std::cout << "(GHDF5::operator[]): Doesn't contain '" + name + "'!!\n";
        exit(-1);
    }

    else
        return objs.at(name);

} // operator[]

GHDF5 &GHDF5::operator[](const uint32_t index)
{
    String name = std::to_string(index);
    if (objs.find(name) == objs.end())
    {
        objs[name] = GHDF5(name);
        return objs.at(name);
    }

    else
        return objs.at(name);

} // operator[]

const GHDF5 &GHDF5::operator[](const uint32_t index) const
{
    String name = std::to_string(index);
    if (objs.find(name) == objs.end())
    {
        std::cout << "(GHDF5::operator[]): Doesn't contain '" + name + "'!!\n";
        exit(-1);
    }

    else
        return objs.at(name);

} // operator[]

template <typename T>
void GHDF5::operator=(T value)
{
    nBytes = sizeof(T);
    shape = {1, 1};

    uint8_t *ptr = reinterpret_cast<uint8_t *>(&value);
    buffer.resize(nBytes);
    memcpy(buffer.data(), ptr, nBytes);

    if (typeid(T) == typeid(bool))
        typeID = G5_BOOL;
    else if (typeid(T) == typeid(uint8_t))
        typeID = G5_UINT8;
    else if (typeid(T) == typeid(uint16_t))
        typeID = G5_UINT16;
    else if (typeid(T) == typeid(uint32_t))
        typeID = G5_UINT32;
    else if (typeid(T) == typeid(char))
        typeID = G5_CHAR;
    else if (typeid(T) == typeid(int))
        typeID = G5_INT;
    else if (typeid(T) == typeid(float))
        typeID = G5_FLOAT;
    else if (typeid(T) == typeid(double))
        typeID = G5_DOUBLE;

    else
        g_assert(false, "ERROR: GHDF5 doesn't support this format!");
}

template <>
void GHDF5::operator=(String value)
{
    nBytes = value.size();
    shape = {value.size(), 1};
    typeID = G5_STRING;

    buffer.resize(nBytes);
    memcpy(buffer.data(), value.c_str(), nBytes);
}

template <>
void GHDF5::operator=(const char *value)
{
    nBytes = strlen(value);
    shape = {nBytes, 1};
    typeID = G5_STRING;

    buffer.resize(nBytes);
    memcpy(buffer.data(), value, nBytes);
}

template <typename T>
void GHDF5::operator=(const GHDF5_ptr<T> ptrDim)
{
    auto &[ptr, width, height] = ptrDim;
    createFromPointer(ptr, width, height);
}

template <typename T>
void GHDF5::createFromPointer(const T *ptr, size_t width, size_t height)
{
    nBytes = width * height * sizeof(T);
    shape = {width, height};

    const uint8_t *loc = reinterpret_cast<const uint8_t *>(ptr);
    buffer.insert(buffer.end(), loc, loc + nBytes);

    if (typeid(T) == typeid(bool))
        typeID = G5_ARRAY_BOOL;
    else if (typeid(T) == typeid(uint8_t))
        typeID = G5_ARRAY_UINT8;
    else if (typeid(T) == typeid(uint16_t))
        typeID = G5_ARRAY_UINT16;
    else if (typeid(T) == typeid(uint32_t))
        typeID = G5_ARRAY_UINT32;
    else if (typeid(T) == typeid(char))
        typeID = G5_ARRAY_INT;
    else if (typeid(T) == typeid(float))
        typeID = G5_ARRAY_FLOAT;
    else if (typeid(T) == typeid(double))
        typeID = G5_ARRAY_DOUBLE;
    else
        g_assert(false, "ERROR: GHDF5 doesn't support this format!");
} // createFromPointer

///////////////////////////////////////////////////////////////////////////////

bool GHDF5::contains(const String &name) const
{
    for (auto &[tag, obj] : objs)
        if (tag.compare(name) == 0)
            return true;

    return false;
} // contains

template <typename T>
T GHDF5::getValue(void) const
{
    uint32_t loc;
    if (typeid(T) == typeid(bool))
        loc = G5_BOOL;
    else if (typeid(T) == typeid(uint8_t))
        loc = G5_UINT8;
    else if (typeid(T) == typeid(uint16_t))
        loc = G5_UINT16;
    else if (typeid(T) == typeid(uint32_t))
        loc = G5_UINT32;
    else if (typeid(T) == typeid(int))
        loc = G5_INT;
    else if (typeid(T) == typeid(float))
        loc = G5_FLOAT;
    else if (typeid(T) == typeid(double))
        loc = G5_DOUBLE;

    g_assert(loc == typeID, "ERROR (getValue): Requested typeID is different from obj!");

    T value;
    memcpy(&value, buffer.data(), nBytes);
    return value;
}

template <>
String GHDF5::getValue(void) const
{
    g_assert(G5_STRING == typeID, "ERROR (getValue): Expected string typeID!");

    String value((char *)buffer.data(), nBytes);
    return value;
}

template <typename T>
std::vector<T> GHDF5::getArray(void) const
{
    uint32_t loc;
    if (typeid(T) == typeid(bool))
        loc = G5_ARRAY_BOOL;
    else if (typeid(T) == typeid(uint8_t))
        loc = G5_ARRAY_UINT8;
    else if (typeid(T) == typeid(uint16_t))
        loc = G5_ARRAY_UINT16;
    else if (typeid(T) == typeid(uint32_t))
        loc = G5_ARRAY_UINT32;
    else if (typeid(T) == typeid(int))
        loc = G5_ARRAY_INT;
    else if (typeid(T) == typeid(float))
        loc = G5_ARRAY_FLOAT;
    else if (typeid(T) == typeid(double))
        loc = G5_ARRAY_DOUBLE;

    g_assert(loc == typeID, "ERROR (getValue): Requested typeID is different from obj!");

    std::vector<T> vec(shape.x);
    memcpy(vec.data(), buffer.data(), nBytes);
    return vec;
} // getArray

template <typename T, typename SP>
SP GHDF5::getTemplatedMatrix(void) const
{
    uint32_t loc;
    if (typeid(T) == typeid(bool))
        loc = G5_ARRAY_BOOL;
    else if (typeid(T) == typeid(uint8_t))
        loc = G5_ARRAY_UINT8;
    else if (typeid(T) == typeid(uint16_t))
        loc = G5_ARRAY_UINT16;
    else if (typeid(T) == typeid(uint32_t))
        loc = G5_ARRAY_UINT32;
    else if (typeid(T) == typeid(int))
        loc = G5_ARRAY_INT;
    else if (typeid(T) == typeid(float))
        loc = G5_ARRAY_FLOAT;
    else if (typeid(T) == typeid(double))
        loc = G5_ARRAY_DOUBLE;

    g_assert(loc == typeID, "ERROR (getValue): Requested typeID is different from obj!");

    SP mat(shape.y, shape.x);
    memcpy(mat.data(), buffer.data(), nBytes);

    return mat;
} // getTemplatedMatrix

///////////////////////////////////////////////////////////////////////////////

template <typename T>
void GHDF5::setAttribute(const String &name, T attrib)
{
    if (attribs.find(name) == attribs.end())
        attribs[name] = GHDF5(name);

    attribs[name] = attrib;
} // setAttribute

template <typename T>
T GHDF5::getAttribute(const String &name) const
{
    return attribs.at(name).getValue<T>();
} // getAttribute

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <typename T>
static H5::DataType getDataType(void)
{
    if (typeid(T) == typeid(bool))
        return H5::DataType(H5::PredType::NATIVE_UINT8);

    else if (typeid(T) == typeid(char))
        return H5::DataType(H5::PredType::NATIVE_CHAR);

    else if (typeid(T) == typeid(uint8_t))
        return H5::DataType(H5::PredType::NATIVE_UINT8);

    else if (typeid(T) == typeid(uint16_t))
        return H5::DataType(H5::PredType::NATIVE_UINT16);

    else if (typeid(T) == typeid(uint32_t))
        return H5::DataType(H5::PredType::NATIVE_UINT32);

    else if (typeid(T) == typeid(int))
        return H5::DataType(H5::PredType::NATIVE_INT32);

    else if (typeid(T) == typeid(float))
        return H5::DataType(H5::PredType::NATIVE_FLOAT);

    else
        return H5::DataType(H5::PredType::NATIVE_DOUBLE);

} // getDataType

template <typename T, class CL>
static void writeAttrib(const CL &obj, const std::string &path, const T &value)
{
    // setup data space
    H5::DataSpace dataspace(H5S_SCALAR);

    // determine data type
    H5::DataType dtype = getDataType<T>();

    // Creating attribute and writing data into it
    H5::Attribute att = obj.createAttribute(path, dtype, dataspace);
    att.write(dtype, &value);
    att.close();

} // writeGMatrix

template <class CL>
static void writeStrAttrib(const CL &obj, const std::string &path, const std::string &ptr)
{
    // Create new dataspace for attribute
    hsize_t dim[] = {ptr.size()};
    H5::DataSpace dataspace(H5S_SCALAR, dim);

    // Create new string datatype for attribute
    H5::StrType dtype(H5::PredType::C_S1, ptr.size());

    // Create attribute and write to it
    H5::Attribute att = obj.createAttribute(path, dtype, dataspace);
    att.write(dtype, ptr.data());

} // writeGMatrix

template <typename T, class CL>
static void writeArray(const CL &obj, const std::string &path, const T *ptr,
                       size_t nRows, size_t nCols)
{
    // determine data type
    H5::DataType dtype = getDataType<T>();
    H5::DataSpace dspace;

    // setup data space
    if (nCols == 0)
    {
        hsize_t dim = nRows;
        dspace = H5::DataSpace(1, &dim);
    }
    else
    {
        hsize_t dim[] = {nRows, nCols};
        dspace = H5::DataSpace(2, dim);
    }

    // Creating dataset and writing data into it
    H5::DataSet dset = obj.createDataSet(path, dtype, dspace);

    dset.write(ptr, dtype);
    dset.close();
} // writeGMatrix

template <typename CL>
static void saveAttribute(CL &group, const GHDF5 &elem)
{
    // loop-over attributes
    for (auto &[name, att] : elem.attributes())
        if (att.getType() == G5_BOOL)
        {
            bool val;
            memcpy(&val, att.getRawPointer(), sizeof(bool));
            writeAttrib<bool>(group, name, val);
        }
        else if (att.getType() == G5_UINT8)
        {
            uint8_t val;
            memcpy(&val, att.getRawPointer(), sizeof(uint8_t));
            writeAttrib<uint8_t>(group, name, val);
        }
        else if (att.getType() == G5_UINT16)
        {
            uint16_t val;
            memcpy(&val, att.getRawPointer(), sizeof(uint16_t));
            writeAttrib<uint16_t>(group, name, val);
        }
        else if (att.getType() == G5_UINT32)
        {
            uint32_t val;
            memcpy(&val, att.getRawPointer(), sizeof(uint32_t));
            writeAttrib<uint32_t>(group, name, val);
        }
        else if (att.getType() == G5_CHAR)
        {
            char val;
            memcpy(&val, att.getRawPointer(), sizeof(char));
            writeAttrib<char>(group, name, val);
        }
        else if (att.getType() == G5_STRING)
        {
            String val((char *)att.getRawPointer(), att.getSize());
            writeStrAttrib(group, name, val);
        }
        else if (att.getType() == G5_INT)
        {
            int val;
            memcpy(&val, att.getRawPointer(), sizeof(int));
            writeAttrib<int>(group, name, val);
        }
        else if (att.getType() == G5_FLOAT)
        {
            float val;
            memcpy(&val, att.getRawPointer(), sizeof(float));
            writeAttrib<float>(group, name, val);
        }
        else
        {
            double val;
            memcpy(&val, att.getRawPointer(), sizeof(double));
            writeAttrib<double>(group, name, val);
        }

} // saveAttributes

template <typename CL>
static void saveData(CL &group, const GHDF5 &parent)
{

    for (auto &[name, obj] : parent.children())
    {

        if (obj.getType() == G5_GROUP)
        {
            H5::Group inner = group.createGroup(name);
            saveData(inner, obj);
            inner.close();
        }
        else if (obj.getType() == G5_BOOL || obj.getType() == G5_ARRAY_BOOL)
        {
            Shape dim = obj.getShape();
            const bool *ptr = (const bool *)obj.getRawPointer();
            writeArray<bool>(group, name, ptr, dim.y, dim.x);
        }

        else if (obj.getType() == G5_UINT8 || obj.getType() == G5_ARRAY_UINT8)
        {
            Shape dim = obj.getShape();
            const uint8_t *ptr = (const uint8_t *)obj.getRawPointer();
            writeArray<uint8_t>(group, name, ptr, dim.y, dim.x);
        }

        else if (obj.getType() == G5_UINT16 || obj.getType() == G5_ARRAY_UINT16)
        {
            Shape dim = obj.getShape();
            const uint16_t *ptr = (const uint16_t *)obj.getRawPointer();
            writeArray<uint16_t>(group, name, ptr, dim.y, dim.x);
        }

        else if (obj.getType() == G5_UINT32 || obj.getType() == G5_ARRAY_UINT32)
        {
            Shape dim = obj.getShape();
            const uint32_t *ptr = (const uint32_t *)obj.getRawPointer();
            writeArray<uint32_t>(group, name, ptr, dim.y, dim.x);
        }

        else if (obj.getType() == G5_INT || obj.getType() == G5_ARRAY_INT)
        {
            Shape dim = obj.getShape();
            const int *ptr = (const int *)obj.getRawPointer();
            writeArray<int>(group, name, ptr, dim.y, dim.x);
        }

        else if (obj.getType() == G5_FLOAT || obj.getType() == G5_ARRAY_FLOAT)
        {
            Shape dim = obj.getShape();
            const float *ptr = (const float *)obj.getRawPointer();
            writeArray<float>(group, name, ptr, dim.y, dim.x);
        }

        else if (obj.getType() == G5_DOUBLE || obj.getType() == G5_ARRAY_DOUBLE)
        {
            Shape dim = obj.getShape();
            const double *ptr = (const double *)obj.getRawPointer();
            writeArray<double>(group, name, ptr, dim.y, dim.x);
        }

        else if (obj.getType() == G5_CHAR || obj.getType() == G5_STRING)
        {
            Shape dim = obj.getShape();
            const char *ptr = (const char *)obj.getRawPointer();
            writeArray<char>(group, name, ptr, dim.y, dim.x);
        }

        // Saving Attributes
        if (obj.attributes().size() > 0)
        {
            if (obj.getType() == G5_GROUP)
            {
                H5::Group aux = group.openGroup(obj.getName());
                saveAttribute(aux, obj);
                aux.close();
            }
            else
            {
                H5::DataSet aux = group.openDataSet(obj.getName());
                saveAttribute(aux, obj);
                aux.close();
            }
        }
    } // loop-children

} // saveGroup

void GHDF5::save(const String &filename)
{
    g_assert(nameID.compare("root") == 0,
             "ERROR: Only root group can be exported to file!");

    H5::H5File arq(filename, H5F_ACC_TRUNC);
    saveData(arq, *this);

    // Saving root attributes
    if (attribs.size() > 0)
        saveAttribute(arq, *this);

    arq.close();

} // save

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <typename T, class CL>
static bool readAttrib(const CL &pred, GHDF5 &hdf, H5::Attribute &att)
{
    H5::DataType dtype = att.getDataType();

    if (dtype.getClass() == H5T_STRING)
    {
        std::string var;
        var.resize(att.getDataType().getSize());
        att.read(att.getDataType(), (void *)var.data());
        hdf.setAttribute<String>(att.getName(), var);
        return true;
    }

    if (dtype == H5::DataType(pred))
    {
        T var;
        att.read(dtype, &var);
        hdf.setAttribute<T>(att.getName(), var);
        return true;
    }

    return false;
} // readAttrib

template <class CL>
static void loadAttributes(CL &data, GHDF5 &hdf)
{
    for (int id = 0; id < data.getNumAttrs(); id++)
    {
        H5::Attribute attr = data.openAttribute(id);

        if (readAttrib<uint8_t>(H5::PredType::NATIVE_UINT8, hdf, attr))
            continue;

        if (readAttrib<uint16_t>(H5::PredType::NATIVE_UINT16, hdf, attr))
            continue;

        if (readAttrib<uint32_t>(H5::PredType::NATIVE_UINT32, hdf, attr))
            continue;

        if (readAttrib<size_t>(H5::PredType::NATIVE_UINT64, hdf, attr))
            continue;

        if (readAttrib<int>(H5::PredType::NATIVE_INT32, hdf, attr))
            continue;

        if (readAttrib<float>(H5::PredType::NATIVE_FLOAT, hdf, attr))
            continue;

        if (readAttrib<double>(H5::PredType::NATIVE_DOUBLE, hdf, attr))
            continue;

        if (readAttrib<char>(H5::PredType::NATIVE_CHAR, hdf, attr))
            continue;

    } // loop-attributes

} // loadAttributes

template <typename T>
static bool loadDataSet(const H5::PredType &pred, const H5::DataSet &dset,
                        GHDF5 &hdf, const String &name)
{
    H5::DataType dtype = dset.getDataType();

    if (!(dtype == H5::DataType(pred)))
        return false;

    H5::DataSpace space = dset.getSpace();

    hsize_t dim[2] = {1, 1};
    space.getSimpleExtentDims(dim);

    const size_t N = dim[0] * dim[1];
    if (N == 1)
    {
        T value;
        dset.read(&value, dtype);
        hdf[name] = value;
    }
    else
    {
        std::vector<T> vec(N);
        dset.read(vec.data(), dtype);
        hdf[name].createFromPointer<T>(vec.data(), dim[1], dim[0]);
    }

    loadAttributes(dset, hdf[name]);

    return true;
} // loadDataSet

static void loadFunction(const H5::Group &arq, GHDF5 &hdf)
{

    for (uint32_t movID = 0; movID < arq.getNumObjs(); movID++)
    {
        const String name = arq.getObjnameByIdx(movID);
        H5G_obj_t objID = arq.getObjTypeByIdx(movID);

        if (objID == H5G_GROUP)
        {
            H5::Group group = arq.openGroup(name);
            GHDF5 &h5 = hdf[name];
            loadFunction(group, h5);
            loadAttributes(group, h5);
        }

        else if (objID == H5G_DATASET)
        {
            H5::DataSet dset = arq.openDataSet(name);
            H5::DataSpace space = dset.getSpace();
            H5::DataType dtype = dset.getDataType();

            hsize_t dims[2] = {1, 1};
            space.getSimpleExtentDims(dims);

            if (loadDataSet<uint8_t>(H5::PredType::NATIVE_UINT8, dset, hdf, name))
                continue;

            if (loadDataSet<uint16_t>(H5::PredType::NATIVE_UINT16, dset, hdf, name))
                continue;

            if (loadDataSet<uint32_t>(H5::PredType::NATIVE_UINT32, dset, hdf, name))
                continue;

            if (loadDataSet<size_t>(H5::PredType::NATIVE_UINT64, dset, hdf, name))
                continue;

            if (loadDataSet<int32_t>(H5::PredType::NATIVE_INT32, dset, hdf, name))
                continue;

            if (loadDataSet<float>(H5::PredType::NATIVE_FLOAT, dset, hdf, name))
                continue;

            if (loadDataSet<double>(H5::PredType::NATIVE_DOUBLE, dset, hdf, name))
                continue;

            if (loadDataSet<char>(H5::PredType::NATIVE_CHAR, dset, hdf, name))
                continue;

            // There is a problem somewhere
            std::cout << "ERROR (GHDF5::loadFunction): '" << name
                      << "' couldn't be loaded!!\n";

            exit(-1);

        } // if-dataset

        else
        {
            std::cout << "ERROR: (GHDF5::loadFunction): Object type is not supported!\n";
            exit(-1);
        }

    } // loop-objects
}

void GHDF5::open(const String &filename)
{
    H5::H5File arq(filename, H5F_ACC_RDONLY);
    H5::Group group = arq.openGroup("/");
    loadFunction(group, *this);
    arq.close();
} // open