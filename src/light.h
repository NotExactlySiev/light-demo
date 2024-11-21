typedef enum {
    LIGHT_NONE,
    LIGHT_SUN,
    LIGHT_POINT,
} LightKind;

typedef struct {
    Vec vec;  // direction or position
    Vec color;
    fx power;
    LightKind kind;
} Light;

void calculate_lights(Vec *out, Light *lights, int n, Vec pos);
void update_llm(Vec *lights, Mat mat);
