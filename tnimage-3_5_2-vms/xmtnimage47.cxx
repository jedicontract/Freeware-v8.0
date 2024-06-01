//--------------------------------------------------------------------------//
// xmtnimage47.cc                                                           //
// Latest revision: 02-21-2004                                              //
// Copyright (C) 2004 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
// warp                                                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
int ***ggg;
int newino, nnnx, nnny;
int in_warp = 0;
int warp_direction = 1;

//-------------------------------------------------------------------------//
// warp                                                                    //
// Change shape of image                                                   //
//-------------------------------------------------------------------------//
void warp(void)
{
  static Dialog *dialog;
  int j,k;
  if(in_warp) return;
  dialog = new Dialog;
  if(dialog==NULL){ message(g.nomemory); in_warp = 0; return; }
  in_warp = 1;
  strcpy(dialog->title,"Warping");

  //-----Dialog box to get title & other options------------//

  strcpy(dialog->radio[0][0],"Operation");
  strcpy(dialog->radio[0][1],"1-D warp");
  strcpy(dialog->radio[0][2],"2-D warp");
  strcpy(dialog->radio[0][3],"Fix barrel/pincushion");
  strcpy(dialog->radio[0][4],"Restore original");
  dialog->radiono[0]=5;
  dialog->radioset[0] = -1;

  strcpy(dialog->radio[1][0],"Direction");
  strcpy(dialog->radio[1][1],"Vertical");
  strcpy(dialog->radio[1][2],"Horizontal");
  dialog->radiono[1]=3; 
  dialog->radioset[1] = warp_direction;

  for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;  

  strcpy(dialog->boxes[0],"2D Warp Parameters");
  dialog->boxtype[0]=LABEL;

  strcpy(dialog->boxes[1],"Pixels/grid point");
  dialog->boxtype[1]=INTCLICKBOX;
  sprintf(dialog->answer[1][0], "%d", g.warp_gridpoints);
  dialog->boxmin[1]=2; dialog->boxmax[1]=400;

  strcpy(dialog->boxes[2],"Select group of points");
  dialog->boxtype[2]=TOGGLE;
  dialog->boxset[2]=0;   

  strcpy(dialog->boxes[3],"Cursor movement");
  dialog->boxtype[3]=INTCLICKBOX;
  sprintf(dialog->answer[3][0], "%d", g.warp_cursor);
  dialog->boxmin[3]=1; dialog->boxmax[3]=100;
  
  dialog->want_changecicb = 0;
  dialog->f1 = warpcheck;
  dialog->f2 = null;
  dialog->f3 = null;
  dialog->f4 = warpfinish;
  dialog->f5 = null;
  dialog->f6 = null;
  dialog->f7 = null;
  dialog->f8 = null;
  dialog->f9 = null;
  dialog->width = 0;  // calculate automatically
  dialog->height = 0; // calculate automatically
  dialog->noofradios = 2;
  dialog->noofboxes = 4;
  dialog->helptopic = 65;
  dialog->transient = 1;
  dialog->wantraise = 0;

  //// Use a custom format dialog box
  dialog->radiousexy = 0;
  dialog->boxusexy = 0;
  strcpy(dialog->path, ".");
  strcpy(dialog->message, " ");      
  dialog->message_x1 = 160;
  dialog->message_y1 =  98;
  strcpy(dialog->message2, "");      
  dialog->message_x2 =   0;
  dialog->message_y2 =   0;
  dialog->busy = 0;
  dialogbox(dialog);
  return;
}


//--------------------------------------------------------------------------//
//  warpfinish                                                              //
//--------------------------------------------------------------------------//
void warpfinish(dialoginfo *a)
{
   a=a;
   in_warp = 0;
   printstatus(NORMAL);
   XFlush(g.display);
}



//--------------------------------------------------------------------------//
//  warpcheck                                                               //
//--------------------------------------------------------------------------//
void warpcheck(dialoginfo *a, int radio, int box, int boxbutton)
{
   static int inwarpcheck=0;
   if(inwarpcheck) return;  //// XtSetValues->dialogstringcb->fftcheck
   warp_direction = a->radioset[1];
   inwarpcheck=1;
   box=box; boxbutton=boxbutton;
   g.warp_gridpoints = atoi(a->answer[1][0]);
   g.warp_cursor = atoi(a->answer[3][0]);
   if(a->boxwidget[2][0]) XtSetSensitive(a->boxwidget[2][0], False);

   if(radio==0) switch(a->radioset[0])
   {
        case 1: warp1d(warp_direction); break;
        case 2: 
                XtSetSensitive(a->boxwidget[2][0], True);
                dialog_message(a->message_widget, (char*)"Press Esc when finished");
                warp2d(a, g.warp_gridpoints, g.warp_gridpoints);
                dialog_message(a->message_widget, (char*)"");
                break;
        case 3: 
                XtSetSensitive(a->boxwidget[2][0], False);
                dialog_message(a->message_widget, (char*)"Press Esc when finished");
                warp2d(a, z[ci].xsize/4, z[ci].ysize/4);
                dialog_message(a->message_widget, (char*)"");
                break;
        case 4: restoreimage(1); break;
   }
   unset_radio(a, 0);
   inwarpcheck=0;
   XFlush(g.display);
}


//-------------------------------------------------------------------------//
// warp1d                                                                  //
// Change shape of image                                                   //
//-------------------------------------------------------------------------//
void warp1d(int direction)
{
   if(ci<0){  message("Please select an image first\n");  return; }
   if(z[ci].floating) return;
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
   const int NMAX = 1000;
   int bpp, ino, d=0, i, j, k, xstart=0, xend, x, y, yalign, hindex;
   for(k=0; k<g.image_count; k++) z[k].hit=0;
   uint value;  
  
   XYData data;
   data.x = new int[NMAX]; if(data.x==NULL){ message(g.nomemory,ERROR); return; }
   data.y = new int[NMAX]; 
          if(data.y==NULL){ delete[] data.x; message(g.nomemory,ERROR); return; }
   data.v = NULL;
   data.n = 0;
   data.dims = 1;
   data.nmin = 0;
   data.nmax = NMAX;
   data.duration = TEMP;
   data.wantpause = 0;
   data.wantmessage = 1;
   if(direction==1)  data.type = 0;  // bezier
   if(direction==2)  data.type = 8;  // sketch
   data.win  = 0; // calculate window automatically on drawing area
 
   for(k=0;k<g.xres;k++) g.highest[k]=0;        // Initialize baseline buf.

   ////  bezier_curve_start() sets bezier_state to CURVE.                    
   ////  When user presses a key to stop the Bezier curve, bezier_curve_end()
   ////  sets bezier_state to 'NORMAL'.                                             
   ////  bezier_curve_end() also deallocates arrays x & y.                        
   ////  User selected curve is put in 1d int array g.highest[].

   bezier_curve_start(&data, NULL);
   g.block++;
   while(g.bezier_state==CURVE)
        XtAppProcessEvent(XtWidgetToApplicationContext(g.drawing_area),XtIMAll);
   g.block = max(0,g.block-1);

   xstart = 0;               // Left x coord of user's Bezier
   xend = g.xres;            // Right x coord of user's Bezier

   for(k=0;k<g.xres;k++) if(g.highest[k]!=0) { xstart=k; break; }
   for(k=g.xres;k>=xstart;k--) if(g.highest[k]!=0) { xend=k; break; }

   ////  Scale user's curve if ci is zoomed
   
   for(k=xstart; k<=xend; k++) 
        g.highest[k] = zoom_y_coordinate(g.highest[k], ci);


   ////  This section warps all the pixels in the selected image. Pixels outside
   ////  the image are  not affected.
   ////  highest[] is the Bezier curve for each x relative to the left of the 
   ////  screen. Subtract main_xpos to get the coordinate for drawing pixels
   ////  (which may be negative).

   if(direction==1)
   {
        message("Click at y value to align to");
        getpoint(x,y);

        yalign = zoom_y_coordinate(y, ci);
        xstart = zoom_x_coordinate(xstart - g.main_xpos - z[ci].xpos, ci);
        xend   = zoom_x_coordinate(xend   - g.main_xpos - z[ci].xpos, ci);
        xstart += z[ci].xpos;
        xend   += z[ci].xpos;

        for(i=xstart; i<=xend; i++)
        {   
            // d is distance to move down
            hindex = zoom_x_coordinate_of_index(i, ci) + g.main_xpos - z[ci].xpos;
            if(g.highest[hindex] == 0) continue;
            d =  yalign - g.highest[hindex];   

            if(d>0)                     // Pixels to be moved down 
            {   for(j=z[ci].ypos+z[ci].ysize-1; j>=z[ci].ypos; j--)
                {   if(j-d >= z[ci].ypos)
                      value = readpixelonimage(i,j-d,bpp,ino);
                    else
                      value = g.bcolor;
                    if(ino>=0)
                    {  if((g.bitsperpixel==8)&&(bpp!=8)&&(z[ino].hitgray==0))
                           z[ino].hit=1;
                       if(!g.wantr || !g.wantg || !g.wantb) z[ino].hit=1;
                       if(bpp==8 || z[ino].colortype==GRAY) z[ino].hit=1;
                    }
                    setpixelonimage(i,j,value,g.imode,bpp);
                }
            }else                       // Pixels to be moved up
            {   for(j=z[ci].ypos;j<z[ci].ysize+z[ci].ypos;j++)
                {   if(j-d < z[ci].ysize+z[ci].ypos)
                       value = readpixelonimage(i,j-d,bpp,ino);
                    else
                       value = g.bcolor;
                    if(ino>=0)
                    {  if((g.bitsperpixel==8)&&(bpp!=8)&&(z[ino].hitgray==0))
                           z[ino].hit=1;
                       if(!g.wantr || !g.wantg || !g.wantb) z[ino].hit=1;
                       if(bpp==8 || z[ino].colortype==GRAY) z[ino].hit=1;
                    }
                    setpixelonimage(i,j,value,g.imode,bpp);
                }
            }
            if(g.getout){ g.getout=0; break;}
        }    
   }else message("Function not implemented");

   g.state=NORMAL;
   g.bezier_state=NORMAL;
   delete[] data.x;
   delete[] data.y;
   for(k=0; k<g.image_count; k++) 
   {  if(z[k].hit)
      { 
           repairimg(k,0,0,z[k].xsize,z[k].ysize); 
           redraw(k); 
      }
   }
  switch_palette(ci);
  rebuild_display(ci); 
  redraw(ci); 
}


//-------------------------------------------------------------------------//
// warp2d                                                                  //
// Change shape of image n = no.of grid points                             //
//-------------------------------------------------------------------------//
void warp2d(dialoginfo *a, int nx, int ny)
{
   if(z[ci].floating) return;
   static int in_warp2d = 0;
   static int count=0;   
   int want_group=0;
   want_group = a->boxset[2];
   if(in_warp2d) return;
   in_warp2d=1;
   int b,bpp,ct,f,ino,i,j,x1,y1,x2,y2,xx=0,yy=0,xar,yar,need_redraw,ulx,uly,lrx,lry;
   int ox1 = 0, ox2 = 0, oy1 = 0, oy2 = 0, oinmenu;
   int gxmin,gxmax,gymin,gymax;
   ino = ci;
   if(!between(ino, 1, g.image_count-1)){ in_warp2d=0; return; }
   int xsize = z[ino].xsize;
   int ysize = z[ino].ysize;
   int xpos = z[ino].xpos;
   int ypos = z[ino].ypos;
   int newxpos = xpos + 100;
   int newypos = ypos + 100;
   int grid_visible = 1;
   char title[100];
   int c;
   void (*oldexposefunc)(void *) = NULL;  
   uchar temp[4];
   uint keysym=0;

   f = z[ino].cf;
   ct = z[ino].colortype;
   newino = ci;
   bpp = z[ino].bpp;
   b = g.off[bpp];
   xar = xsize/nx+1;
   yar = ysize/ny+1;
   array<int> grid(xar+10, yar+10, 2);
   if(!grid.allocated){ message(g.nomemory); in_warp2d=0; return; }
   array<int> selected_point(xar+10, yar+10);
   if(!selected_point.allocated){ message(g.nomemory); in_warp2d=0; return; }
   ggg = grid.q;
   nnnx = nx;
   nnny = ny;
   g.key_ascii = 0;

   initialize_grid(grid.q, xar, yar, nx, ny);
   initialize_selected_point(selected_point.p, xar, yar);
   duplicate_image(ino, newxpos, newypos, g.want_shell, g.window_border);
   setimagetitle(ci,"Warped");
   z[ci].touched = 1;
   newino = ci;

   message("Drag grid points to destination\nto define the coordinate mapping.\nClick Cancel when ready");
   g.getout = 0;
   if(!between(ino, 1, g.image_count-1)){ in_warp2d=0; return; }
   draw_grid(NULL);  
   g.busy++;
   oinmenu = g.inmenu;
   while(!g.getout)
   {    g.inmenu++;  
        g.warp_cursor = atoi(a->answer[3][0]);
        if(grid_visible && between(newino, 0, g.image_count-1))
        {   oldexposefunc = z[newino].exposefunc; 
            z[newino].exposefunc = draw_grid; 
        }
        ox1 = x1; ox2 = x2; oy1 = y1; oy2 = y2;

        want_group = a->boxset[2];
        initialize_selected_point(selected_point.p, xar, yar);
        if(want_group)
        {   dialog_message(a->message_widget, (char*)"Select group of points");
            getbox(ulx,uly,lrx,lry);  // Event can occur here, must check for change in state
            dialog_message(a->message_widget, (char*)"Drag group to new location");
            if(g.getout) break;
        }else
            dialog_message(a->message_widget, (char*)"Drag point to new location");
        getline(x1,y1,x2,y2); // Event can occur here, must check for change in state
        dialog_message(a->message_widget, (char*)"Press Esc when finished");

        if(g.getout) break;
        if(g.getout)
        {   g.inmenu=oinmenu; g.getout=0; g.busy=max(0,g.busy-1); in_warp2d=0; message("Aborted"); return; }
        if(!between(ci,     0, g.image_count-1) ||
           !between(newino, 0, g.image_count-1) || 
           !between(ino,    0, g.image_count-1)) 
        {   g.inmenu=oinmenu; g.getout=0; g.busy=max(0,g.busy-1); in_warp2d=0; message("Missing image",ERROR); return; }
        if(ci != newino) switchto(newino);

        need_redraw = 0;
        if(grid_visible) z[newino].exposefunc = oldexposefunc; 
        newxpos = z[newino].xpos;                         // Recalculate in case 
        newypos = z[newino].ypos;                         // image was moved

        x1 -= newxpos; y1 -= newypos;
        x2 -= newxpos; y2 -= newypos;
        ulx -= newxpos; uly -= newypos;
        lrx -= newxpos; lry -= newypos;
        g.inmenu=oinmenu;
        if(g.getout) break;
                                                          // Grid point to move
                                                          // xx,yy = closest point
        if((x1!=ox1 || y1!=oy1 || y2!=oy2 || y2!=oy2) ||  // A line was selected  (not a keypress)
           (x1!=x2  || y1!= y2))                          // A line of nonzero length (not a point) 
        {  
           if(want_group)
               find_selected_point(selected_point.p, newino, ulx, uly, lrx, lry, xar, yar, 
                              gxmin, gymin, gxmax, gymax); 
           else
               find_closest_point(grid.q, newino, x1, y1, xar, yar, nx, ny, xx, yy, 
                              gxmin, gymin, gxmax, gymax); 
        }

        if((x1!=ox1 || y1!=oy1 || y2!=oy2 || y2!=oy2) &&  // A line was selected (not a keypress)
           (x1!=x2  || y1!= y2))                          // A line of nonzero length (not a point)
        {      
           if(want_group)
           {    for(j=0; j<yar; j++)
                for(i=0; i<xar; i++)
                if(selected_point.p[j][i])
                {   grid.q[0][j][i] += x2-x1;               // Set new grid point
                    grid.q[1][j][i] += y2-y1;  
                    need_redraw = 1;
                }
           }else
           {   if(between(xx,0,xar) && between(yy,0,yar))
               {   grid.q[0][yy][xx] = x2;                    // Set new grid point
                   grid.q[1][yy][xx] = y2;  
                   need_redraw = 1;
               }
           }
        }

        c = 0;
        c = getcharacter(); 
        g.key_ascii = 0;                                  // get rid of keystroke
        temp[1] = c & 0xff;
        temp[0] = 255;
        keysym = (uint)temp[0]*256 + (uint)temp[1];
        XSync(g.display, 0);
        switch(c)
        {  case 'r':                                      // restore original grid
                initialize_grid(grid.q, xar, yar, nx, ny);
                remap_coordinates(ino, f, newino, grid.q, 0, 0, xar-1, yar-1, nx, ny);
                x1=x2=newxpos;
                y1=y2=newypos;
                repair(newino);
                redraw(newino);
                break;
           case 'g':
                grid_visible = 1 - grid_visible;
                gxmin=gymin=0;
                gxmax=xar;
                gymax=yar;
                break;
           case 'c':
                if(!between(newino, 0, g.image_count-1))
                {   message("Missing image",ERROR); g.getout=0; in_warp2d=0; g.busy=max(0,g.busy-1); return; }
                duplicate_image(newino, newxpos+20, newypos+20, g.want_shell, 
                   g.window_border);
                switchto(newino);
                if(g.getout) break;
                break;
           case 27:
                g.getout=1;
        }
        switch(keysym)
        {
           case XK_Left:  grid.q[0][yy][xx]-=g.warp_cursor; break;
           case XK_Right: grid.q[0][yy][xx]+=g.warp_cursor; break;
           case XK_Up:    grid.q[1][yy][xx]-=g.warp_cursor; break;
           case XK_Down:  grid.q[1][yy][xx]+=g.warp_cursor; break;     
        }
        if(need_redraw || c) 
        {   remap_coordinates(ino, f, newino, grid.q, gxmin, gymin, gxmax, gymax, nx, ny);
            repair(newino);
        }
        c = 0;
   }
   in_warp2d=0;
   z[newino].exposefunc = NULL;
   repair(newino);
   if(g.autoundo) backupimage(newino, 0); 
   sprintf(title, "warped_%d", ++count);
   setimagetitle(newino, title);
   z[newino].touched = 1;
   switchto(newino);
   g.getout = 0;
   g.busy=max(0,g.busy-1);
   g.inmenu=oinmenu;
   return;
}

//-------------------------------------------------------------------------//
// initialize_grid                                                         //
//-------------------------------------------------------------------------//
void initialize_grid(int ***g, int xar, int yar, int nx, int ny)
{
   if(!in_warp) return;
   int i,j;
   for(j=0;j<=yar;j++)
   for(i=0;i<=xar;i++)
   {
      g[0][j][i] = i*nx;  // x coord
      g[1][j][i] = j*ny;  // y coord
   }
}

//-------------------------------------------------------------------------//
// initialize_selected_point                                               //
//-------------------------------------------------------------------------//
void initialize_selected_point(int **s, int xar, int yar)
{
   if(!in_warp) return;
   int i,j;
   for(j=0;j<=yar;j++)
   for(i=0;i<=xar;i++) s[j][i] = 0;
}


//-------------------------------------------------------------------------//
// find_closest_point                                                      //
//-------------------------------------------------------------------------//
void find_closest_point(int ***grid, int ino, int x, int y, int xar, int yar, 
     int nx, int ny, int &xclosest, int &yclosest, int &gxmin, int &gymin, int &gxmax, 
     int &gymax)
{
   if(!in_warp) return;
   if(!between(ino, 1, g.image_count-1)) return; 
   int d,i,j,cd;
   int dx = z[ino].xsize / nx;
   int dy = z[ino].ysize / ny;
   xclosest = (x+nx/2)/nx;  // first approx. is unmoved point
   yclosest = (y+ny/2)/ny;       
   cd = (x-xclosest)*(x-xclosest) + (y-yclosest)*(y-yclosest);
   for(j=0;j<yar;j++)
   for(i=0;i<xar;i++)
   {
      d = (x-grid[0][j][i]) * (x-grid[0][j][i]) + 
          (y-grid[1][j][i]) * (y-grid[1][j][i]);        
      if(d<cd){ xclosest=i; yclosest=j; cd=d;}
   }

   ////  Find minimum & maximum grid coordinates to redraw
   gxmin = gxmax = xclosest;
   gymin = gymax = yclosest;
   for(j=0;j<yar;j++)
   for(i=0;i<xar;i++)
   {
       if(i<=gxmin && (x <= dx*i)) gxmin = i;
       if(i>=gxmax && (x >= dx*i)) gxmax = i;
       if(j<=gymin && (y <= dy*j)) gymin = j;
       if(j>=gymax && (y >= dy*j)) gymax = j;
   }
   gxmin = max(0,gxmin-1);
   gymin = max(0,gymin-1);
   gxmax = min(xar,gxmax+1);
   gymax = min(yar,gymax+1);
}


//-------------------------------------------------------------------------//
// find_selected_point                                                     //
//-------------------------------------------------------------------------//
void find_selected_point(int **selected_point, int ino, int x1, int y1, int x2, int y2, 
     int xar, int yar, int &gxmin, int &gymin,  int &gxmax, int &gymax)
{
   if(!in_warp) return;
   if(!between(ino, 1, g.image_count-1)) return; 
   int i,j;
   int dx = z[ino].xscreensize / xar;
   int dy = z[ino].yscreensize / yar;
   x1 = zoom_x_coordinate(x1, ino);
   y1 = zoom_y_coordinate(y1, ino);
   x2 = zoom_x_coordinate(x2, ino);
   y2 = zoom_y_coordinate(y2, ino);
   gxmin = xar; gxmax = 0;
   gymin = yar; gymax = 0;
   for(j=0;j<yar;j++)
   for(i=0;i<xar;i++)
   {
      if(between(i*dx, x1, x2) && between(j*dy, y1, y2))
             selected_point[j][i] = 1;
      gxmin = min(gxmin, i);
      gymin = min(gymin, j);
      gxmax = max(gxmax, i);
      gymax = max(gymax, j);
   }
}


//-------------------------------------------------------------------------//
// draw_grid                                                               //
//-------------------------------------------------------------------------//
void draw_grid(void *ptr)
{
   if(!in_warp) return;
   ptr = ptr;
   int nx = nnnx;
   int ny = nnny;
   int ino = newino;
   if(!between(ino, 1, g.image_count-1)) return; 
   int i,j,x1,y1,x2,y2,xe,ye;
   int xsize = z[ino].xsize;
   int ysize = z[ino].ysize;
   xe = 0;
   ye = 0;

   g.inmenu++;
   for(j=0;j<=ysize/ny;j++)
   for(i=1;i<=xsize/nx;i++)
   {  x1 = ggg[0][j][i-1];
      x2 = ggg[0][j][i  ];
      y1 = ggg[1][j][i-1];
      y2 = ggg[1][j][i  ];
      x1 = cint((double)x1 * z[ino].zoomx);
      x2 = cint((double)x2 * z[ino].zoomx);
      y1 = cint((double)y1 * z[ino].zoomy);
      y2 = cint((double)y2 * z[ino].zoomy);
      line(x1+xe, y1+ye, x2+xe, y2+ye, 255, SET, z[ino].win);
   }
   for(j=1;j<=ysize/ny;j++)
   for(i=0;i<=xsize/nx;i++)
   {  x1 = ggg[0][j-1][i];
      x2 = ggg[0][j  ][i];
      y1 = ggg[1][j-1][i];
      y2 = ggg[1][j  ][i];
      x1 = cint((double)x1 * z[ino].zoomx);
      x2 = cint((double)x2 * z[ino].zoomx);
      y1 = cint((double)y1 * z[ino].zoomy);
      y2 = cint((double)y2 * z[ino].zoomy);
      line(x1+xe,y1+ye,x2+xe,y2+ye,255,SET,z[ino].win);
   }
   g.inmenu--;
}


//-------------------------------------------------------------------------//
// remap_coordinates                                                       //
//-------------------------------------------------------------------------//
void remap_coordinates(int ino, int f, int newino, int ***grid, int gridx1,
     int gridy1, int gridx2, int gridy2, int nx, int ny)
{
   if(!in_warp) return;
   if(!between(ino, 1, g.image_count-1)) return; 
   if(!between(newino, 1, g.image_count-1)) return; 
   int b,bpp,i,j,k,l,v,x,y,x1,y1,x2,y2,x3,y3,x4,y4;
   int xsrc,ysrc;
   int xsize = z[ino].xsize;
   int ysize = z[ino].ysize;
   double dx,dy,xa,xb,ya,yb; 
   uchar* address;
   bpp = z[ino].bpp;
   b = g.off[bpp];

   for(j=gridy1; j<gridy2; j++)
   for(i=gridx1; i<gridx2; i++)
   {  
       x1 = grid[0][j  ][i  ];  y1 = grid[1][j  ][i  ]; 
       x2 = grid[0][j  ][i+1];  y2 = grid[1][j  ][i+1]; 
       x3 = grid[0][j+1][i  ];  y3 = grid[1][j+1][i  ]; 
       x4 = grid[0][j+1][i+1];  y4 = grid[1][j+1][i+1];   // 4 closest grid pts.

       for(l=0; l<ny; l++)
       for(k=0; k<nx; k++)
       {
            x = min(xsize-1, k + i*nx);     // Destination coordinates 
            y = min(ysize-1, l + j*ny);
       
            xb = (double)k / nx;    // x fractional offset of point in box 
            yb = (double)l / ny;    // y fractional offset of point in box 
            xa = 1 - xb;
            ya = 1 - yb;

            dx = ya * (xa*(x1-x) + xb*(x2-x)) +
                 yb * (xa*(x3-x) + xb*(x4-x));

            dy = xa * (ya*(y1-y) + yb*(y3-y)) +
                 xb * (ya*(y2-y) + yb*(y4-y));
             
            xsrc = x - (int)dx;     // Source coordinates
            ysrc = y - (int)dy; 
            if(between(xsrc,0,xsize-2) && between(ysrc,0,ysize-2))
               v = pixelat(z[ino].image[f][ysrc] + xsrc*b, bpp);
            else v=0;
            address = z[newino].image[0][y] + x*b;
            putpixelbytes(address, v, 1, bpp);
       }
   }
}
