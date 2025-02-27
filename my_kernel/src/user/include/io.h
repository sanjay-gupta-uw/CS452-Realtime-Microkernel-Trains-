#ifndef _io_h_
#define _io_h_

#define RET_BUF_SIZE 256

#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_BLACK "\033[30m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"

#define RESET_FORMATTING "\033[0m"
// #define CLEAR_SCREEN "\033[2J"
#define CLEAR_SCREEN " "
#define CLEAR_TO_END_LINE "\033[K"
#define MOVE_CURSOR "\033[%d;%dH"
#define SAVE_CURSOR "\033[s"
#define RESTORE_CURSOR "\033[u"
class IO
{

    int IO_SERVER_TID;

public:
    IO();
    ~IO();

    void Print(const char *fmt, ...);
    void pad_and_print(const char *str, int width);

    void reset_formatting();
    void clear_screen();
    void clear_to_end_line();
    void move_cursor(int row, int col);
    void SaveCursor();
    void RestoreCursor();

    void color_red();
    void color_green();
    void color_black();
    void color_yellow();
    void color_blue();
    void color_magenta();
    void color_cyan();
    void color_white();
};

#endif //   _io_h_
