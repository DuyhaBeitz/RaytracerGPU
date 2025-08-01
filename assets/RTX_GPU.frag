#version 330



in vec2 fragTexCoord; // [0:1]
out vec4 FragColor;

uniform float iTime;
uniform vec2 iResolution;

// Camera
float vfov    = 90;            // Vertical view angle (field of view)
float defocus_angle = 0.3;  // Variation angle of rays through each pixel
float focus_dist = 0.5;
uniform vec3 lookfrom = vec3(0,0,0);   // Point camera is looking from
uniform vec3 lookat   = vec3(0,0,-1);  // Point camera is looking at
vec3 vup              = vec3(0,1,0);   // Camera-relative "up" direction

vec3 defocus_disk_u;
vec3 defocus_disk_v;

vec3 pixel00_loc;
vec3 pixel_delta_u;
vec3 pixel_delta_v;

#define MAX_BOUNCES 6
#define SPHERE_COUNT 13
#define PI 3.14159265359

// Material types
#define LAMBERTIAN 0
#define METAL      1
#define DIELECTRIC 2

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
/*************************************************************************************/

// Bad distribution, elongated shadows
vec3 WrongRandomUnitVec3(vec3 seed) {
    return normalize(vec3(random(seed), random(3*seed), random(7*seed)));

    while (true) {
        vec3 vec = vec3(random(seed.x), random(seed.y), random(seed.z));
        if (length(vec) <= 1.0 && length(vec) > 0.01) return normalize(vec);
        // since random is deterministic here, without offsetting the loop can go indefinitly
        seed.x += random(seed.x)-0.5;
        seed.y += random(seed.y)-0.5;
        seed.z += random(seed.z)-0.5;
    }
}

// More unifrom distribution, circle shadows
vec3 RandomUnitVec3(vec3 seed) {
    vec3 rand = WrongRandomUnitVec3(seed);
    rand.z *= -1;
    rand.x *= -1;
    rand *= 0.5;
    rand += WrongRandomUnitVec3(seed*seed)*0.5;
    return rand;
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

struct Material {
    vec3 albedo;
    int mat_id;
    float fuzz; // for metalic
    float refraction_index; // for dielectric
};

struct Sphere {
    vec3 center;
    float radius;
    Material material;
};

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct HitResult {
    vec3 hit_pos;
    vec3 normal;
    Material mat;
    float t;
    bool front_face;
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

Sphere spheres[SPHERE_COUNT];

void setupScene() {
    spheres[0] = Sphere(vec3(0.0, -1000, 0.0), 1000.0, Material(vec3(0.5, 0.5, 0.5), LAMBERTIAN, 0.0, 0.0)); // ground
    spheres[1] = Sphere(vec3(0.0, 4.0, 0.0), 4.0, Material(vec3(0.99, 0.9, 0.9), DIELECTRIC, 0.3, 1/1.3));
    spheres[2] = Sphere(vec3(9.0, 5.0, 9.0), 5.0, Material(vec3(0.8, 0.1, 0.1), METAL, 0.1, 1/1.3));

    for (int i = 3; i < SPHERE_COUNT; i++) {
        float radius = random(float(i));
        vec3 color = vec3(random(i), random(i*i), random(i*i*i));
        vec3 pos = RandomUnitVec3(vec3(i*i));
        pos.xz *= 30;
        spheres[i] = Sphere(pos, radius, Material(color, LAMBERTIAN, 0.0, 0.0));
    }
}

HitResult hit_sphere(Sphere sphere, Ray r) {
    vec3 oc = sphere.center - r.origin;
    float lr = length(r.direction);
    float loc = length(oc);
    float a = lr*lr;
    float h = dot(r.direction, oc);
    float c = loc*loc - sphere.radius*sphere.radius;
    float discriminant = h*h - a*c;

    HitResult res;
    if (discriminant < 0) {
        res.t = -1.0;
    } else {
        res.t = (h - sqrt(discriminant)) / a;
    }
    res.mat = sphere.material;
    res.hit_pos = RayAt(r, res.t);
    vec3 normal = normalize(RayAt(r, res.t) - sphere.center);
    SetFaceNormal(r, normal, res);
    return res;
}

HitResult hit_world(Ray ray, float max_t) {
    HitResult result;
    result.t = max_t; // by default no hit

    bool hit_any = false;
    for (int i = 0; i < SPHERE_COUNT; i++) {
        HitResult new_res = hit_sphere(spheres[i], ray);
        if (new_res.t >= 0.001 && new_res.t <= max_t) {
            if (new_res.t < result.t) {
                result = new_res;
            }
            hit_any = true;
        }
    }
    if (!hit_any) result.t = -1.0; // from max_t to -1.0
    return result;
}

vec3 SkyColor(Ray ray) {
    vec3 unit_direction = normalize(ray.direction);
    float a = 0.5*(unit_direction.y + 1.0);
    return (1.0-a)*vec3(1.0, 1.0, 1.0) + a*vec3(0.5, 0.7, 1.0);
}

bool Vec3NearZero(in vec3 v) {
    return abs(v.x) < 0.0001 && abs(v.y) < 0.0001 && abs(v.z) < 0.0001;
}

bool LambertianScatter(inout Ray r_in, inout HitResult rec, inout vec3 attenuation, inout Ray scattered) {
    vec3 scatter_direction = rec.normal + RandomUnitVec3(rec.hit_pos*iTime);

    // Catch degenerate scatter direction
    if (Vec3NearZero(scatter_direction)) scatter_direction = rec.normal;

    scattered = Ray(rec.hit_pos, scatter_direction);
    attenuation = rec.mat.albedo;
    return true;
}

bool MetalScatter(inout Ray r_in, inout HitResult rec, inout vec3 attenuation, inout Ray scattered) {
    vec3 reflected = reflect(r_in.direction, rec.normal);
    reflected += rec.mat.fuzz * RandomUnitVec3(rec.hit_pos*iTime);
    scattered = Ray(rec.hit_pos, reflected);
    attenuation = rec.mat.albedo;
    return dot(scattered.direction, rec.normal) > 0;
}

float reflectance(float cosine, float refraction_index) {
    // Use Schlick's approximation for reflectance.
    float r0 = (1 - refraction_index) / (1 + refraction_index);
    r0 = r0*r0;
    return r0 + (1-r0)*pow((1 - cosine),5);
}

bool DielectricScatter(inout Ray r_in, inout HitResult rec, inout vec3 attenuation, inout Ray scattered) {
    attenuation = rec.mat.albedo;
    float ri = rec.front_face ? (1.0/rec.mat.refraction_index) : rec.mat.refraction_index;

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
    
    if (rec.mat.mat_id == LAMBERTIAN) {
        return LambertianScatter(r_in, rec, attenuation, scattered);
    }
    else if (rec.mat.mat_id == METAL) {
        return MetalScatter(r_in, rec, attenuation, scattered);
    }
    else if (rec.mat.mat_id == DIELECTRIC) {
        return DielectricScatter(r_in, rec, attenuation, scattered);
    }

    return true;
}

vec3 RayColor(Ray ray) {
    vec3 color = vec3(1.0);
    for (int bounce = 0; bounce < MAX_BOUNCES; bounce++) {
        HitResult res = hit_world(ray, 100000000);
        if (res.t > 0.001) {
            // scatter ray
            Ray scattered;
            vec3 attenuation;
            if (Scatter(ray, res, attenuation, scattered)) {
                color *= attenuation;
                vec3 p = RayAt(ray, res.t);
                //vec3 random_offset = RandomUnitVec3(p*iTime);
                //ray = Ray(p, res.normal+random_offset);
                ray = scattered;
            }
            else return vec3(0.0); // ray fully absorbed
        }
        else {
            color *= SkyColor(ray);
            break;
        }
    }
    
    return color;
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

    vec3 offset = SampleSquare(pixel_center*iTime);

    vec3 ray_origin = (defocus_angle <= 0) ? lookfrom : DefocusDiskSample(pixel_center*iTime);
    vec3 ray_direction = pixel_center - ray_origin;

    return Ray(ray_origin, ray_direction);
}

void main() {
    vec2 uv = fragTexCoord / iResolution;
    uv = 2.0 * uv - 1.0;
    uv.x *= iResolution.x / iResolution.y;

    setupScene();

    Initialize();

    vec3 col = vec3(0.0);
    int samples = 4;
    for (int i = 0; i < samples; ++i) {
        vec2 offset = vec2(float(i) / float(samples), fract(sin(float(i)*13.37*iTime)));
        Ray ray = getRay(fragTexCoord+offset*0.001);
        col += RayColor(ray);
    }
    col /= float(samples);
    
    // Gamma correction
    col = pow(col, vec3(1.0 / 2.2));
    FragColor = vec4(col, 1.0);
}