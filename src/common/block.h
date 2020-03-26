#ifndef BLOCK_H
#define BLOCK_H

#include <string>

using namespace std;

class Block
{
public:
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

    friend ostream& operator<<(ostream& os, const Block& wp);

private:
    string name;
    uint8_t no;

    string strip_icon;
    string strip_button_txt;
    string key;
    string group;
};

#endif // BLOCK_H
