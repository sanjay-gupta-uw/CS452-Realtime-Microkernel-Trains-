## 1.0 Overview

This program communicates with a Marklin trainset by issuing commands to affecting the train speed/direction, the orientation of switches along the track, and processes sensor data to keep track of recently triggered sensors.

## 2.0 Program Operation

### 2.1 Compilation

We compile an image that gets directly uploaded to the Rasberry Pi's as follows.
First to compile the code, navigate from the root directory to
`assignments/a0/` and issue the `make` command. This should compile without critical errors. Then navigate back to the root folder, (the assignments directory should belong to this current working directory). Issue the following command:

`./upload.sh assignments/a0/iotest.img d8:3a:dd:1b:36:9e`
to upload the image to the rasberry pi.

### 2.2 CMD Usage

The program will initialize a collection variables, corresponding to the clock, switches, sensors, and train state.After this initialization, the UI will load and the following commands will be available to the user:

- `tr <train_number> <train_speed>`

- - Set a given train to a desired speed (from 0-14, 15 is stop, add 16 for headlights in facing direction)

- `rv <train_number>`
- - Reverse the train

- `sw <switch_number> <switch_direction>
- - Change the switch to Straight (S) or Curved/Branched (C)

Also note that the code treats the following trains as valid: 1, 54, 55, 58, 77.

## 3.0 Program Description

## 3.1 Polling Loop

The polling loop functions as follows:

- Update the clock variable by converting the CLO register into elapsed tenths, and only updating the physical clock when the display would result in a change (boolean flag).
- Update the UI to display recently triggered sensors, active switch configurations, and the command prompt for users to communicate directly with the Marklin.
- We then check if a command was entered by the user, and process this command into a status type structure -- when the user enters a successfully defined command, the command is turned into a command datatype, containing the address and data associated for the command (if applicable). This is then pushed into a ring buffer to throttle requests sent to the hardware (also note that our handle_command function echoes back characters to the command prompt until the user submits the command -- backspace also works for deletion).
- We then check if our command buffer is empty, if it's not we dequeue the command and parse it (explained below).
- Finally, we query the sensors if we are done processing commands.

## 3.2 Parsing

- A switch harness is used to detect the command -- ensuring the start of the command is recognized.
- Each case invokes an associated function that communicates directly with the Marklin controller. For example, when we issue the tr command, assuming a valid input, the code will invoke the accelerate_train method that will pass the speed and address as a byte each to command the Marklin to control the train.
- We also define an appropriate reverse catch; it checks if a train is moving, and if so it makes it come to a stop. After a delay of 4s, we call reverse_train() and then accelerate_train() to make it move in the reverse direction. Note that we maintain an array of the trains and there set speeds to restore such speeds on reversals.

## 3.3 Organization

The code is organized into distinct files to separate core functionality, with the aim of converting the codebase to C++ in the future for added safety.

- Defined two ringbuffer files, one that uses a command buffer and another for integers (used to indicate recently triggered sensors).
- Created train, switch, and sensor which are defined ffor the purpose of communicating with the Marklin. We maintain a state array of switches, trains, and sensors in order to create a "view" of the physical system.
- `command.h` is used for user commands (processing/handling/parsing) -- handle_command() is used to update the display as the user types and passes the code to process_command() when a command is submitted. process_command() creates a command structure if the given commmand is valid, which is then passed to the parsing harness which redirects to the appropriate controllers (as discussed above).
- `ui.h` defines the locations for updated prints using ANSI escape codes for formatting. Within each of the train/command/sensor/switch files, there are init_display() and display() functions. The init_display's are drawn once at program load, and every time we execute the polling loop, we use the corresponding display() functions to update components of the UI.

## 3.4 Datastructures

- We mentioned the ring buffer above. This is used as a circular memory management tool where we update the head for each data push, and the tail for each read/pop; this gives us the queue-like behaviour, while simply wrapping around when we exceed the end of the buffer. Note that if the command buffer is full, we skip the incoming new commands to prevent a stall.
- We have our arrays for tracking the state of the track:
- - Array of train structs: speed, train number (need to remove isActive flag in future)
- - Array of switch structs: address and orientaion
    -- Array of sensor structs: contains the bank (A-E), id (1-16), and status (1 if sensor was recently triggered).

## 3.5 Known Bugs

- There is a slight bug with printing the latency:

      // uint32_t time_start = get_current_time();

      sensor_read_all(NUM_BANKS, &sensor_buffer); // keep this

      /*
      uint32_t end_time = get_current_time();`

      move_cursor(CONSOLE, 70, 1);

      clear_to_end_line(CONSOLE);

      uart_printf(CONSOLE, "Latency: {%d} ", (end_time - time_start));

      */

- I've removed this because the terminal was getting bugged, but if it's un commented out it will clearly display the response time for recieving the sensor data at the bottom.

- Finally, there is a bug in the recently triggered sensors, where the same sensor gets spammed in the queue (for example, multiple lines could be C9), but running the trains across the tracks demonstrates the functionality of the sensors. (migraine prevented me from fixing these issues properly)

## 4.0 Answers

### 4.1 Clock

- We ensure the clock does not lose time by carefully creating non-blocking calls that can update the clock during timer delays.
- We also consider the time it takes for the clock to update within a single loop: without timing the whole loop, and before the update_display flag was added, the terminal would reprint the same time repeatedly -- this implies that the time of the loop is well under 1/10 of a second, since the lowest unit of time we track is elapsed tenths of a second.

### 4.2 Sensor Query

- The hardware takes approxiamtely 20010 microseconds to reply to a sensor query (Note that this is for the time it takes the the 10 bytes of sensor data to be passed and is what is currently commented out in the main.c file). The time for a single byte to be recieved was initially coded in the sensor.c file, though the results were misleading as it depended on the clock delay after issuing the sensor command.
