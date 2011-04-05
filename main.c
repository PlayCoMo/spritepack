#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <png.h>
#include <ftw.h>
#include <assert.h>

#include "img.h"
#include "pack.h"

#define DEFAULT_MAX_SIZE 1024
#define DEFAULT_BORDER_WIDTH 4

static unsigned int max_size = 0;
static unsigned int border_width = 0;

static int pngCount = 0;
static img_t ** images = NULL;
static int imageIndex = 0;


/* image_comparator - a qsort-compatible comparison function for img_t*'s
 * Sorts first by width, then height */
int image_comparator(const void* a, const void* b) {
    img_t* aimg = *(img_t**)a;
    img_t* bimg = *(img_t**)b;
    
    if (aimg->w < bimg->w)
        return -1;
    else if (aimg->w > bimg->w)
        return 1;
    else {
        if (aimg->h < bimg->h)
            return -1;
        if (aimg->h > bimg->h)
            return 1;
        else
            return 0;
    }
}

int filename_comparator(const void* a, const void* b) {
    img_t* aimg = *(img_t**)a;
    img_t* bimg = *(img_t**)b;
    
    return strcmp(aimg->filename, bimg->filename);
}

img_t* load_png(char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "ERROR: Could not open input file %s\n", filename);
        exit(-1);
    }
    
    png_byte pngsig[8];
    fread(pngsig, 1, 8, fp);
    int is_png = !png_sig_cmp(pngsig, 0, 8);
    if (!is_png) {
        fprintf(stderr, "ERROR: File %s is not a PNG!\n", filename);
        fclose(fp);
        exit(-1);
    }
    
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (!png_ptr) {
        fprintf(stderr, "ERROR: Could not open input file %s\n", filename);
        fclose(fp);
        exit(-1);
    }
    
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, 0, 0);
        fprintf(stderr, "ERROR: Could not open input file %s\n", filename);
        fclose(fp);
        exit(-1);
    }
    
    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        fprintf(stderr, "ERROR: Could not open input file %s\n", filename);
        fclose(fp);
        exit(-1);
    }
    
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fprintf(stderr, "ERROR: Could not open input file %s\n", filename);
        fclose(fp);
        exit(-1);
    }
    
    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);
    
    png_uint_32 width, height;
    int bit_depth, color_type;
    
    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height,
                 &bit_depth, &color_type, NULL, NULL, NULL);
    
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_expand (png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand (png_ptr);
    if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_expand (png_ptr);
    if (bit_depth == 16)
        png_set_strip_16 (png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb (png_ptr);
    if (color_type == PNG_COLOR_TYPE_RGB)
        png_set_add_alpha (png_ptr, 0xFF, PNG_FILLER_AFTER);
    
    png_read_update_info (png_ptr, info_ptr);
    png_get_IHDR (png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
                  NULL, NULL, NULL);
    
    unsigned row_bytes = png_get_rowbytes (png_ptr, info_ptr);
    
    unsigned char* png_pixels = malloc (row_bytes * height * sizeof (png_byte));
    unsigned char** ret = malloc(height * sizeof (png_bytep));
    unsigned i;
    for (i = 0; i < (height); i++)
        ret[i] = png_pixels + i * row_bytes;
    
    png_read_image (png_ptr, ret);
    png_read_end (png_ptr, info_ptr);
    fclose(fp);
    
    img_t* image = malloc(sizeof(img_t));
    image->pixels = ret;
    image->top  = 0;
    image->left = 0;
    image->w = width;
    image->h = height;
    image->pixel_width  = width;
    image->pixel_height = height;
    image->center_x = width / 2;
    image->center_y = height / 2;
    image->filename = strdup(filename);
    
    return image;
}

void write_png(char* filename, unsigned w, unsigned h,
               unsigned char** data, png_text* comments) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "ERROR: Could not open output file %s\n", filename);
        exit(-1);
    }
    
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (!png_ptr) {
        fprintf(stderr, "ERROR: Could not open output file %s\n", filename);
        fclose(fp);
        exit(-1);
    }
    
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, 0, 0);
        fprintf(stderr, "ERROR: Could not open output file %s\n", filename);
        fclose(fp);
        exit(-1);
    }
    
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fprintf(stderr, "ERROR: Could not open output file %s\n", filename);
        fclose(fp);
        exit(-1);
    }
    
    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, w, h, 8, PNG_COLOR_TYPE_RGB_ALPHA,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_set_text(png_ptr, info_ptr, comments, 1);
    
    png_set_rows(png_ptr, info_ptr, data);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    
    fclose(fp);
}





void create_sprite_sheet(img_t ** sub_images, const unsigned int sub_image_count, 
                         char * output_file_name)
{
    int i = 0;
    
    /* Create a proxy array of rectangles, which pack_rects will use as input.
     * Note that the indexing matches the indexing into the images array for
     * easy correspondence. */
    unsigned biggest_width = 0;
    unsigned width_sum = 0;
    unsigned* rects = malloc(2 * (sub_image_count) * sizeof(unsigned));
    for (i = 0; i < sub_image_count; ++i) {
        rects[2*i] = sub_images[i]->w;
        rects[2*i+1] = sub_images[i]->h;
        if (sub_images[i]->w > biggest_width) 
        {
            biggest_width = sub_images[i]->w;
        }
        width_sum += sub_images[i]->w;
    }
    
    if (width_sum > max_size) width_sum = max_size;
    
    /* Explore all valid widths to find the most efficient packing. */
    unsigned best_MP = 0, max_x, max_y;
    unsigned* ret = 0;
    unsigned curr_width;
    for (curr_width = biggest_width; curr_width <= width_sum; ++curr_width) {
        unsigned curr_height;
        unsigned* new_ret = pack_rects(rects, sub_image_count, curr_width, &curr_height);
        
        if (curr_height > max_size) {
            free(new_ret);
            continue;
        }
        
        if (best_MP == 0 || curr_width * curr_height < best_MP) {
            max_x = curr_width;
            max_y = curr_height;
            ret = new_ret;
            best_MP = curr_width * curr_height;
        } else
            free(new_ret);
    }
    
    free(rects);
    
    // A packing could NOT be found for these images.
    if (ret == NULL)
    {
        fprintf(stderr, "ERROR: Could not find a valid packing configuration for the following files:\n");
        for (i = 0; i < sub_image_count; ++i) {
            fprintf(stderr, "\t%s\n", sub_images[i]->filename);
        }
        return;
    }
    
    // Copy the data returned by the packer back into the img_t structures.
    for (i = 0; i < sub_image_count; ++i) {
        sub_images[i]->offset_x = ret[2*i];
        sub_images[i]->offset_y = max_y - ret[2*i+1] - sub_images[i]->h;
        
        sub_images[i]->offset_x += border_width;
        sub_images[i]->offset_y += border_width;
    }
    free(ret);
    
    // Sort the images by filename
    qsort(sub_images, sub_image_count, sizeof(img_t*), filename_comparator);
    
    /* Spew input image offsets */
    png_text comments;
    comments.compression = PNG_TEXT_COMPRESSION_zTXt;
    comments.key = "sprite";
    
    
    // Setup the comments string for printf'ing.
    unsigned text_space = 2 * (sub_image_count+1) + 1;
    comments.text = malloc(text_space);
    comments.text[0] = 0;
    
    char* num_sprites;
    asprintf(&num_sprites, "%d", sub_image_count);
    while (strlen(comments.text) + strlen(num_sprites) + 1 > text_space) {
        text_space *= 2;
        comments.text = realloc(comments.text, text_space);
    }
    comments.text = strncat(comments.text, num_sprites, strlen(num_sprites));
    
    for (i = 0; i < sub_image_count; ++i) {
        char* output;
        asprintf(&output, ", %d, %d, %d, %d, %d, %d",
                 /* top left corner */     sub_images[i]->offset_x, sub_images[i]->offset_y,
                 /* bottom right corner */ sub_images[i]->offset_x + sub_images[i]->w,  sub_images[i]->offset_y + sub_images[i]->h,
                 /* center offset */       sub_images[i]->center_x, sub_images[i]->center_y);
        
        while (strlen(comments.text) + strlen(output) + 1 > text_space) {
            text_space *= 2;
            comments.text = realloc(comments.text, text_space);
        }
        comments.text = strncat(comments.text, output, strlen(output));
    }
    
    comments.text_length = strlen(comments.text);
    
    /* Allocate the output image */
    unsigned char** out_image = malloc(max_y * sizeof(unsigned char*));
    for (i = 0; i < (int)max_y; ++i)
        out_image[i] = calloc(4 * max_x, sizeof(unsigned char));
    
    /* Blit each input image into the output image at the offset determined
     * by the packing.  This is a bit complicated, since the decoded pixels
     * are indexed from the top-left, while the packing uses bottom-left indexing */
    for (i = 0; i < sub_image_count; ++i) 
    {
        unsigned off_x = sub_images[i]->offset_x;
        unsigned off_y = sub_images[i]->offset_y;
        
        unsigned y;
        for (y = 0; y < sub_images[i]->pixel_height; ++y) {
            unsigned y_pix = off_y + y;
            memcpy(out_image[y_pix] + 4 * off_x,
                   sub_images[i]->pixels[sub_images[i]->top + y] + 4 * sub_images[i]->left,
                   4 * sub_images[i]->pixel_width);
        }
    }
    
    /* Encode and output the output image */
    write_png(output_file_name, max_x, max_y, out_image, &comments);
    
    for (i = 0; i < (int)max_y; ++i)
        free(out_image[i]);
    free(out_image);
}


int count_pngs(const char *name, const struct stat * status, int flag)
{
    if (flag == FTW_F) // file
    {
        FILE* fp = fopen(name, "rb");
        assert(fp);
        png_byte pngsig[8];
        fread(pngsig, 1, 8, fp);
        fclose(fp);
        
        int is_png = !png_sig_cmp(pngsig, 0, 8);
        if (is_png)
        {
            ++pngCount;
        }
    }
    return 0;
}


int load_images(const char *name, const struct stat * status, int flag)
{
    if (flag == FTW_F)
    {
        FILE* fp = fopen(name, "rb");
        assert(fp);
        png_byte pngsig[8];
        fread(pngsig, 1, 8, fp);
        fclose(fp);
        
        int is_png = !png_sig_cmp(pngsig, 0, 8);
        if (is_png)
        {
            images[imageIndex] = load_png((char *)name);
            autotrim(images[imageIndex]);
            imageIndex++;
        }
    }
    return 0;
}


void spritepack(char * outpath, char * file_prefix, char * inpath)
{
    const int max_depth = 10;
    int i;
    
    /* Cound the png files in inpath */
    ftw(inpath, count_pngs, max_depth);
    
    /* Buffer to hold img_t*'s for the input images */
    if (NULL == (images = calloc(1, sizeof(img_t *) * pngCount)))
    {
        fprintf(stderr, "calloc\n");
        exit(0);
    }
    
    /* Loop over the png files in inpath, decoding input PNGs and
     * creating img_t*'s for them */
    ftw(inpath, load_images, max_depth);
    
    /* Sort the images, because pack_rects expects input in sorted order */
    qsort(images, pngCount, sizeof(img_t*), image_comparator);
    
    /* Add boundaries and offsets */
    for (i = 0; i < pngCount; ++i) 
    {
        images[i]->pixel_width  = images[i]->w;
        images[i]->pixel_height = images[i]->h;
        
        images[i]->w += (2 * border_width);
        images[i]->h += (2 * border_width);
        
        images[i]->center_x = images[i]->w / 2;
        images[i]->center_y = images[i]->h / 2;
    }
    
    
    /* Divide the input images across as many output files 
     * as required */
    
    // HACK HACK HACK
    //
    // Troll Computer Science: Image sub dividing using area heuristic.
    // However, just because the heuristic passes does not mean the images
    // will fit in the available space!  The function "create_sprite_sheet"
    // will fail if it can't find a good fit for the images.
    unsigned int max_area = (max_size - 128) * (max_size - 128);
    unsigned int curr_area = 0;
    unsigned int image_area;
    
    img_t * sub_images[pngCount];
    unsigned sub_image_count = 0;
    unsigned file_count = 0;
    
    for (i = 0; i < pngCount; ++i) 
    {
        image_area = (images[i]->w * images[i]->h);
        
        /* if the new image causes us to exceed the maximum area
         * then output all sub images collected, and start a new
         * output image */
        if (curr_area + image_area >= max_area)
        {
            char * name;
            asprintf(&name, "%s/%s%d.png", outpath, file_prefix, file_count++);
            create_sprite_sheet(sub_images, sub_image_count, name);
            free(name);
            
            sub_image_count = 0;
            curr_area = 0;
        }
        
        if (image_area > max_area)
        {
            fprintf(stderr, "Image %s too big to fit, ignoring...\n", 
                    images[i]->filename);
        }
        else 
        {
            curr_area += image_area;
            sub_images[sub_image_count++] = images[i];
        }
    }
    
    if (sub_image_count > 0)
    {
        char * name;
        asprintf(&name, "%s/%s%d.png", outpath, file_prefix, file_count++);
        create_sprite_sheet(sub_images, sub_image_count, name);
        free(name);
    }
    
    free(images);
}


/* Usage: spritepack out_path in_file_path_1 in_file_path_2 ... */
int main(int argc, char** argv) 
{
    max_size = DEFAULT_MAX_SIZE;
    border_width = DEFAULT_BORDER_WIDTH;
    
    if (argc < 3)
    {
        fprintf(stderr, "Usage: spritepack out_path in_file_path_1 in_file_path_2 ...\n");
        exit(-1);
    }
    
    int argcount;
    for (argcount = 2; argcount < argc; argcount++)
    {
        char * inpath = argv[argcount];
        size_t len = strlen(inpath);
        if (strcmp(inpath, "./") == 0)
        {
            spritepack(argv[1], "outfile", inpath);
            break;
        }
        else if (strlen(inpath) > 0)
        {
            if (inpath[len-1] == '/') 
                inpath[len-1] = 0;
            char * out_prefix = strrchr(inpath, '/') + 1;
            spritepack(argv[1], out_prefix, inpath);
        }
    }
    
    return 0;
}
