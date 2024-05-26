//
// Console input and output, to the uart.
// Reads are line at a time.
// Implements special input characters:
//   newline -- end of line
//   control-h -- backspace
//   control-u -- kill line
//   control-d -- end of file
//   control-p -- print process list
//

//int history(int);


#include "types.h"
#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"
#include "top.h"

#define MAX_HISTORY 16
#define BACKSPACE 0x100
//#define PGUP "\\E[5;2~"
//#define PGDN "\\E[6;2~"
#define C(x)  ((x)-'@')  // Control-x

historyBufferArray sharedStruct;
historyBufferArray* sharedStructPtr = &sharedStruct;

// send one character to the uart.
// called by printf(), and to echo input characters,
// but not from write().
//
int keepRunning = 0;
//int ctrl_c_pressed = 0;
// count for up key
int up_count = -1;


void
consputc(int c)
{
  if(c == BACKSPACE){
    // if the user typed backspace, overwrite with a space.
    uartputc_sync('\b'); uartputc_sync(' '); uartputc_sync('\b');
  } else {
    uartputc_sync(c);
  }
}

struct {
  struct spinlock lock;
  
  // input
#define INPUT_BUF_SIZE 128
  char buf[INPUT_BUF_SIZE];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
} cons;

//
// user write()s to the console go here.
//
int
consolewrite(int user_src, uint64 src, int n)
{
  int i;

  for(i = 0; i < n; i++){
    char c;
    if(either_copyin(&c, user_src, src+i, 1) == -1)
      break;
    uartputc(c);
  }

  return i;
}

//
// user read()s from the console go here.
// copy (up to) a whole input line to dst.
// user_dist indicates whether dst is a user
// or kernel address.
//
int
consoleread(int user_dst, uint64 dst, int n)
{
  uint target;
  int c;
  char cbuf;

  target = n;
  acquire(&cons.lock);
  while(n > 0){
    // wait until interrupt handler has put some
    // input into cons.buffer.
    while(cons.r == cons.w){
      if(killed(myproc())){
        release(&cons.lock);
        return -1;
      }
      sleep(&cons.r, &cons.lock);
    }

    c = cons.buf[cons.r++ % INPUT_BUF_SIZE];

    if(c == C('D')){  // end-of-file
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        cons.r--;
      }
      break;
    }

    // copy the input byte to the user-space buffer.
    cbuf = c;
    if(either_copyout(user_dst, dst, &cbuf, 1) == -1)
      break;

    dst++;
    --n;

    if(c == '\n'){
      // a whole line has arrived, return to
      // the user-level read().
      break;
    }
  }
  release(&cons.lock);

  return target - n;
}


////implement of history system call
//struct {
//    char bufferArr[MAX_HISTORY][INPUT_BUF_SIZE];
//    uint lengthsArr[MAX_HISTORY];
//    uint lastCommandIndex;
//    int numOfCommandsInMem;
//    int currentHistory;
//} historyBufferArray;

void insertAtBeginning(int size, char* newString) {
    // Shift all existing strings to the right
    for (int i = size; i > 0; i--) {
        safestrcpy(sharedStruct.bufferArr[i], sharedStruct.bufferArr[i - 1], 128);
    }
    safestrcpy(sharedStruct.bufferArr[0], newString, 128);
    sharedStruct.numOfCommandsInMem++;
}

char* firstWord(char* str) {
    char* start = str; // Start of the string
    while (*str != ' '&& *str != '\n' && *str != '\0') {
        str++; // Move to the next character
    }
    *str = '\0'; // Replace the space or null terminator with a null terminator to end the first word
    return start; // Return the start of the first word
}

int check_is_valid_command(char *commandRequested){
    char commands_array[19][20] = {"README", "cat", "echo", "forktest", "grep", "init", "kill", "ln","ls", "mkdir", "rm", "sh",
                                   "stressfs", "usertests", "grind", "wc","zombie", "console", "top"};
    for (int i = 0; i < 19; ++i) {
        if(strncmp(commandRequested, commands_array[i], 19) == 0){
            return 0;
        }
    }
    return 1;
}

struct proc *p;

//
// the console input interrupt handler.
// uartintr() calls this for input character.
// do erase/kill processing, append to cons.buf,
// wake up consoleread() if a whole line has arrived.
//
void
consoleintr(int c)
{
  acquire(&cons.lock);
  if(c == '\x1B'){
      int c1 = uartgetc();
      int c2 = uartgetc();
      if(c1 == '[' && c2 == 'A'){
          if(up_count == sharedStruct.numOfCommandsInMem - 1){
              up_count = -1;
          }
          up_count++;
          while(cons.e != cons.w &&
                cons.buf[(cons.e-1) % INPUT_BUF_SIZE] != '\n'){
              cons.e--;
              consputc(BACKSPACE);
          }
          int x = 0;
          while(sharedStruct.bufferArr[up_count][x] != '\0' && sharedStruct.bufferArr[up_count][x] != '\n'){
              consputc(sharedStruct.bufferArr[up_count][x]);
              cons.buf[cons.e++ % INPUT_BUF_SIZE] = sharedStruct.bufferArr[up_count][x];
              x++;
          }
//          printf("%s", sharedStruct.bufferArr[up_count]);
      }
      else if(c1 == '[' && c2 == 'B'){
          if(up_count != -1){
              up_count--;
              while(cons.e != cons.w &&
                    cons.buf[(cons.e-1) % INPUT_BUF_SIZE] != '\n'){
                  cons.e--;
                  consputc(BACKSPACE);
              }
              int x = 0;
              while(sharedStruct.bufferArr[up_count][x] != '\0' && sharedStruct.bufferArr[up_count][x] != '\n'){
                  consputc(sharedStruct.bufferArr[up_count][x]);
                  cons.buf[cons.e++ % INPUT_BUF_SIZE] = sharedStruct.bufferArr[up_count][x];
                  x++;
              }
          }
      }
  }
  else{
      switch(c){
          case C('C'):  // Control-C
//              ctrl_c_pressed = 1;
              keepRunning = 1;
              // send a signal to the current process to stop
//              printf("\nhello\n");
//              p = myproc();
//              printf("\nhello1\n");
//              p->killed = 1;
//              printf("\nhello2\n");
//              yield();  // force a context switch
              break;
          case C('P'):  // Print process list.
              procdump();
              break;
          case C('U'):  // Kill line.
              while(cons.e != cons.w &&
                    cons.buf[(cons.e-1) % INPUT_BUF_SIZE] != '\n'){
                  cons.e--;
                  consputc(BACKSPACE);
              }
              break;
//  case "\E[5;2~":  // Kill line.
//      while(cons.e != cons.w &&
//            cons.buf[(cons.e-1) % INPUT_BUF_SIZE] != '\n'){
//          cons.e--;
//          consputc(BACKSPACE);
//      }
//          printf("\nhello Mahdi");
//      break;
          case C('H'): // Backspace
          case '\x7f': // Delete key
              if(cons.e != cons.w){
                  cons.e--;
                  consputc(BACKSPACE);
              }
              break;
          default:
              if(c != 0 && cons.e-cons.r < INPUT_BUF_SIZE){
                  c = (c == '\r') ? '\n' : c;

                  // echo back to the user.
                  consputc(c);

                  // store for consumption by consoleread().
                  cons.buf[cons.e++ % INPUT_BUF_SIZE] = c;

                  if(c == '\n' || c == C('D') || cons.e-cons.r == INPUT_BUF_SIZE){
                      char* cpSentence = "";
                      safestrcpy(cpSentence, &cons.buf[cons.r],128);
                      char* first_word = firstWord(cpSentence);
                      if(check_is_valid_command(first_word) == 0){
                          if(sharedStruct.numOfCommandsInMem == MAX_HISTORY){
                              if(strncmp(&cons.buf[cons.r], sharedStruct.bufferArr[0], 19) != 0){
                                  insertAtBeginning(MAX_HISTORY - 1, &cons.buf[cons.r]);
                              }
                          }
                          else{
                              if(strncmp(&cons.buf[cons.r], sharedStruct.bufferArr[0], 19) != 0){
                                  insertAtBeginning(sharedStruct.numOfCommandsInMem, &cons.buf[cons.r]);
                              }

                          }
                      }
                      else if(check_is_valid_command(first_word) == 1){
//              printf("\nInvalid command brooooooooooooooo!\n");
                      }
                      // wake up consoleread() if a whole line (or end-of-file)
                      // has arrived.
                      up_count = -1;
                      cons.w = cons.e;
                      wakeup(&cons.r);
                  }
              }
              break;
      }

  }

  release(&cons.lock);
}

void
consoleinit(void)
{
  initlock(&cons.lock, "cons");

  uartinit();

  // connect read and write system calls
  // to consoleread and consolewrite.
  devsw[CONSOLE].read = consoleread;
  devsw[CONSOLE].write = consolewrite;
}
