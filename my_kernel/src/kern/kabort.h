#ifndef _kabort_h_
#define _kabort_h_

// accept condition, __LINE__, __FILE__ from kassert
void kabort(const char *condition, int line, const char *file, int EL);

#endif // _kabort_h_