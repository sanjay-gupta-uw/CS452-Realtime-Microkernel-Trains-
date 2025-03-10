#ifndef _dummy_h_
#define _dummy_h_

extern "C" void __SError__(int group);
extern "C" void __IRQ__(int group);
extern "C" void __FIQ__(int group);
extern "C" void __Sync__(int group);

#endif // _dummy_h_