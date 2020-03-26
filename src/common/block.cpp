#include "block.h"
#include <iostream>

Block::Block(string name, uint8_t no) :
    name(name), no(no)
{

}


ostream& operator<<(ostream& os, const Block& bk) {

    os << "{" << bk.name << ", no: " << to_string(bk.no);

    if(bk.strip_icon != "") {
        os << " icon: " << bk.strip_icon;
    }

    if(bk.strip_button_txt != "") {
        os << " text: " << bk.strip_button_txt;
    }

    if(bk.key != "") {
        os << " key: " << bk.key;
    }

    if(bk.group != "") {
        os << " group: " << bk.group;
    }

    os << "}";

    return os;
}
