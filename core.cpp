#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace std;

#include "core.hpp"

bool Parameter::operator==(const Parameter& other)
{
     if (ty == other.ty) {
          switch (ty) {
          case IMM:
               if (idx == other.idx)
                    return true;
               else
                    return false;
          case REG:
               if (reg == other.reg && idx == other.idx)
                    return true;
               else
                    return false;
          case MEM:
               if (idx == other.idx)
                    return true;
               else
                    return false;
          default:
               return false;
          }
     } else {
          return false;
     }
}

bool Parameter::operator<(const Parameter& other) const
{
     if (ty < other.ty)
          return true;
     else if (ty > other.ty)
          return false;
     else {
          switch (ty) {
          case Parameter::IMM:
               if (idx < other.idx)
                    return true;
               else
                    return false;
          case Parameter::REG:
               if (reg < other.reg)
                    return true;
               else if (reg > other.reg)
                    return false;
               else {
                    if (idx < other.idx)
                         return true;
                    else
                         return false;
               }
          case Parameter::MEM:
               if (idx < other.idx)
                    return true;
               else
                    return false;
          default:
               return true;
          }
     }
}

bool Parameter::isIMM()
{
     if (ty == IMM)
          return true;
     else
          return false;
}

string reg2string(Register reg) {
     switch (reg) {
     case EAX:
          return string("eax");
     case EBX:
          return string("ebx");
     case ECX:
          return string("ecx");
     case EDX:
          return string("edx");
     case ESI:
          return string("esi");
     case EDI:
          return string("edi");
     case ESP:
          return string("esp");
     case EBP:
          return string("ebp");
     default:
          return string("unknown");
     }
}

void Parameter::show() const
{
     if (ty == Parameter::IMM) {
          printf("(IMM ");
          printf("0x%x) ", idx);
     } else if (ty == Parameter::REG) {
          cout << "(REG ";
          cout << reg2string(reg) << ") ";
     } else if (ty == Parameter::MEM) {
          cout << "(MEM ";
          printf("%x) ", idx);
     } else {
          cout << "Parameter show() error: unkonwn src type." << endl;
          return;
     }
}

bool isReg32(string reg)
{
     if (reg == "eax" || reg == "ebx" || reg == "ecx" || reg == "edx" ||
         reg == "esi" || reg == "edi" || reg == "esp" || reg == "ebp" ||
         reg == "st0" || reg == "st1" || reg == "st2" || reg == "st3" ||
         reg == "st4" || reg == "st5") {
          return true;
     } else {
          return false;
     }
}

bool isReg16(string reg)
{
     if (reg == "ax" || reg == "bx" || reg == "cx" || reg == "dx" ||
         reg == "si" || reg == "di" || reg == "bp" || reg == "cs" ||
         reg == "ds" || reg == "es" || reg == "fs" || reg == "gs" ||
         reg == "ss") {
          return true;
     } else {
          return false;
     }
}

bool isReg8(string reg)
{
     if (reg == "al" || reg == "ah" || reg == "bl" || reg == "bh" ||
         reg == "cl" || reg == "ch" || reg == "dl" || reg == "dh") {
          return true;
     } else {
          return false;
     }
}

Register getRegParameter(string regname, vector<int> &idx)
{
     if (isReg32(regname)) {
          idx.push_back(0);
          idx.push_back(1);
          idx.push_back(2);
          idx.push_back(3);

          if (regname == "eax") {
               return EAX;
          } else if (regname == "ebx") {
               return EBX;
          } else if (regname == "ecx") {
               return ECX;
          } else if (regname == "edx") {
               return EDX;
          } else if (regname == "esi") {
               return ESI;
          } else if (regname == "edi") {
               return EDI;
          } else if (regname == "esp") {
               return ESP;
          } else if (regname == "ebp") {
               return EBP;
          } else {
               cout << "Unknown 32 bit reg: " << regname << endl;
               return UNK;
          }
     } else if (isReg16(regname)) {
          idx.push_back(0);
          idx.push_back(1);

          if (regname == "ax") {
               return EAX;
          } else if (regname == "bx") {
               return EBX;
          } else if (regname == "cx") {
               return ECX;
          } else if (regname == "dx") {
               return EDX;
          } else if (regname == "si") {
               return ESI;
          } else if (regname == "di") {
               return EDI;
          } else if (regname == "bp") {
               return EBP;
          } else {
               cout << "Unknown 16 bit reg: " << regname << endl;
               return UNK;
          }
     } else if (regname == "al" || regname == "bl" ||
                regname == "cl" || regname == "dl") {
          idx.push_back(0);

          if (regname == "al") {
               return EAX;
          } else if (regname == "bl") {
               return EBX;
          } else if (regname == "cl") {
               return ECX;
          } else if (regname == "dl") {
               return EDX;
          } else {
               cout << "Unknown 8 bit reg: " << regname << endl;
               return UNK;
          }
     } else if (regname == "ah" || regname == "bh" ||
                regname == "ch" || regname == "dh") {
          idx.push_back(1);

          if (regname == "ah") {
               return EAX;
          } else if (regname == "bh") {
               return EBX;
          } else if (regname == "ch") {
               return ECX;
          } else if (regname == "dh") {
               return EDX;
          } else {
               cout << "Unknown 8 bit reg: " << regname << endl;
               return UNK;
          }
     } else {
          cout << "Unknown reg" << endl;
          return UNK;
     }
}

void Inst::addsrc(Parameter::Type t, string s)
{
     if (t == Parameter::IMM) {
          Parameter p;
          p.ty = t;
          p.idx = stoul(s, 0, 16);
          src.push_back(p);
     } else if (t == Parameter::REG) {
          vector<int> v;
          Register r = getRegParameter(s, v);
          for (int i = 0, max = v.size(); i < max; ++i) {
               Parameter p;
               p.ty = t;
               p.reg = r;
               p.idx = v[i];
               src.push_back(p);
          }
     } else {
          cout << "addsrc error!" << endl;
     }
}

void Inst::addsrc(Parameter::Type t, AddrRange a)
{
     for (ADDR32 i = a.first; i <= a.second; ++i) {
          Parameter p;
          p.ty = t;
          p.idx = i;
          src.push_back(p);
     }
}

void Inst::adddst(Parameter::Type t, string s)
{
     if (t == Parameter::REG) {
          vector<int> v;
          Register r = getRegParameter(s, v);
          for (int i = 0, max = v.size(); i < max; ++i) {
               Parameter p;
               p.ty = t;
               p.reg = r;
               p.idx = v[i];
               dst.push_back(p);
          }
     } else {
          cout << "adddst error!" << endl;
     }
}

void Inst::adddst(Parameter::Type t, AddrRange a)
{
     for (ADDR32 i = a.first; i <= a.second; ++i) {
          Parameter p;
          p.ty = t;
          p.idx = i;
          dst.push_back(p);
     }
}

void Inst::addsrc2(Parameter::Type t, string s)
{
     if (t == Parameter::IMM) {
          Parameter p;
          p.ty = t;
          p.idx = stoul(s, 0, 16);
          src2.push_back(p);
     } else if (t == Parameter::REG) {
          vector<int> v;
          Register r = getRegParameter(s, v);
          for (int i = 0, max = v.size(); i < max; ++i) {
               Parameter p;
               p.ty = t;
               p.reg = r;
               p.idx = v[i];
               src2.push_back(p);
          }
     } else {
          cout << "addsrc2 error!" << endl;
     }
}

void Inst::addsrc2(Parameter::Type t, AddrRange a)
{
     for (ADDR32 i = a.first; i <= a.second; ++i) {
          Parameter p;
          p.ty = t;
          p.idx = i;
          src2.push_back(p);
     }
}

void Inst::adddst2(Parameter::Type t, string s)
{
     if (t == Parameter::REG) {
          vector<int> v;
          Register r = getRegParameter(s, v);
          for (int i = 0, max = v.size(); i < max; ++i) {
               Parameter p;
               p.ty = t;
               p.reg = r;
               p.idx = v[i];
               dst2.push_back(p);
          }
     } else {
          cout << "adddst error!" << endl;
     }
}

void Inst::adddst2(Parameter::Type t, AddrRange a)
{
     for (ADDR32 i = a.first; i <= a.second; ++i) {
          Parameter p;
          p.ty = t;
          p.idx = i;
          dst2.push_back(p);
     }
}
