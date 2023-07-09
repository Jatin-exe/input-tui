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




uint8_t editor_handler();

#define MAX_INPUT 20 // box width

struct InputBox { 
    int y, x; /* x and y pos of the input box top left point*/
    int cx;  /* Cursor x position in the input buffer */
    char label[MAX_INPUT]; /* name of the label field*/
    char saved_buff[MAX_INPUT]; /* var for the saved buffer */
    int saved_sel; /* 0 if cursor is on input; 1 if on SAVE button */
    int MAX_LEN; /* max length of the input buffer */
    int buff_size; /*size of the current input*/ // including null term for now

    char * input_buff; /* input_buff to show in the term*/
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