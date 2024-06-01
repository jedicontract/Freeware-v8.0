//--------------------------------------------------------------------------//
// xmtnimage12.cc                                                           //
// Text-related routines                                                    //
// Latest revision: 09-24-2005                                              //
// Copyright (c) 2005 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern int         ci;                 // current image
extern Image      *z;
extern Globals     g;
                                       // Position for data on information window
int ypos[22] = { 14, 28, 42, 56, 70, 84,     // absolute & relative coords
                 98,112,126,140,154,168,     // calibration
                182,196,210,224,238,252,     // intensity, value, rgb, fft re im
                266,280,294,308 };
const int MAXITEMS=100;
extern RGB sample[20][19];            // from xmtnimage66, sample palette
extern Window sample_palette_window;
extern int title_ypos;

//--------------------------------------------------------------------------//
// abouttheprogram                                                          //
//--------------------------------------------------------------------------//
void abouttheprogram(void)
{ 
   int k, count;
#ifdef HAVE_MEMINFO
   FILE *fp;
#endif

   static listinfo *atplist;
   static char **atp_info;                                    
   static int atplist_selection;
   static char *atplist_title;
   atplist_title = new char[100];
   strcpy(atplist_title, "About the Program"); 
   atplist = new listinfo;
   if(memorylessthan(16384)){ message(g.nomemory,ERROR); return; }
   char tempstring[FILENAMELENGTH];
   int listcount=-1;
  
   //-------------Put information in list box-------------------------//

   //----------Increase this size before adding new items-------------//

   atp_info = new char*[19];

   //----program name------------------------------//
   listcount++;
   atp_info[listcount] = new char[100];                       
   strcpy(atp_info[listcount],"Image Measurement and Analysis Lab (tnimage)");

   //----email address for all those bug reports---//
   listcount++;
   atp_info[listcount] = new char[100];                       
   memset(atp_info[listcount], 0, 100);
   sprintf(atp_info[listcount], "Send bug reports to: %s", g.emailaddress);

   //----operating system--------------------------//
   listcount++;
   atp_info[listcount] = new char[100];                       
#ifdef MSDOS
   if(in_windows==0) strcpy(tempstring,"DOS mode");
   if(in_windows==1) strcpy(tempstring,"Windows real or std.mode");
   if(in_windows==2) strcpy(tempstring,"Windows enhanced mode");
#endif
   strcpy(tempstring,"Unix version");
#ifdef LINUX
   strcpy(tempstring,"Linux version");
#endif
#ifdef SOLARIS
   strcpy(tempstring,"Solaris version");
#endif
#ifdef CONVEX
   strcpy(tempstring,"ConvexOS version");
#endif
#ifdef IRIX
   strcpy(tempstring,"Irix version");
#endif
#ifdef UNIX
   if(!strlen(tempstring)) strcpy(tempstring,"UNIX version");
#endif
#ifdef VMS
   strcpy(tempstring,"VMS version");
#endif
   strcpy(atp_info[listcount],tempstring);

   //-------version & compilation date-------------//
   listcount++;
   atp_info[listcount] = new char[100];                       
   sprintf(tempstring,"Version %s, compiled on %s ",g.version,__DATE__);
   strcpy(atp_info[listcount],tempstring);

   //-------amount of free memory------------------//
#ifdef MSDOS  
   listcount++;
   atp_info[listcount] = new char[100];                       
   strcpy(tempstring,"Total free memory: "); 
   k = _x32_allcoreleft();                 // Requires Flashtek x32vm dos extender
   k -= imagesize(410,savedialogboxysize,bitsperpixel); // Size of dialog box 
   k -= imagesize(404,112,bitsperpixel);   // Size of message box
   k -= 2000;                              // Safety margin 
   k = max(0,k);
   itoa(k,tempstring2,10);
   strcat(tempstring,tempstring2);
   strcat(tempstring," bytes");
   strcpy(atp_info[listcount],tempstring);

   listcount++;
   atp_info[listcount] = new char[100];                       
   strcpy(tempstring,"Largest memory block: "); 
   k = _x32_coreleft();
   k -= imagesize(410,savedialogboxysize,bitsperpixel); // Size of dialog box 
   k -= imagesize(404,112,bitsperpixel);   // Size of message box
   k -= 2000;                              // Safety margin 
   k = max(0,k);
   itoa(k,tempstring2,10);
   strcat(tempstring,tempstring2);
   strcat(tempstring," bytes");
   strcpy(atp_info[listcount],tempstring);
#endif
#ifdef HAVE_MEMINFO
   fp = fopen("/proc/meminfo","rt");
   if(fp!=NULL)
   {  
    listcount++;
    atp_info[listcount] = new char[100];                       
    strcpy(atp_info[listcount],"Memory usage");

    listcount++;
    atp_info[listcount] = new char[100];                       
    fgets(tempstring, 80, fp);
    remove_terminal_cr(tempstring);
    strcpy(atp_info[listcount],tempstring);

    listcount++;
    atp_info[listcount] = new char[100];                       
    fgets(tempstring, 80, fp);
    remove_terminal_cr(tempstring);
    strcpy(atp_info[listcount],tempstring);

    listcount++;
    atp_info[listcount] = new char[100];                       
    fgets(tempstring, 80, fp);
    remove_terminal_cr(tempstring);
    strcpy(atp_info[listcount],tempstring);

    fclose(fp);
   }
#endif

   //-------screen mode----------------------------//
   listcount++;
   atp_info[listcount] = new char[100]; 
   sprintf(tempstring,"Screen resolution=%d x %d   %d bits/pixel",
         g.xres, g.yres, max(8, g.bitsperpixel)); 
   strcpy(atp_info[listcount],tempstring);
 
   //-------Window resolution----------------------//
   listcount++;
   atp_info[listcount] = new char[100]; 
   sprintf(tempstring,"Window resolution=%d x %d pixels",
         g.main_xsize, g.main_ysize); 
   strcpy(atp_info[listcount],tempstring);

   //-------colormap status------------------------//
   listcount++;
   atp_info[listcount] = new char[100];  
   if(g.want_colormaps) sprintf(tempstring,"Using colormaps");
   else
   {   count = 0;
       for(k=0;k<256;k++) if(g.reserved[k]) count++;
       sprintf(tempstring,"%d colors allocated",count);
   }
   strcpy(atp_info[listcount],tempstring);

   //-------no. of images in memory----------------//
   listcount++;
   atp_info[listcount] = new char[100];                       
   sprintf(tempstring,"%d image(s) present",g.image_count);
   strcpy(atp_info[listcount],tempstring);

   atplist_selection = 0;
   atplist->title  = atplist_title;
   atplist->info   = atp_info;
   atplist->count  = listcount+1;
   atplist->itemstoshow = listcount-1;
   atplist->firstitem   = 1;
   atplist->wantsort    = 0;
   atplist->wantsave    = 0;
   atplist->helptopic   = 34;
   atplist->allowedit   = 0;
   atplist->selection   = &atplist_selection;
   atplist->width       = 0;
   atplist->transient   = 1;
   atplist->wantfunction = 0;
   atplist->autoupdate   = 0;
   atplist->clearbutton  = 0;
   atplist->highest_erased = 0;
   atplist->f1 = null;
   atplist->f2 = null;
   atplist->f3 = null;
   atplist->f4 = delete_list;
   atplist->f5 = null;
   atplist->f6 = null;
   atplist->listnumber = 0;
   list(atplist);
}


//--------------------------------------------------------------------------//
// image_list                                                               //
//--------------------------------------------------------------------------//
void image_list(void) 
{ 
   static char *imagelist_title;
   static char **imagelist_info;                                    
   static listinfo *imagelist;
   static int imagelist_selection;

   int k;
   if(memorylessthan(16384)){ message(g.nomemory,ERROR); return; }
   int listcount=-1;
  
   imagelist = new listinfo;
   imagelist_info = new char*[MAXIMAGES+2];
   imagelist_title = new char[100];
   strcpy(imagelist_title, "Image List"); 
   listcount = g.image_count + 2;
   for(k=0; k<listcount; k++) 
   {  imagelist_info[k] = new char[128]; imagelist_info[k][0] = 0; }
   strcpy(imagelist_info[0],"_____________________________________________________________________");

   imagelist_selection = 0;
   imagelist->title  = imagelist_title;
   imagelist->info   = imagelist_info;
   imagelist->edittitle   = new char[100]; // if allowedit is 1, this must be allocated
   imagelist->count  = listcount;
   imagelist->itemstoshow = 6;
   imagelist->firstitem   = 1;
   imagelist->wantsort    = 0;
   imagelist->wantsave    = 0;
   imagelist->helptopic   = 34;
   imagelist->allowedit   = 0;
   imagelist->selection   = &imagelist_selection;
   imagelist->width       = 0;
   imagelist->transient   = 1;
   imagelist->wantfunction = 1;  // use f1 to perform an action when item is selected
   imagelist->autoupdate   = 1;  // use f2 to update list
   imagelist->clearbutton  = 0;
   imagelist->highest_erased = 0;
   imagelist->f1 = image_list_switchto;
   imagelist->f2 = image_list_update;
   imagelist->f3 = null;
   imagelist->f4 = delete_list;
   imagelist->f5 = null;
   imagelist->f6 = null;
   imagelist->listnumber = 0;
   imagelist->wc = 0;
   list(imagelist);
}


//--------------------------------------------------------------------------//
// image_list_switchto                                                      //
//--------------------------------------------------------------------------//
void image_list_switchto(listinfo *l)
{
   int answer = *l->selection - l->firstitem;
   l->autoupdate = 0;  // turn off callback that screws up list position
   switchto(answer);
   l->autoupdate = 1;
}


//--------------------------------------------------------------------------//
// image_list_update                                                        //
//--------------------------------------------------------------------------//
void image_list_update(int listno)
{ 
   int k, listpos;
   XmString xms;
   listinfo *l = g.openlist[listno];
   if(g.mouse_button) return;
   Widget w = l->widget;
   Widget browser = l->browser;
   int wantraise=0;
   char is_subframe;
   char isbackup[8];
   char iscolor[8];
   char tempstring[FILENAMELENGTH];
   for(k=0;k<l->count;k++) delete[] l->info[k];              
   l->count = g.image_count + 2;
   for(k=0;k<l->count; k++) { l->info[k] = new char[256];  l->info[k][0] = 0; }

   XtVaGetValues(w, XmNtopItemPosition, &listpos,  NULL);         

   XmListDeselectAllItems(w);
   XmListDeleteAllItems(w);  // XmListDeleteItemsPos crashes in solaris
   strcpy(l->info[0], "Image# Frames x pos. y pos. x size ysize  Bpp    Color Backed up  Name ");
   additemtolist(w, browser, l->info[0], BOTTOM, wantraise);
   for(k=0; k<g.image_count; k++)
   {   
         if(z[k].colortype!=GRAY) strcpy(iscolor,"yes"); else strcpy(iscolor,"no ");
         if(z[k].backedup) strcpy(isbackup,"yes"); else strcpy(isbackup,"no ");
         if(z[k].split_frames) is_subframe= 'f'; else is_subframe = ' ';
         strcpy(tempstring, basefilename(z[k].name));
         tempstring[30]=0;  
         sprintf(l->info[k],"%3d %5d%c%5d  %5d   %6d %6d   %2d (%2d) %3s   %3s        %s     ",
               k, z[k].frames, is_subframe, z[k].xpos, z[k].ypos, z[k].xsize, z[k].ysize, 
               z[k].originalbpp, z[k].bpp, iscolor, isbackup, tempstring);
         if(k==g.image_count-1) wantraise=1;
         XmListAddItemUnselected(w, xms = XmStringCreateSimple(l->info[k]), 0);
         XmStringFree(xms);      
    }

    XmListSetPos(w, listpos);
    select_list_item(w, ci+2);
}


//--------------------------------------------------------------------------//
// aboutthefile                                                             //
//--------------------------------------------------------------------------//
void aboutthefile(int noofargs, char **arg)
{ 
   int k=0, compress=0;
   char *filename, *oldfilename;
   char identifier[64];
   char tempstring[256];
   filename = new char[FILENAMELENGTH];
   oldfilename = new char[FILENAMELENGTH];
   strcpy(oldfilename, "Untitled");
   filename[0]=0;
  
   if(noofargs>=1) strcpy(filename, arg[1]);
   else strcpy(filename, getfilename(oldfilename, NULL));
   if(!g.getout)
   { if(!access(filename,F_OK))    // access = 0 if file exists  
     {
        k=imagefiletype(filename, identifier, compress);
        if(compress)
             sprintf(tempstring,"Image is a compressed %s file", identifier);
        else
             sprintf(tempstring,"Image is a %s file", identifier);
        switch(k)
        {   case TIF: abouttiffile(filename, compress);break;            
            case PCX: aboutpcxfile(filename);break;
            case IMA: message(tempstring);break;
            case IMM: message("Image is a monochrome IMG file");break;
            case IMG: message("Image is an 8-bit IMG file");break;
            case GIF: aboutgiffile(filename);break;
            case GEM: message("Image is a GEM IMG file");break;
            case JPG: message(tempstring);break;
            case TARGA:message("Image is probably a Targa file");break;
            case PICT: message(tempstring);break;
            case IMDS: message("Image is an IMDS (MVS GEM) file");break;
            case BMP: message("Image is a Windows bitmap (BMP) file");break;
            case XWD: aboutxwdfile(filename);break;
            case FITS: aboutfitsfile(filename);break;
            case PDS:  aboutpdsfile(filename);break;
            case PDS2: aboutpdsfile(filename);break;
            case PBM: message("Image is a PBM monochrome ASCII file");break;
            case PGM: message("Image is a PGM grayscale ASCII file");break; 
            case PPM: message("Image is a PPM color ASCII file");break;
            case PBMRAW: message("Image is a raw PBM monochrome file");break;
            case PGMRAW: message("Image is a raw PGM grayscale file");break;
            case PPMRAW: message("Image is a raw PPM color file");break;
            case CUSTOM: aboutcustomfile(filename, identifier);break;
            default: message("Unknown file type");
        }   
     }else message("Can't find the file!");
   }
   delete[] oldfilename;
   delete[] filename;
}                   


//--------------------------------------------------------------------------//
// abouttheimage                                                            //
//--------------------------------------------------------------------------//
void abouttheimage(int noofargs, char **arg)
{ 
   static char *atilist_title;
   static char **atilist_info;                                    
   static listinfo *atilist;
   int k, ino=ci;

   atilist = new listinfo;
   atilist_title =  new char[100];
   strcpy(atilist_title, "About the Image");    
   if(memorylessthan(16384)){ message(g.nomemory,ERROR); return; } 
   if(noofargs>=1) ino = image_number(arg[1]);
   if(ino<0){ message("Please select an image"); return; } 
 
   //-------------Put information in list box-------------------------//

   int listcount=MAXITEMS;              // Change if you add more items
   atilist_info = new char*[MAXITEMS];  // max. MAXITEMS lines - Change if you add more items
   for(k=0; k<=listcount; k++) atilist_info[k] = new char[100];                       
   if(listcount>MAXITEMS) fprintf(stderr, "Error: too many list items\n");
   about_image_fill(ino, atilist_info);   

   atilist->title          = atilist_title;
   atilist->info           = atilist_info;
   atilist->count          = listcount;
   atilist->itemstoshow    = min(20, listcount+1);
   atilist->firstitem      = 0;
   atilist->wantsort       = 0;
   atilist->wantsave       = 0;
   atilist->helptopic      = 1;
   atilist->allowedit      = 0;
   atilist->selection      = &g.crud;
   atilist->width          = 0;
   atilist->transient      = 1;
   atilist->wantfunction   = 0;
   atilist->autoupdate     = 1;
   atilist->clearbutton    = 0;
   atilist->highest_erased = 0;
   atilist->f1 = null;               // use f1 to perform an action when item is selected
   atilist->f2 = about_image_update; // use f2 to update list 
   atilist->f3 = null;
   atilist->f4 = delete_list;
   atilist->f5 = null;
   atilist->f6 = null;
   atilist->listnumber = 0;
   atilist->wc = 0;
   list(atilist);
   g.getout=0;
}


//--------------------------------------------------------------------------//
// about_image_update                                                       //
//--------------------------------------------------------------------------//
void about_image_update(int listno)
{ 
   int k;
   if(g.mouse_button) return;
   XmString *listitems=NULL;
   listitems = new XmString[100];
   listinfo *l = g.openlist[listno];
   char **info = l->info;
   g.openlist[listno]->count = about_image_fill(ci,info);
   for(k=0; k<l->count; k++) 
       listitems[k] = XmStringCreateSimple(info[k]);
   XmListReplaceItemsPosUnselected(l->widget, listitems, l->count, 1);
   for(k=0;k<l->count;k++) XmStringFree(listitems[k]);
   delete[] listitems;
}


//--------------------------------------------------------------------------//
// about_image_fill                                                         //
//--------------------------------------------------------------------------//
int about_image_fill(int ino, char **info)
{
   int f, n=1;
   double a=0.0;
   uint zerocolor=0;
   int rr=0,gg=0,bb=0;
   if(ino<0 || ino>=g.image_count || info==NULL) return 0;
   char tempstring[FILENAMELENGTH];
   sprintf(info[0],"Image no. %d",ino);
   strcpy(tempstring, z[ino].name);   
   tempstring[50]=0;  
   sprintf(info[n++],"Title %s",tempstring);
   sprintf(info[n++],"X size %d",z[ino].xsize);
   sprintf(info[n++],"Y size %d",z[ino].ysize);
   sprintf(info[n++],"Frames %d",z[ino].frames);
   sprintf(info[n++],"X position %d",z[ino].xpos);
   sprintf(info[n++],"Y position %d",z[ino].ypos);
   sprintf(info[n++],"Overlap order %d",z[ino].overlap);
   sprintf(info[n++],"Current frame %d",z[ino].cf);
   sprintf(info[n++],"X zoom factor %g",z[ino].zoomx);
   sprintf(info[n++],"Y zoom factor %g",z[ino].zoomy);
   strcpy(info[n],"Backed up: " );  
      if(z[ino].backedup) strcat(info[n++],"yes"); 
      else strcat(info[n++],"no");
   strcpy(tempstring, basefilename(z[ino].name));
      tempstring[50]=0;  // strncpy doesn't work
      sprintf(info[n++],"Filename %s",tempstring);
   sprintf(info[n++],"File format %s",z[ino].format_name);
   sprintf(info[n++],"Original bits/pixel %d",z[ino].originalbpp);
   sprintf(info[n++],"Current bits/pixel %d",z[ino].bpp);
   strcpy(info[n],"Color type: " );  
      if(z[ino].colortype==GRAY)    strcat(info[n],"Grayscale");
      if(z[ino].colortype==INDEXED) strcat(info[n],"Indexed color");
      if(z[ino].colortype==COLOR)   strcat(info[n],"Color");
      n++;
   sprintf(info[n++],"No. of colors %d",z[ino].ncolors);
   strcpy(info[n],"Grayscale map in effect: " );  
      if(z[ino].hitgray) strcat(info[n++],"yes");
      else              strcat(info[n++],"no");
   strcpy(info[n],"Modified: " );  
      if(z[ino].touched) strcat(info[n++],"yes");
      else              strcat(info[n++],"no");
   strcpy(info[n],"Separate window: " );  
      if(z[ino].shell) strcat(info[n++],"yes");
      else            strcat(info[n++],"no");
   strcpy(info[n],"Chromakeyed: " );  
      if(z[ino].chromakey) strcat(info[n++],"yes");
      else              strcat(info[n++],"no");
   sprintf(info[n++],"Opaque range: %d to %d",z[ino].ck_graymin, z[ino].ck_graymax);
   sprintf(info[n++],"Red opaque range: %d to %d",z[ino].ck_min.red, z[ino].ck_max.red);
   sprintf(info[n++],"Green opaque range: %d to %d", z[ino].ck_min.green, z[ino].ck_max.green);
   sprintf(info[n++],"Blue opaque range: %d to %d",z[ino].ck_min.blue, z[ino].ck_max.blue);
   strcpy(info[n],"x Calibration: " );  
      if(z[ino].cal_log[0]==-1) strcat(info[n],"none");
      if(z[ino].cal_log[0]== 0) strcat(info[n],"linear");
      if(z[ino].cal_log[0]== 1) strcat(info[n],"Log");
      if(z[ino].cal_log[0]== 2) strcat(info[n],"Polynomial");
      if(z[ino].cal_log[0]== 3) strcat(info[n],"Distance from center");
      n++;
   strcpy(info[n],"y Calibration: " );  
      if(z[ino].cal_log[1]==-1) strcat(info[n],"none");
      if(z[ino].cal_log[1]== 0) strcat(info[n],"linear");
      if(z[ino].cal_log[1]== 1) strcat(info[n],"Log");
      if(z[ino].cal_log[1]== 2) strcat(info[n],"Polynomial");
      if(z[ino].cal_log[1]== 3) strcat(info[n],"Distance from center");
      n++;
   sprintf(info[n++],"Calibration dimensions: %d",z[ino].cal_dims);  
   strcpy(info[n],"FFT exists: " );  
      if(z[ino].floatexists) strcat(info[n++],"yes");
      else strcat(info[n++],"no");
   strcpy(info[n],"FFT state " );  
      f = z[ino].fftstate;
      if(f==0) sprintf(info[n],"FFT state: Untransformed");
      if(f>=1) sprintf(info[n],"FFT state: Forward FFT'd %dx",f);
      if(f<=-1)sprintf(info[n],"FFT state: Reverse FFT'd %dx",f);
      n++;
   strcpy(info[n],"FFT display " );  
      switch(z[ino].fftdisplay)
      {  case NONE: strcat(info[n],"Original image"); break;
         case REAL: strcat(info[n],"Real"); break;
         case IMAG: strcat(info[n],"Imag"); break;
         case POWR: strcat(info[n],"Power spectrum"); break;
         case WAVE: strcat(info[n],"Wavelets"); break;
         default:  strcat(info[n],"Other"); break;
      }
      n++;
   sprintf(info[n++],"FFT value for black  %+e",z[ino].fmin);
   sprintf(info[n++],"FFT value for white  %+e",z[ino].fmax);
   strcpy(info[n],"FFT=0 color value: " );  
      a = - z[ino].fmin / z[ino].fmax;
      zerocolor = (uint)(a*g.maxvalue[z[ino].bpp]/(a+1));
      itoa(zerocolor,tempstring,10);
      strcat(info[n],tempstring);
   n++;
   valuetoRGB(zerocolor,rr,gg,bb,z[ino].bpp);
      sprintf(info[n++],"FFT=0 RGB value: r %d g %d b %d",rr,gg,bb);
   sprintf(info[n++],"X zoom scale factor: %g",z[ino].zoomx);
   sprintf(info[n++],"Y zoom scale factor: %g",z[ino].zoomy);
   sprintf(info[n++],"Frames/sec: %g",z[ino].fps);
   sprintf(info[n++],"Transparency: %d",z[ino].transparency);
   strcpy(info[n],"Wavelet exists: " );  
      if(z[ino].waveletexists) strcat(info[n++],"yes");
      else strcat(info[n++],"no");
   strcpy(info[n],"Wavelet state " );  
      f = z[ino].waveletstate;
      if(f==0) sprintf(info[n],"Wavelet state: Untransformed");
      if(f>=1) sprintf(info[n],"Wavelet state: Forward Transformed %dx",f);
      if(f<=-1)sprintf(info[n],"Wavelet state: Reverse Transformed %dx",f);
      n++;
   sprintf(info[n++],"Wavelet minimum value %+e",z[ino].wmin);
   sprintf(info[n++],"Wavelet maximum value %+e",z[ino].wmax);
   sprintf(info[n++],"Image white  %d", z[ino].gray[0]);
   sprintf(info[n++],"Image black  %d", z[ino].gray[1]);
   sprintf(info[n++],"Screen white %d", z[ino].gray[2]);
   sprintf(info[n++],"Screen black %d", z[ino].gray[3]);
   if(n>MAXITEMS) printf("Error in about_image_fill\n");
   return n-1;
}


//--------------------------------------------------------------------------//
// test                                                                     //
//--------------------------------------------------------------------------//
void test(void)
{   
#ifdef CONVEX   // No gettimeofday() in ConvexOS
    message("Frame rate testing not available on Convex",ERROR);
#else    
    if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
    char tempstring[200];
    int k;
    const int FRAMES = 10;
    int color=0;
    static int busy=0;
    g.getout=0;
    message("Click to begin testing...\n(ESC to abort)");
    if(busy) return;
    busy=1;

    timeval t;   

#ifndef VMS
    struct timezone tz;    // This requires the word 'struct'
#endif

    double t1;
    double t2;
    double tt1,tt2;
    double dt;
    double fps=0.0;
    double bps;

#ifdef VMS
    gettimeofday(&t, NULL);
#else
    gettimeofday(&t, &tz);
#endif
    t1 = t.tv_sec;
    tt1 = (double)(t.tv_usec)/1000000.0;
    dt = t1 + tt1;
    printstatus(TESTING);
 
    for(k=0;k<FRAMES;k++)
    {   color++;
        if(color>=(int)g.maxcolor) color=0;
        copyimage(0,0,g.main_xsize-1,g.main_ysize-1,0,0,PUT,
            z[0].image_ximage,g.main_window);
    }
    XFlush(g.display);
    XSync(g.display,TRUE);

   printstatus(g.state);
   if(!g.getout)
    {   
#ifdef VMS
        gettimeofday(&t,NULL);  
#else
        gettimeofday(&t,&tz);  
#endif
        t2 = t.tv_sec - t1;
        tt2 = (double)(t.tv_usec)/1000000.0 - tt1;
        dt = t2 + tt2;
        if(dt!=0.0) fps = FRAMES/dt;
        bps = fps * g.main_xsize * g.main_ysize * g.off[g.bitsperpixel];
        sprintf(tempstring,"cpu time %g   %dx%d pixels\n%g frames/sec\n%g bytes/sec",
           dt, g.main_xsize, g.main_ysize, fps, bps); 
    }else strcpy(tempstring,"Aborted");
    copybackground(1, 1, g.main_xsize-1, g.main_ysize-1, -1); 
    selectimage(ci);
    busy=0;
    message(tempstring);
#endif
}


//--------------------------------------------------------------------------//
// printstatus                                                              //
// Prints the cursor size, drawing color & draw status  (items that don't   //
// change frequently). Called when status changes or mouse moves or clicked.//
//--------------------------------------------------------------------------//
void printstatus(int state)
{   printstatus(state, (char*)" ");
}
void printstatus(int state, char *message)
{
    static char temp[81];
    static char temp2[81];
    int w=0,h=0,fc,bc;
    g.inmenu++;

    ////  Read the pixel value for information_area Widget.
    XtVaGetValues(g.info_area[0], XmNforeground, &fc, XmNbackground, &bc, NULL);         

    ////  Labels for X and Y mouse coordinates
    print((char*)"Absolute coord.",8,ypos[0],fc,bc,&g.gc,g.info_window[0],BACKGROUND,HORIZONTAL);
    print((char*)"x",8,ypos[1],fc,bc,&g.gc,g.info_window[0],BACKGROUND,HORIZONTAL);
    print((char*)"y",8,ypos[2],fc,bc,&g.gc,g.info_window[0],BACKGROUND,HORIZONTAL);
    print((char*)"Relative coord.",8,ypos[3],fc,bc,&g.gc,g.info_window[0],BACKGROUND,HORIZONTAL);
    print((char*)"x",8,ypos[4],fc,bc,&g.gc,g.info_window[0],BACKGROUND,HORIZONTAL);
    print((char*)"y",8,ypos[5],fc,bc,&g.gc,g.info_window[0],BACKGROUND,HORIZONTAL);


    //// Cursor/click size
    sprintf(temp2,"Step=%4.4d",g.csize);
    print(temp2,8,ypos[0],fc,bc,&g.gc,g.info_window[3],BACKGROUND,HORIZONTAL);

    //// Screen bpp
    sprintf(temp,"Screen bpp %d ",g.bitsperpixel);
    print(temp,8,ypos[2],fc,bc,&g.gc,g.info_window[3],BACKGROUND,HORIZONTAL);

    //// Global state
    switch(state)
    {   case MOVEIMAGE:
        case NORMAL:         strcpy(temp,"Normal mode            "); break;
        case MOVE:           strcpy(temp,"Moving region        "); break;
        case COPY:           strcpy(temp,"Copying region       "); break;
        case POSITIONBOX:    strcpy(temp,"Moving a box         "); break;
        case DENSITOMETRY:   strcpy(temp,"Densitometry         "); break;
        case GETBOX:         strcpy(temp,"Select a box         "); break;
        case REDRAW:         strcpy(temp,"Redrawing...         "); break;
        case BUSY:
               strcpy(temp,"Processing...        ");
               break;
        case INITIALIZING:   strcpy(temp,"Initializing..           "); break;
        case INMENU:         strcpy(temp,"In menu              "); break;
        case GETNEXTPOINT:   strcpy(temp,"Select corner        "); break;
        case CALCULATING:    strcpy(temp,"Calculating...         "); break;
        case CURVE:          strcpy(temp,"Drawing curve        "); break;
        case SELECT:         strcpy(temp,"Selecting area       "); break;
        case GETPOINT:       strcpy(temp,"Select a point       "); break;
        case GETLINE:        strcpy(temp,"Select a line        "); break;
        case NEWIMAGE:       strcpy(temp,"Creating image       "); break;
        case ERASING:        strcpy(temp,"Erasing image        "); break;
        case DRAWING:
           switch(g.draw_figure)
           {  case BOX:      strcpy(temp,"Drawing boxes        "); break;
              case SKETCH:   strcpy(temp,"Sketch mode          "); break;
              case LINE:     strcpy(temp,"Drawing lines        "); break;
              case CIRCLE:   strcpy(temp,"Drawing circle       "); break;
              case ARROW:    strcpy(temp,"Drawing arrows       "); break;
              default:       strcpy(temp,"Drawing mode         "); break;
           }
           break;
        case READING:        strcpy(temp,"Reading image        "); break;
        case WRITING:        strcpy(temp,"Writing image        "); break;
        case ROTATEPALETTE:  strcpy(temp,"Intro screen         "); break;
        case WAITCHAR:       strcpy(temp,"Press a key...       "); break;
        case FILL:           strcpy(temp,"Fill mode            "); break;
        case PRINTING:       strcpy(temp,"Printing...          "); break;
        case TESTING:        strcpy(temp,"Testing...wait       "); break;
        case MACRO:          strcpy(temp,"Macro Running        "); break;
        case SCANNING:       strcpy(temp,"Acquiring scan       "); break;
        case XTWARNING:      strcpy(temp,"Motif Warning        "); break;
        case SPRAY:          strcpy(temp,"Spraying             "); break;
        case REDUCE_COLORS:  strcpy(temp,"Reducing colors      "); break;
        case ACQUIRE:        strcpy(temp,"Acquiring image      "); break;
        case MESSAGE:
        case MESSAGE_INVERTED:
                             strcpy(temp,message); break;
        case SELECTCURVE:    strcpy(temp,"Selecting areas      "); break;
        case ZOOMING:        strcpy(temp,"Zooming              "); break;
        case MAGNIFYING:     strcpy(temp,"Magnifying           "); break;
        default:             strcpy(temp,"Other                "); break;
    }
    if(state==MESSAGE_INVERTED || state==NORMAL || state==MOVEIMAGE)    
        print(temp,8,ypos[1],fc,bc,&g.gc,g.info_window[3],BACKGROUND,HORIZONTAL);
    else
        print(temp,8,ypos[1],bc,fc,&g.gc,g.info_window[3],BACKGROUND,HORIZONTAL);

    if(g.selectedimage>=0)
       sprintf(temp,"#%d selected          ",ci);
    else
    {  w = g.selected_lrx-g.selected_ulx;
       h = g.selected_lry-g.selected_uly;
       sprintf(temp,"%d x %d area           ",w,h);
    }
    print(temp,8,ypos[3],fc,bc,&g.gc,g.info_window[3],BACKGROUND,HORIZONTAL);

    XFlush(g.display);
    g.inmenu--;
}                           


//--------------------------------------------------------------------------//
// printcoordinates - print current coordinates at left.                    //
// Prints items that change frequently.  Set 'exposed' to redraw even if    //
//  mx and my are unchanged.                                                //
// If exposed = 2, read colors from sample palette window.                  //
//--------------------------------------------------------------------------//
void printcoordinates(uint mx, uint my, int exposed)
{            
    static uint oldpixel=0;      
    static char temp[1024];
    static char temp2[256];
    static char temp3[256];
    static char tempx[256];
    static char tempy[256];

    static int oino=-1;
    static uint omx=0,omy=0;
    static char tempstring[256];
    static int fc=0, bc=0;
    static int hitcalib=0;
    static int count=0;
    
    int bb, bpp, cf, gg, h, ino, k, oinmenu, rr, x1,y1,x2,y2,ww,hh,len;
    uint pix;
    double fpixel, pixcal[3];

    if(exposed){ omx=0; omy=0; oldpixel=0; }
    ////  Read the colors for information_area Widget.
    if(fc==0 && bc==0)
       XtVaGetValues(g.info_area[0], XmNforeground, &fc, XmNbackground, &bc, NULL);         

    oinmenu=g.inmenu;
    g.inmenu=0;   
    ino = min(max(0, g.imageatcursor), g.image_count-1);
    cf = z[ino].cf;
    bpp = z[ino].bpp;

               //-------------x coordinate-----------------//
    x1 = x2 = mx;
    y1 = y2 = my;
    if(ino>=0 && z[ino].shell) // absolute coord. w.r.t. shell or tnimage window
    {   x1 -= z[ino].xpos;
        y1 -= z[ino].ypos;
    }
    if(ino>=0)                 // relative coord. w.r.t. upper left of image
    {   if(z[ino].is_zoomed)
        {   x2 =  zoom_x_index(x2, ino);
            y2 =  zoom_y_index(y2, ino);
        }else
        {   x2 -= z[ino].xpos;
            y2 -= z[ino].ypos;
        }
    }
    if(between(x2, 0, z[ino].xsize-1) && between(y2, 0, z[ino].ysize-1))
        pix = pixelat(z[ino].image[cf][y2]+x2*g.off[bpp],bpp); 
    else
        pix = 0;

    g.inmenu=oinmenu;
    g.inmenu++;

    if(mx!=omx)
    {   sprintf(tempx,"%d    ",x1);
        print(tempx,24,ypos[1],fc,bc,&g.gc,g.info_window[0],BACKGROUND,HORIZONTAL);
        sprintf(tempx,"%d    ",x2);
        print(tempx,24,ypos[4],fc,bc,&g.gc,g.info_window[0],BACKGROUND,HORIZONTAL);
    }

               //-------------y coordinate-----------------//
    if(my!=omy)
    {   sprintf(tempy,"%d    ",y1);
        print(tempy,24,ypos[2],fc,bc,&g.gc,g.info_window[0], BACKGROUND,HORIZONTAL);
        sprintf(tempy,"%d    ",y2);
        print(tempy,24,ypos[5],fc,bc,&g.gc,g.info_window[0], BACKGROUND,HORIZONTAL);
    }

    if(g.selected_is_square && g.mouse_button && g.selectedimage<0)
    {   ww = abs(g.selected_lrx-g.selected_ulx);
        hh = abs(g.selected_lry-g.selected_uly);
        sprintf(temp,"%d x %d area           ",ww,hh);
        print(temp,8,ypos[3],fc,bc,&g.gc,g.info_window[3],BACKGROUND,HORIZONTAL);
        if(ww)
          sprintf(temp,"%g%c               ",  DEGPERRAD * atan((double)hh/(double)ww),XK_degree);  
        else
          sprintf(temp,"90%c                ",XK_degree);  
        print(temp,8,ypos[4],fc,bc,&g.gc,g.info_window[3],BACKGROUND,HORIZONTAL);
    }



               //-------------Image no.--------------------//

    if(g.image_count>0)
    {   if(exposed==2) sprintf(temp,"Sample palette ");   
        else if(ino>0)
        {   bpp = z[ino].bpp;
            sprintf(temp,"Image#%4.4d    ", g.imageatcursor);   
        }
        else if(ino==0) sprintf(temp,"Background     ");   
        else if(ino==-2) sprintf(temp,"Palette        ");   
        print(temp, 8, ypos[0], fc, bc, &g.gc, g.info_window[2], BACKGROUND, HORIZONTAL);
    }else
    {
        print((char*)"No Images  ",8,ypos[0],fc,bc, &g.gc, g.info_window[2],
            BACKGROUND,HORIZONTAL);
    }
    if(ino==-2) pix = my; 

                 //-----------total color value--------------//

    oinmenu=g.inmenu;
    g.inmenu=0;
    if(g.maxvalue[bpp]) fpixel = (double)pix/(double)g.maxvalue[bpp];
    else fpixel = pix;
    g.inmenu=oinmenu;
    if(pix!=oldpixel || ino!=oino || oino==-1)
    {
       doubletostring(fpixel,g.signif,temp);
       sprintf(temp2,"v=%s             ",temp);
       print(temp2,8,ypos[1],fc,bc, &g.gc, g.info_window[1],BACKGROUND,HORIZONTAL);

       sprintf(temp2,"i=%d             ",pix);
       print(temp2,8,ypos[0],fc,bc, &g.gc, g.info_window[1],BACKGROUND,HORIZONTAL);
         
       ////  rgb value
       ////  In screen modes higher than 8bpp, temporarily switch to the 
       ////  image's palette if it has one (i.e., if image is 8bpp).       
       ////  In 8bpp screen modes, this has already been done.      

       if(exposed == 2)
       {   rr = sample[my][mx].red;
           gg = sample[my][mx].green;
           bb = sample[my][mx].blue;
           sprintf(temp,"r%3.3d g%3.3d b%3.3d ",rr,gg,bb);
           print(temp,8,ypos[2],fc, bc, &g.gc, g.info_window[1], BACKGROUND, HORIZONTAL);
           oldpixel = pix;
       }else if(bpp>=48)
       {
           oinmenu=g.inmenu;
           g.inmenu=0;
           readRGBonimage(mx, my, bpp, ino, rr, gg, bb, -2);
           g.inmenu = oinmenu;
           sprintf(temp,"r %5.5d             ",rr);
           print(temp,8,ypos[0],fc, bc, &g.gc, g.info_window[1], BACKGROUND, HORIZONTAL);
           sprintf(temp,"g %5.5d             ",gg);
           print(temp,8,ypos[1],fc, bc, &g.gc, g.info_window[1], BACKGROUND, HORIZONTAL);
           sprintf(temp,"b %5.5d             ",bb);
           print(temp,8,ypos[2],fc, bc, &g.gc, g.info_window[1], BACKGROUND, HORIZONTAL);
       }else
       {   if(ino>=0 && z[ino].colortype==GRAY)
               pix = grayvalue(pix, ino, bpp);
           image_valuetoRGB(pix,rr,gg,bb,bpp,ino);

           sprintf(temp,"r%3.3d g%3.3d b%3.3d ",rr,gg,bb);
           oldpixel = pix;
           print(temp,8,ypos[2],fc, bc, &g.gc, g.info_window[1], BACKGROUND, HORIZONTAL);
       }
       itoa(bpp,temp,10);                 
   
       if(bpp<10)strcat(temp," ");
       strcat(temp," bits/pixel ");
       print(temp, 8, ypos[1], fc, bc, &g.gc, g.info_window[2], BACKGROUND, HORIZONTAL);

       if(ino>=0) strncpy(temp, basefilename(z[ino].name), 1024);
       len = strlen(temp);
       for(k=len; k<30; k++) temp[k]=' ';
       temp[30]=0;
       print(temp,8,ypos[2],fc,bc,&g.gc,g.info_window[2],BACKGROUND,HORIZONTAL);      
    }
    
    ////  calibration
    for(h=0;h<3;h++)      
        if(ino>=0 && z[ino].cal_log[h]!=-1) hitcalib=1;
    if(ino>=0 && hitcalib && (mx!=omx || my!=omy))
    {  
       if(count++>10 || ino!=oino)
       {  count = 0;
          for(h=0;h<3;h++)
          {  strcpy(tempstring, z[ino].cal_title[h]);
             tempstring[15]=0; 
             strcat(tempstring, "                   "); 
             print(tempstring,8,ypos[6+2*h],fc,bc,&g.gc,g.info_window[0],BACKGROUND,HORIZONTAL);
          }
       }
       for(h=0; h<3; h++)
       {   calibratepixel(mx,my,ino,h,pixcal[h],tempstring);
           tempstring[15]=0;
           strcat(tempstring,"                    ");
           print(tempstring,8,ypos[7+2*h],fc,bc,&g.gc,g.info_window[0],BACKGROUND,HORIZONTAL); 
       }
    } 
      

    ////  fft value

    if(ino>=0 && z[ino].floatexists)  // ino could be -2 if on palette
    {  
        x1 = mx - z[ino].xpos;
        y1 = my - z[ino].ypos;
        if(x1>=0 && x1<z[ino].xsize && y1>=0 && y1<z[ino].ysize)
        {     sprintf(temp2,"Re %+1.4e", z[ino].fft[y1][x1].real());
              for(k=strlen(temp2);k<12;k++) strcat(temp2," ");
              print(temp2,8,ypos[3],fc,bc,&g.gc,g.info_window[1],BACKGROUND,HORIZONTAL);

              sprintf(temp3,"Im %+1.4e ", z[ino].fft[y1][x1].imag());
              for(k=strlen(temp3);k<12;k++) strcat(temp3," ");
              print(temp3,8,ypos[4],fc,bc,&g.gc,g.info_window[1],BACKGROUND,HORIZONTAL);
        }
    }

    ////  wavelet value

    if(ino>=0 && z[ino].waveletexists)  // ino could be -2 if on palette
    {  
        x1 = mx - z[ino].xpos;
        y1 = my - z[ino].ypos;
        if(x1>=0 && x1<z[ino].xsize && y1>=0 && y1<z[ino].ysize)
        {     print((char*)"Wavelet",8,ypos[3],fc,bc,&g.gc,g.info_window[1],BACKGROUND,HORIZONTAL);

              sprintf(temp3,"%+1.4e ", z[ino].wavelet[y1][x1]);
              for(k=strlen(temp3);k<12;k++) strcat(temp3," ");
              print(temp3,8,ypos[4],fc,bc,&g.gc,g.info_window[1],BACKGROUND,HORIZONTAL);
        }
    }

    g.inmenu--;
    oino=ino;
    omx=mx;
    omy=my;
} 


//--------------------------------------------------------------------------//
// print_image_title_at_bottom                                              //
// change pixel interaction mode                                            //
//--------------------------------------------------------------------------//
void print_image_title_at_bottom(int ino)
{

   int k,len;
   char temp[FILENAMELENGTH];
   if(ino<0) return;
   strncpy(temp, basefilename(z[ino].name), 1024);
   len = (int)strlen(temp);
   for(k=len; k<256; k++) temp[k]=' ';
   temp[256] = 0;
   XDrawImageString(g.display, XtWindow(g.title_area), g.gc, 0, 11, temp, 256);
}



//--------------------------------------------------------------------------//
// pixelmode                                                                //
// change pixel interaction mode                                            //
//--------------------------------------------------------------------------//
void pixelmode(int noofargs, char **arg)
{ 
  int j,k;
  static Dialog *dialog;
  if(noofargs==0)
  {    dialog = new Dialog;
       if(dialog==NULL){ message(g.nomemory); return; }
       strcpy(dialog->title,"Pixel interaction mode");
       strcpy(dialog->radio[0][0],"Interaction mode");             
       strcpy(dialog->radio[0][1],"Overwrite");
       strcpy(dialog->radio[0][2],"Maximum");
       strcpy(dialog->radio[0][3],"Minimum");
       strcpy(dialog->radio[0][4],"Add");
       strcpy(dialog->radio[0][5],"Subtract new - old");
       strcpy(dialog->radio[0][6],"Subtract old - new");
       strcpy(dialog->radio[0][7],"XOR");
       strcpy(dialog->radio[0][8],"Average");
       strcpy(dialog->radio[0][9],"Superimpose");
       dialog->radioset[0] = g.imode;
       dialog->radiono[0]=10;
       dialog->radiono[1]=0;
       dialog->radiono[2]=0;
       dialog->radiono[3]=0;
       dialog->noofboxes=0;
       dialog->noofradios=1;
       dialog->helptopic=2;
       dialog->width=160;
       dialog->height=0; // calculate automatically
       dialog->want_changecicb = 0;
       for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
       dialog->f1 = pixelmodecheck;
       dialog->f2 = null;
       dialog->f3 = null;
       dialog->f4 = null;
       dialog->f5 = null;
       dialog->f6 = null;
       dialog->f7 = null;
       dialog->f8 = null;
       dialog->f9 = null;
       dialog->transient = 1;
       dialog->wantraise = 0;
       dialog->radiousexy = 0;
       dialog->boxusexy = 0;
       strcpy(dialog->path,".");
       dialog->message[0]=0;      
       dialog->busy = 0;
       dialogbox(dialog);
  }else g.imode=atoi(arg[1]);
}


//--------------------------------------------------------------------------//
//  pixelmodecheck                                                          //
//--------------------------------------------------------------------------//
void pixelmodecheck(dialoginfo *a, int radio, int box, int boxbutton)
{
  radio=radio; box=box; boxbutton=boxbutton;
  g.imode = a->radioset[0];
}  


//--------------------------------------------------------------------------//
// showoptions - list the valid command line options and terminate.         //
//--------------------------------------------------------------------------//
void showoptions(void)
{
    //// Only useful in DOS
}

