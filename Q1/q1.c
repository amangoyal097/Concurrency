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

int * sharedmemory(size_t size){  				// Fucntion to allocate share memory
     key_t mem_key = IPC_PRIVATE;
     int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
     return (int*)shmat(shm_id, NULL, 0);
}
void selection_sort(int *arr,int start,int end) {			// Fucntion which sorts the array using selection sort
	int ind = start;
	while(ind <= end) {
		int mini = arr[ind];
		int pos = ind;
		for(int j = ind ; j <= end ; j++) {
			if(arr[j] < mini) {
				pos = j;
				mini = arr[j];
			}
		}
		int temp = arr[ind];
		arr[ind] = arr[pos];
		arr[pos] = temp;
		ind++;
	}
}
void combine(int arr[],int start,int mid,int last) 		// Merge function for mergesort which merges the sorted left and right halfs of the array
{
    int len1,len2,i,j,k;
    len1=mid+1;
    len2=last+1;
    int *temp = (int*)malloc(len2*sizeof(int));
    i=start;j=mid+1;k=0;
    while(i<len1&&j<len2)
    {
        if(arr[i]<=arr[j])
        {
            temp[k]=arr[i];
            k++;
            i++;
        }
        else
        {
            temp[k]=arr[j];
            j++;
            k++;
        }
    }
    while(i<len1)
    {
        temp[k]=arr[i];
        i++;
        k++;
    }
    while(j<len2)
    {
        temp[k]=arr[j];
        k++;
        j++;
    }
    for(i=start,j=0;j<k;i++,j++)
        arr[i]=temp[j];
}
void normal_mergesort(int arr[],int start,int end) 			// Function which uses sequential mergesort to sort the array
{    int mid;
    if(start<end)
    {   if(end - start + 1 < 5) {
    		selection_sort(arr,start,end);
    		return;
   	 	}
    	mid=(start+end)/2;
        normal_mergesort(arr,start,mid);
        normal_mergesort(arr,mid+1,end);
        combine(arr,start,mid,end);
    }
}
void concurrent_mergesort(int *arr,int start,int end) {		// Fucntion which uses concurrent mergesort to sort the array
	int mid;
	if(start >= end)
		return;
	if(end - start + 1 < 5) {
    		selection_sort(arr,start,end);
    		return;
   	}
	mid = (start + end) / 2;
	int pid1 = fork();		// Storest the process id for left half
	if(pid1 < 0) {
		perror("fork");
		return;
	}
	if(pid1 == 0) {
		concurrent_mergesort(arr,start,mid);
		exit(0);
	}
	else {
		int pid2 = fork();	// Stores the process id for right half
		if(pid2 < 0) {
			perror("fork");
			return;
		}
		if(pid2 == 0) {
			concurrent_mergesort(arr,mid + 1,end);
			exit(0);
		}
		else {
			int stat1,stat2;
			waitpid(pid1,&stat1,0);			// Wait for both the processes to finish and combine the two sorted halfs
			waitpid(pid2,&stat2,0);
			combine(arr,start,mid,end);
		}
	}
}
typedef struct thread_arg {					// Structure for the input of the threaded mergesort function
	int* arr;
	int start;
	int end;
}s;
void* thread_func(void* arg) {				// Function which uses threaded mergesort to sort the array
	s* args = (s*) arg;
	int *arr = args->arr;
	int start = args->start;
	int end = args->end;
	if(start < end) {
		if(end - start + 1 < 5) {
    		selection_sort(arr,start,end);
    		return NULL;
   		}
		int mid = (start + end) / 2;
		s *arg1 = (s*)malloc(sizeof(s));
		s *arg2 = (s*)malloc(sizeof(s));
		arg1->arr = arr;
		arg1->start = start;
		arg1->end = mid;

		arg2->arr = arr;
		arg2->start = mid + 1;
		arg2->end = end;

		pthread_t tid1,tid2;
		pthread_create(&tid1, NULL, thread_func, (void*)(arg1));	// Create two threads each for a half and in that thread sort the respective halfs
		pthread_create(&tid2, NULL, thread_func, (void*)(arg2));
		pthread_join(tid1, NULL);
		pthread_join(tid2, NULL);									// Wait for both the threads to sort their respective half and then merge them
		combine(arr,start,mid,end);

	}
	return NULL;
}
int main() {
	struct timespec ts;
	int *arr;
	int low,high;
	long double tim,tim1,tim2,tim3;				// Variavles to hold the time taken by each sort
	int n;
	long double st,en;
	scanf("%d",&n);
	arr = malloc(n*sizeof(int));
	int *select_arr = (int*) malloc(n*sizeof(int));
	for(int i = 0 ; i < n ; i++) {
		scanf("%d",&arr[i]);
    	select_arr[i] = arr[i];
    }
	int *temp,*temp2;
	temp2 = (int*)malloc(sizeof(int) * n);			// Storing the original array to be sorted by threaded mergesort
	temp = sharedmemory(sizeof(int) * n);			// Storing the original array to be sorted by concurrent mergesort
	for(int i = 0 ; i < n ; i++) temp[i] = arr[i];
	for(int i = 0 ; i < n ; i++) temp2[i] = arr[i];
  	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
 	st = ts.tv_nsec/(1e9)+ts.tv_sec;
	concurrent_mergesort(temp,0,n - 1);
 	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	en = ts.tv_nsec/(1e9)+ts.tv_sec;
	tim2 = en -st;
	printf("Sorted array: \n");
	    for (int i = 0; i < n; i++)
	    		printf("%d ",temp[i]);
	    puts("");
	printf("Time taken by concurrent mergesort: %0.9Lf\n\n",tim2);
    pthread_t tid;
    s* arg = (s*)malloc(sizeof(s));
    arg->arr = temp2;
    arg->start = 0;
    arg->end = n - 1;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
 	st = ts.tv_nsec/(1e9)+ts.tv_sec;
    pthread_create(&tid,NULL,thread_func, (void*)arg);
    pthread_join(tid,NULL);
 	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	en = ts.tv_nsec/(1e9)+ts.tv_sec;
	tim3 = en - st;
	printf("Sorted array: \n");
	    for (int i = 0; i < n; i++)
	    		printf("%d ",temp2[i]);
	    puts("");
	printf("Time taken by threaded mergesort: %0.9Lf\n\n",tim3);
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
 	st = ts.tv_nsec/(1e9)+ts.tv_sec;
	normal_mergesort(arr,0,n - 1);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);	
	en = ts.tv_nsec/(1e9)+ts.tv_sec;
	tim1 = en -st;
	printf("Sorted array: \n");
	    for (int i = 0; i < n; i++)
	    		printf("%d ",arr[i]);
	    puts("");
	printf("Time taken by normal mergesort: %0.9Lf\n\n",tim1);
	shmdt(temp);
	free(arr);
	free(temp2);
	return 0;
}