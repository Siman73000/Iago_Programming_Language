#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "parser.h"


int main(char *argv[], int argc) {
    char *argv[] = "42";
    int value;
    sscanf(*argv, "%c", &value);
    printf("Parsed value: %d\n", value);
    return 0;
}