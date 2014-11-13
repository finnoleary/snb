#define _GNU_SOURCE
#define _XOPEN_SOURCE_EXTENDED 1

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <error.h>
#include <locale.h>
#include <ncurses.h>
#include <unistd.h>
#include <wchar.h>

#include "user.h"
#include "data.h"
#include "ui.h"
#include "snb.h"

void usage(char *name) {
  fprintf(stderr, "  Usage: %s [options...] (path)\n\n", name);
#ifdef DEFAULT_FILE
  fprintf(stderr, "Default file: %s\n", DEFAULT_FILE);
#endif
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "\t-h        - print this message and exit\n");
  fprintf(stderr, "\t-v        - print version and exit\n");
  fprintf(stderr, "\t-l LOCALE - force locale\n");
#ifdef SCR_WIDTH
  fprintf(stderr, "\t-w WIDTH  - set fixed-column mode (0 - off, default: %d)\n", SCR_WIDTH);
#else
  fprintf(stderr, "\t-w WIDTH  - set fixed-column mode (0 - off, default: off)\n");
#endif
  exit(1);
}

void version() {
  wprintf(L"%S\n", INFO_STR);
  exit(1);
}

int main(int argc, char *argv[]) {
  FILE *fp;
  Result res;
  char *path, *locale;
  int opt;

  locale = "";
  while ((opt = getopt(argc, argv, "hvl:")) != -1) {
    switch (opt) {
      case 'l':
        locale = optarg;
        break;
      case 'v':
        version();
        break;
      case 'h':
      default:
        usage(argv[0]);
        break;
    }
  }

  fp = NULL;

  if (setlocale(LC_ALL, locale) == NULL) {
    fprintf(stderr, "WARN: Couldn't change LC_ALL to '%s'\n", locale);
    fprintf(stderr, "Press enter to continue.\n");
    fgetc(stdin);
  }

  if (optind < argc) {
    fp = fopen(argv[optind], "r");
    if (fp == NULL) {
      perror("Can't open your file");
      exit(1);
    }
    path = argv[optind];
  }
#ifdef DEFAULT_FILE
  else {
    fp = fopen(DEFAULT_FILE, "r");
    path = DEFAULT_FILE;
  }
#endif

  UI_File.path = NULL;
  if (fp) {
    res = data_load(fp);
    UI_File.loaded = true;
    UI_File.path = realpath(path, NULL);
    if (!UI_File.path) {
      perror("Couldn't resolve path");
      exit(3);
    }
    fclose(fp);
  } else {
    res = entry_new(1);
    UI_File.loaded = false;
  }

  if (!res.success) {
    fwprintf(stderr, L"ERROR: %S.\n", res.msg);
    perror("Unix error");
    exit(2);
  }

  ui_start();

  res = ui_set_root((Entry *)res.data);
  if (!res.success) {
    fwprintf(stderr, L"ERROR: %S.\n", res.msg);
    perror("Unix error");
    exit(3);
  }
  ui_refresh();

  ui_mainloop();
  ui_stop();

  res = ui_get_root();
  if (!res.success) {
    fwprintf(stderr, L"ERROR: %S.\n", res.msg);
    perror("Unix error");
    exit(4);
  }

  data_unload((Entry *)res.data);
  free(UI_File.path);

  return 0;
}
