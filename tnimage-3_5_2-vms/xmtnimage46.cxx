//--------------------------------------------------------------------------//
// xmtnimage46.cc                                                           //
// Latest revision: 03-26-2003                                              //
// Copyright (C) 2003 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
// morphometry                                                              //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"
extern Globals     g;
extern Image      *z;
extern int         ci;
uint imax(uint a, uint b);
uint imin(uint a, uint b);
const int fate4[16] = {  0, 0,0,0,0, 0,1,0,0, 1,0,0,0, 1,0,0};     

const int fate8[256] = { 0,0,0,0,0, 1,0,0,0, 0,0,0,1, 1,0,0,0, 
                           1,0,0,0, 1,0,0,1, 1,0,0,1, 1,0,0,0,
                           1,1,1,1, 1,1,1,0, 0,0,0,1, 1,0,0,1, 
                           1,1,1,1, 1,1,1,1, 1,0,0,1, 1,0,0,0,
   
                           1,1,1,1, 1,1,1,0, 0,0,0,0, 1,0,0,0, 
                           1,0,0,0, 1,0,0,0, 0,0,0,0, 0,0,0,0,
                           1,1,1,1, 1,1,1,0, 0,0,0,0, 1,0,0,0, 
                           1,0,0,0, 1,0,1,0, 0,0,0,0, 0,0,0,0,
   
                           1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,0, 
                           1,0,0,0, 1,0,0,1, 1,0,0,1, 1,0,0,0,
                           1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 
                           1,1,0,1, 1,1,1,1, 1,0,0,1, 1,0,0,1,
   
                           1,1,1,1, 1,1,1,0, 0,0,0,0, 1,0,0,0, 
                           1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
                           1,1,1,1, 1,1,1,0, 0,0,0,0, 1,0,0,0, 
                           1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0 };

static inline void fifo_add(array<int>*fifo, int index);
static inline int fifo_first(array<int>*fifo);
static inline int fifo_empty(void);
static inline void getxy(int pixelno, int xsize, int &i, int &j);
int ptr_first=0, ptr_last=0;


//--------------------------------------------------------------------------//
// watershed auxiliary functions                                            //
//--------------------------------------------------------------------------//
static inline void fifo_add(array<int>*fifo, int index)
{
   fifo->data[ptr_last++] = index;  
   if(ptr_last >= fifo->elements) ptr_last = 0;
}
static inline int fifo_first(array<int>*fifo)
{
   int answer = fifo->data[ptr_first++];  
   if(ptr_first >= fifo->elements) ptr_first = 0;
   return answer;
}
static inline int fifo_empty(void)
{
   return(ptr_first==ptr_last);
}
static inline void getxy(int pixelno, int xsize, int &i, int &j)
{
   div_t quotient;
   quotient = div(pixelno, xsize);
   i = quotient.rem; 
   j = quotient.quot;
}


//-------------------------------------------------------------------------//
// morphfilter - morphological filtering                                   //
// Opening is an erosion followed by a dilation operation.                 //
// Closing is a dilation followed by an erosion operation.                 //
//-------------------------------------------------------------------------//
void morphfilter(int noofargs, char **arg)
{
  g.morph_operation = -1;
  noofargs=noofargs; arg=arg;
  static Dialog *dialog;
  int j,k;
  dialog = new Dialog;
  if(dialog==NULL){ message(g.nomemory); return; }
  strcpy(dialog->title,"Morphologic Filtering");

  //-----Dialog box to get title & other options------------//

  strcpy(dialog->radio[0][0],"Operation");
  strcpy(dialog->radio[0][1],"Threshold");
  strcpy(dialog->radio[0][2],"Erosion");
  strcpy(dialog->radio[0][3],"Dilation");
  strcpy(dialog->radio[0][4],"Skeletonization");
  strcpy(dialog->radio[0][5],"Quick segmentation");
  strcpy(dialog->radio[0][6],"Watershed segmentation");
  strcpy(dialog->radio[0][7],"Watershed boundaries");
  strcpy(dialog->radio[0][8],"Contour map");
  strcpy(dialog->radio[0][9],"Restore original");
  dialog->radiono[0]=10;
  dialog->radioset[0] = g.morph_operation;

  strcpy(dialog->radio[1][0],"Structuring element");
  strcpy(dialog->radio[1][1],"N4 (4-connected set)");
  strcpy(dialog->radio[1][2],"N8 (8-connected set)");
  dialog->radiono[1]=3;
  dialog->radioset[1] = g.morph_se;

  strcpy(dialog->radio[2][0],"Type");
  strcpy(dialog->radio[2][1],"Binary");
  strcpy(dialog->radio[2][2],"Graylevel");
  dialog->radiono[2]=3;
  dialog->radioset[2] = g.morph_type;

  strcpy(dialog->radio[3][0],"Maximum signal");
  strcpy(dialog->radio[3][1],"Black");
  strcpy(dialog->radio[3][2],"White");
  dialog->radiono[3]=3;
  if(g.morph_maxsignal==0) dialog->radioset[3] = 1;
  else dialog->radioset[3] = 2;

  for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;  

  strcpy(dialog->boxes[0],"Threshold");
  strcpy(dialog->boxes[1],"Threshold");
  dialog->boxtype[0]=LABEL;
  dialog->boxtype[1]=DOUBLECLICKBOX;
  sprintf(dialog->answer[1][0],"%g", g.morph_thresh);
  dialog->boxmin[1]=0; dialog->boxmax[1]=1;

  strcpy(dialog->boxes[2],"Watershed\nkernel");
  strcpy(dialog->boxes[3],"Watershed Kernel");
  dialog->boxtype[2]=LABEL;
  dialog->boxtype[3]=INTCLICKBOX;
  sprintf(dialog->answer[3][0],"%d", g.morph_ksize);
  dialog->boxmin[3]=1; dialog->boxmax[3]=10;

  strcpy(dialog->boxes[4],"Contour sep.");
  strcpy(dialog->boxes[5],"Contour Separation");
  dialog->boxtype[4]=LABEL;
  dialog->boxtype[5]=INTCLICKBOX;
  sprintf(dialog->answer[5][0],"%d", g.morph_contour_separation);
  dialog->boxmin[5]=1; dialog->boxmax[5]=256;

  dialog->want_changecicb = 0;
  dialog->f1 = morphcheck;
  dialog->f2 = null;
  dialog->f3 = null;
  dialog->f4 = null;
  dialog->f5 = null;
  dialog->f6 = null;
  dialog->f7 = null;
  dialog->f8 = null;
  dialog->f9 = null;
  dialog->noofradios = 4;
  dialog->noofboxes = 6;
  dialog->helptopic = 53;
  dialog->transient = 1;
  dialog->wantraise = 0;

  //// Use a custom format dialog box
  dialog->radiousexy = 0;
  dialog->boxusexy = 1;
  dialog->width = 170;
  dialog->height = 530;

  int y=420, dx=70, dy=20;
  //// threshold
  dialog->boxxy[0][0] = 80;
  dialog->boxxy[0][1] = y;
  dialog->boxxy[0][2] = dx;
  dialog->boxxy[0][3] = dy;

  dialog->boxxy[1][0] = 6;
  dialog->boxxy[1][1] = y;
  dialog->boxxy[1][2] = dx;
  dialog->boxxy[1][3] = dy;

  y += cint(1.2*dy);

  //// watershed kernel
  dialog->boxxy[2][0] = 80;
  dialog->boxxy[2][1] = y-6;
  dialog->boxxy[2][2] = dx;
  dialog->boxxy[2][3] = dy;

  dialog->boxxy[3][0] = 6;
  dialog->boxxy[3][1] = y;
  dialog->boxxy[3][2] = dx;
  dialog->boxxy[3][3] = dy;

  y += cint(1.2*dy);

  //// contour sep
  dialog->boxxy[4][0] = 80;
  dialog->boxxy[4][1] = y;
  dialog->boxxy[4][2] = dx;
  dialog->boxxy[4][3] = dy;

  dialog->boxxy[5][0] = 6;
  dialog->boxxy[5][1] = y;
  dialog->boxxy[5][2] = dx;
  dialog->boxxy[5][3] = dy;


  dialog->spacing = 0;
  dialog->radiostart = 2;
  dialog->radiowidth = 50;
  dialog->boxstart = 2;
  dialog->boxwidth = 50;
  dialog->labelstart = 87;
  dialog->labelwidth = 50;        
  
  strcpy(dialog->path,".");
  dialog->message[0]=0;      
  dialog->busy = 0;
  dialogbox(dialog);
}


//--------------------------------------------------------------------------//
//  morphcheck                                                              //
//--------------------------------------------------------------------------//
void morphcheck(dialoginfo *a, int radio, int box, int boxbutton)
{
  static int inmorphcheck=0;
  g.morph_operation = -1;
  if(radio >=0) g.morph_operation = a->radioset[0];
  g.morph_se        = a->radioset[1];
  g.morph_type      = a->radioset[2];
  g.morph_thresh    = atof(a->answer[1][0]);
  g.morph_ksize     = atoi(a->answer[3][0]);
  g.morph_contour_separation = atoi(a->answer[5][0]);

  if(a->radioset[3]==1) g.morph_maxsignal = 0;
  else g.morph_maxsignal = int(g.maxvalue[z[ci].bpp]);
  int maxsig = g.morph_maxsignal;
  int bpp = z[ci].bpp;
  int se = g.morph_se;
  int mcs = g.morph_contour_separation;
  uint ithresh = (uint)cint((1.0-g.morph_thresh) * g.maxvalue[bpp]);
  if(inmorphcheck) return;  //// XtSetValues->dialogstringcb->fftcheck
  inmorphcheck=1;
  box=box; boxbutton=boxbutton;

  if(radio==0 || radio==-2) switch(g.morph_operation)
  {
      case 1: threshold(ci, ithresh);
              break;
      case 2: if(g.morph_type==1) erode_binary(ci, se, maxsig, ithresh);
              if(g.morph_type==2) dilate_erode_gray(ci, se, 1, maxsig, ithresh);
              break;
      case 3: if(g.morph_type==1) dilate_binary(ci, se, maxsig, ithresh);
              if(g.morph_type==2) dilate_erode_gray(ci, se, 0, maxsig, ithresh);
              break;
      case 4: if(g.morph_type==1) skeletonize_binary(ci, se, maxsig, ithresh);
              if(g.morph_type==2) skeletonize_gray(ci, se, maxsig, ithresh);
              break;
      case 5: quick_segmentation(ci, se, maxsig, ithresh);
              break;
      case 6: watershed(ci, g.morph_ksize, maxsig, 0);
              break;
      case 7: watershed(ci, g.morph_ksize, maxsig, 1);
              break;
      case 8: create_contour_map(ci, se, maxsig, mcs);
              break;
      case 9: restoreimage(1);
              break;
  }
  if(radio>=0) unset_radio(a, 0);
  inmorphcheck=0;
}


//--------------------------------------------------------------------------//
//  erode_binary  (se = structuring element)                                //
//                                                                          //
//   xxx              x                                                     //
//   xPx n8 (se=2)   xPx   n4 (se=1)                                        //  
//   xxx              x                                                     //
//--------------------------------------------------------------------------//
void erode_binary(int ino, int se, int maxsignal, uint thresh)
{
   int bpp,b,f,i,i2,j,bufrow,bufrow_to_draw,v1,val;
   int x1,x2,y1,y2;
   int minsignal;
   
   const int SIZE = 3;              // size of structuring element
   bpp = z[ino].bpp;
   b = g.off[bpp];                  // size of pixel in bytes
   x1 = 1;
   x2 = g.lrx - g.ulx;
   y1 = 1;
   y2 = g.lry - g.uly;
   f = z[ino].cf;
   array<int> data(4+x2-x1,6);      // temporary storage
   if(!data.allocated){ message(g.nomemory); return; }
   bufrow=0;
   if(maxsignal==0) minsignal = int(g.maxvalue[z[ino].bpp]);
   else minsignal = 0;
   
   ////  Erode - binary
   for(j=y1+SIZE; j<y2-SIZE; j++)
   {   for(i=x1+SIZE,i2=(x1+SIZE)*b; i<x2-SIZE; i++,i2+=b)
       {  switch(se)
          {   case 1:  // nearest 4
                if(maxsignal > minsignal)
                {  if((pixelat(z[ino].image[f][j-1]+i2  , bpp) > thresh) &&
                      (pixelat(z[ino].image[f][j+1]+i2  , bpp) > thresh) &&
                      (pixelat(z[ino].image[f][j  ]+i2-b, bpp) > thresh) &&
                      (pixelat(z[ino].image[f][j  ]+i2+b, bpp) > thresh)) v1=1;
                   else v1 = 0;
                }else
                {  if((pixelat(z[ino].image[f][j-1]+i2  , bpp) < thresh) &&
                      (pixelat(z[ino].image[f][j+1]+i2  , bpp) < thresh) &&
                      (pixelat(z[ino].image[f][j  ]+i2-b, bpp) < thresh) &&
                      (pixelat(z[ino].image[f][j  ]+i2+b, bpp) < thresh)) v1=1;
                   else v1 = 0;
                }
		if(v1) data.p[bufrow][i-(x1+SIZE)] = maxsignal; 
                else   data.p[bufrow][i-(x1+SIZE)] = minsignal;
                break;
              case 2:  // nearest 8
                if(maxsignal > minsignal)
                {  if((pixelat(z[ino].image[f][j-1]+i2-b, bpp) > thresh) &&
                      (pixelat(z[ino].image[f][j-1]+i2  , bpp) > thresh) &&
                      (pixelat(z[ino].image[f][j-1]+i2+b, bpp) > thresh) &&
                      (pixelat(z[ino].image[f][j  ]+i2-b, bpp) > thresh) &&
                      (pixelat(z[ino].image[f][j  ]+i2+b, bpp) > thresh) &&
                      (pixelat(z[ino].image[f][j+1]+i2-b, bpp) > thresh) &&
                      (pixelat(z[ino].image[f][j+1]+i2  , bpp) > thresh) &&
                      (pixelat(z[ino].image[f][j+1]+i2+b, bpp) > thresh)) v1=1;
                   else v1=0;
                }else
                {  if((pixelat(z[ino].image[f][j-1]+i2-b, bpp) < thresh) &&
                      (pixelat(z[ino].image[f][j-1]+i2  , bpp) < thresh) &&
                      (pixelat(z[ino].image[f][j-1]+i2+b, bpp) < thresh) &&
                      (pixelat(z[ino].image[f][j  ]+i2-b, bpp) < thresh) &&
                      (pixelat(z[ino].image[f][j  ]+i2+b, bpp) < thresh) &&
                      (pixelat(z[ino].image[f][j+1]+i2-b, bpp) < thresh) &&
                      (pixelat(z[ino].image[f][j+1]+i2  , bpp) < thresh) &&
                      (pixelat(z[ino].image[f][j+1]+i2+b, bpp) < thresh)) v1=1;
                   else v1=0;
                }
                //// Set center pixel to 1 or 0
                if(v1) data.p[bufrow][i-(x1+SIZE)] = maxsignal; 
                else   data.p[bufrow][i-(x1+SIZE)] = minsignal;
                break;
              default: break;
          }
       }
       ////  Draw row of pixels, making sure not to overwrite pixels
       ////  that need to be read.
       if(j >= y1+SIZE+2)          
       {   bufrow_to_draw = bufrow - SIZE;
           if(bufrow_to_draw < 0) bufrow_to_draw += 6;
           for(i=x1+SIZE,i2=(x1+SIZE)*b; i<x2-SIZE; i++,i2+=b)
           {   val = data.p[bufrow_to_draw][i-(x1+SIZE)];
               putpixelbytes(z[ino].image[f][j-SIZE]+i2, val, 1, bpp, 1);
               data.p[bufrow_to_draw][i-(x1+SIZE)] = 0;
           }
           if(bufrow >=5) bufrow = -1;
       }
       bufrow++;
   }
   z[ino].touched=1;
   rebuild_display(ino);
   redraw(ino);
}



//--------------------------------------------------------------------------//
//  dilate_binary  (se = structuring element)                               //
//--------------------------------------------------------------------------//
void dilate_binary(int ino, int se, int maxsignal, uint thresh)
{
   int bpp,b,f,i,i2,j,j2,bufrow,bufrow_to_draw,y;
   int x1,x2,y1,y2;
   int minsignal;
   uint pix=0, val;
   
   const int SIZE = 3;              // size of structuring element
   bpp = z[ino].bpp;

   b = g.off[bpp];                  // size of pixel in bytes
   x1 = 2;
   x2 = g.lrx - g.ulx - 1;
   y1 = 2;
   y2 = g.lry - g.uly - 1;
   f = z[ino].cf;
   array<int> data(4+x2-x1,12);      // temporary storage
   if(!data.allocated){ message(g.nomemory); return; }
   bufrow=1;
   if(maxsignal==0) minsignal = int(g.maxvalue[z[ino].bpp]);
   else minsignal = 0;

   ////  Dilate - binary
   for(j=y1; j<y2; j++)
   {  
       for(i=x1,i2=x1*b; i<x2; i++,i2+=b)
       {  switch(se)
          {   case 1:  // nearest 4
                if(maxsignal > minsignal)
                {  
                   val = pixelat(z[ino].image[f][j]+i2, bpp);
                   if(val >= thresh)
                   {  
                       data.p[bufrow-1][i-x1  ] = maxsignal; 
                       data.p[bufrow  ][i-x1-b] = maxsignal; 
		       data.p[bufrow  ][i-x1  ] = maxsignal; 
                       data.p[bufrow  ][i-x1+b] = maxsignal;           
                       data.p[bufrow+1][i-x1  ] = maxsignal;                                           
		   }
                }else
                {
                   val = pixelat(z[ino].image[f][j]+i2, bpp);
                   if(val < thresh)
                   {  
                       data.p[bufrow-1][i-x1  ] = minsignal; 
                       data.p[bufrow  ][i-x1-b] = minsignal; 
                       data.p[bufrow  ][i-x1  ] = minsignal; 
                       data.p[bufrow  ][i-x1+b] = minsignal;           
                       data.p[bufrow+1][i-x1  ] = minsignal; 
                   }
                }
                break;
              case 2:  // nearest 8
                if(maxsignal > minsignal)
                {  
                   val = pixelat(z[ino].image[f][j]+i2, bpp);
                   if(val >= thresh)
                   {   data.p[bufrow-1][i-x1-b] = maxsignal; 
                       data.p[bufrow-1][i-x1  ] = maxsignal; 
                       data.p[bufrow-1][i-x1+b] = maxsignal; 
                       data.p[bufrow  ][i-x1-b] = maxsignal; 
                       data.p[bufrow  ][i-x1  ] = maxsignal; 
                       data.p[bufrow  ][i-x1+b] = maxsignal; 
                       data.p[bufrow+1][i-x1-b] = maxsignal;           
                       data.p[bufrow+1][i-x1  ] = maxsignal;           
                       data.p[bufrow+1][i-x1+b] = maxsignal;           
                   }
                }else
                {  
                   val = pixelat(z[ino].image[f][j]+i2, bpp);
                   if(val < thresh)
                   {   data.p[bufrow-1][i-x1-b] = minsignal; 
                       data.p[bufrow-1][i-x1  ] = minsignal; 
                       data.p[bufrow-1][i-x1+b] = minsignal; 
                       data.p[bufrow  ][i-x1-b] = minsignal; 
                       data.p[bufrow  ][i-x1  ] = minsignal; 
                       data.p[bufrow  ][i-x1+b] = minsignal;           
                       data.p[bufrow+1][i-x1-b] = minsignal; 
                       data.p[bufrow+1][i-x1  ] = minsignal; 
                       data.p[bufrow+1][i-x1+b] = minsignal;           
                   }
                }
                break;
              default: break;
          }
       }
       ////  Draw SIZE rows of pixels, making sure not to overwrite pixels
       ////  that need to be read. Skip SIZE rows in bufr and reset to 0.
       if(j >= y1+2)          
       {   bufrow_to_draw = bufrow + SIZE;
           if(bufrow_to_draw >=12) bufrow_to_draw = 1;
           for(j2=bufrow_to_draw-1; j2<=bufrow_to_draw+1; j2++)
           {
              y = j-SIZE+j2-(bufrow_to_draw-1)-1;
              for(i=x1,i2=x1*b; i<x2; i++,i2+=b)
              {    val = data.p[j2][i-x1];
                   if(maxsignal > minsignal) pix = 0;
                   else pix = (uint)g.maxvalue[bpp];
                   if(maxsignal > minsignal)
                   {  if(val>pix) putpixelbytes(z[ino].image[f][y]+i2,val,1,bpp);}
                   else             
                   {  if(val<pix) putpixelbytes(z[ino].image[f][y]+i2,val,1,bpp);}
                   data.p[j2][i-x1] = 0;
              }
           }
       }
       bufrow+=SIZE;
       if(bufrow >= 12) bufrow = 1;
   }
   z[ino].touched=1;
   rebuild_display(ino);
   redraw(ino);
}


//--------------------------------------------------------------------------//
//  dilate_erode_gray  (se = structuring element)                           //
//--------------------------------------------------------------------------//
void dilate_erode_gray(int ino, int se, int want_erode, int maxsignal, uint thresh)
{
   int bpp,b,f,i,i2,j,j2,bufrow,bufrow_to_draw,y;
   int x1,x2,y1,y2;
   int minsignal;
   uint maxpix, value, val[9];
   
   const int SIZE = 3;              // size of structuring element
   bpp = z[ino].bpp;
   thresh = 0;

   b = g.off[bpp];                  // size of pixel in bytes
   x1 = 1;
   x2 = g.lrx - g.ulx;
   y1 = 1;
   y2 = g.lry - g.uly;
   f = z[ino].cf;
   array<uint> data(4+x2-x1,12);      // temporary storage
   if(!data.allocated){ message(g.nomemory); return; }
   bufrow=1;
   if(maxsignal==0) minsignal = int(g.maxvalue[z[ino].bpp]);
   else minsignal = 0;
   uint(*func) (uint a, uint b);

   ////  Dilate - grayscale
   for(j=y1; j<y2; j++)
   {  
       if(want_erode){ if(maxsignal > minsignal) func = imin; else func = imax; }
       else          { if(maxsignal > minsignal) func = imax; else func = imin; }
       for(i=x1,i2=x1*b; i<x2; i++,i2+=b)
       {  switch(se)
          {   case 1:  // nearest 4
                val[0] = pixelat(z[ino].image[f][j-1]+i2  , bpp);
                val[1] = pixelat(z[ino].image[f][j  ]+i2-b, bpp);
                val[2] = pixelat(z[ino].image[f][j  ]+i2  , bpp);
                val[3] = pixelat(z[ino].image[f][j  ]+i2+b, bpp);
                val[4] = pixelat(z[ino].image[f][j+1]+i2  , bpp);
                maxpix = func(val[0], func(val[1], func(val[2], 
                         func(val[3], val[4]))));
                data.p[bufrow][i-x1] = maxpix; 
                break;
              case 2:  // nearest 8
                val[0] = pixelat(z[ino].image[f][j-1]+i2-b, bpp);
                val[1] = pixelat(z[ino].image[f][j  ]+i2-b, bpp);
                val[2] = pixelat(z[ino].image[f][j+1]+i2-b, bpp);
                val[3] = pixelat(z[ino].image[f][j-1]+i2  , bpp);
                val[4] = pixelat(z[ino].image[f][j  ]+i2  , bpp);
                val[5] = pixelat(z[ino].image[f][j+1]+i2  , bpp);
                val[6] = pixelat(z[ino].image[f][j-1]+i2+b, bpp);
                val[7] = pixelat(z[ino].image[f][j  ]+i2+b, bpp);
                val[8] = pixelat(z[ino].image[f][j+1]+i2+b, bpp);
                maxpix = func(val[0], func(val[1], func(val[2], 
                         func(val[3], func(val[4], func(val[5],
                         func(val[6], func(val[7], val[8]))))))));
                data.p[bufrow][i-x1] = maxpix; 
                break;
              default: break;
          }
       }
       ////  Draw SIZE rows of pixels, making sure not to overwrite pixels
       ////  that need to be read. Skip SIZE rows in bufr and reset to 0.
       if(j >= y1+2)          
       {   bufrow_to_draw = bufrow + SIZE;
           if(bufrow_to_draw >=12) bufrow_to_draw = 1;
           j2 = bufrow_to_draw;
           y = j-SIZE;
           for(i=x1,i2=x1*b; i<x2; i++,i2+=b)
           {    value = data.p[j2][i-x1];
                putpixelbytes(z[ino].image[f][y]+i2,value,1,bpp);
                data.p[j2][i-x1] = 0;
           }
       }
       bufrow+=SIZE;
       if(bufrow >= 12) bufrow = 1;
   }
   z[ino].touched=1;
   rebuild_display(ino);
   redraw(ino);
}

uint imax(uint a, uint b) { return max(a,b); }
uint imin(uint a, uint b) { return min(a,b); }


//--------------------------------------------------------------------------//
//  dilate_erode_rgb   (se = structuring element)                           //
//--------------------------------------------------------------------------//
void dilate_erode_rgb(int ino, int se, int want_erode, int maxsignal, double erodefac)
{
   int bpp,b,f,i,i2,j,j2,bufrow,bufrow_to_draw,y;
   int x1,x2,y1,y2,rr,gg,bb,orr,ogg,obb;
   int minsignal;
   uint maxred, maxgreen, maxblue, value;
   int val[9], red[9], green[9], blue[9];
   
   const int SIZE = 3;              // size of structuring element
   bpp = z[ino].bpp;
   se = se;
   b = g.off[bpp];                  // size of pixel in bytes
   x1 = b;
   x2 = g.lrx - g.ulx;
   y1 = 1;
   y2 = g.lry - g.uly;
   f = z[ino].cf;
   array<uint> data(4+x2-x1,12);      // temporary storage
   if(!data.allocated){ message(g.nomemory); return; }
   bufrow=1;
   if(maxsignal==0) minsignal = int(g.maxvalue[z[ino].bpp]);
   else minsignal = 0;
   uint(*func) (uint a, uint b);

   ////  Dilate - grayscale
   for(j=y1; j<y2; j++)
   {  
       if(want_erode){ if(maxsignal > minsignal) func = imin; else func = imax; }
       else          { if(maxsignal > minsignal) func = imax; else func = imin; }
       for(i=x1,i2=x1*b; i<x2; i++,i2+=b)
       {  switch(se)
          {   case 1:  // nearest 4
                val[0] = pixelat(z[ino].image[f][j-1]+i2  , bpp);
                val[1] = pixelat(z[ino].image[f][j  ]+i2-b, bpp);
                val[2] = pixelat(z[ino].image[f][j  ]+i2  , bpp);
                val[3] = pixelat(z[ino].image[f][j  ]+i2+b, bpp);
                val[4] = pixelat(z[ino].image[f][j+1]+i2  , bpp);
                valuetoRGB(val[0], red[0], green[0], blue[0], bpp);
                valuetoRGB(val[1], red[1], green[1], blue[1], bpp);
                valuetoRGB(val[2], red[2], green[2], blue[2], bpp);
                valuetoRGB(val[3], red[3], green[3], blue[3], bpp);
                valuetoRGB(val[4], red[4], green[4], blue[4], bpp);
                maxred   = func(red[0], func(red[1], func(red[2], func(red[3], red[4]))));
                maxgreen = func(green[0], func(green[1], func(green[2], func(green[3], green[4]))));
                maxblue  = func(blue[0], func(blue[1], func(blue[2], func(blue[3], blue[4]))));
                valuetoRGB(val[2], orr, ogg, obb, bpp);
                rr = cint(erodefac*(double)maxred   + (1.0-erodefac)*(double)orr); 
                gg = cint(erodefac*(double)maxgreen + (1.0-erodefac)*(double)ogg); 
                bb = cint(erodefac*(double)maxblue  + (1.0-erodefac)*(double)obb);                
                data.p[bufrow][i-x1] = RGBvalue(rr,gg,bb,bpp);
              case 2:  // nearest 8
                val[0] = pixelat(z[ino].image[f][j-1]+i2-b, bpp);
                val[1] = pixelat(z[ino].image[f][j  ]+i2-b, bpp);
                val[2] = pixelat(z[ino].image[f][j+1]+i2-b, bpp);
                val[3] = pixelat(z[ino].image[f][j-1]+i2  , bpp);
                val[4] = pixelat(z[ino].image[f][j  ]+i2  , bpp);
                val[5] = pixelat(z[ino].image[f][j+1]+i2  , bpp);
                val[6] = pixelat(z[ino].image[f][j-1]+i2+b, bpp);
                val[7] = pixelat(z[ino].image[f][j  ]+i2+b, bpp);
                val[8] = pixelat(z[ino].image[f][j+1]+i2+b, bpp);
                valuetoRGB(val[0], red[0], green[0], blue[0], bpp);
                valuetoRGB(val[1], red[1], green[1], blue[1], bpp);
                valuetoRGB(val[2], red[2], green[2], blue[2], bpp);
                valuetoRGB(val[3], red[3], green[3], blue[3], bpp);
                valuetoRGB(val[4], red[4], green[4], blue[4], bpp);
                valuetoRGB(val[5], red[5], green[5], blue[5], bpp);
                valuetoRGB(val[6], red[6], green[6], blue[6], bpp);
                valuetoRGB(val[7], red[7], green[7], blue[7], bpp);
                valuetoRGB(val[8], red[8], green[8], blue[8], bpp);
                valuetoRGB(val[9], red[9], green[9], blue[9], bpp);

                maxred   = func(red[0], func(red[1], func(red[2], 
                           func(red[3], func(red[4], func(red[5], 
                           func(red[6], func(red[7], red[8]))))))));
                maxgreen = func(green[0], func(green[1], func(green[2], 
                           func(green[3], func(green[4], func(green[5], 
                           func(green[6], func(green[7], green[8]))))))));
                maxblue  = func(blue[0], func(blue[1], func(blue[2], 
                           func(blue[3], func(blue[4], func(blue[5], 
                           func(blue[6], func(blue[7], blue[8]))))))));
                valuetoRGB(val[4], orr, ogg, obb, bpp);
                rr = cint(erodefac*(double)maxred   + (1.0-erodefac)*(double)orr); 
                gg = cint(erodefac*(double)maxgreen + (1.0-erodefac)*(double)ogg); 
                bb = cint(erodefac*(double)maxblue  + (1.0-erodefac)*(double)obb);                
                data.p[bufrow][i-x1] = RGBvalue(rr,gg,bb,bpp);
                break;
              default: break;
          }
       }
       ////  Draw SIZE rows of pixels, making sure not to overwrite pixels
       ////  that need to be read. Skip SIZE rows in bufr and reset to 0.
       if(j >= y1+2)          
       {   bufrow_to_draw = bufrow + SIZE;
           if(bufrow_to_draw >=12) bufrow_to_draw = 1;
           j2 = bufrow_to_draw;
           y = j-SIZE;
           for(i=x1,i2=x1*b; i<x2; i++,i2+=b)
           {    value = data.p[j2][i-x1];
                putpixelbytes(z[ino].image[f][y]+i2,value,1,bpp);
                data.p[j2][i-x1] = 0;
           }
       }
       bufrow+=SIZE;
       if(bufrow >= 12) bufrow = 1;
   }
   z[ino].touched=1;
   rebuild_display(ino);
   redraw(ino);
}



//--------------------------------------------------------------------------//
//  skeletonize_binary                                                      //
//--------------------------------------------------------------------------//
void skeletonize_binary(int ino, int se, int maxsignal, uint thresh)
{
   const int SIZE = 3;              // size of structuring element
   int answer=0,bpp,b,f,i,i2,j,j2,bufrow,bufrow_to_draw,minsignal,y;
   int x1,x2,y1,y2;
   uint value;

   bpp = z[ino].bpp;
   b = g.off[bpp];                  // size of pixel in bytes
   x1 = 1;
   x2 = g.lrx - g.ulx;
   y1 = 1;
   y2 = g.lry - g.uly;
   f = z[ino].cf;

   if(maxsignal==0) minsignal = int(g.maxvalue[z[ino].bpp]);
   else minsignal = 0;

   array<uint> data(4+x2-x1,12);      // temporary storage
   if(!data.allocated){ message(g.nomemory); return; }
   bufrow=1;
   for(j=y1; j<y2; j++)
   {   for(i=x1,i2=x1*b; i<x2; i++,i2+=b)
       {  switch(se)
          {   case 1: answer = joins_features4(ino,f,i2,j,bpp,thresh); break;
              case 2: answer = joins_features8(ino,f,i2,j,bpp,thresh); break;
              default: break;
          }
          if(maxsignal > minsignal)
              data.p[bufrow][i-x1] = answer * maxsignal; 
          else
              data.p[bufrow][i-x1] = (1-answer) * minsignal; 
       }
       ////  Draw SIZE rows of pixels, making sure not to overwrite pixels
       ////  that need to be read. Skip SIZE rows in bufr and reset to 0.
       if(j >= y1+2)          
       {   bufrow_to_draw = bufrow + SIZE;
           if(bufrow_to_draw >=12) bufrow_to_draw = 1;
           j2 = bufrow_to_draw;
           y = j-SIZE;
           for(i=x1,i2=x1*b; i<x2; i++,i2+=b)
           {    value = data.p[j2][i-x1];
                putpixelbytes(z[ino].image[f][y]+i2,value,1,bpp);
                data.p[j2][i-x1] = 0;
           }
       }
       bufrow+=SIZE;
       if(bufrow >= 12) bufrow = 1;
   }
   z[ino].touched=1;
   rebuild_display(ino);
   redraw(ino);                     
}



//--------------------------------------------------------------------------//
// skeletonize_gray                                                         //
//--------------------------------------------------------------------------//
void skeletonize_gray(int ino, int struct_element, int maxsignal, uint thresh)
{ 
  //// HeBO3MOXHbIN     
  skeletonize_binary(ino, struct_element, maxsignal, thresh);  
}


//--------------------------------------------------------------------------//
// threshold                                                                //
//--------------------------------------------------------------------------//
void threshold(int ino, uint thresh)
{
   int bpp,b,f,i,i2,j;
   int x1,x2,y1,y2;
   uint val;
   bpp = z[ino].bpp;
   b = g.off[bpp];                  // size of pixel in bytes
   uint maxvalue = int(g.maxvalue[bpp]);
   x1 = 1;
   x2 = g.lrx - g.ulx;
   y1 = 1;
   y2 = g.lry - g.uly;
   f = z[ino].cf;
   for(j=y1; j<y2; j++)
   for(i=x1,i2=x1*b; i<x2; i++,i2+=b)
   {   val = pixelat(z[ino].image[f][j]+i2, bpp);
       if(val > thresh) val = maxvalue; else val = 0;
       putpixelbytes(z[ino].image[f][j]+i2,val,1,bpp);
   }
   z[ino].touched=1;
   rebuild_display(ino);
   redraw(ino);
}


//--------------------------------------------------------------------------//
// quick segmentation                                                       //
//--------------------------------------------------------------------------//
void quick_segmentation(int ino, int struct_element, int maxsignal, uint thresh)
{ 
   int b,bpp,f,i,i2,j,xsize,ysize; 
   uint v;
   bpp = z[ino].bpp;
   b = g.off[bpp];                  // size of pixel in bytes
   f = z[ino].cf;
   xsize = z[ino].xsize;
   ysize = z[ino].ysize;
   
   array<uint> edm(z[ino].xsize, z[ino].ysize);      // Euclidean distance map
   if(!edm.allocated){ message(g.nomemory); return; }
   create_edm(ino, struct_element, thresh, &edm);
   for(j=0; j<ysize; j++)
   for(i=0,i2=0; i<xsize; i++,i2+=b)
   { 
       v = 50 * edm.p[j][i];
       if(between(v, 100, 255)) v = (int)g.maxvalue[bpp]-1; else v = 0; 
       if(maxsignal == 0) v = (int)g.maxvalue[bpp] - v;
       putpixelbytes(z[ino].image[f][j]+i2, v, 1, bpp);      
   }
   rebuild_display(ino);
   redraw(ino);
}


//--------------------------------------------------------------------------//
// create_contour_map                                                       //
//--------------------------------------------------------------------------//
void create_contour_map(int ino, int struct_element, int maxsignal, 
   int contour_separation)
{ 
  Widget www, scrollbar;
  int b,bpp,ct,f,i,i2,increment,j,ok,newino,maxpix,xsize,ysize,thresh,xpos,ypos; 
  uint v;
  bpp = z[ino].bpp;
  ct = z[ino].colortype;
  b = g.off[bpp];                  // size of pixel in bytes
  f = z[ino].cf;
  xsize = z[ino].xsize;
  ysize = z[ino].ysize;
  xpos = z[ino].xpos+10;
  ypos = z[ino].ypos+10;
  maxpix = (int)g.maxvalue[bpp]-1;

  if(bpp > 16) { message("Image must be 16\nbits/pixel or less", WARNING, 55); return; }
  if(ct != GRAY){ message("Image must be grayscale", WARNING, 55); return; }

  switch(bpp)
  {    case 7:
       case 8: increment = contour_separation; break;
       case 15: 
       case 16: increment = 256 * contour_separation; break;
       default: increment = 65536 * contour_separation;
  }
  
  array<uint> edm(z[ino].xsize, z[ino].ysize);      // Euclidean distance map
  if(!edm.allocated){ message(g.nomemory); return; }
  if(newimage("Distance map",0,0,xsize,ysize,bpp,ct,1,0,1,PERM,1,0,0)!=OK){ message(g.nomemory, ERROR); return;}
  newino = ci;

  progress_bar_open(www, scrollbar);
  for(thresh=10; thresh<maxpix; thresh+=increment)
  {  
      progress_bar_update(scrollbar, (thresh-10)*100/(maxpix-10));
      if(g.getout) break;
      create_edm(ino, struct_element, thresh, &edm);
      for(j=0; j<ysize; j++)
      for(i=0,i2=0; i<xsize; i++,i2+=b)
      { 
         v = 50 * edm.p[j][i];
         if(between(v, 100, 255)) v = maxpix; else v = 0; 
         if(v>=(uint)maxpix-1 || thresh==10) ok=1; else ok=0;
         if(maxsignal == 0) v = (int)g.maxvalue[bpp] - v;
         if(ok) putpixelbytes(z[newino].image[f][j]+i2, v, 1, bpp);      
      }
  }
  progress_bar_close(www);
  rebuild_display(newino);
  redraw(newino);
  moveimage(newino,xpos,ypos);
  z[newino].touched = 1;
  if(g.autoundo) backupimage(newino, 0);
}


//--------------------------------------------------------------------------//
// joins_features4                                                          //
//--------------------------------------------------------------------------//
int joins_features4(int ino, int f, int i2, int j, int bpp, uint thresh)
{
   uint index = 0;
   int b = g.off[bpp];                  // size of pixel in bytes
   if(j<1 || i2<1) return 0;
   if(j>z[ino].ysize-2 || i2>b*(z[ino].xsize)-2) return 0;
   if(pixelat(z[ino].image[f][j-1]+i2  , bpp) > thresh) index |= 0x1;
   if(pixelat(z[ino].image[f][j  ]+i2+b, bpp) > thresh) index |= 0x2;
   if(pixelat(z[ino].image[f][j  ]+i2-b, bpp) > thresh) index |= 0x4;
   if(pixelat(z[ino].image[f][j+1]+i2  , bpp) > thresh) index |= 0x8;                 
   return fate4[index];
}

//--------------------------------------------------------------------------//
// joins_features8                                                          //
//--------------------------------------------------------------------------//
int joins_features8(int ino, int f, int i2, int j, int bpp, uint thresh)
{  
   uint index = 0;
   int b = g.off[bpp];                  // size of pixel in bytes
   if(j<1 || i2<1) return 0;
   if(j>z[ino].ysize-2 || i2>b*(z[ino].xsize)-2) return 0;
   if(pixelat(z[ino].image[f][j-1]+i2+b, bpp) > thresh) index |= 0x01;
   if(pixelat(z[ino].image[f][j-1]+i2  , bpp) > thresh) index |= 0x02;
   if(pixelat(z[ino].image[f][j-1]+i2-b, bpp) > thresh) index |= 0x04;
   if(pixelat(z[ino].image[f][j  ]+i2+b, bpp) > thresh) index |= 0x08;                 
   if(pixelat(z[ino].image[f][j  ]+i2-b, bpp) > thresh) index |= 0x10;
   if(pixelat(z[ino].image[f][j+1]+i2+b, bpp) > thresh) index |= 0x20;
   if(pixelat(z[ino].image[f][j+1]+i2  , bpp) > thresh) index |= 0x40;
   if(pixelat(z[ino].image[f][j+1]+i2-b, bpp) > thresh) index |= 0x80; 
   return fate8[index];
}


//--------------------------------------------------------------------------//
// create_edm                                                               //
// Map of distance of each pixel from nearest local background pixel.       //
//--------------------------------------------------------------------------//
void create_edm(int ino, int se, uint thresh, array<uint> *e)
{
   int b,bpp,f,i,i2,j,xsize,ysize; 
   uint v;
   int bufrow,bufrow_to_put;
   const int SIZE=3;
   bpp = z[ino].bpp;
   b = g.off[bpp];                  // size of pixel in bytes
   f = z[ino].cf;
   xsize = z[ino].xsize;
   ysize = z[ino].ysize;
   array<uint> e2(z[ino].xsize+2, 6);    // Temporary storage

   //// Assume background is > thresh
   //// For each pixel in bkgd, assign a 0
   //// For each pixel in a feature assign a large value
   
   for(j=0; j<ysize; j++)
   for(i=0,i2=0; i<xsize; i++,i2+=b)
   {   v = (int)pixelat(z[ino].image[f][j]+i2, bpp);
       if(v<thresh) e->p[j][i] = 4; 
       else e->p[j][i] = 0; 
   }

   //// Assign each pixel in a feature = smallest neighbour + 1
   //// Note, each line overwrites data for the next line.

   if(se==1)
   {   bufrow=0;
       for(j=1; j<ysize-2; j++)
       {  for(i=0; i<xsize; i++) e2.p[bufrow][i] = 0;
          for(i=0,i2=0; i<xsize-2; i++,i2+=b)
          if(pixelat(z[ino].image[f][j]+i2, bpp) < thresh)
          {   v = (uint) 
              min( e->p[j-1][i-1], min( e->p[j-1][i  ], min( e->p[j-1][i+1], 
              min( e->p[j  ][i-1], min( e->p[j  ][i  ], min( e->p[j  ][i+1], 
              min( e->p[j+1][i-1], min( e->p[j+1][i  ], e->p[j+1][i+1]))))))));
              e2.p[bufrow][i] = v + 1;
          }
          if(j >= 3)          
          {   bufrow_to_put = 1 + bufrow - SIZE;
              if(bufrow_to_put < 0) bufrow_to_put += 6;
              for(i=0; i<xsize; i++) e->p[j-SIZE][i] = e2.p[bufrow_to_put][i];
              if(bufrow >=5) bufrow = -1;
          }
          bufrow++;     
          
       }
   }else
   {   //// Do in chunks to avoid gcc bug
       bufrow=0;
       for(j=2; j<ysize-3; j++)
       {  for(i=0; i<xsize; i++) e2.p[bufrow][i] = 0;
          for(i=1,i2=0; i<xsize-2; i++,i2+=b)
          if(pixelat(z[ino].image[f][j]+i2, bpp) < thresh)
          {  v = min(        e->p[j-2][i-2], min( e->p[j-2][i-1], 
                        min( e->p[j-2][i  ],      e->p[j-2][i+1])));
             v = min(v, min( e->p[j-2][i+2], min( e->p[j-1][i-2], e->p[j-1][i-1])));
             v = min(v, min( e->p[j-1][i  ], min( e->p[j-1][i+1], e->p[j-1][i+2])));
             v = min(v, min( e->p[j  ][i-2], min( e->p[j  ][i-1], e->p[j  ][i  ])));
             v = min(v, min( e->p[j  ][i+1], min( e->p[j  ][i+2], e->p[j+1][i-2])));
             v = min(v, min( e->p[j+1][i-1], min( e->p[j+1][i  ], e->p[j+1][i+1])));
             v = min(v, min( e->p[j+1][i+2], min( e->p[j+2][i-2], e->p[j+2][i-1])));
             v = min(v, min( e->p[j+2][i  ], min( e->p[j+2][i+1], e->p[j+2][i+2])));
             e2.p[bufrow][i] = v + 1;
          }  
          if(j >= 4)          
          {   bufrow_to_put = 1 + bufrow - SIZE;
              if(bufrow_to_put < 0) bufrow_to_put += 6;
              for(i=0; i<xsize; i++) e->p[j-SIZE][i] = e2.p[bufrow_to_put][i];
              if(bufrow >=5) bufrow = -1;
          }
          bufrow++;     
       }
   }

   if(se==1)
   {   bufrow=0;
       for(j=ysize-2; j>=1; j--)
       {  for(i=0; i<xsize; i++) e2.p[bufrow][i] = 0;
          for(i=0,i2=0; i<xsize-1; i++,i2+=b)
          if(pixelat(z[ino].image[f][j]+i2, bpp) < thresh)
          {  v = min( e->p[j-1][i-1], min( e->p[j-1][i  ], min( e->p[j-1][i+1], 
                 min( e->p[j  ][i-1], min( e->p[j  ][i  ], min( e->p[j  ][i+1], 
                 min( e->p[j+1][i-1], min( e->p[j+1][i  ], e->p[j+1][i+1]))))))));
             e2.p[bufrow][i] = v + 1;
          }
          if(j <= ysize-1-SIZE)          
          {   bufrow_to_put = 1 + bufrow - SIZE;
              if(bufrow_to_put < 0) bufrow_to_put += 6;
              for(i=0; i<xsize; i++) e->p[j+SIZE][i] = e2.p[bufrow_to_put][i];
              if(bufrow >=5) bufrow = -1;
          }
          bufrow++;     
       }
   }else
   {   //// Do in chunks to avoid gcc bug
       bufrow=0;
       for(j=ysize-3; j>=2; j--)
       {  for(i=0; i<xsize; i++) e2.p[bufrow][i] = 0;
          for(i=2,i2=0; i<xsize-2; i++,i2+=b)
          if(pixelat(z[ino].image[f][j]+i2, bpp) < thresh)
          {  v = min( e->p[j-2][i-2], min( e->p[j-2][i-1], min( e->p[j-2][i  ], e->p[j-2][i+1])));
             v = min(v, min( e->p[j-2][i+2], min( e->p[j-1][i-2], e->p[j-1][i-1])));
             v = min(v, min( e->p[j-1][i  ], min( e->p[j-1][i+1], e->p[j-1][i+2])));
             v = min(v, min( e->p[j  ][i-2], min( e->p[j  ][i-1], e->p[j  ][i  ])));
             v = min(v, min( e->p[j  ][i+1], min( e->p[j  ][i+2], e->p[j+1][i-2])));
             v = min(v, min( e->p[j+1][i-1], min( e->p[j+1][i  ], e->p[j+1][i+1])));
             v = min(v, min( e->p[j+1][i+2], min( e->p[j+2][i-2], e->p[j+2][i-1])));
             v = min(v, min( e->p[j+2][i  ], min( e->p[j+2][i+1], e->p[j+2][i+2])));
             e2.p[bufrow][i] = v + 1;
          } 
          if(j <= ysize-2-SIZE)          
          {   bufrow_to_put = 1 + bufrow - SIZE;
              if(bufrow_to_put < 0) bufrow_to_put += 6;
              for(i=0; i<xsize; i++) e->p[j+SIZE][i] = e2.p[bufrow_to_put][i];
              if(bufrow >=5) bufrow = -1;
          }
          bufrow++;     
       }
   }
}


//--------------------------------------------------------------------------//
// watershed                                                                //
//--------------------------------------------------------------------------//
void watershed(int ino, int KSIZE, int maxsignal, int wantboundaries)
{ 
  maxsignal = maxsignal;
  const int WSHEDMASK  = -2;
  const int WSHED = 0;
  const int WSHEDINIT  = -1;
  Widget www, scrollbar;
  int aa,bb,b,bpp,ct,f,h,i,i2,j,k,l,imo,v,v2,size,xsize,ysize,xpos,ypos;
  int hmin, hmax, maxpix, cdist=0, clabel=0, clabelincrement=1;
  int pstart, pend, index, pindex, value;
  int p,pprime,pprimeprime;
    
  ct = z[ino].colortype;
  bpp = z[ino].bpp;
  if(bpp > 16) { message("Image must be 16\nbits/pixel or less", WARNING, 55); return; }
  if(ct != GRAY){ message("Image must be grayscale", WARNING, 55); return; }
  
  b = g.off[bpp];                  // size of pixel in bytes
  f = z[ino].cf;
  xsize = z[ino].xsize;
  ysize = z[ino].ysize;
  xpos = z[ino].xpos+10;
  ypos = z[ino].ypos+10;
  size = xsize * ysize;
  maxpix = 1+(int)g.maxvalue[bpp];
  hmin = maxpix;
  hmax = 0;
  ptr_first = ptr_last = 0;
  switch(bpp)
  {   case 7:
      case 8: clabelincrement = 1; break;
      case 15:
      case 16: clabelincrement = 256; break;
  }

  //// Allocate a whole bunch of stuff
  array<int> dpix(size+2);
  array<int> outpix(size+2);
  array<int> sorted(size+2);
  array<int> fifo(size+2);
  array<int> freq(maxpix+2);
  if(!freq.allocated){ message(g.nomemory); return; }

  //// output image
  if(newimage("Image",xpos,ypos,xsize,ysize,bpp,ct,1,0,1,PERM,1,0,0)!=OK){ message(g.nomemory, ERROR); return;}
  imo = ci;

  //// Value WSHEDINIT is assigned to each pixel of output image imo
  for(j=0; j<size; j++) dpix.data[j]   = 0;
  for(j=0; j<size; j++) outpix.data[j] = WSHEDINIT;
  for(i=0; i<size; i++) sorted.data[i] = 0;
  for(i=0; i<size; i++) fifo.data[i]   = 0;
  for(j=0; j<maxpix; j++) freq.data[j] = 0;

  //// Determine frequency distribution of image gray level

  for(j=0; j<ysize; j++)
  for(i=0,i2=0; i<xsize; i++,i2+=b)
  {   v = pixelat(z[ino].image[f][j]+i2, bpp);
      freq.data[v]++;
      hmin = min(hmin, v);
      hmax = max(hmax, v);
  }

  //// Cumulative freq distrib, makes freq the index of sorted pixels
  for(j=1; j<maxpix; j++) freq.data[j] += freq.data[j-1];

  //// Sort the pixels of ino in increasing order of their gray values.
  //// sorted pix no.       = sorted.data[0..size]
  //// color of sorted pix. = pixelat(z[ino].image_1d[b*sorted.data[0..size]], bpp) 
  ////                           (pixels are not moved)

  for(i=0,i2=0; i<size; i++,i2+=b)
  {   v = pixelat(z[ino].image_1d + i2, bpp);
      sorted.data[freq.data[v]] = i;
      freq.data[v]--;
  }

  //// Geodesic SKIZ of level h-1 inside level h

  value = hmin;
  pindex = 1;
  if(hmax==hmin) { message("No image data"); return; }
  progress_bar_open(www, scrollbar);
  for(h=hmin; h<=hmax; h++)
  {  
     progress_bar_update(scrollbar, (h-hmin)*100/(hmax-hmin));
     if(keyhit()) if(getcharacter()==27) break;
     pstart = pend = pindex;                 // Starting & ending index of pixels == h
     while(value == h)                       
     {   pend++;
         if(pindex +1 >= size) break;
         i2 = b * sorted.data[++pindex];       
         value = pixelat(z[ino].image_1d + i2, bpp);
     }

     if(pstart==pend) continue;
     for(index=pstart; index<pend; index++)  // for every pixel such that imi(p)=h
     {  
         p = sorted.data[index];             // = the 1D array index in image bufr
         if(p>=size) continue;
         outpix.data[p] = WSHEDMASK;
         getxy(p, xsize, i, j);
         for(l=j-KSIZE; l<=j+KSIZE; l++)     // pixel p' x,y in Ng(p)
         for(k=i-KSIZE; k<=i+KSIZE; k++)
         {   if(!between(l,0,ysize-1) || !between(k,0,xsize-1)) continue;
             pprime = l*xsize + k;             
             if(pprime>=size) continue;
             v2 = outpix.data[pprime];             
             if(v2 >0 || v2 == WSHED)
             {    dpix.data[p] = 1;
                  fifo_add(&fifo, p);
             }
         }
     }
        
     cdist = 1;
     fifo_add(&fifo, -1);                    // -1 = fictitious pixel no.
     while(1)
     {
         p = fifo_first(&fifo);
         if(p == -1)
         {   if(fifo_empty()) break;
             else 
             {
                 fifo_add(&fifo, -1);
                 cdist++;
                 p = fifo_first(&fifo);
             }
         }        
         if(p>=size) continue;

         getxy(p, xsize, i, j);
         for(l=j-KSIZE; l<=j+KSIZE; l++)     // p'
         for(k=i-KSIZE; k<=i+KSIZE; k++)
         {   if(!between(l,0,ysize-1) || !between(k,0,xsize-1)) continue;
             pprime = l*xsize + k;
             if(pprime>=size) continue;

             //// i.e. if p' belongs to an already labeled basin or to the watersheds
             if(dpix.data[pprime] < cdist &&
               (outpix.data[pprime] >0 || outpix.data[pprime] == WSHED))
             {  
                if(outpix.data[pprime] >0)
                {   if(outpix.data[p] == WSHEDMASK || outpix.data[p] == WSHED)
                       outpix.data[p] = outpix.data[pprime];
                    else if(outpix.data[p] != outpix.data[pprime])
                       outpix.data[p] = WSHED;
                }else
                    if(outpix.data[p] == WSHEDMASK) outpix.data[p] = WSHED;
             }else
                if(outpix.data[pprime] == WSHEDMASK && dpix.data[pprime] == 0)
                {
                    dpix.data[pprime] = cdist+1;
                    fifo_add(&fifo, pprime);
                }         
         }
      }

      //// Checks if new minima have been discovered

     for(index=pstart; index<pend; index++)  // For every pixel p such that imi(p) == h
     {  
         p = sorted.data[index];             // = the 1D array index in image bufr
         if(p>=size) continue;
         dpix.data[p] = 0;                   // the distance assoc with p is reset to 0
         if(outpix.data[p] == WSHEDMASK)
         {
            clabel += clabelincrement;
            fifo_add(&fifo, p);
            outpix.data[p] = clabel;
            while(fifo_empty() == FALSE)
            {
               pprime = fifo_first(&fifo);
               if(pprime>=size) continue;
               getxy(pprime, xsize, i, j);        // p'
               for(l=j-KSIZE; l<=j+KSIZE; l++)    // p''
               for(k=i-KSIZE; k<=i+KSIZE; k++)
               {   if(!between(l,0,ysize-1) || !between(k,0,xsize-1)) continue;
                   pprimeprime = l*xsize + k;
                   if(pprimeprime>=size) continue;
                   if(outpix.data[pprimeprime] == WSHEDMASK)
                   {   fifo_add(&fifo, pprimeprime);
                       outpix.data[pprimeprime] = clabel;
                   }
               }
            }
         }
      }  
  }
  progress_bar_close(www);
  for(i=0,i2=0; i<size; i++,i2+=b) 
      putpixelbytes(z[imo].image_1d+i2, outpix.data[i], 1, bpp, 1);


  ////  Complete watershed lines
  for(j=0; j<ysize; j++)
  for(i=1,i2=1*b; i<xsize; i++,i2+=b)
  {  aa = pixelat(z[imo].image[0][j]+i2, bpp);
     bb = pixelat(z[imo].image[0][j]+i2-b, bpp);
     if(aa==0 || bb==0) continue;
     if(aa!=bb) putpixelbytes(z[imo].image[f][j]+i2, 0, 1, bpp, 1);

  }
  for(j=1;j<ysize;j++)
  for(i=0;i<xsize;i++)
  {  aa = pixelat(z[imo].image[0][j]+i2, bpp);
     bb = pixelat(z[imo].image[0][j-1]+i2, bpp);
     if(aa==0 || bb==0) continue;
     if(aa!=bb) putpixelbytes(z[imo].image[f][j]+i2, 0, 1, bpp, 1);
  }
  
  if(wantboundaries)
  {   for(i=0,i2=0; i<size; i++,i2+=b) 
      {   v = pixelat(z[imo].image_1d+i2, bpp);
          if(v) v = maxpix-1;
          putpixelbytes(z[imo].image_1d+i2, v, 1, bpp, 1);
      }
  }

  switchto(imo);
  rebuild_display(imo);
  if(g.autoundo) backupimage(imo, 0);
  redraw(imo);
  z[imo].touched = 1;
  moveimage(imo,xpos,ypos);
}

