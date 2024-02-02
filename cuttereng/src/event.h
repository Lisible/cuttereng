#ifndef CUTTERENG_EVENT_H
#define CUTTERENG_EVENT_H

#include "input.h"

typedef enum {
  EventType_Quit,
  EventType_KeyDown,
  EventType_KeyUp,
  EventType_MouseMotion,
  EventType_ControllerJoystickMotion,
  EventType_ControllerButtonDown,
  EventType_ControllerButtonUp,
  EventType_Unknown,
} EventType;

typedef struct {
  Key key;
} KeyEvent;

typedef struct {
  ControllerButton button;
} ControllerButtonEvent;

typedef struct {
  int relative_x;
  int relative_y;
} MouseMotionEvent;

typedef enum {
  ControllerJoystickAxis_Unknown,
  ControllerJoystickAxis_X,
  ControllerJoystickAxis_Y,
} ControllerJoystickAxis;

typedef struct {
  ControllerJoystick joystick;
  ControllerJoystickAxis axis;
  /// value between -1.0 and 1.0
  float value;
} ControllerJoystickMotion;

struct Event {
  EventType type;
  union {
    KeyEvent key_event;
    MouseMotionEvent mouse_motion_event;
    ControllerButtonEvent controller_button_event;
    ControllerJoystickMotion controller_joystick_motion_event;
  };
};
typedef struct Event Event;

#endif // CUTTERENG_EVENT_H
