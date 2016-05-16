#pragma once

#include <Windows.h>
#include <stdint.h>

class UHInput {
public:
	UHInput();
	~UHInput();
	bool initialize(const char* port);
	bool terminate();
	void update();

	// ems.
	bool ems(uint8_t index);	// 0-7.
	bool increaseEMSVoltage();
	bool decreaseEMSVoltage();
	bool increaseEMSSimulationTime();
	bool decreaseEMSSimulationTime();

	// vibration.
	enum class VibrationType {
		Short = 0,
		Long
	};
	bool vibration(VibrationType type);
	bool vibrationMotor(bool sw);
	

	// accel and gyro.
	inline int getAngleX() const {
		return accelAndGyro_[0];
	}
	inline int getAngleY() const {
		return accelAndGyro_[1];
	}
	inline int getAngleZ() const {
		return accelAndGyro_[2];
	}

	// Photo-reflectors.
	inline int getPhotoReactor(uint32_t index) const {
		int retValue = 0;
		if (index < 8) {
			retValue = photoReflectors_[index];
		}
		return retValue;
	}

	// for thread.
	inline bool threadStateActive() const {
		return threadStateActive_;
	}
	void readParams();
private:
	bool send(const void* data, size_t size);
	bool sendToCommandBuffer(uint8_t command);
	void executeCommandBuffer();
	void tryHighSpeedMode();
	void parseLine(const char* line);

	bool initialized_ = false;
	bool threadStateActive_ = false;
	bool heighSpeedMode_ = false;
	bool ready_ = false;
	HANDLE uhHandle_ = INVALID_HANDLE_VALUE;
	HANDLE uhHandleR_ = INVALID_HANDLE_VALUE;
	int acceleration_[3] = {0, 0, 0};
	int gyro_[3] = {0, 0, 0};
	int temprature_ = 0;
	static const uint32_t commandBufferCount_s = 256;
	struct Command {
		uint8_t cmd;
	};
	Command* commandBuffer_ = nullptr;
	uint32_t currentCommandBBufferPosition_ = 0;
	char tempTextBuffer_[256] = "";
	uintptr_t thread_ = 0;
	OVERLAPPED readOverLaped = {};
	OVERLAPPED writeOverLaped = {};

	int accelAndGyro_[3] = {};
	int photoReflectors_[8] ={};

};