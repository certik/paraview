/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVFileInformation.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVFileInformation.h"

#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkObjectFactory.h"
#include "vtkPVFileInformationHelper.h"
#include "vtkSmartPointer.h"

#if defined(_WIN32)
# define _WIN32_IE 0x0400  // special folder support
# include <windows.h>   // FindFirstFile, FindNextFile, FindClose, ...
# include <direct.h>    // _getcwd
# include <shlobj.h>    // SHGetFolderPath
# include <sys/stat.h>  // stat
# include <string.h>   // for strcasecmp
# define vtkPVServerFileListingGetCWD _getcwd
#else
# include <sys/types.h> // DIR, struct dirent, struct stat
# include <sys/stat.h>  // stat
# include <dirent.h>    // opendir, readdir, closedir
# include <unistd.h>    // access, getcwd
# include <errno.h>     // errno
# include <string.h>    // strerror
# include <stdlib.h>    // getenv
# define vtkPVServerFileListingGetCWD getcwd
#endif
#if defined (__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#include <vtkstd/vector>
#endif

#include <vtksys/SystemTools.hxx>
#include <vtksys/RegularExpression.hxx>
#include <vtkstd/set>
#include <vtkstd/string>

vtkStandardNewMacro(vtkPVFileInformation);
vtkCxxRevisionMacro(vtkPVFileInformation, "$Revision: 1.20 $");

inline void vtkPVFileInformationAddTerminatingSlash(vtkstd::string& name)
{
  if (name.size()>0)
    {
    char last = *(name.end()-1);
    if (last != '/' && last != '\\')
      {
      name += "/";
      }
    }
}

#if defined(_WIN32)
static bool vtkPVFileInformationResolveLink(const vtkstd::string& fname,
                                                      vtkstd::string& result,
                                                      WIN32_FIND_DATA& wfd)
{
  IShellLink* shellLink;
  HRESULT hr;
  char Link[MAX_PATH];
  bool coInit = false;
  bool success = false;

  hr = ::CoCreateInstance(CLSID_ShellLink, NULL, 
                          CLSCTX_INPROC_SERVER, IID_IShellLink,
                          (LPVOID*)&shellLink);
  if(hr == CO_E_NOTINITIALIZED)
    {
    coInit = true;
    ::CoInitialize(NULL);
    hr = ::CoCreateInstance(CLSID_ShellLink, NULL, 
                            CLSCTX_INPROC_SERVER, IID_IShellLink, 
                            (LPVOID*)&shellLink);
    }
  if(SUCCEEDED(hr))
    {
    IPersistFile* ppf;
    hr = shellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
    if(SUCCEEDED(hr))
      {
      vtkstd::wstring wfname(fname.begin(), fname.end());
      hr = ppf->Load((LPOLESTR)wfname.c_str(), STGM_READ);
      if(SUCCEEDED(hr))
        {
        if(shellLink->GetPath(Link, MAX_PATH, &wfd, SLGP_UNCPRIORITY) == NOERROR)
          {
          result = Link;
          success = true;
          }
        ppf->Release();
        }
      }
    shellLink->Release();
    }
  if(coInit)
    {
    CoUninitialize();
    }

  return success;

}
#endif


//-----------------------------------------------------------------------------
class vtkPVFileInformationSet : 
  public vtkstd::set<vtkSmartPointer<vtkPVFileInformation> >
{
};

//-----------------------------------------------------------------------------
vtkPVFileInformation::vtkPVFileInformation()
{
  this->RootOnly = 1;
  this->Contents = vtkCollection::New();
  this->Type = INVALID;
  this->Name = 0;
  this->FullPath = 0;
  this->FastFileTypeDetection = 0;
}

//-----------------------------------------------------------------------------
vtkPVFileInformation::~vtkPVFileInformation()
{
  this->Contents->Delete();
  this->SetName(0);
  this->SetFullPath(0);
}

//-----------------------------------------------------------------------------
void vtkPVFileInformation::CopyFromObject(vtkObject* object)
{
  this->Initialize();

  vtkPVFileInformationHelper* helper = 
    vtkPVFileInformationHelper::SafeDownCast(object);
  if (!helper)
    {
    vtkErrorMacro(
      "Can collect information only from a vtkPVFileInformationHelper.");
    return;
    }

  if (helper->GetSpecialDirectories())
    {
    this->GetSpecialDirectories();
    return;
    }

  this->FastFileTypeDetection = helper->GetFastFileTypeDetection();

  vtkstd::string working_directory = 
    vtksys::SystemTools::GetCurrentWorkingDirectory().c_str();
  if (helper->GetWorkingDirectory() && helper->GetWorkingDirectory()[0])
    {
    working_directory = helper->GetWorkingDirectory();
    }

  vtkstd::string path = vtksys::SystemTools::CollapseFullPath(helper->GetPath(),
    working_directory.c_str());
  
  this->SetName(helper->GetPath());
#if defined(_WIN32)
  vtkstd::string::size_type idx;
  for(idx = path.find('/', 0);
      idx != vtkstd::string::npos;
      idx = path.find('/', idx))
    {
    path.replace(idx, 1, 1, '\\');
    }
  size_t len = path.size();
  WIN32_FIND_DATA data;
  if(len > 4 && strncmp(path.c_str()+len-4, ".lnk", 4) == 0)
  {
  vtkPVFileInformationResolveLink(path, path, data);
  }
#endif
  this->SetFullPath(path.c_str());

  if (!vtksys::SystemTools::FileExists(this->FullPath))
    {
    return;
    }

  bool is_directory = vtksys::SystemTools::FileIsDirectory(this->FullPath);
  this->Type = (is_directory)? DIRECTORY : SINGLE_FILE;
#if defined(_WIN32)
  if (!is_directory)
    {
    // Path may be drive letter.
    char strings[1024];
    DWORD n = GetLogicalDriveStrings(1024, strings);
    char* start = strings;
    char* end = start;
    for(;end != strings+n; ++end)
      {
      if(!*end)
        {
        if (stricmp(start,this->FullPath) == 0)
          {
          is_directory = true;
          this->Type = DRIVE;
          break;
          }
        start = end+1;
        }
      }
    }
#endif

  if (!helper->GetDirectoryListing() || !is_directory)
    {
    return;
    }

  // Since we want a directory listing, we now to platform specific listing 
  // with intelligent pattern matching hee-haa.

#if defined(_WIN32)
  this->GetWindowsDirectoryListing();
#else
  this->GetDirectoryListing();
#endif
}

//-----------------------------------------------------------------------------
void vtkPVFileInformation::GetSpecialDirectories()
{
#if defined (_WIN32)

  // Return favorite directories ...

  TCHAR szPath[MAX_PATH];

  if(SUCCEEDED(SHGetSpecialFolderPath(NULL, szPath, CSIDL_PERSONAL, false)))
    {
    vtkSmartPointer<vtkPVFileInformation> info =
      vtkSmartPointer<vtkPVFileInformation>::New();
    info->SetFullPath(szPath);
    info->SetName("My Documents");
    info->Type = DIRECTORY;
    this->Contents->AddItem(info);
    }

  if(SUCCEEDED(SHGetSpecialFolderPath(NULL, szPath, CSIDL_DESKTOPDIRECTORY, false)))
    {
    vtkSmartPointer<vtkPVFileInformation> info =
      vtkSmartPointer<vtkPVFileInformation>::New();
    info->SetFullPath(szPath);
    info->SetName("Desktop");
    info->Type = DIRECTORY;
    this->Contents->AddItem(info);
    }

  if(SUCCEEDED(SHGetSpecialFolderPath(NULL, szPath, CSIDL_FAVORITES, false)))
    {
    vtkSmartPointer<vtkPVFileInformation> info =
      vtkSmartPointer<vtkPVFileInformation>::New();
    info->SetFullPath(szPath);
    info->SetName("Favorites");
    info->Type = DIRECTORY;
    this->Contents->AddItem(info);
    }

  // Return drive letters ...
  char strings[1024];
  DWORD n = GetLogicalDriveStrings(1024, strings);
  char* start = strings;
  char* end = start;
  for(;end != strings+n; ++end)
    {
    if(!*end)
      {
      vtkSmartPointer<vtkPVFileInformation> info =
        vtkSmartPointer<vtkPVFileInformation>::New();
      info->SetFullPath(start);
      info->SetName(start);
      info->Type = DRIVE;
      this->Contents->AddItem(info);
      start = end+1;
      }
    }

#else // _WIN32
#if defined (__APPLE__ )
    //-------- Get the List of Mounted Volumes from the System
    int idx = 1;
    HFSUniStr255 hfsname;
    FSRef ref;
    while(noErr == FSGetVolumeInfo(kFSInvalidVolumeRefNum, idx++, NULL,
                                   kFSVolInfoNone, NULL, &hfsname, &ref))
    {
      CFURLRef resolvedUrl = CFURLCreateFromFSRef(NULL, &ref);
      if(resolvedUrl)
      {
        CFStringRef url;
        url = CFURLCopyFileSystemPath(resolvedUrl, kCFURLPOSIXPathStyle);
        CFStringRef cfname = CFStringCreateWithCharacters(kCFAllocatorDefault,
                                                          hfsname.unicode, hfsname.length);

        CFIndex pathSize = CFStringGetLength(url)+1;
        vtkstd::vector<char> pathChars(pathSize, 0);
        OSStatus pathStatus = CFStringGetCString(url,&pathChars[0],pathSize, 
                                                  kCFStringEncodingASCII);

        pathSize = CFStringGetLength(cfname)+1;
        vtkstd::vector<char> nameChars(pathSize, 0);
        OSStatus nameStatus = CFStringGetCString(cfname, &nameChars[0], pathSize, 
                                                  kCFStringEncodingASCII);

        if (pathStatus && nameStatus)
        {
          vtkSmartPointer<vtkPVFileInformation> info = 
                          vtkSmartPointer<vtkPVFileInformation>::New();
          info->SetFullPath( &(pathChars.front() ) );
          info->SetName( &(nameChars.front() ) );
          info->Type = DRIVE;
          this->Contents->AddItem(info);
        }
        CFRelease(cfname);
        CFRelease(resolvedUrl);
      }
    }
    //-- Read the com.apple.sidebar.plist file to get the user's list of directories
    CFPropertyListRef p = CFPreferencesCopyAppValue(CFSTR("useritems"),
                                                    CFSTR("com.apple.sidebarlists"));
    if(p && CFDictionaryGetTypeID() == CFGetTypeID(p))
    {
      CFArrayRef r = (CFArrayRef)(CFDictionaryGetValue((CFDictionaryRef)p, CFSTR("CustomListItems")));
      if(r && CFArrayGetTypeID() == CFGetTypeID(r))
      {
        int count = CFArrayGetCount(r);
        for(int i=0; i<count; i++)
        {
          CFDictionaryRef dr = (CFDictionaryRef)CFArrayGetValueAtIndex(r, i);
          if(dr && CFDictionaryGetTypeID() == CFGetTypeID(dr))
          {
            CFStringRef name;
            CFStringRef url;
            CFDataRef alias;
            if(CFDictionaryGetValueIfPresent(dr, CFSTR("Name"), (const void**)&name) &&
               CFDictionaryGetValueIfPresent(dr, CFSTR("Alias"), (const void**)&alias) &&
               name && alias &&
               CFStringGetTypeID() == CFGetTypeID(name) &&
               CFDataGetTypeID() == CFGetTypeID(alias) )
            {
              CFIndex dataSize = CFDataGetLength(alias);
              AliasHandle tAliasHdl = (AliasHandle) NewHandle( dataSize );
              if(tAliasHdl)
              {
                CFDataGetBytes( alias, CFRangeMake( 0, dataSize ), ( UInt8*) *tAliasHdl );
                FSRef tFSRef;
                Boolean changed;
                if(noErr == FSResolveAlias(NULL, tAliasHdl, &tFSRef, &changed))
                {
                  CFURLRef resolvedUrl = CFURLCreateFromFSRef(NULL, &tFSRef);
                  if(resolvedUrl)
                  {
                    url = CFURLCopyFileSystemPath(resolvedUrl, kCFURLPOSIXPathStyle);
                    CFRelease(resolvedUrl);
                  }
                }
                DisposeHandle((Handle)tAliasHdl);
              }
              // now put the name and path into a FileInfo Object
              CFIndex pathSize = CFStringGetLength(url)+1;
              vtkstd::vector<char> pathChars(pathSize, 0);
              OSStatus pathStatus = CFStringGetCString(url,&pathChars[0],pathSize, 
                                                        kCFStringEncodingASCII);

              pathSize = CFStringGetLength(name)+1;
              vtkstd::vector<char> nameChars(pathSize, 0);
              OSStatus nameStatus = CFStringGetCString(name, &nameChars[0], pathSize, 
                                                        kCFStringEncodingASCII);

              if (pathStatus && nameStatus)
              {
                vtkSmartPointer<vtkPVFileInformation> info = 
                                vtkSmartPointer<vtkPVFileInformation>::New();
                info->SetFullPath( &(pathChars.front() ) );
                info->SetName( &(nameChars.front() ) );
                info->Type = DIRECTORY;
                this->Contents->AddItem(info);
              }
            }
          }
        }
      }
    }
#else
  if(const char* home = getenv("HOME"))
    {
    vtkSmartPointer<vtkPVFileInformation> info =
      vtkSmartPointer<vtkPVFileInformation>::New();
    info->SetFullPath(home);
    info->SetName("Home");
    info->Type = DIRECTORY;
    this->Contents->AddItem(info);
    }
#endif
#endif // !_WIN32  
}

//-----------------------------------------------------------------------------
void vtkPVFileInformation::GetWindowsDirectoryListing()
{
#if defined(_WIN32)
  vtkPVFileInformationSet info_set;

  // Search for all files in the given directory.
  vtkstd::string prefix = this->FullPath;
  vtkPVFileInformationAddTerminatingSlash(prefix);
  vtkstd::string pattern = prefix;
  pattern += "*";
  WIN32_FIND_DATA data;
  HANDLE handle = FindFirstFile(pattern.c_str(), &data);
  if(handle == INVALID_HANDLE_VALUE)
    {
    LPVOID lpMsgBuf;
    FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
      GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR)&lpMsgBuf, 0, NULL);
    vtkErrorMacro("Error calling FindFirstFile : "
      << (char*)lpMsgBuf << "\nDirectory: " << prefix.c_str());
    LocalFree(lpMsgBuf);
    return;
    }

  do
    {
    vtkstd::string filename = data.cFileName;
    if(filename == "." || filename == "..")
      continue;
    vtkstd::string fullpath = prefix + filename;
    size_t len = filename.size();
    bool success = true;

    if(len > 4 && strncmp(filename.c_str()+len-4, ".lnk", 4) == 0)
      {
      success = vtkPVFileInformationResolveLink(fullpath, fullpath, data);
      }

    DWORD isdir = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    DWORD isfile = (data.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) ||
                  (!(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) &&
                   !(data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) &&
                   !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));

    FileTypes type = isdir ? DIRECTORY : SINGLE_FILE;

    if(success && (isdir || isfile))
      {
      vtkPVFileInformation* infoD = vtkPVFileInformation::New();
      infoD->SetName(filename.c_str());
      infoD->SetFullPath(fullpath.c_str());
      infoD->Type = type;
      infoD->FastFileTypeDetection = this->FastFileTypeDetection;
      info_set.insert(infoD);
      infoD->Delete();
      }

    // Find the next file.
    } while(FindNextFile(handle, &data) != 0);

  if(GetLastError() != ERROR_NO_MORE_FILES)
    {
    LPVOID lpMsgBuf;
    FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
      GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR)&lpMsgBuf, 0, NULL);
    vtkErrorMacro("Error calling FindNextFile : "
      << (char*)lpMsgBuf << "\nDirectory: " << prefix.c_str());
    LocalFree(lpMsgBuf);
    }

  if(!FindClose(handle))
    {
    vtkErrorMacro("Error calling FindClose.");
    }

  this->OrganizeCollection(info_set);

  for (vtkPVFileInformationSet::iterator iter = info_set.begin();
    iter != info_set.end(); ++iter)
    {
    this->Contents->AddItem(*iter);
    }

#else
  vtkErrorMacro("GetWindowsDirectoryListing cannot be called on non-Windows systems.");
#endif
}

/* There is a problem with the Portland compiler, large file
support and glibc/Linux system headers: 
             http://www.pgroup.com/userforum/viewtopic.php?
             p=1992&sid=f16167f51964f1a68fe5041b8eb213b6
*/
#if defined(__PGI) && defined(__USE_FILE_OFFSET64)
# define dirent dirent64
#endif

//-----------------------------------------------------------------------------
void vtkPVFileInformation::GetDirectoryListing()
{
#if defined(_WIN32)

  vtkErrorMacro("GetDirectoryListing() cannot be called on Windows systems.");
  return;

#else

  vtkPVFileInformationSet info_set;
  vtkstd::string prefix = this->FullPath;
  vtkPVFileInformationAddTerminatingSlash(prefix);

  // Open the directory and make sure it exists.
  DIR* dir = opendir(this->FullPath);
  if(!dir)
    {
    // Could add check of errno here.
    return;
    }

  // Loop through the directory listing.
  while(const dirent* d = readdir(dir))
    {
    // Skip the special directory entries.
    if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0)
      {
      continue;
      }
    vtkPVFileInformation* info = vtkPVFileInformation::New();
    info->SetName(d->d_name);
    info->SetFullPath((prefix + d->d_name).c_str());
    info->Type = INVALID;
    info->FastFileTypeDetection = this->FastFileTypeDetection;
    info_set.insert(info);
    info->Delete();
    }
  closedir(dir);

  this->OrganizeCollection(info_set);

  // Now we detect the file types for items.
  // We dissolve any groups that contain non-file items.
  
  for (vtkPVFileInformationSet::iterator iter = info_set.begin();
    iter != info_set.end(); ++iter)
    {
    vtkPVFileInformation* obj = (*iter);
    if (obj->DetectType())
      {
      this->Contents->AddItem(obj);
      }
    else
      {
      // Add children to contents.
      for (int cc=0; cc < obj->Contents->GetNumberOfItems(); cc++)
        {
        vtkPVFileInformation* child = vtkPVFileInformation::SafeDownCast(
          obj->Contents->GetItemAsObject(cc));
        if (child->DetectType())
          {
          this->Contents->AddItem(child);
          }
        }
      }
    }
#endif
}


//-----------------------------------------------------------------------------
bool vtkPVFileInformation::DetectType()
{
  if (this->Type == FILE_GROUP)
    {
    vtkSmartPointer<vtkCollectionIterator> iter;
    iter.TakeReference(this->Contents->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      vtkPVFileInformation* child = vtkPVFileInformation::SafeDownCast(
        iter->GetCurrentObject());
      if (!child->DetectType() || child->Type != SINGLE_FILE)
        {
        return false;
        }
      if (this->FastFileTypeDetection)
        {
        // Assume all children are same as this child.
        for (; !iter->IsDoneWithTraversal(); iter->GoToNextItem())
          {
          vtkPVFileInformation* child2 = vtkPVFileInformation::SafeDownCast(
            iter->GetCurrentObject());
          child2->Type = child->Type;
          }
        break;
        }
      }
    return true;
    }
  else if (this->Type == INVALID)
    {
    if (vtksys::SystemTools::FileExists(this->FullPath))
      {
      this-> Type = 
        (vtksys::SystemTools::FileIsDirectory(this->FullPath))?
        DIRECTORY : SINGLE_FILE;
      return true;
      }
    return false;
    }
  return true;
}

struct vtkPVFileInformation::vtkInfo
{
  typedef vtkstd::map<int, vtkSmartPointer<vtkPVFileInformation> > ChildrenType;
  vtkSmartPointer<vtkPVFileInformation> Group;
  ChildrenType Children;

};

//-----------------------------------------------------------------------------
void vtkPVFileInformation::OrganizeCollection(vtkPVFileInformationSet& info_set)
{
  typedef vtkstd::map<vtkstd::string, vtkInfo> MapOfStringToInfo;
  MapOfStringToInfo fileGroups;
  
  vtkstd::string prefix = this->FullPath;
  vtkPVFileInformationAddTerminatingSlash(prefix);

  // sequence ending with numbers.
  vtksys::RegularExpression reg_ex("^(.*)\\.([0-9.]+)$");
  // sequence ending with extension.
  vtksys::RegularExpression reg_ex2("^(.*)(\\.|_|-)([0-9.]+)\\.(.*)$");
  
  // sequence ending with extension, but with no ". or _" before
  // the series number.
  vtksys::RegularExpression reg_ex3("^(.*)([a-zA-Z])([0-9.]+)\\.(.*)$");
  // sequence ending with extension, and starting with series number
  // followed by ". or _".
  vtksys::RegularExpression reg_ex4("^([0-9.]+)(\\.|_|-)(.*)\\.(.*)$");
  // sequence ending with extension, and starting with series number,
  // but not followed by ". or _".
  vtksys::RegularExpression reg_ex5("^([0-9.]+)([a-zA-Z])(.*)\\.(.*)$");

  for (vtkPVFileInformationSet::iterator iter = info_set.begin();
    iter != info_set.end(); )
    {
    vtkPVFileInformation* obj = *iter;

    if (obj->Type != FILE_GROUP && obj->Type != DIRECTORY)
      {
      bool match = false;
      vtkstd::string groupName;
      int groupIndex = -1;
      if (reg_ex.find(obj->GetName()))
        {
        groupName = reg_ex.match(1);
        groupIndex = atoi(reg_ex.match(2).c_str());
        match = true;
        }
      else if (reg_ex2.find(obj->GetName()))
        {
        groupName = reg_ex2.match(1) + reg_ex2.match(2) + ".." + reg_ex2.match(4);
        groupIndex = atoi(reg_ex2.match(3).c_str());
        match = true;
        }
      else if (reg_ex3.find(obj->GetName()))
        {
        groupName = reg_ex3.match(1) + reg_ex3.match(2) + ".." + reg_ex3.match(4);
        groupIndex = atoi(reg_ex3.match(3).c_str());
        match = true;
        }
      else if (reg_ex4.find(obj->GetName()))
        {
        groupName = ".." + reg_ex4.match(2) + reg_ex4.match(3) + "." + reg_ex4.match(4);
        groupIndex = atoi(reg_ex4.match(1).c_str());
        match = true;
        }
      else if (reg_ex5.find(obj->GetName()))
        {
        groupName = ".." + reg_ex5.match(2) + reg_ex5.match(3) + "." + reg_ex5.match(4);
        groupIndex = atoi(reg_ex5.match(1).c_str());
        match = true;
        }

      if (match)
        {
        MapOfStringToInfo::iterator iter2 = fileGroups.find(groupName);
        vtkPVFileInformation* group = 0;
        if (iter2 == fileGroups.end())
          {
          group = vtkPVFileInformation::New();
          group->SetName(groupName.c_str());
          group->SetFullPath((prefix + groupName).c_str());
          group->Type = FILE_GROUP;
          group->FastFileTypeDetection = this->FastFileTypeDetection;
          //fileGroups[groupName] = group;
          vtkInfo info;
          info.Group = group;
          fileGroups[groupName] = info;
          group->Delete();

          iter2 = fileGroups.find(groupName);
          }

        iter2->second.Children[groupIndex] = obj;
        vtkPVFileInformationSet::iterator prev_iter = iter++;
        info_set.erase(prev_iter);
        continue;
        }
      }
    ++iter;
    }


  // Now scan through all created groups and dissolve trivial groups
  // i.e. groups with single entries. Add all other groups to the 
  // results.
 for (MapOfStringToInfo::iterator iter2 = fileGroups.begin();
   iter2 != fileGroups.end(); ++iter2)
   {
   vtkInfo& info = iter2->second;
   vtkPVFileInformation* group = info.Group;
   if (info.Children.size() > 1)
     {
     vtkInfo::ChildrenType::iterator childIter = info.Children.begin();
     for (; childIter != info.Children.end();++childIter)
       {
       group->Contents->AddItem(childIter->second.GetPointer());
       }
     // Build group children.
     info_set.insert(group);
     }
   else
     {
     vtkInfo::ChildrenType::iterator childIter = info.Children.begin();
     for (; childIter != info.Children.end();++childIter)
       {
       info_set.insert(
         childIter->second.GetPointer());
       }
     }
   }
}

//-----------------------------------------------------------------------------
void vtkPVFileInformation::CopyToStream(vtkClientServerStream* stream)
{
  *stream << vtkClientServerStream::Reply
    << this->Name
    << this->FullPath
    << this->Type
    << this->Contents->GetNumberOfItems();
  
  vtkSmartPointer<vtkCollectionIterator> iter;
  iter.TakeReference(this->Contents->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkClientServerStream childStream;
    vtkPVFileInformation* child = vtkPVFileInformation::SafeDownCast(
      iter->GetCurrentObject());
    child->CopyToStream(&childStream);
    *stream << childStream;
    }
  *stream << vtkClientServerStream::End;
}

//-----------------------------------------------------------------------------
void vtkPVFileInformation::CopyFromStream(const vtkClientServerStream* css)
{
  this->Initialize();
  const char* temp = 0;
  if (!css->GetArgument(0, 0, &temp))
    {
    vtkErrorMacro("Error parsing Name.");
    return;
    }
  this->SetName(temp);

  if (!css->GetArgument(0, 1, &temp))
    {
    vtkErrorMacro("Error parsing FullPath.");
    return;
    }
  this->SetFullPath(temp);

  if (!css->GetArgument(0, 2, &this->Type))
    {
    vtkErrorMacro("Error parsing Type.");
    return;
    }

  int num_of_children =0;
  if (!css->GetArgument(0, 3, &num_of_children))
    {
    vtkErrorMacro("Error parsing Number of children.");
    return;
    }
  for (int cc=0; cc < num_of_children; cc++)
    {
    vtkPVFileInformation* child = vtkPVFileInformation::New();
    vtkClientServerStream childStream;
    if (!css->GetArgument(0, 4+cc, &childStream))
      {
      vtkErrorMacro("Error parsing child #" << cc);
      return;
      }
    child->CopyFromStream(&childStream);
    this->Contents->AddItem(child);
    child->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkPVFileInformation::Initialize()
{
  this->SetName(0);
  this->SetFullPath(0);
  this->Type = INVALID;
  this->Contents->RemoveAllItems();
}

//-----------------------------------------------------------------------------
void vtkPVFileInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Name: " << (this->Name? this->Name: "(null)") << endl;
  os << indent << "FullPath: " 
    << (this->FullPath? this->FullPath : "(null)") << endl;
  os << indent << "Type: " ;
  switch (this->Type)
    {
  case INVALID:
    os << "INVALID" << endl;
    break;

  case SINGLE_FILE:
    os << "SINGLE_FILE" << endl;
    break;

  case DIRECTORY:
    os << "DIRECTORY" << endl;
    break;

  case FILE_GROUP:
    os << "FILE_GROUP" << endl;
    }
  os << indent << "FastFileTypeDetection: " << this->FastFileTypeDetection << endl;

  for (int cc=0; cc < this->Contents->GetNumberOfItems(); cc++)
    {
    os << endl;
    this->Contents->GetItemAsObject(cc)->PrintSelf(os, indent.GetNextIndent());
    }
}
