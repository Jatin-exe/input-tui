#include "editor.h"
#include "editor-config.h"


#include "ansi.h"
#include <stdint.h>// includes uint8_t
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>  // write(), read(), close()
#include <math.h>
#include <string.h>



// #define ESC "\x1b" already definedin enums



// iif status needed use thisvoid editorSetStatusMessage(const char *fmt, ...);


/*
    menu_send("\x1b[s", 3); // saves current curosr pos in DEC  
    menu_send("\x1b[u",3); // restore saved cursor from DEC
*/



static uint8_t (*menu_send)(uint8_t *buf, uint16_t len)  = cdc_send;
static uint8_t (*menu_recv)(uint8_t *buf, uint16_t len) = cdc_recv;
static uint8_t (*menu_putchar)(uint8_t) = cdc_putchar;
static uint8_t (*menu_getchar)(uint8_t *) = cdc_getchar;



// TRUNCATION IF I DONT CHANGE THIS TO INT 
static uint8_t hide_cursor[6] = {"\x1b[?25l"}; 
static uint8_t show_cursor[6] = {"\x1b[?25h"};

static uint8_t save_cursor[3] = {"\x1b[s"};
static uint8_t load_cursor[3] = {"\x1b[u"};



static uint8_t seq_clear[8] = {"\x1b[2J\x1b[3J"}; //\ 033[K \033[H
static uint8_t seq_clear_2[4] = {"\x1b[H"};



static uint8_t seq_bgcolor[7] = BGPURPLE;
static uint8_t seq_reset[4] = {"\x1b[0m"};
static uint8_t seq_fgcolor[7] = FGWHITE;

struct InputBox * sel_ptr = NULL;


void clear(){
  menu_send(seq_clear, LEN(seq_clear));
  menu_send(seq_clear_2, LEN(seq_clear_2));
}


void reset() { menu_send(seq_reset, LEN(seq_reset)); }
void setbg() { menu_send(seq_bgcolor, LEN(seq_bgcolor)); }



  // send each digit of a 3 digit number as characters
void put_int(uint_fast16_t i){
  // WORKS ONLY FOR THREE DIGIT NUMBER 
  char no[3] = {}; // turn this back to uint_8t if needed
  int x;
  int itr = 2;
  while(i!=0){
    x = i%10;
    i /= 10;
    no[itr] = (char)x + '0';
    itr--;
  }
  
  menu_putchar(no[0]);
  menu_putchar(no[1]);
  menu_putchar(no[2]);
}



void move(uint_fast16_t y, uint_fast16_t x){
  // you can move the cursor position with this fn before printing a specific text or while setting bg
  menu_putchar(0x1b);
  menu_putchar('[');
  put_int(y);
  menu_putchar(';');
  put_int(x);
  menu_putchar('H');
}


// move cursor relatively, A for top , B for bottom, C for rigth and D for left, n defines number if needed
void moveR(char c, int n){ 
    menu_send("\x1b[",2);
    if(n>1) put_int(n);
    menu_putchar(c);
}

/* Handle cursor position change because arrow keys were pressed. */
void editorMoveCursor(struct InputBox * IBox, int key) {
    switch(key) {
    case ARROW_LEFT:
        if (IBox->cx != 0) IBox->cx -= 1;
        break;

    case ARROW_RIGHT:
        if (IBox->cx != IBox->buff_size) IBox->cx += 1;
        break;
    }
}



/* This function writes the whole screen using VT100 escape characters
 * starting from the logical state of the editor in the global state 'E'. */
void editorRefreshScreen(struct InputBox * IBox) {

    menu_send(hide_cursor,LEN(hide_cursor));
    move(IBox->y, IBox->x + strlen(IBox->label) + 1); // move cursor to the current position

    // clear the menu and re render using k and otehr asci codes

    for(int i =0;i< MAX_INPUT; i++){ // or inputBox->MAX_LEN
        menu_send(" ", 1);
    }
    moveR('D',MAX_INPUT);

    char buff[IBox->buff_size+1];
    char *c = IBox->input_buff; 




    for (int j =0 ;j< IBox->buff_size ;j++){                

        buff[j] = (char)*(c+j);
    }

    menu_send(buff, IBox->buff_size); // strlen instead of LEN

    move(IBox->y,IBox->x + strlen(IBox->label) + IBox->cx +1); // move cursor to the current position
    menu_send("\x1b[?25h",6); /* Show cursor. */

    menu_send(show_cursor,LEN(show_cursor));
}

/* Read a key from the terminal put in raw mode, trying to handle
 * escape sequences. */
uint8_t parse_input(uint8_t c) {
    uint8_t seq[3];


    while(1) {
        switch(c) {
        case ESC:    /* escape sequence */
            /* If this is just an ESC, we'll timeout here. */
            if (menu_getchar(seq) == 0) return ESC;
            if (menu_getchar(seq+1) == 0) return ESC;

            

            /* ESC [ sequences. */
            if (seq[0] == '[') {

                if (seq[1] >= '0' && seq[1] <= '9') {
                    /* Extended escape, read additional byte. */
                    if (menu_getchar(seq+2) == 0) return ESC;
                    if (seq[2] == '~') {
                        return DEL_KEY;
                    }
                }
                else {
                    switch(seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    }
                }
            }

            break;

        default:
            return c;
        }
    }
}




/* Insert a character at the specified position in the input Box */
void editorInsertChar(struct InputBox * IBox, int c) {

    if(IBox->buff_size >= IBox->MAX_LEN) return; // max len of input box reached
    int at = IBox->cx;

    IBox->input_buff = realloc(IBox->input_buff, IBox->buff_size+2);
    memmove(IBox->input_buff+at+1,IBox->input_buff+at,IBox->buff_size-at+1);
    IBox->buff_size++;
    
    IBox->input_buff[at] = c;
    IBox->cx++;
}








/* Delete the char at AT */
void editorDelChar(struct InputBox * IBox, int at ) {
    if (at<0 || at >= IBox->buff_size) return;
    memmove(IBox->input_buff+at,IBox->input_buff+at+1,IBox->buff_size-at);
    IBox->buff_size--;
}


/* Process events arriving from the standard input, which is, the user
 * is typing stuff on the terminal. */
void input_handler(struct InputBox *IBox, int fd) {


    uint8_t c = 0;

    while(c==0)
        menu_getchar(&c); 
    
    printf("%c",c);fflush(0);
    c = parse_input(c); // parsing the ESCAPE sequences
    printf("cx:%d\n",IBox->cx);fflush(0);


    switch(c) {
    case ESC:
        // anything to do upon esc ?
        break;
    case ENTER:         /* Enter */
        
        strcpy(sel_ptr->saved_buff, sel_ptr->input_buff);
        move(999,0);
        menu_send("\x1b[K", 3);
        uint8_t buff[40];
        sprintf(buff, "Saved: %s", sel_ptr->saved_buff);
        menu_send(buff, strlen(buff));
        break;


    /* Maybe Insert mode for the editor instead fo normal editor mdoe ?*/
    case BACKSPACE:
        if(IBox->cx == 0) break;
        editorDelChar(IBox, IBox->cx-1);
        IBox->cx--;
        break;
    case DEL_KEY:
        editorDelChar(IBox, IBox->cx+1);
        break;

    case ARROW_UP:
    case ARROW_DOWN:
        if (sel_ptr->saved_sel == 0) {
            sel_ptr->saved_sel = 1;
        } else {
            sel_ptr->saved_sel = 0;
        }
        break;

    case ARROW_LEFT:
    case ARROW_RIGHT:

        editorMoveCursor(IBox, c);
        break;
    default:
        sel_ptr->saved_sel == 0; // unselects the save button
        editorInsertChar(IBox, c);

        break;
    }

}





void redraw()
{
// not using rn 
}






// works for one row input boxes only
void drawInputBox(struct InputBox * input_box){ 

    menu_send(save_cursor, LEN(save_cursor));
    menu_send(hide_cursor, LEN(hide_cursor));

    move(input_box->y, input_box->x);
    menu_send(input_box->label, LEN(input_box->label));
    char ss[10] = {"║"};
    menu_send(ss, strlen(ss));

    moveR('A',1);
    moveR('D',1);
    sprintf(ss, "╔");
    menu_send(ss, strlen(ss));

    for(int i =0;i< MAX_INPUT;i++){
        sprintf(ss,"=");
        menu_send(ss, strlen(ss));
    }
    sprintf(ss,"╗");
    menu_send(ss, strlen(ss));
    moveR('B',1);
    moveR('D',1);

    sprintf(ss,"║");
    menu_send(ss, strlen(ss));

    moveR('B',1);
    moveR('D',1);
    sprintf(ss,"╝");
    menu_send(ss,strlen(ss));

    moveR('D',MAX_INPUT+2);

    sprintf(ss,"╚");
    menu_send(ss,strlen(ss));

    sprintf(ss, "═");
    for(int i =0;i< MAX_INPUT;i++){
        menu_send(ss, strlen(ss));
    }


    // save button here if needed

    menu_send(load_cursor, LEN(load_cursor));
    menu_send(show_cursor, LEN(show_cursor));
}


/*

            ╔═══════════════════════════╗
  Team Name:║asdfas                     ║
            ╚═══════════════════════════╝

*/


uint8_t editor_handler(){


    struct InputBox team_var;
    team_var.x = 10; // make this responsive
    team_var.y = 10;
    team_var.MAX_LEN = MAX_INPUT;
    strcpy(team_var.label,"Team Name:");

    team_var.buff_size = 0;
    team_var.input_buff = malloc(MAX_INPUT+1);
    team_var.input_buff[0] = '\0';

    team_var.cx = 0;


    struct InputBox *team_name  = &team_var;


    sel_ptr = team_name;
    sel_ptr->saved_sel = 0; // cursor on input 


    drawInputBox(team_name);

    move(team_name->y, team_name->x + strlen(team_name->label) + 1); // initial cursor placement

    while (1){
        // drawInputBox(team_name); no need since we are not erasing that 
        input_handler(team_name, serial_port);// o for std input
        editorRefreshScreen(team_name); // refresh struct / input box
        redraw();
    }

    
    return 0; 
}
  



/* STUFF TO DO*/
/*
ctrl a and all are still being printed
make sure all pointers are being freed
flickering of text
responsiveness

*/


/* BUtton stuff not using rn*/

/* 


void setForegroundColor(int color) {

    uint8_t buf[20];
    sprintf(buf,"%s[38;5;%dm", "\x1b", color);

    menu_send(buf, strlen(buf));

}

void setBackgroundColor(int color) {
    uint8_t buf[20];
    sprintf(buf,"%s[48;5;%dm", "\x1b", color);
    menu_send(buf, strlen(buf));

}

void resetColors() {
    uint8_t buf[20];
    sprintf(buf,"%s[0m", "\x1b");
    menu_send(buf, strlen(buf));
}

void drawButton(int x, int y, int width, char* label) {
    // setCursorPosition(x,y ); // specifc and standaride weather we are using   x and y or y and x as representd in lines and colum
    // setForegroundColor(0);      // Set text color to black
    setBackgroundColor(15);    // Set background color to whites
    uint8_t buf[20];
    sprintf(buf," %-*s ", width-2, label);

    menu_send(buf,strlen(buf));

    resetColors();
}


*/

