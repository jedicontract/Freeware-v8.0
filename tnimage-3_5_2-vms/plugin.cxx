//---------------------------------------------------------------------------//
// plugin.cc                                                                 //
// Latest Modification: 05-21-2000                                           //
// Demonstrate creation of plugins for tnimage                               //
// Compile with:  gcc -o plugin -O3 -Wall plugin.cc                          //
// Lines marked "Previously started at 0" need the starting loop value to be //
//   changed from 0 to 1 as of tnimage version 2.6.8 in all plugins.         //
//                                                                           //
// NOTE: write_data has changed as of version 3.1.0. All existing plugins    //
//   should replace their old write_data() function with the new one.        //
//---------------------------------------------------------------------------//

#include "xmtnimage.h"
#include "plugin.h"
Image  *z;
int ci;

int main(void)
{ 
    enum{ TEST, EXECUTE };
    z = new Image[513];
    ParentData g;
    int k,x,y, noofargs;
    char **arg;
    arg = new char*[20];
    for(k=0;k<20;k++) arg[k] = new char[128];
    
    char display_message[1024];
    int have_message=0;

    ////  Read parent data 
    read_data(&g, arg, noofargs);
    
    ////  Print data if in debug mode 
    if(g.mode==TEST) print_data(&g);
 

    /////////////////////////////////////////////////////////////////////
    //////////------Do not change anything above this line-------////////
    //////////------Code to manipulate the images goes here------////////
    //////////------No printf() statements allowed here    ------////////
    /////////////////////////////////////////////////////////////////////

    fprintf(stderr,"\n\nSuccessfully started plugin\n");
    fprintf(stderr,"No. of arguments %d\n",noofargs);
    for(k=0;k<noofargs;k++)
       fprintf(stderr," Argument %d %s\n",k,arg[k]);
    fprintf(stderr,"Current image %d\n",ci);
    fprintf(stderr,"Mode %d\n",g.mode);
    fprintf(stderr,"Selected area:\n");
    fprintf(stderr," ulx %d\n",g.ulx);
    fprintf(stderr," uly %d\n",g.uly);
    fprintf(stderr," lrx %d\n",g.lrx);
    fprintf(stderr," lry %d\n",g.lry);
    fprintf(stderr,"Image count %d\n",g.image_count);
    fprintf(stderr,"Screen bpp  %d\n",g.screen_bpp);
    fprintf(stderr,"Images present: %d\n",g.image_count);
    // Previously started at 0 //
    for(k=1;k<g.image_count;k++)
       fprintf(stderr," Image %d Name=%s Bpp=%d\n",k,
           z[k].name, z[k].bpp);

    fprintf(stderr,"\n\nCreating a new image x=128 y=256 at 8 bpp\n");
    g.image_count++;
    ci = g.image_count - 1;
    newimage(ci,128,256,8,INDEXED,1);

    ////  Create some arbitrary colormap
    for(k=0;k<256;k++) 
    {   z[ci].palette[k].red = k/4;
        z[ci].palette[k].green = 64-k/4;
        z[ci].palette[k].blue = k/4;
    }
    memcpy(z[ci].opalette,z[ci].palette,768);
    memcpy(z[ci].spalette,z[ci].palette,768);

    ////  Put a gradient in the image
    ////  See manual for bit packing format for images more than 8 bits.
    ////  The image bytes can be addressed in 2 ways:
    ////         z[ci].image[0][y][x]     =    3 dimensional array (frame,y,x)
    ////         z[ci].image_1d           =    1 dimensional array of bytes
    ////  This allows interfacing with other code that uses either method.
    
    for(y=0;y<z[ci].ysize;y++)
    for(x=0;x<z[ci].xsize;x++)
        z[ci].image[0][y][x] = y;

    ////  Do something to the image
    invert_image(&g);


    g.changed[ci] = 1;   
    z[ci].touched = 1;
    strcpy(z[ci].name, "New_test_image");

    strcpy(display_message, "No error messages\nto display");
    fprintf(stderr,"\nReady to come back\n");

    ////  Set this flag to display an error message. Do not write 
    ////  to stdout anywhere in the program unless in `test' mode.   
    have_message = 1;

    /////////////////////////////////////////////////////////////////////
    //////////------End of code to manipulate the images---------////////
    //////////------Do not change anything below this line-------////////
    /////////////////////////////////////////////////////////////////////
  
    
    ////  Return modified data back to parent
    if(g.mode==EXECUTE) write_data(have_message, display_message, &g);
    for(k=0;k<20;k++) delete[] arg[k];
    delete arg;  
    fprintf(stderr,"\nDone with plugin\n");
    return EXIT_SUCCESS;
}


//---------------------------------------------------------------------------//
//  invert_image - example image manipulating function                       //
//---------------------------------------------------------------------------//
void invert_image(ParentData *g)
{
    int f,i,ino,j, bytesperline;
    ino = ci;
    bytesperline = z[ino].xsize * g->off[z[ino].bpp];
    for(f=0;f<z[ino].frames;f++)
    for(j=0;j<z[ino].ysize;j++)
    for(i=0; i< bytesperline; i++)
         z[ino].image[f][j][i] = 255 - z[ino].image[f][j][i];

    ////  Set this flag to make the parent redraw the modified image.
    g->changed[ino] = 1;   

    ////  Set the 'image touched' flag so user has opportunity to save it.
    z[ino].touched = 1;
    
    ////  Change the title if desired 
    strcpy(z[ino].name, "Modified");
}


//---------------------------------------------------------------------------//
//  read_data - get data from parent                                         //
//---------------------------------------------------------------------------//
void read_data(ParentData *g, char **arg, int &noofargs)
{
    int k,n,pos,size;
    uchar byte,obyte=' ';
    g->off[8]=1;
    g->off[15]=2;
    g->off[16]=2;
    g->off[24]=3;
    g->off[32]=4;   
    g->off[48]=6;   

    ////  Get program data from parent
    ////  This should match the output from execute_plugin()

    ////  Up to 20 command line arguments length 128
    ////  arg[0] is plugin name, the rest are arguments
    noofargs=0;
    pos = 0;   

    while(1)
    {  read(STDIN_FILENO,&byte,1);
       if(byte==0) break;
       if(byte==' ' && obyte !=' '){ noofargs++; pos=0; }
       obyte=byte;
       if(byte!=' ') arg[noofargs][pos++]=byte;
       arg[noofargs][pos]=0;
    }

    ////  The currently-selected image
    read(STDIN_FILENO, &ci, sizeof(ci));

    ////  'mode' can be TEST or EXECUTE
    read(STDIN_FILENO, &g->mode, sizeof(int));

    ////  The corners of the currently-selected region
    g->ulx = atoi(arg[noofargs-4]);
    g->uly = atoi(arg[noofargs-3]);
    g->lrx = atoi(arg[noofargs-2]);
    g->lry = atoi(arg[noofargs-1]);

    ////  Total no. of images 
    read(STDIN_FILENO, &g->image_count, sizeof(int));

    ////  Bits/pixel of X display - do not change
    read(STDIN_FILENO, &g->screen_bpp, sizeof(int));

    ////  The Images struct and data for each image.
    ////  The size of the image data may exceed SHMMAX so get a copy
    ////  instead of using shared memory.

    // Previously started at 0 //
    for(k=1;k<g->image_count;k++)
    {    n=read(STDIN_FILENO, &z[k], sizeof(z[k]));
         newimage(k,z[k].xsize,z[k].ysize,z[k].bpp,z[k].colortype,z[k].frames);
         size = z[k].frames * z[k].xsize * z[k].ysize * g->off[z[k].bpp];
         read_fd(z[k].image_1d, STDIN_FILENO, size);
         //// Already has z[k].fftxsize and z[k].fftysize
         if(z[k].floatexists)
         {     size = z[k].fftxsize * z[k].fftysize * sizeof(complex);
               read_fd((uchar *)z[k].fft_1d, STDIN_FILENO, size);
         }
         if(z[k].waveletexists)
         {     size = z[k].wavexsize * z[k].waveysize * sizeof(double);
               read_fd((uchar *)z[k].wavelet_1d, STDIN_FILENO, size);
         }
         n=read(STDIN_FILENO, z[k].palette, 768);
         n=read(STDIN_FILENO, z[k].opalette, 768);
         n=read(STDIN_FILENO, z[k].spalette, 768);
         n=read(STDIN_FILENO, z[k].name,  FILENAMELENGTH);
         n=read(STDIN_FILENO, z[k].cal_title[0], FILENAMELENGTH);
         n=read(STDIN_FILENO, z[k].cal_title[1], FILENAMELENGTH);
         n=read(STDIN_FILENO, z[k].gamma, 256*sizeof(short int));
    }
}


//---------------------------------------------------------------------------//
//  write_data - send data back to parent                                    //
//  STDOUT_FILENO is redirected through a stream pipe.                       //
//---------------------------------------------------------------------------//
void write_data(int have_message, char *message, ParentData *g)
{
    int k,n,size;
    write(STDOUT_FILENO, &have_message, sizeof(int));
    write(STDOUT_FILENO, message, 1024);

    write(STDOUT_FILENO, &ci, sizeof(int));
    write(STDOUT_FILENO, &g->ulx, sizeof(int));
    write(STDOUT_FILENO, &g->uly, sizeof(int));
    write(STDOUT_FILENO, &g->lrx, sizeof(int));
    write(STDOUT_FILENO, &g->lry, sizeof(int));
    write(STDOUT_FILENO, &g->image_count, sizeof(int));

    ////  Send image struct and all dynamically-allocated data
    // Previously started at 0 //
    for(k=1;k<g->image_count;k++)
    {   
           n=write(STDOUT_FILENO, &g->changed[k], sizeof(int));
           n=write(STDOUT_FILENO, &z[k].xpos, sizeof(z[k].xpos));
           n=write(STDOUT_FILENO, &z[k].ypos, sizeof(z[k].ypos));
           n=write(STDOUT_FILENO, &z[k].xsize, sizeof(z[k].xsize));
           n=write(STDOUT_FILENO, &z[k].ysize, sizeof(z[k].ysize));
           n=write(STDOUT_FILENO, &z[k].bpp, sizeof(z[k].bpp));
           n=write(STDOUT_FILENO, &z[k].frames, sizeof(z[k].frames));
           n=write(STDOUT_FILENO, &z[k].colortype, sizeof(z[k].colortype));

           size = z[k].frames * z[k].xsize * z[k].ysize * g->off[z[k].bpp];
           n=write_fd(z[k].image_1d, STDOUT_FILENO, size);
           n=write(STDOUT_FILENO, &z[k].floatexists, sizeof(z[k].floatexists));

           //// This part is changed in ver. 3.1.0
           if(z[k].floatexists)
           {     
                 n=write(STDOUT_FILENO, &z[k].fftxsize, sizeof(int));
                 n=write(STDOUT_FILENO, &z[k].fftysize, sizeof(int));
                 size = z[k].fftxsize * z[k].fftysize * sizeof(complex);
                 write_fd((uchar *)z[k].fft_1d, STDOUT_FILENO, size);
           }
           n=write(STDOUT_FILENO, &z[k].waveletexists, sizeof(int));
           if(z[k].waveletexists)
           {     n=write(STDOUT_FILENO, &z[k].wavexsize, sizeof(int));
                 n=write(STDOUT_FILENO, &z[k].waveysize, sizeof(int));
                 n=write(STDOUT_FILENO, &z[k].origxsize, sizeof(int));
                 n=write(STDOUT_FILENO, &z[k].origxsize, sizeof(int));
                 n=write(STDOUT_FILENO, &z[k].wavelettype, sizeof(int));
                 n=write(STDOUT_FILENO, &z[k].nlevels, sizeof(int));
                 size = z[k].wavexsize * z[k].waveysize * sizeof(double);
                 write_fd((uchar *)z[k].wavelet_1d, STDOUT_FILENO, size);
           }
           n=write(STDOUT_FILENO, &z[k].backedup, sizeof(int));
           n=write(STDOUT_FILENO, &z[k].fftdisplay, sizeof(int));
           n=write(STDOUT_FILENO, &z[k].fmin, sizeof(double));
           n=write(STDOUT_FILENO, &z[k].fmax, sizeof(double));
           n=write(STDOUT_FILENO, &z[k].wmin, sizeof(double));
           n=write(STDOUT_FILENO, &z[k].wmax, sizeof(double));
           n=write(STDOUT_FILENO, z[k].gray, 4*sizeof(int));
           n=write(STDOUT_FILENO, &z[k].fftstate, sizeof(int));
           n=write(STDOUT_FILENO, z[k].palette, 768);
           n=write(STDOUT_FILENO, z[k].opalette, 768);
           n=write(STDOUT_FILENO, z[k].spalette, 768);
           n=write(STDOUT_FILENO, z[k].name,  FILENAMELENGTH);
           n=write(STDOUT_FILENO, z[k].cal_title[0], FILENAMELENGTH);
           n=write(STDOUT_FILENO, z[k].cal_title[1], FILENAMELENGTH);
           n=write(STDOUT_FILENO, z[k].gamma, 256*sizeof(short int));
    }
    
}



//---------------------------------------------------------------------------//
//  print_data - dump image data to screen for debugging                     //
//---------------------------------------------------------------------------//
void print_data(ParentData *g)
{
     int f,i,j,k;
     printf("Current image %d\n",ci);
     printf("Total no. of images %d\n", g->image_count);
     printf("X server bits/pixel %d\n", g->screen_bpp);
     // Previously started at 0 //
     for(k=1;k<g->image_count;k++)
     {    printf("\nImage no.%d\n",k);
          printf("Upper left corner of image:\n");
          for(f=0;f<z[k].frames;f++)
          {   printf("Frame %d\n",f);
              for(i=0;i<3;i++)
              {     for(j=0;j<10;j++)
                    {   printf("%d ",z[k].image[f][i][j]);fflush(stdout);
                    }printf("\n");
              }
          }
      
          printf("Palette entry no. 127 = r%d g%d b%d \n",
                  z[k].palette[127].red,z[k].palette[127].green,
                  z[k].palette[127].blue);
          printf("Image title = %s\n", z[k].name);
          printf("Calibration title x = %s\n", z[k].cal_title[0]);
          printf("Calibration title y = %s\n", z[k].cal_title[1]);
          printf("Image width in pixels %d\n",z[k].xsize);
          printf("Image height in pixels %d\n",z[k].ysize);
          printf("Image bits/pixel %d\n",z[k].bpp);
          printf("No. of frames in image %d\n",z[k].frames);
          printf("Current frame %d\n",z[k].cf);
          printf("Image touched flag %d\n", z[k].touched);
          printf("First entry in gamma table %d \n",z[k].gamma[0]);
          switch(z[k].colortype)
          {   case GRAY: printf("Grayscale image\n"); break;
              case INDEXED: printf("Indexed color image\n");  break;
              case COLOR: printf("Color image\n"); break;
          }
     }
}




//---------------------------------------------------------------------------//
//  newimage - allocate arrays for image buffer                              //
//  colortype can be GRAY, INDEXED, or COLOR                                 //
//---------------------------------------------------------------------------//
void newimage(int ino, int xsize, int ysize, int bpp, int colortype,
     int frames)
{
    int bytesperpixel = (7+bpp)/8;
    int k;

    ////  Allocate image buffer:  z[ino].image[frame][y][x]
    ////  The plugin should not try to write to the X server, display
    ////  the image, or create new Widgets.
    ////  Error handling is largely omitted.

    z[ino].xsize = xsize;
    z[ino].ysize = ysize;
    z[ino].frames = frames;
    z[ino].bpp = bpp;
    z[ino].colortype = colortype;
    z[ino].image_1d = (uchar*)malloc(xsize*ysize*frames*bytesperpixel);

    ////  Allows pixels to be addressed by z[ino].image[frame][y][x] 
    z[ino].image=make_3d_alias(z[ino].image_1d,xsize*bytesperpixel,ysize,frames);
    if(z[ino].image==NULL){ fprintf(stderr,"Insufficient memory\n"); exit(1); }

    ////  The colormaps
    z[ino].palette = new RGB[256];
    z[ino].opalette = new RGB[256];
    z[ino].spalette = new RGB[256];
 
    ////  Image title
    z[ino].name = new char[FILENAMELENGTH];

    ////  Title for calibration of image xy coordinates
    z[ino].cal_title[0] = new char[FILENAMELENGTH];      // title for calibration
    z[ino].cal_title[1] = new char[FILENAMELENGTH];      // title for calibration

    ////  Image's 8 bit gamma table
    z[ino].gamma = new short int[256];                // Gamma table

    for(k=0;k<256;k++)
    {   z[ino].gamma[k] = k;
        z[ino].palette[k].red = k/4;
        z[ino].palette[k].green = k/4;
        z[ino].palette[k].blue = k/4;
    }
    memcpy(z[ino].opalette,z[ino].palette,768);
    memcpy(z[ino].spalette,z[ino].palette,768);
    strcpy(z[ino].name,"Untitled");
    strcpy(z[ino].cal_title[0],"x calibration");
    strcpy(z[ino].cal_title[1],"y calibration");
    if(z[ino].gamma==NULL){ fprintf(stderr,"Insufficient memory\n"); exit(1); }

    if(z[ino].floatexists)
    {     z[ino].fft_1d = (complex*)malloc(z[ino].fftysize * z[ino].fftxsize*
              sizeof(complex));                       
          if(z[ino].fft_1d==NULL) fprintf(stderr, "Error creating FFT\n"); 
          z[ino].fft = make_2d_alias(z[ino].fft_1d, z[ino].fftxsize, z[ino].fftysize);
    }
    if(z[ino].waveletexists)
    {   allocate_image_wavelet( z[k] );
    }
    
}


//---------------------------------------------------------------------------//
//  erase - deallocate arrays for image buffer                               //
//---------------------------------------------------------------------------//
void eraseimage(int ino)
{
    delete[] z[ino].gamma;
    delete[] z[ino].cal_title[0];
    delete[] z[ino].cal_title[1];
    delete[] z[ino].name;
    delete[] z[ino].palette;
    delete[] z[ino].opalette;
    delete[] z[ino].spalette;
    if(z[ino].floatexists) 
    {   free(z[ino].fft);
        free(z[ino].fft_1d);
    }
    if(z[ino].waveletexists)
    {   for( int k = 0; k < z[ino].nsubimages; k++ )
          delete[] z[ino].wavelet_3d[k];
        delete[] z[ino].wavelet_3d;
        delete[] z[ino].subimagexsize;;
        delete[] z[ino].subimageysize;;
        free(z[ino].wavelet);
        free(z[ino].wavelet_1d);
    }
    free_3d(z[ino].image, z[ino].frames); 
    free(z[ino].image_1d);      
}


//---------------------------------------------------------------------------//
//  make_3d_alias - create an alias to permit accessing an array with both   //
//  []  and [][][] notation. This makes it compatible with code that uses    //
//  either type.                                                             //
//  The 3d alias must be freed with free_3d().                               //
//---------------------------------------------------------------------------//
uchar ***make_3d_alias(uchar *b, int x, int y, int z)
{   int j,k;
    uchar ***result;
    result = new uchar**[z];
    for(j=0;j<z;j++)
    {   result[j]=new uchar*[y];
        for(k=0;k<y;k++) 
            result[j][k] = b + j*x*y + k*x;      
    }
    return result;                      
}
uchar **make_2d_alias(uchar *b, int x, int y)
{  int k;
   uchar **result;
   result = (uchar**)malloc(y*sizeof(uchar *));
   for(k=0;k<y;k++) result[k] = b + k*x;
   return result;
} 
complex **make_2d_alias(complex *b, int x, int y)
{  int k;
   complex **result;
   result = (complex**)malloc(y*sizeof(complex *));
   for(k=0;k<y;k++) result[k] = b + k*x;
   return result;
} 
double **make_2d_alias(double *b, int x, int y)
{  int k;
   double **result;
   result = (double**)malloc(y*sizeof(double *));
   for(k=0;k<y;k++) result[k] = b + k*x;
   return result;
}


//---------------------------------------------------------------------------//
// allocate_image_wavelet                                                    //
//---------------------------------------------------------------------------//
int allocate_image_wavelet( Image &a )
{   int subim, l;
    a.wavelet_1d = (double*)malloc(a.waveysize*a.wavexsize*sizeof(double));
    if(a.wavelet_1d==NULL)
    {   fprintf(stderr, "Error allocating 1d wavelet array\n");
        return NOMEM;
    }
    a.wavelet = make_2d_alias(a.wavelet_1d, a.wavexsize, a.waveysize);
    if(a.wavelet==NULL)
    {   fprintf( stderr, "Error allocating wavelet array\n" );
        return NOMEM;
    }
    a.wavelet_3d = new double** [ a.nsubimages ];
    a.subimagexsize = new int [ a.nsubimages ];
    a.subimageysize = new int [ a.nsubimages ];
    if(a.wavelet_3d==NULL || a.subimagexsize==NULL || a.subimageysize==NULL)
    {   fprintf( stderr, "Error allocating wavelet 3d array\n" );
        return NOMEM;
    }

    if( a.wavelettype == LAPLACIAN )
    {
        for( l = 0; l < a.nlevels; l++ )
        {   a.subimagexsize[l] = a.origxsize >> l;
            a.subimageysize[l] = a.origysize >> l;
            a.wavelet_3d[l] = new double* [ a.subimageysize[l] ];
            if(a.wavelet_3d[l]==NULL)
            {   fprintf( stderr, "Error allocating wavelet subimage array\n" );
                return NOMEM;
            }
            a.wavelet_3d[l][0] = l == 0 ? a.wavelet_1d :
                 l == 1 ? a.wavelet_3d[0][0] + a.origxsize :
                 a.wavelet_3d[l-1][a.subimageysize[l-1]-1] + a.wavexsize;
            for( int j = 1; j < a.subimageysize[l]; j++ )
            {    a.wavelet_3d[l][j] = a.wavelet_3d[l][j-1] + a.wavexsize;
            }
        }
    } else
    {   for( l = 0; l < a.nlevels - 1; l++ )
        {   for( int k = 0; k < 3; k++ )
            {   subim = 3 * l + k;
                a.subimagexsize[subim] = a.wavexsize >> ( l + 1 );
                a.subimageysize[subim] = a.waveysize >> ( l + 1 );
                a.wavelet_3d[subim] = new double* [ a.subimageysize[subim] ];
                if(a.wavelet_3d[subim]==NULL)
                {   fprintf( stderr, "Error allocating wavelet subimage array\n" );
                    return NOMEM;
                }
                a.wavelet_3d[subim][0] =
                    k == 0 ? a.wavelet[ a.waveysize >> ( l + 1 ) ] + ( a.wavexsize >> ( l + 1 ) ) :
                    k == 1 ? a.wavelet[ 0 ] + ( a.wavexsize >> ( l + 1 ) ) :
                    a.wavelet[ a.waveysize >> ( l + 1 ) ];
                for( int j = 1; j < a.subimageysize[subim]; j++ )
                {   a.wavelet_3d[subim][j] = a.wavelet_3d[subim][j-1] + a.wavexsize;
                }
            }
        }
        subim = a.nsubimages - 1;
        a.subimagexsize[subim] = a.wavexsize >> ( a.nlevels - 1 );
        a.subimageysize[subim] = a.waveysize >> ( a.nlevels - 1 );
        a.wavelet_3d[subim] = new double* [ a.subimageysize[subim] ];
        if(a.wavelet_3d[subim]==NULL)
        {   fprintf( stderr, "Error allocating wavelet subimage array\n" );
            return NOMEM;
        }
        a.wavelet_3d[subim][0] = a.wavelet[ 0 ];
        for( int j = 1; j < a.subimageysize[subim]; j++ )
        {   a.wavelet_3d[subim][j] = a.wavelet_3d[subim][j-1] + a.wavexsize;
        }
    }
    return OK;
}



//---------------------------------------------------------------------------//
//  free_3d                                                                  //
//---------------------------------------------------------------------------//
void free_3d(uchar ***b, int z)
{   int j;
    for(j=0;j<z;j++)
       delete[] b[j];
    delete[] b;
}


//--------------------------------------------------------------------------//
//  write_fd - write to a file descriptor. Can only send 62000 bytes at a   //
//  time so break it up into <=1k chunks.  Returns bytes written.           //
//--------------------------------------------------------------------------//
int write_fd(uchar *source, int fd, int size)
{
   int n;
   int total=0;        // bytes sent
   int rem = size;     // bytes remaining to send
   while(total<size)   
   {     n = write(fd, source + total, min(rem,1024)); 
         if(n<=0){ fprintf(stderr,"Error writing in plugin\n"); break; }
         total += n;
         rem -= n;
   }
   return total;
}


//--------------------------------------------------------------------------//
//  read_fd - read from a file descriptor. Can only read 62000 bytes at a   //
//  time so break it up into <=1k chunks.  Returns bytes read.              //
//--------------------------------------------------------------------------//
int read_fd(uchar *dest, int fd, int size)
{
   int n;
   int total=0;        // bytes read
   int rem = size;     // bytes remaining to read
   while(total<size)   
   {    n = read(fd, dest + total, min(rem,1024));
        if(n<=0){ fprintf(stderr,"Error reading in plugin\n"); break; }
        total += n;
        rem -= n;
   }
   return total;
}

