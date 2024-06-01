//--------------------------------------------------------------------------//
// xmtnimage44.cc                                                           //
// Latest revision: 10-12-2000                                              //
// Copyright (C) 2000 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
// Wavelets                                                                 //
// Includes improvements made by Dietmar Kunz <dkunz@server2.fo.FH-Koeln.DE>//
// Laplacian wavelet functions written by Dietmar Kunz.                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"
#include "xmtnimagec.h"

extern Globals     g;
extern Image      *z;
extern int         ci;

//--------------------------------------------------------------------------//
// wavelet                                                                  //
//--------------------------------------------------------------------------//
void wavelet(int noofargs, char **arg)
{
   arg=arg; noofargs=noofargs; 
   int j, k, ino=ci;
   if(g.wavelet_maxlevels==0) 
   {   for(k=4; k<=min(z[ino].xsize, z[ino].ysize); k*=2) g.wavelet_maxlevels++;
       g.wp.levels = min(g.wp.levels, g.wavelet_maxlevels);
       for(k=g.wavelet_maxlevels; k>=z[ino].wavelet_xminres; k/=2) g.wp.levels--;
   }
   static Dialog *dialog;
   dialog = new Dialog;
   if(dialog==NULL){ message(g.nomemory); return; }
   strcpy(dialog->radio[0][0],"Execute"); 
   strcpy(dialog->radio[0][1],"Decompose");             
   strcpy(dialog->radio[0][2],"Reconstitute");
   strcpy(dialog->radio[0][3],"Copy wavelets->image");
   dialog->radiono[0]=4;
   dialog->radioset[0] = -1; 

   strcpy(dialog->radio[1][0],"Algorithm"); 
   strcpy(dialog->radio[1][1],"Pyramidal");             
   strcpy(dialog->radio[1][2],"Laplacian");
   strcpy(dialog->radio[1][3],"Mallat");
   strcpy(dialog->radio[1][4],"Multiresolution");
   dialog->radiono[1]=5;
   switch(g.wavelet_type)
   {  case NONE:      dialog->radioset[1] = 0; break;
      case PYRAMIDAL: dialog->radioset[1] = 1; break;
      case LAPLACIAN: dialog->radioset[1] = 2; break;
      case MALLAT:    dialog->radioset[1] = 3; break;
      case MULTIRESOLUTION: dialog->radioset[1] = 4; break;
   }

   strcpy(dialog->radio[2][0],"Other functions"); 
   strcpy(dialog->radio[2][1],"Save coefficients");             
   strcpy(dialog->radio[2][2],"Read coefficients");
   strcpy(dialog->radio[2][3],"Restore original");
   dialog->radiono[2]=4;
   dialog->radioset[2] = -1;

   strcpy(dialog->title,"Wavelet transform");
   strcpy(dialog->boxes[0],"Data");
   dialog->boxtype[0]=LABEL;

   strcpy(dialog->boxes[1],"Image no. ");
   dialog->boxtype[1]=INTCLICKBOX;
   sprintf(dialog->answer[1][0],"%d",ino);
   dialog->boxmin[1]=0; 
   dialog->boxmax[1]=g.image_count-1;

   strcpy(dialog->boxes[2],"Wavelet");
   strcpy(dialog->answer[2][0], g.wavelet_file);
   dialog->boxtype[2]=FILENAME;

   strcpy(dialog->boxes[3],"Reconstitution coefficients");
   dialog->boxtype[3]=LABEL;

   strcpy(dialog->boxes[4],"Index range to use");
   dialog->boxtype[4]=STRING;
   strcpy(dialog->answer[4][0], g.wavelet_index_range);

   strcpy(dialog->boxes[5],"Value range to use");
   dialog->boxtype[5]=STRING;
   strcpy(dialog->answer[5][0], g.wavelet_coeff_range);

   strcpy(dialog->boxes[6],"Value range to ignore");
   dialog->boxtype[6]=STRING;
   strcpy(dialog->answer[6][0], g.wavelet_ignore_range);

   strcpy(dialog->boxes[7],"Options");
   dialog->boxtype[7]=LABEL;

   strcpy(dialog->boxes[8],"Show grid");
   dialog->boxtype[8]=TOGGLE;
   dialog->boxset[8] = g.wavelet_grid;

   strcpy(dialog->boxes[9],"Gray value offset");
   dialog->boxtype[9]=TOGGLESTRING;
   sprintf(dialog->answer[9][1], "%d", g.wp.gray_offset);
   dialog->boxset[9] = g.wp.want_gray_offset;

   strcpy(dialog->boxes[10],"Levels of detail");
   dialog->boxtype[10]=INTCLICKBOX;
   sprintf(dialog->answer[10][0],"%d", g.wp.levels);
   dialog->boxmin[10]=1; 
   dialog->boxmax[10] = g.wavelet_maxlevels;

   strcpy(dialog->boxes[11],"Binomial filter length");
   dialog->boxtype[11]=INTCLICKBOX;
   sprintf(dialog->answer[11][0],"%d", g.wp.ntabs);
   dialog->boxmin[11]=1;
   dialog->boxmax[11] = g.wavelet_maxlevels;

   for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
 
   dialog->noofradios=3;
   dialog->noofboxes=12;
   dialog->helptopic=52;
   dialog->want_changecicb = 0;
   dialog->f1 = wavelet_check;
   dialog->f2 = null;
   dialog->f3 = null;
   dialog->f4 = null;
   dialog->f5 = null;
   dialog->f6 = null;
   dialog->f7 = null;
   dialog->f8 = null;
   dialog->f9 = null;
   dialog->width = 450; 
   dialog->height = 0; // calculate automatically
   dialog->transient = 1;
   dialog->wantraise = 0;
   dialog->radiousexy = 0;
   dialog->boxusexy = 0;
   strcpy(dialog->path, g.waveletpath);
   dialog->message[0] = 0;      
   dialog->busy = 0;
   dialogbox(dialog);
}


//--------------------------------------------------------------------------//
// read_wavelet                                                             //
// File Format:                                                             //
//   Lines starting with # or blank = comments                              //
//    [Range low] [Range high]                                              //
//    [Decomposition LP filter coefficients]                                //
//    . . .                                                                 // 
//    [Range low] [Range high]                                              //
//    . . .                                                                 // 
//    [Reconstitution LP filter coefficients]                               //
//    . . . (If absent, use decomposition coefficients)                     // 
//    Any characters after column 80 may be ignored.                        //
//--------------------------------------------------------------------------//
void read_wavelet(Wavelet &w)
{
  FILE *fp;
  int  k, count=0, low=0, high=0, sign = -1;
  char buffer[1024];
  if( g.decimal_point != '.' ) setlocale( LC_NUMERIC, "C" );
  if((fp=fopen(w.filename,"r")) == NULL)
  {   error_message(w.filename, errno);
      return;
  }
  if(read_next_non_comment(fp, buffer)) goto readb;
  sscanf(buffer, "%d %d", &low, &high);
  w.isize = high - low + 1;
  w.jsize = w.isize;
  count = 0;
  ////  Decomposition filter
  for(k=low; k<=high; k++)
  {   if(read_next_non_comment(fp, buffer)) goto readb;
      if(count<1024) w.filter[0][count] = atof(buffer);
      w.filter[1][count] = w.filter[0][count];
      count++;
  }
  if(read_next_non_comment(fp, buffer)) goto readb;
  sscanf(buffer, "%d %d", &low, &high);
  w.jsize = high - low + 1;
  count = 0;
  ////  Reconstitution filter
  for(k=low; k<=high; k++)
  {   if(read_next_non_comment(fp, buffer)) goto readb; 
      if(count<1024) w.filter[1][count] = atof(buffer);
      count++;
  }
  ////  High-pass filters - reversed with alternating sign changes
readb:
  if( g.decimal_point != '.' ) setlocale( LC_NUMERIC, "" );

  w.ioffset = -w.isize/2;
  w.joffset = -(w.jsize+1)/2;
  for(k=0; k<w.isize; k++)
  {
      w.hifilter[1][w.isize-k-1] = sign * w.filter[0][k];
      sign = -sign;
  }
  sign = ((( w.isize - w.jsize ) / 2 ) & 1 ) * 2 - 1; // -1 or +1
          // may still be wrong if one filter is odd and the other even sized
  for(k=0; k<w.jsize; k++)
  {
      w.hifilter[0][w.jsize-k-1] = sign * w.filter[1][k];
      sign = -sign;
  }
  fclose(fp);
}


//--------------------------------------------------------------------------//
// read_next_non_comment                                                    //
//--------------------------------------------------------------------------//
int read_next_non_comment(FILE *fp, char *buffer)
{
   int eof = 0;
   do
   {   fgets(buffer, 1024, fp);
       if(feof(fp)){ eof=1; break; }
       remove_trailing_space(buffer);
   } while( (buffer[0] == '#') || (!strlen(buffer)) );
   return eof;
}


//--------------------------------------------------------------------------//
//  remove_trailing_space - including \f \n \r \t \v and space              //
//--------------------------------------------------------------------------//
void remove_trailing_space(char *s)
{
   int k;
   ////  isspace doesn't match \n as it is supposed to 
   if(strlen(s)>0) for(k=strlen(s)-1; k>=0; k--){ if(isspace(s[k]) || s[k]==10) s[k] = 0; else break; }
}


//--------------------------------------------------------------------------//
//  wavelet_check                                                           //
//--------------------------------------------------------------------------//
void wavelet_check(dialoginfo *a, int radio, int box, int boxbutton)
{
   static int incheck=0;
   if(incheck) return;  //// XtSetValues->dialogstringcb->fftcheck
   int k, noofargs, xss=0, yss=0, status=OK, want_copy_wavelets = 0;
   char **buf;
   char tempstring[128];

   switch(a->radioset[0])
   {  case 1: g.wp.direction=1; break;
      case 2: g.wp.direction = -1; break;
      case 3: want_copy_wavelets = 1; break;
   }      
   switch(a->radioset[1])
   {  case NONE:      g.wavelet_type = 0; break;
      case PYRAMIDAL: g.wavelet_type = 1; break;
      case LAPLACIAN: g.wavelet_type = 2; break;
      case MALLAT:    g.wavelet_type = 3; break;
      case MULTIRESOLUTION: g.wavelet_type = 4; break;
   }
   g.wp.ino = atoi(a->answer[1][0]);
   strcpy(g.waveletpath, a->c[2]->path);
   strcpy(g.wavelet_file, a->answer[2][0]);
   strcpy(g.wp.wave.filename, a->answer[2][0]);
   strcpy(g.wavelet_index_range, a->answer[4][0]);
   strcpy(g.wavelet_coeff_range, a->answer[5][0]);
   strcpy(g.wavelet_ignore_range, a->answer[6][0]);
   g.wavelet_grid = a->boxset[8];
   g.wp.want_gray_offset = a->boxset[9];
   g.wp.gray_offset = atoi(a->answer[9][1]);
   g.wp.levels = atoi(a->answer[10][0]);
   g.wp.ntabs = atoi(a->answer[11][0]);
   
   if(g.wavelet_type == PYRAMIDAL)
       strcpy(g.wp.format, "pyramid");
   else if(g.wavelet_type == LAPLACIAN)
       strcpy(g.wp.format, "Laplace");

   if(want_copy_wavelets){ copy_wavelets(ci); unset_radio(a, radio); return; }

   ////  Bail out if bad ino unless reading one from disk
   if(radio==0 && box==-1 && boxbutton==-1)
   {    if(g.wp.ino<=0 || g.wp.ino>=g.image_count) return;
        if( !z[g.wp.ino].waveletexists )
        {   xss = z[g.wp.ino].origxsize = z[g.wp.ino].xsize;
            yss = z[g.wp.ino].origysize = z[g.wp.ino].ysize;
        }
        if(g.wavelet_type == PYRAMIDAL)
        {
            ////  Make sure image size is a power of 2, otherwise can't 
            ////  handle images whose size is not a power of 2.
            int xlen = z[g.wp.ino].origxsize;
            int ylen = z[g.wp.ino].origysize;
            for(xss=1; xss<=xlen*2; xss*=2) if(xss>=xlen) break;
            for(yss=1; yss<=ylen*2; yss*=2) if(yss>=ylen) break;
            if(z[g.wp.ino].xsize!= xss || z[g.wp.ino].ysize!= yss)
            {   // Enlarge image to nearest power of 2
                if(g.getout){ g.getout=0; return; }
                resize_image(g.wp.ino, xss, yss);
            }
        }
        else if(g.wavelet_type == LAPLACIAN)
        {   xss = (z[g.wp.ino].origxsize * 3)/2;
            yss = z[g.wp.ino].origysize;
            if(z[g.wp.ino].xsize!= xss || z[g.wp.ino].ysize!= yss)
            {   // Enlarge image to accommodate Laplacian pyramid
                if(g.getout){ g.getout=0; return; }
                resize_image(g.wp.ino, xss, yss);
            }
        }
        if(z[g.wp.ino].colortype != GRAY)
        {     message("Warning: converting image\nto grayscale for wavelet transform");
              if(g.getout){ g.getout=0; erase_resize_wavelet(g.wp.ino); return; }
              converttograyscale(g.wp.ino);
        }
   }
   // check that wplevels is not too large
   g.wp.minres = min(xss, yss);
   for(k=1; k<g.wp.levels; k++) g.wp.minres/=2;
   g.wp.minres = max(4, g.wp.minres);

   buf = new char*[4];
   for(k=0;k<4;k++){ buf[k] = new char[32]; buf[k][0] = 0; }

   incheck=1;   // This line must be after last bailout return()

   z[g.wp.ino].wmin = smallest(g.wp.ino);
   z[g.wp.ino].wmax = largest(g.wp.ino);
                                 // index range to use
   strip_extraneous_dashes(g.wavelet_index_range);
   parse_string(g.wavelet_index_range, buf, noofargs, 0, 32);
   if(!strcmp(buf[0],"all"))
   {   g.wp.imin=0; g.wp.imax = xss*yss; }
   else if(!strcmp(buf[0],"none"))
   {   g.wp.imin = 0; g.wp.imax=0; }
   else if(noofargs>=2)
   {   g.wp.imin = atoi(buf[0]);
       if(noofargs==2) g.wp.imax = atoi(buf[1]);
       else g.wp.imax = atoi(buf[2]);
   }else{ g.wp.imin=0; g.wp.imax=xss*yss;}

                              // values to use
   strip_extraneous_dashes(g.wavelet_coeff_range);
   parse_string(g.wavelet_coeff_range, buf, noofargs, 0, 32);
   if(!strcmp(buf[0],"all"))
   {   g.wp.cmin = z[g.wp.ino].wmin*1024; g.wp.cmax = z[g.wp.ino].wmax*1024; }//*max flter length
   else if(!strcmp(buf[0],"none"))
   {   g.wp.cmin=0.0; g.wp.cmax=0.0; }
   else if(noofargs>=2)
   {   g.wp.cmin = atof(buf[0]);
       if(noofargs==2) g.wp.cmax = atof(buf[1]);
       else g.wp.cmax = atof(buf[2]);
   }else{ g.wp.cmin = z[g.wp.ino].wmin*1024; g.wp.cmax = z[g.wp.ino].wmax*1024; }

                              // values to ignore
   strip_extraneous_dashes(g.wavelet_ignore_range);
   parse_string(g.wavelet_ignore_range, buf, noofargs, 0, 32);
   if(!strcmp(buf[0],"all"))
   {   g.wp.ignmin = z[g.wp.ino].wmin; g.wp.ignmax = z[g.wp.ino].wmax; }
   else if(!strcmp(buf[0],"none"))
   {   g.wp.ignmin = 0.0; g.wp.ignmax = 0.0; }
   else if(noofargs>=2)
   {   g.wp.ignmin = atof(buf[0]);
       if(noofargs==2) g.wp.ignmax = atof(buf[1]);
       else g.wp.ignmax = atof(buf[2]);
   }else{ g.wp.ignmin = 0.0; g.wp.ignmax = 0.0; }

   //// Radio buttons
   if(radio==0 && box==-1 && boxbutton==-1)
   {    if( g.wavelet_type == PYRAMIDAL && !strcmp(g.wavelet_file,"none") )
             message("Select a wavelet first");
        else
        {
             ////  Allocate wavelet array for image
             status = allocate_image_wavelet(g.wp.ino, xss, yss, 
                g.wavelet_type, g.wp.levels);
             if(status==OK)
             {  switch(g.wavelet_type)
                {  case PYRAMIDAL: 
                      read_wavelet(g.wp.wave);
                      wavelet_pyramidal_calc(g.wp.wave, g.wp);
                      z[g.wp.ino].wavelettype = PYRAMIDAL;
                      break;
                   case LAPLACIAN:
                      wavelet_laplacian_calc(g.wp);
                      z[g.wp.ino].wavelettype = LAPLACIAN;
                      break;
                   default: message("Not implemented\n",ERROR); break;
                }
                repair(g.wp.ino);
                if(g.wavelet_grid && z[g.wp.ino].waveletstate) 
                     show_wavelet_grid(g.wp.ino, g.wavelet_type);
                if(z[g.wp.ino].waveletstate == 0) erase_resize_wavelet(g.wp.ino);
             }
        }
   }
   if(radio==2 &&box==-1 && boxbutton==-1) 
   {    switch(a->radioset[2])
        {    case 1: write_image_wavelet(g.wp.wave, g.wp); break;
             case 2: read_image_wavelet(0, NULL, g.wp.wave, g.wp); 
                     a->boxmax[1] = g.image_count-1;
                     a->c[1]->maxval = g.image_count-1;
                     sprintf(tempstring, "%d", ci);
                     a->c[1]->answer = ci;
                     a->c[1]->startval = ci;
                     strcpy(a->answer[1][0], tempstring);
                     set_widget_label(a->boxwidget[1][0], tempstring);
                     XtVaSetValues(a->boxwidget[1][0], XmNcursorPosition, strlen(tempstring),NULL); 

                     strcpy(a->answer[2][0], g.wp.wave.filename);
                     strcpy(a->c[2]->title, g.wp.wave.filename);
                     set_widget_label(a->boxwidget[2][0], g.wp.wave.filename);
                     XtVaSetValues(a->boxwidget[2][0], XmNcursorPosition, strlen(g.wp.wave.filename),NULL); 

                     sprintf(tempstring, "%d", g.wp.levels);
                     a->c[10]->answer = g.wp.levels;
                     a->c[10]->startval = g.wp.levels;
                     strcpy(a->answer[10][0], tempstring);
                     set_widget_label(a->boxwidget[10][0], tempstring);
                     XtVaSetValues(a->boxwidget[10][0], XmNcursorPosition, strlen(tempstring),NULL); 
                     break;
             case 3: restoreimage(1); break;
        }
   }
   for(k=0;k<4;k++) delete[] buf[k];
   delete[] buf;
   if(radio == 0) unset_radio(a, radio); 
   incheck=0;
}


//--------------------------------------------------------------------------//
//  allocate_image_wavelet                                                  //
//--------------------------------------------------------------------------//
int allocate_image_wavelet(int ino, int xsize, int ysize, int transform_type, 
    int levels)
{  
    int l;
    if(!between(ino, 0, g.image_count-1) || z[ino].image==NULL){ message("Non-existent image",ERROR); return ERROR; }
    if(z[ino].waveletexists)
    {
        if( z[ino].wavelettype != transform_type )
            erase_resize_wavelet(ino);
        else
            return OK;
    }
    z[ino].wavexsize = xsize;
    z[ino].waveysize = ysize;
    z[ino].nlevels = levels;
    z[ino].wavelettype = transform_type;
    z[ino].wavelet_1d = (double*)malloc(ysize*xsize*sizeof(double));                        
    if(z[ino].wavelet_1d==NULL){ message("Error allocating wavelet array",ERROR); return NOMEM;} 
    z[ino].wavelet = make_2d_alias(z[ino].wavelet_1d, xsize, ysize);
    if(z[ino].wavelet==NULL){ message(g.nomemory,ERROR); return NOMEM; }
    z[ino].nsubimages = transform_type == LAPLACIAN ? levels : 0;
    if(transform_type == LAPLACIAN)
    {
        z[ino].wavelet_3d = new double** [z[ino].nsubimages];
        z[ino].subimagexsize = new int [z[ino].nsubimages];
        z[ino].subimageysize = new int [z[ino].nsubimages];
        if(z[ino].wavelet_3d==NULL || z[ino].subimagexsize==NULL || z[ino].subimageysize==NULL)
        {  message(g.nomemory,ERROR); return NOMEM;
        }
        for( l = 0; l < levels; l++ )
        {   z[ino].subimagexsize[l] = z[ino].origxsize >> l;
            z[ino].subimageysize[l] = z[ino].origysize >> l;
            z[ino].wavelet_3d[l] = new double* [ z[ino].subimageysize[l] ];
            if(z[ino].wavelet_3d[l]==NULL){ message(g.nomemory,ERROR); return NOMEM; }
            z[ino].wavelet_3d[l][0] = l == 0 ? z[ino].wavelet_1d :
                 l == 1 ? z[ino].wavelet_3d[0][0] + z[ino].origxsize :
                 z[ino].wavelet_3d[l-1][z[ino].subimageysize[l-1]-1] + xsize;
            for( int j = 1; j < z[ino].subimageysize[l]; j++ )
            {    z[ino].wavelet_3d[l][j] = z[ino].wavelet_3d[l][j-1] + xsize;
            }
        }
        z[ino].wavelet_xminres = z[ino].origxsize >> ( levels - 1 );
        z[ino].wavelet_yminres = z[ino].origysize >> ( levels - 1 );
        z[ino].wavelet_xlrstart = z[ino].origxsize;
        z[ino].wavelet_ylrstart = 0;
        for( l = 1; l < levels - 1; l++ )
            z[ino].wavelet_ylrstart += z[ino].subimageysize[l];
    }else
    {
        z[ino].wavelet_3d    = NULL;
        z[ino].subimagexsize = NULL; 
        z[ino].subimageysize = NULL;  
        z[ino].nsubimages    = 0;
    } 
    z[ino].waveletexists = 1;
    z[ino].waveletstate = 0;
    z[ino].fmin = 0;
    z[ino].fmax = 0;
    z[ino].wmin = 0;
    z[ino].wmax = 0;
    return OK;
}


//--------------------------------------------------------------------------//
// wavelet_pyramidal_calc                                                   //
// 2-dimensional vavelet transform                                          //
//--------------------------------------------------------------------------//
void wavelet_pyramidal_calc(Wavelet &wave, WaveletParams &wp)
{
   int bpp, ino,f,h,k,nx,ny,v,value,x,y,l,levels,xminres,yminres;
   double *ww;
   double coef=0, frac;
   ino = wp.ino;
   bpp = z[ino].bpp;
   int b = (7+bpp)/8;
   int xsize = z[ino].wavexsize;
   int ysize = z[ino].waveysize;
   f = z[ino].cf;
   z[ino].fmax =-(int)g.maxvalue[bpp];
   z[ino].fmin = (int)g.maxvalue[bpp];
   z[ino].wmax =-(int)g.maxvalue[bpp];
   z[ino].wmin = (int)g.maxvalue[bpp];
   k = min( xsize, ysize );
   levels = 0;
   while( ( k >> levels ) > wp.minres ) levels++;
   z[ino].wavelet_xminres = xminres = xsize >> levels;
   z[ino].wavelet_yminres = yminres = ysize >> levels;
   ww = new double[max(xsize,ysize)];
   frac = (double)wp.gray_offset / g.maxvalue[bpp];

   if(wp.direction==1)    // 1=forward   data >> transform y >> transform x
   {   for(l=0; l<levels; l++)
       {   nx = xsize >> l;
           ny = ysize >> l;
           for(y=0; y<ny; y++)
           {   if(l==0)
               {   if(y<z[ino].ysize) h=z[ino].xsize; else h=0;
                   for(x=0; x<h; x++) 
                       ww[x] = (double)pixelat(z[ino].image[f][y]+x*b, bpp);
                   for(x=h; x<xsize; x++) ww[x] = 0.0;
                   wavelet_transform(wave, ww, nx, 1);
                   for(x=0; x<xsize; x++) z[ino].wavelet[y][x] = ww[x];
               }else
                   wavelet_transform(wave, z[ino].wavelet[y], nx, 1);
           }
           for(x=0; x<nx; x++)
           {   for(y=0; y<ny; y++) ww[y] = z[ino].wavelet[y][x];
               wavelet_transform( wave, ww, ny, 1 );
               for(y=0; y<ny; y++) z[ino].wavelet[y][x] = ww[y];
           }
           if(l==levels-1)
           {   for(y=0; y<ysize; y++)
               {   for(x=0; x<xsize; x++)
                   {   coef = z[ino].wavelet[y][x];
                       if(y>ysize/2 && y>ysize/2)
                       {  z[ino].fmax = max(z[ino].fmax, coef);
                          z[ino].fmin = min(z[ino].fmin, coef);
                       }else
                       if(y<yminres && x<xminres)
                       {  z[ino].wmax = max(z[ino].wmax, coef);
                          z[ino].wmin = min(z[ino].wmin, coef);
                       }
                   }
               }
           }
       }
   }else               // -1=reverse
   { 
       for(y=0; y<ysize; y++)
       for(x=0; x<xsize; x++)
       {   coef = z[ino].wavelet[y][x];
           if(between(coef, wp.cmin, wp.cmax) &&
             !between(coef, wp.ignmin, wp.ignmax))
                z[ino].wavelet[y][x] = coef;
           else z[ino].wavelet[y][x] = 0.0;
       }      
       for(l=levels-1; l>=0; l--)
       {   if(!between(l, wp.imin, wp.imax)) continue;
           nx = xsize >> l;
           ny = ysize >> l;
           for(y=0; y<ny; y++)
               wavelet_transform(wave, z[ino].wavelet[y], nx, 0);
           for(x=0; x<nx; x++)
           {   for(y=0; y<ny; y++) ww[y] = z[ino].wavelet[y][x];
               wavelet_transform(wave, ww, ny, 0);
               for(y=0; y<ny; y++) z[ino].wavelet[y][x] = ww[y];
               if(l==0)
               {   for(y=0; y<z[ino].ysize; y++)
                   {   v = cint(z[ino].wavelet[y][x]);
                       if(wp.want_gray_offset)
                           value = min((int)g.maxvalue[bpp], 
                                   max(0, wp.gray_offset + cint(v*frac)));
                       else
                           value = min((int)g.maxvalue[bpp], max(0,v));
                       putpixelbytes(z[ino].image[f][y]+b*x,value, 1, bpp, 1);
                       z[ino].fmax = max(z[ino].fmax, z[ino].wavelet[y][x]);
                       z[ino].fmin = min(z[ino].fmin, z[ino].wavelet[y][x]);
                   }
               }

           }
       }
   }
   if(wp.direction==1) z[ino].waveletstate++; else  z[ino].waveletstate--;
   if(z[ino].waveletstate) z[ino].fftdisplay = WAVE; else z[ino].fftdisplay = NONE;
   delete[] ww;
}  



//--------------------------------------------------------------------------//
// wavelet_transform - called by wavelet_pyramidal_calc                     //
//--------------------------------------------------------------------------//
void wavelet_transform(Wavelet &wave, double *aa, int n, int direction)
{
   double ai, ai1, *ww;
   int i,ii,jf,jr,k,n1,ni,nj,n2,nmod;
   ww = new double[n];
   if(ww==NULL) message(g.nomemory,ERROR);
   for(k=0;k<n;k++) ww[k]=0.0;
   nmod = (wave.isize+wave.jsize) * n;
   n1 = n-1;
   n2 = n/2;
   if(direction == 1)   // forward
   {   for(ii=0, i=0; i<n; i+=2, ii++)
       {   ni = i + nmod + wave.ioffset;
           nj = i + nmod + wave.joffset;
           for(k=0; k<wave.isize; k++)
           {   jf = n1 & (1 + ni + k);
               ww[ii] += wave.filter[0][k] * aa[jf];
           }
           for(k=0; k<wave.jsize; k++)
           {   jr = n1 & (1 + nj + k);
               ww[ii+n2] += wave.hifilter[0][k] * aa[jr];
           }
       }
   }else               // reverse
   { 
       for(ii=0, i=0; i<n; i+=2, ii++)
       {   ai = aa[ii];
           ai1 = aa[ii+n2];
           nj = i + nmod - wave.jsize - wave.joffset;
           ni = i + nmod - wave.isize - wave.ioffset;
           for(k=0; k<wave.jsize; k++)
           {   jf = n1 & (1 + nj + k);
               ww[jf] += wave.filter[1][k] * ai;
           }
           for(k=0; k<wave.isize; k++)
           {   jr = n1 & (1 + ni + k);
               ww[jr] += wave.hifilter[1][k] * ai1;
           }
      }
   }
   for(k=0;k<n;k++) aa[k] = ww[k];
   delete[] ww;
   ww = NULL;
}


//--------------------------------------------------------------------------//
//  strip_extraneous_dashes                                                 //
//  get rid of any '-' not preceded by a whitespace.                        //
//--------------------------------------------------------------------------//
void strip_extraneous_dashes(char *s)
{
   int k, number=0;
   for(k=0;k<(int)strlen(s);k++)
   {   if(number && s[k]=='-') s[k]=' ';
       if(isdigit(s[k])) number=1;
       if(isspace(s[k])) number=0;
   }   
}


//--------------------------------------------------------------------//
// parse_string                                                       //
// input: a single command with arguments  e.g. "load test 1 1 2 3".  //
// Each word k is placed in buf[k][80]. buf[0][] is the command, the  //
// rest are arguments separated by spaces.                            //
// The no. of arguments, not counting buf[0], is placed in `noofargs'.//
// `startarg' is optional and is 0 by default.                        //
//--------------------------------------------------------------------//
void parse_string(char *cmd, char **buf, int &noofargs, int startarg, int maxlen)
{   
    int count=0;
    char word[128];
    noofargs = 0;
    do
    {    word[0]=0;
         parse_word(cmd, word, 80, count);
         if(strlen(word)==0) break;
         strncpy(buf[startarg + noofargs++], word, maxlen);
         if(count >= (int)strlen(cmd)) break;
    }while(1);
}


//--------------------------------------------------------------------//
// parse_word - returns first word demarcated by a space in `buffer'  //
// 'count' must be initialized to 0 by calling routine before first   //
// call (keeps track of buffer position).                             //
//--------------------------------------------------------------------//
void parse_word(char *buffer, char *word, int maxlen, int &count)
{
    if(maxlen >= 16384) return;
    char tempstring[16384];  
    int k=count, pos=0;
    while(buffer[k])
    {   
         if(buffer[k]!=' ' && buffer[k]!='\n' ) tempstring[pos++]=buffer[k];
         else if(pos) break;
         tempstring[pos]=0;
         k++;
         if(buffer[k]=='\n') break;
    }
    count = k;    
    strcpy(word, tempstring);
}    



//--------------------------------------------------------------------------//
//  smallest                                                                //
//--------------------------------------------------------------------------//
double smallest(int ino)
{
   int i, j;
   double d=0 ,v;
   if(!z[ino].waveletexists) return 0.0;
   for(j=0; j<z[ino].waveysize; j++)
   for(i=0; i<z[ino].wavexsize; i++)
   {   v = z[ino].wavelet[j][i]; 
       if(v < d) d = v;
   } 
   return d;
}

//--------------------------------------------------------------------------//
//  largest                                                                 //
//--------------------------------------------------------------------------//
double largest(int ino)
{
   int i, j;
   double d=0 ,v;
   if(!z[ino].waveletexists) return 0.0;
   for(j=0; j<z[ino].waveysize; j++)
   for(i=0; i<z[ino].wavexsize; i++)
   {   v = z[ino].wavelet[j][i]; 
       if(v > d) d = v;
   } 
   return d;
}


//--------------------------------------------------------------------------//
// setwaveletpixel - if user changes a pixel, change corresponding wavelet  //
// FFT pixels are not changed here, but in setfloatpixel().                 //
//--------------------------------------------------------------------------//
void setwaveletpixel(int x, int y, uint color, int ino)
{
  double value=0;
  if(ino>=0 && z[ino].waveletexists && x>=0 && y>=0 &&
        x<z[ino].wavexsize && y<z[ino].waveysize &&
        z[ino].fftdisplay==WAVE)
  {   if(x<z[ino].wavelet_xminres && y<z[ino].wavelet_yminres && z[ino].wfac!=0.0)
      {
           value = (double)(color - z[ino].gray[3]) / z[ino].ffac + z[ino].gray[1];
      }
      else if(z[ino].ffac!=0.0)
      {
           value = (double)(color - z[ino].gray[3]) / z[ino].ffac + z[ino].gray[1];
      }
      z[ino].wavelet[y][x] = value;
  }
}


//--------------------------------------------------------------------------//
//  show_wavelet_grid                                                       //
//--------------------------------------------------------------------------//
void show_wavelet_grid(int ino, int transform_type)
{
   int i, j;
   int xs = z[ino].xpos;
   int ys = z[ino].ypos;
   int xe;
   int ye;
   g.inmenu++;
   if( transform_type == PYRAMIDAL )
   {
       xe = xs + z[ino].wavexsize - 1;
       ye = ys + z[ino].waveysize - 1;
       line(xs, ys, xs, ye, 255, SET);
       line(xs, ys, xe, ys, 255, SET);
       line(xe, ys, xe, ye, 255, SET);
       line(xs, ye, xe, ye, 255, SET);
       for(j=z[ino].waveysize/2, i=z[ino].wavexsize/2;
          j>z[ino].wavelet_yminres/2;
          i/=2, j/=2)
       {    line(i+xs, 2*j+ys, i+xs, ys, 255, SET);
            line(2*i+xs, j+ys, xs, j+ys, 255, SET);
       }
   } else if( transform_type == LAPLACIAN )
   {
       xe = xs + z[ino].origxsize - 1;
       ye = ys + z[ino].origysize - 1;
       line(xs, ys, xs, ye, 255, SET);
       line(xs, ys, xe, ys, 255, SET);
       line(xe, ys, xe, ye, 255, SET);
       line(xs, ye, xe, ye, 255, SET);
       xs += z[ino].origxsize;
       for(i=1; i < z[ino].nlevels; i++ )
       {    xe = xs + z[ino].subimagexsize[i]-1;
            ye = ys + z[ino].subimageysize[i]-1;
            line(xs, ys, xe, ys, 255, SET);
            line(xe, ys, xe, ye, 255, SET);
            line(xs, ye, xe, ye, 255, SET);
            ys = ye + 1;
       }
   }
   g.inmenu--;
}


//--------------------------------------------------------------------------//
// write_image_wavelet                                                      //
//--------------------------------------------------------------------------//
int write_image_wavelet(Wavelet &wave, WaveletParams &wp)
{
  static PromptStruct ps;
  if(memorylessthan(16384)){ message(g.nomemory,ERROR); return NOMEM; }
  static char waveletfile[FILENAMELENGTH]="wavelet.ascii";
  int ino = wp.ino;

  if(!z[ino].waveletexists)
  {   message("No wavelet exists for this image",ERROR);
      return BADPARAMETERS;
  }
  message("Filename:",waveletfile,PROMPT,FILENAMELENGTH-1,54);
  ps.filename = waveletfile;
  ps.f1 = write_image_wavelet_part2;
  ps.f2 = null;
  ps.ptr2[0] = &wp;
  ps.ptr2[1] = &wave;
  ps.ino = ino;
  check_overwrite_file(&ps);
  return OK;
}

//--------------------------------------------------------------------------//
// write_image_wavelet_part2                                                //
//--------------------------------------------------------------------------//
void write_image_wavelet_part2(PromptStruct *ps)
{
  FILE *fp;
  char *waveletfile = ps->filename;
  WaveletParams wp = *(WaveletParams*) ps->ptr2[0]; 
  Wavelet wave     = *(Wavelet*) ps->ptr2[1]; 
  int ino = ps->ino;
  int i,j;
  int status=OK;
  int size;

  if((fp=fopen(waveletfile, "w")) == NULL)
  {   error_message(waveletfile, errno);
      return;
  }

  fprintf(fp,"Wavelet transform of %s\n",basefilename(z[ino].name));  
  fprintf(fp,"xsize %d\n", z[ino].wavexsize);
  fprintf(fp,"ysize %d\n", z[ino].waveysize);
  fprintf(fp,"bits/pixel %d\n", z[ino].bpp);  
  fprintf(fp,"format %s\n", wp.format);  
  fprintf(fp,"levels %d\n", wp.levels);  
  fprintf(fp,"xminres %d\n", z[ino].wavelet_xminres);  
  fprintf(fp,"xminres %d\n", z[ino].wavelet_yminres);  
  fprintf(fp,"wavelet %s\n", wave.filename);  
  for(j=0;j<z[ino].waveysize;j++)
  {   for(i=0;i<z[ino].wavexsize;i++)
         fprintf(fp,"%g ",z[ino].wavelet[j][i]);
      fprintf(fp,"\n");  
  }

  fclose(fp);
  size = filesize(waveletfile);               // Check to ensure not an empty file
  if(size==0) status=ZEROLENGTH; 
  if(access(waveletfile, F_OK)) status=ERROR; // access = 0 if file exists
  if(status==OK) message("Wavelet coordinates saved");
  else  message("Error saving wavelet",ERROR);
  return;
}


//--------------------------------------------------------------------------//
// read_image_wavelet                                                       //
//--------------------------------------------------------------------------//
int read_image_wavelet(int noofargs, char **arg, Wavelet &wave, WaveletParams &wp)
{
   static char *filename=NULL;
   double ww;
   char temp1[256];
   char temp2[256];
   char temp3[256];
   int bpp=8, xminres, yminres;
   FILE *fp;
   ////  Leave this allocated
   if(filename==NULL){ filename = new char[FILENAMELENGTH]; filename[0]=0; }
   char oldfilename[FILENAMELENGTH]="none";
   if(strlen(filename)) strcpy(oldfilename, filename);
   int i, j, status=OK, xsize, ysize;
   
   if(noofargs == 0) strcpy(filename, getfilename(oldfilename, NULL));
   else  strcpy(filename, arg[1]);
      
   if((fp=fopen(filename, "r")) == NULL)
   {   error_message(filename, errno);
       return(ERROR);
   }
   
   fscanf(fp,"%7s %9s %2s",temp1, temp2, temp3);// 'wavelet transform of'
   if(strcmp(temp1,"Wavelet") != SAME){ message("Invalid wavelet transform file",ERROR); return ERROR; }
   fscanf(fp,"%256s",temp3);                    // [filename]
   fscanf(fp,"%256s %d",temp1, &xsize);         // xsize 12345
   fscanf(fp,"%256s %d",temp1, &ysize);         // ysize 12345   
   fscanf(fp,"%256s %d",temp1, &bpp);           // bits/pixel   
   fscanf(fp,"%256s %s",temp1, wp.format);      // format name
   fscanf(fp,"%256s %d",temp1, &wp.levels);     // no. of levels
   fscanf(fp,"%256s %d",temp1, &xminres);       // x minimum resolution
   fscanf(fp,"%256s %d",temp1, &yminres);       // y minimum resolution
   fscanf(fp,"%256s %s",temp1, wave.filename);  // wavelet [waveletfilename]   
   
   if(newimage("Wavelet",0,0,xsize,ysize,bpp,GRAY,1,g.want_shell,1,PERM,1,
      g.window_border,0)!=OK) return(NOMEM); 
   allocate_image_wavelet(ci, xsize, ysize, PYRAMIDAL, wp.levels );
   z[ci].waveletstate = 1;
   z[ci].wavelet_xminres = xminres;
   z[ci].wavelet_yminres = yminres;
   z[ci].fmin = (int)g.maxvalue[bpp];
   z[ci].fmax = 0;
   z[ci].wmin = (int)g.maxvalue[bpp];
   z[ci].wmax = 0;
   z[ci].fftdisplay = WAVE;

   setimagetitle(ci, temp3);
   for(j=0;j<ysize;j++)
   for(i=0;i<xsize;i++)
   {    if(!fscanf(fp, "%lg", &ww)){ status=ERROR; break; }
        z[ci].wavelet[j][i] = ww; 
        if(j>ysize/2 && i>xsize/2)
        {   z[ci].fmax = max(z[ci].fmax, ww);
            z[ci].fmin = min(z[ci].fmin, ww);
        }else
        if(j<z[ci].wavelet_yminres && i<z[ci].wavelet_xminres)
        {   z[ci].wmax = max(z[ci].wmax, ww);
            z[ci].wmin = min(z[ci].wmin, ww);
        }
   }
   if(status==OK)
   {  switchto(ci);
      repair(ci);
   }else message("Error reading wavelets",ERROR);
   fclose(fp);
   return status;
}


//--------------------------------------------------------------------------//
// wavelet_laplacian_calc                                                   //
// 2-dimensional Burt&Adelson Laplacian pyramid transform                   //
//--------------------------------------------------------------------------//
int wavelet_laplacian_calc(WaveletParams &wp)
{
   int ino,l,levels;
   ino = wp.ino;
   int bpp = z[ino].bpp;
   double **tmp, *tmp_1d;
   double val;
   levels = wp.levels;
   tmp_1d = (double *) malloc( z[ino].subimagexsize[0] * (z[ino].subimageysize[0] / 2) * sizeof(double) );
   if(tmp_1d==NULL){ message("Error allocating temporary",ERROR); return NOMEM;}
   tmp = make_2d_alias(tmp_1d, z[ino].subimagexsize[0], z[ino].subimageysize[0] / 2);
   if(tmp==NULL){ message(g.nomemory,ERROR); free(tmp_1d); return NOMEM; }
   BinomialFilter filter( wp.ntabs );
   if(wp.direction==1)    // 1=forward   data >> transform y >> transform x
   {
       laplace_copy2level0( z[ino] );
       for( l = 1; l < levels; l++ )
       {  laplace_reduce_y( z[ino].wavelet_3d[l-1], tmp, filter,
       			z[ino].subimagexsize[l-1], z[ino].subimageysize[l-1], z[ino].subimageysize[l] );
       	  laplace_reduce_x( tmp, z[ino].wavelet_3d[l], filter,
       			z[ino].subimageysize[l], z[ino].subimagexsize[l-1], z[ino].subimagexsize[l] );
       	  laplace_expand_x( z[ino].wavelet_3d[l], tmp, filter,
       			z[ino].subimageysize[l], z[ino].subimagexsize[l], z[ino].subimagexsize[l-1] );
       	  laplace_expand_y_and_subtract( tmp, z[ino].wavelet_3d[l-1], filter,
       			z[ino].subimagexsize[l-1], z[ino].subimageysize[l], z[ino].subimageysize[l-1] );
       }
       z[ino].fmax =-(int)g.maxvalue[bpp];
       z[ino].fmin = (int)g.maxvalue[bpp];
       z[ino].wmax =-(int)g.maxvalue[bpp];
       z[ino].wmin = (int)g.maxvalue[bpp];
       for( l = 0; l < levels; l++ )
       {	 for( int y = 0; y < z[ino].subimageysize[l]; y++ )
       		 {   for( int x = 0; x < z[ino].subimagexsize[l]; x++ )
       		     {   val = z[ino].wavelet_3d[l][y][x];
      		         if( l == levels - 1 )
      		         {   z[ino].wmax = max(z[ino].wmax, val);
                       z[ino].wmin = min(z[ino].wmin, val);
                   } else
      		         {   z[ino].fmax = max(z[ino].fmax, val);
                       z[ino].fmin = min(z[ino].fmin, val);
                   }
               }
           }
       }
       z[ino].waveletstate++;
    }else                // -1=reverse
    {
       for( l = levels - 1; l > 0; l-- ) {
  	      laplace_expand_x( z[ino].wavelet_3d[l], tmp, filter,
  			      z[ino].subimageysize[l], z[ino].subimagexsize[l], z[ino].subimagexsize[l-1] );
  	      laplace_expand_y_and_add( tmp, z[ino].wavelet_3d[l-1], filter,
  			      z[ino].subimagexsize[l-1], z[ino].subimageysize[l], z[ino].subimageysize[l-1] );
        }
        laplace_copylevel0_2image( z[ino], wp );
        z[ino].waveletstate--;
   }
   if(z[ino].waveletstate) z[ino].fftdisplay = WAVE; else z[ino].fftdisplay = NONE;
   free( tmp_1d );
   free( tmp );
   return OK;
}


//--------------------------------------------------------------------------//
//  BinomialFilter - Provides filter coefficients
//--------------------------------------------------------------------------//
BinomialFilter::BinomialFilter(int ntabs) : length(0), coefficient(0)
{
   length = ntabs;
   coefficient = new double[ntabs];
   int high = ntabs - 1;
   int low = 0;
   int coeff = 1;
   double factor = 1.0 / double( 1 << ( ntabs - 1 ) );
   while( high >= low )  // calculate filter coefficients
   {
      coefficient[ high ] = factor * double( coeff );
      coefficient[ low ] = coefficient[ high ];
      coeff *= high;
      low++;
      coeff /= low;
      high--;
   }
}


//--------------------------------------------------------------------------//
//  laplace_copy2level0  - Copies image to pyramid level 0
//--------------------------------------------------------------------------//
void laplace_copy2level0( Image a )
{
  int bpp = a.bpp;
  int b = (7+bpp)/8;
  int f = a.cf;
	int x, y;
	for( y = 0; y < a.origysize; y++ )
	{
		for( x = 0; x < a.origxsize; x++ )
		{
			a.wavelet_3d[ 0 ][ y ][ x ]
			  = (double)pixelat( a.image[ f ][ y ] + x * b, bpp);
		}
	}
}


//--------------------------------------------------------------------------//
//   Copies image from pyramid level 0 to image
//--------------------------------------------------------------------------//
void laplace_copylevel0_2image( Image a, WaveletParams &wp )
{
  int li, co;
  int bpp = a.bpp;
  int b = (7+bpp)/8;
  int f = a.cf;
  double val;
  int ivalue;
  a.fmax =-(int)g.maxvalue[bpp];
  a.fmin = (int)g.maxvalue[bpp];
  double frac = (double)wp.gray_offset / g.maxvalue[bpp];
  for( li = 0; li < a.origysize; li++ )
  {   for( co = 0; co < a.origxsize; co++ )
      {   val = a.wavelet_3d[0][li][co];
          if( wp.want_gray_offset )
              ivalue = min((int)g.maxvalue[bpp], max(0, wp.gray_offset + cint(val*frac)));
          else
              ivalue = min((int)g.maxvalue[bpp], max(0,cint(val)));
          putpixelbytes(a.image[f][li]+b*co, ivalue, 1, bpp, 1);
          a.fmax = max(a.fmax, val);
          a.fmin = min(a.fmin, val);
      }
  }
}


//--------------------------------------------------------------------------//
//   First reduction step
//--------------------------------------------------------------------------//
void laplace_reduce_y( double **fine_level, double **coarse_level, 
     const BinomialFilter &filter, int size_x, int size_y_fine, int size_y_coarse )
{
	int li, co, k;
	int fsize;
	int offset;
	const double *coeff;
	fsize = filter.tabs();
	offset = fsize / 2;
	coeff = filter.data();
	for( co = 0; co < size_x; co ++)
	{
		for( li = 0; li < size_y_coarse; li++ )
		{
			double sum = 0.0;
			for( k = 0; k < fsize; k++ )
			{
				int j = 2 * li - k + offset; // convolution
				if( j < 0 )
					j = 0;
				else if( j >= size_y_fine )
				  j = size_y_fine - 1;
				sum += coeff[ k ] * fine_level[ j ][ co ];
			}
			coarse_level[ li ][ co ] = sum;
		}
	}
}


//--------------------------------------------------------------------------//
//   Second reduction step
//--------------------------------------------------------------------------//
void laplace_reduce_x( double **fine_level, double **coarse_level, const BinomialFilter & filter,
  			int size_y, int size_x_fine, int size_x_coarse )
{
	int li, co, k;
	int fsize;
	int offset;
	const double *coeff;
	fsize = filter.tabs();
	offset = fsize / 2;
	coeff = filter.data();
	for( li = 0; li < size_y; li ++)
	{
		for( co = 0; co < size_x_coarse; co++ )
		{
			double sum = 0.0;
			for( k = 0; k < fsize; k++ )
			{
				int j = 2 * co - k + offset;  // convolution
				if( j < 0 )
					j = 0;
				else if( j >= size_x_fine )
				  j = size_x_fine - 1;
				sum += coeff[ k ] * fine_level[ li ][ j ];
			}
			coarse_level[ li ][ co ] = sum;
		}
	}
}


//--------------------------------------------------------------------------//
//   Expand image horizontally
//--------------------------------------------------------------------------//
void laplace_expand_x( double **coarse_level, double **fine_level, const BinomialFilter & filter,
  			int size_y, int size_x_coarse, int size_x_fine )
{
	int li, co, k;
	int fsize;
	int offset;
	const double *coeff;
	fsize = filter.tabs();
	offset = ( fsize - 1 ) / 2;
	coeff = filter.data();
	for( li = 0; li < size_y; li ++)
	{
		for( co = 0; co < size_x_fine; co++ )
		{
			double sum = 0.0;
			for( k = ( co + offset ) & 1; k < fsize; k += 2 )
			{       //  co - k + offset is always even
				int j = ( co - k + offset ) / 2;  // convolution
				if( j < 0 )
					j = 0;
				else if( j >= size_x_coarse )
				  j = size_x_coarse - 1;
				sum += coeff[ k ] * coarse_level[ li ][ j ];
			}
			fine_level[ li ][ co ] = 2.0 * sum;
		}
	}
}


//--------------------------------------------------------------------------//
// Expand image vertically and subtract result from existing fine-level image
//  as required for Laplace pyramid generation
//--------------------------------------------------------------------------//
void laplace_expand_y_and_subtract( double **coarse_level, double **fine_level,
				const BinomialFilter & filter,
  			int size_x, int size_y_coarse, int size_y_fine )
{
	int li, co, k;
	int fsize;
	int offset;
	const double *coeff;
	fsize = filter.tabs();
	offset = ( fsize - 1 )/ 2;
	coeff = filter.data();
	for( co = 0; co < size_x; co ++)
	{
		for( li = 0; li < size_y_fine; li++ )
		{
			double sum = 0.0;
			for( k = ( li + offset ) & 1; k < fsize; k += 2 )
			{       //  li - k + offset is always even
				int j = ( li - k + offset ) / 2;  // convolution
				if( j < 0 )
					j = 0;
				else if( j >= size_y_coarse )
				  j = size_y_coarse - 1;
				sum += coeff[ k ] * coarse_level[ j ][ co ];
			}
			fine_level[ li ][ co ] -= 2.0 * sum;
		}
	}
}


//--------------------------------------------------------------------------//
// Expand image vertically and add result to existing fine-level image
//   as required for Laplace pyramid reconstruction
//--------------------------------------------------------------------------//
void laplace_expand_y_and_add( double **coarse_level, double **fine_level,
				const BinomialFilter & filter,
  			int size_x, int size_y_coarse, int size_y_fine )
{
	int li, co, k;
	int fsize;
	int offset;
	const double *coeff;
	fsize = filter.tabs();
	offset = ( fsize - 1 )/ 2;
	coeff = filter.data();
	for( co = 0; co < size_x; co ++)
	{
		for( li = 0; li < size_y_fine; li++ )
		{
			double sum = 0.0;
			for( k = ( li + offset ) & 1; k < fsize; k += 2 )
			{       //  li - k + offset is always even
				int j = ( li - k + offset ) / 2;  // convolution
				if( j < 0 )
					j = 0;
				else if( j >= size_y_coarse )
				  j = size_y_coarse - 1;
				sum += coeff[ k ] * coarse_level[ j ][ co ];
			}
			fine_level[ li ][ co ] += 2.0 * sum;
		}
	}
}


//--------------------------------------------------------------------------//
//  copy_wavelets - grab image of wavelet coeffs                            //
//--------------------------------------------------------------------------//
void copy_wavelets(int ino)
{
   int b,f,bpp,i,i2,j,value;
   if(!z[ino].waveletexists || z[ino].waveletstate==NONE){ message("No wavelets displayed"); return; }

   f = z[ino].cf;
   bpp = z[ino].bpp;
   b = g.off[bpp];

   z[ino].origxsize = z[ino].xsize;
   z[ino].origysize = z[ino].ysize;
   for(j=0; j<z[ino].ysize; j++)
   for(i=0,i2=0; i<z[ino].xsize; i++,i2+=b)
   { 
       value = pixel_equivalent(ino, i, i2, j);
       putpixelbytes(z[ino].image[f][j]+i2, value, 1, bpp);    
   }
}

























                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            