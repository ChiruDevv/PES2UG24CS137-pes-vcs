cat > object.c << 'EOF'
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/evp.h>

// PROVIDED functions remain same...

int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out) {
    const char *type_str;
    if (type == OBJ_BLOB) type_str = "blob";
    else if (type == OBJ_TREE) type_str = "tree";
    else if (type == OBJ_COMMIT) type_str = "commit";
    else return -1;

    char header[64];
    int header_len = snprintf(header, sizeof(header), "%s %zu", type_str, len) + 1;

    size_t total_len = header_len + len;
    char *full = malloc(total_len);
    memcpy(full, header, header_len);
    memcpy(full + header_len, data, len);

    compute_hash(full, total_len, id_out);

    if (object_exists(id_out)) {
        free(full);
        return 0;
    }

    char path[512];
    object_path(id_out, path, sizeof(path));

    char dir[512];
    strncpy(dir, path, sizeof(dir));
    char *slash = strrchr(dir, '/');
    *slash = '\0';

    mkdir(dir, 0755);

    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s/tmpXXXXXX", dir);
    int fd = mkstemp(tmp);

    write(fd, full, total_len);
    fsync(fd);
    close(fd);

    rename(tmp, path);

    free(full);
    return 0;
}

int object_read(const ObjectID *id, ObjectType *type_out, void **data_out, size_t *len_out) {
    char path[512];
    object_path(id, path, sizeof(path));

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);

    char *buf = malloc(size);
    fread(buf, 1, size, f);
    fclose(f);

    char *nul = memchr(buf, '\0', size);
    char type[16];
    size_t len;

    sscanf(buf, "%s %zu", type, &len);

    if (strcmp(type, "blob") == 0) *type_out = OBJ_BLOB;
    else if (strcmp(type, "tree") == 0) *type_out = OBJ_TREE;
    else if (strcmp(type, "commit") == 0) *type_out = OBJ_COMMIT;

    ObjectID check;
    compute_hash(buf, size, &check);
    if (memcmp(&check, id, sizeof(ObjectID)) != 0) return -1;

    *data_out = malloc(len);
    memcpy(*data_out, nul + 1, len);
    *len_out = len;

    free(buf);
    return 0;
}
EOF
