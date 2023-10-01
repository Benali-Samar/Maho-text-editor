/***** Includes *****/


#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <termios.h> //The termios functions describe a general terminal interface that is provided to control asynchronous communications ports.
#include <ctype.h> // To control the printable characters
#include <errno.h>
#include <sys/ioctl.h> // for the window size



/***** Defines *****/ 


// To define the qtrl Q as a quit 
#define CTRL_KEY(k) ((k) & 0x1f)



/***** Data *****/

// For the terminal size rows 
struct editorConfig {
  //for screen size ioctl request
  int screenrows;
  int screencols;
  struct termios orig_termios;
};

struct editorConfig E;



/***** Terminal *****/

// Some error handling stuff ...
void die(const char *s)
{
  // clear the screen when exit
  write (STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  // if anything went wrong error and run (exit).
  perror (s);
  exit(1);
}


// Disable the raw mode when finishing writing else it will not exit the section
void disableRawMode(){
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetarr");
}


// enabling raw mode that can display characters directly while writing, oppositely to canonical mode that waits for a specified  signal to print all typed characters. 
// Canonical mode is the default so we should enable the raw mode instead.
void enableRawMode()
{
  // read the current attributes into a struct
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
    die ("tcsetattr");

  atexit(disableRawMode) ; // after read disable and comback to canonical mode for signals use
  

  struct termios raw = E.orig_termios;
  raw.c_iflag &= ~(ICRNL | IXON | BRKINT | INPCK | ISTRIP);  // disable CTRL+S and CTRL+Q and miscellaneous flags
  raw.c_oflag &= ~(OPOST); // disable all output processing like "\n"
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); // modifying the struct by hand 

 // The c_cc stands for "control characters" an array of bytes to control various terminal settings. 
  // Set the min number of bytes of input needed before read () can return.
  raw.c_cc[VMIN] = 0; 
  //  Set the max amount of time to wait before read() returns.
  raw.c_cc [VTIME] = 1;
  
  // To write the new terminal attriobutes back out
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die ("tcsetattr");

}



//Reading keypress in lowleveel
char editorReadKey()
{
  int nread;
  char c;
  while((nread = read(STDIN_FILENO, &c, 1)) != 1)
  {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }
  return c;
}

//Get window size with IOCTL built in request: TIOCGWINSZ 
// Stands for Input/Output Control Get WINdow size

int getWindowSize(int *rows, int *cols)
{
  struct winsize ws;
  if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)== -1 || ws.ws_col == 0)
  {
    return -1;
  }else{
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/***** Inputs *****/ 


//Mapping the keypress to editor operations
void editorProcessKeypress(char c)
{
  //char c = editorReadKey();

  switch(c) 
  {
    case CTRL_KEY('q'):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H" , 3);
      exit(0);
      break;
  }
}



/***** Outputs *****/ 


//Adding tilde like VIM did ^^ 
void editorDrawRows()
{
  int i;
  for (i = 0; i < E.screenrows; i++)
  {
    write(STDOUT_FILENO, "~\r\n", 3);
    
    //Last line
    if(i <E.screenrows -1)
      write(STDOUT_FILENO, "\r\n", 2);
  }
}


//clearing the screen
void editorRefreshscreen()
{
  // The \x1b is the escape character
  // The J command is to clear the screen, the 2 is to say clear the entire screen
  write(STDOUT_FILENO, "\x1b[2J", 4);
  // The H command for the cursor position
  write (STDOUT_FILENO, "\x1b[H", 3);
  editorDrawRows();

  write (STDOUT_FILENO, "\x1b[H", 3);
}




/***** Init *****/


void initEditor()
{
  if(getWindowSize(&E.screenrows, &E.screencols) == -1)
    die("getWindowSize");
}

int main ()
{
  // All the stuff did upside
  enableRawMode();
  initEditor();
  //Reading chars 
  while(1) {
  	char c = editorReadKey();
    // does c a control character ?
        if (iscntrl(c)) { 
            // print it 
            printf("%d\n", c);
        } else {
            // else to go back in a new line write the whole "\r\n"
            printf("%d ('%c')\r\n", c, c);
        }

    //Process the characters	  
    editorProcessKeypress(c);
    //Flush the screen
    editorRefreshscreen(); 
  }
  return 0;
}
