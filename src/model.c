#include <string.h>
#include "kernel.h"
#include "model.h"

// TODO: move
Vec vecf_ftoq(Vecf v)
{
    return (Vec) {
        .x = ftofx(v.x),
        .y = ftofx(v.y),
        .z = ftofx(v.z),
    };
}

typedef struct [[gnu::packed]] {
    Vecf pos;
    Vecf normal;
} PlyVert;

typedef struct [[gnu::packed]] {
    uint8_t count;
    uint32_t index[];
} PlyFace;

// parses header. returns pointer to right after the header
void *model_new_ply(Model *m, void *data)
{
    char *s = data;
    if (memcmp(s, "ply\n", 4) != 0)
        return NULL;

    *m = (Model) {0};


    const char vert_tag[] = "element vertex ";
    const char face_tag[] = "element face ";
    const char data_tag[] = "end_header\n";

    const char *vert_ptr = strstr(s, vert_tag) + sizeof(vert_tag) - 1;
    const char *face_ptr = strstr(s, face_tag) + sizeof(face_tag) - 1;
    void *data_ptr = strstr(s, data_tag) + sizeof(data_tag) - 1;

    m->nverts = atoi(vert_ptr);
    m->nfaces = atoi(face_ptr);
    printf("V %d\tF %d\n", m->nverts, m->nfaces);
    return data_ptr;
}

// give it the pointer returned from the one above
int model_read_data(Model *m, void *data, fx scale)
{
    PlyVert *fverts = (PlyVert *) data;
    for (int i = 0; i < m->nverts; i++) {
        m->verts[i] = vec_scale(vecf_ftoq(fverts[i].pos), scale);
        m->normals[i] = vecf_ftoq(fverts[i].normal);
    }

    uint8_t *p = (uint8_t *) &fverts[m->nverts];
    for (int i = 0; i < m->nfaces; i++) {
        PlyFace *face = (PlyFace *) p;
        if (face->count != 3) {
            printf("Non tri polygon! %d\n", face->count);
            return -1;
        }
        (*m->faces)[i][0] = face->index[0];
        (*m->faces)[i][1] = face->index[1];
        (*m->faces)[i][2] = face->index[2];
        p += sizeof(PlyFace) + sizeof(uint32_t) * face->count;
    }
    return 0;
}
