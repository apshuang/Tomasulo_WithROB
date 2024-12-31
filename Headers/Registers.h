#pragma once
#include "BasicDefine.h"

class RegisterLine {
private:
    // 该模块中有Busy位、以及占用该register行的功能部件，还有寄存器的值
    int busy;
    string valueString;  // 由于该作业的特殊性，设置该变量用于表示#2-#1这类值
    string stableValue;
    void Reset();
public:
    RegisterLine();
    void SetValue(string value);
    void SetStableValue(string value);
    string GetValue();
    int IsBusy();
    void ReceiveData(string unitName, string value);  // 从CDB那里接收数据
    void RecoverStableValue();
    void SetBusy(int busyValue);
};

class FloatRegisters {
private:
    // 该模块共有F0、F2、F4……F30（F或者R都是浮点寄存器）共16个浮点寄存器，直接实例化RegisterLine
    RegisterLine* registers[REGNUM];
public:
    FloatRegisters();
    void SetLineValue(string value, int reg);
    string GetLineValue(int reg);
    void SetStableValue(string value, int reg);
    int IsBusy(int reg);
    void SetBusy(int busyValue, int reg);
    void ReceiveData(string unitName, string value);  // 从CDB那里接收数据
    void BackTrackClear(string unitName);
    void InsertOutput(vector<string>& table);
};


class IntegerRegisters {
private:
    // 该模块共有x0、x1、x2……x15共16个寄存器，直接实例化RegisterLine
    RegisterLine* registers[REGNUM];
public:
    IntegerRegisters();
    void SetLineValue(string value, int reg);
    string GetLineValue(int reg);
    void SetStableValue(string value, int reg);
    int IsBusy(int reg);
    void SetBusy(int busyValue, int reg);
    void ReceiveData(string unitName, string value);  // 从CDB那里接收数据
    void BackTrackClear(string unitName);
    void InsertOutput(vector<string>& table);
};