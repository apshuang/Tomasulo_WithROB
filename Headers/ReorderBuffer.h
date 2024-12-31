#pragma once
#include "LoadBuffer.h"
#include "Registers.h"
#include "ReservationStation.h"

class CommonDataBus;

class ReorderBufferLine {
private:
    // ��ģ������Busyλ����Ӧָ�ָ��ִ��״̬�����мĴ�����ֵ
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
    string valueString; // ���ڸ���ҵ�������ԣ����øñ������ڱ�ʾ#2-#1����ֵ
    
    string GetName();
public:
    void Reset();
    ReorderBufferLine(int ID, FloatRegisters* floatRegs, IntegerRegisters* integerRegs);
    int IsBusy();
    bool IsBranchInstruction();
    int isBranch();  // ����-1˵��δ׼���ã�����������������0˵������ת��index���£�������1˵����ת��index�ص���Ӧlabel����
    string ROBIssue(string instruction, string unit, string target);
    string ROBIssueBranch(string instruction, string op, string left, string right, string label);
    void Commit(int cycle);
    int GetState();
    string GetInstruction();
    string getValue();
    string GetTargetReg();
    void ReceiveData(string unitName, string value);  // ��CDB�����������
    void ReceiveExecuteSignal(string unitName);  // ��CDB�������exec�������ź�
    void SetState(int newState);
};



class ReorderBuffer {
private:
    // ��ģ��ʹ��ѭ������ʵ�֣���head==tail˵������Ϊ�գ���head==tail+1(mod ENTRYNUM)˵����������
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
    void ReceiveData(string unitName, string value);  // ��CDB�����������
    void ReceiveExecuteSignal(string unitName);  // ��CDB�������exec�������ź�
    void BackTrack();  // ����branchָ���жϴ���������Ҫ����
    bool isAllFree();
};
