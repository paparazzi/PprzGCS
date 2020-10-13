#ifndef BLOCK_H
#define BLOCK_H

#include <string>
#include <map>
#include <vector>
#include "tinyxml2.h"

using namespace std;
using namespace tinyxml2;

struct Stage {
    string instruction;
    map<string, string> attributes;
    uint8_t no;
};

class Block
{
public:
    Block(tinyxml2::XMLElement* blk);
    Block(string name, uint8_t no);

    void setIcon(string icon) {strip_icon = icon;}
    void setText(string text) {strip_button_txt = text;}
    void setKey(string key) {this->key = key;}
    void setGroup(string group) {this->group = group;}

    string getName() {return name;}
    uint8_t getNo() {return no;}
    string getIcon() {return strip_icon;}
    string getText() {return strip_button_txt ;}
    string getKey() {return key;}
    string getGroup() {return group;}
    vector<Stage>& getStages() {return stages;}

    friend ostream& operator<<(ostream& os, const Block& wp);

private:
    void parse_blocks(tinyxml2::XMLElement* blks);
    void parse_block_stages(tinyxml2::XMLElement* blk);

    string name;
    uint8_t no;

    string strip_icon;
    string strip_button_txt;
    string key;
    string group;

    map<string, string> attributes;

    vector<Stage> stages;
};

#endif // BLOCK_H
