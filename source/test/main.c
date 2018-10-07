#include <stdio.h>
#include <Windows.h>
#include <assert.h>
#include <profiler/profiler.h>

void say_hello (const char *name)
{
	PROFILER_BEGIN()
	printf ("Hello %s!\n", name);
	Sleep (rand () % 1);
	PROFILER_END()
}

int main (int argc, const char **argv)
{
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
