#include "mps.h"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
processor_t* main_processor;
clock_t start;
FILE *fw;
int b;
int num_process = 0;
double total_turnaround = 0;
process_t* processes;

void queue_init(ready_queue_t *queue) {
    queue->head = NULL;
    queue->tail = NULL;
    queue->workload = 0;
}

void enqueue(ready_queue_t *queue, process_t **process) {
    pthread_mutex_lock(&queue_lock);
    node_t *new_node = (node_t*)malloc(sizeof(node_t));
    new_node->process = *process;
    new_node->next = NULL;

    if (queue->tail == NULL) {
        queue->head = new_node;
        queue->tail = new_node;
    } else {
        queue->tail->next = new_node;
        queue->tail = new_node;
    }
    queue->workload += (*process)->burst_length;
    pthread_mutex_unlock(&queue_lock);
}

process_t* dequeue(ready_queue_t *queue) {
	pthread_mutex_lock(&queue_lock);
    if (queue->head == NULL) {
        return NULL;
    }

    node_t *node = queue->head;
    process_t *process = node->process;

    queue->head = queue->head->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    queue->workload -= process->burst_length;
    pthread_mutex_unlock(&queue_lock);
    free(node);
    return process;
}

void insertBeforeTail(ready_queue_t *queue, process_t **process) {
	node_t *new_node = (node_t*)malloc(sizeof(node_t));
	new_node->process = *process;
	new_node->next = NULL;
	node_t* current = queue->head;
	node_t* prev = NULL;
	pthread_mutex_lock(&queue_lock);
	
	if(current->process->pid == -1) {
		queue->head = new_node;
		new_node->next = current;
	}
	else {
		while(1) {
			if(current->process->pid == -1) {
				prev->next = new_node;
				new_node->next = current;
				new_node->process->arrival_time = clock();
				break;
			}
			prev = current;
			current = current->next;
		}	
	}
	pthread_mutex_unlock(&queue_lock);
}

void* run_processor(threadBlock* arg) {
    while (1) {
    	if (arg->sap == 'S') {
    		pthread_mutex_lock(&lock);
        	if (main_processor->ready_queue.head == NULL) {
            		pthread_mutex_unlock(&lock);
            		usleep(1000);
            		continue;
        	}
        	if(main_processor->ready_queue.head->process->pid == -1) {
        		free(main_processor->ready_queue.head->process);
        		free(main_processor->ready_queue.head);
        		print_information();
        		exit(0);
        	}
        	execute_process(arg->processor.id, &(main_processor->ready_queue), arg->algo, arg->q, arg->outmode);	
        	pthread_mutex_unlock(&lock);
    	}
    	else if (arg->sap == 'M') {
    		pthread_mutex_lock(&(arg->processor.queue_lock));
    		if (arg->processor.ready_queue.head == NULL) {
            		pthread_mutex_unlock(&lock);
            		usleep(1000);
            		continue;
        	}
        	if(arg->processor.ready_queue.head->process->pid == -1) {
        		free(arg->processor.ready_queue.head->process);
        		free(arg->processor.ready_queue.head);
        		pthread_mutex_unlock(&(arg->processor.queue_lock));
        		pthread_exit(NULL);
        	}
        	execute_process(arg->processor.id, &(arg->processor.ready_queue), arg->algo, arg->q, arg->outmode);
        	pthread_mutex_unlock(&(arg->processor.queue_lock));
    	}
    }
}

void readFile(processor_t* processor, char* inFileName, int outmode) {
    FILE *fr;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    fr = fopen(inFileName, "r");
    if (fr == NULL) {
    	printf("file error");
        exit(EXIT_FAILURE);
    }
    b = 1;
    while ((read = getline(&line, &len, fr)) != -1) {
        char *cur = line, *next = line;
        int a;
        while (cur != NULL) {
            strsep(&next, " ");
            if (cur[strlen(cur)-1] == '\n') {
            	cur = strtok(cur, "\n");
            }

            if (strcmp(cur, "PL") == 0) {
                a = atoi(next);
                process_t* np = (process_t*)malloc(sizeof(process_t));
                np->arrival_time = clock();
                np->burst_length = a;
                np->pid = b++;
                np->processorId = processor->id;
                np->remaining_time = a;
                enqueue(&(processor->ready_queue), &np);
            }
            else if (strcmp(cur, "IAT") == 0) {
                a = atoi(next);
                usleep(a*1000);
            }
            cur = next;
        }
    }
    fclose(fr);
}


void fcfs_run(process_t* process) {
    process->start_time = clock();
    usleep(process->burst_length*1000);
    process->finish_time = clock();
    process->remaining_time = 0;
    process->turnaround_time = (int)(process->finish_time) - (int)(process->arrival_time);
    process->waiting_time = (int)(process->start_time) - (int)(process->arrival_time);
}

process_t* sjf_run(ready_queue_t *queue) {
    node_t *current = queue->head;
    node_t *prev = NULL;
    node_t *prev_prev = NULL;
    node_t *shortest_node = current;
    process_t *shortest_job = current->process;
	
    while (1) {
    	if(current->process->pid == -1) break;
        if (current->process->burst_length < shortest_job->burst_length) {
            shortest_node = current;
            shortest_job = current->process;
            prev_prev = prev;
        }
        prev = current;
        current = current->next;
    }
    
    if (shortest_node == queue->head) {
        queue->head = shortest_node->next;
    } else {
        prev_prev->next = shortest_node->next;
    }

    if (shortest_node == queue->tail) {
        queue->tail = prev;
    }
    free(shortest_node);
    return shortest_job;
}

void rr_run(ready_queue_t* queue, process_t* process, int q) {
	if (process->remaining_time <= q) {
		process->start_time = clock();
    		usleep(process->remaining_time*1000);
    		process->finish_time = clock();
    		process->remaining_time = 0;
    		process->turnaround_time = (int)(process->finish_time) - (int)(process->arrival_time);
    		process->waiting_time = (int)(process->start_time) - (int)(process->arrival_time);
	}
	else {
		process->start_time = clock();
	    	usleep(q*1000);
	    	process->finish_time = clock();
		process->remaining_time -= q;
		process->turnaround_time = (int)(process->finish_time) - (int)(process->arrival_time);
		process->waiting_time = process->turnaround_time - q;
		insertBeforeTail(queue, &process);
	}
}


void execute_process(int cpu, ready_queue_t* ready_queue, char* algo, int q, int outmode) {
    process_t* process;
    if (strcmp(algo, "FCFS") == 0) {
        process = dequeue(ready_queue);
        fcfs_run(process);
    }
    else if (strcmp(algo, "SJF") == 0) {
        process = sjf_run(ready_queue);
        fcfs_run(process);
    }
    else if (strcmp(algo, "RR") == 0){
    	process = dequeue(ready_queue);
    	rr_run(ready_queue, process, q);
    }
	clock_t end = clock();
	int exetime = (int)end - (int)start;

	process->processorId = cpu;

	processes[num_process].pid = process->pid;
	processes[num_process].burst_length = process->burst_length;
	processes[num_process].arrival_time = process->arrival_time;
	processes[num_process].processorId = process->processorId;
	processes[num_process].finish_time = process->finish_time;
	processes[num_process].waiting_time = process->waiting_time;
	processes[num_process].turnaround_time = process->turnaround_time;
	
	total_turnaround += process->turnaround_time;
	num_process++;

	if (outmode == 2) {
	   	fprintf(fw, "time = %d, cpu = %d, pid = %d, burstlen = %d, remainingtime = %d\n", exetime, cpu, process->pid, process->burst_length, process->remaining_time);
	   	printf("time = %d, cpu = %d, pid = %d, burstlen = %d, remainingtime = %d\n", exetime, cpu, process->pid, process->burst_length, process->remaining_time);
	}
	else if (outmode == 3) {
		fprintf(fw, "process with pid: %d is picked for cpu: %d\n", process->pid, cpu);
		fprintf(fw, "time slice expired for a burst = %d\n", process->burst_length - process->remaining_time);
		fprintf(fw, "burst of process with pid: %d is finished at %ld\n", process->pid, end);
		printf("process with pid: %d is picked for cpu: %d\n", process->pid, cpu);
		printf("time slice expired for a burst = %d\n", process->burst_length - process->remaining_time);
		printf("burst of process with pid: %d is finished at %ld\n", process->pid, end);
	}

}

void fillQueue(ready_queue_t *ready_queue, int t, int t1, int t2, int l, int l1, int l2, int pc) {
	int a = 1;
	double u, x;
	
	for (int i = 0; i < pc; i++) {
		while (1) {
			u = (double) rand() / RAND_MAX;
			x = ((-1)*log(1-u))*l;
			if (x >= l1) {
				if (x <= l2) {
					break;
				}
			}
		}
		int burst = (int)x;
		process_t *process = (process_t*)malloc(sizeof(process_t));
		process->arrival_time = clock();
		process->pid = a++;
		process->burst_length = burst;
		process->remaining_time = burst;
		process->processorId = 0;
		enqueue(ready_queue, &process);
	}
}

void print_information() {
	process_t temp;
	for (int i = 0; i < num_process - 1; i++) {
		for (int j = 0; j < num_process - i - 1; j++) {
			if (processes[j].pid > processes[j+1].pid) {
				temp.pid = processes[j].pid;
				temp.burst_length = processes[j].burst_length;
				temp.arrival_time = processes[j].arrival_time;
				temp.processorId = processes[j].processorId;
				temp.finish_time = processes[j].finish_time;
				temp.waiting_time = processes[j].waiting_time;
				temp.turnaround_time = processes[j].turnaround_time;
				
				processes[j].pid = processes[j+1].pid;
				processes[j].burst_length = processes[j+1].burst_length;
				processes[j].arrival_time = processes[j+1].arrival_time;
				processes[j].processorId = processes[j+1].processorId;
				processes[j].finish_time = processes[j+1].finish_time;
				processes[j].waiting_time = processes[j+1].waiting_time;
				processes[j].turnaround_time = processes[j+1].turnaround_time;
				
				processes[j+1].pid = temp.pid;
				processes[j+1].burst_length = temp.burst_length;
				processes[j+1].arrival_time = temp.arrival_time;
				processes[j+1].processorId = temp.processorId;
				processes[j+1].finish_time = temp.finish_time;
				processes[j+1].waiting_time = temp.waiting_time;
				processes[j+1].turnaround_time = temp.turnaround_time;
			}
		}
	}


	printf("pid    cpu    burstlen    arv    finish    waitingtime    turnaround\n");
    
    for (int i = 0; i < num_process; i++) {
    	printf("%3d%7d%12d%7d%10d%15d%14d\n", processes[i].pid, processes[i].processorId, processes[i].burst_length, (int)(processes[i].arrival_time), (int)(processes[i].finish_time), processes[i].waiting_time, processes[i].turnaround_time);
    }
    
    int avg_turnaround_time = (int)(total_turnaround / num_process);
    
    printf("avgerage turnaround time: %d\n", avg_turnaround_time);
}


int main(int argc, char **argv) {
	start = clock();
	int n = 2;
	char* sap = "M";
	char* qs = "RM";
	char* alg = "RR";
	int q = 20;
	char* infile = "in.txt";
	int outmode = 1;
	char* outfile = "out.txt";
	int t = 200, t1 = 10, t2 = 1000, l = 100, l1 = 10, l2 = 500, pc = 10;
	int check = 0;
	
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-n") == 0 ) {
			n = atoi(argv[i+1]);
		}
		
		if (strcmp(argv[i], "-a") == 0) {
			sap = argv[i+1];
			qs = argv[i+2];
		}
		
		if (strcmp(argv[i], "-s") == 0) {
			alg = argv[i+1];
			q = atoi(argv[i+2]);
		}
		
		if (strcmp(argv[i], "-i") == 0) {
			infile = argv[i+1];
		}
		
		if (strcmp(argv[i], "-m") == 0) {
			outmode = atoi(argv[i+1]);
		}
		
		if (strcmp(argv[i], "-o") == 0) {
			outfile = argv[i+1];
		}
		
		if (strcmp(argv[i], "-r") == 0) {
			t = atoi(argv[i+1]);
			t1 = atoi(argv[i+2]);
			t2 = atoi(argv[i+3]);
			l = atoi(argv[i+4]);
			l1 = atoi(argv[i+5]);
			l2 = atoi(argv[i+6]);
			pc = atoi(argv[i+7]);
			check = 1;
		}
	}
	
	main_processor = (processor_t*)malloc(sizeof(processor_t));
	main_processor->id = 0;
	pthread_mutex_init(&(main_processor->queue_lock), NULL);
	queue_init(&(main_processor->ready_queue));
	
	fw = fopen(outfile, "w+");
    	if (fw == NULL) {
    		printf("outfile error\n");
        	exit(EXIT_FAILURE);
    	}
	
	if (!check) {
		readFile(main_processor, infile, outmode);
		processes = (process_t*)malloc(sizeof(process_t)*(b-1));
	}
	else {
		fillQueue(&(main_processor->ready_queue), t, t1, t2, l, l1, l2, pc);
		processes = (process_t*)malloc(sizeof(process_t)*pc);
	}
	
	process_t *dummy = (process_t*)malloc(sizeof(process_t));
	dummy->pid = -1;
	dummy->burst_length = 0;
	enqueue(&(main_processor->ready_queue), &dummy);
	
    	threadBlock *thb = (threadBlock*)malloc(sizeof(threadBlock)*n);
    	pthread_t threads[n];

    for (int i = 0; i < n; i++) {
        thb[i].processor.id = (i + 1);
        thb[i].algo = alg;
        thb[i].inFileName = infile;
        thb[i].sap = sap[0];
        thb[i].q = q;
        thb[i].outmode = outmode;
        pthread_mutex_init(&(thb[i].processor.queue_lock), NULL);
    	queue_init(&(thb[i].processor.ready_queue));
    }
    
    if (sap[0] == 'M') {
    	pthread_mutex_lock(&lock);
    	if(strcmp(qs, "RM") == 0) {
    		while(main_processor->ready_queue.head->process->pid != -1) {
    	
	    		for (int i = 0; i < n; i++) {
	    			if(main_processor->ready_queue.head->process->pid == -1) 
	    				break;
	    			
	    			process_t *process = dequeue(&(main_processor->ready_queue));
	    			pthread_mutex_lock(&(thb[i].processor.queue_lock));
	    			enqueue(&(thb[i].processor.ready_queue), &process);
	    			pthread_mutex_unlock(&(thb[i].processor.queue_lock));
	    		}
	    		
	    	}
    	}
    	
    	else if (strcmp(qs, "LM") == 0) {
	    	while(main_processor->ready_queue.head->process->pid != -1) {
	    	processor_t *smallest_queue_ref;
	    	
	    		for (int i = 0; i < n; i++) {
	    			if (i == 0) {
	    				smallest_queue_ref = &(thb[i].processor);	
	    			}
	    			else {
	    				if (smallest_queue_ref->ready_queue.workload > thb[i].processor.ready_queue.workload) {
	    					smallest_queue_ref = &(thb[i].processor);
	    				}
	    			}
    			}
    			
    			process_t *process = dequeue(&(main_processor->ready_queue));
    			pthread_mutex_lock(&(smallest_queue_ref->queue_lock));
    			enqueue(&(smallest_queue_ref->ready_queue), &process);
    			pthread_mutex_unlock(&(smallest_queue_ref->queue_lock));
    		
    		}
    	}
    	
    	for (int i = 0; i < n; i++) {
    		pthread_mutex_lock(&(thb[i].processor.queue_lock));
    		process_t *dummy = (process_t*)malloc(sizeof(process_t));
		dummy->pid = -1;
		dummy->burst_length = 0;
    		enqueue(&(thb[i].processor.ready_queue), &dummy);
    		pthread_mutex_unlock(&(thb[i].processor.queue_lock));
    	}
    	
    	pthread_mutex_unlock(&lock);
    }
    
    
    for (int i = 0; i < n; i++) {
    	pthread_create(&(threads[i]), NULL, (void*)(&run_processor), &thb[i]);
    }

	for (int i = 0; i < n; i++) {
        pthread_join(threads[i], NULL);
    }
    
    print_information();
    
    fclose(fw);
    free(thb);
    free(processes);

    return 0;
}
