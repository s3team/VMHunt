struct Operation;
struct Value;

// Symbolic execution engine
class SEEngine {
private:
     // Value *eax, *ebx, *ecx, *edx, *esi, *edi, *esp, *ebp;
     map<string, Value*> ctx;
     list<Inst>::iterator start;
     list<Inst>::iterator end;
     list<Inst>::iterator ip;
     map<AddrRange, Value*> mem;              // memory model
     map<Value*, AddrRange> meminput;         // inputs from memory
     map<Value*, string> reginput;            // inputs from registers

     bool memfind(AddrRange ar) {
          map<AddrRange, Value*>::iterator ii = mem.find(ar);
          if (ii == mem.end())
               return false;
          else
               return true;
     }
     bool memfind(ADDR32 b, ADDR32 e) {
          AddrRange ar(b,e);
          map<AddrRange, Value*>::iterator ii = mem.find(ar);
          if (ii == mem.end())
               return false;
          else
               return true;
     }
     bool isnew(AddrRange ar);
     bool issubset(AddrRange ar, AddrRange *superset);
     bool issuperset(AddrRange ar, AddrRange *subset);
     Value* readReg(string &s);
     void writeReg(string &s, Value *v);
     Value* readMem(ADDR32 addr, int nbyte);
     void writeMem(ADDR32 addr, int nbyte, Value *v);
     /* void readornew(int32_t addr, int nbyte, Value *&v); */
     ADDR32 getRegConVal(string reg);
     ADDR32 calcAddr(Operand *opr);
     void printformula(Value* v);

public:
     SEEngine() {
          ctx = { {"eax", NULL}, {"ebx", NULL}, {"ecx", NULL}, {"edx", NULL},
                  {"esi", NULL}, {"edi", NULL}, {"esp", NULL}, {"ebp", NULL}
          };
     };
     void init(Value *v1, Value *v2, Value *v3, Value *v4,
               Value *v5, Value *v6, Value *v7, Value *v8,
               list<Inst>::iterator it1,
               list<Inst>::iterator it2);
     void init(list<Inst>::iterator it1,
               list<Inst>::iterator it2);
     void initAllRegSymol(list<Inst>::iterator it1,
                          list<Inst>::iterator it2);
     int symexec();
     ADDR32 conexec(Value *f, map<Value*, ADDR32> *input);
     void outputFormula(string reg);
     void dumpreg(string reg);
     void printAllRegFormulas();
     void printAllMemFormulas();
     void printInputSymbols(string output);
     Value *getValue(string s) { return ctx[s]; }
     vector<Value*> getAllOutput();
     void showMemInput();
     void printMemFormula(ADDR32 addr1, ADDR32 addr2);
};

void outputCVCFormula(Value *f);
void outputChkEqCVC(Value *f1, Value *f2, map<int,int> *m);
void outputBitCVC(Value *f1, Value *f2, vector<Value*> *inv1, vector<Value*> *inv2,
                  list<FullMap> *result);
map<Value*, ADDR32> buildinmap(vector<Value*> *vv, vector<ADDR32> *input);
vector<Value*> getInputVector(Value *f); // get formula f's inputs as a vector
string getValueName(Value *v);
