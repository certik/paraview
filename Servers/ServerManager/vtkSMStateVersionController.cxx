/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMStateVersionController.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStateVersionController.h"

#include "vtkCollection.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"

#include "vtksys/ios/sstream"

vtkStandardNewMacro(vtkSMStateVersionController);
vtkCxxRevisionMacro(vtkSMStateVersionController, "$Revision: 1.11 $");
//----------------------------------------------------------------------------
vtkSMStateVersionController::vtkSMStateVersionController()
{
}

//----------------------------------------------------------------------------
vtkSMStateVersionController::~vtkSMStateVersionController()
{
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::Process(vtkPVXMLElement* root)
{
  if (!root || strcmp(root->GetName(), "ServerManagerState") != 0)
    {
    vtkErrorMacro("Invalid root element. Expected \"ServerManagerState\"");
    return false;
    }

  int version[3] = {0, 0, 0};
  bool status = true;
  this->ReadVersion(root, version);

  if (this->GetMajor(version) < 3)
    {
    vtkWarningMacro("State file version is less than 3.0.0, "
      "these states may not work correctly.");

    int updated_version[3] = {3, 0, 0};
    this->UpdateVersion(version, updated_version);
    }

  if (this->GetMajor(version)==3 && this->GetMinor(version)==0)
    {
    if (this->GetPatch(version) < 2)
      {
      vtkWarningMacro("Due to fundamental changes in the parallel rendering framework "
        "it is not possible to load states with volume rendering correctly "
        "for versions less than 3.0.2.");
      }
    status = status && this->Process_3_0_To_3_1(root) ;

    // Since now the state file has been update to version 3.1.0, we must update
    // the version number to reflect that.
    int updated_version[3] = {3, 1, 0};
    this->UpdateVersion(version, updated_version);
    }

  return true;
}

//----------------------------------------------------------------------------
bool ConvertViewModulesToViews(
  vtkPVXMLElement* root, void* callData)
{
  vtkSMStateVersionController* self = reinterpret_cast<
    vtkSMStateVersionController*>(callData);
  return self->ConvertViewModulesToViews(root);
}

//----------------------------------------------------------------------------
bool ConvertLegacyReader(
  vtkPVXMLElement* root, void* callData)
{
  vtkSMStateVersionController* self = reinterpret_cast<
    vtkSMStateVersionController*>(callData);
  return self->ConvertLegacyReader(root);
}

//----------------------------------------------------------------------------
// Called for every data-object display. We will update scalar color 
// properties since those changed.
bool ConvertDataDisplaysToRepresentations(vtkPVXMLElement* root,
  void* vtkNotUsed(callData))
{
  // Change proxy type based on the presence of "VolumePipelineType" hint.
  vtkPVXMLElement* typeHint = root->FindNestedElementByName("VolumePipelineType");
  const char* type = "GeometryRepresentation";
  if (typeHint)
    {
    const char* hinttype = typeHint->GetAttribute("type");
    if (hinttype)
      {
      if (strcmp(hinttype, "IMAGE_DATA")==0)
        {
        type = "UniformGridRepresentation";
        }
      else if (strcmp(hinttype, "UNSTRUCTURED_GRID")==0)
        {
        type ="UnstructuredGridRepresentation";
        }
      }
    }
  root->SetAttribute("type", type);
  root->SetAttribute("group", "representations");

  // ScalarMode --> ColorAttributeType
  //  vals: 0/1/2/3 ---> 0 (point-data)
  //      : 4       ---> 1 (cell-data)
  // ColorArray --> ColorArrayName
  //  val remains same.
  unsigned int max = root->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < max; cc++)
    {
    vtkPVXMLElement* child = root->GetNestedElement(cc);
    if (child->GetName() && strcmp(child->GetName(), "Property")==0)
      {
      const char* pname = child->GetAttribute("name");
      if (pname && strcmp(pname, "ColorArray")==0)
        {
        child->SetAttribute("name", "ColorArrayName");
        }
      else if (pname && strcmp(pname, "ScalarMode")==0)
        {
        child->SetAttribute("name", "ColorAttributeType");
        vtkPVXMLElement* valueElement = 
          child->FindNestedElementByName("Element");
        if (valueElement)
          {
          int oldValue = 0;
          valueElement->GetScalarAttribute("value", &oldValue);
          int newValue = (oldValue<=3)? 0 : 1;
          vtksys_ios::ostringstream valueStr;
          valueStr << newValue << ends;
          valueElement->SetAttribute("value", valueStr.str().c_str());
          }
        }
      }
    }

  return true;
}

//----------------------------------------------------------------------------
// Called for every XYPlotDisplay2. The point and cell array status
// arrays changed order and added elements.
bool ConvertLineSeriesArrayStatus(vtkPVXMLElement* root,
  void* vtkNotUsed(callData))
{
  // If there are less than 5 child elements, there is nothing to change.
  unsigned int max = root->GetNumberOfNestedElements();
  for(unsigned int i = 0; i < max; i++)
    {
    vtkPVXMLElement *child = root->GetNestedElement(i);
    const char *name = child ? child->GetName() : 0;
    if(!name || strcmp(name, "Property") != 0)
      {
      continue;
      }

    name = child->GetAttribute("name");
    if(name && (strcmp(name, "YCellArrayStatus") == 0 ||
        strcmp(name, "YPointArrayStatus") == 0))
      {
      unsigned int total = child->GetNumberOfNestedElements();
      if(total <= 1)
        {
        continue;
        }

      // Find the domain element. It should be last.
      vtkSmartPointer<vtkPVXMLElement> domainElement =
          child->GetNestedElement(total - 1);
      name = domainElement ? domainElement->GetName() : 0;
      if(!name || strcmp(name, "Domain") != 0)
        {
        continue;
        }

      // Remove the domain element from the list.
      child->RemoveNestedElement(domainElement);

      // Add elements for the new array entries.
      total -= 1;
      unsigned int newTotal = total * 2; //(total / 5) * 10;
      vtkPVXMLElement *elem = 0;
      for(unsigned int index = total; index < newTotal; index++)
        {
        elem = vtkPVXMLElement::New();
        elem->SetName("Element");
        elem->AddAttribute("index", index);
        elem->AddAttribute("value", "");
        child->AddNestedElement(elem);
        elem->Delete();
        }

      // Add the domain element back in.
      child->AddNestedElement(domainElement);
      domainElement = 0;

      // Move original data at old index to new index:
      // 0 --> 4
      // 1 --> 5
      // 2 --> 6
      // 3 --> 2
      // 4 --> 0
      // 4 --> 1
      //
      // Fill in the rest with new data:
      // inlegend(new index 3) = 1 (int)
      // thickness(new index 7) = 1 (int)
      // linestyle(new index 8) = 1 (int)
      // axesindex(new index 9) = 0 (int)
      //
      // Start at the end where there is space.
      int j = total - 5;
      int k = newTotal - 10;
      vtkPVXMLElement *elem2 = 0;
      for(; j >= 0 && k >= 0; j -= 5, k -= 10)
        {
        // Fill in the new options.
        elem = child->GetNestedElement(k + 7);
        elem->SetAttribute("value", "1");
        elem = child->GetNestedElement(k + 8);
        elem->SetAttribute("value", "1");
        elem = child->GetNestedElement(k + 9);
        elem->SetAttribute("value", "0");

        // Move the green and blue from 1,2 to 5,6.
        elem = child->GetNestedElement(j + 1);
        elem2 = child->GetNestedElement(k + 5);
        elem2->SetAttribute("value", elem->GetAttribute("value"));
        elem = child->GetNestedElement(j + 2);
        elem2 = child->GetNestedElement(k + 6);
        elem2->SetAttribute("value", elem->GetAttribute("value"));

        // Move the array/legend name to from 4 to 1.
        elem = child->GetNestedElement(j + 4);
        elem2 = child->GetNestedElement(k + 1);
        elem2->SetAttribute("value", elem->GetAttribute("value"));

        // Move the red from 0 to 4.
        elem = child->GetNestedElement(j + 0);
        elem2 = child->GetNestedElement(k + 4);
        elem2->SetAttribute("value", elem->GetAttribute("value"));

        // Copy the array/legend name to 0.
        elem = child->GetNestedElement(k + 1);
        elem2 = child->GetNestedElement(k + 0);
        elem2->SetAttribute("value", elem->GetAttribute("value"));

        // Copy the enabled flag from 3 to 2.
        elem = child->GetNestedElement(j + 3);
        elem2 = child->GetNestedElement(k + 2);
        elem2->SetAttribute("value", elem->GetAttribute("value"));

        // Set the in legend flag to true.
        elem = child->GetNestedElement(k + 3);
        elem->SetAttribute("value", "1");
        }
      }
    }

  return true;
}

//----------------------------------------------------------------------------
bool ConvertPVAnimationSceneToAnimationScene(
  vtkPVXMLElement* root, void* callData)
{
  vtkSMStateVersionController* self = reinterpret_cast<
    vtkSMStateVersionController*>(callData);
  return self->ConvertPVAnimationSceneToAnimationScene(root);
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::Process_3_0_To_3_1(vtkPVXMLElement* root)
{
    {
    // Remove all MultiViewRenderModule elements.
    const char* attrs[] = {
      "group","rendermodules",
      "type", "MultiViewRenderModule", 0};
    this->SelectAndRemove( root, "Proxy",attrs);
    }

    {
    // Replace all requests for proxies in the group "rendermodules" by
    // "views".
    const char* attrs[] = {
      "group", "rendermodules", 0};
    const char* newAttrs[] = {
      "group", "views",
      "type", "RenderView", 0};
    this->Select( root, "Proxy", attrs,
      &::ConvertViewModulesToViews,
      this);
    this->SelectAndSetAttributes(root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert different kinds of old-views to new-views.
    const char* attrs[] = {
      "group", "plotmodules",
      "type", "BarChartViewModule", 0};
    const char* newAttrs[] = {
      "group", "views",
      "type", "BarChartView", 0};
    this->Select( root, "Proxy", attrs,
      &::ConvertViewModulesToViews,
      this);
    this->SelectAndSetAttributes(root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert different kinds of old-views to new-views.
    const char* attrs[] = {
      "group", "plotmodules",
      "type", "XYPlotViewModule", 0};
    const char* newAttrs[] = {
      "group", "views",
      "type", "XYPlotView", 0};
    this->Select( root, "Proxy", attrs,
      &::ConvertViewModulesToViews,
      this);
    this->SelectAndSetAttributes(root, "Proxy", attrs, newAttrs);
    }

    {
    // Remove element inspector view.
    const char* attrs[] = {
      "group", "views",
      "type", "ElementInspectorView", 0};
    const char* newAttrs[] = {
      "group", "views",
      "type", "ElementInspectorView", 0};
    this->SelectAndRemove(root, "Proxy", attrs);
    this->SelectAndRemove(root, "Proxy", newAttrs);
    }

    {
    // Convert all geometry displays to representations.
    const char* lodAttrs[] = {
      "type", "LODDisplay", 0};
    const char* compositeAttrs[] = {
      "type", "CompositeDisplay", 0};
    const char* multiAttrs[] = {
      "type", "MultiDisplay", 0};

    this->Select(root, "Proxy", lodAttrs,
      &ConvertDataDisplaysToRepresentations, this);
    this->Select(root, "Proxy", compositeAttrs,
      &ConvertDataDisplaysToRepresentations, this);
    this->Select(root, "Proxy", multiAttrs,
      &ConvertDataDisplaysToRepresentations, this);
    }

    {
    // Convert registration groups from "displays" to
    // "representations"
    const char* attrs[] = {
      "name", "displays", 0};
    const char* newAttrs[] = {
      "name", "representations", 0};
    this->SelectAndSetAttributes(root, "ProxyCollection", attrs, newAttrs);
    }

    {
    // Convert registration groups from "view_modules" to
    // "views"
    const char* attrs[] = {
      "name", "view_modules", 0};
    const char* newAttrs[] = {
      "name", "views", 0};
    this->SelectAndSetAttributes(root, "ProxyCollection", attrs, newAttrs);
    }

    {
    // Convert all text widget displays to representations.
    const char* attrs[] = {
      "type", "TextWidgetDisplay", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "TextWidgetRepresentation", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert all GenericViewDisplay displays to representations.
    const char* attrs[] = {
      "type", "GenericViewDisplay", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "ClientDeliveryRepresentation", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert all displays to representations.
    const char* attrs[] = {
      "type", "BarChartDisplay", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "BarChartRepresentation", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert all displays to representations.
    const char* attrs[] = {
      "type", "XYPlotDisplay2", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "XYPlotRepresentation", 0};
    this->Select( root, "Proxy", attrs,
      &::ConvertLineSeriesArrayStatus, this);
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert all displays to representations.
    const char* attrs[] = {
      "type", "ScalarBarWidget", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "ScalarBarWidgetRepresentation", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Remove element inspector representations. 
    const char* attrs[] = {
      "type", "ElementInspectorDisplay", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "ElementInspectorRepresentation", 0};
    this->SelectAndRemove(root, "Proxy", attrs);
    this->SelectAndRemove(root, "Proxy", newAttrs);
    }

    {
    // Convert Axes to AxesRepresentation.
    const char* attrs[] = {
      "group", "axes", 
      "type", "Axes", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "AxesRepresentation", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Delete all obsolete display types.
    const char* attrsCAD[] = {
      "type", "CubeAxesDisplay",0};
    const char* attrsPLD[] = {
      "type", "PointLabelDisplay",0};

    this->SelectAndRemove(root, "Proxy", attrsCAD);
    this->SelectAndRemove(root, "Proxy", attrsPLD);
    }


    {
    // Select all LegacyVTKFileReader elements 
    const char* attrs[] = {
      "type", "legacyreader", 0};
    this->Select( root, "Proxy", attrs,
      &::ConvertLegacyReader, this);
    }

    {
    // Convert all legacyreader proxies to LegacyVTKFileReader 
    const char* attrs[] = {
      "type", "legacyreader", 0};
    const char* newAttrs[] = {
      "type", "LegacyVTKFileReader", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert all PVAnimationScene proxies to AnimationScene proxies.
    const char* attrs[] = {
      "type", "PVAnimationScene", 0};
    this->Select(
      root, "Proxy", attrs, 
      &::ConvertPVAnimationSceneToAnimationScene, this);
    }


  return true;
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::ConvertViewModulesToViews(
  vtkPVXMLElement* parent)
{
  const char* attrs[] = {
    "name", "Displays", 0 };
  const char* newAttrs[] = {
    "name", "Representations", 0};
  // Replace the "Displays" property with "Representations".
  this->SelectAndSetAttributes(
    parent,
    "Property", attrs, newAttrs);

  // Convert property RenderWindowSize to ViewSize.
  const char* propAttrs[] = {
    "name", "RenderWindowSize", 0};
  const char* propNewAttrs[] = {
    "name", "ViewSize", 0};
  this->SelectAndSetAttributes(
    parent,
    "Property", propAttrs, propNewAttrs);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::ConvertLegacyReader(
  vtkPVXMLElement* parent)
{
  const char* attrs[] = {
    "name", "FileName", 0 };
  const char* newAttrs[] = {
    "name", "FileNames", 0};
  // Replace the "Displays" property with "Representations".
  this->SelectAndSetAttributes(
    parent,
   "Property", attrs, newAttrs);

  return true;
}


//----------------------------------------------------------------------------
bool vtkSMStateVersionController::ConvertPVAnimationSceneToAnimationScene(
  vtkPVXMLElement* parent)
{
  parent->SetAttribute("type", "AnimationScene");

  // ClockTimeRange property changed to "StartTime" and "EndTime"
  vtksys_ios::ostringstream idStr;
  idStr << parent->GetAttribute("id") << ".ClockTimeRange";
  vtkPVXMLElement* ctRange = parent->FindNestedElement(idStr.str().c_str());
  vtkSmartPointer<vtkCollection> elements = vtkSmartPointer<vtkCollection>::New();
  if (ctRange)
    {
    ctRange->GetElementsByName("Element", elements);
    }
  if (elements->GetNumberOfItems() == 2)
    {
    vtkPVXMLElement* startTime = vtkPVXMLElement::New();
    startTime->SetName("Property");
    startTime->SetAttribute("name", "StartTime");
    startTime->SetAttribute("number_of_elements", "1");
    vtksys_ios::ostringstream idST;
    idST << parent->GetAttribute("id") << ".StartTime";
    startTime->SetAttribute("id", idST.str().c_str());
    
    vtkPVXMLElement* element = 
      vtkPVXMLElement::SafeDownCast(elements->GetItemAsObject(0));
    ctRange->RemoveNestedElement(element);
    startTime->AddNestedElement(element);
    
    parent->AddNestedElement(startTime);
    startTime->Delete();

    vtkPVXMLElement* endTime = vtkPVXMLElement::New();
    endTime->SetName("Property");
    endTime->SetAttribute("name", "EndTime");
    endTime->SetAttribute("number_of_elements", "1");
    vtksys_ios::ostringstream idET;
    idET << parent->GetAttribute("id") << ".EndTime";
    endTime->SetAttribute("id", idET.str().c_str());
    
    element = vtkPVXMLElement::SafeDownCast(elements->GetItemAsObject(1));
    ctRange->RemoveNestedElement(element);
    element->SetAttribute("index", "0");
    endTime->AddNestedElement(element);
    
    parent->AddNestedElement(endTime);
    endTime->Delete();

    parent->RemoveNestedElement(ctRange);
    }

  return true;
}

//----------------------------------------------------------------------------
void vtkSMStateVersionController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


