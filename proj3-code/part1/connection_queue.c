#include "connection_queue.h"

#include <stdio.h>
#include <string.h>

int connection_queue_init(connection_queue_t *queue)
{
    // TODO Not yet implemented

    queue->length = 0;
    queue->read_idx = 0;
    queue->write_idx = 0;
    queue->shutdown = 0;

    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->full_queue, NULL);
    pthread_cond_init(&queue->empty_queue, NULL);

    return 0;
}

int connection_queue_enqueue(connection_queue_t *queue, int connection_fd)
{
    // TODO Not yet implemented

    pthread_mutex_lock(&queue->mutex);

    while (queue->length == CAPACITY)
    {
        if (queue->shutdown == -1)
        {
            pthread_mutex_unlock(&queue->mutex);
            return -1;
        }
        pthread_cond_wait(&queue->full_queue, &queue->mutex);
    }

    if (queue->shutdown == -1)
    {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    queue->client_fds[queue->write_idx] = connection_fd;

    queue->length += 1;
    queue->write_idx = (queue->write_idx + 1) % CAPACITY;

    pthread_cond_signal(&queue->empty_queue);
    pthread_mutex_unlock(&queue->mutex);

    return 0;
}

int connection_queue_dequeue(connection_queue_t *queue)
{
    // TODO Not yet implemented

    pthread_mutex_lock(&queue->mutex);

    while (queue->length == 0)
    {
        if (queue->shutdown == -1)
        {
            pthread_mutex_unlock(&queue->mutex);
            return -1;
        }
        pthread_cond_wait(&queue->empty_queue, &queue->mutex);
    }

    int fd = queue->client_fds[queue->read_idx];
    queue->client_fds[queue->read_idx] = -1;
    queue->length -= 1;
    queue->read_idx = (queue->read_idx + 1) % CAPACITY;

    pthread_cond_signal(&queue->full_queue);
    pthread_mutex_unlock(&queue->mutex);

    return fd;
}

int connection_queue_shutdown(connection_queue_t *queue)
{
    // TODO Not yet implemented
    return 0;
}

int connection_queue_free(connection_queue_t *queue)
{
    // TODO Not yet implemented
    return 0;
}
