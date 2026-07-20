c Early-est main driver program for using the focal mechanism subroutines.
c
c 20110525 AJL - original version modified from hash_driver1.f
c
c INPUT FILES:
c
c    "hypofile" Early-est hypos.csv file
c
c    event_id n ph_assoc ph_used dist_min gap1 gap2 sigma_ot origin-time lat lon errH depth errZ T50Ex n Td n TdT50Ex WL mB n Mwp n T0 n Mwpd n Comment_text
c    1288807991280 1 26 20 2.685 94.734 154.842 0.860 2010.11.03-18:13:11.66 40.029 13.280 23.33 464.06 18.22 0.270 14 0.992 7 0.268 GREEN 5.286 19 5.035 7 -1.00 0 -9.000 0 Tyrrhenian_Sea
c
c
c    "pickfile" Early-est picks.csv file
c
c    event_id n event dist az channel stream loc time unc pol toang phase residual tot_wt dist_wt st_q_wt T50 Aref T50Ex Tdom s/n_HF s/n_BRB mB Mwp T0 Mwpd status
c    1288807991280 6 1 2.68 75.5 GE_MATE_--_BHZ BRB L 2010.11.03-18:14:16.60 0.150 -1 141.8 P -0.51 1.000 1.000 1.000 0.000 2465.129 0.000 1.605 264.693 11.738 -9.000 5.129 11.250 -9.000 OK_HF_BRB
c    1288807991280 4 1 1.94 90.2 MN_CUC_--_BHZ BRB - 2010.11.03-18:14:13.23 0.050 -1 151.2 P 0.52 0.000 1.000 1.000 617.816 1933.273 0.320 1.421 299.267 38.846 -9.000 5.138 11.100 -9.000 OK_HF_BRB
c    -1 1 -1 -1 -1 GE_IBBN_--_BHZ BRB L 2010.11.03-18:10:55.30 0.250 0 -1.0 -1 -1 -1 -1 1.000 0.000 90.059 0.000 -1.000 3.052 1.354 -9.000 -9.000 10.900 -9.000 s/n_BRB_low

c

      include 'param.inc'
      include 'rot.inc'

c variables for storing earthquake input information
      integer*8 event_id            ! event ID
      real qlat,qlon,qdep           ! location
      real qmag                     ! magnitude
      integer iyr,imon,idy,ihr,imn  ! origin time, year, month, day, hour, minute
      real qsec                     ! origin time, seconds
      real seh, sez                 ! location error, horizontal & vertical
      real rms                      ! residual travel time error
      real terr                     ! origin time error
      character*1 evtype            ! event type
      character*1 magtype           ! magnitude type
      character*1 locqual           ! location quality
c
c variables for polarity information, input to HASH subroutines
      character*20 sname(npick0)                       ! channel name, e.g. GE_MATE_--_BHZ
      integer p_pol(npick0),p_qual(npick0),spol        ! polarity pick (+/-1),  quality (0/1), and reversal (+/-1)
      real p_azi_mc(npick0,nmc0),p_the_mc(npick0,nmc0) ! azimuth and takeoff angle for each trial
      integer nmc                                      ! number of trails with different azimuth and take-off angles
      integer npol                                     ! number of polarity readings
c
c variables for set of acceptable mechanisms, output of HASH subroutines
      integer nmult                                    ! number of solutions (1 if no mulitples)
      real str_avg(5),dip_avg(5),rak_avg(5)            ! solution(s)
      integer nout2                                    ! number of acceptable mechanisms returned
      real f1norm(3,nmax0),f2norm(3,nmax0)             ! normal vectors to the fault planes
      real strike2(nmax0),dip2(nmax0),rake2(nmax0)     ! strike, dip and rake
      real var_est(2,5),var_avg(5)                     ! variance of each plane, average
      real mfrac(5),stdr(5)                            ! fraction misfit polarities, station distribution ratio
      real prob(5)                                     ! probability true mechanism is "close" to preferred solution(s)
      character*1 qual(5),mflag                        ! solution quality rating, multiple flag
c
c control parameters
      integer npolmin,max_agap,max_pgap                ! minimum polarities, maximum azimuthal & "plungal" gap
      real delmax                                      ! maximum station distance
      real dang,dang2                                  ! grid angle to sample focal sphere
      integer maxout                                   ! max number of acceptable mechanisms output
      real badfrac                                     ! assumed rate of polarity error (fraction)
      real cangle                                      ! definition of "close" == 45 degrees
c
c file names
      character*256 outfile1,outfile2,outfile3
      character*256 filename
      character*256 plfile,pickfile,hypofile
      character*2 cpolarity

c
c added AJL
      integer VERBOSE,MAX_PMECA_OUT
      integer*8 idtmp                   ! temp event ID
      integer*8 ieventid_pick           ! pick event ID
      integer event_index               ! event index (-1 if not available)
      character*64 cformat,cskip
      character*24 locflag,date_time,phase
      real fskip
      real fqual
      real fp1_norm(3),fp1_slip(3)
      real param_wt                     ! function
      real prob_wt, var_avg_wt, mfrac_wt, stdr_wt, tot_qual_wt


      VERBOSE=1
      MAX_PMECA_OUT=20999

      degrad=180./3.1415927
      rad=1./degrad

      print *,'Enter station polarity reversal file'
      read (*,*) plfile
      if (VERBOSE.eq.1) print *,plfile

      print *,
     &  'Enter name of hypos input file (Early-est hypos.csv format)'
      read (*,*) hypofile
      if (VERBOSE.eq.1) print *,hypofile
      open (14,file=hypofile,status='old')

      print *,
     &  'Enter name of picks input file (Early-est picks.csv format)'
      read (*,*) pickfile
      if (VERBOSE.eq.1) print *,pickfile
      open (12,file=pickfile,status='old')

      print *,'Enter output file name for focal mechanisms'
      read (*,*) outfile1
      if (VERBOSE.eq.1) print *,outfile1
      open (13,file=outfile1)

      print *,'Enter output file name for acceptable planes'
      read (*,*) outfile2
      if (VERBOSE.eq.1) print *,outfile2
      open (11,file=outfile2)


      print *,'Enter output file path for Early-est plot files'
      read (*,*) outfile3
      if (VERBOSE.eq.1) print *,outfile3


      print *,'Enter mininum number of polarities (e.g., 8)'
      read *,npolmin

      print *,'Enter maximum azimuthal gap (e.g., 90)'
      read *,max_agap

      print *,'Enter maximum takeoff angle gap (e.g., 60)'
      read *,max_pgap

      print *,'Enter grid angle for focal mech search, in degrees
     &  (min ',dang0,')'
      read *,dang
      dang2=max(dang0,dang) ! don't do finer than dang0

      print *,'Enter number of trials (e.g., 30)'
      read *,nmc

      print *,'Enter maxout for focal mech. output (e.g., 500)'
      read *,maxout

      print *,'Enter fraction of picks presumed bad (e.g., 0.10)'
      read *,badfrac

      print *,'Enter maximum allowed source-station distance,
     &         in km (e.g., 120)'
      read *,delmax

      print *,'Enter angle for computing mechanisms probability,
     &         in degrees (e.g., 45)'
      read *,cangle

      print *,'Enter probability threshold for multiples (e.g., 0.1)'
      read *,prob_max


20    continue  !loop over hypos

c read in earthquake location, etc                  ! Early-est hypos.csv file format
c    event_id assoc_ndx loc_seq_num ph_assoc ph_used dmin(deg) gap1(deg) gap2(deg) atten sigma_otime(sec) otime(UTC) lat(deg) lon(deg) errH(km) depth(km) errZ(km) Q T50Ex n Td(sec) n TdT50Ex WL_col mb n Mwp n T0(sec) n Mwpd n region n_sta_tot n_sta_active assoc_latency
c    1299822380917 1 0 30 26 2.816 122.727 126.190 -1.418 1.398 2011.03.11-05:46:20.92 38.088 142.743 13.74 10.16 13.26 A 3.619 19 11.280 19 40.823 WHITE 6.169 17 8.082 21 148.39 15 8.736 17 Near_East_Coast_of_Honshu,_Japan 60 60 173001741
      read (14,*,end=999) event_id,event_index,fskip,nppick,
     &  fskip,fskip,fskip,
     &  fskip,fskip,terr,date_time,qlat,qlon,seh,qdep,sez,
     &  cskip,fskip,fskip,fskip,fskip,fskip,cskip,qmag
      event_index=event_index-1
c decode date/time
      read (date_time,'(i4,1x,i2,1x,i2,1x,i2,1x,i2,1x,f5.2)')
     &  iyr,imon,idy,ihr,imn,qsec
      if (sez.eq.0.) sez=1.   ! set parameters not given in input file
      rms=terr
      nspick=-9
      evtype='G'
      magtype='X'
      locqual='X'

c set filenames for hypo
      idtmp = event_id
      idec = 0
c        if (VERBOSE.eq.1) print *,'idtmp=<',idtmp,'>','idec=<',idec,'>'
      do while (idtmp.gt.0)
        idtmp=idtmp/10
        idec=idec+1
c        if (VERBOSE.eq.1) print *,'idtmp=<',idtmp,'>','idec=<',idec,'>'
      enddo
      if (idec.eq.0) then
          cformat='(a,a,i2,a)'
      else if (idec.eq.1) then
          cformat='(a,a,i1,a)'
      else if (idec.eq.2) then
          cformat='(a,a,i2,a)'
      else if (idec.eq.3) then
          cformat='(a,a,i3,a)'
      else if (idec.eq.4) then
          cformat='(a,a,i4,a)'
      else if (idec.eq.5) then
          cformat='(a,a,i5,a)'
      else if (idec.eq.6) then
          cformat='(a,a,i6,a)'
      else if (idec.eq.7) then
          cformat='(a,a,i7,a)'
      else if (idec.eq.8) then
          cformat='(a,a,i8,a)'
      else if (idec.eq.9) then
          cformat='(a,a,i9,a)'
      else if (idec.eq.10) then
          cformat='(a,a,i10,a)'
      else if (idec.eq.11) then
          cformat='(a,a,i11,a)'
      else if (idec.eq.12) then
          cformat='(a,a,i12,a)'
      else if (idec.eq.13) then
          cformat='(a,a,i13,a)'
      else if (idec.eq.14) then
          cformat='(a,a,i14,a)'
      else
          cformat='(a,a,i15,a)'
      endif
      if (VERBOSE.eq.1) print *,'event_id=<',event_id,'>','cformat=<',cformat,'>'
      write (filename, cformat) trim(outfile3),'/hypo.',
     & event_id,'.mech.hash.preferred.meca'
      if (VERBOSE.eq.1) print *,filename
      open (15,file=filename)
      write (15,*) 'lon lat depth str dip slip st plon plat text'

      write (filename, cformat) trim(outfile3),'/hypo.',
     & event_id,'.mech.hash.preferred.polar'
      if (VERBOSE.eq.1) print *,filename
      open (16,file=filename)
      write (16,*) 'station_code azimuth take-off angle polarity'

      write (filename, cformat) trim(outfile3),'/hypo.',
     & event_id,'.mech.hash.acceptable.meca'
      if (VERBOSE.eq.1) print *,filename
      open (17,file=filename)
      write (17,*) 'lon lat depth str dip slip st plon plat text'

      write (filename, cformat) trim(outfile3),'/hypo.',
     & event_id,'.mech.hash.qml_data'
      if (VERBOSE.eq.1) print *,filename
      open (18,file=filename)

c read in polarities                 ! Early-est picks.csv file format
      k=1
      rewind(12)
      read (12,*)           ! header line
30    continue
cAJL
      p_qual(k)=0
        sthe=5      ! AJL - TODO: fixed 5 deg error!
        sazi=5      ! AJL - TODO: fixed 5 deg error!
c event_id n event dist az channel stream loc time unc pol pol_type pol_wt toang paz paz_unc paz_calc paz_wt phase residual tot_wt dist_wt st_q_wt T50 Aref Aerr T50Ex Tdom Avel Adisp s/n_HF s/n_BRBV s/n_BRBD mb Mwp T0 Mwpd status sta_corr
        read (12,*,end=40) ieventid_pick,fskip,fskip,qdist,qazi,
     &    sname(k),cskip,locflag,cskip,fskip,p_pol(k),cskip,fqual,qthe,
     &    fskip,fskip,fskip,fskip,phase
        if (ieventid_pick.ne.event_id) goto 30
        if (qdist.gt.delmax) goto 30
        if (fqual.lt.0.5) p_qual(k) = 1
        if (p_qual(k).gt.1) goto 30
        if (p_pol(k).eq.0) goto 30
        if (qthe.lt.0.0) goto 30
        if (locflag.ne.'L') goto 30
c        if (phase.ne.'P'.and.phase(1:3).ne.'PKP') goto 30
c 20170529 AJL - Ignore PKP to match current fmamp functionality
        if (phase.ne.'P') goto 30
cAJL        call CHECK_POL(plfile,sname(k),iyr,imon,idy,ihr,spol)
cAJL                          ! SCSN station polarity reversal information - ** YOU MAY NEED TO USE YOUR OWN SUBROUTINE **
        spol=1.0        ! AJL
        p_pol(k)=p_pol(k)*spol
        qthe=180.-qthe
        if (qazi.lt.0.) qazi=qazi+360.
        p_azi_mc(k,1)=qazi      ! set azimuth and takeoff angle for each trial, given uncertainty
        p_the_mc(k,1)=qthe
        do 105 nm=2,nmc
          call RAN_NORM(val)
          p_azi_mc(k,nm)=qazi+sazi*val
          call RAN_NORM(val)
          p_the_mc(k,nm)=qthe+sthe*val
105     continue
cc view polarity data
        if (VERBOSE.eq.1) then
            print *,k,' ',sname(k),phase, p_azi_mc(k,1),p_the_mc(k,1),
     &           p_azi_mc(k,2),p_the_mc(k,2),p_pol(k),p_qual(k)
        end if
        k=k+1
      goto 30
40    continue
      npol=k-1

c stop if there aren't enough polarities
      if (VERBOSE.eq.1) print *,'cid=',event_id,' npol=',npol
      if (npol.lt.npolmin) then
        str_avg(1)=999
        dip_avg(1)=99
        rak_avg(1)=999
        var_est(1,1)=99
        var_est(2,1)=99
        mfrac(1)=0.99
        qual(1)='F'
        prob(1)=0.0
        nout1=0
        nout2=0
        nmult=0
        if (VERBOSE.eq.1)
     &    print *,'Abort: not enough polarities:',
     &    ' npol=',npol,' npolmin=',npolmin
        goto 400
      end if

c determine maximum azimuthal and takeoff gap in polarity observations
c and stop if either gap is too big
      call GET_GAP(npol,p_azi_mc,p_the_mc,magap,mpgap)
      if ((magap.gt.max_agap).or.(mpgap.gt.max_pgap)) then
        str_avg(1)=999
        dip_avg(1)=99
        rak_avg(1)=999
        var_est(1,1)=99
        var_est(2,1)=99
        mfrac(1)=0.99
        qual(1)='E'
        prob(1)=0.0
        nout1=0
        nout2=0
        nmult=0
        if ((VERBOSE.eq.1).and.(magap.gt.max_agap))
     &    print *,'Abort: azimuth gap too large:',
     &    ' magap=',magap,' max_agap=',max_agap
        if ((VERBOSE.eq.1).and.(mpgap.gt.max_pgap))
     &    print *,'Abort: take-off angle gap too large:',
     &    ' mpgap=',mpgap,' max_pgap=',max_pgap
        goto 400
      end if

c determine maximum acceptable number misfit polarities
      nmismax=max(nint(npol*badfrac),2)
      nextra=max(nint(npol*badfrac*0.5),2)

c find the set of acceptable focal mechanisms for all trials
      call FOCALMC(p_azi_mc,p_the_mc,p_pol,p_qual,npol,nmc,
     &       dang2,nmax0,nextra,nmismax,nf2,strike2,dip2,
     &       rake2,f1norm,f2norm)
      nout2=min(nmax0,nf2)  ! number mechs returned from sub
      nout1=min(maxout,nf2)  ! number mechs to return

c find the probable mechanism from the set of acceptable solutions
      call MECH_PROB(nout2,f1norm,f2norm,cangle,prob_max,nmult,
     &          str_avg,dip_avg,rak_avg,prob,var_est)

      do 390 imult=1,nmult

      var_avg(imult)=(var_est(1,imult)+var_est(2,imult))/2.
      print *,'cid = ',event_id,imult,'  mech = ',
     &          str_avg(imult),dip_avg(imult),rak_avg(imult)

c find misfit for preferred solution
      call GET_MISF(npol,p_azi_mc,p_the_mc,p_pol,p_qual,str_avg(imult),
     &       dip_avg(imult),rak_avg(imult),mfrac(imult),stdr(imult))

c From: 2002__Hardebeck_Shearer__A_New_Method_for_Determining_First-Motion_Focal_Mechanisms__BSSA
c
c prob(imult)
c     The fraction of acceptable solutions that are within 30deg of the preferred solution
c     gives an estimate of how likely it is that the correct solution is within the chosen cluster.
c
c var_avg(imult)
c     The spread of the solutions is measured by the root-mean-square (RMS) angular difference
c     between the acceptable mechanisms and the preferred mechanism.
c
c mfrac(imult)
c     The reported polarity misfit for the preferred solution is computed similarly to FPFIT
c
c stdr(imult)
c     The station distribution ratio (STDR), introduced by Reasenberg and Oppenheimer (1985).
c     The STDR quantifies how the observations are spaced on the focal sphere,
c     relative to the nodal planes, with a larger STDR indicating a better distribution.
c
c     STDR is the station distribution ratio (0.0 <= STDR <= 1.0). This quantity is sensitive
c     to the distribution of the data on the focal sphere, relative to the radiation pattern.
c     When this ratio has a low value (say, STDR < 0.5), then a relatively large number of the
c     data lie near nodal planes in the solution. Such a solution is less robust than one
c     for which STDR > 0.5, and, consequently, should be scrutinized closely and possibly rejected.

c solution quality rating  ** YOU MAY WISH TO DEVELOP YOUR OWN QUALITY RATING SYSTEM **
c      if ((prob(imult).gt.0.8).and.(var_avg(imult).le.25).and.
c     &     (mfrac(imult).le.0.15).and.(stdr(imult).ge.0.5)) then
c        qual(imult)='A'
c      else if ((prob(imult).gt.0.6).and.(var_avg(imult).le.35).and.
c     &  (mfrac(imult).le.0.2).and.(stdr(imult).ge.0.4)) then
c        qual(imult)='B'
c      else if ((prob(imult).gt.0.5).and.(var_avg(imult).le.45).and.
c     &  (mfrac(imult).le.0.3).and.(stdr(imult).ge.0.3)) then
c        qual(imult)='C'
c      else
c        qual(imult)='D'
c      end if

c more balanced and continuous solution quality rating
c 20121030 AJL - added
      prob_wt = param_wt(prob(imult), 0.5, 1.0)
      var_avg_wt = 1.0 - param_wt(var_avg(imult), 0.0, 45.0)
      mfrac_wt = 1.0 - param_wt(mfrac(imult), 0.0, 0.3)
      stdr_wt = param_wt(stdr(imult), 0.3, 1.0)
      tot_qual_wt = (prob_wt + var_avg_wt + mfrac_wt + stdr_wt) / 4.0
      print *,'prob=<',prob(imult),'>',' prob_wt=<',prob_wt,'>'
      print *,'var_avg=<',var_avg(imult),'>',' var_avg_wt=<',
     &   var_avg_wt,'>'
      print *,'mfrac=<',mfrac(imult),'>',' mfrac_wt=<',mfrac_wt,'>'
      print *,'stdr=<',stdr(imult),'>',' stdr_wt=<',stdr_wt,'>'
      print *,'tot_qual_wt=<',tot_qual_wt,'>'
      if (tot_qual_wt.gt.0.75) then
        qual(imult)='A'
      else if (tot_qual_wt.gt.0.5) then
        qual(imult)='B'
      else if (tot_qual_wt.gt.0.25) then
        qual(imult)='C'
      else
        qual(imult)='D'
      end if

390   continue

400   continue

c output preferred mechanism  ** YOU MAY WISH TO CHANGE THE OUTPUT FORMAT **
      if (nmult.gt.1) then
        mflag='*'
      else
        mflag=' '
      end if
      do i=1,nmult
      write (13,411) event_id,iyr,imon,idy,ihr,imn,qsec,evtype,
     &   qmag,magtype,qlat,qlon,qdep,locqual,rms,seh,sez,terr,
     &   nppick+nspick,nppick,nspick,
     &   nint(str_avg(i)),nint(dip_avg(i)),nint(rak_avg(i)),
     &   nint(var_est(1,i)),nint(var_est(2,i)),npol,nint(mfrac(i)*100.),
     &   qual(i),nint(100*prob(i)),nint(100*stdr(i)),mflag
      end do
411   format(i16,1x,i4,1x,i2,1x,i2,1x,i2,1x,i2,1x,f6.3,1x,a1,1x,
     &  f5.3,1x,a1,1x,f9.5,1x,f10.5,1x,f7.3,1x,a1,1x,f7.3,1x,f7.3,
     &  1x,f7.3,1x,f7.3,3x,i4,1x,i4,1x,i4,1x,i4,1x,i3,1x,i4,3x,i2,
     &  1x,i2,1x,i3,1x,i2,1x,a1,1x,i3,1x,i2,1x,a1)

c output set of acceptable mechanisms  ** YOU MAY WISH TO CHANGE THE OUTPUT FORMAT **
      write (11,412) iyr,imon,idy,ihr,imn,qsec,qmag,
     &   qlat,qlon,qdep,sez,seh,npol,nout2,event_id,
     &   str_avg(1),dip_avg(1),rak_avg(1),var_est(1,1),var_est(2,1),
     &   mfrac(1),qual(1),prob(1),stdr(1)
412   format (i4,1x,i2,1x,i2,1x,i2,1x,i2,1x,f6.3,2x,f3.1,1x,f9.4,1x,
     &   f10.4,1x,f6.2,1x,f8.4,1x,f8.4,1x,i5,1x,i5,1x,i16,1x,f7.1,1x,
     &   f6.1,1x,f7.1,1x,f6.1,1x,f6.1,1x,f7.3,2x,a1,1x,f7.3,1x,f4.2)
      do 500 ic=1,nout1
        write (11,550) strike2(ic),dip2(ic),rake2(ic),f1norm(1,ic),
     &      f1norm(2,ic),f1norm(3,ic),f2norm(1,ic),f2norm(2,ic),
     &      f2norm(3,ic)
500   continue
550   format (5x,3f9.2,6f9.4)


c Output for GMT MECA package - 20110524 AJL
c
c ouput psmeca -Sa format
c 1,2:      longitude, latitude of event (−: option interchanges order)     ! AJL use lat lon and -:
c 3:        depth of event in kilometers
c 4,5,6:    strike, dip and rake in degrees
c 7:        magnitude
c 8,9:      longitude, latitude at which to place beach ball.
c           Entries in these columns are necessary with the −C option.
c           Using 0,0 in columns 8 and 9 will plot the beach ball at the
c           longitude, latitude given in columns 1 and 2.
c           The −: option will interchange the order of columns (1,2) and (8,9).
c 10:       Text string to appear above the beach ball (optional).
      write (15,*) qlat,qlon,qdep,str_avg(1),dip_avg(1),rak_avg(1),
     &  qmag,0,0,date_time,' n= ',npol,' q= ',qual(1)
c
c output pspolar
c 1,2,3,4   station_code, azimuth, take-off angle, polarity
c           polarity:
c           - compression can be c,C,u,U,+
c           - rarefaction can be d,D,r,R,-
c           - not defined is anything else
      do 450 k=1,npol
        cpolarity='X'
        if (p_pol(k).eq.1) then
          cpolarity='+'
        else if (p_pol(k).eq.-1) then
          cpolarity='-'
        endif
        write (16,*) sname(k),p_azi_mc(k,1),180.0-p_the_mc(k,1),
     &    cpolarity,0,0
450   continue
c output set of acceptable mechanisms
      imax=min(MAX_PMECA_OUT,nout1)
      do 460 ic=1,imax
        write (17,*) qlat,qlon,qdep,strike2(ic),dip2(ic),rake2(ic),
     &  qmag,0,0,date_time,' n= ',npol,' q= ',qual(1)
460   continue

c output prefered mechanism  ** YOU MAY WISH TO CHANGE THE OUTPUT FORMAT **
c vector, slip, from (strike,dip,rake) or vice versa.
c   idir = 1 compute fnorm,slip
c   idir = c compute strike,dip,rake
c Reference:  Aki and Richards, p. 115
c   uses (x,y,z) coordinate system with x=north, y=east, z=down
c 20121017 AJL - bug fix
c       fp1_norm(1)=f2norm(1,1)
c       fp1_norm(2)=f2norm(2,1)
c       fp1_norm(3)=f2norm(3,1)
c       fp1_slip(1)=f1norm(1,1)
c       fp1_slip(2)=f1norm(2,1)
c       fp1_slip(3)=f1norm(3,1)
c       call FPCOOR(strike_np2,dip_np2,rake_np2,f2norm,slip,-1)
c       call FPCOOR(strike_np2,dip_np2,rake_np2,fp1_norm,fp1_slip,2)
       call FPCOOR(str_avg(1),dip_avg(1),rak_avg(1),fp1_norm,fp1_slip,1)
       call FPCOOR(strike_np2,dip_np2,rake_np2,fp1_slip,fp1_norm,2)

       write (18,*) 'publicID ', event_id
       write (18,*) 'triggeringOriginID ', event_id
       write (18,*) 'nodalPlanes.nodalPlane1.strike ', str_avg(1)
       write (18,*) 'nodalPlanes.nodalPlane1.dip ', dip_avg(1)
       write (18,*) 'nodalPlanes.nodalPlane1.rake ', rak_avg(1)
       write (18,*) 'nodalPlanes.nodalPlane2.strike ', strike_np2
       write (18,*) 'nodalPlanes.nodalPlane2.dip ', dip_np2
       write (18,*) 'nodalPlanes.nodalPlane2.rake ', rake_np2
       write (18,*) 'nodalPlanes.preferredPlane ', -1
c       write (18,*) 'nodalPlanes.nodalPlane2* ', XXX
c       write (18,*) 'principalAxes* ', XXX
       write (18,*) 'azimuthalGap ', magap
       write (18,*) 'stationPolarityCount ', npol
       write (18,*) 'misfit ', mfrac(1)
       write (18,*) 'stationDistributionRatio ', stdr(1)
       write (18,*) 'rmsAngDiffAccPref ', var_avg(1)
       write (18,*) 'fracAcc30degPref ', prob(1)
       write (18,*) 'qualityCode ', qual(1)
       write (18,*) 'methodID ', 'smi:gov.usgs/focalmec/HASH_1.2'
c       write (18,*) 'comment ', XXX
c       write (18,*) 'creationInfo ', XXX


      close (15)
      close (16)
      close (17)
      close (18)

      goto 20

999   close (11)
      close (12)
      close (13)
      close (14)
      stop
      end



c function param_wt converts a parameter value to a weight
c       by linear interpolation between min and max value limits
c    Inputs:    rval   = parameter value
c               rmin = min value limit
c               rmax = max value limit
c    Outputs:   param_wt  = weight (=0 if rval <= rmin, =1 if rval >=rmax,
c                   linear 0-1 interpolation or rval between rmin and rmax otherwise
      real function param_wt(rval, rmin, rmax)
      real rval, rmin, rmax
c      real param_wt
      if (rval .le. rmin) then
          param_wt = 0.0
          return
      endif
      if (rval .ge. rmax) then
          param_wt = 1.0
          return
      endif
      param_wt = (rval - rmin) / (rmax - rmin)
      return
      end
