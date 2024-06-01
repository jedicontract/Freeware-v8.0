//--------------------------------------------------------------------------//
// xmtnimage17.cc                                                           //
// Reading & writing of custom format files                                 //
// Latest revision: 07-02-2000                                              //
// Copyright (C) 2000 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"
#define __USE_POSIX2 1
#define __USE_SVID 1
extern Globals     g;
extern Image      *z;
extern int         ci;
struct Cui
{   short uint off;
    int *var;
    int bytes;
    int def;
};
int noofoffsets=21;               // Change this if adding a new offset

//--------------------------------------------------------------------------//
// create_custom_format                                                     //
// Create a new custom image file format and save the parameters in the     //
// file "Formats". Doesn't save any image.                                  //
//--------------------------------------------------------------------------//
void create_custom_format(void)
{
  if(memorylessthan(16384)){ message(g.nomemory,ERROR); return; }
  static Dialog *dialog;
  static int selection2 = 0;
  static int selection10 = 0;
  dialog = new Dialog;
  if(dialog==NULL){ message(g.nomemory); return; }
  int j,k,rgbbpp=0;
  Format format;
  g.getout=0;  
  initialize_format_descriptor(format);

  strcpy(dialog->title,"Custom Image Format");
  strcpy(dialog->radio[0][0],"Target platform");              
  strcpy(dialog->radio[0][1],"Intel (little end.)");
  strcpy(dialog->radio[0][2],"Mac (big endian)");
  strcpy(dialog->radio[0][3],"MVS Mainframe");
  strcpy(dialog->radio[1][0],"Bit Packing");
  strcpy(dialog->radio[1][1],"TIF-like");
  strcpy(dialog->radio[1][2],"GIF-like");
  strcpy(dialog->radio[1][3],"None");
  strcpy(dialog->radio[2][0],"Format Identification");
  strcpy(dialog->radio[2][1],"By identifier");
  strcpy(dialog->radio[2][2],"By extension");
 
  strcpy(dialog->boxes[0]  ,"Format Parameters");
  strcpy(dialog->boxes[1]  ,"Identifier"); 
  strcpy(dialog->boxes[2]  ,"File Offsets" );
  strcpy(dialog->boxes[3]  ,"Bytes to skip" );
  strcpy(dialog->boxes[4]  ,"External Header File");
  strcpy(dialog->boxes[5]  ," Ext. header file");
  strcpy(dialog->boxes[6]  ,"Header file  " );
  strcpy(dialog->boxes[7]  ,"Header bytes" );
  strcpy(dialog->boxes[8]  ,"Default Parameters");
  strcpy(dialog->boxes[9] ,"Bits/Pixel");
  strcpy(dialog->boxes[10] ,"Color type");
  strcpy(dialog->boxes[11] ,"x size in pixels");
  strcpy(dialog->boxes[12] ,"y size in pixels");
  strcpy(dialog->boxes[13] ,"RGB bits/pixel");
  strcpy(dialog->boxes[14] ,"Black bits/pixel");
  strcpy(dialog->boxes[15] ,"No. of frames");
  strcpy(dialog->boxes[16] ,"Signed pixel values");

  dialog->boxtype[0]=LABEL; 
  dialog->boxtype[1]=STRING;
  dialog->boxtype[2]=LIST; 
  dialog->boxtype[3]=STRING;
  dialog->boxtype[4]=LABEL;
  dialog->boxtype[5]=TOGGLE;
  dialog->boxtype[6]=STRING;
  dialog->boxtype[7]=INTCLICKBOX; 
  dialog->boxtype[8]=LABEL; 
  dialog->boxtype[9]=INTCLICKBOX;
  dialog->boxtype[10]=NON_EDIT_LIST;
  dialog->boxtype[11]=STRING;
  dialog->boxtype[12]=STRING;
  dialog->boxtype[13]=RGBCLICKBOX;
  dialog->boxtype[14]=INTCLICKBOX;
  dialog->boxtype[15]=INTCLICKBOX;
  dialog->boxtype[16]=TOGGLE;
  
  dialog->boxmin[2]=0;           // min menu selection
  dialog->boxmax[2]=noofoffsets; // max no.of menu items to use
  dialog->boxmin[7]=0; dialog->boxmax[7]=1024;
  dialog->boxmin[9]=1; dialog->boxmax[9]=32;
  dialog->boxmin[10]=0; dialog->boxmax[10]=2;
  dialog->boxmin[13]=0; dialog->boxmax[13]=48;
  dialog->boxmin[14]=0; dialog->boxmax[14]=48;
  dialog->boxmin[15]=1; dialog->boxmax[15]=1000;
  
  dialog->l[2]            = new listinfo;
  dialog->l[2]->title     = new char[100];
  dialog->l[2]->info      = new char*[noofoffsets];
  dialog->l[2]->count     = noofoffsets;
  dialog->l[2]->selection = &selection2;
  dialog->l[2]->wantfunction = 0;
  dialog->l[2]->f1        = null;
  dialog->l[2]->f2        = null;
  dialog->l[2]->f3        = null;
  dialog->l[2]->f4        = null; // dialog lists are deleted in dialogcancelcb
  dialog->l[2]->f5        = null;
  dialog->l[2]->f6        = null;
  dialog->l[2]->listnumber = 0;
  dialog->l[2]->wc        = 0;
  for(k=0; k<noofoffsets; k++) dialog->l[2]->info[k] = new char[100];
  strcpy(dialog->l[2]->title, "Offsets for format (bytes)");

  dialog->l[10]            = new listinfo;
  dialog->l[10]->title     = new char[100];
  dialog->l[10]->info      = new char*[4];
  dialog->l[10]->count     = 3;
  dialog->l[10]->selection = &selection10;
  dialog->l[10]->wantfunction = 0;
  dialog->l[10]->f1        = null;
  dialog->l[10]->f2        = null;
  dialog->l[10]->f2        = null;
  dialog->l[10]->f4        = null; // dialog lists are deleted in dialogcancelcb
  dialog->l[10]->f5        = null;
  dialog->l[10]->f6        = null;
  dialog->l[10]->listnumber = 0;
  dialog->l[10]->wc        = 0;
  for(k=0; k<4; k++) dialog->l[10]->info[k] = new char[100];
  strcpy(dialog->l[10]->title,"Default color type");

  //// default values
  dialog->radioset[0]=1;
  dialog->radioset[1]=1;
  dialog->radioset[2]=2;
  dialog->bpp = 24;
  strcpy(dialog->answer[1][0], "(none)");
  strcpy(dialog->l[2]->info[0], "");
  sprintf(dialog->answer[3][0], "%d", 0);  
  dialog->boxset[5] = 0;
  strcpy(dialog->answer[6][0], "");
  sprintf(dialog->answer[7] [0], "%d", 0);
  sprintf(dialog->answer[9] [0], "%d", dialog->bpp);
  strcpy(dialog->l[10]->info[0], "Color");
  sprintf(dialog->answer[11][0], "%d", 256);
  sprintf(dialog->answer[12][0], "%d", 256);
  rgbbpp = RGBvalue(format.defaultrbpp, format.defaultgbpp, format.defaultbbpp, 24);
  sprintf(dialog->answer[13][0], "%d", rgbbpp);
  sprintf(dialog->answer[14][0], "%d", 0);
  sprintf(dialog->answer[15][0], "%d", 1);
  dialog->boxset[16] = 0; 

  dialog->radiono[0]=4;
  dialog->radiono[1]=4;
  dialog->radiono[2]=3;
  for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
  for(j=0;j<noofoffsets;j++) dialog->startcol[j]=0;
  dialog->startcol[2] = 17;
  dialog->startcol[10] = 0;
  dialog->noofradios=3;
  dialog->noofboxes=17;
  dialog->helptopic=24;  
  dialog->want_changecicb = 0;
  dialog->f1 = customcheck;
  dialog->f2 = null;
  dialog->f3 = null;
  dialog->f4 = customfinish;
  dialog->f5 = null;
  dialog->f6 = null;
  dialog->f7 = null;
  dialog->f8 = null;
  dialog->f9 = null;
  dialog->transient = 1;
  dialog->wantraise = 0;
  dialog->radiousexy = 0;
  dialog->boxusexy = 0;
  dialog->width=0;  // calculate automatically
  dialog->height=0; // calculate automatically

  strcpy(dialog->path,".");
  g.getout=0;
  dialog->message[0]=0;      
  dialog->busy = 0;
  dialogbox(dialog);
}


//--------------------------------------------------------------------------//
// update_custom_dialog                                                     //
//--------------------------------------------------------------------------//
void update_custom_dialog(dialoginfo *a, Format &format, int update_widgets)
{
  int i, rgbbpp;
  Widget www;
 
  if(format.platform==PC) a->radioset[0]=1;
  if(format.platform==MAC)a->radioset[0]=2;
  if(format.platform==MVS)a->radioset[0]=3;
  
  if(format.packing==TIF) a->radioset[1]=1;
  if(format.packing==GIF) a->radioset[1]=2;
  if(format.packing==NONE)a->radioset[1]=3;
      
  if(format.useidentifier==1) a->radioset[2]=1;
  if(format.useidentifier==0) a->radioset[2]=2;
 
  strcpy(a->answer[1][0], format.identifier);
  strcpy(a->answer[2][0],"   ");
  sprintf(a->answer[3][0], "%d",format.skipbytes);  
  if(format.useheader) a->boxset[5]=1; 
  else                 a->boxset[5]=0;
  strcpy(a->answer[6][0],format.headerfile);
  sprintf(a->answer[7][0], "%d",  format.headerbytes);
  sprintf(a->answer[9][0], "%d", format.defaultbpp);
  strcpy(a->answer[10][0], " ");
  sprintf(a->answer[11][0], "%d", format.defaultxsize);
  sprintf(a->answer[12][0], "%d", format.defaultysize);
  a->bpp = 24;
  rgbbpp = RGBvalue(format.defaultrbpp, format.defaultgbpp, format.defaultbbpp, 24);
  sprintf(a->answer[13][0], "%d", rgbbpp);
  sprintf(a->answer[14][0], "%d", format.defaultkbpp);
  sprintf(a->answer[15][0], "%d", format.defaultframes);
  sprintf(a->answer[16][0], "%d", format.datatype);

  sprintf(a->l[2]->info[0],"Identifier  (64) %d",format.identifieroff);
  sprintf(a->l[2]->info[1],"X size       (2) %d",format.xoff          );
  sprintf(a->l[2]->info[2],"Y size       (2) %d",format.yoff          );
  sprintf(a->l[2]->info[3],"Bits/pixel   (2) %d",format.bppoff        );
  sprintf(a->l[2]->info[4],"Color type   (2) %d",format.colortypeoff  );
  sprintf(a->l[2]->info[5],"Compression  (2) %d",format.compressflagoff);
  sprintf(a->l[2]->info[6],"Red bpp      (2) %d",format.redbppoff     );
  sprintf(a->l[2]->info[7],"Green bpp    (2) %d",format.greenbppoff   );
  sprintf(a->l[2]->info[8],"Blue bpp     (2) %d",format.bluebppoff    );
  sprintf(a->l[2]->info[9],"Black bpp    (2) %d",format.blackbppoff   );
  sprintf(a->l[2]->info[10],"X position   (2) %d",format.xposoff      );
  sprintf(a->l[2]->info[11],"Y position   (2) %d",format.yposoff      );
  sprintf(a->l[2]->info[12],"Frames       (4) %d",format.framesoff    );
  sprintf(a->l[2]->info[13],"Frames/sec   (2) %d",format.fpsoff       );
  sprintf(a->l[2]->info[14],"Chromakey    (2) %d",format.chromakeyoff );
  sprintf(a->l[2]->info[15],"Chr.gray min (4) %d",format.ck_grayminoff);
  sprintf(a->l[2]->info[16],"Chr.gray max (4) %d",format.ck_graymaxoff);
  sprintf(a->l[2]->info[17],"Chr. RGB min (4) %d",format.ck_minoff    );
  sprintf(a->l[2]->info[18],"Chr. RGB max (4) %d",format.ck_maxoff    );
  sprintf(a->l[2]->info[19],"Palette    (768) %d",format.paletteoff );
  sprintf(a->l[2]->info[20],"Image offset (2) %d",format.imageoff     );

  strcpy(a->l[10]->info[0],"Grayscale");
  strcpy(a->l[10]->info[1],"Indexed color");
  strcpy(a->l[10]->info[2],"True color");
  *a->l[10]->selection = format.defaultcolortype;
  a->boxset[16] = format.datatype; 

  char tempstring[128];
  if(update_widgets)
  {
      for(i=0;i<a->noofradios;i++)
      {   www = a->radiowidget[i][a->radioset[i]];
          XmToggleButtonSetState(www, True, True); 
      }
      for(i=1;i<a->noofboxes;i++)
      { 
          switch(a->boxtype[i])
          {   case TOGGLE:
                 XmToggleButtonSetState(a->boxwidget[i][0],a->boxset[i],False);
                 break;
              case STRING:
              case SCROLLEDSTRING:
                 strcpy(tempstring,a->answer[i][0]);
                 //// Get rid of callback or it will go into a loop
                 XtRemoveCallback(a->boxwidget[i][0], XmNvalueChangedCallback, 
                     (XtCBP)dialogstringcb, (XtP)a);
                 XtVaSetValues(a->boxwidget[i][0], XmNvalue,tempstring, NULL);      
                 XtAddCallback(a->boxwidget[i][0], XmNvalueChangedCallback, 
                     (XtCBP)dialogstringcb, (XtP)a);
                 break;
              case TOGGLESTRING:
                 strcpy(tempstring,a->answer[i][1]);
                 //// Get rid of callback or it will go into a loop
                 XtRemoveCallback(a->boxwidget[i][1], XmNvalueChangedCallback, 
                     (XtCBP)dialogstringcb, (XtP)a);
                 XtVaSetValues(a->boxwidget[i][1], XmNvalue, tempstring, NULL);      
                 XtAddCallback(a->boxwidget[i][1], XmNvalueChangedCallback, 
                     (XtCBP)dialogstringcb, (XtP)a);
                 break;
              case INTCLICKBOX:
                 strcpy(tempstring,a->answer[i][0]);
                 set_widget_label(a->boxwidget[i][0], tempstring);
                 break;
              case LIST:
              case NON_EDIT_LIST:
                 if(i==10)
                 {  if(*a->l[i]->selection == GRAY) strcpy(tempstring,"Grayscale");
                    if(*a->l[i]->selection == INDEXED) strcpy(tempstring,"Indexed");
                    if(*a->l[i]->selection == COLOR) strcpy(tempstring,"Color");
                    if(*a->l[i]->selection == CMYK) strcpy(tempstring,"CMYK");
                    if(*a->l[i]->selection == GRAPH) strcpy(tempstring,"Graph");
                 }
                 set_widget_label(a->boxwidget[i][0], tempstring);
                 break;
         }
      }
  }
  return;
}


//--------------------------------------------------------------------------//
//  customcheck                                                             //
//  update dialogbox with new format information if identifier is changed   //
//--------------------------------------------------------------------------//
void customcheck(dialoginfo *a, int radio, int box, int boxbutton)
{
  static char oanswer[128] = "";
  radio=radio; boxbutton=boxbutton;
  Format format;
  int status=NOTFOUND, rgbbpp;
  char iden[64], match[64];

  if(a->radioset[0]==1) format.platform=PC;
  if(a->radioset[0]==2) format.platform=MAC;
  if(a->radioset[0]==3) format.platform=MVS;
  if(a->radioset[1]==1) format.packing=TIF;
  if(a->radioset[1]==2) format.packing=GIF;
  if(a->radioset[1]==3) format.packing=NONE;
  if(a->radioset[2]==1) format.useidentifier=1;
  if(a->radioset[2]==2) format.useidentifier=0;
  if(a->radioset[3]==1) format.compressflag=1;
  if(a->radioset[3]==2) format.compressflag=0;

  strcpy(format.identifier, a->answer[1][0]);
  format.identifieroff= atoi(a->l[2]->info[0]+a->startcol[2]);
  format.xoff         = atoi(a->l[2]->info[1]+a->startcol[2]);
  format.yoff         = atoi(a->l[2]->info[2]+a->startcol[2]);
  format.bppoff       = atoi(a->l[2]->info[3]+a->startcol[2]);
  format.colortypeoff = atoi(a->l[2]->info[4]+a->startcol[2]);
  format.compressflagoff 
                      = atoi(a->l[2]->info[5]+a->startcol[2]);
  format.redbppoff    = atoi(a->l[2]->info[6]+a->startcol[2]);
  format.greenbppoff  = atoi(a->l[2]->info[7]+a->startcol[2]);
  format.bluebppoff   = atoi(a->l[2]->info[8]+a->startcol[2]);
  format.blackbppoff  = atoi(a->l[2]->info[9]+a->startcol[2]);
  format.xposoff      = atoi(a->l[2]->info[10]+a->startcol[2]);
  format.yposoff      = atoi(a->l[2]->info[11]+a->startcol[2]);
  format.framesoff    = atoi(a->l[2]->info[12]+a->startcol[2]);
  format.fpsoff       = atoi(a->l[2]->info[13]+a->startcol[2]);
  format.chromakeyoff = atoi(a->l[2]->info[14]+a->startcol[2]);
  format.ck_grayminoff= atoi(a->l[2]->info[15]+a->startcol[2]);
  format.ck_graymaxoff= atoi(a->l[2]->info[16]+a->startcol[2]);
  format.ck_minoff    = atoi(a->l[2]->info[17]+a->startcol[2]);
  format.ck_maxoff    = atoi(a->l[2]->info[18]+a->startcol[2]);
  format.paletteoff   = atoi(a->l[2]->info[19]+a->startcol[2]);
  format.imageoff     = atoi(a->l[2]->info[20]+a->startcol[2]);

  format.skipbytes    = atoi(a->answer[3][0]);
  if(a->boxset[5]) format.useheader=1; 
  else format.useheader=0;

  strcpy(format.headerfile,a->answer[6][0]);
  format.headerbytes = atoi(a->answer[7][0]);
  format.defaultbpp  = atoi(a->answer[9][0]);
  format.defaultcolortype = *a->l[10]->selection;
  format.defaultxsize= atoi(a->answer[11][0]);
  format.defaultysize= atoi(a->answer[12][0]);
  rgbbpp = atoi(a->answer[13][0]);
  valuetoRGB(rgbbpp, format.defaultgbpp, format.defaultgbpp, format.defaultgbpp, 24);
  format.defaultkbpp = atoi(a->answer[14][0]);
  format.defaultframes = atoi(a->answer[15][0]);
  format.datatype      = a->boxset[16];

  update_custom_dialog(a, format, 0);

  ////  Check input for consistency

  if((strlen(format.headerfile)==0)&&(format.useheader))
  {  message("No header file specified",ERROR);
  }
  if((format.useheader)&&(format.headerbytes>filesize(format.headerfile)))
  {  message("Specified header bytes exceeds length\nof header file",ERROR);
  }
  if((format.identifieroff==65535)&&(format.useidentifier==1))
  {  format.useidentifier=0;                             
  }   
  if((format.identifieroff!=65535)&&(format.useidentifier==0))
  {  format.useidentifier=1;
  }   

  if(box==1 && strcmp(a->answer[1][0], oanswer))
  {    strncpy(iden, a->answer[1][0], 63);    
       if(is_customfile((char*)"Untitled", iden, NULL, match))
            status = read_format_descriptor(match, format);
       if(status==OK) update_custom_dialog(a, format, 1);
       strcpy(oanswer, a->answer[1][0]);
  }

  ////  Save custom format information
  if(radio == -2) write_format_descriptor(format);  
}


//--------------------------------------------------------------------------//
//  customfinish                                                            //
//--------------------------------------------------------------------------//
void customfinish(dialoginfo *a)
{
  a=a;
}


//--------------------------------------------------------------------------//
// writecustomfile                                                          //
//--------------------------------------------------------------------------//
int writecustomfile(char *filename, int write_all, char *identifier, int compress)
{

  if(memorylessthan(16384)){  message(g.nomemory,ERROR); return(NOMEM); } 
  int direction=1;
  int want_color=0;
  if(g.want_noofcolors>1)want_color=1;   
  int ino,bpp=g.want_bpp;
  int rbpp=g.want_redbpp;
  int gbpp=g.want_greenbpp;
  int bbpp=g.want_bluebpp;
  int kbpp=g.want_blackbpp;
  int a, bb, bufsize, compressflag=0, count, ct, dbpp, evenbits, f, gg, i, 
      intbppfac, j, k, kk=0, mbpp, oldxend, pos, rr, s, size1, size2, ui2, frames=1,
      xsize, xstart, xend, ystart, yend, ysize, alloc1=0, imagestart=0;
  uint value,total=0; 
  FILE *fp;
  FILE *fp2;
  char extension[FILENAMELENGTH]="X";
  char *ptr;
  char tempstring[FILENAMELENGTH];
  char zero[10]={ 0,0,0,0,0,0,0,0,0 };
  uchar* bufr;
  uchar* bufr2;
  char* header = NULL;
  Format format;
  
  ////  Extract extension from filename if format is identified by extension

  strcpy(tempstring, filename);
  ptr = strchr(tempstring,'.');
  if(ptr!=NULL) strcpy(extension,ptr+1);    
  extension[63] = 0;

  if(read_format_descriptor(identifier, format)!=OK)
  {   message("Unknown custom format");
      return ERROR;
  }
  compressflag = format.compressflag;

  if(!format.useidentifier && strcmp(extension, format.identifier)!=SAME)
  {   sprintf(tempstring,"Filename extension must be %s",format.identifier);
      message(tempstring,ERROR);
      return ERROR;
  }

  if(write_all==1)
  {    xstart = z[ci].xpos;
       ystart = z[ci].ypos;
       xend = xstart + z[ci].xsize - 1;
       yend = ystart + z[ci].ysize - 1;
       mbpp = z[ci].bpp;           // mbpp is bits/pixel of image in memory
       frames = z[ci].frames;
  } else 
  {    xstart = g.selected_ulx;
       xend = g.selected_lrx;
       ystart = g.selected_uly;
       yend = g.selected_lry;
       mbpp = g.bitsperpixel;
       frames = 1;
  }

  intbppfac     =   ((7+bpp)/8);
  int round_bpp = 8*((7+bpp)/8);  // bpp rounded up to next 8
  xsize = xend-xstart+1;
  ysize = yend-ystart+1;
  if(bpp==1) message("Saving as monochrome!"); 
  if(g.getout){ g.getout=0; return ABORT; }

  if((fp=open_file(filename, compress, TNI_WRITE, g.compression, g.decompression, 
       g.compression_ext))==NULL) 
       return(CANTCREATEFILE);  // cant create file
  pos = 0;

  if(format.useheader)
  {    if(strcmp(format.headerfile,"")!=0)  // if header file name is given
       {   if((fp2=fopen(format.headerfile,"rb")) == NULL)
           {   error_message(format.headerfile, errno);
               close_file(fp, compress);  
               fclose(fp2);
               return CANTCREATEFILE;      // file error
           }  
           header = new char[format.headerbytes];
           if(header==NULL)
           {   message(g.nomemory); 
               close_file(fp, compress);  
               fclose(fp2); 
               return ERROR; 
           }else alloc1=1;
           fread(header, format.headerbytes, 1, fp2);
           fclose(fp2);
       }
       if(format.platform!=PC)   // Mac or IBM mainframe (big endian)
       {   header[format.xoff]   = xsize >> 8;
           header[format.xoff+1] = xsize & 0xff;
           header[format.yoff]   = ysize >> 8;
           header[format.yoff+1] = ysize & 0xff;
       }else                     // Intel little endian format
       {   header[format.xoff+1] = xsize >> 8;
           header[format.xoff]   = xsize & 0xff;
           header[format.yoff+1] = ysize >> 8;
           header[format.yoff]   = ysize & 0xff;
       }
  }
 
  oldxend = xend;
  evenbits = ((xsize*bpp+7)/8)*8; 
  while(xsize*bpp < evenbits) { xsize++; xend++; }

  size1         = intbppfac*xsize+10;
  size2         = g.off[max(z[ci].bpp,g.bitsperpixel)]*xsize+10;
  bufsize       = max(size1,size2);      
  bufr          = new uchar[bufsize];
  if(bufr==NULL) 
  {   close_file(fp, compress);  
      if(alloc1) delete[] header; 
      return(NOMEM);
  }
  bufr2         = new uchar[bufsize];
  if(bufr2==NULL) 
  {   delete[] bufr; 
      if(alloc1) delete[] header; 
      close_file(fp, compress);  
      return NOMEM; 
  }
  bufr[bufsize-1]  = 99;
  bufr2[bufsize-1] = 99;
  ////   Endian format for integers > 256 (>=16 bpp grayscale only)
  if(format.platform==PC) direction=-1; else direction=1;

  ////   Write header  

  if(format.useheader) 
  {  fwrite(header, format.headerbytes, 1, fp); 
     pos += format.headerbytes;
     imagestart += format.headerbytes;
     delete[] header;
     alloc1 = 0;
  }
  if(format.useidentifier)
  {  
     forward_seek(fp, pos, format.identifieroff, TNI_WRITE);
     format.identifier[63]=0;   // max length is 64
     imagestart += strlen(format.identifier) + 1;
                                // Don't save the final null character
     fwrite(&format.identifier, strlen(format.identifier), 1, fp); 
     pos += strlen(format.identifier);
  }


  //// Note: if 4th parameter is 2, 3rd parameter may not exceed 65535.
  //// Most values are kept at 2 bytes to maintain compatibility with
  //// other formats.
  
  if(g.usegrayscale) ct = GRAY;
  else if(bpp>8) ct = COLOR;
  else ct = INDEXED;

  //// custom_write automatically updates imagestart and pos

  custom_write(fp, format.xoff, (ushort)xsize, imagestart, pos); 
  custom_write(fp, format.yoff, (ushort)ysize, imagestart, pos); 
  custom_write(fp, format.bppoff, (ushort)bpp, imagestart, pos); 
  custom_write(fp, format.colortypeoff, (ushort)ct, imagestart, pos); 
  custom_write(fp, format.compressflagoff, (ushort)compressflag, imagestart, pos); 
  custom_write(fp, format.redbppoff, (ushort)g.want_redbpp, imagestart, pos);
  custom_write(fp, format.greenbppoff, (ushort)g.want_greenbpp, imagestart, pos); 
  custom_write(fp, format.bluebppoff, (ushort)g.want_bluebpp, imagestart, pos);
  custom_write(fp, format.blackbppoff, (ushort)g.want_blackbpp, imagestart, pos); 
  custom_write(fp, format.xposoff, (ushort)xstart, imagestart, pos);
  custom_write(fp, format.yposoff, (ushort)ystart, imagestart, pos); 
  custom_write(fp, format.framesoff, (uint)frames, imagestart, pos);
  custom_write(fp, format.fpsoff, (ushort)z[ci].fps, imagestart, pos); 
  custom_write(fp, format.chromakeyoff, (ushort)z[ci].chromakey, imagestart, pos); 
  custom_write(fp, format.ck_grayminoff, (uint)z[ci].ck_graymin, imagestart, pos); 
  custom_write(fp, format.ck_graymaxoff, (uint)z[ci].ck_graymax, imagestart, pos); 
  custom_write(fp, format.ck_minoff, (RGB)z[ci].ck_min, imagestart, pos); 
  custom_write(fp, format.ck_maxoff, (RGB)z[ci].ck_max, imagestart, pos); 
  custom_write(fp, format.paletteoff, (void*)z[ci].palette, 768, imagestart, pos); 

  //// Override imagestart if fixed offset specified in format
 
  if(format.skipbytes!=0)
     imagestart = format.skipbytes;
  else if(format.imageoff!=65535)
  {  forward_seek(fp, pos, format.imageoff, TNI_WRITE);
     imagestart = 2 + max(format.imageoff, imagestart);
     putword(imagestart, fp); 
     pos += 2;
  }

  forward_seek(fp, pos, imagestart, TNI_WRITE);
  total = 0;

  ////   Write image data    

  for(f=0;f<frames;f++)
  for(j=ystart;j<=yend;j++)
  { if(bpp==1)
    {    for(i=xstart,count=0;count<(xsize>>3);i+=8)
         {  count++;
            s = readbyte(i,j); 
            putbyte(s,fp);
            pos++;
            total++;
            if(format.platform==MVS)  // IBM MVS has 4 extra bytes after 4092
            {  if(total==4092)
               {  putdword(zero[0],fp);
                  pos += 4;
                  total=0;
               }
            }
         }   
    }else
    {    ui2 = 0;      
         getbits(0,bufr,0,RESET);                //reset bits at each scan line
         for(k=xstart;k<=xend;k++)
         {  if(write_all)
               value = pixelat(z[ci].image[f][j-ystart]+ 
                       g.off[mbpp]*(k-xstart),mbpp);
            else
               value = readpixelonimage(k,j,bpp,ino);
   
            if(k>oldxend) value=0;                   
            if(want_color)                       // Adjust color bits to wanted bpp
            {  valuetoRGB(value,rr,gg,bb,mbpp);
               value = 0;
               if(g.save_cmyk && kbpp)           //convert to cmyk
               {   kk = min(g.maxred[mbpp]-rr, min(g.maxgreen[mbpp]-gg,
                            g.maxblue[mbpp]-bb));
                   rr = g.maxred[mbpp]-rr-kk;    // r is now cyan
                   gg = g.maxgreen[mbpp]-gg-kk;  // g is now magenta
                   bb = g.maxblue[mbpp]-bb-kk;   // b is now yellow
                   kk = max(0,min(g.maxred[mbpp] ,kk));
                   rr = max(0,min(g.maxred[mbpp] , rr));
                   gg = max(0,min(g.maxgreen[mbpp],gg));
                   bb = max(0,min(g.maxblue[mbpp], bb));
               }
             
               dbpp = rbpp - g.redbpp[mbpp];
               if(dbpp>0) rr <<= ( dbpp); else rr >>= (-dbpp);
               value += rr << (gbpp + bbpp + kbpp);

               dbpp = gbpp - g.greenbpp[mbpp];    
               if(dbpp>0) gg <<= ( dbpp); else gg >>= (-dbpp);
               value += gg << (bbpp + kbpp);
             
               dbpp = bbpp - g.bluebpp[mbpp];                  
               if(dbpp>0) bb <<= ( dbpp); else bb >>= (-dbpp);
               value += bb << (kbpp);

               if(g.save_cmyk && kbpp)
               {  dbpp = kbpp - g.redbpp[mbpp];                  
                  if(dbpp>0) kk <<= ( dbpp); else kk >>= (-dbpp);
                  value += kk;
               }
            }else if(mbpp==8)               // noofcolors is 1
            {                               // In 8bpp, value is already 
               dbpp = bpp - 8;              // same as luminosity
               if(dbpp>0) value <<= ( dbpp);// Shift luminosity to desired bpp
               if(dbpp<0) value >>= (-dbpp);
            }else                           // In rgb screen mode but
            {                               // want 1 color 
               // Dont do anything to them
            }
               
            putpixelbytes(bufr2+ui2, value, 1, round_bpp, direction);
            ui2 += intbppfac;
         }              
              // `ui2' counts input bytes, `a' counts output (packed) bytes.
              // They are not necessarily the same value.
   
         a = packbits(bpp,bufr2,bufr,intbppfac*xsize); 
         if(format.platform==MVS)    //  IBM MVS has 4 extra bytes after 4092
         {  for(k=0;k<a;k++)
            {  if(total==4092)
               {  putdword(zero[0],fp);
                  pos += 4;
                  total=0;
               }
               putbyte(bufr[k],fp);
               pos++;
               total++;
            }   
         }   
         else
         {
            count = fwrite(bufr,a,1,fp);
            pos += a;
         }
    }
  }

  if(bufr[bufsize-1]!=99) message("Internal error - bufr",ERROR);
  if(bufr2[bufsize-1]!=99) message("Internal error - bufr2",ERROR);

  g.want_redbpp  = rbpp;
  g.want_greenbpp= gbpp;
  g.want_bluebpp = bbpp;
  g.want_blackbpp= kbpp;
  g.want_bpp = bpp;
  
  delete[] bufr;
  delete[] bufr2;
  close_file(fp, compress);  
  return(OK);
}



//--------------------------------------------------------------------------//
// custom_write                                                             //
//--------------------------------------------------------------------------//
void custom_write(FILE *fp, int offset, ushort val, int &imagestart, int &pos)
{
  if(offset!=65535)
  {  forward_seek(fp, pos, offset, TNI_WRITE);
     imagestart = max(offset, imagestart);
     imagestart += 2;
     pos += 2;
     putword(val, fp);
  }
}
void custom_write(FILE *fp, int offset, uint val, int &imagestart, int &pos)
{
  if(offset!=65535)
  {  forward_seek(fp, pos, offset, TNI_WRITE);
     imagestart = max(offset, imagestart);
     imagestart += 4;
     pos += 4;
     putdword(val, fp); 
  }
}
void custom_write(FILE *fp, int offset, RGB val, int &imagestart, int &pos)
{
  if(offset!=65535)
  {  forward_seek(fp, pos, offset, TNI_WRITE);
     imagestart = max(offset, imagestart);
     imagestart += 6;
     pos += 6;
     fwrite(&val, 6, 1, fp);
  }
}
void custom_write(FILE *fp, int offset, void *val, int bytes, int &imagestart, int &pos)
{
  if(offset!=65535)
  {  forward_seek(fp, pos, offset, TNI_WRITE);
     imagestart = max(offset, imagestart);
     imagestart += bytes;
     pos += bytes;
     fwrite(val, bytes, 1, fp);
  }
}


//--------------------------------------------------------------------------//
// custom_read                                                              //
//--------------------------------------------------------------------------//
void custom_read(FILE *fp, int &pos, int offset, int *value, int bytes, int defaultvalue)
{
  if(offset!=65535)
  {  forward_seek(fp, pos, offset, TNI_READ);
     fread(value, bytes, 1, fp);
     pos += bytes;
#ifndef LITTLE_ENDIAN
     if(bytes<=4)
     {   swapbytes((uchar*)value, sizeof(int), sizeof(int));
         if(bytes==2) *value &= 0xffff;
     }
#endif     
  }else *value = defaultvalue;
}


//--------------------------------------------------------------------------//
// forward_seek - for read pipes opened with popen                          //
//--------------------------------------------------------------------------//
void forward_seek(FILE *fp, int &pos, int destination, int mode)
{
  char junk=0;
  int k;
  if(pos>destination) fprintf(stderr,"fd_seek error %d %d\n",pos,destination);
  if(mode==TNI_READ)
     for(k=pos;k<destination;k++,pos++) fread(&junk, 1, 1, fp); 
  else
     for(k=pos;k<destination;k++,pos++) fwrite(&junk, 1, 1, fp); 
}     


//--------------------------------------------------------------------------//
// pseek - for pipes opened with popen or regular files                     //
// Pipes can only seek forward, from current position.                      //
//--------------------------------------------------------------------------//
int pseek(FILE *fp, int bytes, int read_mode, int compressed)
{
  int k;
  char junk=0;
  if(bytes<=0) return 0;
  if(compressed)
  {     if(read_mode==TNI_READ) for(k=0;k<bytes;k++) fread(&junk,1,1,fp); 
        if(read_mode==TNI_WRITE) for(k=0;k<bytes;k++) fwrite(&junk,1,1,fp); 
  }else fseek(fp, bytes, SEEK_CUR);
  return bytes;
}     




//--------------------------------------------------------------------------//
// cui_cmp - for qsort                                                      //
//--------------------------------------------------------------------------//
int cui_cmp(const void *p, const void *q)
{
   return sgn(((Cui*)p)->off - ((Cui*)q)->off);
}


//--------------------------------------------------------------------------//
// readcustomfile                                                           //
// Reads:   Custom image formatted files                                    //
//--------------------------------------------------------------------------//
int readcustomfile(char *filename, char *identifier)
{  
  int ct, xsize=0, ysize=0, f, i, i2, j, k, kk, mm,  rr, gg, bb, bpp=8, 
      ibpp=8, xstart=0, ystart=0, imagestart=0, bufsize, bytesperrow=0, 
      nelements, noofcolors, size1, size2, intbppfac=1, rbpp=0, gbpp=0, 
      bbpp=0, kbpp=0, want_color=0, frames=1,  fps=1, filepos=0,
      chromakey, ck_graymin=0, ck_graymax=0, dbpp=0, compressflag=0;
  FILE *fp;
  Format format;  
  uint value;
  RGB ck_min, ck_max, palette[256], defpalette[256];
  double pfac;
  uchar* buffer;
  memcpy(defpalette, g.palette, 768);

  ////  Determine file parameters

  if(read_format_descriptor(identifier, format)!=OK)
  {    message("Cant read format descriptor"); 
       return ERROR;
  }

  ////  Open the file. Decompression command must write to stdout.
  ////  File is read through a pipe to avoid temporary files.
  ////  Forks & execs decompression command if image is compressed.

  if ((fp=fopen(filename,"rb")) == NULL)    // A regular file
  {   error_message(filename, errno);
      return(ERROR);           
  }

  xstart = 1 + g.tif_xoffset;
  ystart = g.tif_yoffset;

  ////  Read header using offsets from descriptor
  ////  Sort offsets so they are in ascending order (can't fseek backwards
  ////  in a pipe).

  Cui cui[] = 
  { {  format.xoff,         &xsize,           2, format.defaultxsize    },
    {  format.yoff,         &ysize,           2, format.defaultysize    },
    {  format.bppoff,       &bpp ,            2, format.defaultbpp      },
    {  format.colortypeoff, &ct,              2, format.defaultcolortype},
    {  format.compressflagoff, &compressflag, 2, format.compressflag},
    {  format.redbppoff,    &g.want_redbpp,   2, format.defaultrbpp},
    {  format.greenbppoff,  &g.want_greenbpp, 2, format.defaultgbpp},
    {  format.bluebppoff,   &g.want_bluebpp,  2, format.defaultbbpp},
    {  format.blackbppoff,  &g.want_blackbpp, 2, format.defaultkbpp},
    {  format.xposoff,      &xstart,          2, 0},
    {  format.yposoff,      &ystart,          2, 0},
    {  format.framesoff,    &frames,          4, 1},
    {  format.fpsoff,       &fps,             2, 1},
    {  format.chromakeyoff, &chromakey,       2, 0},
    {  format.ck_grayminoff,&ck_graymin,      4, 0},
    {  format.ck_graymaxoff,&ck_graymax,      4, 0},
    {  format.ck_minoff,    (int*)&ck_min,    4, 0},
    {  format.ck_maxoff,    (int*)&ck_max,    4, 0},
    {  format.paletteoff,   (int*)&palette[0], 768, (int)defpalette },
    {  format.imageoff,     &imagestart,      2, format.skipbytes}
  };
  nelements =  sizeof(cui) / sizeof(Cui);
  qsort(cui, nelements, sizeof(Cui), cui_cmp);

  for(k=0;k<nelements;k++)
  {    custom_read(fp, filepos, cui[k].off, cui[k].var, cui[k].bytes, cui[k].def);
       if(cui[k].var <0){ message("Error reading custom file",ERROR); return ERROR; } // VMS CHAPG
  }

  imagestart&=0xffff;
  bpp&=0x3f; 
  ct &=0xf;
  g.want_redbpp&=0x3f;
  g.want_greenbpp&=0x3f;
  g.want_bluebpp&=0x3f;
  g.want_blackbpp&=0x3f;

  rbpp = g.want_redbpp;
  gbpp = g.want_greenbpp;
  bbpp = g.want_bluebpp;
  kbpp = g.want_blackbpp;
  ibpp = 8*((bpp+7)>>3);    // ibpp = image bpp rounded up to next mult of 8
                            // bpp  = image bpp (can be 1 to 32)

  bpp=ibpp;
  if(format.platform==MAC) g.invertbytes=1; else g.invertbytes=0;    

  ////  End of determining image parameters

  pfac = 7.9999/(double)bpp;
  bytesperrow = (int)(xsize/pfac);
 
  ////  Allocate space

  g.tif_xsize=1;
  g.tif_ysize=1;
  // kxfac=(int)g.tif_xsize*64; // integer to speed up multiplication
  //                            // so  tif_xsize*k  ==  ((kxfac*k)>>6).
  //
  // kyfac=(int)g.tif_ysize*64; // integer to speed up multiplication
  //                            // so  yfac*k  ==  ((kyfac*k)>>6).
 
  intbppfac = ((7+bpp)/8);
  size1         = intbppfac*xsize+10;
  size2         = g.off[max(g.bitsperpixel,ibpp)]*xsize+10;
  bufsize       = max(size1,size2);      
  buffer = new uchar[bufsize+10];
  if(buffer==NULL)
  {    message(g.nomemory,ERROR); 
       goto readcustomend;
  }
  buffer[bufsize+9]=99;

  if(newimage(basefilename(filename),xstart+g.tif_xoffset, ystart+g.tif_yoffset,
       (int)(0.99999 + xsize*g.tif_xsize),
       (int)(0.99999 + ysize*g.tif_ysize),
       bpp, ct, frames, g.want_shell, 1, PERM, 1, g.window_border, 0)!=OK) 
  {    
       delete[] buffer;
       goto readcustomend;
  }   
  z[ci].originalbpp = format.defaultbpp;
  
  if(ct==COLOR) want_color=1; else want_color=0;

  ////  Read image
  forward_seek(fp, filepos, imagestart, TNI_READ); 
  for(f=0;f<frames;f++)   
  for(j=0;j<ysize;j++)   
  {
    fread(buffer, bytesperrow, 1, fp);   
    filepos += bytesperrow;
    getbits(0,buffer,0,RESET);     
    for(i=0,i2=0;i<xsize;i++,i2+=g.off[ibpp])   
    {  if(want_color==0)
       {  value = getbits(bpp,buffer,bytesperrow,format.packing);
          if(bpp<=8)                           // Filter thru current palette
          {  
             if(bpp==1) { if(value) value=g.maxcolor; }
             else
             {  dbpp = 8-bpp;                  //  if it is <= 8 bits/pixel
                if(dbpp>0) value <<= ( dbpp); else value >>= (-dbpp);
                if(ibpp>8)                     // Convert to grayscale 
                {  valuetoRGB(value,rr,gg,bb,8);  // Use current palette           
                   value = RGBvalue(rr,gg,bb,ibpp);
                }   
             }   
          }else
          {  
             dbpp = ibpp-bpp;               // If >8 bits/pixel, don't use
             if(dbpp>0) value <<= ( dbpp);  //   palette. 
             if(dbpp<0) value >>= (-dbpp);
          }
       }else
       {  rr = getbits(rbpp,buffer,bytesperrow,format.packing);
          gg = getbits(gbpp,buffer,bytesperrow,format.packing);
          bb = getbits(bbpp,buffer,bytesperrow,format.packing);
          kk= getbits(kbpp,buffer,bytesperrow,format.packing);
          dbpp = g.redbpp[ibpp]  - rbpp;
          if(dbpp>0) rr <<= ( dbpp); else  rr >>= (-dbpp);
          dbpp = g.greenbpp[ibpp]- gbpp;    
          if(dbpp>0) gg <<= ( dbpp); else  gg >>= (-dbpp);
          dbpp = g.bluebpp[ibpp] - bbpp;  
          if(dbpp>0) bb <<= ( dbpp); else  bb >>= (-dbpp);
          dbpp = g.redbpp[ibpp]  - kbpp;  
          if(dbpp>0) kk <<=( dbpp); else kk >>= (-dbpp);
          rr = max(0,min(rr,g.maxred[ibpp]));
          gg = max(0,min(gg,g.maxgreen[ibpp]));
          bb = max(0,min(bb,g.maxblue[ibpp]));
          kk= max(0,min(kk,g.maxred[ibpp]));

          if(kbpp && g.read_cmyk)
          {  mm = g.maxred[ibpp]; 
             rr = max(0,min(mm,mm-rr-kk));
             mm = g.maxgreen[ibpp]; 
             gg = max(0,min(mm,mm-gg-kk));
             mm = g.maxblue[ibpp]; 
             bb = max(0,min(mm,mm-bb-kk));
          }
          value = RGBvalue(rr,gg,bb,ibpp);   
       }                                  
       putpixelbytes(z[ci].image[f][j]+i2,value,1,ibpp,1);
    }
  }
  if(format.datatype==1) g.read_signedpixels=1;

  if(want_color)
  {   noofcolors=0;
      if(rbpp) noofcolors++;                         
      if(gbpp) noofcolors++;                         
      if(bbpp) noofcolors++;                         
      if(kbpp) noofcolors++;
      if(noofcolors) z[ci].colortype=COLOR; else z[ci].colortype=INDEXED;
  }
  z[ci].colortype = ct;
  if(format.chromakeyoff!=65535)  z[ci].chromakey = chromakey;
  if(format.ck_grayminoff!=65535) z[ci].ck_graymin = ck_graymin;
  if(format.ck_graymaxoff!=65535) z[ci].ck_graymax = ck_graymax;
  if(format.ck_minoff!=65535){    z[ci].ck_min.red = ck_min.red;
                                  z[ci].ck_min.green = ck_min.green;
                                  z[ci].ck_min.blue = ck_min.blue; 
                             }
  if(format.ck_maxoff!=65535){    z[ci].ck_max.red = ck_max.red;
                                  z[ci].ck_max.green = ck_max.green;
                                  z[ci].ck_max.blue = ck_max.blue;
                             }
  strcpy((char*)z[ci].format_name, identifier);
  z[ci].format = CUSTOM;
  if(format.fpsoff!=65535)       z[ci].fps = fps;
  if(format.paletteoff!=65535)   setpalette(palette);
  delete[] buffer;

readcustomend: 
  fclose(fp);
  return OK;
}


//--------------------------------------------------------------------------//
// aboutcustomfile                                                          //
// Prints information about custom formatted files                          //
//--------------------------------------------------------------------------//
void aboutcustomfile(char *filename, char *identifier)
{  
  char tempstring[FILENAMELENGTH];
  sprintf(tempstring, "File %s\nis a custom format image (type %s)",filename,identifier);
  message(tempstring);
}


//--------------------------------------------------------------------------//
// ushorttoa                                                                //
//--------------------------------------------------------------------------//
void ushorttoa(unsigned short i, char* a)
{
  itoa(i,a,10);
  if(!strcmp(a,"-1")) strcpy(a,"-1");
}


//--------------------------------------------------------------------------//
// initialize_format_descriptor                                             //
//--------------------------------------------------------------------------//
void initialize_format_descriptor(Format &format)
{
  strcpy(format.identifier, "aaa");
  format.headerfile[0]=0;
  format.platform          =PC; 
  format.packing           =TIF;
  format.useheader         =0;
  format.useidentifier     =1;
  format.headerbytes       =0;
  format.skipbytes         =0;
  format.compressflag      =0;
                                          // Default file offsets
  format.defaultbpp        =8; 
  format.defaultcolortype  =INDEXED; 
  format.defaultxsize      =128; 
  format.defaultysize      =128; 
  format.defaultrbpp       =0; 
  format.defaultgbpp       =0; 
  format.defaultbbpp       =0; 
  format.defaultkbpp       =0; 
  format.defaultframes     =1; 
  format.datatype          =0; 

  format.identifieroff   =65535;
  format.xoff            =65535;
  format.yoff            =65535;
  format.bppoff          =65535;
  format.colortypeoff    =65535;
  format.compressflagoff =65535;
  format.redbppoff       =65535;
  format.greenbppoff     =65535;
  format.bluebppoff      =65535;
  format.blackbppoff     =65535;
  format.xposoff         =65535;
  format.yposoff         =65535;
  format.framesoff       =65535;
  format.fpsoff          =65535;
  format.paletteoff      =65535;
  format.chromakeyoff    =65535;
  format.ck_grayminoff   =65535;
  format.ck_graymaxoff   =65535;
  format.ck_minoff       =65535;
  format.ck_maxoff       =65535;
  format.imageoff        =65535;
}



//--------------------------------------------------------------------------//
// read_format_descriptor                                                   //
// format descriptor is a text file in "name value" format.                 //
//--------------------------------------------------------------------------//
int read_format_descriptor(char *identifier, Format &format)
{
  char filename[FILENAMELENGTH];
  char tempstring[FILENAMELENGTH];
  char *name;
  char value[FILENAMELENGTH];
  FILE *fp;
  int space;

  sprintf(filename,"%s/formats/%s", g.helpdir, identifier);
  if((fp=fopen(filename,"r")) == NULL)
  {   sprintf(filename,"%s/%s", g.formatpath, identifier);
      if((fp=fopen(filename,"r")) == NULL)
      {   error_message(filename, errno);
          sprintf(tempstring,"Cant open format descriptor file (%s)",identifier);
          message(tempstring);
          return ERROR;
      }
  }
  name = new char[FILENAMELENGTH];
  while(1)
  {   fgets(name,FILENAMELENGTH,fp);
      if(feof(fp)) break;
      if(!strlen(name) || name[0]=='#') continue;
      remove_terminal_cr(name);
      space = strchr(name, ' ') - name;
      strcpy(value, name+space+1);
      strlwr(name);
      name[space]=0;
      if(!strcmp(name,"identifier")) strcpy(format.identifier, value);
      if(!strcmp(name,"headerfile")) strcpy(format.headerfile, value);

      if(!strcmp(name,"platform")) format.platform = atoi(value);
      if(!strcmp(name,"packing")) format.packing = atoi(value);
      if(!strcmp(name,"useheader")) format.useheader = atoi(value);
      if(!strcmp(name,"useidentifier")) format.useidentifier = atoi(value);
      if(!strcmp(name,"headerbytes")) format.headerbytes = atoi(value);
      if(!strcmp(name,"skipbytes")) format.skipbytes = atoi(value);
      if(!strcmp(name,"compressflag")) format.compressflag = atoi(value);
      if(!strcmp(name,"datatype")) format.datatype = atoi(value);

      if(!strcmp(name,"defaultbpp")) format.defaultbpp = atoi(value);
      if(!strcmp(name,"defaultcolortype")) format.defaultcolortype = atoi(value);
      if(!strcmp(name,"defaultxsize")) format.defaultxsize = atoi(value);
      if(!strcmp(name,"defaultysize")) format.defaultysize = atoi(value);
      if(!strcmp(name,"defaultrbpp")) format.defaultrbpp = atoi(value);
      if(!strcmp(name,"defaultgbpp")) format.defaultgbpp = atoi(value);
      if(!strcmp(name,"defaultbbpp")) format.defaultbbpp = atoi(value);
      if(!strcmp(name,"defaultkbpp")) format.defaultkbpp = atoi(value);
      if(!strcmp(name,"defaultframes")) format.defaultframes = atoi(value);

      if(!strcmp(name,"identifieroff")) format.identifieroff = atoi(value);
      if(!strcmp(name,"xoff")) format.xoff = atoi(value);
      if(!strcmp(name,"yoff")) format.yoff = atoi(value);
      if(!strcmp(name,"bppoff")) format.bppoff = atoi(value);
      if(!strcmp(name,"colortypeoff")) format.colortypeoff = atoi(value);
      if(!strcmp(name,"compressflagoff")) format.compressflagoff = atoi(value);
      if(!strcmp(name,"redbppoff")) format.redbppoff = atoi(value);
      if(!strcmp(name,"greenbppoff")) format.greenbppoff = atoi(value);
      if(!strcmp(name,"bluebppoff")) format.bluebppoff = atoi(value);
      if(!strcmp(name,"blackbppoff")) format.blackbppoff = atoi(value);
      if(!strcmp(name,"xposoff")) format.xposoff = atoi(value);
      if(!strcmp(name,"yposoff")) format.yposoff = atoi(value);
      if(!strcmp(name,"framesoff")) format.framesoff = atoi(value);
      if(!strcmp(name,"fpsoff")) format.fpsoff = atoi(value);
      if(!strcmp(name,"chromakeyoff")) format.chromakeyoff = atoi(value);
      if(!strcmp(name,"chromakey_graymin_off")) format.ck_grayminoff = atoi(value);
      if(!strcmp(name,"chromakey_graymax_off")) format.ck_graymaxoff = atoi(value);
      if(!strcmp(name,"chromakey_rgbmin_off")) format.ck_minoff = atoi(value);
      if(!strcmp(name,"chromakey_rgbmax_off")) format.ck_maxoff = atoi(value);
      if(!strcmp(name,"paletteoff")) format.paletteoff = atoi(value);
      if(!strcmp(name,"imageoff")) format.imageoff = atoi(value);
  }
  fclose(fp);
  delete[] name;
  return OK;
}


//--------------------------------------------------------------------------//
// write_format_descriptor                                                  //
//--------------------------------------------------------------------------//
void write_format_descriptor(Format &format)
{
  char filename[FILENAMELENGTH];
  char tempstring[FILENAMELENGTH];
  char ok[5];
  FILE *fp;
  
  ////  Check if format already exists
  ////  If not, put it in user's local format directory
 
  sprintf(filename, "%s/%s", g.formatpath, format.identifier);
  sprintf(tempstring,"Saving new format in\n%s",filename);
  message(tempstring);
  if(!access(filename,F_OK))    // access = 0 if file exists
  {     strcpy(ok,"y");
        sprintf(tempstring,"Replace existing format\n%s? (y/n)",format.identifier);
        if(message(tempstring,ok,QUESTION,4,61)!=YES) 
        {    message("Aborted");
             return;
        }
  }    

  if((fp=fopen(filename,"w")) == NULL)
  {     error_message(filename, errno);
        sprintf(tempstring,"Can't open format descriptor file (%s)\n",
             format.identifier);
        message(tempstring);
        return;
  }
  fprintf(fp,"# Format descriptor for %s image format\n",format.identifier);
  fprintf(fp,"# Created by %s %s on %s\n", g.appname, g.version, __DATE__);
  fprintf(fp,"identifier %s\n", format.identifier);
  fprintf(fp,"headerfile %s\n", format.headerfile);

  fprintf(fp,"platform %d\n", format.platform);
  fprintf(fp,"packing %d\n", format.packing);
  fprintf(fp,"useheader %d\n", format.useheader);
  fprintf(fp,"useidentifier %d\n", format.useidentifier);
  fprintf(fp,"headerbytes %d\n", format.headerbytes);
  fprintf(fp,"skipbytes %d\n", format.skipbytes);
  fprintf(fp,"compressflag %d\n", format.compressflag);
  fprintf(fp,"datatype %d\n", format.datatype);

  fprintf(fp,"defaultbpp %d\n", format.defaultbpp);
  fprintf(fp,"defaultcolortype %d\n", format.defaultcolortype);
  fprintf(fp,"defaultxsize %d\n", format.defaultxsize);
  fprintf(fp,"defaultysize %d\n", format.defaultysize);
  fprintf(fp,"defaultrbpp %d\n", format.defaultrbpp);
  fprintf(fp,"defaultgbpp %d\n", format.defaultgbpp);
  fprintf(fp,"defaultbbpp %d\n", format.defaultbbpp);
  fprintf(fp,"defaultkbpp %d\n", format.defaultkbpp);
  fprintf(fp,"defaultframes %d\n", format.defaultframes);

  fprintf(fp,"# Offsets into the uncompressed file:\n");
  fprintf(fp,"#   If the value is not 65535, a pointer to \n");
  fprintf(fp,"#   the specified parameter must exist in the file \n");
  fprintf(fp,"#   at an offset given by the location indicated\n");

  fprintf(fp,"identifieroff %d\n", format.identifieroff);
  fprintf(fp,"xoff %d\n", format.xoff);
  fprintf(fp,"yoff %d\n", format.yoff);
  fprintf(fp,"bppoff %d\n", format.bppoff);
  fprintf(fp,"colortypeoff %d\n", format.colortypeoff);
  fprintf(fp,"compressflagoff %d\n", format.compressflagoff);
  fprintf(fp,"redbppoff %d\n", format.redbppoff);
  fprintf(fp,"greenbppoff %d\n", format.greenbppoff);
  fprintf(fp,"bluebppoff %d\n", format.bluebppoff);
  fprintf(fp,"blackbppoff %d\n", format.blackbppoff);
  fprintf(fp,"xposoff %d\n", format.xposoff);
  fprintf(fp,"yposoff %d\n", format.yposoff);
  fprintf(fp,"framesoff %d\n", format.framesoff);
  fprintf(fp,"fpsoff %d\n", format.fpsoff);
  fprintf(fp,"chromakeyoff %d\n", format.chromakeyoff);
  fprintf(fp,"chromakey_graymin_off %d\n", format.ck_grayminoff);
  fprintf(fp,"chromakey_graymax_off %d\n", format.ck_graymaxoff);
  fprintf(fp,"chromakey_rgbmin_off %d\n", format.ck_minoff);
  fprintf(fp,"chromakey_rgbmax_off %d\n", format.ck_maxoff);
  fprintf(fp,"paletteoff %d\n", format.paletteoff);
  fprintf(fp,"imageoff %d\n", format.imageoff);

  fclose(fp);
}


//--------------------------------------------------------------------------//
// is_customfile                                                            //
// Returns:  0 if file is not a custom format file or if error              //
//           1 if a custom format filename matches identifier               //
//           2 if a custom format filename matching extension               //
//           3 if a custom format filename, when surrounded by dots (.),    //
//             matches a substring in filename (e.g., file.aaa.tar.gz would //
//             match identifier "aaa").                                     //
// Filenames in $HOME/.tnimage/formats are assumed to be the same as the    //
//   identifier or extension. Matching identifier takes precedence.         //
// First check if identifier matches a filename in formats directory, then  //
//   check if extension matches one. If this fails, try stripping off the   //
//   extension and check again, on the possibility it might be compressed.  //
// Puts matching custom format name in 'match'.                             //
//--------------------------------------------------------------------------//
int is_customfile(char *filename, char *identifier, char *extension, char *match)
{ 
  DIR *dir;
  struct dirent *d;
  int match_identifier = 0;
  int match_extension = 0;
  int match_contain = 0;
  char tempstring[FILENAMELENGTH];
  char dirtempstring[FILENAMELENGTH];
  match[0] = 0;

  sprintf(dirtempstring,"%s/formats", g.helpdir);

  if((dir = opendir(dirtempstring)) == NULL)
      message("Cant open formats directory"); 
  else 
  {   while((d = readdir(dir)) != NULL)
      {   if(!strcmp(d->d_name, ".") || !strcmp(d->d_name, "..")) continue;
          if(identifier && !strcmp(d->d_name, identifier))
          {    match_identifier=1; 
               strncpy(match,d->d_name,63); 
               break; 
          }
          if(extension && !strcmp(d->d_name, extension))
          {    match_extension=1;
               strncpy(match,d->d_name,63); 
          }
          sprintf(tempstring, ".%s.",d->d_name);   //// e.g.,   .aaa.
          if(strstr(filename, tempstring)) 
          {    match_contain=1; 
               strncpy(match,d->d_name,63); 
          }      
      }
      if(closedir(dir) < 0) message("Cant close directory!");
  }

  if((dir = opendir(g.formatpath)) != NULL)
  {   while((d = readdir(dir)) != NULL)
      {   
          if(!strcmp(d->d_name, ".") || !strcmp(d->d_name, "..")) continue;
          if(identifier && !strcmp(d->d_name, identifier))
          {    match_identifier=1; 
               strncpy(match,d->d_name,63); 
               break; 
          }
          if(extension && !strcmp(d->d_name, extension))
          {    match_extension=1;
               strncpy(match,d->d_name,63); 
          }
          sprintf(tempstring, ".%s.",d->d_name);   //// e.g.,   .aaa.
          if(strstr(filename, tempstring)) 
          {    match_contain=1; 
               strncpy(match,d->d_name,63); 
          }      
      }
      if(closedir(dir) < 0) message("Cant close directory!");
  }

  if(match_identifier) return 1;  // Most preferred result
  if(match_extension) return 2;
  if(match_contain) return 3;     // Least preferred result
  return 0;
}


//--------------------------------------------------------------------------//
// count_custom_formats                                                     //
//--------------------------------------------------------------------------//
int count_custom_formats(void)
{
  DIR *dir;
  struct dirent *d;
  int count=0;
  char dirtempstring[FILENAMELENGTH];

  sprintf(dirtempstring,"%s/formats", g.helpdir);
  if((dir = opendir(dirtempstring)) == NULL)
      message("Cant open formats directory"); 
  else 
  {   while((d = readdir(dir)) != NULL)
      {   if(!strcmp(d->d_name, ".") || !strcmp(d->d_name, "..")) continue;
          count++;
      }
      if(closedir(dir) < 0) message("Oh Cripes!!!\n cant even close directory!");
  }
  if((dir = opendir(g.formatpath)) != NULL)
  {   while((d = readdir(dir)) != NULL)
      {   if(!strcmp(d->d_name, ".") || !strcmp(d->d_name, "..")) continue;
          count++;
      }
      if(closedir(dir) < 0) message("Cant close directory!");
  }
  return count;
}


//--------------------------------------------------------------------------//
// read_custom_format_list                                                  //
//--------------------------------------------------------------------------//
void read_custom_format_list(char **custom_format_list)
{
  DIR *dir;
  struct dirent *d;
  int count=0;
  char dirtempstring[FILENAMELENGTH];

  sprintf(dirtempstring,"%s/formats", g.helpdir);
  if((dir = opendir(dirtempstring)) == NULL)
      message("Cant open formats directory"); 
  else
  {   while((d = readdir(dir)) != NULL)
      {   if(!strcmp(d->d_name, ".") || !strcmp(d->d_name, "..")) continue;
          strncpy(custom_format_list[count++], d->d_name, 63);
      }
      if(closedir(dir) < 0) message("Cant close directory!");
  }
  
  if((dir = opendir(g.formatpath)) == NULL)
      message("Cant open formats directory"); 
  else
  {   while((d = readdir(dir)) != NULL)
      {   if(!strcmp(d->d_name, ".") || !strcmp(d->d_name, "..")) continue;
          strncpy(custom_format_list[count++], d->d_name, 63);
      }
      if(closedir(dir) < 0) message("Cant close directory!");
  }
  return;
}


//--------------------------------------------------------------------------//
// list_match                                                               //
//--------------------------------------------------------------------------//
int list_match(char **list, char *string, int n)
{
    int k=0;
    for(k=0;k<n;k++) if(!strcmp(string,list[k])) return k;
    return 0;
}



//--------------------------------------------------------------------------//
//  compress_file                                                           //
//--------------------------------------------------------------------------//
int compress_file(char *filename, char *compressor, char *ext)
{
   int status=0;
   char tempstring[FILENAMELENGTH];
   char newfilename[FILENAMELENGTH];
   if(access(compressor,X_OK))
   {    sprintf(tempstring, "Cannot execute %s\nfile was not compressed",compressor); 
        message(tempstring);
        return NOTFOUND;
   }
   if(!strlen(filename)){ message("No filename - cant compress file"); return ERROR; }
   if(!strlen(compressor)){ message("No compression utility specified"); return ERROR; }
   if(!strlen(ext)){ message("No compression extension specified"); return ERROR; }
   sprintf(tempstring, "%s %s", compressor, filename);
   sprintf(newfilename, "%s%s", filename, ext);
   status = system(tempstring);
   if(status==127 || status == -1)
        sprintf(tempstring, "Error executing /bin/sh -c %s %s",compressor,filename);  
   if(!access(newfilename,F_OK))  // File exists
        sprintf(tempstring, 
        "File successfully compressed by\n%s\nFilename changed to\n%s",
        compressor, newfilename);
   else strcpy(tempstring,"Error compressing image file");
   message(tempstring);
   return status;   
}



//--------------------------------------------------------------------------//
// open_file                                                                //
//--------------------------------------------------------------------------//
FILE *open_file(char *filename, int &compress, int mode, char *compressor, 
    char *decompressor, char *ext)
{
   int k,access_error=0;
   FILE *fp;
   char tempstring[FILENAMELENGTH];  
   char modestring[16];
   char program[64];
   if(mode==TNI_WRITE)
   {   strcpy(modestring, "w");
       strncpy(program, compressor, 64);
   }else   // read
   {   strcpy(modestring,"r");
       strncpy(program, decompressor, 64);
   }
   for(k=0;k<(int)strlen(program);k++) if(program[k]==' ') program[k]=0;

   if(!strlen(program)) compress = 0;
   if(compress && access(program, X_OK))
   {   sprintf(tempstring, "Cannot execute %s", program); 
       message(tempstring);
       return (FILE*)NULL;     
   }

   if(compress)
   {  
       if(mode==TNI_WRITE)
            sprintf(tempstring, "%s %c %s%s", compressor, '>', filename, ext);       
       else
            sprintf(tempstring, "%s %s", decompressor, filename);       
       if((fp = (FILE*)popen(tempstring, modestring)) == NULL) // A pipe
       {   
            error_message(filename, errno);
            message("Popen error!",ERROR); 
            return (FILE*)NULL; 
       }

       //// popen incorrectly gives a fp even if user specifies an 
       //// invalid path, then crashes, so check with access().
       
       strcpy(tempstring, filename);       
       for(k=strlen(tempstring); k; k--) 
          if(tempstring[k]=='/'){ tempstring[k+1]=0; break; }
       if(mode==TNI_READ)  access_error = access(tempstring, R_OK);
       if(mode==TNI_WRITE) access_error = access(tempstring, W_OK);
       if(access_error != 0)
       {    error_message(tempstring, errno);
            return (FILE*)NULL; 
       }
   }else
   {   if((fp=fopen(filename, modestring)) == NULL)
       {   error_message(filename, errno);
           return (FILE*)NULL; 
       }
   }
   return fp;
}


//--------------------------------------------------------------------------//
// close_file                                                               //
//--------------------------------------------------------------------------//
int close_file(FILE *fp, int compress)
{
   if(compress) return pclose(fp);
   else return fclose(fp);   
}
