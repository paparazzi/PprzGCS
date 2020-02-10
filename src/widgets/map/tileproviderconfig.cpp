#include "tileproviderconfig.h"
#include <iostream>

void TileProviderConfig::printConfig() {
    std::cout <<
        "Name: " << name.toStdString() <<
        "  Dir: " << dir.toStdString() <<
        "  Addr: " << addr.toStdString() <<
                 std::endl;
}
