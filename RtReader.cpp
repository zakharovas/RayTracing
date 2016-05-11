//
// Created by User on 10.05.2016.
//

#include <assert.h>
#include <ios>
#include <iostream>
#include "RtReader.h"

RtReader::RtReader(const std::string &name_of_file) {
    input_stream_.open(name_of_file.c_str(), std::ifstream::in);
    assert(input_stream_);
}

RtReader::~RtReader() {
    input_stream_.close();
}

World RtReader::CreateWorld() {
    World new_world;
    std::string s;
    std::getline(input_stream_, s);
    while (!input_stream_.eof()) {
        std::string name;
        input_stream_ >> name;
        if (name == "facet") {
            std::shared_ptr<Primitive> triangle = ReadTriangle_();
            new_world.AddPrimitive(triangle);
        }


        //TODO reader
//        if (name == "light") {
//            LightSource light = ReadLight_();
//            new_world.AddLight(light);
//        }
//        if (name == "sphere") {
//            std::shared_ptr<Primitive> sphere = ReadSphere_();
//            new_world.AddPrimitive(sphere);
//        }
//        if (name == "parallelogram") {
//            std::shared_ptr<Primitive> parallelogram = ReadParallelogram_();
//            new_world.AddPrimitive(parallelogram);
//        }
        if (name == "viewport") {
            Camera camera = ReadCamera_();
            new_world.SetCamera(camera);
        }
        if (name == "endsolid") {
            break;
        }
    }
    std::cout << "READING COMPLETED";
    return new_world;
}


Camera RtReader::ReadCamera_() {
    std::string s;
    input_stream_ >> s;
    double x, y, z;
    input_stream_ >> x >> y >> z;
    Point origin(x, y, z);
    input_stream_ >> s;
    input_stream_ >> x >> y >> z;
    Point top_left(x, y, z);
    input_stream_ >> s;
    input_stream_ >> x >> y >> z;
    Point bottom_left(x, y, z);
    input_stream_ >> s;
    input_stream_ >> x >> y >> z;
    Point top_right(x, y, z);
    Point bottom_right = bottom_left + (top_right - top_left);
    input_stream_ >> s;
    return Camera(top_left, top_right, bottom_left, bottom_right, origin);
}








