#!/bin/bash
#
# Author: Jochen Thaeder (LBNL) <jochen@thaeder.de>
# Run script to produce webpage of data usage
#
###################################################

BASEPATH=/global/homes/j/jthaeder/pdsfDiskUsageMonitor

reCreateTree=0

pushd ${BASEPATH} > /dev/null

# -- get latest PROJECTA file
projectAFile=`readlink /global/projecta/statistics/tlprojecta/projecta.listall`

# -- get latest PROJECT folder
projectFolders=`ls -t /project/statistics/DIBS/tlproject2/ | grep -v tarinput 2> /dev/null | head -n 10 | sort -r 2> /dev/null`
for folder in $projectFolders ; do
    if [[ ! -f /project/statistics/DIBS/tlproject2/${folder}/prj2-starprod.list ||
		! -f /project/statistics/DIBS/tlproject2/${folder}/prj2-star.list ||
		! -f /project/statistics/DIBS/tlproject2/${folder}/prj2-alice.list  ]]; then
	continue
    fi
    
    sizeStarProd=`stat -c %s /project/statistics/DIBS/tlproject2/${folder}/prj2-starprod.list`
    sizeStar=`stat -c %s /project/statistics/DIBS/tlproject2/${folder}/prj2-star.list`
    sizeAlice=`stat -c %s /project/statistics/DIBS/tlproject2/${folder}/prj2-alice.list`

    if [[ $sizeStarProd -lt 1000 || $sizeStar -lt 1000 || $sizeAlice -lt 1000 ]] ; then
	continue
    fi

    projectFolder=$folder
    break
done

# -- Get modification dates 
modDatePROJECT=`stat -c %y /project/statistics/DIBS/tlproject2/${projectFolder}/prj2-starprod.list | cut -d' ' -f 1` 
modDatePROJECTA=`stat -c %y /global/projecta/statistics/tlprojecta/${projectAFile} | cut -d' ' -f 1` 

# -- Get old modification dates and check if tree recreation has to be run
if [ -f modDatePROJECT.txt ] ; then 
    oldmodDatePROJECT=`cat modDatePROJECT.txt`
    if [ "$oldmodDatePROJECT" != "$modDatePROJECT" ] ; then 
	reCreateTree=1
    fi
else
    reCreateTree=1
fi

if [ -f modDatePROJECTA.txt ] ; then 
    oldmodDatePROJECTA=`cat modDatePROJECTA.txt`
    if [ "$oldmodDatePROJECTA" != "$modDatePROJECTA" ] ; then 
	reCreateTree=1
    fi
else
    reCreateTree=1
fi

#DEBUGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG
    reCreateTree=1

# -- recreate input Trees
# -------------------------------------------------------
if [ $reCreateTree -eq 1 ] ; then

    # -- get projectA file
    if [ -d projecta ] ; then 
	rm -f projecta/*.list
    else
	mkdir -p projecta
    fi

    grep "/global/projecta/projectdirs/starprod/" /global/projecta/statistics/tlprojecta/${projectAFile} > projecta/prjA-starprod.list

    # -- get project folder
    if [ -d project ] ; then 
	rm -f project/*.list
    else
	mkdir -p project
    fi

    ln -s /project/statistics/DIBS/tlproject2/${projectFolder}/prj2-starprod.list project/
    ln -s /project/statistics/DIBS/tlproject2/${projectFolder}/prj2-star.list project/
    ln -s /project/statistics/DIBS/tlproject2/${projectFolder}/prj2-alice.list project/

    echo $modDatePROJECT > modDatePROJECT.txt
    echo $modDatePROJECTA > modDatePROJECTA.txt

    rm -f treeOutput.root

    # -- run script
    ${BASEPATH}/parseGPFSDump.tcsh ${BASEPATH} 0

    ${BASEPATH}/parseGPFSDump.tcsh ${BASEPATH} 1

    if [ -d output ] ; then 
	rm -f output/*.list
    else
	mkdir -p output
    fi

    mv outfile_* output/
fi

# -- create html
${BASEPATH}/runDiskUsage.sh  ${BASEPATH} 

popd > /dev/null

