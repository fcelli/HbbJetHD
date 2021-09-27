#include "ArgParser.h"

ArgParser::ArgParser(int argc, char **argv){
    if(argc != 7){
        usage();
        exit(0);
    }
    m_inputfile = argv[1];
    m_outputfile = argv[2];
    m_workspace = argv[3];
    m_snapshot = argv[4];
    m_region = argv[5];
    m_observable = argv[6];
}

void ArgParser::usage() const {
    std::cout<<"Usage: ./src/dump_histos input_file output_file workspace snapshot region observable"<<std::endl;
}