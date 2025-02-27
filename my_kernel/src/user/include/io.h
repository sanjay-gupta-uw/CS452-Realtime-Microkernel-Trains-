#ifndef _io_h_
#define _io_h_

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
