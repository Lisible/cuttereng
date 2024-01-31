#include "input.h"
#include "log.h"
#include "src/event.h"
#include <stddef.h>

void InputState_init(InputState *state) {
  state->did_mouse_move = false;
  for (size_t key_index = 0; key_index < Key_Count; key_index++) {
    state->key_states[key_index].is_down = false;
  }
}
void InputState_handle_event(InputState *state, Event *event) {
  state->did_mouse_move = false;
  switch (event->type) {
  case EVT_MOUSEMOTION: {
    int x = event->mouse_motion_event.relative_x;
    int y = event->mouse_motion_event.relative_y;
    state->did_mouse_move = true;
    state->last_mouse_motion.x = x;
    state->last_mouse_motion.y = y;
    LOG_TRACE("Mouse move: (%d, %d)", x, y);
    break;
  }
  case EVT_KEYDOWN: {
    Key key = event->key_event.key;
    LOG_TRACE("Keydown: %d", key);
    state->key_states[key].is_down = true;
    break;
  }
  case EVT_KEYUP: {
    Key key = event->key_event.key;
    LOG_TRACE("Keyup: %d", key);
    state->key_states[key].is_down = false;
    break;
  }
  default:
    break;
  }
}
bool InputState_is_key_down(InputState *state, Key key) {
  return state->key_states[key].is_down;
}
