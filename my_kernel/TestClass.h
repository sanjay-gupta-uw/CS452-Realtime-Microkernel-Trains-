#ifndef TESTCLASS_H
#define TESTCLASS_H

extern int id; // Declaration

#include "rpi.h"

class TestClass
{
public:
   TestClass();  // Constructor
   ~TestClass(); // Destructor

   void printMessage() const; // Method to print a message

private:
   int id_;
};

#endif // TESTCLASS_H
