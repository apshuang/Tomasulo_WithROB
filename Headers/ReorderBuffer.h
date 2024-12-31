#pragma once
#include "LoadBuffer.h"
#include "Registers.h"
#include "ReservationStation.h"

class CommonDataBus;

class ReorderBufferLine {
private:
    // 该模块中有Busy位、对应指令、指令执行状态，还有寄存器的值
    int id;
    int busy;
    string inst;

    string compareType;
    string compareLeft, compareRight;
    string branchLabel;

    int state;
    string targetReg;
    FloatRegisters* floatRegisters;
    IntegerRegisters* integerRegisters;
    string valueString; // 由于该作业的特殊性，设置该变量用于表示#2-#1这类值
    
    string GetName();
public:
    void Reset();
    ReorderBufferLine(int ID, FloatRegisters* floatRegs, IntegerRegisters* integerRegs);
    int IsBusy();
    bool IsBranchInstruction();
    int isBranch();  // 返回-1说明未准备好（继续阻塞），返回0说明不跳转（index向下），返回1说明跳转（index回到对应label处）
    string ROBIssue(string instruction, string unit, string target);
    string ROBIssueBranch(string instruction, string op, string left, string right, string label);
    void Commit(int cycle);
    int GetState();
    string GetInstruction();
    string getValue();
    string GetTargetReg();
    void ReceiveData(string unitName, string value);  // 从CDB那里接收数据
    void ReceiveExecuteSignal(string unitName);  // 从CDB那里接收exec启动的信号
    void SetState(int newState);
};



class ReorderBuffer {
private:
    // 该模块使用循环队列实现，若head==tail说明队列为空，若head==tail+1(mod ENTRYNUM)说明队列已满
    int head;
    int tail;
    int size;
    ReorderBufferLine* entry[ENTRYNUM];
public:
    ReorderBuffer(FloatRegisters* floatRegs, IntegerRegisters* integerRegs);
    int IsFree();
    string ROBIssue(int idx, string instruction, string unit, string target);
    string ROBIssueBranch(int idx, string instruction, string op, string left, string right, string label);
    void Tick(int cycle);
    void InsertOutput(vector<string>& table);
    void ReceiveData(string unitName, string value);  // 从CDB那里接收数据
    void ReceiveExecuteSignal(string unitName);  // 从CDB那里接收exec启动的信号
    void BackTrack();  // 发现branch指令判断错误，所以需要回退
    bool isAllFree();
};
