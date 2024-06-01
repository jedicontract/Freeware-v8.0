//--------------------------------------------------------------------------//
// xmtnimage57.cc                                                           //
// Image registration                                                       //
// Latest revision: 05-23-2001                                              //
// Copyright (C) 2001 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"
#define POINT_MATCH_DELTAS 0   // 0=use raw angles 1=relative angles
                               // 1 works better for large no of points and
                               // is less sensitive to CHECK_POINTS
#define DATASETS 6             // 0=reference 1=unknown 
                               // 2=ref.landmarks backup 3=unknown landmarks backup 
                               // 4=ref.landmarks 5=unknown landmarks
#define SCALE 32

extern Globals     g;
extern Image      *z;
extern int         ci;
int in_registration = 0;
void add_test_data(XYData *data);
void add_test_data2(XYData *data);
void add_test_data3(XYData *data);
int **xbias=NULL, **ybias=NULL, *xbias_1d=NULL, *ybias_1d=NULL;
double **rbias=NULL, *rbias_1d=NULL;
int vector_map_created = 0;
int rotation_map_created = 0;
int warp_smoothing=1;
int rot_smoothing=1;
int smooth_fac[] = { 0,3,5,7,9,11,15,19,21 };

//--------------------------------------------------------------------------//
//  registration                                                            //
//--------------------------------------------------------------------------//
void registration(void)
{
  static Dialog *dialog;
  static XYData *data;
  static int hit = 0;
  int j, k, ino2;
  if(in_registration) return;
  dialog = new Dialog;
  if(dialog==NULL){ message(g.nomemory); return; }
  in_registration = 1;
  if(!hit)     //// Leave data allocated forever for possible reanalysis
  {   data = new XYData[DATASETS];                 //// Don't delete
      for(j=0; j<DATASETS; j++)
      {  
           data[j].x         = new int[RPOINTS];   //// Don't delete
           data[j].y         = new int[RPOINTS];   //// Don't delete
           data[j].x1        = new int[RPOINTS];   //// Don't delete
           data[j].y1        = new int[RPOINTS];   //// Don't delete
           data[j].x2        = new int[RPOINTS];   //// Don't delete
           data[j].y2        = new int[RPOINTS];   //// Don't delete
           data[j].u         = new double[RPOINTS];//// Don't delete
           data[j].v         = new double[RPOINTS];//// Don't delete
           data[j].title     = new char[FILENAMELENGTH];
           strcpy(data[j].title, "1.data");
           data[j].label     = new char*[RPOINTS]; //// Don't delete
           for(k=0; k<RPOINTS; k++)
           {   data[j].label[k] = new char[64];    //// Don't delete
               data[j].label[k][0] = 0;
           }

           if(data[j].v==NULL){ delete dialog; delete[] data; message(g.nomemory); return; }
           data[j].n         = 0;
           data[j].dims      = 2;
           if(ci==g.image_count-1) ino2=max(1, ci-1); else ino2=min(ci+1, g.image_count-1);
           if(j==1 || j==3)
              data[j].ino     = ci;
           else
              data[j].ino     = ino2;
           data[j].nmin      = 1;
           data[j].nmax      = RPOINTS;
           data[j].width     = 0;
           data[j].type      = 7;
           data[j].duration  = TEMP;
           data[j].wantpause = 0;
           data[j].wantmessage = 1;
           data[j].win       = 0; // Calculate automatically
      }
      hit = 1;
  } 
  dialog->ptr[4] = (void*)data;

  //// Dialog

  strcpy(dialog->title,"Image registration");
  strcpy(dialog->boxes[0],"Ref.image  Unknown image");
  strcpy(dialog->boxes[1],"Image no.");

  strcpy(dialog->boxes[2],"Landmarks");
  strcpy(dialog->boxes[3],"Obtain landmks");
  strcpy(dialog->boxes[4],"Transl.landmks");

  strcpy(dialog->boxes[5],"Data points");
  strcpy(dialog->boxes[6],"Read points"); 
  strcpy(dialog->boxes[7],"Save points"); 
  strcpy(dialog->boxes[8],"Restore points"); 

  strcpy(dialog->boxes[9],"Spot Matching");
  strcpy(dialog->boxes[10],"Read landmarks");
  strcpy(dialog->boxes[11],"Edit landmarks"); 
  strcpy(dialog->boxes[12],"Save landmarks");
  strcpy(dialog->boxes[13],"Read match table");
  strcpy(dialog->boxes[14],"Edit data/match table");
  strcpy(dialog->boxes[15],"Save match table");

  strcpy( dialog->boxes[16],"Operation");
  strcpy( dialog->boxes[17],"Correlate points");
  strcpy( dialog->boxes[18],"Calculate rotation map");
  sprintf(dialog->boxes[19],"Rotate image %d", data[1].ino);
  strcpy( dialog->boxes[20],"Smoothing");
  strcpy( dialog->boxes[21],"Calculate warp vector map");
  sprintf(dialog->boxes[22],"Warp image %d", data[1].ino);
  strcpy( dialog->boxes[23],"Show data vectors");
  strcpy( dialog->boxes[24],"Create unwarped spot list"); 
  strcpy( dialog->boxes[25],"Create warped spot list"); 

  dialog->boxtype[0]=LABEL;
  dialog->boxtype[1]=MULTIPUSHBUTTON; 
  dialog->boxtype[2]=LABEL;
  dialog->boxtype[3]=MULTIPUSHBUTTON; 
  dialog->boxtype[4]=MULTIPUSHBUTTON; 

  dialog->boxtype[5]=LABEL;
  dialog->boxtype[6]=MULTIPUSHBUTTON;
  dialog->boxtype[7]=MULTIPUSHBUTTON;
  dialog->boxtype[8]=MULTIPUSHBUTTON;

  dialog->boxtype[9]=LABEL;
  dialog->boxtype[10]=PUSHBUTTON;
  dialog->boxtype[11]=PUSHBUTTON;
  dialog->boxtype[12]=PUSHBUTTON;
  dialog->boxtype[13]=PUSHBUTTON;
  dialog->boxtype[14]=PUSHBUTTON;
  dialog->boxtype[15]=PUSHBUTTON;

  dialog->boxtype[16]=LABEL;
  dialog->boxtype[17]=PUSHBUTTON;
  dialog->boxtype[18]=PUSHBUTTON;
  dialog->boxtype[19]=PUSHBUTTON;
  dialog->boxtype[20]=MULTIPUSHBUTTON;
  dialog->boxtype[21]=PUSHBUTTON;
  dialog->boxtype[22]=PUSHBUTTON;
  dialog->boxtype[23]=PUSHBUTTON;
  dialog->boxtype[24]=PUSHBUTTON;
  dialog->boxtype[25]=PUSHBUTTON;

  dialog->radiono[0]=0;
  for(k=0; k<21; k++) dialog->wcount[k]=2;
  sprintf(dialog->answer[1][0], "Img %d", data[0].ino);
  sprintf(dialog->answer[1][1], "Img %d", data[1].ino);

  strcpy(dialog->answer[20][0], "Warp");
  strcpy(dialog->answer[20][1], "Rot.");
  
  //// dialog->answer, labels and data are set in registrationcheck()

  dialog->noofradios = 0;
  dialog->noofboxes  = 26;
  dialog->helptopic  = 71;  
  dialog->want_changecicb = 0;
  dialog->f1 = registrationcheck;
  dialog->f2 = null;
  dialog->f3 = null;
  dialog->f4 = registrationfinish;
  dialog->f5 = null;
  dialog->f6 = null;
  dialog->f7 = null;
  dialog->f8 = null;
  dialog->f9 = null;
  dialog->width = 225;
  dialog->height = 0; // calculate automatically
  strcpy(dialog->path,".");
  dialog->transient = 1;
  dialog->wantraise = 0;
  dialog->radiousexy = 0;
  dialog->boxusexy = 0;
  dialog->hit = 0;
  dialog->message[0] = 0;      
  dialog->busy = 0;
  dialogbox(dialog);
  return;
}


//--------------------------------------------------------------------------//
//  registrationcheck                                                       //
//--------------------------------------------------------------------------//
void registrationcheck(dialoginfo *a, int radio, int box, int boxbutton)
{
  static int hit=0;
  static int xsize=0,ysize=0;
  static char filename[FILENAMELENGTH]      = "1.match";
  static char land_filename[FILENAMELENGTH] = "1.landmarks";
  static char spot_filename[FILENAMELENGTH] = "1.spots";
  static char *landmarks;
  static int match[RPOINTS];
  box=box; boxbutton=boxbutton; radio=radio;
  int dimension=0;
  g.getout = 0;
  XYData *data = (XYData*)a->ptr[4];
  //// Obtain data
  if(boxbutton >=0) dimension = boxbutton;

  //// Leave allocated permanently in case of reanalysis
  if(!hit)  
  {    g.table = new char[EDITSIZE]; 
       if(g.table==NULL)    { message(g.nomemory, ERROR); return; }
       g.table[0] = 0; 
       landmarks = new char[EDITSIZE]; 
       if(landmarks==NULL)  { message(g.nomemory, ERROR); return; }
       landmarks[0] = 0;  
       if(g.spotlist==NULL) g.spotlist = new char[EDITSIZE]; 
       g.spotlist[0] = 0;  
       if(g.spotlist==NULL) { message(g.nomemory, ERROR); return; }
       hit = 1;
  }
  
  data[0].ino = atoi(4+a->answer[1][0]);
  data[1].ino = atoi(4+a->answer[1][1]);
  data[4].ino = data[0].ino;
  data[5].ino = data[1].ino;
  switch(box)
  {  
       case 0: registration_set_ino(a, 0, data[0].ino);    
               registration_set_ino(a, 1, data[1].ino);
               break;
       case 1: a->param[0] = dimension;
               getinteger("Image no.", &data[dimension].ino, 1, g.image_count-1,
               registration_set_ino_ok, null, a, 60);
               break;          
       // 2 is a label
       case 3: obtain_control_points(&data[4], dimension);
               rebuild_table(&data[4], landmarks); 
               backup_registration_points(data);
               break;
       case 4: translate_points(&data[dimension+4]); 
               rebuild_table(&data[4], landmarks); 
               break;
       // 5 is a label
       case 6: read_warp_data(&data[dimension], dimension); 
               rebuild_table(data, g.table); 
               break;
       case 7: save_warp_data(&data[dimension]); break;
       case 8: restore_registration_points(data); 
               rebuild_table(data, g.table); 
               rebuild_table(&data[4], landmarks); 
               break;
       // 9 is a label
       case 10: read_table(&data[4], landmarks, land_filename); 
                backup_registration_points(data);
                break;
       case 11: edit_table(&data[4], (char*)"Landmarks", landmarks, land_filename); break;
       case 12: save_table(&data[4], landmarks, land_filename); break;

       case 13: read_table(data, g.table, filename); break;
       case 14: edit_table(data, (char*)"Data", g.table, filename); break;
       case 15: save_table(data, g.table, filename); break;

       // 16 is a label
       case 17: 
                calculate_size(data, xsize, ysize);
                create_landmarks(&data[4], landmarks, xsize, ysize);   // correlate
                rebuild_table(&data[4], landmarks);
                break;
       case 18: calculate_size(data, xsize, ysize);
                create_rotation_map(data, g.table, match, xsize, ysize);// rot map
                break;
       case 19: registration_rotate(a, 1);                             // rotate
                rebuild_table(&data[4], landmarks);
                break;
       case 20: if(dimension==0) 
                      getinteger("Smoothing of vector map", &warp_smoothing,
                         1, 8, warpsmoothingcb, null, NULL, 71);
                else
                      getinteger("Smoothing of rotation map", &rot_smoothing,
                         1, 8, rotsmoothingcb, null, NULL, 71);
                break;
       case 21: calculate_size(data, xsize, ysize);
                create_vector_map(data, g.table, match, xsize, ysize); // vec map
                break;
       case 22: registration_warp(a, 1);                               // warp
                rebuild_table(&data[4], landmarks);
                break;  // Warp
       case 23: show_data_vectors(data, match);                        // show vec
                break;
       case 24: create_unwarped_spot_list(data, g.spotlist, xbias, ybias, match, spot_filename,
                   xsize, ysize); 
                break; 
       case 25: create_warped_spot_list(data, g.spotlist, xbias, ybias, match, spot_filename,
                   xsize, ysize); 
                break; 

   }
}


//-------------------------------------------------------------------------//
// warpsmoothingcb et al.                                                  //
//-------------------------------------------------------------------------//
void warpsmoothingcb(clickboxinfo *c){ warp_smoothing=c->answer; }
void rotsmoothingcb(clickboxinfo *c){ rot_smoothing=c->answer; }


//-------------------------------------------------------------------------//
// obtain_control_points                                                   //
//-------------------------------------------------------------------------//
void obtain_control_points(XYData *data, int dimension) 
{
  int k, count, bpp, ino;
  char tempstring[1024];
  if(data[0].ino == data[1].ino){ message("Reference and unknown image\nare the same", ERROR); return; }
  data[dimension].n = 0;
  bezier_curve_start(&data[dimension], NULL);
  if(g.getout){ bezier_curve_end(data); g.getout = 0; return; }
  g.block++;
  while(g.bezier_state==CURVE)
        XtAppProcessEvent(XtWidgetToApplicationContext(g.drawing_area),
        XtIMAll);
  g.block = max(0, g.block-1);

  g.inmenu++;
  for(k=0; k<data[dimension].n; k++)
      draw_cross(data[dimension].x[k], data[dimension].y[k], 192);
  g.inmenu--;
  sprintf(tempstring, "%d Data points obtained", data[dimension].n);
  message(tempstring);

  //// Check data points
  count = 0;
  for(k=0; k<data[dimension].n; k++)
  {   if(whichimage(data[dimension].x[k], data[dimension].y[k], bpp) !=
         data[dimension].ino) count++;
  }
  if(count)       
  {   sprintf(tempstring, "%d Data points on wrong image", count);
      message(tempstring, WARNING);
  }

  //// Convert to relative x,y
  for(k=0; k<data[dimension].n; k++)
  {   ino = data[dimension].ino;
      if(ino<=0) continue;
      data[dimension].x[k] -= z[ino].xpos;
      data[dimension].y[k] -= z[ino].ypos;
  }
}


//-------------------------------------------------------------------------//
// translate_points                                                        //
//-------------------------------------------------------------------------//
void translate_points(XYData *data) 
{
   int i,j;
   static int x[2][2];
   char **label, **headings, ***answer;
   answer = new char**[2];
   label = new char*[2];
   headings = new char*[2];
   for(i=0;i<2;i++)
   {    answer[i] = new char*[2];
        label[i] = new char[64];
        headings[i] = new char[64];
        for(j=0;j<2;j++)
        {   answer[i][j] = new char[64]; 
            sprintf(answer[i][j], "%d", x[i][j]); 
        }
   }
   strcpy(headings[0],"x");
   strcpy(headings[1],"y");
   strcpy(label[0],"Shift");
   getstrings((char*)"Enter shift coordinates", label, headings, answer, 2,1,64);

   x[0][0] = atoi(answer[0][0]); // ref
   x[1][0] = atoi(answer[1][0]); // unk

   for(j=0;j<data->n;j++)
   {   data->x[j] += x[0][0];
       data->y[j] += x[1][0];
   }

   for(i=0;i<2;i++)
   {   for(j=0;j<2;j++) delete[] answer[i][j];
       delete[] answer[i];
       delete[] label[i];
       delete[] headings[i];
   }
   delete[] label;
   delete[] headings;
}
 

//-------------------------------------------------------------------------//
// edit_table                                                              //
//-------------------------------------------------------------------------//
void edit_table(XYData *data, char *title, char *table, char *filename) 
{
  edit(title, title, table, 80, 80, 0, 500,
        EDITSIZE, 71, 0, filename, parse_table, (void*)&data[0]);
}


//-------------------------------------------------------------------------//
// read_table                                                              //
//-------------------------------------------------------------------------//
void read_table(XYData *data, char *table, char *filename) 
{
  data=data;
  FILE *fp;
  g.getout=0;
  memset(table, 0, EDITSIZE);
  message("File name:",filename,PROMPT,FILENAMELENGTH-1,54);
  if(g.getout || !strlen(filename)) return;
  if((fp=fopen(filename,"rb")) == NULL)
  {   error_message(filename, errno);
      chdir(g.currentdir); // Change back to original directory
      return;
  }
  fread(table, 1, EDITSIZE, fp);
  fclose(fp);
  parse_table(data, table); 
}


//-------------------------------------------------------------------------//
// save_table                                                              //
//-------------------------------------------------------------------------//
void save_table(XYData *data, char *table, char *filename) 
{
  static PromptStruct ps;
  int answer=YES;
  data=data;
  g.getout=0;
  answer = message("File name:",filename,PROMPT,FILENAMELENGTH-1,54);
  if(answer==CANCEL || g.getout) return;
  ps.filename = filename;
  ps.text = table;
  ps.f1 = save_table_part2;
  ps.f2 = null;
  check_overwrite_file(&ps);
}


//-------------------------------------------------------------------------//
// save_table_part2                                                        //
//-------------------------------------------------------------------------//
void save_table_part2(PromptStruct *ps)
{ 
  FILE *fp;
  char *table = ps->text;
  char *filename = ps->filename;
  if((fp=fopen(filename,"w")) == NULL)
  {   error_message(filename, errno);
      chdir(g.currentdir); // Change back to original directory
      return;
  }
  fputs(table, fp);
  fclose(fp);
  return;
}


//-------------------------------------------------------------------------//
// rebuild_table  data -> table                                            //
//-------------------------------------------------------------------------//
void rebuild_table(XYData *data, char *table) 
{
  int k;
  char tempstring[1024];
  if(data==NULL || table==NULL) return;
  int n = max(data[0].n, data[1].n);
  for(k=data[0].n; k<n; k++)
  {    strcpy(data[0].label[k], "-");
       data[0].x[k] = 0;
       data[0].y[k] = 0;
       data[0].v[k] = 0.0;
  }
  for(k=data[1].n; k<n; k++)
  {    strcpy(data[1].label[k], "-");
       data[1].x[k] = 0;
       data[1].y[k] = 0;
       data[1].v[k] = 0.0;
  }
  table[0] = 0;
  strcpy(table, "# Ref.\tRef.x\tRef.y\t\tUnk.\tUnk.x\tUnk.y\t\txmin1\tymin1\txmax1\tymax1\txmin2\tymin2\txmax2\tymax2\tScore\n");
  for(k=0; k<n; k++)  
  {    sprintf(tempstring, "%s\t%d\t%d\t\t %s\t%d\t%d\t\t %d\t%d\t%d\t%d\t %d\t%d\t%d\t%d\t%g\n", 
               data[0].label[k], data[0].x[k], data[0].y[k],
               data[1].label[k], data[1].x[k], data[1].y[k],
               data[0].x1[k], data[0].y1[k],
               data[0].x2[k], data[0].y2[k],
               data[1].x1[k], data[1].y1[k],
               data[1].x2[k], data[1].y2[k],
               data[1].v[k]);
       strcat(table, tempstring);
  }
}


//-------------------------------------------------------------------------//
// parse_table  table -> data (2 data sets)                                //
//-------------------------------------------------------------------------//
void parse_table(void *ptr, char *table) 
{
  XYData *data = (XYData*)ptr;
  int count=0, bad=0,r=0;
  char tempstring[1024], label[2][128];
  char *pos, *opos;
  double x[2],y[2],x1[2],y1[2],x2[2],y2[2],junk;
  if(data==NULL || table==NULL) return;
  pos = table;  
  opos = pos;
  pos = 1 + strchr(pos, '\n');
  while(strlen(pos) > 1) 
  { 
       strncpy(tempstring, opos, max(0,pos-opos));
       tempstring[max(0, pos-opos-1)] = 0; 
       if(!strlen(tempstring)) break;
       if(tempstring[0] != '#' && strlen(tempstring) > 5) 
       {   
             data[0].label[count][0] = 0;
             data[0].x[count]  = 0;
             data[0].y[count]  = 0;
             data[0].x1[count] = 0;
             data[0].y1[count] = 0;
             data[0].x2[count] = 0;
             data[0].y2[count] = 0;
             data[0].u[count] = 0.0;
             data[0].v[count] = 0.0;
             data[1].label[count][0] = 0;
             data[1].x[count]  = 0;
             data[1].y[count]  = 0;
             data[1].x1[count] = 0;
             data[1].y1[count] = 0;
             data[1].x2[count] = 0;
             data[1].y2[count] = 0;
             data[1].u[count] = 0.0;
             data[1].v[count] = 0.0;

             r=sscanf(tempstring, "%s\t%lg\t%lg\t\t %s\t%lg\t%lg\t\t %lg\t%lg\t%lg\t%lg\t %lg\t%lg\t%lg\t%lg\t %lg", 
                label[0], &x[0], &y[0],
                label[1], &x[1], &y[1],
                &x1[0], &y1[0],
                &x2[0], &y2[0],
                &x1[1], &y1[1],
                &x2[1], &y2[1],
                &junk);
     
             // Allow user to omit fields        
             if(r>=1) strcpy(data[0].label[count], label[0]);
             if(r>=2) data[0].x[count] = cint(x[0]);
             if(r>=3) data[0].y[count] = cint(y[0]);
             if(r>=4) strcpy(data[1].label[count], label[1]);
             if(r>=5) data[1].x[count] = cint(x[1]);
             if(r>=6) data[1].y[count] = cint(y[1]);
             if(r>=7) data[0].x1[count] = cint(x1[0]);
             if(r>=8) data[0].y1[count] = cint(y1[0]);
             if(r>=9) data[0].x2[count] = cint(x2[0]);
             if(r>=10)data[0].y2[count] = cint(y2[0]);
             if(r>=11) data[1].x1[count] = cint(x1[1]);
             if(r>=12) data[1].y1[count] = cint(y1[1]);
             if(r>=13) data[1].x2[count] = cint(x2[1]);
             if(r>=14) data[1].y2[count] = cint(y2[1]);
             
             if(!between(data[0].x[count], 0, 1000000) ||
                !between(data[0].y[count], 0, 1000000) ||
                !between(data[1].x[count], 0, 1000000) ||
                !between(data[1].y[count], 0, 1000000) ||
                !between(data[0].x1[count], 0, 1000000) ||
                !between(data[0].y1[count], 0, 1000000) ||
                !between(data[1].x1[count], 0, 1000000) ||
                !between(data[1].y1[count], 0, 1000000) ||
                !between(data[0].x2[count], 0, 1000000) ||
                !between(data[0].y2[count], 0, 1000000) ||
                !between(data[1].x2[count], 0, 1000000) ||
                !between(data[1].y2[count], 0, 1000000)) bad++;

             count++;
       }
       opos = pos;
       pos = strchr(pos, '\n');
       if((strlen(pos) <= 1) && opos != table+strlen(table)) pos = table+strlen(table);
       if(strlen(pos) <= 1) break;
       pos++;
  }
  if(bad)
  {    sprintf(tempstring, "%d bad data point(s)\nPossible stray text in data file", bad);
       message(tempstring, ERROR);
  }
  if(count < 1){ message("Table is empty"); return; }
  data[0].n = count;
  data[1].n = count;
}

//-------------------------------------------------------------------------//
// parse_spotdata  table -> data for spot lists (1 data set)               //
//-------------------------------------------------------------------------//
void parse_spotdata(XYData *data, char *table) 
{
  int count=0, bad=0,r;
  char tempstring[1024], label0[256], label2[256];
  double x,y,u,v,x1,y1,x2,y2;
  char *pos, *opos;
  if(data==NULL || table==NULL) return;
  pos = table;  
  opos = pos;
  pos = 1+strchr(pos, '\n');
  while(pos != NULL) 
  { 
       strncpy(tempstring, opos, max(0,pos-opos));
       tempstring[max(0,pos-opos-1)] = 0; 
       if(!strlen(tempstring)) break;
       if(tempstring[0] != '#' && strlen(tempstring) > 5) 
       {    
           label0[0] = 0;
           label2[0] = 0;
           r = sscanf(tempstring, "%s\t%lg\t%lg\t%lg\t%lg\t\t%lg\t%lg\t%lg\t%lg\t%s", 
               label0, &x, &y, 
               &u, &v, &x1, &y1, &x2, &y2, label2);

           // Allow user to omit fields        
           if(r>=1) strcpy(data->label[count], label0);
           if(r>=2) data->x[count] = cint(x);
           if(r>=3) data->y[count] = cint(y);
           if(r>=4) data->u[count] = u;
           if(r>=5) data->v[count] = v;
           if(r>=6) data->x1[count] = cint(x1);
           if(r>=7) data->y1[count] = cint(y1);
           if(r>=8) data->x2[count] = cint(x2);
           if(r>=9) data->y2[count] = cint(y2);
           if(r>=10) strcpy(data->label2[count], label2);
           else data->label2[count][0]=0;

            if(!between(data->x[count], 0, 1000000) ||
               !between(data->y[count], 0, 1000000) ||
               !between(data->x1[count], 0, 1000000) ||
               !between(data->y1[count], 0, 1000000) ||
               !between(data->x2[count], 0, 1000000) ||
               !between(data->y2[count], 0, 1000000)) bad++;

            count++;
       }
       opos = pos;
       pos = strchr(pos, '\n');
       if(pos <= table && opos != table+strlen(table)) pos = table+strlen(table);
       if(pos <= table) break;
       pos++;
  }
  if(bad)
  {    sprintf(tempstring, "%d bad data point(s)\nPossible stray text in data file", bad);
       message(tempstring, ERROR);
  }
  if(count < 1){ message("Spot list table is empty"); return; }
  data->n = count;
}


//-------------------------------------------------------------------------//
// backup_registration_points                                              //
//-------------------------------------------------------------------------//
void backup_registration_points(XYData *data)
{
  int k;
  for(k=0; k<data[4].n; k++)
  {    data[2].x[k] = data[4].x[k];
       data[2].y[k] = data[4].y[k];
       data[3].x[k] = data[5].x[k];
       data[3].y[k] = data[5].y[k];
  }
  data[2].n = data[4].n;
}


//-------------------------------------------------------------------------//
// restore_registration_points                                             //
//-------------------------------------------------------------------------//
void restore_registration_points(XYData *data)
{
  int k;
  for(k=0; k<data[2].n; k++)
  {    data[4].x[k] = data[2].x[k];
       data[4].y[k] = data[2].y[k];
       data[5].x[k] = data[3].x[k];
       data[5].y[k] = data[3].y[k];
  }
  data[4].n = data[2].n;
}               

//--------------------------------------------------------------------------//
//  registration_set_ino_ok                                                 //
//  This is needed to set a value while staying event-oriented.             //
//  (i.e. can't block in clickbox)                                          //
//--------------------------------------------------------------------------//
void registration_set_ino_ok(clickboxinfo *c)
{
  Dialog *a = (Dialog*) c->client_data;
  int dimension = a->param[0];   
  registration_set_ino(a, dimension, c->answer);
}


//--------------------------------------------------------------------------//
//  registration_set_ino                                                    //
//--------------------------------------------------------------------------//
void registration_set_ino(Dialog *a, int dim, int ino)
{
  int k;
  XYData *data = (XYData*)a->ptr[4];
  char tempstring[1024];
  data[dim].ino = ino;
  sprintf(a->answer[1][dim], "%d", ino);
  set_widget_value(a->boxwidget[1][dim], ino);

  //// Change all other labels that indicate ino
  for(k=1; k<9; k++)
  {    if(a->boxtype[k] == MULTIPUSHBUTTON)
       {    sprintf(a->answer[k][dim],"Img %d", ino);
            set_widget_label(a->boxwidget[k][dim], a->answer[k][dim]);
       }else if(dim==1 && strncmp(a->boxes[k], "Warp image #", 11) == SAME)
       {    strcpy(tempstring, a->boxes[k]+13);
            sprintf(a->boxes[k], "Warp image %d %s", ino, tempstring);
            set_widget_label(a->boxwidget[k][0], a->boxes[k]);
       }else if(dim==1 && strncmp(a->boxes[k], "Rotate image #", 13) == SAME)
       {    strcpy(tempstring, a->boxes[k]+15);
            sprintf(a->boxes[k], "Rotate image %d %s", ino, tempstring);
            set_widget_label(a->boxwidget[k][0], a->boxes[k]);
       }
  }
  sprintf(tempstring,"Rotate image %d", data[1].ino);
  set_widget_label(a->boxwidget[19][0], tempstring);

  sprintf(tempstring,"Warp image %d", data[1].ino);
  set_widget_label(a->boxwidget[22][0], tempstring);

}


//--------------------------------------------------------------------------//
//  registrationfinish                                                      //
//--------------------------------------------------------------------------//
void registrationfinish(dialoginfo *a)
{
  a=a;
  if(!in_registration) return;
  in_registration = 0;
}


//--------------------------------------------------------------------------//
//  read_aliased_pixel                                                      //
//  x,y are relative to upper left of image ino                             //
//--------------------------------------------------------------------------//
int read_aliased_pixel(double x, double y, int ino)
{
  int ix,iy,f,bpp,b;
  ix = cint(x);  iy = cint(y);
  if(!between(ix, 1, z[ino].xsize-2)) return 0;
  if(!between(iy, 1, z[ino].ysize-2)) return 0;
  f = z[ino].cf;
  bpp = z[ino].bpp;
  b = g.off[bpp];
  return pixelat(z[ino].image[f][iy]+b*ix, bpp);
}



//--------------------------------------------------------------------------//
//  read_anti_aliased_pixel                                                 //
//  x,y are relative to upper left of image ino                             //
//  calculates pixel value for double x,y by linear interpolation of        //
//  neighboring integers                                                    //
//      ---------                                                           //
//      | 1 | 2 |                                                           //
//      ---------                                                           //
//      | 3 | 4 |                                                           //
//      ---------                                                           //
//--------------------------------------------------------------------------//
int read_anti_aliased_pixel(double x, double y, int ino)
{
  int ix,iy,pix1,pix2,pix3,pix4,v,f,bpp,b,rr,gg,bb;
  int rr1,rr2,rr3,rr4,gg1,gg2,gg3,gg4,bb1,bb2,bb3,bb4;
  double dx, dy;
  ix = int(x);  iy = int(y);
  dx = x - (double)ix;  
  dy = y - (double)iy;
  if(!between(ix, 1, z[ino].xsize-2)) return 0;
  if(!between(iy, 1, z[ino].ysize-2)) return 0;
  f = z[ino].cf;
  bpp = z[ino].bpp;
  b = g.off[bpp];
  
  pix1 = pixelat(z[ino].image[f][iy]  +b*ix,     bpp);
  pix2 = pixelat(z[ino].image[f][iy]  +b*(ix+1), bpp);
  pix3 = pixelat(z[ino].image[f][iy+1]+b*ix,     bpp);
  pix4 = pixelat(z[ino].image[f][iy+1]+b*(ix+1), bpp);

  if(z[ino].colortype == GRAY)
  {   v = cint(
          pix1*(1-dx)*(1-dy) + 
          pix2*(  dx)*(1-dy) + 
          pix3*(1-dx)*(  dy) + 
          pix4*(  dx)*(  dy) );  
  }else
  {
      valuetoRGB(pix1, rr1, gg1, bb1, bpp);
      valuetoRGB(pix2, rr2, gg2, bb2, bpp);
      valuetoRGB(pix3, rr3, gg3, bb3, bpp);
      valuetoRGB(pix4, rr4, gg4, bb4, bpp);
      rr = cint(rr1*(1-dx)*(1-dy) + rr2*(dx)*(1-dy) + rr3*(1-dx)*(dy) + rr4*(dx)*(dy));  
      gg = cint(gg1*(1-dx)*(1-dy) + gg2*(dx)*(1-dy) + gg3*(1-dx)*(dy) + gg4*(dx)*(dy));  
      bb = cint(bb1*(1-dx)*(1-dy) + bb2*(dx)*(1-dy) + bb3*(1-dx)*(dy) + bb4*(dx)*(dy));  
      v = RGBvalue(rr,gg,bb,bpp);  
  }   
  return v;
}


//-------------------------------------------------------------------------//
//  anti_aliased_value                                                     //
//  sets opix to some value between opix and color where gatevalue is an   //
//   RGB composite or grayscale value depending on colortype.              //
//  gatevalue =   0 => 100% and opix =no change                            //
//  gatevalue = 255 => 100% npix 100% change                               //
//  opix = existing pixel npix = anti-aliased bitmap pixel                 //
//-------------------------------------------------------------------------//
int anti_aliased_value(int gatevalue, int opix, int npix, int colortype, int bpp)
{
   int value=0,rr,gg,bb,nrr,ngg,nbb,orr,ogg,obb;
   double ifrac, rfrac, gfrac, bfrac;
   switch(colortype)
   {  
       case GRAY:
        ifrac = (double)gatevalue / 255.0;
        value = cint((double)npix*ifrac + (double)opix*(1.0-ifrac));
        break;
      case INDEXED:
      case COLOR:
        valuetoRGB(gatevalue, nrr, ngg, nbb, g.bitsperpixel);
        valuetoRGB(opix,orr,ogg,obb,bpp);
        valuetoRGB(npix, rr, gg, bb, bpp);
        rfrac = (double)nrr / 255.0;
        rr = cint(rr*rfrac + orr*(1-rfrac));
        gfrac = (double)ngg / 255.0;
        gg = cint(gg*gfrac + ogg*(1-gfrac));
        bfrac = (double)nbb / 255.0;
        bb = cint(bb*bfrac + obb*(1-bfrac));
        value = RGBvalue(rr,gg,bb,bpp);
        break;
   }
   return value;
}



//-------------------------------------------------------------------------//
// read_warp_data                                                          //
//-------------------------------------------------------------------------//
void read_warp_data(XYData *data, int dim)
{
  char filename[FILENAMELENGTH]="";
  char tempstring[FILENAMELENGTH];
  char label[1024], spotsizestring[64]="";
  if(memorylessthan(4096)){  message(g.nomemory,ERROR); return; } 
  FILE *fp;
  double size, signal, x, y, x1, y1, x2, y2;
  int count, r, spotsize=0;
  g.getout=0;
  message("File name:", data[dim].title, PROMPT, FILENAMELENGTH-1, 54);
  if(g.getout) return;
  strcpy(filename, data[dim].title);
  if ((fp=fopen(filename,"rt")) == NULL)
  {   error_message(filename, errno);
      chdir(g.currentdir); // Change back to original directory
      return;
  }
  count = 0;
  
  while(!feof(fp) && count<RPOINTS)
  {   // End of size analysis, start of distribution; or a bad entry
      fgets(tempstring, 1000, fp);
      if(strlen(tempstring)<2) break; 
      if(tempstring[0] == '#') continue;

      data->label[count][0] = 0;
      data->x[count] = 0;
      data->y[count] = 0;
      data->u[count] = 0;
      data->v[count] = 0;
      data->x1[count] = 0;
      data->y1[count] = 0;
      data->x2[count] = 0;
      data->y2[count] = 0;
       
      r = sscanf(tempstring,"%s\t%lg\t%lg\t%lg\t%lg\t%lg\t%lg\t%lg\t%lg", 
           label, &x, &y, &size, &signal, &x1, &y1, &x2, &y2);

      // Allow user to omit fields        
      if(r>=1) strcpy(data->label[count], label);
      if(r>=2) data->x[count] = cint(x);
      if(r>=3) data->y[count] = cint(y);
      if(r>=4) data->u[count] = size;
      if(r>=5) data->v[count] = signal;
      if(r<6 && spotsize<=0) 
      {      getstring("Enter spot size", spotsizestring, 64, 54);
             spotsize = atoi(spotsizestring);
      }
      if(r>=6) data->x1[count] = cint(x1); else data->x1[count] = cint(x)-spotsize;
      if(r>=7) data->y1[count] = cint(y1); else data->y1[count] = cint(y)-spotsize;
      if(r>=8) data->x2[count] = cint(x2); else data->x2[count] = cint(x)+spotsize;
      if(r>=9) data->y2[count] = cint(y2); else data->y2[count] = cint(y)+spotsize;
      count++;
      if(feof(fp)) break;
  }
  data->n = count;
  fclose(fp);  
  return;
}


//-------------------------------------------------------------------------//
// save_warp_data                                                          //
//-------------------------------------------------------------------------//
void save_warp_data(XYData *data)
{
  static PromptStruct ps;
  static char filename[FILENAMELENGTH]="1.points";
  if(memorylessthan(4096)){  message(g.nomemory,ERROR); return; } 
  int answer=OK;
  if(data->n==0){ message("No data to save",ERROR); return; }
  answer = message("New filename:",filename,PROMPT,FILENAMELENGTH-1,54);
  if(answer==CANCEL) return;
  ps.filename = filename;
  ps.f1 = save_warp_data_part2;
  ps.f2 = null;
  ps.ptr = (void*)data;
  check_overwrite_file(&ps);
}


//-------------------------------------------------------------------------//
// save_warp_data_part2                                                    //
//-------------------------------------------------------------------------//
void save_warp_data_part2(PromptStruct *ps)
{
  int k;
  FILE *fp;
  XYData *data = (XYData *)ps->ptr;
  char *filename = ps->filename;
  char temp[FILENAMELENGTH];
  if ((fp=fopen(filename,"w")) == NULL)
      error_message(filename, errno);
  else
  {   fprintf(fp,"#Registration control points\n");
      fprintf(fp,"#Ref.\tx  \ty  \tSize\tSignal\t\txmin\tymin\txmax\tymax\n");

      for(k=0; k<data->n; k++)
      {  
           fprintf(fp,"%s\t%d\t%d\t%g\t%g\t\t%d\t%d\t%d\t%d\t\n", 
                   data->label[k], data->x[k], data->y[k], data->u[k], data->v[k],
                   data->x1[k], data->y1[k], data->x2[k], data->y2[k]);
      }
      sprintf(temp,"Data saved in %s",filename);
      message(temp);
      fclose(fp);
  }
}


//-------------------------------------------------------------------------//
// calc_distance                                                           //
//-------------------------------------------------------------------------//
void calc_distance(double dx, double dy, double &r, double &theta)
{
  theta = atan2(dy, dx);
  if(theta == 0.0) theta = 1e-6; // can be + or -
  r = max(1e-6, sqrt(dx*dx + dy*dy));
}

//-------------------------------------------------------------------------//
// calc_distance                                                           //
//-------------------------------------------------------------------------//
void calc_distance(int dx, int dy, double &r, double &theta)
{
  double x = (double)dx;
  double y = (double)dy; 
  theta = atan2(y, x);
  if(theta == 0.0) theta = 1e-6; // can be + or -
  r = max(1e-6, sqrt(x*x + y*y));
}

//-------------------------------------------------------------------------//
// distance                                                                //
//-------------------------------------------------------------------------//
double distance(int x1, int y1, int x2, int y2)
{
  int dx = x2 - x1;
  int dy = y2 - y1;
  return (dx*dx + dy*dy);
}

//-------------------------------------------------------------------------//
// create_distance_image                                                   //
//-------------------------------------------------------------------------//
int create_distance_image(XYData *data, int xsize, int ysize, char *extension)
{
  int v,i,j,i2;
  char tempstring[FILENAMELENGTH];
  if(!between(xsize, 1, 100000) || !between(ysize, 1, 100000))
  { message("Error creating image\nbad data point or\tstray text in data file", ERROR); return 0; }
  if(newimage("Image",0,0,xsize,ysize,24,COLOR,1,0,0,PERM,1,0,0)!=OK){ message(g.nomemory, ERROR); return 0;}
  v = RGBvalue(127,127,127,24);
  for(j=0;j<ysize;j++)
  for(i=0,i2=0; i<xsize; i++,i2+=3)
      putpixelbytes(z[ci].image[0][j]+i2, v, 1, 24, 1); 
  switchto(ci);
  image_border();
  sprintf(tempstring, "%s.%s", data[1].title, extension);
  setimagetitle(ci, tempstring);
  return ci;
}

//-------------------------------------------------------------------------//
// calculate_size                                                          //
//-------------------------------------------------------------------------//
void calculate_size(XYData *data, int &xsize, int &ysize)
{
  int j;
  int n0 = max(data[0].n, data[1].n);
  xsize = ysize = 0;
  for(j=0; j<n0; j++) xsize = max(xsize, max(data[0].x[j], data[1].x[j]));
  for(j=0; j<n0; j++) ysize = max(ysize, max(data[0].y[j], data[1].y[j]));

  int n4 = max(data[4].n, data[5].n);
  for(j=0; j<n4; j++) xsize = max(xsize, max(data[4].x[j], data[5].x[j]));
  for(j=0; j<n4; j++) ysize = max(ysize, max(data[4].y[j], data[5].y[j]));

  int ino1 = data[1].ino;
  if(between(ino1, 1, g.image_count-1))
  {    xsize = max(xsize, z[ino1].xsize);
       ysize = max(ysize, z[ino1].ysize);
  }
  xsize += 10;
  ysize += 10;
}

//-------------------------------------------------------------------------//
// initialize_vector_map                                                   //
//-------------------------------------------------------------------------//
void initialize_vector_map(int xsize, int ysize)
{
  int i,j;
  if(xbias!=NULL)
  {   free(xbias); free(xbias_1d); 
      free(ybias); free(ybias_1d); 
      free(rbias); free(rbias_1d); 
  }

  xbias_1d = (int*)malloc(xsize*ysize*sizeof(int));
  xbias = make_2d_alias(xbias_1d, xsize, ysize);
  if(xbias==NULL){ message(g.nomemory,ERROR); return; }

  ybias_1d = (int*)malloc(xsize*ysize*sizeof(int));
  ybias = make_2d_alias(ybias_1d, xsize, ysize);
  if(ybias==NULL){ message(g.nomemory,ERROR); return; }

  rbias_1d = (double*)malloc(xsize*ysize*sizeof(double));
  rbias = make_2d_alias(rbias_1d, xsize, ysize);
  if(rbias==NULL){ message(g.nomemory,ERROR); return; }
  
  for(j=0;j<ysize;j++)
  for(i=0;i<xsize;i++)
  {    xbias[j][i] = 0;
       ybias[j][i] = 0;
       rbias[j][i] = 0.0;
  }
}


//-------------------------------------------------------------------------//
// update_image                                                            //
//-------------------------------------------------------------------------//
void update_image(int ino, int **xbias, int **ybias, double **rbias,
     int xsize, int ysize)
{
  int i,i2,j,v,rr,gg,bb;
  g.busy++;
  uchar *address;
  for(j=0; j<ysize; j++) 
  for(i=0,i2=0; i<xsize; i++,i2+=3) 
  {   address = z[ino].image[0][j]+i2;
      v = pixelat(address, 24);
      if(v==0 || v==g.maxvalue[24]) continue;
      rr = max(0,min(255, 127 + xbias[j][i]));
      gg = max(0,min(255, 127 + cint(DEGPERRAD * rbias[j][i])));
      bb = max(0,min(255, 127 + ybias[j][i]));
      v = RGBvalue(rr,gg,bb,24);
      putpixelbytes(address, v, 1, 24, 1);
  }
  repair(ino);
  redraw(ino);
  g.busy = max(0, g.busy-1);
} 


//-------------------------------------------------------------------------//
// draw_spots                                                              //
//-------------------------------------------------------------------------//
void draw_spots(XYData *data, int color)
{
  int color0, color1;
  int j,ino;
  int *x0 = data[0].x;
  int *x1 = data[1].x;
  int *y0 = data[0].y;
  int *y1 = data[1].y;
  int n0 = data[0].n;
  int n1 = data[1].n;
  ino = data[0].ino;
  if(!between(ino, 0, g.image_count-1)) return;
  int xpos = z[ino].xpos;
  int ypos = z[ino].ypos;
  if(color) color0=color; else color0 = (int)g.maxvalue[24];
  if(color) color1=color; else color1 = 0;
  for(j=0; j<n0; j++)
  {   if(x0[j]>0 && y0[j]>0)
         circle(x0[j]+xpos, y0[j]+ypos, 6, 1, color0, SET);
  }
  for(j=0; j<n1; j++)
  {   if(x1[j]>0 && y1[j]>0)
         circle(x1[j]+xpos, y1[j]+ypos, 6, 1, color1, SET);
  }
}


//-------------------------------------------------------------------------//
// draw_spot_labels                                                        //
//-------------------------------------------------------------------------//
void draw_spot_labels(XYData *data, int color)
{
  int color0, color1;
  int j,ino;
  int *x0 = data[0].x;
  int *x1 = data[1].x;
  int *y0 = data[0].y;
  int *y1 = data[1].y;
  int n0 = data[0].n;
  int n1 = data[1].n;
  char label[1024]; 
  ino = data[0].ino;
  if(!between(ino, 0, g.image_count-1)) return;
  int xpos = z[ino].xpos;
  int ypos = z[ino].ypos;
  char newfont[] = "*helvetica-medium-r-normal--10-100-75-75*";
  setfont(newfont);
  if(color) color0=color; else color0 = (int)g.maxvalue[24];
  if(color) color1=color; else color1 = 0;

  print_label((char*)" ", 10, 10, (int)g.maxvalue[24], 0);
  for(j=0; j<n0; j++)
  {   if(x0[j]>0 && y0[j]>0)
      {   strcpy(label, data[0].label[j]);
          print_label(label, x0[j]+xpos, y0[j]+ypos, color0, 0);
      }
  }
  for(j=0; j<n1; j++)
  {   if(x1[j]>0 && y1[j]>0)
      {   strcpy(label, data[1].label[j]);
          print_label(label, x1[j]+xpos, y1[j]+ypos, color1, 0);
      }
  }
  setfont(g.imagefont);
}


//-------------------------------------------------------------------------//
// create_vector_map                                                       //
//-------------------------------------------------------------------------//
void create_vector_map(XYData *data, char *table, int *match, int &xsize, int &ysize)
{
  static int oino=-1, oxsize=-1, oysize=-1;
  int ino=0,k,n,ok0,ok1;
  n = max(data[4].n, data[5].n);
  ok0 = ok1 = 0;

  if(data[4].n == 0 || data[5].n == 0){ message("No landmark points defined"); return; }
  //// Make sure there are non-zero landmark points
  for(k=0; k<n; k++)
  {   if(data[4].x[k] && data[4].y[k]) ok0++; 
      if(data[5].x[k] && data[5].y[k]) ok1++; 
  }
  if(!ok0){ message("No reference landmarks"); return; }
  if(!ok1){ message("No unknown landmarks"); return; }
  printstatus(BUSY);

  initialize_vector_map(xsize, ysize);
  ino = oino;
  if(xsize != oxsize || 
      ysize != oysize || 
      oino == -1 ||
      !between(ino, 1, g.image_count-1) ||
      z[ino].xsize != xsize || 
      z[ino].ysize != ysize ||
      z[ino].bpp != 24)
      ino = create_distance_image(data, xsize, ysize, (char*)"vector_map");
  else 
      ino = oino;
  if(ino<=0){  printstatus(NORMAL); return; }
  for(k=0; k<6; k++) data[k].ino = ino;
  oino = ino;
  oxsize = xsize;
  oysize = ysize;

  //// Create map based on landmark points
  calculate_vector_map(&data[4], xbias, ybias, xsize, ysize);
  update_image(ino, xbias, ybias, rbias, xsize, ysize);
  match_points(data, table, xbias, ybias, match, ino); 
  printstatus(NORMAL);
}


//--------------------------------------------------------------------------//
// calculate_vector_map - points must be already correlated                 //
//--------------------------------------------------------------------------//
void calculate_vector_map(XYData *data, int **dx, int **dy, int xsize, int ysize)
{
  int i,i1,j,j1,k,nn,iscale,jscale;
  int vx,vy,x,y,e,h,xa,xb,xc,xd,ya,yb,yc,yd;
  double fx,fy,dist,mindist;
  Widget www, scrollbar;
  nn = max(data[0].n, data[1].n);
  g.getout = 0;
  array<int>xsmall(xsize/SCALE+SCALE, ysize/SCALE+SCALE);
  array<int>ysmall(xsize/SCALE+SCALE, ysize/SCALE+SCALE);

  printstatus(BUSY);
  progress_bar_open(www, scrollbar);


  for(j=0; j<ysize; j++)
  {   jscale = j/SCALE;
      progress_bar_update(scrollbar, 100*j/ysize);
      check_event();
      if(g.getout){ g.getout=0; break; }
      for(i=0; i<xsize; i++)
      {   iscale = i/SCALE;
          mindist = 1e6;
          for(k=0; k<nn; k++)
          {
              dist = distance(i, j, data[0].x[k],data[0].y[k]); 
              if(dist < mindist)
              {    mindist = dist; 
                   xsmall.p[jscale][iscale] = data[0].x[k] - data[1].x[k];
                   ysmall.p[jscale][iscale] = data[0].y[k] - data[1].y[k];
              }
          }          
     }
  } 

  //// Change last param for smoother vector map
  //// (can be 3,5,7,9,11,15,19,21)
  local_smooth(xsmall.p, xsize/SCALE, ysize/SCALE, smooth_fac[warp_smoothing]);
  local_smooth(ysmall.p, xsize/SCALE, ysize/SCALE, smooth_fac[warp_smoothing]);

  for(j=0; j<ysize/SCALE; j++)
  for(i=0; i<xsize/SCALE; i++)
  {   iscale = i*SCALE;
      jscale = j*SCALE;
      for(h=0; h<SCALE; h++)
      for(e=0; e<SCALE; e++)
      {   
          if(j<ysize/SCALE-1) j1 = j+1; else j1=j;
          if(i<xsize/SCALE-1) i1 = i+1; else i1=i;

          xa = xsmall.p[j ][i ];
          xb = xsmall.p[j ][i1];
          xc = xsmall.p[j1][i ];
          xd = xsmall.p[j1][i1];

          ya = ysmall.p[j ][i ];
          yb = ysmall.p[j ][i1];
          yc = ysmall.p[j1][i ];
          yd = ysmall.p[j1][i1];

          fx = (double)h/SCALE;
          fy = (double)e/SCALE;

          vx = cint((1.0-fx)*(1.0-fy)*(double)xa + 
                    (    fx)*(1.0-fy)*(double)xb + 
                    (1.0-fx)*(    fy)*(double)xc + 
                    (    fx)*(    fy)*(double)xd); 

          vy = cint((1.0-fx)*(1.0-fy)*(double)ya + 
                    (    fx)*(1.0-fy)*(double)yb + 
                    (1.0-fx)*(    fy)*(double)yc + 
                    (    fx)*(    fy)*(double)yd); 

          x = min(xsize-1, max(0, iscale+h));
          y = min(ysize-1, max(0, jscale+e));
          dx[y][x] = vx;
          dy[y][x] = vy;
      }
  }
  for(j=max(1,ysize-SCALE); j<ysize; j++)
  for(i=0; i<xsize; i++)
  {   dx[j][i] = dx[max(1,ysize-(SCALE+1))][i];
      dy[j][i] = dy[max(1,ysize-(SCALE+1))][i];
  }
  for(j=0; j<ysize; j++)
  for(i=max(1,xsize-SCALE); i<xsize; i++)
  {   dx[j][i] = dx[j][max(1,xsize-(SCALE+1))];
      dy[j][i] = dy[j][max(1,xsize-(SCALE+1))];
  }

  /* 

  //// Inverse square weighting

  double invdsquared[nn];
  double total, dxtemp, dytemp, denom, weight; 
  for(j=0; j<ysize; j++)
  {   progress_bar_update(scrollbar, 100*j/ysize);
      if(g.getout) break;
      for(i=0; i<xsize; i++)
      {   dx[j][i] = 0;
          dy[j][i] = 0;
          total = 0.0;

          for(k=0; k<nn; k++)
          {
              dxtemp = (double)i - data[1].x[k]; 
              dytemp = (double)j - data[1].y[k]; 
              denom = dxtemp*dxtemp + dytemp*dytemp; 
              if(denom) invdsquared[k] = 1 / denom; else invdsquared[k] = 100.0;
              total += invdsquared[k];
          }
          for(k=0; k<nn; k++)
          {
              if(total) weight = invdsquared[k] / total; else weight = 1.0;
              dx[j][i] += cint(weight * (data[0].x[k] - data[1].x[k]));
              dy[j][i] += cint(weight * (data[0].y[k] - data[1].y[k]));
          }          
     }
  }  
  */ 
 
  vector_map_created = 1;
  progress_bar_close(www);
  printstatus(NORMAL);
}


//--------------------------------------------------------------------------//
//  registration_warp                                                       //
//  warpmode = 1 - perform warping                                          //
//  warpmode = 2 - show vectors for image, don't warp                       //
//--------------------------------------------------------------------------//
void registration_warp(dialoginfo *a, int warpmode)
{
  static int in_warp=0;
  XYData *data = (XYData*)a->ptr[4];
  int ino = data[1].ino;
  int f,i,i2,j,val,ix1,iy1,ix2,iy2,ino0,ino1,step;
  int k,ix,iy;
  double x,y;
  if(!between(ino, 1, g.image_count-1)){ message("No image to warp",ERROR); return; }
  int bpp = z[ino].bpp;
  int b = g.off[bpp];
  int xsize = z[ino].xsize;
  int ysize = z[ino].ysize;
  f = z[ino].cf;
  lineinfo li;
  ino0 = data[0].ino;
  ino1 = data[1].ino;
  li.ino=ino1;
  li.type=0;
  if(bpp == 8 || z[ino].colortype==GRAY) li.color = 0;
  else li.color = RGBvalue(255,0,0,bpp);
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
  li.window=z[ino1].win;
  li.wantprint=0;
  li.antialias=0;
  step = z[ino].xsize / 10;

  if(!vector_map_created){ message("Create a vector map first", INFO, 71); return; }

  ////  No returns past this point
  
  if(in_warp) return;
  in_warp=1;
  array<int> buffer(xsize, ysize);
  printstatus(BUSY);

  ////  Iterate through destination pixels, calculating source pixel
  ////  coordinates from dx,dy

  
  if(warpmode == 1)  // warp
  {  
      for(j=0; j<ysize; j++)
      for(i=0; i<xsize; i++)
      {
          x = (double)(i - xbias[j][i]); 
          y = (double)(j - ybias[j][i]); 
          val = read_aliased_pixel(x,y,ino);
          buffer.p[j][i] = val;

      }
      for(j=0; j<ysize; j++)
      for(i=0,i2=0; i<xsize; i++,i2+=b)
      {
          putpixelbytes(z[ino].image[f][j]+i2, buffer.p[j][i], 1, bpp, 1);
      }
      rebuild_display(ino);
      repair(ino);
  }
  if(warpmode == 2)  // show vectors for evenly spaced points on image
  {   
      for(j=0; j<ysize; j+=step)
      for(i=0,i2=0; i<xsize; i+=step,i2+=step*b)
      {   ix1 = i + xbias[j][i] + z[ino1].xpos; 
          iy1 = j + ybias[j][i] + z[ino1].ypos; 
          ix2 = i + z[ino1].xpos;
          iy2 = j + z[ino1].ypos;
          line(ix2, iy2, ix1, iy1, SET, &li); 
          draw_arrow(ix2, iy2, ix1, iy1, &li); 
      }
  }
  printstatus(NORMAL);

  //// Warp control points for 2nd iteration
  for(k=0; k<data[5].n; k++)
  {    ix = data[5].x[k];
       iy = data[5].y[k];
       if(between(ix, 0, xsize-1) &&
          between(iy, 0, ysize-1))
       {    data[5].x[k] += xbias[iy][ix];
            data[5].y[k] += ybias[iy][ix];
       }
  }
  switchto(data[5].ino);
  in_warp = 0;
  return;
}


//-------------------------------------------------------------------------//
// create_rotation_map                                                      //
//-------------------------------------------------------------------------//
void create_rotation_map(XYData *data, char *table, int *match, int &xsize, int &ysize)
{
  static int oino=-1, oxsize=-1, oysize=-1;
  table=table;match=match;
  int ino=0,k,n,ok0,ok1;
  n = max(data[4].n, data[5].n);
  ok0 = ok1 = 0;

  if(data[4].n == 0 || data[5].n == 0){ message("No landmark points defined", ERROR, 71); return; }
  //// Make sure there are non-zero landmark points
  for(k=0; k<n; k++)
  {   if(data[4].x[k] && data[4].y[k]) ok0++; 
      if(data[5].x[k] && data[5].y[k]) ok1++; 
  }
  if(!ok0){ message("No reference landmarks"); return; }
  if(!ok1){ message("No unknown landmarks"); return; }

  initialize_vector_map(xsize, ysize);
  ino = oino;
  if(xsize != oxsize || 
      ysize != oysize || 
      oino == -1 ||
      !between(ino, 1, g.image_count-1) ||
      z[ino].xsize != xsize || 
      z[ino].ysize != ysize)
      ino = create_distance_image(data, xsize, ysize, (char*)"rotation_map");
  else 
      ino = oino;
  if(!ino) return;  
  for(k=0; k<6; k++) data[k].ino = ino;
  oino = ino;
  oxsize = xsize;
  oysize = ysize;

  //// Create map based on landmark points
  calculate_rotation_map(&data[4], rbias, xsize, ysize);
  update_image(ino, xbias, ybias, rbias, xsize, ysize);
  printstatus(NORMAL);
}


//--------------------------------------------------------------------------//
// calculate_rotation_map - points must be already correlated               //
//--------------------------------------------------------------------------//
void calculate_rotation_map(XYData *data, double **dr, int xsize, int ysize)
{
  int i,i1,j,j1,k,nn,iscale,jscale;
  int x,y,e,h,dx0,dy0,dx1,dy1,xhalf,yhalf;
  double aa,ab,ac,ad,fx,fy,dist,mindist,v,r0,theta0,r1,theta1,dtheta;
  Widget www, scrollbar;
  nn = max(data[0].n, data[1].n);
  g.getout = 0;
  array<double>asmall(xsize/SCALE+SCALE, ysize/SCALE+SCALE);
  xhalf = xsize/2;
  yhalf = ysize/2;

  printstatus(BUSY);
  progress_bar_open(www, scrollbar);

  for(j=0; j<ysize; j++)
  {   jscale = j/SCALE;
      progress_bar_update(scrollbar, 100*j/ysize);
      check_event();
      if(g.getout){ g.getout=0; break; }
      for(i=0; i<xsize; i++)
      {   iscale = i/SCALE;
          mindist = 1e6;
          for(k=0; k<nn; k++)
          {   
              dist = distance(i, j, data[1].x[k], data[1].y[k]); 
              if(dist < mindist) // If k is the closest landmark to i,j
              {    mindist = dist; 
                   dx0 = data[0].x[k] - xhalf;
                   dy0 = data[0].y[k] - yhalf;
                   dx1 = data[1].x[k] - xhalf;
                   dy1 = data[1].y[k] - yhalf;
                   calc_distance(dx0, dy0, r0, theta0);
                   calc_distance(dx1, dy1, r1, theta1);
                   dtheta = theta0-theta1;
                   asmall.p[jscale][iscale] = dtheta;
              }
          }          
     }
  } 

  //// Change last param for smoother vector map
  local_smooth(asmall.p, xsize/SCALE, ysize/SCALE, smooth_fac[rot_smoothing]);
  
  for(j=0; j<ysize/SCALE; j++)
  for(i=0; i<xsize/SCALE; i++)
  {   iscale = i*SCALE;
      jscale = j*SCALE;
      for(h=0; h<SCALE; h++)
      for(e=0; e<SCALE; e++)
      {   
          if(j<ysize/SCALE-1) j1 = j+1; else j1=j;
          if(i<xsize/SCALE-1) i1 = i+1; else i1=i;

          aa = asmall.p[j ][i ];
          ab = asmall.p[j ][i1];
          ac = asmall.p[j1][i ];
          ad = asmall.p[j1][i1];

          fx = (double)h/SCALE;
          fy = (double)e/SCALE;

          v = (1.0-fx)*(1.0-fy)*(double)aa + 
              (    fx)*(1.0-fy)*(double)ab + 
              (1.0-fx)*(    fy)*(double)ac + 
              (    fx)*(    fy)*(double)ad; 

          x = min(xsize-1, max(0, iscale+h));
          y = min(ysize-1, max(0, jscale+e));
          dr[y][x] = v;
      }
  }

  for(j=max(1,ysize-SCALE); j<ysize; j++)
  for(i=0; i<xsize; i++)
      dr[j][i] = dr[max(1,ysize-(SCALE+1))][i];
  for(j=0; j<ysize; j++)
  for(i=max(1,xsize-SCALE); i<xsize; i++)
      dr[j][i] = dr[j][max(1,xsize-(SCALE+1))];
  rotation_map_created = 1;
  progress_bar_close(www);
  printstatus(NORMAL);
}


//--------------------------------------------------------------------------//
//  registration_rotate                                                     //
//  warpmode = 1 - perform warping                                          //
//  warpmode = 2 - show vectors, don't warp                                 //
//--------------------------------------------------------------------------//
void registration_rotate(dialoginfo *a, int warpmode)
{
  XYData *data = (XYData*)a->ptr[4];
  int ino = data[1].ino,dx,dy,xhalf,yhalf,f,i,i2,j,val;
  int bpp = z[ino].bpp;
  int b = g.off[bpp];
  int xsize = z[ino].xsize;
  int ysize = z[ino].ysize;
  double x, y, r, theta;
  f = z[ino].cf;
  if(!between(ino, 1, g.image_count-1)){ message("No image to rotate",ERROR); return; }
  if(!rotation_map_created){ message("Create a rotation map first"); return; }
  xhalf = xsize/2;
  yhalf = ysize/2;

  printstatus(BUSY);
  array<int> buffer(xsize, ysize);

  ////  Iterate through destination pixels, calculating source pixel
  ////  coordinates from dr
  
  if(warpmode == 1)  // warp
  {   for(j=0; j<ysize; j++)
      for(i=0; i<xsize; i++)
      {
           dx = i - xhalf;
           dy = j - yhalf;
           calc_distance(dx, dy, r, theta);
           theta -= rbias[j][i]; 
           x = xhalf + r * cos(theta);
           y = yhalf + r * sin(theta);
           val = read_aliased_pixel(x,y,ino);
           buffer.p[j][i] = val;
      }
      for(j=0; j<ysize; j++)
      for(i=0,i2=0; i<xsize; i++,i2+=b)
      {
          putpixelbytes(z[ino].image[f][j]+i2, buffer.p[j][i], 1, bpp, 1);
      }
      rebuild_display(ino);
      repair(ino);
  }

  //// Rotate control points for 2nd iteration
  int k,ix,iy;
  for(k=0;k<data[5].n;k++)
  {
      ix = data[5].x[k];
      iy = data[5].y[k];
      if(!between(ix,0,xsize) || !between(iy,0,ysize)) continue;
      dx = ix - xhalf;
      dy = iy - yhalf;
      calc_distance(dx, dy, r, theta);
      theta += rbias[iy][ix]; 
      x = xhalf + r * cos(theta);
      y = yhalf + r * sin(theta);
      data[5].x[k] = cint(x);
      data[5].y[k] = cint(y);
  }
  switchto(data[5].ino);
  printstatus(NORMAL);
}

