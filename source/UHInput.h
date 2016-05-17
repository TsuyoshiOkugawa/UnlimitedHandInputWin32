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
	inline int32_t getAngleX() const {
		return accelAndGyro_[0];
	}
	inline int32_t getAngleY() const {
		return accelAndGyro_[1];
	}
	inline int32_t getAngleZ() const {
		return accelAndGyro_[2];
	}

	// Photo-reflectors.
	enum class PhotoReactor_Type {
		FingerMotion0 = 0,
		FingerMotion1,
		FingerMotion2,
		FingerMotion3,
		RadialAndUlnarDeviation0,
		RadialAndUlnarDeviation1,
		OpenHandAndWristMotion0,
		OpenHandAndWristMotion1,
	};
	inline int32_t getPhotoReactor(PhotoReactor_Type type) const {
		int32_t retValue = 0;
		uint32_t index = static_cast<uint32_t>(type);
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
	int32_t acceleration_[3] = {0, 0, 0};
	int32_t gyro_[3] = {0, 0, 0};
	int32_t temprature_ = 0;
	static const uint32_t commandBufferCount_s = 256;
	struct Command {
		uint8_t cmd;
	};
	Command* commandBuffer_ = nullptr;
	uint32_t currentCommandBBufferPosition_ = 0;
	char tempTextBuffer_[256] = "";
	uintptr_t thread_ = 0;
	OVERLAPPED readOverLaped_ = {};
	OVERLAPPED writeOverLaped_ = {};

	int32_t accelAndGyro_[3] = {};
	int32_t photoReflectors_[8] ={};

};