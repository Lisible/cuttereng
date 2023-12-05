#ifndef CUTTERENG_EVENT_H
#define CUTTERENG_EVENT_H

typedef enum event_type event_type;
enum event_type {
  EVT_QUIT,
  EVT_UNKNOWN,
};

typedef struct event event;
struct event {
  event_type type;
};

#endif // CUTTERENG_EVENT_H
