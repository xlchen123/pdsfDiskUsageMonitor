#!/bin/bash
#
# Author: Jochen Thaeder (LBNL) <jochen@thaeder.de>
# Run script to produce webpage of data usage
#
###################################################

BASEPATH=/global/homes/j/jthaeder/pdsfDiskUsageMonitor

reCreateTree=0

pushd ${BASEPATH} > /dev/null

inputPath=/project/statistics/LIST


# -- get latest PROJECTA file
projectAFile=`readlink /global/projecta/statistics/tlprojecta/projecta.listall`




# -- get latest PROJECT folder
projectFolders=`ls -t ${inputPath}/tlproject2/ | head -n 10 | sort -r 2> /dev/null`
for folder in $projectFolders ; do

    # -- Get file in latest folder
    inFile=`ls ${inputPath}/tlproject2/${folder}/*list.allfiles`
    if [ ! -f ${inFile} ] ; then 
	continue
    fi
	
    # -- Get old modification dates and check if tree recreation has to be run
    modDatePROJECT=${folder}

    if [ -f modDatePROJECT.txt ] ; then 
	oldmodDatePROJECT=`cat modDatePROJECT.txt`
	if [ "$oldmodDatePROJECT" == "$modDatePROJECT" ] ; then 
	    break
	fi
    fi

    reCreateTree=1

    # -- Create input files for parsing
    
    # -- Get project folder
    if [ -d project ] ; then 
	rm -f project/*.list
    else
	mkdir -p project
    fi 
    
    # -- Get input files for parsing
    cat ${inFile} | grep "%2Fprojectdirs%2Fstar%2F" > project/prj2-star.list
    sed -i "s/%2Fproject%2F.snapshots%2F${folder}%2Fprojectdirs%2F//" project/prj2-star.list
    
    cat ${inFile} | grep "%2Fprojectdirs%2Fstarprod%2F" > project/prj2-starprod.list
    sed -i "s/%2Fproject%2F.snapshots%2F${folder}%2Fprojectdirs%2F//" project/prj2-starprod.list
    
    cat ${inFile} | grep "%2Fprojectdirs%2Falice%2F" > project/prj2-alice.list
    sed -i "s/%2Fproject%2F.snapshots%2F${folder}%2Fprojectdirs%2F//" project/prj2-alice.list

    echo $modDatePROJECT > modDatePROJECT.txt

    projectFolder=$folder
    break
done

# -- Get modification dates 
modDatePROJECTA=`stat -c %y /global/projecta/statistics/tlprojecta/${projectAFile} | cut -d' ' -f 1` 

if [ -f modDatePROJECTA.txt ] ; then 
    oldmodDatePROJECTA=`cat modDatePROJECTA.txt`
    if [ "$oldmodDatePROJECTA" != "$modDatePROJECTA" ] ; then 
	reCreateTree=1
    fi
else
    reCreateTree=1
fi


echo $reCreateTree done ...

exit

reCreateTree=1   #debug

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

    if [ -d output ] ; then 
	rm -rf output
    fi

    mkdir -p output

    echo $modDatePROJECTA > modDatePROJECTA.txt

   # rm -f treeOutput.root

    # -- run script
    ${BASEPATH}/parseGPFSDump.tcsh ${BASEPATH} 0

    ${BASEPATH}/parseGPFSDump.tcsh ${BASEPATH} 1
fi

# -- create html
${BASEPATH}/runDiskUsage.sh  ${BASEPATH} 

popd > /dev/null

