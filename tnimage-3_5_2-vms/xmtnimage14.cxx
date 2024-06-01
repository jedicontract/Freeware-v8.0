//--------------------------------------------------------------------------//
// xmtnimage14.cc                                                           //
// Filtering routines                                                       //
// Latest revision: 06-19-2005                                              //
// Copyright (C) 2005 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//
 
#include "xmtnimage.h"


extern int         ci;                 // current image
extern Image      *z;
extern Globals     g;
int nooffilters = 22;   // No.of different filters in menu
char fffilename[1024];
int in_filter = 0;

//--------------------------------------------------------------------------//
// filter_start                                                             //
// 2-d filter the image                                                     //
// Uses a separate routine for 8-bit mode vs color modes.                   //
//--------------------------------------------------------------------------//
void filter_start(int noofargs, char **arg)
{  
  static int hit=0;
  int bpp = z[ci].bpp;
  static filter *ff;
  ff = new filter;
  ff->ino = ci;
  ff->type = g.filter_type;
  ff->x1 = g.ulx;
  ff->x2 = g.lrx-1;
  ff->y1 = g.uly;
  ff->y2 = g.lry-1;
  ff->ksize = g.filter_ksize;
  ff->sharpfac = g.filter_sharpfac;
  ff->range = g.filter_range;
  ff->kmult = g.filter_kmult;
  ff->maxbkg = g.filter_maxbkg;
  ff->entireimage = 0;
  ff->diffsize = g.filter_diffsize;
  ff->ithresh = g.filter_ithresh;
  ff->local_scale = g.filter_local_scale;
  ff->do_filtering = 1;
  ff->decay = g.filter_decay;
  ff->background = g.filter_background;
  ff->want_progress_bar = 1;
  if(ff->background==0) ff->background = cint((double)g.maxvalue[bpp] * 0.75);
  
  if(memorylessthan(16384)){ message(g.nomemory,ERROR); return; } // minimal memory
  int knumber=1;
  
  //-------------------------------------------------------------------------//
  //  To add a new filter                                                    //
  //  1 increase 1st dimension of arrays three, five, nine, and fifteen      //
  //  2 add new element in menu                                              //
  //  3 increase value of dialog->radiono[0]                                 //
  //  4 add filters below - make sure total value of elements in array is 0  //
  //  5 Increase value of `nooffilters'.                                     //
  //  1st dim.= filter type - 5                                              //
  //-------------------------------------------------------------------------//

  drawselectbox(OFF);
  g.state = BUSY;
  printstatus(g.state);
  if(g.selectedimage>-1 && ci>-1) ff->entireimage=1; else ff->entireimage=0;
  if(ci>=0 && z[ci].is_zoomed) ff->entireimage=0;
  if(hit)
      strcpy(ff->filename, fffilename);
  else
      ff->filename[0] = 0;
  if(noofargs==0) filter_image(ff);
  else
  {   if(noofargs>0) ff->type        = atoi(arg[1]);
      if(noofargs>1) knumber         = atoi(arg[2]);
      if(knumber==1) ff->ksize=3; 
      if(knumber==2) ff->ksize=5; 
      if(knumber==3) ff->ksize=9; 
      if(knumber==4) ff->ksize=15; 
      if(noofargs>2) ff->sharpfac    = atoi(arg[3]);
      if(noofargs>3) ff->range       = atoi(arg[4]);
      if(noofargs>4) ff->kmult       = atoi(arg[5]);
      if(noofargs>5) ff->maxbkg      = atoi(arg[6]);
      if(noofargs>6) ff->diffsize    = atoi(arg[7]);
      if(noofargs>7) ff->ithresh     = atoi(arg[8]);
      if(noofargs>8) ff->local_scale = atoi(arg[9]);
      if(noofargs>9) ff->decay       = atof(arg[10]);
      if(noofargs>10) ff->background = atoi(arg[11]);
      filter_region(ff);
 }
 g.getout=0;
 g.state = NORMAL;
 hit = 1;
}


//--------------------------------------------------------------------------//
// filter_image                                                             //
//--------------------------------------------------------------------------//
void filter_image(filter *ff)
{
   static Dialog *dialog;
   dialog = new Dialog;
   if(dialog==NULL){ message(g.nomemory); return; }
   int j,k,bpp;
   bpp = z[ci].bpp;

   strcpy(dialog->title,"Filter");
   dialog->l[1]       = new listinfo;

   dialog->l[1]->title = new char[100];
   dialog->l[1]->info  = new char*[nooffilters];
   dialog->l[1]->count = nooffilters;
   dialog->l[1]->wantfunction = 0;
   dialog->l[1]->f1    = null;
   dialog->l[1]->f2    = null;
   dialog->l[1]->f3    = null;
   dialog->l[1]->f4    = null; // dialog lists are deleted in dialogcancelcb
   dialog->l[1]->f5    = null;
   dialog->l[1]->f6    = null;
   dialog->l[1]->listnumber = 0;
   dialog->l[1]->wc    = 0;
   for(k=0;k<nooffilters;k++) dialog->l[1]->info[k] = new char[100];
   dialog->l[1]->selection = &ff->type;
   strcpy(dialog->l[1]->title,"Filter Type");
   strcpy(dialog->l[1]->info[0],"Low-pass");
   strcpy(dialog->l[1]->info[1],"High-pass");
   strcpy(dialog->l[1]->info[2],"Laplace edge enhan.");
   strcpy(dialog->l[1]->info[3],"Background subtract");
   strcpy(dialog->l[1]->info[4],"Background flatten ");
   strcpy(dialog->l[1]->info[5],"Noise(median) filter");
   strcpy(dialog->l[1]->info[6],"Sharpen (/)");
   strcpy(dialog->l[1]->info[7],"Sharpen (|)");
   strcpy(dialog->l[1]->info[8],"Sharpen (-)");
   strcpy(dialog->l[1]->info[9],"Sharpen (\\)");
   strcpy(dialog->l[1]->info[10],"Edge detect (-)");
   strcpy(dialog->l[1]->info[11],"Edge detect (|)");
   strcpy(dialog->l[1]->info[12],"Sobel edge detect.");
   strcpy(dialog->l[1]->info[13],"Engrave");
   strcpy(dialog->l[1]->info[14],"Multiplicative Sobel");
   strcpy(dialog->l[1]->info[15],"Spatial difference");
   strcpy(dialog->l[1]->info[16],"Maximize local contrast");
   strcpy(dialog->l[1]->info[17],"Adaptive maximum local contrast");
   strcpy(dialog->l[1]->info[18],"Force background to fixed value");
   strcpy(dialog->l[1]->info[19],"Unsharp mask");
   strcpy(dialog->l[1]->info[20],"User-defined");
   strcpy(dialog->l[1]->info[21],"Sharpen edges only");

   strcpy(dialog->radio[0][0],"Kernel");
   strcpy(dialog->radio[0][1]," 3 x  3");
   strcpy(dialog->radio[0][2]," 5 x  5");
   strcpy(dialog->radio[0][3]," 9 x  9");
   strcpy(dialog->radio[0][4],"15 x 15");
 
   if(ff->ksize==3) dialog->radioset[0]=1;
   if(ff->ksize==5) dialog->radioset[0]=2;
   if(ff->ksize==9) dialog->radioset[0]=3;
   if(ff->ksize==15) dialog->radioset[0]=4;

   strcpy(dialog->radio[1][0],"Background level");
   strcpy(dialog->radio[1][1],"Black");
   strcpy(dialog->radio[1][2],"White");

   if(ff->maxbkg==0) dialog->radioset[1]=1;
   else  dialog->radioset[1]=2;

   strcpy(dialog->boxes[0],"Parameters");
   strcpy(dialog->boxes[1],"Filter type");
   strcpy(dialog->boxes[2],"Range(noise filter)");
   strcpy(dialog->boxes[3],"Kernel multiplier");
   strcpy(dialog->boxes[4],"Amount of filtering");
   strcpy(dialog->boxes[5],"Filter filename");
   strcpy(dialog->boxes[6],"Difference filter size");
   strcpy(dialog->boxes[7],"Threshold");
   strcpy(dialog->boxes[8],"Local contrast scale");
   strcpy(dialog->boxes[9],"Decay factor");
   strcpy(dialog->boxes[10],"Background value");
   dialog->boxtype[0]=LABEL;
   dialog->boxtype[1]=NON_EDIT_LIST;
   dialog->boxtype[2]=INTCLICKBOX;
   dialog->boxtype[3]=INTCLICKBOX;
   dialog->boxtype[4]=INTCLICKBOX;
   dialog->boxtype[5]=SCROLLEDSTRING;
   dialog->boxtype[6]=INTCLICKBOX;
   dialog->boxtype[7]=INTCLICKBOX;
   dialog->boxtype[8]=INTCLICKBOX;
   dialog->boxtype[9]=DOUBLECLICKBOX;
   dialog->boxtype[10]=INTCLICKBOX;
 
   dialog->boxmin[1] = 1; dialog->boxmax[1] = nooffilters;
   dialog->boxmin[2] = 1; dialog->boxmax[2] = 200;
   dialog->boxmin[3] = 1; dialog->boxmax[3] = 50;
   dialog->boxmin[4] = 1; dialog->boxmax[4] = 100;
   dialog->boxmin[6] = 1; dialog->boxmax[6] = 100;
   dialog->boxmin[7] = 0; dialog->boxmax[7] = 255;
   dialog->boxmin[8] = 1; dialog->boxmax[8] = 200;
   dialog->boxmin[9] = 0; dialog->boxmax[9] = 1;
   dialog->boxmin[10] = 0; dialog->boxmax[10] = (int)g.maxvalue[bpp];
 
   sprintf(dialog->answer[2][0],"%d", ff->range);
   sprintf(dialog->answer[3][0],"%d", ff->kmult);
   sprintf(dialog->answer[4][0],"%d", ff->sharpfac);
   strcpy( dialog->answer[5][0], ff->filename); 
   sprintf(dialog->answer[6][0],"%d", ff->diffsize);
   sprintf(dialog->answer[7][0],"%d", ff->ithresh);
   sprintf(dialog->answer[8][0],"%d", ff->local_scale);
   sprintf(dialog->answer[9][0],"%g", ff->decay);
   sprintf(dialog->answer[10][0],"%d",ff->background);
 
   dialog->radiono[0]=5;
   dialog->radiono[1]=3;
   dialog->radiono[2]=0;
   for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
 
   dialog->noofradios = 2;
   dialog->noofboxes = 11;
   dialog->helptopic = 7;
   dialog->want_changecicb = 0;
   dialog->f1 = filtercheck;
   dialog->f2 = null;
   dialog->f3 = null;
   dialog->f4 = filterfinish;
   dialog->f5 = null;
   dialog->f6 = null;
   dialog->f7 = null;
   dialog->f8 = null;
   dialog->f9 = null;
   dialog->transient = 1;
   dialog->wantraise = 0;
   dialog->radiousexy = 0;
   dialog->boxusexy = 0;
   dialog->width = 450;
   dialog->height = 0; // calculate automatically
   dialog->ptr[12] = (void*)ff;
   strcpy(dialog->path,".");
   g.getout=0;
   dialog->message[0]=0;      
   in_filter = 1;
   dialog->busy = 0;
   dialogbox(dialog);
}


//--------------------------------------------------------------------------//
//  filtercheck                                                             //
//--------------------------------------------------------------------------//
void filtercheck(dialoginfo *a, int radio, int box, int boxbutton)
{
  if(!in_filter) return;
  radio=radio; box=box; boxbutton=boxbutton;
  int j,k,oci=ci;
  int bsens[11] = { 1,1,0,1,1,1,0,0,0,0,0 }; // box sensitivity
  int rsens[2] = { 1,0 };                    // radio sensitivity
  filter *ff = (filter*)a->ptr[12];
  if(a->radioset[0]==1) ff->ksize = 3; 
  if(a->radioset[0]==2) ff->ksize = 5; 
  if(a->radioset[0]==3) ff->ksize = 9; 
  if(a->radioset[0]==4) ff->ksize = 15;
  ff->type = *a->l[1]->selection;
  if(a->radioset[1]==1) ff->maxbkg=0; else ff->maxbkg=1;
  ff->range = atoi(a->answer[2][0]);
  ff->kmult = atoi(a->answer[3][0]); if(ff->kmult<1) ff->kmult=1;
  ff->sharpfac = atoi(a->answer[4][0]);
  strcpy(ff->filename, a->answer[5][0]);  
  ff->diffsize = atoi(a->answer[6][0]);
  ff->ithresh  = atoi(a->answer[7][0]);
  ff->local_scale = atoi(a->answer[8][0]);
  ff->decay       = atof(a->answer[9][0]);
  ff->background  = atoi(a->answer[10][0]);
  ff->ino = ci;
  ff->x1 = g.ulx;
  ff->x2 = g.lrx-1;
  ff->y1 = g.uly;
  ff->y2 = g.lry-1;

  if(g.selectedimage>-1 && ci>-1) ff->entireimage = 1; else ff->entireimage = 0;
  if(ci>=0 && z[ci].is_zoomed) ff->entireimage=0;
  if(ff->type<=2 && ff->kmult>1)
  {   if(g.want_messages > 1) message("Invalid parameters - \nSetting kernel multiplier to 1",WARNING);
      ff->kmult = 1;
      sprintf(a->answer[3][0],"%d", ff->kmult);      
  }
  switch(ff->type)
  { 
     case  3: rsens[1]=1; break;
     case  5: bsens[2]=1; rsens[1]=1; break;
     case 13: bsens[7]=1; rsens[1]=0; break;
     case 15: bsens[6]=1; rsens[0]=0; break;
     case 16: bsens[8]=1; rsens[0]=0; break;
     case 17: bsens[8]=1; rsens[0]=0; bsens[9]=1; break;
     case 18: bsens[10]=1;break;
     case 19: bsens[10]=1;break;
     case 20: bsens[5]=1; break;
     case 21: bsens[2]=0; bsens[7]=1; break;
  
  }
  for(k=1; k<11; k++) XtSetSensitive(a->boxwidget[k][0], bsens[k]);
  for(k=0; k<2; k++) for(j=1; j<a->radiono[k]; j++) 
      XtSetSensitive(a->radiowidget[k][j], rsens[k]);

  if(radio==-2 && ff->do_filtering) filter_region(ff);
  if(!in_filter) return;
  if(oci != ci) switchto(oci);
  g.filter_type        = ff->type;
  g.filter_ksize       = ff->ksize;
  g.filter_sharpfac    = ff->sharpfac;
  g.filter_range       = ff->range;
  g.filter_kmult       = ff->kmult;
  g.filter_maxbkg      = ff->maxbkg;
  g.filter_diffsize    = ff->diffsize;
  g.filter_ithresh     = ff->ithresh;
  g.filter_local_scale = ff->local_scale;
  g.filter_decay       = ff->decay;
  g.filter_background  = ff->background;
  repair(ci);
}


//--------------------------------------------------------------------------//
//  filterfinish                                                            //
//--------------------------------------------------------------------------//
void filterfinish(dialoginfo *a)
{
  filter *ff = (filter *)a->ptr[12];
  strcpy(fffilename, ff->filename);
  delete[] ff; 
  in_filter = 0;
}


//--------------------------------------------------------------------------//
// filter_region                                                            //
//--------------------------------------------------------------------------//
int filter_region(filter *ff)
{
  if(!in_filter) return 0;
  Widget www, scrollbar;
  int status=OK,f,h=0,bad=0,i,j,k,l,a,index,xindex,ii,jj,asource,
      dx,dy,k2,l2,v,hit=0, skip=0,rr,gg,bb,noscreen=0,
      count,maxbpp,bestvalue=0,sum,rsum,gsum,bsum,
      rvalue=0,gvalue=0,bvalue=0,bpp,obpp=0,newvalue,
      centerfac=1,bufindex,iii,jjj,lll,convert24=0,
      s1,s2,s3,s4,kk,ll,      // For Sobel
      value=0,value2,v1,v2,v3,v4,rv1=0,rv2=0,rv3=0,rv4=0,xsize,
      gv1=0,gv2=0,gv3=0,gv4=0,bv1=0,bv2=0,bv3=0,bv4=0,
      orr,ogg,obb,x1,y1,x2,y2,
      rv,rs1,rs2,rs3,rs4,gv,gs1,gs2,gs3,gs4,bv,bs1,bs2,bs3,bs4;
 
  int *cpix;
  double average=0,fx,fy,dv,dvx,dvy,fx1,fy1=0,fy2=0,rfy1=0,gfy1=0,bfy1=0,
         rdv,rdvx,rdvy,gdv,gdvx,gdvy,bdv,bdvx,bdvy,v21=0,v43=0,v31=0,v42=0,
         shold, shnew;

  if(!ff->do_filtering) return ABORT;

  //// Recalculate coords in case image was moved
  if(ff->entireimage)
  {   ff->x1 = g.ulx;
      ff->x2 = g.lrx-1;
      ff->y1 = g.uly;
      ff->y2 = g.lry-1;
  }

  g.busy=1;
  g.getout = 0;
  switch(ff->type)
  {  case 13: filter_engrave(ff); g.busy=0; return OK;
     case 14: mult_sobel(ff); g.busy=0; return OK;
     case 15: filter_diff(ff); g.busy=0; return OK;
     case 16: localcontrast(ff); g.busy=0; return OK;
     case 17: localcontrast_adaptive(ff); g.busy=0; return OK;
     case 18: force_background(ff); g.busy=0; return OK;
     case 19: subtract_background(ff); g.busy=0; return OK;
     case 20: filter_user_defined(ff); g.busy=0; return OK;
     case 21: filter_sharpen_edges(ff); g.busy=0; return OK;
  }
  if(!between(ff->type, 0, 20)){ message("Bad parameters", ERROR); g.busy=0; return BADPARAMETERS;}

  const int three[6][3][3] = {
                             { {   0, -1,  1 },
                               {  -1,  1,  1 },
                               {  -1,  1,  0 }  },

                             { {  -1,  0,  1 },
                               {  -1,  1,  1 },
                               {  -1,  0,  1 }  },

                             { {  -1, -1, -1 },
                               {   0,  1,  0 },
                               {   1,  1,  1 }  },

                             { {   1,  1,  0 },
                               {   1,  1, -1 },
                               {   0, -1, -1 }  },

                             { {  -1, -1, -1 },
                               {   1,  2,  1 },
                               {   0,  0,  0 }  },

                             { {  -1,  1,  0 },
                               {  -1,  2,  0 },
                               {  -1,  1,  0 }  } };

                                                            
  const int five[6][5][5] = {{ {   0,  0, -1, -1,  1 },
                               {   0, -1, -1,  1,  1 },
                               {  -1, -1,  1,  1,  1 },
                               {  -1, -1,  1,  1,  0 },
                               {  -1,  1,  1,  0,  0 }  },

                             { {  -1, -1,  0,  1,  1 },
                               {  -1, -1,  0,  1,  1 },
                               {  -1, -1,  1,  1,  1 },
                               {  -1, -1,  0,  1,  1 },
                               {  -1, -1,  0,  1,  1 }  },

                             { {  -1, -1, -1, -1, -1 },
                               {  -1, -1, -1, -1, -1 },
                               {   1,  1,  1, -1, -1 },
                               {   1,  1,  1,  1,  1 },
                               {   1,  1,  1,  1,  1 }  },

                             { {   1,  1,  1,  0,  0 },
                               {   1,  1,  1,  1,  0 },
                               {  -1, -1, -1,  1,  1 },
                               {   0, -1, -1, -1,  1 },
                               {   0,  0, -1, -1, -1 }  },

                             { {  -1, -1, -1, -1, -1 },
                               {  -1, -1, -1, -1, -1 },
                               {   2,  2,  3,  2,  2 },
                               {  -0, -0, -0, -0, -0 },
                               {  -0, -0, -0, -0, -0 }  },
 
                             { {  -1, -1,  2,  0,  0 },
                               {  -1, -1,  2,  0,  0 },
                               {  -1, -1,  3,  0,  0 },
                               {  -1, -1,  2,  0,  0 },
                               {  -1, -1,  2,  0,  0 }  } };
  

  const int nine[6][9][9] ={ { {   0,  0,  0, -1, -1, -1, -1, -1,  1 },
                               {   0,  0, -1, -1, -1, -1, -1,  1,  1 },
                               {   0, -1, -1, -1, -1, -1,  1,  1,  1 },
                               {  -1, -1, -1, -1, -1,  1,  1,  1,  1 },
                               {  -1, -1, -1, -1,  1,  1,  1,  1,  1 },
                               {  -1, -1, -1, -1,  1,  1,  1,  1,  1 },
                               {  -1, -1, -1,  1,  1,  1,  1,  1,  0 },
                               {  -1, -1,  1,  1,  1,  1,  1,  0,  0 },
                               {  -1,  1,  1,  1,  1,  1,  0,  0,  0 }  },

                             { {  -1, -1, -1, -1, -0,  1,  1,  1,  1 },
                               {  -1, -1, -1, -1, -0,  1,  1,  1,  1 },
                               {  -1, -1, -1, -1, -0,  1,  1,  1,  1 },
                               {  -1, -1, -1, -1, -0,  1,  1,  1,  1 },
                               {  -1, -1, -1, -1,  1,  1,  1,  1,  1 },
                               {  -1, -1, -1, -1,  0,  1,  1,  1,  1 },
                               {  -1, -1, -1, -1,  0,  1,  1,  1,  1 },
                               {  -1, -1, -1, -1,  0,  1,  1,  1,  1 },
                               {  -1, -1, -1, -1,  0,  1,  1,  1,  1 }  },

                             { {  -1, -1, -1, -1, -1, -1, -1, -1, -1 },
                               {  -1, -1, -1, -1, -1, -1, -1, -1, -1 },
                               {  -1, -1, -1, -1, -1, -1, -1, -1, -1 },
                               {  -1, -1, -1, -1, -1, -1, -1, -1, -1 },
                               {   0,  0,  0,  0,  1,  0,  0,  0,  0 },
                               {   1,  1,  1,  1,  1,  1,  1,  1,  1 },
                               {   1,  1,  1,  1,  1,  1,  1,  1,  1 },
                               {   1,  1,  1,  1,  1,  1,  1,  1,  1 },
                               {   1,  1,  1,  1,  1,  1,  1,  1,  1 }  },

                             { {  -1,  1,  1,  1,  1,  1,  0,  0,  0 },
                               {  -1, -1,  1,  1,  1,  1,  1,  0,  0 },
                               {  -1, -1, -1,  1,  1,  1,  1,  1,  0 },
                               {  -1, -1, -1, -1,  1,  1,  1,  1,  1 },
                               {  -1, -1, -1, -1,  1,  1,  1,  1,  1 },
                               {  -1, -1, -1, -1, -1,  1,  1,  1,  1 },
                               {   0, -1, -1, -1, -1, -1,  1,  1,  1 },
                               {   0,  0, -1, -1, -1, -1, -1,  1,  1 },
                               {   0,  0,  0, -1, -1, -1, -1, -1,  1 }  },

                             { {  -1, -1, -1, -1, -1, -1, -1, -1, -1 },
                               {  -1, -1, -1, -1, -1, -1, -1, -1, -1 },
                               {  -1, -1, -1, -1, -1, -1, -1, -1, -1 },
                               {  -1, -1, -1, -1, -1, -1, -1, -1, -1 },
                               {   4,  4,  4,  4,  5,  4,  4,  4,  4 },
                               {   0,  0,  0,  0,  0,  0,  0,  0,  0 },
                               {   0,  0,  0,  0,  0,  0,  0,  0,  0 },
                               {   0,  0,  0,  0,  0,  0,  0,  0,  0 },
                               {   0,  0,  0,  0,  0,  0,  0,  0,  0 }  },

                             { {  -1, -1, -1, -1,  4,  0,  0,  0,  0 },
                               {  -1, -1, -1, -1,  4,  0,  0,  0,  0 },
                               {  -1, -1, -1, -1,  4,  0,  0,  0,  0 },
                               {  -1, -1, -1, -1,  4,  0,  0,  0,  0 },
                               {  -1, -1, -1, -1,  5,  0,  0,  0,  0 },
                               {  -1, -1, -1, -1,  4,  0,  0,  0,  0 },
                               {  -1, -1, -1, -1,  4,  0,  0,  0,  0 },
                               {  -1, -1, -1, -1,  4,  0,  0,  0,  0 },
                               {  -1, -1, -1, -1,  4,  0,  0,  0,  0 }  } };
                                      


  const int fifteen[6][15][15] = {

        { {    0,  0,  0,  0,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
          {    0,  0,  0,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1 },
          {    0,  0,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1,  1 },
          {    0,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1,  1,  1 },
          {    0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1, -1, -1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1,  1,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0 },
          {   -1, -1, -1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0 },
          {   -1, -1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0 },
          {   -1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0 },
          {    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0 }  },

        { {   -1, -1, -1, -1, -1, -1, -1,  0,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1,  0,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1,  0,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1,  0,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1,  0,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1,  0,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1,  1,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1, -1,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1, -1,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1, -0,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1, -0,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1, -0,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1, -0,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1, -0,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1, -0,  1,  1,  1,  1,  1,  1,  1 }  },

        { {   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
          {   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
          {   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
          {   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
          {   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
          {   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
          {   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
          {    0,  0,  0,  0,  0,  0, -1,  2, -1,  0,  0,  0,  0,  0,  0 },
          {    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
          {    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
          {    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
          {    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
          {    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
          {    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
          {    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 }  },


        { {    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0 },
          {   -1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0 },
          {   -1, -1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0 },
          {   -1, -1, -1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0 },
          {   -1, -1, -1, -1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0 },
          {   -1, -1, -1, -1, -1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1, -1,  1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1, -1, -1,  1,  1,  1,  1,  1,  1 },
          {   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1,  1,  1,  1,  1 },
          {    0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1,  1,  1,  1 },
          {    0,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1,  1,  1 },
          {    0,  0,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1,  1 },
          {    0,  0,  0,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1 },
          {    0,  0,  0,  0,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }  },

        { {   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
          {   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
          {   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
          {   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
          {   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
          {   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
          {   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
          {    7,  7,  7,  7,  7,  7,  7,  8,  7,  7,  7,  7,  7,  7,  7 },
          {    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
          {    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
          {    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
          {    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
          {    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
          {    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
          {    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }  },

        { {   -1, -1, -1, -1, -1, -1, -1,  7,  0,  0,  0,  0,  0,  0,  0 },
          {   -1, -1, -1, -1, -1, -1, -1,  7,  0,  0,  0,  0,  0,  0,  0 },
          {   -1, -1, -1, -1, -1, -1, -1,  7,  0,  0,  0,  0,  0,  0,  0 },
          {   -1, -1, -1, -1, -1, -1, -1,  7,  0,  0,  0,  0,  0,  0,  0 },
          {   -1, -1, -1, -1, -1, -1, -1,  7,  0,  0,  0,  0,  0,  0,  0 },
          {   -1, -1, -1, -1, -1, -1, -1,  7,  0,  0,  0,  0,  0,  0,  0 },
          {   -1, -1, -1, -1, -1, -1, -1,  7,  0,  0,  0,  0,  0,  0,  0 },
          {   -1, -1, -1, -1, -1, -1, -1,  8,  0,  0,  0,  0,  0,  0,  0 },
          {   -1, -1, -1, -1, -1, -1, -1,  7,  0,  0,  0,  0,  0,  0,  0 },
          {   -1, -1, -1, -1, -1, -1, -1,  7,  0,  0,  0,  0,  0,  0,  0 },
          {   -1, -1, -1, -1, -1, -1, -1,  7,  0,  0,  0,  0,  0,  0,  0 },
          {   -1, -1, -1, -1, -1, -1, -1,  7,  0,  0,  0,  0,  0,  0,  0 },
          {   -1, -1, -1, -1, -1, -1, -1,  7,  0,  0,  0,  0,  0,  0,  0 },
          {   -1, -1, -1, -1, -1, -1, -1,  7,  0,  0,  0,  0,  0,  0,  0 },
          {   -1, -1, -1, -1, -1, -1, -1,  7,  0,  0,  0,  0,  0,  0,  0 }  } };


  x1 = ff->x1;  y1 = ff->y1;  
  x2 = ff->x2;  y2 = ff->y2;

  shnew = ((double)ff->sharpfac)/100.0;  // Fraction of new pixel to use
  shold = 1.0-shnew;                     // Fraction of old pixel to keep

  a=ff->ksize/2;        //              kernel     a
                        //                3        1 
                        //                5        2 
                        //                9        4
                        //               15        7
  asource = a*ff->kmult;// 1/2 the kernel size * kernel multiplier, used for
                        // gathering pixels farther away.
                        // asource is max. distance from the center pixel
                        // kmult is no. of source pixels to skip
  centerfac = ff->ksize*ff->ksize;   // Factor to multiply center pixel by (for 
                                     // high-pass), or factor to divide surrounding
                                     // summed pixels by (low-pass).
  xsize = x2 - x1;
  count = ff->ksize*ff->ksize;
  count = min(count, xsize+2*asource-1);
                             // Pick which nth lowest pixel to use if back-
                             // ground or median filtering
  if(ff->type==3) bestvalue = ff->ksize/2; 
  if(ff->type==5) bestvalue = count/2;
                 // Array for storing one line of filtered pixels
                 // Make it big enough for biggest possible image (32 bpp)
  array<int> saveline(xsize+3,18*ff->kmult);      
  array<int> bkbuf(xsize+2,36);        
  if(!bkbuf.allocated) status=NOMEM;
  for(k=0; k<=g.image_count; k++) z[k].hit=0;
                 // Array for storing each center rgb for 1 row of pixels
  cpix     = new int[xsize+2*asource+4];
                 // Array for storing each column of summed source pixels
  array<int> colbuf(xsize+2*asource+4,3); 
  array<int> sorted(xsize+2*asource+2,3);    
  if(!sorted.allocated) status=NOMEM;
  maxbpp = g.bitsperpixel;
                 // Filtering speed can be doubled if we know all the pixels
                 // are on the same image.
  if(ci>=0)
  {   if(z[ci].shell && ff->entireimage) switchto(ci);
      if(z[ci].colortype==INDEXED) 
      {   
          status = change_image_depth(ci,24,0); 
          convert24 = 1; 
      }
      bpp = z[ci].bpp;
  }else bpp = g.bitsperpixel;


  if(status!=OK)
  {   message("Insufficient memory buffers,\naborting",ERROR);
      if(cpix    !=NULL) delete[] cpix;
      g.busy=0;
      return -1;   
  }

  //// Start of filtering - This section tests each pixel to determine    
  //// whether the image at that point wants to be color. If so, the pixel
  //// is treated as color and its r,g,and b are filtered separately      
  //// then combined.                                                     
  //// The destination pixel (pixel being changed) is i,j.                
  //// Leave a boundary of asource pixels around the outside to avoid     
  //// boundary effects.                                                  
  //// For background flattening, get 4 corners for gradient              

  g.want_switching = 0;     //// Make sure they dont click around on other images
  if(ff->type==4)                          // background flattening
  {    dx=(x2-x1)/3; dy=(y2-y1)/3;         // Increments
       get_average_value(x1   ,y1   ,x1+dx,y1+dy,v1,rv1,gv1,bv1);
       get_average_value(x2-dx,y1   ,x2   ,y1+dy,v2,rv2,gv2,bv2);
       get_average_value(x1   ,y2-dy,x1+dx,y2   ,v3,rv3,gv3,bv3);
       get_average_value(x2-dx,y2-dy,x2   ,y2   ,v4,rv4,gv4,bv4);
       average = (v1+v2+v3+v4)/4;
       v21=(double)(v2-v1);
       v43=(double)(v4-v3);
       v31=(double)(v3-v1);
       v42=(double)(v4-v2);
  }  

  ii = ci;
  if(ff->want_progress_bar) progress_bar_open(www, scrollbar);
  g.getout = 0;
  for(j=y1+asource; j<y2-asource; j++)        
  { 
    if(ff->want_progress_bar) progress_bar_update(scrollbar, 100*(j-y1-asource)/(y2-y1-2*asource));
    if(keyhit()) if(getcharacter()==27){ skip = 1; break; }
    if(g.getout){ skip = 1; break; }
    if(j>0)
       index = j%(asource+1); 
    else
       index = asource + (j - asource) % (asource+1);

    switch(ff->type)
    { case 0:       // low pass
      case 1:       // high pass
      case 2:       // laplace
      {
         for(i=x1; i<x2;i++)     
         {  hit=1;         
                    // ii is image no. at pixel i,j.
                    // This line takes 1/4 the processing time
                    // so speed it up by reading directly from image buffer.
            if(ff->entireimage && ii>=0 && z[ii].bpp==8)
            {
               jjj = j-z[ii].ypos;
               iii = i-z[ii].xpos; 
               if(iii<0 || jjj<0)    // Sanity check
               {   fprintf(stderr,"Internal error at line %d in %s, values=%d %d\n",__LINE__,__FILE__,iii,jjj);
                   bad=1;
                   break;
               }
               if(jjj>=0 && jjj<z[ii].ysize && iii>=0 && iii<z[ii].xsize)
                   cpix[i-x1] = z[ii].image[z[ii].cf][jjj][iii];
            }else if(ff->entireimage && ii>=0 && z[ii].bpp>8)
            { 
               bpp = z[ii].bpp;
               jjj = j-z[ii].ypos;
               iii = i-z[ii].xpos; 
               if(iii<0 || jjj<0)    // Sanity check
               {   fprintf(stderr,"Internal error at line %d in %s, values=%d %d\n",__LINE__,__FILE__,iii,jjj);
                   bad=1;
                   break;
             }
               if(jjj>=0 && jjj<z[ii].ysize && iii>=0 && iii<z[ii].xsize)
                   cpix[i-x1] = pixelat(z[ii].image[z[ii].cf][jjj]+iii*g.off[bpp],bpp); 
            }else
            {
               cpix[i-x1] = (int)readpixelonimage(i,j,bpp,ii);
            }
            if(bpp!=obpp) 
            { 
               if(ii>=0 && bpp==8)
               {  obpp=bpp; 
                  switch_palette(ii);
               }
            }   
            if(ii>=0){ z[ii].hit=1; noscreen=1; } else noscreen=0;
            if(ii>=0 && (!g.wantr || !g.wantg || !g.wantb)) z[ii].hit=1;
            maxbpp = max(bpp,maxbpp);
            if((bpp==8)||(ii>=0 && z[ii].hitgray))
            {   
                colbuf.p[0][i-x1]=0; // Collect colums of pixels
                   // Sum the values of all needed source pixels in each column.
                   // This loop takes about 40% the processing time

                if(ff->entireimage)
                {   for(l=j-asource; l<=j+asource; l+=ff->kmult)    
                    {  lll = l-z[ii].ypos;
                       iii = i-z[ii].xpos; 
                       if(lll>=0 && lll<z[ii].ysize && iii>=0 && iii<z[ii].xsize)
                           colbuf.p[0][i-x1] +=
                               pixelat(z[ii].image[z[ii].cf][lll] + iii*g.off[bpp], bpp); 
                    }
                }else
                { 
                     for(l=j-asource; l<=j+asource; l+=ff->kmult)    
                        colbuf.p[0][i-x1]+=(int)readpixelonimage(i,l,bpp,ii);
                }
            }else
            {  
                colbuf.p[0][i-x1]=0; // Collect colums of pixels
                colbuf.p[1][i-x1]=0;
                colbuf.p[2][i-x1]=0;

                   // Sum the values of all needed source pixels in each column.
                   // This loop takes 40% of the processing time. If all
                   //  pixels are on the same image, we can speed it up 2x
                   //  by using pixelat().

                if(ff->entireimage && ii>=0)
                {   bpp=z[ii].bpp;
                    for(l=j-asource; l<=j+asource; l+=ff->kmult)    
                    {  lll = l-z[ii].ypos;
                       iii = i-z[ii].xpos; 
                       if(lll>=0 && lll<z[ii].ysize && iii>=0 && iii<z[ii].xsize)
                           value = pixelat(z[ii].image[z[ii].cf][lll]+iii*g.off[bpp],bpp); 
                       valuetoRGB(value,rr,gg,bb,bpp);
                       colbuf.p[0][i-x1] += rr;
                       colbuf.p[1][i-x1] += gg;
                       colbuf.p[2][i-x1] += bb;
                    }
                }else
                { 
                    for(l=j-asource; l<=j+asource; l+=ff->kmult)    
                    {  value = (int)readpixelonimage(i,l,bpp,ii);
                       valuetoRGB(value,rr,gg,bb,bpp);
                       colbuf.p[0][i-x1] += rr;
                       colbuf.p[1][i-x1] += gg;
                       colbuf.p[2][i-x1] += bb;
                    }
                }  
            }
            
         } 
         if(bad) break;  
                   // Iterate through colbuf, add up the appropriate columns,
                   // and add the center pixel.
                   // This loop takes about 1/2 the processing time.
         for(i=x1+asource; i<x2-asource; i++)     
         { 
           ii=whichimage(i,j,bpp);
           if(ii>=0){ z[ii].hit=1; noscreen=1; } else noscreen=0;
           if(ii>=0 && (!g.wantr || !g.wantg || !g.wantb)) z[ii].hit=1;
           if(bpp!=obpp) 
           {  if(ii>=0 && bpp==8)
              {  obpp=bpp; 
                 switch_palette(ii);
              }
           }   
           maxbpp = max(bpp,maxbpp);

           if(bpp==8 || (ii>=0 && z[ii].hitgray))
           {  sum = 0;
              for(l=i-asource; l<=i+asource; l+=ff->kmult)
                  sum += colbuf.p[0][l-x1];   
              switch(ff->type)
              {   case 0: value = sum / centerfac;            
                          value = (int)(shnew*value + shold*cpix[i-x1]); 
                          break;
                  case 1: 
                          value = -sum + centerfac*cpix[i-x1]; 
                          value = cpix[i-x1] + (int)(shnew*value);
                          break;
                  case 2: value = sum - centerfac*cpix[i-x1]; 
                          value = cpix[i-x1] + (int)(shnew*value);
                          break;
              }   

                   //  Next set the pixel to its new value 
              if(value<0) value=0;
              if(value>(int)g.maxvalue[bpp]) value=(int)g.maxvalue[bpp];
              xindex = i - (x1 + asource);

              if(j-asource > y1+asource)
              {    //  This speeds it up 2x here but it only speeds it up
                   //  for 8 bpp.
                if(ff->entireimage && ii>=0 && bpp==8)
                {   jjj = j-asource-1-z[ii].ypos;
                    iii = i-z[ii].xpos;               
                    if(jjj>=0 && jjj<z[ii].ysize && iii>=0 && iii<z[ii].xsize)
                        z[ii].image[z[ii].cf][jjj][iii] = saveline.p[index][xindex];
                }else
                {  
                    if(j>y1+1+asource &&( g.selected_is_square || inside_irregular_region(i,j-asource-1)))
                    setpixelonimage(i,j-asource-1,saveline.p[index][xindex],SET,
                        bpp,-1,noscreen);
                }
              }
              saveline.p[index][xindex]=value;    
           }else
           {  rsum = 0;
              gsum = 0;
              bsum = 0;
              for(l=i-asource; l<=i+asource; l+=ff->kmult)
              {   rsum += colbuf.p[0][l-x1];
                  gsum += colbuf.p[1][l-x1];
                  bsum += colbuf.p[2][l-x1];
              }    
              valuetoRGB(cpix[i-x1],rr,gg,bb,bpp);                         
              switch(ff->type)
              {   case 0:   rvalue = (int)(rsum * shnew / centerfac + rr*shold); 
                            gvalue = (int)(gsum * shnew / centerfac + gg*shold); 
                            bvalue = (int)(bsum * shnew / centerfac + bb*shold); 
                            break;
                  case 1:   
                            rvalue = -rsum + centerfac * rr; 
                            gvalue = -gsum + centerfac * gg; 
                            bvalue = -bsum + centerfac * bb; 
                            rvalue = rr + (int)(shnew * rvalue); 
                            gvalue = gg + (int)(shnew * gvalue); 
                            bvalue = bb + (int)(shnew * bvalue); 
                            break;
                  case 2: 
                            rvalue = rsum - centerfac * rr; 
                            gvalue = gsum - centerfac * gg; 
                            bvalue = bsum - centerfac * bb; 
                            rvalue = rr + (int)(shnew * rvalue); 
                            gvalue = gg + (int)(shnew * gvalue); 
                            bvalue = bb + (int)(shnew * bvalue); 
                            break;
               }   
                    //  Next set the pixel to its new value  //
               if(rvalue<0) rvalue=0;
               if(gvalue<0) gvalue=0;
               if(bvalue<0) bvalue=0;
               if(rvalue>g.maxred[bpp]  ) rvalue=g.maxred[bpp];
               if(gvalue>g.maxgreen[bpp]) gvalue=g.maxgreen[bpp];
               if(bvalue>g.maxblue[bpp] ) bvalue=g.maxblue[bpp];
               value = RGBvalue(rvalue,gvalue,bvalue,bpp);
               xindex = i - (x1 + asource);
               if(j > y1+1+2*asource)
               {   if(g.selected_is_square || inside_irregular_region(i,j-asource-1))
                   {   newvalue = saveline.p[index][xindex];
                       setpixelonimage(i,j-asource-1,newvalue,SET,bpp,-1,noscreen);
                   }
               }
               saveline.p[index][xindex] = value;
           }
         }
      }
      
      break;

           // Background subtraction or median filtering     
      case 3:
      case 5:
      { 
         bufindex = 0;                    // Collect ksize rows of pixels,
                                          // put in sequential rows of bkbuf
                                          // to avoid reading same xvalue twice
         for(l=j-asource; l<=j+asource; l+=ff->kmult)       // Source y value
         { 
          for(k=x1; k<x2; k++)                             // Source x value
            {   hit=1;
                bkbuf.p[bufindex][k-x1] = (int)readpixelonimage(k,l,bpp,ii); 
            }   
            bufindex++;
           }
         for(i=x1+asource; i<x2-asource; i++)               //Destination x 
         {  h = 0;    
            ii=whichimage(i,j,bpp);
            if(bpp!=obpp) 
            {   if(ii>=0 && bpp==8)
                {  obpp=bpp; 
                   switch_palette(ii);
                }
            }   
            if(ii>=0){ z[ii].hit=1; noscreen=1; } else noscreen=0;
            if(ii>=0 && (!g.wantr || !g.wantg || !g.wantb)) z[ii].hit=1; 
            maxbpp = max(bpp,maxbpp);
                                                            // Read pixels from buffer
            if(bpp==8 || (ii>=0 && z[ii].hitgray))
            {  
               for(bufindex=0; bufindex<ff->ksize; bufindex++)    // Source y value
               for(k=i-asource; k<=i+asource; k+=ff->kmult)// Source x value
               {
                   if(h+1 >= xsize+2*asource) break; 
                   sorted.p[0][h++] = bkbuf.p[bufindex][k-x1];
               }

               if(ff->maxbkg==0)
               {  for(ii=0;ii<=bestvalue;ii++)              // Partial sort
                    for(jj=ii;jj<count;jj++)
                      if(sorted.p[0][ii] > sorted.p[0][jj]) 
                          swap(sorted.p[0][ii], sorted.p[0][jj]);
               }else
               {  for(ii=count-1;ii>=bestvalue;ii--)          // Partial reverse sort
                   for(jj=ii;jj>=0;jj--)
                     if(sorted.p[0][ii] > sorted.p[0][jj]) 
                        swap(sorted.p[0][ii], sorted.p[0][jj]);
               }


               if(ff->type==3)
               {  if(ff->maxbkg!=0) 
                  {  value = (int)readpixelonimage(i,j,bpp,ii);
                     value2 = 255-(sorted.p[0][bestvalue]-value);
                     value = (int)(shold*value + shnew*value2);
                  }   
                  else
                    value=(int)readpixelonimage(i,j,bpp,ii) - sorted.p[0][bestvalue];               
               }
               else 
               {  value = (int)readpixelonimage(i,j,bpp,ii);
                  if(abs(value-sorted.p[0][bestvalue])>ff->range) value = sorted.p[0][bestvalue];
               }
             
                                     //  Set the pixel to its new value 
  
               if(value<0) value=0;
               else if(value>(int)g.maxvalue[bpp]) value=(int)g.maxvalue[bpp];
               xindex = i - (x1 + asource);
     
               value2 = readpixelonimage(i,j,bpp,ii);
               value = cint(shnew*value + shold*value2);            
               value = max(0, min((int)g.maxvalue[bpp],value));
               if(j>y1+1+asource && (g.selected_is_square || inside_irregular_region(i,j-asource-1)))
                  setpixelonimage(i,j-asource-1,saveline.p[index][xindex],SET,bpp,-1,noscreen);
               saveline.p[index][xindex]=value;    
            }else
            {                        // Read pixels from buffer
               for(bufindex=0; bufindex<ff->ksize; bufindex++) // Source y value
               { for(k=i-asource; k<=i+asource; k+=ff->kmult)  // Source x value
                 {   valuetoRGB(bkbuf.p[bufindex][k-x1],rr,gg,bb,bpp);
                     sorted.p[0][h] = rr;
                     sorted.p[1][h] = gg;
                     sorted.p[2][h] = bb;
                     h++;
                 }
               }
               if(ff->maxbkg==0)
               {  for(ii=0;ii<=bestvalue;ii++)             // Partial sort
                    for(jj=ii;jj<count;jj++)
                    { if(sorted.p[0][ii]>sorted.p[0][jj]) 
                         swap(sorted.p[0][ii],sorted.p[0][jj]);
                      if(sorted.p[1][ii]>sorted.p[1][jj]) 
                         swap(sorted.p[1][ii],sorted.p[1][jj]);
                      if(sorted.p[2][ii]>sorted.p[2][jj]) 
                         swap(sorted.p[2][ii],sorted.p[2][jj]);
                    }  
               }else
               {  for(ii=count-1;ii>=bestvalue;ii--)     // Partial reverse sort
                   for(jj=ii;jj>=0;jj--)
                   { if(sorted.p[0][ii]>sorted.p[0][jj])swap(sorted.p[0][ii],sorted.p[0][jj]);
                     if(sorted.p[1][ii]>sorted.p[1][jj])swap(sorted.p[1][ii],sorted.p[1][jj]);
                     if(sorted.p[2][ii]>sorted.p[2][jj])swap(sorted.p[2][ii],sorted.p[2][jj]);
                   }  
               }
               value = (int)readpixelonimage(i,j,bpp,ii);
               valuetoRGB(value,rr,gg,bb,bpp);
               orr = rr; ogg = gg; obb = bb;
               if(ff->type==3)
               {  if(ff->maxbkg!=0) 
                  {  rr = g.maxred[bpp]   - (sorted.p[0][bestvalue] - rr);
                     gg = g.maxgreen[bpp] - (sorted.p[1][bestvalue] - gg);
                     bb = g.maxblue[bpp]  - (sorted.p[2][bestvalue] - bb);
                  }else
                  {  rr = rr - sorted.p[0][bestvalue];               
                     gg = gg - sorted.p[1][bestvalue];               
                     bb = bb - sorted.p[2][bestvalue];               
                  }
               }else
               {  
                  if(abs(rr-sorted.p[0][bestvalue])>ff->range) rr = sorted.p[0][bestvalue];
                  if(abs(gg-sorted.p[1][bestvalue])>ff->range) gg = sorted.p[1][bestvalue];
                  if(abs(bb-sorted.p[2][bestvalue])>ff->range) bb = sorted.p[2][bestvalue];
               }   
             
               rvalue = cint(shnew*rr + shold*orr);            
               gvalue = cint(shnew*gg + shold*ogg);            
               bvalue = cint(shnew*bb + shold*obb);            

                                     //  Set the pixel to its new value 
               rvalue=min(max(0,rvalue),g.maxred[bpp]);
               gvalue=min(max(0,gvalue),g.maxgreen[bpp]);
               bvalue=min(max(0,bvalue),g.maxblue[bpp]);
               xindex = i - (x1 + asource);
               if(j>y1+1+asource &&(g.selected_is_square||inside_irregular_region(i,j-asource-1)))
                 setpixelonimage(i,j-asource-1,saveline.p[index][xindex],SET,bpp,-1,noscreen);
               saveline.p[index][xindex]=RGBvalue(rvalue,gvalue,bvalue,bpp);    
            }
         }      
      }
      break;

    //// For background flattening, subtract dv from each pixel, where dv is
    //// dvx=fx(1-fy)(v2-v1) + fx*fy(v4-v3)     v1  o     o v2              
    //// dvy=fy(1-fx)(v3-v1) + fx*fy(v4-v2)                                 
    //// dv =fx*vx + fy*vy                                                  
    //// fx=frac. along x direction                                         
    //// fy=frac. along y direction             v3  o     o v4              
    //// For color images, this is done for each color.                     

      case 4:
      {
       fy = (double)(j-y1)/(double)(y2-y1);
       if(bpp==8 || (ii>=0 && z[ii].hitgray))
       {  fy1=v21*(1.0-fy);
          fy2=fy*v43;
       }else
       {  rfy1=(double)(rv2-rv1)*(1.0-fy);
          gfy1=(double)(gv2-gv1)*(1.0-fy);
          bfy1=(double)(bv2-bv1)*(1.0-fy);
       }
       for(k=x1; k<=x2; k++)     // Source x value
       {   
         ii=whichimage(k,j,bpp);
         if(ii>=0){ z[ii].hit=1; noscreen=1; } else noscreen=0;
         if(ii>=0 && (!g.wantr || !g.wantg || !g.wantb)) z[ii].hit=1;
         fx = (double)(k-x1)/(double)(x2-x1);
         fx1=1.0-fx;
         if(bpp==8 || (ii>=0 && z[ii].hitgray))
         {   
              dvx = fx*( fy1 + fy2 );
              dvy = fy*( fx1*v31 + fx*v42 );
              dv = fx*dvx + fy*dvy;
              value = (int)readpixelonimage(k,j,bpp,ii);
              value2 = cint(value - dv + average/8);
              value2 = max(0, min((int)g.maxvalue[bpp],value2));
              value = cint(shnew*value2 + shold*value);
              // Make sure it doesn't go to 0
              if(g.selected_is_square || inside_irregular_region(k,j))
                 setpixelonimage(k,j,value,SET,bpp,-1,noscreen);
         }else
         {    rdvx = fx*( rfy1 + fy*(double)(rv4-rv3) );
              gdvx = fx*( gfy1 + fy*(double)(gv4-gv3) );
              bdvx = fx*( bfy1 + fy*(double)(bv4-bv3) );
              rdvy = fy*( (double)(rv3-rv1)*fx1 + fx*(double)(rv4-rv2) );
              gdvy = fy*( (double)(gv3-gv1)*fx1 + fx*(double)(gv4-gv2) );
              bdvy = fy*( (double)(bv3-bv1)*fx1 + fx*(double)(bv4-bv2) );
              rdv = fx*rdvx + fy*rdvy;
              gdv = fx*gdvx + fy*gdvy;
              bdv = fx*bdvx + fy*bdvy;
              value = readpixelonimage(k,j,bpp,ii);
              valuetoRGB(value,rr,gg,bb,bpp);

              rv = rr - cint(rdv);
              gv = gg - cint(gdv);
              bv = bb - cint(bdv);

              rr = cint(shnew*rv + shold*rr);
              gg = cint(shnew*gv + shold*gg);
              bb = cint(shnew*bv + shold*bb);

              rr = max(0,min(rr, g.maxred[bpp]));
              gg = max(0,min(gg, g.maxgreen[bpp]));
              bb = max(0,min(bb, g.maxblue[bpp]));
              value = RGBvalue(rr,gg,bb,bpp);
              if(g.selected_is_square || inside_irregular_region(k,j))
                 setpixelonimage(k,j,value,SET,bpp,-1,noscreen);
         }
       }
       skip=1;
       hit=1;
      }
      break;
    
      //// Sobel edge detection           
      case 12:                                            // Sobel
      {                                                   // i,j=The Pixel
         hit=1;
         for(i=x1+asource; i<x2-asource; i++)             //Dest.x pixel 
         { 
           ii=whichimage(i,j,bpp);
           if(ii>=0){ z[ii].hit=1; noscreen=1; } else noscreen=0;
           if(ii>=0 && (!g.wantr || !g.wantg || !g.wantb)) z[ii].hit=1;
           if(bpp==8 || (ii>=0 && z[ii].hitgray))         // Monochrome Sobel
           {
             s1=0;s2=0;s3=0;s4=0;                         // k,l=source pixel
             for(l=j-asource; l<=j+asource; l+=ff->kmult) // Source y value
             for(k=i-asource; k<=i+asource; k+=ff->kmult) // Source x value
             {
                kk=k-i+asource;     // kk={ 0..kernel size }
                ll=l-j+asource;     // ll={ 0..kernel size }

                v= (int)readpixelonimage(k,l,bpp,ii);     // Source pixel
                if(kk>ll) s1+=v; else if(ll>kk) s1-=v;             // Sum left diag

                if(kk+ll> asource*2) { s2+=v; }                    // Sum r diag
                else if(kk+ll< asource*2){ s2-=v; }

                if(kk>asource) s3+=v; else if(kk<asource) s3-=v;   // Sum vert
                if(ll>asource) s4+=v; else if(ll<asource) s4-=v;   // Sum horiz
             }
             v = max(max(abs(s1),abs(s2)), max(abs(s3),abs(s4)));
             value = readpixelonimage(i,j,bpp,ii);
             v = cint(shnew*v + shold*value);            
             v=max(0,min((int)g.maxvalue[bpp],v));
           }else
           {                                              // Colour Sobel
             rs1=0;rs2=0;rs3=0;rs4=0;                     // k,l=source pixel
             gs1=0;gs2=0;gs3=0;gs4=0;  
             bs1=0;bs2=0;bs3=0;bs4=0;  
             for(l=j-asource; l<=j+asource; l+=ff->kmult) // Source y value
             for(k=i-asource; k<=i+asource; k+=ff->kmult) // Source x value
             {
                kk=k-i+asource;     // kk={ 0..kernel size }
                ll=l-j+asource;     // ll={ 0..kernel size }

                v= (int)readpixelonimage(k,l,bpp,ii);     // Source pixel
                valuetoRGB(v,rr,gg,bb,bpp);
                if(kk>ll){ rs1+=rr; gs1+=gg; bs1+=bb;}             // Sum left diag.
                else if(ll>kk){ rs1-=rr; gs1-=gg; bs1-=bb; }                                      

                if(kk+ll>asource*2){rs2+=rr; gs2+=gg; bs2+=bb; }   // Sum right diag
                else if(kk+ll<asource*2){ rs2-=rr; gs2-=gg; bs2-=bb; }

                if(kk>asource){ rs3+=rr; gs3+=gg; bs3+=bb; }       // Sum |
                else if(kk<asource){ rs3-=rr; gs3-=gg; bs3-=bb; }

                if(ll>asource){ rs4+=rr; gs4+=gg; bs4+=bb; }       // Sum -
                else if(ll<asource){ rs4-=rr; gs4-=gg; bs4-=bb; }
             }
             rv=max(max(abs(rs1),abs(rs2)),max(abs(rs3),abs(rs4)));
             gv=max(max(abs(gs1),abs(gs2)),max(abs(gs3),abs(gs4)));
             bv=max(max(abs(bs1),abs(bs2)),max(abs(bs3),abs(bs4)));
             value = readpixelonimage(i,j,bpp,ii);
             valuetoRGB(value,orr,ogg,obb,bpp);
             rv = cint(shnew*rv + shold*orr);            
             gv = cint(shnew*gv + shold*ogg);            
             bv = cint(shnew*bv + shold*obb);            
             rv=max(0,min(g.maxred[bpp],rv));
             gv=max(0,min(g.maxgreen[bpp],gv));
             bv=max(0,min(g.maxblue[bpp],bv));
             v=RGBvalue(rv,gv,bv,bpp);
           }
           xindex = i - (x1 + asource);
           if(j>y1+1+asource && (g.selected_is_square || inside_irregular_region(i,j-asource-1)))
               setpixelonimage(i,j-asource-1,saveline.p[index][xindex],SET,bpp,-1,noscreen);
           saveline.p[index][xindex]=v;    
         }
      }
      break;
     
      default:                                            // Arbitrary filter 
      {  f = ff->type-6;
         for(i=x1+asource; i<x2-asource; i++)//Dest.x pixel 
         { 
           ii=whichimage(i,j,bpp);
           if(ii>=0){ z[ii].hit=1; noscreen=1; } else noscreen=0;
           if(ii>=0 && (!g.wantr || !g.wantg || !g.wantb)) z[ii].hit=1;
           if(bpp!=obpp) 
           {  if(ii>=0 && bpp==8)
              {  obpp=bpp; 
                 switch_palette(ii);
              }
           }   
           maxbpp = max(bpp,maxbpp);
           if(bpp==8 || (ii>=0 && z[ii].hitgray))
           {   value = 0;
               v = 0;
               for(l=j-asource; l<=j+asource; l+=ff->kmult) // Source y value
               {  l2 = l - j + asource;              
                  for(k=i-asource; k<=i+asource; k+=ff->kmult)// Source x value
                  {  k2 = k - i + asource;
                     hit=1;
                     value = (int)readpixelonimage(k,l,bpp,ii);
                     switch(ff->ksize)
                     { case 3: v += three[f][l2][k2]*value;  break;
                       case 5: v += five[f][l2][k2]*value;   break;
                       case 9: v += nine[f][l2][k2]*value;   break;
                       case 15:v += fifteen[f][l2][k2]*value;break;
                       default: break; 
                     }   
                  }
               }      
               if(ff->sharpfac<100) 
                    value = (int)(shold*readpixelonimage(i,j,bpp,ii) + shnew*v);
               else 
                    value=v;

               if(value<0) value=0;
               if(value>(int)g.maxvalue[bpp]) value=(int)g.maxvalue[bpp];
                                      //  Next set the pixel to its new value  //
               xindex = i - (x1 + asource);
               if(j>1+y1+asource && (g.selected_is_square || inside_irregular_region(i,j-asource-1)))
                 setpixelonimage(i,j-asource-1,saveline.p[index][xindex],SET,
                    bpp,-1,noscreen);
               saveline.p[index][xindex]=value;    
           }else
           {   rvalue = 0;
               gvalue = 0;
               bvalue = 0;
               for(l=j-asource; l<=j+asource; l+=ff->kmult)    // Source y value
               {  l2 = l - j + asource;              
                  for(k=i-asource; k<=i+asource; k+=ff->kmult) // Source x value
                  {  k2 = k - i + asource;
                     hit=1;
                     valuetoRGB((int)readpixelonimage(k,l,bpp,ii),rr,gg,bb,bpp);
                     switch(ff->ksize)
                     {   case 3: rvalue += three[f][l2][k2] * rr;
                                 gvalue += three[f][l2][k2] * gg;
                                 bvalue += three[f][l2][k2] * bb;
                                 break;
                         case 5: rvalue += five[f][l2][k2] * rr;
                                 gvalue += five[f][l2][k2] * gg;
                                 bvalue += five[f][l2][k2] * bb;
                                 break;
                         case 9: rvalue += nine[f][l2][k2] * rr;
                                 gvalue += nine[f][l2][k2] * gg;
                                 bvalue += nine[f][l2][k2] * bb;
                                 break;
                         case 15:rvalue += fifteen[f][l2][k2] * rr;
                                 gvalue += fifteen[f][l2][k2] * gg;
                                 bvalue += fifteen[f][l2][k2] * bb;
                                 break;
                         default: break;
                      }   
                  }
               }      
               if(ff->sharpfac<100)
               {  valuetoRGB((int)readpixelonimage(i,j,bpp,ii),rr,gg,bb,bpp);
                  rvalue = (int)(rr*shold + rvalue*shnew); 
                  gvalue = (int)(gg*shold + gvalue*shnew); 
                  bvalue = (int)(bb*shold + bvalue*shnew); 
               }
               if(rvalue<0) rvalue=0;
               if(gvalue<0) gvalue=0;
               if(bvalue<0) bvalue=0;
               if(rvalue>g.maxred[bpp]  ) rvalue=g.maxred[bpp];
               if(gvalue>g.maxgreen[bpp]) gvalue=g.maxgreen[bpp];
               if(bvalue>g.maxblue[bpp] ) bvalue=g.maxblue[bpp];
                                      //  Next set the pixel to its new value  //
               xindex = i - (x1 + asource);
               if(j>1+y1+asource && (g.selected_is_square || inside_irregular_region(i,j-asource-1)))
                 setpixelonimage(i,j-asource-1,saveline.p[index][xindex],SET,
                   bpp,-1,noscreen);
               saveline.p[index][xindex]=RGBvalue(rvalue,gvalue,bvalue,bpp);    
           }
         }
      }   
    }
    if(bad) break;  
  }
 
  //------------------end of color-specific code for filtering----------//
 
               // Write out the extra pixels             

  if(!hit)
    message("Please select an image or\nlarger area to filter",ERROR);
  else if(!bad)
  {
    if(!skip)
      for(j=y2-asource; j<=y2; j++)    //destination y pixel
      {  if(j>0) index = j%(asource+1); 
         else    index = asource + (j - asource) % (asource+1);
                                                         //destination x pixel
        for(i=x1+asource; i<x2-asource; i++)
         { 
            ii=whichimage(i,j,bpp);
            if(bpp!=obpp) 
            {  if(ii>=0 && bpp==8)
               {  obpp=bpp; 
                  switch_palette(ii);
               }
            }   
            if(ii>=0){ z[ii].hit=1; noscreen=1; } else noscreen=0;
            if(ii>=0 && (!g.wantr || !g.wantg || !g.wantb)) z[ii].hit=1;
            xindex = i - (x1 + asource);
            if(j>y1+1+asource &&(g.selected_is_square||inside_irregular_region(i,j-asource-1)))
               setpixelonimage(i,j-asource-1,saveline.p[index][xindex],SET,bpp,-1,noscreen);
         }   
      }
  }

  if(ff->want_progress_bar) progress_bar_close(www);
  if(convert24) change_image_depth(ci,8,0);

  //// Re-convert any color images touched if in 8-bpp screen mode
  for(k=0; k<g.image_count; k++) 
     if(z[k].hit)
     { 
       rebuild_display(k); 
       redraw(k); 
     }

  redrawscreen();               // Redraw main screen
  delete[] cpix;
  if(bad) printf("Error %d in filter at line __LINE__\n",bad);
  g.want_switching = 1;
  g.busy=0;
  return ii;
}


//--------------------------------------------------------------------------//
// get_average_value                                                        //
//--------------------------------------------------------------------------//
void get_average_value(int x1,int y1,int x2,int y2,int &v,int &rv,int &gv,int &bv)
{ 
    int count,bpp,ii,j,k,rr,gg,bb,value;
    v=0; count=0;
    for(j=y1;j<=y2;j++)                 // Source y value
    for(k=x1;k<=x2;k++)                 // Source x value
    {   value = readpixelonimage(k,j,bpp,ii);
        valuetoRGB(value,rr,gg,bb,bpp);
        v+=value; rv+=rr; gv+=gg; bv+=bb;
        count++;
    }
    v/=count; rv/=count; gv/=count; bv/=count;
    return;
}    


//--------------------------------------------------------------------------//
//  filter_user_defined - not optimized for speed                           //
//--------------------------------------------------------------------------//
void filter_user_defined(filter *ff)
{
  Widget www, scrollbar;
  int count,i,ii,ino,j,k,l,bpp,v=0,noscreen=1,maxv,edge,
      gray=0,pix,rr,gg,bb,rr2,gg2,bb2,rsum=0,gsum=0,bsum=0,
      rstart,gstart,bstart,vstart,bufrow,savebufrow,bufrow_to_draw=0,x1,y1,x2,y2;
  double total, shnew, shold;
  double **filterdata;
  double *filterdata_1d;
  FILE *fp;
  ino = ff->ino;
  bpp = z[ino].bpp;
  shnew = ((double)ff->sharpfac)/100.0;  // Fraction of new pixel to use
  shold = 1.0-shnew;                     // Fraction of old pixel to keep
  vstart = (int) g.maxvalue[bpp] / 2;
  rstart = 0;
  gstart = 0;
  bstart = 0;
  maxv = (int)g.maxvalue[bpp];
  x1 = ff->x1;  x2 = ff->x2;
  y1 = ff->y1;  y2 = ff->y2;

  char tempstring[FILENAMELENGTH];
  for(k=0;k<100;k++) check_event();   
  if((fp=fopen(ff->filename,"r")) == NULL)
  {   error_message(ff->filename, errno);
      return;
  }else
  {   
      if(!strlen(basefilename(ff->filename)))
      {  sprintf(tempstring,"File %s\n not found",ff->filename);
         message(tempstring, ERROR);
         fclose(fp);
         return;
      }
      total = count = 0;
      fscanf(fp,"%d ", &ff->ksize); 

      filterdata_1d = new double[ff->ksize*ff->ksize];                        
      if(filterdata_1d == NULL) { message(g.nomemory,ERROR); return; } 
      filterdata = make_2d_alias(filterdata_1d, ff->ksize, ff->ksize);

      for(j=0;j<ff->ksize;j++)
      for(k=0;k<ff->ksize;k++) 
      {
         fscanf(fp,"%lg ", &filterdata[j][k]); 
         total += filterdata[j][k];
         count++;
      }
      if(count != ff->ksize*ff->ksize)
      {  sprintf(tempstring,"Filter has wrong number of\nelements (expecting %d, found %d)",ff->ksize*ff->ksize,count);
         message(tempstring, ERROR);
         fclose(fp);
         return;
      }
      if(total<1) fprintf(stderr, "Warning: darkening filter\n");
      if(total>1) fprintf(stderr, "Warning: lightening filter\n");
  }
  fclose(fp);

  edge = (ff->ksize-1)/2;
  array<uint> buf(x2-x1+ff->ksize, 2*ff->ksize);    // Temporary storage
  if(!buf.allocated){ message(g.nomemory); return; }

  if(ff->entireimage) progress_bar_open(www, scrollbar);
  bufrow = 1;
  savebufrow = bufrow + edge;
  for(j=y1+edge; j<y2-1; j++)
  {  if(ff->entireimage) progress_bar_update(scrollbar, 100*(j-edge)/(y2-y1-2*edge));
     check_event();           // check for event, filter could be big
     if(keyhit()) if(getcharacter()==27) break;
     if(g.getout) break;
     for(i=x1+edge; i<x2-edge-1; i++)
     {   v = vstart;   
         rsum = rstart;
         gsum = gstart;
         bsum = bstart;
         for(l=j-edge; l<=j+edge; l++)
         for(k=i-edge; k<=i+edge; k++)
         {    pix = (int)readpixelonimage(k,l,bpp,ii);
              if(ii>=0 && z[ii].colortype==GRAY) gray=1; else gray=0;   
              if(gray)
              {   v += cint(filterdata[l-j+edge][k-i+edge] * (double)pix);
              }else
              {   valuetoRGB(pix,rr,gg,bb,bpp);
                  rsum += cint(filterdata[l-j+edge][k-i+edge] * (double)rr);
                  gsum += cint(filterdata[l-j+edge][k-i+edge] * (double)gg);
                  bsum += cint(filterdata[l-j+edge][k-i+edge] * (double)bb);
               }
         }
         if(gray)
             v = max(0, min((int)g.maxvalue[bpp], v));
         else
         { 
             rsum = max(0, min(g.maxred[bpp], rsum));
             gsum = max(0, min(g.maxgreen[bpp], gsum));
             bsum = max(0, min(g.maxblue[bpp], bsum));
             v = RGBvalue(rsum,gsum,bsum,bpp);
         }
         if(ii>=0) z[ii].hit=1; 
         buf.p[savebufrow][i-x1] = v;
     }
     if(j >= y1+edge)          
     {  
        for(i=x1; i<x2; i++) 
        if(g.selected_is_square || inside_irregular_region(i,j))
        {   v = buf.p[bufrow][i-x1];
            pix = readpixelonimage(i,j-edge,bpp,ii);
            if(ii>=0 && z[ii].colortype==GRAY) gray=1; else gray=0;   
            if(gray)
            {  if(ff->sharpfac<100) v = (int)(shold*pix + shnew*v);
               if(v<0) v=0;
               if(v>(int)g.maxvalue[bpp]) v=(int)g.maxvalue[bpp];
            }else
            {  if(ff->sharpfac<100)
               {   valuetoRGB(v,rr,gg,bb,bpp);
                   valuetoRGB(pix,rr2,gg2,bb2,bpp);
                   rr = (int)(rr*shnew + rr2*shold); 
                   gg = (int)(gg*shnew + gg2*shold); 
                   bb = (int)(bb*shnew + bb2*shold); 
                   if(rr<0) rr=0;
                   if(gg<0) gg=0;
                   if(bb<0) bb=0;
                   if(rr>g.maxred[bpp]  ) rr=g.maxred[bpp];
                   if(gg>g.maxgreen[bpp]) gg=g.maxgreen[bpp];
                   if(bb>g.maxblue[bpp] ) bb=g.maxblue[bpp];                
                   v = RGBvalue(rr,gg,bb,bpp);
               }
            }
            setpixelonimage(i, bufrow_to_draw+y1, v, SET, bpp, -1, noscreen,
                z[ii].win, 0, ii, 0);
            buf.p[bufrow][i-x1] = 0;
        }
        bufrow_to_draw++;
     }
     bufrow++;   
     if(bufrow >= ff->ksize) bufrow = 0; 
     savebufrow++;   
     if(savebufrow >= ff->ksize) savebufrow = 0; 
  }
  if(ff->entireimage) progress_bar_close(www);

   //// Re-convert any color images touched if in 8-bpp screen mode
  for(k=0; k<g.image_count; k++) 
     if(z[k].hit){ rebuild_display(k); redraw(k); }
  redrawscreen();               // Redraw main screen
  delete[] filterdata;
  delete[] filterdata_1d;
}


//-------------------------------------------------------------------------//
// filter_engrave                                                          //
//-------------------------------------------------------------------------//
void filter_engrave(filter *ff)
{
  // need to optimize this, code is still crude
  // Can iterate over thresh for better effect but this makes it slow
  static int d = 2;
  int increment=4;
  int i,i2,j,k,l,v,v1,v2,x1,x2,y1,y2;
  int ino = ff->ino;
  int xsize = z[ino].xsize;
  int ysize = z[ino].ysize;
  int bpp = z[ino].bpp;
  int f = z[ino].cf;
  int b = g.off[bpp];
  x1 = ff->x1;  x2 = ff->x2;
  y1 = ff->y1;  y2 = ff->y2;
 
  array<int> cell(xsize, ysize);
  if(!cell.allocated){ message(g.nomemory); return; }

  switch(ff->ksize)
  {   case 3: d=1;  increment=64; break; 
      case 5: d=2;  increment=11; break; 
      case 9: d=3;  increment= 8; break; 
      case 15: d=4; increment= 4; break; 
  }

  for(j=0; j<ysize; j++)
  for(i=0, i2=0; i<xsize; i++,i2+=b)
      cell.p[j][i] = 127;
 
  for(j=d; j<ysize-d-1; j++)
  for(i=d; i<xsize-d-1; i++)
  {   v1 = v2 = 0;
      for(l=j-d; l<j+d; l++)
      for(k=i-d; k<i+d; k++)
         if(pixelat(z[ino].image[f][l]+k, bpp) > (uint)ff->ithresh) v1+=increment; else v1-=increment;
      for(l=j+1; l<=j+d; l++)
      for(k=i-d; k<=i+d; k++)
         if(pixelat(z[ino].image[f][l]+k, bpp) > (uint)ff->ithresh) v2+=increment; else v2-=increment;
      cell.p[j][i] += abs(v2-v1);

      v1 = v2 = 0;
      for(l=j-d; l<j+d; l++)
      for(k=i-d; k<i; k++)
         if(pixelat(z[ino].image[f][l]+k, bpp) > (uint)ff->ithresh) v1+=increment; else v1-=increment;
      for(l=j-d; l<j+d; l++)
      for(k=i+1; k<i+d; k++)
         if(pixelat(z[ino].image[f][l]+k, bpp) > (uint)ff->ithresh) v2+=increment; else v2-=increment;
      cell.p[j][i] += abs(v2-v1);
      switch(d)
      {  case 1:  cell.p[j][i] = 255 - cell.p[j][i] + 192; break;
         case 2:  cell.p[j][i] = 255 - cell.p[j][i] + 110; break;
         case 3:  cell.p[j][i] = 255 - cell.p[j][i] + 168; break;
         case 4:  cell.p[j][i] = 255 - cell.p[j][i] + 143; break;
      }
  }
  for(j=0; j<ysize; j++)
  for(i=0, i2=0; i<xsize; i++,i2+=b)
  {   v = cell.p[j][i];
      v = max(0, min(255,v));
      putpixelbytes(z[ino].image[f][j]+i2, v, 1, bpp);
  }
  repair(ino);
  redraw(ino);

}

//-------------------------------------------------------------------------//
// multiplicative sobel                                                    //
//-------------------------------------------------------------------------//
void mult_sobel(filter *ff)
{
  Widget www, scrollbar;
  int ino = ff->ino;
  int d = ff->ksize;
  int i,i2,j,k2,k,l,v;
  int xsize = z[ino].xsize;
  int ysize = z[ino].ysize;
  int bpp = z[ino].bpp;
  int f = z[ino].cf;
  int b = g.off[bpp];
  int aa,bb;
  int maxpix = (int)g.maxvalue[bpp];
  double quotient=1.0;

  array<int> cell(xsize, ysize);
  if(!cell.allocated){ message(g.nomemory); return; }
  if(ff->want_progress_bar) progress_bar_open(www, scrollbar);
 
  for(j=0; j<ysize; j++)
  for(i=0; i<xsize; i++)
      cell.p[j][i] = maxpix/2;
 
  for(j=d; j<ysize-d-1; j++)
  {  if(ff->want_progress_bar) 
         progress_bar_update(scrollbar, 100*(j-d)/ysize);
     if(g.getout){ g.getout=0; break; }
     for(i=d,i2=b*d; i<xsize-d-1; i++,i2+=b)
     {   for(l=-d; l<=d; l++)
         for(k=-d,k2=-d*b; k<=d; k++,k2+=b)
         {  aa = pixelat(z[ino].image[f][j+l]+i2+k2, bpp);
            bb = pixelat(z[ino].image[f][j-l]+i2-k2, bpp);
            if(aa>bb) aa*=2;
            if(bb>aa) bb*=2;
            if(bb) quotient = fabs((double)aa / (double)bb);
            cell.p[j][i] = cint(cell.p[j][i] * quotient);
         }  
     }
  }
  for(j=0; j<ysize; j++)
  for(i=0, i2=0; i<xsize; i++,i2+=b)
  {   v = cell.p[j][i];
      v = max(0, min(maxpix, v));
      putpixelbytes(z[ino].image[f][j]+i2, v, 1, bpp);
  }
  if(ff->want_progress_bar) progress_bar_close(www);
  repair(ino);
  redraw(ino);
}


//-------------------------------------------------------------------------//
// filter_diff                                                             //
//-------------------------------------------------------------------------//
void filter_diff(filter *ff)
{
   difference_filter(ff->ino, ff->diffsize, ff->sharpfac);
   repair(ff->ino);  
} 



//--------------------------------------------------------------------------//
//  filter_sharpen_edges                                                    //
//--------------------------------------------------------------------------//
void filter_sharpen_edges(filter *ff)
{
  Widget www, scrollbar;
  int ino = ff->ino;
  int d = ff->ksize;
  int i,i2,j,k2,k,l;
  int xsize = z[ino].xsize;
  int ysize = z[ino].ysize;
  int bpp = z[ino].bpp;
  int f = z[ino].cf;
  int b = g.off[bpp];
  int aa,pix,rr,gg,bb,rr0,gg0,bb0;
  int x1 = ff->x1; 
  int x2 = ff->x2;
  int y1 = ff->y1;  
  int y2 = ff->y2;
  int thresh = ff->ithresh;

  array<int> line(xsize, ysize);
  if(ff->want_progress_bar) progress_bar_open(www, scrollbar);
  for(j=y1+d; j<y2-d-1; j++)
  {  if(ff->want_progress_bar) 
         progress_bar_update(scrollbar, 100*(j-d)/ysize);
     if(g.getout){ g.getout=0; break; }
     for(i=x1+d,i2=b*d; i<x2-d-1; i++,i2+=b)
     {   
         pix = pixelat(z[ino].image[f][j]+i2, bpp);
         valuetoRGB(pix, rr0, gg0, bb0, bpp);
         for(l=-d; l<=d; l++)
         for(k=-d,k2=-d*b; k<=d; k++,k2+=b)
         {    aa = pixelat(z[ino].image[f][j+l]+i2+k2, bpp);
              valuetoRGB(aa, rr, gg, bb, bpp);
              if(abs(rr-rr0)>thresh|| abs(gg-gg0)>thresh || abs(gg-gg0)>thresh)
              {
                 rr0--;
                 gg0--;
                 bb0--;
                 
              }
         }  
         rr0 = max(0,min(rr0,255));
         gg0 = max(0,min(gg0,255));
         bb0 = max(0,min(bb0,255));
         aa = RGBvalue(rr0,gg0,bb0,bpp);
         line.p[j][i] = aa;
     }
  }
  for(j=y1+d; j<y2-d-1; j++)
  for(i=x1+d,i2=b*d; i<x2-d-1; i++,i2+=b)
  {    pix = line.p[j][i];
       putpixelbytes(z[ino].image[f][j]+i2, pix, 1, bpp);
  }
  if(ff->want_progress_bar) progress_bar_close(www);
  repair(ino);
  redraw(ino);
}
