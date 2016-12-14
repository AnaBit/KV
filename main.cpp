#include <iostream>
#include <stdio.h>

#include "kv.hpp"

using namespace std;

namespace beeoo {

kv_name(student)
key_begin
num,
arg,
high,
name,
addr,
info,
phone,
sex,
doub,
key_end

ini_begin(ini)
{key::num,   kvt::byte,   0, 0},
{key::arg,   kvt::word,   0, 0},
{key::high,  kvt::dword,  0, 0},
{key::name,  kvt::byteV,  0, 0},
{key::addr,  kvt::wordV,  0, 0},
{key::info,  kvt::dwordV, 0, 0},
{key::phone, kvt::fixed,  12, 0},
{key::sex,   kvt::bytebitN,  3, 0},
{key::doub,   kvt::bytebitC,  2, 0},
ini_end(ini)

kv_name_end

}

int main(int argc, char *argv[])
{
    beeoo::kvconv<beeoo::student::key> stu;
    stu.setini(beeoo::student::ini);
    stu.inerst(beeoo::student::key::num, 1);
    stu.inerst(beeoo::student::key::arg, 2);
    stu.inerst(beeoo::student::key::high, 3);
    stu.inerst(beeoo::student::key::name, "abcde");
    stu.inerst(beeoo::student::key::addr, "ABCDE");
    stu.inerst(beeoo::student::key::info, "12345");
    stu.inerst(beeoo::student::key::phone, "1234567890ab");
    stu.inerst(beeoo::student::key::sex, 7);
    stu.inerst(beeoo::student::key::doub, 3);

    auto buf = stu.pack();

    for (auto & i : buf){
        printf("%02X ", i  & 0xff);
    }
    cout << endl;

    beeoo::kvconv<beeoo::student::key> recv;
    recv.setini(beeoo::student::ini);
    recv.parse(stu.pack());

    cout << "num  = " << recv.value(beeoo::student::key::num) << endl;
    cout << "arg  = " << recv.value(beeoo::student::key::arg) << endl;
    cout << "high  = " << recv.value(beeoo::student::key::high) << endl;
    cout << "name = " << recv.mutarray(beeoo::student::key::name) << endl;
    cout << "addr = " << recv.mutarray(beeoo::student::key::addr) << endl;
    cout << "info = " << recv.mutarray(beeoo::student::key::info) << endl;
    cout << "phone = " << recv.mutarray(beeoo::student::key::phone) << endl;
    cout << "sex  = " << recv.value(beeoo::student::key::sex) << endl;
    cout << "doub  = " << recv.value(beeoo::student::key::doub) << endl;

    return 0;
}
