//
// Created by User on 25.04.2016.
//

#ifndef INC_3DVISUALIZER_STLREADER_H
#define INC_3DVISUALIZER_STLREADER_H


#include <string>
#include <fstream>
#include "World.h"

class StlReader {
public:


    StlReader(const std::string &name_of_file);

    World CreateWorld();

    ~StlReader();

private:

private:
    std::ifstream input_stream_;
};


#endif //INC_3DVISUALIZER_STLREADER_H