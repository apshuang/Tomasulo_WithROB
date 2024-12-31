#pragma once
#include "BasicDefine.h"
#include "LoadBuffer.h"
#include "ReservationStation.h"
#include "Registers.h"
#include "ReorderBuffer.h"

class ReorderBuffer;

class InstructionDecoder {
private:
    unordered_map<string, int> operandNum; // ����ĳ��ָ��Ĳ���������
    unordered_map<string, int> instructionModule; // ����ĳ��ָ��ĵ��õ�ģ�飬LoadΪ1��StoreΪ2��AdderΪ3��MultiplierΪ4
    vector<string> instructions;
    int index;  // �����ڡ���ǰָ���ַ����������ڵ���instructions.size()���Ǿ�˵���Ѿ�����������ָ��
    unordered_map<string, int> labelMap;  // ��¼ĳ��label��index
    unordered_map<string, int> branchInstMap;  // ��¼ĳ��branchָ���index
    vector<Instruction> instructionTime;  // ��¼ÿ��instruction�����ִ�����ʱ��
    unordered_map<string, int> instMap;  // ��¼ĳ��ָ��������ʲôֵ�������ݷ���CDB��ʱ�򣬾���ͬʱ���ִ����ɵ�ʱ��
    unordered_map<string, int> robMap;
    

    IntegerRegisters* integerRegisters;
    FloatRegisters* floatRegisters;
    LoadBuffer* loadBuffer;
    StoreBuffer* storeBuffer;
    ReservationStationADD* reservationAdd;
    ReservationStationMULT* reservationMult;
    ReorderBuffer* reorderBuffer;
    
    bool ParseLoadAndStore(string opcode, string operands, float cycle, int robIndex);
    bool ParseAddAndMult(string opcode, string operands, float cycle, int robIndex); 
    string ParseBranch(string opcode, string operands, float cycle, int robIndex);  // ����parse������label
    string GetRegisterValue(string originValue);
    void SetRegisterValue(string registerName, string funcUnit);
public:
    InstructionDecoder(IntegerRegisters* intRegs, FloatRegisters* floatRegs, LoadBuffer* LDBuffer, StoreBuffer* SDBuffer, ReservationStationADD* RSAdd, ReservationStationMULT* RSMult, ReorderBuffer* rob);
    int GetOperandNum(string instructionType);
    int GetInstructionType(string instructionType);
    int GetOffset(string instructionOperand);
    void ReceiveData(string unitName, string value, int cycle);  // ��CDB�����������
    bool isAllFree();
    void Tick(int cycle);
    void ReceiveCommitSignal(string unitName, string commitMessage, int cycle);
    void OutputInstructionTime();
};
