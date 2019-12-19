#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <zconf.h>
#include "ConcurrentQueue.h"

#define INIT_BUFFER_SIZE 1000
#define MAX_COMMAND_LENGTH 18
#define OUTPUT_FILENAME "result.txt"

void *reader();

void *writeCustom();

void *calc(void *data);

// ------------ THREAD FUNCTIONS ------------
long *calcFib(long *);

long *calcPow(long *);

long *doSort(long *);

// ----------------- UTIL_FUNCTIONS -----------------
int numlen(unsigned int number) {
    int length = 0;

    do {
        length++;
        number = number / 10;
    } while (number + 1 > 1);

    return length;
}

int indexOf(char *string, char symbol) {
    char *findedSymbol = (char *) strchr(string, (int) symbol);
    if (findedSymbol == NULL)
        return -1;

    return (int) (findedSymbol - string);
}

int size(const long *arr) {
    int i = 0;

    while (arr[i] != 0) {
        i++;
    }

    return i;
}

char *readString(size_t size) {
    char *buffer = calloc(INIT_BUFFER_SIZE, size); //TODO: how to free?
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

long *convertParameters(char *string) {
    long *result = calloc(100, sizeof(long));
    int index = 0, counter = 0;

    char *argString;
    long arg;

    do {
        char delim[] = " ";

        if (counter == 0) {
            argString = strtok(string, delim);
            counter++;
        } else
            argString = strtok(NULL, delim);

        if (argString == NULL)
            break;

        arg = (long) atoi(argString);
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

TMessage *createTMessage() {
    TMessage *result = calloc(1, sizeof(TMessage));

    char *buffer = readString(sizeof(uint64_t));
    result->Type = createTMessageType(buffer);
    if (result->Type == STOP) {
        goto exit;
    }

    result->Size = strlen(buffer) * sizeof(long);
    result->Data = calloc(result->Size, sizeof(long));

    fillData(result, buffer);

    free(buffer);

    exit:
    return result;
}

long recursiveFib(long n) {
    if (n == 0 || n == 1)
        return 1;
    else
        return (recursiveFib(n - 1) + recursiveFib(n - 2));
}

void swap(long *val1, long *val2) {
    long temp = *val1;
    *val1 = *val2;
    *val2 = temp;
}

void writeMas(FILE *file, TMessage *tMessage) {
    for (int i = 0; i < size(tMessage->Data); i++)
        fprintf(file, "%hhu ", tMessage->Data[i]);
}

// ------------ END_UTIL_FUNCTIONS ---------------------

pthread_t readerTid, writerTid, calcTid;
pthread_attr_t attr;
ConcurrentQueue *queueForWriting;

int main(int argc, char *argv[]) {
    pthread_attr_init(&attr);

    queueForWriting = init_queue();

    pthread_create(&writerTid, &attr, writeCustom, NULL);

    pthread_create(&readerTid, &attr, reader, NULL);
    pthread_join(readerTid, NULL);
    pthread_join(writerTid, NULL);
}

long *calcFib(long *data) {
    long result = recursiveFib(data[0]);
    data[0] = result;
    return data;
}

long *calcPow(long *data) {
    long result = 1;
    long number = data[0];
    for (int i = 0; i < data[1]; i++) {
        result *= number;

        if (result == 0)
            result = 1;
    }

    data = (long *) malloc(strlen(data));
    *data = result;
    return data;
}

long *doSort(long *data) {
    int n = size(data);

    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - i - 1; j++)
            if (data[j] > data[j + 1])
                swap(&data[j], &data[j + 1]);
}

void *reader() {
    TMessage *tMessage;

    char *buffer = calloc(1000, sizeof(char));

    do {
        fprintf(stdin, "%s", buffer);

        tMessage = createTMessage();

        if (tMessage->Type == STOP) {
            enqueue(queueForWriting, tMessage);
            break;
        }

        pthread_create(&calcTid, &attr, calc, tMessage);
        pthread_join(calcTid, NULL); // ?
    } while (1);

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

    tMessage->Size = numlen(tMessage->Data);

    enqueue(queueForWriting, tMessage);
}

void *writeCustom() {
    FILE *file = fopen(OUTPUT_FILENAME, "w");

    TMessage *tMessage;
    while (1) {
        tMessage = dequeue(queueForWriting);

        if (tMessage != NULL) {
            switch (tMessage->Type) {
                case 0:
                    fprintf(file, "%ld", tMessage->Data[0]);
                    break;
                case 1:
                    fprintf(file, "%ld", tMessage->Data[0]);
                    break;
                case 2:
                    writeMas(file, tMessage);
                    break;
                case 3:
                    fclose(file);
                    goto exit;
            }

            fprintf(file, "\n");
        }
    }
    exit:
    fclose(file);
}