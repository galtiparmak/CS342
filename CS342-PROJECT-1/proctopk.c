#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ctype.h>

#define SIZE_OF_STRUCT sizeof(struct block)

struct block {
    char word[64];
    int frequency;
};

void bubbleSort(struct block **head_ref, int size);
void updateFrequency(struct block **blockArr, char *key, int *size);
void toUpperCase(struct block **head_ref, int size);
void processFile(char *inFileName, const int k, struct block **head_ref, const int n, int *intptr);
void checkFrequency(struct block **head_ref, int size);
void checkWord(struct block *head, int size);


void processFile(char *inFileName, const int k, struct block **head_ref, const int n, int *intptr) {
    FILE *fr;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    struct block *temp = NULL;
    int size = 0;

    fr = fopen(inFileName, "r");
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
    bubbleSort(&temp, size);
    checkWord(temp, size);
    for (int i = 0; i < k; i++) {
    	(*head_ref)[*intptr].frequency = temp[i].frequency;
        strcpy((*head_ref)[*intptr].word, temp[i].word);
        (*intptr)++;
    }
}

void bubbleSort(struct block **head_ref, int size) {
    struct block *head;
    struct block *lptr = NULL;
    char tempWord[64];
    int tempFreq;
    int isSwapped;

    if (head == NULL) {
        return;
    }
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if ((*head_ref)[j].frequency < (*head_ref)[j+1].frequency) {
                tempFreq = (*head_ref)[j].frequency;
                strcpy(tempWord, (*head_ref)[j].word);
                (*head_ref)[j].frequency = (*head_ref)[j+1].frequency;
                strcpy((*head_ref)[j].word, (*head_ref)[j+1].word);
                (*head_ref)[j+1].frequency = tempFreq;
                strcpy((*head_ref)[j+1].word, tempWord);
            }
        }
    }
}

void updateFrequency(struct block **blockArr, char *key, int *size) {
    if ((*blockArr)  != NULL) {
        for (int i = 0; i < (*size); i++) {
            if (strcmp(key, (*blockArr)[i].word) == 0) {
                ((*blockArr)[i].frequency)++;
                return;
            }
        }
        (*size)++;
        *blockArr = realloc(*blockArr, ((*size)) * sizeof(struct block));
        strcpy((*blockArr)[((*size) - 1)].word, key);
        (*blockArr)[((*size) - 1)].frequency = 1;
    }
    else {
    	(*size)++;
        *blockArr = realloc(*blockArr, ((*size)) * sizeof(struct block));
        strcpy((*blockArr)[(*size)-1].word, key);
        (*blockArr)[(*size)-1].frequency = 1;
    }
}

void toUpperCase(struct block **head_ref, int size) {
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < strlen((*head_ref)[i].word); j++) {
			(*head_ref)[i].word[j] = toupper((*head_ref)[i].word[j]);
		}
	}
}

void checkFrequency(struct block **head_ref, int size) {
    char tempWord[64];
    int tempFreq;
    if ((*head_ref) == NULL) {
        return;
    }
    for (int i = 0; i < size - 1; i++) {
        for (int j = i + 1; j < size; j++) {
            if (strcmp((*head_ref)[i].word, (*head_ref)[j].word) == 0) {
		(*head_ref)[i].frequency += (*head_ref)[j].frequency;
		(*head_ref)[j].frequency = 0;
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
    key_t key1 = 2929;
    key_t key2 = 4242;
    const int k = atoi(argv[1]);
    const int n = atoi(argv[3]);
    int shmid1;
    int shmid2;
    char *inFileName;

    shmid1 = shmget(key1, SIZE_OF_STRUCT * k * n, 0666|IPC_CREAT);

    if (shmid1 == -1) {
        perror("Shared memory 1 get: ");
        exit(EXIT_FAILURE);
    }

    shmid2 = shmget(key2, sizeof(int), 0666|IPC_CREAT);

    if (shmid2 == -1) {
        perror("Shared memory 2 get: ");
        exit(EXIT_FAILURE);
    }

    struct block *head = (struct block*) shmat(shmid1, NULL, 0);
    int *index = shmat(shmid2, NULL, 0);
    *index = 0;

    if (head == (struct block*) -1) {
        perror("Shared memory attach: ");
        exit(EXIT_FAILURE);
   }

    for (int i = 0; i < n; i++) {
        pid_t childProcessID = fork();

        if (childProcessID == -1) {
            perror("Child process create: ");
            exit(EXIT_FAILURE);
        }
        else if (childProcessID == 0) {
            inFileName = argv[i+4];
            processFile(inFileName, k, &head, n, index);
            _exit(0);
        }
        else {
            wait(NULL);
        }
    }
    FILE* fw;
    fw = fopen(argv[2], "w+");
    
    toUpperCase(&head, *index);
    checkFrequency(&head, *index);
    bubbleSort(&head, *index);
    checkWord(head, *index);
    
    for (int i = 0; i < k; i++) {
    	if (head[i].frequency != 0) {
    		fprintf(fw, "%s %d\n", head[i].word, head[i].frequency);
    	}
    }
    fclose(fw);
    
    shmdt(head);
    shmdt(index);
    
    
    if (shmctl(shmid1, IPC_RMID, NULL) == -1) {
    	perror("Shared memory 1 remove: ");
        exit(EXIT_FAILURE);
    }
    
    if (shmctl(shmid2, IPC_RMID, NULL) == -1) {
    	perror("Shared memory 2 remove: ");
        exit(EXIT_FAILURE);
    }
    
    
    return 0;
}
