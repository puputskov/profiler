#include <profiler/profiler.h>
#include <WinSock2.h>
#include <assert.h>

#define BUFFER_SIZE 1024
#define MAX_THREADS 64

static SOCKET				g_socket							= INVALID_SOCKET;
static struct sockaddr_in	g_addr								= {0};
static LARGE_INTEGER		g_freq								= {0};
static LARGE_INTEGER		g_start_time						= {0};

static uint32_t				g_packet_id							= 0;
static uint32_t				g_thread_ids[MAX_THREADS] 			= {0};
static uint32_t				g_thread_id_cursor					= 0;
static int32_t				g_thread_level_counter[MAX_THREADS]	= {0};


int32_t profiler_init (PROFILER_TYPE type, uint32_t address, uint16_t port)
{
	WSADATA wsa_data = {0};
	assert (WSAStartup (MAKEWORD (2, 2), &wsa_data) == 0);
	
	g_socket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	assert (g_socket != INVALID_SOCKET);
	
	g_addr.sin_family		= AF_INET;
	g_addr.sin_addr.s_addr	= htonl (address);
	g_addr.sin_port			= htons (port);

	if (type == PROFILER_TYPE_SERVER)
	{
		assert (bind (g_socket, (struct sockaddr *) &g_addr, sizeof (struct sockaddr_in)) == 0);
	}

	else if (type == PROFILER_TYPE_PROFILER)
	{
		struct sockaddr_in tmp	= {0};
		tmp.sin_family			= AF_INET;
		tmp.sin_addr.s_addr		= 0;
		tmp.sin_port			= 0;
		assert (bind (g_socket, (struct sockaddr *) &tmp, sizeof (struct sockaddr_in)) == 0);
	}

	int32_t non_blocking = 1;
	assert (ioctlsocket (g_socket, FIONBIO, &non_blocking) == 0);


	if (type == PROFILER_TYPE_PROFILER)
	{
		uint8_t g_buffer_data [BUFFER_SIZE]	= {0};
		profiler_buffer_t g_buffer			= {0};
		profiler_init_buffer (&g_buffer, g_buffer_data, BUFFER_SIZE);

		QueryPerformanceFrequency	(&g_freq);
		QueryPerformanceCounter		(&g_start_time);

		profiler_buffer_write (&g_buffer, &g_packet_id, sizeof (uint32_t));
		profiler_buffer_write (&g_buffer, "init", 4);
		profiler_buffer_write (&g_buffer, &g_freq.QuadPart, sizeof (int64_t));
		profiler_buffer_write (&g_buffer, &g_start_time.QuadPart,sizeof (int64_t));

		int32_t size = (int32_t) sizeof (struct sockaddr);
		assert (sendto (g_socket, g_buffer.data, (int32_t) g_buffer.cursor, 0, (struct sockaddr *) &g_addr, size) != -1);
		++ g_packet_id;
	}
	return (1);
}

int32_t profiler_quit ()
{
	closesocket (g_socket);
	return WSACleanup ();
}


void profiler_send (PROFILER_PACKET_TYPE type, const char *filename, uint32_t filename_size, const char *function, uint32_t function_size, uint32_t line)
{
	uint8_t g_buffer_data [BUFFER_SIZE]	= {0};
	profiler_buffer_t g_buffer			= {0};
	profiler_init_buffer (&g_buffer, g_buffer_data, BUFFER_SIZE);	
	//profiler_buffer_reset (&g_buffer);

	uint32_t thread_id			= GetCurrentThreadId ();
	//DWORD processor_number	= GetCurrentProcessorNumber ();

	LARGE_INTEGER now;
	QueryPerformanceCounter (&now);
	profiler_buffer_write (&g_buffer, &g_packet_id, sizeof (uint32_t));
	int32_t	level = 0;
	uint32_t i;
	for (i = 0; i < g_thread_id_cursor; ++ i)
	{
		if (g_thread_ids [i] == thread_id)
		{
			if (type == PROFILER_PACKET_TYPE_BEGIN)
			{
				g_thread_level_counter [i] ++;
				level = g_thread_level_counter[i];
			}

			if (type == PROFILER_PACKET_TYPE_END)
			{
				level = g_thread_level_counter [i];
				g_thread_level_counter [i] --;

				if (g_thread_level_counter [i] <= 0)
				{
					// Removes the thread id from the array
					g_thread_level_counter [i] = g_thread_level_counter [g_thread_id_cursor - 1];
					g_thread_ids [i] = g_thread_ids [g_thread_id_cursor - 1];
					g_thread_level_counter [g_thread_id_cursor - 1] = 0;
					g_thread_ids [g_thread_id_cursor - 1]  = 0;
				}
			}

			break;
		}
	}

	if (i == g_thread_id_cursor)
	{
		g_thread_ids [g_thread_id_cursor] = thread_id;
		g_thread_level_counter [g_thread_id_cursor] = 1;
		level = g_thread_level_counter [g_thread_id_cursor];
		g_thread_id_cursor++;
	}

	switch (type)
	{
		case PROFILER_PACKET_TYPE_BEGIN:
		{
			profiler_buffer_write (&g_buffer, "begn", 4);
			profiler_buffer_write (&g_buffer, &filename_size, sizeof (uint32_t));
			profiler_buffer_write (&g_buffer, filename, filename_size);
			profiler_buffer_write (&g_buffer, &function_size, sizeof (uint32_t));
			profiler_buffer_write (&g_buffer, function, function_size);
			profiler_buffer_write (&g_buffer, &line, sizeof (uint32_t));
			profiler_buffer_write (&g_buffer, &thread_id, sizeof (uint32_t));
			profiler_buffer_write (&g_buffer, &level, sizeof (int32_t));
			profiler_buffer_write (&g_buffer, &now.QuadPart, sizeof (int64_t));
		} break;

		case PROFILER_PACKET_TYPE_END:
		{
			profiler_buffer_write (&g_buffer, "end\0", 4);
			profiler_buffer_write (&g_buffer, &thread_id, sizeof (uint32_t));
			profiler_buffer_write (&g_buffer, &level, sizeof (int32_t));
			profiler_buffer_write (&g_buffer, &now.QuadPart, sizeof (int64_t));
		} break;
	}
	

	int32_t size = (int32_t) sizeof (struct sockaddr);
	assert (sendto (g_socket, g_buffer.data, (int32_t) g_buffer.cursor, 0, (struct sockaddr *) &g_addr, size) != -1);
	++ g_packet_id;
}

int32_t profiler_recv (void *data, int32_t max_size)
{
	struct sockaddr_in tmp	= {0};
	int32_t size = (int32_t) sizeof (struct sockaddr);
	return recvfrom (g_socket, data, max_size, 0, (struct sockaddr *) &tmp, &size);
}

void profiler_init_buffer (profiler_buffer_t *buffer, void *begin, size_t size)
{
	if (buffer == NULL)
	{
		// @TODO(Jyri): Set error
		return;
	}

	buffer->data	= begin;
	buffer->size	= size;
	buffer->cursor	= 0;
}

void profiler_buffer_reset (profiler_buffer_t *buffer)
{
	if (buffer == NULL)
	{
		// @TODO(Jyri): Set error
		return;
	}

	buffer->cursor = 0;
}

int32_t profiler_buffer_write (profiler_buffer_t *buffer, const void *data, int32_t size)
{
	if (buffer == NULL || data == NULL || size == 0)
	{
		// @TODO(Jyri): Set error
		return 0;
	}

	if (buffer->cursor + size > buffer->size)
	{
		// @TODO(Jyri): Set error
		return 0;
	}

	CopyMemory (buffer->data + buffer->cursor, data, size);
	buffer->cursor += size;
	return (size);
}

int32_t profiler_buffer_read (profiler_buffer_t *buffer, void *to, int32_t size)
{
	if (buffer == NULL || to == NULL || size == 0)
	{
		// @TODO(Jyri): Set error
		return 0;
	}

	if (buffer->cursor + size > buffer->size)
	{
		// @TODO(Jyri): Set error
		return 0;
	}

	CopyMemory (to, buffer->data + buffer->cursor, size);
	buffer->cursor += size;
	return (size);
}