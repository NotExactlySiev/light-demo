#include <string.h>
#include "model.h"

//int ftoq(float);

/*
typedef struct {
    float x,y,z;
} Vecf;
*/

Vec vecf_ftoq(Vecf v)
{
    return (Vec) {
        .x = ftofx(v.x),
        .y = ftofx(v.y),
        .z = ftofx(v.z),
    };
}

typedef struct {
    Vecf pos;
    Vecf normal;
} __attribute__((packed)) PlyVert;

typedef struct {
    uint8_t count;
    uint32_t index[];
} __attribute__((packed)) PlyFace;

// parses header. returns pointer to right after the header
void *model_new_ply(Model *m, void *data)
{
    char *s = data;
    if (memcmp(s, "ply\n", 4) != 0)
        return -1;

    *m = (Model) {0};


    const char vert_tag[] = "element vertex ";
    const char face_tag[] = "element face ";
    const char data_tag[] = "end_header\n";

    const char *vert_ptr = strstr(s, vert_tag) + sizeof(vert_tag) - 1;
    const char *face_ptr = strstr(s, face_tag) + sizeof(face_tag) - 1;
    void *data_ptr = strstr(s, data_tag) + sizeof(data_tag) - 1;


    m->nverts = atoi(vert_ptr);
    m->nfaces = atoi(face_ptr);
    printf("%d\n", m->nverts);
    printf("%d\n", m->nfaces);

    return data_ptr;
}

// TODO: we should be able to scale the model here (scale factor parameter)
// give it the pointer returned from the one above
int model_read_data(Model *m, void *data)
{
    PlyVert *fverts = (PlyVert *) data;
    for (int i = 0; i < m->nverts; i++) {
        m->verts[i] = vec_scale(vecf_ftoq(fverts[i].pos), FX(ONE/14)); // BAD
        m->normals[i] = vecf_ftoq(fverts[i].normal);
        vec_print(m->verts[i]);
    }

    PlyFace *plyfaces = (PlyFace *) &fverts[m->nverts];
    uint8_t *p = &plyfaces[0];
    for (int i = 0; i < m->nfaces; i++) {
        PlyFace *face = p;
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
