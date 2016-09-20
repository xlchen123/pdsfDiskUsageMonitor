# PDSF Disk Usage Monitor

A a set of scripts to monitor the disk usage of the STAR and ALICE groups at PDSF.

It parses the dumps of the GPFS file system (created by NERSC) and creates a tables and trees on a webpage. Furthermore it also uses the `prjquota` command to get up-to-date information of free space and free inodes

### Monitored entities

| FileSystem | Folder | Comment |
| --- | :---:| --- |
| project | alice | ALICE data |
| project | star | STAR : mainly PWG area|
| project | starprod | STAR : RNC, PicoDst, HFT |
| projecta | starprod | STAR : embedding, commen data |

### Components
* README.md           - *This file*
* parseGPFSDump.C     - *ROOT based parsing script*
* parseGPFSDump.tcsh  - *call parsing script*
* runDiskUsage.sh     - *Build webpage*
* createWebPageFunctions.sh - *helper functions to build webpage*
* run.sh              - *The run script*

#### parseGPFSDump.C

Can be run in two modes: parsing the input and printing the output. The script can be run in compiled mode (`parseGPFSDump.C++`).

###### Mode 0 - parsing:
Script is called extra for every *FileSystem* and *Folder* combination. It reads the text based GPFS dump lines and creates a tree with each `node` being a folder and holding the information about itself and a list of children.

```C++
ULong64_t fOwnSize;     // Sum of size of files in folder
ULong64_t fChildSize;   // Sum of size of child files
Int_t     fNOwnFiles;   // Sum of number of files in folder
Int_t     fNChildFiles; // Sum of number of child files

Int_t     fUid;         // UID - not used
Int_t     fGid;         // GID - not used

Int_t     faTime;       // Last access time
Int_t     fcTime;       // Creation time
Int_t     fmTime;       // Modification time

TList*    fChildren;    // List of child nodes
```

The resulting tree is then written as
`treeOutput_<FileSystem>_<Folder>.root`

###### Mode 1 - printing:

Reading in the output files from mode 0 and printing
different *trees* in JSON format, as well as *HTML table <tr>* lines in text format.

Output is stored in `output` folder.

The depth of the *trees* can be changed via a parameter in the script, default is 6 levels.

```C++
// -- max depth to print nodes to
static const Int_t     gcMaxLevel = 6;
```

The output is colored orange and red based on watermarks

```C++
// -- water marks for the coloring                       
static const ULong64_t gcLowMark  = 1099511627776;    // 1 TB
static const ULong64_t gcHighMark = 3*1099511627776;  // 3 TB    
```
#### parseGPFSDump.tcsh

Call the `parseGPFSDump.C` script for all *FileSystem* and *Folder* combinations.

```BASH
${BASEPATH}/parseGPFSDump.tcsh ${BASEPATH} <Mode>
```
Where `${BASEPATH}` is the path the script resides in.

* **Mode 0:** parsing
* **Mode 1:** printing
* **Mode 2:** parsing and printing

#### runDiskUsage.sh

Script to build webpage at `/project/projectdirs/star/www/diskUsage` or at
`/project/projectdirs/star/www/<username>/diskUsage/`.

Uses functions from `createWebPageFunctions.sh` to create the webpages.

Recreates the actual quota of the **project** and **projecta** filesystems via the `prjquota` and `prjaquota` commands in terms of inodes and space.

#### run.sh
The ***run*** script called by cron or executed by hand.

Checks if a GPFS dump is available in

```BASH
/project/statistics/LIST/<project_folder>/<day>
```
* Where `<project_folder>` is `tlproject2` for the **project** filesystem and `tlprojecta` for the  **projecta** filesystem.
* <day> has the format YYYY-MM-DD

Availablity of a new dump file is indicted by a  `*.completed` file. There is one dump file for each **project** and **projecta** filesystem

If a new file is available,
* the script uses combination of `cat`, `grep`, and `sed` to make dedicated copies for each our *FileSystem* and *Folder* combinations. These intermediate files are stored in `${SCRATCH}/pdsfDiskUsageMonitor/` and used as an input to `parseGPFSDump.C`.

* Afterwards the `parseGPFSDump.tcsh` script is executed in mode 2 for parsing and printing.

At the end, `runDiskUsage.sh` is called to recreate the webpage.


### How to run it

`./run.sh` is executed by a cron job 4 times a day and

```BASH
11  */4  * * *   CHOS=sl64 chos /usr/bin/flock -n /dev/shm/blah /bin/bash -l $HOME/pdsfDiskUsageMonitor/run.sh
```
### Installation

###### Requirements
* The package needs to be installed at NERSC.
* A ROOT enviroments needs to be set up.

###### Workflow

1. Clone from git into `$HOME`
2. Change into `$HOME/pdsfDiskUsageMonitor`
3. execute `run.sh`

```BASH
cd $HOME
git clone https://github.com/jthaeder/pdsfDiskUsageMonitor.git  
cd pdsfDiskUsageMonitor
./run.sh
```

All missing libraries for the webpage are automatically downloaded.
