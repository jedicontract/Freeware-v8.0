//--------------------------------------------------------------------------//
// xmtnimage9.cc                                                            //
// 2D-FFT routines                                                          //
// Latest revision: 04-01-2002                                              //
// Copyright (C) 2002 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"
#include "xmtnimagec.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
extern PrinterStruct printer;
const int FFTOPTS=5;     // Change if adding new fft option
 
//--------------------------------------------------------------------------//
// Example function using template.                                         //
// At least 1 call for each data type to be instantiated must exist in      //
//   the same source module as the file, even if they are not used. Then    //
//   you can call using those data types from other modules.                //
// Example:   testtemplate(1); testtemplate(1.023); testtemplate("Hi");     //
// Template functions have to be before any other functions, so they can    //
//   have global scope.                                                     //
//--------------------------------------------------------------------------//
template<class type> void testtemplate(type k)
{  
     k=k;
     message("Press the Okay button, okay?");
}


//--------------------------------------------------------------------------// 
// transform - entry point into FFT, maximum entropy, etc.                  //
//--------------------------------------------------------------------------//
int transform(int noofargs, char **arg)
{
  if(memorylessthan(16384)){  message(g.nomemory,ERROR); return(NOMEM); } 
  static Dialog *dialog;
  char temp[128];
  int crud=0; if(crud)  testtemplate("Hi");  // needed to initialize template
  int j,k,status=OK;

  if(g.image_count<=0){ message("No images"); return(NOIMAGES); }
  if(ci<=0){ message("Please select an image"); return(NOIMAGES); }
  g.fft_ino1 = ci;
  
  ////  Get parameters
  ////    ino1 = input image #1 (the main image).   
  ////    ino2 = input image #2 (convolutions only) 
  if(noofargs==0)
  {
     dialog = new Dialog;
     if(dialog==NULL){ message(g.nomemory); return(NOMEM); }
     strcpy(dialog->title,"2D Fourier Transform");
     strcpy(dialog->radio[0][0],"Operation"); 
     strcpy(dialog->radio[0][1],"Forward");             
     strcpy(dialog->radio[0][2],"Reverse");
     strcpy(dialog->radio[0][3],"Convolute 2 images");
     strcpy(dialog->radio[0][4],"Deconvolute 2 images");

     strcpy(dialog->radio[1][0],"Display");             
     strcpy(dialog->radio[1][1],"Original image");
     strcpy(dialog->radio[1][2],"Real FFT");
     strcpy(dialog->radio[1][3],"Imaginary FFT");
     strcpy(dialog->radio[1][4],"Power spectrum");
 
     strcpy(dialog->radio[2][0],"Other functions"); 
     strcpy(dialog->radio[2][1],"Save FFT");             
     strcpy(dialog->radio[2][2],"Read FFT");
     strcpy(dialog->radio[2][3],"Erase FFT");
     dialog->radiono[2]=4;
     dialog->radioset[2] = -1;

     dialog->radioset[0] = -1;
     if(g.fft_ino1>0)
     {  if(z[g.fft_ino1].fftdisplay==NONE) dialog->radioset[1]=1;
        if(g.fft_wantdisplay==-1)    dialog->radioset[1]=2;
        if(z[g.fft_ino1].fftdisplay==REAL) dialog->radioset[1]=2;
        if(z[g.fft_ino1].fftdisplay==IMAG) dialog->radioset[1]=3;
        if(z[g.fft_ino1].fftdisplay==POWR) dialog->radioset[1]=4;
     }
     else dialog->radioset[1]=1;

     strcpy(dialog->boxes[0],"Data");
     strcpy(dialog->boxes[1],"Image #1");
     strcpy(dialog->boxes[2],"Image #2");
     strcpy(dialog->boxes[3],"Display scale factors");
     strcpy(dialog->boxes[4],"Black");
     strcpy(dialog->boxes[5],"White");

     itoa(g.fft_ino1,temp,10);
     strcpy(dialog->answer[0][0],temp);
     itoa(g.fft_ino2,temp,10);
     strcpy(dialog->answer[1][0],temp);

     if(g.fft_ino1>0) doubletostring(z[g.fft_ino1].fmin, g.signif, temp);
     else        strcpy(temp,"0.0");
     strcpy(dialog->answer[4][0],temp);

     if(g.fft_ino1>0) doubletostring(z[g.fft_ino1].fmax, g.signif, temp);
     else        strcpy(temp,"0.0");
     strcpy(dialog->answer[5][0],temp);

     strcpy(dialog->boxes[6],"Options");
     strcpy(dialog->boxes[7],"Copy FFT into image buffer");

     dialog->boxtype[0]=LABEL;
     dialog->boxtype[1]=INTCLICKBOX;
     dialog->boxtype[2]=INTCLICKBOX;
     dialog->boxtype[3]=LABEL;
     dialog->boxtype[4]=STRING;
     dialog->boxtype[5]=STRING;
     dialog->boxtype[6]=LABEL;
     dialog->boxtype[7]=TOGGLE;

     dialog->boxmin[1]=1; dialog->boxmax[1]=g.image_count-1;
     dialog->boxmin[2]=1; dialog->boxmax[2]=g.image_count-1;
      
     dialog->radiono[0]=5;
     dialog->radiono[1]=5;
     dialog->radiono[2]=4;
     for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
     itoa(g.fft_ino1,dialog->answer[1][0],10);
     itoa(g.fft_ino2,dialog->answer[2][0],10);

     dialog->noofradios=3;
     dialog->noofboxes=8;
     dialog->helptopic=21;  
     dialog->want_changecicb = 0;
     dialog->f1 = fftcheck;
     dialog->f2 = null;
     dialog->f3 = null;
     dialog->f4 = fftfinish;
     dialog->f5 = null;
     dialog->f6 = null;
     dialog->f7 = null;
     dialog->f8 = null;
     dialog->f9 = null;
     dialog->width = 380;
     dialog->height = 0; // calculate automatically
     dialog->transient = 1;
     dialog->wantraise = 0;
     dialog->radiousexy = 0;
     dialog->boxusexy = 0;
     strcpy(dialog->path,".");
     dialog->message[0] = 0;      
     dialog->busy = 0;
     dialogbox(dialog);
  
  }else
  {  
     g.fft_ino1 = ci;
     if(noofargs>=1) g.fft_direction = atoi(arg[1]); else g.fft_direction=1;
     if(g.fft_direction==-1) g.fft_direction=2;   
     if(noofargs>=2) g.fft_wantdisplay = atoi(arg[2]); else g.fft_wantdisplay=1;
     if(noofargs>=3) g.fft_ino2 = image_number(arg[3]);
     if(g.getout){ g.getout=0; return ERROR; }
     status = start_fft(g.fft_direction, g.fft_ino1, g.fft_ino2, g.fft_wantdisplay);
  }
  return(status);
}


//--------------------------------------------------------------------------// 
// start_fft                                                                //
//--------------------------------------------------------------------------//
int start_fft(int direction, int ino1, int ino2, int wantdisplay)
{
   int xsize3, ysize3, status=OK, frame, bpp, i,j, value;
   uchar *address;
   drawselectbox(OFF);
   if(!between(ino1,1,g.image_count)||!between(ino2,1,g.image_count)) 
      return(BADPARAMETERS);
   switch(direction)
   {  case 1: 
              status=setupfft(ino1,xsize3,ysize3); 
              if(status==OK) status=four2d(ino1,FWD); 
              break;
      case 2: 
              status=setupfft(ino1,xsize3,ysize3); 
              if(status==OK) status=four2d(ino1,REV); 
              break;
      case 3: z[ino1].fftstate=0;   // Causes pixels to overwrite fft[]
              z[ino2].fftstate=0;   // Causes pixels to overwrite fft[]
              status=convolute(ino1,ino2, 1);
              break;
      case 4: z[ino1].fftstate=0;   // Causes pixels to overwrite fft[]
              z[ino2].fftstate=0;   // Causes pixels to overwrite fft[]
              status=convolute(ino1,ino2,-1);
              break;
      case 5: if(z[ino1].floatexists) 
                 status=OK; 
              else 
              {  status=BADPARAMETERS;
                 message("Error - No FFT data for image",ERROR); 
              }
              break;
      default: break;
   }

   ////  Scale the data for display  

   if(ino1>0 && status==OK && direction!=FFTOPTS)
   {  z[ino1].fftdisplay=wantdisplay;
      scalefft(ino1);
      frame = z[ino1].cf;
      bpp = z[ino1].bpp;
      rebuild_display(ino1);
                                                  // Map fft gray values to image[]
      if(between(direction,2,4))
      {   for(j=0;j<z[ino1].ysize;j++)
          for(i=0;i<z[ino1].xsize;i++)
          {   value = pixelat(z[ino1].img[j] + i*g.off[g.bitsperpixel], g.bitsperpixel);  
              address = z[ino1].image[frame][j] + i*g.off[bpp];
              putpixelbytes(address, value, 1, bpp, 1);
          }
      }
      rebuild_display(ino1);
      switchto(ino1);
      redraw(ino1);      
   }
   if(status==ERROR) message("Error - Invalid operation",ERROR);
   return status;
}



//--------------------------------------------------------------------------//
//  fftcheck                                                                //
//--------------------------------------------------------------------------//
void fftcheck(dialoginfo *a, int radio, int box, int boxbutton)
{
   static int infftcheck=0, oci=ci;
   double ofmin=0, ofmax=0;
   char temp[128];
   int i,j,n,hit=0,v;
   int bpp, off1, off2, i2, i3, frame;
   if(infftcheck) return;  //// XtSetValues->dialogstringcb->fftcheck
   infftcheck=1;
   Arg args[100];
   XmString xms;
   box=box; boxbutton=boxbutton;

   if(a->radioset[1]==1) g.fft_wantdisplay = NONE;
   if(a->radioset[1]==2) g.fft_wantdisplay = REAL;
   if(a->radioset[1]==3) g.fft_wantdisplay = IMAG;
   if(a->radioset[1]==4) g.fft_wantdisplay = POWR;
   g.fft_ino1 = ci;
   g.fft_ino2 = atoi(a->answer[2][0]);
   if(oci!=ci)
   {    ////  Redraw the label in the pushbutton widget.
       itoa(ci,temp,10);
       n=0;
       XtSetArg(args[n], XmNlabelString, xms=XmStringCreateSimple(temp)); n++;
       XtSetValues(a->boxwidget[1][0], args, n);
       XmStringFree(xms);
       g.fft_direction = FFTOPTS;
       a->radioset[0] = g.fft_direction;
       unset_radio(a, 0);
       XFlush(g.display);
       oci=ci;
       infftcheck=0;
       return;
   }
   if(between(g.fft_ino1,1,g.image_count))
   {   ofmin = z[g.fft_ino1].fmin;
       ofmax = z[g.fft_ino1].fmax;
       z[g.fft_ino1].fmin = atof(a->answer[4][0]);
       z[g.fft_ino1].fmax = atof(a->answer[5][0]);
       if(between(box, 4,5) && g.fft_wantdisplay &&
         ( (fabs(ofmin-z[g.fft_ino1].fmin)>1.0) ||
           (fabs(ofmax-z[g.fft_ino1].fmax)>1.0)) ) repair(ci); 

   }
   g.fft_direction = a->radioset[0];
   if(radio==0 && ci>=0 && box==-1 && boxbutton==-1 && between(g.fft_direction,0,FFTOPTS-1))
   {   start_fft(g.fft_direction,g.fft_ino1,g.fft_ino2,g.fft_wantdisplay);
       hit=1;
   }

   ////  Change fft display
   if((radio==1 && between(ci,1,g.image_count) && z[ci].floatexists && 
      z[ci].fftdisplay != a->radioset[1]-1) || hit==1)
   {   
       z[ci].fftdisplay = a->radioset[1]-1;
       scalefft(ci);
       rebuild_display(ci);
       redraw(ci);      
       //// Update max & min
       if(ci>0) doubletostring(z[ci].fmin, g.signif, temp);
       else     strcpy(temp,"0.0");
       strcpy(a->answer[4][0],temp);
       XtVaSetValues(a->boxwidget[4][0], XmNvalue, temp, NULL); 

       if(ci>0) doubletostring(z[ci].fmax, g.signif, temp);
       else     strcpy(temp,"0.0");
       strcpy(a->answer[5][0],temp);
       XtVaSetValues(a->boxwidget[5][0], XmNvalue, temp, NULL); 
   }  

   ////  Copy FFT display into image buffer
   if(box==7 && between(ci,1,g.image_count) && z[ci].floatexists)
   {   
       bpp = z[ci].bpp;
       off1 = g.off[g.bitsperpixel];
       off2 = g.off[bpp];
       frame = 0;
       for(j=0;j<z[ci].ysize;j++)
       for(i=0, i2=0, i3=0; i<z[ci].xsize; i++, i2+= off1, i3+=off2)
       {  v = pixelat(z[ci].img[j] + i2, g.bitsperpixel);
          v = convertpixel(v, g.bitsperpixel, bpp, 1); 
          putpixelbytes(z[ci].image[frame][j] + i3, v, 1, bpp, 1);
       }
       message("FFT copied");
       XmToggleButtonSetState(a->boxwidget[7][0], False, False); 
   }

   //// Radio buttons
   if(radio==2 && box==-1 && boxbutton==-1)
   {    switch(a->radioset[2])
        {    case 1: writefft(ci); break;
             case 2: readfft(0, NULL); break; 
             case 3: 
	             if(z[ci].fftstate==1) start_fft(-1, ci, ci, 1);
                     else if(z[ci].fftstate==-1) start_fft(1, ci, ci, 1);
                     erase_fft(ci); 
                     message("FFT erased"); 
                     z[ci].fftstate = 0;
                     repair(ci);
                     break; 
        }
   }
   if(radio==0) unset_radio(a, 0);
   infftcheck=0;
}


//--------------------------------------------------------------------------//
//  fftfinish                                                               //
//--------------------------------------------------------------------------//
void fftfinish(dialoginfo *a)
{  a=a;
   if(z[ci].fftdisplay && message("Reset display to original image?",YESNOQUESTION)==YES)
   {    z[ci].fftdisplay = 0;
        scalefft(ci);
        rebuild_display(ci);
        redraw(ci);      
   }
}


//--------------------------------------------------------------------------//
// convolute                                                                //
//   ino1 and ino2 are the input images. The result is returned in the      //
//     fft buffer for ino1.                                                 //
//   direction = 1 for convolution                                          //
//   direction =-1 for deconvolution                                        //
// Returns error code (OK if successful).                                   //
//--------------------------------------------------------------------------//
int convolute(int ino1, int ino2, int direction)
{  
   if(memorylessthan(4096)){  message(g.nomemory,ERROR); return(NOMEM); } 
   double a,b,c,d,e,f,den;

   int i,j,status=OK;
   int xsize3,ysize3,xsize4,ysize4,factor;
   int divisionsbyzero=0;
   char tempstring[128];
 
   if((ino1<=0)||(ino2<=0))
   {   message("Error: need two images to convolute",ERROR);
       return(BADPARAMETERS);
   }  
   
    //--------------------------------------------------------------------//
    // Allocate fft buffers with sizes equal to a power of 2. They can be //
    // bigger than the image buffers.                                     //
    //                                                                    //
    // xsize3, ysize3, xsize4, and ysize4 are all guaranteed to be powers //
    // of 2 and are always the same size or larger than the size of the   //
    // corresponding image.                                               //
    //--------------------------------------------------------------------//
   
   status=setupfft(ino1,xsize3,ysize3); 
   if(status!=OK) return(status);
                                            
   status=setupfft(ino2,xsize4,ysize4); 
   if(status!=OK) return(status);

   status=four2d(ino1,FWD);
   status=four2d(ino2,FWD);

   factor=xsize3*ysize3;
   
   g.getout=0;
   for(j=0;j<ysize3;j++)
   { for(i=0;i<xsize3;i++)
     {  a = z[ino1].fft[j][i].real();
        b = z[ino1].fft[j][i].imag();
        if((i<xsize4)&&(j<ysize4))
        {  c = z[ino2].fft[j][i].real();
           d = z[ino2].fft[j][i].imag();
        }else
        {  c=0;
           d=0;
        }   

        if(direction==1)
           //  Complex multiplication
           //  
           //  (a,b) * (c,d) = (ac-bd, bc+ad)          
        {
           e = a*c-b*d;
           f = b*c+a*d;
        }
        else 
           //  Complex division
           //
           //  a,b      ac+bd, bc-ad
           //  ---  =   ------------
           //  c,d         cý + dý
        {                 
           den = c*c + d*d;
           if(den==0)
           {  divisionsbyzero++;
              den=0.00001;
           }   
           e = (a*c + b*d) / den;
           f = (b*c - a*d) / den;
        }
        z[ino1].fft[j][i].real() = e;
        z[ino1].fft[j][i].imag() = f;
        if(direction==-1)
           z[ino1].fft[j][i] = z[ino1].fft[j][i] * factor;
        if(g.getout)break;
     }
     if(g.getout)break;
   }
   four2d(ino1,REV);
   four2d(ino2,REV);
   if(divisionsbyzero)
   {  itoa(divisionsbyzero,tempstring,10);
      strcat(tempstring," divisions by zero detected");
      message(tempstring);
   }
   if((xsize3!=xsize4)||(ysize3!=ysize4))
      message("FFT's were different sizes",ERROR);
   return(OK);
}


//--------------------------------------------------------------------------//
// setupfft - allocate space and fill arrays with starting values for FFT   //
//                                                                          //
// ino1 is the input image data, bounded by (x1,y1) and (x2,y2).            //
// The new sizes are returned by ref. in xsize3 and ysize3.                 //
// Returns error code (OK if successful).                                   //
//--------------------------------------------------------------------------//
int setupfft(int ino, int& xsize3, int& ysize3)
{
   if(memorylessthan(4096)){  message(g.nomemory,ERROR); return(NOMEM); } 
  
   if(ino<=0) return ERROR;
   char temp[1024];
   int count,i,i2,j,status=OK,xsize,ysize,x1,y1,x2,y2,bpp;
   double v;
   printstatus(BUSY);
 
   bpp= z[ino].bpp;
   x1 = z[ino].xpos;                     // start x of input image 
   x2 = z[ino].xpos + z[ino].xsize-1;    // end   x of input image 
   y1 = z[ino].ypos;                     // start y of input image 
   y2 = z[ino].ypos + z[ino].ysize-1;    // end   y of input image 

   xsize=x2-x1+1;  // x and y sizes
   ysize=y2-y1+1;

   ////  Adjust x & y size to next highest multiple of 2   

   for(count=0;(1<<count)<xsize;count++) continue; 
   xsize3 = 1<<(count);
   for(count=0;(1<<count)<ysize;count++) continue;
   ysize3 = 1<<(count);
   if(z[ino].colortype != GRAY) 
   {     if(g.want_messages > 1) status = message("Warning: converting image\nto grayscale for FFT");        
         if(status==CANCEL) return ABORT;
         converttograyscale(ino);
   }
   if(z[ino].floatexists==0 && (xsize3!=xsize || ysize3!=ysize))
   {     sprintf(temp,"Warning - enlarging image to \n%d x %d for FFT", xsize3,ysize3);  
         if(g.want_messages > 1) status = message(temp,WARNING);
         if(status==CANCEL) return ABORT;
         resize_image(ino, xsize3, ysize3);
   }
                
   //// Create fft buffer if one doesn't already exist.                
   //// Copy data from original image into the output fft buffer.      
   //// Initialize the fft-related parameters.                         

   if(!z[ino].floatexists)
   {
       z[ino].fftxsize=xsize3;
       z[ino].fftysize=ysize3;
       z[ino].fft_1d = (complex*)malloc(z[ino].fftysize*z[ino].fftxsize*
              sizeof(complex));
                        
       if(z[ino].fft_1d==NULL) message("Error allocating fft matrix",ERROR); 
       z[ino].fft = make_2d_alias(z[ino].fft_1d,z[ino].fftxsize,z[ino].fftysize);

       if(z[ino].fft==NULL)
       {    message(g.nomemory,ERROR);
            return(NOMEM);  
       } 
       z[ino].floatexists = 1;
       z[ino].fmax=g.maxvalue[bpp];
       z[ino].fmin=0;
       z[ino].fftstate=0;
   }

   //// Initialize the fft buffer to pad extra elements with 0's.      
   //// If fft state of image is 0, convert the pixels into floating   
   ////  point numbers in case any changes were made to the raw image. 
   //// If fft state of image is 1 or -1, do not change the fft[]      
   ////  values, so a reverse transform can be done using both real and
   ////  imaginary values calculated earlier.                          

   if(z[ino].fftstate==0)
   {
       for(j=0;j<ysize3;j++)
       {   for(i=0;i<xsize3;i++)
           {  z[ino].fft[j][i].real()=0;
              z[ino].fft[j][i].imag()=0;
           }
       }
       
       ////  Copy data from original image into the fft buffer. 
        
       for(j=0;j<ysize;j++)
       {   for(i=0,i2=0;i<xsize;i++,i2+=g.off[bpp])
           {  v = (double)pixelat(z[ino].image[z[ino].cf][j]+i2,bpp);
              z[ino].fft[j][i].real() = v;
              z[ino].fft[j][i].imag() = 0;
           } 
       }  
   }
   printstatus(NORMAL);
   return OK;
}


//--------------------------------------------------------------------------//
// scalefft                                                                 //
// Find approximate min & max value of fft, ignoring 1st and last 20% which //
// are extraneous values.  Don't create quantiles because a linear corres-  //
// pondence is needed.                                                      //
//--------------------------------------------------------------------------//
void scalefft(int ino)
{
   int i,i2,j,bpp;
   double fvalue;
   printstatus(BUSY);
 
   char tempstring[FILENAMELENGTH];

   if(ino<=0 || !z[ino].floatexists)
   {   message("No FFT present",ERROR); 
       return; 
   }
   if((z[ino].fftdisplay > 4) ||
      (z[ino].fftdisplay == WAVE &&!z[ino].waveletexists))
   {   message("Invalid display mode");
       z[ino].fftdisplay = 0;
       return; 
   }
   
   int x1=(int)(0.2*z[ino].xsize);
   int x2=(int)(0.8*z[ino].xsize);
   int y1=(int)(0.2*z[ino].ysize);
   int y2=(int)(0.8*z[ino].ysize);
   bpp=z[ino].bpp;
   z[ino].fmax = 0; 
   z[ino].fmin = g.maxvalue[bpp];
   for(j=y1;j<y2;j++)
   { for(i=x1,i2=x1*g.off[bpp];i<x2;i++,i2+=g.off[bpp])
     {  switch(z[ino].fftdisplay)
        {   case NONE: fvalue = (double)pixelat(z[ino].image[z[ino].cf][j]+i2,bpp);break;
            case REAL: fvalue = z[ino].fft[j][i].real(); break;
            case IMAG: fvalue = z[ino].fft[j][i].imag(); break;
            case POWR: fvalue = z[ino].fft[j][i].real()*z[ino].fft[j][i].imag(); 
                       break;
            default: fvalue=0;           
        }               
        if (fvalue>z[ino].fmax) z[ino].fmax=fvalue;
        if (fvalue<z[ino].fmin) z[ino].fmin=fvalue;
     }
   }  
   if(z[ino].fmax<z[ino].fmin){ z[ino].fmax=g.maxvalue[bpp]; z[ino].fmin=0; }
   if(z[ino].fmax>MAXINT) z[ino].fmax=MAXINT;
   if(z[ino].fmin<-MAXINT) z[ino].fmin=-MAXINT;
   if(ci>=0)if(strlen(z[ino].name)==0) 
   {  sprintf(tempstring,"FFT_%d",ino);
      setimagetitle(ino, tempstring);
   }
   z[ino].colortype=GRAY;
   z[ino].touched=1;
   printstatus(NORMAL);
}

//--------------------------------------------------------------------------//
// four2d - wrapper for fourn()                                             //
//--------------------------------------------------------------------------//
int four2d(int ino, int isign)
{
  int i,j,ndim;
  int nn[3];
  int status=OK;
  double totallength;
  printstatus(CALCULATING);

  nn[1]=z[ino].fftysize;
  nn[2]=z[ino].fftxsize;
  ndim=2;

  fourn(z[ino].fft_1d, nn,ndim,isign);

  totallength=(double)(z[ino].fftxsize*z[ino].fftysize);
  if(isign==-1)
  {  for(j=0;j<z[ino].fftysize;j++)
     for(i=0;i<z[ino].fftxsize;i++)
     {  z[ino].fft[j][i].real() /= totallength;
        z[ino].fft[j][i].imag() /= totallength;
     }
  }
  
  z[ino].fftstate+=isign;
  printstatus(NORMAL);
  return status;
}


//--------------------------------------------------------------------------//
// fourn - adapted from Numerical Recipes in C, by Press, Vetterling,       //
// Teukolsky, and Flannery (Cambridge University Press, 1992) ,page 523.    //
// Replaces data by its 2-dimensional discrete Fourier transform, if        //
//  isign is 1.  The lengths of each dimension (no.of complex values)       //
//  must be powers of 2.                                                    //
// If isign is -1, data are replaced by the inverse transform times the     //
//  product of the lengths of all dimensions.                               //
// Data contains all the dimensions in 1 array and starts at data[1] not    //
//  data[0]. Lowest dimensions change fastest.                              //
// nn[] contains the no.of elements in each dimension.                      //
// Modified to use complex numbers, and to make array indices start with 0  //
//  instead of 1.                                                           //
//--------------------------------------------------------------------------//
void fourn(complex data[], int nn[], int ndim, int isign)
{  
  int idim;
  uint i1,i2,i3,i2rev,i3rev,ip1,ip2,ip3,ifp1,ifp2;
  uint ibit,k1,k2,n,nprev,nrem,ntot;
  complex temp(0,0), w(0,0), wp(0,0);
  double theta,wtemp;

  ntot=1;
  for(idim=1;idim<=ndim;idim++) ntot *= nn[idim];
  nprev=1;
  
  for(idim=ndim;idim>=1;idim--)            // main loop over the dimensions
  { 
     n=nn[idim];
     nrem=ntot/(n*nprev);
     ip1=nprev<<1;
     ip2=ip1*n;
     ip3=ip2*nrem;
     i2rev=1;
     for(i2=1;i2<=ip2;i2+=ip1)             // Bit-reversal section
     {  if(i2<i2rev)
        {  for(i1=i2;i1<=i2+ip1-2;i1+=2)
           {  for(i3=i1/2;i3<=ip3/2;i3+=ip2/2)
              {  i3rev=i3+(i2rev-i2)/2;
                 cswap(data[i3], data[i3rev]);
              }
           }
        }
        ibit=ip2>>1;
        while((ibit>=ip1)&&(i2rev>ibit))
        {  i2rev-=ibit;
           ibit>>=1;
        }
        i2rev += ibit;
     }
     ifp1=ip1;
     while(ifp1<ip2)                       // Danielson-Lanczos section
     {  ifp2=ifp1<<1;
        theta = (double)isign*6.28318530717959/(double)(ifp2/ip1);
        wtemp = sin(0.5*theta);
        wp.real() = -2.0*wtemp*wtemp;
        wp.imag() = sin(theta);
        w.real() = 1.0;
        w.imag() = 0.0;
        for(i3=1;i3<=ifp1;i3+=ip1)
        {  for(i1=i3;i1<=i3+ip1-2;i1+=2)
           {  for(i2=i1;i2<=ip3;i2+=ifp2)
              {  k1=i2/2;                    // Danielson-Lanczos formula
                 k2=k1+ifp1/2;
                 temp = w * data[k2];        // Complex multiplication
                 data[k2] = data[k1] - temp;
                 data[k1] = data[k1] + temp;
              }
           }
           w = w + wp * w;
        }
        ifp1=ifp2;
     }
     nprev *= n;
  } 
}


//--------------------------------------------------------------------------//
//  cswap                                                                   //
//--------------------------------------------------------------------------//
void cswap(complex &a, complex &b)
{
     dswap(a.real(), b.real());
     dswap(a.imag(), b.imag());
}
