//--------------------------------------------------------------------------//
// xmtnimage68.cc                                                           //
// Latest revision: 03-06-2006                                              //
// Copyright (C) 2006 by Thomas J. Nelson                                   //
// freetype                                                                 //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern int         ci;                 // current image
extern Image      *z;
extern Globals     g;
extern int CHARX, CHARY;  
#ifdef HAVE_XFT
static XftColor* getxftcolor(Pixel pixel);
#endif
int freetypeface=1;
int freetypeweight=1;
int freetypeslant=1;
int freetypesize=3;
extern char label_text[1024];

//-------------------------------------------------------------------------//
// select_freetype_font - dialog for selecting freetype anti-aliased fonts //
//-------------------------------------------------------------------------//
void select_freetype_font(void)
{
   static Dialog *dialog;
   int helptopic = 75;
   dialog = new Dialog;
   if(dialog==NULL){ message(g.nomemory); return; }
   int j,k;
         
   //// Dialog box to get title & other options
   strcpy(dialog->title,"Label Parameters");
   strcpy(dialog->radio[0][0],"Face");
   strcpy(dialog->radio[0][1],"Times");             
   strcpy(dialog->radio[0][2],"Helvetica");
   strcpy(dialog->radio[0][3],"Charter");
   strcpy(dialog->radio[0][4],"Courier");
   strcpy(dialog->radio[0][5],"Symbol");

   strcpy(dialog->radio[1][0],"Weight");
   strcpy(dialog->radio[1][1],"Light");             
   strcpy(dialog->radio[1][2],"Medium");             
   strcpy(dialog->radio[1][3],"Demibold");             
   strcpy(dialog->radio[1][4],"Bold");             
   strcpy(dialog->radio[1][5],"Black");

   strcpy(dialog->radio[2][0],"Slant");
   strcpy(dialog->radio[2][1],"Roman");             
   strcpy(dialog->radio[2][2],"Italic");
   strcpy(dialog->radio[2][3],"Oblique");

   strcpy(dialog->radio[3][0],"Size");
   strcpy(dialog->radio[3][1],"12");             
   strcpy(dialog->radio[3][2],"14");             
   strcpy(dialog->radio[3][3],"16");             
   strcpy(dialog->radio[3][4],"18");             
   strcpy(dialog->radio[3][5],"20");             
   strcpy(dialog->radio[3][6],"22");             
   strcpy(dialog->radio[3][7],"24");             
   strcpy(dialog->radio[3][8],"28");             
   strcpy(dialog->radio[3][9],"32");             
   strcpy(dialog->radio[3][10],"36");             
   strcpy(dialog->radio[3][11],"44");             
   strcpy(dialog->radio[3][12],"60");             
   strcpy(dialog->radio[3][13],"100");             

   dialog->radioset[0] = freetypeface;
   dialog->radioset[1] = freetypeweight;
   dialog->radioset[2] = freetypeslant;
   dialog->radioset[3] = freetypesize;

   dialog->radiono[0] = 6;
   dialog->radiono[1] = 6;
   dialog->radiono[2] = 4;
   dialog->radiono[3] = 14;

   strcpy(dialog->boxes[0],"");
   strcpy(dialog->boxes[1],"Label");             
   dialog->boxtype[0]=LABEL;
   dialog->boxtype[1]=SCROLLEDSTRING;
   strcpy(dialog->answer[1][0], label_text);

   //// Use a custom format dialog box

   dialog->radioxy[0][0] = 6;   //x
   dialog->radioxy[0][1] = 63;  //y
   dialog->radioxy[0][2] = 166; //w
   dialog->radioxy[0][3] = 250; //h

   dialog->radioxy[1][0] = 6;   //x
   dialog->radioxy[1][1] = 180; //y
   dialog->radioxy[1][2] = 166; //w
   dialog->radioxy[1][3] = 250; //h

   dialog->radioxy[2][0] = 6;   //x
   dialog->radioxy[2][1] = 295; //y
   dialog->radioxy[2][2] = 266; //w
   dialog->radioxy[2][3] = 250; //h

   dialog->radioxy[3][0] = 6;   //x
   dialog->radioxy[3][1] = 370; //y
   dialog->radioxy[3][2] = 366; //w
   dialog->radioxy[3][3] = 250; //h

   dialog->boxxy[0][0] = 6;     //x
   dialog->boxxy[1][0] = 6;     //y

   dialog->boxxy[1][0] = 6;     //x
   dialog->boxxy[1][1] = 30;    //y
   dialog->boxxy[1][2] = 140;   //w

   dialog->spacing    = 0;
   dialog->radiostart = 6;
   dialog->radiowidth = 50;
   dialog->boxstart   = 6;
   dialog->boxwidth   = 50;
   dialog->labelstart = 6;
   dialog->labelwidth = 50;        

   for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
   dialog->noofradios = 4;
   dialog->noofboxes = 2;
   dialog->helptopic = helptopic;  
   dialog->want_changecicb = 0;
   dialog->f1 = freetypecheck;
   dialog->f2 = null;
   dialog->f3 = freetypecancel;
   dialog->f4 = freetypefinish;
   dialog->f5 = null;
   dialog->f6 = null;
   dialog->f7 = null;
   dialog->f8 = null;
   dialog->f9 = null;
   dialog->transient = 1;
   dialog->wantraise = 0;
   dialog->radiousexy = 1;
   dialog->boxusexy = 1;
   dialog->width = 170;    
   dialog->height = 670;   
   strcpy(dialog->path,".");
   g.getout=0;
   dialog->message[0]=0;      
   dialog->busy = 0;
   dialogbox(dialog);
}


//--------------------------------------------------------------------------//
//  freetypecancel                                                          //
//--------------------------------------------------------------------------//
void freetypecancel(dialoginfo *a)
{
   a=a;
   g.getout=1;
}


//--------------------------------------------------------------------------//
//  freetypefinish                                                          //
//--------------------------------------------------------------------------//
void freetypefinish(dialoginfo *a)
{
   a=a;
   g.getout=1;
}


//--------------------------------------------------------------------------//
//  freetypecheck                                                           //
//--------------------------------------------------------------------------//
void freetypecheck(dialoginfo *a, int radio, int box, int boxbutton)
{
   box = box; boxbutton=boxbutton;
   int x=0,y=0;
   if(radio == -2) 
   {
       freetypeface     = a->radioset[0];
       freetypeweight   = a->radioset[1];
       freetypeslant    = a->radioset[2];
       freetypesize     = a->radioset[3];
       strcpy(label_text, a->answer[1][0]);
       if(!strlen(label_text)) return;
       freetype(label_text, x, y, freetypeface, freetypeweight, freetypeslant, freetypesize);
   }
}


//-------------------------------------------------------------------------//
// freetype - test of freetype anti-aliased fonts                          //
//-------------------------------------------------------------------------//
void freetype(char *string, int xpos, int ypos, int face, int weight, int slant, int size)
{
   //  XftFontOpen  takes  a list of pattern elements of the form (field, type,
   //  value) terminated with a 0, matches that pattern against  the  available
   //  fonts and opens the matching font.

#ifdef HAVE_XFT
   int w,h, x, y, c, i, ino, j, xx=0, yy=0;
   int xft_size=12;
   int xft_weight=XFT_WEIGHT_MEDIUM;
   char xft_face[128]="";
   int xft_slant=XFT_SLANT_ROMAN;
   XftFont *xftfont;
   int len = min(1024, strlen(string));
   XGlyphInfo extents;
   XftColor *fcolor;
   XftDraw *xftdraw;

   switch(face)
   {   case 1: strcpy(xft_face, "times"); break;
       case 2: strcpy(xft_face, "helvetica"); break;
       case 3: strcpy(xft_face, "charter"); break;
       case 4: strcpy(xft_face, "courier"); break;
       case 5: strcpy(xft_face, "symbol"); break;
   }
   switch(weight)
   {   case 1:  xft_weight = XFT_WEIGHT_LIGHT; break;
       case 2:  xft_weight = XFT_WEIGHT_MEDIUM; break;
       case 3:  xft_weight = XFT_WEIGHT_DEMIBOLD; break;
       case 4:  xft_weight = XFT_WEIGHT_BOLD; break;
       case 5:  xft_weight = XFT_WEIGHT_BLACK; break;
   }   
   switch(slant)
   {   case 1:  xft_slant = XFT_SLANT_ROMAN; break;
       case 2:  xft_slant = XFT_SLANT_ITALIC; break;
       case 3:  xft_slant = XFT_SLANT_OBLIQUE; break;
   }
   switch(size)
   {   case 1:  xft_size = 12; break;
       case 2:  xft_size = 14; break;
       case 3:  xft_size = 16; break;
       case 4:  xft_size = 18; break;
       case 5:  xft_size = 20; break;
       case 6:  xft_size = 22; break;
       case 7:  xft_size = 24; break;
       case 8:  xft_size = 28; break;
       case 9:  xft_size = 32; break;
       case 10:  xft_size = 36; break;
       case 11:  xft_size = 44; break;
       case 12:  xft_size = 60; break;
       case 13:  xft_size = 100; break;
   }
   xftfont  =  XftFontOpen(g.display, g.screen, 
                           XFT_FAMILY, XftTypeString, xft_face, 
                           XFT_SLANT, XftTypeInteger, xft_slant, 
                           XFT_SIZE, XftTypeInteger, xft_size, 
                           XFT_WEIGHT, XftTypeInteger, xft_weight,
			   0, 0, 0);
     
   XftTextExtents8 (g.display, xftfont, (uchar*)string, len, &extents);
   x = extents.x;
   y = extents.y;
   w = min(CHARX, extents.width);
   h = min(CHARY, extents.height);
   
   if(xpos==0 && ypos==0) getboxposition(xpos, ypos, w, h);

   crosshairs(0,0);
   fcolor = getxftcolor(g.fcolor);
   XSetForeground(g.display, g.image_gc,  0);

   //// Put copy of the screen into the pixmap so anti-aliasing can
   //// adapt to its pixels.
   ino = g.imageatcursor;
   XCopyArea(g.display, z[ino].win, g.spixmap, g.image_gc, 
        xpos - z[ino].xpos, ypos - z[ino].ypos, 
        w, h, 0, 0);

   //// Create a XftDraw structure from pixmap for Xft to draw in
   xftdraw = XftDrawCreate(g.display, g.spixmap, g.visual, g.colormap);

   //// Draw the string on the Pixmap
   XSetForeground(g.display, g.image_gc, g.fcolor);
   XftDrawString8 (xftdraw, fcolor, xftfont, x, y, (uchar*)string, len);

   //// Copy pixmap into global XImage
   copyimage(0, 0, w, h, 0, 0, GET, g.stringbufr_ximage, g.spixmap);

   //// Read the pixel values from the XImage buffer and set pixels
   for(j=0;j<h;j++)
   for(i=0;i<w;i++)
   {     c = pixelat(g.stringbufr[j] + i*g.off[g.bitsperpixel], g.bitsperpixel);
         xx = i+xpos;  
         yy = j+ypos;
         setpixelonimage(xx, yy, c, g.imode, 0, -1, 0, 0, 1, 0, 1);
   }
   XftDrawDestroy(xftdraw);
#else
   string=string; xpos=xpos; ypos=ypos; face=face; weight=weight; slant=slant; size=size;
#endif
}


//--------------------------------------------------------------------//
//  getxftcolor                                                       //
//--------------------------------------------------------------------//
#ifdef HAVE_XFT
static XftColor* getxftcolor(Pixel pixel)
{
    static struct 
    {    XftColor color;
         int use;
    } cache[4];
    static int use;
    int i;
    int oldest, oldestuse;
    XColor color;

    oldestuse = 0x7fffffff;
    oldest = 0;
    for (i = 0; i < 4; i++) {
	if (cache[i].use) {
	    if (cache[i].color.pixel == pixel) {
		cache[i].use = ++use;
		return &cache[i].color;
	    }
	}
	if (cache[i].use < oldestuse) {
	    oldestuse = cache[i].use;
	    oldest = i;
	}
    }
    i = oldest;
    color.pixel = pixel;
    XQueryColor(g.display, g.colormap, &color);
    cache[i].color.color.red = color.red;
    cache[i].color.color.green = color.green;
    cache[i].color.color.blue = color.blue;
    cache[i].color.color.alpha = 0xffff;
    cache[i].color.pixel = pixel;
    cache[i].use = ++use;
    return &cache[i].color;
}
#endif


//--------------------------------------------------------------------//
// specialcharacter                                                   //
// read font image and mark it as fonts                               //
//--------------------------------------------------------------------//
void specialcharacter(void)
{ 
  int ino;
  char filename[FILENAMELENGTH];
  strcpy(filename, getfilename(filename, g.fontpath));
  g.window_border++;
  g.want_shell++;
  readimage(filename, 0, NULL);
  ino = ci;
  z[ino].is_font = 1;
  setimagetitle(ino, "Font menu");
  g.window_border--;
  g.want_shell--;
  backupimage(ino, 0); 
}


//--------------------------------------------------------------------//
//  get_special_character                                             //
//--------------------------------------------------------------------//
void get_special_character(void)
{ 
  int ok,i=0,j=0,c,w,h,ino_label,x,y,ino,x0,y0,x1,x2,y1,y2,totalpixels;
  int hitbkg;
  double totalsignal;
  uchar *add;
  RGB rgb_min, rgb_max;

  ino = ci;
  if(!z[ino].is_font){message("Not a font image"); return;}
  x = g.get_x1 - z[ino].xpos;
  y = g.get_y1 - z[ino].ypos;

  //// find boundaries of character in image
  //// assume 8 bit gray scale, bkg = 255
  x0 = z[ino].xpos;
  y0 = z[ino].ypos;
  x1 = x2 = x;
  y1 = y2 = y;
  
  //// find starting pixel
  ok=0;
  for(j=y; j; j--)
  {   for(i=x; i; i--)
         if(z[ino].image[0][j][i] < 255){ ok=1; break;}
      if(ok) break;
  }
  if(!ok) return;
  x = i;
  y = j;

  //// find boundaries
  fill(x + x0, y + y0, x1, y1, x2, y2, 0.5, 1.0, 1.0, totalpixels, totalsignal);
  restoreimage(ino);

  h = y2 - y1 + 1;
  w = x2 - x1 + 1;

  g.imageatcursor = ino;

  if(newimage("Image",0,0,w,h,g.bitsperpixel,g.colortype,1,0,0,TEMP,1,0,0)!=OK) return;
  ino_label = ci;
 
  rgb_min.red = 250;
  rgb_min.green = 250;
  rgb_min.blue = 250;
  rgb_max.red = 255;
  rgb_max.green = 255;
  rgb_max.blue = 255;

  ////  Put part of ino in ino_label.
  ////  Record its original bpp. 
  ////  This causes pixel depths to be converted only if their bpp is different 
  ////  from the destination.
  array<uchar> savebpp(w+1, h+1); 
  for(j=0; j<h; j++) 
  for(i=0; i<w; i++) 
  {    savebpp.p[j][i] = g.bitsperpixel;    
       c = z[ino].image[0][y1+j-y0][x1+i-x0];
       c = convertpixel(c, z[ino].bpp, g.bitsperpixel, 1);
       add = z[ino_label].image[0][j]+i*g.off[g.bitsperpixel];
       if(!get_alpha_bit(ino, 0, x1+i-x0, y1+j-y0)) c=RGBvalue(255,255,255,g.bitsperpixel);
       putpixelbytes(add,c,1,g.bitsperpixel,1);
  }
  box(x1,y1,x2,y2,0,SET);
  switchto(ino_label);
  repairimg(ino_label, 0, 0, w, h);
  repair(ino_label);
  redraw(ino_label);
  check_event();

  //// grab character and paste it somewhere
  switchto(ino);
  g.imageatcursor = ino;
  user_position_copy(ino_label, 0, 0, w, h, 0, ANTI_ALIAS_INVERT,
       1, 0, g.maxcolor, rgb_min, rgb_max, g.bitsperpixel, savebpp.p, 
       NULL, hitbkg, 0);

  switchto(ino);
  restoreimage(ino);

  //// reset alpha bits
  for(j=0; j<z[ino].ysize; j++) 
  for(i=0; i<z[ino].xsize; i++) 
       set_alpha_bit(ino, 0, i, j, 0);

  z[ino].touched = 0;
  z[ino_label].touched = 0;
}









s                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        