#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define INIT_BUFFER_SIZE 1000

typedef enum EType {
    FIBONACCI,
    POW,
    BUBBLE_SORT_UINT64,
    STOP
} EType;

struct TMessage {
    uint8_t Type;
    uint64_t Size;
    uint8_t *Data;
};

int count;

void *reader(void *data);

void *writer(void *data);

// ------------ UTIL_FUNCTIONS ---------------------
int indexOf(char *string, char symbol) {
    char *findedSymbol = (char *) strchr(string, (int) symbol);
    if (findedSymbol == NULL)
        return -1;

    return (int) (findedSymbol - string);
}

char * readString(FILE *file, size_t size) {
//    fgets(string, 100, stdin);
    char *buffer = calloc(INIT_BUFFER_SIZE, size);
//    fscanf(file, "%[^\n]%*c", buffer);
    fscanf(file, "%[^\n]\n", buffer);
//    if (indexOf(buffer, ' ') != -1) // TODO: maybe custom?
//        buffer[strlen(buffer) - 1] = '\0';

    return buffer;
}

EType stringToEType(char *string) {
    if (strcmp(string, "FIBONACCI") == 0)
        return FIBONACCI;
    else if (strcmp(string, "POW") == 0)
        return POW;
    else if (strcmp(string, "BUBBLE_SORT_UINT64") == 0)
        return BUBBLE_SORT_UINT64;
    else if (strcmp(string, "STOP") == 0)
        return STOP;
}

EType createTMessageType(char *message) {
    char *typeString = calloc(18, sizeof(char));
    int indexOfSpace = indexOf(message, ' ');

    if (indexOfSpace != -1) {
        strncpy(typeString, message, indexOfSpace);
        return stringToEType(typeString);
    } else {
        return stringToEType(message);
    }
}

uint8_t *convertParameters(char *string) {
    uint8_t *result = calloc(100, sizeof(char));
    int index = 0, counter = 0;

    char *argString;
    uint8_t arg;

    do {
        char delim[] = " ";

        if (counter == 0) {
            argString = strtok(string, delim);
            counter++;
        } else
            argString = strtok(NULL, delim);

        if (argString == NULL)
            break;

        arg = (uint8_t) atoi(argString);
        if (arg == '\0') {
            errno = EINVAL;
            printf("Incorrect parameter: %s\n", argString);
            exit(errno);
        }

        result[index] = arg;
        index++;
    } while (1);

    return result;
}

void fillData(struct TMessage *tMessage, char *command) {
    int indexOfSpace = indexOf(command, ' ');
    char *typeString = calloc(INIT_BUFFER_SIZE, sizeof(char));
    strncpy(typeString, command + indexOfSpace + 1, strlen(command));

    tMessage->Data = convertParameters(typeString);
}

struct TMessage createTMessage(FILE *file) {
    struct TMessage result;

    char *buffer = readString(file, sizeof(uint64_t));
    result.Type = createTMessageType(buffer);

    result.Size = strlen(buffer) * sizeof(uint8_t);
    result.Data = calloc(result.Size, sizeof(uint8_t));

    fillData(&result, buffer);

    // todo: switch case by ETYPE (int)

    free(buffer);

    return result;
}
// ------------ END_UTIL_FUNCTIONS ---------------------


pthread_t readerTid, writerTid, expThreadTid, fibThreadTid, sortThreadTid;
pthread_attr_t attr;

int main(int argc, char *argv[]) {
    pthread_attr_init(&attr);

    pthread_create(&readerTid, &attr, reader, NULL);
    pthread_join(readerTid, NULL);
}

void *expThread(void *data) {
    printf("Expontiate ...");

}

void *fibThread(void *data) {
    printf("Calculating fibonacci  ...");

}

void *sortThread(void *data) {
    printf("Sorting ...");
}

void *reader(void *data) {
    struct TMessage tMessage;
    char * fileName = "commands.txt";
    FILE *file = fopen (fileName, "r");

    if (file == NULL) {
        printf("open failed on %s", fileName);
        exit(1);
    }

    do {
        tMessage = createTMessage(file);

        if (tMessage.Type == STOP)
            break;

        pthread_create(&writerTid, &attr, writer, &tMessage);
        pthread_join(writerTid, NULL);
    } while (1);

    fclose(file);
}

void *writer(void *data) {
    pthread_join(readerTid, NULL);

    char *fileName = calloc(15, sizeof(char));

    strcat(fileName, "file-");
    char buffer[10];
    sprintf(buffer, "%d", count);
    strcat(fileName, buffer);
    strcat(fileName, ".txt");
    FILE *file = fopen(fileName, "w");

    fprintf(file, (const char *) data);

    fclose(file);
}