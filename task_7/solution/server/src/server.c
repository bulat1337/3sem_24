#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>

#include "server.h"
#include "util.h"

typedef struct
{
	int txfd;
	int rxfd;
} Client;

const char* register_fifo = "/tmp/server_register_fifo";
const size_t buf_size = 4096;
const size_t client_amount = 5;

Client HandleRegister(int register_fd)
{
	MSG("Handling register\n");

	Client client = { .rxfd = -1, .txfd = -1};

	char buf[buf_size] = {};
	int read_bytes = read(register_fd, buf, buf_size);
	if (read_bytes < 0)
	{
		perror("bad_read");
		return client;
	}
	LOG("read %d bytes\n", read_bytes);

	buf[buf_size - 1] = '\0';

	LOG("Recieved from register FIFO:\n"
		"%s\n", buf);

	char* command = strtok(buf, " ");
	if (!command)
	{
		perror("bad strtok");
		return client;
	}

	LOG("First token: %s\n", command);
	if (strncmp(command, "REGISTER", sizeof("REGISTER")))
		return client;

	char* tx_path = strtok(NULL, " ");
	if (!tx_path)
	{
		perror("bad strtok");
		return client;
	}

	LOG("tx_path: %s\n", tx_path);

	unlink(tx_path);
	if (mkfifo(tx_path, 0666) == -1)
	{
		perror("bad mkfifo");
		return client;
	}
	client.txfd = open(tx_path, O_RDWR);

	char* rx_path = strtok(NULL, " ");
	if (!rx_path)
	{
		perror("bad strtok");
		return client;
	}

	LOG("rx_path: %s\n", rx_path);
	unlink(rx_path);
	if (mkfifo(rx_path, 0666) == -1)
	{
		perror("bad mkfifo");
		return client;
	}
	client.rxfd = open(rx_path, O_RDWR);

	LOG("New clinets rxfd: %d\n", client.rxfd);

	return client;
}

int SendFile(int txfd, const char* file_path)
{
    const size_t buf_size = 4096;
    char buf[buf_size];

    int file_fd = open(file_path, O_RDONLY);
    if (file_fd == -1)
    {
        perror("bad open");
        return -1;
    }

    ssize_t read_bytes;
    while ((read_bytes = read(file_fd, buf, buf_size)) > 0)
    {
        ssize_t written_bytes = 0;
        while (written_bytes < read_bytes)
        {
            ssize_t res = write(txfd, buf + written_bytes, read_bytes - written_bytes);
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

void* HandleCommand(void *arg)
{
	Client* client = (Client*)arg;

	char buf[buf_size] = {};
	int read_bytes = read(client->rxfd, buf, buf_size);
	LOG("read %d bytes\n", read_bytes);

	buf[buf_size - 1] = '\0';

	LOG("Read buffer: %s\n", buf);

	char* command = strtok(buf, " ");
	if (!command)
	{
		perror("bad strtok");
		return NULL;
	}

	LOG("command: %s\n", command);
	if (strncmp(command, "GET", sizeof("GET")))
	{
		perror("bad strncmp");
		return NULL;
	}

	char* file_path = strtok(NULL, " ");
	if(!file_path)
	{
		perror("bad strtok");
		return NULL;
	}

	LOG("Sending file: %s\n", file_path);

	SendFile(client->txfd, file_path);

	return NULL;
}

int run_server()
{
	/// read descriptors
	fd_set rfds;
	FD_ZERO(&rfds);

	/// rimeout
	struct timeval tv;
	tv.tv_sec = 100;
	tv.tv_usec = 0;

	/// register FIFO logic
	unlink(register_fifo);
	DO(mkfifo, register_fifo, 0666);
	int register_fd = open(register_fifo, O_RDWR | O_NONBLOCK);

	if (register_fd == -1)
	{
		perror("bad register open");
		return 0;
	}
	LOG("register_fd: %d\n", register_fd);

	Client clients[client_amount] = {};
	size_t client_amount = 0;

	while (1)
	{
		int max_fd = register_fd;
		MSG("Setting register fd\n");
		FD_SET(register_fd, &rfds);

		for (size_t cl_id = 0; cl_id < client_amount; ++cl_id)
		{
			LOG("setting %d in rfds\n", clients[cl_id].rxfd);
			FD_SET(clients[cl_id].rxfd, &rfds);

			if (clients[cl_id].rxfd > max_fd)
				max_fd = clients[cl_id].rxfd;
		}

		MSG("running select\n");
		int status = select(max_fd + 1, &rfds, NULL, NULL, &tv);

		if (status > 0)
		{
			MSG("One of the fds is triggered\n");

			if (FD_ISSET(register_fd, &rfds))
			{
				clients[client_amount] = HandleRegister(register_fd);
				if (clients[client_amount].txfd == -1 || clients[client_amount].rxfd == -1)
				{
					perror("bad register");
					return 0;
				}

				++client_amount;

				continue;
			}

			for (size_t cl_id = 0; cl_id < client_amount; ++cl_id)
			{
				if (FD_ISSET(clients[cl_id].rxfd, &rfds))
				{
					LOG("Recieved command from %lu client\n", cl_id);

					pthread_t thread;
					pthread_create(&thread, NULL, HandleCommand, &clients[cl_id]);
					pthread_detach(thread);

					// HandleCommand(clients[cl_id]);

					break;
				}
			}
		}
		else if (status == -1)
		{
			perror("bad select");
		}
		else
		{
			MSG("Timeout!\n");
		}

	}

	unlink(register_fifo);
	close(register_fd);

	return 0;
}
