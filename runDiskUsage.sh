#!/bin/bash
#
# Author: Jochen Thaeder (LBNL) <jochen@thaeder.de>
# Run script to produce webpage of data usage
#
###################################################

if [ $# -eq 0 ] ; then 
    BASEPATH=`pwd`
else
    BASEPATH=$1
fi

# -- Load functions
source ${BASEPATH}/createWebPageFunctions.sh

now=`date +'%F %H:%m'`

folderList="data overview embedding picoDstSTAR pwgSTAR userRNC userALICE"

# -- Get modification dates
modDatePROJECT=`cat modDate_project.txt`
modDatePROJECTA=`cat modDate_projecta.txt`

pushd ${BASEPATH} > /dev/null  

# -- Check for WWW space
if [ `whoami` == "starofl" ] ; then
    wwwPath=/project/projectdirs/star/www/diskUsage/
else
    wwwPath=/project/projectdirs/star/www/`whoami`/diskUsage/
fi

if [ ! -h www ] ; then 
    ln -sf $wwwPath www
fi

# -- Prepare www
${wwwPath}
if [ ! -d ${wwwPath} ] ; then 
    mkdir -p ${wwwPath}/data
    chmod -R 755 ${wwwPath}/data

    # -- Get styles
    if [ !-d ${wwwPath}/style ] ; then 
	cp -a ${BASEPATH}/www-template/style ${wwwPath}
    fi

    # -- Get java script functions
    if [ ! -d ${wwwPath}/jsLibrary ] ; then 
	cp -a ${BASEPATH}/www-template/jsLibrary ${wwwPath}
    fi

    # -- Get sorttable.js
    if [ ! -f ${wwwPath}/jsLibrary/sorttable.js ] ; then 
	pushd  ${wwwPath}/jsLibrary/ > /dev/null
	wget http://www.kryogenix.org/code/browser/sorttable/sorttable.js
	popd  > /dev/null
    fi

    # -- Get jqTree
    if [ ! -d ${wwwPath}/jqTree ] ; then 
	pushd  ${wwwPath}  > /dev/null
	git clone https://github.com/mbraak/jqTree.git
	popd  > /dev/null
    fi
fi

# -- cp output to www
cp output/outfile*.js www/data
chmod 644 www/data/outfile*.js

# -- Create webpages
pushd www > /dev/null
for ii in ${folderList} ; do 
    mkdir -p ${ii}
    chmod 755 ${ii}
done
popd > /dev/null

printOverview overview
printEmbedding embedding

printUserRNC userRNC
printUserALICE userALICE

printPwgSTAR pwgSTAR
printPicoDSTs picoDstSTAR

pushd www > /dev/null
for ii in ${folderList} ; do 
    chmod 644 ${ii}/*.*
done

cp overview/index.html .
sed -i  's/\..\///' index.html

popd > /dev/null

popd > /dev/null

