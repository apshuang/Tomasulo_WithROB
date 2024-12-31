#include "../Headers/CommonDataBus.h"
#include "../Headers/LoadBuffer.h"
#include "../Headers/ReservationStation.h"
#include "../Headers/InstructionDecoder.h"

queue<string> CommonDataBus::functionUnit;
queue<string> CommonDataBus::valueQueue;
queue<string> CommonDataBus::executeSignalQueue;
queue<pair<string, string>> CommonDataBus::commitSignalQueue;
queue<string> CommonDataBus::functionToBeCleared;

CommonDataBus::CommonDataBus(IntegerRegisters* intRegs, FloatRegisters* floatRegs, LoadBuffer* LDBuffer, StoreBuffer* SDBuffer, ReservationStationADD* RSAdd, ReservationStationMULT* RSMult, InstructionDecoder* decoder, ReorderBuffer* rob) {
	while (functionUnit.size())functionUnit.pop();
	while (valueQueue.size())valueQueue.pop();
	integerRegisters = intRegs;
	floatRegisters = floatRegs;
	loadBuffer = LDBuffer;
	storeBuffer = SDBuffer;
	reservationAdd = RSAdd;
	reservationMult = RSMult;
	instructionDecoder = decoder;
	reorderBuffer = rob;
}


void CommonDataBus::Tick(int cycle) {
	for (int i = 1; i <= ISSUENUM; i++) {
		if (functionUnit.empty()) break;
		string unitName = functionUnit.front();
		functionUnit.pop();
		string value = valueQueue.front();
		valueQueue.pop();
		if (unitName.substr(0, 5) == "Store" || unitName.substr(0,3) == "ROB") {
			// Storeָ�����֮�󲢲���Ҫ������ת����ȥ������Ҳ�Ͳ�ռ������
			// ������Ȼ����������������Ϊ����InstructionDecoderͳ�Ƹ���ָ������ʱ��

			instructionDecoder->ReceiveData(unitName, value, cycle);  // storeָ���ROB��issue��ϢҲҪ����ID�����������¼
			continue;
		}

		integerRegisters->ReceiveData(unitName, value);
		floatRegisters->ReceiveData(unitName, value);
		loadBuffer->ReceiveData(unitName, value);
		storeBuffer->ReceiveData(unitName, value);
		reservationAdd->ReceiveData(unitName, value);
		reservationMult->ReceiveData(unitName, value);
		instructionDecoder->ReceiveData(unitName, value, cycle);
		reorderBuffer->ReceiveData(unitName, value);
	}

	while (executeSignalQueue.size()) {
		string unitName = executeSignalQueue.front();
		reorderBuffer->ReceiveExecuteSignal(unitName);
		executeSignalQueue.pop();
	}
	while (commitSignalQueue.size()) {
		string unitName = commitSignalQueue.front().first;
		string commitMessage = commitSignalQueue.front().second;
		instructionDecoder->ReceiveCommitSignal(unitName, commitMessage, cycle);
		commitSignalQueue.pop();
	}

	while (functionToBeCleared.size()) {
		string unitName = functionToBeCleared.front();
		integerRegisters->BackTrackClear(unitName);
		floatRegisters->BackTrackClear(unitName);
		loadBuffer->BackTrackClear(unitName);
		storeBuffer->BackTrackClear(unitName);
		reservationAdd->BackTrackClear(unitName);
		reservationMult->BackTrackClear(unitName);
		functionToBeCleared.pop();
	}

}