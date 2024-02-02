#include "cuttereng.h"
#include "SDL_events.h"
#include "SDL_gamecontroller.h"
#include "SDL_keyboard.h"
#include "SDL_timer.h"
#include "assert.h"
#include "engine.h"
#include "environment/environment.h"
#include "event.h"
#include "filesystem.h"
#include "json.h"
#include "log.h"
#include "src/memory.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const size_t KB = 1000;

void event_from_sdl_event(SDL_Event *sdl_event, Event *event);

void cutter_bootstrap(EcsInitSystem ecs_init_system) {
  LOG_INFO("Bootstrapping...");
  char *configuration_file_path =
      env_get_configuration_file_path(&system_allocator);
  LOG_DEBUG("Configuration file path: %s", configuration_file_path);
  char *configuration_file_content = filesystem_read_file_to_string(
      &system_allocator, configuration_file_path);

  Json *configuration_json =
      json_parse_from_str(&system_allocator, configuration_file_content);
  if (!configuration_json)
    goto cleanup;

  Configuration config;
  if (!configuration_from_json(configuration_json, &config))
    goto cleanup_2;

  Engine engine;
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
  SDL_Window *window = SDL_CreateWindow(
      config.application_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      config.window_size.width, config.window_size.height, 0);
  int joystick_count = SDL_NumJoysticks();
  LOG_DEBUG("Joystick count: %d", joystick_count);
  SDL_GameController *controller = NULL;
  if (joystick_count > 0) {
    controller = SDL_GameControllerOpen(0);
  }
  engine_init(&engine, &config, ecs_init_system, window);

  Arena frame_arena;
  Allocator frame_allocator = arena_allocator(&frame_arena);
  arena_init(&frame_arena, &system_allocator, 10 * KB);

  float last_frame_time = 0;
  while (engine_is_running(&engine)) {
    float current_time = SDL_GetTicks();
    float dt = (current_time - last_frame_time) / 1000.f;
    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event) != 0) {
      Event event;
      event_from_sdl_event(&sdl_event, &event);
      engine_handle_events(&engine, &event);
    }
    engine_set_current_time(&engine, current_time / 1000.f);
    float update_start_time = SDL_GetTicks();
    engine_update(&frame_allocator, &engine, dt);
    float update_duration = (SDL_GetTicks() - update_start_time) / 1000.f;
    LOG_DEBUG("Update time: %fs", update_duration);

    float render_start_time = SDL_GetTicks();
    engine_render(&frame_allocator, &engine);
    float render_duration = (SDL_GetTicks() - render_start_time) / 1000.f;
    LOG_DEBUG("Render time: %fs", render_duration);

    arena_clear(&frame_arena);

    last_frame_time = current_time;
    ;
  }
  arena_deinit(&frame_arena, &system_allocator);

  SDL_GameControllerClose(controller);
  SDL_DestroyWindow(window);
  engine_deinit(&engine);
  SDL_Quit();

cleanup_2:
  json_destroy(&system_allocator, configuration_json);
cleanup:
  free(configuration_file_path);
  free(configuration_file_content);
}

Key key_from_sdl_keysym(SDL_Keycode keycode) {
  switch (keycode) {
  case SDLK_a:
    return Key_A;
  case SDLK_b:
    return Key_B;
  case SDLK_c:
    return Key_C;
  case SDLK_d:
    return Key_D;
  case SDLK_e:
    return Key_E;
  case SDLK_f:
    return Key_F;
  case SDLK_g:
    return Key_G;
  case SDLK_h:
    return Key_H;
  case SDLK_i:
    return Key_I;
  case SDLK_j:
    return Key_J;
  case SDLK_k:
    return Key_K;
  case SDLK_l:
    return Key_L;
  case SDLK_m:
    return Key_M;
  case SDLK_n:
    return Key_N;
  case SDLK_o:
    return Key_O;
  case SDLK_p:
    return Key_P;
  case SDLK_q:
    return Key_Q;
  case SDLK_r:
    return Key_R;
  case SDLK_s:
    return Key_S;
  case SDLK_t:
    return Key_T;
  case SDLK_u:
    return Key_U;
  case SDLK_v:
    return Key_V;
  case SDLK_w:
    return Key_W;
  case SDLK_x:
    return Key_X;
  case SDLK_y:
    return Key_Y;
  case SDLK_z:
    return Key_Z;
  default:
    return Key_Unknown;
  }
}

ControllerButton
controller_button_sdl_controller_button(SDL_GameControllerButton button) {
  switch (button) {
  case SDL_CONTROLLER_BUTTON_A:
    return ControllerButton_A;
  case SDL_CONTROLLER_BUTTON_B:
    return ControllerButton_B;
  case SDL_CONTROLLER_BUTTON_X:
    return ControllerButton_X;
  case SDL_CONTROLLER_BUTTON_Y:
    return ControllerButton_Y;
  case SDL_CONTROLLER_BUTTON_BACK:
    return ControllerButton_Back;
  case SDL_CONTROLLER_BUTTON_START:
    return ControllerButton_Start;
  case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
    return ControllerButton_LShoulder;
  case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
    return ControllerButton_RShoulder;
  default:
    return ControllerButton_Unknown;
  }
}
void event_from_sdl_event(SDL_Event *sdl_event, Event *event) {
  ASSERT(sdl_event != NULL);
  ASSERT(event != NULL);

  switch (sdl_event->type) {
  case SDL_QUIT:
    event->type = EventType_Quit;
    break;
  case SDL_KEYDOWN:
    event->type = EventType_KeyDown;
    event->key_event.key = key_from_sdl_keysym(sdl_event->key.keysym.sym);
    break;
  case SDL_KEYUP:
    event->type = EventType_KeyUp;
    event->key_event.key = key_from_sdl_keysym(sdl_event->key.keysym.sym);
    break;
  case SDL_MOUSEMOTION:
    event->type = EventType_MouseMotion;
    event->mouse_motion_event.relative_x = sdl_event->motion.xrel;
    event->mouse_motion_event.relative_y = sdl_event->motion.yrel;
    break;
  case SDL_CONTROLLERBUTTONDOWN: {
    SDL_ControllerButtonEvent *button_event = &sdl_event->cbutton;
    ControllerButton controller_button =
        controller_button_sdl_controller_button(button_event->button);
    event->type = EventType_ControllerButtonDown;
    event->controller_button_event.button = controller_button;
    break;
  }
  case SDL_CONTROLLERBUTTONUP: {
    SDL_ControllerButtonEvent *button_event = &sdl_event->cbutton;
    ControllerButton controller_button =
        controller_button_sdl_controller_button(button_event->button);
    event->type = EventType_ControllerButtonUp;
    event->controller_button_event.button = controller_button;
    break;
  }
  case SDL_CONTROLLERAXISMOTION: {
    SDL_ControllerAxisEvent *axis_event = &sdl_event->caxis;
    event->type = EventType_ControllerJoystickMotion;

    float joystick_value = sdl_event->caxis.value / 32768.f;
    event->controller_joystick_motion_event.value = joystick_value;
    switch (axis_event->axis) {
    case SDL_CONTROLLER_AXIS_LEFTX:
      event->controller_joystick_motion_event.joystick =
          ControllerJoystick_Left;
      event->controller_joystick_motion_event.axis = ControllerJoystickAxis_X;
      break;
    case SDL_CONTROLLER_AXIS_LEFTY:
      event->controller_joystick_motion_event.joystick =
          ControllerJoystick_Left;
      event->controller_joystick_motion_event.axis = ControllerJoystickAxis_Y;
      break;
    case SDL_CONTROLLER_AXIS_RIGHTX:
      event->controller_joystick_motion_event.joystick =
          ControllerJoystick_Right;
      event->controller_joystick_motion_event.axis = ControllerJoystickAxis_X;
      break;
    case SDL_CONTROLLER_AXIS_RIGHTY:
      event->controller_joystick_motion_event.joystick =
          ControllerJoystick_Right;
      event->controller_joystick_motion_event.axis = ControllerJoystickAxis_Y;
      break;
    default:
      event->controller_joystick_motion_event.joystick =
          ControllerJoystick_Unknown;
      event->controller_joystick_motion_event.axis =
          ControllerJoystickAxis_Unknown;
      break;
    }
    break;
  }
  default:
    event->type = EventType_Unknown;
    break;
  }
}
