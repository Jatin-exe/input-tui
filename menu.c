#include "menu.h"
#include "menu-config.h"
#include "ansi.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <unistd.h>  // write(), read(), close()


#include <math.h>



#include <string.h>

static void redraw();
static void clear();
static void put_int(uint_fast16_t);
// static void (*menu_send)(uint8_t *buf, uint16_t len) = cdc_send;
static void (*menu_send)(uint8_t *buf, uint16_t len)  = (void (*)(uint8_t *, uint16_t))cdc_send;
static void (*menu_recv)(uint8_t *buf, uint16_t len) = (void (*)(uint8_t *, uint16_t))cdc_recv;
static int (*menu_putchar)(uint8_t) = (int (*)(uint8_t))cdc_putchar;
static int (*menu_getchar)(uint8_t *) = (int (*)(uint8_t *))cdc_getchar;

// static void (*menu_recv)(uint8_t *buf, uint16_t len) = cdc_recv;
// static int (*menu_putchar)(uint8_t) = cdc_putchar;
// static int (*menu_getchar)(uint8_t *) = cdc_getchar;

static int selection = 0;
static uint8_t term_width = 100;
static uint8_t term_height = 100;


static uint8_t menu_width = 30;
static uint8_t desc_width = 90;
// \033[1B GO DOWN ONE LINE E FOR RETURN 

static uint8_t first_line[] = {"\033[1;1H"};



static uint8_t seq_clear[8] = {"\033[2J\033[3J"}; //\033[K 
// static uint8_t seq_clear[8] = {"\033[H"};
static uint8_t seq_clear_2[4] = {"\033[H"};
static uint8_t seq_bgcolor[7] = BGPURPLE;
static uint8_t seq_reset[4] = {"\033[0m"};
static uint8_t seq_fgcolor[7] = FGWHITE;
void clear()
{
  menu_send(seq_clear, LEN(seq_clear));
  menu_send(seq_clear_2, LEN(seq_clear_2));
}


void reset() { menu_send(seq_reset, LEN(seq_reset)); }
void setbg() { menu_send(seq_bgcolor, LEN(seq_bgcolor)); }

void move(uint_fast16_t y, uint_fast16_t x)
{
  // move function will work only if put_int() is completed
  // you can move the cursor position with this fn before printing a specific text or while setting bg
  menu_putchar(0x1b);
  menu_putchar('[');
  put_int(y);
  menu_putchar(';');
  put_int(x);
  menu_putchar('H');
}
void put_int(uint_fast16_t i)
{
  // WORKS ONLY FOR THREE DIGIT NUMBER 
  // send each digit of a 3 digit number as characters
  // ...

  char no[3] = {};


  int x;

  int itr = 2;
  while(i!=0){
    x = i%10;
    i /= 10;
    no[itr] = (char)x + '0';
    itr--;
  }
  // printf("asdf");
  // printf("one%c", no[0]);
  // printf("two%c", no[1]);
  // printf("three%c", no[2]);
  
  menu_putchar(no[0]);
  menu_putchar(no[1]);
  menu_putchar(no[2]);
}
void drawline(float mv)
{
  // Draw a vertical line seperating the list of items and its description
  // ...
  menu_send(first_line, LEN(first_line));
  menu_send("\033[30C", 5);

  for (int i = 0; i < mv; i++)
  {
    menu_send("|\033[1D\033[1B", 9);
  }
}



// void printDescription(const uint8_t* desc) {
//     size_t LEN(desc) = strlen((char*)desc);
// }
void drawlabdetials(uint8_t s)
{
  // code for printing description of the item selected

  menu_send(first_line, LEN(first_line));
  menu_send("\033[33C", 5);

  Menuitem *m = &menu[s];
 desc_width = term_width - menu_width;

  uint8_t line[desc_width+1]; // increase this size err if size > 90 

  // maybe this +1 is causing an issue? , null characetr o something ig is causing the issue


// locla version control system wehn developing ans bug esting adn testing sutff


uint8_t desc[strlen(m->desc)];

  strncpy(desc, m->desc, strlen(m->desc));

// printf("len: %d", strlen(m->desc));

    for (size_t i = 0; i < LEN(desc); i += desc_width) {
        // Calculate the actual width for the current line
        size_t lineLength = (LEN(desc) - i) > desc_width ? desc_width : (LEN(desc) - i);

        // Find the last space within the line width
        size_t lastSpaceIndex = lineLength;
        for (size_t j = lineLength - 1; j > 0; j--) {
            if (desc[i + j] == ' ') {
                lastSpaceIndex = j;
                break;
            }
        }

        // Determine the final line width by considering the last space
        size_t finalLineLength = (lastSpaceIndex < lineLength) ? lastSpaceIndex : lineLength;

        // Print the current line
        char line[desc_width + 1];
        strncpy(line, (char*)&desc[i], finalLineLength);
        line[finalLineLength] = '\0';
        menu_send(line, finalLineLength);
        menu_send("\033[1E\033[33C", 9);
    }

}
void drawfooter()
{

  uint8_t last_line[] = {"\033[999;990H"};
  snprintf(last_line,sizeof(last_line),"\033[999;%dH", term_width-LEN(tag_line)+2);

  menu_send(last_line, LEN(last_line));
  menu_send(tag_line, LEN(tag_line));
  // Display tagline = "bi0s hardware"  at the bottom right of the screen
  // ...
}
void drawlab()
{
  Menuitem *m;
  for (uint_fast16_t i = 0; i < LEN(menu); i++)
  {
    m = &menu[i];
    // menu_send("\n\r",2); //remove this line of code use move() to go to next line
    if (i == selection)
    {
      //move(i, 10);
      // do this before setting bg and printing lab
      //  clear(); this clear fucks things up 

      // to avoiding jumping of text down while pressing arrow keys
      setbg();
    }


    uint8_t title[menu_width]; 
    snprintf(title, sizeof(title), "%-*s", sizeof(title), m->title);

    menu_send(title, LEN(title));
    menu_send("\r\n", 2);
    reset();
  }
}

uint_fast8_t input_handler()
{
  // basically used to detect keypress
  // Changes the selection based on arrow key input


  
  uint8_t c;
  while (1)
  {

    menu_getchar(&c);

    if (c == '\033')
    {


      menu_getchar(&c);
      if (c == ';')
        printf("another one\n");
      if (c == '[')
      {

        menu_getchar(&c);
        if (c == 'A')
        {
          selection = (selection - 1);
          if (selection == -1)
            selection = LEN(menu) - 1;
          return 1;
        }
        else if (c == 'B')
        {
          selection = (selection + 1) % (LEN(menu) - 0);
          return 1;
        }
      }
    }
  }
  return 0;
}
void redraw()
{
  clear();
  drawlab();                 // Lab list
  drawline(LEN(menu));       // Layout
  drawlabdetials(selection); // Lab Description
  drawfooter();              // Footer
}
//////////////////////////////////////////////////
// THIS IS THE FIRST FUNCTION EXECUTED IN THIS FILE
//////////////////////////////////////////////////
uint8_t menu_handler()
{

  // redraw();

  while (1)
  {
        // redraw();

        uint8_t input[100];
          menu_send("\033[18t", 5);  // Request terminal size

         menu_recv(input, 20);

          if(input[2] == '8'){

  // // flush ? 


 
            int i = 0;
            int w = 0;
            int h = 0;

            while(input[i] != ';'){
              i++;

              
            }
              
            

            i++;

            while(input[i] != ';'){
              // get w
              h = h * 10 + input[i] - '0';
              i++;
            }

            i++;
            while (input[i] != 't'){
              // get h
              w = w *10 + input[i] - '0';
              i++;
            }
            printf("Width : %d", w);
            printf("Heigth : %d", h);
            fflush(stdout);
            term_height = h;
            term_width = w;



          }
    if (input_handler())
    {
      // clear();
      printf("%d\n", selection);
      redraw();
        

    }// if 
  }
  return 0;

  
}
  

