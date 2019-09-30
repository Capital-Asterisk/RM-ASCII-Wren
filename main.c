/* 
 * RM-ASCII-Wren
 * Text mode Ray-marcher thrown together in a few hours, after getting bored in
 * an introductory C programming class.
 *
 * Copyright (C) 201]9 Neal Nicdao
 * 
 * Licensed under the Holy Open Software License of the Computer Gods V1
 * 
 * This software comes with ABSOLUTELY NO WARANTEEEEE. By plagiarizing the
 * work, you agree to the HIGHEST POSSIBLE PUNISHMENT from the the might of the
 * COMPUTER GODS through rolling the DICE.
 */

#include <stdio.h> 
#include <string.h> 
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>


#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

// The vector3 used everywhere
typedef float vec3[3];

typedef struct Cameras
{
    vec3 pos;
    vec3 center;
    vec3 up;
} Camera;

void sleepy(int ms)
{
    #ifdef _WIN32
        //Sleep(pollingDelay);
    #else
        usleep(ms * 1000);
    #endif
}

// A bunch of self-explanatory vector functions

// Vector Cross product
void vcross(const vec3 a, const vec3 b, vec3 out)
{
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

// Vector Addition "a + b = out"
void vadd(const vec3 a, const vec3 b, vec3 out)
{
    out[0] = a[0] + b[0];
    out[1] = a[1] + b[1];
    out[2] = a[2] + b[2];
}

// Vector Subtraction "a - b = out"
void vsub(const vec3 a, const vec3 b, vec3 out)
{
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

// Vector magnitude
float vmag(const vec3 a)
{
    return sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
}

// Vector Set "out = in"
void vset(const vec3 in, vec3 out)
{
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
}

// Vector set components "out = (x, y, z)"
void vsetc(float x, float y, float z, vec3 out)
{
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

// Vector scale "a * (scalar b) = out"
void vscale(vec3 a, float b, vec3 out)
{
    out[0] = a[0] * b;
    out[1] = a[1] * b;
    out[2] = a[2] * b;
}

// Vector normalize "out = in / magnitude(in)"
void vnormalize(vec3 in, vec3 out)
{
    vscale(in, 1.0f / vmag(in), out);
}

// the real stuff

int g_resolution[2];
int g_maxMarches;
unsigned long int g_time;
char* g_screenBuffer;

Camera g_camera;


// "colours" used
const char gc_pal[] = {'#', '=', '+', '"', '`'};

// A pseudo pseudo random number generator
int pprng(int i)
{
// a bunch of randomness to make some randomness
    return i * 0xFFFa * 3 + 2511223132121
            + 123123213 + i + i * 231 + i * 23423423423;
}

// Closest distance between a point and a sphere's surface
float distance_sphere(const vec3 from, const vec3 sphere, float radius)
{
    vec3 mcSub;
    vsub(sphere, from, mcSub);
    return vmag(mcSub) - radius;
}


// A bad idea
float draw_atom(const vec3 from, const vec3 pos, int nucleous, int electrons)
{
    float dist = 9999999;

    // Draw protons and neutrons
    for (int i = 0; i < nucleous; i ++)
    {
        vec3 nukPos;
        vsetc((float)(pprng(i) % 256) / 256.0f,
        (float)(pprng(i + 4) % 256) / 256.0f,
        (float)(pprng(i - 2) % 256) / 256.0f, nukPos);
        vadd(pos, nukPos, nukPos);
        dist = fmin(dist, distance_sphere(from, nukPos, 1));
    }

    // Draw electrons

    return dist;
}

float draw_newton_cradle(const vec3 from, const vec3 pos,
                         const int ballz, const int period)
{
    //int period = 2000; // each ball swings for one second
    float timeP = sin((float)(g_time % period) / (float)period * M_PI * 2);
    timeP *= 2;

    float dist = 9999999;

    // draw the inner not-moving ballz
    for (int i = 0; i < ballz - 2; i ++)
    {
        vec3 nukPos;
        // Line them up along the X-axis, and swng very slightly
        vsetc(2 + 2.0f * i + sin(timeP * 0.5f - M_PI) * 0.1,
            0,
            0,
            nukPos);
        vadd(pos, nukPos, nukPos);
        dist = fmin(dist, distance_sphere(from, nukPos, 1));
    }



    vec3 nukPosA;
    vsetc(sin((timeP * 0.5f) * (timeP > 0) - M_PI) * 4,
          cos((timeP * 0.5f) * (timeP > 0) - M_PI) * 4 + 4,
          0, nukPosA);
    vadd(pos, nukPosA, nukPosA);
    dist = fmin(dist, distance_sphere(from, nukPosA, 1));

    vec3 nukPosB;
    vsetc(sin((timeP * 0.5f) * (timeP < 0) - M_PI) * 4 + (ballz - 1) * 2.0f,
          cos((timeP * 0.5f) * (timeP < 0) - M_PI) * 4 + 4,
          0, nukPosB);
    vadd(pos, nukPosB, nukPosB);
    dist = fmin(dist, distance_sphere(from, nukPosB, 1));
}



// return number of bounces
int march_ray(const vec3 pos, const vec3 dir)
{
    vec3 spherePosA;
    vsetc(-4, 0, 0, spherePosA);

    vec3 currentPos;
    vset(pos, currentPos);

    for (int i = 0; i < g_maxMarches; i++)
    {
        float distance = 9999999;

        distance = fmin(distance, draw_newton_cradle(currentPos,
                           spherePosA, 5, 1000));

        distance = fmin(distance, currentPos[1] + 1);


        vec3 step;
        vset(dir, step);
        vscale(step, distance, step);
        vadd(currentPos, step, currentPos);

        //printf("DistanceTosphere%i,%f\n", i, distance);

        if (distance < 0.01)
        {
            //printf("moist\n");
            return i;
        }
    }
    return -1;
}

int main()
{

    // Set a few magic numbers for the time being, maybe use arguments

    g_resolution[0] = 80;
    g_resolution[1] = 25;

    g_maxMarches = 30;

    float fov = 45;
    float charDimRatio = 12.0f / 8.0f; // assume each character is 12x8 pixels

    float displaceChar = 0;

    // Allocate screen buffer
    g_screenBuffer = malloc(sizeof(char) * (g_resolution[0] + 1)
             * g_resolution[1] + 1);

    memset(g_screenBuffer, '-', sizeof(char) * (g_resolution[0] + 1)
                 * g_resolution[1] + 1);

    // Put a null terminator at the end
    g_screenBuffer[(g_resolution[0] + 1) * g_resolution[1]] = 0;

    // Set some default camera stuff
    //vsetc(0, 0, -10, g_camera.pos);
    //vsetc(0, 0, 1, g_camera.center);
    //vsetc(0, 1, 0, g_camera.up);

    // Start timing
    struct timeval timeStart;
    struct timeval timeNow;

    gettimeofday(&timeStart, 0);

    while (1)
    {

        // Rotate camera

        float zoom = 5 + sin((float)(g_time % 30000) / 30000.0f * M_PI * 2);

        // Rotate camera position around the origin, and wave up and down
        vsetc(cos((float)(g_time % 12000) / 12000.0f * M_PI * 2) * zoom,
              sin((float)(g_time % 8000) / 8000.0f * M_PI * 2) * 2 + 2,
              sin((float)(g_time % 12000) / 12000.0f * M_PI * 2) * zoom,
              g_camera.pos);

        // Set up vector to up, so that the camera doesn't roll around
        vsetc(0, 1, 0, g_camera.up);

        // Point the camera at the center. Dir is set to -position
        // and normalized
        // This results in a normalized vector pointed at the origin
        vscale(g_camera.pos, -1, g_camera.center);
        vnormalize(g_camera.center, g_camera.center);

        // Normalized vector pointing directly right of the camera
        // Calculated using cross product between camera center and up.
        // Center and up vectors will not be 90 degrees of each other, since up
        // points directly (0, 1, 0) right now, so it's important to normalize
        vec3 cameraRight;
        vcross(g_camera.center, g_camera.up, cameraRight);
        vnormalize(cameraRight, cameraRight);

        // Recalculate camera up to account for the camera's pitch.
        // If you look straight down, your 'up' will be horizontal
        vcross(cameraRight, g_camera.center, g_camera.up);
        vnormalize(g_camera.up, g_camera.up);

        // Fill the buffer thing with stuff, loop through every pixel
        for (int y = 0; y < g_resolution[1]; y++)
        {
            for (int x = 0; x < g_resolution[0]; x++)
            {
                // Calculate pixel ray
                vec3 dir; // Direction to march rays
                vec3 dirMod; // vector for intermediate calculations
                vset(g_camera.center, dir);

                // Displace the ray left and right
                // by scaling and adding cameraRight
                vset(cameraRight, dirMod);
                vscale(dirMod, (float)(x - g_resolution[0] / 2) * 0.03f,
                       dirMod);
                vadd(dirMod, dir, dir);

                // Displace the ray up and down
                // by scaling and adding camera up
                vset(g_camera.up, dirMod);
                vscale(dirMod, (float)(y - g_resolution[1] / 2) * -0.05f,
                       dirMod);
                vadd(dirMod, dir, dir);

                //dir[0] += (float)(x - g_resolution[0] / 2) * 0.03;
                //dir[1] -= (float)(y - g_resolution[1] / 2) * 0.05;
                //vscale(dir, 1.0 / vmag(dir), dir);

                // Normalize since the vector is slightly longer than 1.0
                vnormalize(dir, dir);

                // Do the raymarch
                // march_ray returns the size of number of marches
                int brightness = march_ray(g_camera.pos, dir);		
                char c;

                // Maximum steps exceeded
                if (brightness != -1)
                {
                    // pick a colour to print
                    c = gc_pal[brightness * sizeof(gc_pal) / sizeof(char)
                        / g_maxMarches];
                }
                else
                {
                    // -1 means max marches exceeded, aka: pointed at the sky
                    c = ' ';
                }

                // Set current pixel to that calculated value
                g_screenBuffer[y * (g_resolution[0] + 1) + x] = c;
            }

            // Add newlines after each line. this can be done in initialization
            // but i'm too lazy
            g_screenBuffer[(y + 1) * (g_resolution[0] + 1) - 1] = '\n';
        }

        // Calculate Elasped Time.

        gettimeofday(&timeNow, 0);
        g_time = (timeNow.tv_sec - timeStart.tv_sec) * 1000.0f
        + (timeNow.tv_usec - timeStart.tv_usec) / 1000.0f;

        // Clear screen and draw buffer

        printf("\e[1;1H\e[2J%s", g_screenBuffer);
        //sleepy(10);
    }
}
