#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024

int count_oc_file(FILE *file, const char *search_string) {
    char line[MAX_LINE_LENGTH];
    int count = 0;

    while (fgets(line, sizeof(line), file)) {
        char *pos = line;
        while ((pos = strstr(pos, search_string)) != NULL) {
            count++;
            pos += strlen(search_string);
        }
    }

    return count;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <file> <string>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];
    const char *search_string = argv[2];

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    int count = count_occurrences(file, search_string);
    printf("The string '%s' occurs %d times in the file '%s'.\n", search_string, count, filename);

    fclose(file);
    return EXIT_SUCCESS;
}


