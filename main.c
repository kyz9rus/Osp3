#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <zconf.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ConcurrentQueue.h"

#define INIT_BUFFER_SIZE 1000
#define MAX_COMMAND_LENGTH 18
#define OUTPUT_FILENAME "result.txt"

ConcurrentQueue *queueForWriting;

void *
reader(void *data); //TODO: переделать, считывать как в примере к лабе - по байтам сначала тайп, потом дату, и т.д. Записывать также

void *writeCustom(void *data);

void *calc(void *data);

// ------------ MAIN FUNCTIONS ------------
uint8_t *calcFib(uint8_t *);

uint8_t *calcPow(uint8_t *);

uint8_t *doSort(uint8_t *);

// ----------------- UTIL_FUNCTIONS -----------------
int indexOf(char *string, char symbol) {
    char *findedSymbol = (char *) strchr(string, (int) symbol);
    if (findedSymbol == NULL)
        return -1;

    return (int) (findedSymbol - string);
}

char *readString(FILE *file, size_t size) {
    char *buffer = calloc(INIT_BUFFER_SIZE, size); //TODO: how to free?
//    fscanf(file, "%[^\n]\n", buffer);
//    char temp;
//    scanf("%c",&temp);
//    scanf("%[^\n]", buffer);

    fgets(buffer, 200, stdin);
    buffer[strlen(buffer) - 1] = '\0';

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
    char *typeString = calloc(MAX_COMMAND_LENGTH, sizeof(char));
    int indexOfSpace = indexOf(message, ' ');

    if (indexOfSpace != -1) {
        strncpy(typeString, message, indexOfSpace);
        EType eType = stringToEType(typeString);
        free(typeString);
        return eType;
    } else {
        return stringToEType(message);
    }
}

uint8_t *convertParameters(char *string) {
    uint8_t *result = calloc(100, sizeof(uint8_t));
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

void fillData(TMessage *tMessage, char *command) {
    int indexOfSpace = indexOf(command, ' ');
    char *buffer = calloc(INIT_BUFFER_SIZE, sizeof(char));
    strncpy(buffer, command + indexOfSpace + 1, strlen(command));

    tMessage->Data = convertParameters(buffer);

    free(buffer);
}

TMessage *createTMessage(FILE *file) {
    TMessage *result = calloc(1, sizeof(TMessage));

    char *buffer = readString(file, sizeof(uint64_t));
    result->Type = createTMessageType(buffer);
    if (result->Type == STOP) {
        goto exit;
    }

    result->Size = strlen(buffer) * sizeof(uint8_t);
    result->Data = calloc(result->Size, sizeof(uint8_t));

    fillData(result, buffer);

    free(buffer);

    exit:
    return result;
}
// ------------ END_UTIL_FUNCTIONS ---------------------


pthread_t readerTid, writerTid, calcTid;
pthread_attr_t attr;

int main(int argc, char *argv[]) {
    FILE *file = fopen(OUTPUT_FILENAME, "w"); // TODO: check for reuding
//    fclose(file);

    pthread_attr_init(&attr);

    queueForWriting = init_queue();

    pthread_create(&writerTid, &attr, writeCustom, file);

    pthread_create(&readerTid, &attr, reader, NULL);
    pthread_join(readerTid, NULL);
    pthread_join(writerTid, NULL);

    fclose(file);
}

uint8_t recursiveFib(uint8_t n) {
    if (n == 0 || n == 1)
        return 1;
    else
        return (recursiveFib(n - 1) + recursiveFib(n - 2));
}

uint8_t *calcFib(uint8_t *data) {
    int n = data[0];
    data = calloc(255, sizeof(uint8_t)); //TODO: fix!

    for (int i = 0; i < n; i++)
        data[i] = recursiveFib(i);

    return data;
}

uint8_t *calcPow(uint8_t *data) {
//    uint8_t result = (int) ((double)pow((double)data[0], (double)data[1]) + 0.5);
    uint8_t result = 1;
    uint8_t number = data[0];
    for (int i = 0; i < data[1]; i++) {
        result *= number;

        if (result == 0)
            result = 1;
    }

    data = (uint8_t *) malloc(strlen(data));
    *data = result;
    return data;
}

void swap(uint8_t *val1, uint8_t *val2) {
    uint8_t temp = *val1;
    *val1 = *val2;
    *val2 = temp;
}

uint8_t *doSort(uint8_t *data) {
    int n = strlen(data);

    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - i - 1; j++)
            if (data[j] > data[j + 1])
                swap(&data[j], &data[j + 1]);
}

void *reader(void *data) {
    TMessage *tMessage;
    char *fileName = "commands.txt";
    FILE *file = fopen(fileName, "r");

    char *buffer = calloc(1000, sizeof(char));

    do {
        fprintf(stdin, "%s", buffer);

        tMessage = createTMessage(file);
        enqueue(queueForWriting, tMessage);

        if (tMessage->Type == STOP)
            break;

        pthread_create(&calcTid, &attr, calc, tMessage);
        pthread_join(calcTid, NULL); // ?
    } while (1);

    fclose(file);
}

void *calc(void *data) {
    TMessage *tMessage = (TMessage *) data;

    switch (tMessage->Type) {
        case 0:
            tMessage->Data = calcFib(tMessage->Data);
            break;
        case 1:
            tMessage->Data = calcPow(tMessage->Data);
            break;
        case 2:;
            doSort(tMessage->Data);
            break;
    }

    tMessage->Size = strlen(tMessage->Data);

    pthread_exit(writerTid);
}

void *writeCustom(void *data) {
    FILE *file = (FILE *) data;

    TMessage *tMessage;
    while(1) {
        tMessage = dequeue(queueForWriting);

        if (tMessage != NULL) {
            if (tMessage->Type == 3) {
                fclose(file);
                break;
            }

            for (int i = 0; i < tMessage->Size; i++)
                fprintf(file, "%hhu ", tMessage->Data[i]);

            fprintf(file, "\n");
        }
    }
}