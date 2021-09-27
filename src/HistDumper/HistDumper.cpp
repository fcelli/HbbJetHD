#include "HistDumper.h"

HistDumper::HistDumper(const RooWorkspace &ws){
    ws.Print();
}