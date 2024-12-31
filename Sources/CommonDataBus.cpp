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
			// Store指令完成之后并不需要将数据转发出去，所以也就不占据总线
			// 这里仍然让它发到总线来是为了让InstructionDecoder统计各条指令的完成时间

			instructionDecoder->ReceiveData(unitName, value, cycle);  // store指令和ROB的issue信息也要发给ID部件，让其记录
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