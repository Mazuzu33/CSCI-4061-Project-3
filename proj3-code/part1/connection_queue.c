#include "connection_queue.h"

#include <stdio.h>
#include <string.h>

int connection_queue_init(connection_queue_t *queue)
{
    // Initialize queue ints
    queue->length = 0;
    queue->read_idx = 0;
    queue->write_idx = 0;
    queue->shutdown = 0;
    int status;
    // Initialize queue mutexes and condition variables
    if ((status = pthread_mutex_init(&queue->mutex, NULL)) != 0)
    {
        fprintf(stderr, "Failed to initialize lock: %s\n", strerror(status));
        return -1;
    }
    if ((status = pthread_cond_init(&queue->full_queue, NULL)) != 0)
    {
        fprintf(stderr, "Failed to initialize condition variable: %s\n", strerror(status));
        pthread_mutex_destroy(&queue->mutex);
        return -1;
    }
    if ((status = pthread_cond_init(&queue->empty_queue, NULL)) != 0)
    {
        fprintf(stderr, "Failed to initialize condition variable: %s\n", strerror(status));
        pthread_mutex_destroy(&queue->mutex);
        pthread_cond_destroy(&queue->full_queue);
        return -1;
    }
    return 0;
}

int connection_queue_enqueue(connection_queue_t *queue, int connection_fd)
{
    // Acquire lock
    int status;
    if ((status = pthread_mutex_lock(&queue->mutex)) != 0)
    {
        fprintf(stderr, "Failed to acquire lock: %s\n", strerror(status));
        return -1;
    }
    // While the queue is full and we are not shutting down, wait for the queue to not be full
    while (queue->length == CAPACITY && queue->shutdown != 1)
    {
        if ((status = pthread_cond_wait(&queue->full_queue, &queue->mutex)) != 0)
        {
            fprintf(stderr, "Failed to wait on condition: %s\n", strerror(status));
            pthread_mutex_unlock(&queue->mutex);
            return -1;
        }
    }
    // If we are shutting down, unlock and return -1
    if (queue->shutdown == 1)
    {
        if ((status = pthread_mutex_unlock(&queue->mutex)) != 0)
        {
            fprintf(stderr, "Failed to release lock: %s\n", strerror(status));
            return -1;
        }
        return -1;
    }
    // Enqueue operation
    queue->client_fds[queue->write_idx] = connection_fd;

    queue->length += 1;
    queue->write_idx = (queue->write_idx + 1) % CAPACITY;
    // Signal that the queue is not empty
    if ((status = pthread_cond_signal(&queue->empty_queue)) != 0)
    {
        fprintf(stderr, "Failed to signal condition variable: %s\n", strerror(status));
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    // Unlock mutex
    if ((status = pthread_mutex_unlock(&queue->mutex)) != 0)
    {
        fprintf(stderr, "Failed to release lock: %s\n", strerror(status));
        return -1;
    }
    return 0;
}

int connection_queue_dequeue(connection_queue_t *queue)
{
    // Acquire lock
    int status;
    if ((status = pthread_mutex_lock(&queue->mutex)) != 0)
    {
        fprintf(stderr, "Failed to acquire lock: %s\n", strerror(status));
        return -1;
    }
    // While the queue is empty and we are not shutting down, wait for the queue to become nonempty
    while (queue->length == 0 && queue->shutdown != 1)
    {
        if ((status = pthread_cond_wait(&queue->empty_queue, &queue->mutex)) != 0)
        {
            fprintf(stderr, "Failed to wait on condition: %s\n", strerror(status));
            pthread_mutex_unlock(&queue->mutex);
            return -1;
        }
    }
    // If we are shutting down and there are no more elements in the queue, unlock and return -1
    if (queue->shutdown == 1 && queue->length == 0)
    {
        if ((status = pthread_mutex_unlock(&queue->mutex)) != 0)
        {
            fprintf(stderr, "Failed to release lock: %s\n", strerror(status));
            return -1;
        }
        return -1;
    }

    // Dequeue operation
    int fd = queue->client_fds[queue->read_idx];
    queue->client_fds[queue->read_idx] = -1;
    queue->length -= 1;
    queue->read_idx = (queue->read_idx + 1) % CAPACITY;

    // Signal that the queue is not full anymore
    if ((status = pthread_cond_signal(&queue->full_queue)) != 0)
    {
        fprintf(stderr, "Failed to signal condition variable: %s\n", strerror(status));
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    // Unlock mutex
    if ((status = pthread_mutex_unlock(&queue->mutex)) != 0)
    {
        fprintf(stderr, "Failed to release lock: %s\n", strerror(status));
        return -1;
    }

    return fd;
}

int connection_queue_shutdown(connection_queue_t *queue)
{
    // Acquire lock
    int status;
    if ((status = pthread_mutex_lock(&queue->mutex)) != 0)
    {
        fprintf(stderr, "Failed to acquire lock: %s\n", strerror(status));
        return -1;
    }
    // Indicate that we are shutting down
    queue->shutdown = 1;
    // Signal all condition variables
    if ((status = pthread_cond_broadcast(&queue->full_queue)) != 0)
    {
        fprintf(stderr, "Failed to signal full_queue condition variable: %s\n", strerror(status));
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    if ((status = pthread_cond_broadcast(&queue->empty_queue)) != 0)
    {
        fprintf(stderr, "Failed to signal empty_queue condition variable: %s\n", strerror(status));
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    // Unlock mutex
    if ((status = pthread_mutex_unlock(&queue->mutex)) != 0)
    {
        fprintf(stderr, "Failed to release lock: %s\n", strerror(status));
        return -1;
    }
    return 0;
}

int connection_queue_free(connection_queue_t *queue)
{
    int status;
    if ((status = pthread_mutex_destroy(&queue->mutex)) != 0)
    {
        fprintf(stderr, "Failed to destroy mutex: %s\n", strerror(status));
        pthread_cond_destroy(&queue->full_queue);
        pthread_cond_destroy(&queue->empty_queue);
        return -1;
    }
    if ((status = pthread_cond_destroy(&queue->full_queue)) != 0)
    {
        fprintf(stderr, "Failed to destroy condition variable: %s\n", strerror(status));
        pthread_cond_destroy(&queue->empty_queue);
        return -1;
    }
    if ((status = pthread_cond_destroy(&queue->empty_queue)) != 0)
    {
        fprintf(stderr, "Failed to destroy condition variable: %s\n", strerror(status));
        return -1;
    }
    return 0;
}
