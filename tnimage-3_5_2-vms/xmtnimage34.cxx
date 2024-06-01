//--------------------------------------------------------------------------//
// xmtnimage34.cc                                                           //
// Latest revision: 10-11-2003                                              //
// Copyright (C) 2003 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
// split, combine, flip, create image                                       //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
extern PrinterStruct printer;
char tempname[FILENAMELENGTH] = "Untitled";
int hshift=0, vshift=0;
uint ofcolor, obcolor;
static int composite_spacing=0;
static char composite_image_list[1024]="";
static int composite_columns=1;
static int composite_want_specify_position=0;
static int in_grab_window=0;

//-------------------------------------------------------------------------//
// setfcolor                                                               //
//-------------------------------------------------------------------------//
void setfcolor(int noofargs, char **arg)
{ 
   static int hit=0;
   if(!hit) ofcolor = g.fcolor;
   hit = 1;
   int rr,gg,bb;
   rr=gg=bb=0;
   if(noofargs) 
   {   if(noofargs >=1) rr=atoi(arg[1]);
       if(noofargs >=2) gg=atoi(arg[2]);
       if(noofargs >=3) bb=atoi(arg[3]);
       g.fcolor = RGBvalue(rr,gg,bb,g.bitsperpixel);
   }else if(between(ci, 1, g.image_count) && z[ci].colortype==GRAY)
       getinteger("Foreground grayscale value", (int*)&ofcolor, 0, (int)g.maxvalue[z[ci].bpp], 
                   setgrayfcolor_part2, null, NULL, 0);
   else
       getcolor("Foreground color", &ofcolor, g.bitsperpixel, 0, setfcolor_part2, NULL); 
}   


//-------------------------------------------------------------------------//
// setfcolor_part2                                                         //
//-------------------------------------------------------------------------//
void setfcolor_part2(clickboxinfo *c)
{  c=c;
   g.fcolor = c->answer;
   ofcolor = g.fcolor;
   g.line.color = g.fcolor;
   if(g.bitsperpixel==8) 
   {   g.fc.red   = g.palette[g.fcolor].red; 
       g.fc.green = g.palette[g.fcolor].green; 
       g.fc.blue  = g.palette[g.fcolor].blue; 
   }
}


//-------------------------------------------------------------------------//
// setgrayfcolor_part2                                                     //
//-------------------------------------------------------------------------//
void setgrayfcolor_part2(clickboxinfo *c)
{  c=c;
   g.fcolor = c->answer;
   ofcolor = g.fcolor;
   if(z[ci].colortype==GRAY)
       g.fcolor = RGBvalue(g.fcolor, g.fcolor, g.fcolor, g.bitsperpixel);
   else
       g.fcolor = convertpixel(g.fcolor, z[ci].bpp, g.bitsperpixel, 1);
   g.line.color = g.fcolor;
   if(g.bitsperpixel==8) 
   {   g.fc.red   = g.palette[g.fcolor].red; 
       g.fc.green = g.palette[g.fcolor].green; 
       g.fc.blue  = g.palette[g.fcolor].blue; 
   }
}


//-------------------------------------------------------------------------//
// setbcolor                                                               //
//-------------------------------------------------------------------------//
void setbcolor(int noofargs, char **arg)
{ 
   int rr,gg,bb;
   static int hit=0;
   if(!hit) obcolor = g.bcolor;
   hit = 1;
   rr=gg=bb=0;
   if(noofargs) 
   {   if(noofargs >=1) rr=atoi(arg[1]);
       if(noofargs >=2) gg=atoi(arg[2]);
       if(noofargs >=3) bb=atoi(arg[3]);
       g.bcolor = RGBvalue(rr,gg,bb,g.bitsperpixel);
   }else if(between(ci, 1, g.image_count) && z[ci].colortype==GRAY)
       getinteger("Background grayscale value", (int*)&obcolor, 0, (int)g.maxvalue[z[ci].bpp], 
                   setgraybcolor_part2, null, NULL, 0);
   else
       getcolor("Background color", &obcolor, g.bitsperpixel, 0, setbcolor_part2, NULL); 

}   


//-------------------------------------------------------------------------//
// setbcolor_part2                                                         //
//-------------------------------------------------------------------------//
void setbcolor_part2(clickboxinfo *c)
{ 
   g.bcolor = c->answer;
   obcolor = g.bcolor;
   if(g.bitsperpixel==8) 
   {   g.bc.red   = g.palette[g.bcolor].red; 
       g.bc.green = g.palette[g.bcolor].green; 
       g.bc.blue  = g.palette[g.bcolor].blue; 
   }
}


//-------------------------------------------------------------------------//
// setgraybcolor_part2                                                     //
//-------------------------------------------------------------------------//
void setgraybcolor_part2(clickboxinfo *c)
{   
   c=c;
   g.bcolor = c->answer;
   obcolor = g.bcolor;
   if(z[ci].colortype==GRAY)
       g.bcolor = RGBvalue(g.bcolor, g.bcolor, g.bcolor, g.bitsperpixel);
   else
       g.bcolor = convertpixel(g.bcolor, z[ci].bpp, g.bitsperpixel, 1);
   g.line.color = g.bcolor;
   if(g.bitsperpixel==8) 
   {   g.bc.red   = g.palette[g.bcolor].red; 
       g.bc.green = g.palette[g.bcolor].green; 
       g.bc.blue  = g.palette[g.bcolor].blue; 
   }
}


//-------------------------------------------------------------------------//
// flip_horiz                                                              //
//-------------------------------------------------------------------------//
void flip_horiz(void)
{
    int h,i2,i3,j,k,bpp,ii;
    uint value;
    uint *buffer;
    for(k=0; k<g.image_count; k++) z[k].hit=0;

    buffer=new uint[6*(g.selected_lrx-g.selected_ulx+2)];
    if(buffer==NULL){ message(g.nomemory,ERROR); return; }

    drawselectbox(OFF);
    if(g.selectedimage>-1 && ci >=0) // Use faster method if region is all same image
    {   bpp = z[ci].bpp;
        for(j=0;j<z[ci].ysize;j++)
        {   for(h=0,i2=0; h<z[ci].xsize; h++,i2+=g.off[bpp])
               buffer[h] = pixelat(z[ci].image[z[ci].cf][j]+i2,bpp);
            i3 = (z[ci].xsize-1)*g.off[bpp];
            for(h=0,i2=0; h<z[ci].xsize; h++,i2+=g.off[bpp])
            {  value=buffer[h];
               putpixelbytes(z[ci].image[z[ci].cf][j]+i3-i2,value,1,bpp);
            }      
        }
        z[ci].hit=1;
    }else
    {   for(j=g.selected_uly;j<g.selected_lry;j++)
        {   for(h=g.selected_ulx,i2=0;h<g.selected_lrx;h++,i2+=4)
            {  value = readpixelonimage(h,j,bpp,ii);
               if(ii>=0){ if((g.bitsperpixel==8)&&(bpp!=8)) z[ii].hit=1;}
               if(ii>=0 && (!g.wantr || !g.wantg || !g.wantb)) z[ii].hit=1;
               buffer[h-g.selected_ulx]=value;               
            }
            for(h=g.selected_ulx,i2=0;h<g.selected_lrx;h++,i2+=4)
            {  if(!g.selected_is_square && !inside_irregular_region(h,j)) continue;      
               bpp = whatbpp(h,j);
               value = buffer[h-g.selected_ulx];
               setpixelonimage(g.selected_lrx-(h-g.selected_ulx),j,value,SET,bpp);  
            }      
        }       
    }
    delete[] buffer;
    for(k=0; k<g.image_count; k++) 
       if(z[k].hit){ rebuild_display(k); redraw(k);  }
    return;
}




//-------------------------------------------------------------------------//
// flip_vert                                                               //
//-------------------------------------------------------------------------//
void flip_vert(void)
{
    int i,i2,j3,j,k,bpp,ii;
    uint value;
    uint *buffer;
    drawselectbox(OFF);
    for(k=0; k<g.image_count; k++) z[k].hit=0;
    buffer=new uint[6*(g.selected_lry-g.selected_uly+2)];
    if(buffer==NULL){ message(g.nomemory,ERROR); return; }
    if(g.selectedimage>-1 && ci >=0) // Use faster method if region is all same image
    {    bpp = z[ci].bpp;
         j3 = z[ci].ysize-1;
         for(i=0,i2=0;i<z[ci].xsize;i++,i2+=g.off[bpp])
         {   for(j=0;j<z[ci].ysize;j++)
                 buffer[j] = pixelat(z[ci].image[z[ci].cf][j]+i2,bpp);
             for(j=0;j<z[ci].ysize;j++)
             {  value = buffer[j];
                putpixelbytes(z[ci].image[z[ci].cf][j3-j]+i2,value,1,bpp);
             }      
         }
         z[ci].hit=1;
    }else
    {    for(i=g.selected_ulx;i<g.selected_lrx;i++)
         {   for(j=g.selected_uly;j<g.selected_lry;j++)
             {  value = readpixelonimage(i,j,bpp,ii);
                if(ii>=0){ if((g.bitsperpixel==8)&&(bpp!=8)) z[ii].hit=1;}
                if(ii>=0 && (!g.wantr || !g.wantg || !g.wantb)) z[ii].hit=1;
                buffer[j-g.selected_uly]=value;
             }
             for(j=g.selected_uly;j<g.selected_lry;j++)
             {  if(!g.selected_is_square && !inside_irregular_region(i,j)) continue;      
                bpp = whatbpp(i,j);
                value = buffer[j-g.selected_uly];
                setpixelonimage(i,g.selected_lry-(j-g.selected_uly),value,SET,bpp);  
             }       
         }
    }
    delete[] buffer;
    for(k=0; k<g.image_count; k++) 
        if(z[k].hit){ rebuild_display(k); redraw(k); }
}



//-------------------------------------------------------------------------//
// image_border - draw a border around the image                           //
//-------------------------------------------------------------------------//
void image_border(void)
{  
  if(ci<0) return;
  box(z[ci].xpos+1, z[ci].ypos, z[ci].xpos+z[ci].xscreensize-1, 
      z[ci].ypos+z[ci].yscreensize-1,  g.imode, &g.line);
  rebuild_display(ci);
  z[ci].touched=1;
  switchto(ci);             
  redraw(ci);
}
 
 

//-------------------------------------------------------------------------//
// paint region                                                            //
//-------------------------------------------------------------------------//
void paint(int color)
{   
  int status=OK, i, j, ok=1, xx1=g.get_x1, xx2=g.get_x2, yy1=g.get_y1, yy2=g.get_y2;
  message("Select area(s) to paint\nclick cancel button when finished");
  if(status == CANCEL) return;
  g.getout=0;
  int square = g.selected_is_square;
  while(!g.getout)
  {   getbox(xx1, yy1, xx2, yy2);
      if(g.getout) break;
      for(j=yy1;j<=yy2;j++)
      for(i=xx1;i<=xx2;i++)
      {   ok = 0;
          if(square) ok=1;
          else if(inside_irregular_region(i,j)) ok=1;
          if(ok) setpixelonimage(i,j,color,SET,0,-1,0,0,1);
      }
  }
}        


//-------------------------------------------------------------------------//
// show od table                                                           //
//-------------------------------------------------------------------------//
void showodtable(void)
{  
    int k,helptopic;
    int *tt;               // Can't plot grayscale directly because it is short int
    double *xdata;
    char **ytitle;

    char title[FILENAMELENGTH] = "Gamma_table";
    ytitle = new char*[1];
    ytitle[0] = new char[256];
    strcpy(ytitle[0],"Optical Density");      

    xdata = new double[256];
    tt = new int[256];     
    if(tt==NULL){  message(g.nomemory,ERROR); return; } 
    for(k=0;k<256;k++) 
    {  if(ci>=0) tt[k] = z[ci].gamma[k];
       else      tt[k] = k;   
       xdata[k] = (double)k;
    }
    helptopic=17;
    plot(title, (char*)"Pixel Value", ytitle, xdata, tt, 256, 1, MEASURE,
         0, 256, NULL, null, null, null, null, null, helptopic);
    if(ci>=0) for(k=0;k<256;k++) z[ci].gamma[k]=(short int)tt[k];
 
    delete[] ytitle[0];
    delete[] ytitle;
    delete[] tt;
    delete[] xdata;
}    
 

//-------------------------------------------------------------------------//
// erase background                                                        //
//-------------------------------------------------------------------------//
void erasebackground(void)
{  int j;
   for(j=0;j<z[0].ysize;j++) 
      putpixelbytes(z[0].image[0][j], g.bcolor, z[0].xsize, g.bitsperpixel, 1);
   copybackground(1,1,z[0].xsize-1,z[0].ysize-1,-1);
   repair(0);
   g.background_touched=0;
}



//-------------------------------------------------------------------------//
// createimage - allocate space for a new image at current bits/pixel.     //
//-------------------------------------------------------------------------//
int createimage(int noofargs, char **arg)
{ 
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return(NOMEM); } 
   static Dialog *dialog;
   int j,k,status=OK;
   if(ci>=0) g.create_cimage = ci;
   if(noofargs==0) 
   {
      dialog = new Dialog;
      if(dialog==NULL){ message(g.nomemory); return(NOMEM); }
      strcpy(dialog->title,"Create/Resize Image");
      strcpy(dialog->radio[0][0],"Method");             
      strcpy(dialog->radio[0][1],"Use mouse");   
      strcpy(dialog->radio[0][2],"Fixed size");  
      strcpy(dialog->radio[0][3],"Duplicate image");   
      strcpy(dialog->radio[0][4],"Panel->multiframe");   
      strcpy(dialog->radio[0][5],"Multiframe->panel");   
      strcpy(dialog->radio[0][6],"Images->multiframe");   
      strcpy(dialog->radio[0][7],"Multiframe->images");   
      strcpy(dialog->radio[0][8],"Add frame to image");   
      strcpy(dialog->radio[0][9],"Grab from screen");   
      strcpy(dialog->radio[0][10],"Composite images");   
      strcpy(dialog->radio[0][11],"Resize image window");   
      strcpy(dialog->radio[0][12],"Chop image->4 pieces");   
      strcpy(dialog->radio[0][13],"Create Subimage");   
      strcpy(dialog->radio[0][14],"Spot list->panel");   
      strcpy(dialog->radio[0][15],"Selected area");   

      strcpy(dialog->radio[1][0],"Add border");             
      strcpy(dialog->radio[1][1],"No");   
      strcpy(dialog->radio[1][2],"Yes");  
      
      dialog->radioset[0] = g.create_method;
      dialog->radioset[1] = g.create_border+1;
    
      strcpy(dialog->boxes[0],"Parameters");
      strcpy(dialog->boxes[1],"Source image");
      strcpy(dialog->boxes[2],"X size");
      strcpy(dialog->boxes[3],"Y size");
      strcpy(dialog->boxes[4],"X position");
      strcpy(dialog->boxes[5],"Y position");
      strcpy(dialog->boxes[6],"New title");
      strcpy(dialog->boxes[7],"No. of frames");
      strcpy(dialog->boxes[8],"Separate window");
      strcpy(dialog->boxes[9],"Manually find frames");
      strcpy(dialog->boxes[10],"Align Images");
      strcpy(dialog->boxes[11],"Columns");
      strcpy(dialog->boxes[12],"X upper left");
      strcpy(dialog->boxes[13],"Y upper left");
      strcpy(dialog->boxes[14],"X lower right");
      strcpy(dialog->boxes[15],"Y lower right");
      strcpy(dialog->boxes[16],"X spacing");
      strcpy(dialog->boxes[17],"Y spacing");
      strcpy(dialog->boxes[18],"Fixed spot xsize");
      strcpy(dialog->boxes[19],"Fixed spot ysize");
      strcpy(dialog->boxes[20],"X spot margin");
      strcpy(dialog->boxes[21],"Y spot margin");
   
      dialog->boxtype[0]=LABEL;
      dialog->boxtype[1]=INTCLICKBOX;
      dialog->boxtype[2]=STRING;
      dialog->boxtype[3]=STRING;
      dialog->boxtype[4]=STRING;
      dialog->boxtype[5]=STRING;
      dialog->boxtype[6]=STRING;
      dialog->boxtype[7]=INTCLICKBOX;
      dialog->boxtype[8]=TOGGLE;
      dialog->boxtype[9]=TOGGLE;
      dialog->boxtype[10]=TOGGLE;
      dialog->boxtype[11]=INTCLICKBOX;
      dialog->boxtype[12]=STRING;
      dialog->boxtype[13]=STRING;
      dialog->boxtype[14]=STRING;
      dialog->boxtype[15]=STRING;
      dialog->boxtype[16]=STRING;
      dialog->boxtype[17]=STRING;
      dialog->boxtype[18]=TOGGLESTRING;
      dialog->boxtype[19]=TOGGLESTRING;
      dialog->boxtype[20]=STRING;
      dialog->boxtype[21]=STRING;
    
      dialog->boxmin[1]=1; dialog->boxmax[1]=g.image_count-1;
      dialog->boxmin[7]=1; dialog->boxmax[7]=1000;
      dialog->boxmin[11]=1; dialog->boxmax[11]=100;
  
      sprintf(dialog->answer[1][0], "%d", g.create_cimage);
      sprintf(dialog->answer[2][0], "%d", g.create_xsize);
      sprintf(dialog->answer[3][0], "%d", g.create_ysize);
      sprintf(dialog->answer[4][0], "%d", g.create_xpos);
      sprintf(dialog->answer[5][0], "%d", g.create_ypos);
      strcpy( dialog->answer[6][0], tempname);
      sprintf(dialog->answer[7][0], "%d", g.create_frames);
      dialog->boxset[8] = g.create_shell;
      dialog->boxset[9] = g.create_find_edge;
      dialog->boxset[10] = g.create_panel;
      sprintf(dialog->answer[11][0], "%d", g.create_cols);
      sprintf(dialog->answer[12][0], "%d", g.create_ulx);
      sprintf(dialog->answer[13][0], "%d", g.create_uly);
      sprintf(dialog->answer[14][0], "%d", g.create_lrx);
      sprintf(dialog->answer[15][0], "%d", g.create_lry);
      sprintf(dialog->answer[16][0], "%d", g.create_xspacing);
      sprintf(dialog->answer[17][0], "%d", g.create_yspacing);
      sprintf(dialog->answer[18][1], "%d", g.create_fixed_xsize);
      dialog->boxset[18] = g.create_want_fixed_xsize;
      sprintf(dialog->answer[19][1], "%d", g.create_fixed_ysize);
      dialog->boxset[19] = g.create_want_fixed_ysize;
      sprintf(dialog->answer[20][0], "%d", g.create_xmargin);
      sprintf(dialog->answer[21][0], "%d", g.create_ymargin);
 
      dialog->radiono[0] = 16;
      dialog->radiono[1] = 3;
      dialog->radiono[2] = 0;
      for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
      dialog->noofradios = 2;
      dialog->noofboxes = 22; 
      dialog->helptopic = 31;  
      dialog->want_changecicb = 1;
      dialog->f1 = createimagecheck;
      dialog->f2 = null;
      dialog->f3 = null;
      dialog->f4 = null;
      dialog->f5 = null;
      dialog->f6 = null;
      dialog->f7 = null;
      dialog->f8 = null;
      dialog->f9 = null;
      dialog->width = 0;  // calculate automatically
      dialog->height = 0; // calculate automatically
      dialog->transient = 1;
      dialog->wantraise = 0;
      dialog->radiousexy = 0;
      dialog->boxusexy = 0;
      strcpy(dialog->path,".");
      dialog->message[0] = 0;      
      dialog->param[0] = ci;
      dialog->busy = 0;
      dialogbox(dialog);
   }else
   {  if(noofargs>=1) g.create_cimage = atoi(arg[1]);
      if(noofargs>=2) g.create_method = atoi(arg[2]);
      if(noofargs>=3) g.create_border = atoi(arg[3]);
      if(noofargs>=4) g.create_xsize  = atoi(arg[4]);
      if(noofargs>=5) g.create_ysize  = atoi(arg[5]);
      if(noofargs>=6) g.create_xpos   = atoi(arg[6]);
      if(noofargs>=7) g.create_ypos   = atoi(arg[7]);
      if(noofargs>=8) g.create_frames = atoi(arg[8]);
      if(noofargs>=9) g.create_shell  = atoi(arg[9]);
      if(noofargs>=10) g.create_find_edge = atoi(arg[10]);
      if(noofargs>=11) g.create_panel = atoi(arg[11]);
      if(noofargs>=12) g.create_cols  = atoi(arg[12]);
      if(noofargs>=13) g.create_ulx = atoi(arg[13]);
      if(noofargs>=14) g.create_uly = atoi(arg[14]);
      if(noofargs>=15) g.create_lrx = atoi(arg[15]);
      if(noofargs>=16) g.create_lry = atoi(arg[16]);
      if(noofargs>=17) g.create_xspacing = atoi(arg[17]);
      if(noofargs>=18) g.create_yspacing = atoi(arg[18]);
      if(noofargs>=19) g.create_xmargin = atoi(arg[19]);
      if(noofargs>=20) g.create_ymargin = atoi(arg[20]);
      if(noofargs>=21) g.create_fixed_xsize = atoi(arg[21]);
      if(noofargs>=22) g.create_fixed_ysize = atoi(arg[22]);
      status = create_new_image(g.create_method, g.create_xpos, 
               g.create_ypos, g.create_xsize, g.create_ysize, 
               g.create_frames, g.create_shell, g.create_border, 
               g.create_find_edge, g.create_cols, g.create_panel,
               g.create_xspacing, g.create_yspacing,
               g.create_xmargin, g.create_ymargin,
               g.create_want_fixed_xsize, g.create_fixed_xsize,
               g.create_want_fixed_ysize, g.create_fixed_ysize, ci);
      status_error_message(status);
   }   
   return status;
}


//--------------------------------------------------------------------------//
//  createimagecheck                                                        //
//--------------------------------------------------------------------------//
void createimagecheck(dialoginfo *a, int radio, int box, int boxbutton)
{
   static int oxsize=0, oysize=0, oci=ci;
   int k, status=OK, newino,ox1,ox2,oy1,oy2;
   radio=radio; box=box; boxbutton=boxbutton;
   ox1 = g.selected_ulx;
   ox2 = g.selected_lrx;
   oy1 = g.selected_uly;
   oy2 = g.selected_lry;


   g.create_method = a->radioset[0];
   if(a->radioset[1]==2) g.create_border = 1; else g.create_border = 0;

   g.create_cimage = atoi(a->answer[1][0]);  
   if(ci != oci)
   {   g.create_cimage = ci;
       g.create_xsize  = z[ci].xsize;
       g.create_ysize  = z[ci].ysize;
                   itoa(g.create_xsize, a->answer[2][0], 10);
                   itoa(g.create_ysize, a->answer[3][0], 10);
                   itoa(g.create_frames, a->answer[7][0], 10);
       oci=ci;
       oxsize=oysize=0;
   }else
   {  
       g.create_xsize  = atoi(a->answer[2][0]);
       g.create_ysize  = atoi(a->answer[3][0]);
   }

   g.create_xpos   = atoi(a->answer[4][0]);
   g.create_ypos   = atoi(a->answer[5][0]);
   strcpy(tempname, a->answer[6][0]);
   g.create_frames = atoi(a->answer[7][0]);
   g.create_shell  = a->boxset[8];
   g.create_find_edge = a->boxset[9];  
   g.create_frames = max(1, g.create_frames);
   g.create_panel  = a->boxset[10];  
   g.create_cols   = max(1,atoi(a->answer[11][0]));
   g.create_ulx    = atoi(a->answer[12][0]);
   g.create_uly    = atoi(a->answer[13][0]);
   g.create_lrx    = atoi(a->answer[14][0]);
   g.create_lry    = atoi(a->answer[15][0]);
   g.create_xspacing         = atoi(a->answer[16][0]);
   g.create_yspacing         = atoi(a->answer[17][0]);
   g.create_want_fixed_xsize = a->boxset[18];
   g.create_fixed_xsize      = atoi(a->answer[18][1]);  
   g.create_want_fixed_ysize = a->boxset[19];
   g.create_fixed_ysize      = atoi(a->answer[19][1]);  
   g.create_xmargin          = atoi(a->answer[20][0]);  
   g.create_ymargin          = atoi(a->answer[21][0]);  


   // If adding more boxes, increase this array and add True or False
   // to each case below.
  
   int sens[22]  = { 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; 
   //                0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21

   switch(a->radioset[0])
   {   case 1: sens[1] = False; sens[4]  = True;  sens[5]  = True; sens[7]  = True; sens[8] = True; 
               break;
       case 2: sens[1] = False; sens[2]  = True;  sens[3]  = True; sens[4]  = True; sens[5] = True; 
               sens[7] = True;  sens[8]  = True; 
               break;
       case 3: sens[4] = True;  sens[5]  = True;  sens[8]  = True; 
               break;
       case 4: 
       case 11:  
               if(a->radioset[0]==4)    
               {    sens[2] = True;  sens[3]  = True;  sens[4]  = True; sens[5]  = True; 
                    sens[7] = True;  sens[8]  = True;  sens[9]  = True; sens[10] = True;
                    sens[13] = True; sens[16] = True;  sens[17] = True;
               }else
               {    sens[2]  = True;  sens[3]  = True;  sens[6]  = False; sens[8] = True; 
               }
               if(g.create_xsize != oxsize || g.create_ysize != oysize)
               {   
                   oxsize = g.create_xsize;   // Must be first to stop endless
                   oysize = g.create_ysize;   // XmValueChangedCallbacks
                   itoa(g.create_xsize, a->answer[2][0], 10);
                   itoa(g.create_ysize, a->answer[3][0], 10);
                   itoa(g.create_frames, a->answer[7][0], 10);
                  // set_widget_value(a->boxwidget[2][0], g.create_xsize);
                  // set_widget_value(a->boxwidget[3][0], g.create_ysize);
                  // set_widget_value(a->boxwidget[7][0], g.create_frames);
               }
               break;
       case 5: sens[4]  = True;  sens[5]  = True;  sens[8]  = True;  sens[11] = True;
               sens[12] = True;  sens[13] = True;  sens[16] = True;  sens[17] = True;
               g.create_rows = int(((double)z[ci].frames / (double)g.create_cols) + 0.9999); 
               g.create_rows = max(1, g.create_rows);
               g.create_frames = z[ci].frames;
               break;
       case 6: sens[1]  = False; sens[4] = True; sens[5] = True; sens[7] = True; 
               sens[8] = True; 
               break;
       case 7: sens[4]  = True;  sens[5]  = True; sens[6]  = False; sens[8] = True; 
               sens[10] = True;  sens[16] = True; sens[17] = True;
               if(g.create_panel) sens[11] = True; 
               g.create_rows  = int(((double)z[ci].frames / (double)g.create_cols) + 0.9999); 
               g.create_rows = max(1, g.create_rows);
               g.create_frames = z[ci].frames;
               break;
       case 8: sens[6]  = False; 
               break;
       case 9: sens[1]  = False; sens[4]  = True;  sens[5]  = True;  sens[8] = True; 
               break;
       case 10:sens[1]  = False; sens[2]  = False; sens[3]  = False; sens[4] = True; 
               sens[5]  = True;  sens[8]  = True; 
               break;
       case 12:sens[4]  = True;  sens[5]  = True;  sens[6]  = False; 
               sens[8]  = True; 
               break;
       case 13:sens[2]  = False; sens[3]  = False; sens[4]  = False; 
               sens[5]  = False; sens[7]  = True;  sens[8]  = True;  sens[12] = True;  
               sens[13] = True;  sens[14] = True;  sens[15] = True; 
               break;
       case 14:sens[2]  = True;  sens[3]  = True;  sens[4]  = True; 
               sens[5]  = True;  sens[7]  = True;  sens[8]  = True;  sens[11] = True; 
               sens[16] = True;  sens[17] = True;  sens[18] = True;  sens[19] = True; 
               sens[20] = True;  sens[21] = True; 
               break;
       case 15:sens[1] = False; sens[2]  = False;  sens[6]  = False; break;
   }
   for(k=18; k<20; k++) XtSetSensitive(a->boxwidget[k][1], sens[k]);
   for(k=1; k<22; k++) XtSetSensitive(a->boxwidget[k][0], sens[k]);

   if(radio != -2) return;   //// User clicked Ok or Enter
   //////////////////////

   drawselectbox(OFF);
   ////  Prevent switching to another image when selecting new area
   if(g.create_method != 4) g.want_switching=0;

   ////  This changes ci
   status = create_new_image(g.create_method, g.create_xpos, 
            g.create_ypos, g.create_xsize, g.create_ysize, 
            g.create_frames, g.create_shell, g.create_border,
            g.create_find_edge, g.create_cols, g.create_panel,
            g.create_xspacing, g.create_yspacing, 
            g.create_xmargin, g.create_ymargin, 
            g.create_want_fixed_xsize, g.create_fixed_xsize,
            g.create_want_fixed_ysize, g.create_fixed_ysize, g.create_cimage);
   newino = ci;
   g.want_switching = 1;
   ////  Can't just set xpos and ypos or it won't redraw
   if(newino>=0 && z[newino].shell) moveimage(newino,0,0);  
   if(g.create_method!=10)
   {   if(status==OK)
       {    create_image_initialize(newino, tempname);
            message("New image created");
       }else if(g.create_method != 9 && g.create_method != 11 && g.create_method != 12)      
            status_error_message(status);
   }
   if(g.selectedimage<=0) select_region(ox1,oy1,ox2,oy2);
   return;
}


//-------------------------------------------------------------------------//
// create_image_initialize                                                 //
//-------------------------------------------------------------------------//
void create_image_initialize(int ino, char *tempname)
{
   z[ino].touched = 1;
   memcpy(z[ino].palette, g.palette, 768);
   memcpy(z[ino].opalette, g.palette, 768);
   memcpy(z[ino].spalette, g.palette, 768);
   rebuild_display(ino);
   redraw(ino);
   switchto(ino);
   if(g.create_border) image_border();
   if(g.autoundo) backupimage(ino,0);
   if(tempname && strlen(tempname)) setimagetitle(ino, tempname);
}


//-------------------------------------------------------------------------//
// create_new_image                                                        //
//-------------------------------------------------------------------------//
int create_new_image(int method, int xpos, int ypos, int xsize,
   int ysize, int frames, int shell, int border, int find_edge, int cols, 
   int panel, int xspacing, int yspacing, int xmargin, int ymargin,
   int want_fixed_xsize, int fixed_xsize,
   int want_fixed_ysize, int fixed_ysize, int ino)
{
   int status = OK;
   if(method ==11 && ci==0){ message("This operation cannot be performed\non the background", ERROR); return ABORT; }
   // Don't resize or duplicate floating magnifier
   if(z[ci].floating) return ABORT;
   switch(method)
   {  case 1: status = create_image_with_mouse(xpos,ypos,frames,shell,border); break;
      case 2: status = create_fixed_size_image(xsize,ysize,xpos,ypos,frames,
                          shell,border); break;
      case 3: status = duplicate_image(ino,xpos,ypos,shell,border); break;
      case 4: status = create_multiframe_image(ino,xsize,ysize,xpos,ypos,
                          frames,shell,border,find_edge);break;
      case 5: status = create_panel(ino,xpos,ypos,shell,border,cols); break;
      case 6: status = combine_frames(xpos,ypos,shell,border); break;
      case 7: status = split_frames(xpos, ypos, ino,shell,border,cols,panel); break;
      case 8: status = add_frame(ino); break;
      case 9: status = grab_window(); break;
      case 10: status = composite_image(xpos,ypos,shell,border); break;
      case 11: resize_image(ino,xsize,ysize); status=NOIMAGES; break;
      case 12: status = chop_image(ino,xpos,ypos,shell,border); break;
      case 13: status = create_subimage(ino,xsize,ysize,frames,shell,border); break;
      case 14: status = create_spotlist_panel(ino,xsize,ysize,xpos,ypos,frames,shell,
                          border,xspacing,yspacing,xmargin,ymargin,
                          want_fixed_xsize,fixed_xsize,
                          want_fixed_ysize,fixed_ysize); break;
      case 15: status = create_image_from_selected(frames,shell,border); break;
      default: message("Unknown image creation option"); 
   }
   return status;
}


//-------------------------------------------------------------------------//
//  create_subimage                                                        //
//-------------------------------------------------------------------------//
int create_subimage(int ino, int xsize, int ysize, int frames, int shell, int border)
{
   //// Copy from preselected region in image
  xsize=xsize; ysize=ysize;
  int b,f,i,i2,i3,j,j2,x1,x2,y1,y2,ct=g.colortype,bpp,ixsize,iysize,status;
  if(!between(ino, 1, g.image_count)) return NOIMAGES;
  bpp = z[ino].bpp;
  b = g.off[bpp];
  ixsize = z[ino].xsize;
  iysize = z[ino].ysize;
  uint value;
  x1 = g.create_ulx;
  y1 = g.create_uly;
  x2 = g.create_lrx;
  y2 = g.create_lry;
  f = z[ino].cf;
  if(ino>=0) ct = z[ino].colortype;
  status = newimage("Image",0,0,x2-x1,y2-y1,bpp,ct,frames,shell,1,PERM,0,border,0);
  if(status==OK)
  {   for(f=0; f<frames; f++)
      for(j=y1,j2=0; j<y2; j++,j2++)
      for(i=x1,i2=x1*b,i3=0; i<x2; i++,i2+=b,i3+=b)
      {   if(!between(j,0,iysize-1) || !between(i,0,ixsize-1)) value=0;
          else value = pixelat(z[ino].image[f][j]+i2, bpp);
          putpixelbytes(z[ci].image[f][j2]+i3,value,1,bpp,1);
      }
  }
  return status;
}


//-------------------------------------------------------------------------//
// create_image_from_selected                                              //
//-------------------------------------------------------------------------//
int create_image_from_selected(int frames, int shell, int border)
{
   int f,i,iii,bbb,i2,j,j2,x,y,x1,x2,y1,y2,ct=g.colortype,ino,oino=-1,bpp,ibpp;
   int different=0,needconvert=0;
   uint value;
   x1 = g.ulx;
   x2 = g.lrx+1;
   y1 = g.uly;
   y2 = g.lry+1;
   ino = whichimage((x1+x2)/2, (y1+y2)/2, bpp);

   //// Find out if all pixels are from same image.
   //// If so, make the new image same depth and colortype.
   if(x2-x1<1 || y2-y1<1) return ABORT;
   for(y=y1;y<y2;y+=4)
   for(x=x1;x<x2;x+=4)
   {    iii = whichimage(x, y, bbb);
        if(iii!=ino || bbb!=bpp) different=1;
   }

   if(different)  bpp = g.bitsperpixel;
   if(bpp==8){ bpp=24; needconvert=1; }
   if(ino>=0) ct = z[ino].colortype;
   if(newimage("Image",x2,y2,x2-x1,y2-y1,bpp,ct,frames,shell,1,PERM,0,border,0)==OK)
   { 
       for(j=y1,j2=0;j<y2;j++,j2++)
       for(i=x1,i2=0;i<x2;i++,i2++)
       { 
           value = readpixelonimage(i, j, ibpp, ino);
           if(ibpp != bpp) value = convertpixel(value, ibpp, bpp, 1);
           if(ino!=oino){ switch_palette(ino); oino=ino; }
           for(f=0; f<frames; f++)
                putpixelbytes(z[ci].image[f][j2]+i2*g.off[bpp],value,1,bpp,1);
       }
   }else return NOMEM;
   if(needconvert) change_image_depth(ci,8,1);
   moveimage(ci,0,0);
   return OK;
}


//-------------------------------------------------------------------------//
// chop_image                                                              //
//-------------------------------------------------------------------------//
int chop_image(int ino, int xpos, int ypos, int shell, int border)
{
  if(!between(ino,1,g.image_count-1)) return BADPARAMETERS;
  char tempstring[128];
  int f,i,i2,j,j2,k,value,x1,y1,x2,y2,xstart,ystart;
  int xsize = z[ino].xsize/2;
  int ysize = z[ino].ysize/2;
  int bpp = z[ino].bpp;
  int ct = z[ino].colortype;
  int frames = z[ino].frames;
  for(k=0; k<4; k++)
  {   if(newimage("Image",xpos,ypos,xsize,ysize,bpp,ct,frames,shell,1,PERM,0,border,0)==OK)
      {   x1 = 0; xstart = xsize*(k%2); 
          y1 = 0; ystart = ysize*(k/2); 
          x2 = x1+xsize; 
          y2 = y1+ysize;
          for(f=0; f<frames; f++)
          for(j=y1,j2=0; j<y2; j++,j2++)
          for(i=x1,i2=0; i<x2; i++,i2+=g.off[bpp])
          {   value = pixelat(z[ino].image[f][j+ystart]+i2+xstart, bpp);
              putpixelbytes(z[ci].image[f][j2]+i2,value,1,bpp,1);
          }
      }else return NOMEM;
      switchto(ci);
      z[ci].touched = 1;
      memcpy(z[ci].palette, g.palette, 768);
      memcpy(z[ci].opalette, g.palette, 768);
      memcpy(z[ci].spalette, g.palette, 768);
      sprintf(tempstring, "Chopped_%d", k+1);
      setimagetitle(ci, tempstring);
      if(g.create_border) image_border();
      if(g.autoundo) backupimage(ci,0);
      rebuild_display(ci);
      redraw(ci);
      xpos +=10;
      ypos +=10;
  }
  return GOTNEW;
}


//-------------------------------------------------------------------------//
// create_image_with_mouse                                                 //
//-------------------------------------------------------------------------//
int create_image_with_mouse(int xpos, int ypos, int frames, int shell, int border)
{
   static int in_create_image_with_mouse = 0;
   int status=OK,f,i,iii,bbb,i2,j,j2,x,y,x1,x2,y1,y2,ct=g.colortype,ino,oino=-1,bpp,ibpp;
   int different=0,needconvert=0;
   uint value;
   if(in_create_image_with_mouse) return IGNORE;
   status = message("Select region for new image");
   if(status == CANCEL){ in_create_image_with_mouse = 0; return ABORT;}
   in_create_image_with_mouse = 1;
   getbox(x1,y1,x2,y2);
   ino = whichimage((x1+x2)/2, (y1+y2)/2, bpp);

   //// Find out if all pixels are from same image.
   //// If so, make the new image same depth and colortype.
   if(x2-x1<1 || y2-y1<1){ in_create_image_with_mouse = 0; return ABORT;}
   for(y=y1;y<y2;y+=4)
   for(x=x1;x<x2;x+=4)
   {    iii = whichimage(x, y, bbb);
        if(iii!=ino || bbb!=bpp) different=1;
   }

   if(different)  bpp = g.bitsperpixel;
   if(bpp==8){ bpp=24; needconvert=1; }
   if(ino>=0) ct = z[ino].colortype;
   if(newimage("Image",x2,y2,x2-x1,y2-y1,bpp,ct,frames,shell,1,PERM,0,border,0)==OK)
   { 
       for(j=y1,j2=0;j<y2;j++,j2++)
       for(i=x1,i2=0;i<x2;i++,i2+=g.off[bpp])
       { 
           ino = whichimg(i, j, ibpp);
           value = readpixel(i, j);
           if(ino!=oino){ switch_palette(ino); oino=ino; }
           for(f=0; f<frames; f++)
                putpixelbytes(z[ci].image[f][j2]+i2,value,1,bpp,1);
       }
   }else 
   {   in_create_image_with_mouse = 0;
       return NOMEM;
   }
   if(needconvert) change_image_depth(ci,8,1);
   moveimage(ci,xpos,ypos);
   in_create_image_with_mouse = 0;
   return OK;
}


//-------------------------------------------------------------------------//
//  create_fixed_size_image                                                //
//-------------------------------------------------------------------------//
int create_fixed_size_image(int xsize, int ysize, int xpos, int ypos,  
    int frames, int shell, int border)
{
   //// Copy from preselected region, possibly off screen
   static int busy=0;
   int status=OK,f,i,i2,j,j2,x1,x2,y1,y2,ct=g.colortype,ino,oino=-1,bpp;
   uint value;
   if(busy) return IGNORE;
   busy = 1;
   status = message("Click at upper left \nof area for new image");
   if(status == CANCEL){ busy=0; return ABORT; }
   getpoint(x1,y1);
   x2 = x1+xsize;
   y2 = y1+ysize;

   ino = whichimage((x1+x2)/2, (y1+y2)/2, bpp);
   int needconvert=0;
   bpp=g.bitsperpixel;
   if(bpp==8){ bpp=24; needconvert=1; }
   if(ino>=0) ct = z[ino].colortype;
   if(newimage("Image",xpos,ypos,x2-x1,y2-y1,bpp,ct,frames,shell,1,PERM,0,border,0)==OK)
   {   for(f=0; f<frames; f++)
       for(j=y1,j2=0;j<y2;j++,j2++)
       for(i=x1,i2=0;i<x2;i++,i2+=g.off[bpp])
       { 
           value = readpixel(i,j);  
           if(ino!=oino){ switch_palette(ino); oino=ino; }
           putpixelbytes(z[ci].image[f][j2]+i2,value,1,bpp,1);
       }
   }else{ busy=0; return NOMEM; }
   if(needconvert) change_image_depth(ci,8,1);
   busy = 0;
   return OK;
}


//-------------------------------------------------------------------------//
// duplicate                                                               //
//-------------------------------------------------------------------------//
int duplicate(int noofargs, char **arg)
{
   int status=OK, ino=ci;   // If no argument specified, make something up
   if(noofargs>=1) ino = image_number(arg[1]); else ino=ci;
   if(g.getout){ g.getout=0; return ERROR; }
   status = duplicate_image(ino, z[ino].xpos+10, z[ino].ypos+10, z[ino].shell,
            z[ino].window_border);
   return status;
}


//-------------------------------------------------------------------------//
// duplicate_image                                                         //
//-------------------------------------------------------------------------//
int duplicate_image(int cimage, int xpos, int ypos, int shell, int border)
{
   int f,i,j,frames,xsize,ysize,xbytes=0,ct,bpp,xsize3,ysize3;
   if(cimage>=g.image_count || cimage<0)
   {   message("Non-existent image",ERROR);
       return(BADPARAMETERS);
   }   
   if(cimage==0)
   {   message("Cannot duplicate the background",ERROR);
       return(BADPARAMETERS);
   }   
   xsize = z[cimage].xsize;
   ysize = z[cimage].ysize;
   ct    = z[cimage].colortype;
   bpp   = z[cimage].bpp;
   frames= z[cimage].frames;
   if(newimage("Image",xpos,ypos,xsize,ysize,bpp,ct,frames,shell,1,PERM,0,border,0)==OK)
   {   
       for(f=0;f<frames;f++)
       {   xbytes=g.off[bpp] * xsize;
           for(j=0;j<ysize;j++)
               memcpy(z[ci].image[f][j],z[cimage].image[f][j],xbytes);
       }
       if(z[cimage].floatexists)
       {   setupfft(ci, xsize3, ysize3);
           for(j=0;j<ysize;j++)
           for(i=0;i<xsize;i++)
           {    z[ci].fft[j][i].real() = z[cimage].fft[j][i].real();
                z[ci].fft[j][i].imag() = z[cimage].fft[j][i].imag();
           }
           z[ci].floatexists = 1;
           z[ci].fftstate = z[cimage].fftstate;
       }
       if(z[cimage].waveletexists) 
       {   z[ci].origxsize = z[cimage].origxsize;
           z[ci].origysize = z[cimage].origysize;
           allocate_image_wavelet
              (ci, xsize, ysize, z[cimage].wavelettype, z[cimage].nlevels);
           for(j=0;j<z[cimage].waveysize;j++)
           for(i=0;i<z[cimage].wavexsize;i++)
                z[ci].wavelet[j][i] = z[cimage].wavelet[j][i];
           z[ci].waveletexists = 1;
           z[ci].fftstate = z[cimage].fftstate;
       }
   }else return NOMEM;
   copyimageparameters(ci, cimage);
   if(z[ci].is_zoomed) 
        resize_img(ci, cint(z[ci].xsize*z[ci].zoomx), cint(z[ci].ysize*z[ci].zoomy)); 
   
   repair(ci);
   switchto(ci);
   moveimage(ci, xpos, ypos);
   redraw(ci);
   return OK;
}


//-------------------------------------------------------------------------//
// create_multiframe_image                                                 //
//-------------------------------------------------------------------------//
int create_multiframe_image(int cimage, int xsize, int ysize, int xpos, int ypos,  
    int frames, int shell, int border, int find_edge)
{
   int status=OK,cf,f,xcount=0,i,i2,j,j2,x,y,xx,yy,ct,bpp,ibpp,ino;
   uint value;
   uchar *add;
   switchto(cimage);
   if(cimage>=g.image_count || cimage<0)
   {   message("Non-existent image",ERROR);
       return(BADPARAMETERS);
   }   
   if(find_edge)
   {   status = message("Click near upper left\nof region for each frame");
       if(status == CANCEL) return ABORT;
   }else
   {   x = g.create_ulx;
       y = g.create_uly;
   }
   cf = z[cimage].cf;
   ct = z[cimage].colortype;
   bpp = z[cimage].bpp;
   if(newimage("Image",-xsize,-ysize,xsize,ysize,bpp,ct,frames,shell,1,PERM,0,border,0)!=OK)
       return NOMEM;
   ino = ci;
   switchto(cimage);
   for(f=0; f<frames; f++)
   {   
       if(find_edge)
       {    getpoint(x,y);
            if(g.create_auto_edge) findedge(cimage,x,y);
            xx = x - z[cimage].xpos;
            yy = y - z[cimage].ypos;
            ////  The xor's can't cross a getpoint() because image 
            ////  might move.
           xor_box(x,y,x+xsize,y+ysize);
#ifdef LINUX
           usleep(200000);   
#else
           sleep(1);
#endif                      
       }else
       {   xx = x;
           yy = y;
       }
       cf = z[cimage].cf;
       ibpp = z[cimage].bpp;
       for(j=yy,j2=0; j<yy+ysize; j++,j2++)
       for(i=xx,i2=0; i<xx+xsize; i++,i2+=g.off[bpp])
       {   if(!between(i,0,z[cimage].xsize-1) ||
              !between(j,0,z[cimage].ysize-1)) value=0;
           else
           {  add = z[cimage].image[cf][j] + g.off[bpp]*i;
              value = pixelat(add,ibpp);  
              value = convertpixel(value, ibpp, bpp,1);
           }
           putpixelbytes(z[ino].image[f][j2]+i2,value,1,bpp,1);
       }
       if(find_edge) 
           xor_box(x,y,x+xsize,y+ysize);
       else
       {   x += xsize + g.create_xspacing; 
           xcount++;
           if(xcount >= g.create_cols)
           {   x = g.create_ulx; 
               xcount=0; 
               y += ysize + g.create_yspacing; 
           }
       }
   }
   switchto(ino);
   moveimage(ino, xpos, ypos);
   return OK;
}


//-------------------------------------------------------------------------//
//  create_panel                                                           //
//-------------------------------------------------------------------------//
int create_panel(int cimage, int xpos, int ypos, int shell, int border, int cols)
{
  int f,frames,frameheight,framewidth,i,i2,j,ct,bpp,rows,x,y,xsize,ysize,
      xstart,value,xoff,yoff;
  xoff = g.create_xspacing;
  yoff = g.create_yspacing;
  switchto(cimage);
  if(cimage >= g.image_count || cimage<0)
  {   message("Non-existent image",ERROR);
      return BADPARAMETERS;
  }   
  frames = z[cimage].frames;
  if(frames<2)
  {   message("Cannot create panel from \nsingle-frame image",ERROR);
      return ERROR;
  }   
  uchar *add;
  ct     = z[cimage].colortype;
  bpp    = z[cimage].bpp;
  frames = z[cimage].frames;
  framewidth  = z[cimage].xsize;
  frameheight = z[cimage].ysize;
  xsize = 1 + cols*(framewidth + xoff) + xoff + 2*g.create_ulx;
  rows  = int(0.9999 + (double)frames / (double)cols);
  ysize = 1 + rows*(frameheight+yoff) + yoff + 2*g.create_uly;
  if(newimage("Image",xpos,ypos,xsize,ysize,bpp,ct,1,shell,1,PERM,0,border,0)!=OK) return NOMEM;
  x = xstart = g.create_ulx;
  y = g.create_uly;
  for(j=0; j<ysize; j++) memset(z[ci].image[0][j], 0, z[ci].xsize*g.off[bpp]);
  for(f=0; f<frames; f++)
  {   for(j=0; j<frameheight; j++)
      for(i=0,i2=0; i<framewidth; i++,i2+=g.off[bpp])
      {   add = z[cimage].image[f][j] + i2;
          value = pixelat(add,bpp);  
          if(j+y<ysize) putpixelbytes(z[ci].image[0][j+y]+i2+x,value,1,bpp,1);
      }
      x += g.off[bpp]*(xoff+framewidth);
      if(x+framewidth >= xsize){ x= xstart; y+=yoff+frameheight;}
  }
  return OK;
}


//-------------------------------------------------------------------------//
//  parse_digits  convert "1 2 3 4-6 8 23-33 44" into int array            //
//-------------------------------------------------------------------------//
int parse_digits(char *inputstring, int *number, int maxcount)
{
   char c, tempstring[16];
   int j=0,k,num1,num2,len,count=0,pos=0;
   len = strlen(inputstring);
   while(j<len && count<maxcount)
   {   c = inputstring[j++];
       pos = 0;
       while(isdigit(c)){ tempstring[pos++] = c; c = inputstring[j++]; }
       num1 = number[count++] = atoi(tempstring);
       if(j>=len || count>=maxcount) break;
       if(c=='-')
       {   pos = 0;
           c = inputstring[j++];
           while(isdigit(c)){ tempstring[pos++] = c; c = inputstring[j++]; }
           num2 = atoi(tempstring);
           if(num1>num2) swap(num1,num2);
           for(k=num1+1;k<=min(maxcount,num2);k++) number[count++]=k;
        }
   }
   return count;
}


//-------------------------------------------------------------------------//
//  split_frames - if panel is 1, images are lined up; otherwise, stacked  //
//-------------------------------------------------------------------------//
int split_frames(int oxpos, int oypos, int cimage, int shell, int border, 
    int cols, int panel)
{
   int ct,bpp,f,framecount=0,i,i2,j,xsize,ysize,xpos,ypos;
   uchar *add;
   uint value;
   if(!between(cimage,1,g.image_count-1)){ message("Bad image number"); return ERROR; }
   ct = z[cimage].colortype;
   bpp = z[cimage].bpp;
   xpos = oxpos;
   ypos = oypos;
   xsize = z[cimage].xsize;
   ysize = z[cimage].ysize;
   for(f=0;f<z[cimage].frames;f++)
   {   
       if(newimage("Image",xpos,ypos,xsize,ysize,bpp,ct,1,shell,1,PERM,0,border,0)!=OK) return NOMEM;
       for(j=0; j<ysize; j++)
       for(i=0,i2=0; i<xsize; i++,i2+=g.off[bpp])
       {   add = z[cimage].image[f][j] + i2;
           value = pixelat(add,bpp);  
           putpixelbytes(z[ci].image[0][j]+i2,value,1,bpp,1);
       }
       rebuild_display(ci);
       redraw(ci);
       if(panel)
       {   xpos += xsize + g.create_xspacing; 
           framecount++;
           if(framecount>=cols)
           {   xpos = oxpos;
               ypos += ysize + g.create_yspacing;
               framecount = 0;
           }           
       }else
       {   xpos+=10; ypos+=10; 
       }
   }
   return OK;
}
 

//-------------------------------------------------------------------------//
// findedge                                                                //
// given starting point x,y on image ino, find upper left corner of item   //
// to make into a frame.                                                   //
//-------------------------------------------------------------------------//
void findedge(int cimage, int &x, int &y)
{
   const int BORDER=1;  // Each frame must have a border at least this size
   int count=0, i,j, ino, bpp, start, value, rr, gg, bb, rr1, gg1, bb1, ulx, uly;
   float d;
   ulx = x;
   uly = y;
   if(!between(cimage,1,g.image_count-1)){ message("Bad image number"); return; }
   start = readpixelonimage(x,y,bpp,ino);  
   valuetoRGB(start,rr1,gg1,bb1,bpp);
   for(i=x;i>=z[cimage].xpos;i--)
   {   value = readpixelonimage(i,y,bpp,ino);
       valuetoRGB(value,rr,gg,bb,bpp);
       d = (float)(abs(rr-rr1) + abs(gg-gg1) + abs(bb-bb1))/(float)g.maxgreen[bpp]; 
       if(d>0.5) count++;
       ulx = i;
       if(count>BORDER){ ulx = i+BORDER+1; break;}
   }
   count=0;
   for(j=y;j>=z[cimage].ypos;j--)
   {   value = readpixelonimage(x,j,bpp,ino);
       valuetoRGB(value,rr,gg,bb,bpp);
       d = (float)(abs(rr-rr1) + abs(gg-gg1) + abs(bb-bb1))/(float)g.maxgreen[bpp]; 
       if(d>0.5) count++;
       uly = j;
       if(count>BORDER){ uly = j+BORDER+1; break;}
   }
   x = ulx;
   y = uly;
}


//-------------------------------------------------------------------------//
// separate_colors                                                         //
//-------------------------------------------------------------------------//
int separate_colors(int ino, int noofargs, char **arg)
{
   noofargs=noofargs; arg=arg;
   int f,i,j,ct,x,y,w,h,rr,gg,bb,bpp,obpp,value,frames,wantpalette,
        shell,rino,gino,bino,status=YES;
   if(ino<0) { message("Please select an image",WARNING); return NOIMAGES; }
   if(z[ino].colortype!=COLOR) 
   {   status = message("Recommend converting image\nto 24 bit color before proceeding",WARNING); 
       if(status != YES) return ABORT;
   }
   x = z[ino].xpos;
   y = z[ino].ypos;
   w = z[ino].xsize;
   h = z[ino].ysize;
   frames = z[ino].frames;
   ct = GRAY;
   bpp = 8;
   obpp = z[ino].bpp;
   shell = z[ino].shell;
   wantpalette = 1;

   ////  Create 3 8-bit images for r,g, and b
      
   if(newimage("Red",x+10,y+10,w,h,bpp,ct,frames,shell,1,PERM,wantpalette,g.window_border,0)!=OK)
   {  return NOMEM; }
   rino = ci;
   if(newimage("Green",x+20,y+20,w,h,bpp,ct,frames,shell,1,PERM,wantpalette,g.window_border,0)!=OK)
   {  eraseimage(rino,0,0,1); return NOMEM; }
   gino = ci;
   if(newimage("Blue",x+30,y+30,w,h,bpp,ct,frames,shell,1,PERM,wantpalette,g.window_border,0)!=OK)
   {   eraseimage(rino,0,0,1);  eraseimage(gino,0,0,1); return NOMEM; }
   bino = ci;

      
   for(f=0;f<frames;f++)
   for(j=0;j<h;j++)
   for(i=0;i<w;i++)
   {    value = pixelat(z[ino].image[f][j] + i*g.off[obpp], obpp);  
        valuetoRGB(value,rr,gg,bb,obpp);
        z[rino].image[f][j][i] = rr;
        z[gino].image[f][j][i] = gg;
        z[bino].image[f][j][i] = bb;
   }
   setimagetitle(rino,"Red");
   setimagetitle(gino,"Green");
   setimagetitle(bino,"Blue");
   z[rino].touched = 1;
   z[gino].touched = 1;
   z[bino].touched = 1;
   rebuild_display(rino);
   rebuild_display(gino);
   rebuild_display(bino);
   redraw(rino);
   redraw(gino);
   redraw(bino);
   switchto(rino);
   switchto(gino);
   switchto(bino);
   if(g.autoundo){ backupimage(rino,0); backupimage(gino,0); backupimage(bino,0); }
   return OK;
}


//-------------------------------------------------------------------------//
// combine_colors                                                          //
//-------------------------------------------------------------------------//
int combine_colors(int noofargs, char **arg)
{
  noofargs=noofargs; arg=arg;
  static clickboxinfo *item;
  static int rino=ci, gino=ci, bino=ci;
  char temp[100];
  int helptopic = 0;
   
  if(noofargs)
  {     if(noofargs>1) rino = image_number(arg[1]);
        if(g.getout){ g.getout=0; return ERROR; }
        if(noofargs>2) gino = image_number(arg[2]);
        if(g.getout){ g.getout=0; return ERROR; }
        if(noofargs>3) bino = image_number(arg[3]); 
        if(g.getout){ g.getout=0; return ERROR; }
  }else
  {     item = new clickboxinfo[3];
        item[0].wantpreview=0;
        item[0].title = new char[128];
        strcpy(item[0].title,"Red");
        item[0].startval = rino;
        item[0].minval = 0;
        item[0].maxval = g.image_count;
        item[0].type = VALSLIDER;
        item[0].wantdragcb = 1;
        item[0].answers = new int[10]; // Must be 10 for multiclickbox cb
        item[0].form = NULL;
        item[0].path = NULL;

        item[1].title = new char[128];
        strcpy(item[1].title,"Green");
        item[1].startval = gino;
        item[1].minval = 0;
        item[1].maxval = g.image_count;
        item[1].type = VALSLIDER;
        item[1].wantdragcb = 1;
        item[1].form = NULL;
        item[1].path = NULL;

        item[2].title = new char[128];
        strcpy(item[2].title,"Blue");
        item[2].startval = bino;
        item[2].minval = 0;
        item[2].maxval = g.image_count;
        item[2].type = 2;
        item[2].wantdragcb = 1;
        item[2].form = NULL;
        item[2].path = NULL;
        strcpy(temp,"Source images for red, green, and blue");

        item[0].ino = ci;
        item[0].noofbuttons = 3;

        item[0].f1 = null;
        item[0].f2 = null;
        item[0].f3 = null;
        item[0].f4 = null;
        item[0].f5 = combinecolorsok;
        item[0].f6 = combinecolorsfinish;
        item[0].f7 = null;
        item[0].f8 = null;
        multiclickbox(temp, 3, item, null, helptopic);
   }
   return OK;
}


//-------------------------------------------------------------------------//
// combinecolorsok                                                         //
//-------------------------------------------------------------------------//
void combinecolorsok(clickboxinfo *c)
{
  int rino, gino, bino;
  int f,h,i,j,ct,x,y,w,rr,gg,bb,value,frames,wantpalette,shell;

  rino = c[0].answers[0];
  gino = c[0].answers[1];
  bino = c[0].answers[2];

  if(z[rino].bpp!=8 || z[gino].bpp!=8 || z[bino].bpp!=8)
  {   message("Source images must be 8 bits/pixel",ERROR); return; }
  if(z[rino].xsize!= z[gino].xsize || 
     z[rino].xsize!= z[bino].xsize || 
     z[gino].xsize!= z[bino].xsize || 
     z[rino].ysize!= z[gino].ysize || 
     z[rino].ysize!= z[bino].ysize || 
     z[gino].ysize!= z[bino].ysize )
  {   message("Source images must be the same size",ERROR); return; }

  ////  Create 24-bit image for combination
  x = z[rino].xpos;
  y = z[rino].ypos;
  w = z[rino].xsize;
  h = z[rino].ysize;
  frames = min(min(z[rino].frames,z[gino].frames),z[bino].frames);
  ct = COLOR;
  shell = z[rino].shell;
  wantpalette = 0;
    
  if(newimage("Image",x+10,y+10,w,h,24,ct,frames,shell,1,PERM,wantpalette,g.window_border,0)!=OK)
      return;

  for(f=0;f<frames;f++)
  for(j=0;j<h;j++)
  for(i=0;i<w;i++)
  {    rr = z[rino].image[f][j][i];
       gg = z[gino].image[f][j][i];
       bb = z[bino].image[f][j][i];
       value = RGBvalue(rr,gg,bb,24);
       putpixelbytes(z[ci].image[f][j]+i*g.off[24], value, 1, 24);
  }
  setimagetitle(ci,"Composite");
  z[ci].touched = 1;
  rebuild_display(ci);
  redraw(ci);
  switchto(ci);
  if(g.autoundo) backupimage(ci, 0); 
  return;
}


//-------------------------------------------------------------------------//
// combinecolorsfinish                                                     //
//-------------------------------------------------------------------------//
void combinecolorsfinish(clickboxinfo *c)
{
  int k;
  delete[] c[0].answers;
  for(k=0;k<3;k++) delete[] c[k].title;
  delete[] c;                                   
}


//-------------------------------------------------------------------------//
// shift_frame                                                             //
//-------------------------------------------------------------------------//
int shift_frame(int noofargs, char **arg)
{
  static clickboxinfo* item;
  char temp[100];
  int helptopic = 0;
  if(ci==0){ message("Can't shift background"); return ERROR; }
  if(ci<0 || ci>=g.image_count) { message("Bad image number"); return ERROR; }
   
  if(noofargs)
  {    if(noofargs>=1) hshift = atoi(arg[1]);
       if(noofargs>=2) vshift = atoi(arg[2]);
       image_shift_frame(ci, hshift, vshift);
  }else
  {    item = new clickboxinfo[2];
       item[0].wantpreview=0;
       item[0].title = new char[128];        
       strcpy(item[0].title,"Vertical");
       item[0].startval = vshift;
       item[0].minval = -100;
       item[0].maxval = 100;
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
       strcpy(item[1].title,"Horizontal");
       item[1].startval = hshift;
       item[1].minval = -100;
       item[1].maxval = 100;
       item[1].type = VALSLIDER;
       item[1].wantdragcb = 0;
       item[1].answers = item[0].answers; 
       strcpy(temp,"Shift frame for current image");
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

       item[0].ino = ci;
       item[0].noofbuttons = 2;
       item[0].f5 = shiftframeok;
       item[0].f6 = shiftframefinish;
       item[0].f7 = null;
       item[0].f8 = null;
       multiclickbox(temp, 2, item, null, helptopic);
  }
  return OK;
}


//-------------------------------------------------------------------------//
// shiftframeok                                                            //
//-------------------------------------------------------------------------//
void shiftframeok(clickboxinfo *c)
{
  int ino = c[0].ino;
  vshift = c[0].answer;
  hshift = c[1].answer;
  image_shift_frame(ino, hshift, vshift);
}


//-------------------------------------------------------------------------//
// image_shift_frame                                                       //
//-------------------------------------------------------------------------//
void image_shift_frame(int ino, int hshift, int vshift)
{
  int bpp,f,i,j,ir,iw,jr,jw,bytesperline,value,xstart,ystart,xend,yend;
  uchar *radd, *wadd;
  f = z[ino].cf;
  bpp = z[ino].bpp;
  xstart = -hshift * g.off[bpp];
  ystart = -vshift;
  bytesperline = z[ino].xsize * g.off[bpp];
  xend   = bytesperline - g.off[bpp];
  yend   = z[ino].ysize - 1;

  if(hshift<0)
  {    for(j=0; j<z[ino].ysize;j++)
       for(iw=0,ir=xstart; iw<bytesperline; iw+=g.off[bpp],ir+=g.off[bpp])
       {    if(ir>=0 && ir<bytesperline)
            {   radd = z[ino].image[f][j] + ir;
                value = pixelat(radd,bpp);
            }else value=0;
            wadd = z[ino].image[f][j] + iw;
            putpixelbytes(wadd, value, 1, bpp);
       }
  }
  if(hshift>0)
  {    for(j=0; j<z[ino].ysize;j++)
       for(iw=xend, ir=xstart+xend; iw>=0; iw-=g.off[bpp],ir-=g.off[bpp])
       {    if(ir>=0 && ir<bytesperline)
            {   radd = z[ino].image[f][j] + ir;
                value = pixelat(radd,bpp);
            }else value=0;
            wadd = z[ino].image[f][j] + iw;
            putpixelbytes(wadd, value, 1, bpp);
       }
  }
  
  if(vshift<0)
  {    for(jw=0,jr=ystart; jw<z[ino].ysize; jw++,jr++)
       for(i=0; i<bytesperline; i+=g.off[bpp])
       {    if(jr>=0 && jr<z[ino].ysize)
            {   radd = z[ino].image[f][jr] + i;
                value = pixelat(radd,bpp);
            }else value=0;
            wadd = z[ino].image[f][jw] + i;
            putpixelbytes(wadd, value, 1, bpp);
       }
  }
  if(vshift>0)
  {    for(jw=yend, jr=ystart+yend; jw>=0; jw--, jr--)
       for(i=0; i<bytesperline; i+=g.off[bpp])
       {    if(jr>=0 && jr<z[ino].ysize)
            {   radd = z[ino].image[f][jr] + i;
                value = pixelat(radd,bpp);
            }else value=0;
            wadd = z[ino].image[f][jw] + i;
            putpixelbytes(wadd, value, 1, bpp);
       }
  }
  rebuild_display(ino); 
  redraw(ino);
  return;
}


//-------------------------------------------------------------------------//
// shiftframefinish                                                        //
//-------------------------------------------------------------------------//
void shiftframefinish(clickboxinfo *c)
{
   delete[] c[0].title;
   delete[] c[0].answers;
   delete[] c;
}



//-------------------------------------------------------------------------//
// grab_window                                                             //
//-------------------------------------------------------------------------//
int grab_window(void)
{
  static PromptStruct ps;
  if(in_grab_window) return IGNORE;
  in_grab_window = 1; 
  ps.f1 = grab_window_part2;
  ps.f2 = grab_window_finish;
  click_prompt((char*)"Click on desired window to grab", &ps, 0);
  return NOIMAGES;
}


//-------------------------------------------------------------------------//
// grab_window_part2                                                       //
//-------------------------------------------------------------------------//
void grab_window_part2(PromptStruct *ps)
{
  ps=ps;
  XTextProperty text_prop;
  in_grab_window = 0;
  char name[256] = "Window";
  int rx,ry,wx,wy,button=0,ct=g.colortype; 
  uint uw,uh,ubw,bpp;   
  int xroot,yroot,xstart=0,ystart=0;
  uint keys;
  Window rwin, cwin;
  if(g.getout) return;
  while(!button)
  {     XQueryPointer(g.display,g.root_window,&rwin,&cwin,&rx,&ry,&wx,&wy,&keys);
        button = keys & Button1Mask;
  }
  // Bypass stupid inconsistency in X API--if pointer is on root window, cwin 
  // returns 0, which crashes XGetGeometry.
  if(cwin==0) cwin=rwin;
  if(cwin==0) return;
  XGetGeometry(g.display, cwin, &rwin, &xroot, &yroot, &uw, &uh, &ubw, &bpp);
  if(XGetWMName(g.display, cwin, &text_prop))
        strcpy(name, (char*)text_prop.value);
  // ignore the sometimes incorrect bpp information from XGetGeometry
  bpp = g.bitsperpixel;  
  // Make sure grabbed window is not off screen
  uw = min(g.xres - xroot - 1, (int)uw);
  uh = min(g.yres - yroot - 1, (int)uh);
  if(xroot < 0){ xstart = -xroot; uw -= xstart; } 
  if(yroot < 0){ ystart = -yroot; uh -= ystart; } 
  if(newimage("Image",0,0,uw,uh,bpp,ct,1,g.want_shell,1,PERM,0,g.window_border,0)==OK)
  {  
      XGetSubImage(g.display, cwin, xstart, ystart, uw, uh, XAllPlanes(), 
         ZPixmap, z[ci].image_ximage, 0, 0); 
      create_image_initialize(ci, name);
  } else message(g.nomemory); 
}


//-------------------------------------------------------------------------//
// grab_window_finish                                                      //
//-------------------------------------------------------------------------//
void grab_window_finish(PromptStruct *ps)
{
   ps=ps;
}


//-------------------------------------------------------------------------//
//  button_clicked                                                         //
//-------------------------------------------------------------------------//
int button_clicked(void)
{
  int rx,ry,wx,wy,button=0;   
  uint keys;
  Window rwin, cwin;
  XQueryPointer(g.display,g.root_window,&rwin,&cwin,&rx,&ry,&wx,&wy,&keys);
  button = keys & Button1Mask;
  return button;
}


//-------------------------------------------------------------------------//
//  get_image_list                                                         //
//-------------------------------------------------------------------------//
int get_image_list(int *source, int &count)
{
   int k, status=OK;
   static char inputstring[1024] = "";
   char tempstring[1024] = "Enter list of image numbers,\ne.g. 1 2 3 6 8 11-23 34\nNote: all images must have the same bits/pixel";
   status = message(tempstring, inputstring, PROMPT, 1023, 60);
   if(status == CANCEL) return ABORT;
   if(!strlen(inputstring)) return ABORT;
   count = parse_digits(inputstring, source, MAXIMAGES);
   for(k=0;k<count;k++) if(source[k]<=0 || source[k]>=g.image_count)
   {   sprintf(tempstring,"Bad image number(%d)",source[k]); 
       message(tempstring,ERROR);
       return ERROR;
   }
   return OK;  
}

//-------------------------------------------------------------------------//
//  check_image_list                                                       //
//-------------------------------------------------------------------------//
int check_image_list(int *source, int count, int check_size)
{
   int k, bpp;
   char tempstring[1024];
   bpp = z[source[0]].bpp;
   for(k=1; k<count; k++) if(z[source[k]].bpp !=bpp)
   {   sprintf(tempstring,"Image number(%d) has wrong bits/pixel (%d)\n\
Convert image to %d bits/pixel first",source[k],z[source[k]].bpp,bpp); 
       message(tempstring,ERROR);
       return ERROR;
   }
   if(count<1) return BADPARAMETERS;
   if(check_size)
   for(k=1; k<count; k++) 
   {   if(z[source[k]].xsize != z[source[0]].xsize ||
          z[source[k]].ysize != z[source[0]].ysize)
       {   sprintf(tempstring,"Image number(%d) has wrong size (%d x %d)",
              source[k], z[source[k]].xsize,  z[source[k]].ysize); 
           message(tempstring,ERROR);
           return ERROR;
       }
   }
   return OK;
}


//-------------------------------------------------------------------------//
//  combine_frames                                                         //
//-------------------------------------------------------------------------//
int combine_frames(int xpos, int ypos, int shell, int border)
{
   int count=0;
   int source[MAXIMAGES];
   if(get_image_list(source, count)!=OK) return ERROR;
   if(check_image_list(source, count, 1)) return ERROR;
   return combine_image_frames(xpos, ypos, shell, border, source, count);
}   


//-------------------------------------------------------------------------//
//  combine_image_frames                                                   //
//-------------------------------------------------------------------------//
int combine_image_frames(int xpos, int ypos, int shell, int border, int *source,
    int count)
{
   int ct,bpp,f,i,i2,ino,j,xsize,ysize;
   uint value;
   uchar *add;
   ct = z[source[0]].colortype;
   bpp = z[source[0]].bpp;
   xsize = z[source[0]].xsize;
   ysize = z[source[0]].ysize;
   if(xsize<1 || ysize<1){ xsize=z[source[0]].xsize; ysize=z[source[0]].ysize; }
   if(newimage("Image",xpos,ypos,xsize,ysize,bpp,ct,count,shell,1,PERM,0,border,0)!=OK) return NOMEM;
   for(f=0; f<count; f++)
   {  ino = source[f];      
      for(j=0; j<ysize; j++)
      for(i=0,i2=0; i<xsize; i++,i2+=g.off[bpp])
      {   add = z[ino].image[0][j] + i2;
          value = pixelat(add,bpp);  
          putpixelbytes(z[ci].image[f][j]+i2,value,1,bpp,1);
      }
   }
   return OK;
}


//-------------------------------------------------------------------------//
// composite_image                                                         //
//-------------------------------------------------------------------------//
int composite_image(int xpos, int ypos, int shell, int border)
{
   static Dialog *dialog;
   int j,k,status=OK;
   if(ci>=0) g.create_cimage = ci;
   dialog = new Dialog;
   if(dialog==NULL){ message(g.nomemory); return(NOMEM); }
   strcpy(dialog->title,"Create Composite Image");

   strcpy(dialog->boxes[0],"Parameters");
   strcpy(dialog->boxes[1],"Source images (e.g. 1 2-4 5)");
   strcpy(dialog->boxes[2],"Spacing");
   strcpy(dialog->boxes[3],"No.of Columns");
   strcpy(dialog->boxes[4],"Manual placement");
   
   dialog->boxtype[0]=LABEL;
   dialog->boxtype[1]=STRING;
   dialog->boxtype[2]=INTCLICKBOX;
   dialog->boxtype[3]=INTCLICKBOX;
   dialog->boxtype[4]=TOGGLE;

   dialog->boxmin[2]=0; dialog->boxmax[2]=1000;
   dialog->boxmin[3]=1; dialog->boxmax[3]=MAXIMAGES;
   sprintf(dialog->answer[1][0], "%s", composite_image_list);
   sprintf(dialog->answer[2][0], "%d", composite_spacing);
   sprintf(dialog->answer[3][0], "%d", composite_columns);
   dialog->boxset[4] = composite_want_specify_position;

   dialog->noofradios = 0;
   dialog->radiono[0] = 0;
   for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;

   dialog->noofboxes = 5; 
   dialog->helptopic = 31;  
   dialog->want_changecicb = 0;
   dialog->f1 = composite_image_check;
   dialog->f2 = null;
   dialog->f3 = null;
   dialog->f4 = null;
   dialog->f5 = null;
   dialog->f6 = null;
   dialog->f7 = null;
   dialog->f8 = null;
   dialog->f9 = null;
   dialog->width = 0;  // calculate automatically
   dialog->height = 0; // calculate automatically
   dialog->transient = 1;
   dialog->wantraise = 0;
   dialog->radiousexy = 0;
   dialog->boxusexy = 0;
   strcpy(dialog->path,".");
   dialog->message[0] = 0;      
   dialog->param[0] = ci;
   dialog->param[1] = xpos;
   dialog->param[2] = ypos;
   dialog->param[3] = shell;
   dialog->param[4] = border;
   dialog->busy = 0;
   dialogbox(dialog);
   return status;
}


//-------------------------------------------------------------------------//
// composite_image_check                                                   //
//-------------------------------------------------------------------------//
void composite_image_check(dialoginfo *a, int radio, int box, int boxbutton)
{
   boxbutton = boxbutton;
   static char pos[2][MAXIMAGES][16];
   static char label[MAXIMAGES][256];
   char ***ptr;
   char **ptr2;

   int ct=0,bpp,bpp2,i,i2,i3,ino,j,k=0,xsubsize,ysubsize,cols,
       xsub,ysub,oci,needconvert=0,xpos,ypos,shell,border,count=0,
       rows,xsize=0,ysize=0, row,col,xmax=0,
       ymax=0,xoffset,yoffset;
   int source[MAXIMAGES];
   char title[64];  
   uchar *add;
   int value;
   int *biggestx, *biggesty;
   char tempstring[FILENAMELENGTH];

   if(strlen(a->answer[1][0])) strcpy(composite_image_list, a->answer[1][0]);
   composite_spacing = atoi(a->answer[2][0]);
   composite_columns = cols = atoi(a->answer[3][0]);
   composite_want_specify_position = a->boxset[4];

   if(box==4 && composite_want_specify_position && strlen((composite_image_list)))
   {   
       count = parse_digits(composite_image_list, source, MAXIMAGES);
       strcpy(title, "Positions for subimages in composite image");
       for(k=0; k<count; k++)
       {   ino = source[k];     
           strcpy(tempstring, basefilename(z[ino].name));
           tempstring[20]=0;  // truncate name
           sprintf(label[k], "x,y position for image %c%d (%s)", '#', ino, tempstring);
           strcpy(pos[0][k], "0");
           strcpy(pos[1][k], "0");
           ct = z[k].colortype;
       }
       //// Don't look at this section
       ptr = new char **[2];
       ptr[0] = new char *[count];
       ptr[1] = new char *[count];      
       ptr2 = new char *[count];
       for(k=0;k<count;k++) 
       {    ptr2[k] = new char[128];        
            ptr[1][k] = new char[128];
            ptr[0][k] = new char[128];
            strcpy(ptr[0][k], pos[0][k]);
            strcpy(ptr[1][k], pos[1][k]);
            strcpy(ptr2[k], label[k]);
       }
       getstrings(title, ptr2, NULL, ptr, 2, count, 16);
       for(k=0;k<count;k++) 
       {    strcpy(pos[0][k], ptr[0][k]);
            strcpy(pos[1][k], ptr[1][k]);
            strcpy(label[k], ptr2[k]);
            delete[] ptr2[k];
            delete[] ptr[1][k];
            delete[] ptr[0][k];
       }
       delete[] ptr2;
       delete[] ptr[1];
       delete[] ptr[0];
       delete[] ptr;
   }

   //////////////////////
   if(radio != -2) return;   //// User clicked Ok or Enter
   //////////////////////

   if(strlen(composite_image_list))
   {   count = parse_digits(composite_image_list, source, MAXIMAGES);
       for(k=0;k<count;k++) if(source[k]<=0 || source[k]>=g.image_count)
       {   sprintf(tempstring,"Bad image number(%d)",source[k]); 
           message(tempstring, ERROR);
           return;
       }
       if(check_image_list(source, count, 0)) return;
       ct = z[source[0]].colortype;
   }
   if(count <= 0) return;
   rows = int(0.99 + (double)count / (double)cols);
   if(composite_want_specify_position)
   {   for(k=0; k<count; k++)
       {   ino = source[k];     
           xmax = max(xmax, atoi(pos[0][k]) + z[ino].xsize + composite_spacing);
           ymax = max(ymax, atoi(pos[1][k]) + z[ino].ysize + composite_spacing);
       }
   }else
   {   
       biggestx = new int[cols+1];
       biggesty = new int[rows+1];
       for(k=0;k<cols+1;k++) biggestx[k]=0;
       for(k=0;k<rows+1;k++) biggesty[k]=0;
       for(k=count-1; k>=0; k--)
       {   ino = source[k];     
           ct = z[ino].colortype;
       }
       for(k=0; k<count; k++)
       {   ino = source[k];     
           row = k / cols;
           col = k % cols;
           biggestx[col+1] = max(biggestx[col+1], z[ino].xsize);
           biggesty[row+1] = max(biggesty[row+1], z[ino].ysize);
       }
       for(k=1; k<count; k++)
       for(j=k-1; j>=0; j--)
       {   biggestx[k] += biggestx[j]; 
           biggesty[k] += biggesty[j]; 
       }
       //// set x,y for each column
       for(k=0; k<count; k++)
       {   ino = source[k];     
           row = k / cols;
           col = k % cols;
           xoffset = col*composite_spacing/2;
           yoffset = row*composite_spacing/2;
           itoa(biggestx[col] + composite_spacing + xoffset, pos[0][k], 10); 
           itoa(biggesty[row] + composite_spacing + yoffset, pos[1][k], 10); 
           xmax = max(xmax, atoi(pos[0][k]) + z[ino].xsize + composite_spacing);
           ymax = max(ymax, atoi(pos[1][k]) + z[ino].ysize + composite_spacing);
       }
       delete[] biggesty;
       delete[] biggestx;
   }
  
   xpos   = a->param[1];
   ypos   = a->param[2];
   shell  = a->param[3];
   border = a->param[4];

   if(!strlen(composite_image_list)) return;
   if(!count) return;
   bpp = bpp2 = z[source[0]].bpp;
   if(ct!=GRAY && bpp==8){ bpp2 = 24; ct=COLOR; needconvert=1; }
   if(xsize<1 || ysize<1){ xsize=z[source[0]].xsize; ysize=z[source[0]].ysize; }
   if(newimage("Image",xpos,ypos,xmax,ymax,bpp2,ct,1,shell,1,PERM,0,border,0)!=OK) return;
   oci = ci;
   for(j=0; j<z[ci].ysize; j++) memset(z[ci].image[0][j],255,z[ci].xsize*g.off[bpp2]);
   bpp = z[source[0]].bpp;
   for(k=0; k<count; k++)
   {   ino = source[k];
       switchto(ino); 
       memcpy(g.palette, z[ino].palette, 768);
       xsub = atoi(pos[0][k]);        // x position for subimage in composite
       ysub = atoi(pos[1][k]);        // y position for subimage in composite
       xsubsize = z[ino].xsize;       // x size of subimage
       ysubsize = z[ino].ysize;       // y size of subimage
       for(j=0; j<ysubsize; j++)
       for(i=0,i2=0,i3=xsub*g.off[bpp2]; i<xsubsize; i++,i2+=g.off[bpp],i3+=g.off[bpp2])
       {   if(j+ysub >= z[oci].ysize || i+xsub >= z[oci].xsize) continue;
           if(j+ysub < 0 || i3 < 0) continue;
           add = z[ino].image[0][j] + i2;
           value = pixelat(add, bpp);  
           if(ct==GRAY) value = convertpixel(value, bpp, bpp2, 0);
           else         value = convertpixel(value, bpp, bpp2, 1);
           putpixelbytes(z[oci].image[0][j+ysub]+i3,value,1,bpp2,1);
       }
   }
   memcpy(z[ci].palette, z[source[0]].palette, 768);
   memcpy(z[ci].opalette, z[source[0]].opalette, 768);
   memcpy(z[ci].spalette, z[source[0]].spalette, 768);
   memcpy(g.palette, z[source[0]].palette, 768);
   switchto(oci);
   if(needconvert) change_image_depth(oci, bpp, 1);
   repair(ci);
   if(g.autoundo) backupimage(ci,0);
   return;
}


//-------------------------------------------------------------------------//
//  create_spotlist_panel                                                  //
//-------------------------------------------------------------------------//
int create_spotlist_panel(int ino, int xsize, int ysize, int xpos, int ypos, 
    int frames, int shell, int border, int xspacing, int yspacing, 
    int xmargin, int ymargin,
    int want_fixed_xsize, int fixed_xsize,
    int want_fixed_ysize, int fixed_ysize)
{
  FILE *fp;
  const int DENS_MAX=10000;
  static char filename[FILENAMELENGTH]="1.spots";
  XYData spotdata;
  g.getout = 0;
  int b,f,x1,y1,x2,y2,i,j,k,row,col,v,xout,yout,xspotsize=0,yspotsize=0,
      xfac,yfac;
  if(!between(ino, 1, g.image_count)) return NOIMAGES;
  int ct = z[ino].colortype;
  int bpp = z[ino].bpp;
  f = z[ino].cf;
  b = g.off[bpp];
  strcpy(filename, getfilename(filename, NULL));   
  if(!strlen(filename)){ message("File not found", ERROR); return NOTFOUND; }
  if(g.getout){ g.getout=0; return ABORT; }
  if(g.spotlist==NULL)
  {   g.spotlist = new char[EDITSIZE]; // leave allocated
      g.spotlist[0] = 0; 
  }
  if(g.spotlist==NULL) { message(g.nomemory,ERROR); return NOMEM; }

  if ((fp=fopen(filename,"rb")) == NULL){ error_message(filename, errno); return ERROR; }
  if(g.getout) return ABORT;
  fread(g.spotlist, 1, EDITSIZE, fp);
  fclose(fp);

  if(newimage("Image",xpos,ypos,xsize,ysize,bpp,ct,frames,shell,1,PERM,0,border,0)!=OK) return NOMEM;
  for(j=0; j<ysize; j++)
  for(i=0; i<xsize; i++)
        putpixelbytes(z[ci].image[0][j]+b*i,(int)g.maxvalue[bpp],1,bpp);      

  spotdata.label = new char*[DENS_MAX];
  spotdata.label2 = new char*[DENS_MAX];
  for(k=0; k<DENS_MAX; k++){ spotdata.label[k] = new char[128]; spotdata.label[k][0]=0; } 
  for(k=0; k<DENS_MAX; k++){ spotdata.label2[k] = new char[128]; spotdata.label2[k][0]=0; } 
  spotdata.x = new int[DENS_MAX];
  spotdata.y = new int[DENS_MAX];
  spotdata.u = new double[DENS_MAX];
  spotdata.v = new double[DENS_MAX];
  spotdata.x1 = new int[DENS_MAX];
  spotdata.y1 = new int[DENS_MAX];
  spotdata.x2 = new int[DENS_MAX];
  spotdata.y2 = new int[DENS_MAX];
  if(spotdata.y2==NULL){ message(g.nomemory, ERROR); return NOMEM; }

  //// No returns past this point

  if(g.spotlist != NULL) parse_spotdata(&spotdata, g.spotlist);
  else{ message("Spot list is empty"); g.getout=1; }
  if(spotdata.n<1){ message("Spot list is empty"); g.getout=1; }


  row = col = 0;
  xout = yout = 10;
  for(k=0;k<spotdata.n;k++)      
  {    xspotsize = max(xspotsize, spotdata.x2[k]-spotdata.x1[k]);
       yspotsize = max(yspotsize, spotdata.y2[k]-spotdata.y1[k]);
  }
  if(want_fixed_xsize) xfac = xspacing + xmargin + fixed_xsize;
  else                 xfac = xspacing + xmargin + xspotsize;
  if(want_fixed_ysize) yfac = yspacing + ymargin + fixed_ysize;
  else                 yfac = yspacing + ymargin + yspotsize;

  if(!g.getout) for(k=0;k<spotdata.n;k++)      
  {
       if(want_fixed_xsize)
       {    x1 = spotdata.x[k] - fixed_xsize/2 - xmargin;
            x2 = spotdata.x[k] + fixed_xsize/2 + xmargin;
       }else 
       {    x1 = spotdata.x1[k] - xmargin;
            x2 = spotdata.x2[k] + xmargin;
       }
       if(want_fixed_ysize)
       {    y1 = spotdata.y[k] - fixed_ysize/2 - ymargin;
            y2 = spotdata.y[k] + fixed_ysize/2 + ymargin;
       }else
       {    y1 = spotdata.y1[k] - ymargin;
            y2 = spotdata.y2[k] + ymargin;
       }
       xout = 10 + col * xfac;
       yout = 10 + row * yfac;
       x1 = min(z[ino].xsize-1, max(x1, 0));
       x2 = min(z[ino].xsize-1, max(x2, 0));
       y1 = min(z[ino].ysize-1, max(y1, 0));
       y2 = min(z[ino].ysize-1, max(y2, 0));

       if(yout>=ysize) break;

       for(j=0; j<y2-y1; j++)
       for(i=0; i<x2-x1; i++)
       {
           if(!between(x1+i, 0, z[ino].xsize-1)) continue;
           if(!between(y1+j, 0, z[ino].ysize-1)) continue;
           if(!between(xout+i, 0, xsize-1)) continue;
           if(!between(yout+j, 0, ysize-1)) continue;
           v = pixelat(z[ino].image[f][y1+j]+b*(x1+i), bpp);
           putpixelbytes(z[ci].image[0][yout+j]+b*(i+xout),v,1,bpp);      
       }
       col++;
       if(10 + xfac + x2 - x1 + xout >= xsize){ col=0; row++; }
  }
  delete[] spotdata.x;
  delete[] spotdata.y;
  delete[] spotdata.u;
  delete[] spotdata.v;
  delete[] spotdata.x1;
  delete[] spotdata.y1;
  delete[] spotdata.x2;
  delete[] spotdata.y2;
  for(k=0; k<DENS_MAX; k++) delete[] spotdata.label[k];  
  delete[] spotdata.label;
  for(k=0; k<DENS_MAX; k++) delete[] spotdata.label2[k];  
  delete[] spotdata.label2;
  spotdata.n = 0;
  repair(ci);
  redraw(ci);
  return OK;
}



