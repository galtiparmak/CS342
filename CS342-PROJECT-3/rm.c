#include "rm.h"

int DA; // indicates if deadlocks will be avoided or not
int N; // number of processes
int M; // number of resource types
bool finished[MAXP];
bool blocked[MAXP];
int ExistingRes[MAXR]; // Existing resources vector
int available[MAXR]; // number of available resources of each type
int allocation[MAXP][MAXR]; // amount of resources of each type allocated to each thread
int request[MAXP][MAXR]; // amount of resources of each type requested by each thread
int max_demand[MAXP][MAXR]; // max amount of resources of each type demanded by each thread
int need[MAXP][MAXR]; // amount of resources of each type needed by each thread
int current_tid;
char* headerMsg = "****************\n";

pthread_mutex_t mutex;
pthread_cond_t cond[MAXP];

int rm_init(int p_count, int r_count, int r_exist[], int avoid) {
    if (p_count <= 0 || r_count <= 0 || N > MAXP || M > MAXR) {
        return -1; // invalid input
    }
    N = p_count;
    M = r_count;
    DA = avoid;
    for (int i = 0; i < M; i++) {
        available[i] = r_exist[i];
    }
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            allocation[i][j] = 0;
            request[i][j] = 0;
            max_demand[i][j] = 0;
            need[i][j] = 0;
        }
    }
    pthread_mutex_init(&mutex, NULL);
    
    for(int i = 0; i < p_count; i++) {
        pthread_cond_init(&cond[i], NULL);
    }

    return 0;
}

int rm_thread_started(int tid) {
    if (tid < 0 || tid >= N) {
        return -1; // invalid tid
    }

    return 0;
}

int rm_claim(int claim[]) {
    // check if the claim is valid (i.e., non-negative and not greater than existing)
    for (int i = 0; i < M; i++) {
        if (claim[i] < 0 || claim[i] > ExistingRes[i]) {
            return -1;  // error: invalid claim
        }
    }
    
    // update the MaxDemand matrix with the claim information
    for (int i = 0; i < M; i++) {
        max_demand[current_tid][i] = claim[i];
    }
    
    return 0;  // success
}

int rm_thread_ended() {
    if (current_tid < 0 || current_tid >= N) {
        return -1; // invalid tid
    }

    for (int i = 0; i < M; i++) {
        allocation[current_tid][i] = 0;
        request[current_tid][i] = 0;
        max_demand[current_tid][i] = 0;
        need[current_tid][i] = 0;
        available[i] += allocation[current_tid][i];
    }

    finished[current_tid] = true;

    return 0;
}

int rm_request(int request[]) {
    pthread_mutex_lock(&mutex);

    // Check if the request is valid
    for (int i = 0; i < M; i++) {
        if (request[i] > (max_demand[current_tid][i] - allocation[current_tid][i])) {
            pthread_mutex_unlock(&mutex);
            return -1;
        }
    }

    // If deadlock avoidance is enabled, check if the request is safe
    if (DA) {
        if (!rm_detection()) {
            blocked[current_tid] = true;
            pthread_cond_wait(&cond[current_tid], &mutex);
        }
    }

    for (int i = 0; i < M; i++) {
        if (request[i] > available[i]) {
            pthread_mutex_unlock(&mutex);
            return -1;
        }
    }

    // If the request can be satisfied, allocate the resources
    for (int i = 0; i < M; i++) {
        allocation[current_tid][i] += request[i];
        if (DA) {
            need[current_tid][i] = max_demand[current_tid][i] - allocation[current_tid][i];
            available[i] -= request[i];
        }
    }

    pthread_mutex_unlock(&mutex);
    return 0;
}

int rm_release(int release[]) {
    // Check if the release request is valid
    for (int i = 0; i < M; i++) {
        if (release[i] > allocation[current_tid][i]) {
            return -1; // Error: Trying to release more instances than allocated
        }
    }

    // Release the resources and update the allocation and need matrices
    for (int i = 0; i < M; i++) {
        allocation[current_tid][i] -= release[i];
        need[current_tid][i] += release[i];
    }

    // Wake up blocked threads that can now proceed
    for (int i = 0; i < N; i++) {
        if (!finished[i] && blocked[i]) {
            for (int j = 0; j < M; j++) {
                if (need[i][j] > available[j]) {
                    break;
                }
            }

            // Thread i can now proceed
            blocked[i] = false;
            pthread_cond_signal(&cond[i]);
        }
    }

    return 0; // Success
}

int rm_detection() {
    pthread_mutex_lock(&mutex);

    int work[M];
    int deadlock = 0;
    
    // Initialize work array with available resources
    for (int i = 0; i < M; i++) {
        work[i] = available[i];
    }

    // Iterate through each thread to check if it can finish
    for (int k = 0; k < N; k++) {
        if (!finished[k]) {
            int can_allocate = 1;
            for (int j = 0; j < M; j++) {
                if (need[k][j] > work[j]) {
                    can_allocate = 0;
                    break;
                }
            }
            if (can_allocate) {
                // Simulate allocation of resources to thread k
                for (int j = 0; j < M; j++) {
                    work[j] += allocation[k][j];
                }
                finished[k] = 1;
                k = -1; // Restart from beginning to check if other threads can finish now
            }
        }
    }
    int i;
    // Check if there are any unfinished threads
    for (i = 0; i < N; i++) {
        if (!finished[i]) {
            deadlock = 1;
            break;
        }
    }

    pthread_mutex_unlock(&mutex);
    
    return deadlock ? i : 0;
}

void rm_print_state (char headermsg[]) {
    // Print header message
    printf(headermsg);
    printf("\n");

    // Print ExistingRes vector
    printf("Exist:\n");
    printf("     ");
    for (int i = 0; i < M; i++) {
        printf("R%d ", i);
    }

    printf("\n");
    printf("     ");
    for (int i = 0; i < M; i++) {
        printf("%d ", ExistingRes[i]);
    }
    printf("\n");

    // Print available vector
    printf("Available:\n");
    printf("     ");
    for (int i = 0; i < M; i++) {
        printf("R%d ", i);
    }

    printf("\n");
    printf("     ");
    for (int i = 0; i < M; i++) {
        printf("%d ", available[i]);
    }
    printf("\n");

    // Print allocation matrix
    printf("Allocation:\n");
    printf("    ");
    for (int j = 0; j < M; j++) {
        printf("R%d ", j);
    }
    printf("\n");
    for (int i = 0; i < N; i++) {
        printf("T%d: ", i);
        for (int j = 0; j < M; j++) {
            printf("%d  ", allocation[i][j]);
        }
        printf("\n");
    }

    // Print request matrix
    printf("Request:\n");
    printf("    ");
    for (int j = 0; j < M; j++) {
        printf("R%d ", j);
    }
    printf("\n");
    for (int i = 0; i < N; i++) {
        printf("T%d: ", i);
        for (int j = 0; j < M; j++) {
            printf("%d  ", request[i][j]);
        }
        printf("\n");
    }

    // Print max_demand matrix
    printf("MaxDemand:\n");
    printf("    ");
    for (int j = 0; j < M; j++) {
        printf("R%d ", j);
    }
    printf("\n");
    for (int i = 0; i < N; i++) {
        printf("T%d: ", i);
        for (int j = 0; j < M; j++) {
            printf("%d  ", max_demand[i][j]);
        }
        printf("\n");
    }

    // Print need matrix
    printf("Need:\n");
    printf("    ");
    for (int j = 0; j < M; j++) {
        printf("R%d ", j);
    }
    printf("\n");
    for (int i = 0; i < N; i++) {
        printf("T%d: ", i);
        for (int j = 0; j < M; j++) {
            printf("%d  ", need[i][j]);
        }
        printf("\n");
    }

    printf("#################\n");

}

void* thread_start1(void* arg) {
    int tid = *((int*)arg);

    rm_thread_started(tid);
    
    if (DA) {
        int claim[1] = {3};
        rm_claim(claim);
    }

    request[1][0] = 3;
    rm_request(request);

    rm_print_state(headerMsg);
    sleep(4);

    rm_release(request);

    rm_thread_ended();

    pthread_exit(NULL);

}

void* thread_start2(void* arg) {
    int tid = *((int*)arg);

    rm_thread_started(tid);
    
    if (DA) {
        int claim[1] = {5};
        rm_claim(claim);
    }

    request[1][0] = 5;
    rm_request(request);

    rm_print_state(headerMsg);
    sleep(4);

    rm_release(request);

    rm_thread_ended();

    pthread_exit(NULL);
}


int main() {
    // example usage
    N = 2;
    M = 1;
    DA = 1;
    ExistingRes[0] = 8;
    rm_init(N, M, ExistingRes, DA);
    pthread_t threads[N];
    int tids[N];
    for (int i = 0; i < N; i++) {
        blocked[i] = false;
        finished[i] = false;
        tids[i] = i;
    }

    pthread_create(&threads[0], NULL, thread_start1, (void*)(&tids[0]));
    pthread_create(&threads[1], NULL, thread_start2, (void*)(&tids[1]));

    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}

