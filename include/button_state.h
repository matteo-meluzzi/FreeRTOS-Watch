#ifndef BUTTON_STATE_H
#define BUTTON_STATE_H

#include "state_fp.h"

typedef voidfp (* button_statefp)(bool long_press, bool short_press);

voidfp not_pressed(bool long_press, bool short_press);
voidfp pressed(bool long_press, bool short_press);
voidfp pressed_long(bool long_press, bool short_press);


#endif