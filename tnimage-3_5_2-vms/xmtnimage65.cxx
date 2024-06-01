//--------------------------------------------------------------------------//
// xmtnimage65.cc  png format                                               //
// Latest revision: 12-04-2002                                              //
// Copyright (C) 2002 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
// This only supports the most rudimentary form of the PNG format.          //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;

extern "C"
{
#include<png.h>
#include<pngconf.h>
}
void user_error_ptr(void);
void read_png(FILE *fp, unsigned int sig_read);
void png_read_row_callback(png_struct *png_ptr, png_uint_32 row, int pass);
void write_row_callback(png_struct *png_ptr, png_uint_32 row, int pass);


//-------------------------------------------------------------------------//
// check_if_png                                                             //
//-------------------------------------------------------------------------//
int check_if_png(char *identifier)
{
   char test[5] = ".PNG";
   test[0] = 0x89;
   return(!memcmp(identifier, test, 4));
}


//-------------------------------------------------------------------------//
// readpngfile                                                             //
//-------------------------------------------------------------------------//
int readpngfile(char *filename)
{
  FILE *fp;
  int bpp, ino, ct=0, k, color_type, interlace_type, compression_type, filter_type;
  int number_of_passes;
  int num_palette;
  png_colorp palette;
  double gamma;
  png_uint_32 w, h;
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
  if (!png_ptr) return ERROR;
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {     png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        return ERROR;
  }
  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info)
  {     png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        return ERROR;
  }
  if ((fp=fopen(filename,"rb")) == NULL){ message("Nope"); return ERROR; }
  if (setjmp(png_ptr->jmpbuf))
  {     png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        return ERROR;
  }
  png_init_io(png_ptr, fp);
  png_set_read_status_fn(png_ptr, png_read_row_callback);
  png_read_info(png_ptr, info_ptr);
  png_get_IHDR(png_ptr, info_ptr, &w, &h, &bpp, &color_type, 
        &interlace_type, &compression_type, &filter_type);
  switch(color_type)
  {     case PNG_COLOR_TYPE_GRAY:
        case PNG_COLOR_TYPE_GRAY_ALPHA:
             ct=GRAY; 
             break;
        case PNG_COLOR_TYPE_PALETTE:
             ct=INDEXED;
             break;
        case PNG_COLOR_TYPE_RGB:
        case PNG_COLOR_TYPE_RGB_ALPHA:
             ct=COLOR;
             bpp=max(bpp, 24);  //// png struct returns wrong bpp
             break;
        default: message("Unsupported PNG type");
        {     png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
              fclose(fp);
              return ERROR;
        }             
  }
  if(!between(bpp,0,48))
  {     png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        return ERROR;
  }
  if (bpp < 8) png_set_packing(png_ptr);
  gamma = 1.0; 
  number_of_passes = png_set_interlace_handling(png_ptr);
  if (color_type == PNG_COLOR_TYPE_GRAY && bpp < 8) png_set_expand(png_ptr);
  if(ct==COLOR) png_set_strip_16(png_ptr);
  png_set_strip_alpha(png_ptr);
  png_read_update_info(png_ptr, info_ptr);
  bpp = 8 * ((bpp + 7) / 8);
  if(newimage(basefilename(filename),0,0,w,h,bpp,ct,1,0,1,PERM,1,0,0)!=OK)
  {     png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        return ERROR;
  }
  ino = ci;

  //// Make an array of ptrs as libpng expects
  uchar **junk;
  junk = new uchar*[h];
  for(k=0; k<(int)h; k++) junk[k] = z[ino].image[0][k];
  png_read_image(png_ptr, &junk[0]);
  delete[] junk;

  if (ct==INDEXED && PNG_COLOR_MASK_COLOR)
  {     png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
        for(k=0;k<256;k++)
        {   g.palette[k].red   = palette[k].red/4;
            g.palette[k].green = palette[k].green/4;
            g.palette[k].blue  = palette[k].blue/4;
        }
  }

  png_set_gamma(png_ptr, 1.0, 1.0); // Image readers should not set gamma
  png_read_end(png_ptr, end_info);
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

  ////  Swap bytes for color only, not grayscale
  if (ct==COLOR && bpp >= 16 && g.ximage_byte_order == LSBFirst) 
       g.invertbytes = 1;
  fclose(fp);
  return OK;
}


void png_read_row_callback(png_struct *png_ptr, png_uint_32 row, int pass)
{
   png_ptr=png_ptr; row=row; pass=pass;
   // printf("row callback\n");
}

//-------------------------------------------------------------------------//
// writepngfile - can only write entire image                              //
//-------------------------------------------------------------------------//
int writepngfile(char *filename, int write_all, int compress)
{
  if(memorylessthan(16384)){ message(g.nomemory,ERROR); return(NOMEM); }
  FILE *fp;
  png_colorp palette;
  int bpp,png_bpp,k,w,h,ct,color_type;
  int xstart,ystart,xend,yend;
  ct = z[ci].colortype;
  bpp = z[ci].bpp;
  //png_color_8p sig_bit = NULL;
  write_all = 1;
  if(write_all==1)
  {   xstart = z[ci].xpos;
      ystart = z[ci].ypos;
      xend = xstart + z[ci].xsize;
      yend = ystart + z[ci].ysize;
  } else
  {   xstart=g.selected_ulx;
      xend=g.selected_lrx;
      ystart=g.selected_uly;
      yend=g.selected_lry;
  }
  w = xend - xstart;
  h = yend - ystart;
  if((fp=open_file(filename, compress, TNI_WRITE, g.compression, g.decompression, 
      g.compression_ext))==NULL) 
      return CANTCREATEFILE; 
  png_structp png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, 
      (void *)NULL, NULL, NULL);
  if (!png_ptr) return ERROR;
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr){ png_destroy_write_struct(&png_ptr, (png_infopp)NULL); return ERROR;  }
  if (setjmp(png_ptr->jmpbuf))
  {   png_destroy_write_struct(&png_ptr, &info_ptr);
      fclose(fp);
      return ERROR;
  }
  png_bpp = 8;

  png_init_io(png_ptr, fp);
  png_set_write_status_fn(png_ptr, write_row_callback);
  switch(ct)
  {     case GRAY: 
             png_bpp = bpp;
             color_type = PNG_COLOR_TYPE_GRAY; 
             break;
        case INDEXED: 
             png_bpp = 8;
             color_type = PNG_COLOR_TYPE_PALETTE; 
             break;
        case COLOR: 
             png_bpp = 8;
             color_type = PNG_COLOR_TYPE_RGB; 
             break;
        default: color_type = PNG_COLOR_TYPE_RGB; break;
  }
  png_set_IHDR(png_ptr, info_ptr, w, h, png_bpp, color_type, PNG_INTERLACE_NONE,
       PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  palette = (png_colorp)png_malloc(png_ptr, 256 * sizeof (png_color));
  if(ct!=GRAY)
  {    for(k=0;k<256;k++)
       {    palette[k].red   = z[ci].palette[k].red * 4;
            palette[k].green = z[ci].palette[k].green * 4;
            palette[k].blue  = z[ci].palette[k].blue * 4;
       }
       png_set_PLTE(png_ptr, info_ptr, palette, 256);
  }

  if(ct == COLOR) 
  { 
  // These all cause crash for unknown reason
  //       sig_bit->red = g.maxred[bpp];
  //       sig_bit->green = g.maxgreen[bpp];
  //       sig_bit->blue = g.maxblue[bpp];
  //       png_set_sBIT(png_ptr, info_ptr, sig_bit);
  }
  //if(ct == GRAY)  
  //{     sig_bit->gray = bpp;
  //      png_set_sBIT(png_ptr, info_ptr, sig_bit);
  //}

  if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA)
       png_set_bgr(png_ptr);
  png_write_info(png_ptr, info_ptr);
  if(bpp==32) png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);
  if (bpp >= 16 && g.ximage_byte_order == LSBFirst) png_set_swap(png_ptr);

  //// Make an array of ptrs as libpng expects
  uchar **junk;
  junk = new uchar*[h];
  for(k=0; k<(int)h; k++) junk[k] = z[ci].image[0][k];
  png_write_image(png_ptr, junk);
  delete[] junk;
  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr);
  close_file(fp, compress);  
  return OK; 
}

void write_row_callback(png_struct *png_ptr, png_uint_32 row, int pass)
{
  png_ptr=png_ptr; row=row; pass=pass;
     /* put your code here */
}
