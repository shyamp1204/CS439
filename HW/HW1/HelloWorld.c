#include <stdio.h>

int main(int argc, char *argv[]) {
  int numOfLoops;
  if(argc == 2) {
    numOfLoops = atoi(argv[1]);  //ascii to int function call
    recursiveHelper(numOfLoops);
    if(numOfLoops == numOfLoops) {
      printf("Goodbye\n");
    }
  }
  else  {
    printf("incorrect arguments, please try again\n");
  }
}

int recursiveHelper(int loops) {
  if(loops <= 0) 
    return 0;
  else {
    printf("Hello, World!\n");
    return recursiveHelper(loops -1);
  }
}
