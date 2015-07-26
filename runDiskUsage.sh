#!/bin/bash
#
# Author: Jochen Thaeder (LBNL) <jochen@thaeder.de>
# Run script to produce webpage of datasage
#
###################################################

BASEPATH=$1

modDateELIZA1=`cat modDateELIZA1.txt`
modDateELIZA2=`cat modDateELIZA2.txt`
modDatePROJECT=`cat modDatePROJECT.txt`

now=`date +'%F %H:%m'`

pushd ${BASEPATH} > /dev/null  

# -- cp output
cp output/outfile*.js www/data
chmod 644 www/data/outfile*.js

# -----------------------------------------------------------------------------------------------------------------
function printBegin() {
    typeTable=$1
    fileName=$2
    if [ $# -eq 3 ] ; then
	useJS=$3
    else
	useJS=YES
    fi
    
    cat >${fileName} <<EOL
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <title>RNC Data USAGAE AT NERSC/LBNL</title>
    
    <script src="../jsLibrary/sorttable.js"></script>
    <script src="http://code.jquery.com/jquery.min.js"></script>
    <script src="../jqTree/tree.jquery.js"></script>
    <link rel="stylesheet" href="../jqTree/jqtree.css">
    <link rel="stylesheet" href="../style/style.css">
EOL

    if [ "${typeTable}" = "alice_user" ] ; then
	dataFiles="alice_userv1 alice_userv2 alice_userv3"
    elif [ "${typeTable}" = "rnc_user" ] ; then
	dataFiles="rnc_userv1 rnc_userv2 rnc_userv3"
    elif [ "${typeTable}" = "embedding"  ] ; then
	dataFiles="embeddingv1 embeddingv2 embeddingv3 embeddingv4 embeddingv5"
    elif [ "${typeTable}" = "embedding_user"  ] ; then
	dataFiles="embeddingv4"
    elif [ "${typeTable}" = "picodsts"  ] ; then
	dataFiles="picodstsv1 picodstsv2 picodstsv3"
    elif [ "${typeTable}" = "pwgstar"  ] ; then
	dataFiles="pwgstarv1 pwgstarv2 pwgstarv3"
    elif [ "${typeTable}" = "overview"  ] ; then
	dataFiles="alice_userv1 alice_userv2 alice_userv3"
	dataFiles="${dataFiles} rnc_userv1 rnc_userv2 rnc_userv3"
	dataFiles="${dataFiles} embeddingv1 embeddingv2 embeddingv3 embeddingv4 embeddingv5"
	dataFiles="${dataFiles} picodstsv1 picodstsv2 picodstsv3"
	dataFiles="${dataFiles} pwgstarv1 pwgstarv2 pwgstarv3"
    elif [ "${typeTable}" = "overview_input"  ] ; then
	dataFiles="alice rnc star project_alice project_star project_starprod"
	dataFiles="${dataFiles} eliza6 eliza14 eliza15 eliza17 project"
    fi

    if [ "${useJS}" = "YES" ] ; then
	for dataFile in ${dataFiles} ; do 
	    addScript ${dataFile} ${fileName}; 
	done

	echo '    <script src="../jsLibrary/functions_'${typeTable}'.js"></script>' >> ${fileName}
    fi

    cat >>${fileName} <<EOL
  </head>
  <body>
    <table width="100%" cellspacing="0" cellpadding="2">
      <tr>
        <td class="menuTD"><a href="../overview/index.html">Overview</a></td>
        <td class="menuTD"><a href="../embeddingSTAR/index.html">STAR embedding</a></td>

        <td class="menuTD"><a href="../userRNC/index.html">RNC users</a></td>
        <td class="menuTD"><a href="../userALICE/index.html">ALICE users</a></td> 

        <td class="menuTD"><a href="../picoDstSTAR/index.html">STAR picoDSTs</a></td>
        <td class="menuTD"><a href="../pwgSTAR/index.html">STAR PWGs</a></td>
     </tr>
      <tr>
        <td class="menuTD"><a href="../overview/indexExt.html">Overview Input</a></td>
        <td class="menuTD"><a href="../embeddingSTAR/indexExt.html">STAR embedding extended</a></td>

        <td class="menuTD"><a href="../userRNC/indexExt.html">RNC users extendend</a></td>
        <td class="menuTD"><a href="../userALICE/indexExt.html">ALICE users extended</a></td> 

        <td class="menuTD"><a href="../picoDstSTAR/indexExt.html">STAR picoDSTs extended</a></td>
        <td class="menuTD"><a href="../pwgSTAR/indexExt.html">STAR PWGs extended </a></td>
     </tr>
      <tr>
        <td class="menuTD">&nbsp;</td>
        <td class="menuTD"><a href="../embeddingSTAR/indexExt2.html">STAR embedding extended2</a></td>

        <td class="menuTD">&nbsp;</td>
        <td class="menuTD">&nbsp;</td>

        <td class="menuTD">&nbsp;</td>
        <td class="menuTD">&nbsp;</td>
     </tr>
    </table>
    <br/>
EOL
}

# -----------------------------------------------------------------------------------------------------------------
function addScript () {
    dataFile=$1
    fileName=$2

    echo '     <script src="../data/outfile_'${dataFile}.js'"></script>' >> ${fileName}
}



# -----------------------------------------------------------------------------------------------------------------
function printBar() {
    fileName=$1
    total=$2
    totalTB=$3
    used=$4
    usedTB=$5
    title="$6"

    percent=$(echo "scale=4; ${used}/${total}*100" | bc -l)

    let percentINT=100*used/total
    if [ $percentINT -ge 90 ] ; then
	barColor=Red
    elif [ $percentINT -ge 80 ] ; then
	barColor=Orange
    else
	barColor=""
    fi

    echo '      <h4>'${title}'</h4>' >> ${fileName}
    echo '        <div class="bar"><div class="value'${barColor}'" style="width:'${percent%00}'%;">&nbsp;&nbsp;'${usedTB}'/'${totalTB}' TB ('${percent%00}'%)</div></div>' >> ${fileName}
}

# -----------------------------------------------------------------------------------------------------------------
function printBarElizaDisk() {
    fileName=$1
    disk=$2
    title="$3"
    
    total=`df | grep ${disk} | awk -F' ' '{ print $2 }'`
    totalTB=$(echo "scale=3; ${total}/1024/1024/1024" | bc -l)
    used=`df | grep ${disk} | awk -F' ' '{ print $3 }'`
    usedTB=$(echo "scale=3; ${used}/1024/1024/1024" | bc -l)

    printBar $fileName $total $totalTB $used $usedTB "$title"
}

# -----------------------------------------------------------------------------------------------------------------
function printBarElizaQuota() {
    fileName=$1
    disk=$2
    pwg=$3
    title="$4"
    
    total=`/usr/common/nsg/bin/getfsquota ${disk} ${pwg} | grep ${pwg} | awk -F' ' '{ print $3 }'`
    totalTB=$(echo "scale=3; ${total}/1024" | bc -l)
    used=`/usr/common/nsg/bin/getfsquota ${disk} ${pwg} | grep ${pwg} | awk -F' ' '{ print $2 }'`
    usedTB=$(echo "scale=3; ${used}/1024" | bc -l)

    printBar $fileName $total $totalTB $used $usedTB "$title"
}



# -----------------------------------------------------------------------------------------------------------------
function printBarProjectQuota() {
    fileName=$1
    disk=$2
    title="$3"

    total=`/usr/common/usg/bin/prjquota $disk | tail -n 1 | awk -F' ' '{ print $3 }'`
    totalTB=$(echo "scale=3; ${total}/1024" | bc -l)
    used=`/usr/common/usg/bin/prjquota $disk | tail -n 1 | awk -F' ' '{ print $2 }'`
    usedTB=$(echo "scale=3; ${used}/1024" | bc -l)

    printBar $fileName $total $totalTB $used $usedTB "$title"
}


# -----------------------------------------------------------------------------------------------------------------
function printLine() {
    fileName=$1

    echo '<div style="width:85%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>' >> ${fileName}
}


# -----------------------------------------------------------------------------------------------------------------
function printEnd() {
    fileName=$1
    
    echo '    <h3>Last Updated (ELIZAs 6/14):  '${modDateELIZA1}'</h3>' >> ${fileName}
    echo '    <h3>Last Updated (ELIZAs 15/17): '${modDateELIZA2}'</h3>' >> ${fileName}
    echo '    <h3>Last Updated (PROJECT):      '${modDatePROJECT}'</h3>' >> ${fileName}
    echo '  </body>' >> ${fileName}
    echo '</html>' >> ${fileName}
}

# -----------------------------------------------------------------------------------------------------------------
function printTable() {
    typeTable=$1
    version=$2
    fileName=$3

    tableName=output/outfile_Table_${typeTable}${version}

    # -- HEAD -------------------------------------------------------
    echo '<table class="sortable" cellspacing="0" cellpadding="2"><thead>' >> ${fileName}

	
    if [[ "${typeTable}" = "alice_user" || "${typeTable}" = "rnc_user"  ]] ; then
	echo '<tr><th class="user">User</th>' >> ${fileName}
	
	if [ "${version}" = "v3" ] ; then
	    echo '<th class="storage">Storage</th>' >> ${fileName}
	fi
    elif [ "${typeTable}" = "embedding"  ] ; then
	echo '<tr><th class="user">TrgSetupName</th>' >> ${fileName}
	if [ "${version}" = "v2" ] ; then
	    echo '<th class="storage">Storage</th>' >> ${fileName}
	elif [ "${version}" = "v3" ] ; then
	    tableName=output/outfile_Table_Ext_${typeTable}v1
	    echo '<th class="user">Production</th>' >> ${fileName}
	    echo '<th class="user">File Set</th>' >> ${fileName}
	fi
    elif [ "${typeTable}" = "picodsts"  ] ; then
	echo '<tr><th class="user">PicoDsts</th>' >> ${fileName}
	if [ "${version}" = "v3" ] ; then
	    echo '<th class="storage">Storage</th>' >> ${fileName}
	fi
    elif [ "${typeTable}" = "pwgstar"  ] ; then
	echo '<tr><th class="user">PWG</th>' >> ${fileName}
	if [ "${version}" = "v3" ] ; then
	    echo '<th class="storage">Storage</th>' >> ${fileName}
	fi
    fi

    echo '<th class="size">Size (GB)</th><th class="nFiles">N Files</th><th class="time">Created</th><th class="time">Last Mod.</th><th class="time">Last Access</th></tr>' >> ${fileName}
	
    echo '</thead>' >> ${fileName}

      # -- BODY -------------------------------------------------------
    echo '<tbody>' >> ${fileName}
    cat ${tableName}.txt >> ${fileName}
    echo '</tbody>' >> ${fileName}

    # -- FOOT -------------------------------------------------------
    echo '<tfoot>' >> ${fileName}
    cat ${tableName}_Sum.txt >> ${fileName}
    echo '</tfoot></table>' >> ${fileName}
}

# -----------------------------------------------------------------------------------------------------------------
function printUserRNC() {
    typeTable=rnc_user
    version=v1
    fileName=www/${1}/index.html

    printBegin ${typeTable} ${fileName}

    echo '    <table width="100%" cellspacing="0" cellpadding="10"><tr>' >> ${fileName}
    echo '    <td class="halfColumn" style="width: 60%">' >> ${fileName}
    echo '      <h3><span style="font-style:italic">RNC users</span> summary table at NERSC</h3>' >> ${fileName}

    printTable ${typeTable} ${version} ${fileName} 

    echo '    </td><td class="halfColumn" style="width: 40%">' >> ${fileName}
    echo '      <h3><span style="font-style:italic">RNC users</span> at NERSC</h3>' >> ${fileName}

    echo '      <h4>user (merged storage)</h4>' >> ${fileName}
    echo '      <div id="rncUserv1"></div>' >> ${fileName}
    echo '      <h4>user &gt; storage</h4>' >> ${fileName}
    echo '      <div id="rncUserv2"></div>' >> ${fileName}        
    echo '      <h4>storage &gt; user</h4>' >> ${fileName}
    echo '      <div id="rncUserv3"></div>' >> ${fileName}        
    echo '    </td></tr></table>' >> ${fileName}

    printEnd ${fileName}

    # -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
    
    version=v3
    fileName=www/${1}/indexExt.html

    printBegin ${typeTable} ${fileName} NONE

    echo '    <table width="100%" cellspacing="0" cellpadding="10"><tr>' >> ${fileName}
    echo '    <td class="halfColumn" style="width: 90%">' >> ${fileName}
    echo '      <h3><span style="font-style:italic">RNC users</span> summary table at NERSC</h3>' >> ${fileName}
    
    printTable ${typeTable} ${version} ${fileName}
    
    echo '    </td><td class="halfColumn" style="width:10%">&nbsp;</td></tr></table>' >> ${fileName}

    printEnd ${fileName}
}

# -----------------------------------------------------------------------------------------------------------------
function printUserALICE() {
    typeTable=alice_user
    version=v1
    fileName=www/${1}/index.html

    printBegin ${typeTable} ${fileName}

    echo '    <table width="100%" cellspacing="0" cellpadding="10"><tr>' >> ${fileName}
    echo '    <td class="halfColumn" style="width: 60%">' >> ${fileName}
    echo '      <h3><span style="font-style:italic">ALICE users</span> summary table at NERSC</h3>' >> ${fileName}

    printTable ${typeTable} ${version} ${fileName}

    echo '    </td><td class="halfColumn" style="width: 40%"> ' >> ${fileName}
    echo '      <h3><span style="font-style:italic">ALICE users</span> at NERSC</h3> ' >> ${fileName}
    echo '      <h4>user (merged storage)</h4>' >> ${fileName}
    echo '      <div id="aliceUserv1"></div>' >> ${fileName}
    echo '      <h4>user &gt; storage</h4>' >> ${fileName}
    echo '      <div id="aliceUserv2"></div>' >> ${fileName}
    echo '      <h4>storage &gt; user</h4>' >> ${fileName}
    echo '      <div id="aliceUserv3"></div>' >> ${fileName}
    echo '    </td></tr></table>' >> ${fileName}

    printEnd ${fileName}

    # -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

    version=v3
    fileName=www/${1}/indexExt.html

    printBegin ${typeTable} ${fileName} NONE

    echo '    <table width="100%" cellspacing="0" cellpadding="10"><tr>' >> ${fileName}
    echo '    <td class="halfColumn" style="width: 90%">' >> ${fileName}
    echo '      <h3><span style="font-style:italic">ALICE users</span> summary table at NERSC</h3>' >> ${fileName}
    
    printTable ${typeTable} ${version} ${fileName}
    
    echo '    </td><td class="halfColumn" style="width:10%">&nbsp;</td></tr></table>' >> ${fileName}
    
    printEnd ${fileName}
}

# -----------------------------------------------------------------------------------------------------------------
function printUserEmbedding() {
    typeTable=embedding
    version=v1
    fileName=www/${1}/index.html

    printBegin ${typeTable} ${fileName} 

    echo '    <table width="100%" cellspacing="0" cellpadding="10"><tr>' >> ${fileName}
    echo '    <td class="halfColumn" style="width: 60%">' >> ${fileName}
    echo '      <h3><span style="font-style:italic">STAR embedding</span> summary table at NERSC</h3>' >> ${fileName}

    printTable ${typeTable} ${version} ${fileName}

    echo '    </td><td class="halfColumn" style="width: 40%">' >> ${fileName}
    echo '      <h3><span style="font-style:italic">STAR embedding productions</span> at NERSC</h3>' >> ${fileName}
    
    echo '      <h4>trgSetupName &gt; storage &gt; fileSet &gt; production</h4>' >> ${fileName}
    echo '      <div id="embeddingv4"></div>' >> ${fileName}        

    echo '    </td></tr></table>' >> ${fileName}

    printEnd ${fileName}
    
    # -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
}

# -----------------------------------------------------------------------------------------------------------------
function printEmbedding() {
    typeTable=embedding
    version=v1
    fileName=www/${1}/index.html

    printBegin ${typeTable} ${fileName} 

    echo '    <table width="100%" cellspacing="0" cellpadding="10"><tr>' >> ${fileName}
    echo '    <td class="halfColumn" style="width: 60%">' >> ${fileName}
    echo '      <h3><span style="font-style:italic">STAR embedding</span> summary table at NERSC</h3>' >> ${fileName}

    printTable ${typeTable} ${version} ${fileName}

    echo '    </td><td class="halfColumn" style="width: 40%">' >> ${fileName}
    echo '      <h3><span style="font-style:italic">STAR Embedding</span> at NERSC</h3>' >> ${fileName}
    
    echo '      <h4>trgSetupName &gt; merged fileSet &gt; production</h4>' >> ${fileName}
    echo '      <div id="embeddingv1"></div>' >> ${fileName}
    
    echo '      <h4>storage &gt; trgSetupName &gt; merged fileSet &gt; production</h4>' >> ${fileName}
    echo '      <div id="embeddingv2"></div>' >> ${fileName}        
    
    echo '      <div style="width:85%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>' >> ${fileName}
    
    echo '      <h4>trgSetupName &gt; fileSet &gt; production (merged storage)</h4>' >> ${fileName}
    echo '      <div id="embeddingv3"></div>' >> ${fileName}        
    
    echo '      <div style="width:85%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>' >> ${fileName}
    
    echo '      <h4>trgSetupName &gt; storage &gt; fileSet &gt; production</h4>' >> ${fileName}
    echo '      <div id="embeddingv4"></div>' >> ${fileName}        

    echo '      <h4>storage &gt; trgSetupName &gt; fileSet &gt; production</h4>' >> ${fileName}
    echo '      <div id="embeddingv5"></div>' >> ${fileName}        

    echo '    </td></tr></table>' >> ${fileName}

    printEnd ${fileName}
    
    # -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

    version=v2
    fileName=www/${1}/indexExt.html

    printBegin ${typeTable} ${fileName} NONE

    echo '    <table width="100%" cellspacing="0" cellpadding="10"><tr>' >> ${fileName}
    echo '    <td class="halfColumn" style="width: 90%">' >> ${fileName}
    echo '      <h3><span style="font-style:italic">STAR embedding</span> summary table at NERSC</h3>' >> ${fileName}
    
    printTable ${typeTable} ${version} ${fileName}
    
    echo '    </td><td class="halfColumn" style="width:10%">&nbsp;</td></tr></table>' >> ${fileName}
    
    printEnd ${fileName}

    # -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

    version=v3
    fileName=www/${1}/indexExt2.html

    printBegin ${typeTable} ${fileName} NONE

    echo '    <table width="100%" cellspacing="0" cellpadding="10"><tr>' >> ${fileName}
    echo '    <td class="halfColumn" style="width: 90%">' >> ${fileName}
    echo '      <h3><span style="font-style:italic">STAR embedding</span> summary table at NERSC</h3>' >> ${fileName}

    printTable ${typeTable} ${version} ${fileName}
    
    echo '    </td><td class="halfColumn" style="width:10%">&nbsp;</td></tr></table>' >> ${fileName}
    
    printEnd ${fileName}
}

# -----------------------------------------------------------------------------------------------------------------
function printPicoDSTs() {
    typeTable=picodsts
    version=v1
    fileName=www/${1}/index.html

    printBegin ${typeTable} ${fileName} 

    echo '    <table width="100%" cellspacing="0" cellpadding="10"><tr>' >> ${fileName}
    echo '    <td class="halfColumn" style="width: 60%">' >> ${fileName}
    echo '      <h3><span style="font-style:italic">STAR picoDSTs</span> summary table at NERSC</h3>' >> ${fileName}

    printTable ${typeTable} ${version} ${fileName}

    echo '    </td><td class="halfColumn" style="width: 40%">' >> ${fileName}
    echo '      <h3><span style="font-style:italic">STAR picoDsts</span> at NERSC</h3>' >> ${fileName}
    echo '      <h4>picoDsts (merged storage)</h4>' >> ${fileName}
    echo '      <div id="picoDstsv1"></div>' >> ${fileName}
    echo '      <h4>picoDsts &gt; storage</h4>' >> ${fileName}
    echo '      <div id="picoDstsv2"></div>' >> ${fileName}
    echo '      <h4>storage &gt; picoDsts</h4>' >> ${fileName}
    echo '      <div id="picoDstsv3"></div>' >> ${fileName}
    echo '    </td></tr></table>' >> ${fileName}

    printEnd ${fileName}

    # -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

    version=v3
    fileName=www/${1}/indexExt.html
    
    printBegin ${typeTable} ${fileName} NONE

    echo '    <table width="100%" cellspacing="0" cellpadding="10"><tr>' >> ${fileName}
    echo '    <td class="halfColumn" style="width: 90%">' >> ${fileName}
    echo '      <h3><span style="font-style:italic">STAR PicoDsts</span> summary table at NERSC</h3>' >> ${fileName}
    
    printTable ${typeTable} ${version} ${fileName}
    
    echo '    </td><td class="halfColumn" style="width:10%">&nbsp;</td></tr></table>' >> ${fileName}
    
    printEnd ${fileName}
}

# -----------------------------------------------------------------------------------------------------------------
function printPwgSTAR() {
    typeTable=pwgstar
    version=v1
    fileName=www/${1}/index.html

    printBegin ${typeTable} ${fileName} 

    echo '    <table width="100%" cellspacing="0" cellpadding="10"><tr>' >> ${fileName}
    echo '    <td class="halfColumn" style="width: 60%">' >> ${fileName}
    echo '      <h3><span style="font-style:italic">STAR PWG directories</span> summary table at NERSC</h3>' >> ${fileName}

    printTable ${typeTable} ${version} ${fileName}

    echo '    </td><td class="halfColumn" style="width: 40%">' >> ${fileName}
    echo '      <h3>FillStatus (Quota): <span style="font-style:italic">ELIZAs</span> ('$now')</h3>' >> ${fileName}
    printBarElizaQuota ${fileName} eliza14 pwg "PWG (eliza14)"
    printBarElizaQuota ${fileName} eliza17 pwg "PWG (eliza17)"

    printLine ${fileName}    

    echo '      <h3><span style="font-style:italic">STAR PWG directories</span> at NERSC</h3>' >> ${fileName}
    echo '      <h4>PWGs (merged storage)</h4>' >> ${fileName}
    echo '      <div id="pwgSTARv1"></div>' >> ${fileName}
    echo '      <h4>PWGs &gt; storage</h4>' >> ${fileName}
    echo '      <div id="pwgSTARv2"></div>' >> ${fileName}
    echo '      <h4>storage &gt; PWGs</h4>' >> ${fileName}
    echo '      <div id="pwgSTARv3"></div>' >> ${fileName}
    echo '    </td></tr></table>' >> ${fileName}

    printEnd ${fileName}
    
    # -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

    version=v3
    fileName=www/${1}/indexExt.html
    
    printBegin ${typeTable} ${fileName} NONE

    echo '    <table width="100%" cellspacing="0" cellpadding="10"><tr>' >> ${fileName}
    echo '    <td class="halfColumn" style="width: 90%">' >> ${fileName}
    echo '      <h3><span style="font-style:italic">STAR PWG directories</span> summary table at NERSC</h3>' >> ${fileName}
    
    printTable ${typeTable} ${version} ${fileName}
    
    echo '    </td><td class="halfColumn" style="width:10%">&nbsp;</td></tr></table>' >> ${fileName}
    
    printEnd ${fileName}
}

# -----------------------------------------------------------------------------------------------------------------
function printOverview() {
    typeTable=overview
    fileName=www/${1}/index.html

    printBegin ${typeTable} ${fileName} 

cat >>${fileName} <<EOL
    <table width="100%" cellspacing="0" cellpadding="2"><tr>
    <td class="column">        
     <h3><span style="font-style:italic">STAR Embedding</span> at NERSC</h3> 

      <h4>trgSetupName &gt; merged fileSet &gt; production</h4>
      <div id="embeddingv1"></div>        

      <h4>storage &gt; trgSetupName &gt; merged fileSet &gt; production</h4>
      <div id="embeddingv2"></div>        

      <div style="width:85%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>

      <h4>trgSetupName &gt; fileSet &gt; production (merged storage)</h4>
      <div id="embeddingv3"></div>        

      <div style="width:85%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>

      <h4>trgSetupName &gt; storage &gt; fileSet &gt; production</h4>
      <div id="embeddingv4"></div>        

      <h4>storage &gt; trgSetupName &gt; fileSet &gt; production</h4>
      <div id="embeddingv5"></div>        
 
    </td><td class="column">

      <h3><span style="font-style:italic">RNC users</span> at NERSC</h3> 

      <h4>user (merged storage)</h4>
      <div id="rncUserv1"></div>
      <h4>user &gt; storage</h4>        
      <div id="rncUserv2"></div>        
      <h4>storage &gt; user</h4>
      <div id="rncUserv3"></div>        

      <div style="width:85%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>
      <!-- ---------------------------------------------------------------------- -->
 
      <h3><span style="font-style:italic">ALICE users</span> at NERSC</h3> 

      <h4>user (merged storage)</h4>
      <div id="aliceUserv1"></div> 
      <h4>user &gt; storage</h4>        
      <div id="aliceUserv2"></div> 
      <h4>storage &gt; user</h4>
      <div id="aliceUserv3"></div> 

    </td><td class="column">        
  
      <h3><span style="font-style:italic">STAR picoDsts</span> at NERSC</h3> 

      <h4>picoDsts (merged storage)</h4>
      <div id="picoDstsv1"></div> 
      <h4>picoDsts &gt; storage</h4>  
      <div id="picoDstsv2"></div>       
      <h4>storage &gt; picoDsts</h4>
      <div id="picoDstsv3"></div> 

      <div style="width:85%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>
      <!-- ---------------------------------------------------------------------- -->
  
      <h3><span style="font-style:italic">STAR PWG directories</span> tree at NERSC</h3> 
      <h4>PWGs (merged storage)</h4>
      <div id="pwgSTARv1"></div> 
      <h4>PWGs &gt; storage</h4>  
      <div id="pwgSTARv2"></div>       
      <h4>storage &gt; PWGs</h4>
      <div id="pwgSTARv3"></div> 

    </td></tr></table>
EOL
    printEnd ${fileName}

    # -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

    typeTable=overview_input
    fileName=www/${1}/indexExt.html

    printBegin ${typeTable} ${fileName} 

cat >>${fileName} <<EOL
    <table width="100%" cellspacing="0" cellpadding="2"><tr>
    <td class="column">        
      <h3>Input tree: <span style="font-style:italic">RNC</span> of <span style="font-style:italic">ELIZA's</span></h3> 
      <div id="rnc"></div>        
      <h3>Input tree: <span style="font-style:italic">STAR</span> of <span style="font-style:italic">ELIZA's</span></h3> 
      <div id="star"></div>        
      <h3>Input tree: <span style="font-style:italic">ALICE</span> of <span style="font-style:italic">ELIZA's</span></h3> 
      <div id="alice"></div>

      <div style="width:85%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>

      <h3>Input trees: <span style="font-style:italic">STAR</span> of <span style="font-style:italic">PROJECT</span></h3> 
      <div id="project_star"></div>        
      <h3>Input trees: <span style="font-style:italic">STARPROD</span> of <span style="font-style:italic">PROJECT</span></h3> 
      <div id="project_starprod"></div> 
      <h3>Input trees: <span style="font-style:italic">ALICE</span> of <span style="font-style:italic">PROJECT</span></h3> 
      <div id="project_alice"></div>        


    </td><td class="column">
      <h3>Storage: <span style="font-style:italic">ELIZA's</span></h3> 
      <div id="eliza6"></div>        
      <div id="eliza14"></div>        
      <div id="eliza15"></div>        
      <div id="eliza17"></div> 

      <div style="width:85%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>
      <h3>Storage: <span style="font-style:italic">PROJECT</span></h3> 
      <div id="project"></div> 

    </td><td class="column">        
EOL
   
echo '   <h3>FillStatus (Quota): <span style="font-style:italic">PROJECT</span> ('$now')</h3>' >> ${fileName}
printBarProjectQuota ${fileName} star star
printBarProjectQuota ${fileName} starprod starprod
printBarProjectQuota ${fileName} alice alice

printLine ${fileName}

echo '   <h3>FillStatus (Disk): <span style="font-style:italic">ELIZAs</span> ('$now')</h3>' >> ${fileName}
printBarElizaDisk ${fileName} eliza6 eliza6
printBarElizaDisk ${fileName} eliza14 eliza14
printBarElizaDisk ${fileName} eliza15 eliza15
printBarElizaDisk ${fileName} eliza17 eliza17

printLine ${fileName}

echo '   <h3>FillStatus (Quota): <span style="font-style:italic">ELIZAs</span> ('$now')</h3>' >> ${fileName}
printBarElizaQuota ${fileName} eliza14 pwg "PWG (eliza14)"
printBarElizaQuota ${fileName} eliza17 pwg "PWG (eliza17)"

#printBarElizaQuota ${fileName} eliza6  star "STAR (eliza6)"
#printBarElizaQuota ${fileName} eliza17 star "STAR (eliza17)"

cat >>${fileName} <<EOL
    </td></tr></table>
EOL
   printEnd ${fileName}

}

# -------------------------------------------------------------------------------
# -------------------------------------------------------------------------------
# -------------------------------------------------------------------------------

folderList="overview embedding embeddingSTAR userRNC userALICE picoDstSTAR pwgSTAR"

pushd www > /dev/null
for ii in ${folderList} ; do 
    mkdir -p ${ii}
    chmod 755 ${ii}
done
popd > /dev/null


printOverview overview

printEmbedding embeddingSTAR

printUserEmbedding embedding

printUserRNC userRNC
printUserALICE userALICE

printPwgSTAR pwgSTAR
printPicoDSTs picoDstSTAR

pushd www > /dev/null
for ii in ${folderList} ; do 
    chmod 644 ${ii}/*.html
done

cp overview/index.html .
sed -i  's/\..\///' index.html

popd > /dev/null

popd > /dev/null

