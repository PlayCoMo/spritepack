# Spritepack

Forked from Owen Anderson's [spritepack](https://github.com/resistor/spritepack).

This program is primarily for creating sprite sheets for OpenGL and OpenGL ES applications.  

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

    in_file_paths: 

is a directory where png files are located.  All the png files found in the input location will be processed.  By default, the program will recurse through the input directory.

Each output file will take the name of the top folder name in the given infile_path, followed by a number indicating which file it is.

For example, if your command is:

    ./spritepack ~/out ~/images/spaceship ~/images/monster ~/images/coins/goldcoins

You will get, at minimum, three files:

    ~/out/spaceship1.png
    ~/out/monster1.png
    ~/out/goldcoins1.png

If, for example, the images in one of the paths need to be spread across multiple out put files, you'll get something like:

    ~/out/spaceship1.png
    ~/out/spaceship2.png
    ~/out/spaceship3.png

...and so forth.


## TODO

I will add options to configure the max texture size, the border width, and whether the program recurses through the input folder paths or not.