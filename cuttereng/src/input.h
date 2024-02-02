#ifndef CUTTERENG_INPUT_H
#define CUTTERENG_INPUT_H

#include <stdbool.h>
typedef struct Event Event;
typedef enum {
  Key_A = 0,
  Key_B,
  Key_C,
  Key_D,
  Key_E,
  Key_F,
  Key_G,
  Key_H,
  Key_I,
  Key_J,
  Key_K,
  Key_L,
  Key_M,
  Key_N,
  Key_O,
  Key_P,
  Key_Q,
  Key_R,
  Key_S,
  Key_T,
  Key_U,
  Key_V,
  Key_W,
  Key_X,
  Key_Y,
  Key_Z,
  Key_Return,
  Key_Space,
  Key_LeftShift,
  Key_LeftControl,
  Key_RightShift,
  Key_RightControl,
  Key_1,
  Key_2,
  Key_3,
  Key_4,
  Key_5,
  Key_6,
  Key_7,
  Key_8,
  Key_9,
  Key_0,
  Key_Count,
  Key_Unknown
} Key;

typedef struct {
  bool is_down;
} KeyState;

typedef struct {
  int x;
  int y;
} MouseMotion;

typedef enum {
  ControllerJoystick_Unknown,
  ControllerJoystick_Left,
  ControllerJoystick_Right,
} ControllerJoystick;

#define JOYSTICK_DEADZONE 0.3f
typedef struct {
  float x;
  float y;
} JoystickState;

typedef enum {
  ControllerButton_A,
  ControllerButton_B,
  ControllerButton_X,
  ControllerButton_Y,
  ControllerButton_Back,
  ControllerButton_Start,
  ControllerButton_LShoulder,
  ControllerButton_RShoulder,
  ControllerButton_Count,
  ControllerButton_Unknown,
} ControllerButton;

typedef struct {
  bool is_down;
} ButtonState;

typedef struct {
  JoystickState left_joystick_state;
  JoystickState right_joystick_state;
  ButtonState button_states[ControllerButton_Count];
} ControllerState;

typedef struct {
  KeyState key_states[Key_Count];
  ControllerState controller;
  MouseMotion last_mouse_motion;
} InputState;

void InputState_init(InputState *state);
void InputState_handle_event(InputState *state, Event *event);
void InputState_on_frame_end(InputState *state);
bool InputState_is_key_down(InputState *state, Key key);
bool InputState_is_controller_button_down(InputState *state,
                                          ControllerButton button);
const JoystickState *InputState_get_joystick(InputState *state,
                                             ControllerJoystick joystick);
#endif // CUTTERENG_INPUT_H
