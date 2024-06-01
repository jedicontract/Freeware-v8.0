//--------------------------------------------------------------------------//
// xmtnimage7.cc                                                            //
// GIF image file reading & writing routines.                               //
// Latest revision: 06-15-2004                                              //
// Copyright (C) 2000 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//  GIF and 'Graphics Interchange Format' are trademarks (tm) of            //
//       Compuserve,  Incorporated, an H&R Block Company                    //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
extern PrinterStruct printer;
 

//------Variables for writing GIF files---//

short int clear=0, codebits=0, colors=0, destoff=0, destseg=0, gend=0, length=0,
    lastentry=0, nbits=0, nbytes=0, entries=0, actual=0, srcoff=0, scrseg=0,
    startbits=0;
ushort next=0, str_index[5003];
int hashcode=0;
int totalbytes=0;

//--------------------------------------------------------------------------//
// aboutgiffile                                                             //
// Display information about gif file.                                      //
//--------------------------------------------------------------------------//
void aboutgiffile(char *filename)
{
   static listinfo *l;
   static char **info;                                    
   static char *listtitle;
   listtitle = new char[100];
   strcpy(listtitle, "About the File");

   if(memorylessthan(16384)){ message(g.nomemory,ERROR); return; }
   cls(g.bcolor);
   FILE* fp;
   int k;
   char junk;
   
   uchar *temp;
   temp = new uchar[1000];
   if(temp==NULL){ message(g.nomemory,ERROR); return; }

   int screenheight=0,screenwidth=0;
   char colortableflag, colorresolution,sortflag,sizeofglobalcolortable;
   int noofcolors=0, bkgcolor=0;
   int imageleft=0,imagetop=0,imagewidth=0,imageheight=0;
   char byte,aspectratio;
   char interlaceflag,sizeoflocalcolortable;
   int startsize=0,clearcode=0,endcode=0;
   
   if ((fp=fopen(filename,"rb")) == NULL)
   {   error_message(filename, errno);
       delete[] temp;
       return;
   }
 
   //-------------Put information in list box-------------------------//

   int lc=-1;
   info = new char*[100];   // max. 100 lines - Change if you add more items

   info[++lc] = new char[100];                       
   sprintf(info[lc],"Filename:  %s",basefilename(filename));

   //-------program name-------------------------//


   if(g.tif_skipbytes) for(k=0;k<g.tif_skipbytes;k++) fread(&junk,1,1,fp);
   fread(temp,1,6,fp);
       temp[6]=0;
       info[++lc]=new char[100]; sprintf(info[lc],"File type %s",(char*)temp);
   screenwidth = getword(fp);

       info[++lc]=new char[100]; sprintf(info[lc],"Screen width %d",screenwidth);
   screenheight = getword(fp);
       info[++lc]=new char[100]; sprintf(info[lc],"Screen height %d",screenheight);
   byte = getbyte(fp);
         //  Global color table flag - if 1, a global color table follows. 
         //  If 0, bkg color index is  meaningless.
     colortableflag = (byte & 127) / 127;
     colorresolution = (byte & 112) / 16 + 1;
     sortflag = (byte & 8) / 8;
     sizeofglobalcolortable = byte & 7;
     noofcolors = 1 <<(sizeofglobalcolortable + 1);
     info[++lc]=new char[100]; sprintf(info[lc],"Color table flag %d",(int)colortableflag);
     info[++lc]=new char[100]; sprintf(info[lc],"Color resolution %d",(int)colorresolution);
     info[++lc]=new char[100]; sprintf(info[lc],"Sort flag %d",(int)sortflag);
     info[++lc]=new char[100]; sprintf(info[lc],"Global color table size %d",(int)sizeofglobalcolortable);
     info[++lc]=new char[100]; sprintf(info[lc],"No. of colors %d",(int)noofcolors);
   bkgcolor = getbyte(fp);
     info[++lc]=new char[100]; sprintf(info[lc],"Background color %d",bkgcolor);
   aspectratio = getbyte(fp);
     info[++lc]=new char[100]; sprintf(info[lc],"Aspect ratio %d",(int)aspectratio);
   if (sizeofglobalcolortable > 0) 
   {   info[++lc]=new char[100]; sprintf(info[lc],"No.palette colors %d",noofcolors);
       fread(temp,1,(3*noofcolors),fp);
   }
   byte = getbyte(fp);
       info[++lc]=new char[100]; sprintf(info[lc],"Image separator %d",(int)byte);
   if(byte!=0x2c)
   {   message("Invalid GIF file",ERROR);  // no image separator
       goto endaboutgif;
   }
   imageleft = getword(fp);
       info[++lc]=new char[100]; sprintf(info[lc],"Left pos %d",imageleft);
   imagetop = getword(fp);
       info[++lc]=new char[100]; sprintf(info[lc],"Top pos %d",imagetop);
   imagewidth = getword(fp);
       info[++lc]=new char[100]; sprintf(info[lc],"Image width %d",imagewidth);
   imageheight = getword(fp);
       info[++lc]=new char[100]; sprintf(info[lc],"Image height %d",imageheight);
   byte = getbyte(fp);
     colortableflag = (byte & 128) / 128;
     interlaceflag = (byte & 64) / 64; 
     sortflag = (byte & 32) / 32;
     sizeoflocalcolortable = (byte & 7);
     sizeoflocalcolortable = 1 << (sizeoflocalcolortable + 1);
       info[++lc]=new char[100]; sprintf(info[lc],"Local color table %d",(int)colortableflag);
       info[++lc]=new char[100]; sprintf(info[lc],"Interlace %d",(int)interlaceflag);
       info[++lc]=new char[100]; sprintf(info[lc],"Sort %d",(int)sortflag);
       info[++lc]=new char[100]; sprintf(info[lc],"Local color table size %d",(int)sizeoflocalcolortable);
   byte = getbyte(fp);                // LZW minimum code size(bits)
     startsize = byte + 1;
     clearcode = 1 << byte;
     endcode = clearcode + 1;
     info[++lc]=new char[100]; sprintf(info[lc],"Starting code size %d",(int)startsize);
     info[++lc]=new char[100]; sprintf(info[lc],"Clear code %d",(int)clearcode);
     info[++lc]=new char[100]; sprintf(info[lc],"End code %d",(int)endcode);

endaboutgif:
   fclose(fp);
   delete[] temp;   

   l = new listinfo;
   l->title = listtitle;
   l->info  = info;
   l->count = lc+1;
   l->itemstoshow = min(20,lc+1);
   l->firstitem   = 0;
   l->wantsort    = 0;
   l->wantsave    = 0;
   l->helptopic   = 0;
   l->allowedit   = 0;
   l->selection   = &g.crud;
   l->width       = 0;
   l->transient   = 1;
   l->wantfunction = 0;
   l->autoupdate   = 0;
   l->clearbutton  = 0;
   l->highest_erased = 0;
   l->f1 = null;
   l->f2 = null;
   l->f3 = null;
   l->f4 = delete_list;
   l->f5 = null;
   l->f6 = null;
   l->listnumber = 0;
   list(l);
}


//--------------------------------------------------------------------------//
// readgiffile                                                              //
// read GIF format file - returns no. of images read                        //
// Returns error code (OK if successful).                                   //
//--------------------------------------------------------------------------//
int readgiffile(char* filename)
{
   if(memorylessthan(4096)){  message(g.nomemory,ERROR); return(NOMEM); } 
   FILE *fp;
   RGB gif_palette[256];
   int done,status=OK,bpp,count=0,i,k,noofcolors=0,localnoofcolors=0,
       delay=0, frame=0, label, byte_count, gotpalette=0, dx, dy,
       startsize, clearcode, kxfac, kyfac, transparency, x, y, rr,gg,bb;
   int *yvalue, *bytesperstrip, *strip;  
   uchar byte=0;
   struct
   {  char signature[6];
      short int  screen_width;
      short int  screen_height;
      uchar global_color_table_size  : 3;  // 3 bits
      uchar sort_flag       : 1;
      uchar color_resolution: 3;
      uchar global_map_flag : 1;
      uchar background;     
      uchar aspect_ratio;
   } screen_descriptor;
   struct
   {  short int x1;                       // 2 bytes
      short int y1;                       // 2 bytes
      short int width;                    // 2 bytes
      short int height;                   // 2 bytes
      uchar local_color_table_size : 3;   // 3 bits
      uchar reserved        : 2;          // 2 bits
      uchar sort_flag       : 1;          // 1 bit
      uchar interlace_flag  : 1;          // 1 bit
      uchar local_color_map : 1;          // 1 bit
   } image_descriptor;
   struct
   {  uchar reserved        : 3;          // 3 bit
      uchar disposal        : 3;          // 3 bit
      uchar user_input      : 1;          // 1 bit
      uchar transparent     : 1;          // 1 bit
      short uint delay;                   // 2 bytes
      uchar tcolor;                       // 1 byte
   } graph_control;

   
   if ((fp=fopen(filename,"rb")) == NULL)
   {   error_message(filename, errno);
       return(NOTFOUND);
   }

   fread(&screen_descriptor, 13, 1, fp);
#ifndef LITTLE_ENDIAN
   swapbytes((uchar*)&screen_descriptor.screen_height, 2, 2); 
   swapbytes((uchar*)&screen_descriptor.screen_width, 2, 2); 
#endif

   bpp = screen_descriptor.global_color_table_size + 1;
   noofcolors = 1 << bpp;

   if(screen_descriptor.global_map_flag)
   {   fread(gif_palette,3*noofcolors,1,fp);
       gotpalette = 1;
       for (i=0; i<noofcolors; i++)
       {  gif_palette[i].red   >>=  2;
          gif_palette[i].green >>=  2;
          gif_palette[i].blue  >>=  2;
       }
   }

   bpp = (bpp+7)/8;

   //// Read images
   done=0;
   do
   {  count=0;
      while(count++ < 65535)
      {   if(feof(fp)) { done=1; break; }
          byte = getbyte(fp);     
          if(feof(fp)) { done=1; break; }
          if(byte==0x2c){ done=0; break; }  // another image
          if(byte==0x3b){ done=1; break; }  // terminator
          if(byte==0x21)                    // extension block
          {    
                 label = getbyte(fp);     
                 switch(label)
                 {  case 0xf9:  // graphic control label
                       byte_count = getbyte(fp);  // better be 4
                       fread(&graph_control, 4,1,fp);
#ifndef LITTLE_ENDIAN
   swapbytes((uchar*)&graph_control.delay, 2, 2); 
#endif
                       delay = graph_control.delay;
                       transparency = graph_control.transparent;
                       break;
                    case 0xff:  // application extension
                    case 0xfe:  // comment label
                    case 0x01:  // plain text 
                       byte_count=1;
                       while(byte_count!=0)
                       {   byte_count = getbyte(fp);                 
                           for(k=0;k<byte_count;k++) getbyte(fp);     
                       }
                       break;
                    default:
                       message("Error reading gif file extension block");
                       fclose(fp);
                       return ERROR;
                 }
                 if(feof(fp)) { done=1; break; }
          }
      }

      if(!done)
      { 
         fread(&image_descriptor, 9, 1, fp); 
#ifndef LITTLE_ENDIAN
         swapbytes((uchar*)&image_descriptor.x1, 2, 2); 
         swapbytes((uchar*)&image_descriptor.y1, 2, 2); 
         swapbytes((uchar*)&image_descriptor.width, 2, 2); 
         swapbytes((uchar*)&image_descriptor.height, 2, 2); 
#endif
         if(image_descriptor.width<0 || image_descriptor.height<0) 
         {    message("Error reading GIF file subimage");
              return ERROR;
         }
         if(image_descriptor.local_color_table_size)
             bpp = image_descriptor.local_color_table_size + 1;
         localnoofcolors = 1 << bpp;
         yvalue = new int[image_descriptor.height+2];
         if(image_descriptor.interlace_flag)
         {  count=0;
            for(k=0;k<image_descriptor.height;k+=8) yvalue[count++]=k;  
            for(k=4;k<image_descriptor.height;k+=8) yvalue[count++]=k;  
            for(k=2;k<image_descriptor.height;k+=4) yvalue[count++]=k;  
            for(k=1;k<image_descriptor.height;k+=2) yvalue[count++]=k;  
            yvalue[image_descriptor.height]=image_descriptor.height;
         }else
         { 
            for(k=0;k<=image_descriptor.height;k++) yvalue[k]=k; 
         }
         if(image_descriptor.local_color_map)
         {    fread(gif_palette,3*localnoofcolors,1,fp);
              for(i=0; i<localnoofcolors; i++)
              {  gif_palette[i].red   >>=  2;
                 gif_palette[i].green >>=  2;
                 gif_palette[i].blue  >>=  2;
              }
         }
         fread(&byte,1,1,fp);            // LZW minimum code size(bits)
         startsize = byte + 1;
         clearcode = 1 << byte;
         kxfac=(int)(g.tif_xsize*64);    // integer to speed up multiplication
                                         // so  xfac*k  ==  ((kxfac*k)>>6).

         kyfac=(int)(g.tif_ysize*64);    // yfac=size in y dimension, 1=full size
                                         // integer to speed up multiplication
                                         // so  yfac*k  ==  ((kyfac*k)>>6).

         if(frame==0)
         {    if(newimage(basefilename(filename),g.tif_xoffset+image_descriptor.x1+1, 
                   g.tif_yoffset + image_descriptor.y1,
                   (int)(0.99999 + image_descriptor.width*g.tif_xsize),
                   (int)(0.99999 + image_descriptor.height*g.tif_ysize), 
                   bpp, INDEXED, 1, 
                   g.want_shell, 1, PERM, 1, g.window_border, 0)!=OK) 
                   {  status=NOMEM; break; }
         }else
         {
             add_frame(ci);         
         }
         bytesperstrip = new int[1];
         strip = new int[1];

         //// Some pathological multi-frame GIFs change x and y sizes
         //// from one frame to the next. 
         
         if(image_descriptor.width  > z[ci].xsize ||
            image_descriptor.height > z[ci].ysize)
                    resize_image(ci, max(image_descriptor.width, z[ci].xsize),
                    max(image_descriptor.height, z[ci].ysize));

         status = readLZWencodedfile(fp, frame, startsize, clearcode,
                    image_descriptor.width,
                    image_descriptor.height,
                    kxfac,kyfac,bpp,GIF,1,bytesperstrip,strip,yvalue);

         dx = image_descriptor.x1 - z[ci].xpos;
         dy = image_descriptor.y1 - z[ci].ypos;
         if(frame >1 && (dx || dy))
         {     z[ci].cf = frame;
               image_shift_frame(ci, dx, dy);
         }
         if(gotpalette==1) 
         {    memcpy(z[ci].palette, gif_palette, 768);
              memcpy(z[ci].opalette, gif_palette, 768);
              setpalette(gif_palette);
         }
         
         //// Fix oddities in multiframe GIFs
         
         if(frame >0)
         {     for(y=0; y<z[ci].ysize; y++)
               for(x=0; x<z[ci].xsize; x++)
               {    valuetoRGB(z[ci].image[frame][y][x], rr, gg, bb, 8);
                    if(rr==0 && gg==0 && bb==0 || 
                       x < image_descriptor.x1 ||
                       y < image_descriptor.y1 ||
                       x > image_descriptor.width + image_descriptor.x1 ||
                       y > image_descriptor.height + image_descriptor.y1)
                           z[ci].image[frame][y][x] = z[ci].image[frame-1][y][x];
               }
         }

         delete[] yvalue;
         delete[] bytesperstrip;
         delete[] strip;
         frame++;
      }
   }while(!done);
   if(frame>1)
   {    z[ci].animation = 1;
        if(delay) z[ci].fps = min(30, 100000.0/(double)delay);
   }
   fclose(fp);
   return status;
}


//--------------------------------------------------------------------------//
// readLZWencodedfile                                                       //
// read GIF or TIF LZW compressed file                                      //
//                                                                          //
// For TIF files, if `noofstrips' is > 1, it checks the array strips[] to   //
//     determine the location of the bytes. The arrays `bytesperstrip',     //
//     'yvalue', and `strip' must already be allocated.                     //
// Returns 0 if successful.                                                 //
// yvalue must be an array filled with the scan lines in the desired fill   //
//     order. Normally this is just a series of integers, except for inter- //
//     laced gif files.                                                     //
//--------------------------------------------------------------------------//
int readLZWencodedfile(FILE* fp, int frame, int startsize, int clearcode, 
    int imagewidth, int imagelength, int kxfac, int kyfac, int bpp,
    int filetype, int noofstrips, int *bytesperstrip, int *strip, 
    int *yvalue)
{
   const int MAXTABLE = 8192*250;
   const int ITEMSIZE = 8192;
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return(NOMEM); } 
   char tempstring[256];
   char temp2[256];
   int j,k,lenout=0, bppfac=1, done=0, yindex=0, onebitfac=1, pix,
       resetcount=0,code=0,hit=0,blocksize=0,
       oldcode=0,endcode,resetvalue,topcode=0,maxLZWvalue,
       resetstartsize=startsize,x,y,x2,y2,status=OK;
   int* ti;                           // index into table for each LZW code
   int* le;                           // lengths for each LZW code

   ti = new int[ITEMSIZE];
   if(ti==NULL){ message(g.nomemory,ERROR); return(NOMEM); }
   le = new int[ITEMSIZE];
   if(le==NULL){ message(g.nomemory,ERROR); free(ti); return(NOMEM); }
   uchar *outstring;
   outstring = new uchar[max(imagewidth+1,ITEMSIZE)];
   if(outstring==NULL){ message(g.nomemory,ERROR); free(le);free(ti); return(NOMEM); }

   int maxblocksize=265;
   char* block;

   if(noofstrips > 1)                 // For TIF, find size of biggest strip
   {  
       for(k=0;k<noofstrips;k++) 
          if(bytesperstrip[k]>maxblocksize) maxblocksize=bytesperstrip[k];
   }

   if(filetype==GIF) 
   {   block = new char[286];  
       block[285]=99;
   }else
   {   blocksize = maxblocksize+10;
       block = new char[blocksize];  
   }

   if(block==NULL)
   { message(g.nomemory,ERROR);
     delete[] ti;
     delete[] le;
     delete[] outstring;
     return(NOMEM); 
   }

   for(k=0;k<256;k++) block[k]=0;
   x=0;
   y=yvalue[yindex];
   getLZWbits(startsize,blocksize,block,ERASE,fp,noofstrips,bytesperstrip,
        strip);

   //// The LZW codes are packed sequentially. To get a code: 
   ////      ti[code] = starting position of code             
   ////      le[code] = length in bytes of the code           
   ////      table[ti[code]] to table[ti[code]+le[code]] is the code 
        
   uchar *table;    
   table=new uchar[MAXTABLE];
   table[MAXTABLE-1] =99;   // This value is constantly checked for overflow
   if(table==NULL)
   {   delete[] table;
       delete[] block;
       delete[] ti;
       delete[] le;
       delete[] outstring;
       message(g.nomemory,ERROR);
       return(NOMEM);
   }

   switch(bpp)
   {  case 1 : if(filetype==TIF){ bppfac=1; onebitfac=8; }
               if(filetype==GIF){ bpp=8; bppfac=1; onebitfac=1; }
               break; 
      case 8 : bppfac=1; break;
      case 15: bppfac=2; break;
      case 16: bppfac=2; break;
      case 24: bppfac=3; break;
      case 32: bppfac=4; break;
      default: message("Unknown pixel depth in LZW file",ERROR);
               return(ERROR);
   }

   totalbytes=0;

   endcode = clearcode + 1;
   topcode = clearcode + 1;
   resetvalue = topcode;
   maxLZWvalue = ( 1 << startsize ) - 1;

   for(k=0;k<=265;k++){ table[k]=k; ti[k]=k; le[k]=1; }
   outstring[0]=0;
   done = 0;
   resetcount = 0;
   do
   {
     if(filetype==GIF) 
     {  blocksize = getbyte(fp);
        blocksize &=0xff;
        fread(block+1,1,blocksize,fp);
     }else if(noofstrips<=1) 

     {  blocksize=256;
        fread(block,1,blocksize,fp);
     }

     do
     {               // blocksize is passed by ref & is sometimes changed
        code = getLZWbits(startsize, blocksize, block, filetype, fp,
                          noofstrips, bytesperstrip, strip);  
        if((code<0)||
          (code > topcode + 1))
        {  
            if(filetype==GIF) strcpy(temp2,"GIF");
            if(filetype==TIF) strcpy(temp2,"TIF");
            sprintf(tempstring,"Error decoding %s file at position %d",
              temp2, totalbytes);
            message(tempstring,ERROR);
            status=ERROR;
            done=1;
            goto endlzw;
        }
        if(code==clearcode)    
        {   
            topcode = resetvalue;              // Topcode starts at 257 the
            resetcount++;                      //   first time, but starts at
            if(resetcount == 1) resetvalue--;  //   256 on subsequent passes. 
            startsize = resetstartsize;    
            maxLZWvalue = (1 << startsize) - 1;
            lenout=0;
        }else if(code == endcode) 
        {  
            if(filetype==GIF) {  done=1; break; }
            if(filetype==TIF) 
            {   
                 // For TIF files, at the end of a line there is a end of
                 // information code. The remaining bits in the current byte
                 // are discarded. Then there is a clear code and you start
                 // over with a new table.
                 // The clear code is not always in the correct startsize, 
                 // so it is necessary to manually reset it to 9 bits.

                 getLZWbits(startsize,blocksize,block,RESET,fp,noofstrips,
                              bytesperstrip,strip);
                 startsize=9;
                 maxLZWvalue = (1 << startsize) - 1;
            }            
        }else 
        {  
           if(code <= topcode)    
           { 
               outstring[0]=0;            
               lenout = le[code];
               memcpy(outstring, &(table[ti[code]]), min(ITEMSIZE,lenout) );
               if(hit)                          // new entry
               {   topcode++;
                   if(topcode>4096)
                   {   if(filetype==GIF) message("Unable to read GIF file",ERROR);
                       if(filetype==TIF) message("Unable to read TIF file",ERROR);
                       status=ERROR;
                       goto endlzw;
                   }
                   le[topcode] = le[oldcode] + 1; 
                   ti[topcode] = ti[topcode-1] + le[topcode-1]; 
                   memcpy(&(table[ti[topcode]]), &(table[ti[oldcode]]), le[oldcode]);
                   table[ti[topcode] + le[topcode]-1] = table[ti[code]];
               }
           }
           else                                 // old entry
           {  
               memcpy(outstring, &table[ti[oldcode]], min(ITEMSIZE,le[oldcode]));
               if(le[oldcode]>ITEMSIZE) 
               {   // Make sure they don't know what to do
                   message("Code too big\nAsk a wizard to enlarge me!",ERROR);
                   break;
               }    
               outstring[le[oldcode]] = table[ti[oldcode]];
               lenout = le[oldcode] + 1;
               topcode++;
               le[topcode] = lenout;
               ti[topcode] = ti[topcode-1] + le[topcode-1];
               memcpy(&table[ti[topcode]], outstring, lenout);
           }
           oldcode = code;
           hit = 1;
           for(j=0;j<lenout;j++)
           { 
              if(kxfac==64){ x2=x; y2=y; }
              else
              {   x2 = (x*kxfac)>>6;   // safe for 8bpp images only
                  y2 = (y*kyfac)>>6;
              }
              if(bpp==1)
              {   for(k=0;k<8;k++)
                  {   if(outstring[j] & (1<<k)) pix=255; else pix=0; 
                      z[ci].image[frame][y2][x2*8-k] = pix;  
                  }
              }else
                  z[ci].image[frame][y2][x2] = outstring[j];  
              x++;
              if(x*onebitfac >= imagewidth*bppfac)
              {    x = 0; 
                   yindex++;
                   y=yvalue[yindex]; 
                   if(y>=imagelength)goto endlzw;
              }
              
           }

           //---TIF files use 1 less than the maximum number of codes,----//
           //---for some unfathomable reason.                         ----//
           
           if(filetype==GIF)
           {  if(topcode >= maxLZWvalue)     
              {   startsize++;
                  if(startsize >= 13) startsize=12;
                  maxLZWvalue = (1 << startsize) - 1;
              }
           } else
           if(filetype==TIF)
           {  if(topcode >=maxLZWvalue-1)     
              {   startsize++;
                  if(startsize >= 13) startsize=12;
                  maxLZWvalue = (1 << startsize) - 1;
              }
           }   
        }
        if(table[MAXTABLE-1]!=99){ status=ERROR; break;}
        if(keyhit()) if(getcharacter()==27){ done=1; break; }
     } while(1); 
   } while((!done) && (!feof(fp))); 
   
endlzw:
   if(filetype==GIF && block[285]!=99) message("Error-7:583",ERROR);
   delete[] outstring;
   delete[] le;
   delete[] ti;
   delete[] table;
   delete[] block;
   return(status);
}
 

//--------------------------------------------------------------------------//
// bits                                                                     //
// Returns an int containing a portion of a byte starting at bit# 'start'   //
// and including the next n bits. The result is right-shifted.              //
// The bits are numbered 7 to 0 from left to right. (For GIF file reader)   //
//--------------------------------------------------------------------------//
int bits(char byte, int start, int n)
{
   static int shift,mask;
   shift = start - n + 1;
   byte >>= shift;
   mask = ( 1 << n ) - 1;
   return(mask & byte);               // Mask off unwanted bits at left
}


//--------------------------------------------------------------------------//
// getLZWbits                                                               //
// Variation of getbits for reading GIF or TIF file.                        //
// Gets n bits from a file. n can be 0 to sizeof(int). Usually 9-12.        //
// Returns an int. The bits are right-shifted.                              //
// Works for 16 or 32-bit integers.                                         //
//                                                                          //
// Blockpos  = Position in array `block'                                    //
// Blocksize = The 1st char of each block in a GIF is the no. of bytes      //
//             that follow (0-255). Not allowed to exceed 255.              //
//             In TIF files, blocksize is kept constant at 255.             //
// Block     = char array of the data already read from disk (must be       //
//             previously allocated)                                        //
//                                                                          //
// For TIF files, if `noofstrips' is 1, it checks the array strips[] to     //
//     determine the location of the bytes. This means the no.of bytes in   //
//     a block can exceed the length of a scan line.                        //
// Keeps track of bits left over and bytes unread.                          //
//                                                                          //
// If mode==RESET                                                           //
//     Initializes `getLZWbits' static variables (must be called before     //
//     using).                                                              //
//                                                                          //
// If mode==GIF                                                             //
//  Assumes bits are packed in the following format (example for n=9) :     //
//                      7  6  5  4    3  2  1  0                            //
//                    ----------------------------                          //
//     Byte 1          a7 a6 a5 a4   a3 a2 a1 a0                            //
//     Byte 2          b6 b5 b4 b3   b2 b1 b0 a8                            //
//     Byte 3          c5 c4 c3 c2   c1 c0 b8 b7                            //
//     Byte 4          d4 c3 d2 d1   d0 c8 c7 c6                            //
//     ...                                                                  //
// If mode==TIF                                                             //
//  Assumes bits are packed in the following format (example for n=9) :     //
//                      7  6  5  4    3  2  1  0                            //
//                    ----------------------------                          //
//     Byte 1          a8 a7 a6 a5   a4 a3 a2 a1                            //
//     Byte 2          a0 b8 b7 b6   b5 b4 b3 b2                            //
//     Byte 3          b1 b0 c8 c7   c6 c5 c4 c3                            //
//     Byte 4          c2 c1 c0 d8   d7 d6 d5 d4                            //
//     ...                                                                  //
//--------------------------------------------------------------------------//
int getLZWbits(int n, int &blocksize, char* block, int mode, FILE* fp,
            int noofstrips, int* bytesperstrip, int* strip)
{
   static int hit=0;
   static uchar nextbyte=0;
   static int stripno =0;
   static int blockpos=0;
   static int bitsleft=0;
   static int bytesused=0;

   int newno=0,answer=0;
   int need = n;
   int ok=0;
   int n1 = n;                                 // No.of bits from 1st byte
   switch(mode)
   {  case ERASE:
        blockpos=0;
        nextbyte=0;
        bitsleft=0;
        bytesused=0;
        hit=0;
        stripno=0;
        break;
      case RESET:
        blockpos=0;
        bitsleft=0;
        break;
      case GIF:
        do
        { n1 = min(min(8, need), bitsleft);
          newno = bits(nextbyte, 7 + n1 - bitsleft, n1); // This also right-shifts newno
          newno <<= (n - need);
          answer += newno;
          bitsleft -= n1;
          need -= n1;
          if(!bitsleft)
          {   blockpos++;        
              nextbyte = block[blockpos];        // Get rest of bits from next byte
              totalbytes++;
              if(blockpos >= blocksize)
              {  blocksize=0;
                 blocksize = getbyte(fp);
                 blocksize &=0xff;
                 blockpos = 0;
                 if(blocksize<=0){ message("Invalid block size",ERROR);return(-1); }
                 fread(block+1,1,blocksize,fp);
              }
              bitsleft = 8;
          }
        }while(need);
        break;
      case TIF:
        do
        {  if(!hit)
           {  if(noofstrips>1)
              {    fseek(fp, g.tif_skipbytes+strip[stripno], SEEK_SET);
                   fread(block,1,bytesperstrip[stripno],fp);
              }
              bytesused=0;
              hit=1;
           }
                                            // Nextbyte is the byte being used
           n1 = min(min(8, need), bitsleft);// n1=No.of bits to take from byte
                                            // newno = Value obtained from the
                                            //   n1 bits (right-shifted by the
                                            //   function bits()).
           newno = bits(nextbyte, bitsleft-1, n1); 
           bitsleft -= n1;
           need -= n1;
           newno <<= need;
           answer += newno;
           if(!bitsleft)
           {   nextbyte = block[blockpos];   // Get rest of bits from next byte
               bytesused++;
               totalbytes++;
               blockpos++;        
               ok=0;
               if(noofstrips<=1)if(blockpos >= blocksize) ok=1;
               if(noofstrips> 1)if(bytesused >= bytesperstrip[stripno])ok=1;
               if(ok)
               {  blockpos = 0;
                  if(noofstrips>1)
                  {  stripno++;
                     fseek(fp, g.tif_skipbytes+strip[stripno], SEEK_SET);
                     fread(block,1,bytesperstrip[stripno],fp);
                     bytesused=0;
                     answer = 0x100;
                     break;
                  }else
                     fread(block,1,blocksize,fp);
               }
               bitsleft = 8;
           }
        } while(need);
        break;
   }
   return(answer);
}


//--------------------------------------------------------------------------//
// writegiffile                                                             //
// save image in GIF format                                                 //
//  GIF and 'Graphics Interchange Format' are trademarks (tm) of            //
//       Compuserve,  Incorporated, an H&R Block Company                    //
// Adapted from 'savegif.c' by Roger T. Stevens  12-31-91                   //
//--------------------------------------------------------------------------//
int writegiffile(char *filename, int write_all, int compress)
{      
 if(memorylessthan(16384)){  message(g.nomemory,ERROR); return(NOMEM); } 
  FILE* fp;
  uchar test[100];
  short int i, row, col, color, temp;
  ushort hashentry;
  uchar bits;
  int bpp,ino;
  short int x1,y1,x2,y2,xsize,ysize;
  uchar *buffer;
  RGB color_table[256];
  char block[266];  
  struct
  {    char name[3];
       char version[3];
       short int xres, yres;
       ushort packed;
       char back_col_index;
       char aspect_ratio;
  } gif_header;

  //-----------------------------------------------

  if(write_all==1)
  {   x1 = z[ci].xpos;
      y1 = z[ci].ypos;
      x2 = x1 + z[ci].xsize;
      y2 = y1 + z[ci].ysize;
  } else
  {   x1 = g.selected_ulx;
      x2 = g.selected_lrx;
      y1 = g.selected_uly;
      y2 = g.selected_lry;
  }
  xsize = x2-x1;
  ysize = y2-y1;
  
  buffer = new uchar[16384];
  if(buffer==NULL){ message(g.nomemory,ERROR); return(ABORT); }

  if((g.want_color_type==0)||(g.want_color_type>2))
  {  message("GIF files must be 8 bits/pixel!",ERROR); 
     delete[] buffer;
     return(ERROR); 
  }
  if((fp=open_file(filename, compress, TNI_WRITE, g.compression, g.decompression, 
     g.compression_ext))==NULL) 
  {
       delete[] buffer;
       return ERROR;
  }
 
  //---------------------------------------------------------
 
  strcpy(gif_header.name,"GIF");
  strcpy(gif_header.version, "87a");

  //// This must be the width and height of the image to accommodate
  //// brain-dead renderers such as Mozilla.

  gif_header.xres = (short int)xsize;
  gif_header.yres = (short int)ysize;
  gif_header.packed = 0xf7;
  bits = 8;
  colors = 256;
  gif_header.back_col_index = 0;
  gif_header.aspect_ratio = 0;

  fwrite(gif_header.name,3,1,fp);
  fwrite(gif_header.version,3,1,fp);
  putword(gif_header.xres,fp);
  putword(gif_header.yres,fp);
  putbyte(gif_header.packed,fp);
  putbyte(gif_header.back_col_index,fp);
  putbyte(gif_header.aspect_ratio,fp);
  
  setpalette(z[ci].palette);
  if (gif_header.packed == 0xF7)
  {  for (i=0;i<256;i++)
     {   color_table[i].red  = g.palette[i].red << 2;
         color_table[i].green= g.palette[i].green << 2;
         color_table[i].blue = g.palette[i].blue << 2;
     }
     fwrite(&color_table,1,768,fp);
  }
  putbyte(',',fp);
  putbyte(0,fp);
  putbyte(0,fp);
  putbyte(0,fp);
  putbyte(0,fp);
  putword(xsize,fp);
  putword(ysize,fp);
  putbyte(0,fp);
  startbits = bits+1;
  clear = 1 << (startbits - 1);
  gend = clear+1;
  putbyte(bits,fp);
  codebits = startbits;
  nbytes = 0;
  nbits = 0;

  for (i=0; i<266; i++) block[i] = 0;
  initializegif(buffer,block,fp);
  for(row=y1; row<y2; row++)
  {  for(col=x1; col<x2; col++)
     {  color = readpixelonimage(col,row,bpp,ino);
        convertpixel(color,bpp,8,1);          /* Convert to 8 bpp   */
        test[0] = ++length;
        test[length] = color;
        switch(length)
        {        case 1:
                    lastentry = color;
                    break;
                 case 2:
                    hashcode = 301 * (test[1]+1);
                 default:
                    hashcode *= (color + length);
                    hashentry = ++hashcode % 5003;
                    for(i=0; i<5003; i++)
                    {    hashentry = (hashentry + 1) % 5003;
                         if (memcmp(&buffer[str_index[hashentry]+2],
                             test,length+1) == 0)
                             break;
                         if (str_index[hashentry] == 0) i = 5003;
                    }
                    if (str_index[hashentry] != 0 && length < 97)
                    {   memcpy(&lastentry,&buffer[str_index[hashentry]],2);
                        break;
                    }
                    write_code(lastentry,block,fp);
                    entries++;
                    if (str_index[hashentry] == 0)
                    {   temp = entries + gend;
                        str_index[hashentry] = next;
                        memcpy(&buffer[next],&temp,2);
                        memcpy(&buffer[next+2],test,length+1);
                        next += length+3;
                        actual++;
                    }
                    test[0] = 1;
                    test[1] = color;
                    length = 1;
                    lastentry = color;
                    if ((entries + gend) == (1<<codebits)) codebits++;
                    if (entries + gend > 4093 || actual > 3335 || next > 15379)
                    {   write_code(lastentry,block,fp);
                        initializegif(buffer,block,fp);
                    }
             }
     }
  }
  write_code(lastentry,block,fp);
  write_code(gend,block,fp);
  putbyte(0,fp);
  putbyte(';',fp);
  close_file(fp, compress);  
  delete[] buffer;
  return OK;
}


//--------------------------------------------------------------------------//
// initializegif                                                            //
// initializes GIF file writing routine                                     //
//--------------------------------------------------------------------------//
void initializegif(uchar* buffer, char* block, FILE* fp)
{
       write_code(clear,block,fp);
       entries = 0;
       actual = 0;
       next = 1;
       length = 0;
       codebits = startbits;
       buffer[0] = 0;
       memset(str_index,0x00,10006);
}

//--------------------------------------------------------------------------//
// write_code                                                               //
// Writes GIF codes to disk file                                            //
//--------------------------------------------------------------------------//
void write_code(unsigned short int code, char* block, FILE* fp)
{
       block[nbytes  ] |= ((code << nbits) & 0xFF);
       block[nbytes+1] |= ((code >> (8 - nbits)) & 0xFF);
       block[nbytes+2] |= (((code>>(8 - nbits)) >> 8) & 0xFF);
       nbits += codebits;
       while (nbits >= 8)
       {      nbits -= 8;
              nbytes++;
       }
       if (nbytes < 251 && code != gend)
              return;
       if (code == gend)
       {  while (nbits > 0)
          {   nbits -= 8;
              nbytes++;
          }
       }
       putbyte(nbytes,fp);
       fwrite(block,nbytes,1,fp);
       memcpy(block,&block[nbytes],5);
       memset(&block[5],0x00,260);
       nbytes = 0;
}


//--------------------------------------------------------------------------//
// checktime                                                                //
// Primitive profiler  Usage: t=checktime(RESET) = gives current time       //
//                     then   profile[k]+=checktime(NORMAL) = accumulate    //
//                                time for a specific event                 //
//--------------------------------------------------------------------------//
double checktime(int mode)
{ 
    static timeval t;
#ifndef VMS
    struct timezone tz;  
#endif
    static double t1=0,t2=0,tt1=0,tt2=0;
    double dt;
    if(mode==RESET)
    {
#ifdef VMS
         gettimeofday(&t, NULL);
#else
         gettimeofday(&t, &tz);
#endif
         t1 = t.tv_sec;
         tt1 = (double)(t.tv_usec)/1000000.0;
         dt = t1 + tt1;
    }else
    {
#ifdef VMS
         gettimeofday(&t, NULL);
#else
         gettimeofday(&t, &tz);
#endif
	 t2 = t.tv_sec - t1;
         tt2 = (double)(t.tv_usec)/1000000.0 - tt1;
         dt = t2 + tt2;
    }
    return dt;
} 



//--------------------------------------------------------------------------//
// writegif89file                                                           //
// save image in GIF89a format                                              //
//  GIF and 'Graphics Interchange Format' are trademarks (tm) of            //
//       Compuserve,  Incorporated, an H&R Block Company                    //
//--------------------------------------------------------------------------//
int writegif89file(char *filename, int write_all, int compress)
{      
 if(memorylessthan(16384)){  message(g.nomemory,ERROR); return(NOMEM); } 
  FILE* fp;
  uchar test[100];
  uchar bits;
  uchar *buffer;
  short int f, i, row, col, color, temp, x1, y1, x2, y2, xsize, ysize;
  ushort hashentry, delay=0;
  int bpp, ino, frames;
  RGB color_table[256];
  char block[266];  
  struct
  {    char name[3];
       char version[3];
       short int xres, yres;
       ushort packed;
       char back_col_index;
       char aspect_ratio;
  } gif_header;

     //-----------------------------------------------

  if(write_all==1)
  {   x1 = z[ci].xpos;
      y1 = z[ci].ypos;
      x2 = x1 + z[ci].xsize;
      y2 = y1 + z[ci].ysize;
      frames = z[ci].frames;
  } else
  {   x1 = g.selected_ulx;
      x2 = g.selected_lrx;
      y1 = g.selected_uly;
      y2 = g.selected_lry;
      frames = 1;
  }
  xsize = x2-x1;
  ysize = y2-y1;
  
  buffer = new uchar[16384];
  if(buffer==NULL){ message(g.nomemory,ERROR); return ABORT; }

  if((g.want_color_type==0)||(g.want_color_type>2))
  {  message("GIF files must be 8 bits/pixel!",ERROR); 
     delete[] buffer;
     return ERROR; 
  }
  if((fp=open_file(filename, compress, TNI_WRITE, g.compression, g.decompression, 
       g.compression_ext))==NULL) 
  {  delete[] buffer;
     return ERROR;  // cant create file
  }
 
  //---------------------------------------------------------
 
  strcpy(gif_header.name,"GIF");
  strcpy(gif_header.version, "89a");
  gif_header.xres = 1+xsize;
  gif_header.yres = 1+ysize;
  gif_header.packed = 0xf7;
  bits = 8;
  colors = 256;
  gif_header.back_col_index = 0;
  gif_header.aspect_ratio = 0;

  if(z[ci].fps) delay=(ushort)(1000.0/z[ci].fps);

  fwrite(gif_header.name,3,1,fp);
  fwrite(gif_header.version,3,1,fp);

  ////  Screen descriptor
  putword(gif_header.xres,fp);
  putword(gif_header.yres,fp);
  putbyte(gif_header.packed,fp);
  putbyte(gif_header.back_col_index,fp);
  putbyte(gif_header.aspect_ratio,fp);
  

  ////  Global colour table
  setpalette(z[ci].palette);
  if (gif_header.packed == 0xF7)
  {  for (i=0;i<256;i++)
     {   color_table[i].red  = g.palette[i].red << 2;
         color_table[i].green= g.palette[i].green << 2;
         color_table[i].blue = g.palette[i].blue << 2;
     }
     fwrite(&color_table,1,768,fp);
  }

  ////  Application extension - needed to make image cycle
  ////  This is not documented anywhere.

  if(frames>1)
  {    putbyte(0x21,fp);
       putbyte(0xff,fp);
       putbyte(11,fp);
       putstring((char*)"NETSCAPE2.0",11,fp);
       putbyte(3,fp);
       putbyte(1,fp);
       putword(0,fp);  // no. of loops, 0=infinity
       putbyte(0,fp);
  }

  for(f=0; f<frames; f++)
  {    
       ////  Graphics control block
       ////  Delay between frames
       if(frames>1)
       {    putbyte(0x21,fp);
            putbyte(0xf9,fp);
            putbyte(4,fp);
            putbyte(0x09,fp);
            putword(delay/10,fp);
            putbyte(0xef,fp);
            putbyte(0,fp);
       }
       
       ////  Image descriptor
       putbyte(0x2c,fp);
       putword(0,fp);   // This would be position on the screen (x1,y1).
       putword(0,fp);   // If these values are not 0,0 it crashes Netscape.
       putword(xsize,fp);
       putword(ysize,fp);
       putbyte(0,fp);

       startbits = bits+1;
       clear = 1 << (startbits - 1);
       gend = clear+1;
       putbyte(bits,fp);
       codebits = startbits;
       nbytes = 0;
       nbits = 0;
       memset(block, 0, 266);  
       initializegif(buffer,block,fp);
       for(row=y1; row<y2; row++)
       {  for(col=x1; col<x2; col++)
          {  
             if(write_all)
             {   bpp = z[ci].bpp;
                 color = pixelat(z[ci].image[f][row-y1]+col-x1, bpp); 
                 convertpixel(color,bpp,8,1);          /* Convert to 8 bpp   */                
             }else
             {   color = readpixelonimage(col,row,bpp,ino);
                 convertpixel(color,bpp,8,1);          /* Convert to 8 bpp   */
             }
             test[0] = ++length;
             test[length] = color;
             switch(length)
             {   case 1:
                    lastentry = color;
                    break;
                 case 2:
                    hashcode = 301 * (test[1]+1);
                 default:
                    hashcode *= (color + length);
                    hashentry = ++hashcode % 5003;
                    for(i=0; i<5003; i++)
                    {    hashentry = (hashentry + 1) % 5003;
                         if (memcmp(&buffer[str_index[hashentry]+2],
                             test,length+1) == 0)
                             break;
                         if (str_index[hashentry] == 0) i = 5003;
                    }
                     if (str_index[hashentry] != 0 && length < 97)
                     {   memcpy(&lastentry,&buffer[str_index[hashentry]],2);
                         break;
                     }
                     write_code(lastentry,block,fp);
                     entries++;
                     if (str_index[hashentry] == 0)
                     {   temp = entries + gend;
                         str_index[hashentry] = next;
                         memcpy(&buffer[next],&temp,2);
                         memcpy(&buffer[next+2],test,length+1);
                         next += length+3;
                         actual++;
                     }
                     test[0] = 1;
                     test[1] = color;
                     length = 1;
                     lastentry = color;
                     if ((entries + gend) == (1<<codebits)) codebits++;
                     if (entries + gend > 4093 || actual > 3335 || next > 15379)
                     {   write_code(lastentry,block,fp);
                         initializegif(buffer,block,fp);
                     }
             }
          }
       }
       write_code(lastentry,block,fp);
       write_code(gend,block,fp);
       putbyte(0,fp);
  }
  putbyte(0x3b,fp);
  close_file(fp, compress);  
  delete[] buffer;
  return(OK);
}
