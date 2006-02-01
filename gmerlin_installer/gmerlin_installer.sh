#!/bin/sh
clear
##################################################################
# 
#  gmerlin_installer.sh
# 
#  Copyright (c) 2006 by Michael Gruenert - one78@web.de
# 
#  http://gmerlin.sourceforge.net
# 
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
#
################################################################## 





############################################### DEFINE THE IMPORTANT THINGS ##############################################################


                                        ###### Define VARIABLES for installation ######

                                            # Define the old PKG_CONFIG_PATH #
OLD_PKG_CONFIG=$PKG_CONFIG_PATH

                                           # Define the INSTALLATION directorys #
LOGS=".gmerlin_logs"     ;     HELPS=".gmerlin_helps"     ;      HOME="GMerlin_INstallation"

                                              # Define the PACKS how need #
BASE="yum apt-get"
BASE_HELP="rpm dpkg"
TOOLS="tar grep wget findutils"

APT_LIBS_OPTI="libtiff4-dev libpng12-dev libjpeg62-dev libgtk2.0-dev libvorbis-dev libesd0-dev"
APT_LIBS_NEED="libasound2-dev zlib1g-dev libxml2-dev libxinerama-dev libxv-dev libflac-dev libsmbclient-dev libxxf86vm-dev"
APT_LIBS_IMPO="gcc g++ autoconf automake1.9"

YUM_LIBS_OPTI="libpng-devel libtiff-devel libvorbis-devel esound-devel flac-devel libjpeg-devel"
YUM_LIBS_NEED="alsa-lib-devel libxml2-devel samba-common xmms-devel xorg-x11-devel zlib-devel gtk+-devel gtk2-devel"
YUM_LIBS_IMPO="gcc gcc-c++ autoconf automake "

                                        ###### Define COLORS and POSITIONS ######

                                                # Define COLORS for text #
COL_DEF="\033[0m"
COL_RED="\033[31m"    ;    COL_RED_HIGH="\033[31;1m"    ;    COL_RED_LINE="\033[31;4m"    ;    COL_RED_LINE_HIGH="\033[31;4;1m"
COL_GRE="\033[32m"    ;    COL_GRE_HIGH="\033[32;1m"    ;    COL_GRE_LINE="\033[32;4m"    ;    COL_GRE_LINE_HIGH="\033[32;4;1m"
COL_YEL="\033[33m"    ;    COL_YEL_HIGH="\033[33;1m"    ;    COL_YEL_LINE="\033[33;4m"    ;    COL_YEL_LINE_HIGH="\033[33;4;1m"
COL_BLU="\033[34m"    ;    COL_BLU_HIGH="\033[34;1m"    ;    COL_BLU_LINE="\033[34;4m"    ;    COL_BLU_LINE_HIGH="\033[34;4;1m"

                                             # Define the POSITION strings #
POSITION_HEADLINE="\n\033[1C"               ;          POSITION_HEADLINE_COMMENT="\033[2C"
POSITION_HEADLINE_COMMENT_2="\033[3C"       ;          POSITION_INFO="\033[4C"       
POSITION_STATUS="\r\033[56C"                ;          POSITION_DISKRIPTION="\r\033[55C"
POSITION_PAGE_HEAD_LINE="\033[15C"          ;          POSITION_PAGE_COMMENT_LINE="\033[5C"
YES_NO_EXIT_CLEAR="\033[1A$POSITION_DISKRIPTION\033[K"
 
					     # Generate the INFORMATION strings #
OK="$POSITION_STATUS[ $COL_GRE_HIGH OK $COL_DEF ]"
OK2="$POSITION_STATUS[ $COL_BLU_HIGH OK $COL_DEF ]"
YES_NO_EXIT="$COL_DEF$POSITION_DISKRIPTION[ \033[32;1mY\033[0mES,\033[31;1mN\033[0mO,E\033[33;1mX\033[0mIT ]\033[5C Y\033[1D"
FAIL="$POSITION_STATUS[$COL_RED_HIGH FAIL $COL_DEF]"
FAIL2="$POSITION_STATUS[$COL_YEL_HIGH FAIL $COL_DEF]"
YES_NO="$COL_DEF$POSITION_DISKRIPTION[ \033[32;1mY\033[0mES,\033[31;1mN\033[0mO ]\033[5C Y\033[1D"

					###### Define FUNCTIONS how need ######

					        # Page DESIGN for text #
# $1 = GMERLIN_PACK TO FIND // $NEWEST = RETURN WERT
function FIND_NEWEST_PACKET_FUNC()
{
wget http://prdownloads.sourceforge.net/gmerlin/ -O $HELPS/packs.txt >& $HELPS/DUMP
READY_EXIT_FUNC "Can not check the Gmerlin Packets" 
NEWEST=`grep /gmerlin/$1 $HELPS/packs.txt | awk '{ print $6}' | awk  -F '"' '{ print $2 }' | sort -n -r | awk -F '/' '{ printf $3 ; printf " "}' | awk '{ print $1 }'`
DEL_FILE_FUNC "$HELPS/packs.txt" "Can not delete file"
READY_EXIT_FUNC "Can not delete DUMP && packs.txt"
DEL_FILE_FUNC "$HELPS/DUMP" "Can not delete file"
READY_EXIT_FUNC "Can not delete DUMP && packs.txt";
}

# $1 = GMERLIN_PACK TO FIND 
function DOWNLOAD_NEWEST_PACKET_FUNC()
{
wget http://prdownloads.sourceforge.net/gmerlin/ -O $HELPS/packs.txt >& $HELPS/DUMP
VAR=`grep /gmerlin/$1 $HELPS/packs.txt | awk '{ print $6}' | awk  -F '"' '{ print $2 }' | sort -n -r | awk -F '/' '{ printf $3 ; printf " "}' | awk '{ print $1 }'`
wget http://prdownloads.sourceforge.net/gmerlin/$VAR -O $HELPS/mirrors.txt >& $HELPS/DUMP
VAR2=`grep -B 1 use_mirror $HELPS/mirrors.txt | awk '{ print  $1 , $2 }' | awk -F "<td>" '{ print $1 $2 }' | awk -F "</td>" '{ print $1 $2 }' | awk -F "<a " '{ print $1 $2 }' | awk -F "\"" '{ print $1 $2 }' | awk -F "--" '{ print $1 $2 }' | sed -e '/^[ ]*$/d' | awk -F "href=/gmerlin/" '{ print $1 , $2 }' | awk '{ printf $1 ; printf " " }'`
ZAHL=0 ; ZEILE=0

PRINT_NEW_LINE_FUNC 1
PRINT_COMMENT_LINE_FUNC "  NR \r\033[8C LAND \r\033[15C = \r\033[29C MIRROR" 
PRINT_COMMENT_LINE_FUNC "------------------------------------------------"
PRINT_NEW_LINE_FUNC 1

for i in $VAR2
  do
  if [ "$ZAHL" = "1" ]
      then
      echo -e `echo $i| awk -F "=" '{print $2}'`
      let ZAHL=$ZAHL+1
  fi
  if [ "$ZAHL" = "0" ]
   then
   echo -ne "$POSITION_INFO$ZEILE \r\033[8C $i \r\033[15C = \r\033[30C"  
   let ZAHL=$ZAHL+1
   let ZEILE=ZEILE+1
  fi
  if [ "$ZAHL" = "2" ]
   then
   ZAHL=0
  fi
done
PRINT_NEW_LINE_FUNC 1

let ZEILE=$ZEILE-1

if [ "$AUTO_INSTALL" = true ]
    then
    PACK=0
    ANSWER_OK=true
else
    echo -e $POSITION_INFO$COL_RED"X for EXIT$COL_DEF"
    PRINT_NEW_LINE_FUNC 1
    ANSWER_OK=false
fi

while ! $ANSWER_OK
  do
  echo -ne $POSITION_INFO"What mirror do you want, pleace give the number?\033[5C 0\033[1D"
  read ANSWER
  for i in `seq 0 $ZEILE`
    do
    if [ "$ANSWER" = "$i" ]
        then
        PACK=$i
        ANSWER_OK=true
    fi
    if [ "$ANSWER" = "" -o "$ANSWER" = " " ]
        then
        PACK=0
        ANSWER_OK=true
    fi
    if [ "$ANSWER" = "x" -o "$ANSWER" = "X" ]
	then
	DEL_FILE_FUNC "$HELPS/packs.txt" "Can not delete file"
	DEL_FILE_FUNC "$HELPS/mirrors.txt" "Can not delete file"
	DEL_FILE_FUNC "$HELPS/DUMP" "Can not delete file"
	PRINT_NEW_LINE_FUNC 2
	exit
    fi
  done
done

ZAHL=0 ; let ZEILE=($PACK*2)+1
for i in $VAR2
  do
  if [ "$ZAHL" = "$ZEILE" ]
   then
   wget http://prdownloads.sourceforge.net/gmerlin/$i -O $HELPS/download_html.txt >& $HELPS/DUMP
  fi
  let ZAHL=$ZAHL+1
done

DOWN=`grep URL= $HELPS/download_html.txt | awk -F ";" '{print $2}' | awk -F "=" '{print $2}' | awk -F "\"" '{print $1}'`
PRINT_NEW_LINE_FUNC 1
PRINT_INFO_LINE_FUNC "download $1"
echo -ne "$POSITION_STATUS   "
wget -N $DOWN 2>&1 | tee $HELPS/DUMP | awk '$7~/%$/ { printf "%4s" ,$7; system("echo -ne \"\033[4D\"")}' 
grep -q '100%' $HELPS/DUMP
if [ "$?" = "0" ]
    then
    DEL_FILE_FUNC "$HELPS/packs.txt" "Can not delete file"
    DEL_FILE_FUNC "$HELPS/mirrors.txt" "Can not delete file"
    DEL_FILE_FUNC "$HELPS/download_html.txt" "Can not delete file"
    DEL_FILE_FUNC "$HELPS/DUMP" "Can not delete file"
    return 0
else
    cp $HELPS/DUMP $INSTALL_HOME/$1.wget.log ; READY_EXIT_FUNC "Can not copy file"
    cp $HELPS/DUMP $LOGS/$1.wget.log ; READY_EXIT_FUNC "Can not copy file"
    DEL_FILE_FUNC "$HELPS/packs.txt" "Can not delete file"
    DEL_FILE_FUNC "$HELPS/mirrors.txt" "Can not delete file"
    DEL_FILE_FUNC "$HELPS/download_html.txt" "Can not delete file"
    DEL_FILE_FUNC "$HELPS/DUMP" "Can not delete file"
    return 1
fi;
}


# $1 = HOW MANY NEW LINES
function PRINT_NEW_LINE_FUNC()
{
    for i in `seq 1 $1`; do echo ; done;
}

# $1 = PAGE_HEAD_LINE TEXT
function PRINT_PAGE_HEAD_LINE_FUNC()
{
    PRINT_NEW_LINE_FUNC 2
    echo -e "$POSITION_PAGE_HEAD_LINE$COL_BLU_LINE_HIGH$1$COL_DEF"
    PRINT_NEW_LINE_FUNC 1;
}

# $1 = PAGE_COMMENT_LINE TEXT ; $2 = ECHO OPTION
function PRINT_PAGE_COMMENT_LINE_FUNC()
{
    echo $2 "$POSITION_PAGE_COMMENT_LINE$1$COL_DEF";
}

function AUTO_CHECK_FUNC()
{
    if [ "$AUTO_CHECK" = true ]
	then
	echo -ne "\n$YES_NO_EXIT_CLEAR\n"
	ANSWER=true
	ANSWER_OK=true
    else
	ANSWER_OK=false
    fi;
}

function AUTO_INSTALL_FUNC()
{
    if [ "$AUTO_INSTALL" = true ]
	then
	echo -ne "\n$YES_NO_EXIT_CLEAR\n"
	ANSWER=true
	ANSWER_OK=true
    else
	ANSWER_OK=false
    fi;
}

function YES_NO_EXIT_FUNC()
{
    while ! $ANSWER_OK
      do
      read ANSWER
      if [ "$ANSWER" = "y" -o "$ANSWER" = "Y" -o "$ANSWER" = "" ]
	  then
	  echo -e "$YES_NO_EXIT_CLEAR"
	  ANSWER=true
	  ANSWER_OK=true
      fi
      if [ "$ANSWER" = "n" -o "$ANSWER" = "N" ]
	  then
	  echo -e "$YES_NO_EXIT_CLEAR"
	  ANSWER=false
	  ANSWER_OK=true
      fi
      if [ "$ANSWER" = "x" -o "$ANSWER" = "X" ]
	  then
	  echo -e "$YES_NO_EXIT_CLEAR\n\n"
	  exit
      fi
    done;
}

function YES_NO_FUNC()
{
    while ! $ANSWER_OK
      do
      read ANSWER
      if [ "$ANSWER" = "y" -o "$ANSWER" = "Y" -o "$ANSWER" = "" ]
	  then
	  ANSWER=true
	  ANSWER_OK=true
      fi
      if [ "$ANSWER" = "n" -o "$ANSWER" = "N" ]
	  then
	  ANSWER=false
	  ANSWER_OK=true
      fi
    done;
}

# $1 = HEAD_LINE TEXT 
function PRINT_HEAD_LINE_FUNC()
{
    echo -ne "$POSITION_HEADLINE$COL_YEL_LINE_HIGH$1$COL_DEF$YES_NO_EXIT"
    AUTO_CHECK_FUNC
    YES_NO_EXIT_FUNC;
}

# $1 = COMMENT_LINE TEXT
function PRINT_COMMENT_LINE_FUNC()
{
    echo -e "$POSITION_HEADLINE_COMMENT$COL_YEL$1$COL_DEF";
}

# $1 = COMMENT_LINE TEXT
function PRINT_COMMENT_2_LINE_FUNC()
{
    echo -e "$POSITION_HEADLINE_COMMENT_2$COL_YEL$1$COL_DEF";
}

# $1 = INFO_LINE TEXT ; $2 = EXTRA TEXT
function PRINT_INFO_LINE_FUNC()
{
    echo -ne "$COL_DEF$POSITION_INFO$1:$2$COL_DEF";
}

# $1 = ERROR_LINE TEXT ; $2 = ECHO OPTION
function PRINT_ERROR_MESSAGE_LINE_FUNC()
{
    echo $2 "$COL_RED$POSITION_INFO$1$COL_DEF";
}


					        # help FUNCTIONS for script #
# TEST $? ; IF ERROR RETURN 1
function READY_FUNC()
{
    if test $? != 0
	then
	return 1
    fi;
}

# TEST $? ; $1 = ERROR TEXT
function READY_EXIT_FUNC()
{
    if test $? != 0
	then
        echo -e "\n$COL_RED_HIGH$POSITION_INFO$1$COL_DEF\n"
	exit
    fi;
}

# TEST DIRECTORY ; $1 = DIRECTORY ; $2 = ERROR TEXT
function MAKE_DIRECTORY_FUNC()
{
    if test ! -d $1 
	then
	mkdir $1
	READY_EXIT_FUNC $2
    fi;
}

# $1 = FILE TO DELETE ; $2 = ERROR TEXT
function DEL_FILE_FUNC()
{
    if test -f $1 
	then
	rm $1
	READY_EXIT_FUNC $2
    fi;
}

# $1 = DIRECTORY TO DELETE ; $2 = ERROR TEXT
function DEL_DIRECTORY_FUNC()
{
    if test -d $1 
	then
	rm -rf $1
	READY_EXIT_FUNC $2
    fi;
}

# $1 = THE PACKET_MANAGER ; $2 = THE PACKET ; $3 = BUG DATEI
function TAKE_PACKETS_FUNC()
{
    if [ "$1" = "apt-get" ]
	then
	$MANAGER -y update >& $3
	READY_FUNC
	if test $? != 0 ; then return 1 ; fi  
    fi
    $1 -y install $2 >& $3  
    READY_FUNC
    if test $? != 0 ; then return 1 ; fi  
    return 0;
}

# $1 = COLUM STRING 1  ;   $2 = POSITION FROM LEFT STRING 1   ;  $3 = ILLUSTRATION FOR STRING 1 
# $4 = COLUM STRING 2  ;   $5 = POSITION FROM LEFT STRING 2   ;  $6 = ILLUSTRATION FOR STRING 2
function PRINT_DOUBLE_LINE_FUNC()
{
LENGHT_COL_1=1
LENGHT_COL_2=1

for i in $1
  do
  echo -e "$2$i$3"
  let LENGHT_COL_1=$LENGHT_COL_1+1
done

BACK_TO_START=$LENGHT_COL_1"A"

echo -e "\033[$BACK_TO_START"

for i in $4
  do
  echo -e "$5$i$6"
  let LENGHT_COL_2=$LENGHT_COL_2+1
done

if [ "$LENGHT_COL_2" -lt "$LENGHT_COL_1" ]
  then
  let REST=$LENGHT_COL_1-$LENGHT_COL_2
  for i in `seq 1 $REST`; do
     echo
  done
else
  let REST=$LENGHT_COL_2-$LENGHT_COL_1
  BACK_TO_START=$REST"A"
  echo -e "\033[$BACK_TO_START"
fi;
}

# $1 = MANAGER FOR INSTALL ; $2 = PACKET ; $3 = PACKET COMMENT 
function INSTALL_PACKETS_FUNC()
{
    for i in $2
      do
      PRINT_INFO_LINE_FUNC "$i" "$3"
      TAKE_PACKETS_FUNC $1 $i "$LOGS/BUG_$i"
      if test $? = 0 
	  then
	  DEL_FILE_FUNC "$LOGS/BUG_$i" "$COL_DEF Can not delete $COL_RED_LINE_HIGH$LOGS/BUG_$i$COL_DEF"
	  echo -e "$OK"
      else
	  ERROR_SAVE=true
	  cp $LOGS/BUG_$i $INSTALL_HOME ; READY_EXIT_FUNC "Can not copy file"
	  echo -e "$FAIL"
      fi
    done;
}

# $1 = PACKET TOOL FOR CHECKING ; $2 = PACKET ; $3 = PACKET COMMENT 
function CHECK_PACKETS_FUNC()
{
    for i in $2
      do
      PRINT_INFO_LINE_FUNC "$i" "$3"
      if [ "$PACKET_TOOL" = "" ]
	  then 
	  which $i >& $LOGS/BUG_$i 
	  if test $? = 0 
	      then
	      DEL_FILE_FUNC "$LOGS/BUG_$i" "$COL_DEF Can not delete $COL_RED_LINE_HIGH$LOGS/BUG_$i$COL_DEF"
	      echo -e "$OK"
	  else
	      ERROR_SAVE=$ERROR
	      ERROR="$ERROR_SAVE $i"
	      echo -e "$FAIL"
	  fi
      elif [ "$1" = "dpkg" ]
	  then
	  dpkg --get-selections | grep $i | awk '{print $1}' >& $LOGS/BUG_$i  
	  if test $? = 0 
	      then
	      DEL_FILE_FUNC "$LOGS/BUG_$i" "$COL_DEF Can not delete $COL_RED_LINE_HIGH$LOGS/BUG_$i$COL_DEF"
	      echo -e "$OK"
	  else 
	      ERROR_SAVE=$ERROR
	      ERROR="$ERROR_SAVE $i"
	      echo -e "$FAIL"
	  fi
      elif [ "$1" = "rpm" ]
	  then 
	  rpm -q $i >& $LOGS/BUG_$i
	  if test $? = 0 
	      then
	      DEL_FILE_FUNC "$LOGS/BUG_$i" "$COL_DEF Can not delete $COL_RED_LINE_HIGH$LOGS/BUG_$i$COL_DEF"
	      echo -e "$OK"
	  else 
	      ERROR_SAVE=$ERROR
	      ERROR="$ERROR_SAVE $i"
	      echo -e "$FAIL"
	  fi
      else
	  PRINT_NEW_LINE_FUNC 1
	  PRINT_ERROR_MESSAGE_LINE_FUNC "$COL_RED_HIGH FATAL ERROR: unknown manager or packet tool $COL_DEF" "-e"
	  PRINT_NEW_LINE_FUNC 2
	  exit
      fi
    done;
}

# BUILD THE NEW PKG_CONFIG_PATH ; $1 = ERROR_DATA(LOCATION)
function FIND_PKG_FUNC()
{
    j=true
    PKG_CONFIG_OLD=$PKG_CONFIG_PATH
    for i in `ls /usr/`
      do
      if [ "$j" = true ] ; then echo -ne "$POSITION_STATUS Please wait ..."  ; j=false ; else echo -ne "$POSITION_STATUS\033[K" ; j=true ; fi
      find /usr/$i -type d >& $1.usr
      READY_FUNC ; if test $? != 0 ; then cp $1.usr $INSTALL_HOME ; READY_EXIT_FUNC "Can not copy file" ; return 1 ; fi  
      PKG_USR_S=$PKG_USR
      PKG_USR="$PKG_USR_S`grep pkgconfig $1.usr | awk '{printf $1"/:" }' 2> DUMP`"
      READY_FUNC ; if test $? = 0 ; then DEL_FILE_FUNC "$1.usr" "Can not delete $1.usr" ; else cp $1.usr $INSTALL_HOME ; READY_EXIT_FUNC "Can not copy file" ; return 1 ; fi  
      DEL_FILE_FUNC "DUMP" "Can not delete DUMP"
      if [ "$j" = false ] ; then echo -ne "$POSITION_STATUS Please wait ..."  ; j=false ; else echo -ne "$POSITION_STATUS\033[K" ; j=true  ; fi
    done

    for i in `ls /opt/`
      do
      if [ "$j" = true ] ; then echo -ne "$POSITION_STATUS Please wait ..."  ; j=false ; else echo -ne "$POSITION_STATUS\033[K" ; j=true ; fi
      find /opt/$i -type d >& $1.opt
      READY_FUNC ; if test $? != 0 ; then cp $1.opt $INSTALL_HOME ; READY_EXIT_FUNC "Can not copy file" ; return 1 ; fi  
      PKG_OPT_S=$PKG_OPT
      PKG_OPT="$PKG_OPT_S`grep pkgconfig $1.opt | awk '{printf $1"/:" }' 2> DUMP`"
      READY_FUNC ; if test $? = 0 ; then DEL_FILE_FUNC "$1.opt" "Can not delete $1.opt" ; else cp $1.opt $INSTALL_HOME ; READY_EXIT_FUNC "Can not copy file" ; return 1 ; fi  
      DEL_FILE_FUNC "DUMP" "Can not delete DUMP"
      if [ "$j" = false ] ; then echo -ne "$POSITION_STATUS Please wait ..."  ; j=false ; else echo -ne "$POSITION_STATUS\033[K" ; j=true  ; fi
    done

    for i in `ls .`
      do
      if [ "$j" = true ] ; then echo -ne "$POSITION_STATUS Please wait ..."  ; j=false ; else echo -ne "$POSITION_STATUS\033[K" ; j=true ; fi
      find ~/$i -type d >& $1.hom
      READY_FUNC ; if test $? != 0 ; then cp $1.hom $INSTALL_HOME ; READY_EXIT_FUNC "Can not copy file" ; return 1 ; fi  
      PKG_HOM_S=$PKG_HOM
      PKG_HOM="$PKG_HOM_S`grep pkgconfig $1.hom | awk '{printf $1"/:" }' 2> DUMP`"
      READY_FUNC ; if test $? = 0 ; then DEL_FILE_FUNC "$1.hom" "Can not delete $1.hom" ; else cp $1.hom $INSTALL_HOME ; READY_EXIT_FUNC "Can not copy file" ; return 1 ; fi  
      DEL_FILE_FUNC "DUMP" "Can not delete DUMP"
      if [ "$j" = false ] ; then echo -ne "$POSITION_STATUS Please wait ..."  ; j=false ; else echo -ne "$POSITION_STATUS\033[K" ; j=true  ; fi
    done
    
    PKG_CONF_NEW="$PKG_USR$PKG_OPT$PKG_HOME/opt/gmerlin/lib/pkgconfig"
    return 0;
}




############################################### MAKE THE DIRECTORYS FOR INSTALLATION ##############################################################

					  ######## CHECK THE BASE TOOLS ###########

                                                    # Check CORETILS #
TEXT="COREUTILS"    ;    INSTALL_HOME=`pwd`    ;      READY_EXIT_FUNC "$COL_DEF Please install the $COL_RED_LINE_HIGH$TEXT$COL_DEF packet"

                                                # Exit when we are not ROOT #
if [ `whoami` != root ]
    then
    echo -e "\n$POSITION_INFO Must be$COL_RED_HIGH ROOT$COL_DEF to run the install script\n"
    exit 
fi

                                                # Make use our OWN directory #
MAKE_DIRECTORY_FUNC $HOME "$COL_DEF Can not create $COL_RED_LINE_HIGH$HOME$COL_DEF directory" 
cd $HOME 
READY_EXIT_FUNC "$COL_DEF Can not change to $COL_RED_LINE_HIGH$HOME$COL_DEF directory"
HOME=`pwd`
READY_EXIT_FUNC "$COL_DEF Can not find the Path of$COL_RED_LINE_HIGH$HOME$COL_DEF directory"

                                                # Make help DIRECTORYS #
# LOGS
DEL_DIRECTORY_FUNC $LOGS "$COL_DEF Can not delete $COL_RED_LINE_HIGH$LOGS$COL_DEF directory"
MAKE_DIRECTORY_FUNC $LOGS "$COL_DEF Can not create $COL_RED_LINE_HIGH$LOGS$COL_DEF directory" 
cd $LOGS
READY_EXIT_FUNC "$COL_DEF Can not change to $COL_RED_LINE_HIGH$LOGS$COL_DEF directory"
LOGS=`pwd`
READY_EXIT_FUNC "$COL_DEF Can not find the Path of$COL_RED_LINE_HIGH$LOGS$COL_DEF directory"
cd $HOME
READY_EXIT_FUNC "$COL_DEF Can not change to $COL_RED_LINE_HIGH$HOME$COL_DEF directory"

# HELPS
DEL_DIRECTORY_FUNC $HELPS "$COL_DEF Can not delete $COL_RED_LINE_HIGH$HELPS$COL_DEF directory"
MAKE_DIRECTORY_FUNC $HELPS "$COL_DEF Can not create $COL_RED_LINE_HIGH$HELPS$COL_DEF directory" 
cd $HELPS
READY_EXIT_FUNC "$COL_DEF Can not change to $COL_RED_LINE_HIGH$HELPS$COL_DEF directory"
HELPS=`pwd`
READY_EXIT_FUNC "$COL_DEF Can not find the Path of$COL_RED_LINE_HIGH$HELPS$COL_DEF directory"
cd $HOME
READY_EXIT_FUNC "$COL_DEF Can not change to $COL_RED_LINE_HIGH$HOME$COL_DEF directory"



############################################### FIRST CHECK BASH FUNCTIONS  ##############################################################


	                                 ######## CHECK THE SCRIPT PARAMETER ##########
HAND=false
AUTO_CHECK=false
AUTO_INSTALL=false
if [ "$1" = "-a" -o "$2" = "-a" ]
    then
    AUTO_CHECK=true
fi
if [ "$1" = "-i" -o "$2" = "-i" ]
    then
    AUTO_INSTALL=true
fi




############################################### SEND WELCOME MESSAGE AND INFOS ##############################################################

PRINT_PAGE_HEAD_LINE_FUNC "Welcome to the Gmerlin installation" "-e"
PRINT_PAGE_COMMENT_LINE_FUNC "This script will be install Gmerlin on your System, with all features" "-e"
PRINT_PAGE_COMMENT_LINE_FUNC "The needed components and librarys will downloaded by internet" "-e"
PRINT_NEW_LINE_FUNC 1
PRINT_PAGE_COMMENT_LINE_FUNC "Please make sure, that the internet connection is ready" "-e"
PRINT_PAGE_COMMENT_LINE_FUNC "Gmerlin will be downloaded 10 to 50 MByte" "-e"
PRINT_PAGE_COMMENT_LINE_FUNC "You can download an install all components form hand, the script" "-e"
PRINT_PAGE_COMMENT_LINE_FUNC "will be find it!" "-e"


                                              # Define the newest ALL-IN-ONE packs #
FIND_NEWEST_PACKET_FUNC "gmerlin-all-in-one"
GMERLIN_ALL_IN_ONE_PACKETS_NAME=$NEWEST
                                             # Define the newest DEPENDENCIE packs #
FIND_NEWEST_PACKET_FUNC "gmerlin-dependencies"
GMERLIN_DEPENDENCIES_PACKETS_NAME=$NEWEST

ALL_PACKS="$GMERLIN_DEPENDENCIES_PACKETS_NAME $GMERLIN_ALL_IN_ONE_PACKETS_NAME"

if [ "$AUTO_CHECK" = false ]
    then
    PRINT_NEW_LINE_FUNC 1
    PRINT_PAGE_COMMENT_LINE_FUNC "Ready to install Gmerlin: $POSITION_DISKRIPTION$YES_NO" "-ne" 
    AUTO_CHECK_FUNC ; YES_NO_FUNC ; if [ "$ANSWER" = false ] ; then	PRINT_NEW_LINE_FUNC 2 ; exit ; fi
fi

					    # PAGE TITLE_LINE AFTER WELCOME #
PRINT_PAGE_HEAD_LINE_FUNC "Begin with Checking System components" "-e"




############################################### CHECK SYSTEM BASE TOOLS ##############################################################

                                              # Define MACKER for this funktion #
ERROR=""                ;            ERROR_SAVE=""    
MANAGER=""              ;            PACKET_TOOL=""
 
                                             # Begin with the CHECKING tools #
PRINT_HEAD_LINE_FUNC "Check the System BASE tools:"
if [ "$ANSWER" = true ]
    then
    PRINT_COMMENT_LINE_FUNC "(We hope to found yum && rpm or apt-get && dpkg)"

                                                 # Check BASE tools #
    for i in $BASE
      do
      PRINT_INFO_LINE_FUNC "$i"
      DUMP=`which $i 2> $LOGS/BUG_$i` 
      if test $? = 0 
	  then
	  MANAGER=`basename $DUMP`
	  DEL_FILE_FUNC "$LOGS/BUG_$i" "$COL_DEF Can not delete $COL_RED_LINE_HIGH$LOGS/BUG_$i$COL_DEF"
	  echo -e "$OK"
      else
	  ERROR_SAVE=$ERROR
	  ERROR="$ERROR_SAVE $i"
	  echo -e "$FAIL"
      fi
    done
    
                                                 # Check BASE_HELP tools #
    for i in $BASE_HELP
      do
      PRINT_INFO_LINE_FUNC "$i"
      DUMP=`which $i 2> $LOGS/BUG_$i` 
      if test $? = 0 
	  then
	  PACKET_TOOL=`basename $DUMP`
	  DEL_FILE_FUNC "$LOGS/BUG_$i" "$COL_DEF Can not delete $COL_RED_LINE_HIGH$LOGS/BUG_$i$COL_DEF"
	  echo -e "$OK"
      else
	  ERROR_SAVE=$ERROR
	  ERROR="$ERROR_SAVE $i"
	  echo -e "$FAIL"
      fi
    done
    
                                                 # If error SEND user Message #
    if [ "$MANAGER" = "" -o "$PACKET_TOOL" = "" ]
	then
	PRINT_NEW_LINE_FUNC 1
 	PRINT_ERROR_MESSAGE_LINE_FUNC "On the System we does not found following Packets:" "-e"
	PRINT_ERROR_MESSAGE_LINE_FUNC "      $COL_RED_HIGH$ERROR$COL_DEF" "-e"
 	PRINT_ERROR_MESSAGE_LINE_FUNC "The Packet are important for downloading Packets" "-e"
	if [ "$AUTO_INSTALL" = false ]
	    then
	    PRINT_ERROR_MESSAGE_LINE_FUNC "Go on the installation? $YES_NO" "-ne"
	    AUTO_INSTALL_FUNC ; YES_NO_FUNC
	    if [ "$ANSWER" = false ] ; then	PRINT_NEW_LINE_FUNC 2 ; exit ; fi
	else
	    PRINT_NEW_LINE_FUNC 1
	    PRINT_ERROR_MESSAGE_LINE_FUNC "$COL_RED_HIGH Without an Packet Manager, auto install does not go" "-ne"
	    PRINT_NEW_LINE_FUNC 2
	    exit
	fi
    fi
fi




############################################### CHECK AND INSTALL SYSTEM TOOLS ##############################################################

                                              # Define MACKER for this funktion #
ERROR=""                ;            ERROR_SAVE=""       ;        DUMP=false   
 
                                             # Begin with the CHECKING tools #
PRINT_HEAD_LINE_FUNC "Check/install the SYSTEM tools:"
if [ "$ANSWER" = true ]
    then
    PRINT_COMMENT_LINE_FUNC "(This Tools are needed for running this Script)"
    
                                                 # Check BASE tools #
    for i in $TOOLS
      do
      PRINT_INFO_LINE_FUNC "$i"
      if [ "$i" = "findutils" ]
	  then
	  DUMP=`which find 2> $LOGS/BUG_$i`
      else
	  DUMP=`which $i 2> $LOGS/BUG_$i`
      fi
      if test $? = 0 
	  then
	  DEL_FILE_FUNC "$LOGS/BUG_$i" "$COL_DEF Can not delete $COL_RED_LINE_HIGH$LOGS/BUG_$i$COL_DEF"
	  echo -e "$OK"
      else
	  ERROR_SAVE=$ERROR
	  ERROR="$ERROR_SAVE $i"
	  echo -e "$FAIL"
      fi
    done
    
                                               # If error SEND user Message #
    if [ "$ERROR" != "" ]
	then
	PRINT_NEW_LINE_FUNC 1
 	PRINT_ERROR_MESSAGE_LINE_FUNC "On the System we does not found following Packets:" "-e"
	PRINT_ERROR_MESSAGE_LINE_FUNC "      $COL_RED_HIGH$ERROR$COL_DEF" "-e"
 	PRINT_ERROR_MESSAGE_LINE_FUNC "The Packet are important for running this Script" "-e"
	if [ "$MANAGER" != "" -a "$AUTO_INSTALL" = false ]
	    then
	    PRINT_ERROR_MESSAGE_LINE_FUNC "The script can install it with $MANAGER? $YES_NO" "-ne"
	    AUTO_INSTALL_FUNC ; YES_NO_FUNC
	    if [ "$ANSWER" = false ]
		then
		PRINT_ERROR_MESSAGE_LINE_FUNC "Install it jet and go on? $YES_NO" "-ne"
		AUTO_INSTALL_FUNC ; YES_NO_FUNC ; HAND=true
		if [ "$ANSWER" = false ] ; then	PRINT_NEW_LINE_FUNC 2 ; exit ; fi
	    fi
	else
	    if [ "$AUTO_INSTALL" = false ]
		then
		PRINT_ERROR_MESSAGE_LINE_FUNC "Install it jet and go on? $YES_NO" "-ne"
		AUTO_INSTALL_FUNC ; YES_NO_FUNC ; HAND=true
		if [ "$ANSWER" = false ] ; then PRINT_NEW_LINE_FUNC 2 ; exit ; fi
	    fi
	fi
	
                                                 # Check BASE tools too #
	ERROR_SAVE=""
	PRINT_NEW_LINE_FUNC 1
	if [ "$HAND" = true ]
	    then
	    for i in $ERROR
	      do
	      PRINT_INFO_LINE_FUNC "check too $i"
	      if [ "$i" = "findutils" ]
		  then
		  DUMP=`which find 2> $LOGS/BUG_$i`
	      else
		  DUMP=`which $i 2> $LOGS/BUG_$i`
	      fi
	      if test $? = 0 
		  then
		  DEL_FILE_FUNC "$LOGS/BUG_$i" "$COL_DEF Can not delete $COL_RED_LINE_HIGH$LOGS/BUG_$i$COL_DEF"
		  echo -e "$OK"
	      else
		  ERROR_SAVE=true
		  cp $LOGS/BUG_$i $INSTALL_HOME ; READY_EXIT_FUNC "Can not copy file"
		  echo -e "$FAIL"
	      fi
	    done
	else
	    INSTALL_PACKETS_FUNC "$MANAGER" "$ERROR" "$COL_RED\r\033[25C( import )$COL_DEF"
	fi
    fi
    
                                                 # If ERROR too #
    if [ "$ERROR_SAVE" != "" ]
	then
	PRINT_NEW_LINE_FUNC 1
 	PRINT_ERROR_MESSAGE_LINE_FUNC "On the System we does not found all needed Packets:" "-e"
 	PRINT_ERROR_MESSAGE_LINE_FUNC "Please look at$COL_RED_HIGH BUG_* files$COL_DEF$COL_RED to find the error and restart the script" "-e"
	PRINT_NEW_LINE_FUNC 2
	exit
    fi
fi




############################################### CHECK AND INSTALL SYSTEM TOOLS ##############################################################

					            # PAGE TITLE_LINE #
PRINT_PAGE_HEAD_LINE_FUNC "Check and/or install the System librarys" "-e"


					       # Define MACKER for this funktion #
ERROR=""                ;            ERROR_SAVE=""       

                                               # Begin with the CHECKING tools #
PRINT_HEAD_LINE_FUNC "Check/install the SYSTEM librarys:"
if [ "$ANSWER" = true ]
    then
    PRINT_COMMENT_LINE_FUNC "(This librarys are need for the gmerlin installation)"
# NO MANAGER && NO PACKET_TOOL
    if [ "$MANAGER" = "" -a "$PACKET_TOOL" = "" ]
	then
	PRINT_COMMENT_2_LINE_FUNC "No Packet Manager like YUM/APT-GET and no Packet Tool like RPM/DPKG found."
	PRINT_COMMENT_2_LINE_FUNC "The Script does not check/install the library, pleace look at the"
	PRINT_COMMENT_2_LINE_FUNC "following list and find out and install the right librarys for your System."
	PRINT_NEW_LINE_FUNC 1
	PRINT_COMMENT_2_LINE_FUNC "\033[1C$COL_BLU UBUNTU: \033[31C$COL_BLU FEDORA:"
	PRINT_DOUBLE_LINE_FUNC "$APT_LIBS_IMPO" "\033[5C"  "$COL_RED\r\033[25C( important )$COL_DEF" "$YUM_LIBS_IMPO" "\033[45C" "$COL_RED\r\033[65C( important )$COL_DEF"
	PRINT_DOUBLE_LINE_FUNC "$APT_LIBS_NEED" "\033[5C"  "$COL_YEL\r\033[25C( needed )$COL_DEF" "$YUM_LIBS_NEED" "\033[45C" "$COL_YEL\r\033[65C( needed )$COL_DEF"
	PRINT_DOUBLE_LINE_FUNC "$APT_LIBS_OPTI" "\033[5C"  "$COL_GRE\r\033[25C( optional )$COL_DEF" "$YUM_LIBS_OPTI" "\033[45C" "$COL_GRE\r\033[65C( optional )$COL_DEF"
	PRINT_ERROR_MESSAGE_LINE_FUNC "Install it jet and go on? $YES_NO" "-ne"
	AUTO_INSTALL_FUNC ; YES_NO_FUNC ; HAND=true
	if [ "$ANSWER" = false ] ; then PRINT_NEW_LINE_FUNC 2 ; exit ; fi
# YES MANAGER && YES PACKET_TOOL
    elif [ "$MANAGER" != "" -a "$PACKET_TOOL" != "" ]
	then
	if [ "$PACKET_TOOL" = "dpkg" ]
	    then
	    CHECK_PACKETS_FUNC "$PACKET_TOOL" "$APT_LIBS_IMPO" "$COL_RED\r\033[25C( import )$COL_DEF"
	    CHECK_PACKETS_FUNC "$PACKET_TOOL" "$APT_LIBS_NEED" "$COL_YEL\r\033[25C( needed )$COL_DEF"
	    CHECK_PACKETS_FUNC "$PACKET_TOOL" "$APT_LIBS_OPTI" "$COL_GRE\r\033[25C( option )$COL_DEF"
	elif [ "$PACKET_TOOL" = "rpm" ]
	    then
	    CHECK_PACKETS_FUNC "$PACKET_TOOL" "$YUM_LIBS_IMPO" "$COL_RED\r\033[25C( import )$COL_DEF"
	    CHECK_PACKETS_FUNC "$PACKET_TOOL" "$YUM_LIBS_NEED" "$COL_YEL\r\033[25C( needed )$COL_DEF"
	    CHECK_PACKETS_FUNC "$PACKET_TOOL" "$YUM_LIBS_OPTI" "$COL_GRE\r\033[25C( option )$COL_DEF"
	else
	    PRINT_ERROR_MESSAGE_LINE_FUNC "$COL_RED_HIGH FATAL ERROR: unknown manager or packet tool $COL_DEF" "-e"
	fi
	ERROR_SAVE=""       
	if [ "$ERROR" != "" ]
	    then
	    PRINT_NEW_LINE_FUNC 1
	    PRINT_ERROR_MESSAGE_LINE_FUNC "We can not found the FAILED Packets" "-e"
	    if [ "$AUTO_INSTALL" = false ]
		then
		PRINT_ERROR_MESSAGE_LINE_FUNC "Would you install it? $YES_NO" "-ne"
		AUTO_INSTALL_FUNC ; YES_NO_FUNC
	    fi
	    if [ "$ANSWER" = false ]
		then
		PRINT_ERROR_MESSAGE_LINE_FUNC "Go on the installation? $YES_NO" "-ne"
		AUTO_INSTALL_FUNC ; YES_NO_FUNC
		if [ "$ANSWER" = false ] ; then	PRINT_NEW_LINE_FUNC 2 ; exit ; fi
	    else
		PRINT_NEW_LINE_FUNC 1
		if [ "$MANAGER" = "apt-get" ]
		    then
		    INSTALL_PACKETS_FUNC "$MANAGER" "$ERROR" ""
		elif [ "$MANAGER" = "yum" ]
		    then
		    INSTALL_PACKETS_FUNC "$MANAGER" "$ERROR" ""
		else
		    PRINT_ERROR_MESSAGE_LINE_FUNC "$COL_RED_HIGH FATAL ERROR: unknown manager or packet tool $COL_DEF" "-e"
		fi
		if [ "$ERROR_SAVE" = true ]
		    then
		    PRINT_NEW_LINE_FUNC 1
		    PRINT_ERROR_MESSAGE_LINE_FUNC "We can not install the FAIL Packets, please look" "-e"
		    PRINT_ERROR_MESSAGE_LINE_FUNC "at the$COL_RED_HIGH BUG_*$COL_DEF$COL_RED files to find the error." "-e"
		    PRINT_ERROR_MESSAGE_LINE_FUNC "Go on the installation? $YES_NO" "-ne"
		    AUTO_INSTALL_FUNC ; YES_NO_FUNC
		    if [ "$ANSWER" = false ] ; then	PRINT_NEW_LINE_FUNC 2 ; exit ; fi
		fi
	    fi
	fi
# YES MANAGER && NO PACKET_TOOL
    elif [ "$MANAGER" != "" -a "$PACKET_TOOL" = "" ]
	then
	PRINT_COMMENT_2_LINE_FUNC "No Packet Tool like RPM/DPKG found, about this we can only install"
	PRINT_COMMENT_2_LINE_FUNC "the following Packets. ( This can take a little bit longer )"
	if [ "$MANAGER" = "apt-get" ]
	    then
	    INSTALL_PACKETS_FUNC "$MANAGER" "$APT_LIBS_IMPO" "$COL_RED\r\033[25C( import )$COL_DEF"
	    INSTALL_PACKETS_FUNC "$MANAGER" "$APT_LIBS_NEED" "$COL_YEL\r\033[25C( needed )$COL_DEF"
	    INSTALL_PACKETS_FUNC "$MANAGER" "$APT_LIBS_OPTI" "$COL_GRE\r\033[25C( option )$COL_DEF"
	elif [ "$MANAGER" = "yum" ]
	    then
	    INSTALL_PACKETS_FUNC "$MANAGER" "$YUM_LIBS_IMPO" "$COL_RED\r\033[25C( import )$COL_DEF"
	    INSTALL_PACKETS_FUNC "$MANAGER" "$YUM_LIBS_NEED" "$COL_YEL\r\033[25C( needed )$COL_DEF"
	    INSTALL_PACKETS_FUNC "$MANAGER" "$YUM_LIBS_OPTI" "$COL_GRE\r\033[25C( option )$COL_DEF"
	else
	    PRINT_ERROR_MESSAGE_LINE_FUNC "$COL_RED_HIGH FATAL ERROR: unknown manager or packet tool $COL_DEF" "-e"
	fi
	if [ "$ERROR_SAVE" != "" ]
	    then
	    PRINT_NEW_LINE_FUNC 1
	    PRINT_ERROR_MESSAGE_LINE_FUNC "We can not install the FAIL Packets, please look" "-e"
	    PRINT_ERROR_MESSAGE_LINE_FUNC "at the$COL_RED_HIGH BUG_*$COL_DEF$COL_RED files to find the error." "-e"
	    PRINT_ERROR_MESSAGE_LINE_FUNC "Go on the installation? $YES_NO" "-ne"
	    AUTO_INSTALL_FUNC ; YES_NO_FUNC
	    if [ "$ANSWER" = false ] ; then	PRINT_NEW_LINE_FUNC 2 ; exit ; fi
	fi
# NO MANAGER && YES PACKET_TOOL
    elif [ "$MANAGER" = "" -a "$PACKET_TOOL" != "" ]
	then
	PRINT_COMMENT_2_LINE_FUNC "No Packet Manager like YUM/APT-GET found, about this we can only check"
	PRINT_COMMENT_2_LINE_FUNC "the following Packets. Pleace install the failed Packets from Hand."
	if [ "$PACKET_TOOL" = "dpkg" ]
	    then
	    CHECK_PACKETS_FUNC "$PACKET_TOOL" "$APT_LIBS_IMPO" "$COL_RED\r\033[25C( import )$COL_DEF"
	    CHECK_PACKETS_FUNC "$PACKET_TOOL" "$APT_LIBS_NEED" "$COL_YEL\r\033[25C( needed )$COL_DEF"
	    CHECK_PACKETS_FUNC "$PACKET_TOOL" "$APT_LIBS_OPTI" "$COL_GRE\r\033[25C( option )$COL_DEF"
	elif [ "$PACKET_TOOL" = "rpm" ]
	    then
	    CHECK_PACKETS_FUNC "$PACKET_TOOL" "$YUM_LIBS_IMPO" "$COL_RED\r\033[25C( import )$COL_DEF"
	    CHECK_PACKETS_FUNC "$PACKET_TOOL" "$YUM_LIBS_NEED" "$COL_YEL\r\033[25C( needed )$COL_DEF"
	    CHECK_PACKETS_FUNC "$PACKET_TOOL" "$YUM_LIBS_OPTI" "$COL_GRE\r\033[25C( option )$COL_DEF"
	else
	    PRINT_ERROR_MESSAGE_LINE_FUNC "$COL_RED_HIGH FATAL ERROR: unknown manager or packet tool $COL_DEF" "-e"
	fi
	if [ "$ERROR" != "" ]
	    then
	    PRINT_NEW_LINE_FUNC 1
	    PRINT_ERROR_MESSAGE_LINE_FUNC "We can not find the Packets:" "-e"
	    PRINT_ERROR_MESSAGE_LINE_FUNC "$COL_RED_HIGH$ERROR$COL_DEF$COL_RED." "-e"
	    PRINT_ERROR_MESSAGE_LINE_FUNC "install it and go on the installation? $YES_NO" "-ne"
	    AUTO_INSTALL_FUNC ; YES_NO_FUNC
	    if [ "$ANSWER" = false ] ; then	PRINT_NEW_LINE_FUNC 2 ; exit ; fi
	fi
    else
	PRINT_NEW_LINE_FUNC 1
 	PRINT_ERROR_MESSAGE_LINE_FUNC "$COL_RED_HIGH FATAL ERROR: unknown manager or packet tool $COL_DEF" "-e"
	PRINT_NEW_LINE_FUNC 2
	exit
    fi
fi




############################################### PREPAIRING SYSTEM FOR INSTALLATION ##############################################################

					              # PAGE TITLE_LINE #
PRINT_PAGE_HEAD_LINE_FUNC "Prepairing the System installation" "-e"

					       # Define MACKER for this funktion #
ERROR=""                        ;            ERROR_SAVE=""       

                                          # Begin with find PKG and gmerlin COMPONENTS #
PRINT_HEAD_LINE_FUNC "Prepaire the SYSTEM conditions:"
if [ "$ANSWER" = true ]
    then
    PRINT_COMMENT_LINE_FUNC "(Build PGK_CONFIG and find/download gmerlin Packets)"
   
                                                   # Build PKG_CONFIG_PATH #     
    PRINT_INFO_LINE_FUNC "pkg_config_path"
    FIND_PKG_FUNC "$LOGS/BUG_pkg"
    if test $? = 0 ; then echo -e "$OK\033[K" ; else echo -e "$FAIL\033[K" ; fi
    export PKG_CONFIG_PATH=$PKG_CONF_NEW
    READY_EXIT_FUNC "$COL_DEF Can not export$COL_RED_LINE_HIGH PKG_CONFIG_PATH $COL_DEF"
    
                                                # Find GMERLIN components #
    PRINT_INFO_LINE_FUNC "find $GMERLIN_DEPENDENCIES_PACKETS_NAME"
    echo -ne "$POSITION_STATUS Please wait ..."
    cd $INSTALL_HOME
    READY_EXIT_FUNC "$COL_DEF Can not change to $COL_RED_LINE_HIGH$INSTALL_HOME$COL_DEF directory"
    test -f "$GMERLIN_DEPENDENCIES_PACKETS_NAME"    
    if test $? = 0
	then
	cp $GMERLIN_DEPENDENCIES_PACKETS_NAME $HOME ; READY_EXIT_FUNC "Can not copy file"
	echo -e "$OK\033[K"
    else
	cd $HOME
	READY_EXIT_FUNC "$COL_DEF Can not change to $COL_RED_LINE_HIGH$HOME$COL_DEF directory"
	test -f "$GMERLIN_DEPENDENCIES_PACKETS_NAME"    
	if test $? = 0
	    then
	    cp $GMERLIN_DEPENDENCIES_PACKETS_NAME $INSTALL_HOME ; READY_EXIT_FUNC "Can not copy file"
	    echo -e "$OK\033[K"
	else
	    ERROR_SAVE=$ERROR
	    ERROR="$ERROR_SAVE $GMERLIN_DEPENDENCIES_PACKETS_NAME"
	    echo -e "$FAIL\033[K"
	fi
    fi
    PRINT_INFO_LINE_FUNC "find $GMERLIN_ALL_IN_ONE_PACKETS_NAME"
    echo -ne "$POSITION_STATUS Please wait ..."
    cd $INSTALL_HOME
    READY_EXIT_FUNC "$COL_DEF Can not change to $COL_RED_LINE_HIGH$INSTALL_HOME$COL_DEF directory"
    test -f "$GMERLIN_ALL_IN_ONE_PACKETS_NAME"    
    if test $? = 0
	then
	cp $GMERLIN_ALL_IN_ONE_PACKETS_NAME $HOME ; READY_EXIT_FUNC "Can not copy file"
	echo -e "$OK\033[K"
    else
	cd $HOME
	READY_EXIT_FUNC "$COL_DEF Can not change to $COL_RED_LINE_HIGH$HOME$COL_DEF directory"
	test -f "$GMERLIN_ALL_IN_ONE_PACKETS_NAME"    
	if test $? = 0
	    then
	    cp $GMERLIN_ALL_IN_ONE_PACKETS_NAME $INSTALL_HOME ; READY_EXIT_FUNC "Can not copy file"
	    echo -e "$OK\033[K"
	else
	    ERROR_SAVE=$ERROR
	    ERROR="$ERROR_SAVE $GMERLIN_ALL_IN_ONE_PACKETS_NAME"
	    echo -e "$FAIL\033[K"
	fi
    fi
    
                                               # If error SEND user Message #
    if [ "$ERROR" != "" ]
	then
	PRINT_NEW_LINE_FUNC 1
 	PRINT_ERROR_MESSAGE_LINE_FUNC "On the System we does not found all Packets:" "-e"
 	PRINT_ERROR_MESSAGE_LINE_FUNC "The Packet are important for installing Gmerlin" "-e"
	if [ "$AUTO_INSTALL" = false ]
	    then
	    PRINT_ERROR_MESSAGE_LINE_FUNC "The script can download it? $YES_NO" "-ne"
	    AUTO_INSTALL_FUNC ; YES_NO_FUNC
	    if [ "$ANSWER" = false ]
		then
		PRINT_ERROR_MESSAGE_LINE_FUNC "Download to $INSTALL_HOME" "-e"
		PRINT_ERROR_MESSAGE_LINE_FUNC "Download it jet and go on? $YES_NO" "-ne"
		AUTO_INSTALL_FUNC ; YES_NO_FUNC ; HAND=true
		if [ "$ANSWER" = false ] ; then	PRINT_NEW_LINE_FUNC 2 ; exit ; fi
	    fi
	fi
    fi	

					       # Check and DOWNLOAD too #
   ERROR_SAVE=false ; SAVE=""
   if [ "$ERROR" != "" ]
       then
       if [ "$HAND" = true ]
	   then
	   for i in $ERROR
	     do
	     PRINT_INFO_LINE_FUNC "find $i"
	     echo -ne "$POSITION_STATUS Please wait ..."
	     cd $INSTALL_HOME
	     READY_EXIT_FUNC "$COL_DEF Can not change to $COL_RED_LINE_HIGH$INSTALL_HOME$COL_DEF directory"
	     test -f "$i"    
	     if test $? = 0
		 then
		 cp $i $HOME ; READY_EXIT_FUNC "Can not copy file"
		 echo -e "$OK\033[K"
	     else
		 cd $HOME
		 READY_EXIT_FUNC "$COL_DEF Can not change to $COL_RED_LINE_HIGH$HOME$COL_DEF directory"
		 test -f "$i"    
		 if test $? = 0
		     then
		     cp $i $INSTALL_HOME ; READY_EXIT_FUNC "Can not copy file"
		     echo -e "$OK\033[K"
		 else
		     ERROR_SAVE=true
		     echo -e "$FAIL\033[K"
		 fi
	     fi
	   done
       else
	   for i in $ERROR
	     do
	     SAVE=$i
    	     DOWNLOAD_NEWEST_PACKET_FUNC $i
	     if test $? = 0
		 then
		 cp $SAVE $INSTALL_HOME ; READY_EXIT_FUNC "Can not copy file"
		 echo -e "$OK\033[K"
	     else
		 ERROR_SAVE=true
		 DEL_FILE_FUNC $SAVE "Can not delete file"
		 echo -e "$FAIL\033[K"
	     fi
	   done
       fi
    fi
						 # If ERROR too #
   if [ "$ERROR_SAVE" = true ]
       then
       PRINT_NEW_LINE_FUNC 1
       PRINT_ERROR_MESSAGE_LINE_FUNC "Downloading the needed Packets brocken:" "-e"
       PRINT_ERROR_MESSAGE_LINE_FUNC "Please look at$COL_RED_HIGH *.wget.log files$COL_DEF$COL_RED to find the error and restart the script" "-e"
       PRINT_NEW_LINE_FUNC 2
       exit
   fi
fi
						# Extra check DIRECKTORY #
cd $HOME
READY_EXIT_FUNC "$COL_DEF Can not change to $COL_RED_LINE_HIGH$HOME$COL_DEF directory"




############################################### UNPACKING AND CHECK THE PACKETS ##############################################################

					       # Define MACKER for this funktion #
ERROR=false                        ;            ERROR_SAVE=false       

                                                # UNPACK AND CHECK THE PACKETS #
PRINT_HEAD_LINE_FUNC "Unpack/check the Gmerlin Packets:"
if [ "$ANSWER" = true ]
    then
    PRINT_COMMENT_LINE_FUNC "(After unpack, check if the Packets complite)"
    for i in $ALL_PACKS
	do
	PRINT_INFO_LINE_FUNC "unpack $i"
	DEL_DIRECTORY_FUNC `echo $i| awk -F "." '{print $1}'` "$COL_DEF Can not delete $COL_RED_LINE_HIGH`echo $i| awk -F "." '{print $1}'`$COL_DEF directory"
	tar -xvjf $i >& $LOGS/$i.tar.log 
	if test $? = 0
	    then
	    DEL_FILE_FUNC "$LOGS/$i.tar.log" "Can not delete file"
	    echo -e "$OK\033[K"
	else
	    cp $LOGS/$i.tar.log $INSTALL_HOME/$1.tar.log ; READY_EXIT_FUNC "Can not copy file"
	    ERROR_SAVE=true
	    echo -e "$FAIL\033[K"
	fi
    done
    for i in $ALL_PACKS
	do
	PRINT_COMMENT_2_LINE_FUNC "Check `echo $i| awk -F "." '{print $1}'` are complete:"
	DIR=`echo $i| awk -F "." '{print $1}'`
	cd $DIR ; READY_EXIT_FUNC "$COL_DEF Can not change to $COL_RED_LINE_HIGH$DIR$COL_DEF directory"
	if test -f dirs 
	    then
	    for i in `cat dirs`
	      do
	      PRINT_INFO_LINE_FUNC "check $i"
	      if test -d $i ; then echo -e "$OK2"
	      else ERROR=true ; echo -e "$FAIL2"
	      fi
	    done
	    PRINT_INFO_LINE_FUNC "$COL_GRE$DIR"
	    if [ "$ERROR" = false ] ; then echo -e "$OK"
	    else ERROR_SAVE=true ; echo -e "$FAIL"
	    fi
	else PRINT_INFO_LINE_FUNC "$COL_RED$DIR" ; ERROR_SAVE=true ; echo -e "$FAIL"
	fi
	cd $HOME ; READY_EXIT_FUNC "$COL_DEF Can not change to $COL_RED_LINE_HIGH$HOME$COL_DEF directory"
    done
						 # If ERROR #
    if [ "$ERROR_SAVE" = true ]
	then
	PRINT_NEW_LINE_FUNC 1
	PRINT_ERROR_MESSAGE_LINE_FUNC "The Packets or the tar Archive are not complete:" "-e"
	PRINT_ERROR_MESSAGE_LINE_FUNC "Please look at$COL_RED_HIGH *.log files$COL_DEF$COL_RED to find the error and restart the script" "-e"
	PRINT_NEW_LINE_FUNC 2
	exit
    fi
fi




############################################### INSTALL THE GMERLIN PACKETS ##############################################################

					              # PAGE TITLE_LINE #
PRINT_PAGE_HEAD_LINE_FUNC "Installation of Gmerlin Packets" "-e"

					       # Define MACKER for this funktion #
ERROR=false                        ;            ERROR_SAVE=""       

                                          # Begin with find PKG and gmerlin COMPONENTS #
PRINT_HEAD_LINE_FUNC "Install the Gmerlin Packets:"
if [ "$ANSWER" = true ]
    then
    PRINT_COMMENT_LINE_FUNC "(Install to /opt/gmerlin/...)"
    for i in $ALL_PACKS
	do
	PRINT_COMMENT_2_LINE_FUNC "Install `echo $i| awk -F "." '{print $1}'`:"
	echo -ne "\033[1A$POSITION_DISKRIPTION$YES_NO_EXIT"
	AUTO_CHECK_FUNC ; YES_NO_EXIT_FUNC
	if [ "$ANSWER" = true ]
	    then
	    DIR=`echo $i| awk -F "." '{print $1}'`
	    cd $DIR ; READY_EXIT_FUNC "$COL_DEF Can not change to $COL_RED_LINE_HIGH$DIR$COL_DEF directory"
	    if test -f dirs 
		then
		for i in `cat dirs`
		  do
		  PACK=$i
		  PRINT_INFO_LINE_FUNC "Install $i"
		  ./build.sh $i 2>&1 | tee $LOGS/$PACK.build.log | grep -n "" | awk -F ':' '{printf "   %5s" , $1 ; system("echo -ne \"\033[8D\"") }'
		  grep -q 'build.sh completed successfully' $LOGS/$PACK.build.log
		  if [ "$?" = "0" ]
		      then
		      DEL_FILE_FUNC "$LOGS/$PACK.build.log" "Can not delete file"
		      echo -e "$OK2"
		  else
		      
		      #Save config.log !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		      
		      ERROR=true
		      echo -e "$FAIL2"
		  fi
		done
		PRINT_INFO_LINE_FUNC "$COL_GRE$DIR"
		if [ "$ERROR" = false ]
		    then
		    echo -e "$OK"
		else
		    ERROR_SAVE=true
		    echo -e "$FAIL"
		fi
	    else
		PRINT_INFO_LINE_FUNC "$COL_RED$DIR"
		ERROR_SAVE=true
		echo -e "$FAIL"
	    fi
	    cd $HOME ; READY_EXIT_FUNC "$COL_DEF Can not change to $COL_RED_LINE_HIGH$HOME$COL_DEF directory"
	fi
    done
fi




############################################### CLEAN UP AND EXIT INSTALLATION ##############################################################

cd $INSTALL_HOME ; READY_EXIT_FUNC "$COL_DEF Can not change to $COL_RED_LINE_HIGH$HOME$COL_DEF directory"
export PKG_CONFIG_PATH=$PKG_CONF_OLD
READY_EXIT_FUNC "$COL_DEF Can not export$COL_RED_LINE_HIGH PKG_CONFIG_PATH $COL_DEF"
#tar cvjf gmerlin_logs.tar.bz2 $LOGS/ >& $LOGS/tar.logs.log
#rm *.log
