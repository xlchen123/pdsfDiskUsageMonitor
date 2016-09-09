#!/bin/bash
#
# Author: Jochen Thaeder (LBNL) <jochen@thaeder.de>
# Run script to produce webpage of data usage
#
###################################################

BASEPATH=$PWD

if [ $# -eq 1 ] ; then 
    LOGDIR=$1
    isSGE=1
else
    isSGE=0
fi

reCreateTree=0

pushd ${BASEPATH} > /dev/null
inputPath=/project/statistics/LIST

# -- get latest PROJECT/PROJECTA folder
for prjFolder in project projecta  ; do

    if [ "${prjFolder}" == "project" ] ; then 
	prjPath=tlproject2
    elif [ "${prjFolder}" == "projecta" ] ; then 
	prjPath=tlprojecta
    fi
    
    projectFolders=`ls -t ${inputPath}/${prjPath}/ | head -n 10 | sort -r 2> /dev/null`

    for folder in $projectFolders ; do

	# -- Get file in latest folder
	inFile=`ls ${inputPath}/${prjPath}/${folder}/*list.allfiles | sort | head -n 1 2> /dev/null`
	if [ ! -f ${inFile} ] ; then 
	    continue
	fi
	
	# -- Check the latest file is completed 
	if [ ! -f ${inFile}.completed ] ; then 
	    continue
	fi

	# -- Get old modification dates and check if tree recreation has to be run
	#    Always run in SGE job
	if [[ ! $isSGE && -f modDate_${prjFolder}.txt ]] ; then 
	    oldmodDate=`cat modDate_${prjFolder}.txt`
	    if [ "$oldmodDate" == "$folder" ] ; then 
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

	if [ "${prjFolder}" == "project" ] ; then 
	    cat ${inFile} | grep "%2Fprojectdirs%2Fstarprod%2F" > ${prjFolder}/prj-starprod.list
	    sed -i "s/%2F${prjFolder}%2F.snapshots%2F${folder}%2Fprojectdirs%2F//" ${prjFolder}/prj-starprod.list

	    cat ${inFile} | grep "%2Fprojectdirs%2Fstar%2F" > ${prjFolder}/prj-star.list
	    sed -i "s/%2F${prjFolder}%2F.snapshots%2F${folder}%2Fprojectdirs%2F//" ${prjFolder}/prj-star.list
	    
	    cat ${inFile} | grep "%2Fprojectdirs%2Falice%2F" > ${prjFolder}/prj-alice.list
	    sed -i "s/%2F${prjFolder}%2F.snapshots%2F${folder}%2Fprojectdirs%2F//" ${prjFolder}/prj-alice.list
	else
	    cat ${inFile} | grep "%2Fprojectdirs%2Fstarprod%2F" > ${prjFolder}/prj-starprod.list
	    sed -i "s/%2Fglobal%2F${prjFolder}%2F.snapshots%2F${folder}%2Fprojectdirs%2F//" ${prjFolder}/prj-starprod.list
	fi

	echo $folder > modDate_${prjFolder}.txt

	break
    done
done

# -- recreate input Trees
# -------------------------------------------------------
if [ $reCreateTree -eq 1 ] ; then

    if [ -d output ] ; then 
	rm -rf output
    fi

    mkdir -p output

    # -- run script
    ${BASEPATH}/parseGPFSDump.tcsh ${BASEPATH} 0

    ${BASEPATH}/parseGPFSDump.tcsh ${BASEPATH} 1
fi

# -- create html
${BASEPATH}/runDiskUsage.sh ${BASEPATH} 

popd > /dev/null

