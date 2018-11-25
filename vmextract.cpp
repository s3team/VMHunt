#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <map>
#include <stack>
#include <vector>
#include <set>

using namespace std;

#include "core.hpp"
#include "parser.hpp"

list<Inst> instlist;

// Data structures for identify functions
struct FuncBody {
     int start;
     int end;
     int length;                // end - start
     unsigned int startAddr;
     unsigned int endAddr;
     int loopn;
};

struct Func {
     unsigned int callAddr;              // same as the startAddr in FuncBody
     list<struct FuncBody *> body;
};


// jmp instruction names in x86 assembly
string jmpInstrName[33] = {"jo","jno","js","jns","je","jz","jne",
                           "jnz","jb","jnae","jc","jnb","jae",
                           "jnc","jbe","jna","ja","jnbe","jl",
                           "jnge","jge","jnl","jle","jng","jg",
                           "jnle","jp","jpe","jnp","jpo","jcxz",
                           "jecxz", "jmp"};


set<int> *jmpset;        // jmp instructions
map<string, int> *instenum;     // instruction enumerations

string getOpcName(int opc, map<string, int> *m)
{
     for (map<string, int>::iterator it = m->begin(); it != m->end(); ++it) {
          if (it->second == opc)
               return it->first;
     }

     return "unknown";
}

void printInstlist(list<Inst> *L, map<string, int> *m)
{
     for (list<Inst>::iterator it = L->begin(); it != L->end(); ++it) {
          cout << it->id << ' ';
          cout << hex << it->addrn << ' ';
          cout << it->addr << ' ';
          cout << it->opcstr << ' ';
          cout << getOpcName(it->opc, m) << ' ';
          cout << it->oprnum << endl;
          for (vector<string>::iterator ii = it->oprs.begin(); ii != it->oprs.end(); ++ii) {
               cout << *ii << endl;
          }
     }
}


map<unsigned int, list<FuncBody *> *> *
buildFuncList(list<Inst> *L)
{
     map<unsigned int, list<FuncBody *> *> *funcmap =
          new map<unsigned int, list<FuncBody *> *>;
     // list<Func> *funclist = new list<Func>;
     stack<list<Inst>::iterator> stk;


     for (list<Inst>::iterator it = L->begin(); it != L->end(); ++it) {
          // parse the whole instlist to build funclist

          if (it->opcstr == "call") {
               stk.push(it);
               // search whether the function is in the function list
               // if yes, identify whether it is a new function instance
               // if not, create a new function
               map<unsigned int, list<FuncBody *> *>::iterator i = funcmap->find(it->addrn);
               if (i == funcmap->end()) {
                    unsigned int calladdr = stoul(it->oprs[0], nullptr, 16);
                    funcmap->insert(pair<unsigned int, list<FuncBody *> *>(calladdr, NULL));
               }
          } else if (it->opcstr == "ret") {
               if (!stk.empty()) stk.pop();
          } else {}
     }

     return funcmap;
}

void printFuncmap(map<unsigned int, list<FuncBody *> *> *funcmap)
{
     map<unsigned int, list<FuncBody *> *>::iterator it;
     for (it = funcmap->begin(); it != funcmap->end(); ++it) {
          cout << hex << it->first << endl;
     }
}

map<string, int> *buildOpcodeMap(list<Inst> *L)
{
     map<string, int> *instenum = new map<string, int>;
     for (list<Inst>::iterator it = L->begin(); it != L->end(); ++it) {
          if (instenum->find(it->opcstr) == instenum->end())
               instenum->insert(pair<string, int>(it->opcstr, instenum->size()+1));
     }

     return instenum;
}

int getOpc(string s, map<string, int> *m)
{
     map<string, int>::iterator it = m->find(s);
     if (it != m->end())
          return it->second;
     else
          return 0;
}

bool isjump(int i, set<int> *jumpset)
{
     set<int>::iterator it = jumpset->find(i);
     if (it == jumpset->end())
          return false;
     else
          return true;
}


void countindjumps(list<Inst> *L) {
     int indjumpnum = 0;
     for (list<Inst>::iterator it = L->begin(); it != L->end(); ++it) {
          if (isjump(it->opc, jmpset) && it->oprd[0]->ty != Operand::IMM) {
               ++indjumpnum;
               cout << it->addr << "\t" << it->opcstr << " " << it->oprs[0] << endl;
          }
     }
     cout << "number of indirect jumps: " << indjumpnum << endl;
}

void peephole(list<Inst> *L)
{
     for (list<Inst>::iterator it = L->begin(); it != L->end(); ++it) {
          if ((it->opcstr == "pushad" && next(it,1)->opcstr == "popad") ||
              (it->opcstr == "popad" && next(it,1)->opcstr == "pushad") ||
              (it->opcstr == "push" && next(it,1)->opcstr == "pop" && it->oprs[0] == next(it,1)->oprs[0]) ||
              (it->opcstr == "pop" && next(it,1)->opcstr == "push" && it->oprs[0] == next(it,1)->oprs[0]) ||
              (it->opcstr == "add" && next(it,1)->opcstr == "sub" && it->oprs[0] == next(it,1)->oprs[0] && it->oprs[1] == next(it,1)->oprs[1]) ||
              (it->opcstr == "sub" && next(it,1)->opcstr == "add" && it->oprs[0] == next(it,1)->oprs[0] && it->oprs[1] == next(it,1)->oprs[1]) ||
              (it->opcstr == "inc" && next(it,1)->opcstr == "dec" && it->oprs[0] == next(it,1)->oprs[0]) ||
              (it->opcstr == "dec" && next(it,1)->opcstr == "inc" && it->oprs[0] == next(it,1)->oprs[0]) ) {
               it = L->erase(it);
               it = L->erase(it);
               continue;
          }
     }
}

struct ctxswitch {
     list<Inst>::iterator begin;
     list<Inst>::iterator end;
     ADDR32 sd;         // stack depth
};

list<ctxswitch> ctxsave;                  // context save instructions
list<ctxswitch> ctxrestore;               // context restore instructions
list<pair<ctxswitch, ctxswitch> > ctxswh;            // paired context switch instructions

bool isreg(string s)
{
     if (s == "eax" || s == "ebx" || s == "ecx" || s == "edx" ||
         s == "esi" || s == "edi" || s == "ebp") {
          return true;
     } else {
          return false;
     }
}

bool chkpush(list<Inst>::iterator i1, list<Inst>::iterator i2)
{
     int opcpush = getOpc("push", instenum);
     for (list<Inst>::iterator it = i1; it != i2; ++it) {
          if (it->opc != opcpush || !isreg(it->oprs[0]))
               return false;
     }
     set<string> opcs;
     for (list<Inst>::iterator it = i1; it != i2; ++it) {
          if (opcs.find(it->oprs[0]) == opcs.end())
               opcs.insert(it->oprs[0]);
          else
               return false;
     }
     return true;
}

bool chkpop(list<Inst>::iterator i1, list<Inst>::iterator i2)
{
     int opcpop = getOpc("pop", instenum);
     for (list<Inst>::iterator it = i1; it != i2; ++it) {
          if (it->opc != opcpop || !isreg(it->oprs[0]))
               return false;
     }
     set<string> opcs;
     for (list<Inst>::iterator it = i1; it != i2; ++it) {
          if (opcs.find(it->oprs[0]) == opcs.end())
               opcs.insert(it->oprs[0]);
          else
               return false;
     }
     return true;
}


// search the instruction list L and extract VM snippets
void vmextract(list<Inst> *L)
{
     for (list<Inst>::iterator it = L->begin(); it != L->end(); ++it) {
          if (chkpush(it, next(it,7))) {
               ctxswitch cs;
               cs.begin = it;
               cs.end   = next(it,7);
               cs.sd    = next(it,7)->ctxreg[6];
               ctxsave.push_back(cs);
               cout << "push found" << endl;
               cout << it->id << " " << it->addr << " " << it-> assembly << endl;
          } else if (chkpop(it, next(it,7))) {
               ctxswitch cs;
               cs.begin = it;
               cs.end   = next(it,7);
               cs.sd    = it->ctxreg[6];
               ctxrestore.push_back(cs);
               cout << it->id << " " << it->addr << " " << it-> assembly << endl;
          }
     }

     for (list<ctxswitch>::iterator i = ctxsave.begin(); i != ctxsave.end(); ++i) {
          for (list<ctxswitch>::iterator ii = ctxrestore.begin(); ii != ctxrestore.end(); ++ii) {
               if (i->sd == ii->sd) {
                    ctxswh.push_back(pair<ctxswitch,ctxswitch>(*i, *ii));
               }
          }
     }
}

void outputvm(list<pair<ctxswitch, ctxswitch> > *ctxswh)
{
     int n = 1;
     for (list<pair<ctxswitch,ctxswitch> >::iterator i = ctxswh->begin(); i != ctxswh->end(); ++i) {
          list<Inst>::iterator i1 = i->first.begin;
          list<Inst>::iterator i2 = i->second.end;

          string vmfile = "vm" + to_string(n++) + ".txt";
          FILE *fp = fopen(vmfile.c_str(), "w");

          for (list<Inst>::iterator ii = i1; ii != i2; ++ii) {
               fprintf(fp, "%s;%s;", ii->addr.c_str(), ii->assembly.c_str());
               for (int j = 0; j < 8; ++j) {
                    fprintf(fp, "%x,", ii->ctxreg[j]);
               }
               fprintf(fp, "%x,%x\n", ii->raddr, ii->waddr);
          }

          fclose(fp);
     }
}


bool ishex(string &s) {
     if (s.compare(0, 2, "0x") == 0)
          return true;
     else
          return false;
}

struct Edge;

// One basic block
struct BB {
     vector<Inst> instvec;
     ADDR32 beginaddr;
     ADDR32 endaddr;
     vector<int> out;
     int ty;                    // 1: end with jump
                                // 2: no jump

     BB() {}
     BB(ADDR32 begin, ADDR32 end);
     BB(ADDR32 begin, ADDR32 end, int type);
};

BB::BB(ADDR32 begin, ADDR32 end) : ty(0) {
     beginaddr = begin;
     endaddr = end;
}

BB::BB(ADDR32 begin, ADDR32 end, int type) {
     beginaddr = begin;
     endaddr = end;
     ty = type;
}

// Control flow between blocks
struct Edge {
     int from;
     int to;
     bool jumped;
     int ty;                    // 1: indirect jump
                                // 2: direct jump
                                // 3: return
     int count;
     ADDR32 fromaddr;
     ADDR32 toaddr;

     Edge() {}
     Edge(ADDR32 addr1, ADDR32 addr2, int type, int num);
};

Edge::Edge(ADDR32 addr1, ADDR32 addr2, int type, int num)
{
     fromaddr = addr1;
     toaddr = addr2;
     ty = type;
     count = num;
}

class CFG {
     vector<BB> bbs;
     vector<Edge> edges;
     map<ADDR32, int> bbmap;

public:
     CFG() {}
     CFG(list<Inst> *L);
     void checkConsist();
     void showCFG();
     void outputDot();
     void outputSimpleDot();
     void showTrace(list<Inst> *L);
     void compressCFG();
};

// build CFG based on the trace L
// use the addrn of the next instruction after a jump as the target address
// the operand in the jump instruction are only used to decide whether it is
// a direct or indirect jump
CFG::CFG(list<Inst> *L)
{
     list<Inst>::iterator it;
     ADDR32 addr1, addr2;     // addr1 is start and addr2 is end
     it = L->begin();
     addr1 = it->addrn;
     while (it != L->end()) {
          if (isjump(it->opc, jmpset) || it->opcstr == "ret" || it->opcstr == "call") {
               addr2 = it->addrn;

               // build basic blocks
               int curbb, max;
               int res = 0;         // show the inquiry result of curbb
                                    // 1: already exists
                                    // 2 and 3: has overlap
                                    // 4: newbb
                                    // 0: others

               for (curbb = 0, max = bbs.size(); curbb < max; ++curbb) {
                    if (bbs[curbb].endaddr == addr2) break;
               }

               if (curbb != max) {
                    if (bbs[curbb].beginaddr == addr1) {
                         // the bb is already there
                         res = 1;
                    } else if (bbs[curbb].beginaddr < addr1) {
                         // the bb is a subset, need to split
                         res = 2;
                    } else {
                         // the bb is larger than the existing one
                         res = 3;
                    }
               } else {
                    // create a new bb
                    res = 4;
               }

               BB *tempBB;
               Edge *tempEdge;
               switch (res) {
               case 1:
                    break;
               case 2:
                    bbs[curbb].endaddr = addr1 - 1;
                    tempBB = new BB(addr1, addr2, 2);
                    bbs.push_back(*tempBB);
                    tempEdge = new Edge(bbs[curbb].endaddr, addr1, 2, 1);
                    edges.push_back(*tempEdge);
                    break;
               case 3:          // still buggy, but can do now. Need more work.
               {
                    int i, max;
                    for (i = 0, max = bbs.size(); i < max; ++i) {
                         if (bbs[i].beginaddr == addr1) {
                              break;
                         }
                    }
                    if (i == max) {
                         tempBB = new BB(addr1, bbs[curbb].beginaddr-1, 2);
                         bbs.push_back(*tempBB);
                    } else {
                         // do nothing
                    }

                    for (i = 0, max = edges.size(); i < max; ++i) {
                         if (edges[i].fromaddr == bbs[curbb].beginaddr-1 && edges[i].toaddr == bbs[curbb].beginaddr) {
                              edges[i].count++;
                              break;
                         }
                    }
                    if (i == max) {
                         tempEdge = new Edge(bbs[curbb].beginaddr-1, bbs[curbb].beginaddr, 2, 1);
                         edges.push_back(*tempEdge);
                    }
                    break;
               }
               case 4:
                    tempBB = new BB(addr1, addr2);
                    bbs.push_back(*tempBB);
                    break;
               case 0:
                    printf("others\n");
                    printf("addr1: %x, addr2: %x\n", addr1, addr2);
                    printf("curbb begin: %x, end %x\n", bbs[curbb].beginaddr, bbs[curbb].endaddr);
                    break;
               default:
                    break;
               }

               addr1 = next(it, 1)->addrn;
          } else {
               // other instructions, read them into instvec
          }

          ++it;
     }

     // handle the last BB when the last instruction
     // is not jump or ret
     it = prev(L->end());
     if (!(isjump(it->opc, jmpset) || it->opcstr == "ret" || it->opcstr == "call")) {
          ADDR32 lastaddr = it->addrn;
          BB *lastBB = new BB(addr1, lastaddr);
          bbs.push_back(*lastBB);
     }

     // Add edges
     // Ignore the last instruction. If it is an jump/ret/call instruction, we don't
     // know where the target is.
     list<Inst>::iterator nit;
     for (it = L->begin(), nit = next(it, 1); nit != L->end(); ++it, nit = next(it, 1)) {
          ADDR32 curaddr, targetaddr;
          int jumpty;

          curaddr = it->addrn;
          targetaddr = nit->addrn;
          if (isjump(it->opc, jmpset)) {
               string target = it->oprs[0];
               if (ishex(target))
                    jumpty = 2; // is direct jump
               else
                    jumpty = 1; // is indirect jump
          } else if (it->opcstr == "ret") {      // is ret jump
               jumpty = 3;
          } else if (it->opcstr == "call") {
               string target = it->oprs[0];
               if (ishex(target))
                    jumpty = 4; // is direct call
               else
                    jumpty = 5; // is indirect call

          } else {
               continue;        // do nothing on other instructions
          }

          int i, max;
          for (i = 0, max = edges.size(); i < max; ++i) {
               if (edges[i].fromaddr == curaddr && edges[i].toaddr == targetaddr) {
                    edges[i].count++;
                    break;
               }
          }

          if (i == max) {       // not in current edges, add a new edge
               Edge newedge(curaddr, targetaddr, jumpty, 1);
               edges.push_back(newedge);
          }
     }

     // complete information in bbs and edges
     for (int i = 0, max = edges.size(); i < max; ++i) {
          ADDR32 addr1, addr2;
          addr1 = edges[i].fromaddr;
          addr2 = edges[i].toaddr;

          int frombb, tobb, j, maxbbs;
          for (j = 0, maxbbs = bbs.size(); j < maxbbs; ++j) {
               if (bbs[j].endaddr == addr1) {
                    frombb = j;
                    break;
               }
          }
          if (j == maxbbs)
               printf("error: no endaddr == %x\n", addr1);
          for (j = 0, maxbbs = bbs.size(); j < maxbbs; ++j) {
               if (bbs[j].beginaddr == addr2) {
                    tobb = j;
                    break;
               }
          }
          if (j == maxbbs)
               printf("error: no beginaddr == %x\n", addr2);

          edges[i].from = frombb;
          edges[i].to = tobb;
          int outmax;
          for (j = 0, outmax = bbs[frombb].out.size(); j < outmax; ++j) {
               if (bbs[frombb].out[j] == tobb) break;
          }
          if (j == outmax) {
               bbs[frombb].out.push_back(tobb);
          }
     }
}

void CFG::showCFG()
{
     FILE *fp = fopen("cfginfo.txt", "w");

     int bbnum = bbs.size();
     fprintf(fp, "Total BB number: %d\n", bbnum);

     int edgenum = edges.size();
     fprintf(fp, "Total edge number: %d\n", edgenum);

     fprintf(fp, "BBs:\n");
     for (int i = 0, max = bbs.size(); i < max; ++i) {
          int outnum = bbs[i].out.size();
          fprintf(fp, "BB%d: %x, %x, %d\n", i, bbs[i].beginaddr, bbs[i].endaddr, outnum);
     }
     fprintf(fp, "\n");
     fprintf(fp, "Edges:\n");
     for (int i = 0, max = edges.size(); i < max; ++i) {
          fprintf(fp, "Edge%d: %d -> %d, %d, %x, %x, %d\n", i, edges[i].from, edges[i].to, edges[i].count,
                  edges[i].fromaddr, edges[i].toaddr, edges[i].ty);
     }

     fclose(fp);
}

void CFG::outputDot()
{
     FILE *fp = fopen("cfg.dot", "w");

     fprintf(fp, "digraph G {\n");
     for (int i = 0, max = edges.size(); i < max; ++i) {
          string lab;
          switch (edges[i].ty) {
          case 1:
               lab = "i";
               break;
          case 2:
               lab = "d";
               break;
          case 3:
               lab = "r";
               break;
          case 4:
               lab = "dc";
               break;
          case 5:
               lab = "ic";
               break;
          default:
               printf("unknown edge label: %d\n", edges[i].ty);
               break;
          }
          fprintf(fp, "BB%d -> BB%d [label=\"%d,%s\"];\n", edges[i].from, edges[i].to, edges[i].count, lab.c_str());
     }
     fprintf(fp, "}\n");

     fclose(fp);
}

void CFG::outputSimpleDot()
{
     FILE *fp = fopen("simplecfg.dot", "w");

     fprintf(fp, "digraph G {\n");

     for (int i = 0, max = bbs.size(); i < max; ++i) {
          fprintf(fp, "BB%d [shape=record,label=\"\"];\n", i);
     }
     for (int i = 0, max = edges.size(); i < max; ++i) {
          fprintf(fp, "BB%d -> BB%d;\n", edges[i].from, edges[i].to);
     }
     fprintf(fp, "}\n");

     fclose(fp);
}


// combine all BBs which only have one predecessor and one successor
void CFG::compressCFG()
{
     struct BBinfo {
          int id;
          int nfrom;            // number of predecessor
          int nto;              // nuber of successor

          BBinfo(int a, int b, int c) {
               id = a;
               nfrom = b;
               nto = c;
          }
     };

     list<BBinfo> bblist;
     for (int i = 0, max = bbs.size(); i < max; ++i) {
          BBinfo temp(i, 0, 0);
          bblist.push_back(temp);
     }
     for (int i = 0, max = edges.size(); i < max; ++i) {
          for (list<BBinfo>::iterator it = bblist.begin(); it != bblist.end(); ++it) {
               if (it->id == edges[i].from) {
                    it->nto++;
               } else if (it->id == edges[i].to) {
                    it->nfrom++;
               } else {
                    continue;
               }
          }
     }
     for (int i = 0, max = edges.size(); i < max; ++i) {
          list<BBinfo>::iterator ifrom, ito;
          for (list<BBinfo>::iterator it = bblist.begin(); it != bblist.end(); ++it) {
               if (it->id == edges[i].from) ifrom = it;
               if (it->id == edges[i].to) ito = it;
          }
          int bb1 = ifrom->id;
          int bb2 = ito->id;
          if (ifrom->nto == 1 && ito->nfrom == 1) {
               edges.erase(edges.begin()+i);
               for (int j = 0, maxj = edges.size(); j < maxj; ++j) {
                    if (edges[j].from == bb2) edges[j].from = bb1;
               }
               ifrom->nto = ito->nto;
               bblist.erase(ito);

               i = 0;
               max = edges.size();
          }
     }

     FILE *fp = fopen("compcfg.dot", "w");

     fprintf(fp, "digraph G {\n");
     for (int i = 0, max = edges.size(); i < max; ++i) {
          string lab;
          switch (edges[i].ty) {
          case 1:
               lab = "i";
               break;
          case 2:
               lab = "d";
               break;
          case 3:
               lab = "r";
               break;
          case 4:
               lab = "dc";
               break;
          case 5:
               lab = "ic";
               break;
          default:
               printf("unknown edge label: %d\n", edges[i].ty);
               break;
          }
          fprintf(fp, "BB%d -> BB%d [label=\"%d,%s\"];\n", edges[i].from, edges[i].to, edges[i].count, lab.c_str());
     }
     fprintf(fp, "}\n");

     fclose(fp);
}


// check whether all basic blocks are correctly set
void CFG::checkConsist()
{
     int bbnum = bbs.size();
     printf("Total BB number: %d\n", bbnum);

     int edgenum = edges.size();
     printf("Total edge number: %d\n", edgenum);

     for (int i = 0, max = bbs.size(); i < max; ++i) {
          ADDR32 addr1, addr2, addr3, addr4;
          addr1 = bbs[i].beginaddr;
          addr2 = bbs[i].endaddr;

          // When the bb has only one instruction,
          // the begin addr can be equivalent to the
          // end addr.
          if (addr1 > addr2) {
               printf("bad bbs: 1, BB%d\n", i);
          }

          for (int j = i + 1; j < max; ++j) {
               addr3 = bbs[j].beginaddr;
               addr4 = bbs[j].endaddr;

               if(addr3 <= addr4 && (addr2 < addr3 || addr4 < addr1)) {
                    // good bbs
               } else {
                    printf("bad bbs: 2, BB%d and BB%d\n", i, j);
               }
          }
     }
}

void CFG::showTrace(list<Inst> *L)
{
     FILE *fp = fopen("traceinfo.txt", "w");

     for (list<Inst>::iterator it = L->begin(); it != L->end(); ++it) {
          ADDR32 addr = it->addrn;
          int i, max;
          for (i = 0, max = bbs.size(); i < max; ++i) {
               if (bbs[i].beginaddr == addr) break;
          }
          if (i != max) {
               fprintf(fp, "%d -> ", i);
          }
     }
     fprintf(fp, "end\n");

     fclose(fp);
}

void preprocess(list<Inst> *L)
{
     // build global instruction enum based on the instlist
     instenum = buildOpcodeMap(L);

     // update opc field in L
     for (list<Inst>::iterator it = L->begin(); it != L->end(); ++it) {
          it->opc = getOpc(it->opcstr, instenum);
     }

     // create a set containing the opcodes of all jump instructions
     jmpset = new set<int>;
     for (string &s : jmpInstrName) {
          int n;
          if ((n = getOpc(s, instenum)) != 0) {
               jmpset->insert(n);
          }
     }

}


int main(int argc, char **argv) {
     if (argc != 2) {
          fprintf(stderr, "usage: %s <tracefile>\n", argv[0]);
          return 1;
     }

     ifstream infile(argv[1]);
     if (!infile.is_open()) {
          fprintf(stderr, "Open file error!\n");
          return 1;
     }

     parseTrace(&infile, &instlist);

     infile.close();

     preprocess(&instlist);

     peephole(&instlist);

     vmextract(&instlist);

     outputvm(&ctxswh);

     return 0;
}
