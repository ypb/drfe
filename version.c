
#include <stdio.h>
#include "version.h"

void version() {
  printf("v%u.%u.%u-%s", MAJOR, MINOR, PATCH, GITSHA);
}

void drfe_banner() {
  printf("; DrFe "); version(); printf(" GNU GPL3 (c)(r) 2011 ypb\n");
  printf("%% Proście, a będzie wam dane;\n %% szukajcie, a znajdziecie;\n%% kołaczcie, a otworzą wam.\n");
}

void drfe_help() {
  printf("  Usage: drfe [say|ask] ...\n");
}

