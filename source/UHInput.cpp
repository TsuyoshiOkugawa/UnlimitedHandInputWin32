#include "UHInput.h"

#include <algorithm>
#include <process.h>


#ifdef _DEBUG
#	define UHINPUT_DEBUG_PRINTF(...) printf(__VA_ARGS__);
#else
#	define UHINPUT_DEBUG_PRINTF(...)
#endif

namespace {
	uint32_t __stdcall uhInputThread(void *lpx) {
		UHInput* input = reinterpret_cast<UHInput*>(lpx);
		while (input->threadStateActive()) {
			input->readParams();
		}
		OutputDebugStringA("[UHInput] finish UHInput thread.\n");
		_endthreadex(0);
		return 0;
	}
}

UHInput::UHInput() : commandBuffer_(new Command[commandBufferCount_s]){
}

UHInput::~UHInput() {
	delete[] commandBuffer_;
}

bool UHInput::initialize(const char* port) {
	bool ret = true;
	uhHandle_ = CreateFileA(port, GENERIC_WRITE | GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
	if (uhHandle_ == INVALID_HANDLE_VALUE) {
		ret = false;
	} else {
		if (SetupComm(uhHandle_, 1024, 1024)) {
			if (!PurgeComm(uhHandle_, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR)) {
				ret = false;
			} else {
				DCB dcb;
				GetCommState(uhHandle_, &dcb);
				dcb.DCBlength = sizeof(DCB);
				dcb.BaudRate = CBR_115200;
				dcb.fBinary = FALSE;
				dcb.ByteSize = 8;
				dcb.fParity = NOPARITY;
				dcb.StopBits = ONESTOPBIT;
				if (SetCommState(uhHandle_, &dcb)) {
					thread_ = _beginthreadex(0, 0, uhInputThread, this, 0, nullptr);
					SetThreadPriority(reinterpret_cast<HANDLE>(thread_), THREAD_PRIORITY_ABOVE_NORMAL);
					SetThreadAffinityMask(reinterpret_cast<HANDLE>(thread_), 0xf);
					threadStateActive_ = true;
					initialized_ = true;
				} else {
					ret = false;
				}
			}
		} else {
			ret = false;
		}
	}

	return ret;
}

bool UHInput::terminate() {
	bool ret = true;
	if (initialized_) {
		threadStateActive_ = false;
		WaitForSingleObject(reinterpret_cast<HANDLE>(thread_), INFINITE);
		CloseHandle(uhHandle_);
		uhHandle_ = INVALID_HANDLE_VALUE;
		initialized_ = false;
		ready_ = false;
	}
	return ret;
}

void UHInput::update() {
	if (initialized_ && ready_) {
		tryHighSpeedMode();
		executeCommandBuffer();
	}
}

void UHInput::tryHighSpeedMode() {
	if (ready_ && false == heighSpeedMode_) {
		uint8_t data = static_cast<uint8_t>('s');
		heighSpeedMode_ = send(&data, sizeof(uint8_t));
	}
}

void UHInput::readParams() {
	DWORD err = 0;
	COMSTAT comstat = {};
	ClearCommError(uhHandle_, &err, &comstat);
	DWORD cout = comstat.cbInQue;
	if (comstat.cbInQue > 0) {
		char* text = new char[comstat.cbInQue + 1];
		memset(text, 0, comstat.cbInQue);
		DWORD readSize = 0;
		if (FALSE == ReadFile(uhHandle_, text, comstat.cbInQue, &readSize, &readOverLaped_)) {
			BOOL readErr = GetLastError();
			if (readErr == ERROR_IO_PENDING) {
				DWORD written = 0;
				GetOverlappedResult(uhHandle_, &readOverLaped_, &written, TRUE);
			} else {
				UHINPUT_DEBUG_PRINTF("read failed\n");
			}
		}
		text[comstat.cbInQue] = '\0';
		//UHINPUT_DEBUG_PRINTF("%s", text);

		if (false == ready_ && nullptr != strstr(text, "Please input any charactor:")) {
			UHINPUT_DEBUG_PRINTF("ready\n");
			ready_ = true;
		} else if (ready_) {
			size_t currentTextSize = strlen(tempTextBuffer_);
			if ((currentTextSize + comstat.cbInQue + 1) < 255) {
				strcat(tempTextBuffer_, text);
			} else {
				strcpy(tempTextBuffer_, "");
			}

			// compleate.
			char* returnPos = strstr(tempTextBuffer_, "\n");
			if (nullptr != returnPos) {
				parseLine(tempTextBuffer_);

				// carry over.
				strcpy(tempTextBuffer_, &returnPos[1]);
			}

		}

		delete text;
	}
}

void UHInput::parseLine(const char* line) {
	char tempLine[256] = "";
	strcpy(tempLine, line);
	char* textTerm = strstr(tempLine, "\n");
	textTerm[1] = '\0';

	uint32_t length = static_cast<uint32_t>(strlen(tempLine));
	// get accel and gyro and Photo-reflectors
	uint32_t cCount = 0;
	uint32_t ubCount  = 0;
	for (uint32_t i = 0; i < length; ++i) {
		if (tempLine[i] == ',') {
			cCount ++;
		} else if (tempLine[i] == '_') {
			ubCount++;
		}
	}
	if (cCount == 3 && ubCount == 7) {
		sscanf(tempLine, "%d,%d,%d,%d_%d_%d_%d_%d_%d_%d_%d", &accelAndGyro_[0], &accelAndGyro_[1], &accelAndGyro_[2], &photoReflectors_[0], &photoReflectors_[1], &photoReflectors_[2], &photoReflectors_[3], &photoReflectors_[4], &photoReflectors_[5], &photoReflectors_[6], &photoReflectors_[7]);
	}

	//UHINPUT_DEBUG_PRINTF("parse line:%s", tempLine);
}

void UHInput::executeCommandBuffer() {
	if (currentCommandBBufferPosition_ > 0) {
		PurgeComm(uhHandle_, PURGE_RXABORT | PURGE_RXCLEAR);
	}
	for (uint32_t i = 0; i < currentCommandBBufferPosition_; ++i) {
		send(&commandBuffer_[i].cmd, sizeof(uint8_t));
	}
	currentCommandBBufferPosition_ = 0;
}

bool UHInput::sendToCommandBuffer(uint8_t command) {
	bool ret = false;
	if (currentCommandBBufferPosition_ < commandBufferCount_s) {
		commandBuffer_[currentCommandBBufferPosition_].cmd = command;
		currentCommandBBufferPosition_ ++;
		ret = true;
	}
	return ret;
}

bool UHInput::send(const void* data, size_t size) {
	bool ret = true;
	if (initialized_ && ready_) {
		DWORD sendSize = 0;
		if (FALSE == WriteFile(uhHandle_, data, static_cast<DWORD>(size), &sendSize, &writeOverLaped_)) {
			BOOL err = GetLastError();
			if (err == ERROR_IO_PENDING) {
				DWORD written = 0;
				GetOverlappedResult(uhHandle_, &writeOverLaped_, &written, TRUE);
			} else {
				printf("write failed[%s]\n", reinterpret_cast<const char*>(data));
				ret = false;
			}
		}
	}
	return ret;
}

bool UHInput::ems(uint8_t index) {
	uint8_t emsData = static_cast<uint8_t>('0') + std::min<uint8_t>(7, index);
	return sendToCommandBuffer(emsData);
}

bool UHInput::increaseEMSVoltage() {
	uint8_t emsData = static_cast<uint8_t>('h');
	return sendToCommandBuffer(emsData);
}
bool UHInput::decreaseEMSVoltage() {
	uint8_t emsData = static_cast<uint8_t>('i');
	return sendToCommandBuffer(emsData);
}
bool UHInput::increaseEMSSimulationTime() {
	uint8_t emsData = static_cast<uint8_t>('m');
	return sendToCommandBuffer(emsData);

}
bool UHInput::decreaseEMSSimulationTime() {
	uint8_t emsData = static_cast<uint8_t>('n');
	return sendToCommandBuffer(emsData);
}


bool UHInput::vibration(VibrationType type) {
	uint8_t vibData = static_cast<uint8_t>('b');
	switch (type) {
	case VibrationType::Short:
		vibData = static_cast<uint8_t>('b');
		break;

	case VibrationType::Long:
		vibData = static_cast<uint8_t>('B');
		break;

	default:
		// TODO:err
		vibData = static_cast<uint8_t>('b');
		break;
	}
	return sendToCommandBuffer(vibData);
}


bool UHInput::vibrationMotor(bool sw) {
	uint8_t vibData = static_cast<uint8_t>('e');
	if (sw) {
		vibData = static_cast<uint8_t>('d');
	}
	return sendToCommandBuffer(vibData);
}