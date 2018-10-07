#include <profiler/profiler.h>
#include <WinSock2.h>
#include <assert.h>

#define BUFFER_SIZE 1024
#define MAX_THREADS 64

static SOCKET				g_socket							= INVALID_SOCKET;
static struct sockaddr_in	g_addr								= {0};
static LARGE_INTEGER		g_freq								= {0};
static LARGE_INTEGER		g_start_time						= {0};
static uint8_t				g_buffer [BUFFER_SIZE]				= {0};
static size_t				g_write_cursor						= 0;
static uint32_t				g_packet_id							= 0;
static uint32_t				g_thread_ids[MAX_THREADS] 			= {0};
static uint32_t				g_thread_id_cursor					= 0;
static int32_t				g_thread_level_counter[MAX_THREADS]	= {0};

void reset_buffer ()
{
	g_write_cursor = 0;
}

// @NOTE(Jyri): Nothing here is thread safe...
void write_to_buffer (const void *data, size_t size)
{
	if (g_write_cursor + size < BUFFER_SIZE)
	{
		CopyMemory (g_buffer + g_write_cursor, data, size);
		g_write_cursor += size;
	}
}


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
		QueryPerformanceFrequency	(&g_freq);
		QueryPerformanceCounter		(&g_start_time);

		write_to_buffer (&g_packet_id, sizeof (uint32_t));
		write_to_buffer ("init", 4);
		write_to_buffer (&g_freq, sizeof (LARGE_INTEGER));
		write_to_buffer (&g_start_time, sizeof (LARGE_INTEGER));

		int32_t size = (int32_t) sizeof (struct sockaddr);
		assert (sendto (g_socket, g_buffer, (int32_t) g_write_cursor, 0, (struct sockaddr *) &g_addr, size) != -1);
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
	reset_buffer ();

	DWORD thread_id		= GetCurrentThreadId ();
	LARGE_INTEGER now;
	QueryPerformanceCounter (&now);
	write_to_buffer (&g_packet_id, sizeof (uint32_t));
	int32_t	level = 0;
	uint32_t i;
	for (i = 0; i < g_thread_id_cursor; ++ i)
	{
		if (g_thread_ids [i] == thread_id)
		{
			if (type == PROFILER_PACKET_TYPE_BEGIN)
			{
				g_thread_level_counter [i] ++;
				level = g_thread_level_counter;
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
		g_thread_ids [i] = thread_id;
	}

	switch (type)
	{
		case PROFILER_PACKET_TYPE_BEGIN:
		{
			write_to_buffer ("begn", 4);
			write_to_buffer (&filename_size, sizeof (uint32_t));
			write_to_buffer (filename, filename_size);
			write_to_buffer (&function_size, sizeof (uint32_t));
			write_to_buffer (function, function_size);
			write_to_buffer (&line, sizeof (uint32_t));
			write_to_buffer (&thread_id, sizeof (DWORD));
			write_to_buffer (&level, sizeof (int32_t));
			write_to_buffer (&now.QuadPart, sizeof (int64_t));
		} break;

		case PROFILER_PACKET_TYPE_END:
		{
			write_to_buffer ("end\0", 4);
			write_to_buffer (&thread_id, sizeof (DWORD));
			write_to_buffer (&level, sizeof (int32_t));
			write_to_buffer (&now.QuadPart, sizeof (int64_t));
		} break;
	}
	

	int32_t size = (int32_t) sizeof (struct sockaddr);
	assert (sendto (g_socket, g_buffer, (int32_t) g_write_cursor, 0, (struct sockaddr *) &g_addr, size) != -1);
	++ g_packet_id;
}

int32_t profiler_recv (void *data, int32_t max_size)
{
	struct sockaddr_in tmp	= {0};
	int32_t size = (int32_t) sizeof (struct sockaddr);
	return recvfrom (g_socket, data, max_size, 0, (struct sockaddr *) &tmp, &size);
}
