#version 330


in vec2 fragTexCoord; // [0:1]
out vec4 FragColor;

uniform float iTime;
uniform vec2 iResolution;

// Camera
uniform float vfov    = 90;            // Vertical view angle (field of view)
uniform float defocus_angle = 0.3;  // Variation angle of rays through each pixel
uniform float focus_dist = 0.5;
uniform vec3 lookfrom = vec3(0,0,0);   // Point camera is looking from
uniform vec3 lookat   = vec3(0,0,-1);  // Point camera is looking at
vec3 vup              = vec3(0,1,0);   // Camera-relative "up" direction

uniform bool ionly_normals = false;

vec3 defocus_disk_u;
vec3 defocus_disk_v;

vec3 pixel00_loc;
vec3 pixel_delta_u;
vec3 pixel_delta_v;

const int MAX_BOUNCES = 8;
const int SAMPLES     = 8;
const float PI        = 3.14159265359;

const float INFINITY = 100000000.0;
const float EPSILON = 0.0001;

#define MATERIAL_COUNT 100
#define OBJECT_COUNT   18

// Material types
#define LAMBERTIAN    0
#define METAL         1
#define DIELECTRIC    2
#define DIFFUSE_LIGHT 3

// Procedural sky
#define SKY_DARK -2
#define SKY_BLUE -1

// A single iteration of Bob Jenkins' One-At-A-Time hashing algorithm.
uint hash( uint x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}

// Compound versions of the hashing algorithm I whipped together.
uint hash( uvec2 v ) { return hash( v.x ^ hash(v.y)                         ); }
uint hash( uvec3 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z)             ); }
uint hash( uvec4 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w) ); }

/*************************************************************************************/


// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct( uint m ) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat( m );       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}

// Pseudo-random value in half-open range [0:1].
float random( float x ) { return floatConstruct(hash(floatBitsToUint(x))); }
float random( vec2  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
float random( vec3  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
float random( vec4  v ) { return floatConstruct(hash(floatBitsToUint(v))); }









float interpolate(float a, float b, float c, float d, float x) {
    float p = (d - c) - (a - b);
    
    return x * (x * (x * p + ((a - b) - p)) + (c - a)) + b;
}

float sampleX(vec3 at) {
    float floored = floor(at.x);
    
    return interpolate(
        random(vec3(floored - 1.0, at.yz)),
        random(vec3(floored, at.yz)),
        random(vec3(floored + 1.0, at.yz)),
        random(vec3(floored + 2.0, at.yz)),
    	at.x - floored) * 0.5 + 0.25;
}

float sampleY(vec3 at) {
    float floored = floor(at.y);
    
    return interpolate(
        sampleX(vec3(at.x, floored - 1.0, at.z)),
        sampleX(vec3(at.x, floored, at.z)),
        sampleX(vec3(at.x, floored + 1.0, at.z)),
        sampleX(vec3(at.x, floored + 2.0, at.z)),
        at.y - floored);
}

float cubicNoise(vec3 at) {
    float floored = floor(at.z);
    
    return interpolate(
        sampleY(vec3(at.xy, floored - 1.0)),
        sampleY(vec3(at.xy, floored)),
        sampleY(vec3(at.xy, floored + 1.0)),
        sampleY(vec3(at.xy, floored + 2.0)),
        at.z - floored);
}

/*************************************************************************************/

float rand(in vec2 co) {
    return fract(sin(dot(co, vec2(12.9898,78.233))) * 43758.5453);
}

// 3D uniformly distributed random unit vector from a vec2 seed
vec3 randomUnitVector(vec2 seed) {
    float z = rand(seed * 1.0) * 2.0 - 1.0; // z ∈ [-1, 1]
    float a = rand(seed * 2.0) * 6.2831853; // angle θ ∈ [0, 2π]
    float r = sqrt(1.0 - z * z);            // radius at height z

    return vec3(r * cos(a), r * sin(a), z); // spherical to Cartesian
}

vec3 RandomUnitDisk(vec3 seed) {
    while (true) {
        vec3 p = vec3(random(seed)*2.0-1.0, random(seed)*2.0-1.0, 0);
        if (length(p) < 1.0) return p;
        seed.x += random(seed.x)-0.5;
        seed.y += random(seed.y)-0.5;
        seed.z += random(seed.z)-0.5;
    }
}



vec3 SampleSquare(vec3 seed) {
    // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
    return vec3(0.0);
    return vec3(random(seed.x) - 0.5, random(seed.y) - 0.5, 0.0);
}

vec3 DefocusDiskSample(vec3 seed) {
    // Returns a random point in the camera defocus disk.
    vec3 p = RandomUnitDisk(seed);
    return lookfrom + (p.x * defocus_disk_u) + (p.y * defocus_disk_v);
}



struct Ray {
    vec3 origin;
    vec3 direction;
};


struct HitResult {
    vec3 hit_pos;
    vec3 normal;
    int mat_id;
    float t;
    bool front_face;
    vec2 uv;
}; 

void SetFaceNormal(Ray r, vec3 outward_normal, inout HitResult hit_res) {
    // Sets the hit record normal vector.
    // NOTE: the parameter `outward_normal` is assumed to have unit length.
    hit_res.front_face = dot(r.direction, outward_normal) < 0;
    hit_res.normal = hit_res.front_face ? outward_normal : -outward_normal;
}

vec3 RayAt(Ray ray, float t) {
    return ray.origin + t*ray.direction;
}

bool Vec3NearZero(in vec3 v) {
    return abs(v.x) < 0.0000001 && abs(v.y) < 0.0000001 && abs(v.z) < 0.0000001;
}

uniform int u_sky_tex_id;
uniform vec3 u_albedo[MATERIAL_COUNT];
uniform int u_mat_type[MATERIAL_COUNT];
uniform int u_tex_id[MATERIAL_COUNT];
uniform int u_emit_tex_id[MATERIAL_COUNT];
uniform float u_fuzz[MATERIAL_COUNT];
uniform float u_refraction_index[MATERIAL_COUNT];

#define EMPTY_GEOM -1
#define SPHERE      0
#define PLANE       1
#define QUAD        2

uniform int u_geometry_type[OBJECT_COUNT];
uniform int u_mat_id[OBJECT_COUNT];
uniform vec3 u_a[OBJECT_COUNT]; // center for spheres
uniform vec3 u_b[OBJECT_COUNT]; // .x is radius for sheres
uniform vec3 u_c[OBJECT_COUNT];

// Quad/plane variables (caching to speed up)
uniform vec3 u_n[OBJECT_COUNT];
uniform float u_D[OBJECT_COUNT];
uniform vec3 u_w[OBJECT_COUNT];

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;
uniform sampler2D tex5;
uniform sampler2D tex6;
uniform sampler2D tex7;
uniform sampler2D tex8;
uniform sampler2D tex9;
uniform sampler2D tex10;
uniform sampler2D tex11;
uniform sampler2D tex12;
uniform sampler2D tex13;
uniform sampler2D tex14;
uniform sampler2D tex15;

vec4 GetTextureColor(vec2 uv, int tex_id) {
    if (tex_id == 0) return texture2D(tex0, uv);
    if (tex_id == 1) return texture2D(tex1, uv);
    if (tex_id == 2) return texture2D(tex2, uv);
    if (tex_id == 3) return texture2D(tex3, uv);
    if (tex_id == 4) return texture2D(tex4, uv);
    if (tex_id == 5) return texture2D(tex5, uv);
    if (tex_id == 6) return texture2D(tex6, uv);
    if (tex_id == 7) return texture2D(tex7, uv);
    if (tex_id == 8) return texture2D(tex8, uv);
    if (tex_id == 9) return texture2D(tex9, uv);
    if (tex_id == 10) return texture2D(tex10, uv);
    if (tex_id == 11) return texture2D(tex11, uv);
    if (tex_id == 12) return texture2D(tex12, uv);
    if (tex_id == 13) return texture2D(tex13, uv);
    if (tex_id == 14) return texture2D(tex14, uv);
    if (tex_id == 15) return texture2D(tex15, uv);    
    return texture2D(tex0, uv);
}

vec2 SphereUV(vec3 p) {
    // p: a given point on the sphere of radius one, centered at the origin.
    // p can also be outward normal for arbitrary sphere
    // u: returned value [0,1] of angle around the Y axis from X=-1.
    // v: returned value [0,1] of angle from Y=-1 to Y=+1.
    //     <1 0 0> yields <0.50 0.50>       <-1  0  0> yields <0.00 0.50>
    //     <0 1 0> yields <0.50 1.00>       < 0 -1  0> yields <0.50 0.00>
    //     <0 0 1> yields <0.25 0.50>       < 0  0 -1> yields <0.75 0.50>

    float theta = acos(-p.y);
    float phi = atan(-p.z, p.x) + PI;

    return vec2(phi / (2*PI), theta / PI);
}

vec2 EquirectangularProjectionUV(vec3 dir) {
    // Normalize the direction vector
    vec3 normalizedDir = normalize(dir);
    
    // Calculate U and V
    float u = 0.5 + atan(normalizedDir.z, normalizedDir.x) / (2.0 * PI);
    float v = 0.5 - asin(normalizedDir.y) / PI;
    
    return vec2(u, v);
}

vec3 CheckerPattern(in vec3 point, vec3 color) {
    int xInteger = int(floor(point.x));
    int yInteger = int(floor(point.y));
    int zInteger = int(floor(point.z));

    bool isEven = (xInteger + yInteger + zInteger) % 2 == 0;

    return isEven ? color : vec3(1.0);
}

vec3 MatColor(int mat_id, vec2 uv, inout vec3 point) {
    int tex_id = u_tex_id[mat_id];
    if (tex_id < 0) {
        return u_albedo[mat_id];
    }
    else if (tex_id < 16) {
        return u_albedo[mat_id] * GetTextureColor(uv, tex_id).rgb;
    }
    else return CheckerPattern(point, u_albedo[mat_id]);
}

vec3 EmitColor(int mat_id, vec2 uv, inout vec3 point) {
    int emit_tex_id = u_emit_tex_id[mat_id];
    if (emit_tex_id < 0) {
        if (u_mat_type[mat_id] == DIFFUSE_LIGHT) return u_albedo[mat_id];
        return vec3(0.0);
    }
    else {
        return GetTextureColor(uv, emit_tex_id).rgb;
    }    
    return vec3(0.0);
}


bool HitSphere(int obj_id, Ray ray, float t_min, float t_max, inout HitResult res) {

    vec3 center = u_a[obj_id];
    float radius = u_b[obj_id].x;

    vec3 oc = center - ray.origin;
    float lr = length(ray.direction);
    float loc = length(oc);
    float a = lr*lr;
    float h = dot(ray.direction, oc);
    float c = loc*loc - radius*radius;
    float discriminant = h*h - a*c;

    if (discriminant >= 0) {
        res.t = (h - sqrt(discriminant)) / a; // root with smaller t (-)
        if (res.t > t_max) return false;
        else if (res.t < t_min) {
            res.t = (h + sqrt(discriminant)) / a; // root with bigger t (+)
            if (res.t > t_max || res.t < t_min) return false;
        }
        res.mat_id = u_mat_id[obj_id];
        res.hit_pos = RayAt(ray, res.t);
        vec3 normal = normalize(RayAt(ray, res.t) - center);
        SetFaceNormal(ray, normal, res);

        res.uv = SphereUV(normal);
        return true;
    }
    else {
        return false;
    }
}

bool PlaneHit(int obj_id, Ray ray, float t_min, float t_max, inout HitResult res) {
    
    // finding plane
    vec3 Q = u_a[obj_id];
    vec3 u = u_b[obj_id];
    vec3 v = u_c[obj_id];

    vec3 normal = u_n[obj_id];
    float D = u_D[obj_id];
    vec3 w = u_w[obj_id];

    float denom = dot(normal, ray.direction);

    // No hit if the ray is parallel to the plane.
    if (abs(denom) < 1e-8) {
        return false;
    }

    // Return false if the hit point parameter t is outside the ray interval.
    float t = (D - dot(normal, ray.origin)) / denom;
    if (t < t_min || t > t_max) return false;

    // Determine if the hit point lies within the planar shape using its plane coordinates.
    vec3 intersection = RayAt(ray, t);
    vec3 planar_hitpt_vector = intersection - Q;
    float alpha = dot(w, cross(planar_hitpt_vector, v));
    float beta = dot(w, cross(u, planar_hitpt_vector));

    // Ray hits the 2D shape; set the rest of the hit record and return true.
    res.t = t;
    res.hit_pos = intersection;
    res.mat_id = u_mat_id[obj_id];;
    res.uv = vec2(alpha, beta);
    SetFaceNormal(ray, normal, res);

    return true;
}

bool QuadHit(int obj_id, Ray ray, float t_min, float t_max, inout HitResult res) {
    
    // finding plane
    vec3 Q = u_a[obj_id];
    vec3 u = u_b[obj_id];
    vec3 v = u_c[obj_id];

    vec3 normal = u_n[obj_id];
    float D = u_D[obj_id];
    vec3 w = u_w[obj_id];


    float denom = dot(normal, ray.direction);

    // No hit if the ray is parallel to the plane.
    if (abs(denom) < 1e-8) {
        return false;
    }

    // Return false if the hit point parameter t is outside the ray interval.
    float t = (D - dot(normal, ray.origin)) / denom;
    if (t < t_min || t > t_max) return false;

    // Determine if the hit point lies within the planar shape using its plane coordinates.
    vec3 intersection = RayAt(ray, t);
    vec3 planar_hitpt_vector = intersection - Q;
    float alpha = dot(w, cross(planar_hitpt_vector, v));
    float beta = dot(w, cross(u, planar_hitpt_vector));

    if (alpha < 0 || alpha > 1 || beta < 0 || beta > 1) return false;

    // Ray hits the 2D shape; set the rest of the hit record and return true.
    res.t = t;
    res.hit_pos = intersection;
    res.mat_id = u_mat_id[obj_id];;
    res.uv = vec2(alpha, beta);
    SetFaceNormal(ray, normal, res);

    return true;
}

bool AbstractHit(int obj_id, Ray ray, float t_min, float t_max, inout HitResult res) {
    if (u_geometry_type[obj_id] == EMPTY_GEOM) {}
    else if (u_geometry_type[obj_id] == SPHERE) {
        return HitSphere(obj_id, ray, t_min, t_max, res);
    }
    else if (u_geometry_type[obj_id] == QUAD) {
        return QuadHit(obj_id, ray, t_min, t_max, res);
    }
    else if (u_geometry_type[obj_id] == PLANE) {
        return PlaneHit(obj_id, ray, t_min, t_max, res);
    }
    return false;
}

bool HitWorld(Ray ray, float t_min, float t_max, inout HitResult res) {
    bool hit_any = false;
    for (int i = 0; i < OBJECT_COUNT; i++) {
        HitResult new_res;
        if (AbstractHit(i, ray, t_min, t_max, new_res)) {
            if (new_res.t < res.t || !hit_any) {
                res = new_res;
            }
            hit_any = true;
        }        
    }
    return hit_any;
}

bool LambertianScatter(inout Ray r_in, inout HitResult rec, inout vec3 attenuation, inout Ray scattered) {
    //vec3 scatter_direction = rec.normal + RandomUnitVec3(rec.hit_pos*iTime);
    //vec3 scatter_direction = rec.normal + WrongRandomUnitVec3(rec.hit_pos*iTime);
    vec3 scatter_direction = rec.normal + randomUnitVector(rec.hit_pos.xy*iTime);

    // Catch degenerate scatter direction
    if (Vec3NearZero(scatter_direction)) scatter_direction = rec.normal;

    scattered = Ray(rec.hit_pos, scatter_direction);
    attenuation = MatColor(rec.mat_id, rec.uv, rec.hit_pos);
    if (attenuation.r > 1 || attenuation.g > 1 || attenuation.b > 1) attenuation = vec3(0.0);
    return true;
}

bool MetalScatter(inout Ray r_in, inout HitResult rec, inout vec3 attenuation, inout Ray scattered) {
    vec3 reflected = reflect(r_in.direction, rec.normal);
    reflected += u_fuzz[rec.mat_id] * randomUnitVector(rec.hit_pos.xy*iTime);
    scattered = Ray(rec.hit_pos, reflected);
    attenuation = MatColor(rec.mat_id, rec.uv, rec.hit_pos);
    return dot(scattered.direction, rec.normal) > 0;
}

float reflectance(float cosine, float refraction_index) {
    // Use Schlick's approximation for reflectance.
    float r0 = (1 - refraction_index) / (1 + refraction_index);
    r0 = r0*r0;
    return r0 + (1-r0)*pow((1 - cosine),5);
}

bool DielectricScatter(inout Ray r_in, inout HitResult rec, inout vec3 attenuation, inout Ray scattered) {
    attenuation = MatColor(rec.mat_id, rec.uv, rec.hit_pos);

    float ri = rec.front_face ? (1.0/u_refraction_index[rec.mat_id]) : u_refraction_index[rec.mat_id];

    vec3 unit_direction = normalize(r_in.direction);
    float cos_theta = min(dot(-unit_direction, rec.normal), 1.0);
    float sin_theta = sqrt(1.0 - cos_theta*cos_theta);

    bool cannot_refract = ri * sin_theta > 1.0;
    vec3 direction;

    if (cannot_refract || reflectance(cos_theta, ri) > random(rec.hit_pos))
        direction = reflect(unit_direction, rec.normal);
    else
        direction = refract(unit_direction, rec.normal, ri);

    scattered = Ray(rec.hit_pos, direction);
    return true;
}

bool Scatter(inout Ray r_in, inout HitResult rec, inout vec3 attenuation, inout Ray scattered) {
    if (u_mat_type[rec.mat_id] == LAMBERTIAN) {
        return LambertianScatter(r_in, rec, attenuation, scattered);
    }
    else if (u_mat_type[rec.mat_id] == METAL) {
        return MetalScatter(r_in, rec, attenuation, scattered);
    }
    else if (u_mat_type[rec.mat_id] == DIELECTRIC) {
        return DielectricScatter(r_in, rec, attenuation, scattered);
    }
    else if (u_mat_type[rec.mat_id] == DIFFUSE_LIGHT) {
        return false;
    }
    return false;
}

vec3 SkyColor(Ray ray) {
    if (u_sky_tex_id == SKY_DARK) {
        return vec3(0.0);
    }
    if (u_sky_tex_id == SKY_BLUE) {
        vec3 unit_direction = normalize(ray.direction);
        float a = 0.5*(unit_direction.y + 1.0);
        return (1.0-a)*vec3(1.0, 1.0, 1.0) + a*vec3(0.5, 0.7, 1.0);
    }

    vec3 e = GetTextureColor(EquirectangularProjectionUV(ray.direction), u_sky_tex_id).rgb;
    return e*e; // tunemapping reversed?
}

/*
vec3 SkyEmit(Ray ray) {
    vec3 e = GetTextureColor(EquirectangularProjectionUV(ray.direction), 1).rgb;
    return e*e; // tunemapping reversed?
    //return vec3(0.0);
    // vec3 unit_direction = normalize(ray.direction);
    // const vec3 sun_direction = normalize(vec3(1.0, 1.0, 0.0)); // directly overhead

    // float alignment = dot(unit_direction, sun_direction); // cosine of angle to sun
    // float intensity = smoothstep(0.98, 1.0, alignment);    // narrow bright spot

    // return mix(vec3(0.5), vec3(1.0), intensity);
}
*/

vec3 RayColor(Ray ray) {
    vec3 color = vec3(0.0);
    vec3 T = vec3(1.0);
    for (int bounce = 0; bounce < MAX_BOUNCES; bounce++) {
        HitResult res;
        bool hit = HitWorld(ray, EPSILON, INFINITY, res);
        if (ionly_normals) {
            if (hit) return vec3(1.0);
            return vec3(0.0);
        }
        
        if (hit) {
            // scatter ray
            Ray scattered;
            vec3 attenuation;
            if (Scatter(ray, res, attenuation, scattered)) {
                ray = scattered;
                color += T*EmitColor(res.mat_id, res.uv, res.hit_pos);
                T *= attenuation;
                //color += EmitColor(res.mat_id, res.uv, res.hit_pos);
            }
            else {
                color += EmitColor(res.mat_id, res.uv, res.hit_pos);
                break;
            }
        }
        else {
            vec3 sky = SkyColor(ray);
            return T*sky + sky;  // Faster cuz sky col = sky emit rn
            //return T*color*SkyColor(ray) + SkyEmit(ray);


            //return SkyColor(ray);
            // color += SkyEmit(ray);
            // T *= SkyColor(ray);
            // break;
        }
    }
    
    return T*color;
    //return color;
}



void Initialize() {  
    // Determine viewport dimensions.
    float theta = radians(vfov);
    float h = tan(theta/2);
    float viewport_height = 2 * h * focus_dist;
    float viewport_width = viewport_height * iResolution.x/iResolution.y;

    // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
    vec3 w = normalize(lookfrom - lookat);
    vec3 u = normalize(cross(vup, w));
    vec3 v = cross(w, u);


    // Calculate the vectors across the horizontal and down the vertical viewport edges.
    vec3 viewport_u = viewport_width * u;    // Vector across viewport horizontal edge
    vec3 viewport_v = viewport_height * -v;  // Vector down viewport vertical edge

    // Calculate the horizontal and vertical delta vectors from pixel to pixel.
    pixel_delta_u = viewport_u / iResolution.x;
    pixel_delta_v = viewport_v / iResolution.y;

    // Calculate the location of the upper left pixel.
    vec3 viewport_upper_left = lookfrom - (focus_dist * w) - viewport_u/2 - viewport_v/2;
    pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

    // Calculate the camera defocus disk basis vectors.
    float defocus_radius = focus_dist * tan(radians(defocus_angle / 2));
    defocus_disk_u = u * defocus_radius;
    defocus_disk_v = v * defocus_radius;
}

// Camera
Ray getRay(vec2 viewport_coord) {
    vec3 pixel_center = pixel00_loc + (viewport_coord.x * pixel_delta_u * iResolution.x) + (viewport_coord.y * pixel_delta_v * iResolution.y);
    vec3 ray_origin = (defocus_angle <= 0) ? lookfrom : DefocusDiskSample(pixel_center*iTime);
    vec3 ray_direction = pixel_center - ray_origin;

    return Ray(ray_origin, ray_direction);
}


void main() {
    Initialize();

    vec3 col = vec3(0.0);

    for (int i = 0; i < SAMPLES; ++i) {
        vec2 offset = vec2(float(i) / float(SAMPLES), fract(sin(float(i)*13.37*iTime)));
        Ray ray = getRay(fragTexCoord+offset*EPSILON);
        col += RayColor(ray);
    }
    col /= float(SAMPLES);

    
    // Gamma correction
    col = pow(col, vec3(1.0 / 2.0));
    FragColor = vec4(col, 1.0);

    // test texture

    //FragColor = vec4(texture2D(tex1, fragTexCoord).a);
    //FragColor = texture2D(tex0, fragTexCoord);


    // test noise

    // float a = cubicNoise(vec3(fragTexCoord*100, 0.0));
    // FragColor = vec4(vec3(a), 1.0);
}