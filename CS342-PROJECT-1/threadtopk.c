#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>


struct block {
    char word[64];
    int frequency;
};

struct threadBlock {
    int k;
    char *inFileName;
    struct block *head;
};


void bubbleSort(struct block *head, int size);
void updateFrequency(struct block **head_ref, char *key, int *size);
void toUpperCase(struct block *head, int size);
void *processFile(struct threadBlock *argument);
void checkFrequency(struct block *head, int size);
void checkWord(struct block *head, int size);


void *processFile(struct threadBlock *argument) {
    FILE *fr;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    struct block *temp = NULL;
    int size = 0;

    fr = fopen( argument->inFileName, "r");
    if (fr == NULL) {
        exit(EXIT_FAILURE);
    }
    while ((read = getline(&line, &len, fr)) != -1) {
        char *cur = line, *next = line;
        while (cur != NULL) {
            strsep(&next, " ");
            if (cur[strlen(cur)-1] == '\n') {
            	cur = strtok(cur, "\n");
            }
            updateFrequency(&temp, cur, &size);
            cur = next;
        }
    }
    bubbleSort(temp, size);
    checkWord(temp, size);
    for (int i = 0; i < argument->k; i++) {
    	argument->head[i].frequency = temp[i].frequency;
        strcpy(argument->head[i].word, temp[i].word);
    }
    free(temp);
    return 0;
}

void bubbleSort(struct block *head, int size) {
    struct block *lptr = NULL;
    char tempWord[64];
    int tempFreq;
    int isSwapped;

    if (head == NULL) {
        return;
    }
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (head[j].frequency < head[j+1].frequency) {
                tempFreq = head[j].frequency;
                strcpy(tempWord, head[j].word);
                head[j].frequency = head[j+1].frequency;
                strcpy(head[j].word, head[j+1].word);
                head[j+1].frequency = tempFreq;
                strcpy(head[j+1].word, tempWord);
            }
        }
    }
}

void updateFrequency(struct block **head_ref, char *key, int *size) {
    if ((*head_ref)  != NULL) {
        for (int i = 0; i < (*size); i++) {
            if (strcmp(key, (*head_ref)[i].word) == 0) {
                ((*head_ref)[i].frequency)++;
                return;
            }
        }
        (*size)++;
        (*head_ref) = realloc((*head_ref), ((*size)) * sizeof(struct block));
        strcpy((*head_ref)[((*size) - 1)].word, key);
        (*head_ref)[((*size) - 1)].frequency = 1;
    }
    else {
    	(*size)++;
        (*head_ref) = realloc((*head_ref), ((*size)) * sizeof(struct block));
        strcpy((*head_ref)[(*size)-1].word, key);
        (*head_ref)[(*size)-1].frequency = 1;
    }
}

void toUpperCase(struct block *head, int size) {
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < strlen(head[i].word); j++) {
			head[i].word[j] = toupper(head[i].word[j]);
		}
	}
}

void checkFrequency(struct block *head, int size) {
    char tempWord[64];
    int tempFreq;
    if (head == NULL) {
        return;
    }
    for (int i = 0; i < size - 1; i++) {
        for (int j = i + 1; j < size; j++) {
            if (strcmp(head[i].word, head[j].word) == 0) {
		        head[i].frequency += head[j].frequency;
		        head[j].frequency = 0;
            }
        }
    }
}

void checkWord(struct block *head, int size) {
	for (int i = 0; i < size - 1; i++) {
        	for (int j = i + 1; j < size; j++) {
            		if (head[i].frequency == head[j].frequency) {
		        	int cmp = strcmp(head[i].word, head[j].word);
		        	if (cmp > 0) {
		        		char temp[64];
		        		strcpy(temp, head[i].word);
		        		strcpy(head[i].word, head[j].word);
		        		strcpy(head[j].word, temp);
		        	}
            		}
        	}
    	}
}


int main(int argc, char **argv) {
    const int k = atoi(argv[1]);
    const int n = atoi(argv[3]);

    struct threadBlock *thb = (struct threadBlock*)malloc(sizeof(struct threadBlock)*n);
    for (int i = 0; i < n; i++) {
    	thb[i].head = (struct block*)malloc(sizeof(struct block)*k);
    }

    pthread_t threads[n];
    

    for (int i = 0; i < n; i++) {
        thb[i].k = k;
        thb[i].inFileName = argv[i+4];

       	pthread_create(&(threads[i]), NULL, (void*)(&processFile), &(thb[i]));

        pthread_join(threads[i], NULL);
    }

    struct block *allInOne = (struct block*)malloc(sizeof(struct block)*n*k);

    int count = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < k; j++) {
            strcpy(allInOne[count].word, thb[i].head[j].word);
            allInOne[count].frequency = thb[i].head[j].frequency;
            count++;
        }
    }
    toUpperCase(allInOne, count);
    checkFrequency(allInOne, count);
    bubbleSort(allInOne, count);
    checkWord(allInOne, count);

    FILE* fw;
    fw = fopen(argv[2], "w+");
    for (int i = 0; i < k; i++) {
        if (allInOne[i].frequency != 0) {
    		fprintf(fw, "%s %d\n", allInOne[i].word, allInOne[i].frequency);
    	}
    }
    fclose(fw);
    free(allInOne);
    for(int i = 0; i < n; i++) {
    	free(thb[i].head);
    }
    free(thb);

    return 0;
}