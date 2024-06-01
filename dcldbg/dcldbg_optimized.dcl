$ss$_CSI[0,8]=155
$ss$_ESC[0,8]=27
$ss$_c_off=ss$_CSI+"?25l"
$ss$_c_on=ss$_CSI+"?25h"
$ss$_EL=ss$_CSI+"K"
$ss$_d="''ss$_ESC'(0q''ss$_ESC'(B"
$ss$_b=ss$_CSI+"1m"
$ss$_n=ss$_CSI+"0m"
$ss$_lst_area="''ss$_CSI'2;18r''ss$_CSI'9;1H  "
$ss$_wtc_area="''ss$_CSI'24;24r''ss$_CSI'24;1H''ss$_CSI'0K"
$ss$_frame="qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"
$ss$_say="writ sys$output"
$ss$_say ""
$ss$_say ss$_b,"DCL DEBUGGER - © 1990,1991 by Jan Holzer, 2005 by Ferry Bolhar",ss$_n
$ss$_err_unrent="%SS-W-UNRENT, Unrecognized entry - ignored. Re-enter"
$ss$_err_notfnd="%SS-I-NOTFND, Search string not found"
$ss$_err_brniif="%SS-W-BRNIIF, Branching from within an IF structure"
$ss$_err_nogosb="%SS-W-NOGOSB, No GOSUB/CALL call to return to"
$ss$_err_lnbeof="%SS-W-LNBEOF, Line is beyond the end of file - ignored"
$ss$_err_nocmdl="%SS-W-NOCMDL, No command on line - ignored"
$ss$_err_lbnfnd="%SS-W-LBNFND, Label not found - ignored"
$ss$_err_notope="%SS-F-NOTOPE, Cannot open "
$ss$_err_iniope="%SS-F-INIOPE, Cannot open initialization file "
$ss$_fil_nam=f$pars(p1,".COM",,,"SYNTAX_ONLY")-";"
$if ss$_fil_nam.eqs."" then goto FILE_NG
$ss$_rld="F"
$ss$_p1=p2
$ss$_p2=p3
$ss$_p3=p4
$ss$_p4=p5
$ss$_p5=p6
$ss$_p6=p7
$ss$_p7=p8
$DO_RELOAD:
$ss$_dclstat=1
$ss$_inexit=0
$ss$_exitstat=1
$ss$_ini_nam=f$trnl("DCLDBG$INIT")
$if ss$_ini_nam.nes.""
$then
$ss$_ini_fil=f$pars(ss$_ini_nam,"SYS$LOGIN:.COM")-";"
$if ss$_ini_fil.eqs.""
$then
$ss$_ini_fil=ss$_ini_nam
$goto INI_NG
$endi
$ss$_old_state=f$envi("KEY_STATE")
$set key/nolog/stat=ss$_state
$open/read/erro=INI_NG ss$_ini_file 'ss$_ini_fil'
$READ_LOOP1:
$read/end=END_LOOP1/erro=INI_NG ss$_ini_file ss$_ini_ln
'ss$_ini_ln'
$goto READ_LOOP1
$END_LOOP1:
$clos ss$_ini_file
$endi
$open/read/erro=FILE_NG ss$_com_file 'ss$_fil_nam'
$if $severity.ne.1 then goto FILE_NG
$p1=ss$_p1
$p2=ss$_p2
$p3=ss$_p3
$p4=ss$_p4
$p5=ss$_p5
$p6=ss$_p6
$p7=ss$_p7
$on erro then cont
$on cont then goto EXIT
$ss$_say "Loading ",ss$_fil_nam," ..."
$ss$_ln_no=0
$READ_LOOP:
$read/end=RUN_IT/erro=FILE_NG ss$_com_file ss$_cmd_ln
$ss$_ln_no=ss$_ln_no+1
$ss$_ln'ss$_ln_no'=ss$_cmd_ln
$ss$_cmd_ln=f$edit(ss$_cmd_ln,"LOWERCASE,UNCOMMENT,COMPRESS,TRIM")
$ss$_sub_lbl="F"
$if f$loca("subroutine",ss$_cmd_ln).lt.f$leng(ss$_cmd_ln)
$then
$ss$_sub_lbl="T"
$goto SUB_LABEL
$endi
$if f$loca(":",ss$_cmd_ln).ne.f$leng(ss$_cmd_ln)-1 then goto READ_LOOP
$if f$loca("$",ss$_cmd_ln).ne.0 then goto READ_LOOP
$ss$_frst_blnk=f$loca(" ",ss$_cmd_ln)
$if ss$_frst_blnk.ne.f$leng(ss$_cmd_ln).and.ss$_frst_blnk.ne.1 then goto READ_LOOP
$if f$elem(2," ",ss$_cmd_ln).nes." " then goto READ_LOOP
$SUB_LABEL:
$ss$_cmd_ln=f$edit(ss$_cmd_ln,"COLLAPSE")
$if ss$_sub_lbl
$then ss$_lbl=f$extr(1,f$loca(":",ss$_cmd_ln)-1,ss$_cmd_ln)
$else ss$_lbl=f$extr(1,f$leng(ss$_cmd_ln)-2,ss$_cmd_ln)
$endi
$ss$_lbl_is_'ss$_lbl'=ss$_ln_no
$ss$_ln_'ss$_ln_no'_is_label="T"
$goto READ_LOOP
$RUN_IT:
$clos ss$_com_file
$ss$_mx_lns=ss$_ln_no
$gosu GET_NOR_SCR
$ss$_last_ln=ss$_ln_no
$ss$_ln_no=0
$ss$_$_cmd="F"
$ss$_call_lvl=0
$ss$_gosub_lvl=0
$ss$_ret_call_ln'ss$_call_lvl'=0
$ss$_ret_gosub_ln'ss$_gosub_lvl'=0
$ss$_prv_ln=-17
$ss$_brk_pt="F"
$ss$_brk_line=0
$ss$_oldbrk_line=0
$ss$_if_lvl=0
$ss$_skp_lvl=0
$ss$_skp_to="F"
$ss$_ctrl_y="F"
$ss$_run="F"
$ss$_cnt=0
$ss$_srch="F"
$ss$_in_cll="F"
$ss$_srch_endsub="F"
$ss$_wtc="F"
$if.not.ss$_rld then ss$_say ss$_CSI,"2J",ss$_CSI,"0;0H",ss$_CSI,"?7l"
$set term/nowrap
$ss$_rld="F"
$NEXT_CMD:
$gosu GET_NEXT_CMD
$if.not.ss$_skp_to then goto SEARCH_TEST
$gosu TRAP_BRANCHES
$goto NEXT_CMD
$SEARCH_TEST:
$if.not.ss$_srch then goto AROUND_IT
$if f$loca(ss$_srch_str,ss$_cmd).eq.f$leng(ss$_cmd) then goto NEXT_CMD
$ss$_srch="F"
$AROUND_IT:
$if.not.ss$_run then gosu DISPLAY_CMD
$if ss$_brk_pt.and.ss$_cmd_ln_no.eq.ss$_brk_line.or.ss$_ctrl_y then goto BREAK
$if f$type(ss$_ln_'ss$_ln_no'_is_label).nes."" then goto NEXT_CMD
$if ss$_run.or.ss$_cnt.gt.0 then goto DO_IT
$INQUIRE:
$inqu/nopunc ss$_rsp "''ss$_prompt'"
$ss$_typ=f$edit(f$extr(0,1,ss$_rsp),"UPCASE")
$if ss$_typ.eqs."" then goto DO_IT
$if ss$_typ.eqs."S" then goto NEXT_CMD
$if ss$_typ.eqs."$" then goto CMD_IN
$if f$type(ss$_rsp).eqs."INTEGER" then goto DO_N
$if ss$_typ.eqs."G" then goto GO
$if ss$_typ.eqs."J" then goto JUMP
$if ss$_typ.eqs."P" then goto PRINT_LINE
$if ss$_typ.eqs."V" then goto VIEW_BR
$if ss$_typ.eqs."E" then goto EXAMINE
$if ss$_typ.eqs."U" then goto STEP_UP
$if ss$_typ.eqs."D" then goto STEP_DOWN
$if ss$_typ.eqs."R" then goto REPAINT
$if ss$_typ.eqs."F" then goto FIND_STRING
$if ss$_typ.eqs."B" then goto SET_BREAK_PT
$if ss$_typ.eqs."C" then goto CLEAR_BREAK_PT
$if ss$_typ.eqs."L" then goto RELOAD
$if ss$_typ.eqs."W" then goto WATCH
$if ss$_typ.eqs."Q".or.ss$_typ.eqs."X" then goto EXIT
$if ss$_typ.eqs."H".or.ss$_typ.eqs."?" then goto HELP
$ss$_say ss$_err_unrent
$goto INQUIRE
$DO_IT:
$gosu TRAP_BRANCHES
$'ss$_cmd'
$if.not.ss$_inexit
$then ss$_dclstat=$status
$else
$ss$_dclstat=ss$_exitstat
$ss$_inexit=0
$endi
$if ss$_cnt.gt.0 then ss$_cnt=ss$_cnt-1
$if.not.ss$_run.and.ss$_wtc then gosu DO_WATCH
$goto NEXT_CMD
$DO_N:
$ss$_cnt=f$inte(ss$_rsp)
$if ss$_cnt.gt.0 then goto DO_IT
$ss$_say ss$_err_unrent
$goto INQUIRE
$CMD_IN:
$ss$_save_cmd=ss$_cmd
$ss$_cmd=f$extr(1,f$leng(ss$_rsp)-1,ss$_rsp)
$ss$_cmd=f$edit(ss$_cmd,"LOWERCASE,UNCOMMENT,TRIM")
$gosu TRAP_BRANCHES
$ss$_$_cmd="T"
$ss$_brnch="F"
$'ss$_cmd'
$if.not.ss$_inexit
$then ss$_dclstat=$status
$else ss$_inexit=0
$endi
$if ss$_wtc then gosu DO_WATCH
$ss$_$_cmd="F"
$if ss$_brnch then goto NEXT_CMD
$ss$_cmd=ss$_save_cmd
$goto INQUIRE
$VIEW_BR:
$if ss$_brk_pt
$then ss$_say f$fao("Breakpoint at %!SW",ss$_brk_line)
$else ss$_say "No breakpoint set"
$endi
$goto INQUIRE
$JUMP:
$ss$_oldbrk_line=0
$if ss$_brk_pt then ss$_oldbrk_line=ss$_brk_line
$ss$_brk_pt="T"
$ss$_brk_line=ss$_ln_no+1
$GO:
$ss$_run="T"
$ss$_say ss$_go_output
$goto DO_IT
$STEP_UP:
$ss$_offset=f$elem(1," ",ss$_rsp)
$if ss$_offset.eq.0 then ss$_offset=1
$ss$_ln_no=ss$_ln_no-f$inte(ss$_offset)-1
$if ss$_ln_no.lt.1 then ss$_ln_no=0
$goto NEXT_CMD
$STEP_DOWN:
$ss$_offset=f$elem(1," ",ss$_rsp)
$if ss$_offset.eq.0 then ss$_offset=1
$ss$_ln_no=ss$_ln_no+f$inte(ss$_offset)-1
$if ss$_ln_no.gt.ss$_last_ln then ss$_ln_no=ss$_last_ln-1
$goto NEXT_CMD
$REPAINT:
$gosu DO_REPAINT
$goto INQUIRE
$FIND_STRING:
$ss$_srch_str=f$edit(f$elem(1," ",ss$_rsp),"LOWERCASE,TRIM")
$ss$_srch="T"
$ss$_srch_line=ss$_cmd_ln_no
$goto NEXT_CMD
$PRINT_LINE:
$ss$_loc_prm=f$elem(1," ",ss$_rsp)
$gosu GET_LINE_NO
$if ss$_lbl_ln_no.eq.0 then goto INQUIRE
$ss$_say f$stri(ss$_lbl_ln_no)+": "+ss$_ln'ss$_lbl_ln_no'
$goto INQUIRE
$SET_BREAK_PT:
$ss$_loc_prm=f$elem(1," ",ss$_rsp)
$gosu GET_LINE_NO
$if ss$_lbl_ln_no.eq.0 then goto INQUIRE
$ss$_brk_pt="T"
$ss$_brk_line=ss$_lbl_ln_no
$goto INQUIRE
$CLEAR_BREAK_PT:
$ss$_brk_pt="F"
$goto INQUIRE
$BREAK:
$ss$_run="F"
$ss$_ctrl_y="F"
$ss$_cnt=0
$if ss$_oldbrk_line.ne.0
$then ss$_brk_line=ss$_oldbrk_line
$else ss$_brk_pt="F"
$endi
$if f$type(ss$_ln_'ss$_ln_no'_is_label).nes."" then goto NEXT_CMD
$goto REPAINT
$END:
$if.not.ss$_srch then goto EXIT
$ss$_say ss$_err_notfnd
$ss$_ln_no=ss$_srch_line-1
$ss$_srch="F"
$goto NEXT_CMD
$EXIT:
$ss$_say ss$_c_off,ss$_CSI,"1;24r",ss$_CSI,"24;1H",ss$_c_on
$set term/wrap
$set cont=(y,t)
$if f$type(ss$_old_state).nes."" then set key/nolog/stat='ss$_old_state'
$exit 'ss$_exitstat'
$HELP:
$ss$_say " ''ss$_b'Enter/CR''ss$_n' (step), ''ss$_b's''ss$_n'kip it, "+-
      "''ss$_b'u''ss$_n'p [<n>], ''ss$_b'd''ss$_n'own [<n>], "+-
      "''ss$_b'$''ss$_n'<DCL command>, ''ss$_b'<n>''ss$_n' (n steps)"
$ss$_say " e''ss$_b'x''ss$_n'it, ''ss$_b'q''ss$_n'uit, "+-
      "''ss$_b'g''ss$_n'o, ''ss$_b'e''ss$_n'xamine <symbol/logical>, "+-
      "''ss$_b'b''ss$_n'reak %<line>/<label>, ''ss$_b'c''ss$_n'lear break"
$ss$_say " ''ss$_b'w''ss$_n'atch [<symbol/logical>], "+-
      "''ss$_b'f''ss$_n'ind <string>, ''ss$_B'j''ss$_n'ump, "+-
      "re''ss$_b'l''ss$_n'oad, ''ss$_B'r''ss$_n'efresh, ''ss$_b'h''ss$_n'elp or ''ss$_b'?''ss$_n'"
$goto INQUIRE
$EXAMINE:
$ss$_var=f$extr(1,f$leng(ss$_rsp)-1,ss$_rsp)
$ss$_var=f$edit(ss$_var,"LOWERCASE,UNCOMMENT,TRIM")
$if ss$_var.eqs."" then goto INQUIRE
$if f$trnl("''ss$_var'","LNM$DCL_LOGICAL").eqs.""
$then show symb 'ss$_var'
$else show logi 'ss$_var'
$endi
$goto INQUIRE
$WATCH:
$ss$_wtc_nam=f$extr(1,f$leng(ss$_rsp)-1,ss$_rsp)
$ss$_wtc_nam=f$edit(ss$_wtc_nam,"LOWERCASE,UNCOMMENT,TRIM")
$if ss$_wtc_nam.eqs.""
$then
$gosu GET_NOR_SCR
$ss$_wtc="F"
$else
$gosu GET_WTC_SCR
$ss$_wtc="T"
$endi
$gosu DO_REPAINT
$goto INQUIRE
$DO_WATCH:
$ss$_say ss$_wtc_area
$if f$trnl("''ss$_wtc_nam'","LNM$DCL_LOGICAL").eqs.""
$then show symb 'ss$_wtc_nam'
$else show logi 'ss$_wtc_nam'
$endi
$ss$_say ss$_ctrl_area
$retu
$RELOAD:
$ss$_rld="T"
$goto DO_RELOAD
$GET_NOR_SCR:
$ss$_ctrl_area="''ss$_CSI'9;1H''ss$_b'->''ss$_n'''ss$_CSI'20;24r''ss$_CSI'24-1;1H"
$ss$_go_output="''ss$_CSI'20;24r''ss$_CSI'24;1H''ss$_CSI'1A"
$ss$_prompt="''ss$_CSI'24;1HDCLDBG> ''ss$_CSI'0K"
$ss$_scr="''ss$_CSI'1;1H''ss$_b'''ss$_d' SOURCE: ''ss$_fil_nam' "+-
    "''ss$_ESC'(0''f$extr(1,56-f$leng(ss$_fil_nam),ss$_frame)'''ss$_ESC'(B"+-
    " lines:"+f$fao("!4SL",ss$_mx_lns)+" ''ss$_d'''ss$_CSI'19;1H"+-
    "''ss$_d' PROMPT ''ss$_d'errors''ss$_d'program''ss$_d'input''ss$_d'"+-
    "output''ss$_ESC(0''f$extr(1,43,ss$_frame)'''ss$_ESC(B''ss$_n'"
$retu
$GET_WTC_SCR:
$ss$_ctrl_area="''ss$_CSI'9;1H''ss$_b'->''ss$_n'''ss$_CSI'20;22r''ss$_CSI'22-1;1H"
$ss$_go_output="''ss$_CSI'20;22r''ss$_CSI'22;1H''ss$_CSI'1A"
$ss$_prompt="''ss$_CSI'22;1HDCLDBG> ''ss$_CSI'0K"
$ss$_scr="''ss$_CSI'1;1H''ss$_b'''ss$_d' SOURCE: ''ss$_fil_nam' "+-
    "''ss$_ESC'(0''f$extr(1,56-f$leng(ss$_fil_nam),ss$_frame)'''ss$_ESC'(B"+-
    " lines:"+f$fao("!4SL",ss$_mx_lns)+" ''ss$_d'"+-
    "''ss$_CSI'19;1H''ss$_d' PROMPT ''ss$_d'errors''ss$_d'program''ss$_d'"+-
    "input''ss$_d'output''ss$_ESC'(0''f$extr(1,43,ss$_frame)'''ss$_ESC'(B"+-
    "''ss$_CSI'23;1H''ss$_d' WATCH ''ss$_d' ''ss$_wtc_nam' "+-
    "''ss$_ESC'(0''f$extr(1,69-f$leng(ss$_wtc_nam),ss$_frame)'''ss$_ESC'(B''ss$_n'"
$retu
$GET_NEXT_CMD:
$ss$_cmd=""
$ss$_cmd_ln_no=ss$_ln_no+1
$CMD_LOOP:
$ss$_goto_cmd_loop="F"
$ss$_ln_no=ss$_ln_no+1
$if ss$_ln_no.gt.ss$_last_ln then goto END
$ss$_cmd_ln=ss$_ln'ss$_ln_no'
$ss$_tmp_line=f$edit(ss$_cmd_ln,"LOWERCASE,UNCOMMENT,COMPRESS,TRIM")
$if ss$_srch_endsub
$then
$ss$_endsub=f$loca("endsubroutine",ss$_tmp_line).ne.f$leng(ss$_tmp_line)
$if ss$_endsub then ss$_srch_endsub="F"
$ss$_goto_cmd_loop="T"
$else
$ss$_sub=f$loca(" subroutine",ss$_tmp_line).ne.f$leng(ss$_tmp_line).and..not.ss$_in_cll
$if ss$_sub
$then
$ss$_srch_endsub="T"
$ss$_goto_cmd_loop="T"
$endi
$endi
$if ss$_goto_cmd_loop then goto CMD_LOOP
$if f$loca("$",ss$_cmd_ln).eq.0 then ss$_cmd_ln=f$extr(1,f$leng(ss$_cmd_ln)-1,ss$_cmd_ln)
$ss$_cmd_ln=f$edit(ss$_cmd_ln,"LOWERCASE,UNCOMMENT,COMPRESS,TRIM")
$ss$_cmd=ss$_cmd+ss$_cmd_ln
$if f$extr(f$leng(ss$_cmd_ln)-1,1,ss$_cmd_ln).nes."-" then goto GOT_CMD
$ss$_cmd=f$extr(0,f$leng(ss$_cmd)-1,ss$_cmd)
$goto CMD_LOOP
$GOT_CMD:
$if ss$_cmd.eqs."" then goto GET_NEXT_CMD
$retu
$TRAP_BRANCHES:
$L_22:
$ss$_len=f$leng(ss$_cmd)
$ss$_spos=f$loca("$status",ss$_cmd)
$if ss$_spos.eq.ss$_len then goto E_22
$ss$_cmd=f$extr(0,ss$_spos,ss$_cmd)+"ss$_dclstat"+f$extr(ss$_spos+7,ss$_len,ss$_cmd)
$goto L_22
$E_22:
$if(f$loca("if ",ss$_cmd).eq.0.or.f$loca("if(",ss$_cmd).eq.0.or.-
    f$loca("if.",ss$_cmd).eq.0).and.f$loca(" then ",ss$_cmd).eq.ss$_len then goto STRUCT_IF
$if ss$_cmd.eqs."then" then goto STRUCT_THEN
$if ss$_cmd.eqs."else" then goto STRUCT_ELSE
$if ss$_cmd.eqs."endif" then goto STRUCT_ENDIF
$if ss$_skp_to then retu
$ss$_goto=f$loca("goto ",ss$_cmd)
$if ss$_goto.lt.ss$_len then goto REPLACE_GOTO
$ss$_gsb=f$loca("gosub ",ss$_cmd)
$if ss$_gsb.lt.ss$_len then goto REPLACE_GOSUB
$ss$_return=f$loca("return",ss$_cmd)
$if ss$_return.lt.ss$_len then goto REPLACE_RETURN
$ss$_cll=f$loca("call ",ss$_cmd)
$if ss$_cll.lt.ss$_len then goto REPLACE_CALL
$ss$_exit=f$loca("exit",ss$_cmd)
$if ss$_exit.lt.ss$_len
$then if ss$_in_cll
$then goto REPLACE_CALL_EXIT
$else goto REPLACE_EXIT
$endi
$endi
$ss$_eof=f$loca("/end=",ss$_cmd)
$if ss$_eof.lt.ss$_len then goto REPLACE_EOF
$LOOKFOR_ERROR:
$ss$_len=f$leng(ss$_cmd)
$ss$_error=f$loca("/error=",ss$_cmd)
$if ss$_error.lt.ss$_len then goto REPLACE_ERROR
$retu
$STRUCT_IF:
$if ss$_skp_to
$then ss$_skp_lvl=ss$_skp_lvl+1
$else
$ss$_if_lvl=ss$_if_lvl+1
$ss$_'ss$_if_lvl'_test="F"
$ss$_cmd=ss$_cmd+" then ss$_''ss$_if_lvl'_test=""T"""
$endi
$retu
$STRUCT_THEN:
$if.not.ss$_skp_to
$then if.not.ss$_'ss$_if_lvl'_test
$then ss$_skp_to="T"
$endi
$ss$_cmd=""
$endi
$retu
$STRUCT_ELSE:
$if ss$_skp_lvl.eq.0
$then if.not.ss$_'ss$_if_lvl'_test
$then ss$_skp_to="F"
$else ss$_skp_to="T"
$endi
$ss$_cmd=""
$endi
$retu
$STRUCT_ENDIF:
$if ss$_skp_lvl.gt.0
$then ss$_skp_lvl=ss$_skp_lvl-1
$else
$ss$_skp_to="F"
$ss$_if_lvl=ss$_if_lvl-1
$ss$_cmd=""
$endi
$retu
$REPLACE_GOTO:
$ss$_beg=f$extr(0,ss$_goto,ss$_cmd)
$ss$_loc_prm=f$extr(ss$_goto+5,(ss$_len-ss$_goto-5),ss$_cmd)
$ss$_cmd=ss$_beg+" GOSUB GOTO_SUB "
$retu
$REPLACE_GOSUB:
$ss$_beg=f$extr(0,ss$_gsb,ss$_cmd)
$ss$_loc_prm=f$extr(ss$_gsb+6,(ss$_len-ss$_gsb-6),ss$_cmd)
$ss$_cmd=ss$_beg+" GOSUB GOSUB_SUB "
$retu
$REPLACE_RETURN:
$ss$_pos=f$loca("return",ss$_cmd)
$if ss$_pos.ne.ss$_len
$then
$ss$_exitstat=f$inte(f$extr(ss$_pos+7,ss$_len,ss$_cmd))
$if ss$_exitstat.eq.0 then ss$_exitstat=1
$ss$_inexit=1
$endi
$ss$_beg=f$extr(0,ss$_return,ss$_cmd)
$ss$_cmd=ss$_beg+" GOSUB RETURN_SUB "
$retu
$REPLACE_CALL:
$ss$_cmd_sav=ss$_cmd
$ss$_beg=f$extr(0,ss$_cll,ss$_cmd)
$ss$_loc_prm=f$elem(0," ",f$extr(ss$_cll+5,ss$_len-ss$_cll-5,ss$_cmd))
$ss$_cmd=ss$_beg+" GOSUB CALL_SUB "
$ss$_c=1
$PARAM_LOOP_1:
$ss$_tmp=p'ss$_c'
$ss$_p'ss$_c'sav="''ss$_tmp'"
$ss$_tmp=f$elem(ss$_c," ",f$extr(ss$_cll+5,f$leng(ss$_cmd_sav),ss$_cmd_sav))
$ss$_prm_cnt=ss$_c
$if ss$_tmp.eqs." " then retu
$p'ss$_c'='ss$_tmp'
$ss$_c=ss$_c+1
$if ss$_c.lt.8 then goto PARAM_LOOP_1
$retu
$REPLACE_CALL_EXIT:
$ss$_pos=f$loca("exit",ss$_cmd)
$ss$_exitstat=f$inte(f$extr(ss$_pos+5,ss$_len,ss$_cmd))
$if ss$_exitstat.eq.0 then ss$_exitstat=1
$ss$_inexit=1
$ss$_beg=f$extr(0,ss$_exit,ss$_cmd)
$ss$_cmd=ss$_beg+" GOSUB CALL_EXIT_SUB "
$retu
$REPLACE_EXIT:
$ss$_pos=f$loca("exit",ss$_cmd)
$if ss$_pos.ne.ss$_len then ss$_exitstat=f$inte(f$extr(ss$_pos+5,ss$_len,ss$_cmd))
$if ss$_exitstat.eq.0 then ss$_exitstat=1
$REPLACE_EXIT_1:
$ss$_beg=f$extr(0,ss$_exit,ss$_cmd)
$ss$_cmd=ss$_beg+" GOTO EXIT "
$retu
$REPLACE_EOF:
$ss$_beg=f$extr(0,ss$_eof+5,ss$_cmd)
$ss$_end=f$extr(ss$_eof+5,ss$_len-ss$_eof-5,ss$_cmd)
$ss$_rst=f$loca("/",ss$_end)
$if ss$_rst.eq.f$leng(ss$_end) then ss$_rst=f$loca(" ",ss$_end)
$ss$_end_prm=f$extr(0,ss$_rst,ss$_end)
$ss$_end=f$extr(ss$_rst,f$leng(ss$_end)-ss$_rst,ss$_end)
$ss$_cmd=ss$_beg+"END_TRAP"+ss$_end
$goto LOOKFOR_ERROR
$REPLACE_ERROR:
$ss$_beg=f$extr(0,ss$_error+7,ss$_cmd)
$ss$_end=f$extr(ss$_error+7,ss$_len-ss$_error-7,ss$_cmd)
$ss$_rst=f$loca("/",ss$_end)
$if ss$_rst.eq.f$leng(ss$_end) then ss$_rst=f$loca(" ",ss$_end)
$ss$_error_prm=f$extr(0,ss$_rst,ss$_end)
$ss$_end=f$extr(ss$_rst,f$leng(ss$_end)-ss$_rst,ss$_end)
$ss$_cmd=ss$_beg+"ERR_TRAP"+ss$_end
$retu
$GOTO_SUB:
$gosu GET_LINE_NO
$if ss$_lbl_ln_no.eq.0 then retu
$if ss$_if_lvl.gt.0
$then
$ss$_say ss$_err_brniif
$ss$_if_lvl=0
$endi
$ss$_ln_no=ss$_lbl_ln_no-1
$ss$_brnch="T"
$retu
$GOSUB_SUB:
$gosu GET_LINE_NO
$if ss$_lbl_ln_no.eq.0 then retu
$ss$_ret_gosub_ln'ss$_gosub_lvl'=ss$_ln_no+1
$if ss$_$_cmd then ss$_ret_gosub_ln'ss$_gosub_lvl'=ss$_ln_no
$ss$_gosub_lvl=ss$_gosub_lvl+1
$ss$_ln_no=ss$_lbl_ln_no-1
$ss$_brnch="T"
$retu
$RETURN_SUB:
$if ss$_ret_gosub_ln0.eq.0 then goto NESTING_ERROR
$ss$_gosub_lvl=ss$_gosub_lvl-1
$ss$_temp=ss$_ret_gosub_ln'ss$_gosub_lvl'
$ss$_ln_no=ss$_temp-1
$ss$_ret_gosub_ln'ss$_gosub_lvl'=0
$ss$_brnch="T"
$retu
$CALL_SUB:
$gosu GET_LINE_NO
$if ss$_lbl_ln_no.eq.0 then retu
$ss$_ret_call_ln'ss$_call_lvl'=ss$_ln_no+1
$if ss$_$_cmd then ss$_ret_call_ln'ss$_call_lvl'=ss$_ln_no
$ss$_call_lvl=ss$_call_lvl+1
$ss$_ln_no=ss$_lbl_ln_no-1
$ss$_brnch="T"
$ss$_in_cll="T"
$retu
$CALL_EXIT_SUB:
$if ss$_ret_call_ln0.eq.0 then goto NESTING_ERROR
$ss$_call_lvl=ss$_call_lvl-1
$ss$_temp=ss$_ret_call_ln'ss$_call_lvl'
$ss$_ln_no=ss$_temp-1
$ss$_ret_call_ln'ss$_call_lvl'=0
$ss$_brnch="T"
$if ss$_call_lvl.eq.0
$then
$ss$_in_cll="F"
$goto REPLACE_EXIT_1
$endi
$ss$_c=1
$PARAM_LOOP_2:
$ss$_tmp=ss$_p'ss$_c'sav
$p'ss$_c'="''ss$_tmp'"
$ss$_c=ss$_c+1
$if ss$_c.lt.ss$_prm_cnt then goto PARAM_LOOP_2
$ss$_prm_cnt=0
$retu
$NESTING_ERROR:
$ss$_say ss$_err_nogosb
$retu
$END_TRAP:
$ss$_dclstat=$status
$ss$_loc_prm=ss$_end_prm
$gosu GOTO_SUB
$goto NEXT_CMD
$ERR_TRAP:
$ss$_dclstat=$status
$ss$_loc_prm=ss$_error_prm
$gosu GOTO_SUB
$goto NEXT_CMD
$GET_LINE_NO:
$if f$loca("%",ss$_loc_prm).eq.0 then goto LINE_NUM
$ss$_lbl=f$edit(ss$_loc_prm,"LOWERCASE")
$gosu FIND_LABEL
$retu
$LINE_NUM:
$ss$_lbl_ln_no=f$extr(1,f$leng(ss$_loc_prm)-1,ss$_loc_prm)
$if f$type(ss$_ln'ss$_lbl_ln_no').eqs."" then goto NO_EXIST
$if f$extr(1,1,f$edit(ss$_ln'ss$_lbl_ln_no',"TRIM,COLLAPSE")).eqs."!" then goto COMMENT_LINE
$if ss$_lbl_ln_no.le.ss$_last_ln then retu
$NO_EXIST:
$ss$_say ss$_err_lnbeof
$ss$_lbl_ln_no=0
$retu
$COMMENT_LINE:
$ss$_say ss$_err_nocmdl
$ss$_lbl_ln_no=0
$retu
$FIND_LABEL:
$if f$type(ss$_lbl_is_'ss$_lbl').eqs."" then goto LABEL_NOT_FOUND
$ss$_lbl_ln_no=ss$_lbl_is_'ss$_lbl'
$retu
$LABEL_NOT_FOUND:
$ss$_say ss$_err_lbnfnd
$ss$_lbl_ln_no=0
$retu
$DISPLAY_CMD:
$if ss$_ln_no.gt.ss$_prv_ln.and.ss$_ln_no.lt.ss$_prv_ln+17 then goto SCROLL_UP
$if ss$_ln_no.lt.ss$_prv_ln.and.ss$_ln_no.gt.ss$_prv_ln-17 then goto SCROLL_DOWN
$if ss$_ln_no.eq.ss$_prv_ln then retu
$DO_REPAINT:
$ss$_say ss$_CSI,"2J"
$ss$_say/symbol ss$_scr
$ss$_say ss$_c_off,ss$_lst_area
$ss$_dsp_ln=ss$_ln_no-8
$ss$_curs_ctl=ss$_CSI+"2;1H"
$REPAINT_LOOP:
$ss$_dsp_ln=ss$_dsp_ln+1
$ss$_in_rng=ss$_dsp_ln.gt.0.and.ss$_dsp_ln.le.ss$_last_ln
$if ss$_in_rng then ss$_txt="   "+f$fao("!4SL:",ss$_dsp_ln)+" "+ss$_ln'ss$_dsp_ln'
$if.not.ss$_in_rng then ss$_txt=" "
$if ss$_dsp_ln.gt.ss$_ln_no+8 then goto REPAINT_DONE
$ss$_say ss$_curs_ctl,ss$_EL,ss$_txt
$ss$_curs_ctl=""
$goto REPAINT_LOOP
$REPAINT_DONE:
$ss$_say ss$_EL,ss$_txt,ss$_ctrl_area,ss$_c_on
$ss$_prv_ln=ss$_ln_no
$if ss$_wtc then gosu DO_WATCH
$retu
$SCROLL_UP:
$ss$_say ss$_c_off,ss$_lst_area,ss$_CSI,"18;1H"
$ss$_dsp_ln=ss$_prv_ln+9
$UP_LOOP:
$ss$_dsp_ln=ss$_dsp_ln+1
$ss$_in_rng=ss$_dsp_ln.le.ss$_last_ln
$if ss$_in_rng then ss$_txt="   "+f$fao("!4SL:",ss$_dsp_ln)+" "+ss$_ln'ss$_dsp_ln'
$if.not.ss$_in_rng then ss$_txt=" "
$if ss$_dsp_ln.eq.ss$_ln_no+9 then goto UP_DONE
$ss$_say ss$_EL,ss$_txt
$goto UP_LOOP
$UP_DONE:
$ss$_say ss$_EL,ss$_txt,ss$_ctrl_area,ss$_c_on
$ss$_prv_ln=ss$_ln_no
$retu
$SCROLL_DOWN:
$ss$_say ss$_c_off,ss$_lst_area,ss$_CSI,"2;1H"
$ss$_dsp_ln=ss$_prv_ln-7
$DOWN_LOOP:
$ss$_dsp_ln=ss$_dsp_ln-1
$ss$_in_rng=ss$_dsp_ln.gt.0
$if ss$_in_rng then ss$_txt="   "+f$fao("!4SL:",ss$_dsp_ln)+" "+ss$_ln'ss$_dsp_ln'
$if.not.ss$_in_rng then ss$_txt=" "
$if ss$_dsp_ln.eq.ss$_ln_no-7 then goto DOWN_DONE
$ss$_say ss$_CSI,"1A",ss$_CSI,"1L",ss$_txt
$goto DOWN_LOOP
$DOWN_DONE:
$ss$_say ss$_CSI,"1A",ss$_CSI,"1L",ss$_txt,ss$_ctrl_area,ss$_c_on
$ss$_prv_ln=ss$_ln_no
$retu
$FILE_NG:
$ss$_say ss$_err_notope,ss$_fil_nam
$exit
$INI_NG:
$ss$_say ss$_err_iniope,ss$_ini_fil
$if f$type(ss$_old_state).nes."" then set key/nolog/stat=ss$_old_state
$exit
