#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "client.h"
#include "util.h"

const size_t buf_size = 4096;

const char* register_fifo = "/tmp/server_register_fifo";

int SendToServer(int fd, const char* msg)
{
	size_t write_bytes = write(fd, msg, strlen(msg));
	if (write_bytes < 0)
	{
		perror ("bad write");
		return -1;
	}

	LOG("write %lu bytes\n", write_bytes);

	return 0;
}

int RecieveFile(int rxfd, const char* file_path)
{
    const size_t buf_size = 4096;
    char buf[buf_size];

    int file_fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_fd == -1)
    {
        perror("Failed to open file for writing");
        return -1;
    }

    ssize_t read_bytes = 0;
    while ((read_bytes = read(rxfd, buf, buf_size)) > 0)
    {
        ssize_t written_bytes = 0;
        while (written_bytes < read_bytes)
        {
            ssize_t res = write(file_fd, buf + written_bytes, read_bytes - written_bytes);
            if (res == -1)
            {
                perror("bad write");
                close(file_fd);
                return -1;
            }
            written_bytes += res;
        }
    }

    if (read_bytes == -1)
    {
        perror("bad read");
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

	LOG("register fd: %d\n", register_fd);

	char tx_path[buf_size] = {};
	char rx_path[buf_size] = {};

	while (1)
	{
		char buf[buf_size] = {};
		if (fgets(buf, buf_size, stdin) == NULL)
		{
			perror("Failed to read command");
			return -1;
		}

		buf[strcspn(buf, "\n")] = '\0';

		LOG("scanned msg: %s\n", buf);

		char bufcpy[buf_size] = {};
		strncpy(bufcpy, buf, buf_size);
		char* cmd = strtok(bufcpy, " ");

		LOG("command: %s\n", cmd);

		if (!strncmp(cmd, "REGISTER", buf_size))
		{
			MSG("sending REGISTER command to server\n");

			DO(SendToServer, register_fd, buf);

			strcpy(tx_path, strtok(NULL, " "));
			strcpy(rx_path, strtok(NULL, " "));

			LOG("transit path: %s\n", tx_path);
			LOG("recieve path: %s\n", rx_path);
		}
		else if (!strncmp(cmd, "GET", buf_size))
		{
			MSG("sending GET command to server\n");

			int rxfd = open(rx_path, O_WRONLY);
			if (rxfd == -1)
			{
				perror("bad open");
				return 0;
			}

			DO(SendToServer, rxfd, buf);

			int txfd = open(tx_path, O_RDONLY);
			if (txfd == -1)
			{
				perror("bad open");
				return -1;
			}

			const char* recieved_file = "/tmp/recieved_by_client";
			MSG("recieving file\n");
			RecieveFile(txfd, recieved_file);
		}
		else
		{
			fprintf(stderr, "Invalid command: %s\n", cmd);
		}
	}


    close(register_fd);


	return 0;
}
