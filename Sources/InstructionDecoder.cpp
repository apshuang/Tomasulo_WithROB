#include "../Headers/InstructionDecoder.h"
#include "../Headers/ReorderBuffer.h"

InstructionDecoder::InstructionDecoder(IntegerRegisters* intRegs, FloatRegisters* floatRegs, LoadBuffer* LDBuffer, StoreBuffer* SDBuffer, ReservationStationADD* RSAdd, ReservationStationMULT* RSMult, ReorderBuffer* rob) {
	operandNum.clear();
	instructionModule.clear();
	instructions.clear();
	index = 0;
	labelMap.clear();
	instMap.clear();
	robMap.clear();

	integerRegisters = intRegs;
	floatRegisters = floatRegs;
	loadBuffer = LDBuffer;
	storeBuffer = SDBuffer;
	reservationAdd = RSAdd;
	reservationMult = RSMult;
	reorderBuffer = rob;

	//�ڱ�ʵ���У�ȫ����������������ָ��
	for (auto Instruction : OperandNum3Instuction) {
		operandNum[Instruction] = 3;
	}

	instructionModule["LD"] = LOAD; //Loadָ������Ϊ1
	instructionModule["fld"] = LOAD;
	instructionModule["ld"] = LOAD;
	instructionModule["SD"] = STORE; //Storeָ������Ϊ2
	instructionModule["sd"] = STORE;
	instructionModule["ADDD"] = ADDER; // ����ADDģ���ָ������Ϊ3
	instructionModule["fadd.d"] = ADDER;
	instructionModule["addi"] = ADDER;
	instructionModule["SUBD"] = ADDER; 
	instructionModule["fsub.d"] = ADDER; 
	instructionModule["MULTD"] = MULTIPLIER; // ����MULTģ���ָ������Ϊ4
	instructionModule["fmul.d"] = MULTIPLIER;
	instructionModule["DIVD"] = MULTIPLIER;
	instructionModule["fdiv.d"] = MULTIPLIER;
	instructionModule["bne"] = BRANCH;

	while (1) {
		string s = "";
		getline(cin, s);
		if (s == "")break;  // �Ѿ���������ָ����
		instructions.push_back(s);
	}

}

int InstructionDecoder::GetOperandNum(string instructionType) {
	return operandNum[instructionType];
}
int InstructionDecoder::GetInstructionType(string instructionType) {
	return instructionModule[instructionType];
}
int InstructionDecoder::GetOffset(string instructionOperand) {
	return stoi(instructionOperand);
}

string InstructionDecoder::GetRegisterValue(string originValue) {
	// ���ĳ����ֵ�ǼĴ������Ǳ����Ƿ���"F1��x2��R3"���ָ�ʽ
	char label = originValue[0];
	label = tolower(label);
	if (label == 'f' || label == 'r') {
		// ����Ĵ���
		int index = stoi(originValue.substr(1));
		return floatRegisters->GetLineValue(index);
	}
	else if (label == 'x') {
		int index = stoi(originValue.substr(1));
		return integerRegisters->GetLineValue(index);
	}
	else return originValue;  // ����ֱ�ӷ���
}

void InstructionDecoder::SetRegisterValue(string registerName, string funcUnit) {
	char label = registerName[0];
	label = tolower(label);
	int index = stoi(registerName.substr(1));
	if (label == 'f' || label == 'r') {
		// ����Ĵ���
		floatRegisters->SetLineValue(funcUnit, index);
	}
	else if (label == 'x') {
		integerRegisters->SetLineValue(funcUnit, index);
	}
	else {
		cout << "��Ӧ���г�����Ϊָ���dest��" << endl;
		abort();
	}
}

bool InstructionDecoder::ParseLoadAndStore(string opcode, string operands, float cycle, int robIndex) {
	string dest;
	string destValue;
	string originOperands = operands;
	size_t commaPos = operands.find(",");
	if (commaPos != std::string::npos) {
		dest = operands.substr(0, commaPos);
		operands = operands.substr(commaPos + 1);
		destValue = GetRegisterValue(dest);
	}

	string offset;
	size_t leftParenPos = operands.find("(");
	if (leftParenPos != std::string::npos) {
		offset = operands.substr(0, leftParenPos);
		operands = operands.substr(leftParenPos + 1);
		offset = GetRegisterValue(offset);
	}

	string base;
	size_t rightParenPos = operands.find(")");
	if (rightParenPos != std::string::npos) {
		base = operands.substr(0, rightParenPos);
		base = GetRegisterValue(base);
	}
	string unitName = "";
	string originInst = opcode + " " + originOperands;
	string robName = "";
	if (instructionModule[opcode] == LOAD) {
		unitName = loadBuffer->LoadIssue(base, offset, cycle);
		if (unitName == "")return false;
		SetRegisterValue(dest, unitName);  // ��������
		robName = reorderBuffer->ROBIssue(robIndex, originInst, unitName, dest);
	}
	else {
		unitName = storeBuffer->StoreIssue(base, offset, dest, cycle);
		if (unitName == "")return false;

		robName = reorderBuffer->ROBIssue(robIndex, originInst, unitName, "");  // storeָ���Ҫ����targetReg
	}

	instMap[unitName] = instructionTime.size();
	robMap[robName] = instructionTime.size();
	instructionTime.push_back({ originInst, (int)cycle });
	
	return true;
}

bool InstructionDecoder::ParseAddAndMult(string opcode, string operands, float cycle, int robIndex) {
	string dest;

	size_t commaPos = operands.find(",");
	string originOperands = operands;
	if (commaPos != std::string::npos) {
		dest = operands.substr(0, commaPos);
		operands = operands.substr(commaPos + 1);
	}

	string src1, src2;
	commaPos = operands.find(",");
	if (commaPos != std::string::npos) {
		src1 = operands.substr(0, commaPos);
		src1 = GetRegisterValue(src1);
		src2 = operands.substr(commaPos + 1);
		src2 = GetRegisterValue(src2);
	}

	string unitName = "";
	if (instructionModule[opcode] == ADDER) {
		unitName = reservationAdd->AddIssue(opcode, src1, src2, cycle);
	}
	else {
		unitName = reservationMult->MultIssue(opcode, src1, src2, cycle);
	}

	if (unitName == "")return false;
	string originInst = opcode + " " + originOperands;

	SetRegisterValue(dest, unitName);  // ���üĴ�������
	string robName = reorderBuffer->ROBIssue(robIndex, originInst, unitName, dest);

	instMap[unitName] = instructionTime.size();
	robMap[robName] = instructionTime.size();
	instructionTime.push_back({ originInst, (int)cycle });

	return true;
}

string InstructionDecoder::ParseBranch(string opcode, string operands, float cycle, int robIndex) {
	string compareType = opcode;
	string compareLeft, compareRight, branchLabel;
	size_t commaPos = operands.find(",");
	string originOperands = operands;
	if (commaPos != std::string::npos) {
		compareLeft = operands.substr(0, commaPos);
		compareLeft = GetRegisterValue(compareLeft);
		operands = operands.substr(commaPos + 1);
	}

	commaPos = operands.find(",");
	if (commaPos != std::string::npos) {
		compareRight = operands.substr(0, commaPos);
		compareRight = GetRegisterValue(compareRight);
		branchLabel = operands.substr(commaPos + 1);
	}

	string originInst = opcode + " " + originOperands;
	
	string robName = reorderBuffer->ROBIssueBranch(robIndex, originInst, compareType, compareLeft, compareRight, branchLabel);
	robMap[robName] = instructionTime.size();
	branchInstMap[originInst] = index;

	instructionTime.push_back({ originInst, (int)cycle });
	return branchLabel;
}


void InstructionDecoder::Tick(int cycle) {
	/*
	��ע�⣡��decoderֻ�ܴ������ϸ�ʽ��inst����ʽ���£�
	(label:)    opcode operands  (// comments)
	���������ڵ�label��comments���ǿ�ѡ��
	��operands����ָ�����ģ�����addָ����"dest,src1,src2"����ldָ����"dest,offset(base)"�����parse��������ȥ����
	*/
	for (int i = 0; i < ISSUENUM; i++) {
		float exactCycle = cycle + (float)i / ISSUENUM;
		if (index >= instructions.size()) return;  // �Ѷ�������ָ��
		string inst = instructions[index];

		// ���������ĩβ��ע���Լ�ͷβ�Ŀ��ַ�
		size_t commentPos = inst.find("//");
		if (commentPos != std::string::npos) {
			inst = inst.substr(0, commentPos);
		}
		inst.erase(0, inst.find_first_not_of(" \t"));
		inst.erase(inst.find_last_not_of(" \t") + 1);

		// ��label������еĻ���mark����
		size_t labelPos = inst.find(":");
		if (labelPos != std::string::npos) {
			labelMap[inst.substr(0, labelPos)] = index;
			inst = inst.substr(labelPos + 1);
			inst.erase(0, inst.find_first_not_of(" \t"));
		}

		// ����opcode�����ʱ��ʣ�µ�inst��������"opcode operands"�ĸ�ʽ��
		string opcode;
		string operands;
		size_t opcodePos = inst.find(" ");
		if (opcodePos != std::string::npos) {
			opcode = inst.substr(0, opcodePos);
			operands = inst.substr(opcodePos + 1);
		}

		int robIndex = reorderBuffer->IsFree();
		if (robIndex == -1) {
			// û�п��е�rob��
			break;
		}
		if (instructionModule.count(opcode)) {
			if (instructionModule[opcode] == LOAD || instructionModule[opcode] == STORE) {
				// LD/SD��operands�ĸ�ʽΪ"dest,offset(base)"
				if (ParseLoadAndStore(opcode, operands, exactCycle, robIndex)) index++;
				else {
					// ��ǰָ���ʧ�ܣ���˵���������޷���������ָ���ֱ������ѭ��
					break;
				}
			}
			else if (instructionModule[opcode] == ADDER || instructionModule[opcode] == MULTIPLIER) {
				// ADDER��MULTIPLIER��operands�ĸ�ʽΪ"dest,src1,src2"
				if (ParseAddAndMult(opcode, operands, exactCycle, robIndex)) index++;
				else {
					// ��ǰָ���ʧ�ܣ���˵���������޷���������ָ���ֱ������ѭ��
					break;
				}
			}
			else if (instructionModule[opcode] == BRANCH) {
				// BRANC��Hoperands�ĸ�ʽΪ"compareLeft,compareRight,label"
				string label = ParseBranch(opcode, operands, cycle, robIndex);
				index = labelMap[label];
			}
		}
		else {
			cout << "You have not registered " << opcode << " instruction yet!!" << endl;
			abort();
		}
	}
}

bool InstructionDecoder::isAllFree() {
	return index >= instructions.size();
}

void InstructionDecoder::ReceiveData(string unitName, string value, int cycle) {
	if (instMap.count(unitName)) {
		int idx = instMap[unitName];
		instMap.erase(unitName);
		instructionTime[idx].executeComplete = cycle;
	}
}

void InstructionDecoder::ReceiveCommitSignal(string unitName, string commitMessage, int cycle) {
	if (robMap.count(unitName)) {
		int idx = robMap[unitName];
		robMap.erase(unitName);
		instructionTime[idx].commitComplete = cycle;
	}

	string branchResult, originInstruction;
	size_t cuttingPos = commitMessage.find(":");
	branchResult = commitMessage.substr(0, cuttingPos);
	originInstruction = commitMessage.substr(cuttingPos + 1);
	if (branchResult == "not jump") {
		// ����ǲ���ת���Ǿ���ζ���Ʋ�ִ�д����ˣ������������½�index��ΪԴinst��index+1
		index = branchInstMap[originInstruction] + 1;
	}
}


void InstructionDecoder::OutputInstructionTime() {
	const int tableWidth = 85;
	const int colWidth[] = { 20, 15, 15, 15, 15 };
	printHeader("Instruction Complete Time", tableWidth);

	// ��ӡ�б���
	cout << "|"
		<< centerString("Instruction", colWidth[0]) << "|"
		<< centerString("Issue", colWidth[1]) << "|"
		<< centerString("Execute", colWidth[2]) << "|"
		<< centerString("WriteBack", colWidth[3]) << "|"
		<< centerString("Commit", colWidth[4]) << "|\n";
	cout << string(tableWidth, '-') << "\n";

	// ��ӡָ������
	for (const auto& instr : instructionTime) {
		if (instr.commitComplete == -1) {
			// ˵������ָ������;������ģ��������ǲ���ӡ
			continue;
		}
		cout << "|"
			<< centerString(instr.inst, colWidth[0]) << "|"
			<< centerString(to_string(instr.issueComplete), colWidth[1]) << "|"
			<< centerString(to_string(instr.executeComplete), colWidth[2]) << "|"
			<< centerString(to_string(instr.executeComplete + 1), colWidth[3]) << "|"
			<< centerString(to_string(instr.commitComplete), colWidth[4]) << "|\n";
	}

	// ��ӡ��β��
	cout << string(tableWidth, '-') << "\n";
}