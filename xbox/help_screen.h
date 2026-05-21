#pragma once
/* help_screen.h -- GLSnake Xbox controls overlay interface. */

#ifdef __cplusplus
extern "C" {
#endif

void xbox_help_init    (void);
void xbox_help_shutdown(void);
void xbox_help_toggle  (void);
int  xbox_help_visible (void);
/* Call before FakeSwapBuffers when visible; screen_w/h are logical display dims. */
void xbox_help_render  (int screen_w, int screen_h);

#ifdef __cplusplus
}
#endif
