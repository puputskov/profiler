#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <Windows.h>
#include "gl.h"
#include <profiler/profiler.h>


GLuint compile_shader (const char *source, GLenum gl_shader_type)
{
	assert (gl_shader_type != GL_INVALID_ENUM);
	GLuint shader = glCreateShader (gl_shader_type);

	GLint len = (GLint) strlen (source);
	glShaderSource (shader, 1, (const GLchar **) &source, &len);
	//assert (glGetError () == GL_NO_ERROR);

	glCompileShader (shader);
	//assert (glGetError () == GL_NO_ERROR);

	GLint success = 0;
	glGetShaderiv (shader, GL_COMPILE_STATUS, &success);
	assert (glGetError () == GL_NO_ERROR);
	//assert (success == GL_TRUE);

	if (success != GL_TRUE)
	{
		printf ("ERROR: Failed to compile shader!\n");

		int32_t max_length = 0;
		glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &max_length);
		
		int32_t length 	= 0;
		char *log 	= (char *) xmalloc (sizeof (char) * max_length);
		glGetShaderInfoLog (shader, max_length, &length, log);

		printf("GLSL Error: %s\n", log);
		free (log);

		glDeleteShader (shader);
		return (0);
	}

	return (shader);
}

void destroy_shader (GLuint shader)
{
	if (shader != 0)
	{
		glDeleteShader (shader);
	}
}



GLuint create_shader_program ()
{
	return glCreateProgram ();
}

void destroy_shader_program (GLuint program)
{
	glDeleteProgram (program);
}

void shader_program_attach_shader (GLuint program, GLuint shader)
{
	glAttachShader (program, shader);
}

void link_shader_program (GLuint program)
{
	glLinkProgram (program);

	GLint success = 0;
	glGetProgramiv	(program, GL_LINK_STATUS, &success);
	if (success != GL_TRUE)
	{
		printf ("ERROR: Failed to link shader program!\n");

		int32_t max_length = 0;
		glGetProgramiv (program, GL_INFO_LOG_LENGTH, &max_length);
		
		int32_t length 	= 0;
		char *log 	= (char *) xmalloc (sizeof (char) * max_length);
		glGetProgramInfoLog (program, max_length, &length, log);

		printf("GLSL Error: %s\n", log);
		free (log);
		
		glDeleteProgram (program);
		program = 0;
	}
}

const char vertex_shader_source [] = {
	"#version 330\n"
	"\n"
	"in vec2 POSITION;\n"
	"in vec2 TEXCOOD0;\n"
	"in vec4 COLOR;\n"
	"uniform mat4 prjection;\n"
	"out vec2 uv;\n"
	"out vec4 color;\n"
	"void main ()\n"
	"{\n"
	"	gl_Position = prjection * vec4 (POSITION.xy, 0.0, 1.0);\n"
	"	uv = TEXCOOD0;\n"
	"	color = COLOR;\n"
	"}\n"
};

const char pixel_shader_source [] = {
	"#version 420\n"
	"uniform sampler2D _texture;\n"
	"in vec2 uv;"
	"in vec4 color;"
	"out vec4 out_color;\n"
	"void main ()\n"
	"{\n"
	"	vec4 tc = texture (_texture, uv);"
	"	out_color = color * tc;\n"
	//"	out_color = clamp (color + tc, vec4 (0.0), vec4 (1.0));\n"
	"}\n"
};

static GLuint vbo;
static GLuint ebo;
static GLuint shader_program;


GLuint create_texture (uint8_t *pixels, uint32_t w, uint32_t h, uint32_t channels)
{
	GLenum format = GL_RGB;
	switch (channels)
	{
		case 1:
		{
			format = GL_RED;
		} break;

		case 2:
		{
			format = GL_RG;
		} break;

		case 3:
		{
			format = GL_RGB;
		} break;

		case 4:
		{
			format = GL_RGBA;
		} break;
	}

	GLuint texture = 0;
	glGenTextures (1, &texture);
	glBindTexture (GL_TEXTURE_2D, texture);
	glTexImage2D (GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, pixels);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture (GL_TEXTURE_2D, 0);	
	return (texture);
}

void user_ui_init (int32_t screen_width, int32_t screen_height)
{
	ui_init (screen_width, screen_height, (ui_create_texture_callback_t) create_texture);
	/*ui_state.io.screen_width = screen_width;
	ui_state.io.screen_height = screen_height;
	ui_state.margin_vertical = 8;*/

	GLuint vertex_shader = compile_shader (vertex_shader_source, GL_VERTEX_SHADER);
	GLuint pixel_shader = compile_shader (pixel_shader_source, GL_FRAGMENT_SHADER);
	shader_program = glCreateProgram ();
	glAttachShader (shader_program, vertex_shader);
	glAttachShader (shader_program, pixel_shader);

	glBindAttribLocation (shader_program, 0, "POSITION");
	glBindAttribLocation (shader_program, 1, "TEXCOOD0");
	glBindAttribLocation (shader_program, 2, "COLOR");
	link_shader_program (shader_program);

	glGenBuffers (1, &vbo);
	glGenBuffers (1, &ebo);
}

void user_ui_update (app_t *app)
{
	ui_update ();
	ui_state.io.screen.size.x = app->size.x;
	ui_state.io.screen.size.y = app->size.y;
	ui_state.io.mouse.state = app->mouse_state;
	ui_state.io.mouse.position.x = app->cursor.x;
	ui_state.io.mouse.position.y = app->cursor.y;
}

void user_ui_render ()
{
	float width		= (float) ui_state.io.screen.size.x;
	float height	= (float) ui_state.io.screen.size.y;
	float projection[16] = {
		2.0f / width,	0.0f,			0.0f, 0.0f,
		0.0f,			2 / -height,	0.0f, 0.0f,
		0.0f,			0.0f,			1.0f, 0.0f,
		-1.0f,			1.0f,			0.0f, 1.0f
	};

	glViewport (0, 0, (int32_t) width, (int32_t) height);
	glUseProgram (shader_program);
	GLint prj_location = glGetUniformLocation (shader_program, "prjection");
	glUniformMatrix4fv (prj_location, 1, GL_FALSE, projection);
	
	

	GLuint vao;
	glGenVertexArrays (1, &vao);	
	glBindVertexArray (vao);
	glEnable (GL_SCISSOR_TEST);
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	size_t i;
	for (i = 0; i < array_size (ui_state.windows); ++ i)
	{
		ui_window_t *window = &ui_state.windows [i];

		
		int32_t y = ((int32_t) height - window->size.y) - window->position.y - 1;
		glScissor (window->position.x - 1, y, window->size.x + 2, window->size.y + 2);
		
		size_t j;
		for (j = 0; j < window->cmd_list_count; ++ j)
		{
			ui_cmd_buffer_t *cmd = &window->cmd_list [j];

			GLuint texture = (GLuint) cmd->texture_id;
			glActiveTexture (GL_TEXTURE0);
			glBindTexture (GL_TEXTURE_2D, texture);
			glUniform1i (glGetUniformLocation (shader_program, "_texture"), 0);

			glBindBuffer (GL_ARRAY_BUFFER, vbo);
			glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, ebo);
			glBufferData (GL_ARRAY_BUFFER,			sizeof (ui_vertex_t) * cmd->vertex_count, cmd->vertices, GL_DYNAMIC_DRAW);
			glBufferData (GL_ELEMENT_ARRAY_BUFFER,	sizeof (uint32_t) * cmd->index_count, cmd->indices, GL_DYNAMIC_DRAW);

			glEnableVertexAttribArray	(0);
			glVertexAttribPointer		(0, 2, GL_FLOAT, GL_FALSE, sizeof (ui_vertex_t), NULL);
			glEnableVertexAttribArray	(1);
			glVertexAttribPointer		(1, 2, GL_FLOAT, GL_FALSE, sizeof (ui_vertex_t), (void *) (sizeof (ui_vector2_t)));

			glEnableVertexAttribArray	(2);
			glVertexAttribPointer		(2, 4, GL_FLOAT, GL_FALSE, sizeof (ui_vertex_t), (void *) (sizeof (ui_vector2_t) * 2));

			glDrawElements (GL_TRIANGLES, (GLsizei) cmd->index_count, GL_UNSIGNED_INT, NULL);
		}
	}
	glDisable (GL_SCISSOR_TEST);
	glDeleteVertexArrays (1, &vao);
}


typedef struct
{
	uint32_t	length;
	char		*data;
} string_t;

typedef enum
{
	PROFILER_ENTRY_STATE_WAITING = 0,
	PROFILER_ENTRY_STATE_FINISHED,
} PROFILER_ENTRY_STATE;

typedef struct
{
	PROFILER_ENTRY_STATE state;

	string_t	filename;
	string_t	function;
	uint32_t	line;
	uint32_t	thread_id;
	int32_t		level;

	int64_t	begin;
	int64_t	end;
} profiler_entry_t;

typedef struct
{
	uint32_t	last_packet_id;
	int64_t		freq;
	int64_t		start;

	profiler_entry_t	*entries;
	uint32_t			entry_capacity;
	uint32_t			number_of_entries;
} profiler_state_t;


profiler_entry_t *add_new_entry (profiler_state_t *state)
{
	if (state->entries == NULL)
	{
		state->entry_capacity = 1024;
		state->entries = (profiler_entry_t *) malloc (sizeof (profiler_entry_t) * state->entry_capacity);
		state->number_of_entries = 0;
	}

	else if (state->entries != NULL && state->number_of_entries + 1 >= state->entry_capacity)
	{
		state->entry_capacity += 1024;
		state->entries = (profiler_entry_t *) realloc (state->entries, sizeof (profiler_entry_t) * state->entry_capacity);
	}

	return &state->entries [state->number_of_entries ++];
}

int main(int argc, char **argv)
{
	assert (profiler_init (PROFILER_TYPE_SERVER, PROFILER_MAKE_ADDRESS (0, 0, 0, 0), 1672));
	app_t app = create_app("Profiler", 1280, 720);

	int32_t show = 0;
	float red = 0.1f;
	float green = 0.1f;
	float blue = 0.1f;

	user_ui_init (app.size.x, app.size.y);
	float zoom = 2.0f;
	int32_t x_offset = 0;
	profiler_state_t profiler_state = {0};
	int32_t last_x = 0;
	int32_t selected = -1;

	while (app.open)
	{
		last_x = app.cursor.x;
		update_app(&app);
		user_ui_update (&app);

		uint8_t buffer_data [1024] = {0};
		int32_t data_to_read = 0;
		while ((data_to_read = profiler_recv (buffer_data, 1024)) > 0)
		{
			profiler_buffer_t buffer = {0};
			profiler_init_buffer (&buffer, buffer_data, data_to_read);
			uint32_t buffer_read_index = 0;

			uint32_t packet_id = 0;
			profiler_buffer_read (&buffer, &packet_id, sizeof (uint32_t));
			if (profiler_state.last_packet_id < packet_id)
			{
				profiler_state.last_packet_id = packet_id;
			}


			char packet_type[4] = {0};	// This could be an uint32_t
			profiler_buffer_read (&buffer, packet_type, 4);
			if (strncmp (packet_type, "init", 4) == 0)
			{
				profiler_buffer_read (&buffer, &profiler_state.freq, sizeof (int64_t));
				profiler_buffer_read (&buffer, &profiler_state.start, sizeof (int64_t));
			}

			else if (strncmp (packet_type, "begn", 4) == 0)
			{
				profiler_entry_t *entry = add_new_entry (&profiler_state);
				entry->state = PROFILER_ENTRY_STATE_WAITING;

				profiler_buffer_read (&buffer, &entry->filename.length, sizeof (uint32_t));
				entry->filename.data = (char *) malloc (entry->filename.length + 1);
				profiler_buffer_read (&buffer, entry->filename.data, entry->filename.length);
				entry->filename.data [entry->filename.length] = 0;

				profiler_buffer_read (&buffer, &entry->function.length, sizeof (uint32_t));
				entry->function.data = (char *) malloc (entry->function.length + 1);
				profiler_buffer_read (&buffer, entry->function.data, entry->function.length);
				entry->function.data [entry->function.length] = 0;

				profiler_buffer_read (&buffer, &entry->line,		sizeof (uint32_t));
				profiler_buffer_read (&buffer, &entry->thread_id,	sizeof (uint32_t));
				profiler_buffer_read (&buffer, &entry->level,		sizeof (int32_t));
				profiler_buffer_read (&buffer, &entry->begin,		sizeof (int64_t));
			}

			else if (strncmp (packet_type, "end\0", 4) == 0)
			{
				uint32_t thread_id	= 0;
				int32_t level		= 0;
				int64_t time		= 0;
				profiler_buffer_read (&buffer, &thread_id,	sizeof (uint32_t));
				profiler_buffer_read (&buffer, &level,		sizeof (int32_t));
				profiler_buffer_read (&buffer, &time,		sizeof (int64_t));


				uint32_t i;
				for (i = 0; i < profiler_state.number_of_entries; ++ i)
				{
					if (profiler_state.entries [i].state == PROFILER_ENTRY_STATE_WAITING &&
						profiler_state.entries [i].thread_id == thread_id &&
						profiler_state.entries [i].level == level)
					{
						profiler_state.entries [i].state = PROFILER_ENTRY_STATE_FINISHED;
						profiler_state.entries [i].end = time;
						break;
					}
				}
			}
		}

		if (app.mouse_state & APP_MOUSE_STATE_BUTTON_RIGHT)
		{
			x_offset += (app.cursor.x - last_x);
		}

		if (zoom > 1.0f)
		{
			zoom += app.wheel_delta;
		}

		else if (zoom > 0.5f)
		{
			zoom += app.wheel_delta * 0.1f;
		}

		else if (zoom >= 0.01f)
		{
			zoom += app.wheel_delta * 0.01f;
			if (zoom <= 0.01f)
			{
				zoom = 0.01f;
			}
		}

		


		glClearColor (red, green, blue, 1.0f);
		glClear (GL_COLOR_BUFFER_BIT);

		int32_t x = 0, y = 0, w = 256, h = app.size.y;
		ui_begin ("Sessions", &x, &y, &w, &h);
		if (ui_button ("Quit")) 
		{
			app.open = false;
		}

		x = 256, y = 0, w = app.size.x - 256, h = app.size.y;
		ui_begin ("Profiler Data", &x, &y, &w, &h);

		uint32_t i;
		for (i = 0; i < profiler_state.number_of_entries; ++ i)
		{
			switch (profiler_state.entries [i].state)
			{
				case PROFILER_ENTRY_STATE_WAITING:
				{

				} break;

				case PROFILER_ENTRY_STATE_FINISHED:
				{
					int64_t elapsed_start = (profiler_state.entries [i].begin - profiler_state.start) * 1000000;
					elapsed_start /= profiler_state.freq;

					int64_t elapsed = (profiler_state.entries [i].end - profiler_state.entries [i].begin) * 1000000;
					elapsed /= profiler_state.freq;

					// HACK(Jyri): Get rid of the id
					if (ui_rect (__LINE__ + i, x_offset + (int32_t)(((float)elapsed_start / 100.0f) * zoom), 64 * (profiler_state.entries[i].level - 1), (int32_t)(((float)elapsed / 100.0f) * zoom)))
					{
						printf ("File:      %.*s\n", profiler_state.entries [i].filename.length, profiler_state.entries [i].filename.data);
						printf ("Function:  %.*s\n", profiler_state.entries [i].function.length, profiler_state.entries [i].function.data);
						printf ("Line:      %u\n", profiler_state.entries [i].line);
						printf ("Time:      %lli\n\n", elapsed);

						selected = i;
					}
				} break;
			}
		}

		if (selected != -1)
		{
			ui_draw_rect_std (256, app.size.y - 98, app.size.x - 256, 96 + 2, 0, 0, 0, 1);
			ui_draw_rect_std (257, app.size.y - 97, app.size.x - 256 - 2, 96, 1.0f, 1.0f, 1.0f, 1);
			
			char str_buffer [1024] = {0};
			ui_draw_text ((ui_vector2i_t) {257 + 32, app.size.y - 98}, "File: ", (ui_vector4_t) {0.0f, 0.0f, 0.0f, 1.0f});
			ui_draw_text ((ui_vector2i_t) {257 + 128, app.size.y - 98}, profiler_state.entries [selected].filename.data, (ui_vector4_t) {0.0f, 0.0f, 0.0f, 1.0f});
			ui_draw_text ((ui_vector2i_t) {257 + 32, app.size.y - 98 + 24}, "Function: ", (ui_vector4_t) {0.0f, 0.0f, 0.0f, 1.0f});
			ui_draw_text ((ui_vector2i_t) {257 + 128, app.size.y - 98 + 24}, profiler_state.entries [selected].function.data, (ui_vector4_t) {0.0f, 0.0f, 0.0f, 1.0f});

			sprintf (str_buffer, "%i", profiler_state.entries [selected].line);
			ui_draw_text ((ui_vector2i_t) {257 + 32, app.size.y - 98 + 48}, "Line: ", (ui_vector4_t) {0.0f, 0.0f, 0.0f, 1.0f});
			ui_draw_text ((ui_vector2i_t) {257 + 128, app.size.y - 98 + 48}, str_buffer, (ui_vector4_t) {0.0f, 0.0f, 0.0f, 1.0f});

			sprintf (str_buffer, "%lli", profiler_state.entries [selected].end - profiler_state.entries [selected].begin);
			ui_draw_text ((ui_vector2i_t) {257 + 32, app.size.y - 98 + 72}, "Ticks: ", (ui_vector4_t) {0.0f, 0.0f, 0.0f, 1.0f});
			ui_draw_text ((ui_vector2i_t) {257 + 128, app.size.y - 98 + 72}, str_buffer, (ui_vector4_t) {0.0f, 0.0f, 0.0f, 1.0f});

		}

		user_ui_render ();
		app_swap_buffers (&app);
		Sleep(16);
	}

	profiler_quit ();
	return (0);
}
