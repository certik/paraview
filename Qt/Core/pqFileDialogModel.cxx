/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqFileDialogModel.cxx,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqFileDialogModel.h"

#include <vtkstd/algorithm>

#include <QFileIconProvider>
#include <QStyle>
#include <QDir>
#include <QApplication>

#include <pqServer.h>
#include <vtkClientServerStream.h>
#include <vtkCollection.h>
#include <vtkCollectionIterator.h>
#include <vtkProcessModule.h>
#include <vtkPVFileInformation.h>
#include <vtkPVFileInformationHelper.h>
#include <vtkSmartPointer.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkStringList.h>

#include "pqSMAdaptor.h"

//////////////////////////////////////////////////////////////////////
// pqFileDialogModelFileInfo

class pqFileDialogModelFileInfo
{
public:
  pqFileDialogModelFileInfo()
  {
  }

  pqFileDialogModelFileInfo(const QString& l, const QString& filepath, 
           const bool isdir, const QList<pqFileDialogModelFileInfo>& g =
           QList<pqFileDialogModelFileInfo>()) :
    Label(l),
    FilePath(filepath),
    IsDir(isdir),
    IsLink(false),
    Group(g)
  {
  if(this->Label.endsWith(".lnk"))
    {
    this->IsLink = true;
    this->Label.chop(4);
    }
  }

  const QString& label() const
  {
    return this->Label;
  }

  const QString& filePath() const 
  {
    return this->FilePath;
  }
  
  bool isDir() const
  {
    return this->IsDir;
  }
  
  bool isLink() const
  {
    return this->IsLink;
  }

  bool isGroup() const
  {
    return !this->Group.empty();
  }

  const QList<pqFileDialogModelFileInfo>& group() const
  {
    return this->Group;
  }

private:
  QString Label;
  QString FilePath;
  bool IsDir;
  bool IsLink;
  QList<pqFileDialogModelFileInfo> Group;
};

/////////////////////////////////////////////////////////////////////
// Icons

class pqFileDialogModelIconProvider : protected QFileIconProvider
{
public:
  enum IconType { Drive, Folder, File, FolderLink, FileLink };
  pqFileDialogModelIconProvider()
    {
    QStyle* style = QApplication::style();
    this->FolderLinkIcon = style->standardIcon(QStyle::SP_DirLinkIcon);
    this->FileLinkIcon = style->standardIcon(QStyle::SP_FileLinkIcon);
    }

  QIcon icon(IconType t) const
    {
    switch(t)
      {
      case Drive:
        return QFileIconProvider::icon(QFileIconProvider::Drive);
      case Folder:
        return QFileIconProvider::icon(QFileIconProvider::Folder);
      case File:
        return QFileIconProvider::icon(QFileIconProvider::File);
      case FolderLink:
        return this->FolderLinkIcon;
      case FileLink:
        return this->FileLinkIcon;
      }
    return QIcon();
    }

  QIcon icon(const pqFileDialogModelFileInfo& info) const
    {
    if(info.isDir())
      return icon(info.isLink() ? FolderLink : Folder);
    return icon(info.isLink() ? FileLink : File);
    }
  QIcon icon(const QFileInfo& info) const
    {
    return QFileIconProvider::icon(info);
    }
  QIcon icon(QFileIconProvider::IconType ico) const
    {
    return QFileIconProvider::icon(ico);
    }

protected:
  QIcon FolderLinkIcon;
  QIcon FileLinkIcon;
};

Q_GLOBAL_STATIC(pqFileDialogModelIconProvider, Icons);

namespace {

///////////////////////////////////////////////////////////////////////
// CaseInsensitiveSort

bool CaseInsensitiveSort(const pqFileDialogModelFileInfo& A, const
  pqFileDialogModelFileInfo& B)
{
  // Sort alphabetically (but case-insensitively)
  return A.label().toLower() < B.label().toLower();
}

class CaseInsensitiveSortGroup 
  : public vtkstd::binary_function<pqFileDialogModelFileInfo, pqFileDialogModelFileInfo, bool>
{
public:
  CaseInsensitiveSortGroup(const QString& groupName)
    {
    this->numPrefix = groupName.length();
    }
  bool operator()(const pqFileDialogModelFileInfo& A, const pqFileDialogModelFileInfo& B) const
    {
    QString aLabel = A.label();
    QString bLabel = B.label();
    aLabel = aLabel.right(aLabel.length() - numPrefix);
    bLabel = bLabel.right(bLabel.length() - numPrefix);
    return aLabel < bLabel;
    }
private:
  int numPrefix;
};

} // namespace

/////////////////////////////////////////////////////////////////////////
// pqFileDialogModel::Implementation

class pqFileDialogModel::pqImplementation
{
public:
  pqImplementation(pqServer* server) :
    Separator(0),
    Server(server)
  {
  
    // if we are doing remote browsing
    if(server)
      {
      vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();

      vtkSMProxy* helper = pxm->NewProxy("misc", "FileInformationHelper");
      this->FileInformationHelperProxy = helper;
      helper->SetConnectionID(server->GetConnectionID());
      helper->SetServers(vtkProcessModule::DATA_SERVER_ROOT);
      helper->Delete();
      helper->UpdateVTKObjects();
      helper->UpdatePropertyInformation();
      QString separator = pqSMAdaptor::getElementProperty(
        helper->GetProperty("PathSeparator")).toString();
      this->Separator = separator.toAscii().data()[0];
      }
    else
      {
      vtkPVFileInformationHelper* helper = vtkPVFileInformationHelper::New();
      this->FileInformationHelper = helper;
      helper->Delete();
      this->Separator = helper->GetPathSeparator()[0];
      }

    this->FileInformation.TakeReference(vtkPVFileInformation::New());

    // current path
    vtkPVFileInformation* info = this->GetData(false, ".", false);
    this->CurrentPath = info->GetFullPath();
  }
  
  ~pqImplementation()
  {
  }

  /// Removes multiple-slashes, ".", and ".." from the given path string,
  /// and points slashes in the correct direction for the server
  const QString cleanPath(const QString& Path)
  {
    QString result = QDir::cleanPath(Path);
    result.replace('/', this->Separator);
    return result.trimmed();
  }

  /// query the file system for information
  vtkPVFileInformation* GetData(bool dirListing, 
                                const QString& path,
                                bool specialDirs)
    {
    return this->GetData(dirListing, "", path, specialDirs);
    }

  /// query the file system for information
  vtkPVFileInformation* GetData(bool dirListing, 
                                const QString& workingDir,
                                const QString& path,
                                bool specialDirs)
    {
    if(this->FileInformationHelperProxy)
      {
      // send data to server
      vtkSMProxy* helper = this->FileInformationHelperProxy;
      pqSMAdaptor::setElementProperty(
        helper->GetProperty("WorkingDirectory"), workingDir);
      pqSMAdaptor::setElementProperty(
        helper->GetProperty("DirectoryListing"), dirListing);
      pqSMAdaptor::setElementProperty(
        helper->GetProperty("Path"), path.toAscii().data());
      pqSMAdaptor::setElementProperty(
        helper->GetProperty("SpecialDirectories"), specialDirs);
      helper->UpdateVTKObjects();
      
      // get data from server
      this->FileInformation->Initialize();
      vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
      pm->GatherInformation(this->FileInformationHelperProxy->GetConnectionID(),
        vtkProcessModule::DATA_SERVER, 
        this->FileInformation, 
        this->FileInformationHelperProxy->GetID());
      }
    else
      {
      vtkPVFileInformationHelper* helper = this->FileInformationHelper;
      helper->SetDirectoryListing(dirListing);
      helper->SetPath(path.toAscii().data());
      helper->SetSpecialDirectories(specialDirs);
      helper->SetWorkingDirectory(workingDir.toAscii().data());
      this->FileInformation->CopyFromObject(helper);
      }
    return this->FileInformation;
    }

  /// put queried information into our model
  void Update(const QString& path, vtkPVFileInformation* dir)
    {
    this->CurrentPath = path;
    this->FileList.clear();

    QList<pqFileDialogModelFileInfo> dirs;
    QList<pqFileDialogModelFileInfo> files;

    vtkSmartPointer<vtkCollectionIterator> iter;
    iter.TakeReference(dir->GetContents()->NewIterator());

    for (iter->InitTraversal(); 
         !iter->IsDoneWithTraversal(); 
         iter->GoToNextItem())
      {
      vtkPVFileInformation* info = vtkPVFileInformation::SafeDownCast(
        iter->GetCurrentObject());
      if (!info)
        {
        continue;
        }
      if (info->GetType() == vtkPVFileInformation::DIRECTORY)
        {
        dirs.push_back(pqFileDialogModelFileInfo(info->GetName(), info->GetFullPath(), true));
        }
      else if (info->GetType() == vtkPVFileInformation::SINGLE_FILE)
        {
        files.push_back(pqFileDialogModelFileInfo(info->GetName(), info->GetFullPath(), false));
        }
      else if (info->GetType() == vtkPVFileInformation::FILE_GROUP)
        {
        QList<pqFileDialogModelFileInfo> groupFiles;
        vtkSmartPointer<vtkCollectionIterator> childIter;
        childIter.TakeReference(info->GetContents()->NewIterator());
        for (childIter->InitTraversal(); !childIter->IsDoneWithTraversal();
          childIter->GoToNextItem())
          {
          vtkPVFileInformation* child = vtkPVFileInformation::SafeDownCast(
            childIter->GetCurrentObject());
          groupFiles.push_back(pqFileDialogModelFileInfo(child->GetName(), child->GetFullPath(),
                                   false));
          }
        files.push_back(pqFileDialogModelFileInfo(info->GetName(), groupFiles[0].filePath(), false,
            groupFiles));
        }
      }

    qSort(dirs.begin(), dirs.end(), CaseInsensitiveSort);
    qSort(files.begin(), files.end(), CaseInsensitiveSort);

    for(int i = 0; i != dirs.size(); ++i)
      {
      this->FileList.push_back(dirs[i]);
      }
    for(int i = 0; i != files.size(); ++i)
      {
      this->FileList.push_back(files[i]);
      }
    }

  QStringList getFilePaths(const QModelIndex& Index)
    {
    QStringList results;

    QModelIndex p = Index.parent();
    if(p.isValid())
      {
      if(p.row() < this->FileList.size())
        {
        pqFileDialogModelFileInfo& file = this->FileList[p.row()];
        const QList<pqFileDialogModelFileInfo>& grp = file.group();
        if(Index.row() < grp.size())
          {
          results.push_back(grp[Index.row()].filePath());
          }
        }
      }
    else if(Index.row() < this->FileList.size())
      { 
      pqFileDialogModelFileInfo& file = this->FileList[Index.row()];
      if (file.isGroup() && file.group().count()>0)
        {
        for (int i=0; i<file.group().count();i++)
          {
          results.push_back(file.group().at(i).filePath());
          }
        }
      else
        {
        results.push_back(file.filePath());
        }
      }

    return results;
    }

  bool isDir(const QModelIndex& Index)
    {
    if(Index.row() >= this->FileList.size())
      return false;

    pqFileDialogModelFileInfo& file = this->FileList[Index.row()];
    return file.isDir();
    }

  bool isRemote()
    {
    return !!this->Server;
    }

  pqServer* getServer()
    {
    return this->Server;
    }

  /// Path separator for the connected server's filesystem. 
  char Separator;

  /// Current path being displayed (server's filesystem).
  QString CurrentPath;
  /// Caches information about the set of files within the current path.
  QVector<pqFileDialogModelFileInfo> FileList;  // adjacent memory occupation for QModelIndex

private:   
  // server vs. local implementation private
  pqServer* Server;
  
  vtkSmartPointer<vtkPVFileInformationHelper> FileInformationHelper;
  vtkSmartPointer<vtkSMProxy> FileInformationHelperProxy;
  vtkSmartPointer<vtkPVFileInformation> FileInformation;
};
  
//////////////////////////////////////////////////////////////////////////
// pqFileDialogModel
pqFileDialogModel::pqFileDialogModel(pqServer* _server, QObject* Parent) :
  base(Parent),
  Implementation(new pqImplementation(_server))
{
}

pqFileDialogModel::~pqFileDialogModel()
{
  delete this->Implementation;
}

pqServer* pqFileDialogModel::server() const
{
  return this->Implementation->getServer();
}

void pqFileDialogModel::setCurrentPath(const QString& Path)
{
  QString cPath = this->Implementation->cleanPath(Path);
  vtkPVFileInformation* info;
  info = this->Implementation->GetData(true, cPath, false);
  this->Implementation->Update(cPath, info);
  this->reset();
}

QString pqFileDialogModel::getCurrentPath()
{
  return this->Implementation->CurrentPath;
}

QString pqFileDialogModel::absoluteFilePath(const QString& path)
{
  if(path.isEmpty())
    {
    return QString();
    }

  vtkPVFileInformation* info;
  info = this->Implementation->GetData(false,
    this->getCurrentPath(), path, false);
  return this->Implementation->cleanPath(info->GetFullPath());
}

QStringList pqFileDialogModel::getFilePaths(const QModelIndex& Index)
{
  if(Index.model() == this)
    {
    return this->Implementation->getFilePaths(Index);
    }
  return QStringList();
}

bool pqFileDialogModel::isDir(const QModelIndex& Index)
{
  if(Index.model() == this)
    return this->Implementation->isDir(Index);
  
  return false;    
}

bool pqFileDialogModel::fileExists(const QString& file, QString& fullpath)
{
  QString FilePath = this->Implementation->cleanPath(file);
  vtkPVFileInformation* info;
  info = this->Implementation->GetData(false, FilePath, false);
  
  // try again for shortcut
  if(info->GetType() != vtkPVFileInformation::SINGLE_FILE)
    {
    info = this->Implementation->GetData(false, FilePath + ".lnk", false);
    }

  if(info->GetType() == vtkPVFileInformation::SINGLE_FILE)
    {
    fullpath = info->GetFullPath();
    return true;
    }
  return false;
}

bool pqFileDialogModel::makeDirEntry(const QString& dirName)
{
  QString path;
  QString dirPath = this->absoluteFilePath(dirName);
  if(this->dirExists(dirPath,path))
    {
    return false;
    }

  this->Implementation->FileList.push_back(pqFileDialogModelFileInfo(dirName, dirPath, true));
  this->reset();

  return true;
}

bool pqFileDialogModel::removeDirEntry(const QString& dirName)
{
  QString dirPath = this->absoluteFilePath(dirName);
  int i=0;
  for(i=0; i<this->Implementation->FileList.size(); i++)
    {
    if(this->Implementation->FileList[i].isDir() && this->Implementation->FileList[i].filePath()==dirPath)
      {
      break;
      }
    }
  
  if(i<this->Implementation->FileList.size())
    {
    this->Implementation->FileList.remove(i);
    this->reset();
    }

  return true;
}

bool pqFileDialogModel::makeDir(const QString& dirName)
{
  QString path;
  QString dirPath = this->absoluteFilePath(dirName);
  if(this->dirExists(dirPath,path))
    {
    return false;
    }

  if(this->Implementation->getServer())
    {
    vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    vtkClientServerID dirID = pm->NewStreamObject("vtkDirectory", stream);
    stream << vtkClientServerStream::Invoke
          << dirID << "MakeDirectory" << dirPath.toAscii().data()
          << vtkClientServerStream::End;
    pm->SendStream(
      this->Implementation->getServer()->GetConnectionID(),
      vtkProcessModule::DATA_SERVER, stream);
    pm->DeleteStreamObject(dirID, stream);
    pm->SendStream(
      this->Implementation->getServer()->GetConnectionID(),
      vtkProcessModule::DATA_SERVER, stream);
    }
  else
    {
    QDir dir(this->getCurrentPath());
    if(!dir.mkdir(dirName))
      {
      return false;
      }
    }

  QString cPath = this->Implementation->cleanPath(this->getCurrentPath());
  vtkPVFileInformation* info;
  info = this->Implementation->GetData(true, cPath, false);
  this->Implementation->Update(cPath, info);

  this->reset();

  return true;
}

bool pqFileDialogModel::dirExists(const QString& path, QString& fullpath)
{
  QString dir = this->Implementation->cleanPath(path);
  vtkPVFileInformation* info;
  info = this->Implementation->GetData(false, dir, false);
  
  // try again for shortcuts
  if(info->GetType() != vtkPVFileInformation::DIRECTORY &&
     info->GetType() != vtkPVFileInformation::DRIVE)
    {
    info = this->Implementation->GetData(false, dir + ".lnk", false);
    }

  if(info->GetType() == vtkPVFileInformation::DIRECTORY ||
     info->GetType() == vtkPVFileInformation::DRIVE)
    {
    fullpath = info->GetFullPath();
    return true;
    }
  return false;
}

QChar pqFileDialogModel::separator() const
{
  return this->Implementation->Separator;
}

int pqFileDialogModel::columnCount(const QModelIndex& /*idx*/) const
{
  return 1;
}

QVariant pqFileDialogModel::data(const QModelIndex & idx, int role) const
{
  const pqFileDialogModelFileInfo* file = NULL;

  if(idx.isValid() && 
     NULL == idx.internalPointer() &&
     idx.row() >= 0 && 
     idx.row() < this->Implementation->FileList.size())
    {
    file = &this->Implementation->FileList[idx.row()];
    }
  else if(idx.isValid() && idx.internalPointer())
    {
    pqFileDialogModelFileInfo* ptr = reinterpret_cast<pqFileDialogModelFileInfo*>(idx.internalPointer());
    const QList<pqFileDialogModelFileInfo>& grp = ptr->group();
    if(idx.row() >= 0 && idx.row() < grp.size())
      {
      file = &grp[idx.row()];
      }
    }

  if(!file)
    {
    // should never get here anyway
    return QVariant();
    }

  if(role == Qt::DisplayRole && idx.column() == 0)
    {
    return file->label();
    }
  else if(role == Qt::DecorationRole && idx.column() == 0)
    {
    return Icons()->icon(*file);
    }

  return QVariant();
}

QModelIndex pqFileDialogModel::index(int row, int column, 
                                     const QModelIndex& p) const
{
  if(!p.isValid())
    {
    return this->createIndex(row, column);
    }
  if(p.row() >= 0 && 
     p.row() < this->Implementation->FileList.size() &&
     NULL == p.internalPointer())
    {
    pqFileDialogModelFileInfo* fi = &this->Implementation->FileList[p.row()];
    return this->createIndex(row, column, fi);
    }

  return QModelIndex();
}

QModelIndex pqFileDialogModel::parent(const QModelIndex& idx) const
{
  if(!idx.isValid() || !idx.internalPointer())
    {
    return QModelIndex();
    }
  
  const pqFileDialogModelFileInfo* ptr = reinterpret_cast<pqFileDialogModelFileInfo*>(idx.internalPointer());
  int row = ptr - &this->Implementation->FileList.first();
  return this->createIndex(row, idx.column());
}

int pqFileDialogModel::rowCount(const QModelIndex& idx) const
{
  if(!idx.isValid())
    {
    return this->Implementation->FileList.size();
    }
  
  if(NULL == idx.internalPointer() &&
     idx.row() >= 0 && 
     idx.row() < this->Implementation->FileList.size())
    {
    return this->Implementation->FileList[idx.row()].group().size();
    }

  return 0;
}

bool pqFileDialogModel::hasChildren(const QModelIndex& idx) const
{
  if(!idx.isValid())
    return true;

  if(NULL == idx.internalPointer() && 
     idx.row() >= 0 && 
     idx.row() < this->Implementation->FileList.size())
    {
    return this->Implementation->FileList[idx.row()].isGroup();
    }

  return false;
}

QVariant pqFileDialogModel::headerData(int section, 
                                       Qt::Orientation, int role) const
{
  switch(role)
    {
  case Qt::DisplayRole:
    switch(section)
      {
    case 0:
      return tr("Filename");
      }
    }

  return QVariant();
}

