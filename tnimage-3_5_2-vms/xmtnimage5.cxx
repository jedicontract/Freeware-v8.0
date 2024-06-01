//--------------------------------------------------------------------------//
// xmtnimage5.cc                                                            //
// getcharacter, keyhit, configure, attributes                              //
// Latest revision: 09-29-2005                                              //
// Copyright (C) 2005 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;

int windowsareon=0;
uint keycode=0;
const int NOOFCURSORS=12;
int old210;

//--------------------------------------------------------------------------//
// getcharacter - return most recent keypress (different from DOS)          //
// key_ascii is set in keyhit().                                            //
//--------------------------------------------------------------------------//
int getcharacter(void)
{  
  return g.key_ascii;
}    

//--------------------------------------------------------------------------//
// keyhit                                                                   //
//--------------------------------------------------------------------------//
int keyhit(void)
{ 
   char buffer[20];
   KeySym key;
   XComposeStatus compose;
   XKeyEvent keyevent;
  
   int hit=0, bufsize=20;
   XEvent event;
   hit = XCheckMaskEvent(g.display, KeyPress, &event);
   if(hit) 
   {   keyevent = event.xkey;
       keycode = event.xkey.keycode;
       XLookupString(&keyevent,buffer,bufsize,&key,&compose);
       g.key_ascii= key & 0xff; 
   }
   return hit;
} 
   
//--------------------------------------------------------------------------//
//  configure                                                               //
//--------------------------------------------------------------------------//
void configure(void)
{
   int j,k;
   if(memorylessthan(16384)){ message(g.nomemory,ERROR); return; }
   static Cursor cursor = g.xlib_cursor;
   static int selection;
   Dialog *dialog;
   dialog = new Dialog;
   char tempstring[128];
   strcpy(dialog->title,"Configuration");

   strcpy(dialog->radio[0][0],"Color reduction method");             
   strcpy(dialog->radio[0][1],"Quantization");
   strcpy(dialog->radio[0][2],"Fit current colormap");
   strcpy(dialog->radio[0][3],"None");

   strcpy(dialog->radio[1][0],"Update undo buffer");
   strcpy(dialog->radio[1][1],"Automatically");
   strcpy(dialog->radio[1][2],"Manually");
   strcpy(dialog->radio[1][3],"Never");

   strcpy(dialog->radio[2][0],"Colormap usage");
   strcpy(dialog->radio[2][1],"Allocate colors");
   strcpy(dialog->radio[2][2],"Install colormap");

   strcpy(dialog->radio[3][0],"Message boxes");
   strcpy(dialog->radio[3][1],"None");
   strcpy(dialog->radio[3][2],"Minimal");
   strcpy(dialog->radio[3][3],"Normal");

   strcpy(dialog->radio[4][0],"Area Selection Mode");
   strcpy(dialog->radio[4][1],"Single (freehand)");
   strcpy(dialog->radio[4][2],"Multiple (freehand)");
   strcpy(dialog->radio[4][3],"Polygon (adjustable)");
   strcpy(dialog->radio[4][4],"Point-to-point");

   strcpy(dialog->boxes[0],"Luminosity factor (0-1)");
   strcpy(dialog->boxes[1],"Red");
   strcpy(dialog->boxes[2],"Green");
   strcpy(dialog->boxes[3],"Blue");

   strcpy(dialog->boxes[4],"Significant digits"); 
   strcpy(dialog->boxes[5],"Cursor movement"); 
   strcpy(dialog->boxes[6],"Spray factor"); 

   strcpy(dialog->boxes[7],"Active Color Planes"); 
   strcpy(dialog->boxes[8],"Red"); 
   strcpy(dialog->boxes[9],"Green"); 
   strcpy(dialog->boxes[10],"Blue"); 
 
   strcpy(dialog->boxes[11],"Main cursor"); 

   dialog->l[11]       = new listinfo;
   dialog->l[11]->title = new char[100];
   dialog->l[11]->info  = new char*[NOOFCURSORS+1];
   dialog->l[11]->count = NOOFCURSORS;
   dialog->l[11]->wantfunction = 0;
   dialog->l[11]->f1    = null;
   dialog->l[11]->f2    = null;
   dialog->l[11]->f3    = null;
   dialog->l[11]->f4    = null; // dialog lists are deleted in dialogcancelcb
   dialog->l[11]->f5    = null;
   dialog->l[11]->f6    = null;
   dialog->l[11]->listnumber = 0;
   for(k=0; k<NOOFCURSORS+1; k++) dialog->l[11]->info[k] = new char[100];
   switch(cursor)
   {  case XC_draft_large:    selection = 0;break;
      case XC_draft_small:    selection = 1;break;
      case XC_arrow:          selection = 2;break;
      case XC_crosshair:      selection = 3;break;
      case XC_cross:          selection = 4;break;
      case XC_left_ptr:       selection = 5;break;
      case XC_plus:           selection = 6;break;
      case XC_top_left_arrow: selection = 7;break;
      case XC_xterm:          selection = 8;break;
      case XC_target:         selection = 9;break;
      default:                selection = 0;break;
   }
   if(g.want_crosshairs==1) selection = 10;
   if(g.want_crosshairs==2) selection = 11;
   dialog->l[11]->selection = &selection;
   
   strcpy(dialog->l[11]->title, "Cursor");
   strcpy(dialog->l[11]->info[0]," draft large   ");                
   strcpy(dialog->l[11]->info[1]," draft small   ");                 
   strcpy(dialog->l[11]->info[2]," arrow         ");          
   strcpy(dialog->l[11]->info[3]," crosshair     "); 
   strcpy(dialog->l[11]->info[4]," cross         ");     
   strcpy(dialog->l[11]->info[5]," left ptr      ");
   strcpy(dialog->l[11]->info[6]," plus          ");
   strcpy(dialog->l[11]->info[7]," top left arrow");
   strcpy(dialog->l[11]->info[8]," xterm         ");
   strcpy(dialog->l[11]->info[9]," target        ");
   strcpy(dialog->l[11]->info[10]," full screen crosshairs");
   strcpy(dialog->l[11]->info[11]," multiple crosshairs");

   strcpy(dialog->boxes[12],"Raise image on focus"); 
   strcpy(dialog->boxes[13],"Separate windows"); 
   strcpy(dialog->boxes[14],"Window border"); 
   strcpy(dialog->boxes[15],"Show image title"); 
   strcpy(dialog->boxes[16],"Text spacing"); 
   strcpy(dialog->boxes[17],"Split frame cols."); 
   strcpy(dialog->boxes[18],"Object threshold"); 
   strcpy(dialog->boxes[19],"Zoom factor");
   strcpy(dialog->boxes[20],"Capture root win.");
   strcpy(dialog->boxes[21],"Use libtiff if possible");

   dialog->radioset[0] = g.want_quantize;
   dialog->radioset[1] = 3 - g.autoundo;
   dialog->radioset[2] = g.want_colormaps + 1;
   dialog->radioset[3] = g.want_messages + 1;
   dialog->radioset[4] = g.area_selection_mode;
  
   doubletostring(g.luminr,g.signif,tempstring);    
   strcpy(dialog->answer[1][0],tempstring);
   doubletostring(g.luming,g.signif,tempstring);    
   strcpy(dialog->answer[2][0],tempstring);
   doubletostring(g.luminb,g.signif,tempstring);    
   strcpy(dialog->answer[3][0],tempstring);
   itoa(g.signif,tempstring,10);
   strcpy(dialog->answer[4][0],tempstring);
   itoa(g.csize,tempstring,10);
   strcpy(dialog->answer[5][0],tempstring);
   itoa(g.sprayfac,tempstring,10);
   strcpy(dialog->answer[6][0],tempstring);

   if(g.wantr) dialog->boxset[8]=1; else dialog->boxset[8]=0;
   if(g.wantg) dialog->boxset[9]=1; else dialog->boxset[9]=0;
   if(g.wantb) dialog->boxset[10]=1; else dialog->boxset[10]=0;
   if(g.want_raise) dialog->boxset[12]=1; else dialog->boxset[12]=0;
   if(g.want_shell) dialog->boxset[13]=1; else dialog->boxset[13]=0;
   if(g.window_border) dialog->boxset[14]=1; else dialog->boxset[14]=0;
   if(g.want_title)    dialog->boxset[15]=1; else dialog->boxset[15]=0;
   sprintf(dialog->answer[16][0],"%d", g.text_spacing);
   sprintf(dialog->answer[17][0],"%d", g.create_cols);
   sprintf(dialog->answer[18][0],"%d", 
      RGBvalue(g.object_threshold.red, 
               g.object_threshold.green,
               g.object_threshold.blue, 24));
   doubletostring(g.zoomfac,g.signif,tempstring);    
   strcpy(dialog->answer[19][0],tempstring);
   dialog->boxset[20] = g.floating_magnifier_on_root;
   dialog->boxset[21] = g.uselibtiff;

   dialog->boxtype[0]=LABEL;
   dialog->boxtype[1]=STRING;
   dialog->boxtype[2]=STRING;
   dialog->boxtype[3]=STRING;
   dialog->boxtype[4]=INTCLICKBOX; dialog->boxmin[4]=0; dialog->boxmax[4]=10;
   dialog->boxtype[5]=INTCLICKBOX; dialog->boxmin[5]=1; dialog->boxmax[5]=200;
   dialog->boxtype[6]=INTCLICKBOX; dialog->boxmin[6]=1; dialog->boxmax[6]=200;
   dialog->boxtype[7]=LABEL; 
   dialog->boxtype[8]=TOGGLE;
   dialog->boxtype[9]=TOGGLE;
   dialog->boxtype[10]=TOGGLE;
   dialog->boxtype[11]=NON_EDIT_LIST; 
      dialog->boxmin[11]=0; dialog->boxmax[11]=NOOFCURSORS;
   dialog->boxtype[12]=TOGGLE;
   dialog->boxtype[13]=TOGGLE;
   dialog->boxtype[14]=TOGGLE;
   dialog->boxtype[15]=TOGGLE;
   dialog->boxtype[16]=INTCLICKBOX; dialog->boxmin[16]=0; dialog->boxmax[16]=100;
   dialog->boxtype[17]=INTCLICKBOX; dialog->boxmin[17]=1; dialog->boxmax[17]=100;
   dialog->boxtype[18]=RGBCLICKBOX; dialog->boxmin[18]=1; dialog->boxmax[18]=100;
   dialog->boxtype[19]=STRING;
   dialog->boxtype[20]=TOGGLE;
   dialog->boxtype[21]=TOGGLE;

   dialog->bpp = 24;
   dialog->noofboxes=22;
   dialog->noofradios=5;
   dialog->radiono[0]=4;
   dialog->radiono[1]=4;
   dialog->radiono[2]=3;
   dialog->radiono[3]=4;
   dialog->radiono[4]=5;
   dialog->helptopic=16;

   for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
   old210 = dialog->radioset[2];
   dialog->want_changecicb = 0;
   dialog->f1 = configcheck;
   dialog->f2 = null;
   dialog->f3 = configcheck;
   dialog->f4 = configfinish;
   dialog->f5 = null;
   dialog->f6 = null;
   dialog->f7 = null;
   dialog->f8 = null;
   dialog->f9 = null;
   dialog->width = 430;  
   dialog->height = 0; // calculate automatically
   dialog->transient = 1;
   dialog->wantraise = 0;
   dialog->radiousexy = 0;
   dialog->boxusexy = 0;
   strcpy(dialog->path,".");
   dialog->message[0] = 0;      
   dialog->busy = 0;
   dialogbox(dialog); 
}


//--------------------------------------------------------------------------//
//  configcheck                                                             //
//--------------------------------------------------------------------------//
void configcheck(dialoginfo *a, int radio, int box, int boxbutton)
{    radio=radio; box=box; boxbutton=boxbutton;
     configcheck(a);
}
void configcheck(dialoginfo *a)
{
   int rr,gg,bb,v;
   Cursor cursor = g.xlib_cursor;
   static Cursor old_cursor = cursor;
   int owant_crosshairs = g.want_crosshairs;
   g.want_crosshairs = 0;
   switch(*a->l[11]->selection)
   {    case 0: cursor=XC_draft_large;break;
        case 1: cursor=XC_draft_small;break;
        case 2: cursor=XC_arrow      ;break;
        case 3: cursor=XC_crosshair  ;break;
        case 4: cursor=XC_cross      ;break;
        case 5: cursor=XC_left_ptr   ;break;
        case 6: cursor=XC_plus       ;break;
        case 7: cursor=XC_top_left_arrow;break;
        case 8: cursor=XC_xterm         ;break;
        case 9: cursor=XC_target        ;break;
        case 10: g.want_crosshairs = 1  ;break;
        case 11: g.want_crosshairs = 2  ;break;
        default: cursor=XC_draft_large  ;break;
   }
   if(g.want_crosshairs)
   {    g.cursor = g.no_cursor;
        XDefineCursor(g.display, g.main_window, g.cursor);
        old_cursor = g.cursor;
   }
   else if(cursor!=old_cursor)
   {    if(owant_crosshairs==0) XFreeCursor(g.display, g.normal_cursor);
        g.normal_cursor = XCreateFontCursor(g.display, cursor);
        g.xlib_cursor = cursor;
        old_cursor = cursor;
        g.cursor = g.normal_cursor;
        XDefineCursor(g.display, g.main_window, g.cursor);
   }

   g.want_quantize = a->radioset[0];
   g.autoundo = 3 - a->radioset[1];
   g.want_messages = a->radioset[3] - 1;
   g.area_selection_mode = a->radioset[4];

   ////  If user switched to `allocate-colors', try to reinitialize colors.

   if(a->radioset[2] != old210)
   {   if( a->radioset[2] == 1)
       {   message("Switching to allocated colors\nPlease exit and restart the program now");
           g.want_colormaps = 0;
           setup_colors();
           g.main_fcolor = g.fcolor;
           g.main_bcolor = g.bcolor;
       }
       if(a->radioset[2] == 2)
       {   message("Switching to colormaps\nPlease exit and restart the program now");
           g.fcolor = 0;
           g.bcolor = 1;
           g.main_fcolor = 0;
           g.main_bcolor = 1;
           g.want_colormaps = 1;
       }
       old210 = a->radioset[2];
   }
   g.luminr = atof(a->answer[1][0]);
   g.luming = atof(a->answer[2][0]);
   g.luminb = atof(a->answer[3][0]);
   g.signif = atoi(a->answer[4][0]);
   g.csize  = atoi(a->answer[5][0]);
   g.sprayfac = atoi(a->answer[6][0]);

   if(a->boxset[8]==1) g.wantr=1; else g.wantr=0;
   if(a->boxset[9]==1) g.wantg=1; else g.wantg=0;
   if(a->boxset[10]==1) g.wantb=1; else g.wantb=0;
   if(a->boxset[12]==1) g.want_raise=1; else g.want_raise=0;
   if(a->boxset[13]==1) g.want_shell=1; else g.want_shell=0;
   if(a->boxset[14]==1) g.window_border=1; else g.window_border=0;
   if(a->boxset[15]==1) g.want_title=1; else g.want_title=0;
   g.text_spacing = atoi(a->answer[16][0]);
   g.create_cols = atoi(a->answer[17][0]);
   v = atoi(a->answer[18][0]);
   g.zoomfac = atof(a->answer[19][0]);
   g.floating_magnifier_on_root = a->boxset[20];
   g.uselibtiff = a->boxset[21];

   valuetoRGB(v,rr,gg,bb,24);
   g.object_threshold.red = rr;
   g.object_threshold.green = gg;
   g.object_threshold.blue = bb;
   if(g.diagnose) print_configuration();
   return;
}


//--------------------------------------------------------------------------//
//  configfinish                                                            //
//--------------------------------------------------------------------------//
void configfinish(dialoginfo *a)
{
   a=a; g.getout=0;
}


//--------------------------------------------------------------------------//
// attributes                                                               //
//--------------------------------------------------------------------------//
void attributes(void)
{
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
   Dialog *dialog;
   int j, k;
   char tempstring[1024] = "0";
   if(ci==-1)
   {     message("Set properties on which image?\n(Use 0 to change background)",tempstring,PROMPT,10,60); 
         ci = atoi(tempstring);
         if(ci>=g.image_count) return;
   }
   dialog = new Dialog;

   dialog->helptopic=48;  
   strcpy(dialog->title,"Image Properties");
   if(g.attributes_isource == 0) g.attributes_isource = ci;
   if(g.attributes_idest == 0) g.attributes_idest = ci;

   ////--Radio buttons--////

   strcpy(dialog->radio[0][0],"Copy tables");             
   strcpy(dialog->radio[0][1],"No change ");   
   strcpy(dialog->radio[0][2],"Copy colormap");   
   strcpy(dialog->radio[0][3],"Copy gamma table");   
   strcpy(dialog->radio[0][4],"Copy calibration");   

   strcpy(dialog->radio[1][0],"Color type");
   strcpy(dialog->radio[1][1],"Grayscale");   
   strcpy(dialog->radio[1][2],"Indexed");   
   strcpy(dialog->radio[1][3],"Color");   

   dialog->radioset[0] = g.attributes_set0;
   if(ci>=0)
       dialog->radioset[1] = z[ci].colortype+1;
   else
       dialog->radioset[1] = g.colortype+1;
   dialog->radiono[0]=5;
   dialog->radiono[1]=4;
   dialog->radiono[2]=0;
   for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
   dialog->noofradios=2;

   ////-----Boxes-----////
   
   strcpy(dialog->boxes[0],"Image Number ");
   strcpy(dialog->boxes[1],"Image to change");
   strcpy(dialog->boxes[2],"Copy from");

   strcpy(dialog->boxes[3],"Image attributes");
   strcpy(dialog->boxes[4],"Title");
   strcpy(dialog->boxes[5],"Chromakeyed");             
   strcpy(dialog->boxes[6],"X position");
   strcpy(dialog->boxes[7],"Y position");
   strcpy(dialog->boxes[8],"Bits/pixel");
   strcpy(dialog->boxes[9],"Transparency");             
   strcpy(dialog->boxes[10],"Animation");             
   strcpy(dialog->boxes[11],"Frames/sec");             
   
   dialog->boxtype[0]=LABEL;
   dialog->boxtype[1]=INTCLICKBOX;
   dialog->boxtype[2]=INTCLICKBOX;
   dialog->boxtype[3]=LABEL;
   dialog->boxtype[4]=STRING;
   dialog->boxtype[5]=TOGGLE;
   dialog->boxtype[6]=INTCLICKBOX;
   dialog->boxtype[7]=INTCLICKBOX;
   dialog->boxtype[8]=INTCLICKBOX;
   dialog->boxtype[9]=INTCLICKBOX;
   dialog->boxtype[10]=TOGGLE;
   dialog->boxtype[11]=DOUBLECLICKBOX;
   
   dialog->boxmin[1]=0; dialog->boxmax[1]=g.image_count-1;
      sprintf(dialog->answer[1][0], "%d", ci);
   dialog->boxmin[2]=0; dialog->boxmax[2]=g.image_count-1;
      sprintf(dialog->answer[2][0], "%d", g.attributes_isource);
   strcpy(dialog->answer[4][0],z[ci].name);
   dialog->boxset[5] = (z[ci].chromakey!=0);
   dialog->boxmin[6]=-2*g.xres; dialog->boxmax[6]=2*g.xres;
      sprintf(dialog->answer[6][0], "%d", z[ci].xpos);
   dialog->boxmin[7]=-2*g.yres; dialog->boxmax[7]=2*g.yres;
      sprintf(dialog->answer[7][0], "%d", z[ci].ypos); 
   dialog->boxmin[8]=1; dialog->boxmax[8]=48;
      sprintf(dialog->answer[8][0], "%d", z[ci].bpp);
   dialog->boxmin[9]=0; dialog->boxmax[9]=100;
      sprintf(dialog->answer[9][0], "%d", z[ci].transparency);
   dialog->boxset[10] = (z[ci].animation!=0);
   dialog->boxmin[11]=0; dialog->boxmax[11]=200;
      sprintf(dialog->answer[11][0], "%f", z[ci].fps);
 
   dialog->noofboxes=12; 
   dialog->want_changecicb = 0;
   dialog->f1 = attributescheck;
   dialog->f2 = null;
   dialog->f3 = null;
   dialog->f4 = null;
   dialog->f5 = null;
   dialog->f6 = null;
   dialog->f7 = null;
   dialog->f8 = null;
   dialog->f9 = null;
   dialog->width = 430;
   dialog->height = 0; // calculate automatically
   dialog->transient = 1;
   dialog->wantraise = 0;
   dialog->radiousexy = 0;
   dialog->boxusexy = 0;
   strcpy(dialog->path,".");
   dialog->message[0] = 0;      
   dialog->busy = 0;
   dialogbox(dialog);
}


//--------------------------------------------------------------------------//
//  attributescheck                                                         //
//--------------------------------------------------------------------------//
void attributescheck(dialoginfo *a, int radio, int box, int boxbutton)
{
   radio=radio; box=box; boxbutton=boxbutton;
   int bpp, delay, j, k, idest, isource, test, colortype, ocolortype;
   if(g.getout){ g.getout=0; return; }
   if(radio != -2) return;
   
   g.attributes_idest   = idest   = atoi(a->answer[1][0]);
   g.attributes_isource = isource = atoi(a->answer[2][0]);
   ocolortype = z[idest].colortype;
   char **arg;

   g.attributes_set0 = a->radioset[0];
   switch(g.attributes_set0)
   {    case 1: break;
        case 2: memcpy(z[idest].palette, z[isource].palette, 768);
                break;
        case 3: memcpy(z[idest].gamma, z[isource].gamma, 256*sizeof(short));
                break;
        case 4: 
                strcpy(z[idest].cal_title[0], z[isource].cal_title[0]); 
                strcpy(z[idest].cal_title[1], z[isource].cal_title[1]); 
                for(k=0; k<3; k++)
                {   z[idest].cal_slope[k]   = z[isource].cal_slope[k];
                    z[idest].cal_int[k]     = z[isource].cal_int[k];
                    z[idest].cal_log[k]     = z[isource].cal_log[k];
                    z[idest].cal_xorigin[k] = z[isource].cal_xorigin[k];
                    z[idest].cal_yorigin[k] = z[isource].cal_yorigin[k];
                    for(j=0; j<10; j++) z[idest].cal_q[k][j] = z[isource].cal_q[k][j];  
                }
                z[idest].cal_dims = z[isource].cal_dims;
                z[idest].cal_scale = z[isource].cal_scale;
                break;
        default: break;
   }
   setimagetitle(idest, a->answer[4][0]);  
   test = a->boxset[5];
   if(test && z[idest].chromakey==0) set_chromakey(idest);
   else z[idest].chromakey=0;
   if(z[idest].chromakey!=0 && z[idest].shell) 
       message("Warning: image in a floating window\nhas chromakey set",WARNING);
   z[idest].xpos = atoi(a->answer[6][0]);
   z[idest].ypos = atoi(a->answer[7][0]);
   if(!z[idest].shell) moveimage(idest, z[idest].xpos, z[idest].ypos);

   z[idest].transparency = atoi(a->answer[9][0]);
   z[idest].fps = atof(a->answer[11][0]);
   arg = new char*[3];
   arg[0] = new char[128];
   arg[1] = new char[128];
   arg[2] = new char[128];

   bpp = atoi(a->answer[8][0]);
   sprintf(arg[0],"%d", idest);
   sprintf(arg[1],"%d", bpp);
   changecolordepth(2, arg);
   rebuild_display(idest);

   colortype = a->radioset[1]-1;
   if(z[idest].bpp <= 8)
   {   if(colortype == COLOR) colortype = INDEXED; 
   }else
   {   if(colortype == INDEXED) colortype = COLOR;
   }
   if(colortype == GRAY && ocolortype != GRAY) 
   {   converttograyscale(idest); // z[].colortype cannot already be gray
       repair(idest);
   }
   z[idest].colortype = colortype; 

   if(z[idest].transparency) redraw_transparent_image(idest);
   else if(!z[idest].chromakey) redraw(idest); // redraw chromakey is elsewhere

   g.getout = 0;  
   if(g.diagnose) print_image_info();
   delete[] arg[2];
   delete[] arg[1];
   delete[] arg[0];
   delete arg;

   //// Keep this at end 
   //// Control stays here until cancelled - idest and ci can change
   
   if(!a->boxset[10])
   {   check_event();
       z[idest].animation=0; 
   }else if(z[idest].fps) 
   {   delay = (int)(1000/z[idest].fps);
       z[idest].animation = 1;
       movie(delay);
   }
   return;
}


//--------------------------------------------------------------------------//
// set_chromakey                                                            //
//--------------------------------------------------------------------------//
void set_chromakey(int ino)
{
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
   uint mincolor, maxcolor;
   char tempstring[20];
   Dialog *dialog;
   if(ino==-1)
   {     message("Set chromakey on which image?",tempstring,PROMPT,10,60); 
         ino = atoi(tempstring);
         if(ino<0 || ino>=g.image_count) return;
   }
   dialog = new Dialog;
   int bpp = z[ino].bpp;

   dialog->helptopic=0;  
   strcpy(dialog->title,"Chromakey settings");

   strcpy(dialog->boxes[0],"Grayscale");
   strcpy(dialog->boxes[1],"Min opaque pixel");
   strcpy(dialog->boxes[2],"Max opaque pixel");
   strcpy(dialog->boxes[3],"Color");
   strcpy(dialog->boxes[4],"Min opaque color");
   strcpy(dialog->boxes[5],"Max opaque color");
   strcpy(dialog->boxes[6],"Invert chromakey");

   dialog->boxmin[1]=0; dialog->boxmax[1] = (int)g.maxvalue[bpp];
      sprintf(dialog->answer[1][0], "%d", z[ino].ck_graymin);
   dialog->boxmin[2]=0; dialog->boxmax[2] = (int)g.maxvalue[bpp];
      sprintf(dialog->answer[2][0], "%d", z[ino].ck_graymax);
      
   mincolor = RGBvalue((int)z[ino].ck_min.red, (int)z[ino].ck_min.green,
                       (int)z[ino].ck_min.blue, bpp);
   dialog->boxmin[4]=0; 
   dialog->boxmax[4] = (int)g.maxvalue[bpp];
   sprintf(dialog->answer[4][0], "%d", mincolor);

   maxcolor = RGBvalue((int)z[ino].ck_max.red, (int)z[ino].ck_max.green,
              (int)z[ino].ck_max.blue, bpp);

   dialog->boxmin[5]=0; 
   dialog->boxmax[5] = (int)g.maxvalue[bpp];
   sprintf(dialog->answer[5][0], "%d", maxcolor);


   if(z[ino].chromakey==-1) dialog->boxset[6]=1; else dialog->boxset[6]=0;

   dialog->boxtype[0]=LABEL;
   dialog->boxtype[1]=INTCLICKBOX;
   dialog->boxtype[2]=INTCLICKBOX;
   dialog->boxtype[3]=LABEL;
   dialog->boxtype[4]=RGBCLICKBOX;
   dialog->boxtype[5]=RGBCLICKBOX;
   dialog->boxtype[6]=TOGGLE;
 
   dialog->bpp = bpp;
   dialog->noofboxes=7; 
   dialog->want_changecicb = 0;
   dialog->f1 = chromakeycheck;
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
   dialog->param[3] = ino;
   dialog->message[0] = 0;      
   dialog->busy = 0;
   dialogbox(dialog);
}


//--------------------------------------------------------------------------//
//  chromakeycheck                                                          //
//--------------------------------------------------------------------------//
void chromakeycheck(dialoginfo *a, int radio, int box, int boxbutton)
{
   static uint omincolor=0, omaxcolor=0;
   static int oino=0, ockey=0;
   int k,tmin,tmax,answer;
   int cminr, cming, cminb, cmaxr, cmaxg, cmaxb;
   int ino = a->param[3];
   uint mincolor, maxcolor;
   radio=radio; box=box; boxbutton=boxbutton;
   if(ino<0) return;
   int bpp = z[ino].bpp;
   if(z[ino].colortype==GRAY)
   {    for(k=0;k<=2;k++) 
        {    if(a->boxwidget[1][k]) XtSetSensitive(a->boxwidget[1][k],True);
             if(a->boxwidget[2][k]) XtSetSensitive(a->boxwidget[2][k],True);
             if(a->boxwidget[4][k]) XtSetSensitive(a->boxwidget[4][k],False);
             if(a->boxwidget[5][k]) XtSetSensitive(a->boxwidget[5][k],False);
        }
   }else
   {    for(k=0;k<=2;k++) 
        {    if(a->boxwidget[1][k]) XtSetSensitive(a->boxwidget[1][k],False);
             if(a->boxwidget[2][k]) XtSetSensitive(a->boxwidget[2][k],False);
             if(a->boxwidget[4][k]) XtSetSensitive(a->boxwidget[4][k],True);
             if(a->boxwidget[5][k]) XtSetSensitive(a->boxwidget[5][k],True);
        }
   }
   tmin = atoi(a->answer[1][0]); 
   tmax = atoi(a->answer[2][0]);
   if(tmin > tmax) swap(tmin,tmax);
   z[ino].ck_graymin = tmin; 
   z[ino].ck_graymax = tmax;

   mincolor = atoi(a->answer[4][0]); 
   maxcolor = atoi(a->answer[5][0]);   
   if(a->boxset[6]) z[ino].chromakey=CHROMAKEY_INVERT; 
   else z[ino].chromakey=CHROMAKEY_NORMAL;
   valuetoRGB(mincolor, cminr, cming, cminb, bpp);
   valuetoRGB(maxcolor, cmaxr, cmaxg, cmaxb, bpp);
   if(cminr > cmaxr) swap(cminr, cmaxr);
   if(cming > cmaxg) swap(cming, cmaxg);
   if(cminb > cmaxb) swap(cminb, cmaxb);
   z[ino].ck_min.red = cminr;
   z[ino].ck_min.green = cming;
   z[ino].ck_min.blue = cminb;
   z[ino].ck_max.red = cmaxr;
   z[ino].ck_max.green = cmaxg;
   z[ino].ck_max.blue = cmaxb;

   answer = YES;
   if(omincolor != mincolor || omaxcolor != maxcolor || oino != ino ||
      ockey != z[ino].chromakey)
   {   if(!image_has_opaque_pixels(ino))
          answer = message("Warning: selected parameters will make \nimage completely invisible!\nProceed?",
              YESNOQUESTION);
       if(answer != YES) z[ino].chromakey=0;
       omincolor = mincolor;
       omaxcolor = maxcolor;
       oino = ino;
       ockey = z[ino].chromakey;
       redraw_chromakey_image(ino,1);
   }
}



//--------------------------------------------------------------------------//
// image_has_opaque_pixels                                                  //
//--------------------------------------------------------------------------//
int image_has_opaque_pixels(int ino)
{   
    int bpp,cf,i,j,tmin,tmax,rmin,rmax,gmin,gmax,bmin,bmax,rr,gg,bb,v;
    if(ino<0) return 1;
    if(!z[ino].chromakey) return 1;

    cf = z[ino].cf;
    bpp = z[ino].bpp;
    if(z[ino].colortype==GRAY)
    {    tmin = z[ino].ck_graymin;
         tmax = z[ino].ck_graymax;
         for(j=0;j<z[ino].ysize;j++)
         for(i=0;i<z[ino].xsize;i++)
         {    v = pixelat(z[ino].image[cf][j] + i*g.off[bpp], bpp);
              if(is_opaque(v,tmin,tmax,z[ino].chromakey)) return 1;
         }
    }else
    {    rmin = z[ino].ck_min.red;
         rmax = z[ino].ck_max.red;
         gmin = z[ino].ck_min.green;
         gmax = z[ino].ck_max.green;
         bmin = z[ino].ck_min.blue;
         bmax = z[ino].ck_max.blue;
         for(j=0;j<z[ino].ysize;j++)
         for(i=0;i<z[ino].xsize;i++)
         {    v = pixelat(z[ino].image[cf][j] + i*g.off[bpp], bpp);
              valuetoRGB(v,rr,gg,bb,bpp);
              convertRGBpixel(rr,gg,bb,bpp,bpp);
              if(is_opaque(rr,rmin,rmax,gg,gmin,gmax,bb,bmin,bmax,z[ino].chromakey))
                  return 1;
         }    
    }
    return 0;
}


//-------------------------------------------------------------------------//
// draw_image_title                                                        //
//-------------------------------------------------------------------------//
void draw_image_title(int ino)
{
  g.inmenu++;
  int color = (int)g.maxvalue[g.bitsperpixel];
  char tempstring[FILENAMELENGTH];
  strcpy(tempstring,basefilename(z[ino].name));
  if(ino>0) print(tempstring, 0, 10, 0, color, &g.gc, z[ino].win, 0, 0);  
  g.inmenu--;
}
