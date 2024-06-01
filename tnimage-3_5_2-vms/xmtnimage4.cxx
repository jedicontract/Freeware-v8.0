//--------------------------------------------------------------------------//
// xmtnimage4.cc                                                            //
// Color palette-related routines                                           //
// Latest revision: 03-26-2003                                              //
// Copyright (C) 2003 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"
#define COLORS_EQUAL 0   // Set this to 1 to keep color balance constant in
                         // histogram equalization

extern Globals     g;
extern Image      *z;
extern int         ci;
RGB otherpalette[256];
int inpalette=0;
int incolorpalette=0;
static inline int get2graypixels(int i, int j, int ino, int bpp);
static inline void get2colorpixels(int i, int j, int ino, int bpp, int &rr,
   int &gg, int &bb);
int in_unmanagecb = 0;
int in_samplepalettepseudocb = 0;

//--------------------------------------------------------------------------//
// get2graypixels                                                           //
//--------------------------------------------------------------------------//
static inline int get2graypixels(int i, int j, int ino, int bpp)
{
  int val, value1, value2;
  int j2;
  int b = g.off[bpp];
  int i2 = i * b;
  int f = z[ino].cf;
  value1 = (int)pixelat(z[ino].image[f][j]+i2, bpp);
  if(bpp>8) return value1;
  j2 = j-1; if(j2<0) j2 = 2;
  value2 = pixelat(z[ino].image[f][j2]+i2, bpp);
  val = value1*256 + value2;
  return val;
}

//--------------------------------------------------------------------------//
// get2colorpixels                                                          //
//--------------------------------------------------------------------------//
static inline void get2colorpixels(int i, int j, int ino, int bpp, int &rr,
   int &gg, int &bb)
{
  int value1, value2,r1,r2,g1,g2,b1,b2;
  int j2;
  int b = g.off[bpp];
  int i2 = i * b;
  int f = z[ino].cf;
  value1 = (int)pixelat(z[ino].image[f][j]+i2, bpp);
  valuetoRGB(value1, r1, g1, b1, bpp);
  j2 = j-1; if(j2<0) j2 = 2;
  value2 = pixelat(z[ino].image[f][j2]+i2, bpp);
  valuetoRGB(value2, r2, g2, b2, bpp);
  rr = r1*256+r2;
  gg = g1*256+g2;
  bb = b1*256+b2;
}


//--------------------------------------------------------------------------//
// changepalette                                                            //
//--------------------------------------------------------------------------//
void changepalette(int noofargs, char **arg)
{   
   static listinfo *l;
   static int option = 0;
   static char **info; 
   static char *listtitle;
   if(memorylessthan(16384)){ message(g.nomemory,ERROR); return; }
   g.getout=0;

   int helptopic=5;
   if(noofargs==0)
   { 
     listtitle = new char[100];
     info = new char*[12];
     strcpy(listtitle, "Colormap options");
     info[0] = new char[100]; strcpy(info[0],"Select colormap..."); 
     info[1] = new char[100]; strcpy(info[1],"Grayscale intensity mapping"); 
     info[2] = new char[100]; strcpy(info[2],"Read colormap from disk"); 
     info[3] = new char[100]; strcpy(info[3],"Save colormap to disk"); 
     info[4] = new char[100]; strcpy(info[4],"Create colormap"); 
     info[5] = new char[100]; strcpy(info[5],"Invert colormap"); 
     info[6] = new char[100]; strcpy(info[6],"Rotate colormap"); 
     info[7] = new char[100]; strcpy(info[7],"Restore original colormap"); 
     info[8] = new char[100]; strcpy(info[8],"Sort colormap"); 
     info[9] = new char[100]; strcpy(info[9],"Remap to other colormap"); 
     info[10]= new char[100]; strcpy(info[10],"Select false color map for grayscale"); 
     info[11]= new char[100]; strcpy(info[11],"Reset grayscale to default"); 
     
     l = new listinfo;
     l->title = listtitle;
     l->info  = info;
     l->count = 12;
     l->itemstoshow = 12;
     l->firstitem   = 0;
     l->wantsort    = 0;
     l->wantsave    = 0;
     l->helptopic   = helptopic;
     l->selection   = &option;
     l->allowedit   = 0;
     l->width       = 0;
     l->transient   = 1;    
     l->wantfunction = 0;
     l->autoupdate   = 0;
     l->clearbutton  = 0;
     l->highest_erased = 0;
     l->f1 = changepalettefinish;
     l->f2 = null;
     l->f3 = null;
     l->f4 = delete_list;
     l->f5 = null;
     l->f6 = null;
     l->listnumber = 0;
     list(l);
   }else 
   {  setSVGApalette(atoi(arg[1]));
      apply_newpalette();
   }   
}

//--------------------------------------------------------------------------//
//  changepalettefinish                                                     //
//--------------------------------------------------------------------------//
void changepalettefinish(listinfo *l)
{
  int crud=0,k,option=0,newpalette=0;
  static int l2k;
  static listinfo *l2;
  static char **pinfo; 
  static char *listtitle;
  int helptopic = 5;
  option = *l->selection - l->firstitem;
  switch(option)
  {  case 0:
     case 10:     
          l2 = new listinfo;
          listtitle = new char[100];
          pinfo = new char*[9];
          strcpy(listtitle, "Select colormap");
          pinfo[0] = new char[100]; strcpy(pinfo[0],"Gray scale/false color"); 
          pinfo[1] = new char[100]; strcpy(pinfo[1],"Spectrum"); 
          pinfo[2] = new char[100]; strcpy(pinfo[2],"Multi-color 1"); 
          pinfo[3] = new char[100]; strcpy(pinfo[3],"Multi-color 2"); 
          pinfo[4] = new char[100]; strcpy(pinfo[4],"Multi-color 3"); 
          pinfo[5] = new char[100]; strcpy(pinfo[5],"RGBI"); 
          pinfo[6] = new char[100]; strcpy(pinfo[6],"Black to green"); 
          pinfo[7] = new char[100]; strcpy(pinfo[7],"Zebra"); 
          pinfo[8] = new char[100]; strcpy(pinfo[8],"Other..."); 

          l2->title = listtitle;
          l2->count = 9;
          l2->info  = pinfo;
          l2->itemstoshow = 9;
          l2->firstitem   = 0;
          l2->wantsort    = 0;
          l2->wantsave    = 0;
          l2->helptopic   = helptopic;
          l2->selection   = &l2k;
          l2->allowedit   = 0;
          l2->width       = 0;
          l2->transient   = 1;
          l2->wantfunction = 0;
          l2->autoupdate   = 0;
          l2->clearbutton  = 0;
          l2->highest_erased = 0;
          l2->f1 = paletteselectfinish;
          l2->f2 = null;
          l2->f3 = null;
          l2->f4 = delete_list;
          l2->f5 = null;
          l2->f6 = null;
          l2->listnumber = 0;
          list(l2);
          break;
     case 1: 
          if(z[ci].colortype!=GRAY)
             message("Image must be converted \nto grayscale first");
          else
             grayscalemap(); 
          break;     
          
     case 2: readpalette(); newpalette=1; break;
     case 3: savepalette(); break;
     case 4: createpalette(); newpalette=1; break;
     case 5: invertpalette(); newpalette=1; break;
     case 6: clickbox("Rotate colormap", 8, &crud, -1, 2, rotatepalette,
                  cset, null, NULL, NULL, 28);
             break;
     case 7: setSVGApalette(-3); break;
     case 8: sortpalette(ci); repair(ci); break;
     case 9: remap_image_colormap(ci); break;
     case 11: setSVGApalette(-5); 
              g.false_color = 0;
              for(k=0;k<g.image_count;k++) if(z[k].colortype==GRAY) repair(k);
              break;
     default: fprintf(stderr,"Error at 4.%d option=%d\n",__LINE__,option);
  }
  if(g.getout){  g.getout=0; newpalette=0; }
  if(newpalette) apply_newpalette();
}  


//--------------------------------------------------------------------------//
//  paletteselectfinish                                                     //
//--------------------------------------------------------------------------//
void paletteselectfinish(listinfo *l)
{
  int j,k,newpalette=0;
  k = *l->selection - l->firstitem;
  if(!g.getout)
  {   if(k==8)
          setSVGApalette(-1); 
      else if(k==10)
      {   g.false_color = 1;
          newpalette=0;
          memcpy(g.fc_palette, g.palette, 768); 
          for(j=0;j<g.image_count;j++) if(z[j].colortype==GRAY) repair(j);
      }else
      {   g.currentpalette=k+1;
          setSVGApalette(k+1);                   
          apply_newpalette();
      }
  }
}


//--------------------------------------------------------------------------//
//  apply_newpalette                                                        //
//--------------------------------------------------------------------------//
void apply_newpalette(void)
{  
   if(g.selectedimage>=0)
   {   if(z[ci].colortype!=INDEXED && z[ci].colortype!=GRAPH) 
            message("Not an indexed-color image\nretaining old colormap",WARNING);
       memcpy(z[ci].palette,g.palette,768);
       memcpy(z[ci].opalette,g.palette,768);
       rebuild_display(ci);
       z[ci].touched=1;
   }else
   {   memcpy(g.b_palette, g.palette, 768);
       setpalette(g.b_palette);
   }
   redrawscreen();
}


//--------------------------------------------------------------------------//
// setSVGApalette - sets 256 color registers to:                            //
//   paletteno =                                                            //
//              -5 - Change false color palette to grayscale                //
//              -4 - Original background palette                            //
//              -3 - Original palette belonging to ci                       //
//              -2 - Palette belonging to ci                                //
//              -1 - Other (randomly-generated)                             //
//               0 - Other (e.g., inverse)                                  //
//               1 - false color (usually grayscale)                        //
//               2 - spectrum                                               //
//               3 - multi colors 1                                         //
//               4 - multi colors 2                                         //
//               5 - multi colors 3                                         //
//               6 - RGBI                                                   //
//               7 - black to green                                         //
//               8 - zebra                                                  //
// Set 'wantset'=0 to reinitialize the palette without actually changing it.//
//--------------------------------------------------------------------------//
void setSVGApalette(int paletteno, int wantset)
{
  static char temp[40] = "1";
  static int seed=1000;
  int i;

  switch(paletteno)
  {
    case -5:               // Change false color palette to grayscale
        memcpy(g.fc_palette, g.gray_palette,768);
        break;
    case -4:
        memcpy(g.palette,g.b_palette,768);
        break;
    case -3:
        if(ci>=0)
        {    memcpy(z[ci].palette,z[ci].opalette,768);
             memcpy(g.palette,z[ci].opalette,768);
        }  
        break;
    case -2:
        if(ci>=0) 
        {    memcpy(z[ci].palette,z[ci].opalette,768);
             memcpy(g.palette,z[ci].opalette,768);
        }
        break;
    case -1:
        strcpy(temp,"Enter colormap number (0-10000)");
        clickbox(temp, 9, &seed, 0, 10000, randompalettecb, cset, null, 
                 NULL, NULL, 0);
        break;
    case 0:                // Other - read from disk, inverted, etc.
        memcpy(g.palette,otherpalette,768);
        break;
    case 1:                // False-color palette (usually grayscale)
        memcpy(g.palette, g.fc_palette,768);
        break; 
    case 2:                 // spectrum
        for (i=0; i<64; i++)
        {
           g.palette[i].blue = 63-i;
           g.palette[i].green = i;
           g.palette[i].red = 0;
           
           g.palette[i+64].blue = 0;
           g.palette[i+64].green = 63; 
           g.palette[i+64].red = i;

           g.palette[i+128].blue = 0;
           g.palette[i+128].green = 63-i;
           g.palette[i+128].red =63;
           
           g.palette[i+192].blue = i;
           g.palette[i+192].green = i;
           g.palette[i+192].red = 63;
                
        }
        g.palette[0].red = 0;
        g.palette[0].green = 0;
        g.palette[0].blue = 0; 
        break;          
    case 3:                 // multi colors
        for(i=0;i<=255;i++)
        {   
            if((i%128)<=63)
                 g.palette[i].red   = i%64;
            else
                 g.palette[i].red   = 63 - i%64;
            g.palette[i].green  = i/4;
            if((i%32)>15) 
                 g.palette[i].blue  = 63-4*(i%16);
            else
                 g.palette[i].blue  = 4*(i%16);
        }

        g.palette[0].red = 0;
        g.palette[0].green = 0;
        g.palette[0].blue = 0;
        break;  
    case 4:                 // multi colors 2
        for(i=0;i<=255;i++)
        {   
            g.palette[i].green = i%64;
            g.palette[i].red   = i/4;
            g.palette[i].blue  = 4*(i%16);
        }
        g.palette[0].red = 0;
        g.palette[0].green = 0;
        g.palette[0].blue = 0;
        break;  
    case 5:                 // multi colors 3
        for(i=0;i<254;i++)
        {   
            g.palette[i+1].blue  = i%64;
            g.palette[i+1].red   = i/4;
            g.palette[i+1].green = 63-4*(i%16);
        }

        g.palette[0].red = 0;
        g.palette[0].green = 0;
        g.palette[0].blue = 0;
        break;  
    case 6:                 // RGBI
        for (i=0; i<64; i++)
        {   g.palette[i].red       = i;
            g.palette[i].green     = 0;
            g.palette[i].blue      = 0;
           
            g.palette[i+64].red    = 0;
            g.palette[i+64].green  = i;
            g.palette[i+64].blue   = 0;
           
            g.palette[i+128].red   = 0;
            g.palette[i+128].green = 0;
            g.palette[i+128].blue  = i;
           
            g.palette[i+192].red   = i;
            g.palette[i+192].green = i;
            g.palette[i+192].blue  = i;
        }  
        break;         
    case 7:                //black to green
        for (i=0; i<64; i++)
        {   g.palette[i*4].red     = 0;  
            g.palette[i*4].green   = i;
            g.palette[i*4].blue    = 0;

            g.palette[i*4+1].red   = 1;
            g.palette[i*4+1].green = i;
            g.palette[i*4+1].blue  = 1;

            g.palette[i*4+2].red   = 2;
            g.palette[i*4+2].green = i;
            g.palette[i*4+2].blue  = 2;

            g.palette[i*4+3].red   = 3;
            g.palette[i*4+3].green = i;
            g.palette[i*4+3].blue  = 3;           
        }
        g.palette[0].red = 0;
        g.palette[0].green = 0;
        g.palette[0].blue = 0;
        g.palette[255].red = 63;
        g.palette[255].green = 63;
        g.palette[255].blue = 63;
        break; 
    case 8:
        for (i=0; i<254; i+=2)
        {  g.palette[i].blue = 0;
           g.palette[i].green = 0;
           g.palette[i].red = 0;
           g.palette[i+1].blue = 63;
           g.palette[i+1].green = 63;
           g.palette[i+1].red = 63;
        }
        break;
    default:
        randompalette(paletteno);      
        break;
  }

  if(wantset)
  {  g.currentpalette=paletteno; 
     setpalette(g.palette); 
  }
}


//--------------------------------------------------------------------------//
// grayscalemap  - adjust monochrome images using multiclickbox()           //
// This should only be called if ci is a grayscale image.                   //
//--------------------------------------------------------------------------//
int grayscalemap(void)
{ 
  if(memorylessthan(4096)){  message(g.nomemory,ERROR); return(NOMEM); } 

  int start0,start1,start2,start3;
  if(ci==-1){ message("No images",ERROR); return(NOIMAGES);}
  if(z[ci].colortype!=GRAY)
  {     message("Image is color\nNo grayscale map present");
        return(BADPARAMETERS);
  }
  static clickboxinfo* item;
  int helptopic = 27;
  int maximum,minimum;
  char temp[128];

  if(z[ci].floatexists)
  {   maximum = (int)(2 * max(z[ci].fmax, z[ci].fmin));
      minimum = (int)(2 * min(z[ci].fmax, z[ci].fmin));
      maximum=max((uint)maximum,(uint)g.maxvalue[z[ci].bpp]);
      if(minimum>0) minimum=0;
  }else
  {   maximum=(uint)g.maxvalue[z[ci].bpp];
      minimum = 0;
  }
  
  if(z[ci].gray[0]+z[ci].gray[1]>0)
  {  start0 = z[ci].gray[0];
     start1 = z[ci].gray[1];
     start2 = z[ci].gray[2];
     start3 = z[ci].gray[3];
  }else
  {  start0 = maximum; 
     start1 = 0; 
     start2 = g.maxgray[g.bitsperpixel];
     start3 = 0;
  }

  item = new clickboxinfo[4];
  item[0].wantpreview=1;
  item[0].title = new char[128];
  strcpy(item[0].title,"Image white");
  item[0].startval = start0;
  item[0].minval = minimum;
  item[0].maxval = maximum;
  item[0].type = VALSLIDER;
  item[0].answers = new int[10];  // Must be 10 for multiclickbox cb
  item[0].wantdragcb = 0;
  item[0].form = NULL;
  item[0].path = NULL;

  item[1].title = new char[128];
  strcpy(item[1].title,"Image black");
  item[1].startval = start1;
  item[1].minval = minimum;
  item[1].maxval = maximum;
  item[1].type = VALSLIDER;
  item[1].wantdragcb = 0;
  item[1].answers = item[0].answers;
  item[1].form = NULL;
  item[1].path = NULL;

  item[2].title = new char[128];
  strcpy(item[2].title,"Screen white");
  item[2].startval = start2;
  item[2].minval = min(0, g.maxgray[g.bitsperpixel]);
  item[2].maxval = max(0, g.maxgray[g.bitsperpixel]);
  item[2].type = VALSLIDER; 
  item[2].wantdragcb = 0;
  item[2].answers = item[0].answers;
  item[2].form = NULL;
  item[2].path = NULL;

  item[3].title = new char[128];
  strcpy(item[3].title,"Screen black");
  item[3].startval = start3;
  item[3].minval = min(0, g.maxgray[g.bitsperpixel]);
  item[3].maxval = max(0, g.maxgray[g.bitsperpixel]);
  item[3].type = VALSLIDER; 
  item[3].wantdragcb = 0;
  item[3].answers = item[0].answers;
  item[3].form = NULL;
  item[3].path = NULL;
  strcpy(temp,"Brightness/contrast (grayscale)");
  g.getout=0;

  z[ci].hitgray=1;
  item[0].ino = ci;
  item[0].noofbuttons = 4;
  item[0].f1 = null;
  item[0].f2 = null;
  item[0].f3 = null;
  item[0].f4 = null;
  item[0].f5 = grayscalemapok;
  item[0].f6 = grayscalemapfinish;
  item[0].f7 = null;
  item[0].f8 = null;
  multiclickbox(temp,4,item,brightnessadjust,helptopic);
  return OK; 
}
 

//-------------------------------------------------------------------------//
// grayscalemapok                                                          //
//-------------------------------------------------------------------------//
void grayscalemapok(clickboxinfo *c)
{
  int ino = c[0].ino;
  z[ino].gray[0] = c[0].answer;
  z[ino].gray[1] = c[1].answer;
  z[ino].gray[2] = c[2].answer;
  z[ino].gray[3] = c[3].answer;
  z[ino].fmax = z[ino].gray[0];
  z[ino].fmin = z[ino].gray[1];
  rebuild_display(ino);
  switchto(ino);
  redraw(ino);
  z[ino].adjustgray = 0;
}


//-------------------------------------------------------------------------//
// grayscalemapfinish                                                      //
//-------------------------------------------------------------------------//
void grayscalemapfinish(clickboxinfo *c)
{
  int k;
  delete[] c[0].answers;
  for(k=0; k<4; k++) delete[] c[k].title;
  delete[] c;                                   
}


//--------------------------------------------------------------------------//
// brightnessadjust - adjust monochrome images using multiclickbox()        //
// This should only be called if ci is a grayscale image.                   //
// This is only called by multiclickbox().                                  //
//                                                                          //
// Inputs:      a[0] = image white level - 0 to maxvalue of image bpp       //
//  (from       a[1] = image black level - 0 to maxvalue of image bpp       //
//   multi-     a[2] = screen white level - 0 to maxvalue of screen bpp     //
//   clickbox)  a[3] = screen black level - 0 to maxvalue of screen bpp     //
//                                                                          //
// These are kept as raw pixel levels to make it easy for other routines    //
//       to estimate them.                                                  //
// Use:  Used by convert to map image pixel values to screen values,        //
//       using the formula: val = ((pixel-blackin)*fac)>>8 + blackout       //
//                                                                          //
//--------------------------------------------------------------------------//
void brightnessadjust(int a[10])
{
    if(memorylessthan(4096)){  message(g.nomemory,ERROR); return; } 
    if(ci<0) return;
    int oadjustgray=0;
    int whitein=a[0],blackin=a[1];
    int whiteout=a[2],blackout=a[3];
    
    z[ci].gray[0] = whitein;
    z[ci].gray[1] = blackin;
    z[ci].gray[2] = whiteout;
    z[ci].gray[3] = blackout;
    if(z[ci].floatexists)
    {  z[ci].fmax=z[ci].gray[0];
       z[ci].fmin=z[ci].gray[1];
    }

    oadjustgray = z[ci].adjustgray;      
    z[ci].adjustgray = 1;  // Allow setting of gray mapping
    if(g.selectedimage>-1)
    { 
       rebuild_display(ci);
       switchto(ci);
       redraw(ci);
    }   
    z[ci].adjustgray = oadjustgray; 
}



//--------------------------------------------------------------------------//
// rotatepalette                                                            //
//--------------------------------------------------------------------------//
void rotatepalette(int k)
{ 
    int j,red,blue,green;
    int s=1;
    if(k<=0)
    {    red  = g.palette[s].red;
         green= g.palette[s].green;
         blue = g.palette[s].blue;
         for(j=s;j<254;j++)
         {   g.palette[j].red  = g.palette[j+1].red;
             g.palette[j].green= g.palette[j+1].green;
             g.palette[j].blue = g.palette[j+1].blue;
         }
         g.palette[254].red  = red;
         g.palette[254].green= green;
         g.palette[254].blue = blue;
    } 
    if(k>0)       
    {    red  = g.palette[254].red;
         green= g.palette[254].green;
         blue = g.palette[254].blue;
         for(j=254;j>s;j--)
         {   g.palette[j].red  = g.palette[j-1].red;
             g.palette[j].green= g.palette[j-1].green;
             g.palette[j].blue = g.palette[j-1].blue;
         }
         g.palette[s].red  = red;
         g.palette[s].green= green;
         g.palette[s].blue = blue;
    }    
    g.palette[255].red=63;
    g.palette[255].green=63;
    g.palette[255].blue=63;
    setpalette(g.palette);
}


//--------------------------------------------------------------------------//
// sortpalette                                                              //
//--------------------------------------------------------------------------//
void sortpalette(int ino)
{
    if(ino<0){ message("No images",ERROR); return; }
    int f,rr,gg,bb,i,j,k,s;
    RGB temppal[256];
    double intensity, item[256];
    int sorted[256], index[256], remap[256];
    drawselectbox(OFF);
    for(k=0;k<256;k++)
    {    rr = z[ino].palette[k].red;
         gg = z[ino].palette[k].green;
         bb = z[ino].palette[k].blue;
         intensity = rr*g.luminr + gg*g.luming + bb*g.luminb;
         index[k] = k;   
         item[k] = intensity;
    }
    memcpy(temppal, z[ino].palette, 768);
    sort(item, sorted, 256);
    for(k=0;k<256;k++) remap[k]=k;
    for(k=0;k<256;k++)
    {    i = index[k];
         s = sorted[k];
         temppal[i].red = g.palette[s].red;
         temppal[i].green = g.palette[s].green;
         temppal[i].blue = g.palette[s].blue;
         remap[s]=i;
    }
    memcpy(z[ino].palette, temppal, 768);
    setpalette(z[ino].palette);

    if(z[ino].bpp == 8)
    {    for(f=0;f<z[ino].frames;f++)
         for(j=0;j<z[ino].ysize;j++)
         for(i=0;i<z[ino].xsize;i++)
              z[ino].image[f][j][i] = remap[z[ino].image[f][j][i]]; 
    }
}



//--------------------------------------------------------------------------//
// randompalettecb                                                          //
//--------------------------------------------------------------------------//
void randompalettecb(int seed)
{  
     randompalette(seed);
     setpalette(g.palette);
}

//--------------------------------------------------------------------------//
// randompalette                                                            //
//--------------------------------------------------------------------------//
void randompalette(int seed)
{
    srand(seed);
    int i;
    g.palette[0].red = 0;
    g.palette[0].green = 0;
    g.palette[0].blue = 0;  
    int direction1,direction2,direction3, fac1,fac2,fac3;

    if(seed<200)
    {
       direction1 = 1;
       fac1 = 1+random_number()/16384+ranfunc()+ranfunc()+ranfunc();
       for(i=1;i<255;i++)
       {    
         g.palette[i].blue  = g.palette[i-1].blue  + fac1*direction1;
         g.palette[i].green = g.palette[i-1].green + fac1*direction1;
         g.palette[i].red   = g.palette[i-1].red   + fac1*direction1;
                  // If at top change direction to 0 or -1
         if(g.palette[i].blue >=62-fac1) direction1=-ranfunc();
                  // If at bottom change direction to 0 or 1
         if(g.palette[i].blue < fac1)    direction1=ranfunc();
       }
    }
    else if(seed<1000)
    {
       direction1 = 1;
       direction2 = 1;
       direction3 = 1;
       if(seed<1000){fac1=1;fac2=1;fac3=0; }
       if(seed<900){ fac1=0;fac2=1;fac3=1; }
       if(seed<800){ fac1=1;fac2=0;fac3=1; }
       if(seed<700){ fac1=2;fac2=1;fac3=1; }
       if(seed<600){ fac1=1;fac2=2;fac3=2; }
       if(seed<500){ fac1=1;fac2=2;fac3=1; }
       if(seed<400){ fac1=1;fac2=1;fac3=2; }
       if(seed<300){ fac1=1;fac2=1;fac3=1; }
     
       for(i=1;i<255;i++)
       {    
         g.palette[i].blue  = g.palette[i-1].blue  + fac1*direction1;
         g.palette[i].green = g.palette[i-1].green + fac2*direction2;
         g.palette[i].red   = g.palette[i-1].red   + fac3*direction3;
                  // If at top change direction to 0 or -1
         if(g.palette[i].blue >=62-fac1) direction1=-ranfunc();
         if(g.palette[i].green>=62-fac2) direction2=-ranfunc();
         if(g.palette[i].red  >=62-fac3) direction3=-ranfunc();
                  // If at bottom change direction to 0 or 1
         if(g.palette[i].blue < fac1)    direction1=ranfunc();
         if(g.palette[i].green< fac2)    direction2=ranfunc();
         if(g.palette[i].red  < fac3)    direction3=ranfunc();
       }
    }        
    else
    {
       direction1 = 1;
       direction2 = 1;
       direction3 = 1;
       fac1 =  random_number()/8192;
       fac2 =  random_number()/8192; 
       fac3 =  random_number()/8192;
       for(i=1;i<255;i++)
       {    
         g.palette[i].blue  = g.palette[i-1].blue  + fac1*direction1;
         g.palette[i].green = g.palette[i-1].green + fac2*direction2;
         g.palette[i].red   = g.palette[i-1].red   + fac3*direction3;
         if((g.palette[i].blue >=62-fac1)||(g.palette[i].blue <fac1))direction1*=-1;
         if((g.palette[i].green>=62-fac2)||(g.palette[i].green<fac2))direction2*=-1;
         if((g.palette[i].red  >=62-fac3)||(g.palette[i].red  <fac3))direction3*=-1;
       }
    }

    g.palette[0].red = 0;
    g.palette[0].green = 0;
    g.palette[0].blue = 0;
    g.palette[255].red = 63;
    g.palette[255].green = 63;
    g.palette[255].blue = 63;
}


//--------------------------------------------------------------------------//
//  ranfunc                                                                 //
//--------------------------------------------------------------------------//
int ranfunc(void)   // Returns mostly 0's and some 1's for random palette
{  int a;
   a = random_number();
   if(a<31000) return(0); else return(1);
}


//--------------------------------------------------------------------------//
//  random_number  - return a random number between 0 and a defined maximum.//
//  Use this instead of rand() when porting DOS programs.                   //
//--------------------------------------------------------------------------//
int random_number(void)   
{  
#ifdef DIGITAL
   #define MAXNUMBER 32768
#else
   const int MAXNUMBER=32768;
#endif
   const double f = (double)MAXNUMBER / (double)RAND_MAX;
   int a;
   a = rand();
   return (int)(a*f);
}


//--------------------------------------------------------------------------//
// samplepalette (Motif)                                                    //
//--------------------------------------------------------------------------//
void samplepalette(void)
{
   if(g.bitsperpixel==8 || (ci>=0 && z[ci].bpp==8))
      sample_palette_pseudo();
   else
      sample_palette_color();
}


//--------------------------------------------------------------------------//
// sample_palette_color (Motif)                                             //
// Draw a sample palette square                                             //
//--------------------------------------------------------------------------//
void sample_palette_color(void)
{
   int n, xsize=130, ysize=171;
   Arg args[100];
   XmString xms;
   Widget form, drawing_area, helpbutton, cancelbutton;
   incolorpalette=1;
   int helptopic=5;

   n=0;
   XtSetArg(args[n], XmNwidth, xsize); n++;
   XtSetArg(args[n], XmNheight, ysize); n++;
   XtSetArg(args[n], XmNtransient, False); n++;
   XtSetArg(args[n], XmNresizable, False); n++;
   XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
   XtSetArg(args[n], XmNautoUnmanage, False); n++;
   XtSetArg(args[n], XmNdialogTitle, xms=XmStringCreateSimple((char*)"Colormap")); n++;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   form = XmCreateFormDialog(g.main_widget, (char*)"SampleColormapColorForm", args, n);
   XmStringFree(xms);
   
   n=0;
   XtSetArg(args[n], XmNtitle, "Colormap"); n++;
   XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
   XtSetArg(args[n], XmNwidth, xsize); n++;
   XtSetArg(args[n], XmNmappedWhenManaged, True); n++;
   XtSetArg(args[n], XmNleftPosition, 0); n++;            // % of width fromleft
   XtSetArg(args[n], XmNrightPosition, 100); n++;         // % of width fromleft
   XtSetArg(args[n], XmNtopPosition, 0); n++;             // % of height from top
   XtSetArg(args[n], XmNbottomPosition, 76); n++;         // % of width from top
   XtSetArg(args[n], XmNfractionBase, 100); n++;          // Use percentages
   XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   drawing_area = XmCreateDrawingArea(form, (char*)"drawing_area", args, n);
  
   n=0;
   XtSetArg(args[n], XmNleftPosition, 0); n++;            // % of width fromleft
   XtSetArg(args[n], XmNrightPosition, 100); n++;         // % of width fromleft
   XtSetArg(args[n], XmNtopPosition, 76); n++;            // % of height from top
   XtSetArg(args[n], XmNbottomPosition, 88); n++;         // % of width from top
   XtSetArg(args[n], XmNfractionBase, 100); n++;          // Use percentages
   XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   cancelbutton = XmCreatePushButton(form, (char*)"Cancel", args, n);

   n=0;
   XtSetArg(args[n], XmNleftPosition, 0); n++;            // % of width fromleft
   XtSetArg(args[n], XmNrightPosition, 100); n++;         // % of width fromleft
   XtSetArg(args[n], XmNtopPosition, 88); n++;            // % of height from top
   XtSetArg(args[n], XmNbottomPosition, 100); n++;        // % of width from top
   XtSetArg(args[n], XmNfractionBase, 100); n++;          // Use percentages
   XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   helpbutton = XmCreatePushButton(form, (char*)"Help", args, n);

   XtManageChild(cancelbutton);
   XtManageChild(helpbutton);
   XtManageChild(form);
   XtManageChild(drawing_area);

   XtAddCallback(form, XmNunmapCallback, 
       (XtCBP)samplepalettecolorunmapcb, (XtP)NULL);
   XtAddCallback(drawing_area, XmNexposeCallback,
       (XtCBP)samplepalettecolorexposecb, (XtP)NULL);
   XtAddCallback(drawing_area, XmNinputCallback, 
       (XtCBP)samplepalettecolorclickcb, (XtP)NULL);
   XtAddCallback(cancelbutton, XmNactivateCallback, (XtCBP)unmanagecb, (XtP)NULL);
   XtAddCallback(helpbutton, XmNactivateCallback, (XtCBP)helpcb, (XtP)&g.helptopic[helptopic]);
}

//--------------------------------------------------------------------------//
// samplepalettecolorexposecb - expose events for color sample palette      //
//--------------------------------------------------------------------------//
void samplepalettecolorexposecb(Widget w, XtP client_data, XmACB *call_data)
{ 
   call_data=call_data; client_data=client_data; // keep compiler quiet
   int color,i,j;
   for(j=0;j<129;j++)
   {  for(i=0; i<129; i++)
      {   color = palettecolor(i,j);   
          XSetForeground(g.display, g.image_gc, (ulong)color);  
          XDrawPoint(g.display, XtWindow(w), g.image_gc, i, j);
       }  
   }
}

//--------------------------------------------------------------------------//
// palettecolor - calculate color for sample color palette                  //
//--------------------------------------------------------------------------//
int palettecolor(int i, int j)
{   
    int rr,gg,bb;
    rr = (int)(256-1.25*i-1.25*j);
    rr += (int)(1.25*i*j*i*j/1048576);
    gg = i+i;
    bb = j+j;
    rr = max(0,min(255,rr));
    gg = max(0,min(255,gg));
    bb = max(0,min(255,bb));
    rr = rr*g.maxred[g.bitsperpixel]/255;
    gg = gg*g.maxgreen[g.bitsperpixel]/255;
    bb = bb*g.maxblue[g.bitsperpixel]/255;
    if(j<4){ bb = 0; }
    if(j>124){ bb = 255; }
    if(i<8){rr= gg = bb; }
    return RGBvalue(rr,gg,bb,g.bitsperpixel);
}        


//--------------------------------------------------------------------------//
// samplepalettecolorclickcb - click on palette to change drawing colors    //
//--------------------------------------------------------------------------//
void samplepalettecolorclickcb(Widget w, XtP client_data, XmACB *call_data)
{ 
  w=w; client_data=client_data; // keep compiler quiet
  int color;
  XEvent event;  
  XmDrawingAreaCallbackStruct *ptr;
  int button,x,y;
  ptr = (XmDrawingAreaCallbackStruct*) call_data;
  if(ptr==NULL) return;
  event = *(ptr->event);
  x = (int)event.xbutton.x;
  y = (int)event.xbutton.y;
  button = event.xbutton.button;
  color = palettecolor(x,y);
  if(button==1){ g.fcolor = color; g.line.color=color; }
  if(button>1) g.bcolor = color;
}


//--------------------------------------------------------------------------//
// samplepalettecolorunmapcb                                                //
//--------------------------------------------------------------------------//
void samplepalettecolorunmapcb(Widget w, XtP client_data, XmACB *call_data)
{
  if(!in_unmanagecb) unmanagecb(w, client_data, call_data);
}


//--------------------------------------------------------------------------//
// unmanagecb - callback for "cancel" button in sample color palette        //
//--------------------------------------------------------------------------//
void unmanagecb(Widget w, XtP client_data, XmACB *call_data)
{
   call_data=call_data; client_data=client_data; // keep compiler quiet
   in_unmanagecb = 1;
   XtUnmanageChild(XtParent(w));
   incolorpalette=0;
   in_unmanagecb = 0;
}


//--------------------------------------------------------------------------//
// sample_palette_pseudo (Motif)                                            //
// Draw a sample palette bar which can manipulate the colormap.             //
//--------------------------------------------------------------------------//
void sample_palette_pseudo(void)
{
  static int hit=0;
  static Widget form,drawing_area,drag_area,helpbutton,cancelbutton;
  static p3d *p;
  XmString xms;
  Arg args[100];
  int xsize = 100;
  int ysize = 557;
  int n;
  int helptopic=38;
  static palettecbstruct pp;
  static Pixmap icon;

  const uchar icon_data[] = {
  0x54, 0x01, 0xa8, 0x02, 0x50, 0x05, 0xa0, 0x0a, 
  0x50, 0x15, 0xa8, 0x2a, 0x14, 0x55, 0x0a, 0xaa, 
  0x05, 0x55, 0x82, 0x2a, 0x45, 0x15, 0xaa, 0x0a,
  0x54, 0x05, 0xa8, 0x02, 0x54, 0x01, 0xaa, 0x00,
  };

  if(inpalette) return;
  inpalette = 1;
  p = new p3d;

  pp.top=0;
  pp.center=256;
  pp.bottom=512;
  pp.button=0;
  pp.dragging=NONE;

  if(!hit)
  {    n=0;
       XtSetArg(args[n], XmNwidth, xsize); n++;
       XtSetArg(args[n], XmNmaxWidth, xsize); n++;
       XtSetArg(args[n], XmNheight, ysize); n++;
       XtSetArg(args[n], XmNminHeight, ysize); n++;
       XtSetArg(args[n], XmNmaxHeight, ysize); n++;
       XtSetArg(args[n], XmNtransient, False); n++;
       XtSetArg(args[n], XmNresizable, False); n++;
       XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
       XtSetArg(args[n], XmNautoUnmanage, False); n++;
       XtSetArg(args[n], XmNdialogTitle, xms=XmStringCreateSimple((char*)"Colormap")); n++;
       ////  Stop Motif from trying to grab another color if none are available.
       if(g.want_colormaps)  
       {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
            XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
       }
       form = XmCreateFormDialog(g.main_widget, (char*)"SampleColormapPseudoForm", args, n);
       XmStringFree(xms);

       n=0;
       XtSetArg(args[n], XmNtitle, "Colormap"); n++;
       XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
       XtSetArg(args[n], XmNwidth, xsize - 10); n++;
       XtSetArg(args[n], XmNmappedWhenManaged, True); n++;
       XtSetArg(args[n], XmNleftPosition, 0); n++;            // % of width fromleft
       XtSetArg(args[n], XmNrightPosition, 90); n++;          // % of width fromleft
       XtSetArg(args[n], XmNtopPosition, 0); n++;             // % of height from top
       XtSetArg(args[n], XmNbottomPosition, 92); n++;         // % of width from top
       XtSetArg(args[n], XmNfractionBase, 100); n++;          // Use percentages
       XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNresizable, False); n++;
       XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
       ////  Stop Motif from trying to grab another color if none are available.
       if(g.want_colormaps)  
       {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
            XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
       }
       drawing_area = XmCreateDrawingArea(form, (char*)"drawing_area", args, n);

       n=0;
       XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
       XtSetArg(args[n], XmNwidth, 10); n++;
       XtSetArg(args[n], XmNmappedWhenManaged, True); n++;
       XtSetArg(args[n], XmNleftPosition, 90); n++;           // % of width fromleft
       XtSetArg(args[n], XmNrightPosition, 100); n++;         // % of width fromleft
       XtSetArg(args[n], XmNtopPosition, 0); n++;             // % of height from top
       XtSetArg(args[n], XmNbottomPosition, 92); n++;         // % of width from top
       XtSetArg(args[n], XmNfractionBase, 100); n++;          // Use percentages
       XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
       ////  Stop Motif from trying to grab another color if none are available.
       if(g.want_colormaps)  
       {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
            XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
       }
       drag_area = XmCreateDrawingArea(form, (char*)"drag_area", args, n);

       n=0;
       XtSetArg(args[n], XmNleftPosition, 0); n++;            // % of width fromleft
       XtSetArg(args[n], XmNrightPosition, 90); n++;          // % of width fromleft
       XtSetArg(args[n], XmNtopPosition, 92); n++;            // % of height from top
       XtSetArg(args[n], XmNbottomPosition, 96); n++;         // % of width from top
       XtSetArg(args[n], XmNfractionBase, 100); n++;          // Use percentages
       XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
       ////  Stop Motif from trying to grab another color if none are available.
       if(g.want_colormaps)  
       {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
            XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
       }
       cancelbutton = XmCreatePushButton(form, (char*)"Ok", args, n);

       n=0;
       XtSetArg(args[n], XmNleftPosition, 0); n++;            // % of width fromleft
       XtSetArg(args[n], XmNrightPosition, 90); n++;          // % of width fromleft
       XtSetArg(args[n], XmNtopPosition, 96); n++;            // % of height from top
       XtSetArg(args[n], XmNbottomPosition, 100); n++;        // % of width from top
       XtSetArg(args[n], XmNfractionBase, 100); n++;          // Use percentages
       XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
       ////  Stop Motif from trying to grab another color if none are available.
       if(g.want_colormaps)  
       {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
            XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
       }
       helpbutton = XmCreatePushButton(form, (char*)"Help", args, n);
  }
  icon = XCreateBitmapFromData(g.display, RootWindow(g.display, 
         DefaultScreen(g.display)), (char*)icon_data, 16, 16);
  if(icon != (Pixmap)None)
  {    n=0;
       XtSetArg(args[n], XmNiconic, False); n++;
       XtSetArg(args[n], XmNiconName, "Icon"); n++;
       XtSetArg(args[n], XmNiconPixmap, icon); n++;
       XtSetValues(XtParent(form), args, n);
  }else fprintf(stderr, "Can't create icon\n");

  XtManageChild(cancelbutton);
  XtManageChild(helpbutton);
  XtManageChild(form);
  XtManageChild(drawing_area);
  XtManageChild(drag_area);

  pp.win = XtWindow(drag_area);  // Wait until managed so window exists.

  if(!hit) XtAddCallback(form, XmNunmapCallback, (XtCBP)samplepalettepseudounmapcb,(XtP)p);
  XtAddCallback(drawing_area,XmNexposeCallback,(XtCBP)samplepaletteexposecb,(XtP)&pp);
  XtAddCallback(drawing_area,XmNinputCallback,(XtCBP)samplepaletteclickcb,(XtP)&pp);
  XtAddEventHandler(drawing_area,g.mouse_mask,False,(XtEH)samplepalettemousecb,(XtP)&pp);
  XtAddCallback(drag_area, XmNexposeCallback,(XtCBP)dragpaletteexposecb,(XtP)&pp);
  XtAddCallback(drag_area, XmNinputCallback, (XtCBP)dragpaletteclickcb, (XtP)&pp);
  XtAddEventHandler(drag_area,g.mouse_mask,False,(XtEH)dragpalettemousecb,(XtP)&pp);
  XtAddCallback(cancelbutton, XmNactivateCallback, (XtCBP)sample_palette_pseudocb, (XtP)p);
  XtAddCallback(helpbutton, XmNactivateCallback, (XtCBP)helpcb, (XtP)&g.helptopic[helptopic]);

  p->ptr[8] = &pp;
  p->ptr[9] = (void*)icon;
  p->form = form;
  p->miscwidget[0] = drawing_area;
  p->miscwidget[1] = drag_area;
  p->miscwidget[2] = cancelbutton;
  p->miscwidget[3] = helpbutton;
  hit=1;
}


//--------------------------------------------------------------------------//
// sample_palette_pseudocb                                                  //
//--------------------------------------------------------------------------//
void sample_palette_pseudocb(Widget w, XtP client_data, XmACB *call_data)
{
  if(!inpalette) return;
  w=w;call_data=call_data;
//  int helptopic=0;
  p3d *p = (p3d*)client_data; 
  in_samplepalettepseudocb = 1;
if(!inpalette) return;
if(!p) return;
//  palettecbstruct *pp  = (palettecbstruct *) p->ptr[8];
  Widget form         = p->form;
//  Widget drawing_area = p->miscwidget[0];
//  Widget drag_area    = p->miscwidget[1];
//  Widget cancelbutton = p->miscwidget[2];
//  Widget helpbutton   = p->miscwidget[3];
//  Pixmap icon         = (Pixmap) p->ptr[9];
  in_samplepalettepseudocb = 1;
if(!form) return;

/*
  XtRemoveCallback(drawing_area,XmNexposeCallback,(XtCBP)samplepaletteexposecb,(XtP)&pp);
  XtRemoveCallback(drawing_area,XmNinputCallback,(XtCBP)samplepaletteclickcb,(XtP)&pp);
  XtRemoveEventHandler(drawing_area,g.mouse_mask,False,(XtEH)samplepalettemousecb,(XtP)pp);
  XtRemoveCallback(drag_area, XmNexposeCallback,(XtCBP)dragpaletteexposecb,(XtP)&pp);
  XtRemoveCallback(drag_area, XmNinputCallback, (XtCBP)dragpaletteclickcb, (XtP)&pp);
  XtRemoveEventHandler(drag_area,g.mouse_mask,False,(XtEH)dragpalettemousecb,(XtP)pp);
  XtRemoveCallback(cancelbutton, XmNactivateCallback, (XtCBP)sample_palette_pseudocb, (XtP)p);
  XtRemoveCallback(helpbutton, XmNactivateCallback, (XtCBP)helpcb, (XtP)&g.helptopic[helptopic]);
  XtRemoveCallback(form, XmNunmapCallback, (XtCBP)sample_palette_pseudocb, (XtP)p);
*/

  if(XtIsManaged(form)) XtUnmanageChild(form);
  inpalette = 0;
//  if(icon != (Pixmap)None) XFreePixmap(g.display, icon);
//  if(p) delete p;
  in_samplepalettepseudocb = 0;
}

//--------------------------------------------------------------------------//
// samplepalettepseudounmapcb                                               //
//--------------------------------------------------------------------------//
void samplepalettepseudounmapcb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; client_data=client_data;call_data=call_data;
  p3d *p = (p3d*)client_data; 
  Widget form         = p->form;
  if(XtIsManaged(form)) XtUnmanageChild(form);
  inpalette = 0;
}


//--------------------------------------------------------------------------//
// update_sample_palette - only needed if not in 8 bit screen mode          //
//--------------------------------------------------------------------------//
void update_sample_palette(void)
{
    if(inpalette) samplepaletteexposecb(0,0,0);
}


//--------------------------------------------------------------------------//
// samplepaletteexposecb - expose events for sample palette                 //
//--------------------------------------------------------------------------//
void samplepaletteexposecb(Widget w, XtP client_data, XmACB *call_data)
{ 
  if(!inpalette) return;
  static Widget samplepalettewidget=0;
  if(w) samplepalettewidget = w;
  w = samplepalettewidget;
  if(w==0) return;
  call_data=call_data; client_data=client_data; // keep compiler quiet
  int k,color,rr,gg,bb;
  for(k=0;k<256;k++)
  {   rr = g.palette[k].red;
      gg = g.palette[k].green;
      bb = g.palette[k].blue;
      convertRGBpixel(rr,gg,bb,8,g.bitsperpixel);
      color = RGBvalue(rr,gg,bb,g.bitsperpixel);
      XSetForeground(g.display, g.image_gc, color);
      XDrawLine(g.display,XtWindow(w),g.image_gc,1,k*2,90,k*2);
      XDrawLine(g.display,XtWindow(w),g.image_gc,1,k*2+1,90,k*2+1);
  }
  XRaiseWindow(g.display, XtWindow(XtParent(XtParent(w))));
}


//--------------------------------------------------------------------------//
// samplepaletteclickcb - click on palette to change drawing colors         //
//--------------------------------------------------------------------------//
void samplepaletteclickcb(Widget w, XtP client_data, XmACB *call_data)
{ 
  if(!inpalette) return;
  w=w; client_data=client_data; // keep compiler quiet
  XEvent event;  
  XmDrawingAreaCallbackStruct *ptr;
  int button,y;
  ptr = (XmDrawingAreaCallbackStruct*) call_data;
  if(ptr==NULL) return;
  event = *(ptr->event);
  y = (int)event.xbutton.y;
  button = event.xbutton.button;
  if(button==1){ g.fcolor = y/2; g.line.color = y/2; }
  if(button>1) g.bcolor = y/2;
}


//--------------------------------------------------------------------------//
// samplepalettemousecb                                                     //
//--------------------------------------------------------------------------//
void samplepalettemousecb (Widget w, XtP client_data, XEvent *event)
{  
  if(!inpalette) return;
  w=w; client_data=client_data; // keep compiler quiet
  int x, y, ogi=g.imageatcursor;
  x = event->xmotion.x;
  y = event->xmotion.y;
  g.imageatcursor = -2;
  printcoordinates(x,y/2,0);
  g.imageatcursor = ogi;
}


//--------------------------------------------------------------------------//
// dragpaletteexposecb - expose events for drag area on sample palette      //
//--------------------------------------------------------------------------//
void dragpaletteexposecb(Widget w, XtP client_data, XmACB *call_data)
{ 
  if(!inpalette) return;
  w=w; call_data=call_data; // keep compiler quiet
  palettecbstruct *pp = (palettecbstruct *)client_data;
  XSetForeground(g.display, g.image_gc, BlackPixel(g.display, g.screen));
  drawpalettearrows(pp);
}


//--------------------------------------------------------------------------//
// drawpalettearrows                                                        //
//--------------------------------------------------------------------------//
void drawpalettearrows(palettecbstruct *pp)
{
  if(!inpalette) return;
  static int otop=0, ocenter=0, obottom=0;
  XDrawImageString(g.display, pp->win, g.gc, 0, max(10,min(508,otop))    ,"  ",2);
  XDrawImageString(g.display, pp->win, g.gc, 0, max(10,min(508,ocenter)) ,"  ",2);
  XDrawImageString(g.display, pp->win, g.gc, 0, max(10,min(508,obottom)) ,"  ",2);
  XDrawString(g.display, pp->win, g.gc, 0, max(10,min(508,pp->top))    ,"< ",2);
  XDrawString(g.display, pp->win, g.gc, 0, max(10,min(508,pp->center)) ,"= ",2);
  XDrawString(g.display, pp->win, g.gc, 0, max(10,min(508, pp->bottom)),"< ",2);

  otop = pp->top;
  ocenter = pp->center;
  obottom = pp->bottom;
}


//--------------------------------------------------------------------------//
// dragpaletteclickcb                                                       //
//--------------------------------------------------------------------------//
void dragpaletteclickcb (Widget w, XtP client_data, XmACB *call_data)
{ 
  if(!inpalette) return;
  w=w;  // keep compiler quiet
  XEvent event;  
  int x,y;
  XmDrawingAreaCallbackStruct *ptr;
  ptr = (XmDrawingAreaCallbackStruct*) call_data;
  if(ptr==NULL) return;
  event = *(ptr->event);
  x = (int)event.xbutton.x;
  y = (int)event.xbutton.y;
  palettecbstruct *pp = (palettecbstruct *)client_data;
  pp->button = event.xbutton.button;
  printcoordinates(x,y,0);
  switch(ptr->event->type)
  {    case ButtonPress: 
           if(abs(y - max(0,min(512,pp->top)))<20) pp->dragging=SCRUNCHDOWN;
           if(abs(y - pp->center)<20) pp->dragging=ROTATE;
           if(abs(y - max(0,min(512,pp->bottom)))<20) pp->dragging=SCRUNCHUP;
           break;
       case ButtonRelease: 
           pp->dragging = NONE;
           break;
       default: break;
   }  
}


//--------------------------------------------------------------------------//
// dragpalettemousecb                                                       //
// Called when mouse is dragged in palette. pp->top  and pp->bottom can be  //
// any value but if off scale they are displayed at the edge of the palette.//
//--------------------------------------------------------------------------//
void dragpalettemousecb(Widget w, XtP client_data, XEvent *event)
{  
  if(!inpalette) return;
  w=w;  // keep compiler quiet
  palettecbstruct *pp = (palettecbstruct *)client_data;
  int y, dy, oy;
  y = event->xmotion.y;
  if(pp->dragging == SCRUNCHDOWN) pp->top = y;
  if(pp->dragging == ROTATE) 
  {   oy = pp->center;
      pp->center = max(0,min(512,y));
      dy = y - oy;
      pp->top    += dy;
      pp->bottom += dy;
  }
  if(pp->dragging == SCRUNCHUP) pp->bottom = y;
  if(pp->dragging)
  {    drawpalettearrows(pp);
       dragpalette(pp);
  }
}  


//--------------------------------------------------------------------------//
// dragpalette                                                              //
// Change contrast or brightness by dragging on palette                     //
// Called by samplepalettecallback() when mouse drag event occurs           //
//--------------------------------------------------------------------------//
int dragpalette(palettecbstruct *pp)
{
  if(!inpalette) return ABORT;
  int i,k;
  double f=1.0;
  RGB newpalette[256];
  if(pp->dragging<0 || pp->dragging>3) return ERROR; // Sanity check

  if(g.selectedimage>-1 && ci>=0) g.currentpalette=-2;
  else g.currentpalette = g.bkgpalette;
  setSVGApalette(g.currentpalette,0);         // Reset palette before changing 
  
  if(pp->bottom - pp->top !=0)
    f = 512.0/(double)(pp->bottom - pp->top);  // Contrast factor

  for(k=1;k<=min(255,pp->top/2);k++)          // Set colors 0 to top all the same
  {  newpalette[k].red  = g.palette[0].red; 
     newpalette[k].green= g.palette[0].green;
     newpalette[k].blue = g.palette[0].blue;
  }

  for(k=max(0,pp->bottom/2);k<=255;k++)       // Set colors bottom to 255 all the same
  {  newpalette[k].red  = g.palette[255].red; 
     newpalette[k].green= g.palette[255].green;
     newpalette[k].blue = g.palette[255].blue;
  }  
                                              // Scale rest of colors with f 
  for(k=max(0,pp->top/2);k<=min(255,pp->bottom/2);k++)
  {  i =  (int)(f * (double)(k - pp->top/2));
     i = max(1,min(255,i));
     newpalette[k].red  = g.palette[i].red; 
     newpalette[k].green= g.palette[i].green;
     newpalette[k].blue = g.palette[i].blue;
  }
  
  memcpy(g.palette,newpalette,768); 
  setpalette(g.palette);
  memcpy(otherpalette,g.palette,768);          
  if(g.selectedimage>=0) 
         memcpy(z[g.selectedimage].palette,g.palette,768); 
  else  
         memcpy(g.b_palette,g.palette,768);                  
  return OK;
}


//--------------------------------------------------------------------------//
// setpalette                                                               //
// Motif version - sets all 256 colors of colormap except those listed in   //
// global array 'reserved[]'.  ' pal' is a pointer  to a packed RGB struct. //
// Before calling setpalette(), the calling program should first decide     //
// which colors are reserved, then remap the reserved pixel values to the   //
// next closest color.                                                      //
//--------------------------------------------------------------------------//
void setpalette(RGB* pal)
{   
  static RGB opal[256];
  int k,same;
  Window win=0;
  memcpy(g.palette,pal,768);  // Make `palette' the current palette 

  if(g.bitsperpixel>8) 
  {  
      //// Remapping colors in >8bpp mode is expensive so check first
      same=1;
      for(k=0;k<256;k++)
      {    if(pal[k].red   != opal[k].red   ||
              pal[k].green != opal[k].green ||
              pal[k].blue  != opal[k].blue)  {  same=0; break; }
      }      

      if(!same && z[ci].colortype==INDEXED) 
      {   
           memcpy(z[ci].palette, pal, 768);
           rebuild_display(ci);
           redraw(ci);         
           memcpy(opal, pal,768);
      }
      update_sample_palette();
      return;
  }
  if(g.visual->c_class == StaticColor ||
     g.visual->c_class == StaticGray  ||
     g.visual->c_class == TrueColor   ||
     g.visual->c_class == DirectColor) return;

  ////  8 bit/pixel mode from here on

  Colormap defcmap;           // Default colormap of theDisplay
  int colormap_size;       
  int ncolors = 256;          // No.of colors to allocate
  XColor *color;              // The defined color values to be sent to server
  defcmap = XDefaultColormap(g.display, g.screen);

  //// Re-check the default colormap

  XQueryColors(g.display, defcmap, g.def_colors, 256);
  if(g.want_colormaps)
  {
     //// Don't allocate color cells, just change colormap.           
     //// Any allocated color cells must be freed before going here.  
     //// 'xcolormap' is associated with a single window (theWindow)  
     //// when it was created in init_data().                         
     
     colormap_size = DisplayCells(g.display, g.screen);
     if((color = (XColor *)calloc(colormap_size+2,sizeof(XColor)))==NULL)
     {   fprintf(stderr," Cant allocate colors\n");
         return;
     }  
  
     for(k=0;k<256;k++)
     {  color[k].pixel = k; 
        color[k].flags = DoRed | DoGreen | DoBlue;
        if(g.reserved[k])
        {  color[k].red   = g.def_colors[k].red;
           color[k].green = g.def_colors[k].green;
           color[k].blue  = g.def_colors[k].blue;
        }else
        {  color[k].red   = pal[k].red*1024; 
           color[k].green = pal[k].green*1024; 
           color[k].blue  = pal[k].blue*1024;
        }
     } 
     if(g.selectedimage==-1)
          win = g.main_window;
     else if(ci>=0)
          win = z[ci].win;
     else win=0;
     
     if(win!=0)      // win could be 0 if setpalette is called too soon
     {    XStoreColors(g.display, g.xcolormap, color, ncolors);
          XSetWindowColormap(g.display, win, g.xcolormap); 
          XtVaSetValues(g.drawing_area, XmNcolormap, g.xcolormap,NULL);
          XtVaSetValues(g.main_widget, XmNcolormap, g.xcolormap,NULL);
          XInstallColormap(g.display, g.xcolormap);
          free(color); 
     }
  }else
  {
     ////  Modify the rgb of the colors owned by application
     XColor xcolor;
     for(k=0;k<256;k++) 
     {   if(g.reserved[k]) continue;
         xcolor.pixel = k;
         xcolor.red = pal[k].red*1024;
         xcolor.green = pal[k].green*1024;
         xcolor.blue = pal[k].blue*1024;
         xcolor.flags = DoRed|DoGreen|DoBlue;
         XStoreColor(g.display,defcmap,&xcolor);
     }

  }
  update_sample_palette();
}



//--------------------------------------------------------------------------//
// invertpalette                                                            //
//--------------------------------------------------------------------------//
int invertpalette(void)
{   int i;
    RGB *temppal;
    temppal = new RGB[256];
    if(temppal==NULL){message(g.nomemory,ERROR);return(NOMEM);}
    for(i=0;i<256;i++) 
    { temppal[i].red  = g.palette[255-i].red; 
      temppal[i].green= g.palette[255-i].green; 
      temppal[i].blue = g.palette[255-i].blue; 
    }
    temppal[0].red=0;        // These 6 have to be explicitly set
    temppal[0].green=0;      // or #9GXE64 card screws up the palette
    temppal[0].blue=0;       // for unknown reason in DOS.
    temppal[255].red=63;
    temppal[255].green=63;
    temppal[255].blue=63;
    setpalette(temppal);
    delete[] temppal;
    return(OK);
} 


//--------------------------------------------------------------------------//
// readpalette                                                              //
// Read ASCII file with user-defined palette                                //
//  col.1=index  col.2=red  col.3=green col.4=blue                          //
//--------------------------------------------------------------------------//
int readpalette(void)
{
  static char filename[FILENAMELENGTH]="1.pal";

  char tempstring[64];
  FILE *fp;
  int k,rr,gg,bb;
  g.getout=0;
  message("Colormap file name:",filename,PROMPT,FILENAMELENGTH-1,54);
  if(g.getout) return(ABORT);

  chdir(g.startdir);       // Change back to start-up directory
  if ((fp=fopen(filename,"r")) == NULL)
  {   error_message(filename, errno);
      chdir(g.currentdir); // Change back to original directory
      return(ERROR);
  }
  while(!feof(fp)) 
  {   fgets(tempstring, 64, fp);
      sscanf(tempstring,"%d %d %d %d",&k, &rr, &gg, &bb); 
      if(between(k, 0, 255))
      {   g.palette[k].red = rr;
          g.palette[k].green = gg; 
          g.palette[k].blue = bb; 
      }
  }
  fclose(fp);  
  chdir(g.currentdir);     // Change back to original directory
  setpalette(g.palette);
  return(OK);
}


//--------------------------------------------------------------------------//
// savepalette                                                              //
// Save ASCII file with user-defined palette                                //
//  col.1=index  col.2=red  col.3=green col.4=blue                          //
//--------------------------------------------------------------------------//
int savepalette(void)
{
  static PromptStruct ps;
  static char filename[FILENAMELENGTH]="1.pal";
  g.getout=0;
  message("Colormap file name:",filename,PROMPT,FILENAMELENGTH-1,54);
  if(g.getout) return ABORT;
  ps.filename = filename;
  ps.f1 = savepalette_part2;
  ps.f2 = null;
  check_overwrite_file(&ps);
  return OK;
}  


//--------------------------------------------------------------------------//
// savepalette_part2                                                        //
//--------------------------------------------------------------------------//
void savepalette_part2(PromptStruct *ps)
{
  FILE *fp;
  char *filename = ps->filename;
  int k;
  chdir(g.startdir);            // Change back to start-up directory
  if ((fp=fopen(filename,"w")) == NULL)
  {   error_message(filename, errno);
      chdir(g.currentdir);      // Change back to original directory
      return;
  }
  for(k=0;k<256;k++) 
    fprintf(fp,"%d %d %d %d\n",k,g.palette[k].red,g.palette[k].green,
        g.palette[k].blue); 
  
  fclose(fp);  
  chdir(g.currentdir);          // Change back to original directory
  setpalette(g.palette);
  return;
}


//--------------------------------------------------------------------------//
// createpalette                                                            //
// Graphically create a new user-defined palette                            //
//--------------------------------------------------------------------------//
int createpalette(void)
{
    int i,k;
    if(memorylessthan(16384)){ message(g.nomemory,ERROR); return(NOMEM); }
    char title[FILENAMELENGTH] = "Colormap";
    array<int> pdata(256,3);   
    array<double> xdata(256);
    array<char> ytitle(256,3);
    for(k=0;k<3;k++) ytitle.p[k] = new char[256];
    for(i=0;i<256;i++)
    {   pdata.p[0][i] = g.palette[i].red; 
        pdata.p[1][i] = g.palette[i].green;
        pdata.p[2][i] = g.palette[i].blue;   
        xdata.data[i] = (double)i;
    } 
    strcpy(ytitle.p[0]," Red");
    strcpy(ytitle.p[1]," Gren");
    strcpy(ytitle.p[2]," Blue");
    plot(title, (char*)"Intensity", ytitle.p, xdata.data, pdata.p, 
        256, 3, CHANGE, 0, 64, NULL, plotpalettecallback,  
        plotinstallcolormapcb, null, null, null, 12);
    return OK;
}


//--------------------------------------------------------------------------//
// plotpalettecallback                                                      //
// A callback called from graphhandle() after mouse-drag events             //
//  v.x = x to change                                                       //
//  v.y = y value                                                           //
//  v.focus = focus                                                         //
//--------------------------------------------------------------------------//
void plotpalettecallback(pcb_struct v)
{
   v.y = max(0,min(v.y,63));
   v.x = max(0,min(v.x,255));
   switch(v.focus)     
   {  case 0: g.palette[v.x].red  = v.y; break;
      case 1: g.palette[v.x].green= v.y; break;
      case 2: g.palette[v.x].blue = v.y; break;
   }
   setpalette(g.palette);
}


//--------------------------------------------------------------------------//
// plotinstallcolormap                                                      //
//--------------------------------------------------------------------------//
void plotinstallcolormapcb(void *p, int n1, int n2)
{
  n1=n1; n2=n2;
  PlotData *pd = (PlotData*)p;
  int k,focus;
  focus = pd->focus;
  for(k=0;k<256;k++)
  {  switch(focus)
     {    case 0: z[ci].palette[k].red   = (int)pd->data[focus][k]; break;
          case 1: z[ci].palette[k].green = (int)pd->data[focus][k]; break;
          case 2: z[ci].palette[k].blue  = (int)pd->data[focus][k]; break;
     }
  }
  setpalette(z[ci].palette);
  if(g.selectedimage>=0)
  {   if(z[ci].colortype!=INDEXED && z[ci].colortype!=GRAPH) 
          message("Not an indexed-color image\nretaining old colormap",WARNING);
      memcpy(z[ci].palette,g.palette,768);
      memcpy(z[ci].opalette,g.palette,768);
      rebuild_display(ci);
      z[ci].touched=1;
  }else
  {   memcpy(g.b_palette, g.palette, 768);
      setpalette(g.b_palette);
  }
  redrawscreen();
}


//--------------------------------------------------------------------------//
// histogram                                                                //
//--------------------------------------------------------------------------//
void histogram(Graph graph)
{  
  static int ino;
  int bpp,dim,obpp,i,j,graphwidth,k,rr,gg,bb;
  int gray, ogray, xdisplayfac;
  uint val;
  char badbpp[50] ="Selected area is invalid\nfor histogram";
  char **ytitle;
  double scalefac, ddisplayfac, rscalefac, gscalefac, bscalefac;
  PlotData *pd;
  static void (*p4cb)(void *pd, int n1, int n2) = NULL;
  static int *edata = NULL;

  ytitle = new char*[4];
  for(k=0;k<4;k++) ytitle[k] = new char[256];
  array<int> histogram(267,3);
  static array<int> histogram16(65536);
  static array<double> xdata(256);
  static array<int> count(256);

  //// Don't use whichimage, may want bpp of screen if dragging mouse
  readpixelonimage((g.ulx+g.lrx)/2, (g.uly+g.lry)/2, obpp, ino);
  ogray = (z[ino].colortype == GRAY);
  char title[FILENAMELENGTH] = "Histogram";

  for(k=0;k<256;k++)
  {   for(j=0;j<3;j++) histogram.p[j][k]=0;
      xdata.data[k] = (double)k;
      count.data[k] = 0;
  }    

  ddisplayfac = (1.0 + g.maxvalue[obpp]) / 256.0;
  xdisplayfac = (int)((1.0 + g.maxvalue[obpp]) / 256.0);

  if(ogray)
  {   graphwidth = 256; 
      strcpy(ytitle[0],"Count");
      scalefac = 1.0 / ddisplayfac;  /// scale gray to 0..255
      rscalefac = gscalefac = bscalefac = 1.0;
  }else 
  {   graphwidth = 256;
      strcpy(ytitle[0],"Red");
      strcpy(ytitle[1],"Green");
      strcpy(ytitle[2],"Blue");
      rscalefac = 255.0 / g.maxred[obpp];
      gscalefac = 255.0 / g.maxgreen[obpp];
      bscalefac = 255.0 / g.maxblue[obpp];
      scalefac = max(rscalefac, max(gscalefac, bscalefac));
  }

  if(ogray && z[ci].bpp == 16) 
  {    p4cb = histogram_savecb;
       edata = histogram16.data;
  }
  else 
  {    p4cb = (void (*)(void *, int, int)) null;
       edata = NULL;
  }

  for(j=g.uly; j<g.lry; j++)
  for(i=g.ulx; i<g.lrx; i++)
  {  if(!g.selected_is_square && !inside_irregular_region(i,j)) continue;
     gray = (z[ino].colortype == GRAY);
     if(gray) 
     {   val = readpixelonimage(i,j,bpp,ino);
         count.data[int(val*scalefac)]++;
         histogram.p[0][int(val*scalefac)]++;
         if(z[ci].bpp==16) histogram16.data[val]++;
     }else
     {   readRGBonimage(i,j,bpp,ino,rr,gg,bb,-2);
         histogram.p[0][int(rr*rscalefac)]++;
         histogram.p[1][int(gg*gscalefac)]++;
         histogram.p[2][int(bb*bscalefac)]++;
     }
     if((bpp != obpp) || (gray != ogray)) { message(badbpp,ERROR); return; }
  }

  if(graph.type == NONE) 
  {   
       if(ogray)
            pd = plot(title,(char*)"Pixel value", ytitle, xdata.data, 
            count.data, 256, 1, MEASURE, 0, 0, edata, null, null, null, 
            p4cb, null, 18, xdisplayfac, 1);
       else
            pd = plot(title,(char*)"Pixel value", ytitle, xdata.data, 
            histogram.p, graphwidth, 3, MEASURE, 0, 0, edata, null, null, 
            null, p4cb, null, 19);
       open_graph(pd, ci, HISTOGRAM_GRAPH);  
  }else 
  {    pd = graph.pd;
       for(dim=0; dim<pd->dims; dim++)
       {   delete[] pd->data[dim];  
           pd->data[dim] = new double[pd->n+1];       
           for(j=0; j<pd->n; j++) pd->data[dim][j] = (double)histogram.p[dim][j];
           redraw_graph(pd);
       }
  }   
  for(k=0;k<4;k++) delete[] ytitle[k];
  delete[] ytitle;
}


//--------------------------------------------------------------------------//
//  histogram_savecb                                                        //
//--------------------------------------------------------------------------//
void histogram_savecb(void *ptr, int n1, int focus)
{
   PlotData *pd = (PlotData *)ptr;
   n1=n1; focus=focus;
   save_scan(pd->title, pd->edata, 65536, 0);
}


//--------------------------------------------------------------------------//
// remapcolors                                                              //
// For 8-bit/pixel mode:                                                    //
//  Read ASCII file which consists of 2 columns. Map all pixel values in    //
//  column 1 to value in column 2.                                          //
// For color modes:                                                         //
//  Swap r,g, and b values to user-specified permutation of r,g, and b.     //
//--------------------------------------------------------------------------//
void remapcolors(void)
{
  static char filename[FILENAMELENGTH]="1.map";
  static char temp[40]="For color region: map ";
  static char cmap[3][10] = { "r","g","b" };
  static int remap[3] = { 0,1,2 };

  if(memorylessthan(4096)){  message(g.nomemory,ERROR); return; } 
  int color[3];
  char tempstring[128];
  int map[256];
  int gotbw=0,gotcolor=0;
  FILE *fp;
  int i,j,k,bpp,ino;
  uint value;
  
  for(k=0; k<g.image_count; k++) z[k].hit=0;

  g.getout=0;
  if(whatbpp(g.selected_ulx,g.selected_uly)==8) gotbw=1; else gotcolor=1;
  if(whatbpp(g.selected_ulx,g.selected_lry)==8) gotbw=1; else gotcolor=1;
  if(whatbpp(g.selected_lrx,g.selected_uly)==8) gotbw=1; else gotcolor=1;
  if(whatbpp(g.selected_lrx,g.selected_lry)==8) gotbw=1; else gotcolor=1;
  if(whatbpp((g.selected_ulx+g.selected_lrx)/2,
         (g.selected_uly + g.selected_lry)/2)==8) gotbw=1; else gotcolor=1;
 
  if(gotbw)
  {   message("Map file name for 8 bit region:",filename,PROMPT,FILENAMELENGTH-1,58);
      if(g.getout){ g.getout=0; return; }
      for(k=0;k<256;k++) map[k]=k;
      chdir(g.startdir);      // Change back to start-up directory
      if ((fp=fopen(filename,"r")) == NULL)
      {   error_message(filename, errno);
          chdir(g.currentdir);      // Change back to original directory
          return;
      }
      while(!feof(fp)) 
      {   fscanf(fp,"%d",&k); 
          fscanf(fp,"%d",&map[k]); 
      }
      fclose(fp);  
      chdir(g.currentdir);      // Change back to original directory
   }
   if(gotcolor)
   {  strcpy(tempstring,temp);
      strcat(tempstring,"red to (r,g,b):");
      message(tempstring,cmap[0],PROMPT,9,59);
      if(g.getout){ g.getout=0; return; }
      strcpy(tempstring,temp);
      strcat(tempstring,"green to (r,g,b):");
      message(tempstring,cmap[1],PROMPT,9,59);
      if(g.getout){ g.getout=0; return; }
      strcpy(tempstring,temp);
      strcat(tempstring,"blue to (r,g,b):");
      message(tempstring,cmap[2],PROMPT,9,59);
      if(g.getout){ g.getout=0; return; }

      for(k=0;k<3;k++)
      {   strlwr(cmap[k]);
          cmap[k][1] = 0;
          if(cmap[k][0]=='r') remap[k]=0;
          if(cmap[k][0]=='g') remap[k]=1;
          if(cmap[k][0]=='b') remap[k]=2;
      }    
   }   


   drawselectbox(OFF);
   for(j=g.selected_uly; j<=g.selected_lry; j++)
   {  for(i=g.selected_ulx; i<=g.selected_lrx; i++)
      {  if(!g.selected_is_square && !inside_irregular_region(i,j)) continue;
         value = readpixelonimage(i,j,bpp,ino);
         if(ino>=0)
         {    if((g.bitsperpixel==8)&&(bpp!=8)&&(z[ino].hitgray==0)) 
                  z[ino].hit=1;
              if(!g.wantr || !g.wantg || !g.wantb) z[ino].hit=1;  
         }
         
         if(bpp==8)
              value = map[value];
         else
         {    valuetoRGB(value,color[0],color[1],color[2],bpp);
              value=RGBvalue(color[remap[0]],color[remap[1]],color[remap[2]],bpp);
         }   
         setpixelonimage(i,j,value,g.imode,bpp);
      }
      if(keyhit()) if(getcharacter()==27) break;
   }
   for(k=0; k<g.image_count; k++) 
      if(z[k].hit){ rebuild_display(k); redraw(k);}
   redrawscreen();
}


//--------------------------------------------------------------------------//
// remap_image_colormap                                                     //
// Change entire image to fit to colormap of specified image                //
//--------------------------------------------------------------------------//
void remap_image_colormap(int ino)
{
   static int match_ino = ci;
   static int iii = ino;
   clickbox("Image with desired colormap to match", 10, &match_ino, 0,
       g.image_count-1, null, cset, null, &iii, NULL, 0);
}

//--------------------------------------------------------------------------//
// remap_image_colormap_part2                                               //
//--------------------------------------------------------------------------//
void remap_image_colormap_part2(clickboxinfo *c)
{
   int ncolors=0;
   int *iii = (int*)c->client_data;
   int ino = *iii;
   int match_ino = c->answer;
   if(!between(ino, 0, g.image_count-1)) return;
   if(!between(match_ino, 0, g.image_count-1)) return;
   if(z[ino].bpp !=8 || z[match_ino].bpp !=8)
   {   message("Both images must be 8 bits/pixel");
       return;
   }
   switchto(ino);
   switch_palette(ino);

   fitpalette(8, z[ino].image, z[ino].image, z[match_ino].palette, 
             z[ino].xsize, z[ino].ysize, z[ino].frames, ncolors);
   memcpy(z[ino].palette, z[match_ino].palette, 768);
}


//--------------------------------------------------------------------------//
// switch_palette                                                           //
//--------------------------------------------------------------------------//
void switch_palette(int ino)
{
   if(between(ino, 0, g.image_count-1)) memcpy(g.palette, z[ino].palette, 768);
}


//--------------------------------------------------------------------------//
// histogram_equalize                                                       //
//--------------------------------------------------------------------------//
void histogram_equalize(int ino)
{    
  int count,rcount,gcount,bcount,i,ii,j,k,rr,gg,bb,
      colorsize,histogramsize,freqsize,graysize;
  int color;
  int value;
  int bpp = z[ino].bpp;
  int obpp = bpp;
  int b = g.off[bpp];
  int xsize = z[ino].xsize;
  int ysize = z[ino].ysize;
  int gray = z[ino].colortype == GRAY;
  int size = xsize * ysize;
  int owantr = g.wantr;
  int owantg = g.wantg;
  int owantb = g.wantb;
  double rcontrast, gcontrast, bcontrast, rgbcontrast, pixperbin;
  int rtot, gtot, btot, rvalue, gvalue, bvalue;

  int maxpix = 65536;
  int maxval = (int)g.maxvalue[bpp];
  if(gray)
  {   colorsize = 1;
      histogramsize = 1;
      graysize = size;
      freqsize = maxpix;
      pixperbin = max(1.0, (double)size / (double)maxval);
  }else
  {   z[ino].hit=2;   
      if(change_image_depth(ino,24,0)!=OK)
      {  message("Error changing image depth",ERROR); return; }
      pixperbin = max(1.0, (double)size / 256.0);
      bpp = 24;
      b = 3;
      colorsize = size;
      graysize = 1;
      histogramsize = maxpix;
      freqsize = 1;
  }
   
  if(gray && bpp>16){ message("Unable to equalize histogram",ERROR); return; } 
  
  //// Needed for grayscale
  //// Can't put array declarations behind an if.
  array<int> freq(freqsize);
  if(!freq.allocated){ message(g.nomemory, ERROR); return; }
  array<int> sorted(graysize+2);
  if(!sorted.allocated){ message(g.nomemory, ERROR); return; }

  //// Needed for color
  array<int> histogram(histogramsize,3);
  if(!histogram.allocated){ message(g.nomemory, ERROR); return; }
  array<int> rsorted(colorsize+2);
  if(!rsorted.allocated){ message(g.nomemory, ERROR); return; }
  array<int> gsorted(colorsize+2);
  if(!gsorted.allocated){ message(g.nomemory, ERROR); return; }
  array<int> bsorted(colorsize+2);
  if(!bsorted.allocated){ message(g.nomemory, ERROR); return; }

  if(gray)
  {   for(k=0;k<maxpix;k++) freq.data[k]=0;
      for(i=0; i<size; i++) sorted.data[i] = 0;
  }else
  {   for(i=0; i<colorsize; i++) 
          rsorted.data[i] = gsorted.data[i] = bsorted.data[i] = 0;
      for(i=0; i<histogramsize; i++) 
      {   histogram.p[0][i] = 0;
          histogram.p[1][i] = 0;
          histogram.p[2][i] = 0;
      }
  }

  //// Obtain histogram
  rtot = gtot = btot = 0;
  for(j=0; j<ysize; j++)
  for(i=0; i<xsize; i++)
  { 
      if(gray)
      {   value = get2graypixels(i,j,ino,bpp);
          freq.data[value]++;
      }else
      {   get2colorpixels(i,j,ino,bpp,rr,gg,bb);
          histogram.p[0][rr]++;
          histogram.p[1][gg]++;
          histogram.p[2][bb]++;
          rtot += rr/256;
          gtot += gg/256;
          btot += bb/256;
      }
  }

  rcontrast = (double)rtot / (double)size;
  gcontrast = (double)gtot / (double)size;
  bcontrast = (double)btot / (double)size;
  rgbcontrast = (rcontrast + gcontrast + bcontrast)/3;
  rcontrast /= rgbcontrast;
  gcontrast /= rgbcontrast;
  bcontrast /= rgbcontrast;


  //// Sort pixels
  //// Cumulative freq distrib, makes freq the index of sorted pixels
  if(gray)
  {    for(j=1; j<maxpix; j++) freq.data[j] += freq.data[j-1]; 
  }else
  {    for(j=1; j<maxpix; j++) 
       {   histogram.p[0][j] += histogram.p[0][j-1]; 
           histogram.p[1][j] += histogram.p[1][j-1]; 
           histogram.p[2][j] += histogram.p[2][j-1]; 
       }
  }

  //// Sort the pixels of ino in increasing order of their gray values.
  //// sorted pix no.       = sorted.data[0..size]
  //// color of sorted pix. = pixelat(z[ino].image_1d[b*sorted.data[0..size]], bpp) 
  ////                           (pixels are not moved)

  count = rcount = gcount = bcount = 0;
  for(j=0; j<ysize; j++)
  for(i=0; i<xsize; i++)
  {   
      if(gray)
      {   value = get2graypixels(i,j,ino,bpp);
          sorted.data[freq.data[value]] = count++;
          freq.data[value]--;
      }else
      {   get2colorpixels(i,j,ino,bpp,rr,gg,bb);
          rsorted.data[histogram.p[0][rr]] = rcount++;
          gsorted.data[histogram.p[1][gg]] = gcount++;
          bsorted.data[histogram.p[2][bb]] = bcount++;
          histogram.p[0][rr]--;
          histogram.p[1][gg]--;
          histogram.p[2][bb]--;
      }
  }

  //// Start at pix val = 0, grabbing pixels as needed
  if(gray)
  {   for(k=0; k<size; k++)
      {   ii = sorted.data[k];          
          color = int((double)k / pixperbin);
          putpixelbytes(z[ino].image_1d + ii*b, color, 1, bpp, 1);
      }
  }else
  {   for(k=0; k<size; k++)
      {   rr = rsorted.data[k];
          gg = gsorted.data[k];
          bb = bsorted.data[k];
          color = int((double)k / pixperbin);

          if(COLORS_EQUAL)
          {   rvalue = min(255, cint(color * rcontrast));
              gvalue = min(255, cint(color * gcontrast));
              bvalue = min(255, cint(color * bcontrast));
          }else
              rvalue = gvalue = bvalue = color;

          // Should work on all endians
          value = RGBvalue(rvalue, gvalue, bvalue, bpp); 
          g.wantr=1; g.wantg=0; g.wantb=0;
          putpixelbytes(z[ino].image_1d + rr*b, value, 1, bpp, 1);

          g.wantr=0; g.wantg=1; g.wantb=0;
          putpixelbytes(z[ino].image_1d + gg*b, value, 1, bpp, 1);

          g.wantr=0; g.wantg=0; g.wantb=1;
          putpixelbytes(z[ino].image_1d + bb*b, value, 1, bpp, 1);

          // May be faster but only works on Intel
          // z[ino].image_1d[rr*3+2] = k;
          // z[ino].image_1d[gg*3+1] = k;
          // z[ino].image_1d[bb*3+0] = k;
      }
  }

  g.wantr = owantr;
  g.wantg = owantg;
  g.wantb = owantb;

  if(!gray) 
  {    change_image_depth(ino,obpp,0); 
       rebuild_display(ino);
  }  
  repair(ino);
  redraw(ino);
  z[ino].touched = 1;
}


                                                                                                                                                                                                                                                                                                                                                                                          