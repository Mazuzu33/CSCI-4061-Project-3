import collections
import socket
import threading


# Raised by enqueue/dequeue options when the queue has been shut down
class ShutDownException(Exception):
    pass


# A thread-safe, bounded-size queue
class ConnectionQueue:
    # Initialize a new queue and any underlying synchronization primitives
    def __init__(self, capacity: int) -> None:
        self.elements = collections.deque()
        self.shutdown = False
        self.capacity = capacity
        self.lock = threading.Lock()
        self.cv_full = threading.Condition(lock=self.lock)
        self.cv_empty = threading.Condition(lock=self.lock)
        # TODO Add the necessary thread synchronization primitives

    # Add a new socket to the queue
    # This should block the calling thread if the number of elements in the
    # queue has already reached its capacity and only proceed once a slot has
    # opened up.
    # When a new item is successfully enqueued, one thread blocking on a dequeue
    # should be unblocked (if such a thread exists).
    # Raises ShutDownException if the queue was shut down while the calling thread
    # was in the middle of this operation.
    def enqueue(self, conn: socket.socket) -> None:
        with self.lock:
            # While the queue is full and we are not shutting down, wait for the queue to not be full
            while len(self.elements) == self.capacity and self.shutdown == False:
                self.cv_full.wait()
            # If we are shutting down
            if self.shutdown:
                raise ShutDownException
            # Proceed with the enqueue operation
            self.elements.append(conn)
            # Notify that the queue is not empty anymore
            self.cv_empty.notify()

    # Remove a socket from the queue
    # This should block the calling thread if the queue is empty. The thread
    # will only proceed once it can successfully retrieve an item from the
    # queue.
    # When an item is successfully removed from the queue, one thread blocking
    # on an enqueue should be unblocked (if such a thread exists)
    # Raises ShutDownException if the queue was shut down while the calling thread
    # was in the middle of this operation.
    def dequeue(self) -> socket.socket:
        with self.lock:
            # While the queue is empty and we are not shutting down, wait for the queue to not be empty
            while len(self.elements) == 0 and self.shutdown == False:
                self.cv_empty.wait()
            # If we are shutting down and the queue is empty
            if self.shutdown == True and len(self.elements) == 0:
                raise ShutDownException
            # Proceed with the dequeue operation
            sock = self.elements.popleft()
            # Notifay that the queue is not full anymore
            self.cv_full.notify()
            return sock

    # Shut down the queue
    # All threads blocking on either an enqueue or dequeue are unblocked and
    # will observe a ShutDownException.
    def shut_down(self) -> None:
        with self.lock:
            # Indicate we are shutting down and signal all condition variables
            self.shutdown = True
            self.cv_empty.notify_all()
            self.cv_full.notify_all()
