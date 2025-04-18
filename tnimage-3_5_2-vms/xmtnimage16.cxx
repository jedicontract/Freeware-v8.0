//--------------------------------------------------------------------------//
// xmtnimage16.cc                                                           //
// JPEG image reading & writing                                             // 
// Latest revision: 02-05-2000                                              //
// See xmtnimage.h for Copyright Notice                                     //
//                                                                          //
// In order to get this to compile, you have to download a jpeg-x.tar.gz    //
// (where x is a version number) from ftp.uu.net, compile it, replace       //
// /usr/lib/libjpeg.a with the new libjpeg.a, and copy all the .h files to  //
// /usr/include/gr. Gcc complains that Libjpeg redefines `SIZEOF' but this  //
// does not cause any problems. If libjpeg is not at uu.net, try            // 
// www.ijg.org.                                                             //
//                                                                          // 
// This software is the work of Tom Lane, Philip Gladstone, Luis Ortiz, Jim //
// Boucher, Lee Crocker, Julian Minguillon, George Phillips, Davide Rossi,  //
// Ge' Weijers, and other members of the Independent JPEG Group.            //
// IJG is not affiliated with the official ISO JPEG standards committee.    //
//--------------------------------------------------------------------------//


#include "xmtnimage.h"

/*-------Jpeg-specific stuff-----------------------------------------------*/

#include <stdio.h>
#include <setjmp.h>

extern "C"
{
 #include <jpeglib.h>
 void write_JPEG_file (char * filename, int quality, int xstart, 
        int ystart,  int xend, int yend, int compress);
 static void my_error_exit (j_common_ptr cinfo);
 int read_JPEG_file (char * filename);

}

JSAMPLE * image_buffer;  /* Points to large array of R,G,B-order data */

//--------------------------------------------------------------------------//

extern Globals     g;
extern Image      *z;
extern int         ci;

char junk;


//--------------------------------------------------------------------------//
// writejpeg                                                                //
// Save image in JPEG format after setting up parameters.                   //
// Calls libjpeg functions compiled in C                                    //
// Based on software from the Independent JPEG Group.                       //
//--------------------------------------------------------------------------//
int writejpeg(char * filename, int write_all, int compress)
{ 
  if(memorylessthan(16384)){  message(g.nomemory,ERROR); return(NOMEM); } 
  int xstart,ystart,xend,yend,image_width;

  if(write_all==1)
  {   xstart = z[ci].xpos;
      ystart = z[ci].ypos;
      xend = xstart + z[ci].xsize - 1;
      yend = ystart + z[ci].ysize - 1;
  } else
  {   xstart=g.selected_ulx;
      xend=g.selected_lrx;
      ystart=g.selected_uly;
      yend=g.selected_lry;
  }

  image_width  = 1+xend-xstart;
  image_buffer = new JSAMPLE[image_width*3];
  if(image_buffer==NULL)
  {   message(g.nomemory,ERROR);
      return(ABORT);
  }
  write_JPEG_file(filename, g.jpeg_qfac, xstart, ystart, xend, yend, compress);
  delete[] image_buffer;
  return OK;
}


//--------------------------------------------------------------------------//
// write_JPEG_file                                                          //
// Called only by writejpeg() - Don't call directly.                        //
// Sample routine for JPEG compression.  We assume that target file name    //
// and a compression quality factor are passed in.                          //
// Based on software from the Independent JPEG Group.                       //
//--------------------------------------------------------------------------//
void write_JPEG_file (char * filename, int quality, int xstart, int ystart, 
     int xend, int yend, int compress)
{

  int bpp,ino,value,x,y,k3;   // Not part of JPEG's stuff
  int rr,gg,bb;
  int image_height;        /* Number of rows in image */
  int image_width;         /* Number of columns in image */
  image_height = 1+yend-ystart;
  image_width  = 1+xend-xstart;


  /*------------------------------------------------------------------------*/
  /* This struct contains the JPEG compression parameters and pointers to   */
  /* working space (which is allocated as needed by the JPEG library).      */
  /* It is possible to have several such structures, representing multiple  */
  /* compression/decompression processes, in existence at once.  We refer   */
  /* to any one struct (and its associated working data) as a "JPEG object".*/
  /*------------------------------------------------------------------------*/

  struct jpeg_compress_struct cinfo;

  /*------------------------------------------------------------------------*/
  /* This struct represents a JPEG error handler.  It is declared separately*/
  /* because applications often want to supply a specialized error handler  */
  /* (see the second half of this file for an example).  But here we just   */
  /* take the easy way out and use the standard error handler, which will   */
  /* print a message on stderr and call exit() if compression fails.        */
  /*------------------------------------------------------------------------*/

  struct jpeg_error_mgr jerr;
  FILE * outfile;               /* target file */
  JSAMPROW row_pointer[1];      /* pointer to JSAMPLE row[s] */

  /*------------------------------------------------------------------------*/
  /* Step 1: allocate and initialize JPEG compression object.               */
  /* We have to set up the error handler first, in case the initialization  */
  /* step fails.  (Unlikely, but it could happen if you are out of memory.) */
  /* This routine fills in the contents of struct jerr, and returns jerr's  */
  /* address which we place into the link field in cinfo.                   */
  /*------------------------------------------------------------------------*/

  cinfo.err = jpeg_std_error(&jerr);

  /*------------------------------------------------------------------------*/
  /* Now we can initialize the JPEG compression object.                     */
  /*------------------------------------------------------------------------*/

  jpeg_create_compress(&cinfo);

  /*------------------------------------------------------------------------*/
  /* Step 2: specify data destination (eg, a file)                          */
  /* Note: steps 2 and 3 can be done in either order.                       */
  /* Here we use the library-supplied code to send compressed data to a     */
  /* stdio stream.  You can also write your own code to do something else.  */
  /* VERY IMPORTANT: use "b" option to fopen() if you are on a machine that */
  /* requires it in order to write binary files.                            */
  /*------------------------------------------------------------------------*/

  if((outfile = open_file(filename, compress, TNI_WRITE, g.compression, g.decompression, 
      g.compression_ext))==NULL) 
      return; 
  jpeg_stdio_dest(&cinfo, outfile);

  /*------------------------------------------------------------------------*/
  /* Step 3: set parameters for compression.                                */
  /* First we supply a description of the input image.                      */
  /* Four fields of the cinfo struct must be filled in:                     */
  /*------------------------------------------------------------------------*/

  cinfo.image_width = image_width;      /* image width and height, in pixels*/
  cinfo.image_height = image_height;
  cinfo.input_components = 3;           /* # of color components per pixel  */
  cinfo.in_color_space = JCS_RGB;       /* colorspace of input image        */

  /*------------------------------------------------------------------------*/
  /* Now use the library's routine to set default compression parameters.   */
  /* (You must set at least cinfo.in_color_space before calling this,       */
  /* since the defaults depend on the source color space.)                  */
  /*------------------------------------------------------------------------*/

  jpeg_set_defaults(&cinfo);

  /*------------------------------------------------------------------------*/
  /* Now you can set any non-default parameters you wish to.                */
  /* Here we just illustrate the use of quality (quantization table) scaling*/
  /*------------------------------------------------------------------------*/

  jpeg_set_quality(&cinfo, quality, TRUE); /* limit to baseline-JPEG values */

  /*------------------------------------------------------------------------*/
  /* Step 4: Start compressor                                               */
  /* TRUE ensures that we will write a complete interchange-JPEG file.      */
  /* Pass TRUE unless you are very sure of what you're doing.               */
  /*------------------------------------------------------------------------*/

  jpeg_start_compress(&cinfo, TRUE);

  /*------------------------------------------------------------------------*/
  /* Step 5: while (scan lines remain to be written)                        */
  /*           jpeg_write_scanlines(...);                                   */
  /* Here we use the library's state variable cinfo.next_scanline as the    */
  /* loop counter, so that we don't have to keep track ourselves.           */
  /* To keep things simple, we pass one scanline per call; you can pass     */
  /* more if you wish, though.                                              */
  /*------------------------------------------------------------------------*/

  while (cinfo.next_scanline < cinfo.image_height) 
  { 

     //------------------------------------------------------------------//
     // Fill one line with pixel bytes                                   //
     // The following loop is not part of JPEG's stuff.                  //
     // image_buffer is a "JSAMPLE" i.e. unsigned char.                  //
     //------------------------------------------------------------------//
     y = cinfo.next_scanline;
     for(x=0,k3=0;x<image_width;x++,k3+=3)
     { 
        value = readpixelonimage(x+xstart,y+ystart,bpp,ino);
        if(bpp==8)
        {   valuetoRGB(value,rr,gg,bb,bpp);
            image_buffer[k3  ]=rr*4;
            image_buffer[k3+1]=gg*4;
            image_buffer[k3+2]=bb*4;
        }else    
        {   valuetoRGB(value,rr,gg,bb,bpp);
            value = convertpixel(value,bpp,24,1);
            image_buffer[k3  ]=rr;
            image_buffer[k3+1]=gg;
            image_buffer[k3+2]=bb;
        }
     }   

     row_pointer[0] = &image_buffer[0];
     (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  /*------------------------------------------------------------------------*/
  /* Step 6: Finish compression                                             */
  /*------------------------------------------------------------------------*/

  jpeg_finish_compress(&cinfo);

  /*------------------------------------------------------------------------*/
  /* After finish_compress, we can close the output file.                   */
  /*------------------------------------------------------------------------*/
  
  fflush(outfile);
  close_file(outfile, compress);  

  /*------------------------------------------------------------------------*/
  /* Step 7: release JPEG compression object                                */
  /* This is an important step since it will release a good deal of memory. */
  /*------------------------------------------------------------------------*/

  jpeg_destroy_compress(&cinfo);

  /* And we're done! */
}


/*
 * SOME FINE POINTS:
 *
 * In the above loop, we ignored the return value of jpeg_write_scanlines,
 * which is the number of scanlines actually written.  We could get away
 * with this because we were only relying on the value of cinfo.next_scanline,
 * which will be incremented correctly.  If you maintain additional loop
 * variables then you should be careful to increment them properly.
 * Actually, for output to a stdio stream you needn't worry, because
 * then jpeg_write_scanlines will write all the lines passed (or else exit
 * with a fatal error).  Partial writes can only occur if you use a data
 * destination module that can demand suspension of the compressor.
 * (If you don't know what that's for, you don't need it.)
 *
 * If the compressor requires full-image buffers (for entropy-coding
 * optimization or a noninterleaved JPEG file), it will create temporary
 * files for anything that doesn't fit within the maximum-memory setting.
 * (Note that temp files are NOT needed if you use the default parameters.)
 * On some systems you may need to set up a signal handler to ensure that
 * temporary files are deleted if the program is interrupted.  See libjpeg.doc.
 *
 * Scanlines MUST be supplied in top-to-bottom order if you want your JPEG
 * files to be compatible with everyone else's.  If you cannot readily read
 * your data in that order, you'll need an intermediate array to hold the
 * image.  See rdtarga.c or rdbmp.c for examples of handling bottom-to-top
 * source data using the JPEG code's internal virtual-array mechanisms.
 */



/******************** JPEG DECOMPRESSION SAMPLE INTERFACE *******************/

/* This half of the example shows how to read data from the JPEG decompressor.
 * It's a bit more refined than the above, in that we show:
 *   (a) how to modify the JPEG library's standard error-reporting behavior;
 *   (b) how to allocate workspace using the library's memory manager.
 *
 * Just to make this example a little different from the first one, we'll
 * assume that we do not intend to put the whole image into an in-memory
 * buffer, but to send it line-by-line someplace else.  We need a one-
 * scanline-high JSAMPLE array as a work buffer, and we will let the JPEG
 * memory manager allocate it for us.  This approach is actually quite useful
 * because we don't need to remember to deallocate the buffer separately: it
 * will go away automatically when the JPEG object is cleaned up.
 */


//--------------------------------------------------------------------------//
// readjpgfile - because it is impossible to determine the size of the      //
// image, most of the code that normally goes here is in read_JPEG_file()   //
//--------------------------------------------------------------------------//
int readjpgfile(char* filename)
{
   FILE *fp;
   int status=OK;
   if ((fp=fopen(filename,"rb")) == NULL)
   {   error_message(filename, errno);
       return(NOTFOUND);
   }
   fclose(fp);
   status = read_JPEG_file(filename);
   if(z[ci].bpp>=24) swap_image_bytes(ci); 
   return(status);
}


//--------------------------------------------------------------------------//
// my_err_mgr  - this junk has to go before read_JPEG_file or it won't      //
// compile.                                                                 //
// Based on software from the Independent JPEG Group.                       //
//--------------------------------------------------------------------------//
/*
 * ERROR HANDLING:
 *
 * The JPEG library's standard error handler (jerror.c) is divided into
 * several "methods" which you can override individually.  This lets you
 * adjust the behavior without duplicating a lot of code, which you might
 * have to update with each future release.
 *
 * Our example here shows how to override the "error_exit" method so that
 * control is returned to the library's caller when a fatal error occurs,
 * rather than calling exit() as the standard error_exit method does.
 *
 * We use C's setjmp/longjmp facility to return control.  This means that the
 * routine which calls the JPEG library must first execute a setjmp() call to
 * establish the return point.  We want the replacement error_exit to do a
 * longjmp().  But we need to make the setjmp buffer accessible to the
 * error_exit routine.  To do this, we make a private extension of the
 * standard JPEG error handler object.  (If we were using C++, we'd say we
 * were making a subclass of the regular error handler.)
 *
 * Here's the extended error handler struct:
 */

struct my_error_mgr 
{
  struct jpeg_error_mgr pub;    /* "public" fields */

  jmp_buf setjmp_buffer;        /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

static void my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}


//--------------------------------------------------------------------------//
// read_JPEG_file                                                           //
// Called only by readjpeg() - Don't call directly.                         //
// Sample routine for JPEG decompression. We assume that source file name   //
// is passed in.  We want to return 1 on success, 0 on error.  No, we       //
// want to return 0 on success, error code on error.                        //
// Based on software from the Independent JPEG Group.                       //
//--------------------------------------------------------------------------//
int read_JPEG_file (char * filename)
{

  int xsize,ysize;
  int k, ct=COLOR, bpp=24;

  /*------------------------------------------------------------------------*/
  /* This struct contains the JPEG decompression parameters and pointers to */
  /* working space (which is allocated as needed by the JPEG library).      */
  /*------------------------------------------------------------------------*/

  struct jpeg_decompress_struct cinfo;

  /*------------------------------------------------------------------------*/
  /* We use our private extension JPEG error handler.                       */
  /*------------------------------------------------------------------------*/

  struct my_error_mgr jerr;
  FILE * infile;                /* source file */
  JSAMPARRAY buffer;            /* Output row buffer */
  int row_stride;               /* physical row width in output buffer */

  /*------------------------------------------------------------------------*/
  /* We want to open the input file before doing anything else,             */
  /* so that the setjmp() error recovery below can assume the file is open. */
  /* VERY IMPORTANT: use "b" option to fopen() if you are on a machine that */
  /* requires it in order to read binary files.                             */
  /*------------------------------------------------------------------------*/

  if ((infile = fopen(filename, "rb")) == NULL) 
  {   error_message(filename, errno);
      return(ERROR);
  }
  if(g.tif_skipbytes) for(k=0;k<g.tif_skipbytes;k++) fread(&junk,1,1,infile);

  /*------------------------------------------------------------------------*/
  /* Step 1: allocate and initialize JPEG decompression object              */
  /* We set up the normal JPEG error routines, then override error_exit.    */
  /*------------------------------------------------------------------------*/

  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  /*------------------------------------------------------------------------*/
  /* Establish the setjmp return context for my_error_exit to use.          */
  /*------------------------------------------------------------------------*/

  if (setjmp(jerr.setjmp_buffer)) 
  {
    /* If we get here, the JPEG code has signaled an error. We need   */
    /* to clean up the JPEG object, close the input file, and return. */
       
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return(ERROR);
  }

  /*------------------------------------------------------------------------*/
  /* Now we can initialize the JPEG decompression object.                   */
  /*------------------------------------------------------------------------*/

  jpeg_create_decompress(&cinfo);

  /*------------------------------------------------------------------------*/
  /* Step 2: specify data source (eg, a file)                               */
  /*------------------------------------------------------------------------*/

  jpeg_stdio_src(&cinfo, infile);

  /*------------------------------------------------------------------------*/
  /* Step 3: read file parameters with jpeg_read_header()                   */
  /*------------------------------------------------------------------------*/

  (void) jpeg_read_header(&cinfo, TRUE);

  /*------------------------------------------------------------------------*/
  /* We can ignore the return value from jpeg_read_header since             */
  /*   (a) suspension is not possible with the stdio data source, and       */
  /*   (b) we passed TRUE to reject a tables-only JPEG file as an error.    */
  /* See libjpeg.doc for more info.                                         */
  /* Step 4: set parameters for decompression                               */
  /* In this example, we don't need to change any of the defaults set by    */
  /* jpeg_read_header(), so we do nothing here.                             */
  /* Step 5: Start decompressor                                             */
  /*------------------------------------------------------------------------*/

  jpeg_start_decompress(&cinfo);

  /*------------------------------------------------------------------------*/
  /* We may need to do some setup of our own at this point before reading   */
  /* the data.  After jpeg_start_decompress() we have the correct scaled    */
  /* output image dimensions available, as well as the output colormap      */
  /* if we asked for color quantization.                                    */
  /* In this example, we need to make output work buffer of the right size. */
  /* JSAMPLEs per row in output buffer                                      */
  /*------------------------------------------------------------------------*/

  row_stride = cinfo.output_width * cinfo.output_components;

  //------------------------------------------------------------------------//
  // This is not part of JPEG.                                              //
  //------------------------------------------------------------------------//

  xsize = cinfo.output_width;
  ysize = cinfo.output_height;
  if(cinfo.output_components==1) { bpp=8; ct=GRAY; } else { bpp=24; ct=COLOR; }
  
  if(newimage(basefilename(filename),0,0,xsize,ysize,bpp,ct,1,g.want_shell,
      1,PERM,1,g.window_border,0)!=OK) 
      return NOMEM;

  /*------------------------------------------------------------------------*/
  /* Make a one-row-high sample array that will go away when done with image*/
  /*------------------------------------------------------------------------*/

  buffer=(*cinfo.mem->alloc_sarray)
                ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

  /*------------------------------------------------------------------------*/
  /* Step 6: while (scan lines remain to be read)                           */
  /*           jpeg_read_scanlines(...);                                    */
  /* Here we use the library's state variable cinfo.output_scanline as the  */
  /* loop counter, so that we don't have to keep track ourselves.           */
  /*------------------------------------------------------------------------*/

  while (cinfo.output_scanline < cinfo.output_height)  
  {  
        (void) jpeg_read_scanlines(&cinfo, buffer, 1);
        // Assume put_scanline_someplace wants a pointer and sample count.   
        // put_scanline_someplace(buffer[0], row_stride);
       
        memcpy(z[ci].image[z[ci].cf][cinfo.output_scanline-1],buffer[0],row_stride);
      
  }
  
  /*------------------------------------------------------------------------*/
  /* Step 7: Finish decompression                                           */
  /*------------------------------------------------------------------------*/

  (void) jpeg_finish_decompress(&cinfo);

  /*------------------------------------------------------------------------*/
  /* We can ignore the return value since suspension is not possible        */
  /* with the stdio data source.                                            */
  /* Step 8: Release JPEG decompression object                              */
  /* This is an important step since it will release a good deal of memory. */
  /*------------------------------------------------------------------------*/

  jpeg_destroy_decompress(&cinfo);

  /*------------------------------------------------------------------------*/
  /* After finish_decompress, we can close the input file.                  */
  /* Here we postpone it until after no more JPEG errors are possible,      */
  /* so as to simplify the setjmp error logic above.  (Actually, I don't    */
  /* think that jpeg_destroy can do an error exit, but why assume anything) */
  /*------------------------------------------------------------------------*/
  
  fclose(infile);

  /*------------------------------------------------------------------------*/
  /* At this point you may want to check to see whether any corrupt-data    */
  /* warnings occurred (test whether jerr.pub.num_warnings is nonzero).     */
  /* And we're done!                                                        */
  /*------------------------------------------------------------------------*/

  if(jerr.pub.num_warnings) message("Corrupt data in JPEG file\n",WARNING); 
  return OK;

}


/*
 * SOME FINE POINTS:
 *
 * In the above code, we ignored the return value of jpeg_read_scanlines,
 * which is the number of scanlines actually read.  We could get away with
 * this because we asked for only one line at a time and we weren't using
 * a suspending data source.  See libjpeg.doc for more info.
 *
 * We cheated a bit by calling alloc_sarray() after jpeg_start_decompress();
 * we should have done it beforehand to ensure that the space would be
 * counted against the JPEG max_memory setting.  In some systems the above
 * code would risk an out-of-memory error.  However, in general we don't
 * know the output image dimensions before jpeg_start_decompress(), unless we
 * call jpeg_calc_output_dimensions().  See libjpeg.doc for more about this.
 *
 * Scanlines are returned in the same order as they appear in the JPEG file,
 * which is standardly top-to-bottom.  If you must emit data bottom-to-top,
 * you can use one of the virtual arrays provided by the JPEG memory manager
 * to invert the data.  See wrbmp.c for an example.
 *
 * As with compression, some operating modes may require temporary files.
 * On some systems you may need to set up a signal handler to ensure that
 * temporary files are deleted if the program is interrupted.  See libjpeg.doc.
 */

