//-----------------------------------------------------------------------//
//  xmtnimaged.h                                                         //
//  Latest revision: 03-12-1999                                          //
//  Copyright (C) 1999 by Thomas J. Nelson                               //
//  See xmtnimage.h for Copyright Notice                                 //
//-----------------------------------------------------------------------//


#ifndef DIGITAL
////  Scanner device inquiry codes without commands 
////  (terminate with E; answer follows d and ends with V, or N if null)
const int SL_ADF_HAS_PAPER     = 25;
const int SL_ADF_OPENED        = 26;
const int SL_ADF_UNLOAD_RDY    = 27;
const int SL_ERR_MAX_DEPTH     = 256;
const int SL_ERR_DEPTH         = 257;
const int SL_ERR_CURRENT       = 259;
const int SL_ERR_OLDEST        = 261;
const int SL_PIXELS_PER_LINE   = 1024;
const int SL_BYTES_PER_LINE    = 1025;
const int SL_LINES_PER_SCAN    = 1026;
const int SL_ADF_READY         = 1027;
const int SL_PIXELS_PER_INCH   = 1028;
const int SL_OPTICAL_RES       = 1029;
const int SL_XPA_AVAILABLE     = 28;
const int SL_XPA_READY         = 30;
const int SL_SHOULD_BE         = 1034;
const int SL_XPA_STATUS        = 29;
const int SL_BYTES_IN_LASTSCAN = 1037;
const int SL_WHICH_ADF         = 1041;
const int SL_SCAN_DIRECTION    = 1047;
const int SL_AUTO_COEF         = 1046;
                          
////  Device inquiry codes returning strings
////  (terminate with E; answer follows d and contains a W or ends with N)
const int SL_DATE_CODE         =  4;
const int SL_SCAN_STATUS       =  5;
const int SL_ADF_STATUS        =  6;
const int SL_MODEL_1           =  3;
const int SL_MODEL_2           =  10;
const int SL_MODEL_3           =  9;
const int SL_MODEL_4           =  11;
const int SL_MODEL_5           =  12;
const int SL_MODEL_6           =  14;
const int SL_MODEL_7           =  13;
const int SL_MODEL_8           =  15;
const int SL_MODEL_9           =  16;

                             
////  High, low, or current inquiry codes
////  (terminate with L,H,or R; answer follows k,g, or p and ends with V or N)
const int SL_X_RESOLUTION    = 10323;
const int SL_Y_RESOLUTION    = 10324;
const int SL_X_SCALE         = 10310;
const int SL_Y_SCALE         = 10311;
const int SL_XPOS_DECI       = 10329;
const int SL_YPOS_DECI       = 10330;
const int SL_XEXT_DECI       = 10321;
const int SL_YEXT_DECI       = 10322;
const int SL_XPOS_PIXEL      = 10489;
const int SL_YPOS_PIXEL      = 10490;
const int SL_XEXT_PIXEL      = 10481;
const int SL_YEXT_PIXEL      = 10482;
const int SL_INTENSITY       = 10317;
const int SL_CONTRAST        = 10316;
const int SL_NEGATIVE        = 10314;
const int SL_MIRROR          = 10318;
const int SL_AUTO_BACKGROUND = 10307;
const int SL_DATA_TYPE       = 10325;
const int SL_BITS_PER_PIXEL  = 10312;
const int SL_PATTERN         = 10315;
const int SL_TONEMAP         = 10956;
const int SL_DOWNLOAD_TYPE   = 10309;
const int SL_DOWNLOAD        = 10328;
const int SL_YPOS_BAR        = 10471;
const int SL_TEST_LAMP       = 10477;
const int SL_TEST_SCANNER    = 10485;
const int SL_FILTER          = 10951;
const int SL_COLOR_PATTERN   = 10955;
const int SL_MATRIX          = 10965;
const int SL_ADF_UNLOAD      = 10966;
const int SL_ADF_CHANGE      = 10969;
const int SL_CALIB_Y         = 10946;
const int SL_CALIB_PARAM     = 10948;
const int SL_XPA_DISABLE     = 10953;
const int SL_LIGHT_POWER     = 10957;
const int SL_BYTEORDER       = 10947;
const int SL_CAL_MODE        = 10952;
const int SL_SPEED_MODE      = 10950;
const int SL_COMPRESSION     = 10308;
const int SL_SCAN_SPEED      = 10327;
const int SL_RECONNECT_LEVEL = 10331;
const int SL_NUM_OF_VIEWS    = 10466;
const int SL_VIEW_TYPE       = 10462;
const int SL_MEDIA_TYPE      = 10469;
const int SL_COORD_SYSTEM    = 10470;
const int SL_RESERVED1       = 10320;

#else  // for digital compiler (can't handle const int)
////  Scanner device inquiry codes without commands 
////  (terminate with E; answer follows d and ends with V, or N if null)
#define SL_ADF_HAS_PAPER      25
#define SL_ADF_OPENED         26
#define SL_ADF_UNLOAD_RDY     27
#define SL_ERR_MAX_DEPTH      256
#define SL_ERR_DEPTH          257
#define SL_ERR_CURRENT        259
#define SL_ERR_OLDEST         261
#define SL_PIXELS_PER_LINE    1024
#define SL_BYTES_PER_LINE     1025
#define SL_LINES_PER_SCAN     1026
#define SL_ADF_READY          1027
#define SL_PIXELS_PER_INCH    1028
#define SL_OPTICAL_RES        1029
#define SL_XPA_AVAILABLE      28
#define SL_XPA_READY          30
#define SL_SHOULD_BE          1034
#define SL_XPA_STATUS         29
#define SL_BYTES_IN_LASTSCAN  1037
#define SL_WHICH_ADF          1041
#define SL_SCAN_DIRECTION     1047
#define SL_AUTO_COEF          1046
                          
////  Device inquiry codes returning strings
////  (terminate with E; answer follows d and contains a W or ends with N)
#define SL_DATE_CODE           4
#define SL_SCAN_STATUS         5
#define SL_ADF_STATUS          6
#define SL_MODEL_1             3
#define SL_MODEL_2             10
#define SL_MODEL_3             9
#define SL_MODEL_4             11
#define SL_MODEL_5             12
#define SL_MODEL_6             14
#define SL_MODEL_7             13
#define SL_MODEL_8             15
#define SL_MODEL_9             16

                             
////  High, low, or current inquiry codes
////  (terminate with L,H,or R; answer follows k,g, or p and ends with V or N)
#define SL_X_RESOLUTION     10323
#define SL_Y_RESOLUTION     10324
#define SL_X_SCALE          10310
#define SL_Y_SCALE          10311
#define SL_XPOS_DECI        10329
#define SL_YPOS_DECI        10330
#define SL_XEXT_DECI        10321
#define SL_YEXT_DECI        10322
#define SL_XPOS_PIXEL       10489
#define SL_YPOS_PIXEL       10490
#define SL_XEXT_PIXEL       10481
#define SL_YEXT_PIXEL       10482
#define SL_INTENSITY        10317
#define SL_CONTRAST         10316
#define SL_NEGATIVE         10314
#define SL_MIRROR           10318
#define SL_AUTO_BACKGROUND  10307
#define SL_DATA_TYPE        10325
#define SL_BITS_PER_PIXEL   10312
#define SL_PATTERN          10315
#define SL_TONEMAP          10956
#define SL_DOWNLOAD_TYPE    10309
#define SL_DOWNLOAD         10328
#define SL_YPOS_BAR         10471
#define SL_TEST_LAMP        10477
#define SL_TEST_SCANNER     10485
#define SL_FILTER           10951
#define SL_COLOR_PATTERN    10955
#define SL_MATRIX           10965
#define SL_ADF_UNLOAD       10966
#define SL_ADF_CHANGE       10969
#define SL_CALIB_Y          10946
#define SL_CALIB_PARAM      10948
#define SL_XPA_DISABLE      10953
#define SL_LIGHT_POWER      10957
#define SL_BYTEORDER        10947
#define SL_CAL_MODE         10952
#define SL_SPEED_MODE       10950
#define SL_COMPRESSION      10308
#define SL_SCAN_SPEED       10327
#define SL_RECONNECT_LEVEL  10331
#define SL_NUM_OF_VIEWS     10466
#define SL_VIEW_TYPE        10462
#define SL_MEDIA_TYPE       10469
#define SL_COORD_SYSTEM     10470
#define SL_RESERVED1        10320

#endif // ifdef digital (end of directives for braindead compilers)

#ifndef LINUX
struct sg_header
 {
  int pack_len;    /* length of incoming packet <4096 (including header) */
  int reply_len;   /* maximum length <4096 of expected reply */
  int pack_id;     /* id number of packet */
  int result;      /* 0==ok, otherwise refer to errno codes */
  unsigned int twelve_byte:1; /* Force 12 byte command length for group 6 & 7 commands  */
  unsigned int other_flags:31;                  /* for future use */
  unsigned char sense_buffer[16]; /* used only by reads */
  /* command follows then data for command */
 };     
#endif


