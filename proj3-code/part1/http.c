#include "http.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFSIZE 512

const char *get_mime_type(const char *file_extension)
{
    if (strcmp(".txt", file_extension) == 0)
    {
        return "text/plain";
    }
    else if (strcmp(".html", file_extension) == 0)
    {
        return "text/html";
    }
    else if (strcmp(".jpg", file_extension) == 0)
    {
        return "image/jpeg";
    }
    else if (strcmp(".png", file_extension) == 0)
    {
        return "image/png";
    }
    else if (strcmp(".pdf", file_extension) == 0)
    {
        return "application/pdf";
    }
    else if (strcmp(".mp3", file_extension) == 0)
    {
        return "audio/mpeg";
    }

    return NULL;
}

// dont forget to close fd if there is an error
int read_http_request(int fd, char *resource_name)
{
    // TODO Not yet implemented

    char buf[BUFSIZE];

    ssize_t bytes;
    ssize_t total_bytes = 0;
    char *eof = NULL;

    // part that reads from fd
    while (eof == NULL)
    {
        if (total_bytes == BUFSIZE - 1)
        {
            close(fd);
            return -1;
        }

        bytes = read(fd, &buf[total_bytes], BUFSIZE - total_bytes - 1);

        // closes if it reads to the end but cannot find ending sequence
        if (bytes == 0)
        {
            close(fd);
            return -1;
        }
        else if (bytes == -1) // closes if read returns an error
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                close(fd);
                return -1;
            }
        }
        else // read successfully reads ig
        {
            total_bytes += bytes;

            if (total_bytes <= BUFSIZE - 1)
            {
                buf[total_bytes] = '\0';
                eof = strstr(buf, "\r\n\r\n");
            }
        }
    }

    char *saveptr_header;
    char *saveptr_name;

    // separate the first line
    char *tokens = strtok_r(buf, "\r\n", &saveptr_header);

    // separate GET
    tokens = strtok_r(tokens, " ", &saveptr_name);
    // separate name
    tokens = strtok_r(NULL, " ", &saveptr_name);

    strcpy(resource_name, tokens);
    return 0;
}

int write_http_response(int fd, const char *resource_path)
{
    // TODO Not yet implemented

    char path[BUFSIZE];
    int fp_len;
    char header[512];
    struct stat file_info;

    if ((fp_len = snprintf(path, sizeof(header), "server_files%s", resource_path)) == -1 || fp_len >= BUFSIZE)
    {
        strcpy(header, "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n");
        write(fd, header, strlen(header));
        return -1;
    }

    if (stat(path, &file_info) == -1)
    {
        strcpy(header, "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n");
        write(fd, header, strlen(header));
        return -1;
    }

    char *type = get_mime_type(path);

    if ((fp_len = snprintf(header, sizeof(header), "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %jd\r\n\r\n", type, (__intmax_t)(file_info.st_size))) == -1 || fp_len >= BUFSIZE)
    {
        return -1;
    }

    size_t total_written = 0;

    while (total_written < strlen(header))
    {
        int temp = write(fd, header + total_written, strlen(header) - total_written);
        if (temp == -1)
        {
            return -1;
        }
        else
        {
            total_written += temp;
        }
    }

    int fd_r = open(path, O_RDONLY);
    if (fd_r == -1)
    {
        return -1;
    }

    char file[4096];
    int bytes_read = 0;

    while ((bytes_read = read(fd_r, file, 4096)) > 0)
    {
        int temp = 0;
        int wr;
        while ((wr = write(fd, file + temp, bytes_read)) > 0)
        {
            temp += wr;
            bytes_read -= wr;
        }

        if (wr == -1)
        {
            return -1;
        }
    }

    if (bytes_read == -1)
    {
        return -1;
    }

    close(fd_r);

    return 0;
}
