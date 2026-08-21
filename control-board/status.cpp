#include "status.h"

const char* controlErrorAsString(ControlError error) {
  switch (error) {
    case ControlError::None:
      return nullptr;
    case ControlError::InvalidJson:
      return "invalid_json";
    case ControlError::InvalidCommand:
      return "invalid_command";
    case ControlError::InvalidValue:
      return "invalid_value";
  }
  return "invalid_command";
}
