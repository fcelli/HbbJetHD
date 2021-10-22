#include "ArgParser.h"

ArgParser::ArgParser(int argc, char **argv){
    if(argc != 4) {
        usage();
        exit(1);
    }
    ifileName = argv[1];
    ofileName = argv[2];
    cfgName   = argv[3];
}

void ArgParser::usage() const {
    std::cout << "Usage: ./apps/dump_histos input_file output_file config_file" << std::endl;
}