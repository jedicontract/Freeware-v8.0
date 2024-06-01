//--------------------------------------------------------------------------//
// xmtnimage3.cc                                                            //
// Image file reading, writing                                              //
// Latest revision: 04-22-2006                                              //
// Copyright (C) 2006 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//
 
#include "xmtnimage.h"
extern Globals     g;
extern Image      *z;
extern int        ci;
extern int    blocked_while_saving;

int override=0;

////  If a new format is added, increase this value

const int STDFORMATS=13;
int noofformats;  
char **custom_format_list;
char identifier[64] = "NONE";
int hitnonstandard=0;
int customcount=0;
int combinedino;
char read_filename[FILENAMELENGTH]="Untitled";
int in_save=0;
int in_read=0;
 
//--------------------------------------------------------------------------//
// read_start                                                               //
// Entry point for reading images                                           //
//--------------------------------------------------------------------------//
void readstart(int noofargs, char **arg)
{  
  if(memorylessthan(16384)){ message(g.nomemory,ERROR);  return; }
  static char **filelist = NULL;
  int j,k;
  char temp2[202];
  char *ptr; 
  static Dialog *dialog;
  if(in_read) return;
  in_read = 1;
  g.getout=0;
  if(noofargs==0)
  {  dialog = new Dialog;
     if(dialog==NULL){ message(g.nomemory); return; }
     strcpy(dialog->title,"Read Image");
     strcpy(dialog->radio[0][0],"File type");
     strcpy(dialog->radio[0][1],"Auto file type");
     strcpy(dialog->radio[0][2],"Raw bytes");
     strcpy(dialog->radio[0][3],"ASCII");
     strcpy(dialog->radio[0][4],"Raw 3-D");
     strcpy(dialog->radio[1][0],"Color reduction");
     strcpy(dialog->radio[1][1],"Quantization");
     strcpy(dialog->radio[1][2],"Fit current palette");
     strcpy(dialog->radio[1][3],"None");

  
     dialog->radioset[0] = g.read_raw + 1;
     dialog->radioset[1] = g.want_quantize;
     dialog->radiono[0] = 5;
     dialog->radiono[1] = 4;
     dialog->radiono[2] = 0;
     for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;

     strcpy(dialog->boxes[0],"File Parameters");
     strcpy(dialog->boxes[1],"Filename");
     strcpy(dialog->boxes[2],"X position");
     strcpy(dialog->boxes[3],"Y position");
     strcpy(dialog->boxes[4],"X size");
     strcpy(dialog->boxes[5],"Y size");
     strcpy(dialog->boxes[6],"CMYK->RGB");
     strcpy(dialog->boxes[7],"Convert->Gray scale");
     strcpy(dialog->boxes[8],"Convert->Scrn depth");
     strcpy(dialog->boxes[9],"Negative image");
     strcpy(dialog->boxes[10],"Separate window");
     strcpy(dialog->boxes[11],"Swap bytes");
     strcpy(dialog->boxes[12],"Pre-processing");
     strcpy(dialog->boxes[13],"Command");
     strcpy(dialog->boxes[14],"Extension");
     strcpy(dialog->boxes[15],"Split frames");
     strcpy(dialog->boxes[16],"Fix CMYK");

     dialog->boxtype[0]=LABEL;
     dialog->boxtype[1]=MULTIFILENAME; dialog->boxcount[1]=0;
     dialog->boxtype[2]=INTCLICKBOX; 
     dialog->boxtype[3]=INTCLICKBOX;
     dialog->boxtype[4]=INTCLICKBOX;
     dialog->boxtype[5]=INTCLICKBOX;
     dialog->boxtype[6]=TOGGLE;    
     dialog->boxtype[7]=TOGGLE;     
     dialog->boxtype[8]=TOGGLE;     
     dialog->boxtype[9]=TOGGLE;     
     dialog->boxtype[10]=TOGGLE;     
     dialog->boxtype[11]=TOGGLE;     
     dialog->boxtype[12]=LABEL;     
     dialog->boxtype[13]=STRING;     
     dialog->boxtype[14]=STRING;     
     dialog->boxtype[15]=TOGGLE;     
     dialog->boxtype[16]=TOGGLE;     
     dialog->boxmin[2]=0; dialog->boxmax[2]=g.xres;
     dialog->boxmin[3]=0; dialog->boxmax[3]=g.yres;
     dialog->boxmin[4]=0; dialog->boxmax[4]=100;
     dialog->boxmin[5]=0; dialog->boxmax[5]=100;

     strcpy(dialog->answer[1][0], read_filename);
     itoa(g.tif_xoffset,temp2,10);
     strcpy(dialog->answer[2][0], temp2);
     itoa(g.tif_yoffset,temp2,10);
     strcpy(dialog->answer[3][0], temp2);
     doubletostring(g.tif_xsize*100,5,temp2);       //  tif_xsize and tif_ysize are
                                          //  temporarily multiplied by 100
                                          //  for use in click box.
     strcpy(dialog->answer[4][0], temp2);
     doubletostring(g.tif_ysize*100,5,temp2);
     strcpy(dialog->answer[5][0], temp2);
     if(g.read_cmyk)     dialog->boxset[6]=1; else dialog->boxset[6]=0;
     if(g.read_grayscale)dialog->boxset[7]=1; else dialog->boxset[7]=0;
     if(g.read_convert)  dialog->boxset[8]=1; else dialog->boxset[8]=0;
     if(g.tif_positive)  dialog->boxset[9]=0; else dialog->boxset[9]=1;
     if(g.want_shell)    dialog->boxset[10]=1; else dialog->boxset[10]=0;
     if(g.read_swap_bytes) dialog->boxset[11]=1; else dialog->boxset[11]=0;
     strcpy(dialog->answer[13][0], g.decompression);
     strcpy(dialog->answer[14][0], g.compression_ext);
     dialog->boxset[15] = g.read_split_frames;
     dialog->boxset[16] = g.fix_cmyk;

     dialog->boxlist[1] = filelist;
     dialog->noofradios=2;
     dialog->noofboxes=17;
     dialog->helptopic=11;  
     dialog->want_changecicb = 0;
     dialog->f1 = readcheck;
     dialog->f2 = null;
     dialog->f3 = null;
     dialog->f4 = readfinish;
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
     dialog->busy = 0;
     dialogbox(dialog);

  }else
  {  if(noofargs>1) g.tif_xoffset = atoi(arg[2]);
     if(noofargs>2) g.tif_yoffset = atoi(arg[3]);
     if(noofargs>3) g.tif_xsize   = strtod(arg[4],&ptr);
     if(noofargs>4) g.tif_ysize   = strtod(arg[5],&ptr);
     if(noofargs>0) readimage(arg[1], noofargs, arg);     
     if(g.tif_xsize>1) g.tif_xsize=1;
     if(g.tif_ysize>1) g.tif_ysize=1;
     if(g.tif_xsize<0) g.tif_xsize=0;
     if(g.tif_ysize<0) g.tif_ysize=0;
     in_read = 0;
  }
}


//--------------------------------------------------------------------------//
//  readcheck                                                               //
//--------------------------------------------------------------------------//
void readcheck(dialoginfo *a, int radio, int box, int boxbutton)
{
   box=box; radio=radio; boxbutton=boxbutton;
   int k;
   int count = a->boxcount[1];
   int status=OK;
   char **filenames;
   char *ptr; 
   g.read_raw = a->radioset[0]-1;
   g.want_quantize = a->radioset[1];
   g.tif_xoffset = atoi(a->answer[2][0]);
   g.tif_yoffset = atoi(a->answer[3][0]);
   g.tif_xsize  = strtod(a->answer[4][0], &ptr)/100.0;
   g.tif_ysize  = strtod(a->answer[5][0], &ptr)/100.0;
   if(g.tif_xsize>1) g.tif_xsize = 1;
   if(g.tif_ysize>1) g.tif_ysize = 1;
   if(g.tif_xsize<0) g.tif_xsize = 0;
   if(g.tif_ysize<0) g.tif_ysize = 0;
   if(a->boxset[6]) g.read_cmyk     = 1; else g.read_cmyk     = 0;
   if(a->boxset[7]) g.read_grayscale= 1; else g.read_grayscale= 0;
   if(a->boxset[8]) g.read_convert  = 1; else g.read_convert  = 0;
   if(a->boxset[9]) g.tif_positive  = 0; else g.tif_positive  = 1;
   if(a->boxset[10]) g.want_shell   = 1; else g.want_shell    = 0;
   if(a->boxset[11]) g.read_swap_bytes = 1; else g.read_swap_bytes = 0;
   strcpy(g.decompression, a->answer[13][0]);
   strcpy(g.compression_ext, a->answer[14][0]);
   g.read_split_frames = a->boxset[15];
   g.fix_cmyk = a->boxset[16];

   filenames = a->boxlist[1];  
   if(radio == -2 && filenames != NULL)   //// User clicked Ok or Enter
   {   for(k=0;k<count;k++)
       {   status = readimage(filenames[k], 0, NULL);
           status_error_message(status);
       }
   }
}


//--------------------------------------------------------------------------//
//  readfinish                                                              //
//--------------------------------------------------------------------------//
void readfinish(dialoginfo *a)
{
   if(!in_read) return;
   int k, count = a->boxcount[1];
   char **filenames = (char**)a->boxlist[1];  
   for(k=0;k<count;k++) XtFree(filenames[k]);
   if(count) delete[] filenames;
   g.getout=0;
   in_read = 0;
}


//--------------------------------------------------------------------------//
// readfiles                                                                //
// Reads one or more image files. Accepts wildcards.                        //
// Use this function instead of readimage() for loading images when there   //
// is a chance the filename may contain wildcard characters.                //
// Returns the error status.                                                //
//--------------------------------------------------------------------------//
int readfiles(char* filename, int noofargs, char **arg)
{
   if(memorylessthan(4096)){  message(g.nomemory,ERROR); return(0); } 
   int status=OK;

   // In Unix, wildcard expansion is done by the shell. So we only 
   // have to read the specified file. Set skipswitch to 0 to prevent
   // redrawing until the end.

   if(filename!=NULL) 
   {  g.read_skipswitch = 1;
      status = readimage(filename, noofargs, arg);
      g.read_skipswitch = 0;
   }   
   return(status);
}


//--------------------------------------------------------------------------//
// readimage                                                                //
// Determines whether image is a TIF (=1), PCX (=2), IMA (3), IMG (4), etc. //
// Then automatically calls the appropriate file reading routine.           //
// This function only loads a single file.                                  //
// Use readfiles(), which checks for wildcards, to load an image with a     //
// user-specified filename which could have wildcards.                      //
//--------------------------------------------------------------------------//
int readimage(char* filename, int noofargs, char **arg)
{   
   if(g.diagnose){ printf("Reading image %s\n",filename);fflush(stdout); }
   int k,type,status=OK,oci=ci;
   static char actual_filename[FILENAMELENGTH];
   char tempstring[FILENAMELENGTH];
   char program[64];
   char fname[1024];
   int compress = 0;

   strcpy(read_filename, filename);
   FILE *fp;
   ////  Test if file is readable by trying to open it - a more thorough 
   ////  test than access(). Also check if it's a directory.

   if(is_dir(filename)) 
   {   sprintf(tempstring, "%s\nis a directory", filename);
       message(tempstring, ERROR); 
       return ERROR; 
   }   
   if((fp=fopen(filename,"rb")) == NULL)
   {   error_message(filename, errno);
       return(ERROR);
   }
   fclose(fp);
   g.invertbytes=0;
  
   switch(g.read_raw)
   {   case 1:  type = RAW; break;
       case 2:  type = ASCII; break;
       case 3:  type = RAW3D; break;
       default: type = imagefiletype(filename, identifier, compress);
   }
   g.read_signedpixels = 0;
   g.state=READING;
   printstatus(g.state);
   drawselectbox(OFF);
   setSVGApalette(1,0); // Switch to grayscale in case image has no palette

   //// This temporary file is the only way to decompress some image file formats.
   //// The limiting factor is TIFF which is impossible to read without using
   //// fseek or lseek which can't be used in a pipe.
   //// No such limitation exists for writing image files, where we control
   //// the format, so writing can use a pipe.

   if(compress)
   {   strncpy(program, g.decompression, 64);
       for(k=0;k<(int)strlen(program);k++) if(program[k]==' ') program[k]=0;  
       if(access(program, X_OK)) 
       {    compress = 0;
            sprintf(tempstring, "Cannot execute %s", program);
            message(tempstring);
            return ABORT;
       }
       strcpy(fname, "/tmp/tni-temp");
       sprintf(tempstring, "%s %s %c %s", g.decompression, filename, '>', fname);
       system(tempstring);
       filename[strlen(filename) - strlen(g.compression_ext)] = 0;   
   }else
       strcpy(fname, filename);

   ////  If image reading routine calls a dialog, it must call 
   ////  initialize_image by itself, so it needs 'filename', 'compress',
   ////  and 'identifier'.

   strcpy(actual_filename, basefilename(filename));  // for title in WM
   printstatus(READING);
   switch(type)       // status==OK means no errors
   {   case TIF:   status=readtiffile(fname, actual_filename);     break;
       case PCX:   status=readpcxfile(fname);     break;
       case IMA:   status=readimafile(fname);     break;
       case IMM:
       case IMG:
       case RAW:
       case ASCII: 
                   noofargs = max(noofargs, 2);
                   strcpy(g.raw_filename, fname); // don't pass string, goes out of scope
                   strcpy(g.raw_filename2, filename);
                   status=readimgfile(type,noofargs,arg,compress,identifier);
                   break;
       case RAW3D: status=read3Dfile(fname);      break;
       case MULTIFRAME: status=readmultiframefile(fname);      break;
       case GIF:   status=readgiffile(fname);     break;
       case GEM:
       case IMDS:  status=readgemfile(fname,type);break;
       case JPG:   status=readjpgfile(fname);     break;
       case BMP:   status=readbmpfile(fname);     break;
       case TARGA: status=readtgafile(fname);     break;
       case PICT:  status=readjpgfile(fname);     break;
       case XWD:   status=readxwdfile(fname);     break;
       case FITS:  status=readfitsfile(fname);    break;
       case PDS:   status=readpdsfile(fname);     break;
       case PDS2:  status=readpds2file(fname);    break;
       case PBM:    
       case PGM:    
       case PPM:  
       case PBMRAW: 
       case PGMRAW: 
       case PPMRAW: status=readpbmfile(fname,0); break;
       case PNG:    status=readpngfile(fname); break;
       case CUSTOM: status=readcustomfile(fname, identifier);break;  
       case BIORAD: status=readbioradfile(fname);break;  
       case TEXT:   status=readtextfile(fname); break;
   }   


   //// Only executed if image reading function does not call a dialog.
   //// Image reading function sets g.read_signedpixels if necessary.

   if(ci != oci) initialize_image(fname, filename, ci, type, compress, 
                 g.read_signedpixels, identifier, status);

   printstatus(NORMAL);
   return OK;
}


//--------------------------------------------------------------------------//
// initialize_image                                                         //
//--------------------------------------------------------------------------//
void initialize_image(char *fname, char *filename, int ino, int type, 
                      int compress, int signedpixels, char *identifier,
                      int status)
{
   int delay,i,i2,j,k,oci,byteswap = 0;
   uint color;

   char tempstring[FILENAMELENGTH];
   //// For image types whose bytes are not automatically swapped in Solaris
   if(g.ximage_byte_order != LSBFirst)
   {   if(g.bitsperpixel == 8)
       {   switch(type)
           {    case JPG:
                case BMP:
                case CUSTOM:
                case TARGA:
                case PNG:
                   byteswap = 1;
                   break;
           }
       }
   }
   if(compress)
   {   unlink(fname);
       z[ino].was_compressed = 1;
   }
   printstatus(g.state);
   if(!g.tif_positive) invertimage(); 
   if(type != TIF && z[ino].bpp > 8 && z[ino].colortype == GRAY) byteswap = 1;
   if(g.invertbytes) byteswap = 1 - byteswap;
   if(g.read_swap_bytes) byteswap = 1 - byteswap;
   if(type==ASCII) byteswap=0;
   if(byteswap) swap_image_bytes(ino);
                                // Must be done after bytes are swapped 
                                // Set in "read raw bytes"
   if(signedpixels && (type==RAW || type==ASCII || type==CUSTOM)) 
      scale_signed_image(ino);
   if(ino>=0) setimagetitle(ino, strip_relative_path(filename));

   if(g.want_shell)
   {   
      XtVaSetValues(z[ino].widget, XmNtitle,
           basefilename(filename),
           XmNiconName, basefilename(filename), (char *)0);
      XtVaSetValues(XtParent(z[ino].widget), XmNtitle,
           basefilename(filename),
           XmNiconName, basefilename(filename), (char *)0);
      XtVaSetValues(XtParent(XtParent(z[ino].widget)), XmNtitle,
           basefilename(filename),
           XmNiconName, basefilename(filename), (char *)0);
   }
   strcpy(z[ino].format_name, identifier);

   if(status!=OK)
   {  sprintf(tempstring,"Error reading %s file",z[ino].format_name);
      message(tempstring,ERROR);
   }
   printstatus(g.state);
   if(g.read_grayscale) converttograyscale(ino);
                                 // Expand image if in 15-24 bpp modes
   if(g.read_convert) change_image_depth(ino,g.bitsperpixel,PERM);
   memcpy(z[ino].palette, g.palette, 768);
//   if(!g.want_colormaps) sortpalette(ino); // This also remaps used colors
   memcpy(z[ino].opalette, g.palette, 768);
   memcpy(z[ino].spalette, g.palette, 768);
                                 // Check if image is actually grayscale

//   if(unmodified_grayscale(z[ino].palette) &&  
//      z[ino].bpp==8 && type != GEM) z[ino].colortype=GRAY;  
  if(g.autoundo) backupimage(ino,0); 
   if(g.read_skipswitch == 0) switchto(ino);
   setpalette(z[ino].palette);
   rebuild_display(ino);
   repair(ino);
                                 // Different pixel interact modes
   if(g.imode!=SET)
   {  for(j=z[ino].ypos;j<z[ino].ypos+z[ino].ysize;j++)
      { for(i=z[ino].xpos,i2=0;i<z[ino].xpos+z[ino].xsize;i++,
              i2+=g.off[z[ino].bpp])
        {  color = pixelat(z[ino].image[z[ino].cf][j-z[ino].ypos]+i2,z[ino].bpp);
                                 // Take color from the image
                                 // Interact it with topmost pixel from
                                 // another image or background but not
                                 // itself (skip ino).
           setpixelonimage(i,j,color,g.imode,z[ino].bpp,ino);
        }   
        if(keyhit()) if(getcharacter()==27) break;
      }
      if(g.read_skipswitch==0) switchto(ino);          
      repairimg(ino,0,0,z[ino].xsize,z[ino].ysize);             // Fix up display
   }
       
   if(g.diagnose)
   {  printf("done, ino %d x %d y %d bpp %d\n",ino,z[ino].xsize,z[ino].ysize,z[ino].bpp); }
   g.state=NORMAL;
   printstatus(g.state);

   if(g.read_split_frames)
   {   oci = ci;
       split_frames(z[ci].xpos+10, z[ci].ypos+50, ci, 0, z[ci].window_border, 
           g.create_cols, 1);
       for(k=oci+1; k<=oci+z[oci].frames; k++)
       {   z[k].split_frames = 1;
           z[k].split_frame_start = oci + 1;
           z[k].split_frame_end = oci + z[oci].frames;       
           z[k].oframe_count = z[oci].frames;
           if(g.autoundo) backupimage(k, 0); 
           switchto(oci+1);
       }
   }
   if(z[ino].animation && z[ino].fps) 
   {  delay = (int)(1000/z[ino].fps);
      movie(delay);
   }
   return;
}


//--------------------------------------------------------------------------//
// fix_negative_value - rescale signed pixel to unsigned range              //
//--------------------------------------------------------------------------//
uint fix_negative_value(int v, int bpp)
{
   switch(bpp)
   {  case 7:
      case 8:  if(v & 0x80) return 255 - (v & 0x7f); 
      case 10: return 1024 - v;
      case 12: return 4096 - v;
      case 15:
      case 16: if(v & 0x8000) return 32767 - (v & 0x7fff);
      case 24:
      case 32: if(v & 0x800000) return 16777215 - (v & 0x7fffff); 
      default: return v;
   }
}


//--------------------------------------------------------------------------//
// scale_signed_image                                                       //
//--------------------------------------------------------------------------//
void scale_signed_image(int ino)
{
   int value, bpp, r, f, i, j;
   bpp = z[ino].bpp;
   r = g.off[bpp];
   for(f=0; f<z[ino].frames; f++)
   for(j=0; j<z[ino].ysize; j++)
   for(i=0; i<r*z[ino].xsize; i+=r)
   {  value = pixelat(z[ino].image[f][j]+i, bpp);
      value = fix_negative_value(value, z[ino].originalbpp);
      putpixelbytes(z[ino].image[f][j]+i,value,1,bpp,1);
   }   
   repair(ino);
}


//--------------------------------------------------------------------------//
// saveimage                                                                //
// Entry point for saving image on disk                                     //
//--------------------------------------------------------------------------//
int saveimage(int ino, int noofargs, char **arg)
{
  if(memorylessthan(16384)){ message(g.nomemory,ERROR); return NOMEM; }
  static Dialog *dialog;
  static char filename[FILENAMELENGTH];   
  static int write_all=1;
  static int file_format=TIF;
  static int compress=0;
  static int selection=0;
  static int ascii_selection=1;

  if(in_save) return BUSY;
  in_save = 1;
  int bpp,j,k; 
  combinedino = -1;
  if(g.selectedimage > -1) 
  {    write_all=1; 
       if(recombine_frames(ino)==CANCEL){ in_save=0; return ABORT; }// This can create image and change ino
  }else 
  {    write_all=0;
       if((g.selected_lrx - g.selected_ulx) *
          (g.selected_lry - g.selected_uly) <= 4)
       {   message("Please select an area \nor image to save",ERROR); 
           in_save = 0;
           return BADPARAMETERS;
       }
  }
  if(write_all && ino>=0 && strlen(z[ino].name)>0) 
  {    if(strchr(z[ino].name, '/'))                // If name has path
           strcpy(filename,z[ino].name);
       else                                        // If no path, add cwd
           sprintf(filename,"%s/%s",g.currentdir,z[ino].name);
       bpp = z[ino].bpp;
       strcpy(identifier, z[ino].format_name);
  }else 
  {    sprintf(filename,"%s/Untitled",g.currentdir);
       bpp = g.bitsperpixel;
       strcpy(identifier, "TIF");
  }
  if(ino>=0) file_format = z[ino].format;
  else file_format = TIF;

  ////  Get list of custom formats from disk

  customcount = count_custom_formats();
  custom_format_list = new char*[customcount];
  for(k=0;k<customcount;k++){ custom_format_list[k]=new char[64]; custom_format_list[k][0]=0;}
  read_custom_format_list(custom_format_list);
  noofformats = STDFORMATS + customcount;
  if(write_all && ino>=0) compress = z[ino].was_compressed;

  if(noofargs)
  {    if(noofargs>=1) strcpy(filename, arg[1]);
       if(noofargs>=2) file_format = atoi(arg[2]);
       if(noofargs>=3) g.want_bpp = atoi(arg[3]);
       write_all = 1;
       if(noofargs>=4) compress = atoi(arg[4]);
       save_image_file(filename, file_format, write_all, ino, compress);        
       in_save = 0;
       blocked_while_saving = 0;
       g.block = max(0,g.block-1);
  }else
  {    dialog = new Dialog;
       if(dialog==NULL){ message(g.nomemory); in_save = 0; return NOMEM; }
       strcpy(dialog->title,"Save Image");
       strcpy(dialog->radio[0][0],"Save What");             
       strcpy(dialog->radio[0][1],"Entire image");
       strcpy(dialog->radio[0][2],"Selected region");

       strcpy(dialog->radio[1][0],"Image type");             
       strcpy(dialog->radio[1][1],"1 bpp Monochrome");    // want_color_type==0
       strcpy(dialog->radio[1][2],"8 bpp Gray Scale");    // want_color_type==1
       strcpy(dialog->radio[1][3],"8 bpp Indexed Color"); // want_color_type==2
       strcpy(dialog->radio[1][4],"15 bits/pixel");       // want_color_type==3
       strcpy(dialog->radio[1][5],"16 bits/pixel");       // want_color_type==4
       strcpy(dialog->radio[1][6],"24 bits/pixel");       // want_color_type==5
       strcpy(dialog->radio[1][7],"32 bits/pixel");       // want_color_type==6
       strcpy(dialog->radio[1][8],"48 bits/pixel");       // want_color_type==7
       strcpy(dialog->radio[1][9],"Custom");              // want_color_type==8

       strcpy(dialog->radio[2][0],"Treat data as");             
       strcpy(dialog->radio[2][1],"Color");
       strcpy(dialog->radio[2][2],"Gray scale");

       //// Default radio settings

       if(write_all) dialog->radioset[0]=1;
                else dialog->radioset[0]=2;

       if(ino>=0 && z[ino].colortype==GRAY)
       {     dialog->radioset[2]=2;
             g.usegrayscale=1;
       }else
       {     dialog->radioset[2]=1;
             g.usegrayscale=0;
       }

       switch(bpp)     
       {   case 8:  if(g.usegrayscale) g.want_color_type=1; 
                    else g.want_color_type=2; 
                    break;
           case 15: g.want_color_type=3; break;
           case 16: g.want_color_type=4; break;
           case 24: g.want_color_type=5; break;
           case 32: g.want_color_type=6; break;
           case 48: g.want_color_type=7; break;
       }
       dialog->radioset[1] = g.want_color_type+1;
       dialog->noofradios=3;
       dialog->radiono[0]=3;
       dialog->radiono[1]=9;
       dialog->radiono[2]=3;
       dialog->radiono[3]=5;
       dialog->radiono[4]=0;
       for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
   
       ////  Boxes  

       strcpy(dialog->boxes[0],"File Parameters");
       strcpy(dialog->boxes[1],"Filename");
       strcpy(dialog->boxes[2],"Format");
       strcpy(dialog->boxes[3],"Image no.");

       strcpy(dialog->boxes[4],"Extra Parameters");
       strcpy(dialog->boxes[5],"Primary colors");
       strcpy(dialog->boxes[6],"Red bits/pixel");
       strcpy(dialog->boxes[7],"Green bits/pixel");
       strcpy(dialog->boxes[8],"Blue bits/pixel");
       strcpy(dialog->boxes[9],"Black bits/pixel");
       strcpy(dialog->boxes[10],"RGB->CMYK");
       strcpy(dialog->boxes[11],"JPEG quality");

       strcpy(dialog->boxes[12] ,"Post-processing");
       strcpy(dialog->boxes[13] ,"Process file");
       strcpy(dialog->boxes[14] ,"Command");
       strcpy(dialog->boxes[15] ,"Extension");

       strcpy(dialog->boxes[16] ,"Misc. parameters");
       strcpy(dialog->boxes[17] ,"Custom bits/pixel");
       strcpy(dialog->boxes[18] ,"ASCII Format");

       //// Default box values

       strcpy(dialog->answer[1][0],filename);
       //// 2 is below
       sprintf(dialog->answer[3][0],"%d",ino);
       sprintf(dialog->answer[5][0],"%d",g.want_noofcolors);
       sprintf(dialog->answer[6][0],"%d",g.want_redbpp);
       sprintf(dialog->answer[7][0],"%d",g.want_greenbpp);
       sprintf(dialog->answer[8][0],"%d",g.want_bluebpp);
       sprintf(dialog->answer[9][0],"%d",g.want_blackbpp);
       dialog->boxset[10] = g.save_cmyk;
       sprintf(dialog->answer[11][0],"%d",g.jpeg_qfac);
       dialog->boxset[13] = compress;
       strcpy(dialog->answer[14][0], g.compression);
       strcpy(dialog->answer[15][0], g.compression_ext);
       sprintf(dialog->answer[17][0],"%d",g.want_bpp);

       dialog->l[2]        = new listinfo;
       dialog->l[2]->title = new char[100];
       dialog->l[2]->title[0] = 0;
       dialog->l[2]->info  = new char*[noofformats];
       dialog->l[2]->count = noofformats;
       dialog->l[2]->wantfunction = 0;
       dialog->l[2]->f1    = null;
       dialog->l[2]->f2    = null;
       dialog->l[2]->f3    = null;
       dialog->l[2]->f4    = null; // dialog lists are deleted in dialogcancelcb
       dialog->l[2]->f5    = null;
       dialog->l[2]->f6    = null;
       dialog->l[2]->listnumber = 0;
       for(k=0; k<noofformats; k++)
       {    dialog->l[2]->info[k] = new char[100]; 
            dialog->l[2]->info[k][0] = 0;
       } 
       selection = format_selection(file_format);
       dialog->l[2]->selection = &selection;

       ////  Do not exceed 100 characters for the following menu items
       strcpy(dialog->l[2]->title, "File Format");
       strcpy(dialog->l[2]->info[0]," TIFF                ");
       strcpy(dialog->l[2]->info[1]," PCX                 ");
       strcpy(dialog->l[2]->info[2]," IMG (Frame grabber) ");
       strcpy(dialog->l[2]->info[3]," JPG (Jpeg)          ");
       strcpy(dialog->l[2]->info[4]," TGA (Targa RLE)     ");
       strcpy(dialog->l[2]->info[5]," BMP (Windows bitmap)");
       strcpy(dialog->l[2]->info[6]," GIF (GIF87a)        ");
       strcpy(dialog->l[2]->info[7]," GIF (GIF89a)        ");
       strcpy(dialog->l[2]->info[8]," ASCII               ");
       strcpy(dialog->l[2]->info[9]," RAW                 ");
       strcpy(dialog->l[2]->info[10]," Multiframe         ");
       strcpy(dialog->l[2]->info[11]," FITS                ");
       strcpy(dialog->l[2]->info[12]," PNG                 ");

       for(k=STDFORMATS; k<noofformats; k++)
           sprintf(dialog->l[2]->info[k]," %s", custom_format_list[k-STDFORMATS]);

       dialog->l[18]        = new listinfo;
       dialog->l[18]->title = new char[100];
       dialog->l[18]->title[0] = 0;
       dialog->l[18]->info  = new char*[4];
       dialog->l[18]->count = 4;
       dialog->l[18]->wantfunction = 0;
       dialog->l[18]->f1    = null;
       dialog->l[18]->f2    = null;
       dialog->l[18]->f3    = null;
       dialog->l[18]->f4    = null; // dialog lists are deleted in dialogcancelcb
       dialog->l[18]->f5    = null;
       dialog->l[18]->f6    = null;
       dialog->l[18]->listnumber = 0;
       for(k=0; k<4; k++)
       {    dialog->l[18]->info[k] = new char[100]; 
            dialog->l[18]->info[k][0] = 0;
       } 
       switch(g.save_ascii_format)
       {    case INTEGER:  ascii_selection=0; break;
            case DOUBLE:   ascii_selection=1; break;
            case RGBVALUE: ascii_selection=2; break;
            case CALIBRATED_VALUE: ascii_selection=3; break;
       }
       dialog->l[18]->selection = &ascii_selection;
       ////  Do not exceed 100 characters for the following menu items
       strcpy(dialog->l[18]->title, "ASCII Format");
       strcpy(dialog->l[18]->info[0]," Integer             ");
       strcpy(dialog->l[18]->info[1]," Density (0-1)       ");
       strcpy(dialog->l[18]->info[2]," RGB                 ");
       strcpy(dialog->l[18]->info[3]," Calibrated value    ");



       dialog->boxtype[0]=LABEL;        
       dialog->boxtype[1]=FILENAME;       
       dialog->boxtype[2]=NON_EDIT_LIST;
       dialog->boxtype[3]=INTCLICKBOX;
       dialog->boxtype[4]=LABEL;        
       dialog->boxtype[5]=INTCLICKBOX;
       dialog->boxtype[6]=INTCLICKBOX;
       dialog->boxtype[7]=INTCLICKBOX;
       dialog->boxtype[8]=INTCLICKBOX;  
       dialog->boxtype[9]=INTCLICKBOX;  
       dialog->boxtype[10]=TOGGLE;      
       dialog->boxtype[11]=INTCLICKBOX; 
       dialog->boxtype[12]=LABEL; 
       dialog->boxtype[13]=TOGGLE;      
       dialog->boxtype[14]=STRING;      
       dialog->boxtype[15]=STRING;
       dialog->boxtype[16]=LABEL;        
       dialog->boxtype[17]=INTCLICKBOX;
       dialog->boxtype[18]=NON_EDIT_LIST;

       dialog->boxmin[2]=0;  dialog->boxmax[2]=noofformats;
       dialog->boxmin[3]=1;  dialog->boxmax[3]=g.image_count-1;
       dialog->boxmin[5]=1;  dialog->boxmax[5]=4;
       dialog->boxmin[6]=0;  dialog->boxmax[6]=32;
       dialog->boxmin[7]=0;  dialog->boxmax[7]=32;
       dialog->boxmin[8]=0;  dialog->boxmax[8]=32;
       dialog->boxmin[9]=0;  dialog->boxmax[9]=32;
       dialog->boxmin[11]=1; dialog->boxmax[11]=99;
       dialog->boxmin[17]=1; dialog->boxmax[17]=32;
       dialog->boxmin[18]=0; dialog->boxmax[18]=4;

       dialog->noofboxes=19;
       dialog->helptopic=3;
       dialog->want_changecicb = 1;
       dialog->f1 = savecheck;
       dialog->f2 = null;
       dialog->f3 = unblock_save;
       dialog->f4 = savefinish;
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
       dialog->busy = 0;
       dialogbox(dialog);
  }
  return OK;
}   


//--------------------------------------------------------------------------//
//  savecheck                                                               //
//--------------------------------------------------------------------------//
void savecheck(dialoginfo *a, int radio, int box, int boxbutton)
{
  static int insavecheck=0;
  static int oci=-1, oxsize=-1, oysize=-1;
  static int oselectedimage = -1;
  static char filename[FILENAMELENGTH];
  char tempstring[128];
  if(!in_save){ message("Error",ERROR); return; }
  insavecheck=1;
  ////  If a new std. color type is added, add more entries to this table 
  ////  and inrease dimension of arrays if necessary   
  g.getout = 0;
  // want_color_type      0  1  2  3  4  5  6  7  8  9
  int w_bpp[10]       = { 1, 8, 8,15,16,24,32,48, 0, 8 };
  int w_noofcolors[10]= { 1, 1, 1, 3, 3, 3, 4, 3, 0, 1 };
  int w_redbpp[10]    = { 0, 6, 6, 5, 5, 8, 8,16, 0, 6 };
  int w_greenbpp[10]  = { 0, 6, 6, 5, 6, 8, 8,16, 0, 6 };
  int w_bluebpp[10]   = { 0, 6, 6, 5, 5, 8, 8,16, 0, 6 };
  int w_blackbpp[10]  = { 0, 0, 0, 0, 0, 0, 8, 0, 0, 6 };
  int w_savecmyk[10]  = { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 };

  radio=radio; box=box; boxbutton=boxbutton;
  int file_format=TIF,ino,j,k,tif=0,jpeg=0, compress=0, write_all,xsize=0,ysize=0;
  int selection = *(int*)a->l[2]->selection;
  int ascii_selection = *(int*)a->l[18]->selection;
  compress = a->boxset[13];

  if(ci>=0){ xsize = z[ci].xsize; ysize = z[ci].ysize; }
  if(ci != oci || oselectedimage != g.selectedimage || xsize !=oxsize || ysize != oysize)
  {   oci = ci;
      oxsize = xsize;
      oysize = ysize;

      oselectedimage = g.selectedimage;
      sprintf(filename, "%s/%s", g.currentdir, basefilename(z[ci].name));
      strcpy(a->answer[1][0], filename);

      compress = z[ci].was_compressed;
      a->boxset[13] = compress;

      if(g.selectedimage < 0) file_format = TIF;
      else 
      {    strcpy(identifier, z[ci].format_name);
           file_format = which_file_format(identifier);
      }
      selection = format_selection(file_format);
      *(int*)a->l[2]->selection = selection;

      ino = ci;
      itoa(ino, a->answer[3][0], 10);

      a->radioset[2]=1; 
      switch(z[ci].bpp)     
      {    case 8:  if(g.usegrayscale) g.want_color_type=1; 
                    else g.want_color_type=2; 
                    break;
           case 15: g.want_color_type=3; break;
           case 16: g.want_color_type=4; break;
           case 24: g.want_color_type=5; break;
           case 32: g.want_color_type=6; break;
           case 48: g.want_color_type=7; break;
      }
      if(g.selectedimage >=0)
      {    a->radioset[0] = 1;  // entire image
           write_all = 1;
      }else
      {    a->radioset[0] = 2;  // selected region
           write_all = 0;
      }
      if(z[ci].colortype==GRAY){ a->radioset[2]=2; g.usegrayscale=1;} 
      else { a->radioset[1] = g.want_color_type+1; g.usegrayscale=0;}

      g.want_bpp = z[ci].bpp;
      
      set_widget_string(a->boxwidget[1][0], filename);
      itoa(ino, tempstring, 10);
      set_widget_label(a->boxwidget[3][0], tempstring);
      set_widget_label(a->boxwidget[2][0], z[ino].format_name);
      set_widget_toggle(a->boxwidget[13][0], compress);     
      set_widget_radio(a, 0, a->radioset[0]);
      set_widget_radio(a, 1, a->radioset[1]);
      set_widget_radio(a, 2, a->radioset[2]);
  }
  switch(selection)
  {   case 0: file_format=TIF; tif=1; break;
      case 1: file_format=PCX;   break;
      case 2: file_format=IMG;   break;
      case 3: file_format=JPG; jpeg=1; break;
      case 4: file_format=TARGA; break;
      case 5: file_format=BMP;   break;
      case 6: file_format=GIF;   break;
      case 7: file_format=GIF89; break;
      case 8: file_format=ASCII; break;
      case 9: file_format=RAW;   break;
      case 10: file_format=MULTIFRAME; break;
      case 11: file_format=FITS; break;
      case 12: file_format=PNG;  break;
      default: file_format=CUSTOM;
               tif=1;
               strcpy(identifier, custom_format_list[selection-STDFORMATS]);
  }

  for(k=0;k<3;k++)
  {   for(j=5;j<=10;j++)
      {  if(a->boxwidget[j][k])
         {   if(tif)  XtSetSensitive(a->boxwidget[j][k],True);
             else     XtSetSensitive(a->boxwidget[j][k],False);
      }  }
      if(a->boxwidget[11][k])
      {   if(jpeg)  XtSetSensitive(a->boxwidget[11][k],True);
          else      XtSetSensitive(a->boxwidget[11][k],False);
      }
  }
  if(file_format==ASCII)
  {   XtSetSensitive(a->boxwidget[18][0],True);
      switch(ascii_selection)
      {    case 0: g.save_ascii_format = INTEGER; break;
           case 1: g.save_ascii_format = DOUBLE; break;
           case 2: g.save_ascii_format = RGBVALUE; break;
           case 3: g.save_ascii_format = CALIBRATED_VALUE; break;
      }
  }else   
      XtSetSensitive(a->boxwidget[18][0],False);

  for(k=14;k<=15;k++)
      if(compress) XtSetSensitive(a->boxwidget[k][0],True);
      else XtSetSensitive(a->boxwidget[k][0],False);
  if(a->radioset[1] == 8)
      XtSetSensitive(a->boxwidget[17][0],True);
  else
      XtSetSensitive(a->boxwidget[17][0],False);

  //// Save user settings
  
  for(k=1; k<=8; k++) if(a->radioset[1]==k) g.want_color_type=k-1;
  if(a->radioset[2]==2) g.usegrayscale=1; else g.usegrayscale=0;

  g.jpeg_qfac = atoi(a->answer[11][0]);
  if(g.jpeg_qfac<1) g.jpeg_qfac=80;
  if(g.want_color_type==8)                    // custom
  {  g.want_noofcolors = atoi(a->answer[5][0]);
     g.want_redbpp     = atoi(a->answer[6][0]);
     g.want_greenbpp   = atoi(a->answer[7][0]);
     g.want_bluebpp    = atoi(a->answer[8][0]);
     g.want_blackbpp   = atoi(a->answer[9][0]);
     g.save_cmyk       = a->boxset[10];
     g.want_bpp        = atoi(a->answer[17][0]);
  }else                                       // standard
  {  g.want_noofcolors= w_noofcolors[g.want_color_type];
     g.want_redbpp    = w_redbpp[g.want_color_type];
     g.want_greenbpp  = w_greenbpp[g.want_color_type];
     g.want_bluebpp   = w_bluebpp[g.want_color_type];
     g.want_blackbpp  = w_blackbpp[g.want_color_type];
     g.save_cmyk      = w_savecmyk[g.want_color_type];
     g.want_bpp       = w_bpp[g.want_color_type];
     switch(g.want_bpp)     
     {   case 8:  if(g.usegrayscale) g.want_color_type=1; 
                  else g.want_color_type=2; 
                  break;
         case 15: g.want_color_type=3; break;
         case 16: g.want_color_type=4; break;
         case 24: g.want_color_type=5; break;
         case 32: g.want_color_type=6; break;
         case 48: g.want_color_type=7; break;
     }
  }
  if(strlen(a->answer[14][0])) strcpy(g.compression, a->answer[14][0]);
  if(strlen(a->answer[15][0])) strcpy(g.compression_ext, a->answer[15][0]);

  if(g.usegrayscale) g.want_noofcolors = 1;

  //// If user clicked ok button in dialog, save image

  if(radio == -2 && in_save) 
  {  if(a->radioset[0]==1) write_all=1; else write_all=0;
     strcpy(filename, a->answer[1][0]);
     ino = atoi(a->answer[3][0]);
     ino = min(g.image_count-1, ino);
     if(!between(ino, 0, g.image_count-1)) message("Image not found");
     else save_image_file(filename, file_format, write_all, ino, compress);
  }
  insavecheck=0;
  return;
}


//--------------------------------------------------------------------------//
// format_selection                                                         //
//--------------------------------------------------------------------------//
int format_selection(int file_format)
{  
  int selection=0;
  if(file_format==TIF)   selection = 0;
  if(file_format==PCX)   selection = 1;
  if(file_format==IMG)   selection = 2;
  if(file_format==JPG)   selection = 3;
  if(file_format==TARGA) selection = 4;
  if(file_format==BMP)   selection = 5;
  if(file_format==GIF)   selection = 6;
  if(file_format==GIF89) selection = 7;
  if(file_format==ASCII) selection = 8;
  if(file_format==RAW)   selection = 9;
  if(file_format==MULTIFRAME) selection = 10;
  if(file_format==FITS)  selection = 11;
  if(file_format==PNG)   selection = 12;
  if(file_format==CUSTOM)selection = 
         min(noofformats, STDFORMATS + list_match(custom_format_list,
                               identifier, noofformats));
  return selection;
}


//--------------------------------------------------------------------------//
// which_file_format                                                        //
//--------------------------------------------------------------------------//
int which_file_format(char *identifier)
{
   if(!strcmp(identifier, "TIF"))   return TIF;
   if(!strcmp(identifier, "TIFF"))  return TIF;
   if(!strcmp(identifier, "PCX"))   return PCX;
   if(!strcmp(identifier, "IMG"))   return IMG;
   if(!strcmp(identifier, "JPEG"))  return JPG;
   if(!strcmp(identifier, "Targa")) return TARGA;
   if(!strcmp(identifier, "TARGA")) return TARGA;
   if(!strcmp(identifier, "TGA"))   return TARGA;
   if(!strcmp(identifier, "BMP"))   return BMP;
   if(!strcmp(identifier, "GIF"))   return GIF;
   if(!strcmp(identifier, "GIF87")) return GIF;
   if(!strcmp(identifier, "GIF89")) return GIF89;
   if(!strcmp(identifier, "ASCII")) return ASCII;
   if(!strcmp(identifier, "RAW"))   return RAW;
   if(!strcmp(identifier, "RAW3D")) return RAW3D;
   if(!strcmp(identifier, "MULTIFRAME")) return MULTIFRAME;
   if(!strcmp(identifier, "Multiframe")) return MULTIFRAME;
   if(!strcmp(identifier, "FITS"))  return FITS;
   if(!strcmp(identifier, "PNG"))   return PNG;
   if(!strcmp(identifier, "CUSTOM"))return CUSTOM;
   if(!strcmp(identifier, "BIORAD"))return BIORAD;
   return TIF;
}


//--------------------------------------------------------------------------//
//  unblock_save                                                            //
//--------------------------------------------------------------------------//
void unblock_save(dialoginfo *a)
{
   a=a;
   blocked_while_saving = 0;
   g.block = max(0,g.block-1);
}


//--------------------------------------------------------------------------//
//  savefinish                                                              //
//--------------------------------------------------------------------------//
void savefinish(dialoginfo *a)
{
   int k;
   a=a;
   if(!in_save) return;
   for(k=0; k<customcount; k++) if(custom_format_list[k]) delete[] custom_format_list[k];
   if(between(combinedino, 1, g.image_count-1)) eraseimage(combinedino,0,0,0);
   combinedino = -1;
   if(custom_format_list) delete[] custom_format_list;
   hitnonstandard = 0;
   g.getout = 0;
   //// Unblock save_ok() which was waiting for user to save image before exiting
   blocked_while_saving = 0;
   in_save = 0; 
}


//--------------------------------------------------------------------------//
//  save_image_file - called only by savecheck, don't call directly         //
//--------------------------------------------------------------------------//
void save_image_file(char *filename, int file_format, int write_all, int ino, 
     int compress)
{
  int status=OK;
  static int param[10];
  static char actual_file[FILENAMELENGTH];   
  static PromptStruct ps;
  if(g.getout){  message("File transfer aborted", WARNING); g.getout=0; return; }
  
  ////  Check for consistency with specified file format
  int type = file_format;
  if(file_format>=100) type=CUSTOM;
  switch(check_save_parameters(filename, ino, type, write_all))
  {   case OK: break;
      case ABORT: message("File transfer aborted", WARNING);  return;
      default: return;
  }

  if(write_all && ino>=0) z[ino].was_compressed = compress;
  if(write_all && ino==-1) status=NOIMAGES;      // can't save, no images present
  if(g.getout){ g.getout=0; status=ABORT; }
  if(status==OK)
  {    if(write_all && ino>=0 && z[ino].fftstate!=0)
       {   write_all=0; setSVGApalette(1,0); }
       strcpy(actual_file, filename);
       if(compress) strcat(actual_file, g.compression_ext);         
       param[0] = file_format;
       param[1] = write_all;
       param[2] = ino;
       param[3] = compress;

       ps.filename = actual_file;    // filename to check for overwrite
       ps.text = filename;           // filename to use
       ps.f1 = save_image_file_part2;
       ps.f2 = null;
       ps.data = param;
       check_overwrite_file(&ps);
  }else message("Bad parameters");
}  
      

//--------------------------------------------------------------------------//
//  save_image_file_part2                                                   //
//--------------------------------------------------------------------------//
void save_image_file_part2(PromptStruct *ps)
{
  char *filename  = ps->text;       // filename w/o .gz extension
  char *actual_file = ps->filename; // filename w/.gz extension if compress==1
  int *param = (int*)ps->data;
  int file_format = param[0];
  int write_all   = param[1];
  int ino         = param[2];
  int compress    = param[3];
  char tempstring[FILENAMELENGTH];   
  int evenbits, savedxsize, savedysize, status=OK;
  uint size;
  if(g.getout || !in_save){ message("Aborted"); return; }
  if(write_all) switchto(ino);      
  printstatus(WRITING);

  ////  Writing routine is free to ignore 'compress' variable if
  ////  compression program is not found.

  g.busy=1;
  if(file_format==TIF)   status = writetiffile(filename,write_all,compress);
  if(file_format==PCX)   status = writepcxfile(filename,write_all,compress);
  if(file_format==IMG)   status = writeimgfile(filename,write_all,compress);
  if(file_format==GIF)   status = writegiffile(filename,write_all,compress);
  if(file_format==GIF89) status = writegif89file(filename,write_all,compress);
  if(file_format==JPG)   status = writejpeg(filename,write_all,compress);
  if(file_format==BMP)   status = writebmpfile(filename,write_all,BMP,compress);
  if(file_format==TARGA) status = writetgafile(filename,write_all,compress);
  if(file_format==ASCII) status = writeasciifile(filename,write_all,compress);
  if(file_format==RAW)   status = writerawfile(filename,write_all,compress);
  if(file_format==RAW3D) status = write3Dfile(filename,write_all,compress);
  if(file_format==MULTIFRAME) status = writemultiframefile(filename,compress);
  if(file_format==FITS)  status = writefitsfile(filename,write_all,compress);
  if(file_format==PNG)   status = writepngfile(filename,write_all,compress);
  if(file_format==CUSTOM)status = writecustomfile(filename,write_all,identifier,compress);
  g.busy=0;
  printstatus(NORMAL);
 
  size = filesize(actual_file);                 // Check to ensure not an empty file
  if(size==0 && status==OK) status=ZEROLENGTH; 

  if(status==OK && !access(actual_file, F_OK))  // access = 0 if file exists
  {   if(write_all && ino>=0)
      {  savedxsize = z[ino].xsize;
         savedysize = z[ino].ysize;
      }else
      {  savedxsize = 1+g.selected_lrx-g.selected_ulx;
         savedysize = 1+g.selected_lry-g.selected_uly;
      }   
      evenbits = ((savedxsize*g.want_bpp+7)/8)*8; 
      while(savedxsize*g.want_bpp < evenbits) savedxsize++;
      sprintf(tempstring,"%d x %d pixel region saved\nin %s\n(size=%d bytes)",
          savedxsize, savedysize, actual_file, size);
      message(tempstring);
      if(ino>=0) z[ino].touched=0;
  }else 
  {   if(status==OK) error_message("File error", errno);
      else status_error_message(status);
  }   
  
  ////  Update image statistics

  if(write_all && status==OK && ino>=0)
  {   if(g.usegrayscale) z[ino].colortype=GRAY;
      if(ino>=0) setimagetitle(ino, filename);
      strcpy(z[ino].format_name, identifier);
      z[ino].format = file_format;
  }   
  return;
}


//--------------------------------------------------------------------------//
//  check_save_parameters                                                   //
//--------------------------------------------------------------------------//
int check_save_parameters(char *filename, int ino, int type, int write_all)
{
  int status=OK, tot_colors, gota0=0, standard=1; 
  int tot_bpp = g.want_redbpp+g.want_greenbpp+g.want_bluebpp+g.want_blackbpp;
  char msg[FILENAMELENGTH] = "";  
  char extension[FILENAMELENGTH]="X";
  char extension_upper[FILENAMELENGTH]="X";
  char tempstring[FILENAMELENGTH];   
  char *ptr;
  switch(type)
  {   case CUSTOM:
      case TIF:
         tot_colors=0;
         {  if(g.want_blackbpp)tot_colors++;
            if(g.want_bluebpp) tot_colors++; else if(tot_colors)gota0=1;
            if(g.want_greenbpp)tot_colors++; else if(tot_colors)gota0=1;
            if(g.want_redbpp)  tot_colors++; else if(tot_colors)gota0=1;
         }
         if(g.want_noofcolors!=tot_colors && g.want_color_type>2 &&  g.want_noofcolors>1)
         {  strcpy(msg,"Number of colors doesn't add up.\nChange the bits/pixel on a color\nor number of primary colors");
            status=BADPARAMETERS;
         }                 
         if(g.want_noofcolors>1 && g.want_color_type==8 && g.want_bpp!=tot_bpp)
         {  strcpy(msg,"RGB bits/pixel don't add up to total.\nChange total bits/pixel or number of colors");
            status=BADPARAMETERS;
         }
         if(g.want_noofcolors>1 && g.usegrayscale && g.want_color_type==8)
         {  strcpy(msg,"You must have only 1 primary color\nto save data as grayscale");
            status=BADPARAMETERS;
         }
         if(g.want_noofcolors==1 && !g.usegrayscale && g.want_color_type==8)
         {  strcpy(msg,"You must have more than 1 primary color\nto save data as color");
            status=BADPARAMETERS;
         }
         if(g.save_cmyk && (g.want_blackbpp==0 || g.want_color_type<6) && g.want_color_type>2) 
         {  strcpy(msg,"Not enough bits for CMYK");
            status=BADPARAMETERS;
         }
         if(g.want_blackbpp && !g.save_cmyk)
         {  strcpy(msg,"You must select CMYK to use black bits");
            status=BADPARAMETERS;
         }
         if(g.want_noofcolors<4 && g.save_cmyk)
         {  strcpy(msg,"Must have 4 colors for CMYK"); 
            status=BADPARAMETERS;
         }
         if(g.usegrayscale && g.want_bpp>24)
         {  strcpy(msg,"Grayscale TIFF files cannot exceed 24 bits/pixel"); 
            status=BADPARAMETERS;
         }
         if(!hitnonstandard)
         {  standard = 1;                
            if(g.want_color_type==8)
            {  if(g.want_redbpp%8  || g.want_greenbpp%8) standard=0;
               if(g.want_bluebpp%8 || g.want_blackbpp%8) standard=0;
               if(g.want_bpp>8     && g.want_noofcolors<3) standard=0;
               if(g.want_bpp<=8    && g.want_noofcolors>1) standard=0;
               if(g.want_bpp<8     && g.want_bpp>1)        standard=0;
               if(g.want_bpp>24    && g.want_noofcolors<4) standard=0;
               if(g.want_bpp<=24   && g.want_noofcolors==4)standard=0;
               if(g.want_bpp==16   && g.want_redbpp==5 && g.want_greenbpp==6 && g.want_bluebpp==5) standard=1;
               if(g.want_bpp==15   && g.want_redbpp==5 && g.want_greenbpp==5 && g.want_bluebpp==5) standard=1;
               if(gota0) standard=0;                    
           }
           if(standard==0)   
           {  strcpy(msg,"Non-standard image format\nIs this ok?");
              if(message(msg,YESNOQUESTION)!=YES) status=ABORT;
              hitnonstandard = 1;
           }   
         }
         break;
      case GIF:  
      case GIF89:if(g.want_bpp!=8)
                 {  strcpy(msg,"GIF files must be 8 bits/pixel");
                    status=BADPARAMETERS;
                 }
                 break;
      case PCX:  if(g.want_bpp!=8)
                 {  strcpy(msg,"PCX files must be 8 bits/pixel");
                    status=BADPARAMETERS;
                 }
                 break;
      case IMG:  if(g.want_bpp!=1 && g.want_bpp!=8)
                 {  strcpy(msg,"IMG files must be 1 or 8 bits/pixel");
                    status=BADPARAMETERS;
                 }
                 break;
      case RAW:
      case RAW3D:if(g.want_bpp!=8 && g.want_bpp!=15 && g.want_bpp!=16 && 
                    g.want_bpp!=24 && g.want_bpp!=32)
                 {  strcpy(msg,"Raw files must be 8,15,16,24,or 32 bits/pixel");
                    status=BADPARAMETERS;
                 }
                 break;
      case FITS:if(g.want_bpp!=8 && g.want_bpp!=16 && g.want_bpp!=32)
                 {  strcpy(msg,"FITS files must be 8,16, \nor 32 bits/pixel");
                    status=BADPARAMETERS;
                 }
                 break;
      case TARGA:if(g.want_bpp!=8 && g.want_bpp!=15 && g.want_bpp!=24 && g.want_bpp!=32)
                 {  strcpy(msg,"Targa files must be 8,15,24, \nor 32 bits/pixel");
                    status=BADPARAMETERS;
                 }
                 strcpy(tempstring, filename);
                 ptr = strchr(tempstring,'.');
                 if(ptr!=NULL) strcpy(extension, ptr);  
                 strcpy(extension_upper, extension);
                 strupr(extension_upper);
                 if(strcmp(extension_upper,".TGA")) //TGA can only be id'd by ext'n
                 {  strcpy(msg,"Extension must be TGA for Targa format");
                    status=BADPARAMETERS;
                 }
                 break;
      case BMP:  if(g.want_bpp!=4 && g.want_bpp!=8 && g.want_bpp!=24)
                 {  strcpy(msg,"BMP files must be 4,8,or 24 bits/pixel");
                    status=BADPARAMETERS;
                 }
                 if(g.want_color_type==1)
                 {  strcpy(msg,"8 bit BMP files must have a palette");
                    status=BADPARAMETERS;
                 }
                 break;
      case JPG:  if(g.want_bpp != 24)
                    strcpy(msg,"Selected bit/pixel values\nwill be ignored");
                 if(g.usegrayscale)
                 {  strcpy(msg,"Image must be converted to color first");
                    status=BADPARAMETERS;
                 }
                 break;
      case PNG:  if(g.usegrayscale && g.want_bpp>24)
                 {  strcpy(msg,"Grayscale PNG files cannot exceed 24 bits/pixel"); 
                    status=BADPARAMETERS;
                 }
                 break;
      default:   break;           
  }
  if(filename[0]==0)
  {     strcpy(msg,"You must specify a filename");
        status=BADPARAMETERS;
  }
  if(write_all && ino>=0 && z[ino].bpp==48 && type != TIF)
  {   strcpy(msg,"Must use TIFF format for 48-bit images");
      status=BADPARAMETERS;
  }
  if(status!=OK && strlen(msg)) message(msg, ERROR);

  //// Error messages requiring a response
  if( (ino>=0 && write_all && g.want_color_type<=2 && z[ino].bpp>8) ||
      (!write_all && g.want_color_type<=2 && g.bitsperpixel>8 ))
  {   strcpy(msg,"Color information will be lost!");
      if(!write_all) strcat(msg,"\nYou should create an 8 bit/pixel subimage first");
      else           strcat(msg,"\nYou should convert image to 8 bits/pixel first");
      strcat(msg,"\nOk to discard color information?");
      if(message(msg, YESNOQUESTION) != YES) status=ABORT;
  }
  if(write_all && z[ino].frames>1) 
  {   switch(type)
      {   case GIF89:
          case CUSTOM:
          case MULTIFRAME:
          case RAW3D: break;
          default:
             strcpy(msg,"Only current frame will be saved\nIs this ok?");
             if(message(msg, YESNOQUESTION) != YES) status=ABORT;
      }
  }
  if(ci==0) 
      if(message("Saving background", YESNOQUESTION) != YES) status=ABORT;
  return status;
}


//--------------------------------------------------------------------------//
// packbits                                                                 //
// Packs n bits into a block. Returns no.of bytes obtained.                 //
// Output block is the input bits packed into unsigned characters. This     //
//   means if n is a multiple of 8, the data are returned unchanged.        //
//   If not, the high-order bits from n to the next higher multiple of 8    //
//   are thrown away. The input data should                                 //
//   be right-shifted first if the high-order bits are to be saved.         //
//                                                                          //
// inpos     = Current position in array `inblock' (wraps to 0 if all the   //
//             bytes are used, i.e. if blockpos exceeds blocksize.          //
// outpos    = Current position in array `outblock'.                        //
// blocksize = Size of input array `inblock'                                //
// inblock   = char array of the data to be packed                          //
// outblock  = packed data which is returned                                //
//                                                                          //
// Bits are packed in the following format.                                 //
//                                                                          //
// Example for n=15:                                                        //
//     Input            7   6   5   4   3   2   1   0                       //
//     ------         --------------------------------                      //
//     Byte 1          a15 a14 a13 a12 a11 a10 a09 a08                      //
//     Byte 2          a07 a06 a05 a04 a03 a02 a01 a00                      //
//     Byte 3          b15 b14 b13 b12 b11 b10 b09 b08                      //
//     Byte 4          b07 b06 b05 b04 b03 b02 b01 b00                      //
//     ...                                                                  //
//     Output           7   6   5   4   3   2   1   0   Note: bit 15 is     //
//     ------         --------------------------------  thrown away.        //
//     Byte 1          a14 a13 a12 a11 a10 a09 a08 a07                      //
//     Byte 2          a06 a05 a04 a03 a02 a01 a00 b14                      //
//     Byte 3          b13 b12 b11 b10 b09 b08 b07 b06                      //
//     Byte 4          b05 b04 b03 b02 b01 b00 c14 c13                      //
//     ...                                                                  //
//                                                                          //
// Example for n=5:                                                         //
//     Input            7   6   5   4   3   2   1   0                       //
//     ------         --------------------------------                      //
//     Byte 1           a7  a6  a5  a4  a3  a2  a1  a0                      //
//     Byte 2           b7  b6  b5  b4  b3  b2  b1  b0                      //
//     Byte 3           c7  c6  c5  c4  c3  c2  c1  c0                      //
//     Byte 4           d7  d6  d5  d4  d3  d2  d1  d0                      //
//     ...                                                                  //
//     Output           7   6   5   4   3   2   1   0   Note: bits 7-4 are  //
//     ------         --------------------------------  thrown away. The    //
//     Byte 1           a4  a3  a2  a1  a0  b4  b3  b2  rest are packed     //
//     Byte 2           b1  b0  c4  c3  c2  c1  c0  d4  together.           //
//     Byte 3           d3  d2  d1  d0  e4  e3  e2  e1                      //
//     Byte 4           e0  f4  f3  f2  f1  f0  g4  g3                      //
//     ...                                                                  //
//--------------------------------------------------------------------------//
int packbits(int n,uchar* inblock,uchar* outblock, int blocksize)
{
   int k;
   int inpos = 0;   // input buffer byte position
   int outpos = 0;  // output buffer byte position
   int bitstoget=8; // no.of bits it is possible to get
   int need = 8;    // output bit position 8-0
   int total=0;     // input bit position 0-8
   int unusedbits;  // no.of bits to throw away
   int roundbytes;  // bytes in input for each n (n rounded up to next byte)
   int a;

   getbits(0,inblock,0,RESET);  //reset bits at each scan line

   roundbytes = (n+7) >> 3;
   for(k=0;k<blocksize;k++) outblock[k]=0;
   getbits(0,inblock,blocksize,RESET);
   unusedbits = (8 - (n%8))%8;  // 0,7,6,5,4,3,2,1,0,...
   getbits(unusedbits,inblock,blocksize,TIF);

   do
   {  bitstoget = min(need,n-total);
      a = getbits(bitstoget,inblock,blocksize,TIF) << (need-bitstoget);
      outblock[outpos] += a;
      total += bitstoget;
      need -= bitstoget;
      if(need==0) { outpos++; need=8; }
      if(total==n) 
      {  getbits(unusedbits,inblock,blocksize,TIF);
         total=0;
         inpos += roundbytes;
      }
   }while(inpos<blocksize);   
   return(outpos);
}


//--------------------------------------------------------------------------//
// imagefiletype                                                            //
// Returns integer TIF or whatever or 0 if unknown image file type.         //
// Returns string format identifier (must have enough for 64 bytes).        //
//--------------------------------------------------------------------------//
int imagefiletype(char *filename, char *identifier, int &compressed)
{  
   if(memorylessthan(16384)){ message(g.nomemory,ERROR); return(0); }

   uchar header[1034];  // Make static to avoid out-of-memory
   uchar oheader[1034];
   char tempstring[1024];
   FILE *fp;
   int dot,k,h,ocompressed;
   short uint word1=0;
   short uint word2=0;
   short uint word3=0;
   short uint word4=0;
   short int pcx_id=0;
   char fitsheader[20];
   char rawheader[20];
   char newfilename[FILENAMELENGTH];
   char gifid[50];
   char pbmid[5];
   char match[64] = "";
   char extension[FILENAMELENGTH]="X";
   char extension_upper[FILENAMELENGTH]="X";
   uchar pcx_version=0;
   short int xsize,ysize;
   long int fsize;
   int pos, elength;
   
   compressed = 0;
   elength = (int)strlen(g.compression_ext);
   if(elength) pos = (int)strlen(filename) - elength; else pos=0;
   if(pos>=0 && strcmp(filename + pos, g.compression_ext)==SAME) compressed = 1;

   //// This will change 'compressed' if g.decompression is not found

   ocompressed = compressed;
   if((fp=open_file(filename, compressed, TNI_READ, g.compression, g.decompression, 
       g.compression_ext))==NULL) 
       return 0;
   compressed = ocompressed;

   if(compressed){ strncpy(newfilename, filename, pos); newfilename[pos]=0; }
   else strcpy(newfilename, filename);
   
   fread(header,1024,1,fp);
   strncpy(identifier, (char*)header, 63);
   memcpy(oheader,header,128);
   close_file(fp, compressed);
   g.tif_skipbytes=0;

   //-------------Extract extension from filename------------//
   strcpy(tempstring, newfilename);
   dot = 0;
   for(k=0;k<(int)strlen(tempstring);k++) if(tempstring[k]=='.') dot=k;
   if(dot) strcpy(extension, tempstring+dot+1);

   //-------------Identify format----------------------------//
   for(h=0;h<=1;h++)
   { 
      word1 = header[0] + (header[1]<<8);
      word2 = header[2] + (header[3]<<8);
      word3 = header[4] + (header[5]<<8);
      word4 = header[6] + (header[7]<<8);
      pcx_id = word1 & 0xff;
      pcx_version = word1 & 0xff;
      word1 &= 0xffff;
      word2 &= 0xffff;
      strncpy(gifid, (char*)header, 5);
      gifid[5] = 0;
      pbmid[0]=header[0]; pbmid[1]=header[1]; pbmid[2]=0;
      override = 0;

      strncpy(fitsheader, (char*)header, 6);
      strncpy(rawheader, (char*)header, 10);
      strcpy(extension_upper, extension);
      strupr(extension_upper);
      fitsheader[6]=0;
      if((word2==42)&&(word1==0x4949))
      {    strcpy(identifier,"TIF");  return(TIF); }            // Intel TIF
      else if((word2==0x2A00)&&(word1==0x4d4d))
      {    strcpy(identifier,"TIF");  return(TIF); }            // Macintosh TIF
      else if(!strcmp(extension,"ascii"))  
      {    strcpy(identifier,"ASCII");  return(ASCII); }
      else if(!strcmp(extension,"txt"))  
      {    strcpy(identifier,"TEXT");  return(TEXT); }
      else if(pcx_id==0x0A)
      {    strcpy(identifier,"PCX");  return(PCX); }
      else if(pcx_version==253)
      {    strcpy(identifier,"IMA");  return(IMA); }    
      else if(!strcmp(gifid,"GIF87"))
      {    strcpy(identifier,"GIF87");  return(GIF); }       
      else if(!strcmp(gifid,"GIF89"))
      {    strcpy(identifier,"GIF89");  return(GIF); }       
      else if((word1==0x0100)&&((word2==0x0800)||(word2==0x0900)))// Regular GEM IMG
      {    strcpy(identifier,"GEM");  return(GEM); }
      else if((word1==0x0070)&&(word2==0x0191))    // IMDS = MVS mainframe GEM
      {    strcpy(identifier,"IMDS"); return(IMDS); }
      else if(((word1)==0xD8FF)&&((word2&0xFF)==0xFF))
      {    strcpy(identifier,"JPEG"); return(JPG); }
      else if((gifid[0]=='B')&&(gifid[1]=='M'))
      {    strcpy(identifier,"BMP");  return(BMP); }
      else if(word1==0x0000 && word3==0x0000 && word4==0x0700)
      {    strcpy(identifier,"XWD screen dump"); return(XWD); }
      else if(!strcmp(extension_upper,"TGA"))   // TGA can only be ident'd by extension
      {    strcpy(identifier,"Targa"); return(TARGA); }
      else if(!strcmp(fitsheader,"SIMPLE"))  
      {    strcpy(identifier,"FITS"); return(FITS); }
      else if(!strcmp(rawheader,"Multiframe"))  
      {    strcpy(identifier,"MULTIFRAME"); return(MULTIFRAME); }
      else if(strstr((char*)header,"FILE_TYPE"))  
      {    strcpy(identifier,"PDS");  return(PDS); }
      else if(strstr((char*)header,"SFDU_LABEL"))  
      {    strcpy(identifier,"PDS2"); return(PDS2); }
      else if(!strcmp(pbmid,"P1"))  
      {    strcpy(identifier,"PBM");  return(PBM); }
      else if(!strcmp(pbmid,"P2"))  
      {    strcpy(identifier,"PGM");  return(PGM); }
      else if(!strcmp(pbmid,"P3"))  
      {    strcpy(identifier,"PPM");  return(PPM); }
      else if(!strcmp(pbmid,"P4"))  
      {    strcpy(identifier,"PBMRAW");  return(PBMRAW); }
      else if(!strcmp(pbmid,"P5"))  
      {    strcpy(identifier,"PGMRAW");  return(PGMRAW); }
      else if(!strcmp(pbmid,"P6"))  
      {    strcpy(identifier,"PPMRAW");  return(PPMRAW); }
      else if(check_if_png(identifier))  
      {    strcpy(identifier,"PNG");  return(PNG); }
      else if((word1==0xAFAF)&&(word2==0x7453))
      {    strcpy(identifier,"BIORAD");  return(BIORAD); }     
      else if(is_customfile(filename, identifier, extension, match))
      {    strcpy(identifier, match); return(CUSTOM);   }
      else                    
      {                             // Check if an IMG by checking its size.
          xsize=word1;
          ysize=word2;
          fsize = filesize(newfilename);
          k = 2*sizeof(short int) + xsize*ysize;   // Expected file size
                                                   // Some valid files have a 
                                                   // header or tail at end 
                                                   // Assume>10 pixels in image
          if((fsize!=0 && k>10) || strcmp(extension,"img")==SAME)
          {
            if(((double)k/(double)fsize >=.95) && ((double)k/(double)fsize <=1.1))
            {    strcpy(identifier,"IMG"); return(IMG); }    // 8-bit bitmap
            else 
            if(((double)k/(double)(fsize*8)>=.95)&&((double)k/(double)(fsize*8)<=1.1))
            {    strcpy(identifier,"IMM"); return(IMM); }    // Monochrome bitmap
          }
      }
                                  // Check if there is an extra 128-byte
                                  // header added by Mac-TCP.                            
      for(k=0;k<128;k++) header[k]=header[k+128];
      g.tif_skipbytes+=128;
   }

   ////  Eliminating Mac header didn't help so reset tif_skipbytes to 0.
   g.tif_skipbytes=0;   
   sprintf(tempstring,"%s\nInvalid image file - try to read it anyway?",filename);
   if(message(tempstring,QUESTION)==YES)
   {     override=1; 
         strcpy(identifier,"RAW");  
         return(RAW); 
   }
   return(0);
}


//--------------------------------------------------------------------------//
// strupr                                                                   //
//--------------------------------------------------------------------------//
char *strupr(char *s)
{
   int k;
   for(k=0;k<(int)strlen(s);k++) s[k]=toupper(s[k]);
   return s;
}


//--------------------------------------------------------------------------//
// strlwr                                                                   //
//--------------------------------------------------------------------------//
char *strlwr(char *s)
{
   int k;
   for(k=0;k<(int)strlen(s);k++) s[k]=tolower(s[k]);
   return s;
}


//--------------------------------------------------------------------------//
//  remove_terminal_cr                                                      //
//--------------------------------------------------------------------------//
void remove_terminal_cr(char *s)
{
    int pos = max(0, (int)(strlen(s))-1);
    if(s[pos]=='\n') s[pos]=0;
}


//--------------------------------------------------------------------------//
//  filesize                                                                //
//--------------------------------------------------------------------------//
int filesize(char *filename)
{
     struct stat dstatus;
     stat(filename, &dstatus);
     return dstatus.st_size;
}  
 
//--------------------------------------------------------------------------//
//  is_dir                                                                  //
//--------------------------------------------------------------------------//
int is_dir(char *filename)
{
     struct stat dstatus;
     int mode;
     stat(filename, &dstatus);
     mode = dstatus.st_mode;
     if(S_ISDIR(mode)) return 1;
     return 0;
}  


//--------------------------------------------------------------------------//
//  recombine_frames - returns 0 unless user clicked Cancel or error        //
//--------------------------------------------------------------------------//
int recombine_frames(int &ino)
{
  char tempstring[FILENAMELENGTH];
  int source[MAXIMAGES];
  int k, count=0, obpp, answer=OK; 
  if(!between(ino, 0, g.image_count-1) || !z[ino].split_frames) return OK;
  count = 0;
  for(k=z[ino].split_frame_start; k<=z[ino].split_frame_end; k++) source[count++]=k;
  if(count <= 1)
  {   sprintf(tempstring,"Only 1 frame left\n(Originally %d frames)", z[ino].oframe_count);
      return message(tempstring, WARNING);
  }
  sprintf(tempstring,"Image was originally %d frames.\nThere are %d frames remaining.\nShould they be\nrecombined before saving?", z[ino].oframe_count,count);
  if((answer = message(tempstring, YESNOQUESTION)) == YES) 
  {   if(check_image_list(source, count, 1)) return CANCEL; // c.i.l displays err mesage
      obpp = z[source[0]].bpp;
  }else return answer;
  for(k=source[0]; k<=source[count-1]; k++)
  {   if(!between(k, 0, g.image_count-1) || !z[k].split_frames)
      {    message("Image not found\nCan't combine original frames", ERROR);
           return CANCEL;
      }
      switchto(k);
      change_image_depth(k, 24, 1);  // Convert to 24 so common colormap can be generated
      rebuild_display(k);
      redraw(k);
  }
  
  //// This creates new image & changes ci
  if(combine_image_frames(z[ino].xpos+10, z[ino].ypos+10, z[ino].shell, 
       z[ino].window_border, source, count) == OK) 
  {    ino = combinedino = ci;
       switchto(ino);
       change_image_depth(ino, obpp, 1);
       z[ino].format = GIF89;
       rebuild_display(ino);
       redraw(ino);
       setimagetitle(ino, "Combined");
  }else
  {    message("Unable to combine frames", ERROR); 
       return CANCEL;
  }
  
  for(k=source[0]; k<=source[count-1]; k++)
  {    if(between(k, 0, g.image_count-1) && z[k].split_frames)
       {   switchto(k);                    
           change_image_depth(k, obpp, 1);
           rebuild_display(k);
           redraw(k);
       }
  }
  switchto(ino);
  return OK; 
}
