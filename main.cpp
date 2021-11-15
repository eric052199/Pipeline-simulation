#include <fstream>
#include <string>
#include <iostream>
#include <iomanip>
#include <bitset>
#include <istream>

// Sign extension
#define SIGNFLAG (1<<15)                        // 0b1000000000000000
#define DATABITS (SIGNFLAG-1)                   // 0b0111111111111111

// Control signals
#define R_TYPE 0                                // 0b000000
#define LW_OP 35                                // 0b100011
#define SW_OP 43                                // 0b101011
#define BNQ_OP 5                                // 0b000101

#define REG_MAX 10                              // Set number of registers    const
#define DATA_MAX 5                              // Set data memory capacity   const
#define INST_MAX 10                             // Set instruction memory capacity

using namespace std;

// Pipeline Register initialization
struct IF_ID{
    int PC;
    string Inst;
} Reg_IF_ID = {0, "00000000000000000000000000000000"}, temp_IF_ID = {0, "00000000000000000000000000000000"};

struct ID_EX{
    int ReadData1;
    int ReadData2;
    int sign_ext;
    int Rs;
    int Rt;
    int Rd;
    string Control;
} Reg_ID_EX = {0, 0, 0, 0, 0 ,0, "000000000"}, temp_ID_EX = {0, 0, 0, 0, 0 ,0, "000000000"};

struct EX_MEM{
    int ALUout;
    int WriteData;
    int Rt_Rd;
    string Control;
} Reg_EX_MEM = {0, 0, 0, "00000"}, temp_EX_MEM = {0, 0, 0, "00000"};

struct MEM_WB{
    int ReadData;
    int ALUout;
    int Rt_Rd;
    string Control;
} Reg_MEM_WB = {0, 0, 0, "00"}, temp_MEM_WB = {0, 0, 0, "00"};



int Reg[REG_MAX];                    // Register
int Data[DATA_MAX];                  // Data memory
int PC;                     // Program counter
string Inst[INST_MAX];               // Inst memory
int count_inst = 0;         // Number of Inst
int Cur_CC = 1;             // Current clock cycle
bool Forward_Rs = false;             // update Register value
bool Forward_Rt = false;             // update Register value
bool Flush = false;                  // bne flush IF
string INPUT_FILE;
string OUTPUT_FILE;
ifstream file;
ofstream Result_file;

// all function

void initialize();
void readInst();
bool isEmpty();
void IF();
void ID();
void EX();
void MEM();
void WB();
void updateReg();
void print();
void next();
string Control_R(string);
string Control_LW(string);
string Control_SW(string);
string Control_BNQ(string);
string Control_I(string);
bool Reg_test();

int main(){

    for(int i = 0; i < 4; i++){
        if(i == 0){
            INPUT_FILE = "General.txt";
            OUTPUT_FILE = "genResult.txt";
        }
        else if(i == 1){
            INPUT_FILE = "Datahazard.txt";
            OUTPUT_FILE = "dataResult.txt";
        }
        else if(i == 2){
            INPUT_FILE = "Lwhazard.txt";
            OUTPUT_FILE = "loadResult.txt";
        }
        else if(i == 3){
            INPUT_FILE = "Branchhazard.txt";
            OUTPUT_FILE = "branchResult.txt";
        }
        initialize(); // Initialize registers and instruction memory
        readInst(); // read instruction from txt to instruction memory

        do{
            IF();

            ID();

            EX();

            MEM();

            WB();

            updateReg();

            print();

            if(Flush)
                Reg_IF_ID.Inst = "00000000000000000000000000000000";

            Forward_Rs = Forward_Rt = Flush = false;

            ++Cur_CC;
        }while(!isEmpty());
        next();
    }
    return 0;
}

// Initialize registers and instruction memory
void initialize(){

    Reg[0] = 0;
    Reg[1] = 9;
    Reg[2] = 5;
    Reg[3] = 7;
    for(int i=4; i<10; ++i)
        Reg[i] = i - 3;

    Data[0] = 5;
    Data[1] = 9;
    Data[2] = 4;
    Data[3] = 8;
    Data[4] = 7;

    PC = 0;
}

// read instruction from txt to instruction memory
void readInst(){
    file.open(INPUT_FILE, fstream::in);
    string s1, s2;
    file >> s1;

    while(file >> Inst[count_inst]){
        count_inst++;
    }

    for(count_inst; count_inst<=(s1.length()/32)-1; count_inst++){
        Inst[count_inst] = s2.assign(s1, count_inst*32, 32);
        cout << Inst[count_inst];
    }

    file.close();
    Result_file.open(OUTPUT_FILE,  ofstream::out | ofstream::trunc);
    Result_file.close();
}

bool isEmpty(){
    if(PC/4 > count_inst){
        // all the register return to initial state
        if(Reg_test())
            return true;
        else
            return false;
    }
    else
        return false;
}

void IF(){

    int instruction_point = PC/4;

    if(instruction_point >= count_inst){
        temp_IF_ID.Inst = "00000000000000000000000000000000";
        PC += 4;
        temp_IF_ID.PC = PC;

        return;
    }

    temp_IF_ID.Inst = Inst[instruction_point];
    PC += 4;
    temp_IF_ID.PC = PC;

    return;
}

void ID(){

    string inst_ID = Reg_IF_ID.Inst;
    int PC_ID = Reg_IF_ID.PC;

    int Rs_ID = 0, Rt_ID = 0, Rd_ID = 0;
    int Op_ID;
    int data1_ID = 0, data2_ID = 0;
    int sign_ext_ID = 0;
    string Control_ID = "000000000";

    // Rs Rt Rd part
    Rs_ID = stoul(inst_ID.substr(6,5), nullptr, 2);
    Rt_ID = stoul(inst_ID.substr(11,5), nullptr, 2);
    Rd_ID = stoul(inst_ID.substr(16,5), nullptr, 2);

    // data1 and data2 part
    data1_ID = Reg[Rs_ID];
    data2_ID = Reg[Rt_ID];

    // sign extension part
    sign_ext_ID = stoi(inst_ID.substr(16,16), nullptr, 2);
    if((sign_ext_ID & SIGNFLAG) != 0){
        sign_ext_ID = (~sign_ext_ID & DATABITS) + 1;
        sign_ext_ID = -sign_ext_ID;
    }

    //Control signals part
    Op_ID = stoul(inst_ID.substr(0,6), nullptr, 2);
    switch (Op_ID) {
        case R_TYPE:
            Control_ID = Control_R(inst_ID);
            break;
        case LW_OP:
            Control_ID = Control_LW(inst_ID);
            break;
        case SW_OP:
            Control_ID = Control_SW(inst_ID);
            break;
        case BNQ_OP:
            Control_ID = Control_BNQ(inst_ID);
            break;
        default:
            Control_ID = Control_I(inst_ID);

    }

    // Branch not equal part
    if(Control_ID[4] == '1'){
        if(data1_ID != data2_ID){
            Flush = true;
            PC += sign_ext_ID * 4 - 4;
        }
    }

    // Detect lw hazard part
    string Control_EX = Reg_ID_EX.Control;
    int Rt_EX = Reg_ID_EX.Rt;
    if(Control_EX[5] == '1' && (Rt_EX == Rt_ID || Rt_EX == Rs_ID)){
        PC -= 4;
        temp_IF_ID.PC -= 4;
        temp_IF_ID.Inst = Inst[(PC-4)/4];
        temp_ID_EX.Control = "000000000";
        temp_ID_EX.ReadData1 = data1_ID;
        temp_ID_EX.ReadData2 = data2_ID;
        temp_ID_EX.sign_ext = sign_ext_ID;
        temp_ID_EX.Rs = Rs_ID;
        temp_ID_EX.Rt = Rt_ID;
        temp_ID_EX.Rd = Rd_ID;
        return;
    }

    temp_ID_EX.ReadData1 = data1_ID;
    temp_ID_EX.ReadData2 = data2_ID;
    temp_ID_EX.sign_ext = sign_ext_ID;
    temp_ID_EX.Rs = Rs_ID;
    temp_ID_EX.Rt = Rt_ID;
    temp_ID_EX.Rd = Rd_ID;
    temp_ID_EX.Control = Control_ID;


    return;
}

void EX(){

    int data1_EX = Reg_ID_EX.ReadData1;
    int data2_EX = Reg_ID_EX.ReadData2;
    int sign_ext_EX = Reg_ID_EX.sign_ext;
    int Rs_EX = Reg_ID_EX.Rs;
    int Rt_EX = Reg_ID_EX.Rt;
    int Rd_EX = Reg_ID_EX.Rd;
    string Control_EX = Reg_ID_EX.Control.substr(0,4);

    int ALUout_EX = 0;
    int WriteData_EX = data2_EX;
    int Rt_Rd_EX = 0;
    string Control_MEM_WB = Reg_ID_EX.Control.substr(4,5);

    // Rt/Rd part
    if(Control_EX[0] == '0')
        Rt_Rd_EX = Rt_EX;
    else
        Rt_Rd_EX = Rd_EX;

    //Forwarding part
    int src1 = 0;
    int src2 = 0;
    int ALUout_MEM = Reg_EX_MEM.ALUout;
    int ALUout_WB = Reg_MEM_WB.ALUout;
    int Rt_Rd_MEM = Reg_EX_MEM.Rt_Rd;
    int Rt_Rd_WB = Reg_MEM_WB.Rt_Rd;
    string Control_MEM = Reg_EX_MEM.Control;
    string Control_WB = Reg_MEM_WB.Control;

    int Data_WB;
    if(Control_WB[1] == '0')
        Data_WB = ALUout_WB;
    else
        Data_WB = Reg_MEM_WB.ReadData;

    if(Control_MEM[3] == '1' && Rt_Rd_MEM != 0){
        if(Rt_Rd_MEM == Rs_EX)
            src1 = 1;

        if(Rt_Rd_MEM == Rt_EX)
            src2 = 1;
    }

    if(Control_WB[0] == '1' && Rt_Rd_WB != 0 && (Rt_Rd_MEM != Rs_EX || Rt_Rd_MEM != Rt_EX)){
        if(Rt_Rd_WB == Rs_EX){
            src1 = 2;
            Forward_Rs = true;
        }

        if(Rt_Rd_WB == Rt_EX){
            src2 = 2;
            Forward_Rt = true;
        }
    }

    // ALUout part
    int ALUsrc1_EX;
    int ALUsrc2_EX;

    switch (src1){
        case 0:
            ALUsrc1_EX = data1_EX;
            break;
        case 1:
            ALUsrc1_EX = ALUout_MEM;
            break;
        case 2:
            ALUsrc1_EX = Data_WB;
            break;
    }

    switch (src2){
        case 0:
            ALUsrc2_EX = (Control_EX[3] == '1') ? sign_ext_EX : data2_EX;
            break;
        case 1:
            ALUsrc2_EX = ALUout_MEM;
            WriteData_EX = ALUsrc2_EX;
            break;
        case 2:
            ALUsrc2_EX = Data_WB;
            WriteData_EX = ALUsrc2_EX;
            break;
    }

    string Function_EX = bitset<6>(sign_ext_EX).to_string();
    string temp = Control_EX.substr(1,2);
    if(temp == "00"){
        ALUout_EX = ALUsrc1_EX + ALUsrc2_EX;
    }
    else if(temp == "01"){
        ALUout_EX = ALUsrc1_EX - ALUsrc2_EX;
    }
    else if(temp == "10"){
        if(Function_EX == "100000")
            ALUout_EX = ALUsrc1_EX + ALUsrc2_EX;
        else if(Function_EX == "100010")
            ALUout_EX = ALUsrc1_EX - ALUsrc2_EX;
        else if(Function_EX == "100100")
            ALUout_EX = ALUsrc1_EX & ALUsrc2_EX;
        else if(Function_EX == "100101")
            ALUout_EX = ALUsrc1_EX | ALUsrc2_EX;
        else if(Function_EX == "101010")
            ALUout_EX = (ALUsrc1_EX <= ALUsrc2_EX) ? 1 : 0;
    }
    else if(temp == "11"){
        ALUout_EX = ALUsrc1_EX & ALUsrc2_EX;
    }

    temp_EX_MEM.ALUout = ALUout_EX;
    temp_EX_MEM.WriteData = WriteData_EX;
    temp_EX_MEM.Rt_Rd = Rt_Rd_EX;
    temp_EX_MEM.Control = Control_MEM_WB;

    return;
}

void MEM(){

    int WriteData_MEM = Reg_EX_MEM.WriteData;
    string Control_MEM = Reg_EX_MEM.Control.substr(0,3);
    string Control_WB = Reg_EX_MEM.Control.substr(3,2);

    int ALUout_MEM = Reg_EX_MEM.ALUout;
    int ReadData_MEM = 0;
    int Rt_Rd_MEM = Reg_EX_MEM.Rt_Rd;

    // Read memory part
    if(Control_MEM[1] == '1')
        ReadData_MEM = Data[ALUout_MEM/4];

    // Write memory
    if(Control_MEM[2] == '1')
        Data[ALUout_MEM/4] = WriteData_MEM;

    temp_MEM_WB.ALUout = ALUout_MEM;
    temp_MEM_WB.ReadData = ReadData_MEM;
    temp_MEM_WB.Rt_Rd = Rt_Rd_MEM;
    temp_MEM_WB.Control = Control_WB;

    return;
}

void WB(){

    int ReadData_WB = Reg_MEM_WB.ReadData;
    int ALUout_WB = Reg_MEM_WB.ALUout;
    unsigned int Rt_Rd_WB = Reg_MEM_WB.Rt_Rd;
    string Control_WB = Reg_MEM_WB.Control;

    if(Control_WB[0] == '1' && Rt_Rd_WB != 0){
        if(Control_WB[1] == '1')
            Reg[Rt_Rd_WB] = ReadData_WB;
        else
            Reg[Rt_Rd_WB] = ALUout_WB;
    }

    if(Rt_Rd_WB == temp_ID_EX.Rs && Forward_Rs && Rt_Rd_WB != 0)
        temp_ID_EX.ReadData1 = Reg[Rt_Rd_WB];

    if(Rt_Rd_WB == temp_ID_EX.Rt && Forward_Rt && Rt_Rd_WB != 0)
        temp_ID_EX.ReadData2 = Reg[Rt_Rd_WB];

    return;
}

void updateReg(){

    Reg_IF_ID = temp_IF_ID;
    Reg_ID_EX = temp_ID_EX;
    Reg_EX_MEM = temp_EX_MEM;
    Reg_MEM_WB = temp_MEM_WB;

    return;
}

void print(){

    Result_file.open(OUTPUT_FILE, ofstream::app);

    Result_file << "CC " << Cur_CC << ":" << endl << endl;
    Result_file << "Registers:" << endl;

    for(int i=0; i<REG_MAX; ++i)
        Result_file << "$" << i << ": " << Reg[i] << endl;

    Result_file << endl << "Data memory:" << endl;
    Result_file << "0x00: " << Data[0] << endl;
    Result_file << "0x04: " << Data[1] << endl;
    Result_file << "0x08: " << Data[2] << endl;
    Result_file << "0x0C: " << Data[3] << endl;
    Result_file << "0x10: " << Data[4] << endl;

    Result_file << endl << "IF/ID :" << endl;
    Result_file << left << setw(17) << "PC" << Reg_IF_ID.PC << endl;
    Result_file << left << setw(17) << "Instruction" << Reg_IF_ID.Inst << endl;

    Result_file << endl << "ID/EX :" << endl;
    Result_file << left << setw(17) << "ReadData1" << Reg_ID_EX.ReadData1 << endl;
    Result_file << left << setw(17) << "ReadData2" << Reg_ID_EX.ReadData2 << endl;
    Result_file << left << setw(17) << "sign_ext" << Reg_ID_EX.sign_ext << endl;
    Result_file << left << setw(17) << "Rs" << Reg_ID_EX.Rs << endl;
    Result_file << left << setw(17) << "Rt" << Reg_ID_EX.Rt << endl;
    Result_file << left << setw(17) << "Rd" << Reg_ID_EX.Rd << endl;
    Result_file << left << setw(17) << "Control signals" << Reg_ID_EX.Control << endl;
    Result_file << endl << "EX/MEM :" << endl;
    Result_file << left << setw(17) << "ALUout" << Reg_EX_MEM.ALUout << endl;
    Result_file << left << setw(17) << "WriteData" << Reg_EX_MEM.WriteData << endl;
    Result_file << left << setw(17) << "Rt/Rd" << Reg_EX_MEM.Rt_Rd << endl;
    Result_file << left << setw(17) << "Control signals" << Reg_EX_MEM.Control << endl;
    Result_file << endl << "MEM/WB :" << endl;
    Result_file << left << setw(17) << "ReadData" << Reg_MEM_WB.ReadData << endl;
    Result_file << left << setw(17) << "ALUout" << Reg_MEM_WB.ALUout << endl;
    Result_file << left << setw(17) << "Rt/Rd" << Reg_MEM_WB.Rt_Rd << endl;
    Result_file << left << setw(17) << "Control signals" << Reg_MEM_WB.Control << endl;
    Result_file << "=================================================================" << endl;

    Result_file.close();

    return;
}

string Control_R(string inst){

    if(stoul(inst, nullptr, 2) != 0)
        return "110000010";
    else
        return "000000000";
}

string Control_LW(string inst){
    return "000101011";
}

string Control_SW(string inst){
    return "000100100";
}

string Control_BNQ(string inst){

    return "001010000";
}

string Control_I(string inst){

    string Op_I = inst.substr(0,6);
    if(Op_I == "001000")       // addi
        return "000100010";
    else if(Op_I == "001100")  // andi
        return "011100010";
    else
        return "error";        // error
}

void next(){
        //IF_ID
        Reg_IF_ID.PC = 0;
        temp_IF_ID.Inst = "00000000000000000000000000000000";
        Reg_IF_ID.PC = 0;
        temp_IF_ID.Inst = "00000000000000000000000000000000";
        // ID_EX
        Reg_ID_EX.ReadData1 = 0;
        temp_ID_EX.ReadData1 = 0;
        Reg_ID_EX.ReadData2 = 0;
        temp_ID_EX.ReadData2 = 0;
        Reg_ID_EX.sign_ext = 0;
        temp_ID_EX.sign_ext = 0;
        Reg_ID_EX.Rs = 0;
        temp_ID_EX.Rs = 0;
        Reg_ID_EX.Rt = 0;
        temp_ID_EX.Rt = 0;
        Reg_ID_EX.Rd = 0;
        temp_ID_EX.Rd = 0;
        Reg_ID_EX.Control = "000000000";
        temp_ID_EX.Control = "000000000";
        // EX_MEM
        Reg_EX_MEM.ALUout = 0;
        temp_EX_MEM.ALUout = 0;
        Reg_EX_MEM.WriteData = 0;
        temp_EX_MEM.WriteData = 0;
        Reg_EX_MEM.Rt_Rd = 0;
        temp_EX_MEM.Rt_Rd = 0;
        Reg_EX_MEM.Control = "00000";
        temp_EX_MEM.Control = "00000";
        // MEM_WB
        Reg_MEM_WB.ReadData = 0;
        temp_MEM_WB.ReadData = 0;
        Reg_MEM_WB.ALUout = 0;
        temp_MEM_WB.ALUout = 0;
        Reg_MEM_WB.Rt_Rd = 0;
        temp_MEM_WB.Rt_Rd = 0;
        Reg_MEM_WB.Control =  "00";
        temp_MEM_WB.Control = "00";
        // Variable
        string Inst[INST_MAX]={};
        PC = 0;
        count_inst=0;
        Cur_CC= 1;
        Flush = false;
        Forward_Rs = false;
        Forward_Rt = false;
}

bool Reg_test(){
    if(!(Reg_IF_ID.Inst == "00000000000000000000000000000000"))
        return false;
    if(!(Reg_ID_EX.ReadData1 == 0 && Reg_ID_EX.ReadData2 == 0 && Reg_ID_EX.sign_ext == 0 && Reg_ID_EX.Rs == 0 && Reg_ID_EX.Rt == 0 && Reg_ID_EX.Rd == 0 && Reg_ID_EX.Control == "000000000"))
        return false;
    if(!(Reg_EX_MEM.ALUout == 0 && Reg_EX_MEM.WriteData == 0 && Reg_EX_MEM.Rt_Rd == 0 && Reg_EX_MEM.Control == "00000"))
        return false;
    if(!(Reg_MEM_WB.ReadData == 0 && Reg_MEM_WB.ALUout == 0 && Reg_MEM_WB.Rt_Rd == 0 && Reg_MEM_WB.Control == "00"))
        return false;
    else
        return true;

}
