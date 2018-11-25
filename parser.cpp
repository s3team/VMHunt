#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <string>
#include <list>
#include <map>
#include <vector>
#include <set>
#include <regex>

using namespace std;

#include "core.hpp"
#include "parser.hpp"


Operand *createAddrOperand(string s)
{
     // regular expressions addresses
     regex addr1("0x[[:xdigit:]]+");
     regex addr2("eax|ebx|ecx|edx|esi|edi|esp|ebp");
     regex addr3("(eax|ebx|ecx|edx|esi|edi|esp|ebp)\\*([[:digit:]])");

     regex addr4("(eax|ebx|ecx|edx|esi|edi|esp|ebp)(\\+|-)(0x[[:xdigit:]]+)");
     regex addr5("(eax|ebx|ecx|edx|esi|edi|esp|ebp)\\+(eax|ebx|ecx|edx|esi|edi|esp|ebp)\\*([[:digit:]])");
     regex addr6("(eax|ebx|ecx|edx|esi|edi|esp|ebp)\\*([[:digit:]])(\\+|-)(0x[[:xdigit:]]+)");

     regex addr7("(eax|ebx|ecx|edx|esi|edi|esp|ebp)\\+(eax|ebx|ecx|edx|esi|edi|esp|ebp)\\*([[:digit:]])(\\+|-)(0x[[:xdigit:]]+)");


     Operand *opr = new Operand();
     smatch m;

     // pay attention to the matching order: long sequence should be matched first,
     // then the subsequence.
     if (regex_search(s, m, addr7)) { // addr7: eax+ebx*2+0xfffff1
          opr->ty = Operand::MEM;
          opr->tag = 7;
          opr->field[0] = m[1]; // eax
          opr->field[1] = m[2]; // ebx
          opr->field[2] = m[3]; // 2
          opr->field[3] = m[4]; // + or -
          opr->field[4] = m[5]; // 0xfffff1
     } else if (regex_search(s, m, addr4)) { // addr4: eax+0xfffff1
          // cout << "addr 4: " << s << endl;
          opr->ty = Operand::MEM;
          opr->tag = 4;
          opr->field[0] = m[1]; // eax
          opr->field[1] = m[2]; // + or -
          opr->field[2] = m[3]; // 0xfffff1
     } else if (regex_search(s, m, addr5)) { // addr5: eax+ebx*2
          opr->ty = Operand::MEM;
          opr->tag = 5;
          opr->field[0] = m[1]; // eax
          opr->field[1] = m[2]; // ebx
          opr->field[2] = m[3]; // 2
     } else if (regex_search(s, m, addr6)) { // addr6: eax*2+0xfffff1
          opr->ty = Operand::MEM;
          opr->tag = 6;
          opr->field[0] = m[1]; // eax
          opr->field[1] = m[2]; // 2
          opr->field[2] = m[3]; // + or -
          opr->field[3] = m[4]; // 0xfffff1
     } else if (regex_search(s, m, addr3)) { // addr3: eax*2
          opr->ty = Operand::MEM;
          opr->tag = 3;
          opr->field[0] = m[1]; // eax
          opr->field[1] = m[2]; // 2
     } else if (regex_search(s, m, addr1)) { // addr1: Immdiate value address
          opr->ty = Operand::MEM;
          opr->tag = 1;
          opr->field[0] = m[0];
     } else if (regex_search(s, m, addr2)) { // addr2: 32 bit register address
          // cout << "addr 2: " << s << endl;
          opr->ty = Operand::MEM;
          opr->tag = 2;
          opr->field[0] = m[0];
     } else {
          cout << "Unknown addr operands: " << s << endl;
     }

     return opr;
}

Operand* createDataOperand(string s)
{
     // Regular expressions for Immvalue and Registers
     regex immvalue("0x[[:xdigit:]]+");
     regex reg8("al|ah|bl|bh|cl|ch|dl|dh");
     regex reg16("ax|bx|cx|dx|si|di|bp|cs|ds|es|fs|gs|ss");
     regex reg32("eax|ebx|ecx|edx|esi|edi|esp|ebp|st0|st1|st2|st3|st4|st5");

     Operand *opr = new Operand();
     smatch m;
     if (regex_search(s, m, reg32)) { // 32 bit register
          opr->ty = Operand::REG;
          opr->bit = 32;
          opr->field[0] = m[0];
     } else if (regex_search(s, m, reg16)) { // 16 bit register
          opr->ty = Operand::REG;
          opr->bit = 16;
          opr->field[0] = m[0];
     } else if (regex_search(s, m, reg8)) { // 8 bit register
          opr->ty = Operand::REG;
          opr->bit = 8;
          opr->field[0] = m[0];
     } else if (regex_search(s, m, immvalue)) {
          opr->ty = Operand::IMM;
          opr->bit = 32;
          opr->field[0] = m[0];
     } else {
          cout << "Unknown data operands: " << s << endl;
     }

     return opr;
}

Operand* createOperand(string s)
{
     regex ptr("ptr \\[(.*)\\]");
     regex byteptr("byte ptr \\[(.*)\\]");
     regex wordptr("word ptr \\[(.*)\\]");
     regex dwordptr("dword ptr \\[(.*)\\]");
     regex segptr("dword ptr (fs|gs):\\[(.*)\\]");
     smatch m;

     Operand * opr;

     if (s.find("ptr") != string::npos) { // Operand is a mem access addr
          if (regex_search(s, m, segptr)) {
               opr = createAddrOperand(m[2]);
               opr->issegaddr = true;
               opr->bit = 32;
               opr->segreg = m[1];
          } else if (regex_search(s, m, dwordptr)) {
               opr = createAddrOperand(m[1]);
               opr->bit = 32;
          } else if (regex_search(s, m, wordptr)) {
               opr = createAddrOperand(m[1]);
               opr->bit = 16;
          } else if (regex_search(s, m, byteptr)) {
               opr = createAddrOperand(m[1]);
               opr->bit = 8;
          } else  if (regex_search(s, m, ptr)) {
               opr = createAddrOperand(m[1]);
               opr->bit = 32;
          } else {
               cout << "Unkown addr: " << s << endl;
          }
     } else {                   // Operand is data
          // cout << "data operand: " << s << endl;
          opr = createDataOperand(s);
     }

     return opr;
}

void parseOperand(list<Inst>::iterator begin, list<Inst>::iterator end)
{
     // parse operands
     for (list<Inst>::iterator it = begin; it != end; ++it) {
          for (int i = 0; i < it->oprnum; ++i) {
               it->oprd[i] = createOperand(it->oprs[i]);
          }
     }

}

// parse the whole trace into a instruction list L
void parseTrace(ifstream *infile, list<Inst> *L)
{
     string line;
     int num = 1;

     while (infile->good()) {
          getline(*infile, line);
          if (line.empty()) { continue; }

          istringstream strbuf(line);
          string temp, disasstr;

          Inst *ins = new Inst();
          ins->id = num++;

          // get the instruction address
          getline(strbuf, ins->addr, ';');
          ins->addrn = stoul(ins->addr, 0, 16);

          // get the disassemble string
          getline(strbuf, disasstr, ';');
          ins->assembly = disasstr;

          istringstream disasbuf(disasstr);
          getline(disasbuf, ins->opcstr, ' ');
          // ins->opc = getOpcode(ins->opcstr);

          while (disasbuf.good()) {
               getline(disasbuf, temp, ',');
               if (temp.find_first_not_of(' ') != string::npos)
                    ins->oprs.push_back(temp);
          }
          ins->oprnum = ins->oprs.size();

          // parse 8 context reg values
          for (int i = 0; i < 8; ++i) {
               getline(strbuf, temp, ',');
               ins->ctxreg[i] = stoul(temp, 0, 16);
          }

          // parse memory access addresses
          getline(strbuf, temp, ',');
          ins->raddr = stoul(temp, 0, 16);
          getline(strbuf, temp, ',');
          ins->waddr = stoul(temp, 0, 16);

          L->push_back(*ins);
     }
}

void printfirst3inst(list<Inst> *L)
{
     int i = 0;
     for (list<Inst>::iterator it = L->begin(); it != L->end() && i < 3; ++it, ++i) {
          cout << it->opcstr << '\t';
          for (vector<string>::iterator ii = it->oprs.begin(); ii != it->oprs.end(); ++ii) {
               cout << *ii << '\t';
          }
          for (int i = 0; i < 8; ++i) {
               printf("%x, ", it->ctxreg[i]);
          }
          printf("%x,%x,\n", it->raddr, it->waddr);
     }
}

void printTraceHuman(list<Inst> &L, string fname)
{
     FILE *ofp;
     ofp = fopen(fname.c_str(), "w");
     for (list<Inst>::iterator it = L.begin(); it != L.end(); ++it) {
          fprintf(ofp, "%s %s  \t", it->addr.c_str(), it->assembly.c_str());
          fprintf(ofp, "(%x, %x)\n", it->raddr, it->waddr);
     }
     fclose(ofp);
}

void printTraceLLSE(list<Inst> &L, string fname)
{
     FILE *ofp;
     ofp = fopen(fname.c_str(), "w");
     for (list<Inst>::iterator it = L.begin(); it != L.end(); ++it) {
          fprintf(ofp, "%s;%s;", it->addr.c_str(), it->assembly.c_str());
          for (int i = 0; i < 8; ++i)
               fprintf(ofp, "%x,", it->ctxreg[i]);
          fprintf(ofp, "%x,%x,\n", it->raddr, it->waddr);
     }
     fclose(ofp);
}
