#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include "status.h"

void setupMotors();
void applyMotorState(const DeviceState& state);
void stopMotors();

#endif
