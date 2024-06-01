//--------------------------------------------------------------------------//
// xmtnimage35.cc                                                           //
// Latest revision: 06-10-2005                                              //
// Copyright (C) 2005 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
// brightness, contrast                                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
extern PrinterStruct printer;
static char notcolor[] = "Cannot change RGB of grayscale image\nConvert image to color first"; 
static char notbw[] = "Cannot change grayscale of RGB image\nConvert image to grayscale first"; 

//-------------------------------------------------------------------------//
// changecontrast2                                                         //
//-------------------------------------------------------------------------//
void changecontrast2(int noofargs, char **arg)
{
   if(noofargs>0){ changecontrast(noofargs, arg); return; }
   int i,k;
   char title[128] = "Color";
   //// these are deleted in graphcancelcb
   array<int> ydata(256,3);   
   array<double> xdata(256);
   array<char> ytitle(256,3);
   for(k=0; k<3; k++) ytitle.p[k] = new char[256];
   for(i=0; i<256; i++)
   {   ydata.p[0][i] = z[ci].red_brightness[i]; 
       ydata.p[1][i] = z[ci].green_brightness[i]; 
       ydata.p[2][i] = z[ci].blue_brightness[i]; 
       xdata.data[i] = i;
   } 
   strcpy(ytitle.p[0]," Red");
   strcpy(ytitle.p[1]," Gren");
   strcpy(ytitle.p[2]," Blue");
   plot(title, 
       (char*)"Contrast", 
       ytitle.p, 
       xdata.data, 
       ydata.p, 
       256, 
       3, 
       BEZIER, 
       0, 
       255, 
       NULL, 
       null,             // f1
       null,             // f2
       image_bounds,     // f3
       null,             // f4
       changecontrastcb, // f5
       14);
}


//-------------------------------------------------------------------------//
// changegraycontrast                                                      //
//-------------------------------------------------------------------------//
void changegraycontrast(int noofargs, char **arg)
{
   if(noofargs>0){ changecontrast(noofargs, arg); return; }
   int i,k;
   char title[128] = "Grayscale";
   //// these are deleted in graphcancelcb
   static array<int> ydata(256,1);   
   static array<double> xdata(256);
   static array<char> ytitle(256,1);
   for(k=0; k<1; k++) ytitle.p[k] = new char[256];
   for(i=0; i<256; i++)
   {   ydata.p[0][i] = z[ci].gray_brightness[i]; 
       xdata.data[i] = i;
   } 
   strcpy(ytitle.p[0]," Grayscale");
   plot(title, 
       (char*)"Grayscale", 
       ytitle.p, 
       xdata.data, 
       ydata.p, 
       256, 
       1, 
       BEZIER, 
       0, 
       255, 
       NULL, 
       null,                 // f1
       null,                 // f2
       image_bounds,         // f3
       null,                 // f4
       changegraycontrastcb, // f5
       14);
}


//-------------------------------------------------------------------------//
// change_intensity                                                        //
//-------------------------------------------------------------------------//
void change_intensity(int noofargs, char **arg)
{
   int k;
   if(noofargs>0)
   {    noofargs = 6;
        strcpy(arg[6], arg[1]);
        for(k=1;k<6;k++) strcpy(arg[k], "100");
        changecontrast(noofargs, arg); 
   }else message("Intensity increment not specified");
}
  

//-------------------------------------------------------------------------//
// lighten                                                                 //
//-------------------------------------------------------------------------//
void lighten(int noofargs, char **arg)
{
   int k;
   if(noofargs>0)
   {    noofargs = 6;
        strcpy(arg[6], arg[1]);
        for(k=1;k<6;k++) strcpy(arg[k], "0");
        changebrightness(noofargs, arg); 
   }else message("Brightening increment not specified");
}


//-------------------------------------------------------------------------//
// darken                                                                  //
//-------------------------------------------------------------------------//
void darken(int noofargs, char **arg)
{
   int k;
   if(noofargs>0)
   {    noofargs = 6;
        sprintf(arg[6], "%d", -atoi(arg[1]));
        for(k=1; k<6; k++) strcpy(arg[k], "0");
        changebrightness(noofargs, arg); 
   }else message("Darkening increment not specified");
}

//--------------------------------------------------------------------------//
// image_stats                                                              //
//--------------------------------------------------------------------------//
void image_stats(int x1, int y1, int x2, int y2, int &ino, int &bpp, int &ymin, 
     int &ymax, int &rmin, int &gmin, int &bmin, int &rmax, int &gmax, int &bmax)
{
  int f,i,j,ii,jj,rr,gg,bb,value=0;
  ymax = rmax = gmax = bmax = 0; 
  ymin = rmin = gmin = bmin = MAXINT; 
  for(j=y1;j<=y2;j++)
  for(i=x1;i<=x2;i++)
  {   
     if(!g.selected_is_square && !inside_irregular_region(i,j)) continue;
     ino = whichimage(i, j, bpp);
     f = z[ino].cf;
     jj = j - z[ino].ypos;
     ii = g.off[bpp] * (i - z[ino].xpos);
     value = pixelat(z[ino].image[f][jj]+ii, bpp);
     valuetoRGB(value,rr,gg,bb,bpp);
     ymax = max(ymax, value);
     ymin = min(ymin, value);
     rmax = max(rmax, rr);
     rmin = min(rmin, rr);
     gmax = max(gmax, gg);
     gmin = min(gmin, gg);
     bmax = max(bmax, bb);
     bmin = min(bmin, bb);
  }
}


//--------------------------------------------------------------------------//
// image_bounds - get data for draw_graph                                   //
//--------------------------------------------------------------------------//
void image_bounds(void *p, int n1, int n2)
{
   n1=n1;n2=n2;
   PlotData *pd = (PlotData *)p;
   int x1,y1,x2,y2,ino,bpp,ymin,ymax,rmin,gmin,bmin,rmax,gmax,bmax;
   x1=g.ulx; x2=g.lrx; y1=g.uly; y2=g.lry;
   image_stats(x1,y1,x2,y2,ino,bpp,ymin,ymax,rmin,gmin,bmin,rmax,gmax,bmax);
   //// In anticipation of converting image to 24 bpp
   if(bpp==8){ bpp=24; rmin*=4; rmax=255; gmin*=4; gmax*=4; bmin*=4; bmax*=4; }
   switch(pd->focus)
   {    case 0:  pd->ymin=(double)rmin; 
                 pd->ymax=(double)rmax;
                 pd->ulimit=g.maxred[bpp];
                 break;
        case 1:  pd->ymin=(double)gmin; 
                 pd->ymax=(double)gmax;
                 pd->ulimit=g.maxgreen[bpp];
                 break;
        case 2:  pd->ymin=(double)bmin; 
                 pd->ymax=(double)bmax; 
                 pd->ulimit=g.maxblue[bpp];
                 break;
   }
}


//--------------------------------------------------------------------------//
// changecontrastcb                                                         //
//--------------------------------------------------------------------------//
void changecontrastcb(void *p, int n1, int n2)
{
  n1 = n1; n2 = n2;
  int f,focus,hitgray=0,i,j,k,ino,bpp,rr,gg,bb,value,rr2,gg2,bb2;
  int oinmenu=g.inmenu;
  PlotData *pd = (PlotData*)p;
  int x1=g.ulx;
  int x2=g.lrx;
  int y1=g.uly;
  int y2=g.lry;
  g.inmenu = 0;
  printstatus(CALCULATING);
  focus = pd->focus;
  ino = g.selectedimage;
  printstatus(BUSY);

  static int hit = 0;
  static int ofocus = 0;
  static double olddata[256];
  int changed = 0;

  //// Make sure data is changed before starting time-consuming redraw
  if(!hit)
       for(k=0;k<256;k++) olddata[k]=0.0;
  for(k=0;k<256;k++)
       if(pd->data[focus][k] != olddata[k]){ changed++; break; }
  for(k=0;k<256;k++)
       olddata[k] = pd->data[focus][k];
  if(focus != ofocus) changed++;
  ofocus = focus;

  //// Return after first box drawn, nothing changed
  if(!changed ){ g.inmenu = oinmenu; return; } 
  hit = 1;
  if(g.selectedimage>0) restoreimage(0); // Don't restore colortype

  //// If in 8bpp mode, just change colormap
  if(g.selectedimage!=-1 && z[g.selectedimage].bpp==8)
  {  
      if(z[ino].colortype == GRAY) hitgray=1;
      else 
      {   for(k=0;k<256;k++)
          {   z[ino].palette[k].red   = (int)pd->data[0][k]/4; 
              z[ino].palette[k].green = (int)pd->data[1][k]/4; 
              z[ino].palette[k].blue  = (int)pd->data[2][k]/4;
          }
          z[ino].hit = 1;
          setpalette(z[ino].palette);
      }
  }else
  {
      for(k=0;k<MAXIMAGES;k++) z[k].hit=0;
      for(j=y1;j<=y2;j++)
      for(i=x1;i<=x2;i++)
      {   
         if(!g.selected_is_square && !inside_irregular_region(i,j)) continue;
         value = readpixelonimage(i,j,bpp,ino);
         if(ino<0) continue;
         if(z[ino].colortype == GRAY) { hitgray=1; continue; }
         f = z[ino].cf;

         if(ino>=0 && z[ino].hit==0 && z[ino].colortype==INDEXED) 
         {   z[ino].hit=2;    // Must be set before change_image_depth                   
             if(change_image_depth(ino,24,0)!=OK) 
             {   message("Insufficient image buffers available",ERROR);
                 g.getout=1; 
                 return;
             }
             rebuild_display(ino);           
             bpp=24;
             
         }

         if(!z[ino].hit) z[ino].hit=1;
         valuetoRGB(value,rr,gg,bb,g.bitsperpixel); 

         rr2 = (int)pd->data[0][rr];
         gg2 = (int)pd->data[1][gg];
         bb2 = (int)pd->data[2][bb];
         if(rr2 == rr && gg2 == gg && bb2 == bb) continue;
         rr = max(0, min(rr2, (int)g.maxred[g.bitsperpixel]));
         gg = max(0, min(gg2, (int)g.maxgreen[g.bitsperpixel]));
         bb = max(0, min(bb2, (int)g.maxblue[g.bitsperpixel]));

         value = RGBvalue(rr,gg,bb,g.bitsperpixel);
         value = convertpixel(value, g.bitsperpixel, bpp,1);
         setpixelonimage(i,j,value,SET);
         if(ino>=0 && (!z[ino].hit)) z[ino].hit=1;
      }
      for(k=0; k<g.image_count; k++) 
      {   if(z[k].hit==2)
          {   z[k].hit=1;
              change_image_depth(k,8,0);
          }
      }
  }
  for(k=0;k<256;k++)
  {    z[ino].red_brightness[k]   = (int)pd->data[0][k];
       z[ino].green_brightness[k] = (int)pd->data[1][k];
       z[ino].blue_brightness[k]  = (int)pd->data[2][k];
  }
  if(hitgray) message(notcolor, WARNING);
  g.inmenu = oinmenu;  
  for(k=0;k<MAXIMAGES;k++) if(z[k].hit) { rebuild_display(k); redraw(k); z[k].hit=0; }
  printstatus(NORMAL);
  rebuild_display(ino);
  redraw(ino);
}


//-------------------------------------------------------------------------//
// addvalue                                                                //
// Change color/intensity of selected region by adding a value.            //
//-------------------------------------------------------------------------//
void addvalue(int noofargs, char **arg)
{
    int dr=g.raddvalue, dg=g.gaddvalue, db=g.baddvalue;
    int darker=g.vaddvalue;
    if(memorylessthan(16384)){ message(g.nomemory,ERROR); return; }
    int k,bpp=0,maxbpp=0,gotbw=0,gotcolor=0,white,maxwhite=0,black,helptopic=14;

    static clickboxinfo* item;
    g.getout=0;
    char temp[128];
    int x1=g.selected_ulx;
    int x2=g.selected_lrx;
    int y1=g.selected_uly;
    int y2=g.selected_lry;

    for(k=0; k<g.image_count; k++) z[k].hit=0;
     
    pointinfo(x1,y1,bpp,gotcolor,gotbw,white,black);
    maxbpp=max(bpp,maxbpp);
    maxwhite=max(white,maxwhite);
    pointinfo(x1,y2,bpp,gotcolor,gotbw,white,black);
    maxbpp=max(bpp,maxbpp);
    maxwhite=max(white,maxwhite);
    pointinfo(x2,y1,bpp,gotcolor,gotbw,white,black);
    maxbpp=max(bpp,maxbpp);
    maxwhite=max(white,maxwhite);
    pointinfo(x2,y2,bpp,gotcolor,gotbw,white,black);
    maxbpp=max(bpp,maxbpp);
    maxwhite=max(white,maxwhite);
    pointinfo((x1+x2)/2,(y1+y2)/2,bpp,gotcolor,gotbw,white,black);
    maxbpp=max(bpp,maxbpp);
    maxwhite=max(white,maxwhite);

    if(noofargs)
    {  if(noofargs>=1) darker=atoi(arg[1]);
       if(noofargs>=2) dr=atoi(arg[2]);
       if(noofargs>=3) dg=atoi(arg[3]);
       if(noofargs>=4) db=atoi(arg[4]);
       clickboxinfo c[3];
       c[0].noofbuttons=3;
       c[0].answer = dr;
       c[1].answer = dg;
       c[2].answer = db;
       addvalueok(c);
    }else
    {  if(gotbw)
       {  strcpy(temp,"Change pixel value");
          if(gotcolor) strcat(temp," in mono. part");
          clickbox(temp, 14, &darker, -maxwhite, maxwhite, null, 
               addvalueok, null, NULL, NULL, helptopic);
       }
       if(gotcolor)
       {   
          item = new clickboxinfo[3];
          item[0].title = new char[128];        
          item[0].wantpreview=0;
          strcpy(item[0].title,"Red");
          item[0].startval = dr;
          item[0].minval = -g.maxred[g.bitsperpixel];
          item[0].maxval = g.maxred[g.bitsperpixel];
          item[0].type = VALSLIDER;
          item[0].wantdragcb = 0;
          item[0].answers = new int[10]; // Must be 10 for multiclickbox cb
          item[0].form = NULL;
          item[0].path = NULL;

          item[1].title = new char[128];        
          strcpy(item[1].title,"Green");
          item[1].startval = dg;
          item[1].minval = -g.maxgreen[g.bitsperpixel];
          item[1].maxval = g.maxgreen[g.bitsperpixel];
          item[1].type = VALSLIDER;
          item[1].wantdragcb = 0;
          item[1].form = NULL;
          item[1].path = NULL;

          item[2].title = new char[128];        
          strcpy(item[2].title,"Blue");
          item[2].startval = db;
          item[2].minval = -g.maxblue[g.bitsperpixel];
          item[2].maxval = g.maxblue[g.bitsperpixel];
          item[2].type = VALSLIDER;
          item[2].wantdragcb = 0;
          item[2].form = NULL;
          item[2].path = NULL;

          strcpy(temp,"Pixel value adjustment");
          if(gotbw) strcat(temp," in color part");

          item[0].ino = ci;
          item[0].noofbuttons = 3;
          item[0].f1 = null;
          item[0].f2 = null;
          item[0].f3 = null;
          item[0].f4 = null;
          item[0].f5 = addvalueok;
          item[0].f6 = addvaluefinish;
          item[0].f7 = null;
          item[0].f8 = null;
          multiclickbox(temp, 3, item, null, helptopic);
       }
    }
}


//-------------------------------------------------------------------------//
// addvalueok                                                              //
//-------------------------------------------------------------------------//
void addvalueok(clickboxinfo *c)
{
   int dr=0,dg=0,db=0,darker=0,f=0,ino,j,k,i,rr,gg,bb,bpp=0,obpp=0,
       value,x1,x2,y1,y2;
   dr = c[0].answer;
   darker = dr;
   if(c[0].noofbuttons >= 2) dg = c[1].answer;
   if(c[0].noofbuttons >= 3) db = c[2].answer;
   drawselectbox(OFF);
   printstatus(BUSY);
   x1 = g.selected_ulx;  
   y1 = g.selected_uly;
   x2 = g.selected_lrx;  
   y2 = g.selected_lry;
   printstatus(BUSY);

   g.raddvalue = dr;
   g.gaddvalue = dg;
   g.baddvalue = db;
   g.vaddvalue = darker;

   if(!g.getout)
   {  for(j=y1; j<=y2; j++)
      {  if(keyhit())if(getcharacter()==27)break;  
         for(i=x1; i<=x2; i++)
         {   
             if(!g.selected_is_square && !inside_irregular_region(i,j)) continue;
             value = readpixelonimage(i,j,bpp,ino);
             if(ino>=0)
             {  if(z[ino].hit==0 && z[ino].colortype==INDEXED) 
                {  z[ino].hit=2;  // Must be set before change_image_depth
                   if(change_image_depth(ino, 24, 0)!=OK) 
                   {  message("Insufficient image buffers available",ERROR);
                      g.getout=1; 
                      break;
                   }
                   rebuild_display(ino);   // create new img buffer
                   bpp = 24;
                 }
             }
                 
             ////  Crossing boundary to image with different bpp
             if(bpp!=obpp)       
             {  if((ino>=0)&&(bpp==8))   
                {  obpp=bpp; 
                   switch_palette(ino);
                }
             }   
             if(ino>=0) f=z[ino].cf;
             if(z[ino].hit==0) z[ino].hit=1;
             if((bpp==8)||(ino>=0 && z[ino].colortype==GRAY))
             {  value += darker;
                value = min((uint)g.maxvalue[bpp],(uint)max(0,value));
             }else
             {  valuetoRGB(value,rr,gg,bb,bpp);
                rr = max(0,min((int)g.maxred[bpp],( rr+dr )));
                gg = max(0,min((int)g.maxgreen[bpp], ( gg+dg )));
                bb = max(0,min((int)g.maxblue[bpp], ( bb+db )));
                value = RGBvalue(rr,gg,bb,bpp);
             }
             setpixelonimage(i,j,value,SET,bpp);
         }
         if(g.getout)break;
      }
           
      ////  Re-palettize any color images touched if in 8-bpp screen mode
      for(k=0; k<g.image_count; k++) 
      {  if(z[k].hit==2) 
         {     z[k].hit=1;
               change_image_depth(k,8,0);
         }
         if(z[k].hit) repair(k);
         z[k].hit = 0;
      }
      g.getout=0;
   }  
   printstatus(NORMAL);
}


//-------------------------------------------------------------------------//
// addvaluefinish                                                          //
//-------------------------------------------------------------------------//
void addvaluefinish(clickboxinfo *c)
{
  delete[] c[0].answers;
  delete[] c[0].title;
  delete[] c[1].title;
  delete[] c[2].title;
  delete[] c;                                   
}


//-------------------------------------------------------------------------//
// changebrightness                                                        //
//-------------------------------------------------------------------------//
void changebrightness(int noofargs, char **arg)
{
  if(memorylessthan(16384)){ message(g.nomemory,ERROR); return; }
  int helptopic=14;
  static clickboxinfo* item;
  int dr=g.radjust, dg=g.gadjust, db=g.badjust, 
      dh=g.hadjust, ds=g.sadjust, dv=g.vadjust,
      di=g.iadjust;
  int value[10] = { 0,0,0,0,0,0,0,0,0,0 };
  int bpp = g.bitsperpixel;

  if(ci>=0)
  {   switch_palette(ci);
      memcpy(z[ci].opalette,z[ci].palette,768); 
      bpp = z[ci].bpp;
  }else
      memcpy(g.palette,g.b_palette,768); 

  g.getout=0;
  if(noofargs>0)          // Macro
  {    if(noofargs>=1) value[0]=atoi(arg[1]);  // r
       if(noofargs>=2) value[1]=atoi(arg[2]);  // g
       if(noofargs>=3) value[2]=atoi(arg[3]);  // b
       if(noofargs>=4) value[3]=atoi(arg[4]);  // h
       if(noofargs>=5) value[4]=atoi(arg[5]);  // s
       if(noofargs>=6) value[5]=atoi(arg[6]);  // v
       if(noofargs>=7) value[6]=atoi(arg[7]);  // i
       value[6]=1;        // make permanent
       addpalettecolors(value);   
       repair(ci);
  }else
  {    item = new clickboxinfo[7];
       item[0].wantpreview=0;
       item[0].title = new char[128];
       strcpy(item[0].title,"Red");
       item[0].startval = dr;
       item[0].minval = -g.maxred[g.bitsperpixel];
       item[0].maxval = g.maxred[g.bitsperpixel];
       item[0].type = 2;
       item[0].wantdragcb = 0;
       item[0].answers = new int[10];   // Must have 10 for addpalettecolors()
       item[0].answers[0]=0;
       item[0].answers[1]=0;
       item[0].answers[2]=0;
       item[0].answers[3]=0;
       item[0].answers[4]=0;
       item[0].answers[5]=0;
       item[0].answers[6]=0;
       item[0].form = NULL;
       item[0].path = NULL;

       item[1].title = new char[128];
       strcpy(item[1].title,"Green");
       item[1].startval = dg;
       item[1].minval = -g.maxgreen[g.bitsperpixel];
       item[1].maxval = g.maxgreen[g.bitsperpixel];
       item[1].type = 2;
       item[1].wantdragcb = 0;
       item[1].answers = item[0].answers;
       item[1].form = NULL;
       item[1].path = NULL;

       item[2].title = new char[128];
       strcpy(item[2].title,"Blue");
       item[2].startval = db;
       item[2].minval = -g.maxblue[g.bitsperpixel];
       item[2].maxval = g.maxblue[g.bitsperpixel];
       item[2].type = 2;
       item[2].wantdragcb = 0;
       item[2].answers = item[0].answers;
       item[2].form = NULL;
       item[2].path = NULL;

       item[3].title = new char[128];
       strcpy(item[3].title,"Hue");
       item[3].startval = dh;
       item[3].minval = -360;
       item[3].maxval = 360;
       item[3].type = 2;
       item[3].wantdragcb = 0;
       item[3].answers = item[0].answers;
       item[3].form = NULL;
       item[3].path = NULL;

       item[4].title = new char[128];
       strcpy(item[4].title,"Saturation");
       item[4].startval = ds;
       item[4].minval = -g.maxblue[g.bitsperpixel];
       item[4].maxval = g.maxblue[g.bitsperpixel];
       item[4].type = 2;
       item[4].wantdragcb = 0;
       item[4].answers = item[0].answers;
       item[4].form = NULL;
       item[4].path = NULL;

       item[5].title = new char[128];
       strcpy(item[5].title,"Value");
       item[5].startval = dv;
       if(g.bitsperpixel==8)
       { item[5].minval = -63;
         item[5].maxval = 63;
       }else
       { item[5].minval = -g.maxgray[g.bitsperpixel];
         item[5].maxval = g.maxgray[g.bitsperpixel];
       }
       item[5].type = 2;
       item[5].wantdragcb = 0;
       item[5].answers = item[0].answers;
       item[5].form = NULL;
       item[5].path = NULL;

       item[6].title = new char[128];
       strcpy(item[6].title,"Intensity");
       item[6].startval = di;
       item[6].minval = -(int)g.maxvalue[bpp];
       item[6].maxval = (int)g.maxvalue[bpp];
       item[6].type = 2;
       item[6].wantdragcb = 0;
       item[6].answers = item[0].answers;
       item[6].form = NULL;
       item[6].path = NULL;

       item[0].ino = ci;
       item[0].noofbuttons = 7;
       item[0].f1 = null;
       item[0].f2 = null;
       item[0].f3 = null;
       item[0].f4 = null;
       item[0].f5 = changebrightnessok;
       item[0].f6 = changebrightnessfinish;
       item[0].f7 = null;
       item[0].f8 = null;
       multiclickbox("Color adjustment", 7, item, addpalettecolors, helptopic);
  }
}


//-------------------------------------------------------------------------//
// changebrightnessok                                                      //
//-------------------------------------------------------------------------//
void changebrightnessok(clickboxinfo *c)
{
  int *value = c[0].answers;
  int ino = c[0].ino;
  g.radjust = value[0] = c[0].answer;
  g.gadjust = value[1] = c[1].answer;
  g.badjust = value[2] = c[2].answer;
  g.hadjust = value[3] = c[3].answer;
  g.sadjust = value[4] = c[4].answer;
  g.vadjust = value[5] = c[5].answer;
  g.iadjust = value[6] = c[6].answer;
  value[7] = 1;  // Make palette change permanent 
  drawselectbox(OFF);
  addpalettecolors(value);   
  if(ino>=0)
  {   memcpy(z[ino].palette,g.palette,768); 
      setpalette(z[ino].palette);
      rebuild_display(ino);
      redraw(ino);
      z[ino].touched = 1;
  }else memcpy(g.b_palette, g.palette, 768); 
  g.getout=0;
  value[7]=  0;   // Make palette change temporary (for slider)
}


//-------------------------------------------------------------------------//
// changebrightnessfinish                                                  //
//-------------------------------------------------------------------------//
void changebrightnessfinish(clickboxinfo *c)
{
  int k;
  delete[] c[0].answers;
  for(k=0;k<=6;k++) delete[] c[k].title; 
  delete[] c;          
}


//--------------------------------------------------------------------------//
// addpalettecolors - add r,g,b,& i in palette by dr,dg,db,di               //
// d[] is value to add to 0=r 1=g 2=b 3=h 4=s 5=v.                          //
// If value[6] is 1, the change is permanent, otherwise it is temporary.    //
// Compatible with multiclickbox                                            //
//--------------------------------------------------------------------------//
void addpalettecolors(int d[10])
{ 
   RGB opal[256];
   int crud=256,hitgray=0,f=0,ino=ci,i,i2,j,k,bpp,value,rr,gg,bb,obpp=0,noscreen=1,xx,yy;
   int x1,y1,x2,y2;
   int dr,dg,db,dfac=1;
   int dh,ds,dv,di,hh,ss,vv;
   printstatus(BUSY);

   ////  If color, changing 'intensity' should change rg&b
   dr = d[0];
   dg = d[1];
   db = d[2];
   dh = d[3];
   ds = d[4];
   dv = d[5];
   di = d[6];

   for(k=0; k<g.image_count; k++) z[k].hit=0;
   
   if(d[7]==0)                          // Fast as possible & temporary
   {    if(g.bitsperpixel==8)           // If 8bpp, adjust palette only
        {
            memcpy(opal,g.palette,768);
            for(j=0;j<256;j++) 
            {   g.palette[j].red   = max(0,min(63,(uchar)(dr + g.palette[j].red))); 
                g.palette[j].green = max(0,min(63,(uchar)(dg + g.palette[j].green)));
                g.palette[j].blue  = max(0,min(63,(uchar)(db + g.palette[j].blue)));
            }
            setpalette(g.palette);  
            memcpy(g.palette,opal,768); // restore original palette 
        }else                           // Otherwise, adjust z.img
        {
            ino = ci;
            if(ino>=0) f=z[ino].cf;
            if(g.selectedimage>=0 && ino>=0 && z[ino].colortype==GRAY)  // Entire image
            {    bpp = z[ino].bpp;
                 dr=dv;dg=dv;db=dv; 
                 int save = z[ino].gray[0];
                 int save2 = z[ino].gray[1];
                 z[ino].gray[0] = (int)(z[ino].gray[0] - di); 
                 z[ino].gray[1] = (int)(z[ino].gray[1] - di); 
                 z[ino].hitgray = 1;  
                 rebuild_display(ino);
                 z[ino].gray[0] =save;
                 z[ino].gray[1] =save2;
            }else                                                      // Part of image
            {   for(j=g.selected_uly;j<=g.selected_lry;j++)
                for(i=g.selected_ulx;i<=g.selected_lrx;i++)
                {                  
                    if(!g.selected_is_square && !inside_irregular_region(i,j)) continue;
                    value = readpixel(i,j);
                    ino = whichimage(i,j,bpp);
                    valuetoRGB(value,rr,gg,bb,g.bitsperpixel);
                    rr = max(0,min((int)g.maxred[g.bitsperpixel], (int)(rr+dr)));
                    gg = max(0,min((int)g.maxgreen[g.bitsperpixel], (int)(gg+dg)));
                    bb = max(0,min((int)g.maxblue[g.bitsperpixel], (int)(bb+db)));
                    if(dh || ds || dv)
                    {    RGBtoHSV(rr, gg, bb, hh, ss, vv, g.bitsperpixel);
                         hh = max(0,min(360, hh+dh));
                         ss = max(0,min(g.maxgray[g.bitsperpixel],  ss+ds));
                         vv = max(0,min(g.maxgray[g.bitsperpixel],  vv+dv));
                         HSVtoRGB(hh, ss, vv, rr, gg, bb, g.bitsperpixel);
                    }
                    value = RGBvalue(rr,gg,bb,g.bitsperpixel);
                    setpixel(i,j,value,SET);
                }
            }
        }
        redraw(ino);
        redrawscreen();

   }else                                // Slow but accurate
   {    
        ino = ci;
        if(g.selectedimage>=0 && ino>=0) // Entire image
        {   bpp = z[ino].bpp;
            f = z[ino].cf;
            if(z[ino].colortype==GRAY)
            {  
                hitgray = 1;
                for(j=0; j<z[ino].ysize; j++)
                for(i=0,i2=0; i<z[ino].xsize; i++,i2+=g.off[bpp])
                {   
                    value = pixelat(z[ino].image[f][j]+i2,bpp);
                    value = min((int)g.maxvalue[bpp], (int)max(0,(di + value)));
                    putpixelbytes(z[ino].image[f][j]+i2, value, 1, bpp, 1);
                    ////  This only needs to be done for grayscale
                    if(z[ino].floatexists || z[ino].waveletexists)
                    {   xx = i + z[ino].xpos;
                        yy = j + z[ino].ypos;
                        if(z[ino].floatexists) setfloatpixel(xx,yy,value,ino);
                        if(z[ino].waveletexists) setwaveletpixel(xx,yy,value,ino);
                    }
                }
            }else
            {   //// If part of an indexed image, must convert to color
                //// then regenerate colormap

                convertRGBpixel(dr,dg,db,24,bpp);
                convertRGBpixel(crud,ds,dv,24,bpp);
                if(ino>=0 && z[ino].hit==0 && z[ino].colortype==INDEXED) 
                {    z[ino].hit=2;    // Must be set before change_image_depth
                     dfac = 4;
                     if(change_image_depth(ino,24,0)!=OK) 
                     {   message("Insufficient image buffers available",ERROR);
                         g.getout=1; 
                         return;
                     }
                     bpp=24;
                }
                for(j=0;j<z[ino].ysize;j++)
                for(i=0,i2=0;i<z[ino].xsize;i++,i2+=g.off[bpp])
                { 
                    RGBat(z[ino].image[f][j]+i2, bpp, rr, gg, bb);
                    rr = max(0,min(g.maxred[bpp],   rr+dr*dfac));
                    gg = max(0,min(g.maxgreen[bpp], gg+dg*dfac));
                    bb = max(0,min(g.maxblue[bpp],  bb+db*dfac));
                    if(dh || ds || dv)
                    {    RGBtoHSV(rr, gg, bb, hh, ss, vv, bpp);
                         hh = max(0,min(360, hh+dh));
                         ss = max(0,min(g.maxgray[bpp],  ss+ds));
                         vv = max(0,min(g.maxgray[bpp],  vv+dv*dfac));
                         HSVtoRGB(hh, ss, vv, rr, gg, bb, bpp);
                    }
                    putRGBbytes(z[ino].image[f][j]+i2, rr, gg, bb, bpp);
                }
                for(k=0; k<g.image_count; k++) 
                {   if(z[k].hit==2)
                    {     z[k].hit=1;
                          change_image_depth(k,8,0);
                    }
                }
            }
            redraw(ino);
        }else                 // Selected region only
        {
            x1 = g.ulx;
            y1 = g.uly;
            x2 = g.lrx;
            y2 = g.lry;
            for(j=y1;j<=y2;j++)
            for(i=x1;i<=x2;i++)
            {   
                if(!g.selected_is_square && !inside_irregular_region(i,j)) continue;
                ino = whichimage(i,j,bpp);
                ////  Crossing boundary to image with different bpp
                     
                if(ino>=0 && bpp!=obpp && bpp==8)
                {    obpp=bpp; 
                     switch_palette(ino);
                }   
                    
                //// If part of an indexed image, must convert to color
                //// then regenerate colormap
                        
                if(ino>=0 && z[ino].hit==0 && z[ino].colortype==INDEXED) 
                {    
                     z[ino].hit=2;    // Must be set before change_image_depth
                     if(change_image_depth(ino,24,0)==OK) bpp=24;
                     else
                     {   message("Insufficient image buffers available",ERROR);
                         g.getout=1; 
                         break;
                     }
                     rebuild_display(ino);
                }
                noscreen=1;
                  
                ////  Must convert to screen bits/pixel so the increment is the
                ////  same as the preview above. Otherwise the increase in brightness
                ////  could be doubled if, for example, a 16 bpp image was adjusted 
                ////  at 24 bpp screen depth.
                
                if(ino>=0 && z[ino].colortype==GRAY)
                {  
                     value = readpixelonimage(i,j,bpp,ino);
                     value = min((int)g.maxvalue[bpp], (int)(value+di));
                     hitgray = 1;
                     setpixelonimage(i,j,value,SET);
                }else
                {   
                     value = readpixelonimage(i,j,bpp,ino);
                     valuetoRGB(value,rr,gg,bb,bpp);
                     rr = max(0,min((int)g.maxred[g.bitsperpixel], (int)(rr+dr)));
                     gg = max(0,min((int)g.maxgreen[g.bitsperpixel], (int)(gg+dg)));
                     bb = max(0,min((int)g.maxblue[g.bitsperpixel], (int)(bb+db)));
                     if(dh || ds || dv)
                     {    RGBtoHSV(rr, gg, bb, hh, ss, vv, g.bitsperpixel);
                          hh = max(0,min(360, hh+dh));
                          ss = max(0,min(g.maxgray[g.bitsperpixel],  ss+ds));
                          vv = max(0,min(g.maxgray[g.bitsperpixel],  vv+dv));
                          HSVtoRGB(hh, ss, vv, rr, gg, bb, g.bitsperpixel);
                     }
                     value = RGBvalue(rr,gg,bb,bpp);
                     setpixelonimage(i,j,value,SET);
                }
            }            
           
            ////  Re-palettize any color images touched if in 8-bpp screen mode.
            ////  See invertcolors() in xmtnimage2.cc for an explanation of this code.
            for(k=0; k<g.image_count; k++) 
            {   if(z[k].hit==2) 
                {     z[k].hit=1;
                      change_image_depth(k,8,0);
                }
            }
            for(k=0; k<g.image_count; k++) 
            {  if(z[k].hit)
               {     if(k!=ci) switchto(k);
                     rebuild_display(k); 
                     redraw(k); 
                     if(z[k].colortype==INDEXED) memcpy(z[k].opalette,z[k].palette,768);
               }
            }
        }
    }
    if(hitgray && (d[0] || d[1] || d[2])) message(notcolor, WARNING);
    printstatus(NORMAL);
}


//--------------------------------------------------------------------------//
// changegraycontrastcb                                                     //
//--------------------------------------------------------------------------//
void changegraycontrastcb(void *p, int n1, int n2)
{
  n1=n1; n2=n2;
  int f,i,j,k,ino,bpp,value,index,grayfac;
  double grayfacinv;
  int oinmenu=g.inmenu;
  PlotData *pd = (PlotData*)p;
  int x1=g.ulx;
  int x2=g.lrx;
  int y1=g.uly;
  int y2=g.lry;
  g.inmenu = 0;
  printstatus(CALCULATING);

  static int hit = 0;
  static double olddata[256];
  int changed = 0;

  //// Make sure data is changed before starting time-consuming redraw
  if(!hit)
       for(k=0;k<256;k++) olddata[k]=0.0;
  for(k=0;k<256;k++)
       if(pd->data[0][k] != olddata[k]){ changed++; break; }
  for(k=0;k<256;k++)
       olddata[k] = pd->data[0][k];
  hit = 1;

  //// Return after first box drawn, nothing changed
  if(!changed){ g.inmenu = oinmenu; return; } 
  if(g.selectedimage>0) restoreimage(0); // Don't restore colortype
 
  ino = ci;
  if(!between(ino, 1, g.image_count)){ g.inmenu = oinmenu; return; }
  bpp = z[ino].bpp;
  grayfac = int(+g.maxvalue[bpp] / (double)255);
  grayfacinv = 1.0 / grayfac;     //// scale pixel value to 0-255
  if(z[ino].colortype != GRAY){ message(notbw, WARNING); g.inmenu = oinmenu; return; }
  printstatus(BUSY);

  //// If in 8bpp mode, just change colormap
  for(k=0;k<MAXIMAGES;k++) z[k].hit=0;
  for(j=y1;j<=y2;j++)
  for(i=x1;i<=x2;i++)
  {      if(!g.selected_is_square && !inside_irregular_region(i,j)) continue;
         value = readpixelonimage(i,j,bpp,ino);
         if(ino<0) continue;
         f = z[ino].cf;
         if(!z[ino].hit) z[ino].hit=1;
         index = int(value * grayfacinv);
         if(z[ino].gray_brightness[index] == (int)pd->data[0][index]) continue;
         value = grayfac * (int)pd->data[0][index];
         value = max(0,min(value,(int)g.maxvalue[bpp]));
         setpixelonimage(i,j,value,SET,z[ino].bpp);
         if(ino>=0 && (!z[ino].hit)) z[ino].hit=1;
  }
  for(k=0;k<256;k++)
        z[ino].gray_brightness[k] = (int)pd->data[0][k];

  g.inmenu = oinmenu;  
  for(k=0;k<MAXIMAGES;k++) if(z[k].hit) { rebuild_display(k); redraw(k); z[k].hit=0; }
  printstatus(NORMAL);
}

