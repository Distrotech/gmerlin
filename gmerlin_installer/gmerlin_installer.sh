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

                                              # Define the PLAUM variables #
DATE="20051226"

                                            # Define the old PKG_CONFIG_PATH #
OLD_PKG_CONFIG=$PKG_CONFIG_PATH

                                             # Define the BUG MAIL address #
MAIL="one78@web.de"

                                           # Define the INSTALLATION directorys #
LOGS=".gmerlin_logs"     ;     HELPS=".gmerlin_helps"     ;      HOME="GMerlin_INstallation"

                                              # Define the ALL-IN-ONE packs #
GMERLIN_ALL_IN_ONE_PACKETS_NAME="gmerlin-all-in-one-$DATE"

                                             # Define the DEPENDENCIE packs #
GMERLIN_DEPENDENCIES_PACKETS_NAME="gmerlin-dependencies-$DATE"

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
YES_NO_EXIT="$COL_DEF$POSITION_DISKRIPTION[ \033[32;1mY\033[0mES,\033[31;1mN\033[0mO,E\033[33;1mX\033[0mIT ]\033[5C Y\033[1D"
FAIL="$POSITION_STATUS[$COL_RED_HIGH FAIL $COL_DEF]"
YES_NO="$COL_DEF$POSITION_DISKRIPTION[ \033[32;1mY\033[0mES,\033[31;1mN\033[0mO ]\033[5C Y\033[1D"

					###### Define FUNCTIONS how need ######

					        # Page DESIGN for text #
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
    return 0
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
fi
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
 	PRINT_ERROR_MESSAGE_LINE_FUNC "On the System we doesn't found following Packets:" "-e"
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
PRINT_HEAD_LINE_FUNC "Check the System SYSTEM tools:"
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
 	PRINT_ERROR_MESSAGE_LINE_FUNC "On the System we doesn't found following Packets:" "-e"
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
	    for i in $ERROR
	      do
	      PRINT_INFO_LINE_FUNC "$MANAGER $i"
	      TAKE_PACKETS_FUNC $MANAGER $i "$LOGS/BUG_$i"
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
	fi
    fi
    
                                                 # If ERROR too #
    if [ "$ERROR_SAVE" != "" ]
	then
	PRINT_NEW_LINE_FUNC 1
 	PRINT_ERROR_MESSAGE_LINE_FUNC "On the System we doesn't found all needed Packets:" "-e"
 	PRINT_ERROR_MESSAGE_LINE_FUNC "Please look at$COL_RED_HIGH BUG_* files$COL_DEF$COL_RED to find the error and restart the script" "-e"
	PRINT_NEW_LINE_FUNC 2
	exit
    fi
fi




############################################### CHECK AND INSTALL SYSTEM TOOLS ##############################################################

					     # PAGE TITLE_LINE AFTER WELCOME #
PRINT_PAGE_HEAD_LINE_FUNC "Check and/or install the System librarys" "-e"


					    # Define MACKER for this funktion #
ERROR=""                ;            ERROR_SAVE=""       ;        DUMP=false   

                                             # Begin with the CHECKING tools #
PRINT_HEAD_LINE_FUNC "Check the System SYSTEM librarys:"
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
	echo "YES MANAGER && YES PACKET_TOOL"
# YES MANAGER && NO PACKET_TOOL
    elif [ "$MANAGER" != "" -a "$PACKET_TOOL" = "" ]
	then
	PRINT_COMMENT_2_LINE_FUNC "No Packet Tool like RPM/DPKG found, about this we can only install"
	PRINT_COMMENT_2_LINE_FUNC "the following Packets. ( This can take a little bit longer )"
	if [ "$MANAGER" = "apt-get" ]
	    then
	    for i in $APT_LIBS_IMPO
		do
		PRINT_INFO_LINE_FUNC "$i" "$COL_RED\r\033[25C( import )$COL_DEF "
		TAKE_PACKETS_FUNC $MANAGER $i "$LOGS/BUG_$i"
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
	    for i in $APT_LIBS_NEED
		do
		PRINT_INFO_LINE_FUNC "$i" "$COL_YEL\r\033[25C( needed )$COL_DEF"
		TAKE_PACKETS_FUNC $MANAGER $i "$LOGS/BUG_$i"
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
	    for i in $APT_LIBS_OPTI
		do
		PRINT_INFO_LINE_FUNC "$i" "$COL_GRE\r\033[25C( option )$COL_DEF"
		TAKE_PACKETS_FUNC $MANAGER $i "$LOGS/BUG_$i"
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
	elif [ "$MANAGER" = "yum" ]
	    then
	    for i in $YUM_LIBS_IMPO
		do
		PRINT_INFO_LINE_FUNC "$i" "$COL_RED\r\033[25C( import )$COL_DEF "
		TAKE_PACKETS_FUNC $MANAGER $i "$LOGS/BUG_$i"
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
	    for i in $YUM_LIBS_NEED
		do
		PRINT_INFO_LINE_FUNC "$i" "$COL_YEL\r\033[25C( needed )$COL_DEF"
		TAKE_PACKETS_FUNC $MANAGER $i "$LOGS/BUG_$i"
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
	    for i in $YUM_LIBS_OPTI
		do
		PRINT_INFO_LINE_FUNC "$i" "$COL_GRE\r\033[25C( option )$COL_DEF"
		TAKE_PACKETS_FUNC $MANAGER $i "$LOGS/BUG_$i"
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
	    PRINT_ERROR_MESSAGE_LINE_FUNC "$COL_RED_HIGH FATAL ERROR: unknown manager or packet tool $COL_DEF" "-e"
	fi
# NO MANAGER && YES PACKET_TOOL
    elif [ "$MANAGER" = "" -a "$PACKET_TOOL" != "" ]
	then
	PRINT_COMMENT_2_LINE_FUNC "No Packet Manager like YUM/APT-GET found, about this we can only check"
	PRINT_COMMENT_2_LINE_FUNC "the following Packets. Pleace install the failed Packets from Hand."
	PRINT_NEW_LINE_FUNC 1
	
    else
	PRINT_NEW_LINE_FUNC 1
 	PRINT_ERROR_MESSAGE_LINE_FUNC "$COL_RED_HIGH FATAL ERROR: unknown manager or packet tool $COL_DEF" "-e"
	PRINT_NEW_LINE_FUNC 2
	exit
    fi
fi

