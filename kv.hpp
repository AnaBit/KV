#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <cassert>
#include <memory.h>

using namespace std;

namespace beeoo {

using byte  = uint8_t;
using word  = uint16_t;
using dword = uint32_t;

// base          -> byte word int long
// array         -> fixed
// mutable array -> byte word int long
// bit           -> byte word int

enum class kvt {
    byte,
    word,
    dword,

    byteV,
    wordV,
    dwordV,

    fixed,

    bytebitN,
    bytebitC,

    wordbitN,
    wordbitC,

    dwordbitN,
    dwordbitC,
    //
    kvte
};

dword mask (dword size);

bool isString(kvt type);
bool isValue(kvt type);

byte * headString(const std::string & str);

dword setBitValue(dword value, dword new_value, size_t bytes, size_t size, size_t offset);
dword bitValue(dword value, size_t bytes, size_t size, size_t offset);

template<typename T>
byte * headValue(T & t)
{
    return reinterpret_cast<byte *>(&t);
}

bool bigEndian();
void to_net(const void * v, dword v_size, void * net, dword net_size);
void to_local(const void * v, dword v_size, void * local, dword local_size);
dword valueFromNet(const void * n, dword size);

template <typename S, typename T>
bool inRange (kvt type, S size, T value)
{
    dword com = static_cast<dword>(value);
    switch (type) {
        case kvt::fixed :
        case kvt::byteV :
        case kvt::wordV :
        case kvt::dwordV:
        return false;

        case kvt::byte : {
            byte v = 0 - 1;
            return com > v ? false : true;
        }

        case kvt::word : {
            word v = 0 - 1;
            return com > v ? false : true;
        }

        case kvt::dword : {
            dword v = 0 - 1;
            return com > v ? false : true;
        }

        case kvt::bytebitN :
        case kvt::bytebitC : {
            return (sizeof (byte) * 8 >= size) && (mask(size) >= com) ? true : false;
        }

        case kvt::wordbitN :
        case kvt::wordbitC :{
            return (sizeof (word) * 8 >= size) && (mask(size) >= com) ? true : false;
        }

        case kvt::dwordbitN :
        case kvt::dwordbitC :{
            return (sizeof (dword) * 8 >= size) && (mask(size) >= com) ? true : false;
        }

        default:
        return false;
    }
}

template <typename S>
bool inRange (kvt type, S size, const std::string & value)
{
    switch (type) {
        case kvt::byteV : {
            byte v = 0 - 1;
            return value.size() > v ? false : true;
        }
        case kvt::wordV : {
            word v = 0 - 1;
            return value.size() > v ? false : true;
        }
        case kvt::dwordV: {
            dword v = 0 - 1;
            return value.size() > v ? false : true;
        }

        case kvt::fixed : {
            dword v = 0 - 1;
            return (value.size() > v || value.size() !=  size) ? false : true;
        }

        default:
        return false;
    }
}

template<typename T>
struct element {
    T   key;
    kvt type;
    dword size;
    dword bit_offset;
};

template<typename T>
struct elements {
    const element<T> * ele;
    const dword size;

    const element<T> & operator [] (size_t index) const
    {
        if (index >= size) {
            throw(std::out_of_range("index is out"));
        }
        return ele[index];
    }
};

#define    kv_name(x)   namespace x {
#define   kv_name_end   }
#define     key_begin   enum class key {
#define       key_end   key_size };
#define  ini_begin(x)   constexpr static element<key> __##x [] = {
#define    ini_end(x)   }; constexpr static elements<key> x = {__##x, sizeof(__##x) / sizeof(__##x[0])};


template <typename Key>
class kvconv {
public:
    kvconv();
    ~kvconv();

    void setini(const elements<Key> & ini);

    template<typename T>
    void inerst(Key key, T value);
    void inerst(Key key, const std::string & value);
    void inerst(Key key, const char * value);
    void inerst(Key key, const void * value, size_t n);
    void inerst(Key key, std::string && value);

    dword value(Key key) const;
    std::string & mutarray(Key key) const;

    size_t parse(const std::string & stream, size_t pos = 0);
    std::string pack() const;

public:
    struct unit {
        const element<Key> * ele = nullptr;
        std::string mutarray;
        dword value = 0;
    };

private:
    bool keySaft(Key key) const;

    template <typename T>
    unit & unitAt(T t) const;
    void iniCheck(const elements<Key> & ele);
    size_t pack_size() const;
    void reset();

private:
    const std::shared_ptr<unit> _units;
    const size_t _unitSize;

    bool _hasini = false;
};

template <typename S>
kvconv<S>::kvconv()
    : _units(new unit[static_cast<size_t>(S::key_size)], [] (unit * p) { delete [] p; }),
_unitSize(static_cast<size_t>(S::key_size))
{

}

template <typename Key>
kvconv<Key>::~kvconv()
{

}

template <typename Key>
void kvconv<Key>::setini(const elements<Key> & ini)
{
    iniCheck(ini);
    unit * u = _units.get();
    std::string error(__func__);
    error.append(" index = ");
    if (!_hasini) {
        for(size_t index = 0; index < _unitSize; ++index) {
            u[index].ele = (ini.ele + index);
            // fixed need resize mutarray
            if (u[index].ele->type == kvt::fixed) {
                u[index].mutarray.resize(u[index].ele->size);
            }
        }
    } else {
        for(size_t index = 0; index < _unitSize; ++index) {
            if(u[index].ele == nullptr) {
                error.append(std::to_string(index + 1));
                error.append(" nullptr");
                throw(std::out_of_range(error));
            }

            auto & unit_ = u[index];
            auto & local = u[index].ele;
            auto * set   = (ini.ele + index);

            if (isString(local->type)) {
                if (inRange(set->type, set->size, unit_.mutarray)) {
                    local = set;
                } else {
                    error.append(std::to_string(index + 1));
                    throw(std::out_of_range(error));
                }
            } else {
                if (inRange(set->type, set->size, unit_.value)) {
                    local = set;
                } else {
                    error.append(std::to_string(index + 1));
                    throw(std::out_of_range(error));
                }
            }
        }
    }
    _hasini = true;
}

template <typename Key>
bool kvconv<Key>::keySaft(Key key) const
{
    if (static_cast<dword>(key) >= _unitSize) {
        return false;
    } else {
        return true;
    }
}

template <typename Key>
template <typename T>
typename kvconv<Key>::unit & kvconv<Key>::unitAt(T t) const
{
    if (static_cast<size_t>(t) >= _unitSize) {
        throw(std::out_of_range("unit key"));
    }
    return _units.get()[static_cast<size_t>(t)];
}

template <typename Key>
template <typename T>
void kvconv<Key>::inerst(Key key, T value)
{
    if (!keySaft(key)) {
        throw(std::runtime_error("key is out"));
    }

    unit & u = unitAt(key);
    if (!inRange(u.ele->type, u.ele->size, value)) {
        throw(std::out_of_range("kvconv insert"));
    }

    u.value = value;
}

template <typename Key>
void kvconv<Key>::inerst(Key key, const std::string & value)
{
    if (!keySaft(key)) {
        throw(std::runtime_error("key is out"));
    }

    unit & u = unitAt(key);
    if (!inRange(u.ele->type, u.ele->size, value)) {
        throw(std::out_of_range("kvconv insert"));
    }

    u.mutarray = value;
}

template <typename Key>
void kvconv<Key>::inerst(Key key, const char * value)
{
    std::string conv(value);
    inerst(key, conv);
}

template <typename Key>
void kvconv<Key>::inerst(Key key, const void * value, size_t n)
{
    std::string conv;
    conv.resize(n);
    memcpy(&conv.front(), value, n);
    inerst(key, conv);
}

template <typename Key>
void kvconv<Key>::kvconv::inerst(Key key, string && value)
{
    if (!keySaft(key)) {
        throw(std::runtime_error("key is out"));
    }

    unit & u = unitAt(key);
    if (!inRange(u.ele->type, u.ele->size, value)) {
        throw(std::out_of_range("kvconv insert"));
    }

    u.mutarray = std::move(value);
}

template <typename Key>
dword kvconv<Key>::value(Key key) const
{
    if (!keySaft(key)) {
        throw(std::runtime_error("key is out"));
    }

    unit & u = unitAt(key);
    if (!isValue(u.ele->type)) {
        throw(std::runtime_error("key type is not value"));
    } else {
        return u.value;
    }
}

template <typename Key>
string & kvconv<Key>::mutarray(Key key) const
{
    if (!keySaft(key)) {
        throw(std::runtime_error("key is out"));
    }

    unit & u = unitAt(key);
    if (!isString(u.ele->type)) {
        throw(std::runtime_error("key type is not string"));
    } else {
        return u.mutarray;
    }
}

template <typename Key>
void kvconv<Key>::iniCheck(const elements<Key> & ele)
{
    kvt    last_type = kvt::kvte;
    size_t bit_size  = 0;
    size_t offset = 0;

    std::string error("index = ");

    if (ele.size == 0) {
        throw(std::runtime_error("size == 0"));
    }

    for (size_t index = 0; index < ele.size; ++index){
        kvt    now_type  = ele[index].type;
        size_t size      = ele[index].size;
        size_t now_offset= ele[index].bit_offset;

        switch(now_type) {
            case kvt::byte :
            case kvt::word :
            case kvt::dword :
            case kvt::byteV :
            case kvt::wordV :
            case kvt::dwordV :
            case kvt::fixed :
            break;

            case kvt::bytebitN : {
                bit_size = sizeof (byte) * 8;
                offset = now_offset;
                if ((offset + size) > bit_size) {
                    goto outrange;
                }
                offset += size;
            }
            break;

            case kvt::wordbitN : {
                bit_size = sizeof (word) * 8;
                offset = now_offset;
                if ((offset + size) > bit_size) {
                    goto outrange;
                }
                offset += size;
            }
            break;

            case kvt::dwordbitN : {
                bit_size = sizeof (dword) * 8;
                offset = now_offset;
                if ((offset + size) > bit_size) {
                    goto outrange;
                }
                offset += size;
            }
            break;

            case kvt::bytebitC :
            case kvt::wordbitC :
            case kvt::dwordbitC : {
                if (kvt::bytebitN == last_type
                        || kvt::wordbitN == last_type
                        || kvt::dwordbitN == last_type
                        || now_type == last_type) {
                    offset += now_offset;
                    if ((offset + size) > bit_size) {
                        goto outrange;
                    }
                    offset += size;
                } else {
                    goto runtime;

                }
            }
            break;

            default:
                throw(std::runtime_error("type error"));
        }
        last_type = now_type;
        continue;

runtime:
        error.append(std::to_string(index + 1));
        throw(std::runtime_error(error));

outrange:
        error.append(std::to_string(index + 1));
        throw(std::out_of_range(error));
    }
}

template <typename Key>
size_t kvconv<Key>::pack_size() const
{
    size_t size = 0;
    for (size_t index = 0; index < _unitSize; ++index) {
        unit & u = unitAt(index);
        switch(u.ele->type) {
            case kvt::byteV :
                size += u.mutarray.size();
            case kvt::byte :
            case kvt::bytebitN:
                size += sizeof(byte);
            break;

            case kvt::wordV :
                size += u.mutarray.size();
            case kvt::word :
            case kvt::wordbitN :
                size += sizeof(word);
            break;

            case kvt::dwordV :
                size += u.mutarray.size();
            case kvt::dword :
            case kvt::dwordbitN :
                size += sizeof(dword);
            break;

            case kvt::fixed :
                size += u.mutarray.size();
            break;

            default :
            break;
        }
    }
    return size;
}

template <typename Key>
void kvconv<Key>::reset()
{
    for (size_t index = 0; index < _unitSize; ++index) {
        unit & u = unitAt(index);
        if (u.ele->type != kvt::fixed) {
            std::string fixed;
            fixed.resize(u.ele->size);
            u.mutarray.swap(fixed);
            u.value = 0;
        } else {
            u.mutarray.clear();
            u.value = 0;
        }
    }
}

template <typename Key>
size_t kvconv<Key>::parse(const std::string & stream, size_t pos)
{
    std::string error(__func__);
    error.append(" index = ");

    byte * head = headString(stream) + pos;
    const byte * const stream_end = head + stream.length();

    size_t valueBytes = 0;
    dword bit_value = 0;
    // bit field
    kvt last_type = kvt::kvte;
    size_t offset = 0;
    // tmp
    dword mut_size;

    for (size_t index = 0; index < _unitSize; ++index) {
        unit & u = unitAt(index);
        auto & ele = *(u.ele);
        switch(ele.type) {
            case kvt::byte :
                valueBytes = sizeof(byte);
                goto basetype;

            case kvt::word :
                valueBytes = sizeof(word);
                goto basetype;

            case kvt::dword :
                valueBytes = sizeof(dword);
                goto basetype;

            case kvt::byteV :
                valueBytes = sizeof(byte);
                goto mutarray;

            case kvt::wordV :
                valueBytes = sizeof(word);
                goto mutarray;

            case kvt::dwordV :
                valueBytes = sizeof(dword);
                goto mutarray;

            case kvt::bytebitN :
                valueBytes = sizeof(byte);
                goto bitfield;

            case kvt::wordbitN :
                valueBytes = sizeof(word);
                goto bitfield;

            case kvt::dwordbitN :
                valueBytes = sizeof(dword);
                goto bitfield;

            case kvt::bytebitC :
            case kvt::wordbitC :
            case kvt::dwordbitC :

                if (last_type != kvt::bytebitN
                        && last_type != kvt::wordbitN
                        && last_type != kvt::dwordbitN) {
                    error.append(std::to_string(index + 1));
                    throw(std::out_of_range(error));
                }
                offset += ele.bit_offset;
                bit_value = valueFromNet(head - valueBytes, valueBytes);
                bit_value = bitValue(bit_value, valueBytes, ele.size, offset);
                u.value = bit_value;
                goto last;

            case kvt::fixed :
                if (stream_end < head + u.mutarray.length()) {
                    reset();
                    error.append(std::to_string(index + 1));
                    throw(std::out_of_range(error));
                }
                memcpy(headString(u.mutarray), head, u.mutarray.length());
                head += u.mutarray.length();
                goto last;

            default :
                throw(std::out_of_range("pack type error"));
        }
        goto last;

        basetype:
        if (stream_end < head + valueBytes) {
            reset();
            error.append(std::to_string(index + 1));
            throw(std::out_of_range(error));
        }
        u.value = valueFromNet(head, valueBytes);
        head += valueBytes;
        goto last;

        mutarray:
        if (stream_end < head + valueBytes) {
            reset();
            error.append(std::to_string(index + 1));
            throw(std::out_of_range(error));
        }
        mut_size = valueFromNet(head, valueBytes);
        head += valueBytes;

        if (stream_end < head + mut_size) {
            reset();
            error.append(std::to_string(index + 1));
            throw(std::out_of_range(error));
        }
        u.mutarray.resize(mut_size);
        memcpy(headString(u.mutarray), head, mut_size);
        head += mut_size;
        goto last;

        bitfield:
        if (stream_end < head + valueBytes) {
            reset();
            error.append(std::to_string(index + 1));
            throw(std::out_of_range(error));
        }
        offset = ele.bit_offset;
        bit_value = valueFromNet(head, valueBytes);
        bit_value = bitValue(bit_value, valueBytes,
                               ele.size, offset);
        u.value = bit_value;
        offset += ele.size;
        head += valueBytes;
        goto last;

        last:
        last_type = ele.type;
    }
    return stream.length() - (stream_end - head);
}

template <typename Key>
std::string kvconv<Key>::pack() const
{
    auto size = pack_size();
    std::string data;
    data.resize(size);

    byte * head = headString(data);

    size_t valueBytes = 0;
    dword bitValue = 0;
    // bit field
    kvt last_type = kvt::kvte;
    size_t offset = 0;
    // tmp
    dword mut_size;

    for (size_t index = 0; index < _unitSize; ++index) {
        unit & u = unitAt(index);
        auto & ele = *(u.ele);
        switch(ele.type) {
            case kvt::byte :
                valueBytes = sizeof(byte);
                goto basetype;

            case kvt::word :
                valueBytes = sizeof(word);
                goto basetype;

            case kvt::dword :
                valueBytes = sizeof(dword);
                goto basetype;

            case kvt::byteV :
                valueBytes = sizeof(byte);
                goto mutarray;

            case kvt::wordV :
                valueBytes = sizeof(word);
                goto mutarray;

            case kvt::dwordV :
                valueBytes = sizeof(dword);
                goto mutarray;

            case kvt::bytebitN :
                valueBytes = sizeof(byte);
                goto bitfield;

            case kvt::wordbitN :
                valueBytes = sizeof(word);
                goto bitfield;

            case kvt::dwordbitN :
                valueBytes = sizeof(dword);
                goto bitfield;

            case kvt::bytebitC :
            case kvt::wordbitC :
            case kvt::dwordbitC :

                offset += ele.bit_offset;
                bitValue = valueFromNet(head - valueBytes, valueBytes);
                bitValue = setBitValue(bitValue, u.value, valueBytes,
                                       ele.size, offset);
                to_net(headValue(bitValue), sizeof(dword), head - valueBytes, valueBytes);
                goto last;

            case kvt::fixed :
                memcpy(head, headString(u.mutarray), u.mutarray.length());
                head += u.mutarray.length();
                goto last;

            default :
                throw(std::out_of_range("pack type error"));
        }

        basetype:
        to_net(headValue(u.value), sizeof(u.value), head, valueBytes);
        head += valueBytes;
        goto last;

        mutarray:
        mut_size = u.mutarray.length();
        to_net(headValue(mut_size), sizeof(mut_size), head, valueBytes);
        head += valueBytes;

        memcpy(head, headString(u.mutarray), u.mutarray.length());
        head += u.mutarray.length();
        goto last;

        bitfield:
        offset = ele.bit_offset;
        bitValue = setBitValue(0, u.value, valueBytes,
                               ele.size, offset);
        to_net(headValue(bitValue), sizeof(dword), head, valueBytes);
        offset += ele.size;
        head += valueBytes;
        goto last;

        last:
        last_type = ele.type;
    }

    return data;
}

} // namespace beeoo
