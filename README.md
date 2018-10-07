#	PROFILER

##	How to use
```C
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Windows.h>
#include <assert.h>
#include <profiler/profiler.h>

void sort (int32_t *v, int32_t n)
{
	PROFILER_BEGIN ()
	int32_t i, j;
	for (i = 0; i < n - 1; ++ i)
	{
		for (j = i + 1; j < n; ++ j)
		{
			if (i != j)
			{
				if (v [i] > v [j])
				{
					int32_t tmp = v [i];
					v [i] = v [j];
					v [j] = tmp;
				}
			}
		}
	}
	PROFILER_END ()
}

void say_hello (const char *name)
{
	PROFILER_BEGIN()
	printf ("Hello %s!\n", name);
	int32_t n = 1000;
	int32_t v [1000];
	int32_t i;
	for (i = 0; i < n; ++ i)
	{
		v [i] = rand () % 10000;
	}

	sort (v, n);
	int32_t last = v [0];
	for (i = 0; i < n; ++ i)
	{
		assert (last <= v [i]);
		last = v [i];
	}
	PROFILER_END()
}

int main (int argc, const char **argv)
{
	srand (time (NULL));
	assert (profiler_init (PROFILER_TYPE_PROFILER, PROFILER_MAKE_ADDRESS (127, 0, 0, 1), 1672));

	PROFILER_BEGIN()
	int32_t i;
	for (i = 0; i < 100; ++ i)
	{
		say_hello ("World");
	}
	PROFILER_END()

	profiler_quit ();
	return (0);
}

```

## Tool
![alt text](documents/images/tool_ui.png?raw=true "Tool UI")
![alt text](documents/images/tool_ui_with_data.png?raw=true "Tool UI with data")
