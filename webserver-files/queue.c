#include "queue.h"


Node* node_create(Stat* request)
{
	Node* new_node = malloc(sizeof(*new_node));
	if (new_node == NULL)
	{
		return NULL;
	}
	new_node->request = request;
	/*new_node->val = val;
  	gettimeofday(&(new_node->current_time), NULL);
	new_node->next = NULL;
	new_node->prev = NULL;*/
	return new_node;
}

void free_node(Node* to_delete)
{
	if (to_delete == NULL)
	{
		return;
	}
	free(to_delete->request->arrival_time);
	free(to_delete->request);
	free(to_delete);
}

Queue* queue_create ()
{
	Queue* qu = malloc(sizeof(*qu));
	if (qu == NULL)
	{
		return NULL;
	}
	qu->size = 0;
	qu->head = NULL;
	qu->tail = NULL;
	return qu;
}

void queue_remove_node (Queue* qu, Node* node, bool to_close)
{
	if (qu == NULL || node == NULL)
	{
		return;
	}
	if (node == qu->head)
	{
		qu->head = node->next;
	}
	if (node == qu->tail)
	{
		qu->tail = node->prev;
	}
	if (node->prev != NULL)
	{
		Node* prev = node->prev;
		prev->next = node->next;
	}
	if (node->next != NULL)
	{
		Node* next = node->next;
		next->prev = node->prev;
	}
	qu->size--;
	//printf("queue size (r) - %d\n", qu->size);
	if (to_close)
	{
		close(node->request->connfd);
	}
	free_node(node);
}

int queue_get_size (Queue* qu)
{
	if (qu == NULL)
	{
		return -1;
	}
	return qu->size;
}

void queue_destroy(Queue* qu)
{
	if (qu == NULL)
	{
		return;
	}
	queue_clear(qu);
	free(qu);
}

void queue_clear (Queue* qu)
{
	while (!queue_is_empty(qu))
	{
		queue_pop(qu, true);
	}
}

bool queue_is_empty (Queue* qu)
{
	if (qu == NULL)
	{
		return true;
	}
	return (qu->size == 0);
}

bool queue_push_back (Queue* qu, Stat* request)
{
	if (qu == NULL)
	{
		return false;
	}
	Node* to_add = node_create(request);
	if (to_add == NULL)
	{
		return false;
	}
	if (queue_is_empty(qu))
	{
		qu->head = to_add;
		qu->tail = to_add;
		qu->size = 1;
	}
	else
	{
		Node* old_tail = qu->tail;
		qu->tail = to_add;
		old_tail->next = to_add;
		to_add->prev = old_tail;
		qu->size++;
	}
	//printf("queue size (add) - %d\n", qu->size);
	return true;
}

void queue_front  (Queue* qu, Stat* request)
{
	if (qu == NULL || qu->size == 0 || request == NULL)
	{
		return;
	}
	request->connfd = qu->head->request->connfd;
	request->arrive_time->tv_sec = qu->head->request->arrival_time->tv_sec;
	request->arrive_time->tv_usec = qu->head->request->arrival_time->tv_usec;
	/**val = qu->head->val;
	arrive_time->tv_sec = qu->head->current_time.tv_sec;
	arrive_time->tv_usec = qu->head->current_time.tv_usec;*/
}

void queue_pop(Queue* qu, bool to_close)
{
	if (qu == NULL || qu->size == 0)
	{
		return ;
	}
	queue_remove_node(qu, qu->head, to_close);
}

void queue_pop_back (Queue* qu, bool to_close)
{
	if (qu == NULL || qu->size == 0)
	{
		return ;
	}
	queue_remove_node(qu, qu->tail, to_close);
}

void queue_drop_random (Queue* qu, int amount_to_drop)
{
	if (qu == NULL)
	{
		return;
	}
	for (int i = 0; i < amount_to_drop && !(queue_is_empty(qu)); i++)
	{
		int to_drop = rand() % qu->size;
		Node* temp = qu->head; 
		for (int j = 0; temp != NULL && j < to_drop; j++, temp = temp->next)
		{;	}
		queue_remove_node(qu, temp, true);
	}
}
