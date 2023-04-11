# Short walkthrough on how to enable second, hidden PCIe HIP on 5SGSMD5K1F40C1 (5SGSKF40I3LNAC)

1. Create a file named `preload_me.c` with following content:

```c
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
```

2. Compile it to a `.so` file:

```
gcc -shared -o preload_me.so -fPIC preload_me.c
```

3. Launch Quartus with `LD_PRELOAD` to enable second HIP:

```
LD_PRELOAD=`pwd`/preload_me.so /path_to_quartus/intelFPGA/version/quartus/bin/quartus
```

4. Instantiate a second PCIe HIP in the design; it'll be placed in the right spot based on the reference clock I/O contstraint
