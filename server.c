#include "segel.h"
#include "request.h"
//#include <pthread.h>
#include "queue.h"
#include "stats.h"
#include <math.h>
// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

pthread_mutex_t lock_queue;
pthread_cond_t  c_busy;
pthread_cond_t  c_free;
Queue* requests_pending;
int requests_handled;

//pending + active <= queue_size
// HW3: Parse the new arguments too
void getargs(int *port, int argc, char *argv[], int *threads, int* queue_size, char * schedalg)
{
    if (argc < 2 || argc > 5) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    //argv[0] = ./server , argv[1] = portnum,  argv[2] = threads, argv[3] = queue size , argv[4] = schedalg
    *threads = atoi(argv[2]);
    *queue_size = atoi(argv[3]);
    strcpy(schedalg, argv[4]);
}

void* thread_request_handler(void* index)
{
	Stats* thread_stats = malloc(sizeof(*thread_stats));
	if (thread_stats == NULL)
	{ 
		exit(1);
	}
	thread_stats->thread_id = *((int*)index);
	thread_stats->static_req = 0;
	thread_stats->dynamic_req = 0;
	thread_stats->total_req = 0;
	while (1)
	{
		pthread_mutex_lock(&lock_queue);
		while (queue_is_empty(requests_pending)) 
		{
			//printf(" thread number %d going to wait (there are %d reqs waiting and handleing - %d) \n",
			// *((int*)index), queue_get_size(requests_pending), requests_handled);
			pthread_cond_wait(&c_free, &lock_queue);
		}
		//printf(" thread number %d got request\n", *((int*)index));
		int fd_to_handle;  
		queue_front(requests_pending, &fd_to_handle, &thread_stats->arrival_time);
		queue_pop(requests_pending, false);
		requests_handled++;
		
		pthread_mutex_unlock(&lock_queue);

		thread_stats->total_req++;
		struct timeval start_time;
		gettimeofday(&start_time, NULL);
		timersub(&start_time, &(thread_stats->arrival_time), &(thread_stats->wait_time));
		requestHandle(fd_to_handle, thread_stats);
		//printf("after handling the request by thread number %d\n", *((int*)index));
		Close(fd_to_handle);

		pthread_mutex_lock(&lock_queue);
		requests_handled--;
		pthread_cond_signal(&c_busy);
		pthread_mutex_unlock(&lock_queue);
	}
	return NULL;
}

void overload_handler(char* schedalg, int queue_size)
{
	if (schedalg == NULL )
	{
		return;
	}
	if (strcmp(schedalg, "block") == 0)
	{
		while (queue_get_size(requests_pending) + requests_handled >= queue_size)
		{
			pthread_cond_wait(&c_busy, &lock_queue);
		}
	}
	/*else if (strcmp(schedalg, "dt") == 0)
	{
		queue_pop_back(requests_pending, true);
	}*/
	else if (strcmp(schedalg, "random") == 0)
	{
		double calc_to_drop = 0.3 *queue_get_size(requests_pending);
		int amount_to_drop = (int)(ceil(calc_to_drop));
		queue_drop_random(requests_pending, amount_to_drop);
	}
	else if (strcmp(schedalg, "dh") == 0)
	{
		//printf("I am full\n\n");
		queue_pop(requests_pending, true);
	}
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

	int thread_num, queue_size;
	char schedalg [7];
    getargs(&port, argc, argv, &thread_num, &queue_size, schedalg);

    // 
    // HW3: Create some threads...
    //
    if (pthread_mutex_init(&lock_queue, NULL) != 0)
    {
		printf("something went wrong\n");
	}
	pthread_cond_init(&c_busy, NULL);
	pthread_cond_init(&c_free, NULL);
	requests_pending = queue_create();
	requests_handled = 0;
	if ( requests_pending == NULL)
	{
		printf("something went wrong\n");
	}
	int* thread_idx = (int*)malloc(sizeof(int)*thread_num);
    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t)*thread_num);
    if (threads == NULL || thread_idx == NULL)
    {
		printf("something went wrong\n");
	}
	int i = 0;
    for (; i < thread_num; i++)
    {
		thread_idx[i] = i;
		pthread_create(&threads[i], NULL, thread_request_handler, (void*)(thread_idx + i));
	}

    listenfd = Open_listenfd(port);
    while(1) {
		clientlen = sizeof(clientaddr);
		/////////
		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
		if (connfd == -1)
		{
			//printf("byebyebye\n");
			Close(listenfd);
			exit(1);
		}
		//////////
		////// my addition:
		
		pthread_mutex_lock(&lock_queue);
		//printf("worker thread locked\n");
		if (requests_handled == queue_size && strcmp(schedalg, "block") != 0)
		{
			Close(connfd);
			pthread_mutex_unlock(&lock_queue);
			continue;
		}
		else if (queue_get_size(requests_pending) + requests_handled >= queue_size)
		{
			if(strcmp(schedalg, "dt") == 0)
			{
				Close(connfd);
				pthread_mutex_unlock(&lock_queue);
				continue;
			}
			overload_handler(schedalg, queue_size);
		}
		if (!queue_push_back(requests_pending, connfd))
		{
			//printf("something went wrong\n");
		}
		if (queue_get_size(requests_pending))
		{
			pthread_cond_signal(&c_free);
		}
		//printf("worker thread unlocked\n");
		pthread_mutex_unlock(&lock_queue);
		
	}
	exit(1);
	//////////
	// 
	// HW3: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	// 
	//requestHandle(connfd);

	//Close(connfd);
}




    


 
