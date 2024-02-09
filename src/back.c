#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define KEY_LEFT      'h'
#define KEY_RIGHT     'l'
#define KEY_QUIT      'q'
#define KEY_SELECT    13
#define SELECTED_FG   14
#define WIDTH         20
#define DELIMITER     " "


extern int errno;


char* get_current_dir_() {
  size_t buffer_size = 0;
  char* buffer = NULL;
  do {
    errno = 0;
    if (buffer != NULL) free(buffer);
    buffer_size += 64;
    buffer = calloc(buffer_size, buffer_size);
    getcwd(buffer, buffer_size);
  } while (errno == ERANGE);
  return buffer;
}



char** split_str_(const char* str, const char* del, size_t* n) {
  char* str_cp = malloc(strlen(str) + 1);
  strcpy(str_cp, str);
  
  *n = 0;
  char** result = NULL;
  char* ptr = strtok(str_cp, del);
  while (ptr != NULL) {
    (*n)++;
    result = realloc(result, *n * sizeof(char*));
    result[*n - 1] = ptr;
    ptr = strtok(NULL, del);
  }
  return result;
}


char* join_str_(char** array, size_t size, const char* del) {
  size_t del_size = strlen(del);

  size_t str_size = strlen(del) * size;
  for (int i = 0; i < size; ++i) 
    str_size += strlen(array[i]);

  char* str = malloc(str_size + 1);
  char* ptr = str;
  for (int i = 0; i < size; ++i) {
    if (i != 0) {
      strcpy(ptr, del);
      ptr += del_size;
    }
    strcpy(ptr, array[i]);
    ptr += strlen(array[i]);
  }
  return str;
}


char* conc_str_(const char* first, const char* second) {
  char* str = malloc(strlen(first) + strlen(second) + 1);
  strcpy(str, first);
  strcpy(str + strlen(first), second);
  return str;
}


enum State {
  READY_STATE,
  CONFIRMED_STATE,
  QUITED_STATE
};


void handle_input(size_t* cursor_pos, size_t folders_num, enum State* state) {
  char input_key = getc(stdin);
  switch (input_key) {
    case 75:  // left arrow key
    case KEY_LEFT:
      if (*cursor_pos > 0) (*cursor_pos)--;
      break;
    case 77:  // right arrow key
    case KEY_RIGHT:
      if (*cursor_pos < folders_num - 1) (*cursor_pos)++;
      break;
    case KEY_SELECT:
      *state = CONFIRMED_STATE;
      break;
    case KEY_QUIT:
      *state = QUITED_STATE;
      break;
  }
}


int display(char** dirs, size_t size, size_t cursor_pos) {
  printf("%s", DELIMITER);
  for (size_t i = 0; i < size; ++i) {
    char* dir = dirs[i];
    if (dir == NULL) continue;
    if (i == cursor_pos)
      printf("\e[38;5;%dm%.*s\e[0m", SELECTED_FG, WIDTH, dir);
    else
      printf("%.*s\e[0m", WIDTH, dir);
    printf("%s", DELIMITER);
    //printf("\e[38;5;%dm\e[48;5;%dm%.*s\e[0m%s", fg, bg, WIDTH, dir, DELIMITER);
  }
}


char* perform_(char** dirs, size_t n) {
  system("/bin/stty raw"); // changing output behaviour. idk what it actually does

  enum State state = READY_STATE;

  size_t cursor_pos = n - 1;
  do {
    display(dirs, n, cursor_pos);
    handle_input(&cursor_pos, n, &state);
    printf("\r\e[2K");  
  } while (state == READY_STATE);

  system("/bin/stty cooked"); 

  if (state == QUITED_STATE)
    cursor_pos = n - 1;

  char* path = join_str_(dirs, cursor_pos + 1, "/");
  char* path_f = conc_str_("/", path);     
  free(path);
  return path_f;
}


int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("%s\n", "Filepath should be provided");
    return -1;
  }
  char* output_filepath = argv[1];

  char* cwd = get_current_dir_();
  size_t dirs_size;
  char** dirs = split_str_(cwd, "/", &dirs_size);
  free(cwd);

  char* resulted_path = perform_(dirs, dirs_size);

  FILE* output_file = fopen(output_filepath, "w");
  if (output_file == NULL)
    printf("%s%s\n", "Unable to write in file: ", output_filepath);
  else {
    fwrite(resulted_path, 1, strlen(resulted_path), output_file);
    fclose(output_file);
  }

  free(dirs);
  free(resulted_path);

  return 0;
}
