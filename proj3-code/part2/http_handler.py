import socket
from pathlib import Path

MIME_TYPES = {
    ".txt": "text/plain",
    ".html": "text/html",
    ".jpg": "text/jpeg",
    ".png": "image/png",
    ".pdf": "application/pdf",
    ".mp3": "audio/mpeg",
}


# Read in an HTTP request from a socket
# 'conn': The socket to read from
# Returns the name of the requested resource
def read_request(conn: socket.socket) -> str:
    # Treat the socket as a file object
    with conn.makefile("r", encoding="utf-8", newline="\r\n") as f:
        # Read the first line and split it by spaces
        first_line = f.readline()
        first_line_list = first_line.split(" ")
        # Read other lines until blank line reached
        for line in f:
            if line.strip() == "":
                break
    # Return resource requested
    return first_line_list[1]


# Write an HTTP response to a socket
# 'conn': The socket to write the request to
# 'file_path': A complete path (e.g., "server_files/quote.txt") for the file to
#   send in the response. Send a 404 response if this file does not exist.
def write_response(conn: socket.socket, file_path: str) -> None:
    # Check if a file path is valid
    file = Path(file_path)
    if file.is_file():
        # Get file's size
        file_size = file.stat().st_size
        file_chunks = file_path.split(".")
        file_extension = "." + file_chunks[-1]
        file_type = MIME_TYPES[file_extension]
        # Send a found HTTP response
        response = f"HTTP/1.0 200 OK\r\nContent-Type: {file_type}\r\nContent-Length: {file_size}\r\n\r\n"
        conn.sendall(response.encode())
        # Send file over socket
        with open(file, "rb") as f:
            conn.sendfile(f)
    # Send a not found HTTP response
    else:
        response = "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n"
        conn.sendall(response.encode())
