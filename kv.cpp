#include "kv.hpp"

#include <memory.h>

namespace beeoo {


dword mask(dword size)
{
    if (size == 0) {
        return 0;
    }
    dword value = ~((dword)0);
    dword data = (value >> (sizeof(dword) * 8 - size));
    return data;
}

bool isString(kvt type)
{
    switch (type) {
        case kvt::byteV :
        case kvt::wordV :
        case kvt::dwordV:
        case kvt::fixed :
        return true;

        default:
        return false;
    }
    return false;
}

bool isValue(kvt type)
{
    return !isString(type);
}


union host_net{
    short type;
    unsigned char byte[2];
};

bool bigEndian()
{
    host_net conv;
    conv.type = 0x0001;
    if (conv.byte[0]) {
        return false;
    }
    return true;
}

void to_net(const void * v, dword v_size, void * net, dword net_size)
{
    const byte * src = (const byte *)v;
    byte * d = (byte *)net;

    memset(net, 0, net_size);
    dword min = std::min(v_size, net_size);
    if (bigEndian()) {
        while (min--) {
            *(d + --net_size) = *(src + --v_size);
        }
    } else {
        while (min--) {
            *(d + --net_size) = *(src ++);
        }
    }
}

void to_local(const void * v, dword v_size, void * local, dword local_size)
{
    const byte * ptr = (const byte *)v;
    byte * d = (byte *)local;
    memset(local, 0, local_size);
    dword min = std::min(local_size, v_size);
    if (bigEndian()) {
        while (min--) {
            *(d + --local_size) = *(ptr + --v_size);
        }
    } else {
        while (min--) {
            *(d ++) = *(ptr + --v_size);
        }
    }
}

byte *headString(const string & str)
{
    return  (byte *)&(str.front());
}

dword valueFromNet(const void * n, dword size)
{
    dword value = 0;
    to_local(n, size, headValue(value), sizeof(value));
    return value;
}

dword setBitValue(dword value, dword new_value, size_t bytes, size_t size, size_t offset)
{
    if (bytes > sizeof(dword)) {
        throw(std::out_of_range("setBitValue"));
    }

    size_t bit_width = bytes * 8;
    size_t shift = bit_width - size - offset;

    new_value = mask(size) & new_value;

    if (shift >= 32) {
        value = 0;
        return 0;
    }


    value &= ~(mask(size) << (bit_width - size - offset));
    value |=   new_value  << (bit_width - size - offset);
    return value;
}

dword bitValue(dword value, size_t bytes, size_t size, size_t offset)
{
    if (bytes > sizeof(word)) {
        throw(std::out_of_range("bitValue"));
    }

    size_t bit_width = bytes * 8;

    value >>= bit_width - size - offset;
    value  &= mask(size);
    return value;
}

} // namespace beeoo
