//--------------------------------------------------------------------------//
// xmtnimage43.cc                                                           //
// Latest revision: 10-18-2004                                              //
// Copyright (C) 2003 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
// grain counting                                                           //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"
extern Globals     g;
extern Image      *z;
extern int         ci;
Pattern opattern, npattern;
inline int pixel_below_threshold(int v, int vpat, int pix_bpp, 
   int pat_bpp, int pattern_type, int is_color, int threshold);
enum{ EDM, BUF };
enum{ NOTHING, SEGMENTATION, NEURAL, DIFFERENCING };
const int MAXGRAINS = 20000;   //// Change this number to enlarge
int hit_size_distrib = 0;      //// Flag if arrays allocated
int grain_count;               //// No. of grains found
int *grain;                    //// size of up to MAXGRAINS grains
double *grain_signal;          //// signal of each grain
int *grain_size;               //// size distribution
int *show_grain_size;          //// size distribution scaled for graph
int *grain_signal_distribution;//// signal distribution in 256 bins

int **grain_coordinate;        //// 0,1 = center x,y of each grain
                               //// 2,3 = upper left x,y of each grain bounding box
                               //// 4,5 = lower right x,y of each grain bounding box
int in_pattern_recognition=0;
PlotData *pd1;
PlotData *pd2;
int grain_biggest=0;
double grain_biggest_signal=0.0;
double grain_xdensdisplayfac = 1.0;
int grain_ino=0, grain_maxcount=0, grain_most_frequent_size=0;


//-------------------------------------------------------------------------//
// pixel_below_threshold                                                   //
// Use this function only for testing color pixels, where the user doesn't //
// care how long it takes. Part of this is duplicated above in findpattern.//
//-------------------------------------------------------------------------//
inline int pixel_below_threshold(int v, int vpat, int pix_bpp, int pat_bpp, 
   int pattern_type, int is_color, int threshold)
{
   int diff, rr, gg, bb, pat_rr, pat_gg, pat_bb, t3;
   if(pattern_type==GRAIN)
   {   if(is_color)
       {    valuetoRGB(v, rr, gg, bb, pix_bpp);
            valuetoRGB(vpat, pat_rr, pat_gg, pat_bb, pat_bpp);
            t3 = threshold + threshold + threshold; 
            if(rr + gg + bb <= t3 &&
                 pat_rr + pat_gg + pat_bb <= t3)
                 return 1;
            if(rr + gg + bb > t3 &&
                pat_rr + pat_gg + pat_bb > t3)
                 return 0;
       }else
       {    if(v <= threshold && vpat <= threshold)  // match and grain
                return 1;
            if(v > threshold && vpat > threshold)    // match not grain
                return 0;
            return 0;
       }
   }else      // PATTERN
   {   if(is_color)
       {    valuetoRGB(v, rr, gg, bb, pix_bpp);
            valuetoRGB(vpat, pat_rr, pat_gg, pat_bb, pat_bpp);
            t3 = threshold + threshold + threshold; 
            diff = abs(rr-pat_rr) + abs(gg-pat_gg) + abs(bb-pat_bb);
            if(diff < t3)
                return 1;
            else 
                return 0;
       }else
       {    if(abs(v - vpat) <= threshold)
                return 1;
            else 
                return 0;
       }
   }
   return 0;
}


//-------------------------------------------------------------------------//
//  pattern_recognition                                                    //
//-------------------------------------------------------------------------//
void pattern_recognition(void)
{
#define LABELTYPES 4
  static Dialog *dialog;
  int j,k;
  double y,dx,dy;
  if(in_pattern_recognition) return;
  in_pattern_recognition = 1;
  dialog = new Dialog;
  if(dialog==NULL){ message(g.nomemory); in_pattern_recognition = 0; return; }
  strcpy(dialog->title,"Grain/Pattern Counting");

  //-----Dialog box to get title & other options------------//

  strcpy(dialog->radio[0][0],"Method");
  strcpy(dialog->radio[0][1],"Quick segmentation");
  strcpy(dialog->radio[0][2],"Neural network");
  strcpy(dialog->radio[0][3],"Differencing");
  dialog->radiono[0]=4;
  dialog->radioset[0] = g.pattern_method;

  strcpy(dialog->radio[1][0],"Begin Operation");
  strcpy(dialog->radio[1][1],"Select grain/pattern");
  strcpy(dialog->radio[1][2],"Read pattern from disk");
  strcpy(dialog->radio[1][3],"Save pattern on disk");
  strcpy(dialog->radio[1][4],"Count grains");             
  strcpy(dialog->radio[1][5],"Count std. size grains");   
  strcpy(dialog->radio[1][6],"Count patterns");             
  strcpy(dialog->radio[1][7],"Enhance grains");   
  strcpy(dialog->radio[1][8],"Save results");   
  strcpy(dialog->radio[1][9],"Restore original");   
  dialog->radiono[1]=10;
  dialog->radioset[1] = -1;
  
  strcpy(dialog->boxes[0],"Parameters");
  strcpy(dialog->boxes[1],"Filename");
  strcpy(dialog->boxes[2],"Filename");
  strcpy(dialog->boxes[3],"Threshold");
  strcpy(dialog->boxes[4],"Threshold");

  strcpy(dialog->boxes[5],"Weight (-1 to 1)");
  strcpy(dialog->boxes[6],"Match");
  strcpy(dialog->boxes[7],"Match");
  strcpy(dialog->boxes[8],"Mismatch");
  strcpy(dialog->boxes[9],"Mismatch");

  strcpy(dialog->boxes[10],"Size");
  strcpy(dialog->boxes[11],"Size");
  strcpy(dialog->boxes[12],"Label color");
  strcpy(dialog->boxes[13],"Label color");

  strcpy(dialog->boxes[14],"Label");
  strcpy(dialog->boxes[15],"Sz Graph");
  strcpy(dialog->boxes[16],"Dens Graph");
  strcpy(dialog->boxes[18],"Min Size");
  strcpy(dialog->boxes[20],"Max Size");
  strcpy(dialog->boxes[21],"Rectangles");

  dialog->boxtype[0]=LABEL;
  dialog->boxtype[1]=FILENAME;
  dialog->boxtype[2]=LABEL;
  dialog->boxtype[3]=STRING;
  dialog->boxtype[4]=LABEL;
  dialog->boxtype[5]=LABEL;
  dialog->boxtype[6]=STRING;
  dialog->boxtype[7]=LABEL;
  dialog->boxtype[8]=STRING;
  dialog->boxtype[9]=LABEL;
  dialog->boxtype[10]=INTCLICKBOX;
  dialog->boxtype[11]=LABEL;
  dialog->boxtype[12]=INTCLICKBOX;
  dialog->boxtype[13]=LABEL;
  dialog->boxtype[14]=MULTITOGGLE;
  dialog->boxtype[15]=TOGGLE;
  dialog->boxtype[16]=TOGGLE;
  dialog->boxtype[17]=STRING;
  dialog->boxtype[18]=LABEL;
  dialog->boxtype[19]=STRING;
  dialog->boxtype[20]=LABEL;
  dialog->boxtype[21]=TOGGLE;
  strcpy( dialog->answer[1][0], g.pattern_filename);
  sprintf(dialog->answer[3][0], "%g", g.pattern_thresh); 
  sprintf(dialog->answer[6][0], "%g", g.pattern_pweight);
  sprintf(dialog->answer[8][0], "%g", g.pattern_bweight);
  sprintf(dialog->answer[10][0],"%d", g.pattern_size);
  sprintf(dialog->answer[12][0],"%d", g.pattern_labelcolor);
  dialog->boxmin[10]=1; dialog->boxmax[10]=20;
  dialog->boxmin[12]=0; dialog->boxmax[12]=(int)g.maxvalue[z[ci].bpp];

  dialog->boxcount[14] = 9;
  dialog->boxlist[14] = new char*[dialog->boxcount[14]];
  dialog->boxset[14] = g.pattern_labelflags;
  for(k=0; k<dialog->boxcount[14]; k++) dialog->boxlist[14][k] = new char[64];
  strcpy(dialog->boxlist[14][0], "Label"); 
  strcpy(dialog->boxlist[14][1], "Grain number"); 
  strcpy(dialog->boxlist[14][2], "Grain size"); 
  strcpy(dialog->boxlist[14][3], "Total grain signal"); 
  strcpy(dialog->boxlist[14][4], "X position"); 
  strcpy(dialog->boxlist[14][5], "Y position"); 
  strcpy(dialog->boxlist[14][6], "1st dimension calib."); 
  strcpy(dialog->boxlist[14][7], "2nd dimension calib."); 
  strcpy(dialog->boxlist[14][8], "Calibrated pixel value"); 

  dialog->boxset[15] = g.pattern_wantgraph;
  dialog->boxset[16] = g.pattern_wantdensgraph;
  sprintf(dialog->answer[17][0],"%d", g.pattern_minsize);
  sprintf(dialog->answer[19][0],"%d", g.pattern_maxsize);

  dialog->boxset[21] = g.pattern_wantrectangles;

  for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;  

  //// Use a custom format dialog box
  y = 310; dx = 90; dy = 20;  //// no point making dy a non integer, Motif still truncates it
  
  dialog->boxxy[0][0] = 6;    dialog->boxxy[0][1] = y;    // Parameters label
  dialog->boxxy[0][2] = dx;   dialog->boxxy[0][3] = dy;
  y += dy;
  dialog->boxxy[1][0] = 6;    dialog->boxxy[1][1] = y;    // Filename
  dialog->boxxy[1][2] = dx;   dialog->boxxy[1][3] = dy;
  dialog->boxxy[2][0] = 100;  dialog->boxxy[2][1] = y;
  dialog->boxxy[2][2] = dx;   dialog->boxxy[2][3] = dy;
  y += dy;
  dialog->boxxy[3][0] = 6;    dialog->boxxy[3][1] = y;    // Threshold
  dialog->boxxy[3][2] = dx;   dialog->boxxy[3][3] = dy;
  dialog->boxxy[4][0] = 100;  dialog->boxxy[4][1] = y;
  dialog->boxxy[4][2] = dx;   dialog->boxxy[4][3] = dy;
  y += dy;
  dialog->boxxy[17][0] = 6;    dialog->boxxy[17][1] = y;  // Min size
  dialog->boxxy[17][2] = dx;   dialog->boxxy[17][3] = dy;
  dialog->boxxy[18][0] = 100;  dialog->boxxy[18][1] = y;
  dialog->boxxy[18][2] = dx;   dialog->boxxy[18][3] = dy;
  y += dy;
  dialog->boxxy[19][0] = 6;    dialog->boxxy[19][1] = y;  // Max size
  dialog->boxxy[19][2] = dx;   dialog->boxxy[19][3] = dy;
  dialog->boxxy[20][0] = 100;  dialog->boxxy[20][1] = y;
  dialog->boxxy[20][2] = dx;   dialog->boxxy[20][3] = dy;

  y += dy;
  dialog->boxxy[5][0] = 6;    dialog->boxxy[5][1] = y;    // Weight -1 to 1 label
  dialog->boxxy[5][2] = dx;   dialog->boxxy[5][3] = dy;
  y += dy;
  dialog->boxxy[6][0] = 6;    dialog->boxxy[6][1] = y;    // Match
  dialog->boxxy[6][2] = dx;   dialog->boxxy[6][3] = dy;
  dialog->boxxy[7][0] = 100;  dialog->boxxy[7][1] = y;
  dialog->boxxy[7][2] = dx;   dialog->boxxy[7][3] = dy;
  y += dy;
  dialog->boxxy[8][0] = 6;    dialog->boxxy[8][1] = y;    // Mismatch
  dialog->boxxy[8][2] = dx;   dialog->boxxy[8][3] = dy;
  dialog->boxxy[9][0] = 100;  dialog->boxxy[9][1] = y;
  dialog->boxxy[9][2] = dx;   dialog->boxxy[9][3] = dy;
  y += dy;
  dialog->boxxy[10][0] = 6;   dialog->boxxy[10][1] = y;   // Size
  dialog->boxxy[10][2] = dx;  dialog->boxxy[10][3] = dy;
  dialog->boxxy[11][0] = 100; dialog->boxxy[11][1] = y;
  dialog->boxxy[11][2] = dx;  dialog->boxxy[11][3] = dy;
  y += dy;
  dialog->boxxy[12][0] = 6;   dialog->boxxy[12][1] = y;   // Label color
  dialog->boxxy[12][2] = dx;  dialog->boxxy[12][3] = dy;
  dialog->boxxy[13][0] = 100; dialog->boxxy[13][1] = y;   // Label color label
  dialog->boxxy[13][2] = dx;  dialog->boxxy[13][3] = dy;
  y += dy;
  y += dy/2;
  dialog->boxxy[14][0] = 6;   dialog->boxxy[14][1] = y;   // Label toggle button
  dialog->boxxy[14][2] = 76;  dialog->boxxy[14][3] = dy;
  dialog->boxxy[21][0] = 85;  dialog->boxxy[21][1] = y;   // Label toggle button
  dialog->boxxy[21][2] = 76;  dialog->boxxy[21][3] = dy;
  y += dy;
  dialog->boxxy[15][0] = 6;   dialog->boxxy[15][1] = y;   // Size graph toggle button
  dialog->boxxy[15][2] = 76;  dialog->boxxy[15][3] = dy;
  dialog->boxxy[16][0] = 85;  dialog->boxxy[16][1] = y;   // Dens graph toggle button
  dialog->boxxy[16][2] = 76;  dialog->boxxy[16][3] = dy;
  y += dy;

  dialog->spacing = 0;
  dialog->radiostart = 2;
  dialog->radiowidth = 50;
  dialog->boxstart = 2;
  dialog->boxwidth = 50;
  dialog->labelstart = 87;
  dialog->labelwidth = 50;        


  dialog->want_changecicb = 0;
  dialog->f1 = patterncheck;
  dialog->f2 = null;
  dialog->f3 = null;
  dialog->f4 = patternfinish;
  dialog->f5 = null;
  dialog->f6 = null;
  dialog->f7 = null;
  dialog->f8 = null;
  dialog->f9 = null;
  dialog->width = 170;
  dialog->height = 590;
  dialog->noofradios=2;
  dialog->noofboxes=22;
  dialog->helptopic=50;
  dialog->transient = 1;
  dialog->wantraise = 0;
  dialog->radiousexy = 0;
  dialog->boxusexy = 1;
  strcpy(dialog->path,".");
  dialog->message[0]=0;      
  dialog->busy = 0;
  dialogbox(dialog);
}


//--------------------------------------------------------------------------//
//  patternfinish                                                           //
//--------------------------------------------------------------------------//
void patternfinish(dialoginfo *a)
{
  int k;
  a=a;
  if(!in_pattern_recognition) return;
  if(opattern.xsize) free_pattern(&opattern);
  if(npattern.xsize) free_pattern(&npattern);
  in_pattern_recognition = 0;

  for(k=0; k<a->boxcount[14]; k++) delete[] a->boxlist[14][k];
  delete[] a->boxlist[14];
  return;
}


//--------------------------------------------------------------------------//
//  patterncheck                                                            //
//--------------------------------------------------------------------------//
void patterncheck(dialoginfo *a, int radio, int box, int boxbutton)
{
   int k;
   static int inpatterncheck=0;
   strcpy(g.pattern_filename, a->answer[1][0]);
   g.pattern_method     = a->radioset[0];
   g.pattern_thresh     = max(0.0, min(1.0, atof(a->answer[3][0])));
   g.pattern_pweight    = atof(a->answer[6][0]);
   g.pattern_bweight    = atof(a->answer[8][0]);
   g.pattern_size       = atoi(a->answer[10][0]);
   g.pattern_labelcolor = atoi(a->answer[12][0]);
   g.pattern_labelflags      = a->boxset[14];
   g.pattern_wantgraph       = a->boxset[15];
   g.pattern_wantdensgraph   = a->boxset[16];
   g.pattern_minsize         = atoi(a->answer[17][0]);
   g.pattern_maxsize         = atoi(a->answer[19][0]);
   g.pattern_wantrectangles  = a->boxset[21];

   if(inpatterncheck) return;  //// XtSetValues->dialogstringcb->fftcheck
   inpatterncheck=1;

   box=box; boxbutton=boxbutton;
   if(radio==0) switch(a->radioset[0])
   {   case 2:   // neural network
         if(a->boxwidget[1][0]) XtSetSensitive(a->boxwidget[1][0],True);
         for(k=0; k<=2; k++) 
         {  if(a->boxwidget[6][k]) XtSetSensitive(a->boxwidget[6][k],True);
            if(a->boxwidget[8][k]) XtSetSensitive(a->boxwidget[8][k],True);
            if(a->boxwidget[10][k]) XtSetSensitive(a->boxwidget[10][k],False);
            if(a->boxwidget[15][k]) XtSetSensitive(a->boxwidget[15][k],False);
            if(a->boxwidget[16][k]) XtSetSensitive(a->boxwidget[16][k],True);
            if(a->boxwidget[17][k]) XtSetSensitive(a->boxwidget[17][k],False);
            if(a->boxwidget[18][k]) XtSetSensitive(a->boxwidget[18][k],False);
         }
         if(a->radiowidget[1][1]) XtSetSensitive(a->radiowidget[1][1],True);
         if(a->radiowidget[1][2]) XtSetSensitive(a->radiowidget[1][2],True);
         if(a->radiowidget[1][3]) XtSetSensitive(a->radiowidget[1][3],True);
         if(a->radiowidget[1][6]) XtSetSensitive(a->radiowidget[1][6],True);
         if(a->radiowidget[1][10]) XtSetSensitive(a->radiowidget[1][10],False);
         break;
       case 3:   // differencing
         if(a->boxwidget[1][0]) XtSetSensitive(a->boxwidget[1][0],False);
         for(k=0; k<=2; k++) 
         {  
            if(a->boxwidget[6][k]) XtSetSensitive(a->boxwidget[6][k],False);
            if(a->boxwidget[8][k]) XtSetSensitive(a->boxwidget[8][k],False);
            if(a->boxwidget[10][k]) XtSetSensitive(a->boxwidget[10][k],True);
            if(a->boxwidget[15][k]) XtSetSensitive(a->boxwidget[15][k],True);
            if(a->boxwidget[16][k]) XtSetSensitive(a->boxwidget[16][k],True);
            if(a->boxwidget[17][k]) XtSetSensitive(a->boxwidget[17][k],True);
            if(a->boxwidget[18][k]) XtSetSensitive(a->boxwidget[18][k],True);
         }
         if(a->radiowidget[1][1]) XtSetSensitive(a->radiowidget[1][1],False);
         if(a->radiowidget[1][2]) XtSetSensitive(a->radiowidget[1][2],False);
         if(a->radiowidget[1][3]) XtSetSensitive(a->radiowidget[1][3],False);
         if(a->radiowidget[1][6]) XtSetSensitive(a->radiowidget[1][6],False);
         if(a->radiowidget[1][10]) XtSetSensitive(a->radiowidget[1][10],True);
         break;
       default:  // quick segmentation
         if(a->boxwidget[1][0]) XtSetSensitive(a->boxwidget[1][0],False);
         for(k=0; k<=2; k++) 
         {  if(a->boxwidget[6][k]) XtSetSensitive(a->boxwidget[6][k],False);
            if(a->boxwidget[8][k]) XtSetSensitive(a->boxwidget[8][k],False);
            if(a->boxwidget[10][k]) XtSetSensitive(a->boxwidget[10][k],False);
            if(a->boxwidget[15][k]) XtSetSensitive(a->boxwidget[15][k],True);
            if(a->boxwidget[16][k]) XtSetSensitive(a->boxwidget[16][k],True);
            if(a->boxwidget[17][k]) XtSetSensitive(a->boxwidget[17][k],True);
            if(a->boxwidget[18][k]) XtSetSensitive(a->boxwidget[18][k],True);
         }
         if(a->radiowidget[1][1]) XtSetSensitive(a->radiowidget[1][1],False);
         if(a->radiowidget[1][2]) XtSetSensitive(a->radiowidget[1][2],False);
         if(a->radiowidget[1][3]) XtSetSensitive(a->radiowidget[1][3],False);
         if(a->radiowidget[1][6]) XtSetSensitive(a->radiowidget[1][6],False);
         if(a->radiowidget[1][10]) XtSetSensitive(a->radiowidget[1][10],False);
         break;
   } 
   allocate_grain_arrays();
     
   if(radio==1) switch(a->radioset[1])
   {   case 1: set_user_pattern(&opattern);
               break;
       case 2: break;         
       case 3: break;
       case 4:                //// Grains
               find_spots(ci, g.pattern_method, g.pattern_thresh, 
                          g.pattern_pweight, g.pattern_bweight, g.pattern_size, 
                          g.pattern_wantlabels, g.pattern_wantrectangles,
                          g.pattern_labelcolor, 
                          g.pattern_wantgraph, g.pattern_wantdensgraph);
               break;
       case 5:                //// Standard size grains
               switch(g.pattern_method)
               {  case NEURAL:
                     set_grain_pattern(&npattern);
                     patterns(npattern, GRAIN, g.pattern_thresh, 
                              g.pattern_pweight, g.pattern_bweight,
                              g.pattern_wantlabels, g.pattern_wantrectangles,
                              g.pattern_wantgraph, g.pattern_wantdensgraph); 
                  break;
                  case SEGMENTATION:
                     seg_size_distrib(ci, g.pattern_thresh, g.pattern_wantlabels,
                                      g.pattern_wantrectangles, 
                                      g.pattern_labelcolor, g.pattern_wantgraph, 
                                      g.pattern_wantdensgraph, EDM, NULL);
                     break;
                  case DIFFERENCING: 
                     diff_distrib(ci, g.pattern_thresh, g.pattern_size, 
                                  g.pattern_wantlabels, g.pattern_wantrectangles,
                                  g.pattern_labelcolor, 
                                  g.pattern_wantgraph, g.pattern_wantdensgraph);
                     break;
               }
               break;
       case 6:                //// Patterns
               switch(g.pattern_method)
               {  case NEURAL:
                     if(opattern.xsize==0) set_user_pattern(&opattern);
                      patterns(opattern, PATTERN, g.pattern_thresh, 
                               g.pattern_pweight, g.pattern_bweight,
                               g.pattern_wantlabels, g.pattern_wantrectangles,
                               g.pattern_wantgraph, g.pattern_wantdensgraph);
                  break;
                  case SEGMENTATION:
                     seg_size_distrib(ci, g.pattern_thresh, g.pattern_wantlabels,
                                      g.pattern_wantrectangles, 
                                      g.pattern_labelcolor, g.pattern_wantgraph, 
                                      g.pattern_wantdensgraph, EDM, NULL);
                     break;
                  case DIFFERENCING: 
                     break;
               }
               break;
       case 7: enhance(); break;
       case 8: save_sizes(grain_ino, grain, grain_size, grain_signal, 
                  grain_coordinate, grain_signal_distribution, grain_count, 
                  grain_maxcount, grain_most_frequent_size, 
                  grain_biggest, grain_biggest_signal, grain_xdensdisplayfac,
                  g.pattern_thresh, g.pattern_minsize, 
                  g.pattern_minsize, g.pattern_datafile, 1);

               break;
       case 9: if(z[ci].backedup) restoreimage(1); else repair(ci); break;

   }
   if(radio==1) unset_radio(a, 1);
   inpatterncheck=0;
}


//-------------------------------------------------------------------------//
// macro_find_spots - grain counting from macro                            //
//-------------------------------------------------------------------------//
int macro_find_spots(int noofargs, char **arg)
{
   int ino=ci, status=OK; 
   allocate_grain_arrays();
   if(noofargs>=1) g.pattern_method  = atoi(arg[1]);   
   if(noofargs>=2) g.pattern_thresh  = atof(arg[2]);   
   if(noofargs>=3) g.pattern_size    = atoi(arg[3]);   
   if(noofargs>=4) g.pattern_pweight = atoi(arg[4]);   
   if(noofargs>=5) g.pattern_bweight = atoi(arg[5]);   
   if(noofargs>=6) g.pattern_wantlabels = atoi(arg[6]);   
   if(noofargs>=7) g.pattern_labelcolor = atoi(arg[7]);   
   if(noofargs>=8) g.pattern_wantgraph  = atoi(arg[8]);   
   if(noofargs>=9) g.pattern_wantdensgraph  = atoi(arg[9]);   
   if(noofargs>=10) g.pattern_wantrectangles = atoi(arg[10]);   
   if(noofargs>=11) g.pattern_minsize = atoi(arg[11]);   
   if(noofargs>=12) g.pattern_maxsize = atoi(arg[12]);   
   if(noofargs>=13) g.pattern_wantsave       = atoi(arg[13]);
   if(noofargs>=14) strcpy(g.pattern_datafile, arg[14]);
                    else g.pattern_datafile[0] = 0;
   in_pattern_recognition++;
   find_spots(ino, g.pattern_method, g.pattern_thresh, g.pattern_pweight, 
              g.pattern_bweight, g.pattern_size, g.pattern_wantlabels, 
              g.pattern_wantrectangles, g.pattern_labelcolor, 
              g.pattern_wantgraph, g.pattern_wantdensgraph);
   if(g.pattern_wantsave)
   {    save_sizes(grain_ino, grain, grain_size, grain_signal, 
                  grain_coordinate, grain_signal_distribution, grain_count, 
                  grain_maxcount, grain_most_frequent_size, 
                  grain_biggest, grain_biggest_signal, grain_xdensdisplayfac,
                  g.pattern_thresh, g.pattern_minsize, 
                  g.pattern_minsize, g.pattern_datafile, 0);
   }
   in_pattern_recognition--;
   return status;   
}


//-------------------------------------------------------------------------//
// allocate_grain_arrays                                                   //
// Allocate arrays permanently in case user wants to analyze data in macro //
//-------------------------------------------------------------------------//
void allocate_grain_arrays(void)
{
   int k;
   if(!hit_size_distrib)
   {   grain        = new int[MAXGRAINS]; if(grain == NULL){ message(g.nomemory,ERROR); return; }
       grain_signal = new double[MAXGRAINS]; if(grain_signal == NULL){ message(g.nomemory,ERROR); return; }
       grain_size   = new int[MAXGRAINS]; if(grain_size == NULL){ message(g.nomemory,ERROR); return; }
       show_grain_size  = new int[MAXGRAINS]; if(show_grain_size == NULL){ message(g.nomemory,ERROR); return; }
       grain_coordinate = new int*[6]; if(grain_coordinate == NULL){ message(g.nomemory,ERROR); return; }       
       for(k=0;k<6;k++)
       {    grain_coordinate[k] = new int[MAXGRAINS]; 
            if(grain_coordinate[k] == NULL){ message(g.nomemory,ERROR); return; }
       }
       grain_signal_distribution = new int[261]; if(grain_signal_distribution == NULL){ message(g.nomemory,ERROR); return; }
       hit_size_distrib = 1;
   }
}


//-------------------------------------------------------------------------//
// find_spots                                                              //
//-------------------------------------------------------------------------//
void find_spots(int ino, int method, double thresh, double pweight, 
     double bweight, int size, int wantlabels, int wantrectangles, 
     int labelcolor, int wantgraph, int wantdensgraph)
{   
   switch(method)
   {  case NEURAL:
         if(opattern.xsize==0) set_user_pattern(&opattern);
         set_pattern_threshold(&opattern, &npattern, thresh);
         patterns(npattern, GRAIN, thresh, pweight, bweight,
            wantlabels, wantrectangles, wantgraph, wantdensgraph);
         break;
      case SEGMENTATION:
         seg_size_distrib(ino, thresh, wantlabels, wantrectangles,
            labelcolor, wantgraph, wantdensgraph, EDM, NULL);
         break;
      case DIFFERENCING: 
         diff_distrib(ino, thresh, size, wantlabels, wantrectangles,
            labelcolor, wantgraph, wantdensgraph);
   }

}

//-------------------------------------------------------------------------//
//  enhance                                                                //
//-------------------------------------------------------------------------//
void enhance(void)
{
   int k,noofargs;
   char **arg;                // up to 20 arguments
   arg = new char*[20];
   for(k=0;k<20;k++){ arg[k] = new char[128]; arg[k][0]=0; }

   noofargs = 0;
   strcpy(arg[0],"filter");
   strcpy(arg[1],"12"); noofargs++;
   strcpy(arg[2],"2"); noofargs++;
   strcpy(arg[3],"100"); noofargs++;
   filter_start(noofargs, arg);
   invertimage();
   for(k=0;k<20;k++) delete[] arg[k];
   delete arg;  
 }


//-------------------------------------------------------------------------//
// patterns                                                                //
//-------------------------------------------------------------------------//
void patterns(Pattern p, int pattern_type, double thresh, double pweight, 
     double bweight, int wantlabels, int wantrectangles, int wantgraph,
     int wantdensgraph)
{
   Widget www;
   wantlabels=wantlabels; wantgraph=wantgraph; wantrectangles=wantrectangles;
   int count,ino, index, hitabort=0,
       bpp,j,k,x1,y1,x2,y2,x11,y11,x22,y22,total=0,size,ototal,iterations=0,
       threshold, xmax=0, ymax=0, start_ino, most_frequent_size=0;
   int maxcount, biggest, helptopic=50, totalpixels;
   float min_signal, vmax = 0.0, biggest_signal; 
   char tempstring[1024];
   char title2[] = "Signal_distribution";
   char **ytitle = new char*[1]; 
   double xdensdisplayfac = 1.0;
   ytitle[0] = new char[64];
   strcpy(ytitle[0], "No. of objects");

   char **ytitle2 = new char*[1]; 
   ytitle2[0] = new char[64];
   strcpy(ytitle2[0], "Signal");

   char xtitle[] = "Count";
   double *xdata;

   ino = p.ino;
   size = p.xsize * p.ysize;

   if(ci != ino) switchto(ino);
   start_ino = ino;
   bpp = z[ino].bpp;

   int is_color = 1;
   if(z[ino].colortype==GRAY) is_color=0;
   if(z[ino].colortype==INDEXED && unmodified_grayscale(z[ino].palette)) is_color=0;

   threshold = cint((1.0 - thresh) * g.maxgray[bpp]);
   totalpixels = p.xsize * p.ysize;  // constant size

   array<float> layer(z[ino].xsize+1, z[ino].ysize+1); 
   if(!layer.allocated) { message(g.nomemory); return; }
   array<char> hit(z[ino].xsize+1, z[ino].ysize+1); 
   if(!hit.allocated) { message(g.nomemory); return; }

   for(j=0;j<z[ino].ysize;j++)
   for(k=0;k<z[ino].xsize;k++)
       hit.p[j][k] = 0;

   x1 = g.ulx - z[ino].xpos;
   x2 = g.lrx - z[ino].xpos - 1;
   y1 = g.uly - z[ino].ypos;
   y2 = g.lry - z[ino].ypos - 1;

   ////  Find all neurons over threshold. 
   ////  Neurons fire in order of highest to lowest signal. Each
   ////  firing causes the cells in the vicinity to be recalculated
   ////  and the cycle is repeated.

   double minfac;
   total = 0;
   min_signal = thresh * (float)size;
   minfac = 1/((float)size);
   if(pattern_type==GRAIN) 
       strcpy(tempstring, "grains"); 
   else 
       strcpy(tempstring, "patterns");

   printstatus(CALCULATING);
   grain_count = 0;
   do
   {  ototal = total;
      check_event();
      if(ci != start_ino) { message("Unexpected image change",ERROR); g.getout=1; }
      if(g.getout) { g.getout=0; break; }
      iterations++;
      if(neuron_count_mismatch(ino, x1, y1, x2, y2)) break;
      count = find_pattern(p, pattern_type, layer.p, hit.p, ino, threshold, 
              pweight, bweight, min_signal, is_color, x1, y1, x2, y2, 1);
      //// vmax is no. of pixels below threshold for each pattern  
      //// xmax, ymax is location of best fitting point   

      www = message_window_open(tempstring);
      for(k=0;k<count;k++)
      {   
          check_event();
          if(ci != start_ino) { message("Unexpected image change",ERROR); g.getout=1; }
          if(g.getout) break; 
          if(neuron_count_mismatch(ino, x1, y1, x2, y2)) break;
          find_highest_neuron(layer.p, x1, y1, x2, y2, xmax, ymax, vmax);           
          if(g.getout) break; 
          if(vmax >= min_signal)
          {   subtract_pattern(p, layer.p, hit.p, ino, xmax, ymax);
              total++;
              sprintf(tempstring, "Iteration %4d Total %d\nThreshold %4g\nSignal       %5.6f\nPosition %04d %04d",
                     iterations, total, thresh, vmax*minfac, xmax, ymax);
              message_window_update(www, tempstring);

              grain[grain_count] = totalpixels;  // totalpixels is constant for patterns
              if(grain[grain_count] > MAXGRAINS-1 && !hitabort)
              {   message("Grains too big\nAborting count\nIncrease threshold");
                  hitabort = 1;
                  break;
              }else   // handle signal distribution elsewhere
                  grain_size[grain[grain_count]]++;

              grain_signal[grain_count] = vmax*minfac;
              grain_coordinate[0][grain_count] = xmax;
              grain_coordinate[1][grain_count] = ymax;
              grain_coordinate[2][grain_count] = xmax-p.xsize/2;
              grain_coordinate[3][grain_count] = ymax-p.ysize/2;
              grain_coordinate[4][grain_count] = xmax+p.xsize/2;
              grain_coordinate[5][grain_count] = ymax+p.ysize/2;
              grain_count++;

          }else break;
          x11 = xmax - p.xsize; 
          y11 = ymax - p.ysize;
          x22 = xmax + p.xsize; 
          y22 = ymax + p.ysize;
          find_pattern(p, pattern_type, layer.p, hit.p, ino, threshold, 
               pweight, bweight, min_signal, is_color, x11, y11, x22, y22, 0);
      }
      message_window_close(www);
      if(g.getout) { g.getout=0; break; }
   }while(total>ototal);

   maxcount = biggest = 0;
   biggest_signal = 0.0;
   for(k=0; k<MAXGRAINS; k++)
   {   if(grain_size[k]) maxcount = k;    // biggest object     
       if(grain_signal[k] > biggest_signal) biggest_signal = grain_signal[k];
       if(grain_size[k] > biggest){ biggest = grain_size[k]; most_frequent_size = k; }
   }
   if(biggest_signal != 0.0)
   {   for(k=0; k<grain_count; k++)
       {   index = int(256 * (double)grain_signal[k] / biggest_signal);
           grain_signal_distribution[index]++;
       }
       xdensdisplayfac = biggest_signal / 256.0;
   }
   xdata = new double[maxcount+1];
   for(k=0; k<=maxcount; k++) xdata[k] = (double)k;

   if(wantdensgraph)
   {   if(!graph_is_open(pd2))
       {   pd2 = plot(title2, xtitle, ytitle2, xdata, grain_signal_distribution, 
                 260, 1, MEASURE, 0, cint(biggest_signal+1), NULL, null, null, 
                 null, null, null, helptopic, xdensdisplayfac, 1);
           open_graph(pd2, ci, SIGNAL_DISTRIBUTION_GRAPH);
       }else if(!g.getout)
       {   pd2->n = 256;
           delete[] pd2->data[0];  
           pd2->data[0] = new double[pd2->n+1];       
           for(k=0; k<pd2->n; k++) pd2->data[0][k] = grain_signal_distribution[k];
           redraw_graph(pd2);
       }
   }

   if(pattern_type == GRAIN) sprintf(tempstring,"%d grains found",total);
   else sprintf(tempstring,"%d patterns found",total);
   message(tempstring);
   delete[] xdata;
   grain_maxcount = maxcount;
   grain_most_frequent_size = most_frequent_size;
   grain_ino = ino;
   grain_biggest = biggest;
   grain_biggest_signal = biggest_signal;
   grain_xdensdisplayfac = xdensdisplayfac;
   return;
}



//-------------------------------------------------------------------------//
// find_pattern                                                            //
//-------------------------------------------------------------------------//
int find_pattern(Pattern p, int pattern_type, float **layer, char **hit, 
   int ino, int threshold, 
   double pweight, double bweight, float min_signal, int is_color,
   int x1, int y1, int x2, int y2, int want_progress_bar)
{
   Widget www, scrollbar;
   int f,i,ii,j,jj,k,k1,k2,l,l1,oldj=0,bpp,v=0,vpat=0,total=0,xsize,ysize;
   int pluscount=0, minuscount=0, oino=ino;
   f = z[ino].cf;
   uchar **add;
   register uchar *ptr;
   add = z[ino].image[f];
   bpp = z[ino].bpp;
   xsize = z[ino].xsize;
   ysize = z[ino].ysize;
   if(y1<0 || y2 >= ysize) return 0;
   if(x1<0 || x2 >= xsize) return 0;
   if(want_progress_bar) progress_bar_open(www, scrollbar);
   int pysize2 = p.ysize/2;
   int pxsize2 = p.xsize/2;

   for(j=y1; j<y2; j++)
   {
     check_event();
     if(ino != oino) g.getout=1;
     if(want_progress_bar && y2!=y1 && j-oldj > 5)
     {  oldj = j;
        progress_bar_update(scrollbar, (j-y1)*100/(y2-y1));
     }
     if(g.getout) return 0;
     for(i=x1; i<x2; i++)
     {  
        if(!between(j,0,ysize-1)) continue;
        if(!between(i,0,xsize-1)) continue;
        layer[j][i] = 0.0;
        pluscount=0; 
        minuscount=0;
        jj = j - pysize2;
        ii = i - pxsize2;
        if(jj < 0 || jj >=ysize) continue;
        if(ii < 0 || ii >=xsize) continue;
        if(jj+p.ysize < 0 || jj+p.ysize >=ysize) continue;
        if(ii+p.xsize < 0 || ii+p.xsize >=xsize) continue;

        for(l=0; l<p.ysize; l++)
        for(k=0; k<p.xsize; k++)
        {    
             l1 = l + jj;
             k1 = k + ii;
             if(hit[l1][k1]) continue;

             //// Making p.pat an array makes it 23% faster
             vpat = p.pat[l][k];
             k2 = k1*g.off[bpp]; 

             //// This is v = pixelat(add[l1]+k2, bpp);
             //// Putting it here makes it 11% faster. 
             //// Making pixelat inline doesn't help.

             ptr = add[l1]+k2;
             if(g.ximage_byte_order == LSBFirst)
             {    switch(bpp)
                  {   case 7:  
                      case 8:   v = *ptr; break;
                      case 15:  v = 0x7fff & (*(ptr+1)<<8) + *ptr; break;
                      case 16:  v = (*(ptr+1)<<8) + *ptr; break;
                      case 24:
                      case 32:  v = (*(ptr+2)<<16) + (*(ptr+1)<<8) + *ptr; break;
                      case 48:  v = (*(ptr+4)<<16) + (*(ptr+2)<<8) + *ptr; break;
                  }
             }else
             {    switch(bpp)
                  {   case 7:   
                      case 8:   v = *ptr; break;
                      case 15:  v = 0x7fff & (*(ptr+1)<<8) + *ptr; break;
                      case 16:  v = (*(ptr)<<8) + *(ptr+1); break; 
                      case 24:  v = (*(ptr+0)<<16) + (*(ptr+1)<<8) + *(ptr+2); break;
                      case 32:  v = (*(ptr+1)<<16) + (*(ptr+2)<<8) + *(ptr+3); break;
                      case 48:  v = (*(ptr+1)<<16) + (*(ptr+3)<<8) + *(ptr+5); break; 
                  }
             } 

             //// Putting this here instead of in a function makes it 27% faster           

             if(is_color)
             {  if(pixel_below_threshold(v, vpat, bpp, p.bpp, pattern_type, is_color, threshold))
                     pluscount++;
                else
                     minuscount++;
             }else if(pattern_type==GRAIN)
             {  if(v <= threshold && vpat <= threshold)      // match and grain
                     pluscount++;
                else  if(v > threshold && vpat > threshold)  // match not grain
                     minuscount++;
                else                                         // mismatch
                     minuscount++;
             }else
             {   if(abs(v-vpat)<threshold) 
                     pluscount++;
                 else 
                     minuscount++;
             }
        }
        layer[j][i] = pluscount*pweight + minuscount*bweight;
        if(layer[j][i] >= min_signal) total++;
     }
   }
   if(want_progress_bar) progress_bar_close(www);
   return total;
}


//-------------------------------------------------------------------------//
// find_highest_neuron                                                     //
//-------------------------------------------------------------------------//
void find_highest_neuron(float **layer, int x1, int y1, int x2, int y2, 
     int &xmax, int &ymax, float &vmax)
{
   int i,j;
   vmax = 0.0;
   for(j=y1; j<y2; j++)
   for(i=x1; i<x2; i++)
      if(layer[j][i] > vmax)
      {    vmax = layer[j][i];
           xmax = i;
           ymax = j;
      }
}

//-------------------------------------------------------------------------//
// neuron_count_mismatch                                                   //
//-------------------------------------------------------------------------//
int neuron_count_mismatch(int ino, int x1, int y1, int x2, int y2) 
{
   if(!between(x1, 0, z[ino].xsize-1) || 
      !between(x2, 0, z[ino].xsize-1) ||
      !between(y1, 0, z[ino].ysize-1) || 
      !between(y2, 0, z[ino].ysize-1))
      { message("Neuron count mismatch");  return 1; }
   return 0;
}


//-------------------------------------------------------------------------//
// subtract_pattern                                                        //
//-------------------------------------------------------------------------//
void subtract_pattern(Pattern p, float **layer, char **hit, 
     int ino, int xmax, int ymax)
{
   static int color = 0;
   int f, k, k1, k2, k3, l, l1, l2, bpp, v, x, y, xs, ys, vpat;
   if(ino<=0) return;
   f = z[ino].cf;
   bpp = z[ino].bpp;
   if(color==0)
   {   if(g.bitsperpixel == 8) 
          color = 255;
       else
          color = RGBvalue(g.maxred[g.bitsperpixel], 0, 0, g.bitsperpixel);
   }

   for(l=0; l<p.ysize; l++)
   for(k=0; k<p.xsize; k++)
   {    
        l1 = l + ymax - p.ysize/2;
        k1 = k + xmax - p.xsize/2;
        l2 = l1;
        k2 = k1;
        if(k2<0 || l2<0 || 
           l2 >= z[ino].ysize-1 || 
           k2 >= z[ino].xsize-1) continue;
        k3 = k2*g.off[bpp]; 
        v = pixelat(z[ino].image[f][l2]+k3, bpp);
        vpat = p.pat[l][k]; 
        hit[l1][k1] = 1;
        layer[l1][k1] = 0.0;
   } 
   x = z[ino].xpos + xmax;
   y = z[ino].ypos + ymax;
   xs = zoom_x_coordinate_of_index(x, ino);
   ys = zoom_y_coordinate_of_index(y, ino);
   draw_cross(xs,ys,color);
}


//-------------------------------------------------------------------------//
// set_user_pattern                                                        //
//-------------------------------------------------------------------------//
void set_user_pattern(Pattern *p)
{
   char tempstring[1024];
   int bytesperpixel, bpp, ino, i, j, xx1, yy1, xx2, yy2, ulx, uly, lrx, lry, xsize, ysize, v;
   if(p->xsize) free_pattern(p);
   message("Use mouse to select\na representative grain or pattern\n(draw box around pattern,\ninclude some background)");
   ulx = g.ulx; uly = g.uly; lrx = g.lrx; lry = g.lry;
   getbox(xx1, yy1, xx2, yy2);

   g.ulx = ulx; g.uly = uly; g.lrx = lrx; g.lry = lry;
   ino = ci;

   xx1 = zoom_x_coordinate(xx1, ino);
   xx2 = zoom_x_coordinate(xx2, ino);
   yy1 = zoom_y_coordinate(yy1, ino);
   yy2 = zoom_y_coordinate(yy2, ino);

   if(ino <0) return;
   bytesperpixel = (7+z[ino].bpp)/8;
   p->ino = ino;
   p->bpp = z[ino].bpp;
   xsize = yy2 - yy1 + 1;
   ysize = xx2 - xx1 + 1;
   if(!(xsize & 1)) xsize++;
   if(!(ysize & 1)) ysize++;
   p->ysize = xsize;
   p->xsize = ysize;

   p->colortype = z[ino].colortype; 
   if(z[ino].colortype==INDEXED && unmodified_grayscale(z[ino].palette)) p->colortype=GRAY;
   p->pat_1d = (uint*)malloc(p->ysize * p->xsize * sizeof(int));
   if(p->pat_1d==NULL) { message(g.nomemory); return; }
   p->pat    = make_2d_alias(p->pat_1d, p->xsize, p->ysize);
   for(j=0; j<p->ysize; j++)
   for(i=0; i<p->xsize; i++)
   {   v = readpixelonimage(xx1+i, yy1+j, bpp, ino, -1);  
       p->pat[j][i] = v;
   }
   sprintf(tempstring, "%d x %d pattern stored", p->xsize, p->ysize);
   message(tempstring);
}


//-------------------------------------------------------------------------//
//  set_pattern_threshold                                                  //
//-------------------------------------------------------------------------//
void set_pattern_threshold(Pattern *p1, Pattern *p2, double thresh)
{
   int bytesperpixel, i, j, rr, gg, bb, size, vpat;
   int threshold = cint(thresh * 255);
   if(p2->xsize) free_pattern(p2);

   ////  Copy p1 into p2
   p2->ino       = p1->ino;
   p2->bpp       = p1->bpp;
   p2->ysize     = p1->ysize;
   p2->xsize     = p1->xsize;
   p2->colortype = p1->colortype;
   bytesperpixel = (7+p1->bpp)/8;
   p2->pat_1d    = (uint*)malloc(p1->ysize * p1->xsize * sizeof(int));
   if(p2->pat_1d==NULL) { message(g.nomemory); return; }
   p2->pat       = make_2d_alias(p1->pat_1d, p1->xsize, p1->ysize);
   size = p2->xsize * p2->ysize * bytesperpixel;
   memcpy(p2->pat_1d, p1->pat_1d, size);

   //// Threshold pattern p2
   for(j=0; j<p2->ysize; j++)
   for(i=0; i<p2->xsize; i++)
   {  
       vpat = p2->pat[j][i]; 
       if(p2->colortype==GRAY)
       {    
           if(vpat < threshold) p2->pat[j][i]=0; else  p2->pat[j][i]=255;
       }else
       {   valuetoRGB(vpat, rr, gg, bb, p2->bpp);
           if((rr + gg + bb)/3 < threshold) 
                rr = gg = bb = 0; 
           else
           {    rr = g.maxred[p2->bpp];
                gg = g.maxgreen[p2->bpp];
                bb = g.maxblue[p2->bpp];
           }
           vpat = RGBvalue(rr,gg,bb,p2->bpp);
           p2->pat[j][i] = vpat;
      }
   }
}


//-------------------------------------------------------------------------//
// set_grain_pattern                                                       //
//-------------------------------------------------------------------------//
void set_grain_pattern(Pattern *p)
{
   double dist;
   int ino = ci, i, j;
   if(p->xsize) free_pattern(p);
   p->ino = ino;
   p->bpp = z[ino].bpp;
   p->ysize = 11;
   p->xsize = 11;
   p->colortype = z[ino].colortype;
   p->pat_1d = (uint*)malloc(p->ysize * p->xsize * sizeof(int));
   if(p->pat_1d==NULL) { message(g.nomemory); return; }
   p->pat    = make_2d_alias(p->pat_1d, p->xsize, p->ysize);
   for(j=0; j<11; j++)
   for(i=0; i<11; i++)
   {   
      dist = sqrt(abs(i-5)*abs(i-5) + abs(j-5)*abs(j-5));
      if(dist<2.8) p->pat[j][i] = 0; else p->pat[j][i] = (int)g.maxcolor;
   }
}

//-------------------------------------------------------------------------//
// free_pattern                                                            //
//-------------------------------------------------------------------------//
void free_pattern(Pattern *p)
{
   free(p->pat);
   free(p->pat_1d);
   p->ysize = 0;
   p->xsize = 0;
}


//-------------------------------------------------------------------------//
// free_graincounting_arrays                                              //
//-------------------------------------------------------------------------//
void free_grain_counting_arrays(void)
{
   int k;
   if(hit_size_distrib && grain!=NULL)
   {   delete[] grain;
       delete[] grain_signal;
       delete[] grain_size;
       delete[] show_grain_size;
       delete[] grain_signal_distribution;
       for(k=0;k<6;k++) delete[] grain_coordinate[k];
       delete[] grain_coordinate;
       hit_size_distrib = 0;
       grain_count = 0;
   }else message("Grain counting arrays\nwere already freed", ERROR);
}


//-------------------------------------------------------------------------//
// spot_size, etc. - called by calculator.y                                //
//-------------------------------------------------------------------------//
double spot_size(double spotno)
{
  int k = cint(spotno);
  if(hit_size_distrib && between(k,0,grain_count-1)) return(grain_size[k]);
  if(!hit_size_distrib) message("No grain counting data");
  else message("Array out of bounds");
  return 0;
}
double spot_signal(double spotno)
{
  int k = cint(spotno);
  if(hit_size_distrib && between(k,0,grain_count-1)) return(grain_signal[k]);
  if(!hit_size_distrib) message("No grain counting data");
  else message("Array out of bounds");
  return 0;
}
double spot_x(double spotno)
{
  int k = cint(spotno);
  if(hit_size_distrib && between(k,0,grain_count-1)) return(grain_coordinate[0][k]);
  if(!hit_size_distrib) message("No grain counting data");
  else message("Array out of bounds");
  return 0;
}
double spot_y(double spotno)
{
  int k = cint(spotno);
  if(hit_size_distrib && between(k,0,grain_count-1)) return(grain_coordinate[1][k]);
  if(!hit_size_distrib) message("No grain counting data");
  else message("Array out of bounds");
  return 0;
}


//-------------------------------------------------------------------------//
// seg_size_distrib  - source can be EDM or BUF                            //
// Track grains in a circular pattern on edm or preallocated buffer.       //
// If source is BUF, the buffer must already be allocated.                 // 
//-------------------------------------------------------------------------//
void seg_size_distrib(int ino, double thresh, int wantlabels, 
     int wantrectangles, int labelcolor,
     int wantgraph, int wantdensgraph, int source, int **sourcebuf)
{
   int oino = ino;
   if(!between(ino, 0, g.image_count-1)) return;
   if(z[ino].colortype != GRAY){ message("Image must be grayscale"); return; }
   //char newfont[]  = "*helvetica-medium-r-normal--0-0-100-100*";
   //char newfont[] = "*times-medium-r-normal--0-0-75-75*";
   double xdensdisplayfac = 1.0, cal;
   char newfont[256] = "*courier-bold-r-normal--10-100*";
   char calstring[64];
   int bpp,i,index,j,k,k1,x,y,ox,oy,xsize,ysize,xs,ys,xe,ye,minx2,miny2,maxx2,maxy2; 
   int biggest, show_maxcount, hitabort=0;
   int struct_element = 1, maxcount, done=0, most_frequent_size=0;
   int fc, bc, totalpixels, minx, miny, maxx, maxy;
   double totalsignal = 0.0, biggest_signal=0.0;
   double *xdata;
   bpp = z[ino].bpp;
   xsize = z[ino].xsize;
   ysize = z[ino].ysize;
   thresh = (1-thresh)*g.maxvalue[bpp];
   int xpos = z[ino].xpos;
   int ypos = z[ino].ypos;
   int helptopic = -1;

   char title[] = "Size_distribution";
   char title2[] = "Signal_distribution";
   char **ytitle = new char*[1]; 
   ytitle[0] = new char[64];
   strcpy(ytitle[0], "No. of objects");

   char **ytitle2 = new char*[1]; 
   ytitle2[0] = new char[64];
   strcpy(ytitle2[0], "Signal");

   char xtitle[] = "Count";
   double xdisplayfac = 1.0;
   double ydisplayfac = 1.0;

   fc = 255;              //// Make sure nothing in sourcebuf has this value
   bc = (int)g.maxvalue[bpp];

   ////  Get around apparent gcc bug - array can't be behind {}'s
   ////  If it is, assigning edm.p to sourcebuf causes crash

   int xxx, yyy;
   if(source==EDM) { xxx = z[ino].xsize; yyy = z[ino].ysize; }
   else            { xxx = 1; yyy = 1; }
   array<uint> edm(xxx, yyy);   
   if(source==EDM) 
   {
       if(!edm.allocated){ message(g.nomemory); return; }
       create_edm(ino, struct_element, (int)thresh, &edm);
       sourcebuf = (int**)edm.p;
   }
 
   //// From here on, 0's in sourcebuf are considered to be background. 
   //// All other values are considered to be in grains. 
   //// edm should not be referenced from here on.

   for(k=0; k<MAXGRAINS; k++)
   {    grain[k]=0; 
        grain_size[k]=0; 
        show_grain_size[k]=0; 
        grain_signal[k]=0.0; 
   }
   for(k=0; k<260; k++) grain_signal_distribution[k]=0; 
   
   //// Set a small font for labels
   if(wantlabels) setfont(newfont);

   done = 0;
   grain_count = 0;

   g.getout = 0;
   g.inmenu++;  //// No returns after this point
   g.waiting++;
   
   for(j=0; j<ysize; j++)
   {  for(i=0; i<xsize; i++)
      { 
         if(!sourcebuf[j][i]) continue;
         if(sourcebuf[j][i] == fc) continue;
         if(keyhit()) if(getcharacter()==27) done=1;
         check_event();
         if(!in_pattern_recognition){ g.getout=1; done=1; break; }
         if(ino != oino){ g.getout=1; done=1; break; }
         if(g.getout) done=1;
         fillbuf(sourcebuf, i, j, 0, 0, xsize, ysize-1, minx, miny, maxx, maxy,
              fc, 0, 0, totalpixels, totalsignal, 1, ino); 
         grain[grain_count] = totalpixels;
         if(grain[grain_count] <  g.pattern_minsize) continue;
         if(grain[grain_count] >= g.pattern_maxsize) continue;
         grain_signal[grain_count] = totalsignal;
         if(grain[grain_count] > MAXGRAINS-1 && !hitabort)
         {   message("Grains too big\nAborting count\nIncrease threshold");
             hitabort = 1;
             done = 1;
         }else   // handle signal distribution elsewhere
             grain_size[grain[grain_count]]++;

         grain_coordinate[0][grain_count] = i;
         grain_coordinate[1][grain_count] = j;
         grain_coordinate[2][grain_count] = minx;
         grain_coordinate[3][grain_count] = miny;
         grain_coordinate[4][grain_count] = maxx;
         grain_coordinate[5][grain_count] = maxy;
         x = xpos + (minx+maxx)/2;
         y = ypos + (miny+maxy)/2;
         minx2 = zoom_x_coordinate_of_index(minx, ino);
         maxx2 = zoom_x_coordinate_of_index(maxx, ino);
         miny2 = zoom_y_coordinate_of_index(miny, ino);
         maxy2 = zoom_y_coordinate_of_index(maxy, ino);
         xs = zoom_x_coordinate_of_index(x, ino);
         ys = zoom_y_coordinate_of_index(y, ino);
         xe = zoom_x_coordinate_of_index(x+10, ino);
         ye = zoom_y_coordinate_of_index(y+10, ino);
         oy = y;
         if(wantlabels)
         {   g.inmenu--;
             if(g.pattern_labelflags) line(xs, ys, xe, ye, labelcolor, SET, 
                 z[ino].win, ino);
             //// This changes y
             if(g.pattern_labelflags & 0x01) // grain no.
                print_grain_label(grain_count+1, x, y, labelcolor, bc, ino);
             if(g.pattern_labelflags & 0x02) // grain size
                print_grain_label(grain[grain_count], x, y, labelcolor, bc, ino);
             if(g.pattern_labelflags & 0x04) // total grain signal
                print_grain_label(cint(grain_signal[grain_count]), x, y, labelcolor, bc, ino);
             if(g.pattern_labelflags & 0x08) // raw x
                print_grain_label(x, x, y, labelcolor, bc, ino);
             if(g.pattern_labelflags & 0x10) // raw y
                print_grain_label(oy, x, y, labelcolor, bc, ino);
             if(g.pattern_labelflags & 0x20) // 1st dim calib
             {  calibratepixel(x, oy, ino, 0, cal, calstring); 
                print_grain_label(calstring, x, y, labelcolor, bc, ino);
             }
             if(g.pattern_labelflags & 0x40) // 2nd dim calib
             {  calibratepixel(x, oy, ino, 1, cal, calstring); 
                print_grain_label(calstring, x, y, labelcolor, bc, ino);
             }
             if(g.pattern_labelflags & 0x80) // 3rd dim calib
             {  calibratepixel(x, oy, ino, 2, cal, calstring); 
                print_grain_label(calstring, x, y, labelcolor, bc, ino);
             }             
             g.inmenu++;
         }
         if(wantrectangles)
         {   g.inmenu--;
             box(minx2+xpos, miny2+ypos, maxx2+xpos, maxy2+ypos, labelcolor, 
                 SET, z[ino].win, ino);
             g.inmenu++;
         }else 
             draw_cross(xs, ys, 192);
         if(grain_count++ >= MAXGRAINS){ message("Too many grains\nplease enlarge me"); done=1; }
         if(done) break;
      }
      if(done) break;
   }  
   g.inmenu--;
   g.getout = 0;
   if(wantlabels) setfont(g.imagefont);
   maxcount = biggest = 0;
   biggest_signal = 0.0;
   for(k=0; k<MAXGRAINS; k++)
   {   if(grain_size[k]) maxcount = k;    // biggest object
                                          // biggest size count & most frequent size
       if(grain_size[k] > biggest){ biggest = grain_size[k]; most_frequent_size = k; }
       if(grain_signal[k] > biggest_signal) biggest_signal = grain_signal[k]; // biggest signal
   }

   if(biggest_signal)
   {   for(k=0; k<grain_count; k++)
       {   index = int(256 * (double)grain_signal[k] / biggest_signal);
           grain_signal_distribution[index]++;
       }
       xdensdisplayfac = biggest_signal / 256.0;
   }

   xdata = new double[maxcount+1];

   for(k=0; k<=maxcount; k++) xdata[k] = (double)k;
   show_maxcount = maxcount;
   if(maxcount > 0.5*g.xres)
   {   xdisplayfac = max(1, cint(0.5+(double)maxcount/(double)(0.5*g.xres)));
       show_maxcount = cint(xdisplayfac) + (int)(maxcount / xdisplayfac);
   }

   if(wantgraph)
   {   
       //// Shrink data to fit on screen, put highest data point at each pos.
       for(k=0; k<=maxcount; k++) 
       {   k1 = int(k/xdisplayfac);
           show_grain_size[k1] = max(show_grain_size[k1], grain_size[k]);
       }
       if(!graph_is_open(pd1))
       {   pd1 = plot(title, xtitle, ytitle, xdata, show_grain_size, show_maxcount+1, 
                 1, MEASURE, 0, show_maxcount+1, NULL, null, null, null, null, null, 
                 helptopic, xdisplayfac, ydisplayfac);
           open_graph(pd1, ci, SIZE_DISTRIBUTION_GRAPH);
       }else if(!g.getout)
       {   pd1->n = show_maxcount+1;
           delete[] pd1->data[0];  
           pd1->data[0] = new double[pd1->n+1];       
           for(k=0; k<pd1->n; k++) pd1->data[0][k] = show_grain_size[k];
           redraw_graph(pd1);
       }
   }
   if(wantdensgraph)
   {   if(!graph_is_open(pd2))
       {   pd2 = plot(title2, xtitle, ytitle2, xdata, grain_signal_distribution, 
                 260, 1, MEASURE, 0, cint(biggest_signal+1), NULL, null, null, null, 
                 null, null, helptopic, xdensdisplayfac, 1);
           open_graph(pd2, ci, SIGNAL_DISTRIBUTION_GRAPH);
       }else if(!g.getout)
       {   pd2->n = 256;
           delete[] pd2->data[0];  
           pd2->data[0] = new double[pd2->n+1];       
           for(k=0; k<pd2->n; k++) pd2->data[0][k] = grain_signal_distribution[k];
           redraw_graph(pd2);
       }
   }

   add_variable((char*)"GRAINS", (double)grain_count);
   add_variable((char*)"SPOTS", (double)grain_count);

   delete[] xdata;
   delete[] ytitle[0];
   delete[] ytitle;
   delete[] ytitle2[0];
   delete[] ytitle2;
   g.getout = 0;
   if(!wantlabels && !wantgraph && !wantrectangles) getpoint(ox,oy);
   switchto(ino);
   repair(ino);
   g.waiting = max(0, g.waiting-1);
   grain_maxcount = maxcount;
   grain_most_frequent_size = most_frequent_size;
   grain_ino = ino;
   grain_biggest = biggest;
   grain_biggest_signal = biggest_signal;
   grain_xdensdisplayfac = xdensdisplayfac;
   return;
}


//-------------------------------------------------------------------------//
// draw_cross                                                              //
//-------------------------------------------------------------------------//
void draw_cross(int x, int y, int color)
{
   line(x-5, y, x+5, y, color, SET);
   line(x, y-5, x, y+5, color, SET);
}


//-------------------------------------------------------------------------//
// print_grain_label - changes y                                           //
//-------------------------------------------------------------------------//
void print_grain_label(int val, int x, int &y, int fc, int bc, int ino)
{
  char tempstring[64];
  sprintf(tempstring, "%d", val); 
  print(tempstring, x+15, y+15, fc, bc, &g.image_gc, z[ino].win, 0, HORIZONTAL, 0, ino);
  y+=12;
}
void print_grain_label(char *valstring, int x, int &y, int fc, int bc, int ino)
{
  print(valstring, x+15, y+15, fc, bc, &g.image_gc, z[ino].win, 0, HORIZONTAL, 0, ino);
  y+=12;
}

//-------------------------------------------------------------------------//
// print_label                                                             //
//-------------------------------------------------------------------------//
void print_label(int val, int x, int y, int fc, int bc)
{
  int ino = ci;
  char tempstring[64];
  sprintf(tempstring, "%d", val); 
  print(tempstring, x+8, y-1, fc, bc, &g.image_gc, z[ino].win, 0, HORIZONTAL, 0, ino);
}
void print_label(char *valstring, int x, int y, int fc, int bc)
{
  int ino = ci;
  print(valstring, x+8, y-1, fc, bc, &g.image_gc, z[ino].win, 0, HORIZONTAL, 0, ino);
}


//-------------------------------------------------------------------------//
// diff_distrib                                                            //
//-------------------------------------------------------------------------//
void diff_distrib(int ino, double thresh, int size, int wantlabels,
   int wantrectangles, int labelcolor, int wantgraph, int wantdensgraph)
{
   wantlabels=wantlabels;wantgraph=wantgraph;wantrectangles=wantrectangles;
   int bpp,i,j,i2,val;  
   int f = z[ino].cf;
   bpp = z[ino].bpp;
   int b = g.off[bpp];
   int xsize = z[ino].xsize;
   int ysize = z[ino].ysize;
   int ithresh = cint((1-thresh)*g.maxvalue[bpp]);

   //// Diff filter -> threshold -> quick seg. size dist
   //// Note: thresholding is inverted

   array<int> buffer(z[ino].xsize, z[ino].ysize);   
   if(!buffer.allocated){ message(g.nomemory); return; }
         
   difference_filter(ino, size, 1);
   for(j=0; j<ysize; j++)
   for(i=0,i2=0; i<xsize; i++,i2+=b)
   {   val = pixelat(z[ino].image[f][j]+i2, bpp);
       if(val < ithresh) val = 3; else val = 0;
       buffer.p[j][i] = val;
   }
   XFlush(g.display);
   switchto(ino);
   restoreimage(1);
   seg_size_distrib(ino, thresh, wantlabels, wantrectangles, labelcolor, 
       wantgraph, wantdensgraph, BUF, buffer.p);
   switchto(ino);
   redraw(ino);
}


//-------------------------------------------------------------------------//
// difference_filter                                                       //
//-------------------------------------------------------------------------//
void difference_filter(int ino, int size, int sharpfac)
{
   sharpfac = sharpfac;
   Widget www, scrollbar;
   uchar **img;
   int bpp,count, h,i,i2,j,k,kb,hb,l,xsize,ysize; 
   int v[32], vave;
   int f = z[ino].cf;
   int pixcount = 4*(2*size+1)-4; // no of pixels measured
   bpp = z[ino].bpp;
   int b = g.off[bpp];
   int maxpix = (int)g.maxvalue[bpp]; 
   int pixfac = max(1, maxpix / pixcount);  
   xsize = z[ino].xsize;
   ysize = z[ino].ysize;
   array<uint> cat(z[ino].xsize, z[ino].ysize);   
   if(!cat.allocated){ message(g.nomemory); return; }

   img = z[ino].image[f]; 
   progress_bar_open(www, scrollbar);
   for(j=size; j<ysize-size-1; j++)
   {  progress_bar_update(scrollbar, 100*(j-size)/(ysize-1));
      if(keyhit()) if(getcharacter()==27) break; 
      if(g.getout){ g.getout = 0; break; }
      for(i=size,i2=b*size; i<xsize-size-1; i++,i2+=b)
      {  v[0] = pixelat(img[j] + i2, bpp);
         for(h=1;h<=size;h++)
         {   v[h] = 0;
             count = 0;
             hb = h*b;
             for(l=-h; l<=h; l++)
             {   v[h] += pixelat(img[j+l] + i2-hb, bpp);
                 v[h] += pixelat(img[j+l] + i2+hb, bpp);
                 count += 2;
             }
             for(k=-h+1,kb=b*(-h+1); k<h; k++,kb+=b)
             {   v[h] += pixelat(img[j-h] + i2+kb, bpp);
                 v[h] += pixelat(img[j+h] + i2+kb, bpp);
                 count += 2;
             }
             v[h] = v[h] / count;
         }
         vave = 0;
         for(h=0;h<size;h++)
             vave += abs(v[h] - v[h+1]);
         cat.p[j][i] = maxpix - max(0, min(maxpix, vave*pixfac));
      }
   }

   for(j=size; j<ysize-size-1; j++)
   for(i=size,i2=b*size; i<xsize-size-1; i++,i2+=b)
       putpixelbytes(img[j]+i2, cat.p[j][i], 1, bpp);
   progress_bar_close(www);
}


//-------------------------------------------------------------------------//
// save_sizes                                                              //
//-------------------------------------------------------------------------//
void save_sizes(int ino, int *grain, int *size, double *signal, int **coordinate, 
     int *signal_distribution, int count, int maxcount, int most_frequent_size, 
     int biggest, double biggest_signal, double xfac, double thresh, int minsize,
     int maxsize, char *filename, int want_ask_filename)
{
   static PromptStruct ps;
   static int params[10];
   static double dparams[10];
   g.getout = 0;
   if(want_ask_filename)
       message("File name:",filename,PROMPT,FILENAMELENGTH-1,54);
   if(!strlen(filename) || g.getout) return;

   params[0] = ino;
   params[1] = count;
   params[2] = maxcount;
   params[3] = most_frequent_size;
   params[4] = biggest;
   params[6] = minsize;
   params[7] = maxsize;
 
   dparams[0] = xfac;
   dparams[1] = thresh;
   dparams[2] = biggest_signal;

   ps.filename = filename;
   ps.f1 = save_sizes_part2;
   ps.f2 = null;
   ps.params = params;
   ps.dparams = dparams;

   ps.data2[0] = grain;
   ps.data2[1] = size;
   ps.data2[3] = signal_distribution;

   ps.data3[0] = coordinate;
   ps.fdata2[0] = signal;
   check_overwrite_file(&ps);
}


//-------------------------------------------------------------------------//
// save_sizes_part2                                                        //
//-------------------------------------------------------------------------//
void save_sizes_part2(PromptStruct *ps)
{
   FILE *fp;
   char *filename         = ps->filename;
   int *params            = ps->params;
   double *dparams        = ps->dparams;
   int ino                = params[0];
   int count              = params[1];
   int maxcount           = params[2];
   int most_frequent_size = params[3];
   int biggest            = params[4];
   int minsize            = params[6];
   int maxsize            = params[7];

   double xfac            = dparams[0];
   double thresh          = dparams[1];
   double biggest_signal  = dparams[2];
   int *grain             = ps->data2[0];
   int *size              = ps->data2[1];
   double *signal           = ps->fdata2[0];
   int *signal_distribution = ps->data2[3];
   int **coordinate         = ps->data3[0];

   int h,k,x,y;
   if(xfac==0) xfac=1;
   char cs[3][64];
   double cal;
   if(!hit_size_distrib){ message("No grain counting data"); return; }
   if(!strlen(filename)){ message("No filename, aborting"); return; }
   if((fp=fopen(filename,"w")) == NULL){ error_message(filename, errno); return; }

   fprintf(fp, "# Size analysis of %s\n", z[ino].name);
   fprintf(fp, "# Largest signal     %g\n",biggest_signal);
   fprintf(fp, "# Most frequent size %d (count=%d)\n",most_frequent_size, biggest);
   fprintf(fp, "# Largest object     %d\n",maxcount);
   fprintf(fp, "# Largest object     %d\n",maxcount);
   fprintf(fp, "# Threshold          %g\n",thresh);
   fprintf(fp, "# Min. size cutoff   %d\n",minsize);
   fprintf(fp, "# Max. size cutoff   %d\n",maxsize);

   fprintf(fp, "# Obj.\tx pos\ty pos\tSize\tSignal\t\tMin.x\tMin.y\tMax.x\tMax.y\t\t%s\t%s\t%s\n",
       z[ino].cal_title[0], z[ino].cal_title[1], z[ino].cal_title[2]);
   for(k=0; k<count; k++) 
   {   x = coordinate[0][k];
       y = coordinate[1][k];  
       for(h=0; h<3; h++)
       {    calibratepixel(x, y, ino, h, cal, cs[h]); 
            if(!strcmp(cs[h], "Uncalibrated")) cs[h][5]=0;
       }            
       fprintf(fp,"%d\t%d\t%d\t%d\t%g\t\t%d\t%d\t%d\t%d\t\t%s\t%s\t%s\n", 
           k+1, x, y, grain[k], signal[k], coordinate[2][k], coordinate[3][k],
           coordinate[4][k], coordinate[5][k], cs[0], cs[1], cs[2]);
   }
   fprintf(fp, "\n# Size distribution\n# Size\tCount\n");
   for(k=0; k<=maxcount; k++) fprintf(fp,"%d\t%d\n",k,size[k]);

   fprintf(fp, "\n# Signal distribution\n# Signal\tCount\n");
   for(k=0; k<=256; k++) fprintf(fp,"%g\t%d\n", k*xfac, signal_distribution[k]);
   fclose(fp);
   if(filesize(filename)) message("Data saved");
   else message("Error saving data");
   return;
}
]                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                