//-------------------------------------------------------------------------//
// xmtnimage48.cc                                                          //
// Image calibration                                                       //
// Latest revision: 12-03-2003                                             //
// Copyright (C) 2003 by Thomas J. Nelson                                  //
// See xmtnimage.h for Copyright Notice                                    //
//-------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
int dimension = 0;

//// Globals for calibrate
int oldx[3][300]; 
int oldy[3][300]; 
double oldv[3][300]; 
int oldn[3] = { 0,0,0 };
char oldf[3][64] = { "No data","No data","No data" };
char oldr[3][64] = { "No data","No data","No data" };
XYData calibdata[3];
int in_calibrate = 0;
int cal_term[3] = { 1,1,1 };

//-------------------------------------------------------------------------//
// calibrate - MW as function of x or y                                    //
// If an image is selected, the image calib. is position-independent.      //
// If calibrating background, calib. is fixed.                             //
//-------------------------------------------------------------------------//
void calibrate(void)
{
   if(memorylessthan(16384)){ message(g.nomemory,ERROR); return; }
   g.getout=0;
   static Dialog *dialog;
   int j, k, ino, dims=3;
   g.getout=0;
   ino = ci;
   if(ino<0) return;
   if(in_calibrate) return;
   dialog = new Dialog;
   if(dialog==NULL){ message(g.nomemory); return; }
   in_calibrate = 1;
   g.inmenu++;  // no 'return's after this point

   //// Data
   for(k=0; k<3; k++)
   {   calibdata[k].x         = new int[300];
       calibdata[k].y         = new int[300];
       calibdata[k].v         = new double[300];
       calibdata[k].n         = oldn[k];
       calibdata[k].dims      = dims-2;
       calibdata[k].ino       = ino;
       calibdata[k].nmin      = 2;
       calibdata[k].nmax      = 300;
       calibdata[k].width     = 0;
       calibdata[k].type      = 0;
       calibdata[k].duration  = TEMP;
       calibdata[k].wantpause = 0;
       calibdata[k].wantmessage = 1;
       calibdata[k].win       = 0; // Calculate automatically
       for(j=0; j<300; j++)
       {   calibdata[k].x[j] = oldx[k][j];  
           calibdata[k].y[j] = oldy[k][j];  
           calibdata[k].v[j] = oldv[k][j];  
       }
   }


   //// Dialog

   strcpy(dialog->title,"Image Calibration");
   strcpy(dialog->radio[0][0],"Calib. type");             
   strcpy(dialog->radio[0][1],"Non-directional");             
   strcpy(dialog->radio[0][2],"Directional");             

   strcpy(dialog->radio[1][0],"1st dim. calibration");
   strcpy(dialog->radio[1][1],"Linear");             
   strcpy(dialog->radio[1][2],"Logarithmic");
   strcpy(dialog->radio[1][3],"Polynomial");
   strcpy(dialog->radio[1][4],"Distance from 0");

   strcpy(dialog->radio[2][0],"2nd dim. calibration");
   strcpy(dialog->radio[2][1],"Linear");             
   strcpy(dialog->radio[2][2],"Logarithmic");
   strcpy(dialog->radio[2][3],"Polynomial");
   strcpy(dialog->radio[2][4],"Distance from 0");

   strcpy(dialog->radio[3][0],"z (pixel value calib)");
   strcpy(dialog->radio[3][1],"Linear");             
   strcpy(dialog->radio[3][2],"Logarithmic");
   strcpy(dialog->radio[3][3],"Polynomial");
   strcpy(dialog->radio[3][4],"Distance from 0");
   strcpy(dialog->radio[3][5],"Exponential");

   dialog->radioset[0] = g.calib_type;
   for(k=0; k<3; k++) dialog->radioset[k+1] = z[ino].cal_log[k]+1;

   strcpy(dialog->boxes[0],"Data");
   strcpy(dialog->boxes[1],"Image no.");
   strcpy(dialog->boxes[2],"Calib. params.");
   strcpy(dialog->boxes[3],"Units");
   strcpy(dialog->boxes[4],"Obtain data");
   strcpy(dialog->boxes[5],"Edit data");
   strcpy(dialog->boxes[6],"Recalculate");
   strcpy(dialog->boxes[7],"No. of terms");

   strcpy(dialog->boxes[8],"Slope");
   strcpy(dialog->boxes[9],"y intercept");
   strcpy(dialog->boxes[10],"0 order coeff");
   strcpy(dialog->boxes[11],"1 order coeff");
   strcpy(dialog->boxes[12],"2 order coeff");
   strcpy(dialog->boxes[13],"x origin");
   strcpy(dialog->boxes[14],"y origin");
   strcpy(dialog->boxes[15],"Statistics");
   strcpy(dialog->boxes[16],"f value");
   strcpy(dialog->boxes[17],"r² corr coef");

   strcpy(dialog->boxes[18],"Read Calib. Params.");
   strcpy(dialog->boxes[19],"Save Calib. Params.");

   for(k=0;k<=17;k++) dialog->boxtype[k]=MULTISTRING; 
   dialog->boxtype[0]=LABEL;
   dialog->boxtype[1]=INTCLICKBOX; 
   dialog->boxtype[2]=LABEL;
   dialog->boxtype[4]=MULTIPUSHBUTTON; 
   dialog->boxtype[5]=MULTIPUSHBUTTON; 
   dialog->boxtype[6]=MULTIPUSHBUTTON; 
   dialog->boxtype[7]=MULTIPUSHBUTTON; 
   dialog->boxtype[15]=LABEL;
   dialog->boxtype[18]=PUSHBUTTON;
   dialog->boxtype[19]=PUSHBUTTON;

   sprintf(dialog->answer[1][0],"%d",ino);
   dialog->boxmin[1]=0; dialog->boxmax[1]=g.image_count;

   for(k=4; k<=6; k++)
   {   strcpy(dialog->answer[k][0],"1st dim.");
       strcpy(dialog->answer[k][1],"2nd dim.");
       strcpy(dialog->answer[k][2],"pixel val.");
   }
   dialog->boxmin[7] = 1;
   dialog->boxmax[7] = 6;
   sprintf(dialog->answer[7][0],"%d", cal_term[0]);
   sprintf(dialog->answer[7][1],"%d", cal_term[1]);
   sprintf(dialog->answer[7][2],"%d", cal_term[2]);

   for(k=0; k<3; k++)
   {  
       strcpy( dialog->answer[3] [k],        z[ino].cal_title[k]);
       sprintf(dialog->answer[8] [k], "%g ", z[ino].cal_slope[k]);
       sprintf(dialog->answer[9] [k], "%g ", z[ino].cal_int[k]);
       sprintf(dialog->answer[10] [k], "%g ", z[ino].cal_q[k][0]);
       sprintf(dialog->answer[11][k], "%g ", z[ino].cal_q[k][1]);
       sprintf(dialog->answer[12][k], "%g ", z[ino].cal_q[k][2]);
       sprintf(dialog->answer[13][k], "%d ", z[ino].cal_xorigin[k]);
       sprintf(dialog->answer[14][k], "%d ", z[ino].cal_yorigin[k]);
       strcpy( dialog->answer[16][k], oldf[k]);
       strcpy( dialog->answer[17][k], oldr[k]);
   }

   dialog->radiono[0]=3;
   dialog->radiono[1]=5;
   dialog->radiono[2]=5;
   dialog->radiono[3]=6;
   dialog->radiono[4]=0;
   for(j=0;j<10;j++){ for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO; }
   for(k=0;k<20;k++) dialog->wcount[k]=3;

   dialog->noofradios = 4;
   dialog->noofboxes  = 20;
   dialog->helptopic  = 22;  
   dialog->want_changecicb = 0;
   dialog->f1 = calibcheck;
   dialog->f2 = null;
   dialog->f3 = null;
   dialog->f4 = calibfinish;
   dialog->f5 = null;
   dialog->f6 = null;
   dialog->f7 = null;
   dialog->f8 = null;
   dialog->f9 = null;
   dialog->width = 0;  // calculate automatically
   dialog->height = 0; // calculate automatically
   strcpy(dialog->path,".");
   dialog->transient = 1;
   dialog->wantraise = 0;
   dialog->radiousexy = 0;
   dialog->boxusexy = 0;
   dialog->hit = 0;
   dialog->ptr[0] = calibdata;
   dialog->message[0] = 0;      
   dialog->busy = 0;

   dialogbox(dialog);
   g.inmenu--;
   return;
}


//--------------------------------------------------------------------------//
//  calibcheck                                                              //
//--------------------------------------------------------------------------//
void calibcheck(dialoginfo *a, int radio, int box, int boxbutton)
{
   box=box; boxbutton=boxbutton; radio=radio;
   int dims=0, h, ino, j, k, ibpp, ii, value, oinmenu;
   double f, r2;
   XYData *data = (XYData*)a->ptr[0];
   g.getout = 0;
 
   ino = atoi(a->answer[1][0]);
   for(j=0; j<3; j++)
   {   strcpy(oldf[j], a->answer[16][j]);
       strcpy(oldr[j], a->answer[17][j]);
       z[ino].cal_log[j] = a->radioset[j+1] - 1; 
   }
   for(j=0; j<3; j++)
   for(k=0; k<300; k++)
   {   oldx[j][k] = data[j].x[k];
       oldy[j][k] = data[j].y[k];
       oldv[j][k] = data[j].v[k];
   }
   for(j=0; j<3; j++)
   {  oldn[j] = data[j].n;
   }

   g.calib_type = a->radioset[0];
   dims = 3; 
   z[ino].cal_dims = 1;
   if(boxbutton >=0) dimension = boxbutton;
   if(radio     >0)  dimension = radio - 1;
   if(between(dimension,0,2))
       z[ino].cal_log[dimension] = a->radioset[dimension + 1] - 1; 
   ino = data[dimension].ino;
   if(!a->hit) for(k=0;k<3;k++) update_calib_dialog(a, k);

   //// 1st 2 dims are coordinates
   //// data[k].dims should always be 1 for now
   for(k=0; k<3; k++) data[k].dims = dims - 2; 
   for(k=0; k<3; k++)  
   {   a->answer[3][k][18] = 0;   // Truncate string to 18 chars
       strcpy(z[ino].cal_title[k],  a->answer[3] [k]);
       cal_term[k]         =   atoi(a->answer[7][k]);
       z[ino].cal_slope[k] =   atof(a->answer[8] [k]);
       z[ino].cal_int[k]   =   atof(a->answer[9] [k]);
       z[ino].cal_q[k][0]  =   atof(a->answer[10] [k]);
       z[ino].cal_q[k][1]  =   atof(a->answer[11][k]);
       z[ino].cal_q[k][2]  =   atof(a->answer[12][k]);
       z[ino].cal_xorigin[k] = atoi(a->answer[13][k]);
       z[ino].cal_yorigin[k] = atoi(a->answer[14][k]);
   }
   switch(radio)
   {   case 1:
       case 2:
       case 3:
           update_calib_dialog(a, dimension);
           break;
   }
   switch(box)
   {
       case 4:      // obtain data
           data[dimension].n = 0;
           bezier_curve_start(&data[dimension], NULL);
           if(g.getout){ bezier_curve_end(&data[dimension]); return; }
           g.block++;
           while(g.bezier_state==CURVE)
                 XtAppProcessEvent(XtWidgetToApplicationContext(g.drawing_area),
                 XtIMAll);
           g.block = max(0, g.block-1);
           if(dimension==2)
           {   oinmenu=g.inmenu; 
               g.inmenu=0;
               for(k=0;k<data[2].n;k++)
               {  value = readpixelonimage(data[2].x[k], data[2].y[k], ibpp, ii);
                  data[2].x[k] = value;
               }
               g.inmenu=oinmenu;
           }
           // fall thru
       case 5:      //edit data
           get_calibration_data(&data[dimension]);     // Edit control points
           calibrate_calc(&data[dimension], dimension, f, r2); // This calls bezier_curve_end
           update_calib_dialog(a, dimension);
           sprintf(a->answer[16][dimension],"%g",f);
           sprintf(a->answer[17][dimension],"%g",r2);

           z[ino].cal_dims = (z[ino].cal_slope[0]!=0.0)*(z[ino].cal_int[0]!=0.0) + 
                             (z[ino].cal_slope[1]!=0.0)*(z[ino].cal_int[1]!=0.0); 

           break;
       case 6:      // Recalculate
           check_calibration_data(&data[dimension]);
           calibrate_calc(&data[dimension], dimension, f, r2);    
           sprintf(a->answer[16][dimension],"%g",f);
           sprintf(a->answer[17][dimension],"%g",r2);
           
           break;
       case 7:
           a->param[0] = dimension;
           getinteger("No. of terms", &cal_term[dimension], 1, 10, setcalterms, 
               null, a, 0);
           break;
       case 18: 
           read_calib(data);
           for(h=0;h<3;h++)
           {   if(data[h].n) calibrate_calc(&data[h], h, f, r2);
               copy_calib_values(a, h, ino);
               update_calib_dialog(a, h);
           }
           break;
       case 19: save_calib(data); break;
       default:
           if(data->n > 1)
           {  
               check_calibration_data(&data[dimension]);
               calibrate_calc(&data[dimension], dimension, f, r2); // This calls bezier_curve_end
               update_calib_dialog(a, dimension);
           }
           break;
   }
   if(between(box, 4, 7))
   {   calibrate_calc(&data[dimension], dimension, f, r2);    
       sprintf(a->answer[16][dimension],"%g",f);
       sprintf(a->answer[17][dimension],"%g",r2);
   }
   update_calib_dialog(a, dimension);
}


//--------------------------------------------------------------------------//
// setcalterms                                                              //
//--------------------------------------------------------------------------//
void setcalterms(clickboxinfo *c)
{
  char tempstring[32];
  double f, r2;
  Dialog *a = (Dialog*)c->client_data;
  XYData *data = (XYData*)a->ptr[0];
  int dim = a->param[0];
  cal_term[dim] = c->answer;
  calibrate_calc(&data[dim], dim, f, r2);    
  sprintf(a->answer[16][dim],"%g",f);
  sprintf(a->answer[17][dim],"%g",r2);
  update_calib_dialog(a, dim);
  itoa(cal_term[dim], tempstring, 10);
  set_widget_label(a->boxwidget[7][dim], tempstring);
}


//--------------------------------------------------------------------------//
// check_calibration_data                                                   //
//--------------------------------------------------------------------------//
void check_calibration_data(XYData *data)
{
  int k, bad=1, nodata=1;
  for(k=0; k<data->n; k++) if(data->x[k] || data->y[k]) nodata=0;
  for(k=0; k<data->n; k++) if(data->v[k]) bad=0;
  if(nodata) message("No calibration points defined",ERROR);
  else if(bad) message("No calibration values found.\nClick on Edit Data and\nenter calibration values.",ERROR);
}


//--------------------------------------------------------------------------//
//  calibfinish                                                             //
//--------------------------------------------------------------------------//
void calibfinish(dialoginfo *a)
{
   if(!in_calibrate) return;
   XYData *data = (XYData*)a->ptr[0];
   for(int j=0; j<3; j++)
   {   delete[] data[j].v;
       delete[] data[j].y;
       delete[] data[j].x;
   }
   in_calibrate = 0;
}


//-------------------------------------------------------------------------//
// update_calib_dialog                                                     //
//-------------------------------------------------------------------------//
void update_calib_dialog(Dialog *a, int j)
{  
   Widget w;
   int i,k;
   int ino = atoi(a->answer[1][0]);
   int curpos;
   a->hit = 1;
   for(i=0;i<a->noofradios;i++)
   {   
        if(a->radioset[i] > 0)
        {    w = a->radiowidget[i][a->radioset[i]];
             XmToggleButtonSetState(w, True, True); 
        }
   }
   for(k=3;k<=17;k++)
   {    if(k==15) continue;
        if(between(k,4,7)) continue;
        //// Get rid of callback or it will go into a loop
        w = a->boxwidget[k][j];
        XtRemoveCallback(w, XmNvalueChangedCallback, 
          (XtCBP)dialogstringcb, (XtP)a);
        XtVaGetValues(w, XmNcursorPosition, &curpos, NULL);
        XtVaSetValues(w, XmNvalue, a->answer[k][j], 
                         XmNcursorPosition, curpos, 
                         NULL); 
        XtAddCallback(a->boxwidget[k][j], XmNvalueChangedCallback, 
            (XtCBP)dialogstringcb, (XtP)a);
   }
   copy_calib_values(a, j, ino);
}


//-------------------------------------------------------------------------//
// copy_calib_values                                                       //
//-------------------------------------------------------------------------//
void copy_calib_values(Dialog *a, int j, int ino)
{
   int k;
   for(k=0; k<3; k++) a->radioset[k+1] = 1 + z[ino].cal_log[k];
   strcpy( a->answer[3] [j],        z[ino].cal_title[j]);
   sprintf(a->answer[7] [j], "%d ", cal_term[j]);
   sprintf(a->answer[8] [j], "%g ", z[ino].cal_slope[j]);
   sprintf(a->answer[9] [j], "%g ", z[ino].cal_int[j]);
   sprintf(a->answer[10] [j], "%g ", z[ino].cal_q[j][0]);
   sprintf(a->answer[11][j], "%g ", z[ino].cal_q[j][1]);
   sprintf(a->answer[12][j], "%g ", z[ino].cal_q[j][2]);
   sprintf(a->answer[13][j], "%d ", z[ino].cal_xorigin[j]);
   sprintf(a->answer[14][j], "%d ", z[ino].cal_yorigin[j]);
}


//-------------------------------------------------------------------------//
// get_calibration_data                                                    //
//-------------------------------------------------------------------------//
void get_calibration_data(XYData *data)
{
   int h, k, xx, yy;
   char title[64], **label;  
   char ***answer;
   char **headings;
   int cols;      // no of columns for getstrings
   int ino  = data->ino;
   char tempstring[64];
   if(dimension==2) cols=2; else cols=3;
   strcpy(title,"Calibration values");
   label    = new char *[data->n];
   answer   = new char **[cols];
   headings = new char *[cols];
   for(h=0; h<cols; h++)
   {   answer[h] = new char*[300];
       headings[h] = new char[128];
   }
   if(dimension==2)
   {   strcpy(headings[0], "pixel val.");
       strcpy(tempstring, z[ino].cal_title[dimension]);
       tempstring[8] = 0;
       strcpy(headings[1], tempstring);
   }else
   {   strcpy(headings[0], "x coord.");
       strcpy(headings[1], "y coord.");
       strcpy(tempstring, z[ino].cal_title[dimension]);
       tempstring[8] = 0;
       strcpy(headings[2], tempstring);
   }

   for(k=0; k<data->n; k++)
   {  label[k] = new char[128];
      sprintf(label[k], "Calib. Point #%d", k+1);
      xx = data->x[k];
      yy = data->y[k];
      if(ino>=0)                 // relative coord. w.r.t. upper left of image
      {   if(z[ino].is_zoomed)
          {   xx =  zoom_x_index(xx, ino);
              yy =  zoom_y_index(yy, ino);
          }else
          {  
//              xx = xx - z[ino].xpos;
//              yy = yy - z[ino].ypos;
          }
      }

      for(h=0; h<cols; h++) answer[h][k] = new char[128];
      if(dimension==2)
      {   sprintf(answer[0][k], "%d", xx);
          sprintf(answer[1][k], "%g", data->v[k]);
      }else
      {   sprintf(answer[0][k], "%d", xx);
          sprintf(answer[1][k], "%d", yy);
          sprintf(answer[2][k], "%g", data->v[k]);
      }
   }

   getstrings(title, label, headings, answer, cols, data->n, 128);

   if(ino<0) message("Error: image less than 0", BUG);
   for(k=0; k<data->n; k++)
   {  
      xx = atoi(answer[0][k]) + z[ino].xpos;
      yy = atoi(answer[1][k]) + z[ino].ypos;

      data->x[k] = xx;
      if(dimension==2)
          data->v[k] = atof(answer[1][k]);
      else
      {   data->y[k] = yy;
          data->v[k] = atof(answer[2][k]);
       }
   }

   for(k=0;k<cols;k++) delete[] headings[k];
   for(k=0;k<data->n;k++) delete[] label[k];
   for(h=0; h<cols; h++) 
   {   for(k=0; k<data->n; k++) delete[] answer[h][k]; 
       delete answer[h];
   }
   delete[] answer;
   delete[] label;
}

//-------------------------------------------------------------------------//
// calibrate_calc                                                          //
// calculate calibration coeffs from user data                             //
// If calibrating background, calib. is fixed.                             //
// Calibrate 1 dimension at a time                                         //
//-------------------------------------------------------------------------//
void calibrate_calc(XYData *data, int dim, double &f, double &r2)
{
    double x1,y1,m1,m2,b2;
    double *x, *y, *v, *d, q[20], *calib;
    int n, k, ino, calib_log;
    g.getout=0;
    q[0]=0;
    q[1]=0;
    q[2]=0;
    ino = data->ino;
    n = data->n;    
    calib = new double[n+2];
    calib_log = z[ino].cal_log[dim];
    for(k=0; k<n; k++) calib[k] = data->v[k];

    if(calib_log==1)   // logarithmic calib
    {  for(k=0; k<n; k++) 
       {  
          if(calib[k]>0) calib[k] = log10(calib[k]);
          else
          {    message("Error: negative value in logarithm",ERROR);
               calib[k]=0;
               break;
          }
       }
    }      

    x = new double[n+1];
    y = new double[n+1];
    v = new double[n+1];
    d = new double[n+1];

    //// Correlate calib[] with distance from line CD perpendicular to AB.  
    ////                   D       slope of AB = m1                         
    ////                 A/        slope of AC = -1/m1 = m2                 
    ////                 /\        intercept of AB = b1                     
    ////               F/  \E      intercept of CD = b2                     
    ////               /\./ \      A is the origin point, obtained from     
    ////              C  P   B      linear regression of x's and y's.       
    //// For z dimension, correlate calib[] with pixel values.

    for(k=0;k<n;k++)
    {   x[k] = data->x[k] - z[ino].xpos;  
        y[k] = data->y[k] - z[ino].ypos; 
        v[k] = data->x[k];

             ////  For 2-dim calib, force axes to be almost vertical & horizontal
             ////  but not perfect to avoid slope of infinity.
             //// NOT used         
        if(z[ino].cal_dims>=2)  
           if(dim==0) y[k]=0.0001+k*.0001; else x[k]=0.0001+k*.0001;    
    }
    if(calib_log==4)  // exponential calib
    {  for(k=0; k<n; k++) 
       {  
          if(calib[k]>0 && v[k]>0)
          {    calib[k] = log(calib[k]);
               v[k] = log(v[k]);
          }else
          {    message("Error: negative value in logarithm",ERROR);
               v[k] = calib[k] = 0;
               break;
          }
       }
    }      

    if(dim==2)                   // Calibrate z
    {   linreg(v,calib,n,cal_term[2],q,f,r2);  
        m2 = q[1];
        b2 = q[0];
        x1 = y1 = 0;
    }else
    {   linreg(x,y,n,1,q,f,r2);  // Calculate the line AB
        m1 = q[1]; if(m1==0) m1=1e-6;
        ////  b1 = q[0];         // Slope of line segment
        x1 = data->x[0];         // Origin point x1,y1
        y1 = q[0] + q[1]*x1;     // Origin point
        m2 = -1/m1;              // Slope of perp. line thru origin point
        b2 = y1-m2*x1;           // Intercept of CD thru origin point
    
        for(k=0;k<n;k++)         // For each calibr. point P, calc. dist.to AB.
        {   if(data->dims==1) 
               d[k] = pointtoline(x[k],y[k],m2,b2); // Length of FP
            else if(dim==0)
               d[k] = x[k];
            else
               d[k] = y[k];
        }

//        if(calib_log==2)              
//          linreg(d,calib,n,2,q,f,r2); // Correlate data points with perp.distances
//        else
          linreg(d,calib,n,cal_term[dim],q,f,r2); // Correlate data points with perp.distances
    }

    z[ino].cal_slope[dim] = m2;   // slope of perp.line
    z[ino].cal_int[dim]   = b2;   // intercept of perp.line
    z[ino].cal_q[dim][0] = q[0]; 
    z[ino].cal_q[dim][1] = q[1]; 
    z[ino].cal_q[dim][2] = q[2]; 
    z[ino].cal_xorigin[dim] = (int)x1;
    z[ino].cal_yorigin[dim] = (int)y1;

    delete[] d;
    delete[] v;
    delete[] y;
    delete[] x;
    return;
}


//-------------------------------------------------------------------------//
// calibratepixel - returns calibrated value from x,y of a pixel in 'c'    //
// and also puts it in the string 'tempstring'.   ux and uy are raw screen //
// coordinates.  ino is image no, dim must be 0, 1, or 2 (=which calib.)   //
//-------------------------------------------------------------------------//
void calibratepixel(int ux, int uy, int ino, int dim, double &c, char* answerstring)
{
    double cdist=0;
    double slope,intercept,q0=0,q1=0,q2=0;
    int calib_log, ibpp, ii, v, oinmenu;
    int cal_xo=0,cal_yo=0;
    if(ino<0) return;
    if(!between(dim,0,2)){ message("Ouch!", BUG); return; } 
    if(z[ino].cal_log[dim]<0){ c=0; strcpy(answerstring,"Uncalibrated"); return; }
    c = 0.0;
    if(z[ino].is_zoomed)
    {    ux = zoom_x_index(ux, ino);
         uy = zoom_y_index(uy, ino);
    }else
    {  
        ux -= z[ino].xpos;
        uy -= z[ino].ypos;
    }
    slope = z[ino].cal_slope[dim];
    intercept = z[ino].cal_int[dim];
    calib_log = z[ino].cal_log[dim];
    if(calib_log>-1)
    {   q0 = z[ino].cal_q[dim][0];
        q1 = z[ino].cal_q[dim][1];
        q2 = z[ino].cal_q[dim][2];
        cal_xo = z[ino].cal_xorigin[dim];
        cal_yo = z[ino].cal_yorigin[dim];
    }
    answerstring[0]=0;

    if(dim==2)
    {  oinmenu=g.inmenu; 
       g.inmenu = 0;
       v = readpixelonimage(ux,uy,ibpp,ii);
       g.inmenu = oinmenu;
       cdist = (double)v;
       ux = uy = v;
    }else
    {  if(z[ino].cal_dims==0)
          cdist = ux;
       else if(z[ino].cal_dims>=1) 
          cdist = pointtoline((double)ux, (double)uy, slope, intercept);
       else if(dim==0)
          cdist = ux;
       else
          cdist = uy;
    }

    switch(calib_log) 
    { 
         case 4:         // exponential
            c = q0 + pow(cdist, q1);
            doubletostring(c,g.signif,answerstring);
            break;
         case 3:          // distance from 0
            ux = ux - cal_xo;
            uy = uy - cal_yo;
            c = q0 + fabs(q1 * sqrt((double)(ux*ux + uy*uy)));
            doubletostring(c,g.signif,answerstring);
            break;
         case 2:         // polynomial
            c = q0 + cdist*q1 + cdist*cdist*q2;
            doubletostring(c,g.signif,answerstring);
            break;
         case 1:         // logarithmic
            c = q0 + cdist*q1 + cdist*cdist*q2;
            if(c>19)strcpy(answerstring,"Infinity");
            else if(c<-10)strcpy(answerstring,"Zero");
            else
            {   c=pow(10.0,c);
                doubletostring(c,g.signif,answerstring);
            }
            break;
         case 0:          // linear
            c = q0 + cdist*q1;
            doubletostring(c,g.signif,answerstring);
            break;
         case -1:
         default:
            strcpy(answerstring,"Uncalibrated");
            break;
    } 
}


//-------------------------------------------------------------------------//
// pointtoline - Calculates distance from point x,y to line y=mx+b         //
//-------------------------------------------------------------------------//
double pointtoline(double x, double y, double m, double b)
{
    double m2,b2,x2=0,y2,d;
    if(m==0) m=1e-6;
    m2 = -1/m;              // slope of perpendicular
    b2 = y-m2*x;            // y-int.of perpendicular
    if(m2!=m)
      x2 = (b-b2)/(m2-m);   // intersection x coord.
    y2 = m2*x2+b2;          // intersection y coord.
    d = sqrt((x-x2)*(x-x2)+(y-y2)*(y-y2));  // distance
    if( ((y-y2)/m)+x-x2 > 0) return(d);     // if x is right of line, d>0
    return(-d);
}


//-------------------------------------------------------------------------//
// read_calib                                                              //
//-------------------------------------------------------------------------//
void read_calib(XYData *data)
{
  FILE *fp;
  int h,k,ino;
  g.getout=0;
  static char filename[FILENAMELENGTH] = "Calibration";
  static char oldfilename[FILENAMELENGTH] = "Calibration";
  char junk[FILENAMELENGTH];
  strcpy(filename, getfilename(oldfilename, NULL));
  ino = data[0].ino;
  if(g.getout) return;
  if ((fp=fopen(filename,"r")) == NULL)
  {   error_message(filename, errno);
      return;
  }
  fgets(junk, FILENAMELENGTH, fp);
  for(h=0;h<3;h++)
  {   fgets(z[ino].cal_title[h], FILENAMELENGTH, fp);  
      remove_terminal_cr(z[ino].cal_title[h]);
      fscanf(fp,"%d\n", &data[h].n);
      if(data[h].n) for(k=0; k<data[h].n; k++) fscanf(fp,"%d\t%d\t%lg\n",
          &data[h].x[k], &data[h].y[k], &data[h].v[k]);
  }
  for(h=0;h<3;h++)
  {   // need %lg for scanf buf %g for fprintf
      fscanf(fp,"%lg", &z[ino].cal_slope[h]);
      fscanf(fp,"%lg", &z[ino].cal_int[h]);
      fscanf(fp,"%lg", &z[ino].cal_q[h][0]);
      fscanf(fp,"%lg", &z[ino].cal_q[h][1]);
      fscanf(fp,"%lg", &z[ino].cal_q[h][2]);
      fscanf(fp,"%lg", &z[ino].cal_q[h][3]);
      fscanf(fp,"%d", &z[ino].cal_xorigin[h]);
      fscanf(fp,"%d", &z[ino].cal_yorigin[h]);
      fscanf(fp,"%d", &z[ino].cal_log[h]);
  }
  fclose(fp);  
  return;
}


//-------------------------------------------------------------------------//
// save_calib                                                              //
//-------------------------------------------------------------------------//
void save_calib(XYData *data)
{
  static PromptStruct ps;
  static char filename[FILENAMELENGTH] = "Calibration";
  if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
  int status=YES, ino = data[0].ino;
  status = message("New filename:",filename,PROMPT,FILENAMELENGTH-1,54);
  if(status!=YES) return;
  ps.filename = filename;
  ps.f1 = save_calib_part2;
  ps.f2 = null;
  ps.ino = ino;
  ps.ptr = (void*)data;
  check_overwrite_file(&ps);
}


//-------------------------------------------------------------------------//
// save_calib_part2                                                        //
//-------------------------------------------------------------------------//
void save_calib_part2(PromptStruct *ps)
{
  FILE *fp;
  int h, k, ino;
  char *filename = ps->filename;
  char temp[FILENAMELENGTH];
  XYData *data = (XYData *)ps->ptr;
  ino = ps->ino;
  if((fp=fopen(filename,"w")) == NULL)
  {    error_message(filename, errno);
       return;
  }
  fprintf(fp,"Calibration of %s\n", z[ino].name);
  for(h=0;h<3;h++)
  {   fprintf(fp,"%s\n", z[ino].cal_title[h]);  
      fprintf(fp,"%d\n", data[h].n);
      for(k=0; k<data[h].n; k++) fprintf(fp,"%d\t%d\t%g\n",
          data[h].x[k], data[h].y[k], data[h].v[k]);
  }
  for(h=0;h<3;h++)
  {   
      fprintf(fp,"%g\n", z[ino].cal_slope[h]);
      fprintf(fp,"%g\n", z[ino].cal_int[h]);
      fprintf(fp,"%g\n", z[ino].cal_q[h][0]);
      fprintf(fp,"%g\n", z[ino].cal_q[h][1]);
      fprintf(fp,"%g\n", z[ino].cal_q[h][2]);
      fprintf(fp,"%g\n", z[ino].cal_q[h][3]);
      fprintf(fp,"%d\n", z[ino].cal_xorigin[h]);
      fprintf(fp,"%d\n", z[ino].cal_yorigin[h]);
      fprintf(fp,"%d\n", z[ino].cal_log[h]);
  }
  sprintf(temp,"Data saved in %s",filename);
  message(temp);
  fclose(fp);
  return;
}

