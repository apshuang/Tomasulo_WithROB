#pragma once
#include "BasicDefine.h"
#include "Registers.h"

class LoadBuffer;
class StoreBuffer;
class ReservationStationADD;
class ReservationStationMULT;
class InstructionDecoder;
class ReorderBuffer;

class CommonDataBus {
    // ��ģ����Ҫ���𽫹��ܵ�Ԫ���������ֵ���͵���������
    // ��ÿ�ν���һ�飨funcUnit, value����Ȼ��鿴����������������������funcUnit���ͽ����value���͸��ò�����
private:
    static queue<string> functionUnit;
    static queue<string> valueQueue;
    static queue<string> executeSignalQueue;
    static queue<pair<string, string>> commitSignalQueue;
    static queue<string> functionToBeCleared;
    IntegerRegisters* integerRegisters;
    FloatRegisters* floatRegisters;
    LoadBuffer* loadBuffer;
    StoreBuffer* storeBuffer;
    ReservationStationADD* reservationAdd;
    ReservationStationMULT* reservationMult;
    InstructionDecoder* instructionDecoder;
    ReorderBuffer* reorderBuffer;

public:
    CommonDataBus(IntegerRegisters* intRegs, FloatRegisters* floatRegs, LoadBuffer* LDBuffer, StoreBuffer* SDBuffer, ReservationStationADD* RSAdd, ReservationStationMULT* RSMult, InstructionDecoder* decoder, ReorderBuffer* rob);

    static void TransmissValue(string funcUnit, string value) {
        functionUnit.push(funcUnit);
        valueQueue.push(value);
    }

    static void TransmissExecuteSignal(string funcUnit) {
        executeSignalQueue.push(funcUnit);
    }

    static void TransmissCommitSignal(string funcUnit, string message) {
        commitSignalQueue.push({ funcUnit, message });
    }

    static void TransmissClearSignal(string funcUnit) {
        functionToBeCleared.push(funcUnit);
    }
    
    void Tick(int cycle);
};