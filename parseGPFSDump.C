/*
 * Class and macro to parse GPFS meta data dump files in order to 
 * sum up folder sizes
 * Author: Jochen Thaeder <jochen@thaeder.de> (LBNL)
 */

#include "Riostream.h"
#include "Rtypes.h"
#include "TNamed.h"
#include "TString.h"
#include "TList.h"
#include "TFile.h"
#include "TTimeStamp.h"
#include "TObjArray.h"
#include "TObjString.h"

// -- water marks for the coloring 
static const ULong64_t gcLowMark  = 1099511627776;
static const ULong64_t gcHighMark = 3*1099511627776;

static const Int_t     gcUid[4]            = {000, 001, 002, 003};
static const Int_t     gcGid[4]            = {000, 001, 002, 003};

// -- input files
static const Char_t*   gcFolder[2]         = {"alice", "rnc"};
static const Char_t*   gcStorage[2]        = {"project", "projecta"}; 
static const Char_t*   gcStoragePrefix[2]  = {"prj", "prj"}; 

static const Char_t*   gcProjectFolder[3]  = {"alice", "star", "starprod"};

static const Int_t     gcMaxFiles[3]       = {2, 2, 4};

// -- max depth to print nodes po
static const Int_t     gcMaxLevel = 6;

// -- current storage folder
static TString         gStorage("");

// _______________________________________________________________
class node : public TNamed  {

public:
  // ___________________________________________________
  node() :
    fLevel(0),
    fMaxLevel(gcMaxLevel),
    fOwnSize(0),
    fChildSize(0),
    fNOwnFiles(0),
    fNChildFiles(0),
    fUid(-1),
    fGid(-1),
    faTime(-1.), 
    fcTime(2145916800),
    fmTime(-1.), 
    fStorage(gStorage),
    fChildren(NULL) {
    // -- default constructor
    
    SetNameTitle("root", "root");
    fChildren = new TList();
    fChildren->SetOwner(kTRUE);
  }
  
  // ___________________________________________________
  node(TString name, TString title, Int_t level) :
    fLevel(level),
    fMaxLevel(gcMaxLevel),
    fOwnSize(0),
    fChildSize(0),
    fNOwnFiles(0),
    fNChildFiles(0),
    fUid(-1),
    fGid(-1),
    faTime(-1),
    fcTime(2145916800),
    fmTime(-1),
    fStorage(gStorage),
    fChildren(NULL) {
    // -- constructor for adding file only

    SetNameTitle(name, title);
    fChildren = new TList();
    fChildren->SetOwner(kTRUE);
  }

  // ___________________________________________________
  virtual ~node() {
    // -- destructor
    
    if (fChildren) {
      fChildren->Clear();
      delete fChildren;
    }
  }
  
  // -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
  // -- Setter
  // -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 

  // ___________________________________________________
  void CopyProperties(node* orig) {
    // -- Copy properties of a node
    SetProperties(orig->GetOwnSize(), orig->GetChildSize(), 
		  orig->GetNOwnFiles(), orig->GetNChildFiles(), 
		  orig->GetaTime(), orig->GetcTime(), orig->GetmTime());
  }

  // ___________________________________________________
  void CopyPropertiesOwn(node* orig) {
    // -- Copy properties of a node
    SetProperties(orig->GetOwnSize(), 0, 
		  orig->GetNOwnFiles(), 0,
		  orig->GetaTime(), orig->GetcTime(), orig->GetmTime());
  }

  // ___________________________________________________
  void CopyPropertiesChild(node* orig) {
    // -- Copy properties of a node
    SetProperties(0, orig->GetSize(), 
		  0, orig->GetNFiles(), 
		  orig->GetaTime(), orig->GetcTime(), orig->GetmTime());
  }

  // ___________________________________________________
  void SetProperties(const ULong64_t sizeOwn, const ULong64_t sizeChild, const Int_t nFilesOwn, const Int_t nFilesChild, 
		     const Int_t aTime, const Int_t cTime, const Int_t mTime) { 
    // -- Set node properties

    // -- add size and nFiles
    fOwnSize += sizeOwn; 
    fChildSize += sizeChild; 

    fNOwnFiles += nFilesOwn;
    fNChildFiles += nFilesChild;

    // -- use last acess time in folder
    if (aTime > faTime) faTime = aTime;

    // -- use last modification time in folder
    if (mTime > fmTime) fmTime = mTime;

    // -- use first creation time in folder
    if (cTime < fcTime) fcTime = cTime;
  }
  
  // ___________________________________________________
  void SetMaxLevel(Int_t maxLevel) {
    // -- Set MaxLevel to all children (if needed)
    
    fMaxLevel = maxLevel;
    
    if (fLevel <= fMaxLevel) {
      TIter next(fChildren);
      node *child;
      
      while ((child = static_cast<node*>(next())))
	child->SetMaxLevel(maxLevel);
    }
  }

  // -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
  // -- Getter
  // -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 

  ULong64_t     GetOwnSize()     { return fOwnSize; }
  ULong64_t     GetChildSize()   { return fChildSize; }
  ULong64_t     GetSize()        { return fOwnSize+fChildSize; }

  Int_t         GetNOwnFiles()   { return fNOwnFiles; }
  Int_t         GetNChildFiles() { return fNChildFiles; }
  Int_t         GetNFiles()      { return fNOwnFiles+fNChildFiles; }

  TList*        GetChildren()    { return fChildren; }
  node*         GetChild(const Char_t* childName) { return static_cast<node*>(fChildren->FindObject(childName)); }

  Int_t         GetaTime()      { return faTime; }
  Int_t         GetcTime()      { return fcTime; }
  Int_t         GetmTime()      { return fmTime; }

  const Char_t* GetaDate()      { return GetDate(faTime); }
  const Char_t* GetcDate()      { return GetDate(fcTime); }
  const Char_t* GetmDate()      { return GetDate(fmTime); }


  // ___________________________________________________
  void ClearChild(const Char_t* childName) {  
    // -- clear specific child
    
    TString sChildName(childName);
    sChildName.ToLower();

    if (!fChildren)
      return;
    
    TIter next(fChildren);
    node *child;
    while ((child = static_cast<node*>(next()))) {
      if (!child)
	continue;
      if (!sChildName.CompareTo(child->GetName())) {
	child->ClearChilds();
	cout << "CLEARED  childs  " << child->GetName() << endl;
	if (child) {
	  delete child;
	  child = NULL;
	}
	cout << " done " << endl;
      }
    }
  }

  // ___________________________________________________
  void ClearChilds() {  
    // -- clear children
    
    cout << "NAME   " << GetName() << endl;

    if (fChildren) {
      
      TIter next(fChildren);
      node *child;
      while ((child = static_cast<node*>(next())))
	child->ClearChilds();
      cout << "Delete   " << GetName() << endl;
      fChildren->Delete();
	cout << "Delete Done  " << GetName() << endl;
    }
  }

  // ___________________________________________________
  const Char_t* GetDate(UInt_t date) {
    // -- get a human and machine sortable date

    UInt_t year, month, day;

    TTimeStamp timeStamp(date);
    timeStamp.GetDate(0, 0, &year, &month, &day);

    return Form("%d-%02d-%02d", year, month, day);
  }

  // ___________________________________________________
  const Char_t* GetHumanReadableSize() {
    // -- get a human readable size
    
    const Char_t* sizeOrder[] = {"", "k", "M", "G", "T", "P", "E"};

    Int_t idx = 0;
    Double_t size = Double_t(GetSize());
      
    while (1) {
      Double_t tmpSize = size/1024.0;
      if (tmpSize < 1)
	return Form("%.2f%sB", size, sizeOrder[idx]);
      size = tmpSize;
      ++idx;
    }
  }

  // ___________________________________________________
  const Char_t* GetGBSize() {
    // -- get a human readable size in GByte
    return (GetSize() < 5*1024*1024) ? Form("&lt;0.001") : Form("%.2f",  Double_t(GetSize())/(1024.0*1024.0*1024.0));;
  }

  // ___________________________________________________
  const Char_t* GetAlarmLevel() {
    // -- print alarm level

    if (GetSize() <= gcLowMark)
      return "normal";
    else 
      return (GetSize() > gcHighMark) ? "alarm" : "warning";
  }
  
  // -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
  // -- Add methods
  // -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 

  // ___________________________________________________
  void AddFile(TString &title, ULong64_t size, Int_t aTime, Int_t cTime, Int_t mTime) { 
    // -- add file into tree

    // -- find position of '/'
    Short_t first = title.First("|||");

    // -- if leaf  ( no '/' inside)
    //      add own size + child && return
    //    else
    //      add child
    if (first < 0)  {
      SetProperties(size, 0, 1, 0, aTime, cTime, mTime);
      return;
    }
    else 
      SetProperties(0, size, 0, 1, aTime, cTime, mTime);

    // -- current path
    TString current(title(0, first));
    
    // -- remaining path - without current
    title.Remove(0,first+3);
    
    // -- get child node, if no exist -> create it
    node* child = AddNode(current);

    // -- add recursively the child till at the leaf
    child->AddFile(title, size, aTime, cTime, mTime);
  }

  // ___________________________________________________
  node* AddNode(TString &title) {
    // -- Add node to the tree, return if already exists
    
    TString name(title);
    name.ToLower();
    
    // -- check if child exists
    node* child = GetChild(name);
    if (!child) {
      fChildren->Add(new node(name, title, fLevel+1));
      child = static_cast<node*>(fChildren->Last());
    }

    return child;
  }
  
  // ___________________________________________________
  node* AddNode(const Char_t* titleT) {
    // -- Add node to the tree, return if already exists

    TString title(titleT);
    return AddNode(title);
  }
  
  // ___________________________________________________
  node* AddNodeCopy(node* orig, const Char_t* newTitle = "") {
    // -- add a copy of a node
    //    NOT of its children
    
    TString title(newTitle);
    if (title.Length() == 0)
      title += orig->GetTitle();
  
    // -- get new node or already existing
    node* copy = AddNode(title);
    
    // -- add the properties
    copy->CopyPropertiesOwn(orig);
    
    return copy;
  }

  // ___________________________________________________
  node* AddNodeFullCopy(node* orig, const Char_t* newTitle = "") {
    // -- add a copy of a node
    //    NOT of its children
    
    TString title(newTitle);
    if (title.Length() == 0)
      title += orig->GetTitle();
  
    // -- get new node or already existing
    node* copy = AddNode(title);
    
    // -- add the properties
    copy->CopyProperties(orig);
    
    // -- merge the children 
    copy->AddChildren(orig);

    return copy;
  }

  // ___________________________________________________
  void AddChildren(node* orig) {
    // -- Add children from list 

    // -- loop over foreign children list and 
    //    - create a full copy of foreign nodes
    //    - otherwise merge 
    TIter next(orig->GetChildren());
    node *childOrig;
    while ((childOrig = static_cast<node*>(next())))
      AddNodeFullCopy(childOrig);
      SumAddChildren();
  }
  
  // ___________________________________________________
  void SumAddChildren() {
    // -- Sum and add children to current node

    fChildSize   = 0;
    fNChildFiles = 0;

    TIter next(fChildren);
    node *child;
    while ((child = static_cast<node*>(next())))
      CopyPropertiesChild(child);
  }

  // ___________________________________________________
  void PrintNodes(ofstream &fout) { 
    // -- print node recursively
    PrintNodes(fout, fLevel+1);
  }

  // ___________________________________________________
  void PrintNodes(ofstream &fout, Int_t level) { 
    // -- print node recursively

    // -- Re-adjusting the level
    fLevel = level;
  
    TString padding("");
    for (Int_t idx = 0; idx < fLevel; ++idx)
      padding += "     ";

    fout << padding << " { label: '" << GetTitle() << " [<span class=\"nFiles\">" << GetNFiles() 
	 << " files</span>] <span class=\"size " << GetAlarmLevel() << "\">" << GetHumanReadableSize() 
	 << "</span>&nbsp;&nbsp;&nbsp;<span class=\"lastMod\"> {Last Mod. " << GetmDate() << "}</span>'," << endl;

    if (fLevel < fMaxLevel && fChildren->GetEntries() > 0) {
      fout << padding << "   children: [" << endl;

      fChildren->Sort();

      TIter next(fChildren);
      node *child;
      while ((child = static_cast<node*>(next())))
	child->PrintNodes(fout, fLevel+1);

      fout << padding << "   ]," << endl;
    }
    fout << padding << " }," << endl;
  }  

  // ___________________________________________________
  void PrintChildren(Int_t maxLevel = 0, Int_t currentLevel = 0) {
    // -- print names of children down to maxLevel
    
    TString padding("");
    for (Int_t idx = 0; idx < currentLevel; ++idx)
      padding += "     ";
    
    fChildren->Sort();
    
    TIter next(fChildren);
    node *child;
    while ((child = static_cast<node*>(next()))) {
      cout <<  padding << child->GetTitle() << endl;
      if (currentLevel != maxLevel)
	child->PrintChildren(maxLevel, currentLevel+1);
    }
  }
  
  // ___________________________________________________
  void PrintTableEntries(ofstream &fout, Int_t version, const Char_t *arg1 = "") { 
    // -- print childs as table entry
    //    use extended format if arg1 is none empty
    //    version : 1 user || embedding
    //              2 user + storage
    //              0 extended embedding

    if (fChildren->GetEntries() > 0) {
      fChildren->Sort();
      
      TIter next(fChildren);
      node *child;
      while ((child = static_cast<node*>(next()))) {
	fout << "<tr>";
	
	if (version == 1) {
	  fout << "<td class=\"user\">" << child->GetTitle() << "</td>";
	}
	else if (version == 2) {
	  fout << "<td class=\"user\">" << child->GetTitle() << "</td>";
	  fout << "<td class=\"user\">" << GetTitle() << "</td>";
	}
	else if (version == 0) {
	  fout << "<td class=\"user\">" << arg1 << "</td>"
	       << "<td class=\"user\">" << child->GetTitle() << "</td>";
	  fout << "<td class=\"user\">" << GetTitle() << "</td>";
	}
	  
	fout << "<td class=\"size " << child->GetAlarmLevel() << "\">" << child->GetGBSize() << "</td>" 
	     << "<td class=\"nFiles\">" << child->GetNFiles() << "</td>"
	     << "<td class=\"time\">" << child->GetcDate() << "</td>"
	     << "<td class=\"time\">" << child->GetmDate() << "</td>"
	     << "<td class=\"time\">" << child->GetaDate() << "</td></tr>" << endl;
      } // while ((child = static_cast<node*>(next()))) {
    } // if (fChildren->GetEntries() > 0) {
  }

  // ___________________________________________________
  void PrintTableSummary(ofstream &fout, Int_t extraCols = 0) { 
    // -- print childs as table entry
    
    fout << "<tr><td class=\"user footer\" style=\"font-weight: bold;\">TOTAL</td>";
    
    for (Int_t idx = 0; idx < extraCols; ++idx) 
      fout << "<td class=\"user footer\">&nbsp</td>";
    
    fout << "<td class=\"size footer " << GetAlarmLevel() << "\">" << GetGBSize() << "</td>" 
	 << "<td class=\"nFiles footer\">" << GetNFiles() << "</td>"
	 << "<td class=\"time footer\">" << GetcDate() << "</td>"
	 << "<td class=\"time footer\">" << GetmDate() << "</td>"
	 << "<td class=\"time footer\">" << GetaDate() << "</td></tr>" << endl;
  }  
  
private:
  Int_t     fLevel;
  Int_t     fMaxLevel;

  ULong64_t fOwnSize;
  ULong64_t fChildSize;
  Int_t     fNOwnFiles;
  Int_t     fNChildFiles;
  
  Int_t     fUid;
  Int_t     fGid;

  Int_t     faTime;
  Int_t     fcTime;
  Int_t     fmTime;

  TString   fStorage;

  TList*    fChildren;

  ClassDef(node,1)
};

// --------------------------------------------------------------------------------------
node* GetNodeProjectDirs(node* rootIn[][3], Int_t idxStorage, Int_t idxFolder);
node* GetNodeProject(    node* rootIn[][3], Int_t idxStorage, Int_t idxFolder);

node* GetNodeEmbedding(  node* rootIn[][3]);
node* GetNodePicoDsts(   node* rootIn[][3], Int_t idxStorage, Int_t idxFolder);
node* GetNodePwgSTAR(    node* rootIn[][3]);
node* GetNodeUserRNC(    node* rootIn[][3]);

void  processFilePROJECT(ifstream &fin, TString &inFileName, node *fileRootNode);

node* processFolder(node* root, Int_t idxStorage, Int_t idxFolder);

void  printFolder(node* folder);
void  printTable(node* rootIn, Int_t idxVersion = 1);

node* processStorage(  node* rootIn[][3], node* rootOut);
node* processUser(     node* rootIn[][3], node* rootOut, Int_t idxGroup);
node* processEmbedding(node* rootIn[][3], node* rootOut, Int_t version);
node* processPicoDsts( node* rootIn[][3], node* rootOut, Int_t version = 1);
node* processPwgSTAR(  node* rootIn[][3], node* rootOut);

void  parseGPFSDump(Int_t mode = 0, Int_t parseIdx = 0, Int_t folderIdx = 0);

// --------------------------------------------------------------------------------------

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
// -- General Getter
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 

// ________________________________________________________________________________
node* GetNodeProjectDirs(node* rootIn[][3], Int_t idxStorage, Int_t idxFolder) {
  // -- Get the toplevel project nodes

  return (idxStorage == 1 && idxFolder <2) ? NULL : rootIn[idxStorage][idxFolder]->GetChild(Form("raw_%s_%s", gcStorage[idxStorage], gcProjectFolder[idxFolder]));
} 

// ________________________________________________________________________________
node* GetNodeProject(node* rootIn[][3], Int_t idxStorage, Int_t idxFolder) {
  // -- Get the toplevel project nodes

  node *projectDirs = GetNodeProjectDirs(rootIn, idxStorage, idxFolder);
  return (projectDirs) ? projectDirs->GetChild(gcProjectFolder[idxFolder]) : NULL;
} 

// ________________________________________________________________________________
node* GetNodeEmbedding(node* rootIn[][3]) {
  // -- Get embedding node in projecta/embedding

  node* starprod = GetNodeProject(rootIn, 1, 2);
  return (starprod) ? starprod->GetChild("embedding") : NULL;
}

// ________________________________________________________________________________
node* GetNodePicoDsts(node* rootIn[][3], Int_t idxStorage, Int_t idxFolder) {
  // -- get picoDsts node on storage

  node* folder = GetNodeProject(rootIn, idxStorage, idxFolder);
  
  // -- project - star
  if (idxFolder == 1) // --- we should get rid of this data
    return (folder) ? folder->GetChild("starprod")->GetChild("picodsts") : NULL;

  // -- project - starprod
  else if (idxFolder == 2) 
    return (folder) ? folder->GetChild("picodsts") : NULL;
 
  return NULL;
}

// ________________________________________________________________________________
node* GetNodePwgSTAR(node* rootIn[][3]) {
  // -- Get PWGs

  node* star = GetNodeProject(rootIn, 0, 1);
  return (star) ? star->GetChild("pwg") : NULL;
}

// ________________________________________________________________________________
node* GetNodeUserRNC(node* rootIn[][3]) {
  // -- Get PWGs

  node* starprod = GetNodeProject(rootIn, 0, 2);
  return (starprod) ? starprod->GetChild("rnc") : NULL;
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
// -- read in input tree from project and projecta
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 

// ________________________________________________________________________________
void processFilePROJECT(ifstream &fin, TString &inFileName, node *fileRootNode) {
  // -- loop over file and add to fileRootNode node

  /*  here is the list of column names in the list
      0    INODE		20656 - Specifies the file's inode number
      1    GENERATION Number    65544 - Specifies a number that is incremented whenever an INODE number is reused.
      2    SMNAPSHOT ID	        541   - Specifies the snapshot ID
      3    FILE_SIZE 	        512   - Specifies the current size or length of the file, in bytes.
      4    FILESET_NAME 	star  - Specifies the fileset where the path name for the files is located.
      5    GENERATION Number    65544 - Specifies a number that is incremented whenever an INODE number is reused.
      6    MISC_ATTRIBUTES      D2u   - ( see details below )
      7    NLink 		2     - Specifies the number of hard links to the file.
      8    USER_ID		59805 - Specifies the numeric user ID of the owner of the file.
      9    GROUP_ID	        4002  - Specifies the numeric group ID of the file's group.
      10   MODE		        drwxr-x---  - ( see details below )
      11   ACCESS_TIME	        1455931754  - represents atime
      12   MODIFICATION_TIME    1455931754  - represents mtime
      13   BLOCKSIZE	        131072      - Specifies the size, in bytes, of each block of the file
      14   CHANGE_TIME        	1455931754  - represents ctime
      15   -- Separator
      16   FILENAME	        %2Fproject%2F.snapshots%2F2016-02-22%2Fprojectdirs%2Fstar%2Fpwg%2Fstarhf%2Fsimkom...... - Filename with full path
  */

  Int_t nlines = 0;

  // -- Loop of file - line-by-line
  while (1) {

    string line;
    getline (fin, line);
        
    // -- break at at of file
    if (fin.eof()) {
      printf("Processed %d lines of file %s\n", nlines, inFileName.Data());
      break;
    }
    
    // -- break if error occured during reading
    if (!fin.good()) {
      printf("Error after processing %d lines of file %s\n", nlines, inFileName.Data());
      break;
    }

    TString sLine(line);

    // -- tokenize line
    TObjArray *tokenizedLine = sLine.Tokenize(" ");
    if (!tokenizedLine) 
      printf("Error tokenizing line %d: %s\n", nlines, sLine.Data());
    else {

      // -- check for wrong lines
      if (tokenizedLine->GetEntriesFast() < 17 ) {
	printf("Error processing line %d: %s (%d tokens)\n", nlines, sLine.Data(), tokenizedLine->GetEntriesFast());
      }
      else {
	TString attr = ((static_cast<TObjString*>(tokenizedLine->At(6)))->String());

	// -- Process files only 
	if (attr.Contains("F")) {
	  // -- fill next row
	  ULong64_t size  = ((static_cast<TObjString*>(tokenizedLine->At(3)))->String()).Atoi();
	  Int_t     atime = ((static_cast<TObjString*>(tokenizedLine->At(11)))->String()).Atoi();
	  Int_t     mtime = ((static_cast<TObjString*>(tokenizedLine->At(12)))->String()).Atoi();
	  Int_t     ctime = ((static_cast<TObjString*>(tokenizedLine->At(14)))->String()).Atoi();
	  TString   name  = ((static_cast<TObjString*>(tokenizedLine->At(16)))->String()).ReplaceAll("%2F", "|||");

	  // -- handle file names with spaces
	  if (tokenizedLine->GetEntriesFast() > 17 ) {
	    for (Int_t idx = 17; idx < tokenizedLine->GetEntriesFast(); ++idx){
	      name  += " ";
	      name  += ((static_cast<TObjString*>(tokenizedLine->At(idx)))->String()).ReplaceAll("%2F", "|||");
	    }
	  }
	  
	  fileRootNode->AddFile(name, size, atime, ctime, mtime);
	}
      }

      tokenizedLine->Clear();
      delete tokenizedLine;
    }

    ++nlines;
    
    // -- print info on status
    //    if (!(nlines%100000))
    //      printf("Processing line %d of file %s\n", nlines, inFileName.Data());
  } // while (1) {
}

// ________________________________________________________________________________
node* processFolder(node* root, Int_t idxStorage, Int_t idxFolder) {
  // -- process folder
  
  // -- Add folder node
  node* folder = root->AddNode(Form("raw_%s_%s", gcStorage[idxStorage], gcProjectFolder[idxFolder]));

  TString sInFile(Form("%s/%s-%s.list", gcStorage[idxStorage], gcStoragePrefix[idxStorage], gcProjectFolder[idxFolder]));
    
  // -- open input file
  ifstream fin(sInFile);
  if (!fin.good()) {
    printf ("File %s couldn't be opened!\n", sInFile.Data());
    return NULL; 
  }
    
  // -- set global storage name
  gStorage = gcStorage[idxStorage];

  // -- loop over folder
  processFilePROJECT(fin, sInFile, folder);

  // -- close input file
  fin.close();
      
  // -- set new folder structure
  node* final = root->AddNode(Form("%s_%s", gcStorage[idxStorage], gcProjectFolder[idxFolder]));
  final->AddChildren(folder);

  // -- print folder
  printFolder(final);

  return final;
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
// -- Print folder / table
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 

// ________________________________________________________________________________
void printFolder(node* folder) {
  // -- print the output of a folder in the js file

  if (!folder)
    return;

  // -- open output file
  ofstream fout(Form("output/outfile_%s.js", folder->GetName()));
  if (!fout.good()) {
    printf ("OutFile outfile_%s.js couldn't be opened!\n", folder->GetName());
    return; 
  }

  //  printf ("OutFile outfile_%s.js\n", folder->GetName());

  // -- Print Tree 
  fout << "var "<< folder->GetName() << "DATA = [" << endl;
  folder->PrintNodes(fout);
  fout << "]" << endl;

  fout.close();
}

// ________________________________________________________________________________
void printTable(node* rootIn, Int_t idxVersion) {
  // -- print the output of a folder in html table

  if (!rootIn)
    return;

  // -- Sum up children for toplevel
  //  folder->SumAddChildren();

  TString outName("output/outfile_Table");

  if (idxVersion == 0)
    outName += "_Ext";
  
  // -- open output file
  ofstream fout(Form("%s_%s.txt", outName.Data(), rootIn->GetName()));
  if (!fout.good()) {
    printf ("File OutFile %s_%s.txt couldn't be opened!", outName.Data(), rootIn->GetName());
    return; 
  }

  //  printf("OutFile %s_%s.txt\n", outName.Data(), rootIn->GetName());

  // -- Fill table Entries
  if (idxVersion == 1) 
    rootIn->PrintTableEntries(fout, idxVersion);  

  else if (idxVersion == 2) { // ..... maybe obsolete - true for embedding
    // -- loop over storage folder
    for (Int_t idxStorage = 0; idxStorage < 5; ++idxStorage) {
      node* storage = rootIn->GetChild(gcStorage[idxStorage]);
      if (storage)
	storage->PrintTableEntries(fout, idxVersion);  
    }
  }
  else if (idxVersion == 0) {
    // -- loop over trgSetupFolder
    
    TIter next(rootIn->GetChildren());
    node* trgSetupName;
    while ((trgSetupName = static_cast<node*>(next()))) {

      // -- loop over children of production = particle
      TIter nextParticle(trgSetupName->GetChildren());
      node* particle;
      while ((particle = static_cast<node*>(nextParticle()))) {
	particle->PrintTableEntries(fout, idxVersion, trgSetupName->GetTitle());
      }
    }
  }

  fout.close();

  // -- open output file
  ofstream foutSum(Form("%s_%s_Sum.txt", outName.Data(), rootIn->GetName()));
  if (!foutSum.good()) {
    printf ("File OutFile %s_%s_Sum.txt couldn't be opened!", outName.Data(), rootIn->GetName());
    return; 
  }

  // -- Fill table Entries
  if (idxVersion == 1) 
    rootIn->PrintTableSummary(foutSum);  
  else if (idxVersion == 2) 
    rootIn->PrintTableSummary(foutSum, 1);  
  else if (idxVersion == 0) 
    rootIn->PrintTableSummary(foutSum, 2);  

  foutSum.close();
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
// -- Process use cases
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 

// ________________________________________________________________________________
node* processStorage(node* rootIn[][3], node* rootOut) {
  // -- process storage 
  
  for (Int_t idxStorage = 0; idxStorage < 2; ++idxStorage) {
  
    // -- get storage rootNode
    node* storage = rootOut->AddNode(gcStorage[idxStorage]);
    
    // -- loop over all folder
    for (Int_t idxFolder = 0; idxFolder < 3; ++idxFolder) {
      node* projectdirs = GetNodeProjectDirs(rootIn, idxStorage, idxFolder);
      if (projectdirs) 
	storage->AddChildren(projectdirs);
    }
    
    // -- print outfile
    printFolder(storage);
  }

  return rootOut;
}

// ________________________________________________________________________________
node* processUser(node* rootIn[][3], node* rootOut, Int_t idxGroup) {
  // -- process User for RNC and ALICE
  
  // -- add storage rootNode
  node* userRoot = rootOut->AddNode(Form("%s_user", gcFolder[idxGroup]));
  userRoot->SetTitle(Form("%s user", gcFolder[idxGroup]));

  node* group = (idxGroup == 0) ? GetNodeProject(rootIn, 0, 0) : GetNodeUserRNC(rootIn);
  if (!group)
    return NULL;
  
  userRoot->AddChildren(group);
  userRoot->SumAddChildren();
 
  return userRoot;
}

// ________________________________________________________________________________
node* processEmbedding(node* rootIn[][3], node* rootOut, Int_t version) {
  // -- process Embedding files for STAR
  //     V1 : trgSetupName > merged particles > production
  //     V2 : trgSetupName > particles > production
  
  // -- get storage rootNode
  node* embeddingRoot = rootOut->AddNode(Form("embeddingV%d", version));
  embeddingRoot->SetTitle(Form("STAR embedding V%d", version));

  // -- get embedding folder
  node* embedding = GetNodeEmbedding(rootIn);
  if (!embedding)
    return NULL;

  // -- Fill ---------------------
  if (version == 2) {
    embeddingRoot->AddChildren(embedding);
  }
  else if (version == 1) {
    // -- loop over children embedding = trgSetupName
    TIter next(embedding->GetChildren());
    node *trgSetupName;
    while ((trgSetupName = static_cast<node*>(next()))) {
      
      // -- add a copy to the tree
      node* embeddingTrgSetupName = embeddingRoot->AddNodeCopy(trgSetupName);
      
      // -- loop over children of production = particle
      TIter nextParticle(trgSetupName->GetChildren());
      node *particle;
      while ((particle = static_cast<node*>(nextParticle()))) {
	
	// -- find position of '_' and get part before = particle name
	TString sParticle(particle->GetTitle()); 
	if (sParticle.First('_') > 0 )
	  sParticle.Remove(sParticle.First('_'), sParticle.Length());
	
	node* embeddingParticle = embeddingTrgSetupName->AddNodeCopy(particle, sParticle);
	embeddingParticle->AddChildren(particle);
	embeddingTrgSetupName->SumAddChildren();
	
      } // while ((particle = static_cast<node*>(nextParticle()))) {
      embeddingTrgSetupName->SumAddChildren();
      
    } // while ((trgSetupName = static_cast<node*>(next()))) {
  }
  
  embeddingRoot->SumAddChildren();

  return embeddingRoot;
}

// ________________________________________________________________________________
node* processPicoDsts(node* rootIn[][3], node* rootOut, Int_t version) {
  // -- process PicoDsts files for STAR

  // -- add picoDsts rootNode
  node* picoDstsRoot = rootOut->AddNode(Form("picodstsv%d", version));
  picoDstsRoot->SetTitle(Form("STAR picoDsts V%d", version));

  // -- loop over folder
  for (Int_t idxFolder = 1; idxFolder < 3; ++idxFolder) {
    
    // -- get picoDsts folder
    node* picoDsts = GetNodePicoDsts(rootIn, 0, idxFolder);
    if (!picoDsts)
      continue;

    // -- Fill ---------------------

    // -- picoDsts, merged storage
    if (version == 1) {
      picoDstsRoot->CopyPropertiesOwn(picoDsts);
      picoDstsRoot->AddChildren(picoDsts);
    }

    // -- picoDsts > storage
    else if (version == 2) {

      // -- loop over children picoDsts = production
      TIter next(picoDsts->GetChildren());
      node *production;
      while ((production = static_cast<node*>(next()))) {
  	node* picoDstsProduction = picoDstsRoot->AddNodeCopy(production);
	node* picoDstsStorage = picoDstsProduction->AddNode(gcProjectFolder[idxFolder]);
	picoDstsStorage->AddChildren(production);

	picoDstsProduction->SumAddChildren();
      }
    }
    
    // -- storage > picoDsts
    else if (version == 3) {
      node* picoDstsStorage = picoDstsRoot->AddNode(gcProjectFolder[idxFolder]);
      picoDstsStorage->AddChildren(picoDsts);
    }

  } // for (Int_t idxFolder = 0; idxFolder < 5; ++idxFolder) {

  picoDstsRoot->SumAddChildren();

  return picoDstsRoot;
}

// ________________________________________________________________________________
node* processPwgSTAR(node* rootIn[][3], node* rootOut) {
  // -- process PWG files for STAR
  
  // -- add pwgSTAR rootNode
  node* pwgSTARRoot = rootOut->AddNode(Form("pwgstar"));
  pwgSTARRoot->SetTitle(Form("STAR PWGs"));

  // -- get pwg folder
  node* pwg = GetNodePwgSTAR(rootIn);
  if (!pwg)
    return NULL;
  
  pwgSTARRoot->CopyPropertiesOwn(pwg);
  pwgSTARRoot->AddChildren(pwg);
  pwgSTARRoot->SumAddChildren();

  return pwgSTARRoot;
}

// ---------------------------------------------------------------------------------------------------------------------------------------------  
// ---------------------------------------------------------------------------------------------------------------------------------------------  

// ________________________________________________________________________________
void parseGPFSDump(Int_t mode, Int_t parseIdx, Int_t folderIdx) {
  // -- parse input file
  //    mode: 0 - parse (default)
  //          1 - print
  //    parseIdx: 0 - project (default)
  //              1 - projecta
  //        -> Only relevant for mode 0
  //    folderIdx:0 - alice (default)
  //              1 - star
  //              2 - starprod
  //        -> Only relevant for mode 0

  // -- root node
  node *root;

  // -------------------------------------------------------------------------
  // -- parse GPFS Dump and write tree file
  //    inputs : alice, star, starprod - PROJECT
  //    inputs : starprod              - PROJECTA

  if (mode == 0) {
    root = new node;


    // -- process all folders seperatly 
    processFolder(root, parseIdx, folderIdx);

    // -------------------------------------------------------------------------
    // -- Save Parsed Tree
    TFile* outFile = TFile::Open(Form("treeOutput_%s_%s.root", gcStorage[parseIdx], gcProjectFolder[folderIdx]), "RECREATE");
    if (outFile) {
      outFile->cd();
      root->Write();
      outFile->Close();
    }
  }

  // -------------------------------------------------------------------------
  // -------------------------------------------------------------------------
  // -- read tree file and print 
  if (mode == 1) {

    TFile* fin[2][3]; 
    node* rootIn[2][3];

    for (Int_t idxStorage = 0; idxStorage < 2; ++idxStorage) {
      for (Int_t idxFolder = 0; idxFolder < 3; ++idxFolder) {
	if (idxStorage == 1 && idxFolder <2)
	  continue;
	
	fin[idxStorage][idxFolder] = TFile::Open(Form("treeOutput_%s_%s.root", gcStorage[idxStorage], gcProjectFolder[idxFolder]));    
	if (!fin[idxStorage][idxFolder]) {
	  printf("File treeOutput_%s_%s.root couldn't be opened!\n", gcStorage[idxStorage], gcProjectFolder[idxFolder]);
	  continue;
	}
	
	rootIn[idxStorage][idxFolder] = static_cast<node*>(fin[idxStorage][idxFolder]->Get("root"));
      }
    }

    node* root = new node;

    // -------------------------------------------------------------------------
    // -- loop over storage disks
    // -------------------------------------------------------------------------
    node* storage = root->AddNode("storage");
    processStorage(rootIn, storage);

    // -------------------------------------------------------------------------
    // -- create user only
    // -------------------------------------------------------------------------
    node* user = root->AddNode("user");
    for (Int_t idxGroup = 0; idxGroup < 2; ++idxGroup) {
      node* folder = processUser(rootIn, user, idxGroup);  
      printFolder(folder);
      printTable(folder, 1);
    }

    // -------------------------------------------------------------------------
    // -- create embedding only
    // -------------------------------------------------------------------------
    node* embedding = root->AddNode("embedding");
    //    V1 : trgSetupName > merged particles > production
    //    V2 : trgSetupName > particles > production

    for (Int_t idxVersion = 1; idxVersion < 3; ++idxVersion) {
      node* folder = processEmbedding(rootIn, embedding, idxVersion);
      printFolder(folder);
      
      if (idxVersion == 1) {
     	printTable(folder, 1);
	printTable(folder, 0);   // << - extended
      }
    }

    // -------------------------------------------------------------------------
    // -- create picoDsts only
    // -------------------------------------------------------------------------
    node* picoDsts = root->AddNode("picodsts");
    
    //    V1 : picoDsts, merged storage
    //    V2 : picoDsts > storage
    //    V3 : storage > picoDsts
    for (Int_t idxVersion = 1; idxVersion < 4; ++idxVersion) {
      node* folder = processPicoDsts(rootIn, picoDsts, idxVersion);
      folder->SetMaxLevel(gcMaxLevel+1);

      printFolder(folder);
      if (idxVersion == 1)
	printTable(folder, 1);
      else if (idxVersion == 3)
	printTable(folder, 2);

      folder->SetMaxLevel(gcMaxLevel);
    }

    // -------------------------------------------------------------------------
    // -- create STAR PWGs only
    // -------------------------------------------------------------------------
    node* pwgSTAR = root->AddNode("pwgstar");

    node* folder = processPwgSTAR(rootIn, pwgSTAR);
    folder->SetMaxLevel(gcMaxLevel+3);

    printFolder(folder);
    printTable(folder, 1);

    folder->SetMaxLevel(gcMaxLevel);

    // -------------------------------------------------------------------------

    // cout  << " >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
    // cout <<  root->GetTitle() << endl;
    // root->PrintChildren(2);
    // cout  << " >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> " << endl;
  }
  
  return;
}

