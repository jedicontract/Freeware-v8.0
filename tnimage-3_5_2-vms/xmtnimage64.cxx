//--------------------------------------------------------------------------//
// xmtnimage64.cc                                                           //
// Latest revision: 07-07-2004                                              //
// Copyright (C) 2004 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
int alter[MAXIMAGES];
int floating_size=2;
int floating_scale=2;
int fsize=1;
int fscale=1;
int float_shell = 1;
int float_raise = 1;


//-------------------------------------------------------------------------//
// alternate                                                               //
//-------------------------------------------------------------------------//
void alternate(int noofargs, char **arg)
{
  noofargs=noofargs; arg=arg;
  static clickboxinfo* item;
  char temp[100];
  int helptopic = 1;
   
  item = new clickboxinfo[3];
  item[0].param[0] = 2; // no of images 
  item[0].wantpreview=0;
  item[0].title = new char[128];        
  strcpy(item[0].title,"First image");
  item[0].startval = 1;
  item[0].minval = 1;
  item[0].maxval = g.image_count - 1;
  item[0].type = VALSLIDER;
  item[0].wantdragcb = 0;
  item[0].answers = new int[10]; // Must be 10 for multiclickbox cb
  item[0].f1 = null;
  item[0].f2 = null;
  item[0].f3 = null;
  item[0].f4 = null;
  item[0].form = NULL;
  item[0].path = NULL;

  item[1].title = new char[128];        
  strcpy(item[1].title,"Second image");
  item[1].startval = ci;
  item[1].minval = 1;
  item[1].maxval = g.image_count - 1;
  item[1].type = VALSLIDER;
  item[1].wantdragcb = 0;
  item[1].answers = item[0].answers; 
  item[1].f1 = null;
  item[1].f2 = null;
  item[1].f3 = null;
  item[1].f4 = null;
  item[1].f5 = null;
  item[1].f6 = null;
  item[1].f7 = null;
  item[1].f8 = null;
  item[1].form = NULL;
  item[1].path = NULL;

  item[2].title = new char[128];        
  strcpy(item[2].title,"Delay (msec)");
  item[2].startval = 500;
  item[2].minval = 1;
  item[2].maxval = 10000;
  item[2].type = VALSLIDER;
  item[2].wantdragcb = 0;
  item[2].answers = item[0].answers; 
  item[2].f1 = null;
  item[2].f2 = null;
  item[2].f3 = null;
  item[2].f4 = null;
  item[2].f5 = null;
  item[2].f6 = null;
  item[2].f7 = null;
  item[2].f8 = null;
  item[2].form = NULL;
  item[2].path = NULL;

  strcpy(temp,"Alternate two images");
  item[0].ino = ci;
  item[0].noofbuttons = 3;
  item[0].f5 = alternateok;
  item[0].f6 = alternatefinish;
  item[0].f7 = null;
  item[0].f8 = null;
  multiclickbox(temp, 3, item, null, helptopic);
  
}


//-------------------------------------------------------------------------//
// alternateok                                                             //
//-------------------------------------------------------------------------//
void alternateok(clickboxinfo *c)
{
  int k, i1, i2;
  int delay = 0;
  int n;
  if(c[0].param[0] == 2) // no of images 
  {   delay = c[2].answer;   // delay
      alter[0] = c[0].answer;   // 1st
      alter[1] = c[1].answer;   // 2nd
      n = 2;
  }else
  {   delay = c[2].answer;   // delay
      i1 = min(g.image_count, c[0].answer);
      i2 = max(0,c[1].answer);
      n = 1 + i2 - i1;
      for(k=0; k<n; k++) alter[k] = k+i1;
  }          
  alternate_images(n, delay, alter);
}


//-------------------------------------------------------------------------//
// alternatefinish                                                         //
//-------------------------------------------------------------------------//
void alternatefinish(clickboxinfo *c)
{
   delete[] c[0].title;
   delete[] c[0].answers;
   delete[] c;
}


//--------------------------------------------------------------------------//
// alternate_images                                                         //
//--------------------------------------------------------------------------//
void alternate_images(int n, int delay, int *image_list)
{
  static XYData data;
  data.n = n;
  data.x = image_list;
  data.param[0] = delay;
  data.param[1] = 0; //  index of current image
  XtAppAddTimeOut(g.app, delay, alternatecb, (XtP)&data);
}


//--------------------------------------------------------------------------//
// alternatecb                                                              //
//--------------------------------------------------------------------------//
void alternatecb(XtP client_data, XtIntervalId *timer_id)
{  
   int ino;
   XYData *data = (XYData*) client_data;
   int n = data->n;
   int *x = data->x;  // image list
   int delay = data->param[0];
   int current_index = data->param[1];
   XEvent event;
   client_data = client_data;
   timer_id=timer_id;  // Keep compiler quiet

   if(XCheckMaskEvent(g.display, g.main_mask | g.mouse_mask, &event))
         XtAppProcessEvent(g.app, XtIMAll);
   if(!g.getout)
      XtAppAddTimeOut(XtWidgetToApplicationContext(g.main_widget), delay, 
         alternatecb, (XtP)data);
   g.getout = 0;
   current_index++;
   if(current_index >= n) current_index = 0;
   ino = x[current_index];
   data->param[1] = current_index;

   if(!between(ino, 1, g.image_count-1)) return;
   if(z[ino].shell) 
      XRaiseWindow(g.display,XtWindow(XtParent(XtParent(z[ino].widget))));
   else
      XRaiseWindow(g.display,z[ino].win);
}


//--------------------------------------------------------------------------//
// gamma                                                                    //
//--------------------------------------------------------------------------//
void gamma(int noofargs, char **arg)
{
   int f=0,ino,j,k,i,rr,gg,bb,bpp=0,obpp=0,value,x1,x2,y1,y2;
   static char gstring[1024] = "";
   static double gfac = 1.0, pixfac;
   double gam;
   if(noofargs>0) gfac = atof(arg[1]);
   else
   {    getstring("Gamma value:", gstring, 1024, 0);
        gfac = atof(gstring);
   }
   drawselectbox(OFF);
   printstatus(BUSY);
   gam = 1.0 / gfac;

   x1 = g.ulx;  
   y1 = g.uly;
   x2 = g.lrx;  
   y2 = g.lry;
   if(ci<=0){ message("Select an image first"); return; }
   if(z[ci].colortype == GRAY)
      pixfac = (double)g.maxvalue[z[ci].bpp] / (double)pow(g.maxvalue[z[ci].bpp], gam);
   else
      pixfac = (double)g.maxgray[z[ci].bpp] / (double)pow(g.maxgray[z[ci].bpp], gam);
   printstatus(BUSY);
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
             if(ino>=0 && z[ino].colortype==GRAY)
             {  value = cint(pixfac * pow(value, gam));
                value = min((uint)g.maxvalue[bpp],(uint)max(0,value));
             }else
             {  valuetoRGB(value,rr,gg,bb,bpp);
                rr = max(0,min((int)g.maxred[bpp],   ( cint(pixfac * pow(rr, gam)) )));
                gg = max(0,min((int)g.maxgreen[bpp], ( cint(pixfac * pow(gg, gam)) )));
                bb = max(0,min((int)g.maxblue[bpp],  ( cint(pixfac * pow(bb, gam)) )));
                value = RGBvalue(rr,gg,bb,bpp);
             }
             setpixelonimage(i,j,value,SET,bpp,-1,1);
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
// floating_magnifier_dialog                                               //
//-------------------------------------------------------------------------//
void floating_magnifier_dialog(void)
{
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
   static Dialog *dialog;
   dialog = new Dialog;
   if(dialog==NULL){ message(g.nomemory); return; }
   int j,k;
      
   strcpy(dialog->title,"Floating Magnifier");
   strcpy(dialog->radio[0][0],"Size");
   strcpy(dialog->radio[0][1],"9x9");
   strcpy(dialog->radio[0][2],"25x25");
   strcpy(dialog->radio[0][3],"49x49");             
   strcpy(dialog->radio[0][4],"81x81");             
   strcpy(dialog->radio[1][0],"Magnification");             
   strcpy(dialog->radio[1][1],"4x");             
   strcpy(dialog->radio[1][2],"9x");             
   strcpy(dialog->radio[1][3],"16x");             

   strcpy(dialog->boxes[0],"Options");
   strcpy(dialog->boxes[1],"Window frame");
   strcpy(dialog->boxes[2],"Keep on top");
   dialog->boxtype[0]=LABEL;
   dialog->boxtype[1]=TOGGLE;
   dialog->boxtype[2]=TOGGLE;
   dialog->boxset[1] = float_shell;
   dialog->boxset[2] = float_raise;

   dialog->radioset[0] = floating_size;
   dialog->radioset[1] = floating_scale;
   dialog->radiono[0] = 5;
   dialog->radiono[1] = 4;
   for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
   dialog->noofradios = 2;
   dialog->noofboxes = 3;
   dialog->helptopic = 1;  
   dialog->want_changecicb = 0;
   dialog->f1 = floating_magnifier_check;
   dialog->f2 = null;
   dialog->f3 = null;
   dialog->f4 = null;
   dialog->f5 = null;
   dialog->f6 = null;
   dialog->f7 = null;
   dialog->f8 = null;
   dialog->f9 = null;
   dialog->transient = 1;
   dialog->wantraise = 0;
   dialog->radiousexy = 0;
   dialog->boxusexy = 0;
   dialog->width = 0;  // calculate automatically
   dialog->height = 0; // calculate automatically
   g.getout=0;
   dialog->message[0]=0;      
   dialog->busy = 0;
   dialogbox(dialog);
}


//-------------------------------------------------------------------------//
// floating_magnifier_check                                                //
//-------------------------------------------------------------------------//
void floating_magnifier_check(dialoginfo *a, int radio, int box, int boxbutton)
{
   box=box; radio=radio; boxbutton=boxbutton;
   floating_size = a->radioset[0];
   floating_scale = a->radioset[1];
   if(radio)
   {   switch(floating_size)
       {   case 1: fsize=9; break;
           case 2: fsize=25; break;
           case 3: fsize=49; break;
           case 4: fsize=81; break;
       }
       switch(floating_scale)
       {   case 1: fscale=4; break;
           case 2: fscale=9; break;
           case 3: fscale=16; break;
       }
   }
   if(box)
   {   float_shell = a->boxset[1];
       float_raise = a->boxset[2];
   }

   if(radio != -2) return;       // Return unless user clicked OK
   initialize_floating_magnifier(fsize, fscale);
   dialogunmapcb(0, (XtP)a, 0);
}


//-------------------------------------------------------------------------//
// initialize_floating_magnifier                                           //
//-------------------------------------------------------------------------//
int initialize_floating_magnifier(int size, int scale)
{
  int ino, xpos, ypos;
  if(float_shell){xpos = g.main_xpos+100; ypos = g.main_ypos+100;}
  else {xpos = 50; ypos = 50;}
  if(newimage("Magnifier",xpos, ypos,size*scale, size*scale, g.bitsperpixel, 
     COLOR, 1, float_shell, 0, PERM, 0, 1, 0)!=OK)
     { message(g.nomemory); return NOMEM; }
  ino = ci;
  z[ino].floating = 1;
  sprintf(z[ino].name,"%dx magnification",scale);
  setimagetitle(ino, z[ino].name);
  z[ino].exposefunc = float_raisecb;
  return OK;
}


//-------------------------------------------------------------------------//
// float_raisecb                                                           //
//-------------------------------------------------------------------------//
void float_raisecb(void *ptr)
{
  ptr = ptr;
  int k, ino=-1;
  for(k=0;k<g.image_count;k++) if(z[k].floating){ ino = k; break; }
  if(float_raise && between(ino,1,g.image_count) && z[ino].shell) 
        XRaiseWindow(g.display, XtWindow(XtParent(XtParent(z[ino].widget))));
}


//-------------------------------------------------------------------------//
// destroy_floating_magnifier                                              //
//-------------------------------------------------------------------------//
void destroy_floating_magnifier()
{
  int k, ino=-1;
  for(k=0;k<g.image_count;k++) if(z[k].floating){ ino = k; break; }
  if(between(ino,1,g.image_count)) eraseimage(ino,0,0,0);
  g.getout=1;
}


//-------------------------------------------------------------------------//
// update_floating_magnifier                                               //
//-------------------------------------------------------------------------//
int update_floating_magnifier(int x, int y)
{
   static int ox,oy;
   static int orx=0,ory=0;
   uchar *add;
   int center,ino=ci,imag=-1,i,j,k,l,v,xx,xx2,yy,yy2,ibpp, xout, vcenter=0,fsize2;
   int gotupdate = 0;
   if(g.state==MOVEIMAGE && g.mouse_button) return 0;
   for(k=0;k<g.image_count;k++) if(z[k].floating){ imag = k; break; } // floating image
   if(!between(imag,1,g.image_count)) return 0;
   g.getout = 0;

   int rx,ry,wx,wy; 
   uint bpp;
   uint uw,uh,ubw;   
   int xroot,yroot,size;
   uint keys;
   Window rwin, cwin;
   XQueryPointer(g.display,g.main_window,&rwin,&cwin,&rx,&ry,&wx,&wy,&keys);
   fsize2 = fsize / 2;
   if(cwin)
   {
       ino = whichimage(x,y,ibpp,imag);  // source image, imag is excluded
       if(fsize >= z[imag].xsize || fsize >= z[imag].ysize) return 0;
       for(l=0;l<fsize;l++)
       for(k=0;k<fsize;k++)
       {   xx = x + k - fsize2;
           yy = y + l - fsize2; 
           v = readpixel(xx,yy);
           if(k==fsize2 && l==fsize2) vcenter = v;
           for(j=0;j<fscale;j++)
           for(i=0;i<fscale;i++)
           {  xout = fscale * k + i;
              xx2 = xout * g.off[g.bitsperpixel];
              yy2 = fscale * l + j;
              if(g.getout) break;
              if(xout >= z[imag].xsize || yy2 >= z[imag].ysize) continue;
              add = z[imag].img[yy2]+xx2;
              putpixelbytes(add,v,1,g.bitsperpixel,1);
           }
       }
       gotupdate = 1;
   }else if(g.floating_magnifier_on_root)
   {
       XQueryPointer(g.display,g.root_window,&rwin,&cwin,&rx,&ry,&wx,&wy,&keys);
       if(cwin && (rx!=orx || ry!=ory))
       {
           size = cint((double)z[imag].xsize / fscale);
           XGetGeometry(g.display, cwin, &rwin, &xroot, &yroot, &uw, &uh, &ubw, &bpp);
           ////  Ignore the sometimes incorrect bpp information from XGetGeometry
           bpp = g.bitsperpixel;  
           ////  Make sure grabbed window is not off screen
           uw = min(g.xres - xroot - 1, (int)uw);
           uh = min(g.yres - yroot - 1, (int)uh);
           uw = min((int)uw, size);
           uh = min((int)uh, size);
           xx = max(0, min(wx-size/2, g.xres-1-size));
           yy = max(0, min(wy-size/2, g.yres-1-size));
           XGetSubImage(g.display, g.root_window, xx, yy, uw, uh, XAllPlanes(), ZPixmap, z[imag].image_ximage, 0, 0); 
           rebuild_display(imag);
           orx=rx; ory=ry;
           gotupdate = 1;

           ////  Enlarge
           for(l=0;l<fsize;l++)
           for(k=0;k<fsize;k++)
           {  add = z[imag].image[0][l]+k*g.off[g.bitsperpixel];
              v = pixelat(add,bpp);
              if(k==fsize2 && l==fsize2) vcenter = v;
              for(j=0;j<fscale;j++)
              for(i=0;i<fscale;i++)
              {   xout = fscale * k + i;
                  xx = xout * g.off[g.bitsperpixel];
                  yy = fscale * l + j;

                  //// If it has scroll bars, image buffer is automatically
                  //// made smaller in image_resize() after first configure
                  //// event.

                  if(xout >= z[imag].xsize || yy >= z[imag].ysize) continue;
                  add = z[imag].img[yy]+xx;
                  putpixelbytes(add,v,1,g.bitsperpixel,1);
              }
           }
       }
   }

   //// Draw cross
   if(vcenter > (int)g.maxvalue[g.bitsperpixel] / 2)
       v = 0;
   else
       v = (int)g.maxvalue[g.bitsperpixel];
   center = fsize*fscale/2;
   if(g.getout) {g.getout=0; return 0; }
   for(j=0;j<5;j++)
   {    add = z[imag].img[j+center-2]+center*g.off[g.bitsperpixel];
        putpixelbytes(add,v,1,g.bitsperpixel,1);
   }
   for(k=0;k<5;k++)
   {    add = z[imag].img[center]+(k+center-2)*g.off[g.bitsperpixel];
        putpixelbytes(add,v,1,g.bitsperpixel,1);
   }
   redraw(imag);
   ox = x;
   oy = y;
   return gotupdate;
}


//-------------------------------------------------------------------------//
// animate                                                                 //
//-------------------------------------------------------------------------//
void animate(int noofargs, char **arg)
{
  noofargs=noofargs; arg=arg;
  static clickboxinfo* item;
  char temp[100];
  int helptopic = 1;
   
  item = new clickboxinfo[3];
  item[0].param[0] = 0; // set no of images later
  item[0].wantpreview=0;
  item[0].title = new char[128];        
  strcpy(item[0].title,"First image");
  item[0].startval = 1;
  item[0].minval = 1;
  item[0].maxval = g.image_count - 1;
  item[0].type = VALSLIDER;
  item[0].wantdragcb = 0;
  item[0].answers = new int[10]; // Must be 10 for multiclickbox cb
  item[0].f1 = null;
  item[0].f2 = null;
  item[0].f3 = null;
  item[0].f4 = null;
  item[0].form = NULL;
  item[0].path = NULL;

  item[1].title = new char[128];        
  strcpy(item[1].title,"Last image");
  item[1].startval = ci;
  item[1].minval = 1;
  item[1].maxval = g.image_count - 1;
  item[1].type = VALSLIDER;
  item[1].wantdragcb = 0;
  item[1].answers = item[0].answers; 
  item[1].f1 = null;
  item[1].f2 = null;
  item[1].f3 = null;
  item[1].f4 = null;
  item[1].f5 = null;
  item[1].f6 = null;
  item[1].f7 = null;
  item[1].f8 = null;
  item[1].form = NULL;
  item[1].path = NULL;

  item[2].title = new char[128];        
  strcpy(item[2].title,"Delay (msec)");
  item[2].startval = 500;
  item[2].minval = 1;
  item[2].maxval = 10000;
  item[2].type = VALSLIDER;
  item[2].wantdragcb = 0;
  item[2].answers = item[0].answers; 
  item[2].f1 = null;
  item[2].f2 = null;
  item[2].f3 = null;
  item[2].f4 = null;
  item[2].f5 = null;
  item[2].f6 = null;
  item[2].f7 = null;
  item[2].f8 = null;
  item[2].form = NULL;
  item[2].path = NULL;

  strcpy(temp,"Animate images");
  item[0].ino = ci;
  item[0].noofbuttons = 3;
  item[0].f5 = alternateok;
  item[0].f6 = alternatefinish;
  item[0].f7 = null;
  item[0].f8 = null;
  multiclickbox(temp, 3, item, null, helptopic); 

}
