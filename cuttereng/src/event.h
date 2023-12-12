#ifndef CUTTERENG_EVENT_H
#define CUTTERENG_EVENT_H

typedef enum {
  EVT_QUIT,
  EVT_KEYDOWN,
  EVT_UNKNOWN,
} EventType;

typedef struct {
  EventType type;
} Event;

#endif // CUTTERENG_EVENT_H
