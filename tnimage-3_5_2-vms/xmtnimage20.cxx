//-------------------------------------------------------------------------//
// xmtnimage20.cc                                                          //
// Measure                                                                 //
// Latest revision: 10-08-2003                                             //
// Copyright (C) 2003 by Thomas J. Nelson                                  //
// See xmtnimage.h for Copyright Notice                                     //
//-------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
extern Widget www[4];
const int SELECT_MODE = SINGLE;
int measurelist = 0;
double totalarea=0.0;
int allocmeasuredata=0;
int trace_mode = 1;

//-------------------------------------------------------------------------//
// measure                                                                 //
// measure distance, area, angle, etc.                                     //
//-------------------------------------------------------------------------//
void measure(void)
{
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
   static Dialog *dialog;
   static XYData *measuredata;
   static char **measurelist_info;
   static listinfo *l = 0;
   static char *measurelist_title;
   measurelist_title = new char[100];
   strcpy(measurelist_title, "Measurement results");
   int helptopic = 33;
   g.crud = -1;
   g.measure_angle = -1;
 
   dialog = new Dialog;
   if(dialog==NULL){ message(g.nomemory); return; }
   int j,k;
   measurelist_info = new char*[10000];

   if(!measurelist) l = new listinfo;
   l->info   = measurelist_info;
   l->title  = measurelist_title;
   l->count  = 0;
   l->itemstoshow = 2;
   l->firstitem   = 1;
   l->wantsort    = 0;
   l->wantsave    = 1;
   l->helptopic   = helptopic;
   l->allowedit   = 1;   // if allowedit is 1, must allocate edittitle
   l->edittitle  = new char[100]; 
   l->selection   = &g.crud;
   l->width       = 0;
   l->wantfunction = 0;
   l->autoupdate   = 0;
   l->clearbutton  = 1;
   l->highest_erased = 0;
   l->f1 = null;
   l->f2 = null;
   l->f3 = measure_list_finish;
   l->f4 = delete_list;
   l->f5 = measure_list_clear;
   l->f6 = null;
   l->listnumber = 0;
   if(!measurelist){ measurelist++;  list(l); }
   measuredata = new XYData;
   measuredata->type  = 7;
   measuredata->dims  = 2;
   measuredata->width = 0;
   measuredata->n     = 0;
   measuredata->ino   = ci;
   measuredata->nmin  = 0;
   measuredata->nmax  = 300;
   measuredata->duration  = TEMP;
   measuredata->wantpause = 0;
   measuredata->wantmessage = 1;
   measuredata->win       = 0;  // Draw on main drawing_area or image
   measuredata->x = new int[measuredata->nmax];  
       if(measuredata->x==NULL){ message(g.nomemory,ERROR); return; }
   measuredata->y = new int[measuredata->nmax];  
       if(measuredata->y==NULL){ delete[] measuredata->x; message(g.nomemory,ERROR); return; }
   allocmeasuredata=1;
         
   //// Dialog box to get title & other options
   strcpy(dialog->title,"Measure");
   strcpy(dialog->radio[0][0],"");
   strcpy(dialog->radio[0][1],"Distance");             
   strcpy(dialog->radio[0][2],"Angle between 2 lines");
   strcpy(dialog->radio[0][3],"Angle of 1 line");
   strcpy(dialog->radio[0][4],"Length of segmented curve");             
   strcpy(dialog->radio[0][5],"Points");             
   strcpy(dialog->radio[0][6],"Areas");             
    
   dialog->radioset[0] = g.measure_angle;
   dialog->radiono[0] = 7;
   for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
   dialog->noofradios = 1;
   dialog->noofboxes = 0;
   dialog->helptopic = helptopic;  
   dialog->want_changecicb = 0;
   dialog->f1 = measurecheck;
   dialog->f2 = null;
   dialog->f3 = measurecancel;
   dialog->f4 = measurefinish;
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
   dialog->ptr[4] = measuredata;
   dialog->ptr[11] = l;
   strcpy(dialog->path,".");
   g.getout=0;
   dialog->message[0]=0;      
   dialog->busy = 0;
   dialogbox(dialog);
}


//--------------------------------------------------------------------//
// measure_list_finish                                                //
//--------------------------------------------------------------------//
void measure_list_finish(listinfo *l)
{
  l=l;
  measurelist--;
}


//--------------------------------------------------------------------------//
//  measurecancel                                                           //
//--------------------------------------------------------------------------//
void measurecancel(dialoginfo *a)
{
   a=a;
   g.getout=1;
}


//--------------------------------------------------------------------//
// measure_list_clear                                                 //
//--------------------------------------------------------------------//
void measure_list_clear(listinfo *l)
{
  l=l;
  totalarea = 0;
}


//--------------------------------------------------------------------------//
//  measurecheck                                                            //
//--------------------------------------------------------------------------//
void measurecheck(dialoginfo *a, int radio, int box, int boxbutton)
{
   if(!measurelist) return;
   radio=radio; box=box; boxbutton=boxbutton;
   static int oradio=-1;
   double angle=0,dxa,dxb,dya,dyb,length=0,lena=0,lenb=0,xx,
          xa=0.0, ya=0.0, za=0.0, xb=0.0, yb=0.0, dx=0, dy=0;
   int dims,calib_log=0,ino,bpp,gotcal=0,x1,y1,x2,y2,x3,y3,v,
          x4,y4,k,aborted = 0,oinmenu;
   char temp[1024],temp2[1024],temp3[1024];
   XYData *data = (XYData*)a->ptr[4];
   g.measure_angle = a->radioset[0];
   if(g.measure_angle < 0) return;
   listinfo *l = (listinfo*)a->ptr[11];

   if(!allocmeasuredata) return;
   if(g.getout){ measurefinish(a); g.getout=0; return; }
   l->finished = 0;
   if(a->radioset[0] !=0 || radio == -2) 
   {
      g.inmenu++;
      data->n = 0;
      drawselectbox(OFF);  
      while(!l->finished && g.getout==0) 
      {  temp[0] = 0;
         switch(g.measure_angle)
         {
         case 1:       // measure distance - possibly calibrated
             getline(x1,y1,x2,y2);
             if(g.getout) break;
             ino = whichimage(x1,y1,bpp);
             if(ino<0) break;
             gotcal = 0;
             if(z[ino].cal_log[0]!=-1) gotcal=1;  
             calib_log = z[ino].cal_log[0];
             dims = 1 + z[ino].cal_dims;
             if(calib_log==-1) 
             {   length = sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
                 doubletostring(length,g.signif,temp);
             }else if(dims==0) // 1d non-directional 
             {   xa = sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
                 length = fabs(xa * z[ino].cal_q[0][1]);
                 doubletostring(length, g.signif, temp);
             }else if(dims==1) // 1d directional 
             {  
                 calibratepixel(x2,y2,ino,0,xa,temp);
                 calibratepixel(x1,y1,ino,0,xb,temp);
                 length = xa-xb;
                 doubletostring(length,g.signif,temp);
             }else                        // 2d
             { 
                 calibratepixel(x2,y2,ino,0,xa,temp);
                 calibratepixel(x2,y2,ino,1,ya,temp);
                 calibratepixel(x1,y1,ino,0,xb,temp);
                 calibratepixel(x1,y1,ino,1,yb,temp);
                 dy = yb-ya;
                 dx = xa-xb;
                 sprintf(temp,"%g   %g",dx,dy);
             }
             if(gotcal) strcpy(temp2,"Calibrated distance");
             else       strcpy(temp2,"Uncalibrated distance");
             break;
         case 2:       // measure angle between 2 lines
             getline(x1,y1,x2,y2);
             if(g.getout)break;
             line(x1,y1,x2,y2,g.maxcolor,XOR);
             getline(x3,y3,x4,y4);
             line(x1,y1,x2,y2,g.maxcolor,XOR);
             dxa = x2-x1; dxb = x4-x3;
             dya = y2-y1; dyb = y4-y3;
             lena = sqrt( dxa*dxa + dya*dya );
             lenb = sqrt( dxb*dxb + dyb*dyb );
             if(lena*lenb == 0.0)
                 angle = 0.0;
             else
             {   xx = -(dxa*dxb + dya*dyb)/(lena*lenb);
                 if(fabs(xx)<=1.00) angle = acos(xx);
             }
             angle *= 360/6.283185308;
             doubletostring(angle,g.signif,temp);
             strcpy(temp2,"Angle");
             break;
         case 3:       // measure angle of one line
             getline(x1,y1,x2,y2);
             if(g.getout)break;
             line(x1,y1,x2,y2,g.maxcolor,XOR);
             dx = x2-x1; 
             dy = y2-y1; 
             angle = atan2(-dy, dx);              
             angle *= 360/6.283185308;
             if(angle < 0.0) angle += 360.0;
             doubletostring(angle,g.signif,temp);
             strcpy(temp2,"Angle");
             break;
         case 4:       // total length of segmented line
             data->n = 0;
             g.getout = 0;
             bezier_curve_start(data, NULL);
             g.block++;
             if(!g.getout) 
             {    while(g.bezier_state==CURVE)
                     XtAppProcessEvent(XtWidgetToApplicationContext(g.drawing_area),XtIMAll);
                  if(g.getout) aborted=1;
             }
             g.block = max(0, g.block-1);
             draw_boxes(data);                        // off
             draw_curve(data, XOR, OFF);              // off
             bezier_curve_end(data);
             g.bezier_state=NORMAL;
             g.state=NORMAL;

             ino=whichimage(data->x[0],data->y[0],bpp);
             gotcal=0;
             if(ino<0) break;
             if(z[ino].cal_log[0]!=-1) gotcal=1;  
             calib_log = z[ino].cal_log[0];
             length = 0;
             dx = dy = 0;
             for(k=0; k<data->n-1; k++)
             {
                 x1 = data->x[k];
                 y1 = data->y[k];
                 x2 = data->x[k+1];
                 y2 = data->y[k+1];
                 if(calib_log==-1) 
                 {   length += sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
                 }else if(z[ino].cal_dims==0) // 1d non-directional 
                 {   xa = sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
                     length += fabs(xa * z[ino].cal_q[0][1]);
                 }else if(z[ino].cal_dims==1) // 1d directional 
                 {   calibratepixel(x2,y2,ino,0,xa,temp);
                     calibratepixel(x1,y1,ino,0,xb,temp);
                     length += xa-xb;
                 }else                        // 2d
                 { 
                     calibratepixel(x2,y2,ino,0,xa,temp);
                     calibratepixel(x2,y2,ino,1,ya,temp);
                     calibratepixel(x1,y1,ino,0,xb,temp);
                     calibratepixel(x1,y1,ino,1,yb,temp);
                     dy += yb-ya;
                     dx += xa-xb;
                 }
             }
             if(calib_log == -1 || z[ino].cal_dims<=1)
                 doubletostring(length,g.signif,temp);           
             else
                 sprintf(temp,"%g   %g",dx,dy);
             if(gotcal) strcpy(temp2,"Calibrated distance");
             else       strcpy(temp2,"Uncalibrated distance");
             break;
         case 5:       // List of points
             if(g.getout) break;
             getpoint(x1, y1);
             if(g.getout) break;
             oinmenu = g.inmenu;
             g.inmenu = 0;
             v = readpixelonimage(x1,y1,bpp,ino);
             g.inmenu = oinmenu;
             if(ino<0) break;
             temp[0] = 0;
             if(ino>=0 && z[ino].cal_log[0] >= 0)
             {   calibratepixel(x1, y1, ino, 0, xa, temp);
                 calibratepixel(x1, y1, ino, 1, ya, temp);
                 calibratepixel(x1, y1, ino, 2, za, temp);
             }else
             {   xa = (double)x1;
                 ya = (double)y1;
                 za = (double)v;
             }
             sprintf(temp,"%d    %d   %d  -> %g   %g   %g", x1, y1, v, xa, ya, za);
             strcpy(temp2,"Uncalibrated     ->   Calibrated");
             break;
         case 6:       // Areas
             getbox(x1, y1, x2, y2);
             if(g.getout) break;
             ino = whichimage(x1,y1,bpp);
             gotcal = 0;
             if(ino>0)
             {    if(z[ino].cal_log[0]!=-1) gotcal=1;  
                  calib_log = z[ino].cal_log[0];
                  dims = 1 + z[ino].cal_dims;
                  if(calib_log==-1) 
                  {   length = sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
                      doubletostring(length,g.signif,temp);
                  }else if(dims==0) // 1d non-directional 
                  {   xa = sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
                      length = fabs(xa * z[ino].cal_q[0][1]);
                      doubletostring(length, g.signif, temp);
                  }else if(dims==1) // 1d directional 
                  {  
                      calibratepixel(x2,y2,ino,0,xa,temp);
                      calibratepixel(x1,y1,ino,0,xb,temp);
                      length = xa-xb;
                      doubletostring(length,g.signif,temp);
                  }else                        // 2d
                  { 
                      calibratepixel(x2,y2,ino,0,xa,temp);
                      calibratepixel(x2,y2,ino,1,ya,temp);
                      calibratepixel(x1,y1,ino,0,xb,temp);
                      calibratepixel(x1,y1,ino,1,yb,temp);
                      dy = yb-ya;
                      dx = xa-xb;
                  }
             }else
             {    dx = x2 - x1;
                  dy = y2 - y1;
             }
             totalarea += dx*dy;
             sprintf(temp,"area %g total %g",dx*dy, totalarea);
             if(gotcal) strcpy(temp2,"Calibrated distance");
             else       strcpy(temp2,"Uncalibrated distance");
             break;
         }
 
         ////  Display results
         if(strlen(temp))
         {    sprintf(temp3,"%d  %s ",l->count, temp);
              l->info[l->count] = new char[128];        
              temp3[127]=0;
              strcpy(l->info[l->count],temp3);       
              l->count++;
              additemtolist(l->widget, l->browser, temp3 , BOTTOM, 1);     
         }
      }
      g.inmenu--;
   }
   oradio = radio;
   return;
}


//--------------------------------------------------------------------------//
//  measurefinish                                                           //
//--------------------------------------------------------------------------//
void measurefinish(dialoginfo *a)
{
   if(g.diagnose) printf("entering measurefinish\n");
   XYData *measuredata = (XYData *)a->ptr[4];
   if(g.block) return;   // Waiting for input, don't deallocate
   g.getout = 0;
   message("Measurement mode finished");
   if(allocmeasuredata)
   {    if(measuredata->x) delete[] measuredata->x;
        if(measuredata->y) delete[] measuredata->y;
        allocmeasuredata=0;
   }
   if(g.diagnose) printf("leaving measurefinish\n");
}


//-------------------------------------------------------------------------//
// trace curve                                                             //
//-------------------------------------------------------------------------//
int trace_curve(void)
{
  if(memorylessthan(16384)){ message(g.nomemory,ERROR); return(NOMEM); }
  static Dialog *dialog;
  if(ci<0) { message("Please select an image",ERROR); return(NOIMAGES); }
  int j,k;
  dialog = new Dialog;
  if(dialog==NULL){ message(g.nomemory); return(NOMEM); }

  g.getout=0;
  strcpy(dialog->title,"Trace Curve");

  strcpy(dialog->title,"Measure");
  strcpy(dialog->radio[0][0],"Trace");
  strcpy(dialog->radio[0][1],"Top");             
  strcpy(dialog->radio[0][2],"Bottom");
    
  dialog->radioset[0] = trace_mode;
  dialog->radiono[0] = 3;
  for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;

  strcpy(dialog->boxes[0],"Options");
  strcpy(dialog->boxes[1],"Save trace to disk");
  strcpy(dialog->boxes[2],"Plot trace");
  strcpy(dialog->boxes[3],"Debug");
  strcpy(dialog->boxes[4],"Color to track");
  dialog->boxtype[0]=LABEL;
  dialog->boxtype[1]=TOGGLE;
  dialog->boxtype[2]=TOGGLE;
  dialog->boxtype[3]=TOGGLE;
  dialog->boxtype[4]=INTCLICKBOX;

  dialog->boxset[1] = g.trace_wantsave;
  dialog->boxset[2] = g.trace_wantplot;
  dialog->boxset[3] = g.trace_wantslow;
  itoa(g.trace_trackcolor, dialog->answer[4][0], 10);
  dialog->boxmin[4]=0;  dialog->boxmax[4] = (int)g.maxvalue[z[ci].bpp];

  dialog->noofradios=1;
  dialog->noofboxes=5;
  dialog->helptopic=32;  
  dialog->radiono[0]=3;
  dialog->want_changecicb = 0;
  dialog->f1 = tracecheck;
  dialog->f2 = null;
  dialog->f3 = null;
  dialog->f4 = tracefinish;
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
  strcpy(dialog->path, ".");
  dialog->message[0] = 0;      
  dialog->busy = 0;
  dialogbox(dialog);
  return OK;
}


//--------------------------------------------------------------------------//
//  tracefinish                                                             //
//--------------------------------------------------------------------------//
void tracefinish(dialoginfo *a)
{
  a=a;
}


//--------------------------------------------------------------------------//
//  tracecheck                                                              //
//--------------------------------------------------------------------------//
void tracecheck(dialoginfo *a, int radio, int box, int boxbutton)
{
  radio=radio; box=box; boxbutton=boxbutton;
  static int in_trace_check = 0;
  PlotData *pd=0;
  char filename[FILENAMELENGTH] = "1.trace";
  char tempstring[256];
  int helptopic=32;
  int d,j,nn,nmax,oldy=0,x,y1,y2,xstart,ystart=0,xend=0;
  int test1=0,test2=0,tmin=0,bpp,ino;
  int ymax=0;
  int done1=0,done2=0,done3=0;
  int xx,yy;

  trace_mode = a->radioset[0];
  g.trace_wantsave = a->boxset[1];
  g.trace_wantplot = a->boxset[2];
  g.trace_wantslow = a->boxset[3];
  g.trace_trackcolor = atoi(a->answer[4][0]);

  if(radio != -2) return;   //// User clicked Ok or Enter
  if(g.getout) return;
  if(in_trace_check) return;
  in_trace_check = 1;

  double *xdata;
  nmax = max(2*g.xres, 2048);
  xdata = new double[nmax];
  int *trace;
  trace = new int[nmax];
  if(trace==NULL){ message(g.nomemory,ERROR); delete[] xdata; in_trace_check=0; return; }
  char **ytitle;
  ytitle = new char*[1];
  ytitle[0] = new char[256];
  strcpy(ytitle[0],"Signal");      

  message("Move to starting point and click \nthe mouse to start");
  g.getout = 0;
  done3=0;
  ino = ci;
  while(!g.getout)
  {
    getpoint(xstart, ystart);
    oldy = ystart;
    if(g.getout) break;


    //// Begin tracing left to right
    for(x=xstart; x<z[ci].xpos+z[ci].xsize; x++)
    { 
        if(x-xstart >= nmax) break;
        xend = x;
        done1=0;done2=0;
        g.inmenu++;

        if(ino>=0 && z[ino].is_zoomed)
        { xx = cint((x-1)*z[ino].zoomx); yy = cint(oldy*z[ino].zoomy); }
        else
        { xx=(x-1); yy=oldy; }
        if(x > xstart)
        {  line(xx,yy-15,xx,yy+15,g.maxcolor,XOR);  // Cross hairs
           line(xx-15,yy,xx+15,yy,g.maxcolor,XOR);
        }
        g.inmenu--;
    
        if(g.getout) done3=0;
        ymax=oldy;
        tmin = (int)g.maxvalue[z[ci].bpp];

        //// Trace bottom (find closest point)
        //// y1 = tracking down  y2 = tracking up
        if(trace_mode == 2)
        {    for(y1=y2=oldy;;y1--,y2++)
                  {  
                if(y1 > z[ci].ypos) 
                {   test1 = readpixelonimage(x,y1,bpp,ino); 
                    d = abs(test1 - g.trace_trackcolor);
                    if(d<tmin) { tmin=d; ymax=y1; }
                }else done1=1;
                if(y2 < z[ci].ypos + z[ci].ysize) 
                {   test2 = readpixelonimage(x,y2,bpp,ino);
                    d = abs(test2 - g.trace_trackcolor);
                    if(d<tmin) { tmin=d; ymax=y2; }
                }else done2=1;
                if(done1==1 && done2==1)break;
             }  
        }
        if(trace_mode == 1)
        {
             //// Trace top (find highest point)
             for(y1=0; y1<z[ci].ypos + z[ci].ysize; y1++)
             {  
                if(y1 > z[ci].ypos) 
                {   test1 = readpixelonimage(x,y1,bpp,ino); 
                    d = abs(test1 - g.trace_trackcolor);
                    if(d<tmin) { tmin=d; ymax=y1; break; }
                }
             }            
        }

        //// Calculate calibrated x value

        ino = whichimage(x,ymax,bpp);
        if(ino>=0 && z[ino].cal_log[0] >= 0)
            calibratepixel(x, ymax, ino, 0, xdata[x-xstart], tempstring); 
        else
            xdata[x-xstart] = (double)(x-xstart);
  
        if(g.trace_wantslow)
        { 
           printf("x %d y %d value %d Max %d\n",x,ymax,tmin,
              z[ci].ysize-ymax+z[ci].ypos);
#ifdef LINUX
           usleep(100000);
#endif
           XtAppProcessEvent(g.app, XtIMAll);
           if(g.getout) done3=1;
        }
        oldy=ymax;
        g.inmenu++;
        if(ino>=0 && z[ino].is_zoomed)
        { xx = cint(x*z[ino].zoomx); yy = cint(oldy*z[ino].zoomy); }
        else
        { xx=x; yy=oldy; }
        line(xx,yy-15,xx,yy+15,g.maxcolor,XOR); // Cross hairs
        line(xx-15,yy,xx+15,yy,g.maxcolor,XOR);
        g.inmenu--;

        printcoordinates(x,ymax,0);
        trace[x-xstart] = z[ci].ysize - ymax + z[ci].ypos;
        if(done3==1) break;
    }
    
    g.inmenu++;
    if(ino>=0 && z[ino].is_zoomed)
    { xx = cint(x*z[ino].zoomx); yy = cint(oldy*z[ino].zoomy); }
    else
    { xx=x; yy=oldy; }
    line(xx-1,yy-15,x-1,yy+15,g.maxcolor,XOR);  // Cross hairs
    line(xx-15-1,yy,x+15-1,yy,g.maxcolor,XOR);
    g.inmenu--;

    nn = xend - xstart;
    if(g.trace_wantplot && !g.getout) 
    {   if(!graph_is_open(pd))
        { 
            pd = plot((char*)"Signal",(char*)"Pixel value", ytitle, xdata, 
                 trace, nn, 1, MEASURE, 0, 0, NULL, null, null, null, null,
                 null, helptopic, 1, 1);
            open_graph(pd, ci, HISTOGRAM_GRAPH);  
        }else 
        { 
            delete[] pd->data[0];  
            pd->data[0] = new double[nn];       
            pd->n = nn;
            for(j=0; j<nn; j++) pd->data[0][j] = (double)trace[j];
            redraw_graph(pd);
        }
    }
    if(g.trace_wantsave) save_scan(filename, trace, xend-xstart, 0);
  }
  delete[] ytitle[0];
  delete[] ytitle;
  delete[] trace;
  delete[] xdata;
  in_trace_check = 0; 
  g.getout = 0;
  return;
}


