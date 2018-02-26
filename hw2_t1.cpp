#include <sstream>
#include <string>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
using namespace std;
/*#include <stdio.h>
#include <stdlib.h>
#include <string.h>*/
//using namespace clang;
int main(int argc, char **argv){
  int c=0;
  char line[1024];
  FILE *file;
  file = fopen(argv[1],"r");
  if (file==NULL){
    printf("File does not exist");
    exit(1);
  }
  while(fgets(line, sizeof(line), file)!=NULL){
    c+=1;
    if(strstr(line, "if")!= NULL){//find if statements in that line
      if(strstr(line, "==")==NULL){ ///see if the == conditional empty
	if (strstr(line, "=")!=NULL){ //if assigment want to print error
	  if((strstr(line, "!=")==NULL)
	     | (strstr(line, ">=")==NULL)
	     | (strstr(line, "<=")==NULL )){ //other options where = can be in conditional
	    printf("line: %d \n", c);
	  }
	}
      }
    }
  }
}
  
