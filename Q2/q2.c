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
#include<signal.h>
int n,m,o;

pthread_mutex_t *Companies;			// mutex for all the pharma companies 
pthread_mutex_t *zones;				// mutex for all the vaccination zones
typedef struct comp{				// structure which stores the information about the pharma company
	int no_batches;
	int no_vaccine_perbatch;
	int total_vaccines;
	float probability;
	pthread_t id;
}comp;

typedef struct zone{				// structure which stores the information about the vaccination zone
	int no_vaccines;
	int no_slots;
	float probability;
	pthread_t id;

}zone;

typedef struct student{				// structure which stores the information about the student
	int round;
	pthread_t id;
}student;

struct input{						// structure which stores the id in the array for the passed company or the vaccination zone
	int num;
};

comp *Comps;						// array of structures to store info about the companies
zone *Zone;							// array of structures to store info about the vaccination zones
student *Stud;

void batch_ready(int i) {			// function which implements busy waiting till the companies vaccines are consumed
	while(Comps[i].total_vaccines > 0 && o > 0);
	sleep(1);
	if(o == 0)
		return;
	printf("All the vaccines prepared by Pharmaceutical Company %d are emptied. Resuming production now\n",i + 1);
}
void* make_vaccine(void* in) {		// function in which companies come directly and make the vaccines
	int i = ((struct input*)in)->num;
	while(1) {
		if(o == 0)
			return NULL;
		int making_time = rand() % 4 + 2;
		int batches = rand() % 5 + 1;
		printf("\033[0;39m");
		printf("Pharmaceutical company %d is making %d batches of vaccines with success probability %f\n",i + 1,batches,Comps[i].probability);
		sleep(making_time);
		printf("Pharmaceutical Company %d has prepared %d batches of vaccines which have success probability %f. Waiting for all the vaccines to be used to resume production\n",i + 1,batches,Comps[i].probability);
		Comps[i].no_batches = batches;
		Comps[i].no_vaccine_perbatch = rand() % 11 + 10;
		Comps[i].total_vaccines = Comps[i].no_batches * Comps[i].no_vaccine_perbatch;
		printf("\033[1;96m");
		fflush(stdout);
		batch_ready(i);		// calling the busy waiting function
	}
	return NULL;
}
void ready_zone(int ind) {		// busy waiting fucnction and waits till the zone uses all it's vaccines
	while(Zone[ind].no_slots > 0 && o > 0);
	sleep(1);
}
void* vaccinate(void* input) {		// function for the zones to come and allot slots and vaccinate students
	int ind = ((struct input*)input)->num;
	printf("\033[1;37m");
	printf("Vaccination zone %d is waiting for vaccines\n",ind + 1);
	while(1) {
		int flag = 1;
		while(flag) {
			for(int i = 0; i < n; i++) {
				if(o == 0)
					return NULL;
				int compind = i;
				if(pthread_mutex_trylock(&Companies[compind]))
					continue;
				if(Comps[compind].no_batches > 0) {
					flag = 0;
					printf("\033[0;33m");
					printf("Pharmaceutical Company %d is delivering a vaccine batch to Vaccination Zone %d which has success probability %f\n",compind + 1,ind + 1,Comps[compind] .probability);
					printf("\033[0;34m");
					sleep(1);
					printf("Pharmaceutical Company %d has delivered vaccines to Vaccination zone %d, resuming vaccinations now\n",compind + 1,ind + 1);
					Comps[compind].no_batches--;
					Zone[ind].probability = Comps[compind].probability;
					Zone[ind].no_vaccines = Comps[compind].no_vaccine_perbatch;
					pthread_mutex_unlock(&Companies[compind]);
					sleep(1);
					while(Zone[ind].no_vaccines > 0) {
						int upper_limit;
						if(Zone[ind].no_vaccines > 8)
							upper_limit = 8;
						else
							upper_limit = Zone[ind].no_vaccines;
						if(upper_limit > o)
							upper_limit = o;
						if(upper_limit == 0)
							return NULL;
						int slots = rand() % upper_limit + 1;	
						printf("\033[0;31m");
						printf("Vaccination Zone %d is ready to vaccinate with %d slots \n",ind + 1,slots);
						sleep(1);
						printf("\033[1;31m");
						printf("Vaccination zone %d entering Vaccination phase\n",ind + 1);
						fflush(stdout);
						sleep(1);
						Zone[ind].no_slots = slots;
						ready_zone(ind);
						Zone[ind].no_vaccines -= slots;
						if(o == 0)
							return NULL;
					}
					if(o == 0)
						return NULL;	
					Comps[compind].total_vaccines -= Comps[compind].no_vaccine_perbatch; 
					printf("\033[1;36m");
					printf("Vaccination zone %d has run out of vaccines\n",ind + 1);
					sleep(1);
					break;
				}
				else
					pthread_mutex_unlock(&Companies[compind]);
			}
		}
	}
	return NULL;
}

void* take_vaccine(void* input) {			// function for students to come and get vaccinated by finding a free zone
	int ind = ((struct input*)input)->num;
	while(Stud[ind].round <= 3) {
		printf("\033[0;35m");
		printf("Student %d has arrived for his %d round of vaccination\n",ind + 1,Stud[ind].round);
		printf("\033[0;33m");
		printf("Student %d is waiting to be allocated a slot on a Vacination Zone\n",ind + 1);
		sleep(1);
		int flag = 1;
		while(flag) {
			for(int i = 0 ; i < m ; i++) {
				int zoneind = i;
				if(pthread_mutex_trylock(&zones[zoneind]))
					continue;
				if(Zone[zoneind].no_slots > 0) {
					flag = 0;
					Zone[zoneind].no_slots--;
					printf("\033[1;32m");
					printf("Student %d assigned a slot on the Vaccination Zone %d and waiting to be vaccinated\n",ind + 1,zoneind + 1);
					sleep(1);
					printf("\033[1;35m");
					printf("Student %d on Vaccination Zone %d has been vaccinated which has success probability %f\n",ind + 1,zoneind + 1,Zone[zoneind].probability);
					pthread_mutex_unlock(&zones[zoneind]); 
					int prob = (int)(100.0 * rand() / (RAND_MAX + 1.0)) + 1;
					int result;
					if(prob <= Zone[zoneind].probability * 100)
						result = 1;
					else
						result = 0;
					if(result == 1) {
						printf("\033[1;34m");
						printf("Student %d has tested positive for anitbodies\n",ind + 1);
						o--;
						return NULL;
					}
					printf("\033[1;37m");
					printf("Student %d has tested negative for anitbodies\n",ind + 1);
					Stud[ind].round++;
					sleep(1);
					break;
				}
				else
					pthread_mutex_unlock(&zones[zoneind]); 
			}
		}
	}
	o--;
}
int main() {
	srand(time(NULL));
	scanf("%d %d %d",&n,&m,&o);
	Companies = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * n);
	zones = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * m);
	

	Comps = (comp*)malloc(sizeof(comp) * n);
	Zone = (zone*)malloc(sizeof(zone) * m);
	Stud = (student*)malloc(sizeof(student) * o);

	for(int i = 0 ; i < n ; i++) pthread_mutex_init(&Companies[i],NULL);
	for(int i = 0 ; i < m ; i++) pthread_mutex_init(&zones[i],NULL);

	for(int i = 0 ; i < n ; i++) 
		scanf("%f",&Comps[i].probability);
	for(int i = 0 ; i < n ; i++) {
		struct input *temp = (struct input *) malloc(sizeof(struct input));
		temp->num = i;
		pthread_create(&Comps[i].id,NULL,make_vaccine,(void*)temp);
	}
	sleep(6);				// WAITING FOR ALL THE COMPANIES TO FINISH THEIR VACCINE BREWING TIME
	for(int i = 0 ; i < m ; i++) {
		struct input *temp = (struct input *) malloc(sizeof(struct input));
		temp->num = i;
		pthread_create(&Zone[i].id,NULL,vaccinate,(void*)temp);
		usleep(50);
	}
	sleep(1);
	for(int i = 0 ; i < o ; i++) {
		struct input *temp = (struct input *) malloc(sizeof(struct input));
		temp->num = i;
		Stud[i].round = 1;
		pthread_create(&Stud[i].id,NULL,take_vaccine,(void*)temp);
	}
	int co = o;
	for(int i = 0 ; i < co ; i++) {
		pthread_join(Stud[i].id,NULL);
		if(Stud[i].round <= 3) {
			printf("\033[1;33m");
			printf("Student %d has joined the college :)\n",i + 1);
		}
		else {
			printf("\033[1;32m");
			printf("Student %d has been sent home :(\n",i + 1);
		}
		fflush(stdout);
	}
	printf("\033[1;32m");
	puts("Simulation over");
	return 0;
}