//--------------------------------------------------------------------------//
// xmtnimage63.cc                                                           //
// Latest revision: 03-30-2005                                              //
// Copyright (C) 2005 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
extern PrinterStruct printer;
static char notcolor[] = "Cannot change RGB of grayscale image\nConvert image to color first"; 


//-------------------------------------------------------------------------//
// changecontrast                                                          //
//-------------------------------------------------------------------------//
void changecontrast(int noofargs, char **arg)
{
  int dr=g.rcontrast, dg=g.gcontrast, db=g.bcontrast,
      dh=g.hcontrast, ds=g.scontrast, dv=g.vcontrast,
      di=g.icontrast;
  if(memorylessthan(16384)){ message(g.nomemory,ERROR); return; }
  int helptopic=14;
  static clickboxinfo* item;
  static int value[10] = { 100,100,100,100,100,100,100,0,0,0 };
  if(ci>=0)
  {    switch_palette(ci);
       memcpy(z[ci].opalette,z[ci].palette,768); 
  }else
       memcpy(g.palette,g.b_palette,768); 

  g.getout=0;
  if(noofargs>0)          // Macro
  {    if(noofargs>=1) value[0]=atoi(arg[1]);
       if(noofargs>=2) value[1]=atoi(arg[2]);
       if(noofargs>=3) value[2]=atoi(arg[3]);
       if(noofargs>=4) value[3]=atoi(arg[4]);
       if(noofargs>=5) value[4]=atoi(arg[5]);
       if(noofargs>=6) value[5]=atoi(arg[6]);
       if(noofargs>=7) value[6]=atoi(arg[7]);
       value[7]=1;        // make permanent
       multiplypalettecolors(value);   
       repair(ci);
  }else
  {    item = new clickboxinfo[7];
       item[0].title = new char[128];        
       item[0].wantpreview=0;
       strcpy(item[0].title,"Red");
       item[0].startval = dr;
       item[0].done = 0;                // Set to 1 if maximize button is clicked
       item[0].k = 0;                   // which button
       item[0].minval = 0;
       item[0].maxval = 1000;           // maximum percentage factor
       item[0].type = BUTTONVALSLIDER;
       item[0].wantdragcb = 0;
       item[0].answers = new int[10];   // Must have 10 for addpalettecolors()
       item[0].answers[0]=0;            // red contrast
       item[0].answers[1]=0;            // green contrast
       item[0].answers[2]=0;            // blue contrast
       item[0].answers[3]=0;            
       item[0].answers[4]=0;
       item[0].answers[5]=0;            // intensity contrast
       item[0].answers[6]=0;            
       item[0].f1 = NULL;
       item[0].f2 = NULL;
       item[0].f3 = NULL;
       item[0].f4 = maximize_contrast;
       item[0].buttonlabel = new char[64];
       item[0].form = NULL;
       item[0].path = NULL;
       strcpy(item[0].buttonlabel,"Maximize");       
       
       item[1].title = new char[128];        
       strcpy(item[1].title,"Green");
       item[1].startval = dg;
       item[1].done = 0;                // Set to 1 if maximize button is clicked
       item[1].k = 1;                   // which button this is
       item[1].minval = 0;
       item[1].maxval = 1000;
       item[1].type = BUTTONVALSLIDER;
       item[1].wantdragcb = 0;
       item[1].answers = item[0].answers; // Alias all answers to each other
       item[1].f1 = NULL;
       item[1].f2 = NULL;
       item[1].f3 = NULL;
       item[1].f4 = maximize_contrast;
       item[1].buttonlabel = new char[64];
       item[1].form = NULL;
       item[1].path = NULL;
       strcpy(item[1].buttonlabel,"Maximize");       

       item[2].title = new char[128];        
       strcpy(item[2].title,"Blue");
       item[2].startval = db;
       item[2].done = 0;                // Set to 1 if maximize button is clicked
       item[2].k = 2;                   // which button this is
       item[2].minval = 0;
       item[2].maxval = 1000;
       item[2].type = BUTTONVALSLIDER;
       item[2].wantdragcb = 0;
       item[2].answers = item[0].answers; // Alias all answers to each other
       item[2].f1 = NULL;
       item[2].f2 = NULL;
       item[2].f3 = NULL;
       item[2].f4 = maximize_contrast;
       item[2].buttonlabel = new char[64];
       item[2].form = NULL;
       item[2].path = NULL;
       strcpy(item[2].buttonlabel,"Maximize");       

       item[3].title = new char[128];        
       strcpy(item[3].title,"Hue");
       item[3].startval = dh;
       item[3].done = 0;                // Set to 1 if maximize button is clicked
       item[3].k = 3;                   // which button this is
       item[3].minval = 0;
       item[3].maxval = 1000;
       item[3].type = BUTTONVALSLIDER;
       item[3].wantdragcb = 0;
       item[3].answers = item[0].answers; // Alias all answers to each other
       item[3].f1 = NULL;
       item[3].f2 = NULL;
       item[3].f3 = NULL;
       item[3].f4 = maximize_contrast;
       item[3].buttonlabel = new char[64];
       item[3].form = NULL;
       item[3].path = NULL;
       strcpy(item[3].buttonlabel,"Maximize");       

       item[4].title = new char[128];        
       strcpy(item[4].title,"Satur.");
       item[4].startval = ds;
       item[4].done = 0;                // Set to 1 if maximize button is clicked
       item[4].k = 4;                   // which button this is
       item[4].minval = 0;
       item[4].maxval = 1000;
       item[4].type = BUTTONVALSLIDER;
       item[4].wantdragcb = 0;
       item[4].answers = item[0].answers; // Alias all answers to each other
       item[4].f1 = NULL;
       item[4].f2 = NULL;
       item[4].f3 = NULL;
       item[4].f4 = maximize_contrast;
       item[4].buttonlabel = new char[64];
       item[4].form = NULL;
       item[4].path = NULL;
       strcpy(item[4].buttonlabel,"Maximize");       

       item[5].title = new char[128];        
       strcpy(item[5].title,"Value");
       item[5].startval = dv;
       item[5].done = 0;                // Set to 1 if maximize button is clicked
       item[5].k = 5;                   // which button this is
       item[5].minval = 0;
       item[5].maxval = 1000;
       item[5].type = BUTTONVALSLIDER;
       item[5].wantdragcb = 0;
       item[5].answers = item[0].answers; // Alias all answers to each other
       item[5].f1 = NULL;
       item[5].f2 = NULL;
       item[5].f3 = NULL;
       item[5].f4 = maximize_contrast;
       item[5].buttonlabel = new char[64];
       item[5].form = NULL;
       item[5].path = NULL;
       strcpy(item[5].buttonlabel,"Maximize");       

       item[6].title = new char[128];        
       strcpy(item[6].title,"Gray");
       item[6].startval = di;
       item[6].done = 0;                // Set to 1 if maximize button is clicked
       item[6].k = 6;                   // which button this is
       item[6].minval = 0;
       item[6].maxval = 1000;
       item[6].type = BUTTONVALSLIDER;
       item[6].wantdragcb = 0;
       item[6].answers = item[0].answers; // Alias all answers to each other
       item[6].f1 = NULL;
       item[6].f2 = NULL;
       item[6].f3 = NULL;
       item[6].f4 = maximize_contrast;
       item[6].buttonlabel = new char[64];
       item[6].form = NULL;
       item[6].path = NULL;
       strcpy(item[6].buttonlabel,"Maximize");       

       value[7] = 0;  // Dont make palette change permanent 

       item[0].ino = ci;
       item[0].noofbuttons = 7;
       item[0].f5 = changecontrastok;
       item[0].f6 = changecontrastfinish;
       item[0].f7 = null;
       item[0].f8 = null;
       multiclickbox("Contrast adjustment", 7, item, multiplypalettecolors, helptopic);
   }
}   


//-------------------------------------------------------------------------//
// changecontrastok                                                        //
//-------------------------------------------------------------------------//
void changecontrastok(clickboxinfo *c)
{
   drawselectbox(OFF);
   int *value = c[0].answers;
   g.rcontrast = value[0];
   g.gcontrast = value[1];
   g.bcontrast = value[2];
   g.hcontrast = value[3];
   g.scontrast = value[4];
   g.vcontrast = value[5];
   g.icontrast = value[6];

   value[7]=  1;   // Make palette change permanent 
   int ino = c[0].ino;
   multiplypalettecolors(value);   
   if(ino>=0)
   {   memcpy(z[ino].palette,g.palette,768); 
       setpalette(z[ino].palette);
       rebuild_display(ino);
       redraw(ino);
       z[ino].touched = 1;
   }else memcpy(g.b_palette, g.palette, 768); 
   value[7]=  0;   // Make palette change temporary (for slider)
}


//-------------------------------------------------------------------------//
// changecontrastfinish                                                    //
//-------------------------------------------------------------------------//
void changecontrastfinish(clickboxinfo *c)
{
   int k;
   for(k=0; k<7; k++)
   {     delete[] c[k].buttonlabel;
         delete[] c[k].title;
   }
   delete[] c[0].answers;
   delete[] c;          
}


//-------------------------------------------------------------------------//
// maximize_contrast                                                       //
//-------------------------------------------------------------------------//
void maximize_contrast(int noofargs, char **arg)
{
   int button;
   if(noofargs>0) button = atoi(arg[1]); else button=6;
   maximize_contrast(button, (clickboxinfo*) NULL);
}
void maximize_contrast(int button, clickboxinfo *cb)
{
   int d[10];
   int bpp,ino,i,j,x1,y1,x2,y2,rr,gg,bb,hh,ss,vv,ohitgray,
       rmin,gmin,bmin,rmax,gmax,bmax,smin,smax,vmin,vmax,imin,imax,
       value;
   double ifac,rfac,gfac,bfac,sfac,vfac;
   printstatus(BUSY);
   x1 = g.ulx;
   x2 = g.lrx;
   y1 = g.uly;
   y2 = g.lry;
   if(ci<0) return;
   bpp = z[ci].bpp;
   rmax = 0; rmin = g.maxred[bpp];
   gmax = 0; gmin = g.maxgreen[bpp];
   bmax = 0; bmin = g.maxblue[bpp];
   smax = 0; smin = g.maxblue[bpp];
   vmax = 0; vmin = g.maxgray[bpp];
   imax = 0; imin = (int)g.maxvalue[bpp];
   ////  Find smallest & largest r,g,b,i
   for(j=y1;j<y2;j+=2)
   for(i=x1;i<x2;i+=2)
   { 
        value = readpixelonimage(i,j,bpp,ino);
        if(ino > g.image_count) ino = ci;
        if(z[ino].colortype != GRAY)
        {   valuetoRGB(value,rr,gg,bb,bpp);
            RGBtoHSV(rr,gg,bb,hh,ss,vv,bpp);
            rmin=min(rr,rmin);
            gmin=min(gg,gmin);
            bmin=min(bb,bmin);
            smin=min(ss,smin);
            vmin=min(vv,vmin);
        }
        imin=min(value,imin);
        if(z[ino].colortype != GRAY)
        {   rmax=max(rr,rmax);
            gmax=max(gg,gmax);
            bmax=max(bb,bmax);
            smax=max(ss,smax);
            vmax=max(vv,vmax);
        }
        imax=max(value,imax);
   }
   if(rmax!=rmin) rfac=(double)g.maxred[bpp]/(double)(rmax-rmin);  else rfac=1.0;
   if(gmax!=gmin) gfac=(double)g.maxgreen[bpp]/(double)(gmax-gmin);else gfac=1.0;
   if(bmax!=bmin) bfac=(double)g.maxblue[bpp]/(double)(bmax-bmin); else bfac=1.0;
   if(imax!=imin) ifac=(double)g.maxvalue[bpp]/(double)(imax-imin); else ifac=1.0;
   if(smax!=smin) sfac=(double)g.maxgray[bpp]/(double)(smax-smin); else sfac=1.0;
   if(z[ci].colortype==GRAY) 
   {   if(imax>imin) vfac=g.maxvalue[bpp]/(double)(imax-imin);     else vfac=1.0;
       vmin = imin;
       vmax = imax;
   }else
   {   // imin=max(rmin,max(gmin,bmin)); // Doesn't work if one plane is all black
       // imax=min(rmax,min(gmax,bmax));
       vmin = (rmin+gmin+bmin)/3;
       vmax = (rmax+gmax+bmax)/3;
       if(vmax>vmin) vfac=(double)g.maxgreen[bpp]/(double)(vmax-vmin); else vfac=1.0;
   }
   d[0]=0;d[1]=0;d[2]=0;d[3]=0;d[4]=0;d[5]=0;d[6]=0;d[7]=1;
   switch(button)
   {   case 0: d[0] = -rmin; break;
       case 1: d[1] = -gmin; break;
       case 2: d[2] = -bmin; break;
       case 3: d[3] =     0; break; // cant maximize hue
       case 4: d[4] = -smin; break;
       case 5: d[5] = -vmin; break;
       case 6: d[6] = -imin; break;
   }
   addpalettecolors(d);

   d[0]=100;d[1]=100;d[2]=100;d[3]=100;d[4]=100;d[5]=100;d[6]=100;d[7]=1;
   switch(button)
   {  case 0: d[0] = int(100*rfac); break;
      case 1: d[1] = int(100*gfac); break;
      case 2: d[2] = int(100*bfac); break;
      case 3: d[3] =           100; break; // cant maximize hue
      case 4: d[4] = int(100*sfac); break;
      case 5: d[5] = int(100*vfac); break;
      case 6: d[6] = int(100*ifac); break;
   }
   multiplypalettecolors(d);
   repair(ci);

   ////  Force remapping of grayscale values
   ohitgray = z[ci].hitgray;
   z[ci].hitgray = 0;  
   z[ci].adjustgray = 1; // ok to adjust
   rebuild_display(ci);
   z[ci].hitgray = ohitgray;
   z[ci].adjustgray = 0; // not ok to adjust

   redraw(ci);
   z[ci].touched = 1;

   ////  Change label so slider reads 100%
   char string[256]; 
   if(cb != NULL) 
   {    cb->answer=100;
        itoa(cb->answer, string, 10); 
        XmTextSetString(cb->widget[0], string);
   }
   printstatus(NORMAL);
}


//--------------------------------------------------------------------------//
// multiplypalettecolors - multiply r,g,b,& i in palette by dr,dg,db,di     //
// Compatible with multiclickbox                                            //
// If value[7] is 1, the change is permanent, otherwise it is temporary.    //
//--------------------------------------------------------------------------//
void multiplypalettecolors(int d[10])
{ 
   RGB opal[256];
   int f=0,i,i2,j,k,bpp,ino,value,rr,gg,bb,hh,ss,vv,xx,yy,obpp=0;
   int x1,y1,x2,y2;
   double di,dr,dg,db,dh,ds,dv;
   printstatus(BUSY);
   dr = ((double)d[0])/100.0;
   dg = ((double)d[1])/100.0;
   db = ((double)d[2])/100.0;
   dh = ((double)d[3])/100.0;
   ds = ((double)d[4])/100.0;
   dv = ((double)d[5])/100.0;
   di = ((double)d[6])/100.0;
   for(k=0; k<g.image_count; k++) z[k].hit=0;
   if(d[7]==0)                          // Fast as possible & temporary
   {    if(g.bitsperpixel==8 && dh==1.0 && ds==1.0)
        {
            memcpy(opal,g.palette,768);
            for(j=0;j<256;j++) 
            {   g.palette[j].red   = max(0,min(63,(uchar)(dr * di * g.palette[j].red))); 
                g.palette[j].green = max(0,min(63,(uchar)(dg * di * g.palette[j].green)));
                g.palette[j].blue  = max(0,min(63,(uchar)(db * di * g.palette[j].blue)));
            }
            setpalette(g.palette);  
            memcpy(g.palette,opal,768); // restore original palette 
        }else                           // Otherwise, adjust z.img
        {
            for(j=g.selected_uly; j<=g.selected_lry; j++)
            for(i=g.selected_ulx; i<=g.selected_lrx; i++)
            {   
                    if(!g.selected_is_square && !inside_irregular_region(i,j)) continue;
                    value = readpixel(i,j);
                    valuetoRGB(value,rr,gg,bb,g.bitsperpixel);
                    rr = max(0,min((int)g.maxred[g.bitsperpixel], (int)(rr*dr*di)));
                    gg = max(0,min((int)g.maxgreen[g.bitsperpixel], (int)(gg*dg*di)));
                    bb = max(0,min((int)g.maxblue[g.bitsperpixel], (int)(bb*db*di)));
                    if(dh!=1.0 || ds!=1.0 || dv!=1.0)
                    {    RGBtoHSV(rr, gg, bb, hh, ss, vv, g.bitsperpixel);
                         hh = max(0,min(360, cint(hh*dh)));
                         ss = max(0,min(g.maxgray[g.bitsperpixel],  cint(ss*ds)));
                         vv = max(0,min(g.maxgray[g.bitsperpixel],  cint(vv*dv)));
                         HSVtoRGB(hh, ss, vv, rr, gg, bb, g.bitsperpixel);
                    }
                    value = RGBvalue(rr,gg,bb,g.bitsperpixel);
                    setpixel(i,j,value,g.imode);
            }
        }
   }else                                // Slow but accurate
   {    
        ino = ci;
        if(g.selectedimage>=0)          // Entire image
        {   bpp = z[ino].bpp;
            f = z[ino].cf;
            if(z[ino].colortype==GRAY)
            {  
                if(dr!=1 || dg!=1 || db!=1 || dh!=1 || ds!=1)
                {   message(notcolor); d[0]=d[1]=d[2]=d[3]=d[4]=100; return; }
                for(j=0;j<z[ino].ysize;j++)
                for(i=0,i2=0;i<z[ino].xsize;i++,i2+=g.off[bpp])
                {   value = pixelat(z[ino].image[f][j]+i2,bpp);
                    value = min((int)g.maxvalue[bpp], (int)max(0,(di * value)));
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
                if(ino>=0 && z[ino].hit==0 && z[ino].colortype==INDEXED) 
                {   z[ino].hit=2;    // Must be set before change_image_depth                   
                    if(change_image_depth(ino,24,0)!=OK) 
                     {   message("Insufficient image buffers available",ERROR);
                         g.getout=1; 
                         return;
                     }
                     bpp=24;
                }
                for(j=0;j<z[ino].ysize;j++)
                for(i=0,i2=0;i<z[ino].xsize;i++,i2+=g.off[bpp])
                {   RGBat(z[ino].image[f][j]+i2, bpp, rr, gg, bb);
                    rr = max(0,min((int)g.maxred[bpp], (int)(rr*dr*di)));
                    gg = max(0,min((int)g.maxgreen[bpp], (int)(gg*dg*di)));
                    bb = max(0,min((int)g.maxblue[bpp], (int)(bb*db*di)));
                    if(dh!=1.0 || ds!=1.0 || dv!=1.0)
                    {    RGBtoHSV(rr, gg, bb, hh, ss, vv, bpp);
                         hh = max(0,min(360, cint(hh*dh)));
                         ss = max(0,min(g.maxgray[bpp],  cint(ss*ds)));
                         vv = max(0,min(g.maxgray[bpp],  cint(vv*dv)));
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
        }else                      // Part of image
        {   
            x1 = g.ulx;             
            y1 = g.uly;
            x2 = g.lrx;
            y2 = g.lry;
            ino = ci;
            bpp = z[ino].bpp;
            for(j=y1; j<=y2; j++)
            for(i=x1,i2=x1*g.off[bpp]; i<=x2; i++,i2+=g.off[bpp])
            {   
                f = z[ino].cf;
                if(!g.selected_is_square && !inside_irregular_region(i,j)) continue;

                value = readpixelonimage(i,j,bpp,ino);
                ////  Crossing boundary to image with different bpp

                if(bpp!=obpp && bpp==8)
                {    obpp=bpp; 
                     switch_palette(ino);
                }   

                //// If part of an indexed image, must convert to color
                //// then regenerate colormap

                if(z[ino].hit==0 && z[ino].colortype==INDEXED) 
                {    z[ino].hit=2;    // Must be set before change_image_depth
                     if(change_image_depth(ino,24,0)!=OK) 
                     {   message("Insufficient image buffers available",ERROR);
                         g.getout=1; 
                         break;
                     }
                     rebuild_display(ino);
                     bpp=24;
                }

                if(z[ino].colortype==GRAY)
                {    if(dr!=1 || dg!=1 || db!=1 || dh!=1 || ds!=1)
                     {  d[0]=d[1]=d[2]=d[3]=d[4]=100; message(notcolor); return; }
                     value = min((int)g.maxvalue[bpp], (int)(di * value));
                }else
                {    valuetoRGB(value,rr,gg,bb,bpp);
                     convertRGBpixel(rr,gg,bb,bpp,g.bitsperpixel);              
                     rr = max(0,min((int)g.maxred[g.bitsperpixel], (int)(rr*dr)));
                     gg = max(0,min((int)g.maxgreen[g.bitsperpixel], (int)(gg*dg)));
                     bb = max(0,min((int)g.maxblue[g.bitsperpixel], (int)(bb*db)));
                     if(dh!=1.0 || ds!=1.0 || dv!=1.0)
                     {    RGBtoHSV(rr, gg, bb, hh, ss, vv, g.bitsperpixel);
                          hh = max(0,min(360, cint(hh*dh)));
                          ss = max(0,min(g.maxgray[g.bitsperpixel],  cint(ss*ds)));
                          vv = max(0,min(g.maxgray[g.bitsperpixel],  cint(vv*dv)));
                          HSVtoRGB(hh, ss, vv, rr, gg, bb, g.bitsperpixel);
                     }
                     convertRGBpixel(rr,gg,bb,g.bitsperpixel,bpp);              
                     value = RGBvalue(rr,gg,bb,bpp);
                }
                setpixelonimage(i,j,value,g.imode,bpp);
                if(ino>=0 && (!z[ino].hit)) z[ino].hit=1;
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
    printstatus(NORMAL);
}


