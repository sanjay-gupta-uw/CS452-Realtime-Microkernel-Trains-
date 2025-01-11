#ifndef _switch_h
#define _switch_h

// switch state enum
typedef enum
{
   STRAIGHT,
   CURVED
} SwitchState;

typedef struct
{
   // switch state
   SwitchState state;
} Switch;

void switch_init(Switch *sw);
void switch_toggle(Switch *sw);
void switch_display(const Switch *sw);

#endif