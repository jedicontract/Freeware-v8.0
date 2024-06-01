//--------------------------------------------------------------------------//
// xmtnimage24.cc                                                           //
// HP SCSI-2 scanner interface                                              //
// Latest revision: 07-29-2000                                              //
// Copyright (C) 2000 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
// Based on information from the Linux SCSI Programming HOWTO written by    //
// Heiko Eissfeldt heiko@colossus.escape.de  v1.4, 14 June 1995             //
// and on information provided by Hewlett-Packard Corp.                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"
#include "xmtnimaged.h"

extern Globals     g;
extern Image      *z;
extern int         ci;

//// Globals for acquire
ScannerSettings preview;
double previewx=8.5, previewy=11.0;
int noofscanners=0;
int in_acquire=0;

//--------------------------------------------------------------------------//
//  acquire                                                                 //
//--------------------------------------------------------------------------//
int acquire(void)
{
#ifdef CONVEX
  message("Scanning not supported in ConvexOS");
  return ABORT;
#else
  if(memorylessthan(16384)){ message( g.nomemory,ERROR); return NOMEM; }
  static Dialog *dialog;
  if(in_acquire) return BUSY;
  dialog = new Dialog;
  if(dialog==NULL){ message(g.nomemory); return NOMEM; }
  int j,k,inpreview=1;
  char scannerlist[FILENAMELENGTH];
  char tempstring[FILENAMELENGTH];
  in_acquire = 1;
  FILE *fp;
  g.getout=0;
  strcpy(dialog->title,"Acquire Image");
  if(!g.sp.hit)     ////  Starting parameters
  {   
      strcpy(g.sp.scan_device, g.scan_device); 
      g.sp.xres = 150;
      g.sp.yres = 150;
      g.sp.x    = 140;
      g.sp.y    = 140;
      if(g.want_shell && !g.window_border)
      {  g.sp.xpos = 30;
         g.sp.ypos = 30;
      }else
      {  g.sp.xpos = 0;
         g.sp.ypos = 0;
      }
      g.sp.w    = 400;
      g.sp.h    = 500;
      g.sp.brightness= 127;
      g.sp.contrast  = 127;
      g.sp.bpp       = g.bitsperpixel;
      g.sp.hit       = 1;
      g.sp.color     = 0;       
      g.sp.ino       = -1;
      g.sp.shell     = g.want_shell;
      g.sp.border    = g.window_border;
      g.sp.preview   = 0;
      g.sp.mirror    = 0;
      g.sp.widget    = 0;    // Create new Widget
      g.sp.scannerid = g.scannerid;
      g.sp.scanner_name[0] = 0;
      if(g.sp.bpp==8) g.sp.scantype=1; else g.sp.scantype=4;
  }
  preview.hit = 0;
  preview.ino = -1;
  preview.widget = 0;
  strcpy(preview.scanner_name, g.sp.scanner_name);
  preview.scannerid = g.sp.scannerid;
  preview.xpos = g.sp.xpos;
  preview.ypos = g.sp.ypos;

  strcpy(dialog->radio[0][0],"Scan Type");
  strcpy(dialog->radio[0][1],"Preview scan");
  strcpy(dialog->radio[0][2],"Image scan");
  strcpy(dialog->radio[0][3],"Test lamp");
  strcpy(dialog->radio[1][0],"Image bits/pixel");
  strcpy(dialog->radio[1][1]," 8 (grayscale)");
  strcpy(dialog->radio[1][2],"10 (grayscale)");
  strcpy(dialog->radio[1][3],"12 (grayscale)");
  strcpy(dialog->radio[1][4],"24 (color)");
  strcpy(dialog->radio[1][5],"30 (color)");
  strcpy(dialog->radio[1][6],"36 (color)");
  if(inpreview==1) dialog->radioset[0]=1;
  if(inpreview==0) dialog->radioset[0]=2;
  if(inpreview==-1)dialog->radioset[0]=3;
  dialog->radioset[1]=g.sp.scantype;
  dialog->radiono[0]=4;
  dialog->radiono[1]=7;
  dialog->radiono[2]=0;
  for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;

  strcpy(dialog->boxes[0],"Scanner Settings");
  strcpy(dialog->boxes[1],"Scanner");
  strcpy(dialog->boxes[2],"Scanner Device");
  strcpy(dialog->boxes[3],"Resolution(DPI)");
  strcpy(dialog->boxes[4],"Brightness");
  strcpy(dialog->boxes[5],"Contrast");
  strcpy(dialog->boxes[6],"Preview width");
  strcpy(dialog->boxes[7],"Preview height");
  strcpy(dialog->boxes[8],"Mirror image");
  dialog->boxtype[0]=LABEL;
  dialog->boxtype[1]=NON_EDIT_LIST;
  dialog->boxtype[2]=STRING;
  dialog->boxtype[3]=STRING;
  dialog->boxtype[4]=INTCLICKBOX;
  dialog->boxtype[5]=INTCLICKBOX;
  dialog->boxtype[6]=DOUBLECLICKBOX;
  dialog->boxtype[7]=DOUBLECLICKBOX;
  dialog->boxtype[8]=TOGGLE;
  
  strcpy(dialog->answer[2][0],g.sp.scan_device);
  sprintf(dialog->answer[3][0],"%d",g.sp.xres);
  sprintf(dialog->answer[4][0],"%d",g.sp.brightness);
  dialog->boxmin[4]=1; dialog->boxmax[4]=255;
  
  sprintf(dialog->answer[5][0],"%d",g.sp.contrast);
  dialog->boxmin[5]=1; dialog->boxmax[5]=255;
 
  doubletostring(previewx, g.signif, tempstring);
  strcpy(dialog->answer[6][0], tempstring);
  dialog->boxmin[6]=0; dialog->boxmax[6]=15;
 
  doubletostring(previewy, g.signif, tempstring);
  strcpy(dialog->answer[7][0] ,tempstring);
  dialog->boxmin[7]=0; dialog->boxmax[7]=15;
  dialog->boxset[8] = g.sp.mirror;
 
  //// Find list of scanners
  dialog->l[1]       = new listinfo;
  dialog->l[1]->title = new char[128];
  strcpy(dialog->l[1]->title, "Select scanner");
  dialog->l[1]->info  = new char*[1024];   // Max. of 1024 scanners
  noofscanners=1;
  strcpy(g.sp.scanner_name, "hp");
  dialog->l[1]->info[0] = new char[256];
  strcpy(dialog->l[1]->info[0],"hp");

  sprintf(scannerlist,"%s/scanners", g.helpdir);
  if((fp = fopen(scannerlist,"rt")) == NULL)
  {    error_message(scannerlist, errno);
       sprintf(tempstring,"Can't open scanner list\n(should be %s/scanners)", g.helpdir);
       message(tempstring,WARNING);
  }else
  {    while(!feof(fp)) 
       {   dialog->l[1]->info[noofscanners] = new char[256];
           dialog->l[1]->info[noofscanners][0] = 0;
           fgets(dialog->l[1]->info[noofscanners], FILENAMELENGTH, fp);
           remove_terminal_cr(dialog->l[1]->info[noofscanners]);
           noofscanners++;
       }
       fclose(fp);
  }
  sprintf(scannerlist,"%s/scanners",g.homedir);
  if((fp = fopen(scannerlist,"rt")) != NULL)
  {    while(!feof(fp)) 
       {   dialog->l[1]->info[noofscanners] = new char[256];
           dialog->l[1]->info[noofscanners][0]=0;
           fgets(dialog->l[1]->info[noofscanners], FILENAMELENGTH, fp);
           remove_terminal_cr(dialog->l[1]->info[noofscanners]);
           noofscanners++;
       }
       fclose(fp);
  }
  dialog->l[1]->count = noofscanners;
  dialog->l[1]->wantfunction = 0;
  dialog->l[1]->f1    = null;
  dialog->l[1]->f2    = null;
  dialog->l[1]->f3    = null;
  dialog->l[1]->f4    = null; // dialog lists are deleted in dialogcancelcb
  dialog->l[1]->f5    = null;
  dialog->l[1]->f6    = null;
  dialog->l[1]->listnumber = 0;

  g.sp.scannerid = min(g.sp.scannerid, noofscanners-1);
  strcpy(g.sp.scanner_name, dialog->l[1]->info[g.sp.scannerid]);
  strcpy(dialog->answer[1][0],g.sp.scanner_name);
  dialog->l[1]->selection = &g.sp.scannerid;
  dialog->boxmin[1]=0; dialog->boxmax[1]=noofscanners;

  dialog->want_changecicb = 0;
  dialog->f1 = scancheck;
  dialog->f2 = null;
  dialog->f3 = null;
  dialog->f4 = scanfinish;
  dialog->f5 = null;
  dialog->f6 = null;
  dialog->f7 = null;
  dialog->f8 = null;
  dialog->f9 = null;
  dialog->width = 0;  // calculate automatically
  dialog->height = 0; // calculate automatically
  dialog->noofradios = 2;
  dialog->noofboxes = 9;
  dialog->helptopic = 39;
  strcpy(dialog->path, ".");
  dialog->transient = 0;
  dialog->wantraise = 1;
  dialog->radiousexy = 0;
  dialog->boxusexy = 0;
  dialog->message[0] = 0;      
  dialog->busy = 0;
  dialogbox(dialog);
  //g.want_switching = 0;
  return OK;
#endif
}

 
//--------------------------------------------------------------------------//
//  scancheck                                                               //
//--------------------------------------------------------------------------//
void scancheck(dialoginfo *a, int radio, int box, int boxbutton)
{
  int status=OK,inpreview=1,x1,x2,y1,y2;
  radio=radio; box=box; boxbutton=boxbutton;
  g.getout = 0;
  if(radio != -2) return;   //// Return unless user clicked OK

  if(a->radioset[0]==1) inpreview=1; 
  if(a->radioset[0]==2) inpreview=0; 
  if(a->radioset[0]==3) inpreview=-1; // lamp test
  
  g.sp.scantype = a->radioset[1];
  switch(g.sp.scantype)
  {    case  1: g.sp.bpp= 8; g.sp.color=0; break;
       case  2: g.sp.bpp=10; g.sp.color=0; break;
       case  3: g.sp.bpp=12; g.sp.color=0; break;
       case  4: g.sp.bpp=24; g.sp.color=1; break;
       case  5: g.sp.bpp=30; g.sp.color=1; break;
       case  6: g.sp.bpp=36; g.sp.color=1; break;
  }

  strcpy(g.sp.scanner_name, a->answer[1][0]);
  g.sp.scannerid = *a->l[1]->selection;
  g.scannerid = g.sp.scannerid;
  strcpy(g.sp.scan_device, a->answer[2][0]);
  g.sp.xres = max(1, atoi(a->answer[3][0]));
  g.sp.yres = g.sp.xres;
  g.sp.brightness=atoi(a->answer[4][0]);
  g.sp.contrast=atoi(a->answer[5][0]);
  previewx = atof(a->answer[6][0]);
  previewy = atof(a->answer[7][0]);
  g.sp.mirror = a->boxset[8];

  ////  End of input section

  strcpy(preview.scan_device,g.sp.scan_device);
  strcpy(preview.scanner_name,g.sp.scanner_name);
  preview.scannerid = g.sp.scannerid;
  preview.xres = 50;
  preview.yres = 50;
  preview.x    = 0;
  preview.y    = 0;
  ////  Make sure image position is consistent, because the old widget
  ////  will be re-used with a different ino.
  if(preview.ino>=0)
  {    preview.xpos = z[preview.ino].xpos;      
       preview.ypos = z[preview.ino].ypos;      
  }
  preview.w    = (int)(300 * previewx);
  preview.h    = (int)(300 * previewy);  
  preview.brightness  = g.sp.brightness;
  preview.contrast    = g.sp.contrast;
  preview.bpp         = g.sp.bpp;
  preview.color       = g.sp.color;
  preview.shell       = 1;
  preview.border      = 1;
  preview.preview     = 1;
  preview.mirror      = g.sp.mirror;

  if(inpreview==-1) // lamp test
  {    lamptest(&g.sp);      
  }else if(inpreview==1)
  {    
       if(preview.hit && preview.ino > -1) 
            eraseimage(preview.ino,0,1,1);     // Keep old widget
       g.waiting++;
       status = acquire_scan(&preview);        // Get new preview
       g.waiting = max(0, g.waiting-1);
                                               // newimage() failed or SCSI error   
       preview.hit = 1;
       if(preview.ino==-1 || status || g.getout) return; 
  }else if(preview.hit)
  {    
       message("Select a region for image scan");
       if(g.getout)
            g.getout=0;
       else
       {        ////  Make them select region to scan
            getbox(x1,y1,x2,y2);                 
                ////  Device pixels = pixels at 300 dpi
            g.sp.xpos = 0;
            g.sp.ypos = 0;
            g.sp.x = max(0,(x1 -z[preview.ino].xpos));
            g.sp.y = max(0,(y1 -z[preview.ino].ypos));
            g.sp.w = min(x2 - x1, z[preview.ino].xsize);
            g.sp.h = min(y2 - y1, z[preview.ino].ysize);
                ////  Correct for inaccurate size calculated by hp scanner
            if(g.sp.scannerid==0)
            {
                g.sp.x *= 300 / preview.xres;
                g.sp.y *= 300 / preview.xres;
                g.sp.w *= 300 / preview.xres;
                g.sp.h *= 300 / preview.xres;
            }else
            {  
                g.sp.x *= 300 / preview.xres;
                g.sp.y *= 300 / preview.xres;
                g.sp.w *= g.sp.xres / preview.xres;
                g.sp.h *= g.sp.xres / preview.xres;
            }            
            g.waiting++;
            status = acquire_scan(&g.sp);
            g.waiting = max(0, g.waiting-1);
            if(g.sp.ino==-1 || status || g.getout) return;  // newimage() failed or error  
            if(g.sp.ino==-1) return;                    
            if(g.getout) return;
       }
  }else         ////  Make them scan a preview image
  {    message("You must scan a preview image first", ERROR);
       inpreview = 1;
  }
  g.getout=0;
  if(strlen(g.sp.scanner_name)) strcpy(g.scan_device, g.sp.scan_device);
  return;
}


//--------------------------------------------------------------------------//
//  scanfinish                                                              //
//--------------------------------------------------------------------------//
void scanfinish(dialoginfo *a)
{
 a=a;
 in_acquire = 0;
 g.want_switching = 1;
 if(preview.ino > -1) eraseimage(preview.ino, 0, 0, 1);    // Dont ask to save it
}


//--------------------------------------------------------------------------//
//  acquire_scan                                                            //
//--------------------------------------------------------------------------//
int acquire_scan(ScannerSettings *sp)
{
   static int scanno=0;
   FILE *fp;
   Widget www;
   int status=OK;
   int owantshell=g.want_shell;
   char tempfile[FILENAMELENGTH];
 
   char driver[FILENAMELENGTH];
   char tempstring[FILENAMELENGTH];
   char scanner_name[256]="";
   char driver_path[FILENAMELENGTH]="";
   char command[2*FILENAMELENGTH]="";
   char name[256]="";
   char value[256]="";
   char device[256]="";
   char dpi_command[256]="";
   char left_command[256]="";
   char top_command[256]="";
   char width_command[256]="";
   char height_command[256]="";
   char gray_8_command[256]="";
   char gray_10_command[256]="";
   char gray_12_command[256]="";
   char gray_16_command[256]="";
   char color_24_command[256]="";
   char color_30_command[256]="";
   char color_36_command[256]="";
   char gray_command[256]="";
   char color_command[256]="";
   char bpp_command[256]="";
   char brightness_command[256]="";
   char contrast_command[256]="";
   char preview_command[256]="";
   char extra_commands[256]="";
   char redirect[256]="";
   double height_factor=1.0, width_factor=1.0, top_factor=1.0, 
          left_factor=1.0;
   int contrast_add=0, brightness_add=0;
 
   sprintf(tempfile,"%s/fifi",g.homedir);
   sprintf(redirect,">%s",tempfile);
   if(sp->scannerid==0) 
   {     g.busy = 1;
         status = acquire_hp_scan(sp);
         g.busy = 0;
   }else
   {   
        if(sp->preview) g.want_shell = 1;
        sprintf(driver,"%s/%s",g.homedir,sp->scanner_name);
        if((fp = fopen(driver,"rt"))==NULL)   
        {    sprintf(driver,"%s/%s", g.helpdir, sp->scanner_name);
             if((fp = fopen(driver,"rt"))==NULL)   
             {   error_message(driver, errno);
                 sprintf(tempstring,"Can't open scanner configuration file\n(should be %s)",driver);
                 message(tempstring, ERROR);
                 return ERROR;
             }
        }
        while(!feof(fp))
        {   
            fgets(tempstring, FILENAMELENGTH, fp);
            if(!strlen(tempstring) || tempstring[0]=='#') continue;
            name[0]=0;
            value[0]=0;
            sscanf(tempstring,"%s %s",name,value);
            if(!strlen(name) || (!strlen(value))) continue;
            if(!strcmp(name,"name"))            { strcpy(scanner_name,value);continue;}
            if(!strcmp(name,"device"))          { strcpy(device,value);continue;}
            if(!strcmp(name,"driver_path"))     { strcpy(driver_path,value);continue;}
            if(!strcmp(name,"dpi_command"))     { strcpy(dpi_command,value);continue;} 
            if(!strcmp(name,"left_command"))    { strcpy(left_command,value);continue;} 
            if(!strcmp(name,"top_command"))     { strcpy(top_command,value);continue;} 
            if(!strcmp(name,"width_command"))   { strcpy(width_command,value);continue;} 
            if(!strcmp(name,"height_command"))  { strcpy(height_command,value);continue;} 
            if(!strcmp(name,"gray_8_command"))  { strcpy(gray_8_command,value);continue;} 
            if(!strcmp(name,"gray_10_command")) { strcpy(gray_10_command,value);continue;} 
            if(!strcmp(name,"gray_12_command")) { strcpy(gray_12_command,value);continue;} 
            if(!strcmp(name,"gray_16_command")) { strcpy(gray_16_command,value);continue;} 
            if(!strcmp(name,"color_24_command")){ strcpy(color_24_command,value);continue;} 
            if(!strcmp(name,"color_30_command")){ strcpy(color_30_command,value);continue;} 
            if(!strcmp(name,"color_36_command")){ strcpy(color_36_command,value);continue;} 
            if(!strcmp(name,"gray_command"))    { strcpy(gray_command,value);continue;} 
            if(!strcmp(name,"color_command"))   { strcpy(color_command,value);continue;} 
            if(!strcmp(name,"bpp_command"))     { strcpy(bpp_command,value);continue;} 
            if(!strcmp(name,"brightness_command")){ strcpy(brightness_command,value);continue;} 
            if(!strcmp(name,"contrast_command"))  { strcpy(contrast_command,value);continue;} 
            if(!strcmp(name,"preview_command"))   { strcpy(preview_command,value);continue;} 
            if(!strcmp(name,"extra_commands"))    { strcpy(extra_commands,value);continue;} 
            if(!strcmp(name,"height_factor"))     { height_factor=atof(value);continue;} 
            if(!strcmp(name,"width_factor"))      { width_factor=atof(value);continue;} 
            if(!strcmp(name,"top_factor"))        { top_factor=atof(value);continue;} 
            if(!strcmp(name,"left_factor"))       { left_factor=atof(value);continue;} 
            if(!strcmp(name,"contrast_add"))      { contrast_add=atoi(value);continue;} 
            if(!strcmp(name,"brightness_add"))    { brightness_add=atoi(value);continue;} 
        }
        fclose(fp);
        if(!strlen(driver_path))
        {    message("No driver specified",ERROR); 
             g.want_shell=owantshell; 
             return ERROR; 
        }
        strcpy(command, driver_path);
        strcat(command," ");
        add_option(command, dpi_command, sp->xres);
        add_option(command, left_command, (int)(sp->x*left_factor));
        add_option(command, top_command, (int)(sp->y*top_factor));
        add_option(command, width_command, (int)(sp->w*width_factor));
        add_option(command, height_command, (int)(sp->h*height_factor));
        add_option(command, brightness_command, sp->brightness+brightness_add);
        add_option(command, contrast_command, sp->contrast+contrast_add);
        add_option(command, bpp_command, sp->bpp);
        switch(sp->bpp)
        { 
              case 8:  add_option(command, gray_8_command);
              case 10: add_option(command, gray_10_command);
              case 12: add_option(command, gray_12_command);
              case 16: add_option(command, gray_16_command);
              case 24: add_option(command, color_24_command);
              case 30: add_option(command, color_30_command);
              case 36: add_option(command, color_36_command);
        }
        if(sp->color) add_option(command, color_command);
        else add_option(command, gray_command);
        strcat(command,extra_commands);
        strcat(command,redirect);

        if(g.diagnose) printf("Scanner command:\n %s\n",command);
        sprintf(tempstring,"Scanning %s",sp->scanner_name); 
        www = message_window_open(tempstring);

        g.busy = 1;
        status = system(command);
        message_window_close(www);
        if(status) 
        {   message("Error executing scanner command",ERROR); 
            g.want_shell=owantshell; 
            g.busy = 0;
            return ERROR; 
        }

        //// Cant use a fifo because readimage() has to open the
        //// file once to determine the image format and once to
        //// read the image. With a pipe, it would have to assume
        //// a specific image format.
        status = readimage(tempfile,0,NULL);
        unlink(tempfile);     
        g.busy = 0;

        if(status) 
        {   sprintf((char*)"Error reading %s\n",tempfile);
            message(tempstring,ERROR); 
            g.want_shell=owantshell; 
            return ERROR; 
        }
        if(sp->preview) setimagetitle(ci,"Scanner");
        else
        {    sprintf(tempstring,"Scan%d",scanno++);
             setimagetitle(ci,tempstring);
        }
        sp->ino = ci;
        sp->widget = z[ci].widget;
        sp->xpos = z[ci].xpos;
        sp->ypos = z[ci].ypos;
        z[ci].touched = 1;                     // Image is touched
   }
   g.want_shell=owantshell; 
   g.want_switching = 1;
   switchto(ci);
   return status;

}


//--------------------------------------------------------------------------//
//  add_option                                                              //
//--------------------------------------------------------------------------//
void add_option(char *command, char *option)
{
    char tempstring[FILENAMELENGTH];
    if(strlen(option)) 
    {    sprintf(tempstring,"%s ",option);
         strcat(command,tempstring);
    }

}
void add_option(char *command, char *option, int value)
{
    char tempstring[FILENAMELENGTH];
    if(strlen(option)) 
    {    sprintf(tempstring,"%s %d ",option,value);
         strcat(command,tempstring);
    }
}


#ifndef CONVEX
//--------------------------------------------------------------------------//
//  acquire_hp_scan                                                         //
//--------------------------------------------------------------------------//
int acquire_hp_scan(ScannerSettings *sp)
{
  static int scanno = 1;
  Widget progress, scrollbar;
  char buf[8192];
  char tempstring[100];
  char vendor[50], model[50];
  int color_type, ct, data_type, bad=0,b,f,fd,j,k,line=0,n,type=0,x,xx,y,
      offset=0, errors=0, error_code=0, max_bpp, max_xres, maxw, maxh,
      want_shell;
  Widget savewidget;

  if(sp->color) ct=COLOR; else ct=GRAY;
  if(sp->color) color_type=5; else color_type=4;
  if(sp->preview) savewidget=sp->widget; else savewidget=0;
  if((fd = open(sp->scan_device, O_RDWR))<0)
  {   message("Can't open scanner");
      return(NOTFOUND);
  }          

  g.getout = 0;
  type = scanner_type(fd, vendor, model);
  if(type!=3 || strstr(vendor,"HP")==NULL)
      message("Not an HP scanner\nContinuing may cause a lockup\nDo you want to continue?",
         WARNING);
  if(g.getout){ close(fd); return ABORT; }


  ////  Set up scanner

  sg_out(fd, (char*)"\033E");                           // reset scanner
  sg_out(fd, (char*)"\033*a%dR", sp->xres);             // set x resolution in DPI
  sg_out(fd, (char*)"\033*a%dS", sp->yres);             // set y resolution in DPI
  sg_out(fd, (char*)"\033*f%dX", sp->x);                // set x left in device pixels
  sg_out(fd, (char*)"\033*f%dY", sp->y);                // set y top in device pixels
  sg_out(fd, (char*)"\033*f%dP", sp->w);                // set x width in device pixels
  sg_out(fd, (char*)"\033*f%dQ", sp->h);                // set y height in device pixels
  sg_out(fd, (char*)"\033*a%dL", sp->brightness-128);   // set brightness: -127 to +127
  sg_out(fd, (char*)"\033*a%dK", sp->contrast-128);     // set contrast: -127 to +127
  if(sp->bpp==24 || sp->bpp==8) sg_out(fd, (char*)"\033*a1I");          // invert the bytes
  sg_out(fd, (char*)"\033*a%dT",color_type);            // color or grayscale
  sg_out(fd, (char*)"\033*a%dM",sp->mirror);            // mirror image
  sg_out(fd, (char*)"\033*u0K");                        // automatically create tone map
  sg_out(fd, (char*)"\033*a%dG", sp->bpp);              // set bits/pixel

  ////  Check parameters

  sg_ask(fd,SL_BITS_PER_PIXEL,HIGH,&max_bpp);    // find max bpp in scanner
  sg_ask(fd,SL_DATA_TYPE,CURRENT,&data_type);    // 5 = scanner is color
  sg_ask(fd,SL_X_RESOLUTION,HIGH,&max_xres);  
  sg_ask(fd,SL_XEXT_PIXEL,HIGH,&maxw);  
  sg_ask(fd,SL_YEXT_PIXEL,HIGH,&maxh);  
   
  if(sp->bpp > max_bpp)
  {  sprintf(tempstring,"Scanner does not support \n%d bits/pixel",sp->bpp); bad=1; }  
  if(color_type != data_type)
  {  sprintf(tempstring,"Scanner does not support color"); bad=1; }
  if(sp->xres > max_xres)
  {  sprintf(tempstring,"Scanner does not support %d dpi",sp->xres); bad=1; }
  if(sp->w > maxw){ sprintf(tempstring,"Scanning area too wide"); bad=1; }
  if(sp->h > maxh){ sprintf(tempstring,"Scanning area too long"); bad=1; }
  if(bad){ message(tempstring,ERROR); close(fd); return OK; }

  sg_ask(fd,SL_PIXELS_PER_LINE,DEVICE,&x);       // obtain pixels/line in x
  sg_ask(fd,SL_BYTES_PER_LINE,DEVICE,&b);        // obtain bytes/line in b
  sg_ask(fd,SL_LINES_PER_SCAN,DEVICE,&y);        // obtain bytes/line in y

  ////  Allocate space for image

  if(sp->preview) want_shell = 1; else want_shell = g.want_shell;  
  if(newimage("Scan", sp->xpos, sp->ypos, x, y, sp->bpp, ct, 1, want_shell, 0,
       PERM, 1, 1, savewidget) != OK)
  {    close(fd); return(NOMEM); }
  if(sp->preview) setimagetitle(ci,"Scanner");
  else
  {    sprintf(tempstring,"Scan%d",scanno++);
       setimagetitle(ci,tempstring);
  }

  sp->ino = ci;
  sp->widget = z[ci].widget;
  sp->xpos = z[ci].xpos;
  sp->ypos = z[ci].ypos;

  sprintf(tempstring,"Scanning %s",sp->scanner_name); 
  progress_bar_open(progress, scrollbar);
  print((char*)"Scanning..        ", 8, 30, g.main_bcolor, g.main_fcolor, &g.gc, g.info_window[3],
       BACKGROUND,HORIZONTAL);

  ////  Start scan

  sg_out(fd, (char*)"\033*f0S");                        
  xx=0;                                          // x position in image buffer
  while((n = sg_in(fd,buf,x)) > 0 && line<z[ci].ysize)
  {   
      sprintf(tempstring,"%d ",line);
      print(tempstring, 68, 30, g.main_bcolor, g.main_fcolor, &g.gc, g.info_window[3],
          BACKGROUND, HORIZONTAL);
      progress_bar_update(scrollbar, line*100/z[ci].ysize);
      if(g.getout) break;
 
      ////  No. of bytes read n may be less than b because SCSI reads a maximum 
      ////  of 4096 bytes, so retaliate by reading a byte at a time.
      
      for(k=0; k<n; k++)
      {   z[ci].image[0][line][xx++] = buf[k];
          if(xx >= b){ xx=0; line++; }
      }
  }

  ////  Temporary fix until 48 bpp is supported; convert image to an artificial
  ////  48 bpp format compatible with 24 bpp by dropping the low bits.
  ////  byte      0         1        2        3        4       5
  ////  36 bpp 0000rrrr rrrrrrrr 0000gggg gggggggg 0000bbbb bbbbbbbb
  ////  30 bpp 000000rr rrrrrrrr 000000gg gggggggg 000000bb bbbbbbbb
  ////  new    00000000 rrrrrrrr 00000000 gggggggg 00000000 bbbbbbbb
  
  int k1,shift1=0,shift2=0,red,grn,blu;
  f = z[ci].cf;

  if(z[ci].bpp==48)
  {   if(sp->bpp==36){ shift1=4; shift2=4; }
      if(sp->bpp==30){ shift1=6; shift2=2; }
      for(j=0;j<y;j++)
      for(k=0,k1=0;k<x;k++,k1+=6)
      {   red = ((0xff & z[ci].image[f][j][k1+0]) << shift1) + 
                ((0xff & z[ci].image[f][j][k1+1]) >> shift2);
          grn = ((0xff & z[ci].image[f][j][k1+2]) << shift1) + 
                ((0xff & z[ci].image[f][j][k1+3]) >> shift2);
          blu = ((0xff & z[ci].image[f][j][k1+4]) << shift1) + 
                ((0xff & z[ci].image[f][j][k1+5]) >> shift2);
          z[ci].image[f][j][k1+5] = 0;
          z[ci].image[f][j][k1+4] = red;
          z[ci].image[f][j][k1+3] = 0;
          z[ci].image[f][j][k1+2] = grn;
          z[ci].image[f][j][k1+1] = 0;
          z[ci].image[f][j][k1+0] = blu;
      }
  }

  if(!g.getout)
  {  sg_out(fd, (char*)"\033*s1U");                // Read grayscale tone map
     if(sg_in(fd, buf, 266) != 266) 
     {
          message("Error reading grayscale tone map",ERROR);
          for(k=0;k<256;k++)z[ci].gamma[k] = (uchar)k;
     }else 
     {    // offset = (int)strchr(buf,'W')-(int)buf+1; // can't have in intel 64 bit
          offset = 0;
          for(k=0; k<min((int)strlen(buf),266); k++){ if(buf[k]=='W') offset=1+k; break; }
          for(k=0;k<256;k++)z[ci].gamma[k] = (uchar)buf[k+offset];
     }       
     sg_ask(fd, SL_ERR_DEPTH,DEVICE, &errors);  
     if(errors)
     {  
         sg_ask(fd,SL_ERR_CURRENT,DEVICE,&error_code);  
         handle_scanner_error(error_code);
         sg_out(fd, (char*)"\033*oE");             // Clear errors
     }
  }
  sg_out(fd, (char*)"\033E");                      // reset scanner  
  close(fd);
  progress_bar_close(progress);
                                         // Swap bytes in grayscale 10 & 12 bpp  
                                         // Scanner puts bytes in the wrong order
  if(sp->bpp==24 || (sp->bpp>8 && sp->color==0)) swap_image_bytes(ci);
  if(sp->color) z[ci].colortype = COLOR; 
  else z[ci].colortype = GRAY; 
  z[ci].touched = 1;                     // Image is touched
  memcpy(z[ci].palette, g.palette, 768);
  memcpy(z[ci].opalette, g.palette,768);
  rebuild_display(ci);
  if(!sp->preview) if(g.autoundo) backupimage(ci,0); 
  redraw(ci);
  switchto(ci);
  return OK; 
}


//--------------------------------------------------------------------------//
//  sg_ask                                                                  //
//  Inquire for scanner information. `kind' can be LOW, HIGH, CURRENT, or   //
//  DEVICE. The 'kind' has to be compatible with the code (see scl.h in     //
//  HP's documentation).                                                    //
//--------------------------------------------------------------------------//
int sg_ask(int fd, int code, int kind, int *answer)
{    
   char string[256];
   char buffer[256];
   char terminator[10] = {'L','H','R','E'}; 
   char response[10]   = {'k','g','p','d'}; 
   sprintf(string,"\033*s%d%c",code,terminator[kind]);
   sg_out(fd, string);
   sg_in(fd, buffer, 128);
   *answer = atoi(strchr(buffer,response[kind])+1);
   return OK;
}


//--------------------------------------------------------------------------//
//  scanner_type                                                            //
//  Find out if it is a HP scanner, or what.                                //
//--------------------------------------------------------------------------//
int scanner_type(int fd, char *vendor, char *model)
{
   int type;
   uchar cmd[6] = { 0x12, 0, 0, 0, 0xff, 0 };
   uchar data[256];
   memset(data,0,255);
   if(send_scsi_command(fd,cmd,6,data,256)<16) return -1;
   memcpy(vendor, data+8, 8);
   memcpy(model, data+16, 16);
   vendor[8]=0; model[16]=0;
   type = data[0];     // 3 means HP
   return type;
}


//--------------------------------------------------------------------------//
//  sg_in                                                                   //
//  Compose and execute a 'read' command for sg scsi device.                //
//--------------------------------------------------------------------------//
int sg_in(int fd, char *data, int len)
{   
   if(g.getout)return ABORT;
   uchar cmd[6] = { 8,0,0,0,0,0 };
   cmd[2] = len >> 16;
   cmd[3] = len >> 8;
   cmd[4] = len;
   if((len = send_scsi_command(fd,cmd,6,(uchar*)data,len))<0) return -1;
   return len;
}   


//--------------------------------------------------------------------------//
//  sg_out                                                                  //
//  Compose and execute a 'write' command for sg scsi device.               //
//--------------------------------------------------------------------------//
int sg_out(int fd, char *format, int value)
{  
   if(g.getout)return ABORT;
   char tempstring[4096];
   sprintf(tempstring,format,value);
   return sg_out(fd,tempstring);
}

int sg_out(int fd, char *data)
{
   uchar w[4096] = { 0,0,0,0,0,0 };  // SCSI-2 max 4096 bytes unless SG_BIG_BUFF
   int len = strlen(data);
   w[0] = 0xa;
   w[2] = len >> 16;
   w[3] = len >> 8;
   w[4] = len;
   memcpy(w+6,data,len);
   if(send_scsi_command(fd,w,len+6,0,0) < 0) return -1;
   return 0;       
}   


//--------------------------------------------------------------------------//
//  send_scsi_command                                                       //
//  Execute any scsi command.                                               //
//  Max buffer size is 4096 bytes unless SG_BIG_BUFF is defined in kernel.  //
//  Send a SCSI command & data supplied in wbuf. The 1st cmd_len bytes of   //
//  wbuf are the command. The device puts rsize bytes of data back in rbuf. //
//  Cannot be called ''scsi_command'' due to conflicting name on Irix.      //
//--------------------------------------------------------------------------//
int send_scsi_command(int fd, uchar *wbuf, int wsize, uchar *rbuf, int rsize)
{   
#ifdef DIGITAL
#define SCSI_OFF 4096
#else
   const int SCSI_OFF = (int)sizeof(struct sg_header);
#endif
   static uchar buf[4096 + SCSI_OFF]; 
   static struct sg_header sg_hd;

   char tempstring[100];
   int count, status, hdsize;
   hdsize = (int)sizeof(sg_hd);
   sg_hd.pack_len = hdsize + wsize;
   sg_hd.reply_len = hdsize  + rsize;
   memcpy(buf, &sg_hd, hdsize);
   memcpy(buf + hdsize, wbuf, wsize);

   status = write(fd, buf, sg_hd.pack_len);
   if(status<0 || status != SCSI_OFF+wsize || sg_hd.result)
   {   sprintf(tempstring, "SCSI write error \nstatus=%d  %d\nResult = 0x%x",
                  status, SCSI_OFF+wsize, sg_hd.result);
       message(tempstring,ERROR);
       g.getout=1;
       return ERROR;
   }

   count = read(fd, buf, sg_hd.reply_len);
   if(count<0 || count != SCSI_OFF+rsize || sg_hd.result) 
   {   sprintf(tempstring, "SCSI read error\ncount = 0x%x,\n result = 0x%x\ncmd = 0x%x\n",
                count, sg_hd.result, rbuf[SCSI_OFF]);
       message(tempstring,ERROR);
       g.getout=1;
       return ERROR;
   }
   if(count>SCSI_OFF) 
   {   count -= SCSI_OFF;
       memcpy(rbuf, buf + hdsize, count);
   }else count = 0;
   return count;
}   


//-------------------------------------------------------------------------//
// lamptest                                                                //
//-------------------------------------------------------------------------//
void lamptest(ScannerSettings *sp)
{
   int j,k,lamp=OFF;
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
   Dialog  *dialog;
   dialog = new Dialog;

   strcpy(dialog->title,"Test Scanner Lamp");
   strcpy(dialog->radio[0][0],"");
   strcpy(dialog->radio[0][1],"On");             
   strcpy(dialog->radio[0][2],"Off");
   
   if(lamp==ON)dialog->radioset[0]=1;
   if(lamp==OFF)dialog->radioset[0]=2;

   dialog->radiono[0]=3;
   for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
   dialog->noofradios=1;
   dialog->noofboxes=0;
   dialog->helptopic=0;  
   dialog->want_changecicb=0;
   g.getout=0;
   dialog->f1 = lamptestcheck;
   dialog->f2 = null;
   dialog->f3 = null;
   dialog->f4 = null;
   dialog->transient = 1;
   dialog->wantraise = 0;
   dialog->radiousexy = 0;
   dialog->boxusexy = 0;
   dialog->width=0;  // calculate automatically
   dialog->height=0; // calculate automatically
   dialog->ptr[0] = (void*)sp;
   strcpy(dialog->path,".");
   dialog->message[0]=0;      
   dialog->busy = 0;
   dialogbox(dialog);
}


//--------------------------------------------------------------------------//
//  lamptestcheck                                                           //
//--------------------------------------------------------------------------//
void lamptestcheck(dialoginfo *a, int radio, int box, int boxbutton)
{
   radio=radio; box=box; boxbutton=boxbutton;
   char vendor[50], model[50];
   int fd,lamp=OFF,type=0;
   g.getout=0;
   ScannerSettings *sp = (ScannerSettings*) a->ptr[0];
   if((fd = open(sp->scan_device, O_RDWR))<0)
   {   message("Can't open scanner");
       g.getout=1;
   } 
   if(!g.getout) 
   {   type = scanner_type(fd, vendor, model);
       if(type!=3 || strstr(vendor,"HP")==NULL)
       {    message("Not an HP scanner", ERROR);
            close(fd); 
            g.getout=1;
       }
   }
   if(a->radioset[0]==1) lamp=ON; else lamp=OFF;
   if(a->radioset[0]==1) lamp=ON; else lamp=OFF;
   if(!g.getout) sg_out(fd,(char*)"\033*f%dL",lamp); 
   g.getout=0;
} 

#endif


//-------------------------------------------------------------------------//
// handle_scanner_error                                                    //
//-------------------------------------------------------------------------//
void handle_scanner_error(int error_code)
{ 
    switch(error_code)
    {   case 0: message("Scanner command format error",ERROR); break;
        case 1: message("Function not supported by scanner",ERROR); break;
        case 2: message("Scanner parameter error",ERROR); break;        
        case 3: message("Illegal scanning window",ERROR); break;        
        case 4: message("Scanner scaling error",ERROR); break;        
        case 5: message("Scanner dither ID error",ERROR); break;        
        case 6: message("Tone map ID error",ERROR); break;        
        case 7: break;// message("Scanner lamp error",ERROR); break;        
        case 8: message("Scanner matrix ID error",ERROR); break;        
        case 9: //message("Scanner calibration parameter error",ERROR); 
                break;        
        case 10: // message("Scanner calibration error",ERROR); 
                break;        
        case 1024: message("Document feeder jam",ERROR); break;        
        case 1025: message("Scanner home position error",ERROR); break;        
        case 1026: message("Scanner ADF out of paper",ERROR); break;        
        default: message("Unknown scanner error",ERROR); break; 
    }        
}    
        
