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

#include "ArcGISVectorTiledLayer.h"
#include "Basemap.h"
#include "Error.h"
#include "ExportTileCacheTask.h"
#include "ExportVectorTilesJob.h"
#include "ExportVectorTilesParameters.h"
#include "ExportVectorTilesTask.h"
#include "GeometryEngine.h"
#include "Graphic.h"
#include "GraphicListModel.h"
#include "GraphicsOverlay.h"
#include "GraphicsOverlayListModel.h"
#include "ItemResourceCache.h"
#include "LayerListModel.h"
#include "Location.h"
#include "LocationDisplay.h"
#include "Map.h"
#include "MapQuickView.h"
#include "MapTypes.h"
#include "MapViewTypes.h"
#include "OfflineMapTask.h"
#include "Point.h"
#include "Polygon.h"
#include "PolylineBuilder.h"
#include "SimpleLineSymbol.h"
#include "SpatialReference.h"
#include "SymbolTypes.h"
#include "VectorTileCache.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFuture>
#include <QNetworkInformation>
#include <QStandardPaths>


using namespace Esri::ArcGISRuntime;

const QString vtpkPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

QtAcademyProject::QtAcademyProject(QObject* parent /* = nullptr */):
  QObject(parent),
  m_map(new Map(this)),
  m_basemap(new Basemap(BasemapStyle::OsmStandard, this))
{
  m_map->setBasemap(m_basemap);
  m_tempObject.reset(new QObject(this));
}

QtAcademyProject::~QtAcademyProject() = default;

void QtAcademyProject::toggleOffline(bool offline)
{
  if (offline)
  {
    // Set the basemap to a null basemap
    // Use a unique pointer so it will get cleaned up when there are no references to it
    m_map->setBasemap(new Basemap(m_tempObject.get()));
    // Load the offline base layers from memory
    loadOfflineBasemaps();
  }
  else
  {
    // Set the map to the online basemap
    m_map->setBasemap(m_basemap);

    // Delete any offline objects by reseting the parent
    m_tempObject = std::make_unique<QObject>();
  }
}

void QtAcademyProject::loadOfflineBasemaps()
{
  // Check to see if any offline data exists
  if (QFile::exists(vtpkPath+"/vectorTiles.vtpk"))
  {
    // Create a VectorTileCache object from the offline data
    m_vectorTileCache = new VectorTileCache(vtpkPath + "/vectorTiles.vtpk", m_tempObject.get());

    // Check to see if any additional item resources exist and use them in the constructor if so
    if (QFile::exists(vtpkPath+"/itemResources"))
    {
      m_itemResourceCache = new ItemResourceCache(vtpkPath+"/itemResources", m_tempObject.get());
      m_offlineLayer = new ArcGISVectorTiledLayer(m_vectorTileCache, m_itemResourceCache, m_tempObject.get());
    }
    else
    {
      m_offlineLayer = new ArcGISVectorTiledLayer(m_vectorTileCache, m_tempObject.get());
    }

    // Add the created ArcGISVectorTileLayer to the base layers
    m_map->basemap()->baseLayers()->append(m_offlineLayer);
  }
}

void QtAcademyProject::createOfflineAreaFromExtent()
{
  // Export all available ArcGISVectorTileLayers
  for (Layer* layer : *(m_basemap->baseLayers()))
  {
    if (ArcGISVectorTiledLayer* vectorTileLayer = dynamic_cast<ArcGISVectorTiledLayer*>(layer))
      exportVectorTiles(vectorTileLayer);
  }
}

void QtAcademyProject::exportVectorTiles(ArcGISVectorTiledLayer* vectorTileLayer)
{
  // Create a new export task from the layer's source url
  m_exportVectorTilesTask = new ExportVectorTilesTask(vectorTileLayer->url(), m_tempObject.get());

  // Create default parameters for the layer service
  // Normalize the central meridian in case the download area crosses the meridian
  m_exportVectorTilesTask->createDefaultExportVectorTilesParametersAsync(GeometryEngine::normalizeCentralMeridian(m_mapView->visibleArea()), m_mapView->mapScale()*0.1)
      .then(this, [this](const ExportVectorTilesParameters& defaultParams)
  {
    QDir path(vtpkPath);

    // Remove any existing offline files
    path.removeRecursively();

    // Create the directory if it does not already exist
    if (!QDir().exists(vtpkPath))
      QDir().mkdir(vtpkPath);

    const QString vtpkFileName = vtpkPath+"/vectorTiles.vtpk";
    const QString itemResourcesPath = vtpkPath+"/itemResources";

    // Create a Job object to manage the export
    m_exportVectorTilesJob = m_exportVectorTilesTask->exportVectorTiles(defaultParams, vtpkFileName, itemResourcesPath);

    // Monitor the download progress and update the UI every time it changes
    connect(m_exportVectorTilesJob, &Job::progressChanged, this, [this]()
    {
      m_downloadProgress = m_exportVectorTilesJob->progress();
      emit downloadProgressChanged();
    });

    // Display any errors in the console log
    connect(m_exportVectorTilesJob, &Job::errorOccurred, this, [](const Error& e){qWarning() << e.message() << e.additionalMessage();});

    // Once all async slots have been created, start the export
    m_exportVectorTilesJob->start();
  });
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

  // Start the location display and center the map on the user
  m_mapView->locationDisplay()->start();
  m_mapView->locationDisplay()->setAutoPanMode(LocationDisplayAutoPanMode::Recenter);

  // Allow the user to rotate the map by pinching a touchscreen
  m_mapView->setRotationByPinchingEnabled(true);

  // Create and add a GraphicsOverlay to display Graphics on the MapView
  GraphicsOverlay* graphicsOverlay = new GraphicsOverlay(this);
  m_mapView->graphicsOverlays()->append(graphicsOverlay);

  // Create and add a Graphic to show the user's path
  m_offlineMapExtentGraphic = new Graphic(this);
  m_offlineMapExtentGraphic->setSymbol(new SimpleLineSymbol(SimpleLineSymbolStyle::Solid, Qt::blue, 2, this));
  graphicsOverlay->graphics()->append(m_offlineMapExtentGraphic);

  // Create a PolylineBuilder to construct lines given a set of points
  m_lineBuilder = new PolylineBuilder(SpatialReference::wgs84(), this);

  // Whenever the user's location changes, update the line path if necessary and recenter the map
  connect(m_mapView->locationDisplay(), &LocationDisplay::locationChanged, this, [this](const Location& l)
  {
    if (!m_isTracking)
      return;

    m_mapView->locationDisplay()->setAutoPanMode(LocationDisplayAutoPanMode::CompassNavigation);

    m_lineBuilder->addPoint(l.position());
    m_offlineMapExtentGraphic->setGeometry(m_lineBuilder->toGeometry());
  });

  emit mapViewChanged();
}

int QtAcademyProject::downloadProgress()
{
  return m_downloadProgress;
}

bool QtAcademyProject::isTracking() const
{
  return m_isTracking;
}

void QtAcademyProject::setIsTracking(bool value)
{
  if (m_isTracking == value)
    return;

  m_isTracking = value;

  if (m_isTracking)
  {
    // Set the map auto pan mode to compass
    m_mapView->locationDisplay()->setAutoPanMode(LocationDisplayAutoPanMode::CompassNavigation);
    // Clear any previous track
    m_offlineMapExtentGraphic->setGeometry({});
  }

  emit isTrackingChanged();
}

