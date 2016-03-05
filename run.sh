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


# -- get latest PROJECT/PROJECTA folder

for prjFolder in PROJECT PROJECTA  ; do

    if [ "${prjFolder}" == "project" ] ; then 
	prjPath=tlproject2
    elif [ "${prjFolder}" == "projecta" ] ; then 
	prjPath=tlprojecta
    fi
    
    projectFolders=`ls -t ${inputPath}/${prjPath}/ | head -n 10 | sort -r 2> /dev/null`
    for folder in $projectFolders ; do
	
	# -- Get file in latest folder
	inFile=`ls ${inputPath}/${prjPath}/${folder}/*list.allfiles`
	if [ ! -f ${inFile} ] ; then 
	    continue
	fi
	
	# -- Get old modification dates and check if tree recreation has to be run
	modDatePROJECT=${folder}
	
	if [ -f modDatePROJECT.txt ] ; then 
	    oldmodDatePROJECT=`cat modDate${prjFolder}.txt`
	    if [ "$oldmodDatePROJECT" == "$modDatePROJECT" ] ; then 
		break
	    fi
	fi
	
	reCreateTree=1
	
	# -- Create input files for parsing
	# ----------------------------------------------
	
	# -- Get project folder
	if [ -d ${prjFolder} ] ; then 
	    rm -f ${prjFolder}/*.list
	else
	    mkdir -p ${prjFolder}
	fi 
	
	# -- Get input files for parsing
	cat ${inFile} | grep "%2Fprojectdirs%2Fstarprod%2F" > ${prjFolder}/prj-starprod.list
	sed -i "s/%2F${prjFolder}%2F.snapshots%2F${folder}%2Fprojectdirs%2F//" ${prjFolder}/prj-starprod.list

	if [ "${prjFolder}" == "project" ] ; then 
	    cat ${inFile} | grep "%2Fprojectdirs%2Fstar%2F" > ${prjFolder}/prj-star.list
	    sed -i "s/%2F${prjFolder}%2F.snapshots%2F${folder}%2Fprojectdirs%2F//" ${prjFolder}/prj-star.list
	    
	    cat ${inFile} | grep "%2Fprojectdirs%2Falice%2F" > ${prjFolder}/prj-alice.list
	    sed -i "s/%2F${prjFolder}%2F.snapshots%2F${folder}%2Fprojectdirs%2F//" ${prjFolder}/prj-alice.list
	fi

	echo $modDatePROJECT > modDate${prjFolder}.txt
	
	projectFolder=$folder
	break
    done
done

reCreateTree=1   #debug

# -- recreate input Trees
# -------------------------------------------------------
if [ $reCreateTree -eq 1 ] ; then

    if [ -d output ] ; then 
	rm -rf output
    fi

    mkdir -p output

    # rm -f treeOutput.root

    # -- run script
    ${BASEPATH}/parseGPFSDump.tcsh ${BASEPATH} 0

    ${BASEPATH}/parseGPFSDump.tcsh ${BASEPATH} 1
fi

# -- create html
${BASEPATH}/runDiskUsage.sh  ${BASEPATH} 

popd > /dev/null

