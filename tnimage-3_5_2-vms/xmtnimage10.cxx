//--------------------------------------------------------------------------//
//  xmtnimage10.cc                                                          //
//  This module contains Xiao-Lin Wu's algorithm for "efficient color       //
//  quantization", adapted from the algorithm published in the article      //
//  "Efficient statistical computations for optimal color quantization"     //
//  (Graphics Gems Vol. II,  pp. 126-133 (Academic Press, 1991)).           //
// Latest revision: 08-09-2001                                              //
// Copyright (C) 2001 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"
extern Globals     g;

RGB temppal[256];

struct box 
{   int r0;                         /* min value, exclusive */
    int r1;                         /* max value, inclusive */
    int g0;  
    int g1;  
    int b0;  
    int b1;
    int vol;
};
typedef  struct {       /* structure for a cube in color space */
   int   lower;         /* one corner's index in histogram     */
   int   upper;         /* another corner's index in histogram */
   int   count;         /* cube's histogram count              */
   int   level;         /* cube's level                        */
   char  rmin,rmax;   
   char  gmin,gmax;   
   char  bmin,bmax;   
} cube_t;


//----------------Wu specific prototypes-----------------------------------//
void quantize(int bpp);
long int Top(struct box* cube,uchar dir, int pos, int ***mmt);
long int Bottom(struct box* cube,uchar dir, int ***mmt);
long int Vol(struct box* cube, int ***mmt);
void M3d(int ***vwt, int ***vmr, int ***vmg, int ***vmb, double ***m2);
void Hist3d(int ***vwt, int ***vmr, int ***vmg, int ***vmb, double ***m2);
double Var(struct box* cube, double ***m2, int ***mr, int ***mg, int ***mb, 
         int ***wt);
double Maximize(struct box* cube,uchar dir,int first,int last,int* cut,
         long int whole_r,long int whole_g,long int whole_b,long int whole_w,
         int ***mr, int ***mg, int ***mb, int ***wt);
int Cut(struct box* set1, struct box* set2, int ***mr, int ***mg, int ***mb, 
         int ***wt);
void Mark(struct box* cube, int label, uchar* tag);


#define MAXCOLOR   256
#define wuRED        2
#define wuGREEN      1
#define wuBLUE       0


uchar   *Ir, *Ig, *Ib;
int     size; /*image size*/
int     K;    /*color look-up table size*/
ushort *Qadd;

//--------------------------------------------------------------------------//
//           C Implementation of Wu's Color Quantizer (v. 2)                //
//           (see Graphics Gems vol. II, pp. 126-133)                       //
//                                                                          //
//Author:        Xiaolin Wu                                                 //
//               Dept. of Computer Science                                  //
//               Univ. of Western Ontario                                   //
//               London, Ontario N6A 5B7                                    //
//               wu@csd.uwo.ca                                              //
//                                                                          //
//Algorithm: Greedy orthogonal bipartition of RGB space for variance        //
//           minimization aided by inclusion-exclusion tricks.              //
//           For speed no nearest neighbor search is done. Slightly         //
//           better performance can be expected by more sophisticated       //
//           but more expensive versions.                                   //
//                                                                          //
//The author [X.L.W.] thanks Tom Lane at Tom_Lane@G.GP.CS.CMU.EDU for much  //
//of additional documentation and a cure to a previous bug.                 //
//                                                                          //
//Free to distribute, comments and suggestions are appreciated.             //
//                                                                          //
// wuquantize -                                                             //
// bpp is the original bits/pixel of the image to convert.                  //
// buf1 = input image buffer                                                //
// buf2 = output image buffer                                               //
// noofcolors = 1 for grayscale, 3 for RGB                                  //
// This algorithm doesn't work if there are <3 different colors in image.   //
//                                                                          //
// Returns error code (OK if successful).                                   //
//--------------------------------------------------------------------------//
int wuquantize(int bpp, uchar*** buf1, uchar*** buf2, int xsize, int ysize, 
    int frames, int &ncolors)
{
    if(memorylessthan(16384)){ message(g.nomemory,ERROR); return(NOMEM); }
    struct box  cube[MAXCOLOR];
    uchar       lut_r[MAXCOLOR], lut_g[MAXCOLOR], lut_b[MAXCOLOR];
    long int    weight;
    double      vv[MAXCOLOR], temp;
    int alloc1=0,alloc2=0,alloc3=0,alloc4=0,alloc5=0,alloc6=0,alloc7=0,
        alloc8=0,alloc9=0,alloc10=0,next,i,j,k,el=0,bfac=1,f,j2,rr,gg,bb,
        rshift=0, gshift=0, bshift=0;
    size = xsize * ysize + 2;
    char nowu[128];
    printstatus(REDUCE_COLORS);
    
    strcpy(nowu, g.nomemory);
    strcat(nowu,"\nto convert to 8 bits/pixel");
    int status=OK;
    uchar *tag = NULL;
    double *m2_1d = NULL;
    double  ***m2 = NULL;
    int *wt_1d = NULL, *mr_1d = NULL, *mg_1d = NULL, *mb_1d = NULL;
    int  ***wt = NULL,  ***mr = NULL,  ***mg = NULL,  ***mb = NULL;

    ////  Histogram is in elements 1..HISTSIZE along each axis,
    ////  element 0 is for base or marginal value
    ////  NB: these must start out 0!
 
    if(memorylessthan(20*33*33*33 + 5*(size+8)))
    {  status=NOMEM; 
       message(nowu,ERROR);
       goto wuend;
    }

    m2_1d = (double*)malloc(33*33*33*sizeof(double));    
    if(m2_1d==NULL){status=NOMEM; goto wuend;} else alloc1=1;
    m2 = make_3d_alias(m2_1d,33,33,33);

    wt_1d = (int*)malloc(33*33*33*sizeof(int));    
    if(wt_1d==NULL){status=NOMEM; goto wuend;} else alloc2=1;
    wt = make_3d_alias(wt_1d,33,33,33);

    mr_1d = (int*)malloc(33*33*33*sizeof(int));    
    if(mr_1d==NULL){status=NOMEM; goto wuend;} else alloc3=1;
    mr = make_3d_alias(mr_1d,33,33,33);

    mg_1d = (int*)malloc(33*33*33*sizeof(int));    
    if(mg_1d==NULL){status=NOMEM; goto wuend;}else alloc4=1;
    mg = make_3d_alias(mg_1d,33,33,33);

    mb_1d = (int*)malloc(33*33*33*sizeof(int));    
    if(mb_1d==NULL){status=NOMEM; goto wuend;}else alloc5=1;
    mb = make_3d_alias(mb_1d,33,33,33);

    Ir = new uchar[size+8]; if(Ir==NULL){status=NOMEM; goto wuend;}else alloc6=1;
    Ig = new uchar[size+8]; if(Ig==NULL){status=NOMEM; goto wuend;}else alloc7=1;
    Ib = new uchar[size+8]; if(Ib==NULL){status=NOMEM; goto wuend;}else alloc8=1;
    Qadd = new ushort[size+8]; 
    if(Qadd==NULL){status=NOMEM; goto wuend;}else alloc9=1;
    tag = new uchar[33*33*33];    
    if(tag==NULL){status=NOMEM; goto wuend;} else alloc10=1;  
    if(memorylessthan(16384)){status=NOMEM; goto wuend;}
    memset(tag,0,33*33*33);
    for(k=0;k<size;k++){ Ir[k]=0; Ig[k]=0; Ib[k]=0; Qadd[k]=0; }    

    for(k=0;k<MAXCOLOR;k++)
    {   cube[k].r0=0; cube[k].r1=0;
        cube[k].g0=0; cube[k].g1=0;
        cube[k].b0=0; cube[k].b1=0;
        cube[k].vol=0;
        lut_r[k]=0;
        lut_g[k]=0;
        lut_b[k]=0;
        vv[k]=0;
    }   
    
    for(i=0;i<33;i++)    // Initialize all arrays 
    for(j=0;j<33;j++) 
    for(k=0;k<33;k++) 
    {   m2[i][j][k] = 0;
        wt[i][j][k] = 0;
        mr[i][j][k] = 0;
        mg[i][j][k] = 0;
        mb[i][j][k] = 0;
    }

    switch(bpp)
    {   case 7:
        case 8:  bfac=1; rshift= 1; gshift= 1; bshift= 1; break;
        case 15: bfac=2; rshift= 0; gshift= 0; bshift= 0; break;
        case 16: bfac=2; rshift= 0; gshift= 1; bshift= 0; break;
        case 24: bfac=3; rshift= 3; gshift= 3; bshift= 3; break;
        case 32: bfac=4; rshift= 3; gshift= 3; bshift= 3; break;
        case 48: bfac=6; rshift= 11; gshift= 11; bshift= 11; break;
    }
    
    j2=0;
    for(f=0;f<frames;f++)
    for(j=0;j<ysize;j++)
    for(i=0;i<xsize;i++)
    {  
        RGBat(buf1[f][j]+i*bfac, bpp, rr,gg, bb);
        Ir[j2] = rr >> rshift; // Ir,Ig,Ib must be 0..32
        Ig[j2] = gg >> gshift;
        Ib[j2] = bb >> bshift;
        j2++;
    }

    //----------------------------------------------------------------------//
    // This is a temporary fix to add a slight variability into the image   //
    // until I can figure out why Wu's algorithm doesn't work for images    //
    // with 2 or less different colors. This only seems to happen in        //
    // 24 bpp mode.                                                         //
    //----------------------------------------------------------------------//

    if(bpp==24)
    {    if(Ir[0]>0) {  if(Ir[0]==Ir[1]) Ir[0]--; }
         if(Ir[0]<32){  if(Ir[0]==Ir[1]) Ir[0]++; }
    }

    //------start of wu quantization----------------------------------------//
    // input R,G,B components into Ir, Ig, Ib; set size to width*height 

    K = 256;                      // no. of final colors wanted

    Hist3d(wt, mr, mg, mb, m2);   //-----calculate histogram-----//
    M3d(wt, mr, mg, mb, m2);      //-----calculate moments-------//

    cube[0].r0 = 0;
    cube[0].g0 = 0;
    cube[0].b0 = 0;
    cube[0].r1 = 32;
    cube[0].g1 = 32;
    cube[0].b1 = 32;
    next = 0;

    for(i=1; i<K; ++i)
    {
        if(g.reserved[i])continue;
        if (Cut(&cube[next], &cube[i],mr,mg,mb,wt)) 
        {     /* volume test ensures we won't try to cut one-cell box */
             vv[next]=(cube[next].vol>1)?Var(&cube[next],m2,mr,mg,mb,wt):0.0;
             vv[i] = (cube[i].vol>1) ? Var(&cube[i],m2,mr,mg,mb,wt) : 0.0;
        }else
        {    vv[next] = 0.0;   /* don't try to split this box again */
             i--;              /* didn't create box i */
        }
        next = 0; 
        temp = vv[0];
        for(k=0; k<=i; ++k)
        {  
            if (vv[k] > temp) 
            {    temp = vv[k]; 
                 next = k;
            }
        }
        if (temp <= 0.0) 
        {   K = i+1;  
            if(K<32) fprintf(stderr, "Only got %d colors\n", K);
            break;
        }
    }  
    ncolors = K;

        //------Partition done--------//

    for(k=0; k<K; ++k)
    {   
        if(g.reserved[k])continue;
        Mark(&cube[k], k, tag);
        weight = Vol(&cube[k], wt);
        if(weight) 
        {   lut_r[k] = Vol(&cube[k], mr) / weight;
            lut_g[k] = Vol(&cube[k], mg) / weight;
            lut_b[k] = Vol(&cube[k], mb) / weight;
        }else
            lut_r[k] = lut_g[k] = lut_b[k] = 0;          
    }

    //-------------------------------------------------------------------//
    // output lut_r, lut_g, lut_b as color look-up table contents,       //
    // Qadd as the quantized image (array of table addresses).           //
    //--------end wu quantization----------------------------------------//

    for(k=0;k<K;k++)
    {
       if(g.reserved[k])continue;
       g.palette[k].red   = 2*lut_r[k];
       g.palette[k].green = 2*lut_g[k];
       g.palette[k].blue  = 2*lut_b[k];
       //// Preserve white point
       if(g.palette[k].red>32) g.palette[k].red++;
       if(g.palette[k].green>32) g.palette[k].green++;
       if(g.palette[k].blue>32) g.palette[k].blue++;
    }  
    setpalette(g.palette);

    for(f=0;f<frames;f++)
    for(j=0;j<ysize;j++)
    for(i=0;i<xsize;i++)
        buf2[f][j][i] =tag[Qadd[el++]];

wuend:
    if(status==NOMEM) message(nowu,ERROR); 
    if(alloc10) delete[] tag;   
    if(alloc9) delete[] Qadd;   
    if(alloc8) delete[] Ib;     
    if(alloc7) delete[] Ig;     
    if(alloc6) delete[] Ir;     

    if(alloc5){ free_3d(mb,33); free(mb_1d); }       
    if(alloc4){ free_3d(mg,33); free(mg_1d); } 
    if(alloc3){ free_3d(mr,33); free(mr_1d); } 
    if(alloc2){ free_3d(wt,33); free(wt_1d); } 
    if(alloc1){ free_3d(m2,33); free(m2_1d); } 
    return(status);
}


//--------------------------------------------------------------------------//
//  Histogram is in elements 1..HISTSIZE along each axis,                   //
//  element 0 is for base or marginal value                                 //
//  NB: these must start out 0!                                             //
//                                                                          //
//  build 3-D color histogram of counts, r/g/b, c^2                         //
//--------------------------------------------------------------------------//
void Hist3d(int ***vwt, int ***vmr, int ***vmg, int ***vmb, double ***m2) 
{
        int ind, r, g, b;
        int inr, ing, inb, table[256];
        long int i;
                
        for(i=0; i<256; ++i) table[i]=i*i;
        for(i=0; i<size; ++i)
        {   
            r = Ir[i]; 
            g = Ig[i]; 
            b = Ib[i];
            inr=r+1; 
            ing=g+1; 
            inb=b+1; 
            ind=(inr<<10)+(inr<<6)+inr+(ing<<5)+ing+inb;
            Qadd[i]=ind;              /*[inr][ing][inb]*/          
            ++vwt[inr][ing][inb];    
            vmr[inr][ing][inb] += r;
            vmg[inr][ing][inb] += g;
            vmb[inr][ing][inb] += b;
            m2[inr][ing][inb]  += (double)(table[r]+table[g]+table[b]);
        }
}

//--------------------------------------------------------------------------//
//  At conclusion of the histogram step, we can interpret                   //
//    wt[r][g][b] = sum over voxel of P(c)                                  //
//    mr[r][g][b] = sum over voxel of r*P(c)  ,  similarly for mg, mb       //
//    m2[r][g][b] = sum over voxel of c^2*P(c)                              //
//  Actually each of these should be divided by 'size' to give the usual    //
//  interpretation of P() as ranging from 0 to 1, but we needn't do that    //
//  here.                                                                   //
//                                                                          //
//  We now convert histogram into moments so that we can rapidly            //
//  calculate the sums of the above quantities over any desired box.        //
//                                                                          //
//  Compute cumulative moments.                                             //
//--------------------------------------------------------------------------//
void M3d(int ***vwt, int ***vmr, int ***vmg, int ***vmb, double ***m2) 
{
    uchar i, r, g, b;
    long int line, line_r, line_g, line_b,
         area[33], area_r[33], area_g[33], area_b[33];
    double line2, area2[33];

    for(r=1; r<=32; ++r)
    {   for(i=0; i<=32; ++i) 
        {    area2[i]=0;
             area[i]=0;
             area_r[i]=0;
             area_g[i]=0;
             area_b[i]=0;
        }     
        for(g=1; g<=32; ++g)
        {   line2 = 0;
            line = 0;
            line_r = 0;
            line_g = 0;
            line_b = 0;
            for(b=1; b<=32; ++b)
            {   line += vwt[r][g][b];
                line_r += vmr[r][g][b]; 
                line_g += vmg[r][g][b]; 
                line_b += vmb[r][g][b];
                line2 += m2[r][g][b];
                area[b] += line;
                area_r[b] += line_r;
                area_g[b] += line_g;
                area_b[b] += line_b;
                area2[b] += line2;
                vwt[r][g][b] = vwt[r-1][g][b] + area[b];
                vmr[r][g][b] = vmr[r-1][g][b] + area_r[b];
                vmg[r][g][b] = vmg[r-1][g][b] + area_g[b];
                vmb[r][g][b] = vmb[r-1][g][b] + area_b[b];
                m2[r][g][b] = m2[r-1][g][b] + area2[b];
            }
        }
    }
}

//--------------------------------------------------------------------------//
//  Compute sum over a box of any given statistic                           //
//--------------------------------------------------------------------------//
long int Vol(struct box* cube, int ***mmt) 
{
    return( mmt[cube->r1][cube->g1][cube->b1] 
           -mmt[cube->r1][cube->g1][cube->b0]
           -mmt[cube->r1][cube->g0][cube->b1]
           +mmt[cube->r1][cube->g0][cube->b0]
           -mmt[cube->r0][cube->g1][cube->b1]
           +mmt[cube->r0][cube->g1][cube->b0]
           +mmt[cube->r0][cube->g0][cube->b1]
           -mmt[cube->r0][cube->g0][cube->b0] );
}

//--------------------------------------------------------------------------//
//  The next two routines allow a slightly more efficient calculation       //
//  of Vol() for a proposed subbox of a given box.  The sum of Top()        //
//  and Bottom() is the Vol() of a subbox split in the given direction      //
//  and with the specified new upper bound.                                 //
//                                                                          //
//  Compute part of Vol(cube, mmt) that doesn't depend on r1, g1, or b1     //
//  (depending on dir)                                                      //
//--------------------------------------------------------------------------//
long int Bottom(struct box* cube,uchar dir, int ***mmt)
{
    switch(dir)
    {   case wuRED:
            return( -mmt[cube->r0][cube->g1][cube->b1]
                    +mmt[cube->r0][cube->g1][cube->b0]
                    +mmt[cube->r0][cube->g0][cube->b1]
                    -mmt[cube->r0][cube->g0][cube->b0] );
        case wuGREEN:
            return( -mmt[cube->r1][cube->g0][cube->b1]
                    +mmt[cube->r1][cube->g0][cube->b0]
                    +mmt[cube->r0][cube->g0][cube->b1]
                    -mmt[cube->r0][cube->g0][cube->b0] );
        case wuBLUE:
            return( -mmt[cube->r1][cube->g1][cube->b0]
                    +mmt[cube->r1][cube->g0][cube->b0]
                    +mmt[cube->r0][cube->g1][cube->b0]
                    -mmt[cube->r0][cube->g0][cube->b0] );
    }
    return 0;
}

//--------------------------------------------------------------------------//
//  Compute remainder of Vol(cube, mmt), substituting pos for               //
//  r1, g1, or b1 (depending on dir)                                        //
//--------------------------------------------------------------------------//
long int Top(struct box* cube,uchar dir, int pos, int ***mmt)
{
    switch(dir)
    {   case wuRED:
            return( mmt[pos][cube->g1][cube->b1] 
                   -mmt[pos][cube->g1][cube->b0]
                   -mmt[pos][cube->g0][cube->b1]
                   +mmt[pos][cube->g0][cube->b0] );
        case wuGREEN:
            return( mmt[cube->r1][pos][cube->b1] 
                   -mmt[cube->r1][pos][cube->b0]
                   -mmt[cube->r0][pos][cube->b1]
                   +mmt[cube->r0][pos][cube->b0] );
        case wuBLUE:
            return( mmt[cube->r1][cube->g1][pos]
                   -mmt[cube->r1][cube->g0][pos]
                   -mmt[cube->r0][cube->g1][pos]
                   +mmt[cube->r0][cube->g0][pos] );
    }
    return 0;
}

//--------------------------------------------------------------------------//
//  Compute the weighted variance of a box                                  //
//  NB: as with the raw statistics, this is really the variance * size      //
//--------------------------------------------------------------------------//
double Var(struct box* cube, double ***m2, int ***mr, int ***mg, int ***mb, 
    int ***wt)
{
    double dr, dg, db, xx;

    dr = Vol(cube, mr); 
    dg = Vol(cube, mg); 
    db = Vol(cube, mb);
    xx =  m2[cube->r1][cube->g1][cube->b1] 
         -m2[cube->r1][cube->g1][cube->b0]
         -m2[cube->r1][cube->g0][cube->b1]
         +m2[cube->r1][cube->g0][cube->b0]
         -m2[cube->r0][cube->g1][cube->b1]
         +m2[cube->r0][cube->g1][cube->b0]
         +m2[cube->r0][cube->g0][cube->b1]
         -m2[cube->r0][cube->g0][cube->b0];

    return( xx - (double)((double)(dr*dr+dg*dg+db*db)/(double)Vol(cube,wt)));    
}


//--------------------------------------------------------------------------//
//  We want to minimize the sum of the variances of two subboxes.           //
//  The sum(c^2) terms can be ignored since their sum over both subboxes    //
//  is the same (the sum for the whole box) no matter where we split.       //
//  The remaining terms have a minus sign in the variance formula,          //
//  so we drop the minus sign and MAXIMIZE the sum of the two terms.        //
//--------------------------------------------------------------------------//
double Maximize(struct box* cube,uchar dir,int first,int last,int* cut,
         long int whole_r,long int whole_g,long int whole_b,long int whole_w,
         int ***mr, int ***mg, int ***mb, int ***wt)
{
    long int half_r, half_g, half_b, half_w;
    long int base_r, base_g, base_b, base_w;
    int i;
    double temp, max;

    base_r = Bottom(cube, dir, mr);
    base_g = Bottom(cube, dir, mg);
    base_b = Bottom(cube, dir, mb);
    base_w = Bottom(cube, dir, wt);
    max = 0.0;
    *cut = -1;
    for(i=first; i<last; ++i)
    {   half_r = base_r + Top(cube, dir, i, mr);
        half_g = base_g + Top(cube, dir, i, mg);
        half_b = base_b + Top(cube, dir, i, mb);
        half_w = base_w + Top(cube, dir, i, wt);
        /* now half_x is sum over lower half of box, if split at i */
        if (half_w == 0) 
        {                       /* subbox could be empty of pixels! */
          continue;             /* never split into an empty box */
        } else
        temp = ((double)half_r*(double)half_r + (double)half_g*(double)half_g +
                (double)half_b*(double)half_b)/(double)half_w;

        half_r = whole_r - half_r;
        half_g = whole_g - half_g;
        half_b = whole_b - half_b;
        half_w = whole_w - half_w;
        if (half_w == 0) 
        {                       /* subbox could be empty of pixels! */
          continue;             /* never split into an empty box */
        } else
        temp += ((double)half_r*(double)half_r + (double)half_g*(double)half_g +
                 (double)half_b*(double)half_b)/(double)half_w;

            if (temp > max) {max=temp; *cut=i;}
    }
    return(max);
}

//--------------------------------------------------------------------------//
//  cut                                                                     //
//--------------------------------------------------------------------------//
int Cut(struct box* set1, struct box* set2, int ***mr, int ***mg, int ***mb, 
     int ***wt)
{
    uchar dir;
    int cutr, cutg, cutb;
    double maxr, maxg, maxb;
    long int whole_r, whole_g, whole_b, whole_w;

    whole_r = Vol(set1, mr);
    whole_g = Vol(set1, mg);
    whole_b = Vol(set1, mb);
    whole_w = Vol(set1, wt);

    maxr = Maximize(set1, wuRED, set1->r0+1, set1->r1, &cutr,
                    whole_r, whole_g, whole_b, whole_w, mr,mg,mb,wt);
    maxg = Maximize(set1, wuGREEN, set1->g0+1, set1->g1, &cutg,
                    whole_r, whole_g, whole_b, whole_w,mr,mg,mb,wt);
    maxb = Maximize(set1, wuBLUE, set1->b0+1, set1->b1, &cutb,
                    whole_r, whole_g, whole_b, whole_w,mr,mg,mb,wt);

    if( (maxr>=maxg)&&(maxr>=maxb) ) 
    {   dir = wuRED;
        if (cutr < 0) return 0; /* can't split the box */
    }
    else
    if( (maxg>=maxr)&&(maxg>=maxb) ) 
        dir = wuGREEN;
    else
        dir = wuBLUE; 

    set2->r1 = set1->r1;
    set2->g1 = set1->g1;
    set2->b1 = set1->b1;

    switch (dir)
    {   case wuRED:
            set2->r0 = cutr;
            set1->r1 = cutr;
            set2->g0 = set1->g0;
            set2->b0 = set1->b0;
            break;
        case wuGREEN:
            set2->g0 = cutg;
            set1->g1 = cutg;
            set2->r0 = set1->r0;
            set2->b0 = set1->b0;
            break;
        case wuBLUE:
            set2->b0 = cutb;
            set1->b1 = cutb;
            set2->r0 = set1->r0;
            set2->g0 = set1->g0;
            break;
    }
    set1->vol=(set1->r1-set1->r0)*(set1->g1-set1->g0)*(set1->b1-set1->b0);
    set2->vol=(set2->r1-set2->r0)*(set2->g1-set2->g0)*(set2->b1-set2->b0);
    return 1;
}

//--------------------------------------------------------------------------//
//  mark                                                                    //
//--------------------------------------------------------------------------//
void Mark(struct box* cube, int label, uchar* tag)
{
    int r, g, b;

    for(r=cube->r0+1; r<=cube->r1; ++r)
       for(g=cube->g0+1; g<=cube->g1; ++g)
          for(b=cube->b0+1; b<=cube->b1; ++b)
            tag[(r<<10) + (r<<6) + r + (g<<5) + g + b] = label;
}


//--------------------------------------------------------------------------//
// fitpalette                                                               //
// Alternative color reduction method - fits colors to their closest        //
// value in the specified palette 'pal'. Palette of image to be changed     //
// must be the current palette.                                             //
// Capable of recovering 64 different values for each color instead         //
// of only 32. Useful for converting images back to 8 bits that were        //
// originally 8 bits in the same palette without losing information.        //
// Returns 0 if successful, 1 if error.                                     //
//--------------------------------------------------------------------------//
int fitpalette(int bpp, uchar ***buf1, uchar ***buf2, RGB *pal, int xsize, 
    int ysize, int frames, int &ncolors)
{
     int f,i,j,k,rr,gg,bb,status=OK;
     int r1,g1,b1;
     int dist,odist;

     uchar map[64][64][64];
     if(map==NULL){ message(g.nomemory,ERROR); return(NOMEM); }
     for(i=0;i<64;i++) for(j=0;j<64;j++) for(k=0;k<64;k++) map[i][j][k]=0;
     double rfac,gfac,bfac;
     printstatus(REDUCE_COLORS);

     switch(bpp)            // Factor to convert r,g,b of pixel to range 0..63
     {   case 15: rfac=2.0;  gfac=2.0;  bfac=2.0;  break;
         case 16: rfac=2.0;  gfac=1.0;  bfac=2.0;  break;
         case 24: rfac=0.25; gfac=0.25; bfac=0.25; break;
         case 32: rfac=0.25; gfac=0.25; bfac=0.25; break;
         case 48: rfac=0.000961318; gfac=0.000961318; bfac=0.000961318; break;
         default: rfac=0.25; gfac=0.25; bfac=0.25; break;
     }
     for(f=0;f<frames;f++)
     for(j=0;j<ysize;j++)
     for(i=0;i<xsize;i++)
     { 
         odist = 12288;     // 64*64*3, the maximum possible distance
                            // This uses g.palette to convert to rgb
         RGBat(buf1[f][j]+i*g.off[bpp], bpp, rr, gg, bb);
         rr = (int)(rr*rfac);         // Convert pixel to range 0..63
         gg = (int)(gg*gfac);
         bb = (int)(bb*bfac);
         if(map[rr][gg][bb])
             buf2[f][j][i] = map[rr][gg][bb];
         else
         {   for(k=0;k<=255;k++)
             {  r1 = pal[k].red;
                g1 = pal[k].green;
                b1 = pal[k].blue;
                dist = (r1-rr)*(r1-rr) + (g1-gg)*(g1-gg) + (b1-bb)*(b1-bb);
                if(dist<odist)
                {   buf2[f][j][i] = k;
                    odist = dist;
                    map[rr][gg][bb] = k;
                }
                if(dist==0) break;
             }
         }
             
     } 
     ncolors = 256;
     return(status);
}


