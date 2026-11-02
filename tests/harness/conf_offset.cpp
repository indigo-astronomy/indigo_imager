// Prints where the saved solver choice is in the config file and how long the file is,
// so the tests do not hard code the conf_t layout.

#include <cstddef>
#include <cstdio>
#include <conf.h>

conf_t conf;

int main() {
	printf("%zu %zu\n", offsetof(conf_t, solver_agent_type), sizeof(conf_t));
	return 0;
}
