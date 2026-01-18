#include <stdio.h>
#include <stdlib.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void RGB2Grayscale(unsigned char *src, unsigned char *dest, int w, int h)
{
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            dest[(y * w + x)] = (src[(y * w + x) * 3 + 0] + src[(y * w + x) * 3 + 1] + src[(y * w + x) * 3 + 2]) / 3;
        }
    }
}

void Grayscale2BlackandWhite(unsigned char *src, unsigned char *dest, int w, int h, int threshold)
{
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            dest[(y * w + x)] = src[(y * w + x)] > threshold ? 255 : 0;
        }
    }
}

int* init_parent(int w, int h){
    int* parent = malloc((w * h)*sizeof(int));
    for (int i = 0; i < w * h; i++)
    {
        parent[i] = i;
    }
    return parent;
}
int find_(int* parent, int x)
{
    if (parent[x] != x) parent[x] = find_(parent, parent[x]);
    return parent[x];
}

void union_(int* parent, int x, int y)
{
    parent[find_(parent, y)] = find_(parent, x);
}
void CCL(unsigned char *src, unsigned char *dest, int w, int h)
{
    int* parent = init_parent(w,h);
    int background = 255;
    int noLabel = 0;
    int* labels = malloc((w * h)*sizeof(int));
    for(int i=0; i<w*h; i++){
        labels[i] = 0;
    }
    int label = 1;
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            unsigned char cell = src[(y * w + x)];
            if (cell != background)
            {
                int left = (x > 0) ? labels[y * w + (x - 1)] : noLabel;
                int up = (y > 0) ? labels[(y - 1) * w + x] : noLabel;

                // get labels of neighbour cells (4-connectivity)
                if (left == noLabel && up == noLabel)
                {
                    // not connected to any label, set a new label
                    labels[(y * w + x)] = label;
                    label++;
                }
                else if (left != noLabel && up == noLabel)
                {
                    // connected to only one neighbour, get his label
                    labels[(y * w + x)] = left;
                }
                else if (left == noLabel && up != noLabel)
                {
                    // connected to only one neighbour, get his label
                    labels[(y * w + x)] = up;
                }
                else
                {
                    // connected to both neighbours, set to the lowest label
                    labels[(y * w + x)] = MIN(up, left);
                    // they are equivalent, add it the equivalent list
                    union_(parent, left, up);
                }
            }
        }
    }

    // second pass
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            unsigned char cell = src[(y * w + x)];
            if (cell != background)
            {
                labels[(y * w + x)] = find_(parent, labels[(y * w + x)]);
                // Color the regions with random colors
                int labelValue = labels[(y * w + x)];
                // Use label as seed for consistent colors per component
                unsigned char r = (labelValue * 73) % 256;
                unsigned char g = (labelValue * 151) % 256;
                unsigned char b = (labelValue * 223) % 256;
                dest[(y * w + x) * 3 + 0] = r;
                dest[(y * w + x) * 3 + 1] = g;
                dest[(y * w + x) * 3 + 2] = b;
            }else{
                dest[(y * w + x) * 3 + 0] = background;
                dest[(y * w + x) * 3 + 1] = background;
                dest[(y * w + x) * 3 + 2] = background;
            }
        }
    }
    free(parent);
    free(labels);
}

int main(int argc, char** argv)
{
    if(argc != 3){
        printf("Usage: ccl.exe <path-to-img> <threashold>\n");
        return 0;
    }
    int width, height, channels;

    unsigned char *data = stbi_load(
        argv[1], 
        &width,          
        &height,          
        &channels,        
        3                
    );

    if (!data)
    {
        printf("Failed to load image: %s\n", stbi_failure_reason());
        return 1;
    }

    printf("Loaded image: %dx%d, channels: %d\n", width, height, channels);

    unsigned char* grayscaled = malloc(height * width * sizeof(unsigned char));
    RGB2Grayscale(data, grayscaled, width, height);

    int success = stbi_write_png(
        "grayscale.png", 
        width,
        height,
        1,
        grayscaled,
        width 
    );

    if (!success)
        printf("Failed to save image\n");
    else
        printf("Saved grayscale.png\n");

    unsigned char* blackAndWhite = malloc(height * width * sizeof(unsigned char));
    int threashold = atoi(argv[2]);
    Grayscale2BlackandWhite(grayscaled, blackAndWhite, width, height, threashold);

    success = stbi_write_png(
        "blackAndWhite.png", 
        width,
        height,
        1, 
        blackAndWhite,
        width 
    );

    if (!success)
        printf("Failed to save image\n");
    else
        printf("Saved blackAndWhite.png\n");

    unsigned char* labeled = malloc(height * width * 3 * sizeof(unsigned char));
    CCL(blackAndWhite, labeled, width, height);

    success = stbi_write_png(
        "labeled.png", 
        width,
        height,
        3, 
        labeled,
        width * 3
    );

    if (!success)
        printf("Failed to save image\n");
    else
        printf("Saved labeled.png\n");

    stbi_image_free(data);
    free(grayscaled);
    free(blackAndWhite);
    free(labeled);
    return 0;
}
