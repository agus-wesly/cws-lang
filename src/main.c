#include "vm.h"

int IS_IN_REPL = 0;

void rep()
{
    IS_IN_REPL = 1;
    char line[1024];
    memset(line, 0, 1024);
    for (;;)
    {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin))
            break;

        if (line[0] == '\n')
            break;

        // Process the input
        interpret(line);
    }
}

char *read_file(char *file_path)
{
    /*
     * 1) Find and open the file
     * 2) Read and stop while see the EOF
     * 3) Count how many bytes already read
     * 4) Store that inside a variable : `size`
     * */

    FILE *fd = fopen(file_path, "r");
    if (fd == NULL)
    {
        fprintf(stderr, "Cannot open the file\n");
        exit(60);
    }

    fseek(fd, 0L, SEEK_END);
    size_t file_size = ftell(fd);
    rewind(fd);

    char *buff = malloc(file_size + 1);
    if (buff == NULL)
    {
        fprintf(stderr, "Not enough memory to read\n");
        exit(74);
    }

    size_t bytes_read = fread(buff, sizeof(char), file_size, fd);
    if (bytes_read < file_size)
    {
        fprintf(stderr, "Failed to read the file\n");
        exit(60);
    }

    buff[bytes_read] = '\0';
    fclose(fd);
    return buff;
}

void run_file(char *file_path)
{
    char *source = read_file(file_path);
    InterpretResult result = interpret(source);
    free(source);

    if (result == INTERPRET_RUNTIME_ERROR)
        exit(65);
    if (result == INTERPRET_COMPILE_ERROR)
        exit(70);
}

int main(int argc, char **args)
{
    init_vm();

    if (argc == 1)
    {
        rep();
    }
    else if (argc == 2)
    {
        run_file(args[1]);
    }
    else
    {
        printf("Usage : jlox [file_path]\n");
        return 64;
    }

    free_vm();

    return 0;
}
