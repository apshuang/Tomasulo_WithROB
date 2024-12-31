#include "../Headers/ReorderBuffer.h"
#include "../Headers/CommonDataBus.h"

void ReorderBufferLine::Reset() {
	busy = 0;
	inst = "";
	compareType = "";
	compareLeft = "";
	compareRight = "";
	branchLabel = "";
	targetReg = "";
	state = FREE;
	valueString = "";
}

string ReorderBufferLine::GetName() {
	string name = "ROB";
	name += (char)(id + 1 + '0');
	return name;
}

string ReorderBufferLine::GetInstruction() {
	return inst;
}

string ReorderBufferLine::getValue() {
	return valueString;
}

string ReorderBufferLine::GetTargetReg() {
	return targetReg;
}

ReorderBufferLine::ReorderBufferLine(int ID, FloatRegisters* floatRegs, IntegerRegisters* integerRegs) {
	Reset();
	id = ID;
	floatRegisters = floatRegs;
	integerRegisters = integerRegs;
}

int ReorderBufferLine::IsBusy() {
	return busy;
}

bool ReorderBufferLine::IsBranchInstruction() {
	if (compareType == "") return false;
	return true;
}

int ReorderBufferLine::isBranch() {
	if (!checkReady({ compareLeft, compareRight })) return -1;
	if (compareType == "bne") {
		if (compareLeft != compareRight) {
			return 1;
		}
		else return 0;
	}
	else {
		cout << "Unknown branch type" << endl;
		abort();
	}
}

string ReorderBufferLine::ROBIssue(string instruction, string unit, string target) {
	Reset();
	busy = 1;
	inst = instruction;
	valueString = unit;
	targetReg = target;
	state = ISSUE;
	return GetName();
}

string ReorderBufferLine::ROBIssueBranch(string instruction, string op, string left, string right, string label) {
	Reset();
	busy = 1;
	state = ISSUE;
	compareType = op;
	compareLeft = left;
	compareRight = right;
	branchLabel = label;
	inst = instruction;
	return GetName();
}

void ReorderBufferLine::ReceiveData(string unitName, string value) {
	if (valueString == unitName) {
		valueString = value;
	}
	if (compareLeft == unitName) {
		compareLeft = value;
	}
	if (compareRight == unitName) {
		compareRight = value;
	}
}

void ReorderBufferLine::ReceiveExecuteSignal(string unitName) {
	if (valueString == unitName) state = EXEC;
}

void ReorderBufferLine::Commit(int cycle) {
	// 对于ROB来说，commit只会影响到register的旧值（他并不会影响RS中正在执行的任何东西，因为RS只依赖于模块，不依赖于ROB）
	// 而register的旧值实际上是一个稳定值，即如果发生回退的话，就将“旧值”拿出来作为register的值（而不再依赖于后续的ROB）

	string commitMessage = "";
	if (branchLabel == "") {
		char label = targetReg[0];
		label = tolower(label);
		if (label == 'f' || label == 'r') {
			// 浮点寄存器
			int index = stoi(targetReg.substr(1));
			floatRegisters->SetStableValue(valueString, index);
		}
		else if (label == 'x') {
			int index = stoi(targetReg.substr(1));
			integerRegisters->SetStableValue(valueString, index);
		}
	}
	else {
		int branchResult = isBranch();
		if (branchResult == 0) {
			commitMessage = "not jump:" + inst;
		}
		else if (branchResult == 1) {
			commitMessage = "jump:" + inst;
		}
		else {
			cout << "Cannot commit when any compare item is not ready." << endl;
			abort();
		}
	}
	state = COMMIT;
	CommonDataBus::TransmissCommitSignal(GetName(), commitMessage);  // 这个步骤不是传值，而是传commit的时钟
}

int ReorderBufferLine::GetState() {
	return state;
}

void ReorderBufferLine::SetState(int newState) {
	state = newState;
}


ReorderBuffer::ReorderBuffer(FloatRegisters* floatRegs, IntegerRegisters* integerRegs) {
	head = 0;
	tail = 0;
	size = 0;
	for (int i = 0; i < ENTRYNUM; i++) {
		entry[i] = new ReorderBufferLine(i, floatRegs, integerRegs);
	}
}

int ReorderBuffer::IsFree() {
	if (size == ENTRYNUM) {
		return -1; //队列已满
	}
	return tail;
}

string ReorderBuffer::ROBIssue(int idx, string instruction, string unit, string target) {
	string name = entry[idx]->ROBIssue(instruction, unit, target);
	tail = (tail + 1) % ENTRYNUM;
	size++;
	return name;
}
string ReorderBuffer::ROBIssueBranch(int idx, string instruction, string op, string left, string right, string label) {
	string name = entry[idx]->ROBIssueBranch(instruction, op, left, right, label);
	tail = (tail + 1) % ENTRYNUM;
	size++;
	return name;
}

void ReorderBuffer::ReceiveData(string unitName, string value) {
	for (int i = 0; i < ENTRYNUM; i++) {
		entry[i]->ReceiveData(unitName, value);
	}
}

void ReorderBuffer::ReceiveExecuteSignal(string unitName) {
	for (int i = 0; i < ENTRYNUM; i++) {
		entry[i]->ReceiveExecuteSignal(unitName);
	}
}

void ReorderBuffer::BackTrack() {
	// 发现branch指令判断错误，所以需要回退
	// 清掉所有ROB，清掉所有相关的功能单元，将register恢复为稳定值

	int index = head;
	int nowSize = size;
	for (int i = 0; i < nowSize; i++) {
		// 对于还在ISSUE和EXEC的，就清除掉
		if (entry[index]->GetState() == ISSUE || entry[index]->GetState() == EXEC) {
			CommonDataBus::TransmissClearSignal(entry[index]->getValue());  // 此时这个entry的value还是一个依赖值
		}
		
		// 对于ISSUE、EXEC、WRITE的，都要让他们的targetRegister恢复到稳定值（不能再依赖于这个表项）
		CommonDataBus::TransmissClearSignal(entry[index]->GetTargetReg());
		entry[index]->Reset();
		head++;
		size--;
		head %= ENTRYNUM;
		index++;
		index %= ENTRYNUM;
	}
}

void ReorderBuffer::Tick(int cycle) {
	// ReorderBuffer自身只会主动地进行write、commit态的设置（它的issue、exec态的设置都是被动的）
	int index = head;
	int nowSize = size;
	for (int i = 0; i < nowSize; i++) {
		// 先判断当前最旧未COMMIT的指令是否为branch指令
		if (head == index) {
			if (entry[index]->IsBranchInstruction()) {
				int branchResult = entry[index]->isBranch();
				if (branchResult == -1) {
					// 尚未准备好，所以忽略这一行，不commit，所以head不动
					continue;
				}
				else if (branchResult == 0) {
					// 跳转错误，所以要清掉后面所有的ROBLine
					entry[index]->Commit(cycle);  // 先把当前指令提交了
					head++;
					size--;
					head %= ENTRYNUM;
					BackTrack();
				}
				else if (branchResult == 1) {
					// 跳转正确，正常commit。
					entry[index]->Commit(cycle);
					head++;
					size--;
					head %= ENTRYNUM;
				}
			}
		}
		
		if (entry[index]->GetState() == WRITE) {
			// 只要是write，而且是最旧未COMMIT的指令，就可以提交了
			if (head == index) {
				entry[index]->Commit(cycle);
				head++;
				size--;
				head %= ENTRYNUM;
			}
		}

		if (entry[index]->GetState() == EXEC) {
			if (checkReady({ entry[index]->getValue() })) {
				entry[index]->SetState(WRITE);
			}
		}
		index++;
		index %= ENTRYNUM;
	}
}


void ReorderBuffer::InsertOutput(vector<string>& table) {
	const int tableWidth = 80;
	const int colWidth[] = { 10, 8, 20, 70}; // 各列宽度

	// 打印表头
	printHeader("Reorder Buffer", tableWidth);

	// 打印列标题
	cout << "|"
		<< centerString("Entry", colWidth[0]) << "|"
		<< centerString("State", colWidth[1]) << "|"
		<< centerString("Instruction", colWidth[2]) << "|"
		<< centerString("Value", colWidth[3]) << "|\n";
	cout << string(tableWidth, '-') << "\n";

	// 打印ROB数据
	for (size_t i = 0; i < ENTRYNUM; ++i) {
		cout << "|"
			<< centerString("ROB" + to_string(i + 1), colWidth[0]) << "|"
			<< centerString(StateOutput[entry[i]->GetState()], colWidth[1]) << "|"
			<< centerString(entry[i]->GetInstruction(), colWidth[2]) << "|"
			<< centerString(entry[i]->getValue(), colWidth[3]) << "|\n";
	}

	// 打印表尾线
	cout << string(tableWidth, '-') << "\n";
}


bool ReorderBuffer::isAllFree() {
	return size == 0;
}