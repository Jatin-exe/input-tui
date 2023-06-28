#include <stdint.h>

extern int serial_port;
int cdc_send(uint8_t *buf, uint16_t len);
int cdc_recv(uint8_t *buf, uint16_t len);
int cdc_putchar(uint8_t);
int cdc_getchar(uint8_t *);
void HAL_Delay(uint32_t delay);
//////////////////////////////////////
#define LEN(X) sizeof(X) / sizeof(X[0]) 
// i chnaged this ; not yet
// as strlen woudl be better ig ? 
// cuz we get htenoumber of strings 
#define MV 35
#define BI0S "bi0sHardware" // tagline - footer


/* This structure represents a single line of the file we are editing. */
typedef struct erow {
    int idx;            /* Row index in the file, zero-based. */
    int size;           /* Size of the row, excluding the null term. */
    int rsize;          /* Size of the rendered row. */
    char *chars;        /* Row content. */
    char *render;       /* Row content "rendered" for screen (for TABs). */
} erow;

uint8_t editor_handler();

#define MAX_INPUT 20 // box width

struct InputBox { 
    int y;
    int x;
    char label[MAX_INPUT];
    char input_buff[MAX_INPUT];
    char saved_buff[MAX_INPUT];
    int saved_sel; // 0 if cursor is on input; 1 if on SAVE button
    int MAX_LEN;
    struct editorConfig *E;
};

struct editorConfig {
    int cx,cy;  /* Cursor x and y position in characters */
    int x_pos, y_pos ;  /* The x and y position from where the editor box starts*/
    int rowoff;     /* Offset of row displayed. */
    int coloff;     /* Offset of column displayed. */
    int screenrows; /* Number of rows that we can show */
    int screencols; /* Number of cols that we can show */
    int numrows;    /* Number of rows */
    int rawmode;    /* Is terminal raw mode enabled? */
    erow *row;      /* Rows */
    //status message here if needed
    struct InputBox *input_box;

};


/* Mappigns of Keys and their codes*/
enum KEY_ACTION{
        KEY_NULL = 0,       /* NULL */
        CTRL_C = 3,         /* Ctrl-c */
        CTRL_D = 4,         /* Ctrl-d */
        CTRL_F = 6,         /* Ctrl-f */
        CTRL_H = 8,         /* Ctrl-h */
        TAB = 9,            /* Tab */
        CTRL_L = 12,        /* Ctrl+l */
        ENTER = 13,         /* Enter */
        CTRL_Q = 17,        /* Ctrl-q */
        CTRL_S = 19,        /* Ctrl-s */
        CTRL_U = 21,        /* Ctrl-u */
        ESC = 27,           /* Escape */
        BACKSPACE =  127,   /* Backspace */
        /* The following are just soft codes, not really reported by the
         * terminal directly. */
        ARROW_LEFT = 203,
        ARROW_RIGHT = 202,
        ARROW_UP = 200,
        ARROW_DOWN = 201,
        DEL_KEY = 126,
        HOME_KEY,
        END_KEY,
        PAGE_UP,
        PAGE_DOWN,

        
};