#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void *lib_ptr = NULL;
bool (*is_global_id_enabled_next)(void *db, unsigned id);

void do_init() {
	if (lib_ptr != NULL)
		return;
	lib_ptr = dlopen("libddb_dev.so", RTLD_LOCAL | RTLD_LAZY);
	if (lib_ptr == NULL) {
		fprintf(stderr, "failed to open libddb_dev.so\n");
		exit(1);
	}
	is_global_id_enabled_next = dlsym(lib_ptr, "_ZNK12DEV_DIE_INFO20is_global_id_enabledEj");
	if (is_global_id_enabled_next == NULL) {
		fprintf(stderr, "failed to dlsym _ZNK12DEV_DIE_INFO20is_global_id_enabledEj\n");
		exit(1);
	}
}

bool _ZNK12DEV_DIE_INFO20is_global_id_enabledEj(void *db, unsigned id) {
	do_init();
	if (id == 1174964) // global id of forbidden PCIe block
		return 1;
	bool next_result = is_global_id_enabled_next(db, id);
	if (!next_result) // something else disabled, print it for interest's sake
		fprintf(stderr, "disabled %u\n", id);
	return next_result;
}
