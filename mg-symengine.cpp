#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <bitset>
#include <sstream>

using namespace std;

#include "core.hpp"
#include "mg-symengine.hpp"

enum ValueTy {SYMBOL, CONCRETE, HYBRID, UNKNOWN};
enum OperTy {ADD, MOV, SHL, XOR, SHR};
typedef pair<int,int> BitRange;

// A symbolic or concrete value in a formula
struct Value {
     int id;                             // a unique id for each value
     ValueTy valty;
     Operation *opr;
     string conval;                      // concrete value
     bitset<32> bsconval;                // concrete set stored as bitset
     BitRange brange;
     map<BitRange, Value*> childs;       // a list of child values in a hybrid value
     int len;                            // length of the value

     static int idseed;

     Value(ValueTy vty);
     Value(ValueTy vty, int l);
     Value(ValueTy vty, string con); // constructor for concrete value
     Value(ValueTy vty, string con, int l);
     Value(ValueTy vty, Operation *oper);
     Value(ValueTy vty, Operation *oper, int l);
     Value(ValueTy vty, bitset<32> bs);

     bool isSymbol();
     bool isConcrete();
     bool isHybrid();
};

int Value::idseed = 0;

Value::Value(ValueTy vty) : opr(NULL)
{
     id = ++idseed;
     valty = vty;
     len = 32;
}

Value::Value(ValueTy vty, int l) : opr(NULL)
{
     id = ++idseed;
     valty = vty;
     len = l;
}

Value::Value(ValueTy vty, string con) : opr(NULL),bsconval(stoul(con, 0, 16))
{
     id = ++idseed;
     valty = vty;
     conval = con;
     brange.first = 0;
     brange.second = 31;
     len = 32;
}

Value::Value(ValueTy vty, string con, int l) : opr(NULL)
{
     id = ++idseed;
     valty = vty;
     conval = con;
     len = l;
}

Value::Value(ValueTy vty, bitset<32> bs) : opr(NULL)
{
     id = ++idseed;
     valty = vty;
     bsconval = bs;
     len = 32;
}

Value::Value(ValueTy vty, Operation *oper)
{
     id = ++idseed;
     valty = vty;
     opr = oper;
     len = 32;
}

Value::Value(ValueTy vty, Operation *oper, int l)
{
     id = ++idseed;
     valty = vty;
     opr = oper;
     len = l;
}


bool Value::isSymbol()
{
     if (this->valty == SYMBOL)
          return true;
     else
          return false;
}

bool Value::isConcrete()
{
     if (this->valty == CONCRETE)
          return true;
     else
          return false;
}
bool Value::isHybrid()
{
     if (this->valty == HYBRID)
          return true;
     else
          return false;
}


string getValueName(Value *v)
{
     if (v->valty == SYMBOL)
          return "sym" + to_string(v->id);
     else
          return v->conval;
}

// An operation taking several values to calculate a result value
struct Operation {
     string opty;
     Value *val[3];

     Operation(string opt, Value *v1);
     Operation(string opt, Value *v1, Value *v2);
     Operation(string opt, Value *v1, Value *v2, Value *v3);
};

Operation::Operation(string opt, Value *v1)
{
     opty = opt;
     val[0] = v1;
     val[1] = NULL;
     val[2] = NULL;
}

Operation::Operation(string opt, Value *v1, Value *v2)
{
     opty = opt;
     val[0] = v1;
     val[1] = v2;
     val[2] = NULL;
}

Operation::Operation(string opt, Value *v1, Value *v2, Value *v3)
{
     opty = opt;
     val[0] = v1;
     val[1] = v2;
     val[2] = v3;
}

Value *buildop1(string opty, Value *v1)
{
     Operation *oper = new Operation(opty, v1);
     Value *result;

     if (v1->isSymbol())
          result = new Value(SYMBOL, oper);
     else
          result = new Value(CONCRETE, oper);

     return result;
}

Value *buildop2(string opty, Value *v1, Value *v2)
{
     Operation *oper = new Operation(opty, v1, v2);
     Value *result;
     if (v1->isSymbol() || v2->isSymbol())
          result = new Value(SYMBOL, oper);
     else
          result = new Value(CONCRETE, oper);

     return result;
}

// Currently there is no 3-operand operation,
// it is reserved for future.
Value *buildop3(string opty, Value *v1, Value *v2, Value *v3)
{
     Operation *oper = new Operation(opty, v1, v2, v3);
     Value *result;

     if (v1->isSymbol() || v2->isSymbol() || v3->isSymbol())
          result = new Value(SYMBOL, oper);
     else
          result = new Value(CONCRETE, oper);

     return result;
}


// ********************************
//  Class SEEngine Implementation
// ********************************

// return the concrete value in a register
ADDR32 SEEngine::getRegConVal(string reg)
{
     if (reg == "eax")
          return ip->ctxreg[0];
     else if (reg == "ebx")
          return ip->ctxreg[1];
     else if (reg == "ecx")
          return ip->ctxreg[2];
     else if (reg == "edx")
          return ip->ctxreg[3];
     else if (reg == "esi")
          return ip->ctxreg[4];
     else if (reg == "edi")
          return ip->ctxreg[5];
     else if (reg == "esp")
          return ip->ctxreg[6];
     else if (reg == "ebp")
          return ip->ctxreg[7];
     else {
          cerr << "now only get 32 bit register's concrete value." << endl;
          return 0;
     }
}

// Calculate the address in a memory operand
ADDR32 SEEngine::calcAddr(Operand *opr)
{
     ADDR32 r1, r2, c;        // addr = r1 + r2*n + c
     int32_t n;
     switch (opr->tag)
     {
     case 7:                    // addr7 = r1 + r2*n + c
          r1 = getRegConVal(opr->field[0]);
          r2 = getRegConVal(opr->field[1]);
          n  = stoi(opr->field[2]);
          c  = stoul(opr->field[4], 0, 16);
          if (opr->field[3] == "+")
               return r1 + r2*n + c;
          else if (opr->field[3] == "-")
               return r1 + r2*n - c;
          else {
               cerr << "unrecognized addr: tag 7" << endl;
               return 0;
          }
     case 4:                    // addr4 = r1 + c
          r1 = getRegConVal(opr->field[0]);
          c = stoul(opr->field[2], 0, 16);
          if (opr->field[1] == "+")
               return r1 + c;
          else if (opr->field[1] == "-")
               return r1 - c;
          else {
               cerr << "unrecognized addr: tag 4" << endl;
               return 0;
          }
     case 5:                    // addr5 = r1 + r2*n
          r1 = getRegConVal(opr->field[0]);
          r2 = getRegConVal(opr->field[1]);
          n  = stoi(opr->field[2]);
          return r1 + r2*n;
     case 6:                    // addr6 = r2*n + c
          r2 = getRegConVal(opr->field[0]);
          n = stoi(opr->field[1]);
          c = stoul(opr->field[3], 0, 16);
          if (opr->field[2] == "+")
               return r2*n + c;
          else if (opr->field[2] == "-")
               return r2*n - c;
          else {
               cerr << "unrecognized addr: tag 6" << endl;
               return 0;
          }
     case 3:                    // addr3 = r2*n
          r2 = getRegConVal(opr->field[0]);
          n = stoi(opr->field[1]);
          return r2*n;
     case 1:                    // addr1 = c
          c = stoul(opr->field[0], 0, 16);
          return c;
     case 2:                    // addr2 = r1
          r1 = getRegConVal(opr->field[0]);
          return r1;
     default:
          cerr << "unrecognized addr tag" << endl;
          return 0;
     }
}

bool hasVal(Value *v, int start, int end)
{
     BitRange br(start,end);
     map<BitRange, Value*>::iterator i = v->childs.find(br);
     if (i == v->childs.end())
          return false;
     else
          return true;
}

Value *readVal(Value *v, int start, int end)
{
     BitRange br(start,end);
     map<BitRange, Value*>::iterator i = v->childs.find(br);
     if (i == v->childs.end())
          return NULL;
     else
          return i->second;
}

string bs2str(bitset<32> bs, BitRange br){
     int start = br.first, end = br.second;
     unsigned int ui = 0, step = 1;
     for (int i = start; i <= end; ++i, step *= 2) {
          ui += bs[i] * step;
     }
     stringstream strs;
     strs << "0x" << hex << ui;   // the number of bits that should shift right
     string res(strs.str());

     return res;
}

Value *writeVal(Value *from, Value *to, int start, int end)
{
     Value *res = new Value(HYBRID);
     BitRange brfrom(start, end);
     if (to->isHybrid()) {
          map<BitRange, Value*>::iterator i = to->childs.find(brfrom);
          if (i != to->childs.end()) {
               i->second = from;
               return to;
          } else {
               cerr << "writeVal: no child in to match the from!" << endl;
               return NULL;
          }
     } else if (from->isSymbol() && to->isConcrete()) {
          int s1 = to->brange.first;
          int e1 = to->brange.second;
          Value *v1 = new Value(CONCRETE, to->bsconval);
          Value *v2 = new Value(CONCRETE, to->bsconval);
          v1->brange.first = s1;
          v1->brange.second = start-1;
          v1->conval = bs2str(v1->bsconval, v1->brange);

          v2->brange.first = end+1;
          v2->brange.second = e1;
          v2->conval = bs2str(v2->bsconval, v2->brange);

          res->childs.insert(pair<BitRange, Value*>(v1->brange, v1));
          res->childs.insert(pair<BitRange, Value*>(brfrom, from));
          res->childs.insert(pair<BitRange, Value*>(v2->brange, v2));


          return res;
     } else {
          cerr << "writeVal: the case is missing!" << endl;
          return NULL;
     }
}

Value* SEEngine::readReg(string &s)
{
     Value *res;
     if (s == "eax" || s == "ebx" || s == "ecx" || s == "edx" ||
         s == "esi" || s == "edi" || s == "esp" || s == "ebp") { // 32 bit reg
          return ctx[s];
     } else if (s == "ax" || s == "bx" || s == "cx" || s == "dx" ||
                s == "si" || s == "di" || s == "bp") {           // 16 bit reg
          if (hasVal(ctx["e"+s], 0, 15)) {         // already has a 16 bit value in the register
               res = readVal(ctx["e"+s], 0, 15);
               return res;
          } else {                                 // generate a 16-bit value from the 32-bit value in the register
               Value *v0 = ctx["e"+s];
               Value *v1 = new Value(CONCRETE, "0x0000ffff");
               res = buildop2("and", v0, v1);
               return res;
          }
     } else if (s == "al" || s == "bl" || s == "cl" || s == "dl") {
          string rname = 'e' + s.substr(0,1) + 'x';
          if (hasVal(ctx[rname], 0, 7)) {
               res = readVal(ctx[rname], 0, 7);
               return res;
          } else {
               Value *v0 = ctx[rname];
               Value *v1 = new Value(CONCRETE, "0x000000ff");
               res = buildop2("and", v0, v1);
               return res;
          }
     } else if (s == "ah" || s == "bh" || s == "ch" || s == "dh") {
          string rname = 'e' + s.substr(0,1) + 'x';
          if (hasVal(ctx[rname], 8, 15)) {
               res = readVal(ctx[rname], 8, 15);
               return res;
          } else {
               Value *v0 = ctx[rname];
               Value *v1 = new Value(CONCRETE, "0x0000ff00");
               Value *v2 = buildop2("and", v0, v1);
               Value *v3 = new Value(CONCRETE, "0x8");              // shift the value to the
               res = buildop2("shr", v2, v3);                       // most right position
               return res;
          }
     } else {
          cerr << "unknown reg name!" << endl;
          return NULL;
     }
}

void SEEngine::writeReg(string &s, Value *v)
{
     Value *res;
     if (s == "eax" || s == "ebx" || s == "ecx" || s == "edx" ||
         s == "esi" || s == "edi" || s == "esp" || s == "ebp") { // 32 bit reg
          ctx[s] = v;
     } else if (s == "ax" || s == "bx" || s == "cx" || s == "dx" ||
                s == "si" || s == "di" || s == "bp") {           // 16 bit reg
          Value *v0 = new Value(CONCRETE, "0xffff0000");         // mask the low 16 bits
          Value *v1 = ctx["e"+s];
          Value *v2 = buildop2("and", v1, v0);
          res = buildop2("or", v2, v);
          ctx["e"+s] = res;
     } else if (s == "al" || s == "bl" || s == "cl" || s == "dl") {
          string rname = 'e' + s.substr(0,1) + 'x';
          Value *v0 = new Value(CONCRETE, "0xffffff00");          // mask the low 8 bits
          Value *v1 = ctx[rname];
          Value *v2 = buildop2("and", v1, v0);
          res = buildop2("or", v2, v);
          ctx[rname] = res;
     } else if (s == "ah" || s == "bh" || s == "ch" || s == "dh") {
          string rname = 'e' + s.substr(0,1) + 'x';
          if (ctx[rname]->isConcrete() && v->isSymbol()) {
               ctx[rname] = writeVal(v, ctx[rname], 8, 15);
          } else {
               Value *v0 = new Value(CONCRETE, "0x8");                 // shift the written value
               Value *v1 = buildop2("shl", v, v0);                     // to the correct position
               Value *v2 = new Value(CONCRETE, "0xffff00ff");          // mask the high 8 bits
               Value *v3 = ctx[rname];
               Value *v4 = buildop2("and", v3, v2);
               res = buildop2("or", v4, v1);
               ctx[rname] = res;
          }
     } else {
          cerr << "unknown reg name!" << endl;
     }
}


// find whether ar is a subset of the AddrRanges in mem. If true, set
// the AddrRange of the superset
bool SEEngine::issubset(AddrRange ar, AddrRange *superset)
{
     for (map<AddrRange,Value*>::iterator it = mem.begin(); it != mem.end(); ++it) {
          AddrRange curar = it->first;
          if (curar.first <= ar.first && curar.second >= ar.second) {
               superset->first = curar.first;
               superset->second = curar.second;
               return true;
          }
     }
     return false;
}

// find whether ar is a superset of the AddrRanges in mem. If true, set
// the AddrRange of the subset
bool SEEngine::issuperset(AddrRange ar, AddrRange *subset)
{
     for (map<AddrRange,Value*>::iterator it = mem.begin(); it != mem.end(); ++it) {
          AddrRange curar = it->first;
          if (curar.first >= ar.first && curar.second <= ar.second) {
               subset->first = curar.first;
               subset->second = curar.second;
               return true;
          }
     }
     return false;
}

// check whether the address range exists in current memory. Return true if it is new
bool SEEngine::isnew(AddrRange ar)
{
     for (map<AddrRange,Value*>::iterator it = mem.begin(); it != mem.end(); ++it) {
          AddrRange curar = it->first;
          if ((curar.first <= ar.first && curar.second >= ar.first) ||
              (curar.first <= ar.second && curar.second >= ar.second)) {
               return false;
          }
     }
     return true;
}

Value* SEEngine::readMem(ADDR32 addr, int nbyte)
{
     ADDR32 end = addr + nbyte - 1;

     AddrRange ar(addr, end), res;

     if (memfind(ar)) return mem[ar];

     if (isnew(ar)) {
          Value *v = new Value(SYMBOL, nbyte);
          mem[ar] = v;
          meminput[v] = ar;
          return v;
     } else if (issubset(ar, &res)) {
          ADDR32 b1 = ar.first, e1 = ar.second;
          ADDR32 b2 = res.first, e2 = res.second;
          string mask = "0x";

          for (ADDR32 i = e2; i >= b2; --i) { // little endion: read from the most
               if (i >= b1 && i <= e1)          // significant position
                    mask += "ff";
               else
                    mask += "00";
          }
          stringstream strs;
          strs << "0x" << hex << (b1-b2) * 8;   // the number of bits that should shift right
          string low0(strs.str());

          Value *v0 = mem[res];
          Value *v1 = new Value(CONCRETE, mask);
          Value *v2 = buildop2("and", v0, v1);
          Value *v3 = new Value(CONCRETE, low0);
          Value *v4 = buildop2("shr", v2, v3); // shift right

          return v4;
     } else {
          cerr << "readMem: Partial overlapping symbolic memory access is not implemented yet!" << endl;
          return NULL;
     }
}

void SEEngine::writeMem(ADDR32 addr, int nbyte, Value *v)
{
     ADDR32 end = addr + nbyte -1;
     AddrRange ar(addr, end), res;

     if (memfind(ar) || isnew(ar)) {         // The old value is destroyed when the new AddrRange
          mem[ar] = v;                       // is equivalent to the old one, or is a new AddrRange
          return;
     } else if (issuperset(ar, &res)) { // or lager
          mem.erase(res);
          mem[ar] = v;
          return;
     } else if (issubset(ar, &res)) {
          ADDR32 b1 = ar.first, e1 = ar.second;
          ADDR32 b2 = res.first, e2 = res.second;

          string mask = "0x";  // mask used to set the bits to be writen in the old value as 0
          for (ADDR32 i = e2; i >= b2; --i) {
               if (i >= b1 && i <= e1)
                    mask += "00";
               else
                    mask += "ff";
          }

          stringstream strs;
          strs << "0x" << hex << (b1-b2) * 8;
          string low0(strs.str());   // the number of bits that should shift left

          Value *v0 = mem[res];
          Value *v1 = new Value(CONCRETE, mask);
          Value *v2 = buildop2("and", v0, v1);
          Value *v3 = new Value(CONCRETE, low0);
          Value *v4 = buildop2("shl", v, v3); // shift left the new value to the correct location
          Value *v5 = buildop2("or", v2, v4);
          mem[res] = v5;
          return;
     } else {
          cerr << "writeMem: Partial overlapping symbolic memory access is not implemented yet!" << endl;
     }
}

void SEEngine::init(Value *v1, Value *v2, Value *v3, Value *v4,
                    Value *v5, Value *v6, Value *v7, Value *v8,
                    list<Inst>::iterator it1,
                    list<Inst>::iterator it2)
{
     ctx["eax"] = v1;
     ctx["ebx"] = v2;
     ctx["ecx"] = v3;
     ctx["edx"] = v4;
     ctx["esi"] = v5;
     ctx["edi"] = v6;
     ctx["esp"] = v7;
     ctx["ebp"] = v8;

     reginput[v1] = "eax";
     reginput[v2] = "ebx";
     reginput[v3] = "ecx";
     reginput[v4] = "edx";
     reginput[v5] = "esi";
     reginput[v6] = "edi";
     reginput[v7] = "esp";
     reginput[v8] = "ebp";

     this->start = it1;
     this->end = it2;
}

void SEEngine::init(list<Inst>::iterator it1,
                    list<Inst>::iterator it2)
{
     this->start = it1;
     this->end = it2;
}

void SEEngine::initAllRegSymol(list<Inst>::iterator it1,
                               list<Inst>::iterator it2)
{
     Value *v1 = new Value(SYMBOL);
     Value *v2 = new Value(SYMBOL);
     Value *v3 = new Value(SYMBOL);
     Value *v4 = new Value(SYMBOL);
     Value *v5 = new Value(SYMBOL);
     Value *v6 = new Value(SYMBOL);
     Value *v7 = new Value(SYMBOL);
     Value *v8 = new Value(SYMBOL);

     ctx["eax"] = v1;
     ctx["ebx"] = v2;
     ctx["ecx"] = v3;
     ctx["edx"] = v4;
     ctx["esi"] = v5;
     ctx["edi"] = v6;
     ctx["esp"] = v7;
     ctx["ebp"] = v8;

     reginput[v1] = "eax";
     reginput[v2] = "ebx";
     reginput[v3] = "ecx";
     reginput[v4] = "edx";
     reginput[v5] = "esi";
     reginput[v6] = "edi";
     reginput[v7] = "esp";
     reginput[v8] = "ebp";

     start = it1;
     end = it2;
}

// instructions which have no effect in symbolic execution
set<string> noeffectinst = {"test","jmp","jz","jbe","jo","jno","js","jns","je","jne",
                            "jnz","jb","jnae","jc","jnb","jae",
                            "jnc","jna","ja","jnbe","jl",
                            "jnge","jge","jnl","jle","jng","jg",
                            "jnle","jp","jpe","jnp","jpo","jcxz",
                            "jecxz", "ret", "cmp", "call"};

int SEEngine::symexec()
{
     for (list<Inst>::iterator it = start; it != end; ++it) {
          // cout << hex << it->addrn << ": ";
          // cout << it->opcstr << '\n';

          // skip no effect instructions
          ip = it;
          if (noeffectinst.find(it->opcstr) != noeffectinst.end()) continue;

          switch (it->oprnum) {
          case 0:
               break;
          case 1:
          {
               Operand *op0 = it->oprd[0];
               Value *v0, *res, *temp;
               int nbyte;
               if (it->opcstr == "push") {
                    if (op0->ty == Operand::IMM) {
                         v0 = new Value(CONCRETE, op0->field[0]);
                         writeMem(it->waddr, 4, v0);
                    } else if (op0->ty == Operand::REG) {
                         nbyte = op0->bit / 8;
                         temp = readReg(op0->field[0]);
                         writeMem(it->waddr, nbyte, temp);
                    } else if (op0->ty == Operand::MEM) {
                         // The memaddr in the trace is the read address
                         // We need to compute the write address
                         // ADDR32 espval = it->ctxreg[6];
                         // *******************************
                         // New trace include the read/write memory addresses,
                         // so no need to get the esp value.
                         nbyte = op0->bit / 8;
                         v0 = readMem(it->raddr, nbyte);
                         writeMem(it->waddr, nbyte, v0);
                    } else {
                         cout << "push error: the operand is not Imm, Reg or Mem!" << endl;
                         return 1;
                    }
               } else if (it->opcstr == "pop") {
                    if (op0->ty == Operand::REG) {
                         nbyte = op0->bit / 8;
                         temp = readMem(it->raddr, nbyte);
                         writeReg(op0->field[0], temp);
                    } else if (op0->ty == Operand::MEM) {
                         nbyte = op0->bit / 8;
                         temp = readMem(it->raddr, nbyte);
                         // ADDR32 waddr = calcAddr(op0);
                         writeMem(it->waddr, nbyte, temp);
                    } else {
                         cout << "pop error: the operand is not Reg!" << endl;
                         return 1;
                    }
               } else {         // handle other one operand instructions
                    if (op0->ty == Operand::REG) {
                         v0 = readReg(op0->field[0]);
                         res = buildop1(it->opcstr, v0);
                         writeReg(op0->field[0], res);
                    } else if (op0->ty == Operand::MEM) {
                         nbyte = op0->bit / 8;
                         v0 = readMem(it->raddr, nbyte);
                         res = buildop1(it->opcstr, v0);
                         writeMem(it->waddr, nbyte, res);
                    } else {
                         cout << "[Error] Line " << it->id << ": Unknown 1 op instruction!"  << endl;
                         return 1;
                    }
               }
               break;
          }
          case 2:
          {
               Operand *op0 = it->oprd[0];
               Operand *op1 = it->oprd[1];
               Value *v0, *v1, *res, *temp;
               int nbyte;

               if (it->opcstr == "mov") { // handle mov instruction
                    if (op0->ty == Operand::REG) {
                         if (op1->ty == Operand::IMM) { // mov reg, 0x1111
                              v1 = new Value(CONCRETE, op1->field[0]);
                              writeReg(op0->field[0], v1);
                         } else if (op1->ty == Operand::REG) { // mov reg, reg
                              temp = readReg(op1->field[0]);
                              writeReg(op0->field[0], temp);
                         } else if (op1->ty == Operand::MEM) { // mov reg, dword ptr [ebp+0x1]
                              /* 1. Get mem address
                                 2. check whether the mem address has been accessed
                                 3. if not, create a new value
                                 4. else load the value in that memory
                               */
                              nbyte = op1->bit / 8;
                              v1 = readMem(it->raddr, nbyte);
                              writeReg(op0->field[0], v1);
                         } else {
                              cerr << "op1 is not ImmValue, Reg or Mem" << endl;
                              return 1;
                         }
                    } else if (op0->ty == Operand::MEM) {
                         if (op1->ty == Operand::IMM) { // mov dword ptr [ebp+0x1], 0x1111
                              temp = new Value(CONCRETE, op1->field[0]);
                              nbyte = op0->bit / 8;
                              writeMem(it->waddr, nbyte, temp);
                         } else if (op1->ty == Operand::REG) { // mov dword ptr [ebp+0x1], reg
                              temp = readReg(op1->field[0]);
                              nbyte = op0->bit / 8;
                              writeMem(it->waddr, nbyte, temp);
                         }
                    } else {
                         cerr << "Error: The first operand in MOV is not Reg or Mem!" << endl;
                    }
               } else if (it->opcstr == "lea") { // handle lea instruction
                    /* lea reg, ptr [edx+eax*1]
                       interpret lea instruction based on different address type
                       1. op0 must be reg
                       2. op1 must be addr
                     */
                    if (op0->ty != Operand::REG || op1->ty != Operand::MEM) {
                         cerr << "lea format error!" << endl;
                    }
                    switch (op1->tag) {
                    case 5:
                    {
                         Value *f0, *f1, *f2; // corresponding field[0-2] in operand
                         f0 = readReg(op1->field[0]);
                         f1 = readReg(op1->field[1]);
                         f2 = new Value(CONCRETE, op1->field[2]);
                         res = buildop2("imul", f1, f2);
                         res = buildop2("add",f0, res);
                         writeReg(op0->field[0], res);
                         break;
                    }
                    default:
                         cerr << "Other tags in addr is not ready for lea!" << endl;
                         break;
                    }
               } else if (it->opcstr == "xchg") {
                    if (op1->ty == Operand::REG) {
                         v1 = readReg(op1->field[0]);
                         if (op0->ty == Operand::REG) {
                              v0 = readReg(op0->field[0]);
                              writeReg(op1->field[0], v0);
                              writeReg(op0->field[0], v1);
                         } else if (op0->ty == Operand::MEM) {
                              nbyte = op0->bit / 8;
                              v0 = readMem(it->raddr, nbyte);
                              writeReg(op1->field[0], v0);
                              writeMem(it->waddr, nbyte, v1);
                         } else {
                              cerr << "xchg error: 1" << endl;
                         }
                    } else if (op1->ty == Operand::MEM) {
                         nbyte = op1->bit / 8;
                         v1 = readMem(it->raddr, nbyte);
                         if (op0->ty == Operand::REG) {
                              v0 = readReg(op0->field[0]);
                              writeReg(op0->field[0], v1);
                              writeMem(it->waddr, nbyte, v0);
                         } else {
                              cerr << "xchg error 3" << endl;
                         }
                    } else {
                         cerr << "xchg error: 2" << endl;
                    }
               } else if (it->opcstr == "shl") {
                    if (op0->ty == Operand::REG && op1->ty == Operand::IMM && readReg(op0->field[0])->isHybrid()) {
                         v0 = readReg(op0->field[0]);
                         int offset = (int)stoul(op1->field[0], 0, 16);
                         map<BitRange, Value*> newchilds;
                         for (map<BitRange, Value*>::iterator i = v0->childs.begin(); i != v0->childs.end(); ++i) {
                              Value *v = i->second;
                              if (v->valty == SYMBOL) {
                                   if (i->first.first != 0)
                                        v->brange.first = i->first.first + offset;
                                   v->brange.second = i->first.second + offset;
                                   if (v->brange.second > 31)
                                        v->brange.second = 31;
                              } else if (v->valty == CONCRETE) {
                                   v->bsconval <<= offset;
                                   if (v->brange.first != 0)
                                        v->brange.first += offset;
                                   v->brange.second += offset;
                                   if (v->brange.second > 31)
                                        v->brange.second = 31;
                                   v->conval = bs2str(v->bsconval, v->brange);
                              } else {
                                   cout << "shl: unknown value type" << endl;
                              }
                              newchilds.insert(pair<BitRange, Value*>(v->brange, v));
                         }
                         v0->childs = newchilds;
                    } else {   // other situations in shl

                         if (op1->ty == Operand::IMM) {
                              v1 = new Value(CONCRETE, op1->field[0]);
                         } else if (op1->ty == Operand::REG) {
                              v1 = readReg(op1->field[0]);
                         } else if (op1->ty == Operand::MEM) {
                              nbyte = op1->bit / 8;
                              v1 = readMem(it->raddr, nbyte);
                         } else {
                              cerr << "other instructions: op1 is not ImmValue, Reg, or Mem!" << endl;
                              return 1;
                         }

                         if (op0->ty == Operand::REG) { // dest op is reg
                              v0 = readReg(op0->field[0]);
                              res = buildop2(it->opcstr, v0, v1);
                              writeReg(op0->field[0], res);
                         } else if (op0->ty == Operand::MEM) { // dest op is mem
                              nbyte = op0->bit / 8;
                              v0 = readMem(it->raddr, nbyte);
                              res = buildop2(it->opcstr, v0, v1);
                              writeMem(it->waddr, nbyte, res);
                         } else {
                              cout << "other instructions: op2 is not ImmValue, Reg, or Mem!" << endl;
                              return 1;
                         }

                    }

               } else if (it->opcstr == "shr") {
                    if (op0->ty == Operand::REG && op1->ty == Operand::IMM && readReg(op0->field[0])->isHybrid()) {
                         v0 = readReg(op0->field[0]);
                         int offset = (int)stoul(op1->field[0], 0, 16);
                         for (map<BitRange, Value*>::iterator i = v0->childs.begin(); i != v0->childs.end(); ++i) {
                              Value *v = i->second;
                              if (v->valty == SYMBOL) {
                                   if (v->brange.second != 31)
                                        v->brange.second -= offset;
                                   v->brange.first -= offset;
                                   if (v->brange.first < 0) v->brange.first = 0;
                              } else if (v->valty == CONCRETE) {
                                   v->bsconval >>= offset;
                                   if (v->brange.second != 31)
                                        v->brange.second -= offset;
                                   v->brange.first -= offset;
                                   if (v->brange.first < 0) v->brange.first = 0;
                                   v->conval = bs2str(v->bsconval, v->brange);
                              } else {
                                   cout << "shr: unknown value type" << endl;
                              }
                         }
                    } else {   // other situations in shr

                         if (op1->ty == Operand::IMM) {
                              v1 = new Value(CONCRETE, op1->field[0]);
                         } else if (op1->ty == Operand::REG) {
                              v1 = readReg(op1->field[0]);
                         } else if (op1->ty == Operand::MEM) {
                              nbyte = op1->bit / 8;
                              v1 = readMem(it->raddr, nbyte);
                         } else {
                              cerr << "other instructions: op1 is not ImmValue, Reg, or Mem!" << endl;
                              return 1;
                         }

                         if (op0->ty == Operand::REG) { // dest op is reg
                              v0 = readReg(op0->field[0]);
                              res = buildop2(it->opcstr, v0, v1);
                              writeReg(op0->field[0], res);
                         } else if (op0->ty == Operand::MEM) { // dest op is mem
                              nbyte = op0->bit / 8;
                              v0 = readMem(it->raddr, nbyte);
                              res = buildop2(it->opcstr, v0, v1);
                              writeMem(it->waddr, nbyte, res);
                         } else {
                              cout << "other instructions: op2 is not ImmValue, Reg, or Mem!" << endl;
                              return 1;
                         }

                    }

               } else if (it->opcstr == "and") {
                    if (op0->ty == Operand::REG && op1->ty == Operand::IMM && readReg(op0->field[0])->isHybrid()) {
                         v0 = readReg(op0->field[0]);
                         v1 = new Value(CONCRETE, op1->field[0]);
                         bool status = true;
                         for (map<BitRange, Value*>::iterator it = v0->childs.begin(); it != v0->childs.end(); ++it) {
                              if (it->second->isSymbol()) {
                                   for (int i = it->first.first; i <= it->first.second; ++i) {
                                        if (v1->bsconval[i] == 1) status = false;
                                   }
                              }
                         }
                         if (status == true) {
                              Value *res = new Value(CONCRETE);
                              res->brange.first = 0;
                              res->brange.second = 31;
                              for (map<BitRange, Value*>::iterator it = v0->childs.begin(); it != v0->childs.end(); ++it) {
                                   if (it->second->isSymbol()) {
                                        for (int i = it->first.first; i <= it->first.second; ++i) {
                                             res->bsconval[i] = 0;
                                        }
                                   } else if (it->second->isConcrete()) {
                                        for (int i = it->first.first; i <= it->first.second; ++i) {
                                             res->bsconval[i] = it->second->bsconval[i] & v1->bsconval[i];
                                        }
                                   } else {
                                        cout << "unkown val type" << endl;
                                   }
                              }
                              res->conval = bs2str(res->bsconval, res->brange);
                              writeReg(op0->field[0], res);
                         }
                    } else {
                         if (op1->ty == Operand::IMM) {
                              v1 = new Value(CONCRETE, op1->field[0]);
                         } else if (op1->ty == Operand::REG) {
                              v1 = readReg(op1->field[0]);
                         } else if (op1->ty == Operand::MEM) {
                              nbyte = op1->bit / 8;
                              v1 = readMem(it->raddr, nbyte);
                         } else {
                              cerr << "other instructions: op1 is not ImmValue, Reg, or Mem!" << endl;
                              return 1;
                         }

                         if (op0->ty == Operand::REG) { // dest op is reg
                              v0 = readReg(op0->field[0]);
                              res = buildop2(it->opcstr, v0, v1);
                              writeReg(op0->field[0], res);
                         } else if (op0->ty == Operand::MEM) { // dest op is mem
                              nbyte = op0->bit / 8;
                              v0 = readMem(it->raddr, nbyte);
                              res = buildop2(it->opcstr, v0, v1);
                              writeMem(it->waddr, nbyte, res);
                         } else {
                              cout << "other instructions: op2 is not ImmValue, Reg, or Mem!" << endl;
                              return 1;
                         }

                    }
               } if (it->opcstr == "or") {
                    if (op0->ty == Operand::REG && op1->ty == Operand::IMM && readReg(op0->field[0])->isHybrid()) {
                         v0 = readReg(op0->field[0]);
                         v1 = new Value(CONCRETE, op1->field[0]);
                         bool status = true;
                         for (map<BitRange, Value*>::iterator it = v0->childs.begin(); it != v0->childs.end(); ++it) {
                              if (it->second->isSymbol()) {
                                   for (int i = it->first.first; i <= it->first.second; ++i) {
                                        if (v1->bsconval[i] == 0) status = false;
                                   }
                              }
                         }
                         if (status == true) {
                              Value *res = new Value(CONCRETE);
                              res->brange.first = 0;
                              res->brange.second = 31;
                              for (map<BitRange, Value*>::iterator it = v0->childs.begin(); it != v0->childs.end(); ++it) {
                                   if (it->second->isSymbol()) {
                                        for (int i = it->first.first; i <= it->first.second; ++i) {
                                             res->bsconval[i] = 1;
                                        }
                                   } else if (it->second->isConcrete()) {
                                        for (int i = it->first.first; i <= it->first.second; ++i) {
                                             res->bsconval[i] = it->second->bsconval[i] | v1->bsconval[i];
                                        }
                                   } else {
                                        cout << "unkown val type" << endl;
                                   }
                              }
                              res->conval = bs2str(res->bsconval, res->brange);
                              writeReg(op0->field[0], res);
                         }
                    } else {
                         if (op1->ty == Operand::IMM) {
                              v1 = new Value(CONCRETE, op1->field[0]);
                         } else if (op1->ty == Operand::REG) {
                              v1 = readReg(op1->field[0]);
                         } else if (op1->ty == Operand::MEM) {
                              nbyte = op1->bit / 8;
                              v1 = readMem(it->raddr, nbyte);
                         } else {
                              cerr << "other instructions: op1 is not ImmValue, Reg, or Mem!" << endl;
                              return 1;
                         }

                         if (op0->ty == Operand::REG) { // dest op is reg
                              v0 = readReg(op0->field[0]);
                              res = buildop2(it->opcstr, v0, v1);
                              writeReg(op0->field[0], res);
                         } else if (op0->ty == Operand::MEM) { // dest op is mem
                              nbyte = op0->bit / 8;
                              v0 = readMem(it->raddr, nbyte);
                              res = buildop2(it->opcstr, v0, v1);
                              writeMem(it->waddr, nbyte, res);
                         } else {
                              cout << "other instructions: op2 is not ImmValue, Reg, or Mem!" << endl;
                              return 1;
                         }

                    }
               } else { // handle other instructions
                    if (op1->ty == Operand::IMM) {
                         v1 = new Value(CONCRETE, op1->field[0]);
                    } else if (op1->ty == Operand::REG) {
                         v1 = readReg(op1->field[0]);
                    } else if (op1->ty == Operand::MEM) {
                         nbyte = op1->bit / 8;
                         v1 = readMem(it->raddr, nbyte);
                    } else {
                         cerr << "other instructions: op1 is not ImmValue, Reg, or Mem!" << endl;
                         return 1;
                    }

                    if (op0->ty == Operand::REG) { // dest op is reg
                         v0 = readReg(op0->field[0]);
                         res = buildop2(it->opcstr, v0, v1);
                         writeReg(op0->field[0], res);
                    } else if (op0->ty == Operand::MEM) { // dest op is mem
                         nbyte = op0->bit / 8;
                         v0 = readMem(it->raddr, nbyte);
                         res = buildop2(it->opcstr, v0, v1);
                         writeMem(it->waddr, nbyte, res);
                    } else {
                         cout << "other instructions: op2 is not ImmValue, Reg, or Mem!" << endl;
                         return 1;
                    }

               }

               break;
          }
          case 3:
          {
               Operand *op0 = it->oprd[0];
               Operand *op1 = it->oprd[1];
               Operand *op2 = it->oprd[2];
               Value *v1, *v2, *res;

               // three operands instructions are reduced to two operands
               if (it->opcstr == "imul" && op0->ty == Operand::REG &&
                   op1->ty == Operand::REG && op2->ty == Operand::IMM) { // imul reg, reg, imm
                    v1 = readReg(op1->field[0]);
                    v2 = new Value(CONCRETE, op2->field[0]);
                    res = buildop2(it->opcstr, v1, v2);
                    writeReg(op0->field[0], res);
               } else {
                    cerr << "three operands instructions other than imul are not handled!" << endl;
               }
               break;
          }
          default:
               cerr << "all instructions: number of operands is larger than 4!" << endl;
               break;
          }
     }
     return 0;
}

// recursively traverse every operands of v, print v as a parensises formula.
void traverse(Value *v)
{
     if (v == NULL) return;

     Operation *op = v->opr;
     if (op == NULL) {
          if (v->valty == CONCRETE)
               cout << v->conval;
          else
               cout << "sym" << v->id;
     } else {
          cout << "(" << op->opty << " ";
          traverse(op->val[0]);
          cout << " ";
          traverse(op->val[1]);
          cout << ")";
     }
}

// traverse for showing SE result, not a strict CVC formula.
void traverse2(Value *v)
{
     if (v == NULL) return;

     Operation *op = v->opr;
     if (op == NULL) {
          if (v->valty == CONCRETE)
               cout << v->conval;
          else if (v->valty == SYMBOL)
               cout << "sym" << v->id << " ";
          else if (v->valty == HYBRID) {
               cout << "[" << "hyb" << v->id << " ";
               for (map<BitRange, Value*>::iterator i = v->childs.begin(); i != v->childs.end(); ++i) {
                    cout << "[" << i->first.first << "," << i->first.second << "]:";
                    traverse2(i->second);
               }
               cout << "]";
          } else {
               cout << "unknown type" << endl;
               return;
          }
     } else {
          cout << "(" << op->opty << " ";
          traverse2(op->val[0]);
          cout << " ";
          traverse2(op->val[1]);
          cout << ")";
     }
}


void SEEngine::outputFormula(string reg)
{
     Value *v = ctx[reg];
     cout << "sym" << v->id << "=" << endl;
     traverse(v);
     cout << endl;
}

void SEEngine::dumpreg(string reg)
{
     Value *v = ctx[reg];
     cout << "reg " << reg << "=" << endl;
     traverse2(v);
     cout << endl;
}


vector<Value*> SEEngine::getAllOutput()
{
     vector<Value*> outputs;
     Value *v;

     // symbols in registers
     v = ctx["eax"];
     if (v->opr != NULL)
          outputs.push_back(v);
     v = ctx["ebx"];
     if (v->opr != NULL)
          outputs.push_back(v);
     v = ctx["ecx"];
     if (v->opr != NULL)
          outputs.push_back(v);
     v = ctx["edx"];
     if (v->opr != NULL)
          outputs.push_back(v);

     // symbols in memory
     for (auto const& x : mem) {
          v = x.second;
          if (v->opr != NULL)
               outputs.push_back(v);
     }

     return outputs;
}

void SEEngine::printAllRegFormulas()
{
     cout << "eax: ";
     outputFormula("eax");
     printInputSymbols("eax");
     cout << endl;

     cout << "ebx: ";
     outputFormula("ebx");
     printInputSymbols("ebx");
     cout << endl;

     cout << "ecx: ";
     outputFormula("ecx");
     printInputSymbols("ecx");
     cout << endl;

     cout << "edx: ";
     outputFormula("edx");
     printInputSymbols("edx");
     cout << endl;

     cout << "esi: ";
     outputFormula("esi");
     printInputSymbols("esi");
     cout << endl;

     cout << "edi: ";
     outputFormula("edi");
     printInputSymbols("edi");
     cout << endl;
}

void SEEngine::printAllMemFormulas()
{
     for (auto const& x : mem) {
          AddrRange ar = x.first;
          Value *v = x.second;
          printf("[%x,%x]: ", ar.first, ar.second);
          cout << "sym" << v->id << "=" << endl;
          traverse(v);
          cout << endl << endl;
     }
}

set<Value*> *getInputs(Value *output)
{
     queue<Value*> que;
     que.push(output);

     set<Value*> *inputset = new set<Value*>;

     while (!que.empty()) {
          Value *v = que.front();
          Operation *op = v->opr;
          que.pop();

          if(op == NULL) {
               if (v->valty == SYMBOL)
                    inputset->insert(v);
          } else {
               for (int i = 0; i < 3; ++i) {
                    if (op->val[i] != NULL) que.push(op->val[i]);
               }
          }
     }

     return inputset;
}

void SEEngine::printInputSymbols(string output)
{
     Value *v = ctx[output];
     set<Value*> *insyms = getInputs(v);

     cout << insyms->size() << " input symbols: ";
     for (set<Value*>::iterator it = insyms->begin(); it != insyms->end(); ++it) {
          cout << "sym" << (*it)->id << " ";
     }
     cout << endl;
}

// print the complete information of v, including:
// 1. A formula of the whole computation from inputs to v
// 2. The inputs location: register or addrange
void SEEngine::printformula(Value *v)
{
     set<Value*> *insyms = getInputs(v);

     cout << insyms->size() << " input symbols: " << endl;
     for (set<Value*>::iterator it = insyms->begin(); it != insyms->end(); ++it) {
          cout << "sym" << (*it)->id << ": ";
          map<Value*, AddrRange>::iterator it1 = meminput.find(*it);
          if (it1 != meminput.end()) {
               printf("[%x, %x]\n", it1->second.first, it1->second.second);
          } else {
               map<Value*, string>::iterator it2 = reginput.find(*it);
               if (it2 != reginput.end()) {
                    cout << it2->second << endl;
               }
          }
     }
     cout << endl;

     cout << "sym" << v->id << "=" << endl;
     traverse(v);
     cout << endl;
}

void SEEngine::printMemFormula(ADDR32 addr1, ADDR32 addr2)
{
     AddrRange ar(addr1, addr2);
     Value *v = mem[ar];

     printformula(v);
}

// Recursively evaluate a Value
ADDR32 eval(Value *v, map<Value*, ADDR32> *inmap)
{
     Operation *op = v->opr;
     if (op == NULL) {
          if (v->valty == CONCRETE)
               return stoul(v->conval, 0, 16);
          else
               return (*inmap)[v];
     } else {
          ADDR32 op0, op1;
          // ADDR32 op2;

          if (op->val[0] != NULL) op0 = eval(op->val[0], inmap);
          if (op->val[1] != NULL) op1 = eval(op->val[1], inmap);
          // if (op->val[2] != NULL) op2 = eval(op->val[2], inmap);

          if (op->opty == "add") {
               return op0 + op1;
          } else if (op->opty == "sub") {
               return op0 - op1;
          } else if (op->opty == "imul") {
               return op0 * op1;
          } else if (op->opty == "xor") {
               return op0 ^ op1;
          } else if (op->opty == "and") {
               return op0 & op1;
          } else if (op->opty == "or") {
               return op0 | op1;
          } else if (op->opty == "shl") {
               return op0 << op1;
          } else if (op->opty == "shr") {
               return op0 >> op1;
          } else if (op->opty == "neg") {
               return ~op0 + 1;
          } else if (op->opty == "inc") {
               return op0 + 1;
          } else {
               cout << "Instruction: " << op->opty << "is not interpreted!" << endl;
               return 1;
          }
     }
}

// Given inputs, concrete compute the output value of a formula
ADDR32 SEEngine::conexec(Value *f, map<Value*, ADDR32> *inmap)
{
     set<Value*> *inputsym = getInputs(f);
     set<Value*> inmapkeys;
     for (map<Value*, ADDR32>::iterator it = inmap->begin(); it != inmap->end(); ++it) {
          inmapkeys.insert(it->first);
     }

     if (inmapkeys != *inputsym) {
          cout << "Some inputs don't have parameters!" << endl;
          return 1;
     }

     return eval(f, inmap);
}

// build a map based on a var vector and a input vector
map<Value*, ADDR32> buildinmap(vector<Value*> *vv, vector<ADDR32> *input)
{
     map<Value*, ADDR32> inmap;
     if (vv->size() != input->size()) {
          cout << "number of input symbols is wrong!" << endl;
          return inmap;
     }

     for (int i = 0, max = vv->size(); i < max; ++i) {
          inmap.insert(pair<Value*,ADDR32>((*vv)[i], (*input)[i]));
     }

     return inmap;
}

// get all inputs of formula f and push them into a vector
vector<Value*> getInputVector(Value *f)
{
     set<Value*> *inset = getInputs(f);

     vector<Value*> vv;
     for (set<Value*>::iterator it = inset->begin(); it != inset->end(); ++it) {
          vv.push_back(*it);
     }

     return vv;
}


// a post fix in symbol names to identify they are coming from different formulas
string sympostfix;

// output the calculation of v as a formula
void outputCVC(Value *v, FILE *fp)
{
     if (v == NULL) return;

     Operation *op = v->opr;
     if (op == NULL) {
          if (v->valty == CONCRETE) {
               ADDR32 i = stoul(v->conval, 0, 16);
               fprintf(fp, "0hex%08x", i);
          } else
               fprintf(fp, "sym%d%s", v->id, sympostfix.c_str());
     } else {
          if (op->opty == "add") {
               fprintf(fp, "BVPLUS(32, ");
               outputCVC(op->val[0], fp);
               fprintf(fp, ", ");
               outputCVC(op->val[1], fp);
               fprintf(fp, ")");
          } else if (op->opty == "sub") {
               fprintf(fp, "BVSUB(32, ");
               outputCVC(op->val[0], fp);
               fprintf(fp, ", ");
               outputCVC(op->val[1], fp);
               fprintf(fp, ")");
          } else if (op->opty == "imul") {
               fprintf(fp, "BVMULT(32, ");
               outputCVC(op->val[0], fp);
               fprintf(fp, ", ");
               outputCVC(op->val[1], fp);
               fprintf(fp, ")");
          } else if (op->opty == "xor") {
               fprintf(fp, "BVXOR(");
               outputCVC(op->val[0], fp);
               fprintf(fp, ", ");
               outputCVC(op->val[1], fp);
               fprintf(fp, ")");
          } else if (op->opty == "and") {
               outputCVC(op->val[0], fp);
               fprintf(fp, " & ");
               outputCVC(op->val[1], fp);
          } else if (op->opty == "or") {
               outputCVC(op->val[0], fp);
               fprintf(fp, " | ");
               outputCVC(op->val[1], fp);
          } else if (op->opty == "neg") {
               fprintf(fp, "~");
               outputCVC(op->val[0], fp);
          } else if (op->opty == "shl") {
               outputCVC(op->val[0], fp);
               fprintf(fp, " << ");
               outputCVC(op->val[1], fp);
          } else if (op->opty == "shr") {
               outputCVC(op->val[0], fp);
               fprintf(fp, " >> ");
               outputCVC(op->val[1], fp);
          } else {
               cout << "Instruction: " << op->opty << " is not interpreted in CVC!" << endl;
               return;
          }
     }
}

void outputCVCFormula(Value *f)
{
     const char *cvcfile = "formula.cvc";
     FILE *fp = fopen(cvcfile, "w");

     outputCVC(f, fp);

     fclose(fp);
}

// output a CVC formula to check the equivalence of f1 and f2 using the variable mapping in m
void outputChkEqCVC(Value *f1, Value *f2, map<int,int> *m)
{
     const char *cvcfile = "ChkEq.cvc";
     FILE *fp = fopen(cvcfile, "w");

     for (map<int,int>::iterator it = m->begin(); it != m->end(); ++it) {
          fprintf(fp, "sym%da: BV(32);\n", it->first);
          fprintf(fp, "sym%db: BV(32);\n", it->second);
     }
     fprintf(fp, "\n");

     for (map<int,int>::iterator it = m->begin(); it != m->end(); ++it) {
          fprintf(fp, "ASSERT(sym%da = sym%db);\n", it->first, it->second);
     }

     fprintf(fp, "\nQUERY(\n");
     sympostfix = "a";
     outputCVC(f1, fp);
     fprintf(fp, "\n=\n");
     sympostfix = "b";
     outputCVC(f2, fp);
     fprintf(fp, ");\n");
     fprintf(fp, "COUNTEREXAMPLE;\n");
}

// output all bit formulas based on the result of variable mapping
void outputBitCVC(Value *f1, Value *f2, vector<Value*> *inv1, vector<Value*> *inv2,
                            list<FullMap> *result)
{
     for (list<FullMap>::iterator it = result->begin(); it != result->end(); ++it) {
          int n = 1;
          string cvcfile = "formula" + to_string(n++) + ".cvc";
          FILE *fp = fopen(cvcfile.c_str(), "w");

          map<int,int> *inmap = &(it->first);
          map<int,int> *outmap = &(it->second);

          // output bit values of the inputs;
          for (int i = 0, max = 32 * inv1->size(); i < max; ++i) {
               fprintf(fp, "bit%da: BV(1);\n", i);
          }
          for (int i = 0, max = 32 * inv2->size(); i < max; ++i) {
               fprintf(fp, "bit%db: BV(1);\n", i);
          }

          // output inputs mapping
          for (map<int,int>::iterator it = inmap->begin(); it != inmap->end(); ++it) {
               fprintf(fp, "ASSERT(bit%da = bit%db);\n", it->first, it->second);
          }
          fprintf(fp, "\n");


          fprintf(fp, "\nQUERY(\n");

          // concatenate bits into a variable in the formula
          for (int i = 0, max = inv1->size(); i < max; ++i) {
               fprintf(fp, "LET %sa = ", getValueName((*inv1)[i]).c_str());
               for (int j = 0; j < 31; ++j) {
                    fprintf(fp, "bit%da@", i*32+j);
               }
               fprintf(fp, "bit%da IN (\n", i+31);
          }
          for (int i = 0, max = inv2->size(); i < max; ++i) {
               fprintf(fp, "LET %sb = ", getValueName((*inv2)[i]).c_str());
               for (int j = 0; j < 31; ++j) {
                    fprintf(fp, "bit%db@", i*32+j);
               }
               fprintf(fp, "bit%db IN (\n", i+31);
          }

          fprintf(fp, "LET out1 = ");
          sympostfix = "a";
          outputCVC(f1, fp);
          fprintf(fp, " IN (\n");
          fprintf(fp, "LET out2 = ");
          sympostfix = "b";
          outputCVC(f2, fp);
          fprintf(fp, " IN (\n");

          for (map<int,int>::iterator it = outmap->begin(); it != outmap->end(); ++it) {
               if (next(it,1) != outmap->end())
                    fprintf(fp, "out1[%d:%d] = out2[%d:%d] AND\n", it->first, it->first, it->second, it->second);
               else
                    fprintf(fp, "out1[%d:%d] = out2[%d:%d]\n", it->first, it->first, it->second, it->second);
          }

          for (int i = 0, n = inv1->size() + inv2->size(); i < n; ++i) {
               fprintf(fp, ")");
          }
          fprintf(fp, ")));\n");
          fprintf(fp, "COUNTEREXAMPLE;");

          fclose(fp);
     }
}

void SEEngine::showMemInput()
{
     cout << "Inputs in memory:" << endl;
     for (map<Value*,AddrRange>::iterator it = meminput.begin(); it != meminput.end(); ++it) {
          printf("sym%d: ", it->first->id);
          printf("[%x, %x]\n", it->second.first, it->second.second);
     }
     cout << endl;
}
