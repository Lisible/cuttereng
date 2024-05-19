#include "input.h"
#include "event.h"
#include <lisiblestd/assert.h>
#include <lisiblestd/log.h>
#include <stddef.h>

void InputState_init(InputState *state) {
  LSTD_ASSERT(state != NULL);
  for (size_t key_index = 0; key_index < Key_Count; key_index++) {
    state->key_states[key_index].is_down = false;
  }

  state->controller.left_joystick_state.x = 0;
  state->controller.left_joystick_state.y = 0;
  state->controller.right_joystick_state.x = 0;
  state->controller.right_joystick_state.y = 0;
  for (size_t button_index = 0; button_index < ControllerButton_Count;
       button_index++) {
    state->controller.button_states[button_index].is_down = false;
  }
}
void InputState_handle_event(InputState *state, Event *event) {
  LSTD_ASSERT(state != NULL);
  LSTD_ASSERT(event != NULL);
  switch (event->type) {
  case EventType_MouseMotion: {
    int x = event->mouse_motion_event.relative_x;
    int y = event->mouse_motion_event.relative_y;
    state->last_mouse_motion.x = x;
    state->last_mouse_motion.y = y;
    LOG_TRACE("Mouse move: (%d, %d)", x, y);
    break;
  }
  case EventType_KeyDown: {
    Key key = event->key_event.key;
    LOG_TRACE("Keydown: %d", key);
    state->key_states[key].is_down = true;
    break;
  }
  case EventType_KeyUp: {
    Key key = event->key_event.key;
    LOG_TRACE("Keyup: %d", key);
    state->key_states[key].is_down = false;
    break;
  }
  case EventType_ControllerButtonDown: {
    ControllerButton button = event->controller_button_event.button;
    LOG_TRACE("ControllerButtonDown: %d", button);
    state->controller.button_states[button].is_down = true;
    break;
  }
  case EventType_ControllerButtonUp: {
    ControllerButton button = event->controller_button_event.button;
    LOG_TRACE("ControllerButtonUp: %d", button);
    state->controller.button_states[button].is_down = false;
    break;
  }
  case EventType_ControllerJoystickMotion: {
    ControllerJoystickMotion *motion = &event->controller_joystick_motion_event;
    JoystickState *joystick = &state->controller.left_joystick_state;
    if (motion->joystick == ControllerJoystick_Right) {
      joystick = &state->controller.right_joystick_state;
    }

    float value = motion->value;
    if ((value > -JOYSTICK_DEADZONE) && (value < JOYSTICK_DEADZONE)) {
      value = 0;
    }

    if (motion->axis == ControllerJoystickAxis_X) {
      joystick->x = value;
    } else {
      joystick->y = value;
    }
    break;
  }
  default:
    break;
  }
}
bool InputState_is_key_down(InputState *state, Key key) {
  LSTD_ASSERT(state != NULL);
  return state->key_states[key].is_down;
}
bool InputState_is_controller_button_down(InputState *state,
                                          ControllerButton button) {
  LSTD_ASSERT(state != NULL);
  return state->controller.button_states[button].is_down;
}

const JoystickState *InputState_get_joystick(InputState *state,
                                             ControllerJoystick joystick) {
  LSTD_ASSERT(state != NULL);
  if (joystick == ControllerJoystick_Left) {
    return &state->controller.left_joystick_state;
  } else {
    return &state->controller.right_joystick_state;
  }
}
void InputState_on_frame_end(InputState *state) {
  LSTD_ASSERT(state != NULL);
  state->last_mouse_motion.x = 0.f;
  state->last_mouse_motion.y = 0.f;
}
