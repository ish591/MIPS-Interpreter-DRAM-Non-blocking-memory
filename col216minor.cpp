#include<iostream>
#include<fstream>
#include <sstream>
#include<vector>
#include<map>
#include<iomanip>
using namespace std;
// function to convert a decimal number to a hexadecimal number
long long int maxClockCycles = 10000;
bool infinite_loop =false;
struct Instruction
{
    string name;
    string field_1;
    string field_2;
    string field_3;
};
vector <string> words;              //stores the entire file as a vector of strings

vector<int>row_buffer(1024);        //stores the row buffer, required for lw and sw
int row_buffer_number = -1;              //to store which row is present in our row buffer currently
bool DRAM_request = false;          // if it is true, that means we are currently processing a request
string DRAM_operation;              //to print the string corresponding to our DRAM_operation currently being executed
int starting_cycle = 0;             // stores the cycle where the DRAM operations started
string load_word_register="";  //to maintain the register in which a value is being loaded in lw, as this register can't be used
int type = -1;                      // there can be three possible types
int row_buff = 0;                   // gives total number of row buffer updates
int ROW_ACCESS_DELAY;
int COL_ACCESS_DELAY;
int writeback_row;                  //stores row to be written back
int column_access;                  //stores column to be accessed
/*type 0 : total time here : COL delay
 type 1: ROW + COL delays
 type 2 : 2*ROW +COL delays
 */
//while carrying out the DRAM operations
map<int,string> register_numbers;  //maps each number in 0-31 to a register
map<string,int> register_values; //stores the value of data stored in each register
bool validFile = true;          //will be false if file is invalid at any point
vector<Instruction>instructs;   //stores instructions as structs
int memory[(1<<20)]={0}; //memory used to store the data
int PC=0;               // PC pointer, points to the next instruction
int clock_cycles =0;
map<string,int> operation;
map<int,string> intTostr_operation;

void map_operations(){
    //maps each operation to a number
    operation["add"]=1;
    operation["sub"]=2;
    operation["mul"]=3;
    operation["beq"]=4;
    operation["bne"]=5;
    operation["slt"]=6;
    operation["j"]=7;
    operation["addi"]=8;
    operation["lw"]=9;
    operation["sw"]=10;
    
    intTostr_operation[1]="add";
    intTostr_operation[2]="sub";
    intTostr_operation[3]="mul";
    intTostr_operation[4]="beq";
    intTostr_operation[5]="bne";
    intTostr_operation[6]="slt";
    intTostr_operation[7]="j";
    intTostr_operation[8]="addi";
    intTostr_operation[9]="lw";
    intTostr_operation[10]="sw";
    intTostr_operation[10]="addi";
}

void map_register_numbers(){
    //maps each register to a unique number between 0-31 inclusive
    register_numbers[0]="$r0";
    register_numbers[1]="$at";
    register_numbers[2]="$v0";
    register_numbers[3]="$v1";
    for(int i=4;i<=7;i++){
        register_numbers[i]="$a"+to_string(i-4);
    }
    for(int i=8;i<=15;i++){
        register_numbers[i]="$t"+to_string(i-8);
    }
    for(int i=16;i<=23;i++){
        register_numbers[i]="$s"+to_string(i-16);
    }
    register_numbers[24]="$t8";
    register_numbers[25]="$t9";
    register_numbers[26]="$k0";
    register_numbers[27]="$k1";
    register_numbers[28]="$gp";
    register_numbers[29]="$sp";
    register_numbers[30]="$s8";
    register_numbers[31]="$ra";
}

void initialise_Registers(){
    //initialises all the registers
    for(int i=0;i<32;i++){
        register_values[register_numbers[i]]=0;
    }
    register_values["$sp"]=1048532;
}

bool valid_register(string R){
    //to check if a string is a valid register
    return register_values.find(R)!=register_values.end();
}

bool is_integer(string s){
    //to check for integer
    for(int j=0;j<s.length();j++){
        if(isdigit(s[j]) == false && !(j==0 and s[j]=='-')){
            return false;
        }
    }
    return true;
}

int SearchForRegister (int starting_index, int ending_index, string file_string){
    //this is a helper function which searches for a register from starting index and returns the starting point of it
    int start = -1;
    for (int j= starting_index; j<=ending_index; j++){
        if(file_string[j]==' ' || file_string[j] == '\t'){continue;}
        else{start =j;break;}
    }
    if(start == -1 || start + 2 > ending_index){return -1;}
    if (!valid_register(file_string.substr(start,3))){return -1;}
    return start;  //else found a valid register
}

int SearchForCharacter (int starting_index, int ending_index, string file_string ,char Matching){
    //returns the position of Matching if it is the first non-whitespace character to be found, -1 otherwise
    int start = -1;
    for (int j= starting_index; j<=ending_index; j++){
        if(file_string[j]==' ' || file_string[j] == '\t'){continue;}
        else if (file_string[j]==Matching){return j;}
        else{return -1;}
    }
    return -1; //if no character found except whitespace
}

pair<int,int> SearchForInteger (int starting_index, int ending_index, string file_string){
    //returns the starting and ending index of integer if found
    int start=-1;int end=-1;bool firstMinus = true;
    for (int j= starting_index; j<=ending_index; j++){
        if((file_string[j] == ' ' || file_string[j] == '\t') && start ==-1){continue;} //removing the starting spaces and tabs}
        if (isdigit(file_string[j]) || (file_string[j]=='-' && firstMinus)){
            firstMinus = false;
            if(start==-1){start=j;end=j;}
            else{end=j;}
        }
        else{
            return {start,end};
        }
    }
    return {start,end};
}

string Match_Instruction(int start, int end, string file_string){
    //returns the matched instruction
     if (start +3 <=end){
        string ins =file_string.substr(start,4);
        if (ins =="addi"){return ins;}
    }
    if(start + 2 <=end){
        string ins = file_string.substr(start,3);
        if (ins == "add" || ins == "sub" || ins == "mul" || ins == "slt" ||ins =="beq" || ins =="bne"){return ins;}
    }
    if (start +1 <=end){
        string ins =file_string.substr(start,2);
        if (ins =="lw" || ins == "sw"){return ins;}
    }
    if (start<=end){
        string ins= file_string.substr(start,1);
        if (ins== "j"){return ins;}
    }
    return ""; //when no valid instruction found
}

void Create_structs(string file_string){
    //to retrieve instructions from the input file
    int i=0;
    bool instruction_found = false;
    // each line can contain atmost one instruction
    while(i<file_string.size()){
        if(file_string[i]==' ' || file_string[i] == '\t'){i++;continue;} //ignore tabs and whitespaces
        else{
        if(instruction_found){validFile=false;return;}   //if we have already found an instruction and a character appears, file is invalid
        string ins = Match_Instruction(i, file_string.size()-1, file_string);
        if (ins ==""){  //invalid matching
            validFile = false;
            return;
        }
        if(ins == "add" || ins == "sub" || ins == "mul" || ins == "slt" || ins == "beq" || ins == "bne" || ins == "addi"){
            //now, there must be three registers ahead, delimited by comma
            int reg1_start;
            
            if (ins =="addi"){reg1_start= SearchForRegister(i+4, file_string.size()-1, file_string);}
            else{reg1_start = SearchForRegister(i+3, file_string.size()-1, file_string);}
            if(reg1_start==-1){validFile=false;return;}
            string R1=file_string.substr(reg1_start,3);
            //now first register has been found, it must be followed by a comma and there can be 0 or more whitespaces in between
            
            int comma1Pos=SearchForCharacter(reg1_start+3,file_string.size()-1, file_string, ',');
            if (comma1Pos==-1){validFile=false;return;}
            //looking for a comma
            
            int reg2_start=SearchForRegister(comma1Pos+1, file_string.size()-1, file_string);
            if(reg2_start==-1){validFile=false;return;}
            string R2=file_string.substr(reg2_start,3);
            //looking for the next register
            
            int comma2Pos= SearchForCharacter(reg2_start+3, file_string.size()-1, file_string, ',');
            if (comma2Pos==-1){validFile=false;return;}
            int reg3_start= SearchForRegister(comma2Pos+1, file_string.size()-1, file_string);
            //for the next comma
            
            pair<int,int> integer_indices = SearchForInteger(comma2Pos+1,file_string.size()-1, file_string);
            int index_looped;
            if(reg3_start==-1 && integer_indices.first==-1){validFile=false;return;} //neither an integer nor a string
            string R3;
            // for the next register/integer
            
            if (reg3_start!=-1){
                if(ins!="beq" && ins!="bne" && ins!="addi"){       //is a register and instruction is not bne,beq or addi
                    R3=file_string.substr(reg3_start,3);
                    index_looped = reg3_start +3;
                }
                else{       //beq,bne and addi must have the third argument as an integer
                    validFile=false;
                    return;
                }
            }
            else{
                if(ins=="beq" || ins == "bne"){
                    //beq and bne must have positive labels
                    
                    if(file_string[integer_indices.first]=='-'){validFile=false; return;}
                    else{
                        R3= file_string.substr(integer_indices.first, integer_indices.second- integer_indices.first+1);
                        index_looped = integer_indices.second+1;
                    }
                }
                else{
                    R3= file_string.substr(integer_indices.first, integer_indices.second- integer_indices.first+1);
                    index_looped = integer_indices.second+1;
                }    
            }
            struct Instruction new_instr;
            new_instr.name=ins;
            new_instr.field_1 = R1;
            new_instr.field_2 = R2;
            new_instr.field_3 = R3;
            i = index_looped; //increment i
            instruction_found=true;
            instructs.push_back(new_instr);
            continue;
        }
        else if (ins == "j"){
            pair<int,int> integer_indices = SearchForInteger(i+1 ,file_string.size()-1, file_string);
            if (integer_indices.first == -1 || file_string[integer_indices.first]=='-'){validFile=false; return;}
                    struct Instruction new_instr;
                    new_instr.name=ins;
                    new_instr.field_1 = file_string.substr(integer_indices.first, integer_indices.second- integer_indices.first+1);
                    new_instr.field_2 = "";
                    new_instr.field_3 = "";
                    int index_looped = integer_indices.second +1;
                    i = index_looped;
                    instruction_found =true;
                    instructs.push_back(new_instr);
        }
        else if (ins == "lw" || ins == "sw"){
            //this has the format lw $t0, offset($register_name)
            // first of all search for the first register
            int reg1_start=SearchForRegister(i+2, file_string.size()-1, file_string);
            if(reg1_start==-1){validFile=false;return;}
            string R1=file_string.substr(reg1_start,3);
            //now we will search for a comma and match it
            int commaPos=SearchForCharacter(reg1_start+3,file_string.size()-1, file_string, ',');
            if (commaPos==-1){validFile=false;return;}
            // now we will search for an integer offset and match it
            pair<int,int> integer_indices = SearchForInteger(commaPos+1,file_string.size()-1, file_string);
            if(integer_indices.first==-1){validFile=false;return;}
            string offset = file_string.substr(integer_indices.first, integer_indices.second - integer_indices.first+1);
            // now we will match Left parenthesis
            int lparenPos=SearchForCharacter(integer_indices.second+1 ,file_string.size()-1, file_string, '(');
            if (lparenPos==-1){validFile=false;return;}
            //now we will match a register
            int reg2_start=SearchForRegister(lparenPos+1, file_string.size()-1, file_string);
            if(reg2_start==-1){validFile=false;return;}
            string R2=file_string.substr(reg2_start,3);
            // now we will match the right parenthesis
            int rparenPos = SearchForCharacter(reg2_start+3, file_string.size()-1, file_string, ')');
            if (rparenPos ==-1){validFile = false;return;}
            struct Instruction new_instr;
            new_instr.name=ins;
            new_instr.field_1 = R1;
            new_instr.field_2 = offset;
            new_instr.field_3 = R2;
            i= rparenPos+1;
            instruction_found=true;
            instructs.push_back(new_instr);
    }
}
    }
}

void Number_of_times(int ins_count[],int op_count[]){
    //prints number of times each instruction/type of instruction is executed
    cout<<"\nThe number of times each instruction was executed is given below : \n"<<endl;
    for(int i=0; i< instructs.size() ;i++){
        cout<<"Instruction no: "<< std::dec <<i+1<<" was executed "<<std::dec <<ins_count[i]<<" times."<<endl;
    }
    
    cout<<"\nThe number of times each type of instruction was executed is given below : \n"<<endl;
    for(int i=1; i< 11 ;i++){
        cout<<"Operation "<< intTostr_operation[i]<<" was executed "<<std::dec << op_count[i]<<" times."<<endl;
    }
}

/*in each of the below functions, flag= true means we will print something, else we just run a simulation to count the number of clock cycles
and search for infinite loops*/

void add(bool flag){
    struct Instruction current = instructs[PC];
    if(current.field_1 == "$r0"){validFile=false;return;} //cannot change r0
    if(is_integer(current.field_3)){
        register_values[current.field_1]=register_values[current.field_2]+stoi(current.field_3);
    }
    else{
        register_values[current.field_1]=register_values[current.field_2]+register_values[current.field_3];
    }
    if (flag){
        string Out =current.name+" "+current.field_1+", "+current.field_2+", "+current.field_3;
        cout<<left<<setw(30)<<Out;
        string Reg = current.field_1+"="+to_string(register_values[current.field_1]);
        cout<<left<<setw(20)<<Reg;
       }
    PC++;
}

void sub(bool flag){
    struct Instruction current = instructs[PC];
    if(current.field_1 == "$r0"){validFile=false;return;} //cannot change r0
    if(is_integer(current.field_3)){
        register_values[current.field_1]=register_values[current.field_2]-stoi(current.field_3);
    }
    else{
        register_values[current.field_1]=register_values[current.field_2]-register_values[current.field_3];
    }
    if (flag){
      string Out =current.name+" "+current.field_1+", "+current.field_2+", "+current.field_3;
      cout<<left<<setw(30)<<Out;
      string Reg = current.field_1+"="+to_string(register_values[current.field_1]);
      cout<<left<<setw(20)<<Reg;
         }
    PC++;
}


void mul(bool flag){
    struct Instruction current = instructs[PC];
    if(current.field_1 == "$r0"){validFile=false;return;} //cannot change r0
    if(is_integer(current.field_3)){
        register_values[current.field_1]=register_values[current.field_2]*stoi(current.field_3);
    }
    else{
        register_values[current.field_1]=register_values[current.field_2]*register_values[current.field_3];
    }
    if (flag){
        string Out =current.name+" "+current.field_1+", "+current.field_2+", "+current.field_3;
        cout<<left<<setw(30)<<Out;
        string Reg = current.field_1+"="+to_string(register_values[current.field_1]);
        cout<<left<<setw(20)<<Reg;
    }
    PC++;
}


void beq(bool flag){
    struct Instruction current = instructs[PC];
    if (flag){
        string Out =current.name+" "+current.field_1+", "+current.field_2+", "+current.field_3;
        cout<<left<<setw(30)<<Out;
        cout<<left<<setw(20)<<"N.A";
    }
    if(register_values[current.field_1]==register_values[current.field_2]){
        PC=stoi(current.field_3)-1;
    }
    else PC++;
}


void bne(bool flag){
    struct Instruction current = instructs[PC];
    if (flag){
        string Out =current.name+" "+current.field_1+", "+current.field_2+", "+current.field_3;
        cout<<left<<setw(30)<<Out;
        cout<<left<<setw(20)<<"N.A";
    }
    if(register_values[current.field_1]!=register_values[current.field_2]){
        PC=stoi(current.field_3)-1;
    }
    else PC++;
}


void slt(bool flag){
    struct Instruction current = instructs[PC];
    if(current.field_1 == "$r0"){validFile=false;return;} //cannot change the r0 register
    if(is_integer(current.field_3)){
        if(stoi(current.field_3)>register_values[current.field_2])register_values[current.field_1]=1;
        else register_values[current.field_1]=0;
    }
    else{
        if(register_values[current.field_3]>register_values[current.field_2])register_values[current.field_1]=1;
        else register_values[current.field_1]=0;
    }
    if (flag){
        string Out =current.name+" "+current.field_1+", "+current.field_2+", "+current.field_3;
        cout<<left<<setw(30)<<Out;
        string Reg = current.field_1+"="+to_string(register_values[current.field_1]);
        cout<<left<<setw(20)<<Reg;
    }
    PC++;
}


void j(bool flag){
    struct Instruction current = instructs[PC];
    PC=stoi(current.field_1)-1;
    if (flag){
        string Out =current.name+" "+current.field_1;
        cout<<left<<setw(30)<<Out;
        cout<<left<<setw(20)<<"N.A";
    }
}

void addi(bool flag){
    struct Instruction current = instructs[PC];
    //if first register is $r0, throw an error
    if(current.field_1 == "$r0"){validFile=false;return;}
    register_values[current.field_1]=register_values[current.field_2]+stoi(current.field_3);
    if (flag){
        string Out =current.name+" "+current.field_1+", "+current.field_2+", "+current.field_3;
        cout<<left<<setw(30)<<Out;
        string Reg = current.field_1+"="+to_string(register_values[current.field_1]);
        cout<<left<<setw(20)<<Reg;
         }
    PC++;
}
void PRINT_colOff_op(){
    cout<<left<<setw(25);
    string out = "Column access "+to_string(column_access);
    cout<<out;
    cout<<left<<setw(30)<<DRAM_operation<<"\n";
}

void PRINT_load_row(){
    cout<<left<<setw(25);
    string out = "Activated row "+to_string(row_buffer_number);
    cout<<out;
    cout<<left<<setw(30)<<"N.A"<<"\n";
}

void PRINT_writeback(){
    cout<<left<<setw(25);
    string out = "Writeback row "+to_string(writeback_row);
    cout<<out;
    cout<<left<<setw(30)<<"N.A"<<"\n";
}
void PRINT_range(int current_cycle, int maxlimit){
    //to print a range of cycles
    if (current_cycle == maxlimit){
        cout<<left<<setw(18);
        string Cycle_out = "cycle "+to_string(current_cycle)+": ";
        cout<<Cycle_out;
    }
    else{
        cout<<left<<setw(18);
        string Cycle_out = "cycle "+to_string(current_cycle)+"-"+to_string(maxlimit)+": ";
        cout<<Cycle_out;
    }
}

void callFun(int action, bool param){
    if (action ==1){add(param);}else if (action ==2){sub(param);}else if (action ==3){mul(param);}
    else if (action ==4){beq(param);} else if (action ==5){bne(param);}else if (action ==6){slt(param);}
    else if (action ==7){j(param);}else{addi(param);}
}

void DRAM_initialise(){
     DRAM_request = false; DRAM_operation="";type = -1; starting_cycle =0; load_word_register="";
}

void PRINT_subtask1(int execution_no){
    //in subtask one, all the operations of DRAM are executed and then we move ahead, this function is for printing purposes
        if(type == 2){
            PRINT_range(execution_no, execution_no +ROW_ACCESS_DELAY-1);
            cout<<left<<setw(30)<<"N.A";
            cout<<left<<setw(20)<<"N.A";
            PRINT_writeback();
            execution_no+= ROW_ACCESS_DELAY;
        }
        if (type>=1){
            PRINT_range(execution_no, execution_no +ROW_ACCESS_DELAY-1);
            cout<<left<<setw(30)<<"N.A";
            cout<<left<<setw(20)<<"N.A";
            PRINT_load_row();
            execution_no+=ROW_ACCESS_DELAY;
        }
        if (type>=0){
            PRINT_range(execution_no,execution_no + COL_ACCESS_DELAY -1);
            cout<<left<<setw(30)<<"N.A";
            cout<<left<<setw(20)<<"N.A";
            PRINT_colOff_op();
        }
}

void PRINT_subtask2(int current_cycle, bool printRem){
    //if printRem is true, we execute the remaining part of the DRAM
    //printing depends on the current cycle,starting cycle for the instruction and the type(0,1,or 2)
    int maxL = starting_cycle + type*ROW_ACCESS_DELAY+COL_ACCESS_DELAY -1; //this is the max possible cycle upto which a lw/sw command is executed
    if (!printRem){
        //we just perform one cycle of DRAM operations
        if (type == 0){PRINT_colOff_op();}
        else if (type == 1){
            if (current_cycle<=starting_cycle+ROW_ACCESS_DELAY-1){PRINT_load_row();} //loading new row into row buffer
            else{PRINT_colOff_op();}    //manipulating data from column offset
        }
        else {
            if (current_cycle<=starting_cycle+ROW_ACCESS_DELAY-1){PRINT_writeback();} //copying a row back to DRAM
            else if (current_cycle<=starting_cycle + 2*ROW_ACCESS_DELAY-1){PRINT_load_row();}
            else{PRINT_colOff_op();}
        }
        if (current_cycle == maxL){
            DRAM_initialise();
            //the DRAM request has been completed
        }
    }
    else{
        //here we just have to print the remaining part of the request
        if (type == 0){
            PRINT_range(current_cycle, maxL);
            cout<<left<<setw(30)<<"N.A";
            cout<<left<<setw(20)<<"N.A";
            PRINT_colOff_op();
        }
        else if (type == 1){
                if (current_cycle<=starting_cycle+ROW_ACCESS_DELAY-1){
                    PRINT_range(current_cycle, starting_cycle+ ROW_ACCESS_DELAY-1);
                    cout<<left<<setw(30)<<"N.A";
                    cout<<left<<setw(20)<<"N.A";
                    PRINT_load_row();
                    current_cycle = starting_cycle +ROW_ACCESS_DELAY;
                    PRINT_range(current_cycle, current_cycle + COL_ACCESS_DELAY-1);
                    cout<<left<<setw(30)<<"N.A";
                    cout<<left<<setw(20)<<"N.A";
                    PRINT_colOff_op();
            }
            else{
                PRINT_range(current_cycle, maxL);
                cout<<left<<setw(30)<<"N.A";
                cout<<left<<setw(20)<<"N.A";
                PRINT_colOff_op();
            }
        }
        else{
            if (current_cycle<=starting_cycle+ROW_ACCESS_DELAY-1){
                PRINT_range(current_cycle, starting_cycle+ ROW_ACCESS_DELAY-1);
                cout<<left<<setw(30)<<"N.A";
                cout<<left<<setw(20)<<"N.A";
                PRINT_writeback();
                current_cycle = starting_cycle +ROW_ACCESS_DELAY;
                PRINT_range(current_cycle, starting_cycle+ 2*ROW_ACCESS_DELAY-1);
                cout<<left<<setw(30)<<"N.A";
                cout<<left<<setw(20)<<"N.A";
                PRINT_load_row();
                current_cycle = starting_cycle + 2*ROW_ACCESS_DELAY;
                PRINT_range(current_cycle, maxL);
                cout<<left<<setw(30)<<"N.A";
                cout<<left<<setw(20)<<"N.A";
                PRINT_colOff_op();
            }
            else if (current_cycle<=starting_cycle+2*ROW_ACCESS_DELAY-1){
                PRINT_range(current_cycle, starting_cycle + 2*ROW_ACCESS_DELAY-1);
                cout<<left<<setw(30)<<"N.A";
                cout<<left<<setw(20)<<"N.A";
                PRINT_load_row();
                current_cycle = starting_cycle + 2*ROW_ACCESS_DELAY;
                PRINT_range(current_cycle, maxL);
                cout<<left<<setw(30)<<"N.A";
                cout<<left<<setw(20)<<"N.A";
                PRINT_colOff_op();
            }
            else{
               PRINT_range(current_cycle, maxL);
                cout<<left<<setw(30)<<"N.A";
                cout<<left<<setw(20)<<"N.A";
               PRINT_colOff_op();
            }
        }
    }
}

void lw(bool flag, int subtask_number, int execution_no){
    
    struct Instruction current = instructs[PC];
    int address = register_values[current.field_3]+stoi(current.field_2);
    
    //error handling
    if (address >= (1<<(20)) || (address < 4 * instructs.size()) || address%4!=0){validFile =false;return;}
    if(current.field_1 == "$r0"){validFile=false;return;} //r0 is immutable
    
    int row_number = (address/1024);  //0 based indexing
    int column_number = address - 1024*row_number;
    column_access = column_number;
    if (flag){
        cout<<left<<setw(18);
        string Cycle_out = "cycle "+to_string(execution_no)+": ";
        execution_no++;
        cout<<Cycle_out;
        string Out =current.name+" "+current.field_1+", "+current.field_2+"("+current.field_3+")";
        cout<<left<<setw(30)<<Out;
        cout<<left<<setw(20)<<"N.A";
        cout<<left<<setw(25)<<"DRAM request issued";
        cout<<left<<setw(30)<<"N.A"<<"\n";
        
        if (row_buffer_number == -1){
            type = 1;
            // copy the row to the row buffer
            row_buffer_number = row_number;
            for(int i=0;i<1024;i++){
                row_buffer[i] = memory[row_number*1024+i];
                //storing data into row buffer
            }
            register_values[current.field_1] = row_buffer[column_number];
        }
        else{
            if (row_buffer_number == row_number){
                type =0;
                register_values[current.field_1] = row_buffer[column_number];
            }
            else{
        // first we copy the current row back to memory, procure the correct row and then take the column offset
                writeback_row = row_buffer_number;
                type = 2;
                for(int i=0;i<1024;i++){
                    memory[row_buffer_number*1024+i]=row_buffer[i];
                    row_buffer[i]=memory[row_number*1024+i];
                }
                register_values[current.field_1] = row_buffer[column_number];
                row_buffer_number = row_number;
            }
        }
        DRAM_operation = current.field_1+"="+to_string(register_values[current.field_1]);
        load_word_register = current.field_1;
        if (subtask_number == 1){PRINT_subtask1(execution_no);}
    }
    else{
        register_values[current.field_1] = memory[address]; //else it is just an instance of error analyser
        if (row_buffer_number == -1) {type =1; row_buffer_number = row_number; row_buff ++; /*loading new row*/}
        else{if (row_buffer_number == row_number){type=0;}else{type = 2;row_buff++;row_buffer_number = row_number;}}
        execution_no++;
        load_word_register = current.field_1;
        //this is just for simulation purposes
    }
    if (subtask_number == 2){starting_cycle=execution_no;DRAM_request=true;}
    //in subtask2, we need to maintain the DRAM_request variable
    PC++;
}


void sw(bool flag, int subtask_number, int execution_no){

   struct Instruction current = instructs[PC];
   int address = register_values[current.field_3]+stoi(current.field_2);
   
   //error handling
   if (address >= (1<<(20)) || (address < 4 * instructs.size()) ||  address%4!=0){validFile =false;return;}
   if(current.field_1 == "$r0"){validFile=false;return;} //r0 is immutable
    
   int row_number = (address/1024);  //0 based indexing
   int column_number = address - 1024*row_number;
   column_access=column_number;
   if (flag){
       cout<<left<<setw(18);
       string Cycle_out = "cycle "+to_string(execution_no)+": ";
       execution_no++;
       cout<<Cycle_out;
       string Out =current.name+" "+current.field_1+", "+current.field_2+"("+current.field_3+")";
       cout<<left<<setw(30)<<Out;
       cout<<left<<setw(20)<<"N.A";
       cout<<left<<setw(25)<<"DRAM request issued";
       cout<<left<<setw(30)<<"N.A"<<"\n";
       if (row_buffer_number == -1){
           type = 1;
           // copy the row to the row buffer
           row_buffer_number = row_number;
           for(int i=0;i<1024;i++){
               row_buffer[i] = memory[row_number*1024+i];
               //storing data into row buffer
           }
          row_buffer[column_number]= register_values[current.field_1] ;
       }
       else{
           if (row_buffer_number == row_number){
               type =0;
               row_buffer[column_number]=register_values[current.field_1];
           }
           else{
       // first we copy the current row back to memory, procure the correct row and then take the column offset
               writeback_row = row_buffer_number;
               type = 2;
               for(int i=0;i<1024;i++){
                   memory[row_buffer_number*1024+i]=row_buffer[i];
                   row_buffer[i]=memory[row_number*1024+i];
               }
               row_buffer[column_number] =  register_values[current.field_1];
               row_buffer_number = row_number;
           }
       }
       DRAM_operation = "memory address "+to_string(address)+"-"+to_string(address+3)+"="+to_string(register_values[current.field_1]);
       load_word_register = "";
       if (subtask_number == 1){PRINT_subtask1(execution_no);}
   }
   else{
       memory[address] = register_values[current.field_1]; //else it is just an instance of error analyser
       if (row_buffer_number == -1) {type =1; row_buffer_number = row_number;row_buff+=2;
           /*loading new row, and modifying rowbuffer*/}
       else{if (row_buffer_number == row_number){type=0;row_buff++;}else{type = 2;row_buffer_number = row_number;row_buff+=2;}}
       execution_no++;
       load_word_register = "";
       //this is just for simulation purposes
   }
    if (subtask_number == 2){starting_cycle=execution_no;DRAM_request=true;}
   PC++;
}

void error_analyser(int subtask_number){
    //checks for infinite loops and errors, also gives the count of the clock cycles
    //simulates the entire program without printing anything
    row_buffer_number = -1;
    type = -1;
    row_buff=0;
    DRAM_request = false;
    while (PC < instructs.size()){
        struct Instruction current = instructs[PC];
        int action= operation[current.name];
        clock_cycles++;
        if (clock_cycles > maxClockCycles){
            infinite_loop =true;
            return;
        }
        if (subtask_number == 1){
            if (action<=8){callFun(action,false);}
            else{
                if(action == 9){lw(false,subtask_number,clock_cycles); clock_cycles+=type*ROW_ACCESS_DELAY+COL_ACCESS_DELAY;}
                else{sw(false,subtask_number,clock_cycles); clock_cycles+=type*ROW_ACCESS_DELAY+COL_ACCESS_DELAY;}
            }
        }
        else{ //case of subtask 2
            if(!DRAM_request){
                if (action<=8){callFun(action,false);}
                else{
                    if(action == 9){lw(false,2,clock_cycles);}
                    else{sw(false,2,clock_cycles);}
                }
            }
            else{
                if(action<=8){
                     if (load_word_register == ""){//this means previous instruction was of save word, we can simply execute these instructions
                         callFun(action,false);
                         if(clock_cycles == starting_cycle + type*ROW_ACCESS_DELAY+COL_ACCESS_DELAY -1){
                             DRAM_initialise();
                         }
                     }
                     else{ //previous was of load word, if any arguments of these has load_word_register, we print remaining cycles & then proceed
                         if (current.field_1 == load_word_register || current.field_2 == load_word_register || current.field_3 == load_word_register){
                             clock_cycles=starting_cycle + type*ROW_ACCESS_DELAY+COL_ACCESS_DELAY;
                             callFun(action,false);
                             DRAM_initialise();
                         }
                         else{callFun(action,false);
                             if(clock_cycles == starting_cycle + type*ROW_ACCESS_DELAY+COL_ACCESS_DELAY -1){
                                 DRAM_initialise();
                             }
                         }
                     }
                }
            else{
                clock_cycles = starting_cycle + type*ROW_ACCESS_DELAY+COL_ACCESS_DELAY;
                DRAM_initialise();
                if (action == 9){lw(false,2,clock_cycles);}
                else{sw(false,2,clock_cycles);}
            }
         }
     }
        if (!validFile) {return;}
    }
    if (subtask_number == 2 && DRAM_request){
        // some DRAM request is still pending
        clock_cycles = starting_cycle + type*ROW_ACCESS_DELAY + COL_ACCESS_DELAY - 1;
        DRAM_initialise();
    }
}

void subtask1(){
    //in this case, we wait for the DRAM operations to be complete and then proceed
    DRAM_initialise();
    row_buffer_number = -1;
    int execution_no = 1;   //maintains the execution number
    int op_count[11]= {0};  //used to print number of times each instruction was executed
    int ins_count[instructs.size()]; //used to print number of times each instruction was executed
    for (int j=0; j< instructs.size();j++){ins_count[j]=0;}
    cout<<"\n";
    while(PC<instructs.size()){
        ins_count[PC]++;
        struct Instruction current = instructs[PC];
        int action= operation[current.name];
        op_count[action]++;
        if (action<=8){
            cout<<left<<setw(18);
            string Cycle_out = "cycle "+to_string(execution_no)+": ";
            cout<<Cycle_out;
            callFun(action,true);
            cout<<left<<setw(25)<<"N.A";
            cout<<left<<setw(30)<<"N.A";
            cout<<"\n";
        }
        else{
            if(action == 9){lw(true,1,execution_no); execution_no+=type*ROW_ACCESS_DELAY+COL_ACCESS_DELAY;}
            else{sw(true,1,execution_no); execution_no+=type*ROW_ACCESS_DELAY+COL_ACCESS_DELAY;}
        }
        execution_no++;
    }
    Number_of_times(ins_count,op_count);
}

void subtask2(){
    DRAM_initialise();
    row_buffer_number = -1;
    int execution_no = 1;   //maintains the execution number
    int op_count[11]= {0};  //used to print number of times each instruction was executed
    int ins_count[instructs.size()]; //used to print number of times each instruction was executed
    for (int j=0; j< instructs.size();j++){ins_count[j]=0;}
    cout<<"\n";
    while(PC<instructs.size()){
        ins_count[PC]++;
        struct Instruction current = instructs[PC];
        int action= operation[current.name];
        op_count[action]++;
        /*if DRAM_request = false, we simply process except lw and sw. In case of lw, sw we send a DRAM request.
                if DRAM_request = true:
                   if current command is lw, or sw, we wait for the request to be completed and then continue.
                   In prev request was of lw, then lw cannot occur in any of the arguments to add, sub, mul, beq, bne, or slt
                   If it does, we wait for the request to be completed and then proceed
            */
        if (DRAM_request == false){
            if (action<=8){
                cout<<left<<setw(18);
                string Cycle_out = "cycle "+to_string(execution_no)+": ";
                cout<<Cycle_out;
                callFun(action,true);
                cout<<left<<setw(25)<<"N.A";
                cout<<left<<setw(30)<<"N.A";
                cout<<"\n";
            }
            else{
                if(action == 9){
                    lw(true,2,execution_no);
                }
                else{
                    sw(true,2,execution_no);
                }
            }
            execution_no++;
        }
        else{
            if(action<=8){
                if (load_word_register == ""){ //sw DRAM operation was being executed
                    cout<<left<<setw(18);
                    string Cycle_out = "cycle "+to_string(execution_no)+": ";
                    cout<<Cycle_out;
                    callFun(action,true);
                    PRINT_subtask2(execution_no,false);
                    execution_no++;
                }
                else{
                    //previous was of load word, if any arguments of these has load_word_register, we print remaining cycles & then proceed
                    if (current.field_1 == load_word_register || current.field_2 == load_word_register || current.field_3 == load_word_register){
                        PRINT_subtask2(execution_no, true); //prints remaining DRAM operations
                        execution_no = starting_cycle + type*ROW_ACCESS_DELAY+COL_ACCESS_DELAY;
                        cout<<left<<setw(18);
                        string Cycle_out = "cycle "+to_string(execution_no)+": ";
                        cout<<Cycle_out;
                        callFun(action,true);
                        cout<<left<<setw(25)<<"N.A";
                        cout<<left<<setw(30)<<"N.A";
                        cout<<"\n";
                        DRAM_initialise();
                    }
                    else{ //execute instruction as well as DRAM operation
                        cout<<left<<setw(18);
                        string Cycle_out = "cycle "+to_string(execution_no)+": ";
                        cout<<Cycle_out;
                        callFun(action,true);
                        PRINT_subtask2(execution_no,false);
                    }
                    execution_no++;
                }
            }
           else{
               PRINT_subtask2(execution_no, true); //print remaining DRAM operations
               execution_no = starting_cycle + type*ROW_ACCESS_DELAY+COL_ACCESS_DELAY;
               DRAM_initialise();
               if (action ==9){lw(true,2,execution_no);}
               else{sw(true,2,execution_no);}
               execution_no++;
           }
        }
    }
    if (DRAM_request){
        PRINT_subtask2(execution_no, true);
        execution_no = starting_cycle + type*ROW_ACCESS_DELAY+COL_ACCESS_DELAY-1;
        DRAM_initialise();
    }
    Number_of_times(ins_count,op_count);
}

void printData(){
    cout<<"\nTotal number of cycles: "<<clock_cycles<<"\n";
    cout<<"Total number of row buffer updates (includes loading new row into buffer or modifying row buffer): "<<row_buff<<"\n\n";
    cout<<"Memory content at the end of the execution:\n\n";
    for(int i=0;i<(1<<20);i+=4){
        if(memory[i]!=0){
            cout<<i<<"-"<<i+3<<": "<<memory[i]<<"\n";
        }
    }
    cout<<"\nEvery cycle description\n\n";
    cout<<left<<setw(18)<<"Cycle numbers";
    cout<<left<<setw(30)<<"Instructions";
    cout<<left<<setw(20)<<"Register changed";
    cout<<left<<setw(25)<<"DRAM operations";
    cout<<left<<setw(30)<<"DRAM changes\n";
}


int main(int argc, char*argv[]){
    if (argc<5){cout<<"Invalid arguments\n";return -1;}
    //Taking file name, row and column access delays from the command line
      string file_name = argv[1];
      ROW_ACCESS_DELAY = stoi(argv[2]);
      COL_ACCESS_DELAY = stoi(argv[3]);
    if (ROW_ACCESS_DELAY < 1 || COL_ACCESS_DELAY < 1){cout<<"Invalid arguments\n";return -1;}
      int part = stoi(argv[4]);
      ifstream file(file_name);
      string current_line;
      map_register_numbers();
      initialise_Registers();
      validFile = true;
      infinite_loop = false;
      while(getline(file,current_line)){
          Create_structs(current_line);
      }
    map_operations();
    if(!validFile){
        cout<<"Invalid MIPS program"<<endl;
        return -1;
    }
    
    error_analyser(part);
    
    if (infinite_loop){
        cout<<"Time limit exceeded !"<<endl;
        return -1;
    }
    if (!validFile){
        cout<<"Invalid MIPS program"<<endl;  //due to wrong lw and sw addresses, or $r0 being changed
        return -1;
    }
    PC = 0;
    initialise_Registers();
    printData();
    for(int i=0;i<(1<<20);i++){memory[i]=0;}
    if(part==1){subtask1();}else{subtask2();}
    cout<<"\n";
    /*each instruction occupies 4 bytes. So, we will first of all maintain an array of instructions
     to get the instruction starting at memory address i (in the form of a struct). Rest of the memory is used in RAM*/
    /*so memory stores instructions (as structs here) and data as integers in decimal format*/
    
    return 0;
}


