#include "TestClass.h"

int id = 0;

// Constructor
TestClass::TestClass() : id_(id++)
{
   uart_printf(CONSOLE, "TestClass object %u created!\r\n", id_);
}

// Destructor
TestClass::~TestClass()
{
}

// Print a message
void TestClass::printMessage() const
{
   uart_printf(CONSOLE, "Hello from TestClass object %u!\r\n", id_);
}
