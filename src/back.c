#include <unistd.h>
#include <errno.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define KEY_LEFT      'h'
#define KEY_RIGHT     'l'
#define KEY_QUIT      'q'
#define KEY_SELECT    13  // <enter>
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
  char** array = NULL;
  char* ptr_a = strtok(str_cp, del);
  char* ptr_b = strtok(NULL, del);
  while (ptr_b != NULL) {
    (*n)++;
    array = realloc(array, *n * sizeof(char*));
    char* str_n = malloc(strlen(ptr_a) + 1);
    strcpy(str_n, ptr_a);
    array[*n - 1] = str_n;
    ptr_a = ptr_b;
    ptr_b = strtok(NULL, del);
  }
  free(str_cp);
  return array;
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


// returns new array with inserted string, passed data stays unchanged
char** insert_str_(char** array, const char* str, size_t pos, size_t size) {
  char** array_cp = malloc(sizeof(char*) * (size + 1));

  size_t step = 0;
  for (size_t i = 0; i < size + 1; ++i) {
    char* s = NULL;
    if (i != pos) {
      s = malloc(strlen(array[i - step]) + 1);
      strcpy(s, array[i - step]);
    }
    else {
      s = malloc(strlen(str) + 1);
      strcpy(s, str);
      step++;
    }
    array_cp[i] = s;
  }
  return array_cp;
}


char* conc_str_(const char* first, const char* second) {
  char* str = malloc(strlen(first) + strlen(second) + 1);
  strcpy(str, first);
  strcpy(str + strlen(first), second);
  return str;
}

char* slice_str_(const char* str, size_t a, size_t b) {
  char* str_cp = malloc(b - a + 1);
  for (size_t i = a; i < b; ++i)
    str_cp[i - a] = str[i];
  str_cp[b - a] = '\0';
  return str_cp;
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

  char* path;
  if (cursor_pos != 0) {
    char* path_r = join_str_(dirs, cursor_pos + 1, "/");
    path = slice_str_(path, 1, strlen(path));
    free(path_r); 
  }
  else {
    path = conc_str_("/", "");  // to dynamic memory
  }
  return path;
}

void free_array(char** array, size_t size) {
  for (int i = 0; i < size; ++i)
    free(array[i]);
  free(array);
}


int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("%s\n", "Filepath should be provided");
    return -1;
  }
  char* output_filepath = argv[1];

  char* cwd = get_current_dir_();
  size_t dirs_size;
  char** dirs_o = split_str_(cwd, "/", &dirs_size);
  char** dirs = insert_str_(dirs_o, "/", 0, dirs_size);
  free_array(dirs_o, dirs_size);
  dirs_size++;
  free(cwd);

  char* resulted_path = perform_(dirs, dirs_size);

  FILE* output_file = fopen(output_filepath, "w");
  if (output_file == NULL)
    printf("%s%s\n", "Unable to write in file: ", output_filepath);
  else {
    fwrite(resulted_path, 1, strlen(resulted_path), output_file);
    fclose(output_file);
  }

  free_array(dirs, dirs_size);
  free(resulted_path);

  return 0;
}
