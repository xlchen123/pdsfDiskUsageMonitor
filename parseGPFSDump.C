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
static const Char_t*   gcFolder[3]         = {"alice", "rnc", "star"};
static const Char_t*   gcStorage[2]        = {"project", "projecta"}; 

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
    Short_t first = title.First('/');

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
    title.Remove(0,first+1);
    
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
      node* childOwn = AddNodeFullCopy(childOrig);

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

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
// -- read in input tree from project and projecta
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 

// ________________________________________________________________________________
void processFilePROJECT(ifstream &fin, TString &inFileName, node *fileRootNode) {
  // -- loop over file and add to fileRootNode node

  // -- fields of input file
  //    >> inode#|size in bytes|# entries/refs|uid|gid|mode|atime|mtime|blocks|ctime|pathname
  //       0     |1            |2             |3  |4  |5   |6    |7    |8     |9    |10    

  ULong64_t size;
  Int_t     entries, uid, gid, atime, ctime, mtime;
  TString   name;

  TString firstLine;

  Int_t nlines = 0;

  // -- read in first line
  fin >> firstLine; 

  // -- tokenize first line
  TObjArray *tokenizedFirstLine = firstLine.Tokenize("|");
  if (!tokenizedFirstLine) 
    printf("Error tokenizing first line %d: %s\n", nlines, firstLine.Data());
  else {
      
    // -- check unfinished lines
    if (tokenizedFirstLine->GetEntriesFast() != 11 ) 
      printf("Error processing line %d: %s\n", nlines, firstLine.Data());
    else {
      size    = ((static_cast<TObjString*>(tokenizedFirstLine->At(1)))->String()).Atoi();
      entries = ((static_cast<TObjString*>(tokenizedFirstLine->At(2)))->String()).Atoi();
      uid     = ((static_cast<TObjString*>(tokenizedFirstLine->At(3)))->String()).Atoi();
      gid     = ((static_cast<TObjString*>(tokenizedFirstLine->At(4)))->String()).Atoi();
      atime   = ((static_cast<TObjString*>(tokenizedFirstLine->At(6)))->String()).Atoi();
      mtime   = ((static_cast<TObjString*>(tokenizedFirstLine->At(7)))->String()).Atoi();
      ctime   = ((static_cast<TObjString*>(tokenizedFirstLine->At(9)))->String()).Atoi();
      name    = ((static_cast<TObjString*>(tokenizedFirstLine->At(10)))->String());
    }
  }

  // -- Loop of file - line-by-line
  while (1) {
    TString line;
    
    // -- read in
    fin >> line; 

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

    // -- tokenize line
    TObjArray *tokenizedLine = line.Tokenize("|");
    if (!tokenizedLine) 
      printf("Error tokenizing line %d: %s\n", nlines, line.Data());
    else {
      
      // -- check unfinished lines (= left overs 
      if (tokenizedLine->GetEntriesFast() != 11 ) {
	//	printf("Error processing line %d: %s[ ]%s\n", nlines, name.Data(), line.Data());
	name += " ";
	name += line;
	continue;
      }
      else {
	// -- Add last row , entries == 1 -> only files
	if (entries == 1)
	  fileRootNode->AddFile(name, size, atime, ctime, mtime);

	// -- fill next row
	size    = ((static_cast<TObjString*>(tokenizedLine->At(1)))->String()).Atoi();
	entries = ((static_cast<TObjString*>(tokenizedLine->At(2)))->String()).Atoi();
	uid     = ((static_cast<TObjString*>(tokenizedLine->At(3)))->String()).Atoi();
	gid     = ((static_cast<TObjString*>(tokenizedLine->At(4)))->String()).Atoi();
	atime   = ((static_cast<TObjString*>(tokenizedLine->At(6)))->String()).Atoi();
	mtime   = ((static_cast<TObjString*>(tokenizedLine->At(7)))->String()).Atoi();
	ctime   = ((static_cast<TObjString*>(tokenizedLine->At(9)))->String()).Atoi();
	name    = ((static_cast<TObjString*>(tokenizedLine->At(10)))->String());
      }

      tokenizedLine->Clear();
      delete tokenizedLine;
    }

    ++nlines;
    
    // -- print info on status
    if (!(nlines%100000))
      printf("Processing line %d of file %s\n", nlines, inFileName.Data());
  }

  // -- Add last row , entries == 1 -> only files
  if (entries == 1)
    fileRootNode->AddFile(name, size, atime, ctime, mtime);
}

// ________________________________________________________________________________
void processFilePROJECTA(ifstream &fin, TString &inFileName, node *fileRootNode) {
  // -- loop over file and add to fileRootNode node

  // -- fields of input file
  /*
    0  uid      59535    
    1  gid      5008    
    2  size     3790800  
    3  xtime    2015-07-24 18:15:56.378265  
    4  mtime    2015-07-24 19:49:31.803646  
    5  atime    2015-07-24 18:15:56.380693  
    6  ctime    2015-07-24 18:15:56.378265  
    7  pathname /global/projecta/projectdirs/starprod/embedding/AuAu20....
  */
  
  ULong64_t size;
  Int_t     entries, uid, gid;
  TString   atimeS, ctimeS, mtimeS, xtimeS;
  TString   atimeS2, ctimeS2, mtimeS2, xtimeS2;
  TString   name;

  // -- ignore first row of headers
  // fin.ignore(10000,'\n');

  // -- Loop of file - line-by-line
  Int_t nlines = 0;
  while (1) {

    // -- read in
    fin >> uid >> gid >> size >> 
      atimeS >> atimeS2 >> 
      mtimeS >> mtimeS2 >> 
      ctimeS >> ctimeS2 >> 
      xtimeS >> xtimeS2 >> name;
    
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
    
    UInt_t mtimeU = mtimeS.ReplaceAll("-", "").Atoi();
    UInt_t atimeU = atimeS.ReplaceAll("-", "").Atoi();
    UInt_t ctimeU = ctimeS.ReplaceAll("-", "").Atoi();
    UInt_t xtimeU = xtimeS.ReplaceAll("-", "").Atoi();

    TTimeStamp mtimeT(mtimeU, 0u, 0u, kFALSE);
    TTimeStamp atimeT(atimeU, 0u, 0u, kFALSE);
    TTimeStamp ctimeT(ctimeU, 0u, 0u, kFALSE);
    TTimeStamp xtimeT(xtimeU, 0u, 0u, kFALSE);

    Int_t mtime = mtimeT.GetSec();
    Int_t atime = atimeT.GetSec();
    Int_t ctime = ctimeT.GetSec();
    Int_t xtime = xtimeT.GetSec();

    fileRootNode->AddFile(name, size, atime, ctime, mtime);
    
    ++nlines;
    
    // -- print info on status
    if (!(nlines%100000))
      printf("Processing line %d of file %s\n", nlines, inFileName.Data());
  }
}

// ________________________________________________________________________________
node* processFolderPROJECT(node* root, Int_t idxFolder) {
  // -- process folder
  
  // -- Add folder node
  node* folder = root->AddNode(Form("project_%s", gcProjectFolder[idxFolder]));

  TString sInFile(Form("project/prj2-%s.list", gcProjectFolder[idxFolder]));
    
  // -- open input file
  ifstream fin(sInFile);
  if (!fin.good()) {
    printf ("File %s couldn't be opened!", sInFile.Data());
    return NULL; 
  }
    
  // -- set global storage name
  gStorage = gcStorage[0];

  // -- loop over folder
  processFilePROJECT(fin, sInFile, folder);

  // -- set global storage name
  gStorage = "";
  
  // -- close input file
  fin.close();
    
  return folder;
}

// ________________________________________________________________________________
node* processFolderPROJECTA(node* root, Int_t idxFolder) {
  // -- process folder
  
  // -- Add folder node
  node* folder = root->AddNode(Form("projecta_%s", gcProjectFolder[idxFolder]));

  TString sInFile(Form("projecta/prjA-starprod.list"));
    
  // -- open input file
  ifstream fin(sInFile);
  if (!fin.good()) {
    printf ("File %s couldn't be opened!", sInFile.Data());
    return NULL; 
  }
    
  // -- set global storage name
  gStorage = gcStorage[1];

  // -- loop over folder
  processFilePROJECTA(fin, sInFile, folder);

  // -- set global storage name
  gStorage = "";
  
  // -- close input file
  fin.close();
    
  return folder;
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
// -- Print folder / table
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 

// ________________________________________________________________________________
void printFolder(node* folder) {
  // -- print the output of a folder in the js file

  // -- open output file
  ofstream fout(Form("outfile_%s.js", folder->GetName()));
  if (!fout.good()) {
    printf ("File OutFile outfile_%s.js couldn't be opened!", folder->GetName());
    return; 
  }

  // -- Sum up children for toplevel
  // folder->SumAddChildren();

  // -- Print Tree 
  fout << "var "<< folder->GetName() << "DATA = [" << endl;
  folder->PrintNodes(fout);
  fout << "]" << endl;

  fout.close();
}

// ________________________________________________________________________________
void printTable(node* rootIn, Int_t idxVersion = 1) {
  // -- print the output of a folder in html table

  // -- Sum up children for toplevel
  //  folder->SumAddChildren();

  TString outName("outfile_Table");

  if (idxVersion == 0)
    outName += "_Ext";
  
  // -- open output file
  ofstream fout(Form("%s_%s.txt", outName.Data(), rootIn->GetName()));
  if (!fout.good()) {
    printf ("File OutFile %s_%s.txt couldn't be opened!", outName.Data(), rootIn->GetName());
    return; 
  }

  // -- Fill table Entries
  if (idxVersion == 1) 
    rootIn->PrintTableEntries(fout, idxVersion);  

  else if (idxVersion == 2) {
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

  /*
  else if (idxVersion == 3) {
    // -- loop over storage folder
    for (Int_t idxStorage = 0; idxStorage < 5; ++idxStorage) {
      node* storage = rootIn->GetChild(gcStorage[idxStorage]);
      if (storage)
	storage->PrintTableSummary(fout, 1);  
    }
  }
  */
  foutSum.close();
}

// ________________________________________________________________________________
void printTableEmbeddingExt(node* embeddingExtRoot) {
  // -- print the output of a folder in to a html table
  //    -- adding extra fields for embedding

  // -- Sum up children for toplevel
  //  embeddingExtRoot->SumAddChildren();

  // -- open output file
  ofstream fout(Form("outfile_TableExt_%s.txt", embeddingExtRoot->GetName()));
  if (!fout.good()) {
    printf ("File OutFile outfile_TableExt_%s.txt couldn't be opened!", embeddingExtRoot->GetName());
    return; 
  }

  // -- loop over children of embeddingExtRoot = production
  TIter nextProduction(embeddingExtRoot->GetChildren());
  node* production;
  while ((production = static_cast<node*>(nextProduction()))) {

    // -- loop over children of production = particle
    TIter nextParticle(production->GetChildren());
    node* particle;
    while ((particle = static_cast<node*>(nextParticle()))) {
      particle->PrintTableEntries(fout, 4, production->GetTitle());
    } //  while ((production = static_cast<node*>(nextParticle()))) {
  } //  while ((production = static_cast<node*>(nextProduction()))) {
  
  fout.close();


  // -- open output file
  ofstream foutSum(Form("outfile_TableExt_%s_Sum.txt", embeddingExtRoot->GetName()));
  if (!foutSum.good()) {
    printf ("File OutFile outfile_TableExt_%s_Sum.txt couldn't be opened!", embeddingExtRoot->GetName());
    return; 
  }

  embeddingExtRoot->PrintTableSummary(foutSum, 2);  

  foutSum.close();

}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
// -- Process use cases
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 


// ________________________________________________________________________________
node* processStorage(node* rootIn, node* rootOut, Int_t idxStorage) {
  // -- process storage ...
  
  // -- get storage rootNode
  node* storage = rootOut->AddNode(gcStorage[idxStorage]);
  
  // -- elizas
  if (idxStorage < 4) {
        // -- loop over all folder
    for (Int_t idxFolder = 0; idxFolder <3; ++idxFolder) {
      node* folder = rootIn->GetChild(gcFolder[idxFolder]);
      if (!folder)
	continue;
      
      // -- proper storage
      node* dataStorage = folder->GetChild(gcStorage[idxStorage]);
      if (!dataStorage)
	continue;
      
      // -- add children from other list here
      storage->AddChildren(dataStorage);
    }
  }

  // -- project
  else {
      // -- loop over all folder
    for (Int_t idxFolder = 0; idxFolder <3; ++idxFolder) {
      node* folder = rootIn->GetChild(Form("project_%s", gcProjectFolder[idxFolder]));
      if (!folder)
	continue;
      
      node* projectdirs = folder->GetChild("projectdirs");
      if (!projectdirs)
	continue;
      
      // -- add children to list
      storage->AddChildren(projectdirs);
    }
  }
  
  return storage;
}

// ________________________________________________________________________________
node* processUser(node* rootIn, node* rootOut, Int_t version, Int_t idxFolder) {
  // -- process User for RNC and ALICE
  
  // -- add storage rootNode
  node* userRoot = rootOut->AddNode(Form("%s_userv%d", gcFolder[idxFolder], version));
  userRoot->SetTitle(Form("%s user V%d", gcFolder[idxFolder], version));

  // -- loop over storage folder
  for (Int_t idxStorage = 0; idxStorage < 5; ++idxStorage) {
    node* storage = rootIn->GetChild(gcStorage[idxStorage]);
    if (!storage)
      continue;

    // -- get parent folder for users in two different ways = group
    //    for star/rnc on project)
    if (idxFolder == 1 && idxStorage == 4)
      ++idxFolder;

    node* group = storage->GetChild(gcFolder[idxFolder]);
    if (!group)
      continue;

    // -- Fill ---------------------

    // -- sum up over all storage
    if (version == 1) 
      userRoot->AddChildren(group);

    // -- user > storage
    else if (version == 2) {
      // -- loop over children group = user
      TIter next(group->GetChildren());
      node *user;
      while ((user = static_cast<node*>(next()))) {
  	node* userUser = userRoot->AddNodeCopy(user);
	node* userStorage = userUser->AddNode(gcStorage[idxStorage]);
	userStorage->AddChildren(user);

	userUser->SumAddChildren();
      }
    }

    // -- storage > user
    else if (version == 3) {
      node* userStorage = userRoot->AddNode(gcStorage[idxStorage]);
      userStorage->AddChildren(group);
    }
  }

  // -- Sum up children for toplevel
  userRoot->SumAddChildren();
 
  return userRoot;

}

// ________________________________________________________________________________
node* processEmbedding(node* rootIn, node* rootOut, Int_t version) {
  // -- process Embedding files for STAR
  
  // -- get storage rootNode
  node* embeddingRoot = rootOut->AddNode(Form("embeddingV%d", version));
  embeddingRoot->SetTitle(Form("STAR embedding V%d", version));
    
  // -- loop over storage folder
  for (Int_t idxStorage = 0; idxStorage < 5; ++idxStorage) {
    node* storage = rootIn->GetChild(gcStorage[idxStorage]);
    if (!storage)
      continue;

    // -- get starprod folder in two different ways (eliza, project)
    node* starprod = NULL;

    // -- elizas
    if (idxStorage < 4) {
      // -- get star folder - as child of storage folder
      node* star = storage->GetChild("star");
      if (!star)
	continue;

      starprod = star->GetChild("starprod");
    }
    // -- project
    else
      starprod = storage->GetChild("starprod");
   
    if (!starprod)
      continue;
   
    // -- get embedding folder
    node* embedding = starprod->GetChild("embedding");
    if (!embedding)
      continue;

    // -- Fill ---------------------
      
    if (version == 5) {
      node* embeddingStorage = embeddingRoot->AddNodeCopy(embedding, gcStorage[idxStorage]);
      embeddingStorage->AddChildren(embedding);
    }
    else if (version == 4) {
      // -- loop over children embedding = trgSetupName
      TIter next(embedding->GetChildren());
      node *trgSetupName;
      while ((trgSetupName = static_cast<node*>(next()))) {
	
	// -- add trgSetupName -> storage  
	node* embeddingTrgSetupName = embeddingRoot->AddNode(trgSetupName->GetTitle());
	node* embeddingStorage = embeddingTrgSetupName->AddNode(gcStorage[idxStorage]);
	embeddingStorage->AddChildren(trgSetupName);
	
	embeddingTrgSetupName->SumAddChildren();
      }
    }
    if (version == 3) {
      embeddingRoot->AddChildren(embedding);
    }
    else if (version == 2) {
      node* embeddingStorage = embeddingRoot->AddNodeCopy(embedding, gcStorage[idxStorage]);
      
      // -- loop over children embedding = trgSetupName
      TIter next(embedding->GetChildren());
      node *trgSetupName;
      while ((trgSetupName = static_cast<node*>(next()))) {
	
	// -- add a copy to the tree
	node* embeddingTrgSetupName = embeddingStorage->AddNodeCopy(trgSetupName);

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
      embeddingStorage->SumAddChildren();
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
  } // for (Int_t idxStorage = 0; idxStorage < 5; ++idxStorage) { 

  embeddingRoot->SumAddChildren();

  return embeddingRoot;
}

// ________________________________________________________________________________
node* processPicoDsts(node* rootIn, node* rootOut, Int_t version = 1) {
  // -- process PicoDsts files for STAR

  // -- add picoDsts rootNode
  node* picoDstsRoot = rootOut->AddNode(Form("picodstsv%d", version));
  picoDstsRoot->SetTitle(Form("STAR picoDsts V%d", version));

  // -- loop over storage folder
  for (Int_t idxStorage = 0; idxStorage < 5; ++idxStorage) {
    node* storage = rootIn->GetChild(gcStorage[idxStorage]);
    if (!storage)
      continue;
  
    // -- get starprod folder in two different ways (eliza, project)
    node* starprod = NULL;
    
    // -- elizas
    if (idxStorage < 4) {
      // -- get star folder - as child of storage folder
      node* star = storage->GetChild("star");
      if (!star)
	continue;
      
      starprod = star->GetChild("starprod");
    }
    // -- project
    else
      starprod = storage->GetChild("starprod");
   
    if (!starprod)
      continue;
   
    // -- get picoDsts folder
    node* picoDsts = starprod->GetChild("picodsts");
    if (!picoDsts)
      continue;

    // -- Fill ---------------------

    // -- sum up over all storage
    if (version == 1) {
      picoDstsRoot->CopyPropertiesOwn(picoDsts);
      picoDstsRoot->AddChildren(picoDsts);
    }

    // -- user > picoDsts
    else if (version == 2) {

      // -- loop over children picoDts = production
      TIter next(picoDsts->GetChildren());
      node *production;
      while ((production = static_cast<node*>(next()))) {
  	node* picoDstsProduction = picoDstsRoot->AddNodeCopy(production);
	node* picoDstsStorage = picoDstsProduction->AddNode(gcStorage[idxStorage]);
	picoDstsStorage->AddChildren(production);

	picoDstsProduction->SumAddChildren();
      }
    }
    // -- storage > user
    else if (version == 3) {
      node* picoDstsStorage = picoDstsRoot->AddNode(gcStorage[idxStorage]);
      picoDstsStorage->AddChildren(picoDsts);
    }

  } // for (Int_t idxStorage = 0; idxStorage < 5; ++idxStorage) {

  picoDstsRoot->SumAddChildren();

  return picoDstsRoot;
}

// ________________________________________________________________________________
node* processPwgSTAR(node* rootIn, node* rootOut, Int_t version = 1) {
  // -- process PWG files for STAR
  
  // -- add pwgSTAR rootNode
  node* pwgSTARRoot = rootOut->AddNode(Form("pwgstarv%d", version));
  pwgSTARRoot->SetTitle(Form("STAR PWGs V%d", version));

  // -- loop over storage folder
  for (Int_t idxStorage = 0; idxStorage < 5; ++idxStorage) {
    node* storage = rootIn->GetChild(gcStorage[idxStorage]);
    if (!storage)
      continue;

    // -- get star folder
    node* star = storage->GetChild("star");    
    if (!star)
      continue;
    
    // -- get pwg folder
    node* pwg = star->GetChild("pwg");
    if (!pwg)
      continue;
    
    // -- Fill ---------------------

    // -- sum up over all storage
    if (version == 1) {
      pwgSTARRoot->CopyPropertiesOwn(pwg);
      pwgSTARRoot->AddChildren(pwg);
    }

    // -- user > pwgs
    else if (version == 2) {

      // -- loop over children picoDts = production
      TIter next(pwg->GetChildren());
      node *pwgName;
      while ((pwgName = static_cast<node*>(next()))) {
  	node* pwgSTARProduction = pwgSTARRoot->AddNodeCopy(pwgName);
	node* pwgSTARStorage = pwgSTARProduction->AddNode(gcStorage[idxStorage]);
	pwgSTARStorage->AddChildren(pwgName);

	pwgSTARProduction->SumAddChildren();
      }
    }
    // -- storage > user
    else if (version == 3) {
      node* pwgSTARStorage = pwgSTARRoot->AddNode(gcStorage[idxStorage]);
      pwgSTARStorage->AddChildren(pwg);
    }
  }
 
  pwgSTARRoot->SumAddChildren();

  return pwgSTARRoot;
}

// ---------------------------------------------------------------------------------------------------------------------------------------------  
// ---------------------------------------------------------------------------------------------------------------------------------------------  

// ________________________________________________________________________________
void parseGPFSDump(Int_t mode = 0) {
  // -- parse input file
  //    mode: 0 - parse
  //          1 - print
  //          2 - parse and print

  // -- root node
  node *root;

  // -------------------------------------------------------------------------
  // -------------------------------------------------------------------------
  // -- parse GPFS Dump and write tree file
  if (mode == 0 || mode == 2) {
    root = new node;

    // -- loop over all outputs : alice, star, starprod - PROJECT
    for (Int_t idxFolder = 0; idxFolder <3; ++idxFolder) {
      node * folder = processFolderPROJECT(root, idxFolder);
      printFolder(folder);
    }

    // -- loop over all outputs : starprod - PROJECTA
    node * folder = processFolderPROJECTA(root, 2);
    printFolder(folder);
    
    // -------------------------------------------------------------------------
    // -- Save Parsed Tree
    TFile* outFile = TFile::Open("treeOutput.root", "RECREATE");
    if (outFile) {
      outFile->cd();
      root->Write();
      outFile->Close();
    }
  }

  // -------------------------------------------------------------------------
  // -------------------------------------------------------------------------
  // -- read tree file and print 
  if (mode == 1 || mode == 2) {
    
    TFile* fin = TFile::Open("treeOutput.root");
    if (!fin) {
      printf ("File treeOutput.root couldn't be opened!");
      return;
    }
    
    root = static_cast<node*>(fin->Get("root"));

    // -------------------------------------------------------------------------
    // -- loop over storage disks
    // -------------------------------------------------------------------------
    node* storage = root->AddNode("storage");

    for (Int_t idxStorage = 0; idxStorage < 5; ++idxStorage) {
      node* folder = processStorage(root, storage, idxStorage);
      printFolder(folder);
    }

    // -------------------------------------------------------------------------
    // -- create user only
    // -------------------------------------------------------------------------
    node* user = root->AddNode("user");

    // -- process user
    //    V1 : user, merged storage
    //    V2 : user > storage
    //    V3 : storage > user
    for (Int_t idxGroup = 0; idxGroup < 2; ++idxGroup) {
      for (Int_t idxVersion = 1; idxVersion < 4; ++idxVersion) {
	node* folder = processUser(storage, user, idxVersion, idxGroup);  
	printFolder(folder);
	
	if (idxVersion == 1)
	  printTable(folder, 1);
	else if (idxVersion == 3)
	  printTable(folder, 2);
      }
    }

    // -------------------------------------------------------------------------
    // -- create embedding only
    // -------------------------------------------------------------------------
    node* embedding = root->AddNode("embedding");
    // -- process embedding V1 : trgSetupName > merged particles > production
    //                      V2 : storage > trgSetupName > merged particles > production
    //                      V3 : trgSetupName > particles > production
    //                      V4 : trgSetupName > storage > particles > production
    //                      V5 : storage > trgSetupName > particles > production
    for (Int_t idxVersion = 1; idxVersion < 6; ++idxVersion) {
      node* folder = processEmbedding(storage, embedding, idxVersion);
      if (idxVersion == 5 || idxVersion == 2 || idxVersion == 4)
	folder->SetMaxLevel(gcMaxLevel+1);
      printFolder(folder);

      if (idxVersion == 1) {
	printTable(folder, 1);
	printTable(folder, 0);   // << - extended
      }
      else if (idxVersion == 2)
	printTable(folder, 2);

      if (idxVersion == 5 || idxVersion == 2 || idxVersion == 4)
	folder->SetMaxLevel(gcMaxLevel);
    }

    // -------------------------------------------------------------------------
    // -- create picoDsts only
    // -------------------------------------------------------------------------
    node* picoDsts = root->AddNode("picodsts");
    
    //    V1 : picoDsts, merged storage
    //    V2 : picoDsts > storage
    //    V3 : storage > picoDsts
    for (Int_t idxVersion = 1; idxVersion < 4; ++idxVersion) {
      node* folder = processPicoDsts(storage, picoDsts, idxVersion);
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

    //    V1 : pwgSTAR, merged storage
    //    V2 : pwgSTAR > storage
    //    V3 : storage > pwgSTAR
    for (Int_t idxVersion = 1; idxVersion < 4; ++idxVersion) {
      node* folder = processPwgSTAR(storage, pwgSTAR, idxVersion);
      folder->SetMaxLevel(gcMaxLevel+3);
      printFolder(folder);
      if (idxVersion == 1) 
	printTable(folder, 1);
      else if (idxVersion == 3)
	printTable(folder, 2);
      folder->SetMaxLevel(gcMaxLevel);
    }   
    // -------------------------------------------------------------------------
  }

  return;
}

