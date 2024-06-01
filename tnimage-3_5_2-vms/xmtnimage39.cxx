//--------------------------------------------------------------------------//
// xmtnimage39.cc                                                           //
// camera interface                                                         //
// Latest revision: 10-16-2000                                              //
// Copyright (C) 2000 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"
#include "xmtnimaged.h"
#define swapbyte(a,b){ ccc=(a);  (a)=(b); (b)=ccc; }  

extern Globals     g;
extern Image      *z;
extern int         ci;
extern uchar ccc;
int child_status;
int noofcameras = 0;
int in_camera = 0;
int acquiring = 0;  

//--------------------------------------------------------------------------//
//  camera                                                                  //
//--------------------------------------------------------------------------//
int camera(void)
{
  if(memorylessthan(16384)){ message( g.nomemory,ERROR); return(NOMEM); }
  if(in_camera) return BUSY;
  static Dialog *dialog;
  dialog = new Dialog;
  if(dialog==NULL) return NOMEM;
  in_camera=1;
  int j,k;
  char cameralist[FILENAMELENGTH];
  char tempstring[FILENAMELENGTH];
  FILE *fp;
  g.getout=0;
  strcpy(dialog->title,"Camera");
  g.cam.state = ACQUIRE;
  g.cam.ino   = 0;
  if(!g.cam.hit)     ////  Starting parameters
  {   
      g.cam.args  = new char *[64];
      g.cam.argno = 0;
      strcpy(g.cam.name, g.camera); 
      g.cam.camera_id = g.camera_id;
      g.cam.w     = 640;
      g.cam.h     = 480;
      g.cam.scale  = 1;
      g.cam.fps    = 10;
      g.cam.brightness= 127;
      g.cam.contrast  = 127;
      g.cam.widget    = 0;    // Create new Widget
      g.cam.depth     = 3;
      g.cam.bpp       = g.cam.depth*8;
      g.cam.white     = 100;
      g.cam.black     = 100;
      g.cam.saturation= 127;
      g.cam.hue       = 100;
      g.cam.defaults[0] = 0;
      g.cam.shell     = 1;
      g.cam.acq_mode  = 1;
      g.cam.continuous= 1;
      g.cam.state = INITIALIZING;
      g.cam.post_processing_command[0] = 0;
      g.cam.command_is_modified = 0;
  }
  g.cam.dialog    = NULL;
  g.cam.frames    = 0;

  //-----Dialog box to get title & other options------------//

  strcpy(dialog->radio[0][0],"Acquisition mode");
  strcpy(dialog->radio[0][1],"Normal");             
  strcpy(dialog->radio[0][2],"Accumulate frames");
  strcpy(dialog->radio[0][3],"Add");
  strcpy(dialog->radio[0][4],"Subtract");
    
  dialog->radiono[0]=5;
  dialog->radioset[0] = g.cam.acq_mode;

  strcpy(dialog->radio[1][0],"Camera");
  strcpy(dialog->radio[1][1],"Initialize");
  strcpy(dialog->radio[1][2],"Normal");             

  dialog->radiono[1] = 3;
  if(g.cam.state==INITIALIZING) dialog->radioset[1] = 1; else dialog->radioset[1] = 2;

  strcpy(dialog->radio[2][0],"Frames");
  strcpy(dialog->radio[2][1],"Continuous");
  strcpy(dialog->radio[2][2],"Single frame");             

  dialog->radiono[2] = 3;
  if(g.cam.continuous) dialog->radioset[2] = 1; else  dialog->radioset[2] = 2;
  dialog->radiocb[2] = (void*)camera_acquirecb;
  dialog->radiopushbuttonptr[2] = &g.cam;
  strcpy(dialog->radiopushbuttonlabel[2],"START");
  
  strcpy(dialog->boxes[0],"Camera Settings");
  strcpy(dialog->boxes[1],"Camera");
  strcpy(dialog->boxes[2],"Width");
  strcpy(dialog->boxes[3],"Height");
  strcpy(dialog->boxes[4],"Scale fac");
  strcpy(dialog->boxes[5],"Frames/sec.");
  strcpy(dialog->boxes[6],"Brightness");
  strcpy(dialog->boxes[7],"Contrast");
  strcpy(dialog->boxes[8],"Black level");
  strcpy(dialog->boxes[9],"White level"); 
  strcpy(dialog->boxes[10],"Hue");
  strcpy(dialog->boxes[11],"Saturation");
  strcpy(dialog->boxes[12],"Depth");
  strcpy(dialog->boxes[13],"Post process");
  dialog->boxtype[0]=LABEL;
  dialog->boxtype[1]=NON_EDIT_LIST;
  dialog->boxtype[2]=INTCLICKBOX;
  dialog->boxtype[3]=INTCLICKBOX;
  dialog->boxtype[4]=INTCLICKBOX;
  dialog->boxtype[5]=INTCLICKBOX;
  dialog->boxtype[6]=INTCLICKBOX;
  dialog->boxtype[7]=INTCLICKBOX;
  dialog->boxtype[8]=INTCLICKBOX;
  dialog->boxtype[9]=INTCLICKBOX;
  dialog->boxtype[10]=INTCLICKBOX;
  dialog->boxtype[11]=INTCLICKBOX;
  dialog->boxtype[12]=INTCLICKBOX;
  dialog->boxtype[13]=TOGGLESTRING;


  //// Find list of cameras
  dialog->l[1] = new listinfo;
  dialog->l[1]->info = new char*[1024];  // max 1024 cameras

  noofcameras=0;
  sprintf(cameralist,"%s/cameras", g.helpdir);
  if((fp = fopen(cameralist,"rt")) == NULL)
  {   sprintf(cameralist,"%s/cameras", g.homedir);
      if((fp = fopen(cameralist,"rt")) == NULL)
      {   error_message(cameralist, errno);
          in_camera = 0;
          return NOTFOUND;
      }
  }
  while(!feof(fp)) 
  {   dialog->l[1]->info[noofcameras] = new char[256];
      dialog->l[1]->info[noofcameras][0] = 0;
      fgets(dialog->l[1]->info[noofcameras], FILENAMELENGTH, fp);
      remove_terminal_cr(dialog->l[1]->info[noofcameras]);
      noofcameras++;
  }
  fclose(fp);
  
  sprintf(tempstring, "%s/cameras", g.homedir);
  if(strcmp(cameralist, tempstring))
  if((fp = fopen(cameralist,"rt")) != NULL)
  {   while(!feof(fp)) 
      {   dialog->l[1]->info[noofcameras] = new char[256];
          dialog->l[1]->info[noofcameras][0] = 0;
          fgets(dialog->l[1]->info[noofcameras], FILENAMELENGTH, fp);
          remove_terminal_cr(dialog->l[1]->info[noofcameras]);
          noofcameras++;
      }
      fclose(fp);
  }
  dialog->l[1]->count = noofcameras-1;
  dialog->l[1]->wantfunction = 1;
  dialog->l[1]->f1 = camera_load_defaults;
  dialog->l[1]->f2 = null; 
  dialog->l[1]->f3 = null; 
  dialog->l[1]->f4 = null; // dialog lists are deleted in dialogcancelcb
  dialog->l[1]->f5 = null; 
  dialog->l[1]->f6 = null;
  dialog->l[1]->listnumber = 0;
  dialog->l[1]->ptr[1] = (void *)&g.cam;
  if(noofcameras==0)
  {   delete[] dialog->l[1];
      delete[] dialog;
      in_camera=0; 
      return NOTFOUND;
  }

  dialog->l[1]->title = new char[128];
  strcpy(dialog->l[1]->title, "Select camera");
  g.cam.camera_id = min(g.cam.camera_id, noofcameras-1);
  strcpy(g.cam.name, dialog->l[1]->info[g.cam.camera_id]);
  strcpy(dialog->answer[1][0], g.cam.name);
  dialog->l[1]->selection = &g.cam.camera_id;
  dialog->boxmin[1]=0; dialog->boxmax[1]=noofcameras;

  strcpy( dialog->answer[1] [0],g.cam.name);
  sprintf(dialog->answer[2] [0],"%d",g.cam.w);          dialog->boxmin[2]=1; dialog->boxmax[2]=2048;
  sprintf(dialog->answer[3] [0],"%d",g.cam.h);          dialog->boxmin[3]=1; dialog->boxmax[3]=2048;
  sprintf(dialog->answer[4] [0],"%d",g.cam.scale);      dialog->boxmin[4]=1; dialog->boxmax[4]=4;
  sprintf(dialog->answer[5] [0],"%d",g.cam.fps);        dialog->boxmin[5]=1; dialog->boxmax[5]=100;
  sprintf(dialog->answer[6] [0],"%d",g.cam.brightness); dialog->boxmin[6]=0; dialog->boxmax[6]=255;
  sprintf(dialog->answer[7] [0],"%d",g.cam.contrast);   dialog->boxmin[7]=0; dialog->boxmax[7]=255;
  sprintf(dialog->answer[8] [0],"%d",g.cam.black);      dialog->boxmin[8]=0; dialog->boxmax[8]=255;
  sprintf(dialog->answer[9] [0],"%d",g.cam.white);      dialog->boxmin[9]=0; dialog->boxmax[9]=255;
  sprintf(dialog->answer[10][0],"%d",g.cam.hue);       dialog->boxmin[10]=0; dialog->boxmax[10]=255;
  sprintf(dialog->answer[11][0],"%d",g.cam.saturation);dialog->boxmin[11]=0; dialog->boxmax[11]=255;
  sprintf(dialog->answer[12][0],"%d",g.cam.depth);     dialog->boxmin[12]=1; dialog->boxmax[12]=6;
  strcpy( dialog->answer[13][1], g.cam.post_processing_command);

  for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;  
  dialog->radiotype[2][2] = RADIOPUSHBUTTON;

  dialog->want_changecicb = 0;
  dialog->f1 = cameracheck;
  dialog->f2 = null;
  dialog->f3 = camerafinish;
  dialog->f4 = camerafinish2;
  dialog->f5 = null;
  dialog->f6 = null;
  dialog->f7 = null;
  dialog->f8 = null;
  dialog->f9 = null;
  dialog->width = 0;  // calculate automatically
  dialog->height = 0; // calculate automatically
  dialog->noofradios = 3;
  dialog->noofboxes  = 14;
  dialog->helptopic  = 49;
  dialog->transient  = 1;
  dialog->wantraise = 0;
  dialog->radiousexy = 0;
  dialog->boxusexy   = 0;
  strcpy(dialog->path,".");
  dialog->ptr[0]     = &g.cam;
  dialog->message[0] = 0;      

  ////  Keep this null until dialog struct is filled
  g.cam.dialog = dialog;
  dialog->busy = 0;
  dialogbox(dialog);
  return OK;
}


//--------------------------------------------------------------------------//
//  cameracheck                                                             //
//--------------------------------------------------------------------------//
void cameracheck(dialoginfo *a, int radio, int box, int boxbutton)
{
  radio=radio; box=box; boxbutton=boxbutton;
  if(g.block) return;  // Waiting for input; don't deallocate anything yet
  CameraSettings *cam = (CameraSettings*)a->ptr[0];  
  if(radio != -2) return;
  
  delete_previous_command(cam);
  camera_read_defaults(&g.cam);
  compose_camera_command(&g.cam);
}  


//--------------------------------------------------------------------------//
//  camerafinish                                                            //
//--------------------------------------------------------------------------//
void camerafinish(dialoginfo *a)
{
  a=a;
  g.getout = 1;  // break out of acquisition loop
}


  
//--------------------------------------------------------------------------//
//  camerafinish2                                                           //
//--------------------------------------------------------------------------//
void camerafinish2(dialoginfo *a)
{
  CameraSettings *cam = (CameraSettings*)a->ptr[0];  
  cam->dialog = NULL;
  cam->hit = 1;
  if(g.autoundo) backupimage(cam->ino, 0);  
  in_camera = 0;
  g.getout = 0;
  g.busy = 0;
}


//--------------------------------------------------------------------------//
//  camera_acquirecb                                                        //
//  get a single frame in response to pushbutton                            //
//--------------------------------------------------------------------------//
void camera_acquirecb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; call_data=call_data;
  static char tempstring[64];
  int ct=COLOR;
  CameraSettings *cam = (CameraSettings *)client_data;
  if(acquiring) { g.getout=1; return; } // break out
  acquiring = 1;

  if(g.getout) strcpy(tempstring, "START");
  else strcpy(tempstring, "STOP");
  set_widget_label(w, tempstring);
  check_camera_dialog(cam);
  if(cam->ino<=0)
  {  
      if(newimage("Camera",0,0, cam->w, cam->h, cam->bpp, ct, 1, cam->shell, 0, PERM, 1, 
          g.window_border, 0) != OK)
      {    message("Cant create image buffer");
           acquiring = 0;
           return;
      }
      cam->ino = ci;
      z[cam->ino].touched = 1;
      rebuild_display(cam->ino);
      setimagetitle(cam->ino, "Camera");  
  }
  do
  {   
      delete_previous_command(cam);
      if(compose_camera_command(cam) != OK){ message("Aborted"); break; }
      if(!in_camera || g.getout) break;
      if(get_camera_image(cam) != OK) { message("Aborted"); break; }
      g.block++;
      while(XtAppPending(g.app) && !g.getout)    
           XtAppProcessEvent(XtWidgetToApplicationContext(g.drawing_area),XtIMAll);
      g.block = max(0, g.block-1);
      if(!in_camera || g.getout) break;
      check_camera_dialog(cam);
  }while (cam->continuous && g.getout==0 && in_camera);
  g.getout=0;
  acquiring = 0;
}



//--------------------------------------------------------------------------//
// delete_previous_command                                                  //
//--------------------------------------------------------------------------//
void delete_previous_command(CameraSettings *cam)
{  
   int k;
   for(k=0;k<cam->argno;k++) delete[] cam->args[k];
}


//--------------------------------------------------------------------------//
//  add_cam_option                                                          //
//--------------------------------------------------------------------------//
void add_cam_option(CameraSettings *cam, char *option, int value)
{
    if(strlen(option)) 
    {    
         cam->args[cam->argno] = new char[64];
         sprintf(cam->args[cam->argno],"%s",option);
         cam->argno++;

         cam->args[cam->argno] = new char[64];
         sprintf(cam->args[cam->argno],"%d",value);
         cam->argno++; 
    }
}


//--------------------------------------------------------------------------//
//  add_cam_option                                                          //
//--------------------------------------------------------------------------//
void add_cam_option(CameraSettings *cam, char *option)
{
    if(strlen(option)) 
    {    
         cam->args[cam->argno] = new char[64];
         sprintf(cam->args[cam->argno],"%s",option);
         cam->argno++;
    }
}


//--------------------------------------------------------------------------//
//  get_camera_image                                                        //
//--------------------------------------------------------------------------//
int get_camera_image(CameraSettings *cam)
{
  char tempstring[1024];
  int k,ino,bpp,y,bytesperline,bytesperpixel,status=0,v1,rr1,gg1,bb1,
      v2,rr2,gg2,bb2,v,rr,gg,bb,x,rave,gave,bave,rtotal,gtotal,btotal;
  double factor;
  uchar *add;
  mode_t fifo_mode = S_IRUSR | S_IWUSR;
  pid_t pid;
  int getout_save = 0 ;
  const int CMDLEN=128;  
  int noofargs=0;
  char **arg;                // up to 20 arguments for post-processing macro
  char fifofile[FILENAMELENGTH];
  
  ino = cam->ino;
  if(!between(ino, 0, g.image_count-1)){ message("No image\n"); return ABORT; }
  if(g.getout || !in_camera) return ABORT;
  bpp = z[ino].bpp;
  bytesperpixel =g.off[bpp];
  bytesperline = cam->w * bytesperpixel;

  //// If fifi does not exist, it is necessary to create him
  sprintf(fifofile, "%s/fifi", g.homedir);
  if(access(fifofile, F_OK))   
  {   if((status = mkfifo(fifofile, fifo_mode)==-1) && errno !=EEXIST)
           message("Could not create fifi!", ERROR);
  }

  //// Redirect output of camera driver through a fifo.
  //// Must use system() instead of execv() so output can be redirected.

  strcpy(tempstring,cam->args[0]);
  for(k=1;k<cam->argno;k++)
  {   strcat(tempstring," ");
      strcat(tempstring,cam->args[k]);
  }

  g.busy = 1;    // Prevent resizing by dragging on window or deleting image
  if((pid=fork())<0) 
      fprintf(stderr, "Fork error\n");
  else if(pid>0)            // Parent
  {  
      if(cam->w != z[cam->ino].xsize || cam->h != z[cam->ino].ysize)
      {   if(g.want_messages > 1) message("Resizing image");
          resize_image(cam->ino, cam->w, cam->h);
      }
      switch(cam->acq_mode)
      {  case 1:  // Normal mode, replace previous image 
             readpbmfile(fifofile, z[cam->ino].image[0]);
             break;
         case 2:  // Accumulate (average)
             cam->buffer = z[cam->ino].image[0];
             readpbmfile(fifofile, cam->buffer);
             for(y=0;y<cam->h;y++)
             {  add = z[ino].image[0][y];
                for(k=0; k<bytesperline; k+=bytesperpixel) 
                {   v1 = pixelat(z[ino].image[0][y] + k, bpp);  
                    v2 = pixelat(cam->buffer[y] + k, bpp);  
                    valuetoRGB(v1,rr1,gg1,bb1,bpp);
                    valuetoRGB(v2,rr2,gg2,bb2,bpp);
                    rr = (rr1*cam->frames + rr2) / (cam->frames+1);
                    gg = (gg1*cam->frames + gg2) / (cam->frames+1);
                    bb = (bb1*cam->frames + bb2) / (cam->frames+1);
                    v = RGBvalue(rr,gg,bb,bpp);
                    putpixelbytes(add+k,v,1,bpp,1);
                }
             }
             break;
        case 3:  // Add 
             cam->buffer = z[cam->ino].image[0];
             readpbmfile(fifofile, cam->buffer);
             for(y=0;y<cam->h;y++)
             {    add = z[ino].image[0][y]; 
                  for(k=0; k<bytesperline; k+=bytesperpixel) 
                  {   v1 = pixelat(z[ino].image[0][y] + k, bpp);  
                      v2 = pixelat(cam->buffer[y] + k, bpp);  
                      valuetoRGB(v1,rr1,gg1,bb1,bpp);
                      valuetoRGB(v2,rr2,gg2,bb2,bpp);
                      rr = min(g.maxred[bpp], rr1 + rr2/8);
                      gg = min(g.maxgreen[bpp], gg1 + gg2/8);
                      bb = min(g.maxblue[bpp], bb1 + bb2/8);
                      v = RGBvalue(rr,gg,bb,bpp);
                      putpixelbytes(add+k,v,1,bpp);
                  }
             }
             break;
        case 4:  // Subtract - remove dark current.
             cam->buffer = z[cam->ino].image[0];
             readpbmfile(fifofile, cam->buffer);

             ////   Controls degree of subtraction (0..1)
             ////   If subtraction removes too much, make this number smaller.

             factor = 0.5;   
             for(y=0;y<cam->h;y++)
             {  rtotal = gtotal = btotal = 0;
                rave = gave = bave = 0;
                for(x=0;x<bytesperline;x+=bytesperpixel)
                {    v = pixelat(cam->buffer[y] + x, bpp);
                     valuetoRGB(v,rr,gg,bb,bpp);
                     rtotal += rr;
                     gtotal += gg;
                     btotal += bb;
                 }
                 rave = cint(rtotal/(cam->w));
                 gave = cint(gtotal/(cam->w));
                 bave = cint(btotal/(cam->w));
                 add = z[ino].image[0][y];           
                 for(x=0; x<bytesperline; x+=bytesperpixel) 
                 {
                      v1 = pixelat(z[ino].image[0][y] + x, bpp);  
                      v2 = pixelat(cam->buffer[y] + x, bpp);  
                      valuetoRGB(v1,rr1,gg1,bb1,bpp);
                      valuetoRGB(v2,rr2,gg2,bb2,bpp);
                      rr = min(g.maxred[bpp],max(0, rr1 - cint((rr2-rave)*factor)));
                      gg = min(g.maxgreen[bpp],max(0, gg1 - cint((gg2-gave)*factor)));
                      bb = min(g.maxblue[bpp],max(0, bb1 - cint((bb2-bave)*factor)));
                      v = RGBvalue(rr,gg,bb,bpp);
                      putpixelbytes(add+x,v,1,bpp);
                 }
             }
             break;
      }
      status = 0;
      wait(&status);     // Wait for child process to terminate
      status >>= 8;
      error_message(cam->args[0], errno);
      if(status==222)
          message("Error executing camera driver\nPossibly root permissions required");
      else if(status)
          message("Error executing camera driver");
      if(status) g.getout=1;          

  }else                  // Child
  {   
      if(g.diagnose) printf("%s\n",tempstring);
      status = system(tempstring);
      if(status==256){ status = 222; g.getout=1; }
      child_status = status;
      _exit(status);     // Child must exit
  }
  g.busy = 0;

  //// After initialization, the config file changes automatically
  //// Also, switch back to normal mode.
  if(cam->state==INITIALIZING) 
  {   cam->state = ACQUIRE;
      update_camera_dialog(cam);
  }
  cam->frames++;
  switchto(ino);
  rebuild_display(ino);
  redraw(ino);

  ////  Post-processing
  getout_save = g.getout;
  arg = new char*[20];
  for(k=0;k<20;k++){ arg[k] = new char[128]; arg[k][0]=0; }
  parsecommand(cam->post_processing_command, FORMULA, arg, noofargs, CMDLEN, 20);
  if(cam->want_processing && strlen(arg[0])) execute(arg[0],noofargs,arg);
  g.getout = getout_save;
  for(k=0;k<20;k++) delete[] arg[k]; 
  delete[] arg;

  while(XtAppPending(g.app))    
        XtAppProcessEvent(XtWidgetToApplicationContext(g.drawing_area),XtIMAll);

  return OK;
}


//--------------------------------------------------------------------------//
//  check_camera_dialog                                                     //
//--------------------------------------------------------------------------//
void check_camera_dialog(CameraSettings *cam)
{
  char tempstring[FILENAMELENGTH];
  Dialog *dialog = g.cam.dialog;
  cam->acq_mode = dialog->radioset[0]; 
  if(dialog->radioset[1] == 1) cam->state=INITIALIZING ; 
  if(dialog->radioset[1] == 2) cam->state=ACQUIRE ; 
  if(dialog->radioset[2] == 1) cam->continuous=1; 
  if(dialog->radioset[2] == 2) cam->continuous=0; 

  strcpy(cam->name, dialog->answer[1][0]);
  cam->camera_id  = *dialog->l[1]->selection;
  g.camera_id     = cam->camera_id;
  cam->w          = atoi(dialog->answer[2] [0]);
  cam->h          = atoi(dialog->answer[3] [0]);
  cam->scale      = atoi(dialog->answer[4] [0]);
  cam->fps        = atoi(dialog->answer[5] [0]);
  cam->brightness = atoi(dialog->answer[6] [0]);
  cam->contrast   = atoi(dialog->answer[7] [0]);
  cam->black      = atoi(dialog->answer[8] [0]);
  cam->white      = atoi(dialog->answer[9] [0]);
  cam->hue        = atoi(dialog->answer[10][0]);
  cam->saturation = atoi(dialog->answer[11][0]);
  cam->depth      = atoi(dialog->answer[12][0]);
  cam->bpp        = cam->depth*8;
  strcpy(tempstring, cam->post_processing_command);
  strcpy(cam->post_processing_command, dialog->answer[13][1]);
  if(strcmp(cam->post_processing_command, tempstring)) cam->command_is_modified=1;
  if(dialog->boxset[13]) cam->want_processing = 1; else cam->want_processing = 0;
}


//--------------------------------------------------------------------------//
//  update_camera_dialog                                                    //
//--------------------------------------------------------------------------//
void update_camera_dialog(CameraSettings *cam)
{
  int i;
  Widget www;
  Dialog *dialog = cam->dialog;
  if(dialog==NULL) return;

  //// Radio boxes
  if(cam->state==INITIALIZING) dialog->radioset[1] = 1; 
  if(cam->state==ACQUIRE)      dialog->radioset[1] = 2; 
  dialog->radioset[0] = cam->acq_mode;
  if(cam->continuous) dialog->radioset[2] = 1; else  dialog->radioset[2] = 2;
  for(i=0;i<dialog->noofradios;i++)
  {   www = dialog->radiowidget[i][dialog->radioset[i]];
      XmToggleButtonSetState(www, True, True); 
  }

  //// Answer boxes
  sprintf(dialog->answer[2] [0],"%d",cam->w);
  sprintf(dialog->answer[3] [0],"%d",cam->h);
  sprintf(dialog->answer[4] [0],"%d",cam->scale);
  sprintf(dialog->answer[5] [0],"%d",cam->fps);
  sprintf(dialog->answer[6] [0],"%d",cam->brightness);
  sprintf(dialog->answer[7] [0],"%d",cam->contrast);
  sprintf(dialog->answer[8] [0],"%d",cam->black);
  sprintf(dialog->answer[9] [0],"%d",cam->white);
  sprintf(dialog->answer[10][0],"%d",cam->hue);
  sprintf(dialog->answer[11][0],"%d",cam->saturation);
  sprintf(dialog->answer[12][0],"%d",cam->depth);
  strcpy( dialog->answer[13][1], cam->post_processing_command);

  for(i=1;i<dialog->noofboxes;i++)
  {  if(dialog->boxtype[i]!=TOGGLESTRING)
         set_widget_label(dialog->boxwidget[i][0], dialog->answer[i][0]);
     else
         set_widget_string(dialog->boxwidget[i][1], dialog->answer[i][1]);
  }
  XFlush(g.display);
}


//--------------------------------------------------------------------------//
//  dialog_message                                                          //
//--------------------------------------------------------------------------//
void dialog_message(Widget w, char *s)
{   
   if(w && s) set_widget_resizable_label(w, s);
}


//--------------------------------------------------------------------------//
//  set_widget_resizable_label                                              //
//--------------------------------------------------------------------------//
void set_widget_resizable_label(Widget w, char* s)
{
  XmString xms;
  //// CreateLtoR preserves \n's in string, CreateSimple doesn't
  if(!w || ! XtIsManaged(w)) return;
  xms= XmStringCreateLtoR(s, XmFONTLIST_DEFAULT_TAG);
  XtVaSetValues(w, XmNlabelString, xms, NULL);
  XmStringFree(xms);                
}


//--------------------------------------------------------------------------//
//  set_widget_label                                                        //
//--------------------------------------------------------------------------//
void set_widget_label(Widget w, char* s)
{
  XmString xms;
  if(!s) return;
  if(!w) return;
  if(!XtIsManaged(w)) return;
  XtVaSetValues(w, XmNlabelString,  xms = XmStringCreateSimple(s), NULL);  
  XtVaSetValues(w, XmNcursorPosition, strlen(s),NULL); 
  if(xms) XmStringFree(xms);                
  //// Don't set size here to prevent resizing; it will cause a long delay.
  //// Set XmNrecomputeSize to False on widget instead.
}


//--------------------------------------------------------------------------//
//  set_widget_string                                                       //
//--------------------------------------------------------------------------//
void set_widget_string(Widget w, char* s)
{
  XtVaSetValues(w, XmNvalue, s, NULL);  
}


//--------------------------------------------------------------------------//
//  set_widget_value                                                        //
//--------------------------------------------------------------------------//
void set_widget_value(Widget w, int value)
{
  char tempstring[64];
  sprintf(tempstring, "%d", value);
  XtVaSetValues(w, XmNvalue, tempstring, NULL);  
  set_widget_label(w, tempstring);
}


//--------------------------------------------------------------------------//
//  set_widget_double                                                       //
//--------------------------------------------------------------------------//
void set_widget_double(Widget w, double value)
{
  char tempstring[64];
  sprintf(tempstring, "%g", value);
  XtVaSetValues(w, XmNvalue, tempstring, NULL);  
  set_widget_label(w, tempstring);
}


//--------------------------------------------------------------------------//
//  unset_radio                                                             //
//--------------------------------------------------------------------------//
void unset_radio(Dialog *a, int radio)
{
  int j;
  ////  a->radioset[radio] = -1; // don't use
  ////  Last param is false so it doesn't call togglecb. 
  for(j=1; j<a->radiono[radio]; j++)
       XmToggleButtonSetState(a->radiowidget[radio][j], False, False);
}


//--------------------------------------------------------------------------//
//  set_widget_radio                                                        //
//--------------------------------------------------------------------------//
void set_widget_radio(Dialog *a, int radio, int value)
{
  int j;
  a->radioset[radio] = value;
  ////  Last param is false so it doesn't call togglecb.
  for(j=1; j<a->radiono[radio]; j++)
       XmToggleButtonSetState(a->radiowidget[radio][j], j==value, False);
}


//--------------------------------------------------------------------------//
//  set_widget_toggle                                                       //
//--------------------------------------------------------------------------//
void set_widget_toggle(Widget w, int value)
{
   XtVaSetValues(w, XmNset, value, NULL);  
}


//--------------------------------------------------------------------------//
//  camera_load_defaults                                                    //
//--------------------------------------------------------------------------//
void camera_load_defaults(listinfo *l)
{
   CameraSettings *cam = (CameraSettings *)l->ptr[1];
   strcpy(cam->name, l->info[*l->selection]);
   camera_read_defaults(cam);
   update_camera_dialog(cam);
}


//--------------------------------------------------------------------------//
//  camera_read_defaults                                                    //
//  get default parameters for shell, width, and height from cam->name      //
//  (e.g., 'quickcam') - for parameters that are only needed once           //
//--------------------------------------------------------------------------//
void camera_read_defaults(CameraSettings *cam)
{
   char name[128];
   int value;
   char tempstring[128];
   char cameraname[FILENAMELENGTH];
   FILE *fp;

   sprintf(cameraname,"%s/%s", g.helpdir, cam->name);
   if((fp = fopen(cameraname,"rt"))==NULL)   
   {    sprintf(cameraname,"%s/%s", g.homedir, cam->name);
        if((fp = fopen(cameraname,"rt"))==NULL)   
        {    sprintf(cameraname,"%s", cam->name);
             if((fp = fopen(cameraname,"rt"))==NULL)   
             {   error_message(cameraname, errno);
                 sprintf(tempstring,"Can't open camera configuration file\n\
(should be %s/%s)",g.helpdir, cam->name);
                 message(tempstring, WARNING);
                 return;
             }
        }
   }
   while(!feof(fp))
   {    fgets(tempstring, 80, fp);
        if(feof(fp)) break;
        if(!strlen(tempstring) || tempstring[0]=='#') continue;
        sscanf(tempstring,"%s %d",name, &value);
        if(!strlen(name)) continue;     
        if(!strcmp(name,"default_shell")) { cam->shell = value; continue;}
        if(!strcmp(name,"default_width")) { cam->w = value; continue;}
        if(!strcmp(name,"default_height")){ cam->h = value; continue;}
        if(!strcmp(name,"default_scale")) { cam->scale = value; continue;}
        if(!strcmp(name,"default_depth")) { cam->depth = value; continue;}
        if(!strcmp(name,"default_post_command") && !cam->command_is_modified) 
        {   strcpy(cam->post_processing_command, tempstring+1+strlen(name)); 
            remove_trailing_junk(cam->post_processing_command);
            continue; 
        }
   }
   fclose(fp);
}



//--------------------------------------------------------------------------//
//  compose_camera_command - read camera config file into cam struct        //
//  (e.g., 'quickcam')                                                      //
//--------------------------------------------------------------------------//
int compose_camera_command(CameraSettings *cam)
{
   char name[128];
   char value[128];
   char tempstring[FILENAMELENGTH];
   char cameraname[FILENAMELENGTH];
   char tempfile[FILENAMELENGTH];
   int k,space,ret=0;
   cam->argno = 0;
   FILE *fp;
   printstatus(cam->state);

   sprintf(cameraname,"%s/%s",g.helpdir,cam->name);
   if((fp = fopen(cameraname,"rt"))==NULL)   
   {    sprintf(cameraname,"%s/%s",g.homedir,cam->name);
        if((fp = fopen(cameraname,"rt"))==NULL)   
        {    sprintf(cameraname,"%s",cam->name);
             if((fp = fopen(cameraname,"rt"))==NULL)   
             {   error_message(cameraname, errno);
                 sprintf(tempstring,"Can't open camera configuration file\n(should be %s/%s)",g.helpdir,cam->name);
                 message(tempstring,WARNING);
                 return NOTFOUND;
             }
        }
   }
   while(!feof(fp))
   {    fgets(name, 80, fp);
        if(feof(fp)) break;
        if(!strlen(name) || name[0]=='#') continue;
        space = strcspn(name," ");                   // Position of 1st space

        // Can't have this in intel 64 bit
        //ret = (int)index(name, '\n') - (int)name; // Position of final \n
        ret = strlen(name);
        for(k=0; k<=(int)strlen(name); k++) if(name[k]=='\n') ret = k; // Position of final \n

        ret = min((int)strlen(name), ret);
        name[space]=0;
        name[ret]=0;
        strcpy(value, name+space+1);
        if(!strlen(name) || (!strlen(value))) continue;     
        if(!strcmp(name,"driver")){ add_cam_option(cam, value); continue;}
        if(!strcmp(name,"defaults")){ strcpy(cam->defaults, value); continue;}
        if(!strcmp(name,"width")) { add_cam_option(cam, value, cam->w); continue;}
        if(!strcmp(name,"height")){ add_cam_option(cam, value, cam->h); continue;}
        if(!strcmp(name,"other")) { add_cam_option(cam, value); continue;}
        if(!strcmp(name,"scale")) { add_cam_option(cam, value, cam->scale); continue;}
        if(cam->state!=INITIALIZING)
        {  if(!strcmp(name,"brightness")) 
                                     { add_cam_option(cam, value, cam->brightness); continue;}
           if(!strcmp(name,"contrast")) 
                                     { add_cam_option(cam, value, cam->contrast); continue;}
           if(!strcmp(name,"black")) { add_cam_option(cam, value, cam->black); continue;}
           if(!strcmp(name,"white")) { add_cam_option(cam, value, cam->white); continue;}
           if(!strcmp(name,"hue"))   { add_cam_option(cam, value, cam->hue); continue;}
           if(!strcmp(name,"saturation")) 
                                     { add_cam_option(cam, value, cam->saturation); continue;}
        }
        if(cam->state==INITIALIZING)
	{  if(!strcmp(name,"initialize")){ add_cam_option(cam, value); continue;}
        }else
	{  if(!strcmp(name,"normal"))    { add_cam_option(cam, value); continue;}
        }
        if(!strcmp(name,"extra")) { add_cam_option(cam, value); continue;}
    }
    fclose(fp);
    sprintf(tempfile, "> %s/fifi", g.homedir);
    add_cam_option(cam, tempfile);
    cam->args[cam->argno] = new char[64];
    cam->args[cam->argno][0]=0;
    cam->argno++;
    return OK;
}
