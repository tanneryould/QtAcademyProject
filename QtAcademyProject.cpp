// Copyright 2023 ESRI
//
// All rights reserved under the copyright laws of the United States
// and applicable international laws, treaties, and conventions.
//
// You may freely redistribute and use this sample code, with or
// without modification, provided you include the original copyright
// notice and use restrictions.
//
// See the Sample code usage restrictions document for further information.
//

#include "QtAcademyProject.h"

#include "Map.h"
#include "MapTypes.h"
#include "MapQuickView.h"

using namespace Esri::ArcGISRuntime;

QtAcademyProject::QtAcademyProject(QObject* parent /* = nullptr */):
  QObject(parent),
  m_map(new Map(BasemapStyle::OsmStandard, this))
{
}

QtAcademyProject::~QtAcademyProject()
{
}

MapQuickView* QtAcademyProject::mapView() const
{
  return m_mapView;
}

// Set the view (created in QML)
void QtAcademyProject::setMapView(MapQuickView* mapView)
{
  if (!mapView || mapView == m_mapView)
  {
    return;
  }

  m_mapView = mapView;
  m_mapView->setMap(m_map);

  emit mapViewChanged();
}
