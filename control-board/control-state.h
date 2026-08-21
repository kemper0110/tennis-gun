#ifndef CONTROL_STATE_H
#define CONTROL_STATE_H

#include "status.h"

void initializeControlState();
DeviceState getControlState();
void setControlError(ControlError error);
void setRunning(bool running);
void setTopSpeed(uint8_t speed);
void setBottomSpeed(uint8_t speed);
void setDeliverySpeed(uint8_t speed);
void stopForDisconnect();

#endif
