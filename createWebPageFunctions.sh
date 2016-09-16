#!/bin/bash
#
# Author: Jochen Thaeder (LBNL) <jochen@thaeder.de>
# Functions produce webpage of data usage
#
###################################################

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

    if [ "${typeTable}" = "embedding"  ] ; then
	dataFiles="embeddingv1 embeddingv2"
    elif [ "${typeTable}" = "alice_user" ] ; then
	dataFiles="alice_user"
    elif [ "${typeTable}" = "rnc_user" ] ; then
	dataFiles="rnc_user"
    elif [ "${typeTable}" = "picodsts"  ] ; then
	dataFiles="picodstsv1 picodstsv2 picodstsv3"
    elif [ "${typeTable}" = "pwgstar"  ] ; then
	dataFiles="pwgstar"
    elif [ "${typeTable}" = "overview"  ] ; then
	dataFiles="alice_user rnc_user pwgstar"
	dataFiles="${dataFiles} embeddingv1 embeddingv2"
	dataFiles="${dataFiles} picodstsv1 picodstsv2 picodstsv3"
    elif [ "${typeTable}" = "overview_input"  ] ; then
	dataFiles="project_alice project_star project_starprod projecta_starprod"
	dataFiles="${dataFiles} project projecta"
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
        <td class="menuTD"><a href="../embedding/index.html">STAR embedding</a></td>

        <td class="menuTD"><a href="../userRNC/index.html">RNC users</a></td>
        <td class="menuTD"><a href="../pwgSTAR/index.html">STAR PWGs</a></td>

        <td class="menuTD"><a href="../picoDstSTAR/index.html">STAR picoDSTs</a></td>

     </tr>
      <tr>
        <td class="menuTD"><a href="../overview/indexExt.html">Overview Input</a></td>
        <td class="menuTD"><a href="../embedding/indexExt.html">STAR embedding extended</a></td>

        <td class="menuTD"><a href="../userALICE/index.html">ALICE users</a></td> 

        <td class="menuTD">&nbsp;</td>
        <td class="menuTD"><a href="../picoDstSTAR/indexExt.html">STAR picoDSTs extended</a></td>

     </tr>
    </table>
    <br/>
    <div style="width:100%;height: 5px;  border-bottom:solid gray 2px;">&nbsp;</div>
EOL
}

# -----------------------------------------------------------------------------------------------------------------
function addScript () {
    dataFile=$1
    fileName=$2

    echo '     <script src="../data/outfile_'${dataFile}.js'"></script>' >> ${fileName}
}

# -----------------------------------------------------------------------------------------------------------------
function printBarSize() {
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

    echo '      <h4>'${title} - size'</h4>' >> ${fileName}
    echo '        <div class="bar"><div class="value'${barColor}'" style="width:'${percent%00}'%;">&nbsp;&nbsp;'${usedTB}'/'${totalTB}' TB ('${percent%00}'%)</div></div>' >> ${fileName}
}

# -----------------------------------------------------------------------------------------------------------------
function printBarInode() {
    fileName=$1
    total=$2
    used=$3
    title="$4"

    percent=$(echo "scale=4; ${used}/${total}*100" | bc -l)

    let percentINT=100*used/total
    if [ $percentINT -ge 90 ] ; then
	barColor=Red
    elif [ $percentINT -ge 80 ] ; then
	barColor=Orange
    else
	barColor=""
    fi
    echo '      <h4>'${title}' - inodes</h4>' >> ${fileName}
    echo '        <div class="bar"><div class="value'${barColor}'" style="width:'${percent%00}'%;">&nbsp;&nbsp;'${used}'/'${total}' ('${percent%00}'%)</div></div>' >> ${fileName}
}



# -----------------------------------------------------------------------------------------------------------------
function printBarProjectQuota() {
    fileName=$1
    disk=$2
    title="$2"
    
    if [[ $# -eq 3 && "$3" == "a" ]] ; then
	script="/usr/common/usg/bin/prjaquota"
    else
	script="/usr/common/usg/bin/prjquota"
    fi

    total=`$script $disk | tail -n 1 | awk -F' ' '{ print $3 }'`
    totalTB=$(echo "scale=3; ${total}/1024" | bc -l)
    used=`$script $disk | tail -n 1 | awk -F' ' '{ print $2 }'`
    usedTB=$(echo "scale=3; ${used}/1024" | bc -l)

    printBarSize $fileName $total $totalTB $used $usedTB "$disk"

    total=`$script $disk | tail -n 1 | awk -F' ' '{ print $6 }'`
    totalTB=$(echo "scale=3; ${total}/1024" | bc -l)
    used=`$script $disk | tail -n 1 | awk -F' ' '{ print $5 }'`
    usedTB=$(echo "scale=3; ${used}/1024" | bc -l)

    printBarInode $fileName $total $used "$disk"

}


# -----------------------------------------------------------------------------------------------------------------
function printLine() {
    fileName=$1

    echo '<div style="width:85%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>' >> ${fileName}
}


# -----------------------------------------------------------------------------------------------------------------
function printEnd() {
    fileName=$1
    
    echo '    <div style="width:100%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>' >> ${fileName}
    echo '    <h3>Last Updated (PROJECT):      '${modDatePROJECT}'</h3>'  >> ${fileName}
    echo '    <h3>Last Updated (PROJECTA):     '${modDatePROJECTA}'</h3>' >> ${fileName}

    echo '  </body>' >> ${fileName}
    echo '</html>' >> ${fileName}
}

# -----------------------------------------------------------------------------------------------------------------
function printTable() {

    if [ $# -eq 2 ] ; then
	typeTable=$1
	version=""
	fileName=$2
    else 
	typeTable=$1
	version=$2
	fileName=$3
    fi

    tableName=output/outfile_Table_${typeTable}${version}

    # -- HEAD -------------------------------------------------------
    echo '<table class="sortable" cellspacing="0" cellpadding="2"><thead>' >> ${fileName}

	
    if [[ "${typeTable}" = "alice_user" || "${typeTable}" = "rnc_user"  ]] ; then
	echo '<tr><th class="user">User</th>' >> ${fileName}
    elif [ "${typeTable}" = "embedding"  ] ; then
	echo '<tr><th class="user">TrgSetupName</th>' >> ${fileName}
	if [ "${version}" = "v2" ] ; then
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
    fileName=www/${1}/index.html

    printBegin ${typeTable} ${fileName}

    echo '    <table width="100%" cellspacing="0" cellpadding="10"><tr>' >> ${fileName}
    echo '    <td class="halfColumn" style="width: 60%">' >> ${fileName}
    echo '      <h3><span style="font-style:italic">RNC users</span> summary table at NERSC</h3>' >> ${fileName}

    printTable ${typeTable} ${fileName} 

    echo '    </td><td class="halfColumn" style="width: 40%">' >> ${fileName}

    echo '      <h3><span style="font-style:italic">RNC users</span> at NERSC</h3>' >> ${fileName}
    echo '      <h4>user</h4>' >> ${fileName}
    echo '      <div id="rncUser"></div>' >> ${fileName}

    printLine ${fileName}    

    echo '    </td></tr></table>' >> ${fileName}

    printEnd ${fileName}
}

# -----------------------------------------------------------------------------------------------------------------
function printUserALICE() {
    typeTable=alice_user
    fileName=www/${1}/index.html

    printBegin ${typeTable} ${fileName}

    echo '    <table width="100%" cellspacing="0" cellpadding="10"><tr>' >> ${fileName}
    echo '    <td class="halfColumn" style="width: 60%">' >> ${fileName}
    echo '      <h3><span style="font-style:italic">ALICE users</span> summary table at NERSC</h3>' >> ${fileName}

    printTable ${typeTable} ${fileName}

    echo '    </td><td class="halfColumn" style="width: 40%"> ' >> ${fileName}

    echo '      <h3><span style="font-style:italic">ALICE users</span> at NERSC</h3> ' >> ${fileName}
    echo '      <h4>user</h4>' >> ${fileName}
    echo '      <div id="aliceUser"></div>' >> ${fileName}

    printLine ${fileName}    

    echo '    </td></tr></table>' >> ${fileName}

    printEnd ${fileName}
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
    
    echo '      <div style="width:85%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>' >> ${fileName}
    
    echo '      <h4>trgSetupName &gt; fileSet &gt; production</h4>' >> ${fileName}
    echo '      <div id="embeddingv2"></div>' >> ${fileName}        
    
    echo '      <div style="width:85%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>' >> ${fileName}
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
    fileName=www/${1}/index.html

    printBegin ${typeTable} ${fileName} 

    echo '    <table width="100%" cellspacing="0" cellpadding="10"><tr>' >> ${fileName}
    echo '    <td class="halfColumn" style="width: 60%">' >> ${fileName}
    echo '      <h3><span style="font-style:italic">STAR PWG directories</span> summary table at NERSC</h3>' >> ${fileName}

    printTable ${typeTable} ${fileName}

    echo '    </td><td class="halfColumn" style="width: 40%">' >> ${fileName}

    echo '      <h3><span style="font-style:italic">STAR PWG directories</span> at NERSC</h3>' >> ${fileName}
    echo '      <h4>PWGs</h4>' >> ${fileName}
    echo '      <div id="pwgSTAR"></div>' >> ${fileName}

    printLine ${fileName}    

    echo '    </td></tr></table>' >> ${fileName}

    printEnd ${fileName}
}

# -----------------------------------------------------------------------------------------------------------------
function printOverview() {
    typeTable=overview
    fileName=www/${1}/index.html

    printBegin ${typeTable} ${fileName} 

cat >>${fileName} <<EOL

   <table width="100%" cellspacing="0" cellpadding="2"><tr>
    <td class="column">  <!-- ------------------------------------------------------------------------ -->
     <h3><span style="font-style:italic">STAR Embedding</span> at NERSC</h3> 

      <h4>trgSetupName &gt; merged fileSet &gt; production</h4>
      <div id="embeddingv1"></div>        

      <h4>trgSetupName &gt; fileSet &gt; production</h4>
      <div id="embeddingv2"></div>        

      <div style="width:85%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>
 
    </td><td class="column">   <!-- ------------------------------------------------------------------------ -->

      <h3><span style="font-style:italic">RNC users</span> at NERSC</h3> 

      <h4>user</h4>
      <div id="rncUser"></div>

      <div style="width:85%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>     

      <h3><span style="font-style:italic">ALICE users</span> at NERSC</h3> 

      <h4>user</h4>
      <div id="aliceUser"></div> 

      <div style="width:85%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>

    </td><td class="column">   <!-- ------------------------------------------------------------------------ -->
  
      <h3><span style="font-style:italic">STAR picoDsts</span> at NERSC</h3> 

      <h4>picoDsts (merged storage)</h4>
      <div id="picoDstsv1"></div> 
      <h4>picoDsts &gt; storage</h4>  
      <div id="picoDstsv2"></div>       
      <h4>storage &gt; picoDsts</h4>
      <div id="picoDstsv3"></div> 

      <div style="width:85%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>
        
      <h3><span style="font-style:italic">STAR PWG directories</span> tree at NERSC</h3> 
      <h4>PWGs</h4>
      <div id="pwgSTAR"></div> 

      <div style="width:85%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>

    </td></tr></table> 
EOL
    printEnd ${fileName}

    # -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

    typeTable=overview_input
    fileName=www/${1}/indexExt.html

    printBegin ${typeTable} ${fileName} 

cat >>${fileName} <<EOL
  <table width="100%" cellspacing="0" cellpadding="2"><tr>
    <td class="column">   <!-- ------------------------------------------------------------------------ -->
      <h3>Input trees: <span style="font-style:italic">STAR</span> of <span style="font-style:italic">PROJECT</span></h3>  
         <div id="project_star"></div>     
      <h3>Input trees: <span style="font-style:italic">STARPROD</span> of <span style="font-style:italic">PROJECT</span></h3> 
      <div id="project_starprod"></div> 
      <h3>Input trees: <span style="font-style:italic">ALICE</span> of <span style="font-style:italic">PROJECT</span></h3> 
     <div id="project_alice"></div>      

      <div style="width:85%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>

      <h3>Input trees: <span style="font-style:italic">STARPROD</span> of <span style="font-style:italic">PROJECTA</span></h3> 
          <div id="projecta_starprod"></div> 

    </td><td class="column">   <!-- ------------------------------------------------------------------------ -->
      <h3>Storage: <span style="font-style:italic">PROJECT</span></h3> 
      <div id="project"></div> 

      <div style="width:85%;height: 25px;  border-bottom:solid gray 2px;">&nbsp;</div>
      <h3>Storage: <span style="font-style:italic">PROJECTA</span></h3> 
      <div id="projecta"></div> 

    </td><td class="column">   <!-- ------------------------------------------------------------------------ -->
EOL
   
echo '   <h3>FillStatus (Quota): <span style="font-style:italic">PROJECT</span> ('$now')</h3>' >> ${fileName}
printBarProjectQuota ${fileName} star 
printLine ${fileName}
printBarProjectQuota ${fileName} starprod 
printLine ${fileName}
printBarProjectQuota ${fileName} alice 

printLine ${fileName}

echo '   <h3>FillStatus (Quota): <span style="font-style:italic">PROJECTA</span> ('$now')</h3>' >> ${fileName}
printBarProjectQuota ${fileName} starprod a

cat >>${fileName} <<EOL
    </td></tr></table>
EOL
   printEnd ${fileName}

}
