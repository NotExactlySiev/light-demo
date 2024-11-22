typedef enum {
    LIGHT_NONE,
    LIGHT_SUN,
    LIGHT_POINT,
} LightKind;

typedef struct {
    Vec vec;  // direction or position
    LightKind kind;

    Vec ambient;
    Vec diffuse;
    //Vec specular;

    // attenuation factors
    fx32 k0, k1, k2;
    
} Light;

typedef struct {
    Vec ambient;
    Vec diffuse;
    //Vec specular;
    //Vec emissive;
    //per vertex?
} Material;

void calculate_lights(Vec *out, Light *lights, int n, Vec pos);
void update_llm(Vec *lights, Mat mat);
