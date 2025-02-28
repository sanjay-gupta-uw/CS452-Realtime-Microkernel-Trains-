#ifndef _io_h_
#define _io_h_
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_BLACK "\033[30m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"

#define RESET_FORMATTING "\033[0m"
#define CLEAR_SCREEN "\033[2J"
#define CLEAR_TO_END_LINE "\033[K"
#define MOVE_CURSOR "\033[%d;%dH"
#define SAVE_CURSOR "\033[s"
#define RESTORE_CURSOR "\033[u"
namespace IO_NS
{

#define RET_BUF_SIZE 256

    class IO
    {
    public:
        IO();
        ~IO();
    };

    extern "C" void Print(const char *fmt, ...);

}

#endif //   _io_h_
