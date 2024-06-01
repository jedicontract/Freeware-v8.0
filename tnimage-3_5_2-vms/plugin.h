//-----------------------------------------------------------------------//
//  plugin.h = Part of plugin for tnimage (Motif version)                //
//-----------------------------------------------------------------------//

typedef struct ParentData
{   int screen_bpp;         // bits/pixel of X server screen
    int mode;               // TEST or EXECUTE
    int image_count;        // total no. of images
    int ulx, uly, lrx, lry; // upper left and lower right coords of currently-
                            //     selected region
    int off[65];            // bytes/pixel for each bit/pixel screen mode
    int changed[MAXIMAGES]; // flag if image was modified by child
    double  fswap_temp;                // Temporary double for swap macro
    double dswap_temp;                 // Temporary double for swap macro
    int    swap_temp;                  // Temporary int for swap macro
};


////  Plugin-specific prototypes

void newimage(int ino, int xsize, int ysize, int bpp, int colortype, int frames);
void read_data(ParentData *g, char **arg, int &noofargs);
void write_data(int have_message, char *message, ParentData *g);
void print_data(ParentData *g);
void eraseimage(int ino);
void invert_image(ParentData *g);
int allocate_image_wavelet(Image &a);


