#include "block.h"
#include <iostream>
#include <assert.h>

Block::Block(string name, uint8_t no) :
    name(name), no(no)
{

}

Block::Block(tinyxml2::XMLElement* blk) {
    const char* name_p =blk->Attribute("name");
    assert(name_p != nullptr);
    name = name_p;
    const char* no_p = blk->Attribute("no");
    assert(no_p != nullptr);
    no = static_cast<uint8_t>(stoul(no_p));

    const char* icon_p = blk->Attribute("strip_icon");
    if(icon_p != nullptr) {
        setIcon(icon_p);
    }
    const char* text_p = blk->Attribute("strip_button");
    if(text_p != nullptr) {
        setText(text_p);
    }
    const char* key_p = blk->Attribute("key");
    if(key_p != nullptr) {
        setKey(key_p);
    }
    const char* group_p = blk->Attribute("group");
    if(group_p != nullptr) {
        setGroup(group_p);
    }

    for(auto att=blk->FirstAttribute(); att!=nullptr; att=att->Next()) {
        //cout << "attribute " << att->Name() << "  =  " << att->Value() << endl;
        attributes[att->Name()] = att->Value();
    }

    parse_block_stages(blk);
}

void Block::parse_block_stages(tinyxml2::XMLElement* blk) {
    auto stel = blk->FirstChildElement();
    while(stel != nullptr) {
        Stage stage;
        stage.no = 255;
        stage.instruction = stel->Name();
        auto att = stel->FirstAttribute();
        while(att != nullptr) {
            stage.attributes[att->Name()] = att->Value();
            att = att->Next();
        }
        const char* no_p = stel->Attribute("no");
        if(no_p != nullptr) {
            stage.no = static_cast<uint8_t>(stoul(no_p));
        }
        stages.push_back(stage);
        stel = stel->NextSiblingElement();
    }
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
