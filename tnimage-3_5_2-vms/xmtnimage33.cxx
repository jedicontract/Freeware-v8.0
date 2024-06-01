//--------------------------------------------------------------------------//
// xmtnimage33.cc                                                           //
// Latest revision: 03-04-2006                                              //
// Copyright (C) 2006 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
// move, copy, spray, math                                                  //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
extern PrinterStruct printer;
int status=OK;
int inmath=0;

//-------------------------------------------------------------------------//
// move                                                                    //
// move or copy part of image from one area on the screen to another       //
//-------------------------------------------------------------------------//
int move(void)
{ 
  if(memorylessthan(16384)){  message(g.nomemory,ERROR); return ERROR; } 
  static int incopy=0;
  if(incopy) return BUSY;
  incopy=1;
  int bpp,cf,citoerase=0,done=0,frame=0,h,i,ii,ino,oino,j,jj,k,
      w,x1,x2,y1,y2,hitbkg=0,want_remap=1;
  uint value;
  RGB rgb;
  uchar *savebpp_1d;
  uchar **savebpp;
  uchar *alpha_1d;
  uchar **alpha;
  int *repairflags;
  g.getout = 0;
  if(g.getout){ g.getout=0; incopy=0; return ABORT; }
  for(k=0; k<g.image_count; k++) z[k].hit=0;
  hitbkg=0;
  citoerase=0;   
  printstatus(COPY);
  x1 = g.ulx;
  x2 = g.lrx;
  y1 = g.uly;
  y2 = g.lry;
  w = x2-x1;
  h = y2-y1;
  drawselectbox(OFF);

  ////  Make a temporary image at 24 bpp.
  if(newimage("Temporary image",x1,y1,w,h,24,COLOR,1,0,0,TEMP,0,g.window_border,0)!=OK)
  {  incopy=0; return(NOMEM); }
  
  savebpp_1d = new uchar[(h+1)*(w+1)];
  savebpp = make_2d_alias(savebpp_1d, w+1, h+1);
  alpha_1d = new uchar[(h+1)*(w+1)];
  alpha = make_2d_alias(alpha_1d, w+1, h+1);
  repairflags = new int[MAXIMAGES];

  ////  Make sure correct temporary image gets erased.
  citoerase = ci;

  ////  Get stuff to move, save it as pseudo-24 bpp, and record its original bpp. 
  ////  This causes pixel depths to be converted only if their bpp is different 
  ////  from the destination.
  ////  Make sure pixels aren't read from the temp. image (citoerase).

  cf = z[citoerase].cf;
  oino = ci;
  rgb.red = 0;
  rgb.green = 0;
  rgb.blue = 0;

  for(j=y1;j<y2;j++)
  for(i=x1;i<x2;i++)
  {    
       value = readpixelonimage(i,j,bpp,ino,citoerase);
       if(ino>=0) frame = z[ino].cf; else frame=0;
       if(ino!=oino)
       {   if(ino>=0)
           {   if(z[ino].bpp==8) switch_palette(ino); }
           else 
           {   if(g.bitsperpixel==8) memcpy(g.palette, g.b_palette, 768);}
       }oino=ino;

       ii = i-x1;
       jj = j-y1;
       if(!between(ii, 0, z[citoerase].xsize-1) ||
          !between(jj, 0, z[citoerase].ysize-1)) continue;
       putpixelbytes(z[citoerase].image[cf][jj]+g.off[24]*ii, value, 1, 24, 1);
       value = convertpixel(value, bpp, g.bitsperpixel, 1);  
       putpixelbytes(z[citoerase].img[jj] + g.off[g.bitsperpixel]*ii, 
           value, 1, g.bitsperpixel, 1);
       savebpp[jj][ii] = bpp;
       if(ino>=0)       
           alpha[jj][ii]=get_alpha_bit(ino,frame,i-z[ino].xpos,j-z[ino].ypos);
       else
           alpha[jj][ii]=get_alpha_bit(-1,0,i,j);
  }

  repairimg(citoerase,0,0,w,h);
  
  ////  Erase the old region if 'move' was selected.
  for(k=0;k<MAXIMAGES;k++) repairflags[k]=0;
  if(!g.copy)
  {  for(j=y1;j<y2;j++)
     {  for(i=x1;i<x2;i++)
        {   if(!g.selected_is_square && !inside_irregular_region(i,j)) continue;
            erase_pixel(i,j,repairflags);
            done = repairflags[0] & 4;
            if(done) break;
        }if(done) break;
     }
  }
  
  ////  Wait for user to figure out where to put it.     

  user_position_copy(citoerase,x1,y1,x2,y2,want_remap,CHROMAKEY_NONE,
      0,0,0,rgb,rgb,24,savebpp,alpha,hitbkg,0);
  eraseimage(citoerase,0,0,0);                    // Erase the temporary image bufr
  delete[](savebpp);
  free(savebpp_1d);
  delete[](alpha);
  free(alpha_1d);

  for(k=0; k<g.image_count; k++) if(z[k].hit){ rebuild_display(k); redraw(k); }
  incopy=0;
  drawselectbox(ON);
  delete[] repairflags;
  return OK;
}


//-------------------------------------------------------------------------//
// user_position_copy                                                      //
// want_remap should be 1 for images, 0 for text                           //
// citoerase is image no. of the pixels being copied.                      //
// mode can be:                                                            //
//    CHROMAKEY_EQUAL = only sets pixels if they are between rgb_min and   //
//        rgb_max or gray_min & gray_max.                                  //
//    NONE or CHROMAKEY_NONE = sets all pixels                             //
//    ANTI_ALIAS = adjusts transparency depending on whether value is      //
//        closer to rgb_max or gray_max, or closer to rgb_min or gray_min. // 
//        (This is conditional on the setting of g.rotate_anti_alias).     //
// If force_alpha is set, alpha channel is set for all pixels, and **alpha //
//    can be NULL. Otherwise, alpha[][] is used to set the alpha channel   //
//    of the destination pixel.                                            //
//-------------------------------------------------------------------------//
void user_position_copy(int citoerase, int x1, int y1, int x2, int y2,
    int want_remap, int mode, int force_alpha, 
    int gray_min, int gray_max,  RGB rgb_min, RGB rgb_max, 
    int bpp, uchar **savebpp, uchar **alpha, int &hitbkg, int want_background)
{
   int ii,j,k,oino,x,y,ino,winx,winy,source_bpp,dest_bpp,opix,orr,ogg,obb,
       rx,ry,hit=0,dx,dy,value,rr,gg,bb,wantcolor=1,ulx,uly,lrx,lry,
       remapped=0,want_alpha=0,ibpp,rmin,gmin,bmin,rmax,gmax,bmax,
       ox=0,oy=0,rc,bc,gc,iii,omouse_button = 0;
   Window win, rwin, cwin;
   uint keys;
   g.state = GETPOINT;
   ino = oino = g.imageatcursor;
   int owant_title = g.want_title;
   g.want_title = 0;
   dx = x2-x1;
   dy = y2-y1;
   int is_square = g.selected_is_square;
   int remap[256];
   g.block++;
   omouse_button = g.mouse_button;

   while(g.state==GETPOINT)
   {  
          XtAppProcessEvent(g.app, XtIMAll);
          if(g.getout) break;
          if(g.state != GETPOINT) break;
          XQueryPointer(g.display,g.main_window,&rwin,&cwin,&rx,&ry,&x,&y,&keys);
          if(g.mouse_button != omouse_button) break;
          if(x!=ox || y!=oy)
          {
             if(g.imageatcursor != citoerase) ino = g.imageatcursor;
             ////  Reparent to whatever window the user moves to.
             win = z[ino].win;
             winx = x - z[ino].xpos;
             winy = y - z[ino].ypos;
             if(g.floating_magnifier && g.state!=MOVEIMAGE)
                   update_floating_magnifier(x, y);
             printcoordinates(x, y, 1); 
             g.mouse_x = x;
             g.mouse_y = y;
             if(ino != oino || !hit) 
             {     switch_palette(ino);  
                   XReparentWindow(g.display, z[citoerase].win, win, winx, winy);
                   moveimage(citoerase, winx, winy, 1);
                   oino = ino;
                   hit=1;
             }
             ////  Window coordinates are relative to parent
             moveimage(citoerase,winx,winy,0);
             ox=x;oy=y;
          }
   }  
   g.block = max(0, g.block-1);
   if(g.getout) return;
   ////  Put moving Widget back where it belongs so it doesn't interfere

   XReparentWindow(g.display, z[citoerase].win, g.main_window, 2*g.xres, 2*g.yres);
   moveimage(citoerase, 2*g.xres, 2*g.yres);

   ////  Switch to the new window to make sure it gets precedence for 
   ////  whichimage(), if in a separate window.  Set the palette to the
   ////  palette of the source image, so that pixels are converted using
   ////  their original color attributes.

   if(citoerase>=0) switch_palette(citoerase);
   oino = -2;
 
   if(z[ino].is_zoomed)
   {    x = zoom_x_index(x, ino) + z[ino].xpos;
        y = zoom_y_index(y, ino) + z[ino].ypos;
   }

   ino = whichimage(x,y,dest_bpp,citoerase);
   ulx = x;
   uly = y;
   lrx = x+dx;
   lry = y+dy;
   printstatus(COPY);
   drawselectbox(OFF);
   
   for(j=uly;j<lry;j++)                             // Put stuff permanently
   for(k=ulx;k<lrx;k++)                             // Destination coordinates
   {    
          if(j-y<0 || k-x<0) continue;

          if(!force_alpha && !is_square && 
             !inside_irregular_region(k-ulx+g.selected_ulx, j-uly+g.selected_uly)) 
             continue;
          ino = whichimage(k,j,dest_bpp,citoerase);
          ////  Use array instead of normal alpha channel for speed
          if(force_alpha) want_alpha=1; else want_alpha = alpha[j-y][k-x];
          source_bpp = savebpp[j-y][k-x];            
          if(ino!=oino)
          {    
               //// This speeds up RGVvalue() and makes antialiasing more accurate.
               if(g.bitsperpixel==8 && g.rotate_anti_alias && ino>0 && z[ino].colortype==INDEXED)
               {   switchto(ino);
                   sortpalette(ino);
               }
               //// Specify cases where it is necessary to redraw image after paste.
               //// Normal, unzoomed, 24 bit image doesn't need it.
               if(ino>=0)
               {    if(g.bitsperpixel==8 && source_bpp!=8) z[ino].hit=1;
                    if(z[ino].is_zoomed) z[ino].hit=1;
                    if(source_bpp != dest_bpp) z[ino].hit=1;
                    if(dest_bpp==8) z[ino].hit=1;
                    if(!g.wantr || !g.wantg || !g.wantb) z[ino].hit=1;
                    if(g.imode != SET) z[ino].hit=1;
                    if(z[ino].bpp==8) switch_palette(ino);
               }else 
               {    hitbkg=1;
                    if(g.bitsperpixel==8) memcpy(g.palette, g.b_palette, 768);
               }

               if(want_remap && citoerase>=0 && source_bpp==8 && dest_bpp==8 && ino>=0)
               {    remapped=1;
                    remap_palette(z[citoerase].palette,z[ino].palette,remap);
               }else remapped=0;
               oino=ino;
               if(dest_bpp!=8 && citoerase>=0) switch_palette(citoerase);
          }

          if(ino>=0)
               value = pixelat(z[citoerase].image[0][j-y] + g.off[bpp]*(k-x), bpp);
          else
               value = pixelat(z[citoerase].img[j-y]+g.off[g.bitsperpixel]*(k-x),g.bitsperpixel);

          //// Chromakey/antialias copy - skip some pixels or make transparent
          
          if(mode==CHROMAKEY_EQUAL)
          {        if(z[citoerase].colortype==GRAY)
                   {    if(!is_opaque(value,gray_min,gray_max,mode)) 
                            continue; 
                   }else
                   {    valuetoRGB(value,rr,gg,bb,bpp);
                        if(!is_opaque(rr,rgb_min.red,rgb_max.red,
                              gg,rgb_min.green,rgb_max.green,
                              bb,rgb_min.blue,rgb_max.blue, mode)) 
                            continue; 
                   }
                   value = g.fcolor;
          }
          if(mode==ANTI_ALIAS && g.rotate_anti_alias)
          {
                   if(want_background)
                       opix = g.bcolor;
                   else
                       opix = readpixelonimage(k,j,ibpp,iii);
                   if(ibpp != g.bitsperpixel)
                        opix = convertpixel(opix, ibpp, g.bitsperpixel, 1);  
                   if(z[citoerase].colortype==GRAY)
                   {   
                       if(!is_opaque(value, gray_min, gray_max-2, CHROMAKEY_NORMAL)) 
                            continue; 
                        value = intermediate_pixel_gray(g.fcolor, value, opix, 
                                      gray_max, gray_min);
                   }else
                   {    valuetoRGB(value,rr,gg,bb,bpp);
                        rmin = rgb_min.red;   rmax = rgb_max.red;
                        gmin = rgb_min.green; gmax = rgb_max.green;
                        bmin = rgb_min.blue;  bmax = rgb_max.blue;
                        if(!want_background && !is_opaque(rr, rmin, rmax,
                                      gg, gmin, gmax,
                                      bb, bmin, bmax,
                                      CHROMAKEY_NORMAL)) 
                              continue; 
                        valuetoRGB(g.fcolor,rc,gc,bc,bpp);
                        valuetoRGB(opix, orr, ogg, obb, bpp);
                        value = intermediate_pixel_color(
                                   rc, gc, bc,              // color to use
                                   rr, gg, bb,              // anti-alias value
                                   orr, ogg, obb,           // bkg color
                                   rmax, gmax, bmax,        // color for opaque
                                   rmin, gmin, bmin, bpp);  // color for transparent
                   }
          }
          if(mode==ANTI_ALIAS_INVERT)
          {        if(want_background)
                       opix = g.bcolor;
                   else
                       opix = readpixelonimage(k,j,ibpp,iii);
                   if(ibpp != g.bitsperpixel)
                        opix = convertpixel(opix, ibpp, g.bitsperpixel, 1);  
                   valuetoRGB(value,rr,gg,bb,bpp);
                   rmin = rgb_min.red;   rmax = rgb_max.red;
                   gmin = rgb_min.green; gmax = rgb_max.green;
                   bmin = rgb_min.blue;  bmax = rgb_max.blue;
                   if(rr==255 && gg==255 && bb==255) continue; 
                   valuetoRGB(g.fcolor,rc,gc,bc,bpp);
                   valuetoRGB(opix, orr, ogg, obb, bpp);
                   value = intermediate_pixel_color(
                                   rc, gc, bc,              // color to use (the answer)
                                   rr, gg, bb,              // anti-alias value from mask
                                   orr, ogg, obb,           // bkg color (old pixel)
                                   0, 0, 0,                 // color for opaque
                                   255, 255, 255, bpp);     // color for transparent
          }
          if(source_bpp != dest_bpp)
                value = convertpixel(value, source_bpp, dest_bpp, wantcolor);  
          
#ifndef LITTLE_ENDIAN
          if(dest_bpp==8 || dest_bpp==32 || dest_bpp==16) value = swap_pixel_bytes(value, dest_bpp);
#endif

          if(remapped)    // remap colormap
                ii = setpixelonimage(k,j,remap[value],g.imode,dest_bpp,
                     citoerase,0,0,want_alpha,0,0);
          else
                ii = setpixelonimage(k,j,value,g.imode,dest_bpp,
                     citoerase,0,0,want_alpha,0,0);
          if(ii!=citoerase) ino=ii;
   }
   g.state = NORMAL;
   printstatus(NORMAL);
   g.want_title = owant_title;
   return;
}


//-------------------------------------------------------------------------//
//  intermediate_pixel_color                                               //
//  c=color to set pixel                                                   //
//  1=anti-aliased value (0=background to g.maxcolor=foreground)           //
//  2=background value                                                     //
//  3=rgb for 100% opaque (set c = 1) = max                                //
//  4=rgb for 100% transparent (set c = 2) = min                           //
//-------------------------------------------------------------------------//
int intermediate_pixel_color(int rc, int gc, int bc, 
     int r1, int g1, int b1, int r2, int g2, int b2,
     int r3, int g3, int b3, int r4, int g4, int b4, int bpp)
{
   double rtran=0.0, gtran=0.0, btran=0.0;  // transparency 0-1
   int rr, gg, bb;
   if(r3!=r4) rtran = (double)(r1-r4) / (double)(r3-r4);
   if(g3!=g4) gtran = (double)(g1-g4) / (double)(g3-g4);
   if(b3!=b4) btran = (double)(b1-b4) / (double)(b3-b4);
   rr = cint(rtran*rc + (1.0-rtran)*r2);
   gg = cint(gtran*gc + (1.0-gtran)*g2);
   bb = cint(btran*bc + (1.0-btran)*b2);
   return RGBvalue(rr,gg,bb,bpp);
}

     
//-------------------------------------------------------------------------//
//  intermediate_pixel_gray                                                //
//-------------------------------------------------------------------------//
int intermediate_pixel_gray(int vc, int v1, int v2, int v3, int v4)
{
   double tran=0.0;   // opacity 0-1
   if(v3!=v4) tran = (double)(v1-v4) / (double)(v3-v4);
   return cint(tran*vc + (1.0-tran)*v2);
} 


//-------------------------------------------------------------------------//
//  paste                                                                  //
//-------------------------------------------------------------------------//
void paste(void)
{
   if(ci<0){ message("No images selected",ERROR); return; }
   int bpp,ibpp,cf,i,ii=0,ino,j,k,v,x,y,destino,destbpp;
   cf = z[ci].cf;
   ibpp = z[ci].bpp;
   int hit[MAXIMAGES];
   for(k=0;k<g.image_count;k++) hit[k]=0;
   drawselectbox(OFF);
   for(y=g.selected_uly;y<g.selected_lry;y++)
   for(x=g.selected_ulx;x<g.selected_lrx;x++)
   {    
               if(!g.selected_is_square && !inside_irregular_region(x,y)) continue;
               ino = whichimg(x,y,bpp);
               destino = whichimg(x,y,destbpp,ino);
               if(ino==ci) 
               {     cf = z[ino].cf;
                     j = y - z[ino].ypos;
                     i = x - z[ino].xpos;
                     if(!between(i,0,z[ino].xscreensize-1) || !between(j,0,z[ino].yscreensize-1)) continue;
                     v = readpixel(x,y);
                     v = convertpixel(v, g.bitsperpixel, bpp, 1);
                     z[ci].xpos += 2*g.xres;
                     ii = setpixelonimage(x,y,v,SET,ibpp,-1,0,0,0,-1,0);
                     if(ii>0 && z[ii].is_zoomed) hit[ii]=1;
                     if(destino>0 && destbpp==8) hit[destino]=1;
                     z[ci].xpos -= 2*g.xres;
               }
   }
   redrawscreen();
   for(k=0; k<g.image_count; k++) if(hit[k]){ hit[k]=0; rebuild_display(k);}
}


//-------------------------------------------------------------------------//
// spray - sprays things                                                   //
//-------------------------------------------------------------------------//
int spray(int noofargs, char **arg)
{
  static Dialog *dialog;
  int j,k;
  if(memorylessthan(16384)){  message(g.nomemory,ERROR); return NOMEM; } 
  g.getout=0;
  if(noofargs>0) 
  {   g.spray_mode = atoi(arg[1]);
      spray_things(NULL);
  }else
  {   dialog = new Dialog;
      dialog->helptopic=44;
      strcpy(dialog->title,"Spray");

      ////--Radio buttons--////

      strcpy(dialog->radio[0][0],"Spray mode");
      strcpy(dialog->radio[0][1],"Fine spray");
      strcpy(dialog->radio[0][2],"Diffuse spray");
      strcpy(dialog->radio[0][3],"Math spray");
      strcpy(dialog->radio[0][4],"Filter spray");
      strcpy(dialog->radio[0][5],"Erase spray");
      strcpy(dialog->radio[0][6],"Line pattern spray");
      dialog->radioset[0] = g.spray_mode;
      dialog->radiono[0]=7;
      dialog->radiono[1]=0;
      for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
      dialog->noofradios=1;

      ////-----Boxes-----////

      strcpy(dialog->boxes[0],"Spray parameters");             
      strcpy(dialog->boxes[1],"Spray factor");             
      dialog->boxtype[0]=LABEL;
      dialog->boxtype[1]=INTCLICKBOX;
      dialog->boxmin[1]=1; dialog->boxmax[1] = 200;
          sprintf(dialog->answer[1][0], "%d", g.sprayfac);
      dialog->noofboxes=2;
      dialog->want_changecicb = 0;
      dialog->f1 = spraycheck;
      dialog->f2 = null;
      dialog->f3 = sprayfinish;
      dialog->f4 = null;
      dialog->f5 = null;
      dialog->f6 = null;
      dialog->f7 = null;
      dialog->f8 = null;
      dialog->f9 = null;
      dialog->width = 400;
      dialog->height = 0; // calculate automatically
      dialog->transient = 1;
      dialog->wantraise = 0;
      dialog->radiousexy = 0;
      dialog->boxusexy = 0;
      strcpy(dialog->path,".");
      strcpy(dialog->message, " ");
      strcpy(dialog->message2, "");
      dialog->message_x1 = 167;
      dialog->message_y1 = 100;
      dialog->message_x2 = 0;
      dialog->message_y2 = 0;
      dialog->busy = 0;
      dialogbox(dialog);
  }
  return OK;
}


//--------------------------------------------------------------------------//
//  spraycheck                                                             //
//--------------------------------------------------------------------------//
void spraycheck(dialoginfo *a, int radio, int box, int boxbutton)
{
  radio=radio; box=box; boxbutton=boxbutton;
  g.spray_mode = a->radioset[0];
  g.sprayfac = atoi(a->answer[1][0]);
  if(radio == -2) spray_things(a);  //// User clicked Ok or Enter
}


//--------------------------------------------------------------------------//
//  spray_things                                                            //
//--------------------------------------------------------------------------//
void spray_things(dialoginfo *a)
{
  a=a;
  static filter *ff;
  static char *mathtext;
  static int mathallocated = 0;
  static int inspray = 0;
  g.getout = 0;
  if(inspray) return;

  g.waiting++;
  inspray = 1;
  int alloc1=0,bpp,bytesperline,clicked=0,done,f,i,ino=0,oino=0,hit=0,j,k,
      ox1=0,oy1=0,ox2=0,oy2=0,x1,y1,x2,y2,ok=1,rx,ry,sf,size=0,x,y,hitff=0,hitmath=0,
      ospraymode=-1,dx=0,dy=0,c=0,hitgetline=0,length=0,button=0;
  int xx,yy,oldx=0,oldy=0;
  static int oldx1=0, oldx2=0, oldy1=0, oldy2=0;
  int tempalloc[MAXIMAGES];             
  int changed[MAXIMAGES];
  int *repairflags;
  uchar **tempbackup[MAXIMAGES]; // Allocate maximum no. in case user creates a new one
  uchar *tempbackup_1d[MAXIMAGES]; 
  char **touched=NULL;
  char *touched_1d=NULL;
  Window rwin, cwin;
  uint mask;
  double angle=0.0, newangle=0.0, xe, ye, di, dlength=0, frac, dj;

  g.state=SPRAY;
  if(g.getout)
  {    g.getout=0; 
       inspray=0;  
       g.waiting = max(0, g.waiting-1);
       return; 
  }

  //// Make a buffer to record which pixels have been touched already
  size = g.xres*g.yres;
  touched_1d = new char[size+1];
  if(touched_1d == NULL)
  {    message(g.nomemory,ERROR); 
       inspray=0;    
       g.waiting = max(0, g.waiting-1);
       return; 
  } 
  memset(touched_1d, 0, size*sizeof(char));
  touched = make_2d_alias(touched_1d, g.xres, g.yres);
  alloc1=1;

  repairflags = new int[MAXIMAGES];
  for(k=0; k<MAXIMAGES; k++)
  {  tempalloc[k]=0; changed[k]=0; repairflags[k]=0; }
  for(k=0; k<g.image_count; k++) z[k].hit=0;

  ////  Start spraying - no return() beyond this point
  g.getout=0;

  g.draw_figure = SPRAY;
  if(g.spray_mode == LINESPRAY && !hitgetline)
     message("Select line to use as pattern");

  if(g.spray_mode == MATHSPRAY && !hitmath)
  {    if(!mathallocated) 
       {    mathtext = new char[65536];
            mathtext[0] = 0;
       }
       mathallocated = 1;
       hitmath = 1;
       get_math_formulas(mathtext, SPRAY);
       g.getout = 0;
  }
  if(g.spray_mode == FILTERSPRAY && !hitff)
  {    ff = new filter;
       ff->ino = ci;
       ff->type  = g.filter_type;
       ff->x1 = ff->x2 = z[0].xpos+1;
       ff->y1 = ff->y2 = z[0].ypos+1;
       ff->ksize = g.filter_ksize;
       ff->sharpfac = g.filter_sharpfac;
       ff->range = g.filter_range;
       ff->kmult = 1;
       ff->maxbkg   = g.filter_maxbkg;
       ff->entireimage = 0;
       ff->diffsize = g.filter_diffsize;
       ff->filename[0] = 0;
       ff->ithresh  = g.filter_ithresh;
       ff->local_scale = g.filter_local_scale;
       ff->want_progress_bar = 0;
       ff->do_filtering = 0;   // just set up
       filter_image(ff);
       hitff=1;
  }


  g.getout = 0;
  while(g.draw_figure)
  {  
     g.getout = 0;
     check_event(); 
     XQueryPointer(g.display, g.main_window, &rwin, &cwin, &rx, &ry, &x, &y, &mask); 
     check_event(); 
     if(!g.draw_figure) break;
     if(g.spray_mode != ospraymode){ hitff = hitmath = 0; }
     ospraymode = g.spray_mode;
     ino = whichimage(cwin, bpp);
     if(mask && Button1Mask && between(x,0,z[0].xsize) && between(y,0,z[0].ysize)
         && ino>=0) 
     {   clicked = 1;
         if(!g.draw_figure) break;
         ino = whichimg(x,y,bpp);
         switch(g.spray_mode)
         {   case MATHSPRAY:
                 for(j=y-g.sprayfac; j<=y+g.sprayfac; j++)       
                 {   for(i=x-g.sprayfac; i<=x+g.sprayfac; i++)       
                     {   
                         if(between(j,0,g.yres) && between(i,0,g.xres))
                         {   if(touched[j][i]) ok=0; else ok=1;
                             touched[j][i] = 1;
                         }else ok=1;
                         if(ok || !hit) 
                             ino = do_pixel_math(i,j,i,j,mathtext,0);
                         if(!g.draw_figure) break;     
                     }
                     if(!g.draw_figure) break;
                 }
                 check_event(); if(!g.draw_figure) break;
                 hit=1;
                 ox1 = x - g.sprayfac;
                 oy1 = y - g.sprayfac;
                 if(ino>=0) z[ino].hit=1;
                 break;
             case DIFFUSESPRAY:   
                 sf=g.sprayfac;
                 for(k=1;k<g.sprayfac*2;k++)
                 {  setpixelonimage(x+sprayrand(sf),y+sprayrand(sf),g.fcolor,g.imode,0,-1,0,0,1);
                    setpixelonimage(x+sprayrand(sf),y-sprayrand(sf),g.fcolor,g.imode,0,-1,0,0,1);
                    setpixelonimage(x-sprayrand(sf),y+sprayrand(sf),g.fcolor,g.imode,0,-1,0,0,1);
                    setpixelonimage(x-sprayrand(sf),y-sprayrand(sf),g.fcolor,g.imode,0,-1,0,0,1);
                 }
                 break; 
             case FINESPRAY:
                 for(i=x-g.sprayfac/2;i<=x+g.sprayfac/2;i++)
                 for(j=y-g.sprayfac/2;j<=y+g.sprayfac/2;j++)
                     setpixelonimage(i,j,g.fcolor,g.imode,0,-1,0,0,1); 
                 break;
             case FILTERSPRAY:
                 ino = whichimage(x,y,bpp);
                  ////  Copy the backup, then convert indexed images to 24 bpp before
                  ////  filtering. Otherwise it will be done automatically in 
                  ////  filter_region() after every mouse click.

                 if(ino>=0 && z[ino].colortype==INDEXED && tempalloc[ino]==0) 
                 {    if(z[ino].backedup)   
                      {    bytesperline = z[ino].xsize*g.off[z[ino].bpp];
                           tempbackup_1d[ino] = new uchar[z[ino].ysize*bytesperline];
                           tempbackup[ino] = make_2d_alias(tempbackup_1d[ino], 
                                             bytesperline, z[ino].ysize);    
                           if(tempbackup[ino]==NULL)
                                {  message(g.nomemory, ERROR); break; } 
                           else tempalloc[ino]=1;
                           for(k=0;k<z[ino].ysize;k++)
                                memcpy(tempbackup[ino][k],z[ino].backup[z[ino].cf][k], 
                                      bytesperline);                               
                      }
                      change_image_depth(ino,24,0); 
                      changed[ino]=1;
                 }
                 if((ox1!=x || oy1!=y) && ino>=0)
                 {   ff->x1 = x-g.sprayfac;
                     ff->y1 = y-g.sprayfac;
                     ff->x2 = x+g.sprayfac;
                     ff->y2 = y+g.sprayfac;
                     ff->entireimage = 0;
                     ff->want_progress_bar = 0;
                     ff->do_filtering = 1;
                     filter_region(ff); 
                 }
                 hit = 1;
                 ox1 = x;
                 oy1 = y;
                 if(ino>=0) z[ino].hit=1;
                 break;
             case ERASESPRAY:
                 ino = whichimage(x,y,bpp);
                 oino=ino;
                 done=0;
                 x1 = x-g.sprayfac/2;
                 x2 = x+g.sprayfac/2;
                 y1 = y-g.sprayfac/2;
                 y2 = y+g.sprayfac/2;
                 for(k=0; k<g.image_count; k++) repairflags[k]=0;
                 for(i=x1; i<=x2; i++)
                 {   for(j=y1; j<=y2; j++)
                     {   ino=erase_pixel(i,j,repairflags);
                         if(ino!=oino){ switchto(ino); oino=ino; }
                         done = repairflags[0] & 4;
                         if(done) break;
                     }if(done) break;
                 }
                 break;
             case LINESPRAY:
                 if(!hitgetline)
                 {   g.inmenu++;
                     if(!hitgetline) getline(x1,y1,x2,y2);
                     g.inmenu--;
                     g.getout = 0;
                     g.draw_figure = SPRAY;
                     hitgetline = 1;
                     if(x1==x2 && y1==y2 && oldx1!=0 && oldy1!=0)
                     {
                        x1 = oldx1; x2 = oldx2;
                        y1 = oldy1; y2 = oldy2;                     
                     }
                     dx = x2 - x1;
                     dy = y2 - y1;
                     length = cint(sqrt((y2-y1)*(y2-y1)+(x2-x1)*(x2-x1)));
                     button = 1;
                     while(button){ check_event(); button = g.mouse_button; }
                     while(!button){ check_event(); button = g.mouse_button; }
                     x = g.mouse_x;
                     y = g.mouse_y;
                     oldx1 = x1; oldx2 = x2;
                     oldy1 = y1; oldy2 = y2;
                     oldx = x; oldy = y;
                 }
                 if(oy2!=y || ox2!=x)
                 {   newangle = 90*RADPERDEG+atan2(oy2-y, x-ox2);
                     if(newangle-angle>=PI) newangle-=2*PI;
                     if(newangle-angle<=-PI) newangle+=2*PI;
                     angle = 0.99*angle + 0.01*newangle;
                     for(i=-length/2;i<length/2;i++)
                     {
                         di = ((double)i + (double)length/2.0) / (double)length;
                         c = readpixel(x1+cint((double)dx*di), 
                                       y1+cint((double)dy*di));

                         dlength = sqrt((x-oldx)*(x-oldx)+(y-oldy)*(y-oldy));
                         for(dj=.1; dj<=dlength; dj+=0.5)
                         {
                            frac = dj/dlength;
                            xx = oldx + cint(frac * (double)(x-oldx));
                            yy = oldy + cint(frac * (double)(y-oldy));
                            xe = xx + i*cos(angle);
                            ye = yy - i*sin(angle);
                            setpixelonimage(cint(xe+2),cint(ye+2),c,g.imode,0,-1,0,0,1); 
                            setpixelonimage(cint(xe-2),cint(ye-2),c,g.imode,0,-1,0,0,1); 
                            setpixelonimage(cint(xe+1),cint(ye+1),c,g.imode,0,-1,0,0,1); 
                            setpixelonimage(cint(xe-1),cint(ye-1),c,g.imode,0,-1,0,0,1); 
                            setpixelonimage(cint(xe),cint(ye),c,g.imode,0,-1,0,0,1); 
                            if(oldx==x) xx++;
                         }
                     } 
                     oldx = x;
                     oldy = y;
                 }
                 if(abs(ox1-x)>5 && (abs(oy1-y)>5)){ ox2 = ox1; ox1 = x; oy2 = oy1; oy1 = y; }
                 break;
             default: message("",BUG);
         }
     }else if(clicked)   // mouse unclicked, reset touched buffer or whatever else is needed
     {   hit=0;
         clicked = 0;
         for(k=0; k<g.image_count; k++) 
         {   if(z[k].hit)
             {     switchto(k);
                   rebuild_display(k); 
                   redraw(k); 
             }
         }
         if(alloc1) memset(touched_1d, 0, size*sizeof(char));
     }
  }
  g.draw_figure = 0;
  for(k=0; k<g.image_count; k++) 
  {   if(changed[k])     // Convert image back to 8 bits/pixel
      {  
          change_image_depth(k,8,0);
          if(tempalloc[k]==1)
          {    bytesperline = z[k].xsize * g.off[z[k].bpp];
               f = z[k].cf;
               if(z[k].backedup)
                  for(j=0;j<z[k].ysize;j++)
                    memcpy(z[k].backup[f][j], tempbackup[k][j], bytesperline);
               free(tempbackup[k]);
               delete[] tempbackup_1d[k];
               z[k].backedup = 1;
          }
          repair(k); 
          redraw(k);
      }    
      if(z[k].hit || changed[k]) repair(k); 
  }
  g.state = NORMAL;
  delete[] repairflags;
  if(alloc1){ free(touched); delete[] touched_1d; }
  inspray=0;
  g.waiting = max(0,g.waiting-1);
  return;
}


//--------------------------------------------------------------------------//
//  sprayfinish                                                             //
//--------------------------------------------------------------------------//
void sprayfinish(dialoginfo *a)
{
  a=a;
  g.draw_figure = 0;
  g.getout = 0;
}


//-------------------------------------------------------------------//
// sprayrand                                                         //
// return a random number for spray routine                          //
//-------------------------------------------------------------------//
int sprayrand(int sprayfac)
{  
   double ff;
   ff=(random_number()/32768.0)*(random_number()/32768.0)*(double)sprayfac;
   return (int)ff;
}



//-------------------------------------------------------------------------//
// get_math_formulas - mode can be NORMAL or SPRAY                         //
//-------------------------------------------------------------------------//
Widget get_math_formulas(char *text, int mode)
{
  const int helptopic=20;
  Widget w;
  int want_execute_button;
  if(memorylessthan(16384)){ message(g.nomemory); return 0; }
  if(mode==SPRAY)
  {   want_execute_button = 0;
      w = edit("Equation editor","Enter equations / Accept to start / Cancel to finish",
          text,256,80,0,0,256,helptopic,want_execute_button,NULL,null,NULL);
  }else
  {   want_execute_button = 1;
      w = edit("Equation editor","Enter equations (Shift-Enter for single line equation)",
          text,256,80,0,0,256,helptopic,want_execute_button,NULL,null,NULL);
  }
  return w;
}


//-------------------------------------------------------------------------//
// math - mathematical function on pixel r,g,b                             //
//-------------------------------------------------------------------------//
int math(char *text)
{
  if(g.getout){ g.getout=0; return ABORT; }
  printstatus(BUSY);
  inmath++;
  do_pixel_math(g.ulx, g.uly, g.lrx, g.lry, text, 1);
  inmath--;
  printstatus(NORMAL);
  return OK;
}


//-------------------------------------------------------------------------//
//  do_pixel_math - x1,y1,x2,y2 are screen coordinates                     //
//-------------------------------------------------------------------------//
int do_pixel_math(int x1, int y1, int x2, int y2, char *text, int oktoconvert)
{
#ifdef HAVE_LEX
  Widget www, scrollbar;
  if(memorylessthan(16384)){ message(g.nomemory); return -1; }
  if(!strlen(text))return ZEROLENGTH;
  volatile int ii=ci, xx, yy;
  volatile double real=0.0,imag=0.0;
  int i,ii2,j,k,rr,gg,bb,bpp,obpp=0,value,ovalue,datatype;
  double density,odensity;
  char tempstring[1024];
  for(k=0; k<g.image_count; k++) z[k].hit=0;

  g.getout = 0;
  add_all_variables(ci);
  int compensate = (int)read_variable((char*)"COMPENSATE", NULL, datatype);
  int invert     = (int)read_variable((char*)"INVERT", NULL, datatype);

  if(oktoconvert) progress_bar_open(www, scrollbar);
  for(j=y1; j<=y2; j++)
  {  
     if(keyhit()) if(getcharacter()==27)break;  
     if(oktoconvert) 
     {    sprintf(tempstring,"Math..%d        ",j);
          printstatus(MESSAGE, tempstring);
          if(y1!=y2 && oktoconvert) progress_bar_update(scrollbar, (j-y1)*100/(y2-y1));
     }
     check_event();
     if(g.getout) break; 
     for(i=x1; i<=x2; i++)
     {   
         if(!g.selected_is_square && !inside_irregular_region(i,j)) continue;
         value = readpixelonimage(i,j,bpp,ii2);
         if(!between(ii2, 0, g.image_count-1)){ g.getout=1; break; }
         ii = ii2;
         if(bpp!=obpp && ii>=0 && bpp==8)
         {     obpp=bpp; 
               switch_palette(ii);
         }   

         ////  If 8-bit indexed color, convert to 24 bpp before doing math

         if(oktoconvert)
         {    if(z[ii].hit==0 && z[ii].colortype==INDEXED) 
              {                       // z[].hit must be set before change_image_depth
                    z[ii].hit=2;      // 2 = image was converted to 24 bpp
                    if(change_image_depth(ii,24,0)!=OK) 
                    {   message("Insufficient image buffers available,\naborting...");
                        g.getout=1; 
                        break;
                    }
                    value = readpixelonimage(i,j,bpp,ii2);
                    ii = ii2;
              }
         }
         if(z[ii].hit==0) z[ii].hit=1;   // 1 = image was hit
         valuetoRGB(value,rr,gg,bb,bpp);

         ////  Definition of variables x,y,i,r,g, and b for expression evaluator.
         ////  Change the source coordinates if you don't like the
         ////  behavior.

         if(g.selectedimage>=0)                       // Entire image selected
         {   xx = i-z[ii].xpos;                       // x coordinate
             yy = j-z[ii].ypos;                       // y coordinate                              
         }else if(ii>=0)                              // Part of image selected
         {   xx = i-z[ii].xpos;                       // x coordinate
             yy = j-z[ii].ypos;                       // y coordinate
         }else                                        // Non-image selected
         {   xx = i;                                  // x coordinate
             yy = j;                                  // y coordinate
         }
         if(z[ii].floatexists)
         {   real = z[ii].fft[yy][xx].real();
             imag = z[ii].fft[yy][xx].imag();
         }
         density = value/g.maxvalue[z[ii].bpp];
         ovalue = value;
         odensity = density;
 
         add_variable((char*)"x",(double)xx);
         add_variable((char*)"y",(double)yy);
         add_variable((char*)"i",(double)value);
         add_variable((char*)"r",(double)rr);
         add_variable((char*)"g",(double)gg);
         add_variable((char*)"b",(double)bb);
         add_variable((char*)"v", density);
         add_variable((char*)"d",(double)pixeldensity(xx,yy,ii,compensate,invert));
         if(z[ii].floatexists)
         {  add_variable((char*)"re",real);
            add_variable((char*)"im",imag);
         }
         if(z[ii].waveletexists)
            add_variable((char*)"w",z[ii].wavelet[yy][xx]);
         
         ////  End of variable initialization
         if(g.getout) break;         
         if(strlen(text)) eval(text);
         if(g.getout) break;         
         ////  Back from evaluation
         if(status == GOTNEW) break; // Calculator sets this to stop looping
                                     // when it is actually a one-time command
         value = cint(read_variable((char*)"i", NULL, datatype));
         density = read_variable((char*)"v", NULL, datatype);
         rr = cint(read_variable((char*)"r", NULL, datatype));
         gg = cint(read_variable((char*)"g", NULL, datatype));
         bb = cint(read_variable((char*)"b", NULL, datatype));
         if(between(ii,0,g.image_count-1) && z[ii].floatexists)
         {   z[ii].fft[yy][xx].real() = read_variable((char*)"re", NULL, datatype);
             z[ii].fft[yy][xx].imag() = read_variable((char*)"im", NULL, datatype);
         }
         if(z[ii].waveletexists)
             z[ii].wavelet[yy][xx] = read_variable((char*)"w", NULL, datatype);

         if(value != ovalue)  
         {   value = max(0,min((int)g.maxvalue[bpp],value));
         }else if(density != odensity)
         {   value = cint(density * g.maxvalue[bpp]);
             value = max(0,min((int)g.maxvalue[bpp],value));
         }else        
         {   rr = max(0,min(g.maxred[bpp],rr));
             gg = max(0,min(g.maxgreen[bpp],gg));
             bb = max(0,min(g.maxblue[bpp],bb));
             value = RGBvalue(rr,gg,bb,bpp);
         }
         ii = setpixelonimage(i, j, value, g.imode, bpp, -1, 0);
         z[ii].touched = 1;
         if(g.getout)break;
     }
     if(g.getout || status!=OK)break;
  }      
  if(status==GOTNEW) status = OK;

  ////  Re-palettize any color images touched if in 8-bpp screen mode.
  ////  oktoconvert will be 0 if spraying (image bpp conversion will be 
  ////  done afterwards).

  if(z[ii].floatexists) scalefft(ii);

  if(oktoconvert)
  {  for(k=0; k<g.image_count; k++) 
     {   if(z[k].hit==2)
         {     z[k].hit=1;
               change_image_depth(k,8,0);
         }
     }
     for(k=0; k<g.image_count; k++) 
     {   if(z[k].hit)
         { 
               if(ci!=k) switchto(k);
               rebuild_display(k); 
               redraw(k); 
               z[k].hit = 0;
         }
     }
  }
  if(oktoconvert) progress_bar_close(www);
  check_event();
  return ii;
#else
  x1=x1; y1=y1; x2=x2; y2=y2; text=text; oktoconvert=oktoconvert;
  return NOTFOUND;
#endif
}


//-------------------------------------------------------------------------//
//  add_all_variables                                                      //
//-------------------------------------------------------------------------//
void add_all_variables(int ino)
{
  int k;
  char tempstring[128];
  add_variable((char*)"IMAGES",(double)g.image_count);
  add_variable((char*)"XRES",(double)g.xres);
  add_variable((char*)"YRES",(double)g.yres);
  add_variable((char*)"ULX",(double)g.ulx);
  add_variable((char*)"ULY",(double)g.uly);
  add_variable((char*)"LRX",(double)g.lrx);
  add_variable((char*)"LRY",(double)g.lry);
  add_variable((char*)"CI",(double)ino);
  // These two variables are set in get_densitometry_parameters().
  // add_variable("COMPENSATE",0);
  // add_variable("INVERT",0);
  if(ino>=0) 
  {   add_variable((char*)"BPP",(double)z[ino].bpp);
      add_variable((char*)"FRAMES",(double)z[ino].frames);
      add_variable((char*)"XSIZE",(double)z[ino].xsize);
      add_variable((char*)"YSIZE",(double)z[ino].ysize);
      add_variable((char*)"XPOS",(double)z[ino].xpos);
      add_variable((char*)"YPOS",(double)z[ino].ypos);
      add_variable((char*)"CF",(double)z[ino].cf);
      add_variable((char*)"CALLOG0",(double)z[ino].cal_log[0]);
      add_variable((char*)"CALLOG1",(double)z[ino].cal_log[1]);
      add_variable((char*)"CALDIMS",(double)z[ino].cal_dims);
      add_variable((char*)"FFTSTATE",z[ino].fftstate);
      add_variable((char*)"FMIN",z[ino].fmin);
      add_variable((char*)"FMAX",z[ino].fmax);
      add_variable((char*)"FFAC",z[ino].ffac);
      add_variable((char*)"WAVELETSTATE",z[ino].waveletstate);
      add_variable((char*)"WMIN",z[ino].wmin);
      add_variable((char*)"WMAX",z[ino].wmax);
      add_variable((char*)"WFAC",z[ino].wfac);
      for(k=0;k<10;k++)
      {   sprintf(tempstring, "Q0%d",k);
          add_variable(tempstring,(double)z[ino].cal_q[0][k]);
          sprintf(tempstring, "Q1%d",k);
          add_variable(tempstring,(double)z[ino].cal_q[1][k]);
          sprintf(tempstring, "Q2%d",k);
          add_variable(tempstring,(double)z[ino].cal_q[2][k]);
      }
      add_variable((char*)"SCALE",z[ino].cal_scale);
      
  }
}


