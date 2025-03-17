#ifndef _io_h_
#define _io_h_
namespace IO_NS
{
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
#define SMOOTH_SCROLL "\033[?4h"
#define JUMP_SCROLL "\033[?4l"
#define SCROLL_REGION "\033[%d;%dr"
#define SCROLL "\033D"
#define ORIGIN "\033[?6h"
#define UP "\033M"
#define DOWN "\033D"
#define RIGHT "\033C"
#define LEFT "\033D"
#define COLUMN_132 "\033[?3h"
#define AUTO_WRAP "\033[?7h"

#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"

#define CHANGE_COLUMN "\033[%dG"

    // #define LIGHT_MODE "\033[?5h"
    // #define DARK_MODE "\033[?5l"

#define RET_BUF_SIZE 256

    extern "C" void Print(const char *fmt, ...);
    extern "C" void PrintTerminal(const char *fmt, ...);

}

#endif //   _io_h_
