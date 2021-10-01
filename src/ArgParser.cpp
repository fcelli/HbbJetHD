#include "ArgParser.h"

ArgParser::ArgParser(int argc, char **argv){
    if(argc != 7){
        usage();
        exit(0);
    }
    ifileName   = argv[1];
    ofileName   = argv[2];
    wsName      = argv[3];
    snapName    = argv[4];
    regName     = argv[5];
    obsName     = argv[6];
}

void ArgParser::usage() const {
    std::cout << "Usage: ./src/dump_histos input_file output_file workspace snapshot region observable" << std::endl;
}