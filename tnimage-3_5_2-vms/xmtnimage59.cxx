//--------------------------------------------------------------------------//
// xmtnimage59.cc                                                           //
// Latest revision: 10-01-2002                                              //
// Copyright (C) 2000 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"
extern Globals     g;
extern Image      *z;
extern int         ci;
extern int **xbias, **ybias;
extern int vector_map_created;


//-------------------------------------------------------------------------//
// create_landmarks                                                        //
//-------------------------------------------------------------------------//
void create_landmarks(XYData *data, char *table, int &xsize, int &ysize)
{
  int dx,dy,i,j,k,n,ok0,ok1;
  double r,theta;
  n = max(data[0].n, data[1].n);
  ok0 = ok1 = 0;
  if(n>100)
  {
      if(message("Warning: correlating more than\n100 data points will take a long time\nContinue?", 
           YESNOQUESTION)!=YES) return;
  }
  if(n>16380){ message("Too many data points"); return;}

  //// Make sure there are non-zero data points
  for(k=0; k<n; k++)
  {   if(data[0].x[k] && data[0].y[k]) ok0++; 
      if(data[1].x[k] && data[1].y[k]) ok1++; 
  }
  if(!ok0){ message("Data set 1 is empty"); return; }
  if(!ok1){ message("Data set 2 is empty"); return; }
  printstatus(BUSY);

  array<double> dist(n,n,2);
  array<double> angle(n,n,2);
  array<int> closest(n,n,2);
  array<char> ltemp(256,n);
  int *x0 = data[0].x;
  int *y0 = data[0].y;
  int *x1 = data[1].x;
  int *y1 = data[1].y;
  int match[16384];
  for(k=0;k<16384;k++) match[k]=k;

  //// Create list 0 of reference distances from each point
  for(j=0;j<n;j++)
  for(i=0;i<n;i++)
  {    dx = x0[i] - x0[j];
       dy = y0[i] - y0[j];
       calc_distance(dx,dy,r,theta);
       dist.q[0][j][i] = r;
       if(theta < 0.0) theta += 2*PI;
       angle.q[0][j][i] = theta;
  }
  //// Create list 1 
  for(j=0;j<n;j++)
  for(i=0;i<n;i++)
  {    dx = x1[i] - x1[j];
       dy = y1[i] - y1[j];
       calc_distance(dx,dy,r,theta);
       dist.q[1][j][i] = r;
       angle.q[1][j][i] = theta;
  }
  //// Sort distances -> closest.q[image][point][order]
  for(j=0;j<n;j++)
  {    sort(dist.q[0][j], closest.q[0][j], n);
       sort(dist.q[1][j], closest.q[1][j], n);
  }

  //// Create map based on landmark points
  correlate_points(data, closest.q, match, n, angle.q, table, xsize, ysize); 
  printstatus(NORMAL);
}


//-------------------------------------------------------------------------//
// correlate_points                                                        //
//-------------------------------------------------------------------------//
void correlate_points(XYData *data, int ***closest, int *match, int n, 
     double ***angle, char *table, int xsize, int ysize)
{
  Widget www, scrollbar;
  int j,k0,k1,k2;
  int *x0 = data[0].x;  int *y0 = data[0].y;
  int *x1 = data[1].x;  int *y1 = data[1].y;
  if(n>10000){ message("Too many data points"); return;}
  array<int> xclosest(n+1, n+1);
  array<int> xclosest2(n+1, n+1);
  array<double> xdist(n+1, n+1);
  double *score, *minscore, *minscore2, medianscore, *statistic, m2;
  score = new double[n+1];
  minscore = new double[n+1];
  minscore2 = new double[n+1];
  statistic = new double[n+1];

  int *sortedscore, *match2;
  sortedscore = new int[n+1];
  match2 = new int[n+1];
  table[0] = 0;

  for(j=0; j<n; j++) match[j] = -1;
  for(j=0; j<n; j++) match2[j] = -1;
  for(j=0; j<n; j++) minscore[j] = 1e15;
  for(j=0; j<n; j++) minscore2[j] = 1e15;

  cross_sort(data, xdist.p, xclosest2.p);

  for(j=0; j<n; j++) xsize = max(xsize, max(x0[j], x1[j]));
  for(j=0; j<n; j++) ysize = max(ysize, max(y0[j], y1[j]));

  check_event();
  //// Main loop
  //// Pick each combination of 2 points  
  g.getout = 0;
  progress_bar_open(www, scrollbar);

  for(k0=0; k0<n; k0++)     // point in 0
  {
      check_event();
      if(g.getout){ g.getout=0; break; }
      if(x0[k0]<=0 || y0[k0]<=0) continue;
      progress_bar_update(scrollbar, k0*100/n);

      for(k1=0; k1<n; k1++)  // point in 1
      {   if(x1[k1]<=0 || y1[k1]<=0) continue;
          check_event();
          if(g.getout){ g.getout=0; break; }
          score[k0] = fit(data, k0, k1, closest, xdist.p, xclosest.p, angle); 
          if(score[k0] < minscore[k0])
          {    match[k0]=k1;  minscore[k0]=score[k0]; }
          if(score[k0] > minscore[k0] && score[k0] < minscore2[k0])
          {    match2[k0]=k1; minscore2[k0]=score[k0]; }
      }
      k1 = match[k0];
      k2 = match2[k0];
      check_event();
  }
 
  //// Process results
  sort(minscore, sortedscore, n);
  medianscore = minscore[sortedscore[n/2]];


  for(j=0; j<n; j++)
  {  m2 = minscore[j]/(max(0.000001, medianscore));
     m2 = m2/(m2+1.0);
     statistic[j] = 1 - m2*m2;
  }
  progress_bar_close(www);

  delete[] match2;
  delete[] sortedscore;
  delete[] statistic;
  delete[] minscore2;
  delete[] minscore;
  delete[] score;
  return;
}


//-------------------------------------------------------------------------//
// fit                                                                     //
// score & dx, dy for fit assuming point k0 of 0 is point k1 of 1          //
//-------------------------------------------------------------------------//
double fit(XYData *data, int k0, int k1, int ***closest, 
       double **xdist, int **xclosest, double ***angle)
{
  int j,j2,k,n,index0,index1,point0,point1,dx,dy;
  int *x0 = data[0].x;
  int *x1 = data[1].x;
  int *y0 = data[0].y;
  int *y1 = data[1].y;
  n = max(data[0].n, data[1].n);

  double *cdist, score=1.0, *adist, d2=1.0, a0, a1, ascore;
  int *x1a, *y1a, *distsort, *asort;

  cdist = new double[n+1];
  adist = new double[n+1];
  x1a = new int[n+1];
  y1a = new int[n+1];
  distsort = new int[n+1];
  asort = new int[n+1];
  for(k=0;k<n;k++) asort[k]=0;
 
  dx = x0[k0] - x1[k1];
  dy = y0[k0] - y1[k1];
  for(j=0;j<n;j++)          
  {   x1a[j] = x1[j] + dx;
      y1a[j] = y1[j] + dy;
  }
  data[1].x = x1a;
  data[1].y = y1a;

  cross_sort(data, xdist, xclosest);

  for(j=0;j<n;j++)          
  {  index1 = xclosest[j][0];  // index of closest point in 0 to point j in 1
     cdist[j] = xdist[index1][j];
  }
  sort(cdist, distsort, n);

  //// Only take the closest matches to eliminate stray points
  score = 0.0;
  for(j=0; j<min(n,6); j++) score += cdist[distsort[j]];       // 25/56

  d2 = distance(x0[k0],y0[k0],x1[k1],y1[k1]);
  if(d2 > 22500) score =1e9;  // maximum distance 150
  data[1].x = x1;
  data[1].y = y1;


  //// Recursively check neighboring angle patterns
  for(j=0; j<n; j++)             
  {  
       point0 = closest[0][k0][j];   
       point1 = closest[1][k1][j];   

       a0 = angle[0][k0][point0];
       a1 = angle[1][k1][point1];
       adist[j] = closeness(a0, a1);

       //// jth closest point to k0 and k1
       for(j2=0; j2<n; j2++)             
       {  
          index0 = closest[0][point0][j2];
          a0 = angle[0][point0][index0];

          index1 = closest[1][point1][j2];
          a1 = angle[1][point1][index1];
          adist[j] += closeness(a0, a1);      
       }
  }
  sort(adist, asort, n);

  //// Only take the closest matches to eliminate stray points
  ascore = 1.0;
  for(j=1; j<n; j++) 
      ascore += adist[asort[j]];  // don't include 0 here

  delete[] asort;
  delete[] distsort;
  delete[] y1a;
  delete[] x1a;
  delete[] adist;
  delete[] cdist;
  return sqrt(10*d2 + ascore * score);
}


//-------------------------------------------------------------------------//
// draw_vector                                                             //
//-------------------------------------------------------------------------//
void draw_vector(int x1, int y1, int x2, int y2, int color)
{
  double scale = 1.0;
  x1 = cint((double)x1*scale);
  x2 = cint((double)x2*scale);
  y1 = cint((double)y1*scale);
  y2 = cint((double)y2*scale);
  lineinfo li;
  li.ino=ci;
  li.type=0;
  li.color = color;
  li.width=1;
  li.wantgrad=0;
  li.gradr=0;
  li.gradg=0;
  li.gradb=0;
  li.gradi=0;
  li.skew=0;
  li.perp_ends=0;
  li.wantarrow=1;
  li.arrow_width=8.0;
  li.arrow_inner_length=6.0;
  li.arrow_outer_length=6.0;
  li.ruler_scale = 1.0;
  li.ruler_ticlength = 5.0;
  li.window=z[0].win;
  li.wantprint=0;
  li.antialias=0;
  line(x2, y2, x1, y1, SET, &li); 
  draw_arrow(x2, y2, x1, y1, &li); 
}


//-------------------------------------------------------------------------//
// cross_sort                                                              //
//-------------------------------------------------------------------------//
void cross_sort(XYData *data, double **xdist, int **xclosest)
{
  int *x0 = data[0].x;
  int *x1 = data[1].x;
  int *y0 = data[0].y;
  int *y1 = data[1].y;
  int n = max(data[0].n, data[1].n);
  int i,j;
  for(j=0;j<n;j++)
  for(i=0;i<n;i++)
       xdist[j][i] = distance(x1[j], y1[j], x0[i], y0[i]);
  //// Sort distances -> xclosest.p[point][order]
  //// qsort is 50% slower than this
  for(j=0;j<n;j++)  
       sort(xdist[j], xclosest[j], n);
}

//-------------------------------------------------------------------------//
// local_smooth                                                            //
//-------------------------------------------------------------------------//
void local_smooth(int **grid, int xsize, int ysize, int fsize)
{
  int x,y;
  int *temp;
  temp = new int[ysize+1];
  for(y=0; y<ysize; y++)
      smooth(grid[y], xsize, fsize);

  for(x=0; x<xsize; x++)
  {   for(y=0; y<ysize; y++) temp[y] = grid[y][x];
      smooth(temp, ysize, fsize);
      for(y=0; y<ysize; y++) grid[y][x] = temp[y];
  }
  delete[] temp;
}
void local_smooth(double **grid, int xsize, int ysize, int fsize)
{
  int x,y;
  double *dtemp;
  dtemp = new double[ysize+1];
  for(y=0; y<ysize; y++)
      smooth(grid[y], xsize, fsize);

  for(x=0; x<xsize; x++)
  {   for(y=0; y<ysize; y++) dtemp[y] = grid[y][x];
      smooth(dtemp, ysize, fsize);
      for(y=0; y<ysize; y++) grid[y][x] = dtemp[y];
  }
  delete[] dtemp;
}

//-------------------------------------------------------------------------//
// closeness                                                               //
//-------------------------------------------------------------------------//
double closeness(double a1, double a2)
{  
  double dtheta;
  dtheta = a1-a2;      
  return  dtheta*dtheta; 
}


//-------------------------------------------------------------------------//
// match_points                                                            //
//-------------------------------------------------------------------------//
void match_points(XYData *data, char *table, int **xbias, int **ybias,
     int *match, int ino)
{
  char tempstring[1024];
  int j,k0,k1,k4,n,xoff=0,yoff=0,x,y,xx,yy,smallest;
  n = max(data[0].n, data[1].n);
  double  *statistic, dist, mindist, *score, minscore;
  int  *hitmatch;
  statistic = new double[n+1];
  score = new double[n+1];
  hitmatch = new int[n+1];
 
  int *x0 = data[0].x;
  int *x1 = data[1].x;
  int *y0 = data[0].y;
  int *y1 = data[1].y;
  table[0] = 0;
  if(ino>0)
  {   xoff = z[ino].xpos;
      yoff = z[ino].ypos;
  }

  for(k0=0; k0<n; k0++)     // point in 0
  {  
      xx = data[0].x[k0];
      yy = data[0].y[k0];
      mindist = 1e6;
      for(k1=0; k1<n; k1++) // point in 1
      {    x = data[1].x[k1];
           y = data[1].y[k1];
           dist = distance(xx, yy, x+xbias[y][x], y+ybias[y][x]);
           if(dist<mindist){ mindist=dist; match[k0]=k1; score[k0]=dist; }
      }    
  }

  for(j=0; j<n; j++) statistic[j] = score[j];

  //// Get rid of duplicate matches if clear winner by factor of 2
  for(k1=0; k1<n; k1++) 
  {   minscore = 1e6;
      smallest = -1;
      for(k0=0; k0<n; k0++) 
      {    if(match[k0]==k1)
           {   if(score[k0]<minscore){ minscore=score[k0]; smallest=k0; }
           }
      }
      for(k0=0; k0<n; k0++) 
      {    if(match[k0]==k1 && minscore!=0.0)
           {   if(score[k0]/minscore > 2.0)
               {
                  // printf("Eliminating ref=%s <- %s\n", data[0].label[k0],  
                  //         data[1].label[k1]);
                   match[k0]=-1;
               }

           }
      }
  }           

  //// Draw landmark vectors
  for(k4=0; k4<data[4].n; k4++)       // landmark point
  {    if(!strcmp(data[4].label[k4],"-")) break;
       if(!strcmp(data[5].label[k4],"-")) break;
       if(!strlen(data[4].label[k4])) break;
       if(!strlen(data[5].label[k4])) break;
       draw_vector(data[5].x[k4], data[5].y[k4], data[4].x[k4], data[4].y[k4], 0);
  }

  //// Any discrepancies between coords in landmark & in data
  //// will show as red data points
  draw_spots(&data[4], RGBvalue(255,0,0,24));
  draw_spot_labels(&data[4], RGBvalue(255,0,0,24));
  draw_spots(data, 0);
  draw_spot_labels(data, 0);


  //// Build matching table - for 2 sets
  strcpy(table, "# Ref.\tRef.x\tRef.y\t\tUnk.\tUnk.x\tUnk.y\t\tXmin1\tYmin1\tXmax1\tYmax1\tXmin2\tYmin2\tXmax2\tYmax2\tScore\n");
  for(k0=0; k0<n; k0++)     // point in 0
  {   k1 = match[k0];
      if(k1>0 && x0[k0]>0 && x1[k1]>0 && y0[k0]>0 && y1[k1]>0)
      {  
           sprintf(tempstring, "%s\t%d\t%d\t\t %s\t%d\t%d\t\t %d\t%d\t%d\t%d\t %d\t%d\t%d\t%d\t%g\n", 
                   data[0].label[k0], data[0].x[k0], data[0].y[k0],
                   data[1].label[k1], data[1].x[k1], data[1].y[k1],
                   data[0].x1[k0], data[0].y1[k0],
                   data[0].x2[k0], data[0].y2[k0],
                   data[1].x1[k1], data[1].y1[k1],
                   data[1].x2[k1], data[1].y2[k1],
                   data[1].v[k1]);
          strcat(table, tempstring);
      }
  }

  //// Add unmatched points
  strcat(table, "# Unmatched reference points\n");
  for(k0=0; k0<n; k0++) 
  {   if(match[k0]<0 && x0[k0] && y0[k0])
      {   
           sprintf(tempstring, "%s\t%d\t%d\t\t %s\t%d\t%d\t\t %d\t%d\t%d\t%d\t %d\t%d\t%d\t%d\t%g\n", 
                   data[0].label[k0], data[0].x[k0], data[0].y[k0],
                   "None", 0, 0,
                   data[0].x1[k0], data[0].y1[k0],
                   data[0].x2[k0], data[0].y2[k0],
                   0, 0, 0, 0, 
                   0.0);
          strcat(table, tempstring);
      }
  }

  strcat(table, "# Unmatched unknown points\n");
  for(k0=0; k0<n; k0++) hitmatch[k0]=0;
  for(k0=0; k0<n; k0++) hitmatch[match[k0]]=1;
  for(k1=0; k1<n; k1++) 
  {   if(!hitmatch[k1] && x1[k1] && y1[k1])
      {  
           sprintf(tempstring, "%s\t%d\t%d\t\t %s\t%d\t%d\t\t %d\t%d\t%d\t%d\t %d\t%d\t%d\t%d\t%g\n", 
                   "None", 0, 0,
                   data[1].label[k1], data[1].x[k1], data[1].y[k1],
                   0, 0, 0, 0, 
                   data[1].x1[k1], data[1].y1[k1],
                   data[1].x2[k1], data[1].y2[k1],
                   0.0);
          strcat(table, tempstring);
      }
  }
  delete[] hitmatch;
  delete[] score;
  delete[] statistic;
  return;
}


//-------------------------------------------------------------------------//
// create_warped_spot_list                                                 //
//-------------------------------------------------------------------------//
void create_warped_spot_list(XYData *data, char *spotlist, int **xbias, 
     int **ybias, int *match, char *spot_filename, int xsize, int ysize)
{
  int k0,k1,x,xa,ya,y,xb,yb,x1,y1,x2,y2,xb1,yb1,xb2,yb2;
  char title[] = "#Warped spot list";
  char tempstring[1024];
  char matchlabel[128];
  int n0 = data[0].n;
  int n1 = data[1].n;
 
  if(!vector_map_created){ message("Create vector map first"); return; }
  if(n1==0){ message("No data"); return; }

  strcpy(spotlist, title);
  strcat(spotlist,"\n#Label\tnew x\tnew y\torig.x\torig.y\t\txmin\tymin\txmax\tymax\tIdentity\n");
  for(k1=0; k1<n1; k1++)
  {  
       if(!strcmp(data[1].label[k1],"-")) break;
       x = data[1].x[k1];
       y = data[1].y[k1];
       xa = max(1, min(x, xsize-1));
       ya = max(1, min(y, ysize-1));
       xb = x + xbias[ya][xa];
       yb = y + ybias[ya][xa];

       x1 = data[1].x1[k1];
       y1 = data[1].y1[k1];
       xa = max(1, min(x1, xsize-1));
       ya = max(1, min(y1, ysize-1));
       xb1 = x1 + xbias[ya][xa];
       yb1 = y1 + ybias[ya][xa];

       x2 = data[1].x2[k1];
       y2 = data[1].y2[k1];
       xa = max(1, min(x2, xsize-1));
       ya = max(1, min(y2, ysize-1));
       xb2 = x2 + xbias[ya][xa];
       yb2 = y2 + ybias[ya][xa];

       matchlabel[0] = 0;
       for(k0=0; k0<n0; k0++)
           if(k1 == match[k0]){ strcpy(matchlabel, data[0].label[k0]); break; }

       sprintf(tempstring, "%s\t%d\t%d\t%g\t%g\t\t%d\t%d\t%d\t%d\t%s\n", 
               data[1].label[k1],
               xb, yb, 
               data[1].u[k1], data[1].v[k1],
               xb1, yb1, xb2, yb2,
               matchlabel);
       strcat(spotlist, tempstring);
  }  
  edit(title, title, spotlist, 80, 80, 715, 500, EDITSIZE, 71, 0, 
       spot_filename, null, NULL);
}


//-------------------------------------------------------------------------//
// create_unwarped_spot_list                                               //
//-------------------------------------------------------------------------//
void create_unwarped_spot_list(XYData *data, char *spotlist, int **xbias, 
     int **ybias, int *match, char *spot_filename, int xsize, int ysize)
{
  xbias=xbias; ybias=ybias; xsize=xsize; ysize=ysize;
  int k0,k1,x,y,x1,y1,x2,y2;
  char title[] = "#Unwarped spot list";
  char tempstring[1024];
  char matchlabel[128];
  int n0 = data[0].n;
  int n1 = data[1].n;
  double u,v;
 
  if(!vector_map_created){ message("Create vector map first"); return; }
  if(n1==0){ message("No data"); return; }

  strcpy(spotlist, title);
  strcat(spotlist,"\n#Label\torig.x\torig.y\tSize\tSignal\t\txmin\tymin\txmax\tymax\tIdentity\n");
 
  for(k1=0; k1<n1; k1++)
  {  
       if(!strcmp(data[1].label[k1],"-")) break;
       x = data[1].x[k1];
       y = data[1].y[k1];
       x1 = data[1].x1[k1];
       y1 = data[1].y1[k1];
       x2 = data[1].x2[k1];
       y2 = data[1].y2[k1];
       u = data->u[k1]; // size
       v = data->v[k1]; // signal

       matchlabel[0] = 0;
       for(k0=0; k0<n0; k0++)
           if(k1 == match[k0]){ strcpy(matchlabel, data[0].label[k0]); break; }

       sprintf(tempstring, "%s\t%d\t%d\t%g\t%g\t\t%d\t%d\t%d\t%d\t%s\n", 
               data[1].label[k1],
               x, y, u, v, x1, y1, x2, y2,
               matchlabel);
       strcat(spotlist, tempstring);
  }  
  edit(title, title, spotlist, 80, 80, 715, 500, EDITSIZE, 71, 0, 
       spot_filename, null, NULL);
}


//-------------------------------------------------------------------------//
// show_data_vectors                                                       //
//-------------------------------------------------------------------------//
void show_data_vectors(XYData *data, int *match)
{
  int k0, k1, xoff=0, yoff=0;
  int ino = data[0].ino;
  int *x0 = data[0].x;
  int *x1 = data[1].x;
  int *y0 = data[0].y;
  int *y1 = data[1].y;
  int n = data[0].n;
  if(ino>0)
  {   xoff = z[ino].xpos;
      yoff = z[ino].ypos;
  }else{ message("Missing vector map image"); return; }
  if(!vector_map_created){ message("Create vector map first"); return; }
  if(n==0){ message("No data"); return; }
  for(k0=0; k0<n; k0++)     // point in 0
   {   k1 = match[k0];
       if(k1>0 && x0[k0]>0 && x1[k1]>0 && y0[k0]>0 && y1[k1]>0)
            draw_vector(x1[k1]+xoff, y1[k1]+yoff, x0[k0]+xoff, y0[k0]+yoff, 0);
   }  
}
