#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>
#include <errno.h>
#include <semaphore.h>

struct performer{			// structure containing the information about the performer on the stage be it singer or musician
	char* name;
	char code;
	int arrival_time;
	struct performer *partner;			// variable stores the information if some singer joined the musician's performance
	pthread_t id;
}**stage;

typedef struct performer performer;

int total,acoustic,electric,crd,t1,t2,max_wait;
int bufsiz = 256;

sem_t acoustic_sem;						// semaphore which keeps track of the acoustic stages which are free
sem_t electric_sem;						// semaphore which keeps track of the electric stages which are free
sem_t total_sem;						// semaphore which keeps track of the total stages which are free
sem_t singer_sem;
sem_t coordinators;

void swap(int* a,int *b) {
	int *temp = (int*)malloc(sizeof(int));
	*temp = *a;
	*a = *b;
	*b = *temp;
}

void shufflearray(int *arr,int n) {			// function to shuffle the contents of an array
	 for (int i = n - 1; i > 0; i--)  
    {  
        int j = rand() % (i + 1);  
          swap(&arr[i], &arr[j]);  
    } 
}

void* music(void* input) {
	performer *in = (performer*)input;
	sleep(in->arrival_time);			// sleep for the time the performer takes to arrive
	printf("\033[1;32m");
	printf("%s %c arrived\n",in->name,in->code);
	int inacoustic = 1;
	int inelectric = 1;
	if(in->code == 'v')
		inelectric = 0;
	if(in->code == 'b')
		inacoustic = 0;
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += max_wait;
	int duration = rand() % (t2 - t1 + 1) + t1;		// duration for which the musician will play
	int *arr,size;
	if(!inacoustic) {
		arr = (int *)malloc(sizeof(int) * electric);
		size = electric;
		for(int i = 0 ; i < electric ; i++)
			arr[i] = i;
	}
	else if(!inelectric) {
		arr = (int *)malloc(sizeof(int) * acoustic);
		size = acoustic;
		for(int i = 0 ; i < acoustic ; i++)
			arr[i] = electric + i;
	}
	else {
		arr = (int *)malloc(sizeof(int) * (acoustic + electric));
		size = acoustic + electric;
		for(int i = 0 ; i < acoustic + electric ; i++)
			arr[i] = i;
	}
	if(!inacoustic) {					// if the musician plays only on electric stage
		if(sem_timedwait(&electric_sem,&ts) == -1 && errno == ETIMEDOUT) {			// check for the maximum waiting time for a electric stage to get free
			printf("\033[1;32m");
			printf("%s %c left because of impatience\n",in->name,in->code);
			return NULL;
		}
	}
	else if(!inelectric) {
		if(sem_timedwait(&acoustic_sem,&ts) == -1 && errno == ETIMEDOUT) {
			printf("\033[0;31m");
			printf("%s %c left because of impatience\n",in->name,in->code);
			return NULL;
		}
	}
	else {
		if(sem_timedwait(&total_sem,&ts) == -1 && errno == ETIMEDOUT) {
			printf("\033[0;31m");
			printf("%s %c left because of impatience\n",in->name,in->code);
			return NULL;
		}
	}
	shufflearray(arr,size);
	for(int i = 0 ; i < size ; i++) {
		int stgno = arr[i];
		if(stage[stgno] != NULL)
			continue;
		stage[stgno] = in;
		if(stgno < electric) {
			if(!inacoustic) {
				if(sem_trywait(&total_sem))
					continue;
			}
			else {
				if(sem_wait(&electric_sem))
					continue;
			}
			printf("\033[1;34m");
			printf("%s performing %c at electric stage, stage number %d for %d sec\n",in->name,in->code,stgno + 1,duration);
			sleep(duration);
			if(in->partner != NULL) {
				sleep(2);
				stage[stgno] = NULL;
				sem_post(&electric_sem);
				sem_post(&singer_sem);
			}
			else {
				stage[stgno] = NULL;
				sem_post(&electric_sem);
			}
			sem_post(&total_sem);
			printf("\033[1;33m");
			printf("%s performance at electric stage, stage number %d ended\n",in->name,stgno + 1);
		}
		else {
			if(!inelectric) {
				if(sem_trywait(&total_sem))
					continue;
			}
			else {
				if(sem_trywait(&acoustic_sem))
					continue;
			}
			printf("\033[1;34m");
			printf("%s performing %c at acoustic stage, stage number %d for %d sec\n",in->name,in->code,stgno + 1,duration);
			sleep(duration);
			if(in->partner != NULL) {
				sleep(2);
				stage[stgno] = NULL;
				sem_post(&acoustic_sem);
				sem_post(&singer_sem);
			}
			else {
				stage[stgno] = NULL;
				sem_post(&acoustic_sem);
			}
			sem_post(&total_sem);
			printf("\033[01;33m");
			printf("%s performance at acoustic stage, stage number %d ended\n",in->name,stgno + 1);
		}
		break;

	}
	sem_wait(&coordinators);
	printf("\033[0;35m");
	if(in->partner == NULL)
		printf("%s collecting T-shirt\n",in->name);
	else
		printf("%s and their partner %s collecting T-shirt\n",in->name,in->partner->name);
	sleep(2);
	sem_post(&coordinators);
	return NULL;
}


void* sing(void* input) {
	performer *in = (performer*)input;
	sleep(in->arrival_time);
	printf("\033[1;36m");
	printf("%s s arrived\n",in->name);
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += max_wait;
	if(sem_timedwait(&singer_sem,&ts) == -1 && errno == ETIMEDOUT) {
		printf("\033[0;31m"); 
		printf("%s %c left because of impatience\n",in->name,in->code);
		return NULL;
	}
	int duration = rand() % (t2 - t1 + 1) + t1;
	int *arr = (int *)malloc(sizeof(int) * (acoustic + electric));
	int size = acoustic + electric;
	for(int i = 0 ; i < acoustic + electric ; i++)
		arr[i] = i;
	shufflearray(arr,size);
	for(int i = 0 ; i < acoustic + electric ; i++) {
		int stgno = arr[i];
		if(stage[stgno] != NULL && (stage[stgno]->code == 's' || stage[stgno]->partner != NULL))
			continue;
		if(stage[stgno] != NULL) {
			stage[stgno]->partner = in;
			printf("\033[0;35m");
			printf("%s joined %s's performance, performance extended by 2 sec\n",in->name,stage[stgno]->name);
			return NULL;
		}
		else {
			if(stgno < electric && sem_trywait(&electric_sem) == 0) {
				sem_wait(&total_sem);
				stage[stgno] = in;
				printf("\033[1;34m");
				printf("%s performing %c at electric stage, stage number %d for %d sec\n",in->name,in->code,stgno + 1,duration);
				sleep(duration);
				printf("\033[1;33m");
				printf("%s performance at electric stage, stage number %d ended\n",in->name,stgno + 1);
				sem_post(&electric_sem);
				sem_post(&total_sem);
				sem_post(&singer_sem);
				stage[stgno] = NULL;
				break;
			}
			else if(stgno >= electric && sem_trywait(&acoustic_sem) == 0) {
				sem_wait(&total_sem);
				stage[stgno] = in;
				printf("\033[1;34m");
				printf("%s performing %c at acoustic stage, stage number %d for %d sec\n",in->name,in->code,stgno + 1,duration);
				sleep(duration);
				printf("\033[1;33m");
				printf("%s performance at acoustic stage, stage number %d ended\n",in->name,stgno + 1);
				sem_post(&acoustic_sem);
				sem_post(&total_sem);
				sem_post(&singer_sem);
				stage[stgno] = NULL;
				break;
			}
		}
	}
	sem_wait(&coordinators);
	printf("\033[0;35m");
	printf("%s collecting T-shirt\n",in->name);
	sleep(2);
	sem_post(&coordinators);
	return NULL;
}
int main() {
	srand(time(NULL));
	scanf("%d %d %d %d %d %d %d",&total,&acoustic,&electric,&crd,&t1,&t2,&max_wait);

	sem_init(&acoustic_sem,0,acoustic);
	sem_init(&electric_sem,0,electric);
	sem_init(&total_sem,0,acoustic + electric);
	sem_init(&singer_sem,0,acoustic + electric);
	sem_init(&coordinators,0,crd);

	stage = (performer**)malloc(sizeof(performer*) * (acoustic + electric));
	for(int i = 0 ; i < acoustic + electric ; i++) {
		stage[i] = (performer*)malloc(sizeof(performer));	
		stage[i] = NULL; 
	}
	char* type = (char*)malloc(sizeof(char) * total);
	int* arvt = (int*)malloc(sizeof(int) * total);
	char** name = (char**)malloc(sizeof(char*) * total);
	for(int i = 0 ; i < total ; i++) {
		name[i] = (char*)malloc(sizeof(char) * bufsiz);
		scanf("%s %c %d",name[i],&type[i],&arvt[i]); 
	}
	performer* temp = (performer*)malloc(sizeof(performer) * total);
	for(int i = 0 ; i < total ; i++) {
		temp[i].name = name[i];
		temp[i].code = type[i];
		temp[i].partner = NULL;
		temp[i].arrival_time = arvt[i];
		if(type[i] != 's') {
			pthread_create(&temp[i].id,NULL,music,&temp[i]);
		}
		else
			pthread_create(&temp[i].id,NULL,sing,&temp[i]);
		usleep(50);
	}
	for(int i = 0 ; i < total ; i++) {
		pthread_join(temp[i].id,NULL);
	}
	printf("\033[1;31m");
	printf("Finished\n");
	return 0;
}
