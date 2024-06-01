//--------------------------------------------------------------------------//
// xmtnimage67.cc                                                           //
// Latest revision: 03-07-2004                                              //
// Copyright (C) 2003 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern int         ci;                 // current image
extern Image      *z;
extern Globals     g;
static double ca_answers[10][3]={{0,  0,   0},     // 0
                                 {0,  0,   0},     // 1
                                 {0,  0,   0},     // 2 x shift
                                 {0,  0,   0},     // 3 y shift
                                 {1.0, 1.0, 1.0},  // 4 x linear stretch
                                 {1.0, 1.0, 1.0},  // 5 y linear stretch
                                 {1.0, 1.0, 1.0},  // 6 radial stretch
                                 {1.0, 1.0, 1.0},  // 7 radial stretch 2nd order
                                 {0,   0,   0},    // 8 sharpen  
                                 {0,   0,   0}     // 9 erode    
                                 };


//--------------------------------------------------------------------------//
// chromaticaberration                                                      //
//--------------------------------------------------------------------------//
void chromaticaberration(void)
{
   g.getout=0;
   static Dialog *dialog;
   int j, k, ino;
   g.getout=0;
   ino = ci;
   if(ino<0) return;
   dialog = new Dialog;
   if(dialog==NULL){ message(g.nomemory); return; }
   g.inmenu++;  // no 'return's after this point

   //// Dialog

   strcpy(dialog->title,"Chromatic Aberration");

   strcpy(dialog->boxes[0],"");
   strcpy(dialog->boxes[1],"Color channel");
   strcpy(dialog->boxes[2],"x xhift");
   strcpy(dialog->boxes[3],"y shift");
   strcpy(dialog->boxes[4],"x linear stretch");
   strcpy(dialog->boxes[5],"y linear stretch");
   strcpy(dialog->boxes[6],"radial stretch");
   strcpy(dialog->boxes[7],"radial stretch 2nd order");
   strcpy(dialog->boxes[8],"Sharpen");
   strcpy(dialog->boxes[9],"Erode");

   dialog->boxtype[0]=LABEL;
   dialog->boxtype[1]=MULTILABEL; 
   for(k=2;k<=9;k++) dialog->boxtype[k]=MULTISTRING; 

   strcpy(dialog->answer[1][0], "Red");
   strcpy(dialog->answer[1][1], "Green");
   strcpy(dialog->answer[1][2], "Blue");
   for(j=2; j<10; j++)
   for(k=0; k<3; k++)
       sprintf(dialog->answer[j][k], "%g", ca_answers[j][k]);

   dialog->radiono[0]=0;
   for(j=0;j<10;j++){ for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO; }
   for(k=0;k<20;k++) dialog->wcount[k]=3;

   dialog->noofradios = 0;
   dialog->noofboxes  = 10;
   dialog->helptopic  = 1;  
   dialog->want_changecicb = 0;
   dialog->f1 = chromaticaberrationcheck;
   dialog->f2 = null;
   dialog->f3 = null;
   dialog->f4 = chromaticaberrationfinish;
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
   dialog->message[0] = 0;      
   dialog->busy = 0;
   dialogbox(dialog);
   g.inmenu--;
   return;
}


//--------------------------------------------------------------------------//
//  chromaticaberrationcheck                                                //
//--------------------------------------------------------------------------//
void chromaticaberrationcheck(dialoginfo *a, int radio, int box, int boxbutton)
{
   a = a;
   box=box; boxbutton=boxbutton; radio=radio;
   static filter *ff;
   static int hitff = 0;
   g.getout = 0;
   int j;
   int k,ino, owantr=g.wantr, owantg=g.wantg, owantb=g.wantb;
   ino = ci;
   if(!a->hit) for(k=0;k<3;k++) update_chromatic_aberration_dialog(a, k);
   if(!hitff)
   {    ff = new filter;
        ff->ino = ino;
        ff->type  = 1;
        ff->x1 = 0;
        ff->x2 = z[0].xsize-1;
        ff->y1 = 0;
        ff->y2 = z[0].ysize-1;
        ff->ksize = 3;
        ff->sharpfac = 0;  // user-adjustable param
        ff->range = 1;
        ff->kmult = 1;
        ff->maxbkg  = 1;
        ff->entireimage = 1;
        ff->diffsize = 1;
        ff->filename[0] = 0;
        ff->ithresh  = 0;
        ff->local_scale = 0;
        ff->want_progress_bar = 1;
        ff->do_filtering = 1;
        hitff=1;
    }

   if(between(box, 2, 9))  // User changed a number
   {   for(j=2; j<10; j++)
      {   ca_answers[j][0] = atof(a->answer[j][0]);
          ca_answers[j][1] = atof(a->answer[j][1]);
          ca_answers[j][2] = atof(a->answer[j][2]);
      }
   }
   if(radio==-2)         // OK key pressed
   {   //// h shift 
       g.wantr = 1; g.wantg = 0; g.wantb = 0;       
       if(ca_answers[2][0]) image_shift_frame(ino, cint(ca_answers[2][0]), 0);
       g.wantr = 0; g.wantg = 1; g.wantb = 0;       
       if(ca_answers[2][1]) image_shift_frame(ino, cint(ca_answers[2][1]), 0);
       g.wantr = 0; g.wantg = 0; g.wantb = 1;       
       if(ca_answers[2][2]) image_shift_frame(ino, cint(ca_answers[2][2]), 0);

       //// v shift
       g.wantr = 1; g.wantg = 0; g.wantb = 0;       
       if(ca_answers[3][0]) image_shift_frame(ino, 0, cint(ca_answers[3][0]));
       g.wantr = 0; g.wantg = 1; g.wantb = 0;       
       if(ca_answers[3][1]) image_shift_frame(ino, 0, cint(ca_answers[3][1]));
       g.wantr = 0; g.wantg = 0; g.wantb = 1;       
       if(ca_answers[3][2]) image_shift_frame(ino, 0, cint(ca_answers[3][2]));

       //// x linear stretch
       g.wantr = 1; g.wantg = 0; g.wantb = 0;       
       if(ca_answers[4][0]) shrink_image_interpolate_in_place(ino, ca_answers[4][0], 1.0);
       g.wantr = 0; g.wantg = 1; g.wantb = 0;       
       if(ca_answers[4][1]) shrink_image_interpolate_in_place(ino, ca_answers[4][1], 1.0);
       g.wantr = 0; g.wantg = 0; g.wantb = 1;       
       if(ca_answers[4][2]) shrink_image_interpolate_in_place(ino, ca_answers[4][2], 1.0);

       //// y linear stretch
       g.wantr = 1; g.wantg = 0; g.wantb = 0;       
       if(ca_answers[5][0]) shrink_image_interpolate_in_place(ino, 1.0, ca_answers[5][0]);
       g.wantr = 0; g.wantg = 1; g.wantb = 0;       
       if(ca_answers[5][1]) shrink_image_interpolate_in_place(ino, 1.0, ca_answers[5][1]);
       g.wantr = 0; g.wantg = 0; g.wantb = 1;       
       if(ca_answers[5][2]) shrink_image_interpolate_in_place(ino, 1.0, ca_answers[5][2]);

       //// radial stretch
       g.wantr = 1; g.wantg = 0; g.wantb = 0;       
       if(ca_answers[6][0]) shrink_image_interpolate_in_place(ino, 1.0, ca_answers[6][0]);
       if(ca_answers[6][0]) shrink_image_interpolate_in_place(ino, ca_answers[6][0], 1.0);
       g.wantr = 0; g.wantg = 1; g.wantb = 0;       
       if(ca_answers[6][1]) shrink_image_interpolate_in_place(ino, 1.0, ca_answers[6][1]);
       if(ca_answers[6][1]) shrink_image_interpolate_in_place(ino, ca_answers[6][1], 1.0);
       g.wantr = 0; g.wantg = 0; g.wantb = 1;       
       if(ca_answers[6][2]) shrink_image_interpolate_in_place(ino, 1.0, ca_answers[6][2]);
       if(ca_answers[6][2]) shrink_image_interpolate_in_place(ino, ca_answers[6][2], 1.0);

       ////  sharpen
       g.wantr = 1; g.wantg = 0; g.wantb = 0;       
       if(ca_answers[8][0]) { ff->sharpfac = cint(ca_answers[8][0]); filter_region(ff); }
       g.wantr = 0; g.wantg = 1; g.wantb = 0;       
       if(ca_answers[8][1]) { ff->sharpfac = cint(ca_answers[8][1]); filter_region(ff); }
       g.wantr = 0; g.wantg = 0; g.wantb = 1;       
       if(ca_answers[8][2]) { ff->sharpfac = cint(ca_answers[8][2]); filter_region(ff); }
       
       ////  erode
       g.wantr = 1; g.wantg = 0; g.wantb = 0;       
       if(ca_answers[9][0]) { dilate_erode_rgb(ino, 2, 1, 255, 0.01*ca_answers[9][0]); }
       g.wantr = 0; g.wantg = 1; g.wantb = 0;       
       if(ca_answers[9][1]) { dilate_erode_rgb(ino, 2, 1, 255, 0.01*ca_answers[9][1]); }
       g.wantr = 0; g.wantg = 0; g.wantb = 1;       
       if(ca_answers[9][2]) { dilate_erode_rgb(ino, 2, 1, 255, 0.01*ca_answers[9][2]); }

   }

 g.wantr = owantr; g.wantg = owantg; g.wantb = owantb;       
 return;
}


//--------------------------------------------------------------------------//
//  chromaticaberrationfinish                                               //
//--------------------------------------------------------------------------//
void chromaticaberrationfinish(dialoginfo *a)
{
   int j,k;
   for(j=2; j<10; j++)
   for(k=0; k<3; k++)
       ca_answers[j][k] = atof(a->answer[j][k]);
}

//-------------------------------------------------------------------------//
// update_chromatic_aberration_dialog                                      //
//-------------------------------------------------------------------------//
void update_chromatic_aberration_dialog(Dialog *a, int color)
{  
   Widget w;
   int j,k;
   int curpos;
   a->hit = 1;
   k = color;
   for(j=2; j<10; j++)
   {    //// Get rid of callback or it will go into a loop
        w = a->boxwidget[j][k];
        XtRemoveCallback(w, XmNvalueChangedCallback, 
            (XtCBP)dialogstringcb, (XtP)a);
        XtVaGetValues(w, XmNcursorPosition, &curpos, NULL);
        XtVaSetValues(w, XmNvalue, a->answer[j][k], 
                         XmNcursorPosition, curpos, 
                         NULL); 
        XtAddCallback(a->boxwidget[j][k], XmNvalueChangedCallback, 
            (XtCBP)dialogstringcb, (XtP)a);
       sprintf(a->answer[j][k], "%g", ca_answers[j][k]);
   }
}


