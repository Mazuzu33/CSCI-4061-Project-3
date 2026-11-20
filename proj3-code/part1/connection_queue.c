#include "connection_queue.h"

#include <stdio.h>
#include <string.h>

int connection_queue_init(connection_queue_t *queue)
{
    // TODO Not yet implemented
    return 0;
}

int connection_queue_enqueue(connection_queue_t *queue, int connection_fd)
{
    // TODO Not yet implemented
    return 0;
}

int connection_queue_dequeue(connection_queue_t *queue)
{
    // TODO Not yet implemented
    return 0;
}

int connection_queue_shutdown(connection_queue_t *queue)
{
    // TODO Not yet implemented
    int status;
    if ((status = pthread_mutex_lock(&queue->mutex)) == -1)
    {
        fprintf(stderr, "Failed to acquire lock");
        return -1;
    }
    queue->shutdown = 1;
    if ((status = pthread_cond_broadcast(&queue->full_queue)) == -1)
    {
        fprintf(stderr, "Failed to singal full_queue condition variable: %s", strerror(status));
        return -1;
    }
    if ((status = pthread_cond_broadcast(&queue->empty_queue)) == -1)
    {
        fprintf(stderr, "Failed to signal empty_queue condition variable: %s", strerror(status));
        return -1;
    }
    if ((status = pthread_mutex_unlock(&queue->mutex)) == -1)
    {
        fprintf(stderr, "Failed to release lock");
        return -1;
    }
    return 0;
}

int connection_queue_free(connection_queue_t *queue)
{
    // TODO Not yet implemented
    int status;
    if ((status = pthread_mutex_lock(&queue->mutex)) == -1)
    {
        fprintf(stderr, "Failed to acquire lock");
        return -1;
    }
    if ((status = pthread_mutex_destroy(&queue->mutex)) != 0)
    {
        fprintf(stderr, "Failed to destroy mutex: %s", strerror(status));
        pthread_cond_destroy(&queue->full_queue);
        pthread_cond_destroy(&queue->empty_queue);
        return -1;
    }
    if ((status = pthread_cond_destroy(&queue->full_queue)) != 0)
    {
        fprintf(stderr, "Failed to destroy condition variable: %s", strerror(status));
        pthread_cond_destroy(&queue->empty_queue);
        return -1;
    }
    if ((status = pthread_cond_destroy(&queue->empty_queue)) != 0)
    {
        fprintf(stderr, "Failed to destroy condition variable: %s", strerror(status));
        return -1;
    }
    if ((status = pthread_mutex_unlock(&queue->mutex)) == -1)
    {
        fprintf(stderr, "Failed to release lock");
        return -1;
    }
    return 0;
}
