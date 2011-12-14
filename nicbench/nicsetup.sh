#
local_IP="10.10.0.1"

downall()
{
    #echo "ifconfig vadat down"
    ifconfig vadat down > /dev/null 2>&1

    #echo "ianscfg -d -t TEAM -r"
    ianscfg -d -t AFTteam -r > /dev/null 2>&1

    for if in 0 1 2 3 4 5 6 7 8 9
    do
	#echo ifconfig eth${if} down
	ifconfig eth$if down > /dev/null 2>&1
	#echo ifconfig sock${if} down
	ifconfig sock$if down > /dev/null 2>&1
    done
}

rmmodall()
{
    downall

    rmmod eepro100 e100 e100org e1000 e1000org ians
    rmmod eepro100 e100 e100org e1000 e1000org ians > /dev/null 2>&1
}

install()
{
    rmmodall

    #echo "insmod e100"
    insmod e100 > /dev/null 2>&1
    sleep 5

    #echo "insmod e1000"
    insmod e1000 > /dev/null 2>&1
    sleep 5

    downall
}

usage()
{
    echo "Usage: $0 up <ether>"
    echo "       $0 <TEAM> <ether0> [<ether1> ... <ether7>]"
    echo "       $0 down"
    echo "       $0 rmmod"
}

case "$1" in
  down)
    downall ;;

  rmmod)
    rmmodall ;;

  up)

    teamtype=$1 ;
    shift ;
    no_port=$# ;

    if [ $# -ne 1 ]
    then usage; exit;
    fi ;

    install

    #echo "ifconfig $1 up $local_IP netmask 255.0.0.0" ;
    ifconfig $1 up $local_IP netmask 255.0.0.0 ;;

  ALB|AFT|FEC|GEC|802.3ad|FEC/LA/802.3ad|GEC/LA/802.3ad)

    teamtype=$1 ;
    shift ;
    no_port=$# ;

    if [ $# -lt 1 ]
    then usage; exit;
    fi ;
    if [ $# -ge 9 ]
    then usage; exit;
    fi;
	
    install ;
    #echo "insmod ians"
    #insmod ians
    #sleep 5

    echo "ianscfg -a -t TEAM -M ${teamtype}" ;
    ianscfg -a -t TEAM -M ${teamtype} ;

    echo "ianscfg -C -t TEAM -M ${teamtype}" ;
    ianscfg -C -t TEAM -M ${teamtype} ;

    echo "ianscfg -a -t TEAM -m $1 -p primary" ;
    ianscfg -a -t TEAM -m $1 -p primary ;

    shift ;
    if [ $# -ge 1 ]
    then
	echo "ianscfg -a -t TEAM -m $1 -p secondary" ;
	ianscfg -a -t TEAM -m $1 -p secondary ;
	shift ;
	while [ $# -ge 1 ]
	do
	    echo "ianscfg -a -t TEAM -m $1 -p none" ;
	    ianscfg -a -t TEAM -m $1 -p none ;
	    shift ;
	done ;
    fi ;
    echo "ianscfg -a -t TEAM -v vadat" ;
    ianscfg -a -t TEAM -v vadat ;
    echo "ianscfg -c TEAM" ;
    ianscfg -c TEAM ;

    #echo "ifconfig vadat up $local_IP netmask 255.0.0.0" ;
    ifconfig vadat up $local_IP netmask 255.0.0.0 ;
    
    ianscfg -s ;;

  *)
    usage;
    exit ;;
esac

ifconfig
lsmod | egrep 'Module|e100|eepro100|ians'
#lsmod
#sleep 5

