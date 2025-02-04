#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "client.h"
#include "util.h"

const char* register_fifo = "/tmp/server_register_fifo";

int SendToServer(int fd, const char* msg)
{
	size_t write_bytes = write(fd, msg, strlen(msg));
	if (write_bytes < 0)
	{
		perror ("bad write");
		return -1;
	}

	printf ("write %lu bytes\n", write_bytes);

	return 0;
}

int receive_file(int rxfd, const char* file_path)
{
    const size_t buf_size = 4096;
    char buf[buf_size];

    int file_fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_fd == -1)
    {
        perror("Failed to open file for writing");
        return -1;
    }

    ssize_t read_bytes;
    while ((read_bytes = read(rxfd, buf, buf_size)) > 0)
    {
        ssize_t written_bytes = 0;
        while (written_bytes < read_bytes)
        {
            ssize_t res = write(file_fd, buf + written_bytes, read_bytes - written_bytes);
            if (res == -1)
            {
                perror("Failed to write to file");
                close(file_fd);
                return -1;
            }
            written_bytes += res;
        }
    }

    if (read_bytes == -1)
    {
        perror("Failed to read from FIFO");
        close(file_fd);
        return -1;
    }

    close(file_fd);
    return 0;
}

int run_client()
{
	int register_fd = open(register_fifo, O_WRONLY);

    if (register_fd == -1)
	{
        perror("bad register open");
        return 1;
    }

	printf("register fd: %d\n", register_fd);

	const char* tx_path = "/tmp/tx_1";
	const char* rx_path = "/tmp/rx_1";
    const char* register_cmd = "REGISTER /tmp/tx_1 /tmp/rx_1";

	DO(SendToServer, register_fd, register_cmd);

	sleep(1);

	int rxfd = open(rx_path, O_WRONLY);
	if (rxfd == -1)
	{
        perror("bad register open");
        return 1;
    }

	const char* get_cmd = "GET /tmp/file_1";

	DO(SendToServer, rxfd, get_cmd);

	int txfd = open(tx_path, O_RDONLY);

	const char* recieved_file = "/tmp/recieved_by_client";
	receive_file(txfd, recieved_file);

    close(register_fd);
    close(rxfd);


	return 0;
}
