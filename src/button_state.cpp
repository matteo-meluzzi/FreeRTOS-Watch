
#include "button_state.h"

voidfp not_pressed(bool long_press, bool short_press) {
  if (!long_press && !short_press) return (voidfp) pressed;
  return (voidfp) not_pressed;
}
voidfp pressed(bool long_press, bool short_press) {
  if (!long_press && short_press) return (voidfp) not_pressed;
  if (long_press && !short_press) return (voidfp) pressed_long;

  return (voidfp) not_pressed;
}
voidfp pressed_long(bool long_press, bool short_press) {
  return (voidfp) not_pressed;
}