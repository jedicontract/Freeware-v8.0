//--------------------------------------------------------------------------//
// xmtnimage22.cc                                                           //
// Image file printing                                                      //
// Latest revision: 03-11-2003                                              //
// Copyright (C) 2003 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
extern PrinterStruct printer;
int in_printimage=0;
int hit16=0;

//--------------------------------------------------------------------------//
// printimage                                                               //
//--------------------------------------------------------------------------//
void printimage(void)
{
  if(memorylessthan(16384)){ message(g.nomemory,ERROR); return; }
  int j,k;;
  static Dialog *dialog;
  static int selection10, selection11;
  if(in_printimage) return;
  dialog = new Dialog;
  if(dialog==NULL){ message(g.nomemory); return; }
  g.getout=0;
  char temp[202];
  in_printimage = 1;
  create_print_filename(ci, printer.filename);
  
  ////  radios

  if(g.selectedimage==-1) printer.entire=0; else printer.entire=1; 
  strcpy(dialog->title,"Print Image");
  strcpy(dialog->radio[0][0],"Orientation");
  strcpy(dialog->radio[0][1],"Vertical");
  strcpy(dialog->radio[0][2],"Horizontal");
  strcpy(dialog->radio[1][0],"Image");             
  strcpy(dialog->radio[1][1],"Positive");
  strcpy(dialog->radio[1][2],"Negative");
  strcpy(dialog->radio[2][0],"Print what");             
  strcpy(dialog->radio[2][1],"Print entire image");
  strcpy(dialog->radio[2][2],"Print selected area");
  strcpy(dialog->radio[3][0],"Color type");             
  strcpy(dialog->radio[3][1],"Grayscale");
  strcpy(dialog->radio[3][2],"RGB/Indexed color");
  strcpy(dialog->radio[3][3],"CMY color");
  strcpy(dialog->radio[3][4],"CMYK color");
  strcpy(dialog->radio[4][0],"Printer Type");             
  strcpy(dialog->radio[4][1],"PCL");
  strcpy(dialog->radio[4][2],"Postscript");

  if(printer.vertical) dialog->radioset[0]=1;
                  else dialog->radioset[0]=2;
  if(printer.positive) dialog->radioset[1]=1;
                  else dialog->radioset[1]=2;
  if(printer.entire)   dialog->radioset[2]=1;
                  else dialog->radioset[2]=2;
  switch(z[ci].colortype)
  {    case GRAY:
       case GRAPH: printer.palette = 1; break;
       case COLOR: 
       case INDEXED: printer.palette = 2; break;
       case CMYK: printer.palette = 4; break;
  }
  dialog->radioset[3] = printer.palette;

  if(printer.language==1)dialog->radioset[4]=1;
                    else dialog->radioset[4]=2;

  dialog->radiono[0]=3;
  dialog->radiono[1]=3;
  dialog->radiono[2]=3;
  dialog->radiono[3]=5;
  dialog->radiono[4]=3;

  for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
 
  ////  Boxes
  
  for(k=0;k<DCOUNT;k++)dialog->boxset[k]=0;
  strcpy(dialog->boxes[0],"Print parameters");
  strcpy(dialog->boxes[1],"Printer command");
  strcpy(dialog->boxes[2],"Printer device");
  strcpy(dialog->boxes[3],"Print to file");
  strcpy(dialog->boxes[4],"No.of copies");
  strcpy(dialog->boxes[5],"Color/Darkness");
  strcpy(dialog->boxes[6],"Horiz. offset (in.)");
  strcpy(dialog->boxes[7],"Vert. offset (in.)");

  strcpy(dialog->boxes[8],"PCL parameters");
  strcpy(dialog->boxes[9],"Printer res.(DPI)");
  strcpy(dialog->boxes[10],"Dither size");
  strcpy(dialog->boxes[11],"Print quality");
  strcpy(dialog->boxes[12],"Media type");
  strcpy(dialog->boxes[13],"Depletion");
  strcpy(dialog->boxes[14],"Printer gray balance");

  strcpy(dialog->boxes[15],"PostScript parameters");
  strcpy(dialog->boxes[16],"Horiz.image size");
  strcpy(dialog->boxes[17],"Image aspect(V/H)");
  strcpy(dialog->boxes[18],"Horiz.paper size");
  strcpy(dialog->boxes[19],"Vert. paper size");
  strcpy(dialog->boxes[20],"Landscape");
  strcpy(dialog->boxes[21],"Interpolate");


  dialog->boxtype[0]=LABEL; 
  dialog->boxtype[1]=TOGGLESTRING; 
  dialog->boxtype[2]=TOGGLESTRING;
  dialog->boxtype[3]=TOGGLESTRING;
  dialog->boxtype[4]=STRING;
  dialog->boxtype[5]=RGBCLICKBOX;
  dialog->boxtype[6]=STRING;
  dialog->boxtype[7]=STRING;
  dialog->boxtype[8]=LABEL;
  dialog->boxtype[9]=STRING; 
  dialog->boxtype[10]=NON_EDIT_LIST; 
  dialog->boxtype[11]=NON_EDIT_LIST;
  dialog->boxtype[12]=NON_EDIT_LIST;
  dialog->boxtype[13]=NON_EDIT_LIST;
  dialog->boxtype[14]=TOGGLE; 
  dialog->boxtype[15]=LABEL; 
  dialog->boxtype[16]=STRING; 
  dialog->boxtype[17]=STRING;
  dialog->boxtype[18]=STRING; 
  dialog->boxtype[19]=STRING;
  dialog->boxtype[20]=TOGGLE; 
  dialog->boxtype[21]=TOGGLE;

  for(k=1;k<=3;k++) dialog->boxset[printer.device] = 0;
  dialog->boxset[printer.device] = 1;
  dialog->boxmin[5]=0; dialog->boxmax[5]=256;
  dialog->boxmin[10]=0; // min menu selection
  dialog->boxmax[10]=5; // max no.of menu items to use
  dialog->boxmin[11]=0; // min menu selection
  dialog->boxmax[11]=3; // max no.of menu items to use
  dialog->boxmin[12]=0; // min menu selection
  dialog->boxmax[12]=5; // max no.of menu items to use
  dialog->boxmin[13]=0; // min menu selection
  dialog->boxmax[13]=6; // max no.of menu items to use

  strcpy(dialog->answer[1][1], printer.command);
  strcpy(dialog->answer[2][1], printer.devicename);
  strcpy(dialog->answer[3][1], printer.filename);
  sprintf(dialog->answer[4][0],"%d", printer.copies);
  printer.intensity = RGBvalue((int)(printer.rfac*128),
                      (int)(printer.gfac*128),(int)(printer.bfac*128),24);
  itoa(printer.intensity,temp,10); strcpy(dialog->answer[5][0], temp);
  doubletostring(printer.hpos,g.signif,temp);strcpy(dialog->answer[6][0], temp);
  doubletostring(printer.vpos,g.signif,temp);strcpy(dialog->answer[7][0], temp);
  itoa(printer.res,temp,10);       strcpy(dialog->answer[9][0], temp);

  ////  Dither size menu

  dialog->l[10]       = new listinfo;
  dialog->l[10]->title = new char[100];
  dialog->l[10]->info  = new char*[5];
  dialog->l[10]->count = 5;
  dialog->l[10]->wantfunction = 0;
  dialog->l[10]->f1    = null;
  dialog->l[10]->f2    = null;
  dialog->l[10]->f3    = null;
  dialog->l[10]->f4    = null; // dialog lists are deleted in dialogcancelcb
  dialog->l[10]->f5    = null;
  dialog->l[10]->f6    = null;
  dialog->l[10]->listnumber = 0;
  for(k=0; k<5; k++) dialog->l[10]->info[k] = new char[100];
  if(printer.dither==1) selection10 = 0;
  if(printer.dither==2) selection10 = 1;
  if(printer.dither==4) selection10 = 2;
  if(printer.dither==8) selection10 = 3;
  if(printer.dither==16)selection10 = 4;
  dialog->l[10]->selection = &selection10;
  strcpy(dialog->l[10]->title,  "Dither size");
  strcpy(dialog->l[10]->info[0],"1x1");
  strcpy(dialog->l[10]->info[1],"2x2");
  strcpy(dialog->l[10]->info[2],"4x4");
  strcpy(dialog->l[10]->info[3],"8x8");
  strcpy(dialog->l[10]->info[4],"16x16");

  // print quality menu

  dialog->l[11]       = new listinfo;
  dialog->l[11]->title = new char[100];
  dialog->l[11]->info  = new char*[3];
  dialog->l[11]->count = 3;
  dialog->l[11]->wantfunction = 0;
  dialog->l[11]->f1    = null;
  dialog->l[11]->f2    = null;
  dialog->l[11]->f3    = null;
  dialog->l[11]->f4    = null; // dialog lists are deleted in dialogcancelcb
  dialog->l[11]->f5    = null;
  dialog->l[11]->f6    = null;
  dialog->l[11]->listnumber = 0;
  for(k=0; k<3; k++) dialog->l[11]->info[k] = new char[100];
  if(printer.quality==-1) selection11 = 0;
  if(printer.quality== 0) selection11 = 1;
  if(printer.quality== 1) selection11 = 2;
  dialog->l[11]->selection = &selection11;
  strcpy(dialog->l[11]->title,  "Print Quality");
  strcpy(dialog->l[11]->info[0],"Draft       ");
  strcpy(dialog->l[11]->info[1],"Normal      ");
  strcpy(dialog->l[11]->info[2],"Presentation");

  // Media type menu

  dialog->l[12]       = new listinfo;
  dialog->l[12]->title = new char[100];
  dialog->l[12]->info  = new char*[5];
  dialog->l[12]->count = 5;
  dialog->l[12]->wantfunction = 0;
  dialog->l[12]->f1    = null;
  dialog->l[12]->f2    = null;
  dialog->l[12]->f3    = null;
  dialog->l[12]->f4    = null; // dialog lists are deleted in dialogcancelcb
  dialog->l[12]->f5    = null;
  dialog->l[12]->f6    = null;
  dialog->l[12]->listnumber = 0;
  for(k=0; k<5; k++) dialog->l[12]->info[k] = new char[100];
  dialog->l[12]->selection = &printer.papertype;
  strcpy(dialog->l[12]->title,   "Media type ");
  strcpy(dialog->l[12]->info[0], "Plain paper      ");
  strcpy(dialog->l[12]->info[1], "Bond paper       ");
  strcpy(dialog->l[12]->info[2], "Premier paper    ");
  strcpy(dialog->l[12]->info[3], "Glossy film      ");
  strcpy(dialog->l[12]->info[4], "Transparency film");

  // depletion menu

  dialog->l[13]       = new listinfo;
  dialog->l[13]->title = new char[100];
  dialog->l[13]->info  = new char*[6];
  dialog->l[13]->count = 6;
  dialog->l[13]->wantfunction = 0;
  dialog->l[13]->f1    = null;
  dialog->l[13]->f2    = null;
  dialog->l[13]->f3    = null;
  dialog->l[13]->f4    = null; // dialog lists are deleted in dialogcancelcb
  dialog->l[13]->f5    = null;
  dialog->l[13]->f6    = null;
  dialog->l[13]->listnumber = 0;
  for(k=0; k<6; k++) dialog->l[13]->info[k] = new char[100];
  dialog->l[13]->selection = &printer.depletion;
  strcpy(dialog->l[13]->title, "Depletion ");
  strcpy(dialog->l[13]->info[0], "Printer default");
  strcpy(dialog->l[13]->info[1],"Off            ");
  strcpy(dialog->l[13]->info[2],"25%            ");
  strcpy(dialog->l[13]->info[3],"50%            ");
  strcpy(dialog->l[13]->info[4],"25%/gamma corr.");
  strcpy(dialog->l[13]->info[5],"50%/gamma corr.");

  if(printer.graybalance==1)dialog->boxset[14]=1;
 
  doubletostring(printer.hsize,g.signif,temp); strcpy(dialog->answer[16][0],temp);
  doubletostring(printer.ratio,g.signif,temp); strcpy(dialog->answer[17][0],temp);
  doubletostring(printer.paperxsize,g.signif,temp); strcpy(dialog->answer[18][0],temp);
  doubletostring(printer.paperysize,g.signif,temp); strcpy(dialog->answer[19][0],temp);
  dialog->boxset[20] = printer.rotation;
  if(printer.interpolate==1) dialog->boxset[21]=1;
 
  dialog->noofradios=5;
  dialog->noofboxes=22;
  dialog->helptopic=4;  
  dialog->bpp = 24;
  dialog->want_changecicb = 1;
  dialog->f1 = printcheck;
  dialog->f2 = null;
  dialog->f3 = null;
  dialog->f4 = printfinish;
  dialog->f5 = null;
  dialog->f6 = null;
  dialog->f7 = null;
  dialog->f8 = null;
  dialog->f9 = null;
  dialog->width = 440;
  dialog->height = 0; // calculate automatically
  dialog->transient = 1;
  dialog->wantraise = 0;
  dialog->radiousexy = 0;
  dialog->boxusexy = 0;
  strcpy(dialog->path,".");
  g.getout=0;
  dialog->message[0] = 0;      
  dialog->busy = 0;
  dialogbox(dialog);
  return;
}


//--------------------------------------------------------------------------//
//  printcheck                                                              //
//--------------------------------------------------------------------------//
void printcheck(dialoginfo *a, int radio, int box, int boxbutton)
{
  static int oci=-1, oxsize=-1, oysize=-1;
  int j,k,len,status=OK,rr,gg,bb,xsize=0,ysize=0;
  double xinches, yinches, imageratio=1.0, paperratio=1.0, hpaper, vpaper, dtemp;
  char tempstring[128];
  g.busy++;
  if(a->radioset[0]==1) printer.vertical = 1; else printer.vertical = 0;   
  if(a->radioset[1]==1) printer.positive = 1; else printer.positive = 0;   
  int oprinter_entire = printer.entire;
  if(a->radioset[2]==1) printer.entire   = 1; else printer.entire   = 0; 
  if(printer.entire != oprinter_entire)
  {   switch(z[ci].colortype)
      {    case GRAY:
           case GRAPH: printer.palette = 1; break;
           case COLOR: 
           case INDEXED: printer.palette = 2; break;
           case CMYK: printer.palette = 4; break;
      }
  }else printer.palette = a->radioset[3];
  if(a->radioset[4]==1) printer.language = 1; else printer.language = 0; 

  for(k=1; k<=3; k++) if(a->boxset[k]) printer.device = k;
  strcpy(printer.command, a->answer[1][1]);
  strcpy(printer.devicename, a->answer[2][1]);
  strcpy(printer.filename, a->answer[3][1]);
  printer.copies = atoi(a->answer[4][0]);
  printer.intensity = atoi(a->answer[5][0]);
  valuetoRGB(printer.intensity,rr,gg,bb,24);

  printer.rfac = (double)rr/128.0;
  printer.gfac = (double)gg/128.0;
  printer.bfac = (double)bb/128.0;
  printer.ifac = (printer.rfac + printer.gfac + printer.bfac)/3;
  printer.hpos = atof(a->answer[6][0]);
  printer.vpos = atof(a->answer[7][0]);
  printer.res = atoi(a->answer[9][0]);

  if(*a->l[10]->selection==0) printer.dither = 1;
  if(*a->l[10]->selection==1) printer.dither = 2;
  if(*a->l[10]->selection==2) printer.dither = 4;
  if(*a->l[10]->selection==3) printer.dither = 8;
  if(*a->l[10]->selection==4) printer.dither = 16;

  if(*a->l[11]->selection==0) printer.quality = -1;
  if(*a->l[11]->selection==1) printer.quality =  0;
  if(*a->l[11]->selection==2) printer.quality =  1;

  printer.papertype = *a->l[12]->selection;
  printer.depletion = *a->l[13]->selection;
  if(a->boxset[14]) printer.graybalance=1; else printer.graybalance=0;
  
  dtemp = atof(a->answer[16][0]);
  printer.hsize = dtemp;
  printer.ratio = atof(a->answer[17][0]);
  printer.paperxsize = atof(a->answer[18][0]);
  printer.paperysize = atof(a->answer[19][0]);
  printer.rotation = a->boxset[20];

  //// If image changes or margin, aspect, or paper size change
  //// Rescale to fit into paper
  
  if(ci>=0){ xsize = z[ci].xsize; ysize = z[ci].ysize; }
  if(!hit16 && (ci != oci || box==6 || box==7 || box==20 || between(box, 17,20) ||
                xsize !=oxsize || ysize != oysize))
  {  oci = ci;
     oxsize = xsize;
     oysize = ysize;
     hpaper = printer.paperxsize - 2 * printer.hpos;
     vpaper = printer.paperysize - 2 * printer.vpos;

     if(hpaper) paperratio = vpaper / hpaper;
     if(printer.rotation) 
         imageratio = printer.ratio * (double)z[ci].xsize / (double)z[ci].ysize;
     else
         imageratio = printer.ratio * (double)z[ci].ysize / (double)z[ci].xsize;

     if(imageratio < paperratio)
     {    xinches = hpaper;
          yinches = hpaper * imageratio;
     }else
     {    yinches = vpaper;
          xinches = vpaper / imageratio;
     }
     doubletostring(xinches, g.signif, tempstring);  
     strcpy(a->answer[16][0], tempstring);
     set_widget_string(a->boxwidget[16][0], tempstring);
     printer.xsize = xinches;
     printer.ysize = yinches;
  }

  if(a->boxset[21]) printer.interpolate=1; else printer.interpolate=0;

  if(boxbutton==0 && between(box,1,3))                  // Printer device
  { 
     for(k=1;k<=3;k++) a->boxset[k]=False;
     for(k=1;k<=3;k++) if(k==box) a->boxset[k]=True;  
     for(k=1;k<=3;k++) if(k!=box)
         XmToggleButtonSetState(a->boxwidget[k][0],a->boxset[k],False);
  }
  if(radio==4 ||(radio==0 && box==0))   // PCL/PS parameters
  {
       if(a->radioset[4]==1)            // Switch to PCL
       {    printer.language=PCL;
            create_print_filename(ci, printer.filename);
            if(a->boxwidget[3][1]) 
            {   len = strlen(printer.filename);
                XtVaSetValues(a->boxwidget[3][1], XmNvalue, printer.filename, 
                       XmNcursorPosition, len,NULL); 
            }
            for(k=0;k<=2;k++) 
            {   for(j=8;j<=14;j++) 
                    if(a->boxwidget[j][k]) XtSetSensitive(a->boxwidget[j][k],True);
                for(j=15;j<=21;j++) 
                    if(a->boxwidget[j][k]) XtSetSensitive(a->boxwidget[j][k],False);
            }
       }else if(a->radioset[4]==2)      // Switch to Postscript
       {    printer.language=POSTSCRIPT;
            create_print_filename(ci, printer.filename);
            {   len = strlen(printer.filename);
                XtVaSetValues(a->boxwidget[3][1], XmNvalue, printer.filename, 
                      XmNcursorPosition, len,NULL); 
            }
            for(k=0;k<=2;k++) 
            {   for(j=8;j<=14;j++) 
                    if(a->boxwidget[j][k]) XtSetSensitive(a->boxwidget[j][k],False);
                for(j=15;j<=21;j++) 
                    if(a->boxwidget[j][k]) XtSetSensitive(a->boxwidget[j][k],True);
            }
       }
  }
  if(!g.getout && radio==-2)            // OK button clicked
  {    drawselectbox(OFF);
       if(printer.language==PCL) status = write_pcl_file(printer);
       else status = write_postscript_file(printer);
       status_error_message(status);
  }
  g.getout=0;
  g.state=NORMAL;
  g.busy--;
  return;
}


//--------------------------------------------------------------------------//
//  printfinish                                                             //
//--------------------------------------------------------------------------//
void printfinish(dialoginfo *a)
{
  a=a;
  in_printimage = 0;
  hit16 = 0;
}


//--------------------------------------------------------------------------//
//  write_pcl_file                                                          //
//--------------------------------------------------------------------------//
int write_pcl_file(PrinterStruct p)
{
  static PromptStruct ps;
  static char filename[FILENAMELENGTH];
  ps.filename = filename;
  ps.f1 = write_pcl_file_part2;
  ps.f2 = null;
  ps.ptr = &p;
  switch(p.device)
  {   case 1: strcpy(filename, "/tmp/~temp.print");  // system() to lpr
              write_pcl_file_part2(&ps);
              break;
      case 2: strcpy(filename, p.devicename);        // directly-attached printer
              write_pcl_file_part2(&ps);
              break;
      case 3: strcpy(filename, p.filename);          // print to file
              check_overwrite_file(&ps);
              break;
  }
  return OK;
}



//--------------------------------------------------------------------------//
//  write_pcl_file_part2                                                    //
//--------------------------------------------------------------------------//
void write_pcl_file_part2(PromptStruct *ps)
{
  char *filename = ps->filename;
  PrinterStruct p = printer;  // ps->ptr is corrupted at this point
  Widget www, scrollbar;
  int status = OK;
  int ncolors=1;
  int b=0, color, oldb;
  int alloc1=0;
  int ink=0;
  double ratio;
  FILE *p_stream;
  char tempstring[FILENAMELENGTH];
 
  int bitshift=0,i,ii,j,k,k2,pixbpp,ulx,uly,lrx,lry,shingling=0;
  int want_check=0;

  uchar *buffer_1d;
  int bufx,bufy,bufz=0;
  uchar ***buffer=NULL;               /* Temporary storage of bit map data */
                                      /* for one row of pixels, with up to */
                                      /* 16x16 dithering. One for each ink.*/
                             
  ushort map[256][16];                /* 256 bit maps for each shade of    */
                                      /* gray - assume 16-bit short ints   */
                                      /* so it is maximum of 16x16 dither  */
                                      
                                      /* The bit maps are made by filling  */
                                      /* the box cumulatively 1 pixel at a */
                                      /* time in the order given by the    */
                                      /* fill4, fill8 and fill16 arrays.   */
                                      
  uchar fill2[2][2] = { { 0,1 },
                        { 2,3 }};

  uchar fill4[4][4] = { {  5,11,  7,15 },
                        { 13, 1,  9, 4 },
                        {  8,10,  6,14 },
                        {  0, 3, 12, 2 }};

  uchar fill8[8][8] = { { 32,56,27, 0, 31,53,28,63 },
                        { 40, 1,48,15, 38, 4,45,14 },
                        { 21,52,17,43, 23,49,19,41 },
                        { 60, 9,33, 5, 57,11,34, 8 },

                        { 29,55,25,61, 30,54,26,62 },
                        { 39, 3,47,13, 37, 2,46,16 },
                        { 24,51,20,44, 22,50,18,42 },
                        { 59,12,35, 7, 58,10,36, 6 } };

  uchar fill16[16][16] =\
    { {   5,133, 37,167, 13,143, 53,183,   7,135, 39,169, 15,145, 55,185 }, 
      { 199, 97,227, 81,211,121,  0, 69, 201, 99,229, 83,213,123,253, 71 },
      {  57,187, 21,151, 45,175, 29,139,  59,189, 23,153, 47,177, 31,161 },
      { 243, 73,203,109,239, 85,215,113, 245, 75,205,111,241, 87,217,115 },
      {   9,139, 61,191,  1,129, 33,163,  11,141, 63,193,  3,131, 35,165 },
      { 231,105,235,125,255, 65,195,101, 233,107,237,127,251, 67,197,103 },
      {  41,171, 25,155, 49,179, 17,147,  43,173, 27,157, 51,181, 19,149 },
      { 219, 77,207,117,247, 93,223, 89, 221, 79,209,119,249, 95,235, 91 },

      {   8,136, 40,170, 16,146, 56,186,   6,134, 38,168, 14,144, 54,184 },
      { 202,100,230, 84,214,124,254, 72, 200, 98,228, 82,212,122,252, 70 },
      {  60,190, 24,154, 48,178, 32,162,  58,188, 22,152, 46,176, 30,160 },
      { 246, 76,206,112,242, 88,218,116, 244, 74,204,110,240, 86,216,114 },
      {  12,142, 64,184,  4,132, 36,166,  10,140, 62,192,  2,130, 34,164 },
      { 234,108,238,128,137, 68,198,104, 232,106,236,126,138, 66,196,102 },
      {  44,174, 28,158, 52,182, 20,150,  42,172, 26,156, 50,180, 18,148 },
      { 222, 80,210,120,250, 96,226, 92, 220, 78,208,118,248, 94,224, 90 } };
  

    //------End of declarations-----------//
    //------Set bitmap data---------------// 
  
  g.state=PRINTING;
  printstatus(PRINTING);

 
 if(p.entire && ci>=0 && ci<g.image_count)
 {    ulx = z[ci].xpos;
      uly = z[ci].ypos;
      lrx = z[ci].xpos + z[ci].xsize;
      lry = z[ci].ypos + z[ci].ysize;
  }      
  else
  {   ulx = g.selected_ulx;
      uly = g.selected_uly;
      lrx = g.selected_lrx;
      lry = g.selected_lry;
  }   

   switch(p.dither)            /* Create bitmaps up to 16x16 */
   { 
     case 1: break; 
     case 2:
         for(k=0;k<4;k++)          /* color k */
         for(j=0;j<2;j++)          /* y pixel */
         {  map[k][j] = 0;
            for(i=0;i<2;i++)       /* x pixel */
                if(fill2[i][j]<=k) map[k][j]+=(1<<(1-i));
         }      
         break;
     case 4:
         for(k=0;k<16;k++)         /* color k */
         for(j=0;j<4;j++)          /* y pixel */
         {  map[k][j] = 0;
            for(i=0;i<4;i++)       /* x pixel */
                if(fill4[i][j]<=k) map[k][j]+=(1<<(3-i));
         }      
         break;
      case 8:
         for(k=0;k<64;k++)         /* color k */
         for(j=0;j<8;j++)          /* y pixel */
         {  map[k][j] = 0; 
            for(i=0;i<8;i++)       /* x pixel */
                if(fill8[i][j]<=k) map[k][j]+=(1<<(7-i));
         }   
         break;
      case 16:
         for(k=0;k<256;k++)        /* color k */
         for(j=0;j<16;j++)         /* y pixel */
         {  map[k][j] = 0;
            for(i=0;i<16;i++)      /* x pixel */
                if(fill16[i][j]<=k) map[k][j]+=(1<<(15-i));
         }   
         break;
   }

   //-----------End of initializing data------------//
   //-----------Open printer file-------------------//
         
   p_stream = fopen(filename,"w");
   if(p_stream==NULL) 
   {    error_message(filename, errno);
        if(p.device==1) sprintf(tempstring,"Can't create temporary file \n%s",filename);
        if(p.device==2) sprintf(tempstring,"Unable to access\n%s",p.devicename);
        if(p.device==3) sprintf(tempstring,"Can't open file\n%s",p.filename);
        message(tempstring,ERROR);
        status=ERROR;
        goto endprint;
   }

#ifdef MSDOS

   if(isatty(fileno(p_stream))) want_check=1;
   if(want_check) if(check_printer()==ABORT) goto endprint;

#endif

   //-----------Send data to printer-----------------//
 
   fprintf(p_stream,"%cE\n",27);                  /* Esc-E reset           */
   if(p.vertical)      
      fprintf(p_stream,"%c&l0O\n",27);            /* portrait mode         */
   else
      fprintf(p_stream,"%c&l1O\n",27);            /* landscape mode        */ 
   fprintf(p_stream,"%c*rC\n",27);                /* End raster graphics   */  

   fprintf(p_stream,"%c*t%dR\n",27,p.res);        /* set resolution  */
   fprintf(p_stream,"%c&l%dX\n",27,p.copies);     /* no.of copies    */

   if(p.quality==-1) shingling=0;
   if(p.quality== 0) shingling=1;
   if(p.quality== 1) shingling=2;
   if(p.papertype>=3) shingling=min(2,1+shingling);
   if(shingling)
     fprintf(p_stream,"%c*o%dQ",27,shingling);    /* "shingling"        */

   if(p.depletion)
     fprintf(p_stream,"%c*o%dD\n",27,p.depletion);/* "depletion" */

   fprintf(p_stream,"%c&l%dM\n",27,p.papertype);  /* paper type      */
   if(p.quality==-1)
     fprintf(p_stream,"%c*r1Q\n",27);             /* print quality-others */
   else
     fprintf(p_stream,"%c*r2Q\n",27);             /* print quality-others */
   fprintf(p_stream,"%c*o%dM\n",27,p.quality);    /* prnt quality-H/P540 */

   switch(p.palette)
   {  case 1:  ncolors=1;     // Single plane palette
               fprintf(p_stream,"%c*r1U\n",27);
               break; 
      case 2:  ncolors=3;     // RGB
               fprintf(p_stream,"%c*r3U\n",27);
               break;
      case 3:  ncolors=3;     // CMY
               fprintf(p_stream,"%c*r-3U\n",27);
               break; 
      case 4:  ncolors=4;     // KCMY
               fprintf(p_stream,"%c*r-4U\n",27);
               break;
   }
   if(p.graybalance) fprintf(p_stream,"%c*b1B",27);   /* gray balance*/

   print((char*)"Printing...", 8, 30, g.main_fcolor, g.main_bcolor, &g.gc, 
        g.info_window[3], BACKGROUND, HORIZONTAL);

                            /* raster y pos.- don't send form feed */
   fprintf(p_stream,"%c*p%dY",27,(int)(p.vpos * p.res)); 

                            /* raster x pos.- don't send form feed */
   fprintf(p_stream,"%c*p%dX",27,(int)(p.hpos*p.res)); 
   fflush(p_stream);

   // First, calculate b, no.of bytes to send per line for each plane.
   switch(p.dither)
   {    case 1: b=(1+lrx-ulx)/8; break;
        case 2: b=(1+lrx-ulx)/4; break;
        case 4: b=(1+lrx-ulx)/2; break;
        case 8: b=1+lrx-ulx;     break;
        case 16:b=(1+lrx-ulx)*2; break;
   }
   
   // Warn user that RGB printing will be slow.                       
   oldb=b;               
                                  // =(dpi)*(8 inches/page)/(8 bits/byte)
   if(p.palette==2) b=max(b, p.res);  

   ratio = (double)b/(double)oldb;
   if(ratio>1.25 && p.palette==2)
   {  sprintf(tempstring,"RGB printing was selected\nThis may take up to %gx as much time as CMY\n",ratio);
      message(tempstring);
   }   
   if(g.getout){ g.getout=0; return; }
   
   // Make buffer big enough for 16 lines of b bytes/line to send.    
   // Need one of these for each ink.                                 
   bufz = 1+ncolors;
   bufy = 2+b;
   bufx = 1+p.dither;
   buffer_1d = (uchar*)malloc(bufx*bufy*bufz*sizeof(uchar));
   buffer = make_3d_alias(buffer_1d,bufx,bufy,bufz);

   if(buffer==NULL)
   {   message(g.nomemory,ERROR);
       return;
   }   
   alloc1 = 1;
  
   if(p.palette==2) 
   {   for(i=0;i<ncolors;i++)
       for(j=0;j<b;j++)
       for(k=0;k<p.dither;k++) buffer[i][j][k]=255;
   }

   // Start raster graphics                                        
   // For each row, fill a buffer with bitmap patterns, different  
   //   for each resolution mode (dither).                         
   fprintf(p_stream,"%c*r1A\n",27);                // Start raster graphics
   fflush(p_stream);
   if(want_check) if(check_printer()==ABORT) goto endprint;
   progress_bar_open(www, scrollbar);

   for(j=uly;j<=lry;j++)                           // j is y value     
   { 
      progress_bar_update(scrollbar, (j-uly)*100/(lry-uly));
      itoa(j,tempstring,10);          
      strcat(tempstring,"  ");         

      print(tempstring, 68, 30, g.main_fcolor, g.main_bcolor, &g.gc, 
            g.info_window[3], BACKGROUND, HORIZONTAL);

      for(ink=0;ink<ncolors;ink++)
      { 
         if(p.dither==1 || p.dither==2 || p.dither==4)
         {                                         // Zero out buffer for 
                                                   // 1-,2- and 4-bit mode
            if(p.dither==2) bitshift=6;                          
            for(i=0;i<=(lrx-ulx);i++)             
            { switch(p.dither)
              { case 1:  buffer[ink][(i>>3)][0]=0; 
                         break;
                case 2:  for(k=0;k<2;k++) 
                            buffer[ink][(i>>2)][k]&=~(3<<bitshift);
                         bitshift -=2;
                         if(bitshift<0) bitshift=6;
                         break;
                case 4:  for(k=0;k<4;k++) 
                          if(((i>>1)<<1)==i)       // If on an even pixel     
                            buffer[ink][(i>>1)][k] &= 0x0f;
                          else                     // Odd pixel
                            buffer[ink][(i>>1)][k] &= 0xf0;
                         break;
              }
            }  
         }      

         if(p.dither==2) bitshift=6;                          
         if(p.dither==1)
         {  for(i=0;i<=(lrx-ulx);i+=8)             // i is x value 
            { 
              for(k=0;k<8;k++)
              { color=readpixelonimage(i+k+ulx,j,pixbpp,ii);        // read  pixel 
                ii = whichimage(i+k+ulx,j,pixbpp);
                color=grayvalue(color,ii,pixbpp);  // Correct for any gray 
                color=ink_color(color,ink, p.palette, pixbpp, p.positive,
                      p.rfac, p.gfac, p.bfac, p.ifac);
                if(!p.positive) color=255-color;        //  Invert color
                if(color>127) buffer[ink][(i>>3)][0] |= (1<<(7-k));
              }              
            }
         }else                                     // Perform dithering
         {  for(i=0;i<=(lrx-ulx);i++)              // i is x value 
            { 
              color=readpixelonimage(i+ulx,j,pixbpp,ii);           // read the pixel 
              color=grayvalue(color,ii,pixbpp);   // Correct for any gray 
              color=ink_color(color, ink, p.palette, pixbpp, p.positive,
                      p.rfac, p.gfac, p.bfac, p.ifac);
              switch(p.dither)
               { case 2:
                   color >>= 6;                    // scale color to 0..3     
                   for(k=0;k<2;k++)                // k is dithering row #
                      buffer[ink][(i>>2)][k] += (map[color][k])<< bitshift;
                   bitshift -=2;
                   if(bitshift<0) bitshift=6;
                   break;
                 case 4:
                   color >>= 4;                    // scale color to 0..15    
                   for(k=0;k<4;k++)                // k is the dither row
                     if(((i>>1)<<1)==i)            // If on an even pixel     
                                                   // fill left 1/2 of byte   
                        buffer[ink][i>>1][k] += map[color][k] << 4; // x 16
                
                     else                          // If on an odd pixel      
                        buffer[ink][i>>1][k] += map[color][k];          
                                                   // fill right 1/2 of byte 
                   break;
                 case 8:
                   color >>= 2;                    // scale color to 0..63    
                   for(k=0;k<8;k++)                // send 1x8 bytes/pixel
                     buffer[ink][i][k] = map[color][k];          
                   break;
                 case 16:
                   for(k=0;k<16;k++)               // send 2x16 bytes/pixel
                   {  buffer[ink][(i<<1)][k]   = map[color][k] >>8; //  256   
                      buffer[ink][(i<<1)+1][k] = map[color][k] & 0x00ff;     
                   }
                   break;
               }       
            }                 
         }
      }
     

    //--------------------------------------------------------------------//
    // Now the filled buffer is sent to the printer.                      //
    // All but the last plane are sent out using ESC*b#V.                 //
    // The last plane is  sent out using ESC*b#W.                         //
    // For B/W printing, there is only 1 plane, which uses ESC*b#W.       //
    //--------------------------------------------------------------------//

      for(k2=0;k2<p.dither;k2++)
      {  for(ink=0;ink<ncolors;ink++)
         {  if(ink<ncolors-1)
               fprintf(p_stream,"%c*b%dV",27,b);     // Expect b bytes
            else      
               fprintf(p_stream,"%c*b%dW",27,b);     // Expect b bytes
            fflush(p_stream);
#ifdef MSDOS
            if(want_check)    // If sending to stdprn
            {  for(k=0;k<b;k++) _bios_printer(0,0,buffer[ink][k][k2]); }
            else
            {  for(k=0;k<b;k++) fprintf(p_stream,"%c",buffer[ink][k][k2]); }
#else
            for(k=0;k<b;k++) fprintf(p_stream,"%c",buffer[ink][k][k2]);

#endif
         } 
         fflush(p_stream);
      }  

      if(want_check) if(check_printer()==ABORT) break;
      if(keyhit()) if(getcharacter()==27) break;
   }


   //--------------------------------------------------------------------//
   // Reset printer                                                      //
   //--------------------------------------------------------------------//

   fprintf(p_stream,"%c*rC\n",27);                // End raster graphics
   fflush(p_stream);
   fprintf(p_stream,"%c%c\n",27,12);              // Form feed            
   fflush(p_stream);
   fprintf(p_stream,"%cE\n",27);                  // Reset printer        
   fflush(p_stream); 
   progress_bar_close(www);

endprint:
   fclose(p_stream);
   if(p.device==1) 
   {   sprintf(tempstring, "%s %s", p.command, filename);
       status = system(tempstring);  // system() to lpr
       if(status != 0) 
       {    if(errno) error_message(" ", errno);
            else error_message(" ", status);
       }else    // Don't erase their temp file if error
            unlink(filename);
   }
   if(alloc1) free_3d(buffer,bufz);
   g.state=NORMAL;
   return;
}


//--------------------------------------------------------------------------//
// ink_color                                                                //
// Returns the value of a given color at specified bpp.                     //
//                                                                          //
// Extracts the color from the supplied pixel value, converts to CMY,       //
//  KCMY, or RGB if necessary, and returns the value for the color being    //
//  processed. This value is the intensity needed for the specified ink#.   //
// The color value is scaled to {0,255}.                                    //
// If p.positive is 0, it inverts the color.                                //
// Also multiplies r,g,b by user-selected print intensity values to         //
//  match the color.                                                        //
//--------------------------------------------------------------------------//
uint ink_color(uint color, int ink, int p_palette, 
     int bpp, int p_positive, double p_rfac, double p_gfac, double p_bfac,
     double p_ifac)
{
   int rr,gg,bb,cc,mm,yy,kk;
   if(p_palette==1)                             // B/W
   {   color = convertpixel(color,bpp,8,1);     // Convert to 8 bpp of black 
       color=(int)(color*p_ifac);               // Mult. by intensity factor
       color=min(color,255);
       if(p_positive) color=255-color;
   } 
   if(p_palette==2)                             // rgb
   {   color = convertpixel(color,bpp,24,1);    // Convert to 8 bpp of r,g,b
       valuetoRGB(color,rr,gg,bb,24);
       if(ink==0) color=(int)(rr*p_rfac);
       if(ink==1) color=(int)(gg*p_gfac);
       if(ink==2) color=(int)(bb*p_bfac);
       color = min(color,255);
       if(!p_positive) color=255-color;
   }else
   if(p_palette==3)                             // cmy
   {   color=convertpixel(color,bpp,24,1);      // Convert to 8 bpp of r,g,b
       valuetoRGB(color,rr,gg,bb,24);
       if(ink==0) color=(int)((255-rr)*p_rfac); // rr is now cyan
       if(ink==1) color=(int)((255-gg)*p_gfac); // gg is now magenta
       if(ink==2) color=(int)((255-bb)*p_bfac); // bb is now yellow
       color = min(color,255);
       if(!p_positive) color=255-color;
   }else
   if(p_palette==4)                             // kcmy
   {   color=convertpixel(color,bpp,24,1);      // Convert to 8 bpp of r,g,b
       valuetoRGB(color,rr,gg,bb,24);
       kk = min(255-rr,min(255-gg,255-bb));
       cc = 255-rr-kk;          // rr is now cyan
       mm = 255-gg-kk;          // gg is now magenta
       yy = 255-bb-kk;          // bb is now yellow
       if(ink==0) color=(int)(kk*p_ifac);
       if(ink==1) color=(int)(cc*p_rfac);
       if(ink==2) color=(int)(mm*p_gfac);
       if(ink==3) color=(int)(yy*p_bfac);
       color = min(color,255);
       if(!p_positive) color=255-color;
   }
   return(color);
}


//--------------------------------------------------------------------------//
// check_printer - MSDOS only                                               //
//--------------------------------------------------------------------------//
int check_printer(void)
{
   // Should only be called if printing to stdprn (== LPT1: ).
   // If output is directed to a network printer, it is not so easy 
   // to get the printer status because bidirectional communication
   // is needed. This only works in DOS/Windows.

   // 'Printer out of paper' is intercepted by DOS to give a critical
   // error. Sometimes "Offline" is also translated by DOS into a 
   // critical "Write error". In these cases, the program goes to
   // criterr_handler instead.

#ifdef MSDOS
   const int OK=0;
   const int ERR=1;

   char temp[20]="y";
   char a,b,status=OK;
   union REGS reg;
   struct SREGS seg;
   
   reg.h.ah = 0x02;
   reg.x.dx = 0;  // 0=LPT1, 1=LPT2, 2=LPT3
   int86(0x17,&reg,&reg);
   a = reg.h.ah;
   b = ~a;

   if(error_occurred)
   {  error_occurred=0; 
      message("Device error!",ERROR);
   }

   if((a & 0x29) || (b & 0x10)) status=ERR;
   if(status==ERR)
   {
      if(a & 0x01) message("Printer timeout!!",ERROR);
      if(a & 0x08) message("Printer I/O error!!!",ERROR);
      if(b & 0x10) message("Printer offline!!!!",ERROR);
      if(a & 0x20) message("Out of paper!!!!!",ERROR); 
   }

   if(temp[0]!='y') return ABORT;
#endif
   return OK;
}


//--------------------------------------------------------------------------//
//  write_postscript_file                                                   //
//--------------------------------------------------------------------------//
int write_postscript_file(PrinterStruct p)
{
  static char filename[FILENAMELENGTH];
  static PromptStruct ps;
  ps.filename = filename;
  ps.f1 = write_postscript_file_part2;
  ps.f2 = null;
  ps.ptr = &p;
  switch(p.device)
  {   case 1: strcpy(filename, "/tmp/~temp.print");  // system() to lpr
              write_postscript_file_part2(&ps);
              break;
      case 2: strcpy(filename, p.devicename);
              write_postscript_file_part2(&ps);
              break;
      case 3: strcpy(filename, p.filename);
              check_overwrite_file(&ps); 
              break;
  }
  return OK;
}
 

//--------------------------------------------------------------------------//
//  write_postscript_file_part2                                             //
//--------------------------------------------------------------------------//
void write_postscript_file_part2(PromptStruct *ps)
{
  char decode_string[20];
  char tempstring[FILENAMELENGTH];
  char tempstring2[128];
  Widget www, scrollbar;
  double iratio, vsize;
  int bb, bpp, bitspercomponent=8, cc, color, color_type=0, count, gg, ino, 
     linecount=-1, j, k, kk, lrx, lry, mm, ncolors=1, pixbpp, rr, status=OK, 
     bbox_x1, bbox_y1, bbox_x2, bbox_y2, x2, y2,
     ulx, uly, x, xpixels, y=0, ypixels, yy;

  char *filename = ps->filename;
  PrinterStruct *p = &printer;  // ps->ptr is corrupted at this point
  FILE *fp;
  g.getout=0;

  if(p->entire && ci>=0 && ci<g.image_count)
  {   ulx = z[ci].xpos;
      uly = z[ci].ypos;     
      lrx = z[ci].xpos + z[ci].xsize;
      lry = z[ci].ypos + z[ci].ysize;
      bpp = z[ci].bpp;
      strcpy(tempstring2, "image");
  }else
  {   ulx = g.selected_ulx;
      uly = g.selected_uly;
      lrx = g.selected_lrx;
      lry = g.selected_lry;
      bpp = g.bitsperpixel;
      strcpy(tempstring2, "selected region");
  }   
  if(bpp!=8)
  {  sprintf(tempstring,"Check to ensure %s is\n8 bits/pixel before proceeding.\nSome printers will have problems with\n%d bit PostScript files",tempstring2,bpp);
     message(tempstring,WARNING);
     if(g.getout) {g.getout=0; return; }
  }
  xpixels = lrx - ulx;
  ypixels = lry - uly;
  iratio = (double)ypixels / (double)xpixels;
  vsize = p->hsize * iratio;

  switch(p->palette)
  {  case 1: color_type =  GRAY; 
             break;   // GrayScale
     case 2: if(bpp==8) color_type=INDEXED; else color_type=COLOR;  // RGB
             break;
     case 3: 
     case 4: color_type = CMYK; break;    // CMY, CMYK
   }
   
   //-----------End of initializing data------------//
   //-----------Open printer file-------------------//

   g.state=PRINTING;
   drawselectbox(OFF);
   XFlush(g.display); 
 
   printstatus(PRINTING);
   fp = fopen(filename,"w");
   if(fp==NULL) 
   {    error_message(filename, errno);
        if(p->device==1) sprintf(tempstring,"Can't create temporary file \n%s",filename);
        if(p->device==2) sprintf(tempstring,"Unable to access\n%s",p->devicename);
        if(p->device==3) sprintf(tempstring,"Can't open file\n%s",p->filename);
        message(tempstring,ERROR);
        return;
   }
   
   if(p->device==1) sprintf(tempstring,"Printing to\n%s",p->command);
   if(p->device==2) sprintf(tempstring,"Printing to\n%s",p->devicename);
   if(p->device==3) sprintf(tempstring,"Printing to\n%s",filename); 

   progress_bar_open(www, scrollbar);

   //-----------Start of sending file---------------//

   fprintf(fp,"%s!PS-Adobe-2.0 EPSF-2.0\n","%");
   fprintf(fp,"%sTitle: %s\n","%%",p->filename);
   fprintf(fp,"%sCreator: %s version %s\n","%%",g.appname, g.version);
   bbox_x1 = (int)(p->hpos*72.0);
   bbox_x2 = (int)((p->hpos + p->xsize)*72.0);
   bbox_y1 = (int)(p->vpos*72.0);
   bbox_y2 = (int)((p->vpos + p->ysize)*72.0);

   fprintf(fp,"%sBoundingBox: %d %d %d %d\n","%%", bbox_x1, bbox_y1, bbox_x2, bbox_y2);
   fprintf(fp,"%sPages: 1\n","%%");
   fprintf(fp,"%sDocumentFonts:\n","%%");
   fprintf(fp,"%sEndComments\n","%%");
   fprintf(fp,"%sEndProlog\n","%%");

   fprintf(fp,"/origstate save def\n");
   fprintf(fp,"\n");

   fprintf(fp,"%sPage: 1 1\n\n","%%");

   //// Send palette if bpp is 8

   switch(color_type)
   {  case INDEXED: 
         fprintf(fp,"[/Indexed /DeviceRGB 255\n<\n"); 

         ////  If palette is an unmodified grayscale palette, send a
         ////  true grayscale map instead of trying to scale 0..63 to
         ////  0..255. This will give better print quality for grayscale
         ////  images.

         if(unmodified_grayscale(g.palette))
         for(k=0;k<256;k+=8)
         {  for(j=0;j<8;j++) 
            fprintf(fp,"%02x%02x%02x ",k+j, k+j, k+j);
            fprintf(fp,"\n");
         }else
         for(k=0;k<256;k+=8)
         {  for(j=0;j<8;j++) 
            fprintf(fp,"%02x%02x%02x ",
                min(255, 4*(1+g.palette[k+j].red)-1), 
                min(255, 4*(1+g.palette[k+j].green)-1),
                min(255, 4*(1+g.palette[k+j].blue)-1) );   
            fprintf(fp,"\n");
         }
         fprintf(fp,"\n>\n] setcolorspace\n\n");
         strcpy(decode_string,"[0 255]");
         bitspercomponent = 8;
         ncolors=1; 
         break; 
      case GRAY:
         ncolors=1;
         bitspercomponent = bpp;
         strcpy(decode_string,"[0 1]");
         fprintf(fp,"[/DeviceGray] setcolorspace\n"); 
         break;
      case COLOR: 
         ncolors=3;
         bitspercomponent = 8;
         strcpy(decode_string,"[0 1 0 1 0 1]");
         fprintf(fp,"[/DeviceRGB] setcolorspace\n"); 
         break;
      case CMYK:   
         ncolors=4;
         bitspercomponent = 8;
         strcpy(decode_string,"[0 1 0 1 0 1 0 1]");
         fprintf(fp,"[/DeviceCMYK] setcolorspace\n"); 
         break;
   }

   fprintf(fp,"\n");
   fprintf(fp,"/pix %d string def\n",xpixels*ncolors);

   //// Distance of upper left corner from lower left of page

   x = x2 = (int)(.999 + 72.0 * p->hpos);

   ////  Align  margin with bottom of page (looks funny, but gv compatible)
   if(p->device == 3)    // print to file
       y = y2 = (int)(.999 + 72.0 * p->vpos); 
   ////  Align margin with top of page  (looks better on printer)
   else
       y = y2 = (int)(.999+72.0*(p->paperysize - p->vpos - vsize)); 
 
   if(p->rotation) x2 += bbox_x2 - bbox_x1;
   if(p->rotation) y2 = cint(p->vpos * 72);
   fprintf(fp,"%d %d translate\n", x2, y2);
   if(p->rotation) fprintf(fp,"90 rotate\n");

 
   //// Size of image on page, truncate to next lower int for gv

   if(p->rotation) 
       fprintf(fp,"%f %f scale\n\n", (double)int(72.0 * p->ysize),
                                     (double)int(72.0 * p->xsize));
   else
       fprintf(fp,"%f %f scale\n\n", (double)int(72.0 * p->xsize),
                                     (double)int(72.0 * p->ysize));

   //// Image dictionary
  
   fprintf(fp,"<<\n\
      /ImageType 1 \n\
      /Width %d \n\
      /Height %d \n\
      /BitsPerComponent %d \n\
      /Decode %s \n\
      /ImageMatrix [%d 0 0 -%d 0 %d] \n\
      /DataSource { currentfile pix readhexstring pop } \n",
       xpixels, ypixels, bitspercomponent, decode_string, xpixels, ypixels, ypixels);
   fprintf(fp,">>\nimage\n\n");

   //// Send image

   count = 0;
    

   for(j=uly;j<lry;j++)
   { 
     check_event();             
     if(g.getout) break;  // Don't access in_printimage here or it will crash
     if(lry!=uly) progress_bar_update(scrollbar, (j-uly)*100/(lry-uly));
     if(++linecount==10)
     {   sprintf(tempstring,"Printing..%d  ",j);
         printstatus(MESSAGE,tempstring);
         linecount = 0;
     }
     for(k=ulx;k<lrx;k++)
     {  color = readpixelonimage(k,j,pixbpp,ino);  
        if(!between(ino, 0, g.image_count-1)) break;
        if(g.getout) break;
        switch(color_type)
        { case GRAY:
          case INDEXED:                                  // Gray or Indexed
                if(!p->positive) color=255-color;         // Invert color
                fprintf(fp,"%02x",color); 
                break; 
          case COLOR:                                    // RGB
                color = convertpixel(color,pixbpp,24,1); // Convert to 24 bpp
                valuetoRGB(color,rr,gg,bb,24);           // Extract r,g,b
                rr = (int)(rr * p->rfac);
                gg = (int)(gg * p->gfac);
                bb = (int)(bb * p->bfac);     
                if(!p->positive)                          // Invert color
                {    rr = 255 - rr; 
                     gg = 255 - gg;
                     bb = 255 - bb;
                }
                fprintf(fp,"%02x%02x%02x",rr,gg,bb); 
                break;
          case CMYK:                                     // CMYK
                color = convertpixel(color,pixbpp,24,1); // Convert to 24 bpp
                valuetoRGB(color,rr,gg,bb,24);           // Extract r,g,b
                kk = min(255-rr,min(255-gg,255-bb));
                cc = 255-rr-kk;          // r is now cyan
                mm = 255-gg-kk;          // g is now magenta
                yy = 255-bb-kk;          // b is now yellow
                kk = (int)(kk*p->ifac);
                cc = (int)(cc*p->rfac);
                mm = (int)(mm*p->gfac);
                yy = (int)(yy*p->bfac);
                if(!p->positive) 
                {  kk = 255 - kk;
                   cc = 255 - cc;
                   mm = 255 - mm;
                   yy = 255 - yy;
                }
                fprintf(fp,"%02x%02x%02x%02x ",cc,mm,yy,kk); 
                break;
        }
        count += ncolors;
        if(count>=36){ count=0; fprintf(fp,"\n"); }
     }
   }
   //// Finish

   fprintf(fp,"\n\nshowpage\n");
   fprintf(fp,"origstate restore\n");
   fprintf(fp,"%sTrailer\n","%%");

   //-----------End of sending file-----------------//

  fclose(fp);
  if(p->device==1) 
  {    sprintf(tempstring, "%s %s", p->command, filename);
       status = system(tempstring);  // system() to lpr
       if(status != 0) 
       {    if(errno) error_message(filename, errno);
            else error_message(filename, status);
       }else    // Don't erase their temp file if error
       {     status = unlink(filename);
             if(status) message("Error deleting temporary file");
       }
  }

  g.state=NORMAL;
  progress_bar_close(www);
  printstatus(NORMAL);
 
  return;
}


//--------------------------------------------------------------------------//
//  unmodified_grayscale - test if palette is svgapalette 1.                //
//  If so, caller can safely use a true 0..255 grayscale palette instead of //
//  trying to convert the 6-bit/color palette used in 8 bit screen modes.   //
//--------------------------------------------------------------------------//
int unmodified_grayscale(RGB *p)
{
    int i, answer=1, d;
    for (i=0; i<255; i++)
    {  d = p[i+1].red + p[i+1].green + p[i+1].blue - p[i].red - p[i].green - p[i].blue;
       if(!between(d,0,2)) { answer = 0; break; }
    }
    return answer;
}


//--------------------------------------------------------------------------//
//  error_message                                                           //
//--------------------------------------------------------------------------//
void error_message(const char *s, int error)
{ 
    char tempstring[1024];
    if(!error) return;
    if(s==NULL)
        sprintf(tempstring, "%s", strerror(error));
    else 
        sprintf(tempstring, "%s: %s",s, strerror(error));
    message(tempstring, ERROR);
}                 


//--------------------------------------------------------------------------//
//  status_error_message                                                    //
//--------------------------------------------------------------------------//
void status_error_message(int status)
{
   char tempstring[256];
   switch(status)
   {  case OK:break;
      case NOMEM: strcat(tempstring,"Out of memory");break;
      case CANCEL:
      case ABORT: strcpy(tempstring,"Aborted");break;
      case ERROR: strcpy(tempstring,"Error");break;
      case NOTFOUND: strcpy(tempstring,"File not found");break;
      case NOIMAGES: strcpy(tempstring,"No images");break;
      case CANTCREATEFILE: strcpy(tempstring, "Can't create file");break;
      case CRITERR: strcpy(tempstring,"Critical error");break;
      case ZEROLENGTH: strcpy(tempstring,"File was zero length");break;
      case BADPARAMETERS: strcpy(tempstring,"Bad parameters");break;
      case UNKNOWN: strcpy(tempstring,"Unknown file error");break;
      case IGNORE: break;
      default: strcpy(tempstring,"Unknown");break;
   }
   if(status && status !=IGNORE) message(tempstring, ERROR);
}


//--------------------------------------------------------------------------//
// create_print_filename                                                    //
//--------------------------------------------------------------------------//
void create_print_filename(int ino, char *print_filename)
{
    int k,pos=0;
    if(ino<0 || !strlen(z[ino].name)){ strcpy(print_filename, "temp.print"); return; }
    strcpy(print_filename, z[ino].name);
    for(k=0;k<(int)strlen(print_filename);k++) if(print_filename[k]=='.') pos=k;
    if(pos>0) print_filename[pos]=0;
    if(printer.language==POSTSCRIPT) strcat(print_filename, ".ps");
    if(printer.language==PCL) strcat(print_filename, ".prn");
}
