#ifndef PROFILER_PROFILER_H
#define PROFILER_PROFILER_H

#include <stdint.h>

#define PROFILER_MAKE_ADDRESS(a, b, c, d) (((a & 0xFF) << 24) | ((b & 0xFF) << 16) | ((c & 0xFF) << 8) | (d & 0xFF))
#define PROFILER_BEGIN()	profiler_send (PROFILER_PACKET_TYPE_BEGIN, __FILE__, sizeof (__FILE__) / sizeof (char), __FUNCTION__, sizeof (__FUNCTION__) / sizeof (char), __LINE__);
#define PROFILER_END()		profiler_send (PROFILER_PACKET_TYPE_END, NULL,  0, NULL, 0, 0);


typedef enum
{
	PROFILER_TYPE_SERVER,
	PROFILER_TYPE_PROFILER,
} PROFILER_TYPE;

typedef enum 
{
	PROFILER_PACKET_TYPE_BEGIN,
	PROFILER_PACKET_TYPE_END,
} PROFILER_PACKET_TYPE;


typedef struct
{
	uint8_t *data;
	size_t	size;
	size_t	cursor;
} profiler_buffer_t;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int32_t	profiler_init	(PROFILER_TYPE type, uint32_t address, uint16_t port);
int32_t	profiler_quit	();

void	profiler_send	(PROFILER_PACKET_TYPE type, const char *filename, uint32_t filename_size, const char *function, uint32_t function_size, uint32_t line);
int32_t	profiler_recv	(void *data, int32_t max_size);

void 	profiler_buffer_reset	(profiler_buffer_t *buffer);
int32_t	profiler_buffer_write	(profiler_buffer_t *buffer, const void *data, int32_t size);
int32_t	profiler_buffer_read	(profiler_buffer_t *buffer, void *to, int32_t size);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // PROFILER_PROFILER_H
