# Spritepack

Forked from Owen Anderson's [spritepack](https://github.com/resistor/spritepack).

This program is primarily for creating sprite atlases for OpenGL and OpenGL ES applications.  It takes multiple sets of png files as input, and then outputs png atlas files to cover all of the input files.  The output files can be loaded as textures with your favourite PNG loader.  The output files also contain metadata about the input files, the size and location of each individual sprite on the generated image.

The primary difference between this program and the original is that this program has a maximum output file image size, and it will split the input sprite images across several output files, depending on how many output images are required to fit all the input images.  The current default maximum size is 1024x1024 pixels, but ideally this should be the result of the following GL function call on the target system:

    GLint texSize; 
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);

Finally, the program inserts a border around each input file, in case you wish to use the resulting spritesheet for mipmapping.  The default border size is 4 pixels.


## MAKE

There's a makefile.  You'll need libpng installed.


## USAGE

    spritepack out_path in_file_path_1 in_file_path_2 ...

Where

    out_path

is the location where you want output files to go.

And

    in_file_path_1 in_file_path_2 ... 

are directories containing png files.  All the png files in these location will be processed.  By default, the program will recurse through each input directory.

Each output file will take the name of the top folder name in the given infile_path, followed by a number indicating which file it is.

For example, if your command is:

    ./spritepack ~/out ~/images/spaceship ~/images/monster ~/images/coins/goldcoins

You will get, at minimum, three files:

    ~/out/spaceship1.png
    ~/out/monster1.png
    ~/out/goldcoins1.png

If, for example, the images in one of the paths are too big for one single output image, this program will spread across as many output images as required, so you'll get something like:

    ~/out/spaceship1.png
    ~/out/spaceship2.png
    ~/out/spaceship3.png

...and so forth.


## TROUBLESHOOTING

If an input image has a width and/or height greater than 50% of the maximum texture size, it is likely that the image packing algorithm will fail to find an adequate packing configuration for the image.  The sprite packer works best with lots of small images, rather than a small number of large ones.


## TODO

Add options to configure the max texture size, the border width, and whether the program recurses through the input folder paths or not.