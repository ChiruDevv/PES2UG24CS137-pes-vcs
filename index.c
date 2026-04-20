cat > index.c << 'EOF'
#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int index_load(Index *index) {
    FILE *f = fopen(".pes/index", "r");
    if (!f) {
        index->count = 0;
        return 0;
    }

    index->count = 0;
    char hash[65];

    while (fscanf(f, "%o %64s %ld %ld %s",
        &index->entries[index->count].mode,
        hash,
        &index->entries[index->count].mtime_sec,
        &index->entries[index->count].size,
        index->entries[index->count].path) == 5) {

        hex_to_hash(hash, &index->entries[index->count].hash);
        index->count++;
    }

    fclose(f);
    return 0;
}

int index_save(const Index *index) {
    FILE *f = fopen(".pes/index.tmp", "w");
    if (!f) return -1;

    for (int i = 0; i < index->count; i++) {
        char hex[65];
        hash_to_hex(&index->entries[i].hash, hex);

        fprintf(f, "%o %s %ld %ld %s\n",
            index->entries[i].mode,
            hex,
            index->entries[i].mtime_sec,
            index->entries[i].size,
            index->entries[i].path);
    }

    fclose(f);
    rename(".pes/index.tmp", ".pes/index");
    return 0;
}

int index_add(Index *index, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);

    char *buf = malloc(size);
    fread(buf, 1, size, f);
    fclose(f);

    ObjectID id;
    object_write(OBJ_BLOB, buf, size, &id);

    IndexEntry *e = index_find(index, path);
    if (!e) {
        e = &index->entries[index->count++];
    }

    strcpy(e->path, path);
    e->hash = id;
    e->size = size;
    e->mode = 0100644;
    e->mtime_sec = time(NULL);

    free(buf);
    return index_save(index);
}
EOF
