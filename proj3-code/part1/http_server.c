#define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "connection_queue.h"
#include "http.h"

#define N_THREADS 5
#define PATHSIZE 512

int keep_going = 1;
const char *serve_dir;

// Worker function for each thread
void *worker(void *arg)
{
    while (1)
    {
        // printf("New client connected\n"); Delete later
        // Set up strings to hold file name and path
        char resource_name[PATHSIZE];
        char resource_path[PATHSIZE];
        strncpy(resource_path, serve_dir, PATHSIZE);
        // Dequeue client fd
        int client_fd;
        if ((client_fd = connection_queue_dequeue(arg)) == -1)
        {
            break;
        }
        // If an error happens, close the client connection but keep reading requests from other clients
        if (read_http_request(client_fd, resource_name) == -1)
        {
            // printf("Error reading http request, client disconnected\n"); Delete later
            close(client_fd);
            continue;
        }
        // printf("Received http request for: %s\n", resource_name); Delete later
        // Convert the requested resource name to a proper file path
        strncat(resource_path, resource_name, PATHSIZE - strlen(resource_path) - 1);
        // If an error happens, close the client connection but keep reading requests from other clients
        if (write_http_response(client_fd, resource_path) == -1)
        {
            // printf("Error writing http response, client disconnected\n"); Delete later
            close(client_fd);
            continue;
        }
        // printf("Client disconnected\n"); Delete later
        // Try to close client fd
        if (close(client_fd) == -1)
        {
            perror("close");
            return NULL;
        }
    }
    return NULL;
}

void handle_sigint(int signo)
{
    keep_going = 0;
}

// args in argv:
// 1. itself
// 2. directory
// 3. port
int main(int argc, char **argv)
{
    // First argument is directory to serve, second is port
    if (argc != 3)
    {
        printf("Usage: %s <directory> <port>\n", argv[0]);
        return 1;
    }
    // Uncomment the lines below to use these definitions
    serve_dir = argv[1];
    const char *port = argv[2];

    // Initialize the connection queue
    connection_queue_t queue;
    if (connection_queue_init(&queue) == -1)
    {
        return 1;
    }
    // Set up signal handler for SIGINT
    struct sigaction sigact;
    sigact.sa_handler = handle_sigint;
    sigfillset(&sigact.sa_mask);
    // Don't restart system calls
    sigact.sa_flags = 0;
    if (sigaction(SIGINT, &sigact, NULL) == -1)
    {
        perror("sigaction");
        return 1;
    }
    // Block all possible signals
    sigset_t new_mask;
    sigfillset(&new_mask);
    sigset_t old_mask;
    sigprocmask(SIG_SETMASK, &new_mask, &old_mask);
    // Create thread pool
    pthread_t tids[N_THREADS];
    int status;
    for (int i = 0; i < N_THREADS; i++)
    {
        if ((status = pthread_create(tids[i], NULL, worker, &queue)) != 0)
        {
            fprintf(stderr, "Failed to create thread: %s\n", strerror(status));
            for (int j = 0; j < i; j++)
            {
                pthread_join(tids[j], NULL);
            }
            return 1;
        }
    }
    // Restore old signal mask
    sigprocmask(SIG_SETMASK, &old_mask, NULL);
    // Set up hints - take IPv4 or IPv6, TCP socket type, and act as a server
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *server;

    // Set up address info for socket() and connect()
    int ret_val = getaddrinfo(NULL, port, &hints, &server);
    if (ret_val != 0)
    {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(ret_val));
        for (int i = 0; i < N_THREADS; i++)
        {
            pthread_join(tids[i], NULL);
        }
        return 1;
    }

    // Initialize socket file descriptor
    int sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if (sock_fd == -1)
    {
        perror("socket");
        freeaddrinfo(server);
        for (int i = 0; i < N_THREADS; i++)
        {
            pthread_join(tids[i], NULL);
        }
        return 1;
    }

    // Bind socket to receive at a specific port
    if (bind(sock_fd, server->ai_addr, server->ai_addrlen) == -1)
    {
        perror("bind");
        freeaddrinfo(server);
        close(sock_fd);
        for (int i = 0; i < N_THREADS; i++)
        {
            pthread_join(tids[i], NULL);
        }
        return 1;
    }

    freeaddrinfo(server);

    // Designate socket as a server socket
    if (listen(sock_fd, CAPACITY) == -1)
    {
        perror("listen");
        close(sock_fd);
        for (int i = 0; i < N_THREADS; i++)
        {
            pthread_join(tids[i], NULL);
        }
        return 1;
    }

    // Main loop to accept client connections
    while (keep_going != 0)
    {
        // Wait to receive a connection request from client
        // printf("Waiting for client to connect\n"); Delete later
        int client_fd = accept(sock_fd, NULL, NULL);
        if (client_fd == -1)
        {
            // If a error happened
            if (errno != EINTR)
            {
                perror("accept");
                close(sock_fd);
                for (int i = 0; i < N_THREADS; i++)
                {
                    pthread_join(tids[i], NULL);
                }
                return 1;
            }
            // If a SIGINT was recieved, break out of loop to clean up
            else
            {
                break;
            }
        }
        // Add client fd to queue
        if (connection_queue_enqueue(&queue, client_fd) == -1)
        {
            break;
        }
    }

    // Shutdown queue
    if (connection_queue_shutdown(&queue) == -1)
    {
        fprintf(stderr, "Failed to shutdown queue\n", NULL);
        close(sock_fd);
        for (int i = 0; i < N_THREADS; i++)
        {
            pthread_join(tids[i], NULL);
        }
        return 1;
    }

    // Close connection socket
    if (close(sock_fd) == -1)
    {
        perror("close");
        return 1;
    }

    // Join threads
    for (int i = 0; i < N_THREADS; i++)
    {
        if ((status = pthread_join(tids[0], NULL)) != 0)
        {
            fprintf(stderr, "Failed to join thread: %s\n", strerror(status));
            for (int j = i + 1; j < N_THREADS; j++)
            {
                pthread_join(tids[j], NULL);
            }
            return 1;
        }
    }
    return 0;
}
