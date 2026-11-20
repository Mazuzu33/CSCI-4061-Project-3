import signal
import socket
import sys
import threading

import http_handler
from connection_queue import ConnectionQueue, ShutDownException

QUEUE_SIZE = 5
N_WORKERS = 5
keep_going = True

def worker(queue, serve_dir):
    while (True):
        # Dequeue client and read HTTP request and write HTTP response
        try:
            client = queue.dequeue()
        except ShutDownException:
            break
        with client:
            resource_name = read_request(client)
            resource_path = serve_dir + resource_name
            write_response(client, resource_path)



def handle_sigint(_signo, _frame):
    global keep_going
    keep_going = False


def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <directory> <port>")
        sys.exit(1)
    serve_dir = sys.argv[1]
    try:
        port = int(sys.argv[2])
    except ValueError:
        print(f"Invalid port number: {sys.argv[2]}")
        sys.exit(1)

    # TODO Complete the implementation of the rest of this function
    # Create a new connection queue
    queue = ConnectionQueue(N_WORKERS)
    # Set up signal handler
    signal.signal(signal.SIGINT, handle_sigint)
    # Set up thread pool
    threads = []
    for i in range(NUM_THREADS):
        t = threading.Thread(target=worker, args=(queue, serve_dir))
        t.start()
        threads.append(t)
    # Create a TCP socket, binds it, and puts it into listening mode
    with server = socket.create_server(("", port)):
        # Wait 1.0 seconds for a new client to connect
        server.settimeout(1.0)
        while keep_going:
            try:
                conn, _ = server.accept()
            except TimeoutError:
                continue
            try:
                queue.enqueue(conn)
            except ShutDownException:
                break
    # Shutdown queue
    queue.shut_down()
    # Join threads
    for t in threads:
        t.join()
    sys.exit(0)


if __name__ == "__main__":
    main()
