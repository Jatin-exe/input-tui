#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main () {
   int day, year;
   char weekday[20], month[20], dtm[100];
    int shit, x ,y;
   strcpy( dtm, "0;40;7M0;40;7m");
   sscanf( dtm, "%d%*c%d%*c%d", &shit, &x, &y );

   printf("%d %d %d\n", shit, x, y);
    
   return(0);
}
