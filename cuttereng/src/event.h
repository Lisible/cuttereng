#ifndef CUTTERENG_EVENT_H
#define CUTTERENG_EVENT_H

#include "input.h"

typedef enum {
  EVT_QUIT,
  EVT_KEYDOWN,
  EVT_KEYUP,
  EVT_MOUSEMOTION,
  EVT_UNKNOWN,
} EventType;

typedef struct {
  Key key;
} KeyEvent;

typedef struct {
  int relative_x;
  int relative_y;
} MouseMotionEvent;

struct Event {
  EventType type;
  union {
    KeyEvent key_event;
    MouseMotionEvent mouse_motion_event;
  };
};
typedef struct Event Event;

#endif // CUTTERENG_EVENT_H
