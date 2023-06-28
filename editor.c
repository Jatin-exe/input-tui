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



static uint8_t (*menu_send)(uint8_t *buf, uint16_t len)  = cdc_send;
static uint8_t (*menu_recv)(uint8_t *buf, uint16_t len) = cdc_recv;
static uint8_t (*menu_putchar)(uint8_t) = cdc_putchar;
static uint8_t (*menu_getchar)(uint8_t *) = cdc_getchar;



// TRUNCATION IF I DONT CHANGE THIS TO INT 
static uint8_t hide_cursor[6] = {"\x1b[?25l"}; 
static uint8_t show_cursor[6] = {"\x1b[?25h"};

static uint8_t save_cursor[3] = {"\x1b[s"};
static uint8_t load_cursor[3] = {"\x1b[u"};



static uint8_t seq_clear[8] = {"\x1b[2J\x1b[3J"}; //\033[K  \033[H
static uint8_t seq_clear_2[4] = {"\x1b[H"};



static uint8_t seq_bgcolor[7] = BGPURPLE;
static uint8_t seq_reset[4] = {"\x1b[0m"};
static uint8_t seq_fgcolor[7] = FGWHITE;



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











// defined MAX_LEN for later use 

// move cursor relatively, A for top , B for bottom, C for rigth and D for left, n defines number if needed
void moveR(char c, int n){ 
    menu_send("\x1b[",2);
    if(n>1) put_int(n);
    menu_putchar(c);
}





/* Handle cursor position change because arrow keys were pressed. */
void editorMoveCursor(struct editorConfig * E, int key) {
    int filerow = E->rowoff+E->cy;
    int filecol = E->coloff+E->cx;
    int rowlen;
    erow *row = (filerow >= E->numrows) ? NULL : &E->row[filerow];

    switch(key) {
    case ARROW_LEFT:
        if (E->cx == 0) {
            if (E->coloff) {
                E->coloff--;
            } else {
                if (filerow > 0) {
                    E->cy--;
                    E->cx = E->row[filerow-1].size;
                    if (E->cx > E->screencols-1) {
                        E->coloff = E->cx-E->screencols+1;
                        E->cx = E->screencols-1;
                    }
                }
            }
        } else {
            E->cx -= 1;
        }
        break;
    case ARROW_RIGHT:
        if (row && filecol < row->size) {
            if (E->cx == E->screencols-1) {
                E->coloff++;
            } else {
                E->cx += 1;
            }
        }


        break;

    }



    /* Fix cx if the current line has not enough chars. */
    filerow = E->rowoff+E->cy;
    filecol = E->coloff+E->cx;
    row = (filerow >= E->numrows) ? NULL : &E->row[filerow];
    rowlen = row ? row->size : 0;
    if (filecol > rowlen) {
        E->cx -= filecol-rowlen;
        if (E->cx < 0) {
            E->coloff += E->cx;
            E->cx = 0;
        }
    }
}



/* This function writes the whole screen using VT100 escape characters
 * starting from the logical state of the editor in the global state 'E'. */
void editorRefreshScreen(struct editorConfig * E,struct InputBox * input_box) {
    int y;
    erow *r;
    char buf[32];
    // struct abuf ab = ABUF_INIT;

    menu_send(hide_cursor,LEN(hide_cursor));


    move(E->y_pos, E->x_pos);
    
    // abAppend(&ab,"\x1b[10;11H",8); // does nto clear screen same as ctrl + l ig
    // line are at 10;10 so text form colum 11 just use varibes and add one here bro 
            // abAppend(&ab, "\x1b[10C", 5);

    // abAppend(&ab,"\x1b[?25l",6); /* Hide cursor. */ // put up and down to see if gbetter idk 
    // hideCurosr();

    
        // int filerow = E.rowoff;

        // if (filerow >= E.numrows) 



            // maybe implement somethign here ? footer 
            //

        // menu_send("\x1b[0K",4); // method to not have to calculte is better change menu_send

        for(int i =0;i< MAX_INPUT; i++){ // or inputBox->MAX_LEN
            menu_send(" ", 1);
        }
        moveR('D',MAX_INPUT);



        r = &E->row[E->rowoff];

        int len = r->rsize - E->coloff;

        char buff[len+len];
        if (len > 0) {
            if (len > E->screencols) len = E->screencols; // we gotta render what we can show so 

            char *c = r->render+E->coloff;  // usually zero if the line is not too big?


            for (int j =0 ;j<len ;j++){                
                // abAppend(&ab,c+j,1);
                buff[j] = (char)*(c+j);
            }
        }
        menu_send(buff, len); // strlen instead of LEN

    /* Put cursor at its current position. Note that the horizontal position
     * at which the cursor is displayed may be different compared to 'E.cx'
     * because of TABs. */

    int j;
    int cx = 1;
    int filerow = E->rowoff+E->cy;
    erow *row = (filerow >= E->numrows) ? NULL : &E->row[filerow];
    if (row) {
        for (j = E->coloff; j < (E->cx+E->coloff); j++) {
            if (j < row->size && row->chars[j] == TAB) cx += 7-((cx)%8);
            cx++;
        }
    }






    move(E->y_pos,E->x_pos + cx-1); // move cursor to the current position
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

/* Update the rendered version of the row. */
void editorUpdateRow(erow *row) {
    unsigned int tabs = 0, nonprint = 0;
    int j, idx;

   /* Create a version of the row we can directly print on the screen,
     * respecting tabs, substituting non printable characters with '?'. */
    free(row->render);
    for (j = 0; j < row->size; j++)
        if (row->chars[j] == TAB) tabs++;

    unsigned long long allocsize =
        (unsigned long long) row->size + tabs*8 + nonprint*9 + 1;
    if (allocsize > UINT32_MAX) {
        printf("Some line of the edited file is too long\n");
        exit(1);
    }

    row->render = malloc(row->size + tabs*8 + nonprint*9 + 1);
    idx = 0;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == TAB) {
            row->render[idx++] = ' ';
            while((idx+1) % 8 != 0) row->render[idx++] = ' ';
        } else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->rsize = idx;
    row->render[idx] = '\0';


}


/* Insert a character at the specified position in a row, moving the remaining
 * chars on the right if needed. */
void editorRowInsertChar(erow *row, int at, int c) {
    if (at > row->size) {
        /* Pad the string with spaces if the insert location is outside the
         * current length by more than a single character. */
        int padlen = at-row->size;
        /* In the next line +2 means: new char and null term. */
        row->chars = realloc(row->chars,row->size+padlen+2);
        memset(row->chars+row->size,' ',padlen);
        row->chars[row->size+padlen+1] = '\0';
        row->size += padlen+1;
    } else {
        /* If we are in the middle of the string just make space for 1 new
         * char plus the (already existing) null term. */
        row->chars = realloc(row->chars,row->size+2);
        memmove(row->chars+at+1,row->chars+at,row->size-at+1);
        row->size++;
    }
    row->chars[at] = c;
    editorUpdateRow(row);

}



/* Insert a row at the specified position, shifting the other rows on the bottom
 * if required. */
void editorInsertRow(struct editorConfig * E, int at, char *s, size_t len) {
    if (at > E->numrows){  printf("return ");fflush(0);return;}

    
    E->row = realloc(E->row,sizeof(erow)*(E->numrows+1));


    if (at != E->numrows) {
        memmove(E->row+at+1,E->row+at,sizeof(E->row[0])*(E->numrows-at));
        for (int j = at+1; j <= E->numrows; j++) E->row[j].idx++;
    }



    E->row[at].size = len;
    E->row[at].chars = malloc(len+1);
    memcpy(E->row[at].chars,s,len+1);
    E->row[at].render = NULL;
    E->row[at].rsize = 0;
    E->row[at].idx = at;
    editorUpdateRow(E->row+at);
    E->numrows++;
}

/* Insert the specified char at the current prompt position. */
void editorInsertChar(struct editorConfig * E, int c) {



    int filerow = E->cy;

    int filecol = E->coloff+E->cx;
        erow *row = &E->row[filerow];


    /* If the row where the cursor is currently located does not exist in our
     * logical representaion of the file, add enough empty rows as needed. */

    if (!row) {
        while(E->numrows <= filerow){
            editorInsertRow(E, E->numrows,"",0);
        }
    }
    row = &E->row[filerow];

    editorRowInsertChar(row,filecol,c);
    if (E->cx == E->screencols-1)
        E->coloff++;
    else
        E->cx++;
}


/* Free row's heap allocated stuff. */
void editorFreeRow(erow *row) {
    free(row->render);
    free(row->chars);
}






/* Delete the character at offset 'at' from the specified row. */
void editorRowDelChar(erow *row, int at) {
    if (row->size <= at) return;
    memmove(row->chars+at,row->chars+at+1,row->size-at);
    editorUpdateRow(row);
    row->size--;

}

/* Delete the char at the current prompt position. */
void editorDelChar(struct editorConfig * E) {
    int filerow = E->cy;
    int filecol = E->coloff+E->cx;
    erow *row =  &(E->row[filerow]);

    if (!row || (filecol == 0 && filerow == 0)) return;
    
    editorRowDelChar(row,filecol-1);

    if (E->cx == 0 && E->coloff)
        E->coloff--;
    else
        E->cx--;
        

    // ^^ POSSILBE ERROS HERE FOR NOT RENDERING PROPERLY
    
    if (row) editorUpdateRow(row);
}


/* Process events arriving from the standard input, which is, the user
 * is typing stuff on the terminal. */

struct InputBox * sel_ptr = NULL;

void input_handler(struct editorConfig *E, int fd) {


    uint8_t c = 0; // THIS SHOULDL BE CHAR FIX !!

    while(c==0)
        menu_getchar(&c); // breaks only if we get a  input ?   --- possible errors ? bugs cuz handling only for chars idk
        
    c = parse_input(c); // parsing the ESCAPE sequences

    switch(c) {
    case ENTER:         /* Enter */
        strcpy(sel_ptr->saved_buff, sel_ptr->input_buff);
        

        break;

    case BACKSPACE:     /* Backspace */
    case DEL_KEY:
        editorDelChar(E);
        break;

    case ARROW_UP:
    case ARROW_DOWN:
    // sigterm if no input has been writeten  
        // change selection input to ubtton and back/cancel
        printf("wath");fflush(0);

        if (sel_ptr->saved_sel == 0) {
            sel_ptr->saved_sel = 1;
        } else {
            sel_ptr->saved_sel = 0;
        }
        break;

    case ARROW_LEFT:
    case ARROW_RIGHT:

        editorMoveCursor(E, c);
        break;
    default:
        
        editorInsertChar(E, c);
        break;
    }

}




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

void redraw()
{
    // drawEditor() have to implement properly too much drawing is uncessary only
    // right side fo the box is beign deleted when screen is being refreshed

 // clear();
 // impletn
     // drawStatsmsg(); footer maybe ?
    /// draw falcon ?? 
}


void initEditor(struct editorConfig *E, int y, int x, int height, int width) {
    
    E->cx = 0;
    E->cy = 0;
    E->rowoff = 0;
    E->coloff = 0;
    E->numrows = 0;
    E->row = NULL;

    E->y_pos = y;
    E->x_pos = x;

    E->screenrows = height;
    E->screencols = width;
}

/*
    menu_send("\x1b[s", 3); // saves current curosr pos in DEC  
    menu_send("\x1b[u",3); // restore saved cursor from DEC
*/


// works for one row input boxes only
void drawInputBox(struct InputBox * input_box){ 

    menu_send(save_cursor, LEN(save_cursor));
    
    menu_send(hide_cursor, LEN(hide_cursor));

    move(input_box->y, input_box->x);
    menu_send(input_box->label, LEN(input_box->label));
    // menu_putchar('|');
    char ss[10] = {"║"};
    menu_send(ss, strlen(ss));

    moveR('A',1);
    moveR('D',1);
    sprintf(ss, "╔");
    menu_send(ss, strlen(ss));
    // menu_putchar('=');
    // menu_putchar('+');

    for(int i =0;i< MAX_INPUT;i++){
        sprintf(ss,"=");
        menu_send(ss, strlen(ss));
        // menu_putchar('-');
    }
    sprintf(ss,"╗");
    menu_send(ss, strlen(ss));
    // menu_putchar('╗');
    // menu_putchar('+');
    moveR('B',1);
    moveR('D',1);

    sprintf(ss,"║");
    menu_send(ss, strlen(ss));

    // menu_putchar('║');
    // menu_putchar('|');
    moveR('B',1);
    moveR('D',1);
    // menu_putchar('+');
    sprintf(ss,"╝");
    menu_send(ss,strlen(ss));
    // menu_putchar('=');

    moveR('D',MAX_INPUT+2);

    // menu_putchar('+');
    sprintf(ss,"╚");
    menu_send(ss,strlen(ss));
    // menu_putchar('=');

    sprintf(ss, "═");
    for(int i =0;i< MAX_INPUT;i++){
        // menu_putchar('-');
        menu_send(ss, strlen(ss));
        // menu_putchar('═');
    }


    // text  and save button

    menu_send(load_cursor, LEN(load_cursor));
    menu_send(show_cursor, LEN(show_cursor));
}


/*

            ╔═══════════════════════════╗
  Team Name:║asdfas                     ║
            ╚═══════════════════════════╝

*/


uint8_t editor_handler(){

    // x,y pos of the box, height and width of the box /home/jatinexe/bi0s/tui-menu/text_editor/editor.c


    // const char* save = "SAVE"; 
    // drawButton(10,100,10,save);

    // y , x , Label, Varraible
    // drawInput(10,10,"Team Name" );



    // strucutre of input box

    // label name also used as id== varialbe name cant be used twice used as id 



    // struct InputBox  team_var = {10,10,20,"Team Name:"};
    struct InputBox team_var;
    team_var.x = 10;
    team_var.y = 10;
    team_var.MAX_LEN = 20;
    strcpy(team_var.label,"Team Name:");
    

    struct InputBox *team_name  = &team_var;

    sel_ptr = team_name;

    team_name->MAX_LEN = MAX_INPUT;




    struct editorConfig E;


    // array sizes are based on macro define 
    // anoter memeber input_sized req


    initEditor(&E, team_name->y,team_name->x + strlen(team_name->label) + 1  ,1,MAX_INPUT); 
    drawInputBox(team_name);
    // drawEditor();
    /*
    init Inputbox : Team Name

    loop:
        loop
        draw input box struct
        input_handle:
            if up down and sel on buff ( ie : saved_sel is 0) draw cursor at the last text of input
            if saved_sel
                save button as color
        print the saved_buffer value
        */
    move(E.y_pos, E.x_pos); // initial cursor placement

    while (1){
        // drawInputBox(team_name); no need since we are not erasing that 
         input_handler(&E, serial_port);// o for std input
        editorRefreshScreen(&E, team_name); // refresh struct / input box
        // drawEditor();
        redraw();
    }

    
    return 0; 
}
  





/*
make sure all pointers are being freed
flickering of text
responsiveness

*/