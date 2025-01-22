#include "train.h"
#include "clock.h" // this includes util which includes rpi.h

// ********* Train Class *********
// initialize with invaid train number
Train::Train()
{
   train_num = -1;
   train_speed = 0;
}

Train::~Train()
{
}

// use this constructor to set the train number
Train::Train(int train_num) : train_num(train_num)
{
   train_speed = 0;
}

void Train::ActivateHeadlight()
{
   int data = 16;
   SendCommand(data);
}

void Train::Accelerate(int speed, bool headlightOn)
{
   // speed == -1 means reverse with current speed
   if (speed == -1)
   {
      speed = train_speed;
   }
   int data = speed + (headlightOn ? 16 : 0);
   train_speed = speed;
   SendCommand(data);
}

void Train::Reverse()
{
   int data = 15;
   SendCommand(data);
}

void Train::Stop()
{
   train_speed = 0;
   SendCommand(0);
}

// int is_valid_train(int train_num)
// {
//    for (int i = 0; i < 5; ++i)
//    {
//       if (trains[i].train_num == train_num)
//          return i;
//    }
//    return -1;
// }

// bool is_moving(int idx)
// {
//    return (trains[idx].train_speed > 0);
// }

void Train::SendCommand(int data)
{
   uart_putc(MARKLIN, (uint8_t)data);
   uart_putc(MARKLIN, (uint8_t)train_num);
   clock.Delay(30);
}

// ********* End Train Class *********

// ********* Trains Class *********
Trains::Trains()
{
   // initialize with invaid train number
   const int TRAIN_NUMS[5] = {1, 54, 55, 58, 77};
   for (int i = 0; i < 5; ++i)
   {
      trains[i].train_num = TRAIN_NUMS[i];
   }
}

Trains::~Trains()
{
}

int Trains::ActivateHeadlight(int train_num)
{
   for (int i = 0; i < 5; ++i)
   {
      if (trains[i].train_num == train_num)
      {
         trains[i].ActivateHeadlight();
         return 0;
      }
   }
   return -1;
}

int Trains::Accelerate(int train_num, int speed, bool headlightOn)
{
   for (int i = 0; i < 5; ++i)
   {
      if (trains[i].train_num == train_num)
      {
         trains[i].Accelerate(speed, headlightOn);
         return 0;
      }
   }
   return -1;
}

int Trains::Reverse(int train_num)
{
   for (int i = 0; i < 5; ++i)
   {
      if (trains[i].train_num == train_num)
      {
         // move_cursor(CONSOLE, COMMAND_STATUS_LOCATION, 1);
         // clear_to_end_line(CONSOLE);
         int speed = trains[i].train_speed;
         if (speed > 0)
         {
            // uart_printf(CONSOLE, "Stopping train {%d} before reversing", train_num);
            trains[i].Stop();
            clock.Delay(4000);
         }
         // move_cursor(CONSOLE, COMMAND_STATUS_LOCATION, 1);
         // clear_to_end_line(CONSOLE);

         // uart_printf(CONSOLE, "Reversing train {%d}", train_num);
         trains[i].Reverse();
         trains[i].Accelerate(speed, false);
         return 0;
      }
   }
   return -1;
}

int Trains::Stop(int train_num)
{
   for (int i = 0; i < 5; ++i)
   {
      if (trains[i].train_num == train_num)
      {
         trains[i].Stop();
         return 0;
      }
   }
   return -1;
}

// ********* End Trains Class *********