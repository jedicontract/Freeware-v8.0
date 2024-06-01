//--------------------------------------------------------------------------//
// xmtnimage42.cc                                                           //
// Latest revision: 06-11-2005                                              //
// byte order related stuff                                                 //
// Copyright (C) 2005 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;

//--------------------------------------------------------------------------// 
// RGBvalue                                                                 //
// Combines RGB components into a 1,2,3,or 4 byte pixel value.              //
// The RGB components are assumed to be between 0 and the max-              //
// imum value allowed for each bits/pixel.                                  //
// Bits/pixel is a parameter in case we need to convert.                    //
//--------------------------------------------------------------------------//
uint RGBvalue(int red, int green, int blue, int bpp)
{
   uint answer=0;
   int d, k, dmin = 12288;

   switch(bpp)
   {         
       case  7: answer = (red & 0x07)  + ((green & 0x07) << 3) +
                        ((blue & 0x03) << 6);
                break;
       case  8: //// Monochrome: 
                //// answer = 4*(uint)(red*luminr + green*luming + blue*luminb);
                //// Color:
                //// Primitive distance calculation to find nearest palette entry
                for(k=255;k>=0;k--)
                {   
                    d = (red   - g.palette[k].red)  *(red   - g.palette[k].red  ) +
                        (green - g.palette[k].green)*(green - g.palette[k].green) +
                        (blue  - g.palette[k].blue) *(blue  - g.palette[k].blue );
                    if(d<dmin){ dmin=d; answer=k; }
                    if(d==0) break;
                }
                break;
       case 15: answer = ((red & 0x1f) << 10) + ((green & 0x1f) << 5) +
                         (blue & 0x1f);
                break;    
       case 16: answer = ((red & 0x1f) << 11) + ((green & 0x3f) << 5) +
                         (blue & 0x1f);
                break;    
       case 24:
       case 32:
       case 48:
                answer =  (red<<16) + (green<<8) + (blue);
   }   
   return(answer);

}


//--------------------------------------------------------------------------//
// valuetoRGB - split a 1,2,3,or 4 byte pixel value into RGB components.    //
//  Bits/pixel is a parameter in case we need to convert.                   //
//  The palette for the image must have been copied into `palette' if       //
//     the image is 8 bpp, in order to get the right colors.                //
//  If no.of colors=1, the  pixel is interpreted as a bpp bits/pixel        //
//     grayscale and r,g,b are calculated as follows (example for 8bpp):    //
//                                                                          //
//     value   r   g   b     Formulas:                                      //
//     -----  --- --- ---                                                   //
//       0     0   0   0      b=(pixel+db)*bfac = (pixel+db) >> bshift      //
//       1     0   0   1      db = bfac*3-1  bfac=maxblue/maxcolor          //
//       2     0   1   1      bshift = bpp - bluebpp[bpp]                   //
//       3     1   0   1                                                    //
//       4     1   1   1      r=(pixel+dr)*rfac = (pixel+dr) >> rshift      //
//       5     1   1   2      dr = rfac-1   rfac=maxred/maxcolor            //
//       6     1   2   2      rshift = bpp - redbpp[bpp]                    //
//       7     2   1   2                                                    //
//       8     2   2   2      g=pixel*gfac = pixel >> gshift                //
//                            gfac=maxgreen/maxcolor                        //
//             ...            if(!(pixel*4*gfac)&1) then increment g        //
//      255   63  63  63      The pixel is scaled so that each increment    //
//                             of g is 1. If this is even, add 1 to g.      //
//                            gshift = bpp - greenbpp[bpp]                  //
//                                                                          //
//--------------------------------------------------------------------------//
void valuetoRGB(uint pix, int &rr, int &gg, int &bb, int bpp)
{
   switch(bpp)
   {  case 7:    //// For that abomination called "8 bpp true color mode"
                 //// (bbgggrrr)
          bb = (pix & 0xc0) >> 6;
          gg = (pix & 0x38) >> 3;
          rr = (pix & 0x07) ;
          break;
      case 8:
          if(pix>255)pix=255;
          {    bb = g.palette[pix].blue;
               gg = g.palette[pix].green;
               rr = g.palette[pix].red;
          }
          break;
      case 15:
          bb =  pix & 0x001f;
          gg = (pix & 0x03e0) >> 5;
          rr = (pix & 0x7c00) >>10;
          break;
      case 16:
          bb =  pix & 0x001f;
          gg = (pix & 0x07e0) >> 5;
          rr = (pix & 0xf800) >>11;
          break;
      case 24:
          bb =  pix & 0x0000ff;
          gg = (pix & 0x00ff00) >> 8;
          rr = (pix & 0xff0000) >>16;
          break;
      case 32:   
          bb =  pix & 0x0000ff;
          gg = (pix & 0x00ff00) >> 8;
          rr = (pix & 0xff0000) >>16;
          break;
      case 48:   
          bb =  pix & 0x0000ff;
          gg = (pix & 0x00ff00) >> 8;
          rr = (pix & 0xff0000) >>16;
          break;
   }   
}


//--------------------------------------------------------------------------//
//  image_valuetoRGB                                                        //
//--------------------------------------------------------------------------//
void image_valuetoRGB(uint pix, int &rr, int &gg, int &bb, int bpp, int ino)
{
   if(bpp==8 && between(ino, 0, g.image_count-1))
   {      if(pix>255)pix=255;
          {    bb = z[ino].palette[pix].blue;
               gg = z[ino].palette[pix].green;
               rr = z[ino].palette[pix].red;
          }
   }else valuetoRGB(pix,rr,gg,bb,bpp);
}


//--------------------------------------------------------------------------//
// pixelat - returns the pixel value that starts at the specified memory    //
//   location, accounting for specified bits/pixel.                         //
//   (for accessing pixels in arrays).                                      //
//   48 bpp is down-converted to 24.                                        //
//   Doesn't check the address. It will crash if address is wrong.          //
// Put bytes in correct order for the x server. Different servers expect    //
//  pixel bytes to be arranged in different orders.                         //
//--------------------------------------------------------------------------//
uint pixelat(uchar* p, int bpp)
{
  if(g.ximage_byte_order == LSBFirst)
  {    switch(bpp)
       {   case 7:   return ( *p );
           case 8:   return ( *p );
           case 15:  return ( 0x7fff & (*(p+1)<<8) + *p );
           case 16:  return ( (*(p+1)<<8) + *p );
           case 24:
           case 32:  return ( (*(p+2)<<16) + (*(p+1)<<8) + *p );
           case 48:  return ( (*(p+4)<<16) + (*(p+2)<<8) + *p );
       }
  }else
  {    switch(bpp)
       {   case 7:   return ( *p );
           case 8:   return ( *p );
           case 15:  return ( 0x7fff & (*(p+1)<<8) + *p );
           case 16:  return ( (*(p)<<8) + *(p+1) );
           case 24:  return ( (*(p+0)<<16) + (*(p+1)<<8) + *(p+2) ); //ok
           case 32:  return ( (*(p+1)<<16) + (*(p+2)<<8) + *(p+3) );
           case 48:  return ( (*(p+1)<<16) + (*(p+3)<<8) + *(p+5) );
       }
  } 
  return 0;
}


//--------------------------------------------------------------------------//
// RGBat - combine pixelat and valuetoRGB                                   //
//--------------------------------------------------------------------------//
void RGBat(uchar* p, int bpp, int &rr, int &gg, int &bb)
{   
    int value;
    switch(bpp)
    {     case 48:
                  rr = *(p+1)*256 + *(p+0);
                  gg = *(p+3)*256 + *(p+2);
                  bb = *(p+5)*256 + *(p+4);
                  break;
          default:
                  value = pixelat(p, bpp);
                  valuetoRGB(value, rr, gg, bb, bpp);
                  break;                     
    }
}


//--------------------------------------------------------------------------//
// putRGBbytes                                                              //
//--------------------------------------------------------------------------//
void putRGBbytes(uchar *address, int rr, int gg, int bb, int bpp)
{
  int value;
  switch(bpp)
  {  
      case 48:
              *(address + 0) = rr & 0xff;
              *(address + 1) = rr >> 8;
              *(address + 2) = gg & 0xff;
              *(address + 3) = gg >> 8;
              *(address + 4) = bb & 0xff;
              *(address + 5) = bb >> 8;
              break;     
      default:
              value = RGBvalue(rr,gg,bb,bpp);
              putpixelbytes(address, value, 1, bpp, 1);
              break;
    }
}


//--------------------------------------------------------------------------//
// putpixelbytes  (see also 'putpixelbytesfar()' )                          //
// puts pixels rapidly into an image buffer.                                //
// `Repetitions' specifies the no. of repetitions of the same pixel.        //
//  if omitted, it uses 1. Must be less than 65536.                         //
// No address checking is performed. If address is outside of a malloc'd    //
//  area, it will cause a crash.                                            //
// bpp is bitsperpixel of desired result. If omitted, it uses current       //
//  screen bits/pixel.                                                      //
// putpixelbytes() has an extra parameter which permits putting the         //
//  bytes in reverse order (This is used in writing TIF files).             //
//  If direction==1 (the default), it is possible to select one or more     //
//  rgb color planes by setting wantr, wantg, and wantb to 0 or 1.          //
// Put bytes in correct order for the x server. Different servers expect    //
//  pixel bytes to be arranged in different orders.                         //
//--------------------------------------------------------------------------//
void putpixelbytes(uchar *address, uint color, int repetitions,
     int bpp, int direction)
{  
     static int k;
     static ushort *add16;
     static uchar byte0, byte1;
 
      //---If all bytes are the same, use memset if possible---//
              
     if(bpp==0) bpp=g.bitsperpixel;
     if((repetitions>1)&&(bpp==8)) 
         memset((void*)address,color,repetitions);             
     else if((repetitions>1) && (bpp!=8) && ((color==g.maxcolor)||(color==0)))
         memset((void*)address,color & 0xff,repetitions*g.off[bpp]);             
     else if(direction==1)  
     {   switch(bpp)
         {   case 7: 
             case 8: 
                 for(k=0;k<repetitions;k++) *(address+k) = color;
                 break;
             case 15:
                 if(g.wantr && g.wantg && g.wantb)
                 {  add16 = (ushort *)address;
                    for(k=0;k<repetitions;k++) *(add16+k)=color;
                 } 
                 else
                 {  
                    byte0 = *(address+0);
                    byte1 = *(address+1);
                    if(g.wantr)
                    { byte1 &= 0x87;
                      byte1 |= ((color & 0x7800) >> 8);
                    }  
                    if(g.wantg)
                    { byte1 &= 0x78;
                      byte1 |= ((color & 0x8700) >> 8);

                      byte0 &= 0x1f;
                      byte0 |= (color & 0x00e0);
                    }  
                    if(g.wantb)
                    { byte0 &= 0xe0;
                      byte0 |= (color & 0x001f);
                    }  
                    if(g.ximage_byte_order == LSBFirst)
                    {   *(address+0) = byte0;
                        *(address+1) = byte1;
                    }else
                    {   *(address+1) = byte0;
                        *(address+0) = byte1;
                    }
                 }                 
                 break;
             case 16:
                 for(k=0;k<repetitions*2;k+=2)
                 {
                    byte0 = *(address+0+k);
                    byte1 = *(address+1+k);

                    /*  This displays wrong in vnc for 16bit grayscale images
                    if(g.swap_red_blue)   ////  BBBBBGGG GGRRRRR
		    {
		       if(g.wantr)
                       {    byte0 &= 0xc0;
                            byte0 |= (color >> 10);
                       }
                       if(g.wantg)
                       {    byte1 &= 0xf8;
                            byte1 |= ((color & 0x0700) >> 8);
                            byte0 &= 0x3f;
                            byte0 |= (color & 0x00c0);
                       }  
                       if(g.wantb)
                       {    byte1 &= 0x07;
                            byte1 |= ((color & 0x00f8) << 3);
                       }  
                    }
		    else                ////  RRRRRGGG GGGBBBBB
                    */ 
		    {
		       if(g.wantr)
                       {    byte1 &= 0x07;
                            byte1 |= ((color & 0xf800) >> 8);
                       }
                       if(g.wantg)
                       {    byte1 &= 0xf8;
                            byte1 |= ((color & 0x0700) >> 8);
      
                            byte0 &= 0x1f;
                            byte0 |= (color & 0x00e0);
                       }  
                       if(g.wantb)
                       {    byte0 &= 0xe0;
                            byte0 |= (color & 0x001f);
                       }  
                    }
                    if(g.ximage_byte_order == LSBFirst)
                    {    *(address+0+k) = byte0;
                         *(address+1+k) = byte1;
                    }else
                    {    *(address+1+k) = byte0;
                         *(address+0+k) = byte1;
                    }
                 }                 
                 break;
             case 24:
                 if(g.swap_red_blue)
                 {
                   //// Used to be 0 1 2 now is 3 2 1 to match Sun sparc 24 bit display
                   for(k=0;k<repetitions*3;k+=3)
                   {  if(g.wantr) *(address+k+3) = (color & 0xff0000) >> 16;
                      if(g.wantg) *(address+k+2) = (color & 0x00ff00) >>  8;
                      if(g.wantb) *(address+k+1) =  color & 0x0000ff;
                   }
                 }else if(g.ximage_byte_order == LSBFirst)
                 {
                   for(k=0;k<repetitions*3;k+=3)
                   {  if(g.wantr) *(address+k+2) = (color & 0xff0000) >> 16;
                      if(g.wantg) *(address+k+1) = (color & 0x00ff00) >>  8;
                      if(g.wantb) *(address+k+0) =  color & 0x0000ff;
                   }
                 }else
                 {
                   for(k=0;k<repetitions*3;k+=3)
                   {  if(g.wantr) *(address+k+2) = (color & 0xff0000) >> 16;
                      if(g.wantg) *(address+k+1) = (color & 0x00ff00) >>  8;
                      if(g.wantb) *(address+k+0) =  color & 0x0000ff;
                   }
                 }
                 break;
             case 32:
                // add32 = (uint *)address;
                // for(k=0;k<repetitions;k++) *(add32+k)=color;  
                 if(g.swap_red_blue)
                 for(k=0;k<repetitions*4;k+=4)
                 {  *(address+k+3) = (color & 0xff000000) >> 24;
                    //// Used to be 0 1 2 now is 3 2 1 to match Sun sparc 24 bit display
                    if(g.wantr) *(address+k+3) = (color & 0x00ff0000) >> 16;
                    if(g.wantg) *(address+k+2) = (color & 0x0000ff00) >>  8;
                    if(g.wantb) *(address+k+1) =  color & 0x000000ff;
                 }else if(g.ximage_byte_order == LSBFirst)
                 for(k=0;k<repetitions*4;k+=4)
                 {  *(address+k+3) = (color & 0xff000000) >> 24;
                    if(g.wantr) *(address+k+2) = (color & 0x00ff0000) >> 16;
                    if(g.wantg) *(address+k+1) = (color & 0x0000ff00) >>  8;
                    if(g.wantb) *(address+k+0) =  color & 0x000000ff;
                 }else
                 for(k=0;k<repetitions*4;k+=4)
                 {  *(address+k+3) = (color & 0xff000000) >> 24;
                    if(g.wantr) *(address+k+1) = (color & 0x00ff0000) >> 16;
                    if(g.wantg) *(address+k+2) = (color & 0x0000ff00) >>  8;
                    if(g.wantb) *(address+k+3) =  color & 0x000000ff;
                 }
                 break;
             case 48:
                 if(g.ximage_byte_order == LSBFirst)
                 for(k=0;k<repetitions*6;k+=6)
                 {  if(g.wantr){ *(address+k+5) = 0;
                                 *(address+k+4) = (color & 0xff0000) >> 16; }
                    if(g.wantg){ *(address+k+3) = 0;
                                 *(address+k+2) = (color & 0x00ff00) >> 8; }
                    if(g.wantb){ *(address+k+1) = 0;
                                 *(address+k+0) = color & 0x0000ff; }
                 }else
                 for(k=0;k<repetitions*6;k+=6)
                 {  if(g.wantr){ *(address+k+0) = 0;
                                 *(address+k+1) = (color & 0xff0000) >> 16; }
                    if(g.wantg){ *(address+k+2) = 0;
                                 *(address+k+3) = (color & 0x00ff00) >> 8; }
                    if(g.wantb){ *(address+k+4) = 0;
                                 *(address+k+5) = color & 0x0000ff; }
                 }  
                 break;
        }    
     }else  // direction == -1
     {   
         switch(bpp)
         {   case 7: 
             case 8: 
                 for(k=0;k<repetitions;k++) *(address+k) = color;
                 break;
             case 15:
             case 16:
                 for(k=0;k<(repetitions<<1);k+=2)
                 {  *(address+k+0) = (color & 0xff00) >> 8;
                    *(address+k+1) =  color & 0x00ff;
                 }
                 break;
             case 24:
                 for(k=0;k<repetitions*3;k+=3)
                 {  *(address+k+0) = (color & 0xff0000) >> 16;
                    *(address+k+1) = (color & 0x00ff00) >>  8;
                    *(address+k+2) =  color & 0x0000ff;

                 }
                 break;
             case 32:
                 for(k=0;k<repetitions*4;k+=4)
                 {  *(address+k+0) = (color & 0xff000000) >> 24;
                    *(address+k+1) = (color & 0x00ff0000) >> 16;
                    *(address+k+2) = (color & 0x0000ff00) >>  8;
                    *(address+k+3) =  color & 0x000000ff;
                 }
                 break;
             case 48:
                 for(k=0;k<repetitions*6;k+=6)
                 {  *(address+k+5) = 0;
                    *(address+k+4) = (color & 0xff0000) >> 16; 
                    *(address+k+3) = 0;
                    *(address+k+2) = (color & 0x00ff00) >> 8; 
                    *(address+k+1) = 0;
                    *(address+k+0) =  color & 0x0000ff; 
                 }
                 break;
         }    
     }
}
