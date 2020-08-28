#ifndef CondFormats_DataRecord_HeterogeneousHGCalHEFCellPositionsConditionsRecord_h
#define CondFormats_DataRecord_HeterogeneousHGCalHEFCellPositionsConditionsRecord_h

#include "FWCore/Framework/interface/EventSetupRecordImplementation.h"
#include "FWCore/Framework/interface/DependentRecordImplementation.h"
#include "Geometry/HGCalGeometry/interface/HGCalGeometry.h"
//#include "FWCore/Utilities/interface/mplVector.h"

class HeterogeneousHGCalHEFCellPositionsConditionsRecord
    : public edm::eventsetup::DependentRecordImplementation<HeterogeneousHGCalHEFCellPositionsConditionsRecord, edm::mpl::Vector<IdealGeometryRecord>> {};

#endif //CondFormats_DataRecord_HeterogeneousHGCalHEFCellPositionsConditionsRecord_h
