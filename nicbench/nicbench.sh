#

usage()
{
    echo "$0 name buf-size team ether0 [ether1 [... ether7]]"
    echo "    team: ALB AFT FEC GEC 802.3ad FEC/LA/802.3ad GEC/LA/802.3ad"
}

if [ $# -lt 5 ]
then
    usage
    exit
fi

name=$1
bufsize=$2
team=$3

shift
shift
shift

case $# in
1) ip="10.10.0.100" ;;
2) ip="10.10.0.100 10.10.1.100" ;;
3) ip="10.10.0.100 10.10.1.100 10.10.2.100" ;;
4) ip="10.10.0.100 10.10.1.100 10.10.2.100 10.10.3.100" ;;
5) ip="10.10.0.100 10.10.1.100 10.10.2.100 10.10.3.100 10.10.4.100" ;;
6) ip="10.10.0.100 10.10.1.100 10.10.2.100 10.10.3.100 10.10.4.100 10.10.5.100" ;;
7) ip="10.10.0.100 10.10.1.100 10.10.2.100 10.10.3.100 10.10.4.100 10.10.5.100 10.10.6.100" ;;
8) ip="10.10.0.100 10.10.1.100 10.10.2.100 10.10.3.100 10.10.4.100 10.10.5.100 10.10.6.100 10.10.7.100" ;;
*) usage; exit;;
esac

echo sh nicsetup.sh ${team} $@
sh nicsetup.sh  ${team} $@

case ${team} in
  802.3ad) team="8023ad" ;;
  FEC/LA/802.3ad) team="FECLA8023ad" ;;
  GEC/LA/802.3ad) team="GECLA8023ad" ;;
esac

echo "./client_ctl2 ./client2 ${bufsize} 60 ${ip} | tee ${name}_${team}.log"
./client_ctl2 ./client2 ${bufsize} 60 ${ip} | tee ${name}_${team}.log

echo "sh nicsetup.sh up $1"
sh nicsetup.sh up $1

echo "./client_ctl2 ./client2 ${bufsize} 60 ${ip} | tee ${name}.log"
./client_ctl2 ./client2 ${bufsize} 60 ${ip} | tee ${name}.log

echo "cp -i ${name}.log log"
cp -i ${name}.log log

echo "cp -i ${name}_${team}.log log"
cp -i ${name}_${team}.log log
