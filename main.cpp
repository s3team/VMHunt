#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <map>
#include <vector>
#include <set>

using namespace std;

#include "core.hpp"
#include "mg-symengine.hpp"
#include "parser.hpp"

list<Inst> instlist1, instlist2;     // all instructions in the trace

int main(int argc, char **argv) {
     if (argc != 2) {
          fprintf(stderr, "usage: %s <target>\n", argv[0]);
          return 1;
     }
     ifstream infile1(argv[1]);

     if (!infile1.is_open()) {
          fprintf(stderr, "Open file error!\n");
          return 1;
     }

     parseTrace(&infile1, &instlist1);

     infile1.close();

     parseOperand(instlist1.begin(), instlist1.end());

     SEEngine *se1 = new SEEngine();
     se1->initAllRegSymol(instlist1.begin(), instlist1.end());
     se1->symexec();
     se1->dumpreg("eax");

     return 0;
}
