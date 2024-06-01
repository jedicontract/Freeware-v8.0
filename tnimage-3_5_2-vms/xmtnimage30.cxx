//--------------------------------------------------------------------------//
// xmtnimage30.cc                                                           //
// text                                                                     //
// Latest revision: 06-10-2005                                              //
// Copyright (C) 2005 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"
#include "xmtnimagec.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
extern PrinterStruct printer;
int oldci;
int font_first_item = 0;

//--------------------------------------------------------------------------//
// print - handles keystrokes, function keys, & printing text.              //
// The  param. 'win' specifies window or theWindow if 0.                    //
// The  param. 'wantbkg' specifies whether to draw background.              //
// The  param. 'vertical' specifies vertical printing.                      //
// The optional param. 'wantwarp' specifies if cursor should follow string. //
// If optional param. 'onlyino' != 0, printing is clipped inside image      //
// To get text into an image, draw it into a Pixmap instead of a            //
//    Window. Then use XGetSubimage to copy from the Pixmap to the          //
//    XImage. Use XCopyArea to put it on the Window.                        //
// See /usr/X11/include/X11/keysymdef.h for a list of keysyms.              //
//--------------------------------------------------------------------------//
void print(char *string, int x, int y, uint fcol, uint bcol, GC *gc, Window win, 
      int wantbkg, int vertical, int wantwarppointer, int onlyino)
{
  static int oldw=6, oldh=13, shift_state=0;
  int ascent, descent, pos, ee=0, h=0, hh, i, ino, in_info=0, j, k,
      len, s, w=0, ww, xx, yy, xoff=0, yoff=0;
  uint keysym=0;
  Pixmap pixmap = g.spixmap;
  char tempstring[1024]="";
  char cmd[1024]="";
  uchar temp2[1024]="";
  char oimagefont[1024];
  XFontStruct *fontstruct = g.image_font_struct;
  int noofargs=0;
  char **arg;                // up to 20 arguments for post-processing macro

  strcpy(oimagefont, g.imagefont);
  len = strlen(string);
  if(len>0) strncpy((char*)temp2, string, 1023);
  temp2[1023] = 0;
  if(win==0) win = g.main_window; 
  for(k=0;k<4;k++)
      if(win==g.info_window[k]){   pixmap=g.mpixmap[k];  in_info = 1; }

  XSetForeground(g.display,*gc,(ulong)fcol);
  XSetBackground(g.display,*gc,(ulong)bcol);

  if(g.inmenu || in_info)
  {   if(vertical)
      {  wantbkg=0;
         draw_string(string,x,y,fcol,bcol,gc,win,pixmap,wantbkg,vertical,onlyino);
      }
      else
         XDrawImageString(g.display,win,*gc,x,y,string,len);
  }else
  {  keysym = (uint)temp2[1]*256+(uint)temp2[0];
     switch(keysym)
     {  case 4:
        case XK_Delete:
          if(g.selectedimage==-1)   // Dont delete entire image by mistake
          {     drawselectbox(OFF);
                for(j=g.uly;j<=g.lry;j++)
                for(i=g.ulx;i<=g.lrx;i++)
                {   if(!g.selected_is_square && !inside_irregular_region(i,j)) continue;
                    setpixelonimage(i, j, g.bcolor, SET);
                }
                if(ci>=0 && z[ci].shell)
                {   xx = g.ulx - z[ci].xpos; 
                    yy = g.uly - z[ci].ypos; 
                    ww = 1 + g.lrx - g.ulx;
                    hh = 1 + g.lry - g.uly;
                    send_expose_event(z[ci].win, Expose, xx, yy, ww, hh); 
                }
                if(ci>0 && z[ci].is_zoomed) repair(ci);
          }
          break;
        case XK_BackSpace: break;
        case XK_Linefeed: break;
        case XK_Return  : break;
        case XK_Escape  : g.getout=1; break;
        case XK_Tab     : break;
        case XK_Home    :  
           g.mouse_x = x-g.csize;
           g.mouse_y = y-g.csize;
           if(wantwarppointer) XWarpPointer(g.display,None,None,x,y,0,0,-g.csize,-g.csize);
           if(g.draw_figure) draw(TNI_CONTINUE);
           break;
        case XK_KP_7    :  g.selected_lrx-=g.csize; ee=1; break;
        case XK_KP_2    :  g.selected_lry+=g.csize; ee=1; break;
        case XK_KP_3    :  g.selected_ulx+=g.csize; ee=1; break;
        case XK_KP_4    :  g.selected_ulx-=g.csize; ee=1; break;
        case XK_KP_5    :      // bigger
             ee=1;
             g.selected_ulx-=g.csize; g.selected_lrx+=g.csize;
             g.selected_uly-=g.csize; g.selected_lry+=g.csize;
             break;
        case XK_KP_6    :  g.selected_lrx+=g.csize; ee=1; break;
        case XK_KP_1    :  g.selected_uly+=g.csize; ee=1; break;
        case XK_KP_8    :  g.selected_uly-=g.csize; ee=1; break;
        case XK_KP_9    :  g.selected_lry-=g.csize; ee=1; break;
        case XK_KP_0    :      // smaller
             ee=1;
             g.selected_ulx+=g.csize; g.selected_lrx-=g.csize;
             g.selected_uly+=g.csize; g.selected_lry-=g.csize;
             break;
        case XK_Left    :
             g.mouse_x = x-g.csize;
             g.mouse_y = y;
             if(wantwarppointer) XWarpPointer(g.display,None,None,x,y,0,0,-g.csize,0);
             if(g.draw_figure) draw(TNI_CONTINUE);
             break;
        case XK_Right   :
           g.mouse_x = x+g.csize;
           g.mouse_y = y;
           if(wantwarppointer) XWarpPointer(g.display,None,None,x,y,0,0,g.csize,0);
           if(g.draw_figure) draw(TNI_CONTINUE);
           break;
        case XK_Up      :
           g.mouse_x = x;
           g.mouse_y = y-g.csize;
           if(wantwarppointer) XWarpPointer(g.display,None,None,x,y,0,0,0,-g.csize);
           if(g.draw_figure) draw(TNI_CONTINUE);
           break;
        case XK_Down    :
           g.mouse_x = x;
           g.mouse_y = y+g.csize;
           if(wantwarppointer) XWarpPointer(g.display,None,None,x,y,0,0,0,g.csize);
           if(g.draw_figure) draw(TNI_CONTINUE);
           break;
        case XK_Insert  :  break;
        case XK_Num_Lock:  break;
        case XK_End     :  
           g.mouse_x = x-g.csize;
           g.mouse_y = y+g.csize;
           if(wantwarppointer) XWarpPointer(g.display,None,None,x,y,0,0,-g.csize,g.csize);
           if(g.draw_figure) draw(TNI_CONTINUE);
           break; 
        case XK_Begin   :  break;
        case XK_Prior   :   // PgUp
           g.mouse_x = x+g.csize;
           g.mouse_y = y-g.csize;
           if(wantwarppointer) XWarpPointer(g.display,None,None,x,y,0,0,g.csize,-g.csize);
           if(g.draw_figure) draw(TNI_CONTINUE);
           break; 
        case XK_Next    :  // PgDn
           g.mouse_x = x+g.csize;
           g.mouse_y = y+g.csize;
           if(wantwarppointer) XWarpPointer(g.display,None,None,x,y,0,0,g.csize,g.csize);
           if(g.draw_figure) draw(TNI_CONTINUE);
           break; 
        case XK_F1      :help(-1); break;
        case XK_F2      : 
            if(g.draw_figure==SKETCH) 
            {   g.state=NORMAL;  
                g.draw_figure=NONE; 
            }else 
            {   g.state=DRAWING; 
                g.draw_figure=SKETCH; 
                draw(START);
            }           
            reset_user_buttons();   
            printstatus(g.state);           
            break;
        case XK_F3      : if(g.selectcurve==0) select_area(g.area_selection_mode); else g.getout=1;  break;
        case XK_F4      :  break;
        case XK_F5      :  break;
        case XK_F6      :  break;
        case XK_F7      :  break;
        case XK_F8      :  break;
        case XK_F9      :  printf("getpoint ");fflush(stdout); getpoint(x,y); printf("%d %d\n",x,y); break;
        case XK_F10     :  break;
        case XK_F11     :  break;
        case XK_F12     :  break;
        case XK_Control_L:  break;
        case XK_Control_R:  break;
        case XK_Shift_L  : shift_state=1; break;
        case XK_Shift_R  : shift_state=2; break;
        case XK_Caps_Lock:  break;
        case XK_Shift_Lock: break;
        case XK_Alt_L    :
           g.key_alt = 1 - g.key_alt; 
           printstatus(g.state);
           break;
        case XK_Alt_R    :
           g.key_alt = 1 - g.key_alt; 
           printstatus(g.state);
           break;
        case XK_Meta_L   :  break;
        case XK_Meta_R   :  break;
        case XK_Pause    :  break;
        case XK_KP_Multiply: break;
        case XK_KP_Add   :
            g.csize = min(1000,g.csize+1); 
            printstatus(g.state); 
            break;
        case XK_KP_Subtract: 
            g.csize = max(1,g.csize-1); 
            printstatus(g.state);  
            break;
        case XK_KP_Decimal : break;
        case XK_KP_Divide  : break;
        case XK_Clear      : break;
        default:
         if(g.key_alt)  // all alt-characters except F I P C D A o H 
         {  g.key_alt=0;
            switch(keysym)
            { case XK_e: 
                  if(ci>=0)                                 // alt-e
                  {  strcpy(tempstring, "Do you want to close this image?");
                     if(message(tempstring,YESNOQUESTION)==YES) eraseimage(ci,1,0,1); 
                  }
                  break;  
              case XK_1: show_alpha(ci);  break;            // alt-1 1st alpha channel

              case XK_b: ;  break;                          // alt-b
              case XK_t: test(); break;                     // alt-t
              case XK_u:                                    // alt-u diagnostic information
                  print_diagnostic_information();
                  break;                  
              case XK_x:                                    // alt-x
                  if(message("Do you want to quit?",QUESTION)==YES) 
                      quitcb(g.main_widget,NULL,NULL);
                  break;             
              default:   break;
            }
            g.state=NORMAL;
            break;
         }
         if(g.key_ctrl)      
         {  switch(keysym)  // use execute instead of calling directly so 'repeat' works
            {  case XK_a: strcpy(cmd,"acquire"); break;          // ctrl-a
               case XK_b: strcpy(cmd,"backup"); break;           // ctrl-b
               case XK_d: strcpy(cmd,"spotdensitometry"); break; // ctrl-d
               case XK_e: strcpy(cmd,"executeplugin"); break;    // ctrl-e
               case XK_f: strcpy(cmd,"filter"); break;           // ctrl-f
               case XK_i: strcpy(cmd,"macro"); break;            // ctrl-i
               case XK_l: strcpy(cmd,"label"); break;            // ctrl-l
               case XK_m: strcpy(cmd,"changecontrast"); break;   // ctrl-m
               case XK_n: strcpy(cmd,"createimage"); break;      // ctrl-n
               case XK_o: strcpy(cmd,"read"); break;             // ctrl-o
               case XK_s: strcpy(cmd,"save"); break;             // ctrl-s
               case XK_p: strcpy(cmd,"print"); break;            // ctrl-p
               case XK_r: strcpy(cmd,"repair");  break;          // ctrl-r
               case XK_t: strcpy(cmd,"stripdensitometry"); break;// ctrl-t
               case XK_u: strcpy(cmd,"restore"); break;          // ctrl-u
               case XK_v: strcpy(cmd,"invertcolors"); break;     // ctrl-v
               case XK_x: strcpy(cmd,"unloadimage"); break;      // ctrl-x
               case XK_z: print_information(); break;            // ctrl-z
               default: cmd[0]=0; break;       
            }
            g.key_ctrl=0;
            noofargs = 0;
            arg = new char*[20];
            for(k=0;k<20;k++){ arg[k] = new char[128]; arg[k][0]=0; }
            execute(cmd, noofargs, arg);
            for(k=0;k<20;k++) delete[] arg[k]; 
             delete[] arg;
            break;
         }
         for(pos=0; pos<len; pos++)
         {
            s = string[pos];

            //// This will change the string and possibly change the font
            //// if a tex command is found.

            if(s=='\\' || s=='$') process_tex_command(string, pos, xoff, yoff);
            s = string[pos];
            if(s==0) break;
            if(s=='\\') { pos--; continue; }
            tempstring[0]=s;
            tempstring[1]=0;
            ascent = g.image_font_struct->ascent;
            descent = g.image_font_struct->descent;
            w = g.text_spacing + XTextWidth(g.image_font_struct,tempstring,1);
            h = ascent + descent;

            //// Draw the string on the Pixmap then copy to the Window and to XImage.
            //// Can't use XDrawString(), because the pixels have to be
            //// put in the image buffers as well as on screen.
               
            ino = g.imageatcursor;
            draw_string(tempstring, x-ascent/3+xoff, y-ascent/2+yoff, fcol, 
                bcol, gc, win, pixmap, wantbkg, vertical, onlyino);
            if(vertical)
            {   if(wantwarppointer) XWarpPointer(g.display,None,None,x,y,0,0,0,-w);
                y -= w;
            }else
            {   if(wantwarppointer) XWarpPointer(g.display,None,None,x,y,0,0,w,0);
                x += w;
            }
         }
         shift_state = 0;
    }

     //// Restore original font if necessary
    
    if(fontstruct != g.image_font_struct)
    {    fontstruct = XLoadQueryFont(g.display, oimagefont);
         if(fontstruct!=NULL)
         {    strcpy(g.imagefont, oimagefont);
              g.image_font_struct = fontstruct;
              XSetFont(g.display, g.image_gc, fontstruct->fid);
         }else message("Cant restore previous font");
    }
    if(w>0)oldw=w;
    if(h>0)oldh=h;
  }
  g.key_shift = shift_state;

  if(ee)
  {   if(g.selected_ulx>g.selected_lrx) swap(g.selected_ulx,g.selected_lrx);
      if(g.selected_uly>g.selected_lry) swap(g.selected_uly,g.selected_lry);
      g.ulx = zoom_x_coordinate(g.selected_ulx, g.imageatcursor);
      g.uly = zoom_y_coordinate(g.selected_uly, g.imageatcursor);
      g.lrx = zoom_x_coordinate(g.selected_lrx, g.imageatcursor);
      g.lry = zoom_y_coordinate(g.selected_lry, g.imageatcursor);       
  }
  return;
}


 
//--------------------------------------------------------------------//
// draw_string (Called by print()).                                   //
// Copy image of string from Pixmap to temporary XImage.              //
// Don't call this directly.                                          //
//--------------------------------------------------------------------//
void draw_string(char *string, int x, int y, uint fcol, uint bcol,
    GC *gc, Window win, Pixmap spixmap, int wantbkg, int vertical, 
    int onlyino)
{  
   win = win;
   int ascent,descent,direction,offset,pos,i,j,w,h,xx,yy;
   XCharStruct overall;
   XFontStruct *fstruct;
   char tempstring[256];
   int bpp,c,s;
   int len=strlen(string);
   crosshairs(0,0);
   if(onlyino > 0) bpp = z[onlyino].bpp; else bpp = g.bitsperpixel;
   for(pos=0; pos<len; pos++)
   {  s = string[pos];
      tempstring[0]=s;
      tempstring[1]=0;
      if(gc == &g.gc)
          fstruct = g.fontstruct;
      else
          fstruct = g.image_font_struct;

      XTextExtents(fstruct,tempstring,1,&direction,&ascent,&descent,&overall);
      // offset = -min(0,overall.lbearing);
      offset=0;     

      w = XTextWidth(fstruct,tempstring,1);
      h = fstruct->ascent + fstruct->descent;

                               // Draw the string on the Pixmap
      XSetForeground(g.display, *gc, (ulong)fcol);
      if(wantbkg)
         XSetBackground(g.display,*gc,(ulong)bcol);
      else
         XSetBackground(g.display,*gc,(ulong)fcol-1);
                              // Initialize spixmap, otherwise if offset!=0, some
                              // pixels are not set. Mainly needed for 'j'.
      XDrawImageString(g.display, spixmap, *gc, 0, ascent, "   ", 3);
      XDrawImageString(g.display, spixmap, *gc,  offset, ascent, tempstring, 1);
      copyimage(0,0,w,h,0,0,GET,g.stringbufr_ximage,spixmap);
                              // Draw fg pixels one at a time on screen
                              //   and also put in appropriate buffer.
      for(j=0;j<h;j++)
      for(i=0;i<w;i++)
      {  
         c = pixelat(g.stringbufr[j] + i*g.off[g.bitsperpixel], g.bitsperpixel);
         if(wantbkg)  // Want background and want vertical
         { if((uint)c==(uint)bcol)
                XSetForeground(g.display,*gc,(ulong)bcol);
            else
                XSetForeground(g.display,*gc,(ulong)fcol);
         }else 
            if(c!=(int)fcol) continue;
         if(vertical)
         {  xx = x+j;  yy = y-i; }
         else
         {  xx = x+i;  yy = y+j;   } 

         if(onlyino>0)
         {   if(between(xx,0,z[onlyino].xsize-1) && between(yy,0,z[onlyino].ysize-1))
                putpixelbytes(z[onlyino].image[0][yy]+xx*g.off[bpp], c, 1, bpp, 1);
         }else
             setpixelonimage(xx, yy, c, g.imode, 0, -1, 0, win, 1, 0, 1);
      }
      if(vertical) y -= w; else x += w;   
   }
}


//----------------------------------------------------------------------//
// changefont                                                           //
//----------------------------------------------------------------------//
void changefont(void)
{ 
#ifdef DIGITAL
  #define ITEMSTOSHOW  15          // No.of items to show at a time in list box
  #define FONTNAMELENGTH 200       // Maximum length of a font name
#else
  const int ITEMSTOSHOW = 15;      // No.of items to show at a time in list box
  const int FONTNAMELENGTH = 200;  // Maximum length of a font name
#endif

  int k, len, nooffonts=0, size=0;
  static listinfo *l;
  int helptopic = 36;
  static char *listtitle;
  char **info;                                    
  FILE *fp;
  int status=0;
  char fontfile[FILENAMELENGTH];
  char tempstring[FILENAMELENGTH];
  char temp[FONTNAMELENGTH+1];
  temp[0] = 0;

  sprintf(fontfile, "%s/fonts", g.helpdir);
  if(g.diagnose) printf("fontfile %s\n",fontfile);
  if ((fp=fopen(fontfile,"r")) == NULL)
  {   sprintf(fontfile, "%s/fonts",g.homedir);
      if ((fp=fopen(fontfile, "r")) == NULL)
      {   error_message(fontfile, errno);
          sprintf(tempstring, "xlsfonts > %s",fontfile);
          status = system(tempstring);
          if(status==127 || status==-1){ message("Error executing xlsfonts", ERROR); }     
          if ((fp=fopen(fontfile,"r")) == NULL)    // Try it now.
          {   error_message(fontfile, errno);
              return;                               // Give up
          }          
      }
  }

 
  while(!feof(fp))             // Count no. of fonts available
  {  fgets(temp,FONTNAMELENGTH,fp); 
     nooffonts++;
  }
  if(feof(fp)) nooffonts++;
  if(nooffonts < 3)
  {   sprintf(temp, "Invalid font list\n%s",fontfile);
      message(temp);
      return;
  }        
  size = nooffonts;
  info = new char*[size];
  fseek(fp,0,SEEK_SET);        // Go back to start of font list
  for(k=0;k<size;k++)          // Read all the font names into memory
  {   info[k] = new char[FONTNAMELENGTH];
      info[k][0]=0;
      fgets(temp,FONTNAMELENGTH,fp);
      if(strlen(temp)) strcpy(info[k],temp);
      else strcpy(info[k],"  ");
      len = strlen(info[k]);
      info[k][len-1] = 0;      // Get rid of that wacky 0x10 at end
  }  
  fclose(fp);

  if(size)
  {  l = new listinfo;
     listtitle = new char[100];
     strcpy(listtitle, "Available fonts");                  
     l->title = listtitle;
     l->info  = info;
     l->count = size-1;
     l->itemstoshow = ITEMSTOSHOW;
     l->firstitem   = font_first_item;
     l->wantsort    = 0;
     l->wantsave    = 0;
     l->helptopic   = helptopic;
     l->allowedit   = 0;
     l->selection   = &g.font_selection;
     l->width       = 0;
     l->transient   = 1;
     l->wantfunction = 0;
     l->autoupdate   = 0;
     l->clearbutton  = 0;
     l->highest_erased = 0;
     l->f1 = changefontfinish;
     l->f2 = null;
     l->f3 = null;
     l->f4 = delete_list;
     l->f5 = null;
     l->f6 = null;
     l->listnumber = 0;
     list(l);
  }
}


//--------------------------------------------------------------------------//
//  changefontfinish                                                        //
//--------------------------------------------------------------------------//
void changefontfinish(listinfo *l)
{
  char tempstring[FILENAMELENGTH];
  int font = *l->selection;
  if(font>=0 && g.getout==0)
  {   XFreeFont(g.display, g.image_font_struct);
      if((g.image_font_struct = XLoadQueryFont(g.display, l->info[font])) == NULL) 
      {    sprintf(tempstring,"Cannot load the new font:\n%s", l->info[font]); 
           message(tempstring, WARNING);
           if((g.image_font_struct=XLoadQueryFont(g.display,"fixed"))==NULL)
                message("Oh geezz, now cannot recover the old font!!!");
      }else
           strcpy(g.imagefont, l->info[font]);
      XSetFont(g.display,g.image_gc,g.image_font_struct->fid);
  }
  g.font_selection = *l->selection; 
  font_first_item = l->firstitem;
  return;
} 
 

//----------------------------------------------------------------------//
// process_tex_command                                                  //
//----------------------------------------------------------------------//
void process_tex_command(char *string, int pos, int &xoff, int &yoff)
{
   //// These two must match

   static const char *tex[] = { "\\it"  , "\\rm" , "\\sf"  , "\\bf"  , "\\font" , 
                                "\\size", "\\up" , "\\down", "\\left", "\\right",
                                "\\gr"  , "\\med", "\\tt"  , "\\d"   , "\\u"    ,
                                "\\l"   , "\\r"  , "\\s"  };
                          
   enum                 {    IT   ,    RM  ,    SF   ,    BF   ,    FONT  ,   
                             SIZE ,    MOVEUP,  DN   ,    LE   ,    RT    ,
                             GR   ,    MED ,    TT   ,    D    ,    U     ,
                             L    ,    R   ,    S   };

   int c, count, gotspace=0, k, n=-1, nelements, size, noofargs, test;
   int len=strlen(string);   
   char command[128];
   char tempstring[128];
   char tempfontstring[FILENAMELENGTH];
   char **arg;
   memset(command,0,128);   
   XFontStruct *fontstruct;

   ////  Escape a backslash
   if(len>pos && (string[pos+1]=='\\' || string[pos+1]=='$')) 
   {   for(k=pos;k<len;k++) string[k] = string[k+1];
       string[len]=0;
       return; 
   }
   arg = new char*[20];
   for(k=0; k<20; k++) arg[k] = new char[64];
   count = 0;
   for(k=pos; k<len; k++) 
   {   c = string[k];
       if(c=='\\') count++;
       if(count>1) break;                  // stop parsing if hit another command
       if(isspace(c)){gotspace=1; break; } // stop if hit a space, tab, etc
       if(isdigit(c)) break;               // stop if hit a number
       command[k-pos] = c;  
   }   
   
   nelements = sizeof(tex) / sizeof(char*);
   for(k=0;k<nelements;k++) if(!strcmp(tex[k], command)){ n=k; break; }
   if(!strlen(command)) goto end_process_tex_command; 
   if(n<0){ string[pos]=' '; goto end_process_tex_command; }
   strcpy(tempfontstring, g.imagefont);
   remove_arg(string, pos, strlen(tex[n])+gotspace);
   switch(n)
   { 
       case IT: substitute_font(tempfontstring, 3, (char*)"i"); break;
       case RM: 
                substitute_font(tempfontstring, 1, (char*)"times"); 
                substitute_font(tempfontstring, 2, (char*)"medium"); 
                substitute_font(tempfontstring, 3, (char*)"r"); 
                break;
       case SF: substitute_font(tempfontstring, 3, (char*)"s"); break;
       case BF: substitute_font(tempfontstring, 2, (char*)"bold"); break;
       case FONT: 
            parsecommand(string+pos, TEX, arg, noofargs, 64, 1);
            remove_arg(string, pos, 1+strlen(arg[0]));
            substitute_font(tempfontstring, 2, arg[0]);
            break;
       case S: 
       case SIZE: 
            parsecommand(string+pos, TEX, arg, noofargs, 64, 1);
            remove_arg(string, pos, 1+strlen(arg[0]));
            substitute_font(tempfontstring, 7, arg[0]);
            size = atoi(arg[0]);
            if(size<18) xoff = -size/4; 
            if(size>24) xoff = 2;             
            test=1;
            while(XLoadQueryFont(g.display, tempfontstring)==NULL)
            {   itoa(size+test, tempstring, 10);
                substitute_font(tempfontstring, 5, tempstring);
                test *= -1;
                if(test>0) test++; 
            }
            break;
       case U: 
       case MOVEUP: 
            parsecommand(string+pos, TEX, arg, noofargs, 64, 1);
            remove_arg(string, pos, 1+strlen(arg[0]));
            yoff -= atoi(arg[0]);
            break;
       case D: 
       case DN: 
            parsecommand(string+pos, TEX, arg, noofargs, 64, 1);
            remove_arg(string, pos, 1+strlen(arg[0]));
            yoff += atoi(arg[0]);
            break;
       case L: 
       case LE: 
            parsecommand(string+pos, TEX, arg, noofargs, 64, 1);
            remove_arg(string, pos, 1+strlen(arg[0]));
            xoff -= atoi(arg[0]);
            break;
       case R: 
       case RT: 
            parsecommand(string+pos, TEX, arg, noofargs, 64, 1);
            remove_arg(string, pos, 1+strlen(arg[0]));
            xoff += atoi(arg[0]);
            break;
       case GR: 
                substitute_font(tempfontstring, 1, (char*)"symbol"); 
                substitute_font(tempfontstring, 2, (char*)"medium"); 
                break;
       case MED:substitute_font(tempfontstring, 2, (char*)"medium"); break;
       case TT: substitute_font(tempfontstring, 1, (char*)"courier"); break;
       default: sprintf(tempstring,"Unrecognized font command:\n%s",command);
                message(tempstring, WARNING);
                break;
   }     
   fontstruct = XLoadQueryFont(g.display, tempfontstring);
   if(fontstruct!=NULL)
   {    strcpy(g.imagefont, tempfontstring);
        g.image_font_struct = fontstruct;
        XSetFont(g.display, g.image_gc, fontstruct->fid);
   }else
   {    sprintf(tempstring, "Cant load font:\n%s", tempfontstring);
        message(tempstring);
   }
end_process_tex_command:
   for(k=0;k<20;k++) delete[] arg[k];
   delete[] arg;
   return;
}


//----------------------------------------------------------------------//
// remove_arg                                                           //
//----------------------------------------------------------------------//
void remove_arg(char *string, int pos, int arglen)
{ 
   int k,len;
   len = (int)strlen(string);
   for(k=pos;k<len-arglen;k++) string[k]=string[k+arglen];
   string[len-arglen]=0;
}


//----------------------------------------------------------------------//
// substitute_font - substitute string `portion' into nth position      //
// in font string (demarcated by '-'s).                                 //
//----------------------------------------------------------------------//
void substitute_font(char *font, int n, char *portion)
{
   if(!between(n,1,13)) return;
   int start, end, trunc;
   char tempstring[FILENAMELENGTH];
   memset(tempstring, 0, FILENAMELENGTH);
   start = strpos(font, n, '-');
   end = strpos(font, n+1, '-');
   strcpy(tempstring, font);
   tempstring[start+1]=0;
   strcat(tempstring, portion);
   if(end) strcat(tempstring, font+end);
   trunc = strpos(tempstring, 9, '-');
   if(trunc)
   {   tempstring[trunc]='*';
       tempstring[trunc+1]=0;
   }else if(tempstring[strlen(tempstring)-1]!='*')
       strcat(tempstring,"*");
   strcpy(font,tempstring);
}


//----------------------------------------------------------------------//
// strpos - returns position of nth occurrence of c in s                //
//----------------------------------------------------------------------//
int strpos(char *s, int n, char c)
{
   int k,count=0;
   for(k=0;k<(int)strlen(s);k++)
   {    if(s[k]==c) count++;
        if(count>=n) return k;
   }
   return 0;
}


//----------------------------------------------------------------------//
// setfont - set font for putting strings on images                     //
//----------------------------------------------------------------------//
void setfont(char *fontname)
{  
   if(g.diagnose) printf("setting font\n");
   if(g.diagnose) printf("fontname %s\n",fontname);
   if(fontname == NULL) return;
   if(g.diagnose) printf("freeing old font\n");
   if(g.image_font_struct != NULL) XUnloadFont(g.display, g.image_font_struct->fid);
   if(g.diagnose) printf("loading new font\n");
   g.image_font_struct = XLoadQueryFont(g.display, fontname);
   if(g.image_font_struct == NULL)
   {     message("Cant restore previous font, loading fixed font");
         g.image_font_struct = XLoadQueryFont(g.display, "fixed");
   }else 
         XSetFont(g.display, g.image_gc, g.image_font_struct->fid);
   error_message(fontname, errno);
   if(g.diagnose && g.image_font_struct == NULL)
         printf("null image font struct in %s line %d\n",__FILE__,__LINE__);
}


//-------------------------------------------------------------------------//
// print_rotated                                                           //
//-------------------------------------------------------------------------//
void print_rotated(int xs, int ys, char *string, double angle)
{
   char *s;
   int fc,bc,ino,oino,ino_label,ino_rotated=0,j,k,len,w,h,direction,ascent,descent,x,y,dx,dy;   
   int bpp, ct, w_rotated, h_rotated, value, gatevalue, ovalue,xx,yy;
   XCharStruct overall;
   oldci = ci;
   g.getout=0;

   //// Save flags so they can be restored. Rotate image draws lines
   int ostate = g.state;
   int odrawfigure = g.draw_figure;
   int otype = g.line.type;
   g.line.type = 0; 

   bpp = z[oldci].bpp;
   ct = z[oldci].colortype;
   bc = 0; 
   fc = g.maxcolor;
   for(k=0; k<g.image_count; k++) z[k].hit=0;
   len = (int)strlen(string);
   XTextExtents(g.image_font_struct,string,len,&direction,&ascent,&descent,&overall);
   w = 20 + len*g.text_spacing + XTextWidth(g.image_font_struct,string,len);
   h = g.image_font_struct->ascent + g.image_font_struct->descent;
   h *= 2;

   ////  Make a hidden temporary image at screen bpp.

   crosshairs(0,0);
   if(newimage("Label",0,0,w,h,g.bitsperpixel,g.colortype,1,0,0,PERM,1,0,0)!=OK) return;
   ino_label = ci;

   for(j=0;j<h;j++) putpixelbytes(z[ino_label].image[0][j],0,w,g.bitsperpixel,1);
   selectimage(ino_label);
   switchto(ino_label);

   x = ascent/3;
   y = ascent/2;
   s = new char[strlen(string)+4];
   sprintf(s, " %s ", string);
   print(s,x,y,fc,bc,&g.image_gc,z[ino_label].win,1,0,0,0); 
   delete[] s;

   g.getlabelstate = 1;
 
   ////  This creates a new image and changes ci, xsize, and ysize
   rotate_image(ino_label, -angle);
   ino_rotated = ci;
   w_rotated = z[ino_rotated].xsize;
   h_rotated = z[ino_rotated].ysize;
   repairimg(ino_rotated, 0, 0, w_rotated, h_rotated);

   switchto(oldci);

   ////  Find center pixel
   int xmin=w_rotated;
   int xmax=0;
   int ymin=h_rotated;
   int ymax=0;

   for(j=0; j<h_rotated; j++) 
   for(k=0; k<w_rotated; k++) 
   {
       value = pixelat(z[ino_rotated].image[0][j] + k*g.off[g.bitsperpixel], g.bitsperpixel);
       if(value==fc) 
       {   xmin=min(k,xmin);
           xmax=max(k,xmax);
           ymin=min(j,ymin);
           ymax=max(j,ymax);
       }
   }
   dx = -(xmax-xmin)/2;
   dy = -(ymax-ymin)/2;

   oino = ci;
//   for(j=max(0,ymin-6); j<min(ymax+6,h_rotated); j++)    // Put stuff permanently
//   for(k=max(0,xmin-6); k<min(xmax+6,w_rotated); k++)    // Destination coordinates
   for(j=0; j<h_rotated-6; j++)    // Put stuff permanently
   for(k=0; k<w_rotated-6; k++)    // Destination coordinates
   {    
       gatevalue = pixelat(z[ino_rotated].image[0][j] + k*g.off[g.bitsperpixel], g.bitsperpixel);
       xx = k+xs-dx-xmax;
       yy = j+ys-dy-ymax;
       ovalue = readpixel(xx,yy);
       ino = whichimage(xx,yy,bpp,ino_rotated);
       if(ino != oino){ switchto(ino); oino = ino; } // Make sure correct colormap for conversion
       ct = z[ino].colortype;
       fc = convertpixel(g.fcolor, g.bitsperpixel, bpp, 1);
       value = anti_aliased_value(gatevalue, ovalue, fc, ct, bpp);
       setpixelonimage(xx, yy, value, g.imode, bpp, ino_rotated, 0, 0, 1, 0, 1);
   }

   eraseimage(ino_rotated,0,0,1);        // Erase the temporary image bufr
   eraseimage(ino_label,0,0,1);          // Erase the temporary image bufr
   switchto(oldci);
   g.line.type = otype; 
   g.state = ostate;
   g.draw_figure = odrawfigure;
   g.getlabelstate = 0;
}


void print_information(void)
{
   int k;
   printf("\nCurrent image ci=%d  cursor position %d %d on image %d\n",
        ci,g.mouse_x, g.mouse_y, g.imageatcursor);
   printf("Selected image %d \n",g.selectedimage);
   printf("g.ulx      %d %d %d %d \n",g.ulx,g.uly,g.lrx,g.lry);
   printf("g.selected %d %d %d %d \n",g.selected_ulx, g.selected_uly, 
        g.selected_lrx,g.selected_lry);
   printf("Image information\nImage  xsize  ysize xpos ypos overlap format\n");
   for(k=0;k<g.image_count;k++)
   {
       printf(" %d   %d     %d     %d     %d     %d        %d\n",
          k, z[k].xsize, z[k].ysize, z[k].xpos, z[k].ypos, z[k].overlap, z[k].format); 
   }
}
