//--------------------------------------------------------------------------//
// xmtnimage13.cc                                                           //
// Memory allocation and deallocation                                       //
// Latest revision: 03-06-2006                                              //
// Copyright (C) 2006 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"
#include "xmtnimagec.h"

extern int         ci;                 // current image
extern Image      *z;
extern Globals     g;
int undocount=0;
undostruct *undo;
int in_erase=0;

//--------------------------------------------------------------------------//
// newimage                                                                 //
// Returns error code (OK if successful).                                   //
// Creates an image buffer in y,x format to make memcpy faster.             //
// If `shell' is 1, puts the new image in a separate Window shell.          //
// If `permanent' is 0, image no. is not returned in whichimage().          //
// If `window_border' is 1, window shell gets a border & title bar.         //
// If 'Widget' is non-zero, it reuses an old widget instead of creating a   //
//   new one. This is useful in converting image depths.                    //
// 'wantpalette' gives the new image a grayscale colormap.                  //
//--------------------------------------------------------------------------//
int newimage(char *name, int xpos, int ypos, int xsize, int ysize, int bpp, 
  int colortype, int frames, int shell, int scrollbars, int permanent, 
  int wantpalette, int border, Widget widget)
{
   if(g.diagnose){ printf("Creating new image buffer %dx%d...",xsize,ysize);fflush(stdout); }
   int ino,ibpp,j,k,bad=0,bytesperpixel,size;
   ino = g.image_count;
   ibpp = bpp;

   if(xsize==0 || ysize==0) return IGNORE;
   printstatus(NEWIMAGE);

   if(between(ibpp,1,6) || between(ibpp, 8,14))
   {     if(colortype==COLOR) ibpp=16;
         else ibpp = min(32, 8*((7+bpp)/8)); 
   }
   if(ibpp==7 && g.colortype==COLOR) ibpp=7;
   if(between(ibpp,15,15)) ibpp=15;
   if(between(ibpp,16,16)) ibpp=16;
   if(between(ibpp,17,24)) ibpp=24;
   if(between(ibpp,25,31)) ibpp=48;
   if(between(ibpp,32,32)) ibpp=32;
   if(between(ibpp,33,48)) ibpp=48;
 
   if(g.image_count>=MAXIMAGES)
   {    message("Too many images",ERROR);
        return(ERROR);
   }
        // Size of image[] + img[] + backup if needed
   size = imagesize(xsize+8,ysize+8,ibpp,1,g.autoundo,frames); 
        // Add size of `filename' message box so they can give it a name
   size += imagesize(404,112,g.bitsperpixel,1);
   size += 2000;                        // Add a safety margin 

   if(memorylessthan(size)){  message(g.nomemory,ERROR); return(NOMEM); } 

   ino = g.image_count;
   bytesperpixel = (7+ibpp)/8;
   z[ino].image_1d = (uchar*)malloc(xsize*ysize*frames*bytesperpixel);
   z[ino].image = make_3d_alias(z[ino].image_1d, xsize*bytesperpixel, ysize, frames);
   if(z[ino].image==NULL){ message(g.nomemory,ERROR);  bad=1; }
   z[ino].image_ximage = createximage(xsize, ysize, g.bitsperpixel, z[ino].image_1d);

   ////  Alpha channel is 1 bit/pixel                    
   z[ino].alpha_1d = (uchar*)malloc(((xsize+7)/8)*ysize*frames);
   z[ino].alpha=make_3d_alias(z[ino].alpha_1d, (xsize+7)/8, ysize, frames);
   if(z[ino].alpha==NULL){ message(g.nomemory, ERROR);  bad=1; }
   ////  Initialize alpha channel
   memset(z[ino].alpha_1d,0,frames*ysize*((7+xsize)/8));  

   ////  The .img only has one frame
   bytesperpixel = (7+g.bitsperpixel)/8;
   z[ino].img_1d  = (uchar*)malloc(xsize*ysize*bytesperpixel);
   z[ino].img  = make_2d_alias(z[ino].img_1d, xsize*bytesperpixel, ysize);
   if(z[ino].img==NULL) { message(g.nomemory, ERROR);  bad=1;  } 
   z[ino].img_ximage=createximage(xsize, ysize, g.bitsperpixel, z[ino].img_1d);
   
   printstatus(NEWIMAGE);
   ino=g.image_count;
   if((bad==1)||(z[ino].img==NULL))
   {   message(g.nomemory, ERROR);
       free_3d(z[ino].image, z[ino].frames);
       return(NOMEM);
   }  
                
   z[ino].palette = new RGB[256];
   if(z[ino].palette==NULL)
   {   message(g.nomemory, ERROR);
       free_3d(z[ino].image, z[ino].frames);
       free(z[ino].img);
       return(NOMEM);
   }  
   
   z[ino].opalette = new RGB[256];
   z[ino].spalette = new RGB[256];
   if(z[ino].spalette==NULL)
   {   message(g.nomemory,ERROR);
       delete[] z[ino].palette;
       free_3d(z[ino].image, z[ino].frames);
       free(z[ino].img);
       return(NOMEM);
   }               
   g.image_count++;
   //// Initialize image parameters 
   ci = ino;
   z[ci].cf = 0;
                                          // Set palette to gray so it repairs properly
                                          // 2nd param must be 0 or redraw() will crash
                                          // in setpalette().
   if(permanent && wantpalette) setSVGApalette(1, 0);   
   memcpy(z[ci].palette,g.palette,768);   // Give new image the current palette
   memcpy(z[ci].opalette,g.palette,768);  // Set backup palette also
   memcpy(z[ci].spalette,g.palette,768);  
   z[ci].backedup = 0;
   z[ci].backup=NULL;
   z[ci].backup_1d=NULL;
   z[ci].xsize = xsize;
   z[ci].ysize = ysize;
   z[ci].origxsize = xsize;
   z[ci].origysize = ysize;
   z[ci].xpos  = xpos;
   z[ci].ypos  = ypos;
   z[ci].overlap = ci;
   z[ci].name = new char[FILENAMELENGTH];

   if(z[ci].name==NULL)
   {   message(g.nomemory,ERROR);
       free_3d(z[ino].image, z[ino].frames);
       free(z[ino].image_1d);
       free_3d(z[ino].alpha, z[ino].frames);
       free(z[ino].alpha_1d);
       free(z[ino].img);
       free(z[ino].img_1d);
       delete[] z[ino].palette;
       delete[] z[ino].opalette;
       delete[] z[ino].spalette;
       return(NOMEM);
   }               
   z[ci].name[0]=0;
   if(strlen(name)) strcpy(z[ci].name, name);
   else strcpy(z[ci].name, "Image");

   z[ci].notes = NULL;
   z[ci].bpp   = ibpp;
   z[ci].originalbpp = bpp;
   z[ci].colortype  = colortype;
   z[ci].backup_colortype  = colortype;
   z[ci].ncolors = (int)g.maxvalue[ibpp];
   z[ci].floatexists = 0;
   z[ci].waveletexists = 0;
   z[ci].wavelettype = NONE;
   z[ci].wavelet_xminres = xsize / 16;
   z[ci].wavelet_yminres = ysize / 16;
   z[ci].wavelet_xlrstart = 0;
   z[ci].wavelet_ylrstart = 0;
   z[ci].fftdisplay = 0;
   z[ci].fmax = 1.0;
   z[ci].fmin = 0.0;  
   z[ci].wmax = 1.0;
   z[ci].wmin = 0.0;  
   z[ci].ffac = 1.0;  
   z[ci].wfac = 1.0;   
   z[ci].frames=frames;
   z[ci].fps = 1.0;
   z[ci].animation = 0;
   z[ci].fftxsize=0;
   z[ci].fftysize=0;
   z[ci].wavexsize=0;
   z[ci].waveysize=0;
   z[ci].nsubimages=0;
   z[ci].nlevels=0;
   strcpy(z[ci].format_name,"TIF");
   z[ci].format = TIF;
   z[ci].hitgray=0;                     // Flag if gray scaling in effect
   z[ci].adjustgray=1; 
   //  i_gray[0] = input white level (maximum white in image)
   //  i_gray[1] = input black level (minimum black in image)
   //  i_gray[2] = output white level
   //  i_gray[3] = output black level
   //  i_gray[4] = input white level of low resolution wavelet
   //  i_gray[5] = input black level of low resolution wavelet
   z[ci].gray[0] = (uint)g.maxvalue[ibpp];
   z[ci].gray[1] = 0;
   z[ci].gray[2] = g.maxgray[g.bitsperpixel];
   z[ci].gray[3] = 0;
   z[ci].gray[4] = 0;
   z[ci].gray[5] = 0;

   for(k=0; k<3; k++)
   {   for(j=0; j<10; j++)
           z[ci].cal_q[k][j]=0.0;       // lin.reg. params. for calibration 
       z[ci].cal_q[k][1]=1.0;           // slope of l.r.
       z[ci].cal_slope[k]=1;            // slope of calibration line 
       z[ci].cal_int[k]=0;              // intercept of calib. line
       z[ci].cal_log[k]=-1;             // -1=uncalibrated 0=linear 1=logarithm
       z[ci].cal_xorigin[k]=0;
       z[ci].cal_yorigin[k]=0;
       z[ci].cal_title[k] = new char[FILENAMELENGTH];      // title for calibration
   }
   z[ci].cal_dims=-1;
   strcpy(z[ci].cal_title[0],"x units");
   strcpy(z[ci].cal_title[1],"y units");
   strcpy(z[ci].cal_title[2],"z units");
   if(z[ci].cal_title[2]==NULL)
   {   message(g.nomemory,ERROR);
       free_3d(z[ino].image, z[ino].frames);
       free(z[ino].image_1d);
       free_3d(z[ino].alpha, z[ino].frames);
       free(z[ino].alpha_1d);
       free(z[ino].img);
       free(z[ino].img_1d);
       for(k=0;k<3;k++) delete[] z[ino].cal_title[k];
       delete[] z[ino].palette;
       delete[] z[ino].opalette;
       delete[] z[ino].spalette;
       delete[] z[ino].name;
       return(NOMEM);
   }               
   z[ci].cal_scale = 1.0;

   z[ci].sort = ci;
   z[ci].gamma = new short int[256];    // Gamma table
   if(z[ci].gamma==NULL)
   {   message(g.nomemory,ERROR);
       free_3d(z[ino].image, z[ino].frames);
       free(z[ino].image_1d);
       free_3d(z[ino].alpha, z[ino].frames);
       free(z[ino].alpha_1d);
       free(z[ino].img);
       free(z[ino].img_1d);
       delete[] z[ino].palette;
       delete[] z[ino].opalette;  
       delete[] z[ino].spalette;  
       for(k=0;k<3;k++) delete[] z[ino].cal_title[k];
       delete[] z[ino].name;
       return(NOMEM);
   }               
   for(k=0;k<256;k++)z[ci].gamma[k]=k;  // Initialize gamma table
   z[ci].touched=0;                     // Flag if image was modified
   z[ci].hit=0;                         // Flag if image was modified locally
   z[ci].fftstate=0;                    // 0 = not fft'd
   z[ci].waveletstate=0;                // 0 = not dwt'd
   z[ci].shell=shell;                   // 1 = in separate window
   z[ci].scrollbars=scrollbars;         // 1 = scrollbars if in separate window
   z[ci].permanent=permanent;           
   z[ci].window_border=border;          // 1 = has border
   z[ci].chromakey=0;
   z[ci].transparency=0;
   z[ci].ck_graymin=0;
   z[ci].ck_graymax=(int)g.maxvalue[bpp];
   z[ci].ck_min.red=0;
   z[ci].ck_max.red=g.maxred[bpp];
   z[ci].ck_min.green=0;
   z[ci].ck_max.green=g.maxgreen[bpp];
   z[ci].ck_min.blue=0;
   z[ci].ck_max.blue=g.maxblue[bpp];
   z[ci].is_zoomed = 0;
   z[ci].was_compressed = 0;
   z[ci].zoomx=1.0;
   z[ci].zoomy=1.0;
   z[ci].zoomx_inv=1.0;
   z[ci].zoomy_inv=1.0;
   z[ci].zoom_x1=0;
   z[ci].zoom_y1=0;
   z[ci].zoom_x2=0;
   z[ci].zoom_y2=0;
   z[ci].xangle=0;
   z[ci].yangle=0;
   z[ci].zangle=0;
   for(k=0;k<20;k++) z[ci].gparam[k] = 0;
   for(k=0;k<20;k++) z[ci].gbutton[k] = 0;
   z[ci].exposefunc=NULL;
   z[ci].exposeptr=NULL;
   z[ci].oframe_count=0;
   z[ci].split_frames=0;
   z[ci].split_frame_start=0;
   z[ci].split_frame_end=0;
   z[ci].floating=0;
   z[ci].is_font=0;
   for(k=0;k<256;k++) z[ci].red_brightness[k] = k;
   for(k=0;k<256;k++) z[ci].green_brightness[k] = k;
   for(k=0;k<256;k++) z[ci].blue_brightness[k] = k;
   for(k=0;k<256;k++) z[ci].gray_brightness[k] = k;
   z[ci].xscreensize=xsize;
   z[ci].yscreensize=ysize;
   z[ci].c.title = new char[100];
   strcpy(z[ci].c.title, "Movie");
   for(k=0;k<100;k++) z[ci].c.param[k] = 0;

   if(widget)
       z[ci].widget=widget;
   else
       z[ci].widget = imagewidget(xpos, ypos, xsize, ysize, &z[ci]);

   z[ci].win = XtWindow(z[ci].widget);  // Window is not known until widget is mapped
   z[ci].s = new Spreadsheet;
   z[ci].s->cols = xsize;
   z[ci].s->rows = ysize;
   z[ci].s->visible = 0;
   z[ci].s->created = 0;
   z[ci].s->ssform = 0;

   if(g.diagnose) printf("done\n");
   return OK;
} 


//--------------------------------------------------------------------------//
// add_frame - for formats like gif which can't be bothered to indicate how //
// many frames are present before reading the image.                        //
//--------------------------------------------------------------------------//
int add_frame(int ino)
{
   if(ino<=0){ message("Can\'t add frame to background"); return ERROR; }
   int f,j, status;
   int frames = z[ino].frames + 1;
   int bytesperline = z[ino].xsize * g.off[z[ino].bpp];
   status = newimage(z[ino].name, z[ino].xpos, z[ino].ypos, z[ino].xsize, z[ino].ysize, 
            z[ino].bpp, z[ino].colortype, frames, z[ino].shell,
            z[ino].scrollbars, z[ino].permanent, 
            0, g.window_border, 0);

   if(status) return status;
   z[ci].frames = frames;
   for(f=0; f<frames-1; f++)
   for(j=0; j<z[ino].ysize; j++)
        memcpy(z[ci].image[f][j], z[ino].image[f][j], bytesperline);
   memcpy(z[ci].palette,z[ino].palette, 768);
   memcpy(z[ci].spalette,z[ino].spalette, 768);
   memcpy(z[ci].opalette,z[ino].opalette, 768);
   eraseimage(ino,0,0,0);
   return OK;
}


//--------------------------------------------------------------------------//
// imagesize                                                                //
//--------------------------------------------------------------------------//
int imagesize(int x, int y, int bpp, int wantall, int wantbackup, int frames)
{  
   int size;
   size = y*sizeof(void *) + frames*y*x*g.off[bpp] + 2*sizeof(unsigned);  // image[]
   if(wantall)
   {   if(wantbackup) size*=2;
             // For img[]
       size+= y*sizeof(void *) + y*x*g.off[g.bitsperpixel] + 2*sizeof(unsigned); 
       size += 768;   // palette
       size += 80;    // cal_title
       size += 128;   // name    
       size += 512;   // gamma table
   }    
   return(size);
}        


//--------------------------------------------------------------------------//
// erase_fft  - erase fft only to make space                                //
//--------------------------------------------------------------------------//
void erase_fft(int ino)
{
   if(z[ino].floatexists)  
   {   free(z[ino].fft); 
       free(z[ino].fft_1d);
       z[ino].floatexists = 0; 
       z[ino].fftxsize    = 0;
       z[ino].fftysize    = 0;
   }
}


//--------------------------------------------------------------------------//
// erase_wavelet                                                            //
//--------------------------------------------------------------------------//
void erase_wavelet(int ino)
{
   if(z[ino].waveletexists)  
   {    free(z[ino].wavelet); 
        free(z[ino].wavelet_1d);
        for( int i = 0; i < z[ino].nsubimages; i++ )
           delete [] z[ino].wavelet_3d[i];
        if(z[ino].wavelet_3d != NULL)
           delete[] z[ino].wavelet_3d;
        if(z[ino].subimagexsize != NULL)
             delete[] z[ino].subimagexsize;
        if(z[ino].subimageysize != NULL)
        delete[] z[ino].subimageysize;
        z[ino].waveletexists = 0;
        z[ino].wavelettype = NONE;
   }
}


//--------------------------------------------------------------------------//
// erase_resize_wavelet                                                     //
// Erases wavelet and restores image to its original size                   //
//--------------------------------------------------------------------------//
void erase_resize_wavelet(int ino)
{
   if(z[ino].waveletexists)  
   {  erase_wavelet(ino);
      if(z[ino].origxsize != z[ino].xsize || z[ino].origysize != z[ino].ysize)
           resize_image(ino, z[ino].origxsize, z[ino].origysize); 
   }
}


//--------------------------------------------------------------------------//
// unload_image - erase image specified by name or number                   //
//--------------------------------------------------------------------------//
void unload_image(int noofargs, char **arg)
{   
    int ino;
    char tempstring[FILENAMELENGTH];
    if(noofargs<1) ino=ci;
    else ino=image_number(arg[1]);
    if(g.getout){ g.getout=0; return; }
    if(between(ino, 1, g.image_count-1)) 
        eraseimage(ino,1,0,1);
    else 
    {  
        if(noofargs==0 && ino==0)
           sprintf(tempstring,"Can\'t delete background image buffer"); 
        else
           sprintf(tempstring,"Can\'t delete image:\n%s",arg[1]); 
        message(tempstring,ERROR);
    }
}


//--------------------------------------------------------------------------//
// imageunmapcb                                                             //
//--------------------------------------------------------------------------//
void imageunmapcb(Widget w, XtP client_data, XmACB *call_data)
{
   w=w; call_data=call_data;
   int ino, bpp;
   Widget widget = *(Widget *)client_data;
   ino = whichimage(widget, bpp);
   if(!in_erase) eraseimage(ino,1,0,0);
   for(int k=0;k<g.openlistcount;k++) update_list(k);
}


//--------------------------------------------------------------------------//
// eraseimage                                                               //
//--------------------------------------------------------------------------//
void eraseimage(int ino, int ask, int savewidget, int want_redraw)
{   
    if(in_erase) return;
    in_erase=1;
    Widget ssformsave=0;    
    XFlush(g.display);
    XSync(g.display, 0);
    int bytesperpixel,i,k,status=OK,maximage=g.image_count-1;
    if(g.diagnose){ printf("Deleting image buffer %d...",ino);fflush(stdout); }
    if(!between(ino,0,g.image_count-1))   
    {  message("Please select an image to erase"); in_erase=0; return; }
    if(g.busy){ message("Image buffers busy\ncannot unload image"); in_erase=0; return; }
    if(g.block && !g.block_okay)
    { message("Waiting for input\ncannot unload image"); in_erase=0; return; }
    printstatus(ERASING);
    if(maximage<=0){ message("No images"); in_erase=0; return; }
                               // Save the image first
    drawselectbox(OFF);
    if(z[ino].touched && ask) status=save_ok(ino);
    if(status==ABORT){ in_erase=0; return; }  // User clicked Cancel or save was unsuccessful
     
    if(!between(ino,0,g.image_count-1)){ message("Bad image number",ERROR); in_erase=0; return; }
    if(ino>=0 && z[ino].is_zoomed) unpush_main_button(g.zoom_button);
    if(!savewidget)
    {                          // Remove those wacky event handlers first

        XtRemoveEventHandler(z[ino].widget, g.mouse_mask, False,
             (XtEH)mousecb, (XtP)z);
        XtRemoveEventHandler(XtParent(XtParent(z[ino].widget)), 
             StructureNotifyMask, False,
             (XtEH)configurecb, (XtP)z);
        XtRemoveEventHandler(XtParent(XtParent(z[ino].widget)), 
             FocusChangeMask, False, (XtEH)imagefocuscb, (XtP)z);
          
                               // Get rid of the dialog shell if image is in a
                               // separate window
        if(z[ino].shell )
        {    XtRemoveEventHandler(z[ino].widget, 
                StructureNotifyMask, False, (XtEH)resize_scrolled_imagecb, (XtP)z);
             XtRemoveEventHandler(XtParent(z[ino].widget), 
                StructureNotifyMask, False, (XtEH)resize_scrolled_imagecb, (XtP)z);
             XtRemoveEventHandler(XtParent(XtParent(z[ino].widget)), 
                StructureNotifyMask, False, (XtEH)resize_scrolled_imagecb, (XtP)z);
        }
        if(z[ino].shell) 
        {    
              XtUnmanageChild(XtParent(z[ino].widget)); 
              XtDestroyWidget(XtParent(z[ino].widget));
              XtUnmanageChild(XtParent(XtParent(z[ino].widget))); 
              XtDestroyWidget(XtParent(XtParent(z[ino].widget)));
        }
                               // Free the widget. This also frees the icon pixmap. 
        if(XtIsManaged(z[ino].widget)) 
        {     XtUnmanageChild(z[ino].widget); 
              XtDestroyWidget(z[ino].widget);
        }
        XFlush(g.display);
    }

    if(savewidget) ssformsave = z[ino].s->ssform;
    if(z[ino].s->created)  delete_spreadsheet(ino, ssformsave);
    if(z[ino].floating) g.floating_magnifier = 0;
  
    ////  No garbage collection
    ////  Free dynamic data from ino

    if(z[ino].backedup){    free_3d(z[ino].backup, z[ino].frames);
                            free(z[ino].backup_1d);
                       }    
    if(z[ino].image   !=0)  free_3d(z[ino].image, z[ino].frames);
    if(z[ino].image_1d!=0)     free(z[ino].image_1d);
    if(z[ino].alpha   !=0)  free_3d(z[ino].alpha, z[ino].frames);
    if(z[ino].alpha_1d!=0)     free(z[ino].alpha_1d);
    if(z[ino].image_ximage!=0)XFree(z[ino].image_ximage);    
    if(z[ino].img     !=0)     free(z[ino].img);
    if(z[ino].img_1d  !=0)     free(z[ino].img_1d);
    if(z[ino].img_ximage!=0)  XFree(z[ino].img_ximage);    
    if(z[ino].floatexists)   erase_fft(ino);
    if(z[ino].waveletexists) erase_wavelet(ino);
    if(z[ino].palette!=0)      delete[] z[ino].palette;
    if(z[ino].opalette!=0)     delete[] z[ino].opalette;        
    if(z[ino].spalette!=0)     delete[] z[ino].spalette;        
    if(z[ino].name!=0)         delete[] z[ino].name;
    if(z[ino].notes!=NULL)      { delete[] z[ino].notes; z[ino].notes=NULL; }
    for(k=0;k<3;k++)
      if(z[ino].cal_title[k]!=0) delete[] z[ino].cal_title[k];
    if(z[ino].gamma!=0)        delete[] z[ino].gamma;        
    if(z[ino].c.title!=0)      delete[] z[ino].c.title;        

    ////  Copy down all other pointers here to images can be renumbered.
    ////  Anything allocated with make_Xd_alias must be deleted and
    ////  regenerated above. Normal variables are automatically reassigned 
    ////  in this loop by copying z[k].
    ////  Anything that is a pointer must be copied separately in this loop. 
    ////  If any pointers in clickboxdata were allocated, delete them above
    ////  and copy them here.
    ////  Any parameters that are image numbers must also be recalculated.
 
    for(k=ino; k<maximage; k++)
    {    z[k] = z[k+1]; 
         bytesperpixel = (7+g.bitsperpixel)/8;
         z[k].img = make_2d_alias(z[k].img_1d, 
                    bytesperpixel * cint(z[k].xsize * z[k].zoomx), 
                    cint(z[k].ysize * z[k].zoomy));
         bytesperpixel=(7+z[k].bpp)/8;
         z[k].image  = make_3d_alias(z[k].image_1d, bytesperpixel*z[k].xsize,
                       z[k].ysize, z[k].frames);
         z[k].alpha  = make_3d_alias(z[k].alpha_1d, (z[k].xsize+7)/8,
                       z[k].ysize, z[k].frames);
         if(z[k].backedup)
              z[k].backup = make_3d_alias(z[k].backup_1d, 
                            g.off[z[k].bpp]*z[k].xsize,
                            z[k].ysize, z[k].frames);  
         if(z[k].floatexists)
              z[k].fft = make_2d_alias(z[k].fft_1d,z[k].fftxsize,z[k].fftysize);
         if(z[k].waveletexists)
         {    z[k].wavelet = make_2d_alias(z[k].wavelet_1d, z[k].wavexsize, z[k].waveysize);
              for(i=0; i<z[k].nsubimages; i++)
                  z[k].wavelet_3d[i] = z[k+1].wavelet_3d[i];
              z[k].wavelet_3d = z[k+1].wavelet_3d; 
         }

         ////  Copy pointers in clickboxdata struct
         z[k].c.title = z[k+1].c.title;
         z[k].s = z[k+1].s;
         z[k].s->ino = k;

    }

    //// Recalculate how many frames are left
    for(k=1; k<g.image_count; k++)
    {
         if(z[k].split_frames)
         {   if(z[k].split_frame_start > ino) 
                 z[k].split_frame_start = max(1,z[k].split_frame_start-1);
             if(z[k].split_frame_end   >= ino) 
                 z[k].split_frame_end = max(1,z[k].split_frame_end-1);
         }
    }

 
    //// If any 3d graphs of this image are present, flag graph as unusable
    for(k=0;k<g.image_count;k++) 
       if(z[k].gparam[0] == ino && z[k].colortype==GRAPH) z[k].gparam[0] = 0;

    g.image_count--;

    for(k=0;k<g.image_count;k++) z[k].overlap = g.image_count-1-k;
    ci = g.image_count-1;
    if(!savewidget && want_redraw) 
    {   switchto(ci);
        if(ci>=0)
        {   setpalette(z[ci].palette);
            selectimage(ci);
        }
        g.imageatcursor=0;
    }  
   
    if(want_redraw) redrawscreen();
    if(g.diagnose){ printf("done\n");fflush(stdout); }
    XFlush(g.display);
    XSync(g.display, 0);
    in_erase=0;
    return;
}


//--------------------------------------------------------------------------//
// copythebackup - part of eraseimage()                                     //
//--------------------------------------------------------------------------//
void copythebackup(int k)
{
   int f,i,j;                   // Also copy the backup, if present.

   if(z[k].backedup)
   {    free_3d(z[k].backup, z[k].frames);
        free(z[k].backup_1d);
   }

   if(z[k+1].backedup)
   {    z[k].backup_1d = (uchar*)malloc(z[k+1].ysize*z[k+1].xsize*
                          g.off[z[k+1].bpp]*sizeof(uchar)*z[k+1].frames);
        if(z[k].backup_1d==NULL){ message("Error erasing image!",ERROR); return; }
        z[k].backup = make_3d_alias(z[k].backup_1d, g.off[z[k+1].bpp]*z[k+1].xsize,
                      z[k+1].ysize, z[k+1].frames);  
           
        for(f=0;f<z[k+1].frames;f++)
          for(i=0;i<z[k+1].ysize;i++)
            for(j=0;j<g.off[z[k+1].bpp]*z[k+1].xsize;j++)
               z[k].backup[f][i][j] = z[k+1].backup[f][i][j];
          
        z[k].backedup = 1;
   }else z[k].backedup = 0; 
}


//--------------------------------------------------------------------------//
// copyimageparameters                                                      //
// k1 = destination  k2 = source                                            //
//--------------------------------------------------------------------------//
void copyimageparameters(int k1, int k2)
{  int bpp,i,k;
   z[k1].xpos=z[k2].xpos;   
   z[k1].ypos=z[k2].ypos;   
   // These have already been copied
   //   z[k1].xsize=z[k2].xsize;
   //   z[k1].ysize=z[k2].ysize; 
   //   z[k1].bpp = z[k2].bpp;    
   //   z[k1].originalbpp=z[k2].originalbpp;
   z[k1].hitgray=z[k2].hitgray; 
   z[k1].adjustgray=z[k2].adjustgray; 
   z[k1].touched=z[k2].touched; 
   z[k1].hit=z[k2].hit; 
   z[k1].shell=z[k2].shell; 
   z[k1].scrollbars=z[k2].scrollbars; 
   z[k1].permanent=z[k2].permanent; 
   z[k1].window_border=z[k2].window_border; 
   z[k1].chromakey  = z[k2].chromakey;
   z[k1].transparency = z[k2].transparency;

   bpp = z[k1].bpp;
   z[k1].ck_graymin    = min(g.maxgray[bpp], z[k2].ck_graymin);
   z[k1].ck_graymax    = min(g.maxgray[bpp], z[k2].ck_graymax);
   z[k1].ck_min.red    = min(g.maxred[bpp],  z[k2].ck_min.red);
   z[k1].ck_max.red    = min(g.maxred[bpp],  z[k2].ck_max.red);
   z[k1].ck_min.green  = min(g.maxgreen[bpp],z[k2].ck_min.green);
   z[k1].ck_max.green  = min(g.maxgreen[bpp],z[k2].ck_max.green);
   z[k1].ck_min.blue   = min(g.maxblue[bpp], z[k2].ck_min.blue);
   z[k1].ck_max.blue   = min(g.maxblue[bpp], z[k2].ck_max.blue);
   z[k1].is_zoomed     = z[k2].is_zoomed;
   z[k1].was_compressed= z[k2].was_compressed;
   z[k1].zoomx         = z[k2].zoomx;
   z[k1].zoomy         = z[k2].zoomy;
   z[k1].zoomx_inv     = z[k2].zoomx_inv;
   z[k1].zoomy_inv     = z[k2].zoomy_inv;
   z[k1].zoom_x1       = z[k2].zoom_x1;
   z[k1].zoom_y1       = z[k2].zoom_y1;
   z[k1].zoom_x2       = z[k2].zoom_x2;
   z[k1].zoom_y2       = z[k2].zoom_y2;
   z[k1].xangle        = z[k2].xangle;
   z[k1].yangle        = z[k2].yangle;
   z[k1].zangle        = z[k2].zangle;
   for(k=0;k<20;k++) z[k1].gparam[k] = z[k2].gparam[k];
   for(k=0;k<20;k++) z[k1].gbutton[k] = z[k2].gbutton[k];
   z[k1].exposefunc    = z[k2].exposefunc;
   z[k1].exposeptr     = z[k2].exposeptr;
   z[k1].oframe_count  = z[k2].oframe_count;
   z[k1].split_frames  = z[k2].split_frames;
   z[k1].split_frame_start = z[k2].split_frame_start;
   z[k1].split_frame_end   = z[k2].split_frame_end;
   z[k1].xscreensize       = z[k2].xscreensize;
   z[k1].yscreensize       = z[k2].yscreensize;
   z[k1].floating          = z[k2].floating;
   z[k1].is_font           = z[k2].is_font;
   for(k=0;k<256;k++) z[k1].red_brightness[k]   = z[k2].red_brightness[k];
   for(k=0;k<256;k++) z[k1].green_brightness[k] = z[k2].green_brightness[k];
   for(k=0;k<256;k++) z[k1].blue_brightness[k]  = z[k2].blue_brightness[k];
   for(k=0;k<256;k++) z[k1].gray_brightness[k]  = z[k2].gray_brightness[k];

   //// Don't copy z[k2].c (clickboxinfo struct)

   z[k1].gray[0]=z[k2].gray[0]; 
   z[k1].gray[1]=z[k2].gray[1];
   z[k1].gray[2]=z[k2].gray[2]; 
   z[k1].gray[3]=z[k2].gray[3];
   z[k1].gray[4]=z[k2].gray[4]; 
   z[k1].gray[5]=z[k2].gray[5];
   z[k1].fftdisplay=z[k2].fftdisplay;
   z[k1].fmin=z[k2].fmin;   
   z[k1].fmax=z[k2].fmax;
   z[k1].wmin=z[k2].wmin;   
   z[k1].wmax=z[k2].wmax;
   z[k1].ffac=z[k2].ffac;   
   z[k1].wfac=z[k2].wfac;
   z[k1].fftxsize= z[k2].fftxsize;
   z[k1].fftysize= z[k2].fftysize;
   z[k1].fftstate= z[k2].fftstate;
   z[k1].wavexsize= z[k2].wavexsize;
   z[k1].waveysize= z[k2].waveysize;
   z[k1].waveletstate= z[k2].waveletstate;
   z[k1].frames= z[k2].frames;
   z[k1].fps= z[k2].fps;
   z[k1].animation = z[k2].animation;
   z[k1].floatexists=z[k2].floatexists;
   z[k1].backup_colortype=z[k2].backup_colortype;
   z[k1].waveletexists=z[k2].waveletexists;
   z[k1].wavelettype=z[k2].wavelettype;
   z[k1].wavelet_xminres=z[k2].wavelet_xminres;
   z[k1].wavelet_yminres=z[k2].wavelet_yminres;
   //z[k1].overlap=z[k2].overlap;
   z[k1].cal_dims=z[k2].cal_dims; 
   for(k=0; k<3; k++)
   {    z[k1].cal_log[k]     = z[k2].cal_log[k]; 
        z[k1].cal_xorigin[k] = z[k2].cal_xorigin[k]; 
        z[k1].cal_yorigin[k] = z[k2].cal_yorigin[k]; 
        z[k1].cal_int[k]     = z[k2].cal_int[k];
        z[k1].cal_slope[k]   = z[k2].cal_slope[k];
        z[k1].cal_slope[k]   = z[k2].cal_slope[k];
        strcpy(z[k1].cal_title[k], z[k2].cal_title[k]);
        for(i=0;i<10;i++) z[k1].cal_q[k][i] = z[k2].cal_q[k][i];
   }
   z[k1].cal_scale = z[k2].cal_scale;
   for(i=0;i<6;i++)  z[k1].gray[i] =z[k2].gray[i];
   strcpy( z[k1].format_name, z[k2].format_name);
   z[k1].format = z[k2].format;
   strcpy(z[k1].name, z[k2].name);
   if(z[k2].notes != NULL) strcpy(z[k1].notes, z[k2].notes);
   z[k1].origxsize = z[k2].origxsize;
   z[k1].origysize = z[k2].origysize;
   z[k1].wavelet_xlrstart = z[k2].wavelet_xlrstart;
   z[k1].wavelet_ylrstart = z[k2].wavelet_ylrstart;
   z[k1].nsubimages = z[k2].nsubimages;
   z[k1].nlevels = z[k2].nlevels;
}


//--------------------------------------------------------------------//
// resize_widget                                                      //
//--------------------------------------------------------------------//
void resize_widget(Widget w, int xsize, int ysize)
{
  if(g.state != ACQUIRE) XtResizeWidget(w,xsize,ysize,0);
}


//--------------------------------------------------------------------------//
// resize  resize image's window only. Use this function instead of         //
// resize_widget since this function protects against SUTs.                 //
// Use resize_image to safely change window size of image.                  //
//--------------------------------------------------------------------------//
void resize(int noofargs, char **arg)
{    
   int ino=ci, k, xsize, ysize;
   char *title, **label, ***answer;
   char tempstring[64];
   if(noofargs<2) 
   {    title = new char[80]; 
        strcpy(title, "Resize image window");
        label = new char *[3];
        for(k=0;k<3;k++) label[k] = new char[128];
        strcpy(label[0],"Image no.");
        strcpy(label[1],"New X size");
        strcpy(label[2],"New Y size");
        answer = new char **[1];
        answer[0] = new char *[3];
        for(k=0;k<3;k++) answer[0][k] = new char[128];
        sprintf(answer[0][0],"%d",ci);
        sprintf(answer[0][1],"%d",z[ci].xsize);
        sprintf(answer[0][2],"%d",z[ci].xsize);
        getstrings(title, label, NULL, answer, 1, 3, 127);
        ino = atoi(answer[0][0]);
        xsize = atoi(answer[0][1]);
        ysize = atoi(answer[0][2]);
        for(k=0; k<3; k++) delete[] answer[0][k];
        delete[] answer[0];
        delete[] answer;
        for(k=0; k<3; k++) delete[] label[k];
        delete[] label;
        delete[] title;       
   }else 
   {    xsize = max(1,atoi(arg[1]));
        ysize = max(1,atoi(arg[2]));
   }
   if(between(ino, 1, g.image_count-1))
       resize_image(ino, xsize, ysize);
   else 
   {   sprintf(tempstring,"Cannot resize image %d",ino);
       message(tempstring,ERROR); 
   }
}


//--------------------------------------------------------------------------//
// resize_image                                                             //
// Note: also erases wavelet and fft buffers.                               //
//--------------------------------------------------------------------------//
int resize_image(int ino, int xsize, int ysize)
{
   if( !between(ino, 0, g.image_count) || xsize<=0 || ysize<=0)
   {  message("Error resizing image"); return ERROR; }
   if(g.image_count<1) return ERROR;
   if(g.state == ACQUIRE) return ERROR;
     if(g.diagnose){ printf("Resizing image %d to %dx%d...",ino,xsize,ysize);fflush(stdout); }
   int bytesperpixel,screen_bytesperpixel,bytesperline,f,frames,oxsize,
       oysize,j,k,copyx,copyy;
 
   unzoom(ino);
   frames=z[ino].frames;              
   bytesperpixel = (7+z[ino].bpp)/8;
   bytesperline = xsize*frames*bytesperpixel;
   screen_bytesperpixel = (7+g.bitsperpixel)/8;
   uchar ***backup;
   uchar ***image;
   uchar ***alpha;
   uchar **img;
   uchar *backup_1d;
   uchar *image_1d;
   uchar *img_1d;
   uchar *alpha_1d;      
   XImage *image_ximage;
   XImage *img_ximage;    

   printstatus(INITIALIZING);
   image_1d = (uchar*)malloc(ysize*bytesperline);
   image=make_3d_alias(image_1d, xsize*bytesperpixel, ysize, frames);
   if(image==NULL){ message(g.nomemory, ERROR); }
   image_ximage = createximage(xsize, ysize, z[ino].bpp, image_1d);

   alpha_1d = (uchar*)malloc(((xsize+7)/8)*ysize*frames);
   alpha=make_3d_alias(alpha_1d, (xsize+7)/8, ysize, frames);
   if(alpha==NULL){ message(g.nomemory, ERROR); }

   img_1d  = (uchar*)malloc(xsize*ysize*screen_bytesperpixel);
   img  = make_2d_alias(img_1d, xsize*screen_bytesperpixel, ysize);
   if(img==NULL) { message(g.nomemory,ERROR); } 
   img_ximage=createximage(xsize,ysize,g.bitsperpixel,img_1d);

   backup_1d = (uchar*)malloc(ysize*bytesperline);                          
   backup = make_3d_alias(backup_1d, xsize*bytesperpixel, ysize, frames);

   oxsize = z[ino].xsize;
   oysize = z[ino].ysize;
   copyx = min(xsize,oxsize);
   copyy = min(ysize,oysize);

   for(f=0;f<frames;f++)
     for(j=0;j<ysize;j++)
        putpixelbytes(image[f][j], g.bkg_image_color, xsize, z[ino].bpp, 1);

   for(f=0;f<frames;f++)
   { for(j=0;j<copyy;j++)
     {    memcpy(image[f][j], z[ino].image[f][j], copyx*bytesperpixel);
          memcpy(alpha[f][j], z[ino].alpha[f][j], (copyx+7)/8);
          memcpy(img[j], z[ino].img[j],  copyx*screen_bytesperpixel);
          if(z[ino].backedup)
              memcpy(backup[f][j], z[ino].backup[f][j], copyx*bytesperpixel);
     }
   }
   z[ino].xsize = xsize;
   z[ino].ysize = ysize;
   z[ino].xscreensize = xsize;
   z[ino].yscreensize = ysize;

   if(z[ino].s->created) delete_spreadsheet(0,0);
   if(z[ino].backedup){ free_3d(z[ino].backup, z[ino].frames);
                         free(z[ino].backup_1d);   }    
   free_3d(z[ino].image, z[ino].frames);
   free(z[ino].image_1d);
   free_3d(z[ino].alpha, z[ino].frames);
   free(z[ino].alpha_1d);
   XFree(z[ino].image_ximage);    
   free(z[ino].img);
   free(z[ino].img_1d);
   XFree(z[ino].img_ximage);    

   //// These 2 lines prevent wavelets and FFTs from coexisting. 
   //// Anyone wanting FFT of a wavelet should add code here to 
   //// resize the fft and wavelet buffers.
   
   if(z[ino].floatexists) erase_fft(ino);
   if(z[ino].waveletexists) erase_wavelet(ino);
   if(z[ino].backedup){ z[ino].backup = backup;
                        z[ino].backup_1d = backup_1d; }
   z[ino].image = image;                       
   z[ino].image_1d = image_1d;                       
   z[ino].image_ximage = image_ximage;                       

   z[ino].alpha = alpha;                       
   z[ino].alpha_1d = alpha_1d; 

   z[ino].img = img;                       
   z[ino].img_1d = img_1d;                       
   z[ino].img_ximage = img_ximage;   

   resize_widget(z[ino].widget, xsize, ysize);
   if(z[ino].shell)
      resize_widget(XtParent(z[ino].widget), xsize, ysize);
   repair(ino);
   redraw(ino);                    
   printstatus(NORMAL);
   for(k=0;k<g.openlistcount;k++) update_list(k);
   if(g.diagnose) printf("done\n");
   return OK;
}


//--------------------------------------------------------------------------//
// resize_img - resize widget and img screen buffer only                    //
// Does not copy data; caller must restore image data                       //
//--------------------------------------------------------------------------//
int resize_img(int ino, int xsize, int ysize)
{
   if( !between(ino, 0, g.image_count) || xsize<=0 || ysize<=0)
   if(g.image_count<1) return ERROR;
   if(g.state == ACQUIRE) return ERROR;
     if(g.diagnose){ printf("Resizing img %d to %dx%d...",ino,xsize,ysize);fflush(stdout); }
   int bytesperpixel,screen_bytesperpixel,bytesperline,frames,oxsize,
       oysize,k,copyx,copyy;

   frames=z[ino].frames;              
   bytesperpixel = (7+z[ino].bpp)/8;
   bytesperline = xsize*frames*bytesperpixel;
   screen_bytesperpixel = (7+g.bitsperpixel)/8;

   uchar **img;
   uchar *img_1d;
   XImage *img_ximage;    

   printstatus(INITIALIZING);
   img_1d  = (uchar*)malloc(xsize*ysize*screen_bytesperpixel);
   if(img_1d==NULL) { message(g.nomemory,ERROR); } 
   img  = make_2d_alias(img_1d, xsize*screen_bytesperpixel, ysize);
   if(img==NULL) { message(g.nomemory,ERROR); } 
   img_ximage=createximage(xsize,ysize,g.bitsperpixel,img_1d);

   oxsize = z[ino].xsize;
   oysize = z[ino].ysize;
   copyx = min(xsize,oxsize);
   copyy = min(ysize,oysize);

   free(z[ino].img);
   free(z[ino].img_1d);
   XFree(z[ino].img_ximage);    
   z[ino].img = img;                       
   z[ino].img_1d = img_1d;                       
   z[ino].img_ximage = img_ximage;   

   resize_widget(z[ino].widget, xsize, ysize);
   if(z[ino].shell)
      resize_widget(XtParent(z[ino].widget), xsize, ysize);
   printstatus(NORMAL);
   for(k=0;k<g.openlistcount;k++) update_list(k);
   if(g.diagnose) printf("done\n");
   return OK;
}


//--------------------------------------------------------------------------//
// backupimage - back up an image by creating a copy.                       //
//--------------------------------------------------------------------------//
void backupimage(int ino, int notify)
{   
   int f,k, bytesperline;
   char tempstring[100]="0";
   int size;
   char temp[256];
   if(!between(ino, 0, g.image_count-1))
   {   if(notify)
       {   message("Backup which image?",tempstring,PROMPT,10,60); 
           ino = atoi(tempstring);
           if(ino<0 || ino>=g.image_count){ message("Image not found",ERROR); return; }
       }else return;
   }
   size = z[ino].xsize * z[ino].ysize * g.off[z[ino].bpp];
   if(memorylessthan(size))
   {   printf("Insufficient memory to back up image\n");
       message(g.nomemory,ERROR); return; 
   }

   if(g.diagnose){ printf("backing up %d\n",ino);fflush(stdout); }
   memcpy(z[ino].opalette, z[ino].palette, 768);
   memcpy(z[ino].spalette, z[ino].palette, 768);

   bytesperline = z[ino].xsize*g.off[z[ino].bpp];
   if(!z[ino].backedup)
   {   
        z[ino].backup_1d = (uchar*)malloc(z[ino].ysize * bytesperline *
                           sizeof(uchar)*z[ino].frames);                          
        z[ino].backup = make_3d_alias(z[ino].backup_1d, bytesperline,
            z[ino].ysize, z[ino].frames);
   }
   if(z[ino].backup==NULL){  message(g.nomemory,ERROR); return; }       
   z[ino].backedup = 1;     
   z[ino].backup_colortype = z[ino].colortype;
   z[ino].originalbpp = z[ino].bpp;

   for(f=0;f<z[ino].frames;f++)
   for(k=0;k<z[ino].ysize;k++)
        memcpy(z[ino].backup[f][k], z[ino].image[f][k],bytesperline);
   if(notify){ sprintf(temp,"Image # %d backed up",ino); message(temp); }    
}


//--------------------------------------------------------------------------//
// restoreimage - restore a previously backed-up image to the screen        //
//--------------------------------------------------------------------------//
void restoreimage(int want_restore_colortype)
{   
   int bytes, f, k;
   if(ci<0)return;
   char tempstring[128];
   drawselectbox(OFF);
   bytes = z[ci].xsize * g.off[z[ci].bpp];

   if(z[ci].backedup)
   {   if(want_restore_colortype) z[ci].colortype = z[ci].backup_colortype;
       for(f=0;f<z[ci].frames;f++)
       for(k=0;k<z[ci].ysize;k++)
           memcpy(z[ci].image[f][k], z[ci].backup[f][k], bytes);

       memcpy(z[ci].palette, z[ci].spalette, 768);
       memcpy(z[ci].opalette, z[ci].spalette, 768);
       setpalette(z[ci].palette);

       if(z[ci].colortype==GRAY)
           setSVGApalette(1,1);
       else
       {   memcpy(z[ci].palette, g.palette, 768);
           memcpy(z[ci].opalette, z[ci].spalette, 768);
           setpalette(z[ci].palette);
       }
       repair(ci);
   } 
   else
   {   if(g.image_count>0) 
       {  sprintf(tempstring, "Image %d was not backed up", ci);
          message(tempstring);
       }           
       else message("No images");
   }
   if(z[ci].bpp <= 8)
   {   if(z[ci].colortype == COLOR) z[ci].colortype = INDEXED; 
   }else
   {   if(z[ci].colortype == INDEXED) z[ci].colortype = COLOR;
   }

   ////  Warn in case fft'd
   z[ci].fmax = z[ci].gray[0];
   z[ci].fmin = z[ci].gray[1];
   if(z[ci].fftdisplay!=NONE) 
   {   message("Switching display to original\nuntransformed image",WARNING);
       if(z[ci].fftdisplay==WAVE) 
       {    z[ci].waveletstate=0;
            z[ci].fftdisplay=NONE;
       }else
       {    z[ci].fftdisplay=NONE;
            scalefft(ci);    
       }
   }
   for(k=0;k<256;k++) z[ci].red_brightness[k] = k;
   for(k=0;k<256;k++) z[ci].green_brightness[k] = k;
   for(k=0;k<256;k++) z[ci].blue_brightness[k] = k;
   for(k=0;k<256;k++) z[ci].gray_brightness[k] = k;
   repair(ci);
}


//--------------------------------------------------------------------------//
// change_image_depth                                                       //
// Optional parameter 'permanent' can be set to 0 to prevent rebuilding     //
//  screen display (default=1). This is useful for making temporary 24 bit  //
//  image buffers for intermediate calculations. In this case, the backup   //
//  (i.e., z[ino].backup) will be the wrong depth until image is converted  //
//  back to its original depth.                                             //
//--------------------------------------------------------------------------//
int change_image_depth(int ino, int bpp, int permanent)
{
   if(bpp>48){ message("Feature not implemented"); return ABORT; }
   if(g.image_count<1)return BADPARAMETERS;
   if(g.diagnose){ printf("Converting image %d to %d bpp",ino,bpp);fflush(stdout); }
   int bytesperpixel,screen_bytesperpixel,bytesperline,frames,
       f,j,obpp,status=OK,xsize,ysize,obytesperpixel,obytesperline;
   double zoomx,zoomy;
   RGB common_palette[256];
   uchar ***backup=0, ***image, ***alpha, **img, *backup_1d=0, *image_1d,
         *img_1d, *alpha_1d;
   XImage *image_ximage, *img_ximage;    

   obpp = z[ino].bpp;
   if(bpp == obpp) return OK;
   if(permanent>1) fprintf(stderr,"error in change_image_depth\n"); // probable error
   switch_palette(ino);
   frames  = z[ino].frames;
   xsize   = z[ino].xsize;
   ysize   = z[ino].ysize;
   zoomx   = z[ino].zoomx;
   zoomy   = z[ino].zoomy;
   bytesperpixel  = (7+bpp)/8;
   bytesperline   = xsize*bytesperpixel;
   obytesperpixel = (7+obpp)/8;
   obytesperline  = xsize*obytesperpixel;
   screen_bytesperpixel = (7+g.bitsperpixel)/8;

   printstatus(BUSY);
   image_1d     = (uchar*)malloc(frames*ysize*bytesperline);
   image        = make_3d_alias(image_1d, xsize*bytesperpixel, ysize, frames);
   if(image==NULL){ message(g.nomemory, ERROR); } 
   image_ximage = createximage(xsize, ysize, g.bitsperpixel, image_1d);
   alpha_1d     = (uchar*)malloc(((xsize+7)/8)*ysize*frames);
   alpha        = make_3d_alias(alpha_1d, (xsize+7)/8, ysize, frames);
   if(alpha==NULL){ message(g.nomemory, ERROR); }

   int size1, size2, size3, size4, zx, zy;
   zx = cint((double)xsize * zoomx);
   zy = cint((double)ysize * zoomy);
   size1 = screen_bytesperpixel * zx * zy;
   size2 = screen_bytesperpixel * zx;
   size3 = zx;
   size4 = zy;
   img_1d       = (uchar*)malloc(8+size1);
   img          = make_2d_alias(img_1d, size2, size4);
   if(img==NULL) { message(g.nomemory,ERROR); } 
   img_ximage   = createximage(size3, size4, g.bitsperpixel, img_1d);

   if(permanent && z[ino].backedup)
   {   backup_1d = (uchar*)malloc(frames*ysize*bytesperline);                          
       backup = make_3d_alias(backup_1d, xsize*bytesperpixel, ysize, frames);
   }
 
   if(z[ino].colortype==INDEXED && bpp>8) z[ino].colortype=COLOR;
   else if(z[ino].colortype==COLOR && bpp<=8) z[ino].colortype=INDEXED;

   setpalette(z[ino].palette);
   memcpy(common_palette, z[ino].palette, 768);
   switch_palette(ino);

   if(permanent && z[ino].backedup)        // Convert backup
   { 
      if(bpp>obpp)                         // Up-convert 
      {  
           setpalette(z[ino].spalette);
           bytesperpixel = (7+obpp)/8;
           obytesperline = xsize*bytesperpixel;
           for(f=0;f<frames;f++)           // Copy to new image buffer
           for(j=0;j<ysize;j++)            // Copy to new backup buffer
                memcpy(backup[f][j], z[ino].backup[f][j], obytesperline);
                                           // Convert in place to bpp
           status = change_depth(ino, frames, z[ino].backup_colortype, 
                backup, backup, obpp, bpp, 0, z[ino].spalette); 
           if(g.want_quantize==3) memcpy(g.palette, common_palette, 768);
      }else                                // Down-convert
      { 
          status = change_depth(ino, frames, z[ino].backup_colortype, 
                 z[ino].backup, z[ino].backup, obpp, bpp, 0, z[ino].spalette);
                                           // Copy into new backup buffer
          if(g.want_quantize==3) memcpy(g.palette, common_palette, 768);
          bytesperpixel = (7+bpp)/8;
          bytesperline = xsize*bytesperpixel;
          for(f=0;f<frames;f++)           // Copy to new image buffer
          for(j=0; j<ysize; j++) 
              memcpy(backup[f][j], z[ino].backup[f][j], bytesperline);
          if(g.want_quantize==3) memcpy(g.palette, common_palette, 768);
          else switch_palette(ino);
      }
      
   }

   if(bpp>obpp)                            // Up-convert 
   {  
       for(f=0;f<frames;f++)               // Copy to new image buffer
       for(j=0;j<ysize;j++)                // Copy to new image buffer
           memcpy(image[f][j], z[ino].image[f][j], obytesperline);
                                           // Change image bufr depth in place
                                           // New image is now bpp in temp bufr
       status = change_depth(ino, frames, z[ino].colortype, image, image, 
                obpp, bpp, 1, z[ino].palette); 
       if(g.want_quantize==3) memcpy(g.palette, common_palette, 768);
   }else                                   // Down-convert
   {                                       // Convert old image to bpp in ino
                                           // Turn off chromakey
       if(z[ino].chromakey) z[ino].chromakey = 0;
       status = change_depth(ino, frames, z[ino].colortype, z[ino].image,
              z[ino].image, obpp, bpp, 1, z[ino].palette);
       if(g.want_quantize==3) memcpy(g.palette, common_palette, 768);
       for(f=0;f<frames;f++)               // Copy to new image buffer
       for(j=0;j<ysize;j++)                // Copy into new image buffer
           memcpy(image[f][j], z[ino].image[f][j], xsize*g.off[bpp]);
       if(g.want_quantize==3) memcpy(g.palette, common_palette, 768);
       switch_palette(ino);
   }

   if(permanent && z[ino].backedup)
   {    free_3d(z[ino].backup, z[ino].frames);
        free(z[ino].backup_1d);   
   }    
   free_3d(z[ino].image, z[ino].frames);
   free(z[ino].image_1d);
   free_3d(z[ino].alpha, z[ino].frames);
   free(z[ino].alpha_1d);
   XFree(z[ino].image_ximage);    
   free(z[ino].img);
   free(z[ino].img_1d);
   XFree(z[ino].img_ximage);    
   if(permanent && z[ino].backedup)
   {   z[ino].backup = backup;
       z[ino].backup_1d = backup_1d; 
   }
   z[ino].image = image;                       
   z[ino].image_1d = image_1d;                       
   z[ino].alpha = alpha;                       
   z[ino].alpha_1d = alpha_1d; 
   z[ino].image_ximage = image_ximage;                       
   z[ino].img = img;                       
   z[ino].img_1d = img_1d;                       
   z[ino].img_ximage = img_ximage;   
   z[ino].bpp = bpp;
   z[ino].touched=1;
   printstatus(NORMAL);
   if(g.diagnose) printf("done\n");
#ifndef LITTLE_ENDIAN
   if(permanent && bpp==24 && (obpp != 24)) swap_image_bytes(ino);
#endif
   z[ino].hitgray = 0;    // Force remapping of gray levels
   z[ino].adjustgray = 1; // Allow remapping of gray levels
   return status;
}


//--------------------------------------------------------------------------//
// sortimages - create a list of images sorted by size                      //
//--------------------------------------------------------------------------//
void sortimages(void)
{  int j,k,s1,s2;
   for(k=0; k<g.image_count; k++) z[k].sort=k;
   for(k=0; k<g.image_count; k++)
   {    s1 = z[k].xsize * z[k].ysize * z[k].frames;
        for(j=k+1; j<g.image_count; j++)
        {    s2 = z[j].xsize * z[j].ysize * z[j].frames;
             if(s2>s1) swap(z[k].sort,z[j].sort);
        }
   }
}


//--------------------------------------------------------------------------//
// memorylessthan  - returns TRUE if less than specified free memory        //
// Must use malloc, not new because new will just crash.                    //
//--------------------------------------------------------------------------//
int memorylessthan(int amount)
{
   int answer=FALSE;
   char* crud;
   crud = (char*)malloc(amount);    
   if(crud==NULL){ answer=TRUE; printf("Insufficient memory\n"); }
   free(crud);
   return answer;
}


//--------------------------------------------------------------------------//
// make_2d_alias                                                            //
// usage: uint** array;                                                     //
//        uint* array_1d;                                                   //
//        array_1d = (uint*)malloc(x*y*sizeof(uint));                       //
//        array = make_2d_alias(array_1d,x,y);   // Dont mult.by sizeof uint//
//         ...                                   // compiler knows its size //
//        array[y][x] = 4;                                                  //
//        array_1d[y*ysize + x] = 4;                                        //
//        free(array);                                                      //
//        free(array_1d);                                                   //
// This method allows the same data to be addressed using [][] notation as  //
//   a matrix, or by [] notation for compatibility with older libraries     //
//   (such as Xlib) which expect the data as a 1-D array. It also permits   //
//   the array to be moved or memcpy'd as a memory block. It is only        //
//   necessary to call make_2d_alias again.                                 //
// WARNING: parameters are given as (x,y) but the array must be addressed   //
//   as array[y][x]. This is critical for performance in case the array is  //
//   paged out.                                                             //
// NOTE: For 3d arrays, you must use free_3d instead of free() or delete[]. //
//--------------------------------------------------------------------------//
int **make_2d_alias(int *b, int x, int y)
{  int k;
   int **result;
   result = (int**)malloc(y*sizeof(int *));
   for(k=0;k<y;k++) result[k] = b + k*x;
   return result;
}
uint **make_2d_alias(uint *b, int x, int y)
{  int k;
   uint **result;
   result = (uint**)malloc(y*sizeof(uint *));
   for(k=0;k<y;k++) result[k] = b + k*x;
   return result;
} 
char **make_2d_alias(char *b, int x, int y)
{  int k;
   char **result;
   result = (char**)malloc(y*sizeof(char *));
   for(k=0;k<y;k++) result[k] = b + k*x;
   return result;
} 
uchar **make_2d_alias(uchar *b, int x, int y)
{  int k;
   uchar **result;
   result = (uchar**)malloc(y*sizeof(uchar *));
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
complex **make_2d_alias(complex *b, int x, int y)
{  int k;
   complex **result;
   result = (complex**)malloc(y*sizeof(complex *));
   for(k=0;k<y;k++) result[k] = b + k*x;
   return result;
} 

double ***make_3d_alias(double *b, int x, int y, int z)
{ int j,k;
  double ***result;
  result = new double**[z];
  for(j=0;j<z;j++)
  { result[j]=new double*[y];
    for(k=0;k<y;k++) 
      result[j][k] = b + j*x*y + k*x;      
  }
  return result;                      
}


int ***make_3d_alias(int *b, int x, int y, int z)
{ int j,k;
  int ***result;
  result = new int**[z];
  for(j=0;j<z;j++)
  { result[j]=new int*[y];
    for(k=0;k<y;k++) 
      result[j][k] = b + j*x*y + k*x;      
  }
  return result;                      
}

uchar ***make_3d_alias(uchar *b, int x, int y, int z)
{ int j,k;
  uchar ***result;
  result = new uchar**[z];
  for(j=0;j<z;j++)
  { result[j]=new uchar*[y];
    for(k=0;k<y;k++) 
      result[j][k] = b + j*x*y + k*x;      
  }
  return result;                      
}

void free_3d(double ***b, int z)
{ int j;
  for(j=0;j<z;j++)
     delete[] b[j];
  delete[] b;  
}

void free_3d(int ***b, int z)
{ int j;
  for(j=0;j<z;j++)
     delete[] b[j];
  delete[] b;  
}

void free_3d(uchar ***b, int z)
{ int j;
  for(j=0;j<z;j++)
     delete[] b[j];
  delete[] b;
}

