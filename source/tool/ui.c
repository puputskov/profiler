#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

void *xmalloc (size_t size)
{
	void *mem = malloc (size);
	if (mem == NULL)
	{
		printf("malloc failed!\n");
		exit (0);
	}

	return (mem);
}

void *xrealloc (void *mem, size_t size)
{
	void *tmp = realloc (mem, size);
	if (tmp == NULL)
	{
		printf("realloc failed!\n");
		exit (0);
	}

	return (tmp);
}

typedef struct
{
	size_t	size;
	size_t	capacity;
} array_header_t;

#define array_header(a)	((array_header_t *)((char *)(a) - sizeof (array_header_t)))
#define array_size(a)		((a) ? array_header (a)->size : 0)
#define array_capacity(a)	((a) ? array_header (a)->capacity : 0)
#define array_full(a)		(array_size (a) == array_capacity (a))
#define array_push(a, item)	((array_full (a) ? (a = array_grow (a, sizeof (*a))) : 0), (a[array_header (a)->size ++] = item))
#define array_free(a)		((a) ? (free (array_header (a)), a = NULL) : 0)
#define array_clear(a)		((a) ? (array_header (a)->size = 0) : 0)

void *array_grow (void *arr, size_t element_size)
{
	size_t new_capacity = (1 + 2 * array_capacity (arr));
	size_t new_size = (new_capacity * element_size) + sizeof (array_header_t);
	array_header_t *new_header = NULL;
	if (arr == NULL)
	{
		new_header = (array_header_t *) xmalloc (new_size);
		new_header->size = 0;
	}

	else
	{
		new_header = (array_header_t *) xrealloc (array_header (arr), new_size);
	}

	new_header->capacity = new_capacity;
	return ((char *) new_header + sizeof (array_header_t));
}


uint64_t ui_hash (const char *str)
{
	// DJB2 Hash
	uint64_t hash = 5381;
	uint64_t c;
	while (c = *str++)
	{
		hash = ((hash << 5) + hash) + c;
	}

	return (hash);
}


typedef void *ui_texture_id_t;

typedef struct 
{
	int32_t x, y;
} ui_vector2i_t;

typedef struct 
{
	int32_t x, y, z;
} ui_vector3i_t;

typedef struct 
{
	int32_t x, y, z, w;
} ui_vector4i_t;

typedef struct 
{
	float x, y;
} ui_vector2_t;

typedef struct 
{
	float x, y, z;
} ui_vector3_t;

typedef struct 
{
	float x, y, z, w;
} ui_vector4_t;

typedef struct
{
	ui_vector2_t position;
	ui_vector2_t uv;
	ui_vector4_t color;
} ui_vertex_t;

typedef struct
{
	ui_texture_id_t texture_id;
	ui_vertex_t		*vertices;
	uint32_t		*indices;
	uint32_t		vertex_count;
	uint32_t		index_count;
} ui_cmd_buffer_t;

typedef struct
{
	uint64_t		id;
	uint32_t		flags;
	ui_vector2i_t	position;
	ui_vector2i_t	size;

	ui_cmd_buffer_t	*cmd_list;
	uint32_t		cmd_list_count;

	ui_vector2i_t	current_item_position;
} ui_window_t;


typedef struct
{
	struct
	{
		ui_vector2i_t	size;	
	} screen;

	struct
	{
		float	global;
		float	delta;
	} time;
	
	struct
	{
		ui_vector2i_t	position;
		uint32_t		state;
	} mouse;
} ui_io_t;


typedef struct
{
	ui_texture_id_t	texture_id;
	stbtt_bakedchar	cdata[96];
} ui_font_t;


typedef void *(*ui_create_texture_callback_t)(uint8_t *pixels, uint32_t w, uint32_t h, uint32_t channels);
typedef struct
{
	ui_io_t			io;
	ui_io_t			last_io;
	
	ui_window_t		*windows;
	uint32_t		window_count;

	ui_window_t		*current_working_window;


	uint64_t		hot_item;
	uint64_t		active_item;
	uint64_t		hot_window;
	uint64_t		active_window;
	ui_font_t		font;
	ui_texture_id_t	no_texture;

	struct
	{
		ui_create_texture_callback_t create_texture;
	} callback;
} ui_state_t;
static ui_state_t ui_state;

void ui_push_texture (ui_texture_id_t texture)
{
	ui_window_t *wnd = ui_state.current_working_window;
	if (wnd->cmd_list_count > 0)
	{
		if (wnd->cmd_list [wnd->cmd_list_count - 1].texture_id == texture)
		{
			return;
		}
	}

	ui_cmd_buffer_t buffer	= {0};
	buffer.texture_id		= texture;
	buffer.vertex_count		= 0;
	buffer.index_count		= 0;
	array_push (wnd->cmd_list, buffer);
	wnd->cmd_list_count ++;
}

void ui_draw_rect (ui_vector2i_t position, ui_vector2i_t size, ui_vector4_t color)
{
	ui_cmd_buffer_t *cmd = &ui_state.current_working_window->cmd_list [ui_state.current_working_window->cmd_list_count - 1];
	array_push (cmd->vertices, ((ui_vertex_t) {{(float) position.x,						(float) position.y}, 					{0.0f, 0.0f}, color}));
	array_push (cmd->vertices, ((ui_vertex_t) {{(float) position.x + (float) size.x,	(float) position.y}, 					{1.0f, 0.0f}, color}));
	array_push (cmd->vertices, ((ui_vertex_t) {{(float) position.x + (float) size.x,	(float) position.y + (float) size.y}, 	{1.0f, 1.0f}, color}));
	array_push (cmd->vertices, ((ui_vertex_t) {{(float) position.x,						(float) position.y + (float) size.y}, 	{0.0f, 1.0f}, color}));

	array_push (cmd->indices, cmd->vertex_count + 0);
	array_push (cmd->indices, cmd->vertex_count + 1);
	array_push (cmd->indices, cmd->vertex_count + 2);
	array_push (cmd->indices, cmd->vertex_count + 2);
	array_push (cmd->indices, cmd->vertex_count + 3);
	array_push (cmd->indices, cmd->vertex_count + 0);

	cmd->vertex_count += 4;
	cmd->index_count += 6;
}

void ui_draw_rect_uv (ui_vector2i_t position, ui_vector2i_t size, ui_vector4_t uv, ui_vector4_t color)
{
	ui_cmd_buffer_t *cmd = &ui_state.current_working_window->cmd_list [ui_state.current_working_window->cmd_list_count - 1];
	array_push (cmd->vertices, ((ui_vertex_t) {{(float) position.x,						(float) position.y}, 					{uv.x, uv.y}, color}));
	array_push (cmd->vertices, ((ui_vertex_t) {{(float) position.x + (float) size.x,	(float) position.y}, 					{uv.z, uv.y}, color}));
	array_push (cmd->vertices, ((ui_vertex_t) {{(float) position.x + (float) size.x,	(float) position.y + (float) size.y}, 	{uv.z, uv.w}, color}));
	array_push (cmd->vertices, ((ui_vertex_t) {{(float) position.x,						(float) position.y + (float) size.y}, 	{uv.x, uv.w}, color}));

	array_push (cmd->indices, cmd->vertex_count + 0);
	array_push (cmd->indices, cmd->vertex_count + 1);
	array_push (cmd->indices, cmd->vertex_count + 2);
	array_push (cmd->indices, cmd->vertex_count + 2);
	array_push (cmd->indices, cmd->vertex_count + 3);
	array_push (cmd->indices, cmd->vertex_count + 0);

	cmd->vertex_count += 4;
	cmd->index_count += 6;
}


ui_vector2i_t ui_draw_text (ui_vector2i_t position, const char *title, ui_vector4_t color)
{
	ui_vector2i_t result = {0, 0};
	ui_push_texture (ui_state.font.texture_id);

	position.x += 8;
	position.y += 20;



	int32_t x = 0;
	int32_t y = 0;
	int32_t line = 0;

	ui_cmd_buffer_t *cmd = &ui_state.current_working_window->cmd_list [ui_state.current_working_window->cmd_list_count - 1];
	char *text = title;
	while (*text != 0)
	{
		if (*text == '\n')
		{
			x = 0;
			position.y += 20;
		}

		else if (*text >= 32 && *text < 128)
		{
			stbtt_aligned_quad q;
			stbtt_GetBakedQuad (ui_state.font.cdata, 512, 512, (*text - 32), &x, &y, &q, 1);
			ui_draw_rect_uv ((ui_vector2i_t) {q.x0 + position.x, q.y0 + position.y}, (ui_vector2i_t) {(q.x1 - q.x0), (q.y1 - q.y0)}, (ui_vector4_t){q.s0, q.t0, q.s1, q.t1}, color);

			/*array_push (cmd->vertices, ((ui_vertex_t) {{q.x0 + position.x, q.y0 + position.y}, 	{q.s0, q.t0}, color}));
			array_push (cmd->vertices, ((ui_vertex_t) {{q.x1 + position.x, q.y0 + position.y}, 	{q.s1, q.t0}, color}));
			array_push (cmd->vertices, ((ui_vertex_t) {{q.x1 + position.x, q.y1 + position.y}, 	{q.s1, q.t1}, color}));
			array_push (cmd->vertices, ((ui_vertex_t) {{q.x0 + position.x, q.y1 + position.y}, 	{q.s0, q.t1}, color}));

			array_push (cmd->indices, cmd->vertex_count + 0);
			array_push (cmd->indices, cmd->vertex_count + 1);
			array_push (cmd->indices, cmd->vertex_count + 2);
			array_push (cmd->indices, cmd->vertex_count + 2);
			array_push (cmd->indices, cmd->vertex_count + 3);
			array_push (cmd->indices, cmd->vertex_count + 0);

			cmd->vertex_count += 4;
			cmd->index_count += 6;*/

			if (q.x1 > result.x)
			{
				result.x = q.x1;
			}

			else if (q.x0 > result.x)
			{
				result.x = q.x0;
			}

			if (q.y1 > result.y)
			{
				result.y = q.y1;
			}

			else if (q.y0 > result.y)
			{
				result.y = q.y0;
			}
		}

		++ text;
	}

	ui_push_texture (ui_state.no_texture);
	return (result);
}


void ui_draw_rect_std (int32_t x, int32_t y, int32_t w, int32_t h, float r, float g, float b, float a)
{
	ui_draw_rect ((ui_vector2i_t) {x, y}, (ui_vector2i_t) {w, h}, (ui_vector4_t) {r, g, b, a});
}


ui_window_t *ui_find_window_by_id (uint64_t id)
{
	uint32_t i;
	for (i = 0; i < ui_state.window_count; ++ i)
	{
		if (ui_state.windows [i].id == id)
		{
			return (&ui_state.windows [i]);
		}
	}

	return (NULL);
}


int32_t ui_is_mouse_clicked ()
{
	return (ui_state.io.mouse.state == 0 && ui_state.last_io.mouse.state == 1);
}

int32_t ui_is_hot_and_active (uint64_t id)
{
	return (ui_state.hot_item == id && ui_state.active_item == id);
}


int32_t ui_is_mouse_in (int32_t x, int32_t y, int32_t w, int32_t h)
{
	return (ui_state.io.mouse.position.x >=x && ui_state.io.mouse.position.x <x + w &&
			ui_state.io.mouse.position.y >=y && ui_state.io.mouse.position.y <y + h);
}

ui_vector2i_t ui_mouse_to_window (ui_window_t *window)
{
	ui_vector2i_t position = {
		ui_state.io.mouse.position.x - window->position.x,
		ui_state.io.mouse.position.y - window->position.y
	};

	return (position);
}


void ui_begin (const char *title, int32_t *x, int32_t *y, int32_t *w, int32_t *h)
{
	uint64_t id = ui_hash (title);
	ui_window_t *wnd = ui_find_window_by_id (id);

	if (wnd == NULL)
	{
		// Create new window
		ui_window_t window = {0};
		window.id = id;
		window.position.x = *x;
		window.position.y = *y;
		window.size.x = *w;
		window.size.y = *h;

		array_push (ui_state.windows, window);
		wnd = &ui_state.windows [ui_state.window_count ++];
	}

	ui_state.current_working_window = wnd;

	if (wnd->position.x != *x || wnd->position.y != *y)
	{
		wnd->position.x = *x;
		wnd->position.y = *y;
	}

	if (wnd->size.x != *w || wnd->size.y != *h)
	{
		wnd->size.x = *w;
		wnd->size.y = *h;
	}

	array_clear (wnd->cmd_list);


	/*ui_cmd_buffer_t buffer	= {0};
	buffer.texture_id		= ui_state.no_texture;
	buffer.vertex_count		= 0;
	buffer.index_count		= 0;
	array_push (wnd->cmd_list, buffer);*/
	ui_push_texture (ui_state.no_texture);

	ui_vector4_t border_color		= {0.2f, 0.2f, 1.0f, 1.0f};
	ui_vector4_t background_color	= {0.90f, 0.90f, 0.90f, 1.0f};
	ui_vector4_t titlebar_color		= {1.0f, 1.0f, 1.0f, 1.0f};
	ui_vector4_t text_color			= {0.0f, 0.0f, 0.0f, 1.0f}; 
	ui_draw_rect_std	(wnd->position.x - 1, wnd->position.y - 1,
						wnd->size.x + 2, wnd->size.y + 2,
						border_color.x, border_color.y,
						border_color.z, border_color.w);

	ui_draw_rect (wnd->position, wnd->size, background_color);
	ui_draw_rect (wnd->position, (ui_vector2i_t) {wnd->size.x, 32}, titlebar_color);
	ui_draw_text (wnd->position, title, text_color);

	wnd->current_item_position.x = wnd->position.x + 8;
	wnd->current_item_position.y = wnd->position.y + 42;
	
}


int32_t ui_button (const char *title)
{
	ui_window_t *wnd = ui_state.current_working_window;

	

	ui_vector4_t border_color		= {0.2f, 0.2f, 0.2f, 1.0f};
	ui_vector4_t background_color	= {0.90f, 0.90f, 0.90f, 1.0f};
	ui_vector4_t text_color			= {0.0f, 0.0f, 0.0f, 1.0f}; 
	

	


	uint64_t id = ui_hash (title);
	ui_vector2i_t position = wnd->current_item_position;
	wnd->current_item_position.y += 40;
	

	int32_t width = 64;
	int32_t height = 32;

	if (ui_is_mouse_in (position.x, position.y, width, height))
	{
		ui_state.hot_item = id;
		if (ui_state.io.mouse.state == 1)
		{
			ui_state.active_item = id;

			background_color.x = 0.3f;
			background_color.y = 0.3f;
			background_color.z = 0.3f;
			border_color = background_color;
		}

		else
		{
			background_color.x = 0.45f;
			background_color.y = 0.45f;
			background_color.z = 0.45f;
			border_color.x = 0.3f;
			border_color.y = 0.3f;
			border_color.z = 0.3f;
		}		
	}

	ui_draw_rect ((ui_vector2i_t) {position.x - 1, position.y - 1}, (ui_vector2i_t) {64 + 2, 32 + 2}, border_color);
	ui_draw_rect (position, (ui_vector2i_t) {64, 32}, background_color);
	ui_draw_text (position, title, text_color);

	return (ui_is_hot_and_active (id) && ui_is_mouse_clicked ());
}



int32_t ui_rect (int32_t x, int32_t y, int32_t w)
{
	ui_window_t *wnd = ui_state.current_working_window;

	

	ui_vector4_t border_color		= {0.2f, 0.2f, 1.0f, 1.0f};
	ui_vector4_t background_color	= {0.2f, 1.0f, 0.2f, 1.0f};
	ui_vector4_t text_color			= {0.0f, 0.0f, 0.0f, 1.0f}; 
	

	


	//uint64_t id = ui_hash (title);
	ui_vector2i_t position = wnd->current_item_position;
	position.x += x;
	position.y += y;
	//wnd->current_item_position.y += 40;
	

	int32_t width = w;
	int32_t height = 48;
	//wnd->current_item_position.x += w;

	if (ui_is_mouse_in (position.x, position.y, width, height))
	{
		ui_state.hot_item = 0xFFFFFFFFFF;
		if (ui_state.io.mouse.state == 1)
		{
			ui_state.active_item = 0xFFFFFFFFFF;

			background_color.x = 0.3f;
			background_color.y = 0.3f;
			background_color.z = 0.3f;
			border_color = background_color;
		}

		else
		{
			background_color.x = 0.45f;
			background_color.y = 0.45f;
			background_color.z = 0.45f;
			border_color.x = 0.3f;
			border_color.y = 0.3f;
			border_color.z = 0.3f;
		}		
	}

	ui_draw_rect ((ui_vector2i_t) {position.x - 1, position.y - 1}, (ui_vector2i_t) {width + 2, height + 2}, border_color);
	ui_draw_rect (position, (ui_vector2i_t) {width, height}, background_color);
	//ui_draw_text (position, title, text_color);

	return (ui_is_hot_and_active (0xFFFFFFFFFF) && ui_is_mouse_clicked ());
}


void ui_row (int32_t height)
{
	ui_window_t *wnd = ui_state.current_working_window;
	wnd->current_item_position.x = wnd->position.x + 8;
	wnd->current_item_position.y += height;
}

void ui_check_box (const char *str, int32_t *value)
{
	uint64_t id = ui_hash (str);
	ui_window_t *wnd = ui_state.current_working_window;
	ui_vector2i_t position = wnd->current_item_position;
	wnd->current_item_position.y += 40;

	int32_t width = 24;
	int32_t height = 24;

	ui_vector4_t border_color = {0.2f, 0.2f, 0.2f, 1.0f};
	ui_vector4_t bg_color = {0.4f, 0.4f, 0.4f, 1.0f};
	ui_vector4_t text_color			= {0.0f, 0.0f, 0.0f, 1.0f}; 

	position.x -= 8;
	ui_vector2i_t text_size = ui_draw_text (position, str, text_color);
	position.x += text_size.x + 16;
	position.y += 4;

	if (ui_is_mouse_in (position.x, position.y, width, height))
	{
		ui_state.hot_item = id;
		if (ui_state.io.mouse.state == 1)
		{
			ui_state.active_item = id;

			bg_color.x = 0.3f;
			bg_color.y = 0.3f;
			bg_color.z = 0.3f;
			border_color = bg_color;
		}

		else
		{
			bg_color.x = 0.45f;
			bg_color.y = 0.45f;
			bg_color.z = 0.45f;
			border_color.x = 0.3f;
			border_color.y = 0.3f;
			border_color.z = 0.3f;
		}
	}


	ui_draw_rect ((ui_vector2i_t) {position.x - 1, position.y - 1}, (ui_vector2i_t) {width + 2, height + 2}, border_color);
	ui_draw_rect (position, (ui_vector2i_t) {width, height}, bg_color);

	int32_t result = (ui_is_hot_and_active (id) && ui_is_mouse_clicked ());
	if (value != NULL)
	{
		if (result)
		{
			*value = !(*value);
		}

		if (*value)
		{
			ui_draw_rect ((ui_vector2i_t) {position.x + 4, position.y + 4}, (ui_vector2i_t) {width - 8, height - 8}, border_color);
		}
	}
	
}


void ui_slider (const char *str, float min, float max, float *value)
{
	uint64_t id = ui_hash (str);
	ui_window_t *wnd = ui_state.current_working_window;
	ui_vector2i_t position = wnd->current_item_position;
	wnd->current_item_position.y += 48;

	int32_t width = wnd->size.x - 16;
	int32_t height = 24;

	ui_vector4_t border_color = {0.2f, 0.2f, 0.2f, 1.0f};
	ui_vector4_t bg_color = {0.4f, 0.4f, 0.4f, 1.0f};
	ui_vector4_t text_color			= {0.0f, 0.0f, 0.0f, 1.0f}; 

	ui_vector2i_t mouse_position = ui_mouse_to_window (wnd);

	float mouse_percent = ((float)(mouse_position.x - 16) / ((float)width - 24));
	if (mouse_percent < 0.0f)
	{
		mouse_percent = 0.0f;
	}
	else if (mouse_percent > 1.0f)
	{
		mouse_percent = 1.0f;
	}

	position.x -= 8;
	char buffer [512] = {0};
	sprintf (buffer, "%s: %.4f", str, *value);
	ui_draw_text (position, buffer, text_color);
	position.y += 24;
	position.x += 8;

	float percent = (*value / max);
	if (ui_is_mouse_in (position.x, position.y, width, height))
	{
		ui_state.hot_item = id;
		if (ui_state.io.mouse.state == 1)
		{
			ui_state.active_item = id;

			border_color.x = 0.3f;
			border_color.y = 0.3f;
			border_color.z = 0.3f;

			if (value != NULL)
			{
				percent = mouse_percent;
				*value = max * percent;
				if (*value < min)
				{
					*value = min;
				}
				else if (*value > max)
				{
					*value = max;
				}
			}
		}

		else
		{
			bg_color.x = 0.45f;
			bg_color.y = 0.45f;
			bg_color.z = 0.45f;
			border_color.x = 0.3f;
			border_color.y = 0.3f;
			border_color.z = 0.3f;
		}
	}


	ui_draw_rect ((ui_vector2i_t) {position.x - 1, position.y - 1}, (ui_vector2i_t) {width + 2, height + 2}, border_color);
	ui_draw_rect (position, (ui_vector2i_t) {width, height}, bg_color);
	ui_draw_rect ((ui_vector2i_t) {position.x + 4 + (int32_t) ((float)(width - 24) * percent), position.y + 4}, (ui_vector2i_t) {16, 16}, border_color);
}

void ui_init (int32_t screen_width, int32_t screen_height, ui_create_texture_callback_t create_texture)
{
	ui_state = (ui_state_t) {0};
	ui_state.io.screen.size = (ui_vector2i_t) {screen_width, screen_height};

	ui_state.callback.create_texture = create_texture;
	if (ui_state.callback.create_texture != NULL)
	{
		uint8_t *ttf_buffer = NULL;
		uint8_t tmp_bitmap[512*512];

		FILE *f = fopen ("C:\\Windows\\Fonts\\consola.ttf", "rb");
		fseek (f, 0, SEEK_END);
		size_t file_size = ftell (f);
		fseek (f, 0, SEEK_SET);

		ttf_buffer = (uint8_t *) xmalloc (sizeof (uint8_t) * file_size);
		fread (ttf_buffer, file_size, 1, f);
		fclose (f);


		ui_font_t font = {0};
	  	stbtt_BakeFontBitmap (ttf_buffer,0, 16.0, tmp_bitmap, 512,512, 32,96, font.cdata); // no guarantee this fits!
		free (ttf_buffer);


		uint8_t *rgba_font_pixels = xmalloc (sizeof (uint8_t) * 512 * 512 * 4);
		uint32_t y, x;
		for (y = 0; y < 512; ++ y)
		{
			for (x = 0; x < 512; ++ x)
			{
				uint32_t index_rgba = (x * 4) + (y * 512 * 4);
				uint32_t index = x + (y * 512);

				rgba_font_pixels [index_rgba + 0] = tmp_bitmap [index];
				rgba_font_pixels [index_rgba + 1] = tmp_bitmap [index];
				rgba_font_pixels [index_rgba + 2] = tmp_bitmap [index];
				rgba_font_pixels [index_rgba + 3] = tmp_bitmap [index];
			}
		}

		font.texture_id = ui_state.callback.create_texture (rgba_font_pixels, 512, 512, 4);
		ui_state.font = font;
		//stbtt_FreeBitmap (tmp_bitmap, NULL);
		
		uint8_t pixels [4] = {0xFF, 0xFF, 0xFF, 0xFF};
		ui_state.no_texture = ui_state.callback.create_texture (pixels, 1, 1, 4);
	}
}


void ui_update ()
{
	uint32_t i;
	for (i = 0; i < ui_state.window_count; ++ i)
	{
		uint32_t j;
		for (j = 0; j < ui_state.windows [i].cmd_list_count; ++ j)
		{
			array_clear (ui_state.windows [i].cmd_list [j].vertices);
			array_clear (ui_state.windows [i].cmd_list [j].indices);
			ui_state.windows [i].cmd_list [j].vertex_count = 0;
			ui_state.windows [i].cmd_list [j].index_count = 0;
		}

		array_clear (ui_state.windows [i].cmd_list);
		ui_state.windows [i].cmd_list_count = 0;
	}


	ui_state.last_io	= ui_state.io;
	ui_state.hot_item	= 0;
	ui_state.hot_window	= 0;

	//ui_begin ("DEBUG");
}



