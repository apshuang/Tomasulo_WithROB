#include "../Headers/BasicDefine.h"
#include "../Headers/Registers.h"


void RegisterLine::Reset() {
    busy = 0;
    valueString = "";
}

RegisterLine::RegisterLine() {
	Reset();
}

void RegisterLine::SetValue(string value) {
	valueString = value;
}

void RegisterLine::SetStableValue(string value) {
	stableValue = value;
}

void RegisterLine::RecoverStableValue() {
	valueString = stableValue;
}

string RegisterLine::GetValue() {
	return valueString;
}

int RegisterLine::IsBusy() {
	return busy;
}

void RegisterLine::SetBusy(int busyValue) {
	busy = busyValue;
}

void RegisterLine::ReceiveData(string unitName, string value) {
	if (valueString == unitName) valueString = value;
}


FloatRegisters::FloatRegisters() {
	for (int i = 0; i < REGNUM; i++) {
		registers[i] = new RegisterLine();
		string initValue;
		initValue = "Regs[F" + OffsetToString(i * 2) + "]";
		registers[i]->SetValue(initValue);
	}
}

void FloatRegisters::SetLineValue(string value, int reg) {
	// 寄存器F0对应reg0，F2对应reg1，F4对应reg2……所以要转码
	registers[reg / 2]->SetValue(value);
}

void FloatRegisters::SetStableValue(string value, int reg) {
	registers[reg / 2]->SetStableValue(value);
}

int FloatRegisters::IsBusy(int reg) {
	return registers[reg / 2]->IsBusy();
}

string FloatRegisters::GetLineValue(int reg) {
	return registers[reg / 2]->GetValue();
}

void FloatRegisters::SetBusy(int busyValue, int reg) {
	registers[reg / 2]->SetBusy(busyValue);
}

void FloatRegisters::ReceiveData(string unitName, string value) {
	for (int i = 0; i < REGNUM; i++) {
		registers[i]->ReceiveData(unitName, value);
	}
}

void FloatRegisters::BackTrackClear(string unitName) {
	char label = unitName[0];
	label = tolower(label);
	if (label == 'f' || label == 'r') {
		// 浮点寄存器
		int index = stoi(unitName.substr(1));
		registers[index / 2]->RecoverStableValue();
	}
}

void FloatRegisters::InsertOutput(vector<string>& table) {
	printHeader("Float Registers", 94);


	std::cout << "|" << centerString("Register", 14) << "|";
	std::cout << centerString("Busy", 6) << "|";
	std::cout << centerString("Value", 70) << "|\n";
	std::cout << std::string(94, '-') << "\n";  // 打印分隔线

	// 打印每个寄存器的名字、值和 busy 状态
	for (int i = 0; i < REGNUM; ++i) {
		std::cout << "|" << centerString("R" + std::to_string(i * 2), 14) << "|";
		std::cout << centerString(std::to_string(registers[i]->IsBusy()), 6) << "|";
		std::cout << centerString(registers[i]->GetValue(), 70) << "|\n";

	}
	std::cout << std::string(94, '-') << "\n";  // 打印分隔线
	cout << endl;
}



IntegerRegisters::IntegerRegisters() {
	for (int i = 0; i < REGNUM; i++) {
		registers[i] = new RegisterLine();
		string initValue;
		initValue = "Regs[x" + OffsetToString(i) + "]";
		registers[i]->SetValue(initValue);
	}

	// SPECIAL SETTING!!! 用于控制循环的结束
#if ISSUENUM == 2
	registers[3]->SetValue("Regs[x1] + 8 + 8 + 8");
#endif
}

void IntegerRegisters::SetLineValue(string value, int reg) {
	// 寄存器x0就对应reg0，x1就对应reg1，x2对应reg2，与浮点寄存器不一样
	registers[reg]->SetValue(value);
}

int IntegerRegisters::IsBusy(int reg) {
	return registers[reg]->IsBusy();
}

string IntegerRegisters::GetLineValue(int reg) {
	return registers[reg]->GetValue();
}

void IntegerRegisters::SetStableValue(string value, int reg) {
	registers[reg]->SetStableValue(value);
}

void IntegerRegisters::SetBusy(int busyValue, int reg) {
	registers[reg]->SetBusy(busyValue);
}

void IntegerRegisters::ReceiveData(string unitName, string value) {
	for (int i = 0; i < REGNUM; i++) {
		registers[i]->ReceiveData(unitName, value);
	}
}

void IntegerRegisters::BackTrackClear(string unitName) {
	char label = unitName[0];
	label = tolower(label);
	if (label == 'x') {
		// 整型寄存器
		int index = stoi(unitName.substr(1));
		registers[index]->RecoverStableValue();
	}
}

void IntegerRegisters::InsertOutput(vector<string>& table) {
	printHeader("Integer Registers", 94);

	std::cout << "|" << centerString("Register", 14) << "|";
	std::cout << centerString("Busy", 6) << "|";
	std::cout << centerString("Value", 70) << "|\n";
	std::cout << std::string(94, '-') << "\n";  // 打印分隔线

	// 打印每个寄存器的名字、值和 busy 状态
	for (int i = 0; i < REGNUM; ++i) {
		std::cout << "|" << centerString("x" + std::to_string(i), 14) << "|";
		std::cout << centerString(std::to_string(registers[i]->IsBusy()), 6) << "|";
		std::cout << centerString(registers[i]->GetValue(), 70) << "|\n";

	}
	std::cout << std::string(94, '-') << "\n";  // 打印分隔线
	cout << endl;
}