#ifndef HeterogeneousHGCalHEFRecHitProducer_h
#define HeterogeneousHGCalHEFRecHitProducer_h

#include <iostream>
#include <string>
#include <memory>
#include <chrono>
#include <cuda_runtime.h>

#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "HeterogeneousCore/CUDAUtilities/interface/cudaCheck.h"
#include "DataFormats/HGCRecHit/interface/HGCRecHit.h"
#include "DataFormats/HGCRecHit/interface/HGCRecHitCollections.h"
#include "DataFormats/ForwardDetId/interface/HGCSiliconDetId.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/Utilities/interface/EDPutToken.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "RecoLocalCalo/HGCalRecAlgos/interface/RecHitTools.h"
#include "Geometry/HGCalGeometry/interface/HGCalGeometry.h"
#include "Geometry/HGCalCommonData/interface/HGCalDDDConstants.h"

#include "HeterogeneousCore/CUDACore/interface/ScopedContext.h"
#include "HeterogeneousCore/CUDACore/interface/ContextState.h"
#include "HeterogeneousCore/CUDAServices/interface/CUDAService.h"
#include "HeterogeneousCore/CUDAUtilities/interface/cudaCheck.h"

#include "HeterogeneousHGCalProducerMemoryWrapper.h"
#include "KernelManagerHGCalRecHit.h"

class HeterogeneousHGCalHEFRecHitProducer: public edm::stream::EDProducer<edm::ExternalWork> 
{
 public:
  explicit HeterogeneousHGCalHEFRecHitProducer(const edm::ParameterSet& ps);
  ~HeterogeneousHGCalHEFRecHitProducer() override;

  virtual void acquire(edm::Event const&, edm::EventSetup const&, edm::WaitingTaskWithArenaHolder) override;
  virtual void produce(edm::Event&, const edm::EventSetup&) override;

 private:
  unsigned int nhitsmax_ = 0;
  unsigned int stride_ = 0;
  edm::EDGetTokenT<HGChefUncalibratedRecHitCollection> token_;
  const std::string collection_name_ = "HeterogeneousHGChefUncalibratedRecHits";
  edm::Handle<HGChefUncalibratedRecHitCollection> handle_hef_; 
  size_t handle_size_;
  std::unique_ptr< HGChefRecHitCollection > rechits_;
  cms::cuda::ContextState ctxState_;

  //constants
  HGChefUncalibratedRecHitConstantData cdata_;
  HGCConstantVectorData vdata_;

  //memory
  void allocate_memory_();
  cms::cuda::host::noncached::unique_ptr<std::byte[]> h_mem_const_;
  cms::cuda::device::unique_ptr<std::byte[]> d_mem_const_;
  cms::cuda::host::noncached::unique_ptr<std::byte[]> h_mem_in_;
  cms::cuda::device::unique_ptr<std::byte[]> d_mem_;
  cms::cuda::host::unique_ptr<std::byte[]> h_mem_out_;

  //geometry
  void set_geometry_(const edm::EventSetup&);
  std::unique_ptr<hgcal::RecHitTools> tools_;
  const HGCalDDDConstants* ddd_ = nullptr;

  //data processing
  void convert_collection_data_to_soa_(const edm::SortedCollection<HGCUncalibratedRecHit>&, HGCUncalibratedRecHitSoA*, const unsigned int&);
  void convert_soa_data_to_collection_(HGCRecHitCollection&, HGCRecHitSoA*, const unsigned int&);
  void convert_constant_data_(KernelConstantData<HGChefUncalibratedRecHitConstantData>*);

  HGCUncalibratedRecHitSoA *old_soa_ = nullptr, *d_oldhits_ = nullptr, *d_newhits_ = nullptr;
  HGCRecHitSoA *d_newhits_final_ = nullptr, *h_newhits_ = nullptr;
  KernelModifiableData<HGCUncalibratedRecHitSoA, HGCRecHitSoA> *kmdata_;
  KernelConstantData<HGChefUncalibratedRecHitConstantData> *h_kcdata_;
  KernelConstantData<HGChefUncalibratedRecHitConstantData> *d_kcdata_;
};

#endif //HeterogeneousHGCalHEFRecHitProducer_h
