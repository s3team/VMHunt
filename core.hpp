typedef uint32_t ADDR32;
typedef pair<ADDR32,ADDR32> AddrRange;

enum Register {
     EAX, EBX, ECX, EDX,        // common registers
     ESI, EDI, ESP, EBP,

     ST0, ST1, ST2, ST3,
     ST4, ST5,

     CS, DS, ES, FS, GS,        // 16 bit segment registers
     SS,

     UNK,                       // Unknown register
};

struct Operand {
     enum Type {IMM, REG, MEM};
     Type ty;
     int tag;
     int bit;
     bool issegaddr;
     string segreg;             // for seg mem access like fs:[0x1]
     string field[5];

     Operand() : bit(0),issegaddr(false) {}
};


// Parameter is the fine grained operand definition. Each REG or MEM Parameter
// represent one byte data in memory or register.
// IMM: idx is the concrete value
// REG: reg is the register name
//      idx is from 0 to 7
// MEM: idx is the memory address
struct Parameter {
     enum Type {IMM, REG, MEM};
     Type ty;
     Register reg;
     ADDR32 idx;

     bool operator==(const Parameter& other);
     bool operator<(const Parameter& other) const;
     bool isIMM();
     void show() const;
};

struct Inst {
     int id;                    // unique instruction id
     string addr;               // instruction address: string
     unsigned int addrn;        // instruction address: unsigned number
     string assembly;           // assembly code, including opcode and operands: string
     int opc;                   // opcode: number
     string opcstr;             // opcode: string
     vector<string> oprs;       // operands: string
     int oprnum;                // number of operands
     Operand *oprd[3];          // parsed operands
     ADDR32 ctxreg[8];          // context registers
     ADDR32 raddr;              // read memory address
     ADDR32 waddr;              // write memroy address

     vector<Parameter> src;     // source parameters
     vector<Parameter> dst;     // destination parameters
     vector<Parameter> src2;    // src and dst for extra dependendency such as in xchg
     vector<Parameter> dst2;

     void addsrc(Parameter::Type t, string s);
     void addsrc(Parameter::Type t, AddrRange a);
     void adddst(Parameter::Type t, string s);
     void adddst(Parameter::Type t, AddrRange a);
     void addsrc2(Parameter::Type t, string s);
     void addsrc2(Parameter::Type t, AddrRange a);
     void adddst2(Parameter::Type t, string s);
     void adddst2(Parameter::Type t, AddrRange a);
};

typedef pair< map<int,int>, map<int,int> > FullMap;

string reg2string(Register reg);
